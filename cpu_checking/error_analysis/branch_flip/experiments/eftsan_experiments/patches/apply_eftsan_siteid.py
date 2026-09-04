#!/usr/bin/env python3
"""
apply_eftsan_siteid.py -- install exact (module_id, site_id, k) event
identification into EFTSanitizer, replacing the earlier stderr fileId print.

Run from anywhere; point --root at the EFTSanitizer checkout.

  ./apply_eftsan_siteid.py --root /usr/workspace/das9/EFTSanitizer --check
  ./apply_eftsan_siteid.py --root /usr/workspace/das9/EFTSanitizer --apply
  ./apply_eftsan_siteid.py --root /usr/workspace/das9/EFTSanitizer --revert

What --apply does:
  writes  llvm_pass/EFTSan/EFTSan_SiteId.h        (new)
  writes  runtime/eftsan_bf_counter.h             (new)
  edits   llvm_pass/EFTSan/EFTSanitizer.h         include + Sites member
  edits   llvm_pass/EFTSan/EFTSanitizer.cpp       prepass in runOnModule,
                                                  handleFcmp hook args
  edits   runtime/handleReal.cpp                  include, six branch hooks,
                                                  eftsan_finish dump

Backups go to *.bak-siteid. The earlier *.bak-fileid backups are never touched.
--apply is idempotent: it refuses to run twice rather than double-patching.

The one thing this script cannot do for you: collectControllingFCmps() in
EFTSan_SiteId.h is a reimplementation of brtrace's isFPControlled(). Replace
its body with brtrace's predicate verbatim before trusting any site count.
"""

import argparse
import os
import re
import shutil
import sys

MARK = "EFTSAN_SITEID_PATCH"

# ===========================================================================
# new file 1: llvm_pass/EFTSan/EFTSan_SiteId.h
# ===========================================================================

SITEID_H = r'''//===- EFTSan_SiteId.h --------------------------------------------------===//
//
// Cross-tool site identification for EFTSanitizer, matching brtrace/FPChecker.
//
//   module_id   FNV-1a-32 over basename(DIFile)        hash
//   site_id     ordinal, position in the module walk   ordinal (static)
//   k           position in a site's run sequence      ordinal (dynamic)
//
// DIFFERENCE FROM FPCHECKER: EFTSan runs `opt -eftsan` on one llvm-linked
// module, so getModuleIdentifier() names the merged .ll, not a TU. The module
// hash is therefore taken PER INSTRUCTION from the DILocation's file. site_id
// ordinals are unique within the EFTSan run but do NOT match brtrace's per-TU
// ordinals -- the cross-tool join key is (module_id, line, col). site_id is
// only an array index for the runtime counter table.
//
//===--------------------------------------------------------------------===//

#ifndef EFTSAN_SITEID_H
#define EFTSAN_SITEID_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>
#include <cstdint>
#include <system_error>

namespace eftsan_site {

enum SiteKind : uint8_t {
  SK_NONE   = 0,   // fcmp controls neither a branch nor a select
  SK_BRANCH = 1,
  SK_SELECT = 2
};

struct SiteInfo {
  int32_t  id      = -1;      // ordinal within its kind's space, -1 = out of scope
  uint32_t mod     = 0;       // FNV-1a-32 of basename(DIFile)
  uint32_t line    = 0;
  uint32_t col     = 0;
  SiteKind kind    = SK_NONE;
  std::string file;
  std::string func;
  unsigned nFcmp   = 1;
};

// The only cross-tool hash. Constants and the basename-not-fullpath choice
// are FIXED BY CONVENTION -- byte-identical to brtrace's moduleHash.
inline uint32_t moduleHash(llvm::StringRef S) {
  uint32_t H = 2166136261u;
  for (unsigned char C : S.bytes()) { H ^= C; H *= 16777619u; }
  return H;
}

inline std::string fileBasename(const llvm::DILocation *DL) {
  if (!DL) return std::string();
  return llvm::sys::path::filename(DL->getFilename()).str();
}

// !!! PORT NOTE !!!
// Must accept EXACTLY the instruction shapes brtrace's isFPControlled() does.
// Copy brtrace's predicate verbatim rather than trusting this version.
inline void collectControllingFCmps(llvm::Value *Cond,
                                    llvm::SmallVectorImpl<llvm::FCmpInst *> &Out,
                                    llvm::DenseSet<llvm::Value *> &Seen,
                                    unsigned Depth = 0) {
  using namespace llvm;
  if (!Cond || Depth > 16) return;
  if (!Seen.insert(Cond).second) return;

  if (auto *FC = dyn_cast<FCmpInst>(Cond)) { Out.push_back(FC); return; }

  // -O0 spills i1 through alloca; follow load -> stores into that alloca.
  if (auto *LI = dyn_cast<LoadInst>(Cond)) {
    if (auto *AI = dyn_cast<AllocaInst>(LI->getPointerOperand()))
      for (User *U : AI->users())
        if (auto *SI = dyn_cast<StoreInst>(U))
          collectControllingFCmps(SI->getValueOperand(), Out, Seen, Depth + 1);
    return;
  }

  if (auto *I = dyn_cast<Instruction>(Cond)) {
    switch (I->getOpcode()) {
    case Instruction::ZExt:
    case Instruction::SExt:
    case Instruction::Trunc:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
    case Instruction::Select:
    case Instruction::PHI:
      for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i)
        collectControllingFCmps(I->getOperand(i), Out, Seen, Depth + 1);
      return;
    default:
      return;
    }
  }
}

class SiteTable {
public:
  llvm::DenseMap<llvm::FCmpInst *, SiteInfo> Map;
  std::vector<SiteInfo> Branches;
  std::vector<SiteInfo> Selects;

  // Runs at the top of runOnModule(), before any instrumentation, so the walk
  // sees pristine IR. Two disjoint ordinal spaces, assigned in module order.
  void build(llvm::Module &M) {
    using namespace llvm;
    int32_t nextBr = 0, nextSel = 0;
    Map.clear(); Branches.clear(); Selects.clear();

    for (Function &F : M) {
      if (F.isDeclaration()) continue;

      for (BasicBlock &BB : F)
        for (Instruction &I : BB) {
          auto *SI = dyn_cast<SelectInst>(&I);
          if (!SI) continue;
          SmallVector<FCmpInst *, 4> Ctl;
          DenseSet<Value *> Seen;
          collectControllingFCmps(SI->getCondition(), Ctl, Seen);
          if (Ctl.empty()) continue;
          record(Ctl, SK_SELECT, nextSel++, &F, SI, Selects);
        }

      for (BasicBlock &BB : F) {
        auto *BI = dyn_cast<BranchInst>(BB.getTerminator());
        if (!BI || !BI->isConditional()) continue;
        SmallVector<FCmpInst *, 4> Ctl;
        DenseSet<Value *> Seen;
        collectControllingFCmps(BI->getCondition(), Ctl, Seen);
        if (Ctl.empty()) continue;
        record(Ctl, SK_BRANCH, nextBr++, &F, BI, Branches);
      }
    }
  }

  // Unmapped fcmps are out of scope: id = -1, which the runtime never counts.
  SiteInfo lookup(llvm::FCmpInst *FC) const {
    auto It = Map.find(FC);
    return It == Map.end() ? SiteInfo() : It->second;
  }

  void writeManifest(const std::string &Path) const {
    std::error_code EC;
    llvm::raw_fd_ostream OS(Path, EC, llvm::sys::fs::F_Text);
    if (EC) {
      llvm::errs() << "EFTSan_SiteId: cannot open manifest " << Path
                   << ": " << EC.message() << "\n";
      return;
    }
    OS << "# kind,site_id,module_id,file,line,col,function,n_fcmp\n";
    for (const auto &S : Branches) emitRow(OS, "branch", S);
    for (const auto &S : Selects)  emitRow(OS, "select", S);
    llvm::errs() << "EFTSan_SiteId: " << Branches.size() << " branch sites, "
                 << Selects.size() << " select sites -> " << Path << "\n";
  }

private:
  static void emitRow(llvm::raw_fd_ostream &OS, const char *K,
                      const SiteInfo &S) {
    OS << K << ',' << S.id << ',' << S.mod << ',' << S.file << ','
       << S.line << ',' << S.col << ',' << S.func << ',' << S.nFcmp << '\n';
  }

  void record(llvm::SmallVectorImpl<llvm::FCmpInst *> &Ctl, SiteKind K,
              int32_t Id, llvm::Function *F, llvm::Instruction *Anchor,
              std::vector<SiteInfo> &Space) {
    using namespace llvm;

    // Location from the fcmp, not the terminator: at -O0 the br often carries
    // the enclosing statement's line while the fcmp carries the comparison's
    // own line/col, which is what brtrace records.
    const DILocation *DL = Ctl[0]->getDebugLoc();
    if (!DL) DL = Anchor->getDebugLoc();

    SiteInfo S;
    S.id    = Id;
    S.kind  = K;
    S.file  = fileBasename(DL);
    S.mod   = DL ? moduleHash(S.file) : 0u;
    S.line  = DL ? DL->getLine() : 0u;
    S.col   = DL ? DL->getColumn() : 0u;
    S.func  = F->getName().str();
    S.nFcmp = Ctl.size();

    // Multiplicity is recorded, not papered over. One br fed by two fcmps
    // makes EFTSan tick twice per branch execution while brtrace ticks once;
    // the drift gate excludes n_fcmp > 1 sites from event-level scoring.
    for (auto *FC : Ctl) Map[FC] = S;
    Space.push_back(S);
  }
};

} // namespace eftsan_site

#endif // EFTSAN_SITEID_H
'''

# ===========================================================================
# new file 2: runtime/eftsan_bf_counter.h
# ===========================================================================

COUNTER_H = r'''//===- eftsan_bf_counter.h ----------------------------------------------===//
//
// Per-site occurrence counter and branch-flip event emission for EFTSanitizer.
//
// THE ONE INVARIANT: the counter ticks on EVERY execution of an in-scope
// comparison, whether or not a flip is reported. If it ticked only on reports,
// k would count reports while brtrace's E_S counts executions, and the two
// would drift apart at the first silent execution.
//
// Branch and select are disjoint id spaces, so they get separate tables.
//
// Output: EFTSAN_BF_OUT (default eftsan_events.csv), one row per in-scope
// execution that flipped or whose shadow was non-finite. Executions that were
// both stable and finite emit nothing; TN is recoverable as E - F - FP from
// the brtrace census.
//
//===--------------------------------------------------------------------===//

#ifndef EFTSAN_BF_COUNTER_H
#define EFTSAN_BF_COUNTER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define EFTSAN_BF_TABLE_BITS 16
#define EFTSAN_BF_TABLE_SIZE (1u << EFTSAN_BF_TABLE_BITS)

// EFTSan's OWN LOGIC IS NOT TOUCHED. m_check_branch already handles
// non-finite operands -- every ordered predicate in its switch requires
// !isnan on both sides and falls through to false otherwise -- so a
// non-finite shadow still produces whatever verdict upstream produces, and
// flipsCount still increments exactly when upstream increments it.
//
// The non-finite flag is an ANNOTATION on the row, not a gate on the code
// path. It records that a verdict rested on a non-finite shadow, which is
// worth reporting and worth excluding at SCORING time if you choose to;
// deciding that inside the tool would change the tool's answer.
//
//   FLIP            shadow and computed disagreed
//   FLIP_NONFINITE  same, but a shadow-corrected operand was inf/NaN
//   NONFINITE       no disagreement, but the shadow was non-finite
#define EFTSAN_V_FLIP           1
#define EFTSAN_V_FLIP_NONFINITE 2
#define EFTSAN_V_NONFINITE      3

#define EFTSAN_K_NONE   0
#define EFTSAN_K_BRANCH 1
#define EFTSAN_K_SELECT 2
#define EFTSAN_NKINDS   3

typedef struct {
  uint64_t key;    // (mod << 32) | (uint32_t)site
  uint64_t count;  // executions of this site so far
  uint64_t flips;      // FLIP + FLIP_NONFINITE == upstream's flipsCount
  uint64_t nonfinite;  // FLIP_NONFINITE + NONFINITE
  uint8_t  used;       // separate flag: mod = 0, site = 0 is a legal key
} eftsan_bf_slot;

static eftsan_bf_slot _eftsan_bf_tab[EFTSAN_NKINDS][EFTSAN_BF_TABLE_SIZE];
static FILE *_eftsan_bf_out  = NULL;
static int   _eftsan_bf_init = 0;

// splitmix64 -- bucket index only, never leaves the process. splitmix rather
// than a mask on the low bits because site ids are small consecutive integers.
static inline uint64_t _eftsan_bf_mix(uint64_t k) {
  k ^= k >> 33; k *= 0xff51afd7ed558ccdULL;
  k ^= k >> 33; k *= 0xc4ceb9fe1a85ec53ULL;
  k ^= k >> 33;
  return k;
}

static inline uint64_t _eftsan_bf_key(uint32_t mod, int32_t site) {
  return ((uint64_t)mod << 32) | (uint64_t)(uint32_t)site;
}

static void _eftsan_bf_setup(void) {
  if (_eftsan_bf_init) return;
  _eftsan_bf_init = 1;
  const char *p = getenv("EFTSAN_BF_OUT");
  _eftsan_bf_out = fopen(p ? p : "eftsan_events.csv", "w");
  if (_eftsan_bf_out)
    fprintf(_eftsan_bf_out, "kind,module_id,site_id,k,verdict\n");
}

static inline eftsan_bf_slot *_eftsan_bf_find(uint8_t kind, uint64_t key) {
  eftsan_bf_slot *T = _eftsan_bf_tab[kind];
  uint64_t i = _eftsan_bf_mix(key) & (EFTSAN_BF_TABLE_SIZE - 1);
  for (uint32_t probe = 0; probe < EFTSAN_BF_TABLE_SIZE; ++probe) {
    eftsan_bf_slot *s = &T[i];
    if (s->used && s->key == key) return s;
    if (!s->used) {
      s->used = 1; s->key = key;
      s->count = 0; s->flips = 0; s->nonfinite = 0;
      return s;
    }
    i = (i + 1) & (EFTSAN_BF_TABLE_SIZE - 1);
  }
  return NULL; // table full; drop the event rather than corrupt k
}

// Call at the TOP of every eftsan_check_branch_* hook, before the shadow
// comparison, before any non-finite early return, before the flip test.
static inline uint64_t eftsan_bf_tick(uint32_t mod, int32_t site, uint8_t kind) {
  if (site < 0 || kind == EFTSAN_K_NONE || kind >= EFTSAN_NKINDS)
    return UINT64_MAX;
  if (!_eftsan_bf_init) _eftsan_bf_setup();
  eftsan_bf_slot *s = _eftsan_bf_find(kind, _eftsan_bf_key(mod, site));
  return s ? s->count++ : UINT64_MAX;
}

static inline void eftsan_bf_emit(uint32_t mod, int32_t site, uint8_t kind,
                                  uint64_t k, int verdict) {
  if (k == UINT64_MAX || !_eftsan_bf_out) return;
  eftsan_bf_slot *s = _eftsan_bf_find(kind, _eftsan_bf_key(mod, site));
  if (s) {
    if (verdict != EFTSAN_V_NONFINITE) s->flips++;
    if (verdict != EFTSAN_V_FLIP)      s->nonfinite++;
  }
  fprintf(_eftsan_bf_out, "%s,%u,%d,%llu,%s\n",
          kind == EFTSAN_K_SELECT ? "select" : "branch",
          mod, site, (unsigned long long)k,
          verdict == EFTSAN_V_FLIP ? "FLIP" :
          verdict == EFTSAN_V_FLIP_NONFINITE ? "FLIP_NONFINITE" : "NONFINITE");
}

// Call from eftsan_finish(). `executions` is T_S, the full-run per-site count
// the path-equivalence check compares against brtrace elementwise.
static inline void eftsan_bf_dump_totals(void) {
  const char *p = getenv("EFTSAN_BF_TOTALS");
  FILE *f = fopen(p ? p : "eftsan_totals.csv", "w");
  if (!f) return;
  fprintf(f, "kind,module_id,site_id,executions,flips,nonfinite\n");
  for (uint8_t kind = 1; kind < EFTSAN_NKINDS; ++kind)
    for (uint32_t i = 0; i < EFTSAN_BF_TABLE_SIZE; ++i) {
      eftsan_bf_slot *s = &_eftsan_bf_tab[kind][i];
      if (!s->used) continue;
      fprintf(f, "%s,%u,%d,%llu,%llu,%llu\n",
              kind == EFTSAN_K_SELECT ? "select" : "branch",
              (uint32_t)(s->key >> 32),
              (int32_t)(uint32_t)(s->key & 0xFFFFFFFFULL),
              (unsigned long long)s->count,
              (unsigned long long)s->flips,
              (unsigned long long)s->nonfinite);
    }
  fclose(f);
  if (_eftsan_bf_out) { fclose(_eftsan_bf_out); _eftsan_bf_out = NULL; }
}

#endif // EFTSAN_BF_COUNTER_H
'''

# ===========================================================================
# runtime hook bodies -- generated from a spec so the six stay consistent
# ===========================================================================

HOOKS = [
    # name,   ty,       sig,                                              op1 expr,             op2 expr,            bf_map operand
    ("f1",  "float",  "float op1d,\n\t\t\t\t     float op2d, smem_entry* op2",
     "(double)op1d", "(double)op2d + op2->error", "op2"),
    ("f2",  "float",  "float op1d, smem_entry* op1,\n\t\t\t\t     float op2d",
     "(double)op1d + op1->error", "(double)op2d", "op1"),
    ("f",   "float",  "float op1d, smem_entry* op1,\n\t\t\t\t     float op2d, smem_entry* op2",
     "(double)op1d + op1->error", "(double)op2d + op2->error", "op1"),
    ("d1",  "double", "double op1d,\n\t\t\t\t     double op2d, smem_entry* op2",
     "op1d", "op2d + op2->error", "op2"),
    ("d2",  "double", "double op1d, smem_entry* op1,\n\t\t\t\t     double op2d",
     "op1d + op1->error", "op2d", "op1"),
    ("d",   "double", "double op1d, smem_entry* op1,\n\t\t\t\t     double op2d, smem_entry* op2",
     "op1d + op1->error", "op2d + op2->error", "op1"),
]

HOOK_TMPL = '''extern "C" bool eftsan_check_branch_{name}({sig},
\t\t\t\t     size_t fcmpFlag, bool computedRes,
\t\t\t\t     uint32_t lineNo, uint32_t modId,
\t\t\t\t     int32_t siteId, uint8_t kindId){{

  uint64_t k = eftsan_bf_tick(modId, siteId, kindId);

  double op1Total = {op1};
  double op2Total = {op2};

  /* Annotation only: no early return, no change to what m_check_branch is
     asked or to when flipsCount increments. This hook must report exactly
     what unpatched EFTSan reports. */
  int nonFinite = (!std::isfinite(op1Total) || !std::isfinite(op2Total));

  bool realRes = m_check_branch(op1Total, op2Total, fcmpFlag);
  if(realRes != computedRes){{
    flipsCount++;
#if STATIC
    m_bf_map[{bf}->lineno]++;
#endif
    eftsan_bf_emit(modId, siteId, kindId, k,
                   nonFinite ? EFTSAN_V_FLIP_NONFINITE : EFTSAN_V_FLIP);
    if(getenv("EFTSAN_BF_VERBOSE"))
      fprintf(stderr, "branch flip @ mod %u site %d k %llu line %u%s\\n",
              modId, siteId, (unsigned long long)k, lineNo,
              nonFinite ? " [non-finite shadow]" : "");
  }}
  else if(nonFinite){{
    eftsan_bf_emit(modId, siteId, kindId, k, EFTSAN_V_NONFINITE);
  }}
  return realRes;
}}'''


def hook_text(name, ty, sig, op1, op2, bf):
    return HOOK_TMPL.format(name=name, sig=sig, op1=op1, op2=op2, bf=bf)


# ===========================================================================
# pass-side handleFcmp replacement blocks
# ===========================================================================

OLD_HASH_BLOCK = """  uint64_t fileHash = 0;
  if (instDebugLoc) {
    StringRef fn = instDebugLoc->getScope()->getFilename();
    size_t slash = fn.rfind('/');
    StringRef base = (slash == StringRef::npos) ? fn : fn.substr(slash + 1);
    fileHash = 14695981039346656037ULL;
    for (char c : base) { fileHash ^= (uint64_t)(unsigned char)c; fileHash *= 1099511628211ULL; }
  }
  Constant* fileId = ConstantInt::get(Int64Ty, fileHash);
"""

NEW_HASH_BLOCK = """  // """ + MARK + """ : 32-bit FNV-1a over basename(DIFile), matching brtrace.
  // The old 64-bit fileId used different constants and could not be joined.
  IntegerType* Int8Ty = Type::getInt8Ty(M->getContext());
  uint32_t modHash = 0;
  if (instDebugLoc) {
    StringRef fn = instDebugLoc->getScope()->getFilename();
    size_t slash = fn.rfind('/');
    StringRef base = (slash == StringRef::npos) ? fn : fn.substr(slash + 1);
    modHash = 2166136261u;
    for (char c : base) { modHash ^= (unsigned char)c; modHash *= 16777619u; }
  }
  eftsan_site::SiteInfo bfSite = Sites.lookup(FCI);
  Constant* modId  = ConstantInt::get(Int32Ty, modHash);
  Constant* siteId = ConstantInt::get(Int32Ty, (uint32_t)bfSite.id);
  Constant* kindId = ConstantInt::get(Int8Ty,  (uint8_t)bfSite.kind);
"""

PREPASS_ANCHOR = """  // Find functions that perform floating point computation. No
  // instrumentation if the function does not perform any FP
  // computations."""

PREPASS_BLOCK = """  // """ + MARK + """ : enumerate FP-controlled branch and select sites on
  // pristine IR, before any instrumentation, and dump the manifest for the
  // drift gate. Must run before anything below touches the module.
  Sites.build(M);
  {
    const char *sp = getenv("EFTSAN_SITES");
    Sites.writeManifest(sp ? sp : "eftsan_sites.csv");
  }

"""

HDR_INCLUDE_ANCHOR = "#include <set>\n"
HDR_INCLUDE_BLOCK = '#include <set>\n#include "EFTSan_SiteId.h"   // ' + MARK + '\n'

HDR_MEMBER_ANCHOR = "    virtual bool runOnModule(Module &module);\n"
HDR_MEMBER_BLOCK = (
    "    eftsan_site::SiteTable Sites;   // " + MARK + "\n"
    "    virtual bool runOnModule(Module &module);\n"
)

RT_INCLUDE_ANCHOR = '#include "handleReal.h"\n'
RT_INCLUDE_BLOCK = ('#include "handleReal.h"\n'
                    '#include "eftsan_bf_counter.h"   // ' + MARK + '\n')

FINISH_ANCHOR = 'extern "C" void eftsan_finish() {\n'
FINISH_BLOCK = ('extern "C" void eftsan_finish() {\n'
                '  eftsan_bf_dump_totals();   // ' + MARK + '\n')


# ===========================================================================
# machinery
# ===========================================================================

class Fail(Exception):
    pass


def paths(root):
    return {
        "siteid_h":  os.path.join(root, "llvm_pass/EFTSan/EFTSan_SiteId.h"),
        "counter_h": os.path.join(root, "runtime/eftsan_bf_counter.h"),
        "pass_h":    os.path.join(root, "llvm_pass/EFTSan/EFTSanitizer.h"),
        "pass_cpp":  os.path.join(root, "llvm_pass/EFTSan/EFTSanitizer.cpp"),
        "rt_cpp":    os.path.join(root, "runtime/handleReal.cpp"),
    }


def read(p):
    with open(p) as f:
        return f.read()


def write(p, s):
    with open(p, "w") as f:
        f.write(s)


def sub_once(text, old, new, what):
    n = text.count(old)
    if n != 1:
        raise Fail("%s: expected exactly 1 occurrence, found %d" % (what, n))
    return text.replace(old, new)


def find_hook(text, name):
    """Return (start, end) of the whole eftsan_check_branch_<name> definition."""
    sig = 'extern "C" bool eftsan_check_branch_%s(' % name
    i = text.find(sig)
    if i < 0:
        raise Fail("hook eftsan_check_branch_%s not found" % name)
    if text.find(sig, i + 1) >= 0:
        raise Fail("hook eftsan_check_branch_%s appears more than once" % name)
    j = text.find("\n}\n", i)
    if j < 0:
        raise Fail("hook eftsan_check_branch_%s has no closing brace" % name)
    return i, j + 2


def check(root, verbose=True):
    P = paths(root)
    problems = []
    ok = []

    for key in ("pass_h", "pass_cpp", "rt_cpp"):
        if not os.path.isfile(P[key]):
            problems.append("missing %s" % P[key])
    if problems:
        return problems, ok

    ph, pc, rc = read(P["pass_h"]), read(P["pass_cpp"]), read(P["rt_cpp"])

    already = [k for k, t in (("EFTSanitizer.h", ph),
                              ("EFTSanitizer.cpp", pc),
                              ("handleReal.cpp", rc)) if MARK in t]
    if already:
        problems.append("already patched: %s (use --revert first)"
                        % ", ".join(already))
        return problems, ok

    # pass header anchors
    for label, txt, anchor, fn in (
            ("EFTSanitizer.h include", ph, HDR_INCLUDE_ANCHOR, "pass_h"),
            ("EFTSanitizer.h member",  ph, HDR_MEMBER_ANCHOR,  "pass_h"),
            ("EFTSanitizer.cpp prepass", pc, PREPASS_ANCHOR,   "pass_cpp"),
            ("handleReal.cpp include", rc, RT_INCLUDE_ANCHOR,  "rt_cpp"),
            ("handleReal.cpp finish",  rc, FINISH_ANCHOR,      "rt_cpp")):
        n = txt.count(anchor)
        if n != 1:
            problems.append("%s: anchor found %d times, expected 1" % (label, n))
        else:
            ok.append(label)

    # the old fileId block
    n = pc.count(OLD_HASH_BLOCK)
    if n == 1:
        ok.append("handleFcmp fileId block (will be replaced)")
    elif n == 0:
        problems.append(
            "handleFcmp: the 64-bit fileHash block was not found. Either the "
            "fileId patch was never applied to this tree, or it was edited. "
            "Compare against *.bak-fileid.")
    else:
        problems.append("handleFcmp: fileHash block found %d times" % n)

    # six signature/call pairs
    for name in [h[0] for h in HOOKS]:
        sigpat = '"eftsan_check_branch_%s", Int1Ty' % name
        if pc.count(sigpat) != 1:
            problems.append("handleFcmp: getOrInsertFunction for %s found %d times"
                            % (name, pc.count(sigpat)))
        else:
            ok.append("handleFcmp %s signature" % name)
        try:
            find_hook(rc, name)
            ok.append("runtime hook %s" % name)
        except Fail as e:
            problems.append(str(e))

    if pc.count("OpCode, I, lineNumber, fileId}") != 6:
        problems.append("handleFcmp: expected 6 call sites passing fileId, found %d"
                        % pc.count("OpCode, I, lineNumber, fileId}"))
    else:
        ok.append("handleFcmp 6 call sites")

    return problems, ok


def apply(root):
    P = paths(root)
    problems, _ = check(root)
    if problems:
        raise Fail("pre-flight failed:\n  " + "\n  ".join(problems))

    for key in ("pass_h", "pass_cpp", "rt_cpp"):
        bak = P[key] + ".bak-siteid"
        if not os.path.exists(bak):
            shutil.copy2(P[key], bak)
            print("backup  %s" % bak)

    write(P["siteid_h"], SITEID_H)
    print("write   %s" % P["siteid_h"])
    write(P["counter_h"], COUNTER_H)
    print("write   %s" % P["counter_h"])

    # ---- EFTSanitizer.h
    ph = read(P["pass_h"])
    ph = sub_once(ph, HDR_INCLUDE_ANCHOR, HDR_INCLUDE_BLOCK, "pass header include")
    ph = sub_once(ph, HDR_MEMBER_ANCHOR, HDR_MEMBER_BLOCK, "pass header member")
    write(P["pass_h"], ph)
    print("edit    %s  (include + Sites member)" % P["pass_h"])

    # ---- EFTSanitizer.cpp
    pc = read(P["pass_cpp"])
    pc = sub_once(pc, PREPASS_ANCHOR, PREPASS_BLOCK + PREPASS_ANCHOR,
                  "runOnModule prepass")
    pc = sub_once(pc, OLD_HASH_BLOCK, NEW_HASH_BLOCK, "handleFcmp hash block")

    for name, _, _, _, _, _ in HOOKS:
        # signature: trailing Int64Ty (fileId) -> Int32Ty, Int32Ty, Int8Ty
        pat = re.compile(
            r'("eftsan_check_branch_%s", Int1Ty[^;]*?)Int32Ty, Int64Ty\);' % name,
            re.S)
        pc, n = pat.subn(r'\1Int32Ty, Int32Ty, Int32Ty, Int8Ty);', pc)
        if n != 1:
            raise Fail("signature rewrite for %s matched %d times" % (name, n))

    pc, n = re.subn(r'OpCode, I, lineNumber, fileId\}',
                    'OpCode, I, lineNumber, modId, siteId, kindId}', pc)
    if n != 6:
        raise Fail("call-site rewrite matched %d times, expected 6" % n)
    write(P["pass_cpp"], pc)
    print("edit    %s  (prepass, hash block, 6 signatures, 6 calls)" % P["pass_cpp"])

    # ---- handleReal.cpp
    rc = read(P["rt_cpp"])
    rc = sub_once(rc, RT_INCLUDE_ANCHOR, RT_INCLUDE_BLOCK, "runtime include")
    for spec in HOOKS:
        name = spec[0]
        i, j = find_hook(rc, name)
        rc = rc[:i] + hook_text(*spec) + rc[j:]
    rc = sub_once(rc, FINISH_ANCHOR, FINISH_BLOCK, "eftsan_finish dump")
    write(P["rt_cpp"], rc)
    print("edit    %s  (include, 6 hooks, finish dump)" % P["rt_cpp"])

    print("""
applied.

NEXT, in order:

  1. Replace collectControllingFCmps() in
       llvm_pass/EFTSan/EFTSan_SiteId.h
     with brtrace's isFPControlled() verbatim. Until you do, the two tools
     may disagree about which branches are in scope.

  2. Build:
       source /usr/workspace/das9/miniconda3/etc/profile.d/conda.sh
       conda activate eftsan_env
       cd %s && source setup_eftsan.sh
       cd llvm_pass/build && make -j4
       cd ../../runtime && rm -rf obj && make CXX=clang++ LINKER=clang++

     The rm -rf obj is not optional: eftsan_bf_counter.h is not a make
     dependency, so an incremental build keeps the stale object and the
     hooks silently disagree about arity.

  3. Build LULESH -O0, then:
       awk -F, '$4=="lulesh.cc"{print $3; exit}' eftsan_sites.csv
     must print 1179233406. Anything else means the hash input is wrong and
     nothing downstream will join.

  4. python3 check_sites_eftsan.py --eftsan-sites eftsan_sites.csv \\
        --brsites '<gt build>/*.brsites'
     Expect 75 branch sites on both sides for LULESH -O0 fp-only.
""" % root)


def revert(root):
    P = paths(root)
    n = 0
    for key in ("pass_h", "pass_cpp", "rt_cpp"):
        bak = P[key] + ".bak-siteid"
        if os.path.exists(bak):
            shutil.copy2(bak, P[key])
            print("restore %s" % P[key])
            n += 1
        else:
            print("no backup for %s, left alone" % P[key])
    for key in ("siteid_h", "counter_h"):
        if os.path.exists(P[key]):
            os.remove(P[key])
            print("remove  %s" % P[key])
    if n:
        print("\nreverted. rm -rf runtime/obj before rebuilding.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="EFTSanitizer checkout")
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--check", action="store_true")
    g.add_argument("--apply", action="store_true")
    g.add_argument("--revert", action="store_true")
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    try:
        if args.check:
            problems, ok = check(root)
            for o in ok:
                print("  ok    %s" % o)
            for p in problems:
                print("  FAIL  %s" % p)
            print("\n%s" % ("READY" if not problems else "NOT READY"))
            return 1 if problems else 0
        if args.apply:
            apply(root)
            return 0
        if args.revert:
            revert(root)
            return 0
    except Fail as e:
        print("error: %s" % e, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())