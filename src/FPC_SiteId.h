#ifndef SRC_FPC_SITEID_H_
#define SRC_FPC_SITEID_H_

/* FPC_SiteId.h -- stable (module_id, site_id) for FP-controlled branches.
 *
 *   module_id = FNV-1a-32 over basename(Module::getModuleIdentifier())
 *   site_id   = ordinal of the branch in a deterministic module walk,
 *               counting only branches whose condition is FP-controlled
 *
 * Source location alone cannot name a site: `a > 0 && b > 0` is two branches
 * on one line, and inlined headers are reported under the including TU.
 *
 * This is an independent implementation of the convention brtrace uses;
 * nothing here reads brtrace output. It emits <module>.fpcsites so the two
 * enumerations can be diffed (check_sites.py). Must run at the same pipeline
 * point as brtrace, before any FPChecker instrumentation, and at the same
 * optimisation level for the dynamic data (occurrence indices) to match. */

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <string>
#include <system_error>

namespace FPCSite {

/// FNV-1a, 32-bit. Constants fixed by the convention.
inline uint32_t fnv1a32(llvm::StringRef S) {
  uint32_t H = 2166136261u;
  for (unsigned char C : S.bytes()) {
    H ^= C;
    H *= 16777619u;
  }
  return H;
}

/// module_id: hash of the basename of getModuleIdentifier(), so the same
/// source built in different directories gets the same id.
inline uint32_t moduleId(const llvm::Module &M) {
  std::string Name = M.getModuleIdentifier();
  if (Name.empty())
    Name = "module";
  llvm::StringRef Base(Name);
  size_t Slash = Base.find_last_of("/\\");
  if (Slash != llvm::StringRef::npos)
    Base = Base.substr(Slash + 1);
  return fnv1a32(Base);
}

/// True iff V is built from floating-point compares through i1 logic, i1
/// casts, freeze, and a restricted phi (every incoming an fcmp or constant i1
/// -- the -O0 lowering of a short-circuit `&&`). Does not recurse through
/// select or wider integers, which would mislabel integer loop counters. Must
/// match the reference implementation exactly: any difference shifts every
/// later site_id.
inline bool isFPControlled(const llvm::Value *V,
                           llvm::SmallPtrSetImpl<const llvm::Value *> &Seen,
                           unsigned Depth = 0) {
  if (Depth > 16 || !Seen.insert(V).second)
    return false;
  if (llvm::isa<llvm::FCmpInst>(V))
    return true;
  if (!V->getType()->isIntegerTy(1))
    return false;
  const auto *U = llvm::dyn_cast<llvm::User>(V);
  if (!U)
    return false;
  if (llvm::isa<llvm::BinaryOperator>(U)) { // and/or/xor on i1
    for (const llvm::Value *Op : U->operands())
      if (isFPControlled(Op, Seen, Depth + 1))
        return true;
  } else if (const auto *CI = llvm::dyn_cast<llvm::CastInst>(U)) {
    const llvm::Value *Src = CI->getOperand(0);
    if (Src->getType()->isIntegerTy(1) || CI->getType()->isIntegerTy(1))
      if (isFPControlled(Src, Seen, Depth + 1))
        return true;
  } else if (const auto *FI = llvm::dyn_cast<llvm::FreezeInst>(U)) {
    if (isFPControlled(FI->getOperand(0), Seen, Depth + 1))
      return true;
  } else if (const auto *PN = llvm::dyn_cast<llvm::PHINode>(U)) {
    // Restricted phi: every incoming an fcmp or a constant i1 (the -O0
    // lowering of a short-circuit `&&`). Phis merging icmps stay rejected.
    bool sawFCmp = false;
    for (const llvm::Value *In : PN->incoming_values()) {
      if (llvm::isa<llvm::FCmpInst>(In))
        sawFCmp = true;
      else if (!llvm::isa<llvm::ConstantInt>(In))
        return false;
    }
    if (sawFCmp)
      return true;      // a phi of only constants controls nothing
  }
  return false;
}

inline bool isFPControlled(const llvm::Value *V) {
  llvm::SmallPtrSet<const llvm::Value *, 16> Seen;
  return isFPControlled(V, Seen);
}

/// The distinct fcmps reachable through the same chain isFPControlled walks.
/// >1 means several comparison sites map onto one branch site.
inline void collectFCmps(llvm::Value *V,
                         llvm::SmallPtrSetImpl<llvm::Value *> &Seen,
                         llvm::SmallVectorImpl<llvm::FCmpInst *> &Out,
                         unsigned Depth = 0) {
  if (Depth > 16 || !Seen.insert(V).second)
    return;
  if (auto *FC = llvm::dyn_cast<llvm::FCmpInst>(V)) {
    Out.push_back(FC);
    return;
  }
  if (!V->getType()->isIntegerTy(1))
    return;
  auto *U = llvm::dyn_cast<llvm::User>(V);
  if (!U)
    return;
  if (llvm::isa<llvm::BinaryOperator>(U)) {
    for (llvm::Value *Op : U->operands())
      collectFCmps(Op, Seen, Out, Depth + 1);
  } else if (auto *CI = llvm::dyn_cast<llvm::CastInst>(U)) {
    llvm::Value *Src = CI->getOperand(0);
    if (Src->getType()->isIntegerTy(1) || CI->getType()->isIntegerTy(1))
      collectFCmps(Src, Seen, Out, Depth + 1);
  } else if (auto *FI = llvm::dyn_cast<llvm::FreezeInst>(U)) {
    collectFCmps(FI->getOperand(0), Seen, Out, Depth + 1);
  } else if (auto *PN = llvm::dyn_cast<llvm::PHINode>(U)) {
    // Same phi restriction as isFPControlled(); the two walks must agree.
    bool ok = true;
    for (llvm::Value *In : PN->incoming_values())
      if (!llvm::isa<llvm::FCmpInst>(In) && !llvm::isa<llvm::ConstantInt>(In))
        ok = false;
    if (ok)
      for (llvm::Value *In : PN->incoming_values())
        if (auto *FC = llvm::dyn_cast<llvm::FCmpInst>(In))
          if (Seen.insert(FC).second)
            Out.push_back(FC);
  }
}

/// "file:line:col" from the instruction's own DILocation, walking
/// getInlinedAt() to the outermost frame.
inline std::string locString(const llvm::Instruction *I) {
  if (const llvm::DebugLoc &DL = I->getDebugLoc()) {
    if (llvm::DILocation *Loc = DL.get()) {
      while (llvm::DILocation *Up = Loc->getInlinedAt())
        Loc = Up;
      return (Loc->getFilename() + ":" + llvm::Twine(Loc->getLine()) + ":" +
              llvm::Twine(Loc->getColumn()))
          .str();
    }
  }
  return "<no-dbg>:0:0";
}

/// Just the file part of locString, for the hook's file_name argument.
inline std::string fileOf(const llvm::Instruction *I) {
  if (const llvm::DebugLoc &DL = I->getDebugLoc()) {
    if (llvm::DILocation *Loc = DL.get()) {
      while (llvm::DILocation *Up = Loc->getInlinedAt())
        Loc = Up;
      return Loc->getFilename().str();
    }
  }
  return "Unknown";
}

/// Line, walking getInlinedAt().
inline int lineOf(const llvm::Instruction *I) {
  if (const llvm::DebugLoc &DL = I->getDebugLoc()) {
    if (llvm::DILocation *Loc = DL.get()) {
      while (llvm::DILocation *Up = Loc->getInlinedAt())
        Loc = Up;
      return (int)Loc->getLine();
    }
  }
  return -1;
}

/// FPChecker's own runtime functions (force-included into every TU). Skipped:
/// the oracle never sees them, and counting them would shift every site_id.
inline bool isFPCheckerRuntime(const llvm::Function &F) {
  llvm::StringRef N = F.getName();
  return N.str().find("_FPC_") != std::string::npos ||
         N.str().find("FPC_") != std::string::npos;
}

/// One entry per numbered branch site.
struct SiteInfo {
  uint32_t SiteId;
  unsigned NumFCmps;      ///< >1 means several fcmps share this site
  std::string Loc;        ///< file:line:col
  std::string Func;
};

/// The per-module site map. Build ONCE, before instrumenting anything.
class SiteMap {
public:
  /// OptLevel is recorded in the manifest; pass "" if unknown.
  explicit SiteMap(llvm::Module &M, llvm::StringRef OptLevel = "")
      : ModId(moduleId(M)), Opt(OptLevel.str()) {
    build(M);
  }

  uint32_t getModuleId() const { return ModId; }
  uint32_t getNumSites() const { return NumSites; }

  /// site_id for an fcmp, or -1 if it does not control a numbered branch.
  ///
  /// -1 if the fcmp controls no numbered branch (select, zext-to-bool, call).
  /// Report it with -1; do not drop it.
  int32_t siteFor(const llvm::Instruction *FCmp) const {
    auto It = FCmpToSite.find(FCmp);
    return It == FCmpToSite.end() ? -1 : (int32_t)It->second;
  }

  unsigned fcmpsAt(uint32_t SiteId) const {
    return SiteId < Sites.size() ? Sites[SiteId].NumFCmps : 0;
  }

  /// Write <module>.fpcsites for the drift gate. Same shape as .brsites.
  void writeManifest(const llvm::Module &M) const {
    std::string Path = M.getModuleIdentifier();
    if (Path.empty())
      Path = "module";
    Path += ".fpcsites";
    std::error_code EC;
    llvm::raw_fd_ostream OS(Path, EC);
    if (EC)
      return;
    /* Version 3: restricted-phi rule; v2 and v3 manifests are not comparable. */
    OS << "# fpchecker-table-version 3\n";
    OS << "# kind branch\n";
    /* Label anchor: the fcmp when there is one, else the branch terminator. */
    OS << "# loc_anchor fcmp  (branch terminator where n_fcmp > 1)\n";
    OS << "# fp_only 1\n";
    OS << "# opt " << (Opt.empty() ? "unknown" : Opt) << "\n";
    OS << "# module_id " << ModId << "  module " << M.getModuleIdentifier()
       << "\n";
    OS << "# n_sites " << NumSites << "\n";
    OS << "# site_id\tfile:line:col\tfunction\tn_fcmp\n";
    for (const SiteInfo &S : Sites)
      OS << S.SiteId << "\t" << S.Loc << "\t" << S.Func << "\t" << S.NumFCmps
         << "\n";
  }

  /// Sites whose fcmp count is >1, i.e. where the fcmp->branch map is not 1:1.
  unsigned countMultiFCmpSites() const {
    unsigned N = 0;
    for (const SiteInfo &S : Sites)
      if (S.NumFCmps > 1)
        ++N;
    return N;
  }

private:
  void build(llvm::Module &M) {
    for (llvm::Function &F : M) {
      if (F.isDeclaration())
        continue;
      if (isFPCheckerRuntime(F))
        continue;
      for (llvm::BasicBlock &BB : F) {
        auto *BI = llvm::dyn_cast_or_null<llvm::BranchInst>(BB.getTerminator());
        if (!BI || !BI->isConditional())
          continue;
        if (!isFPControlled(BI->getCondition()))
          continue;

        llvm::SmallPtrSet<llvm::Value *, 16> Seen;
        llvm::SmallVector<llvm::FCmpInst *, 4> Cmps;
        collectFCmps(BI->getCondition(), Seen, Cmps);

        for (llvm::FCmpInst *C : Cmps)
          FCmpToSite[C] = NumSites;

        SiteInfo SI;
        SI.SiteId = NumSites;
        SI.NumFCmps = (unsigned)Cmps.size();
        // Anchor on the fcmp when there is exactly one, else on the branch.
        const llvm::Instruction *Anchor =
            (Cmps.size() == 1) ? (const llvm::Instruction *)Cmps[0]
                               : (const llvm::Instruction *)BI;
        SI.Loc = locString(Anchor);
        SI.Func = F.getName().str();
        Sites.push_back(SI);

        ++NumSites;
      }
    }
  }

  uint32_t ModId;
  std::string Opt;
  uint32_t NumSites = 0;
  llvm::DenseMap<const llvm::Instruction *, uint32_t> FCmpToSite;
  std::vector<SiteInfo> Sites;
};

} // namespace FPCSite

#endif /* SRC_FPC_SITEID_H_ */
