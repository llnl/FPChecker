//===-- NsanBFSites.cpp ---------------------------------------------------===//
//
// Site enumeration and occurrence ticks for the nsan branch-flip study.
//
// nsan reports a flip as file:line on stderr, which cannot be joined against
// the brtrace oracle keyed on (module_id, site_id, k). This pass numbers every
// FP-controlled branch and select exactly as brtrace does (isFPControlled,
// firstFCmp and collectFCmps are transcribed from BranchTrace_mtu.cpp, table
// version 4 -- change them there first), writes the .nsansites manifest, and
// inserts a __nsan_bf_tick call before every fcmp nsan instruments. nsan's
// fail hook runs directly after the fcmp, so the runtime shim reads the
// pending site from the tick. Stock compiler-rt is enough.
//
// Every instrumentable fcmp is ticked, including out-of-scope ones (site -1),
// so a stale pending site can never be inherited. An fcmp feeding both a br
// and a select is attributed to the first site in walk order and counted as
// shared_fcmps in the manifest header.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <system_error>
#include <vector>

using namespace llvm;

// Matches brtrace's table version.
#define NSANBF_TABLE_VERSION 4

static cl::opt<bool> ClQuiet("nsanbf-quiet", cl::init(false),
                             cl::desc("suppress the per-TU site banner"),
                             cl::Hidden);

static cl::opt<bool>
    ClNoSelect("nsanbf-no-select", cl::init(false),
               cl::desc("do not number select sites (branch ids unaffected)"),
               cl::Hidden);

namespace {

enum SiteKind : uint32_t { KindBranch = 0, KindSelect = 1, KindOutOfScope = 2 };

// FNV-1a 32-bit. Must be bit-identical to brtrace's moduleHash.
static uint32_t moduleHash(StringRef s) {
  uint32_t h = 2166136261u;
  for (unsigned char c : s.bytes()) {
    h ^= c;
    h *= 16777619u;
  }
  return h;
}

// Types nsan carries a shadow for; ticking anything else would desynchronise
// the pending site.
static bool hasNsanShadow(Type *Ty) {
  Type *S = Ty->getScalarType();
  return S->isFloatTy() || S->isDoubleTy() || S->isX86_FP80Ty();
}

// Transcribed from BranchTrace_mtu.cpp; do not change here.

static bool isFPControlled(Value *V, SmallPtrSetImpl<Value *> &Seen,
                           unsigned Depth = 0) {
  if (Depth > 16 || !Seen.insert(V).second)
    return false;
  if (isa<FCmpInst>(V))
    return true;
  if (!V->getType()->isIntegerTy(1))
    return false;
  auto *U = dyn_cast<User>(V);
  if (!U)
    return false;
  if (isa<BinaryOperator>(U)) {
    for (Value *Op : U->operands())
      if (isFPControlled(Op, Seen, Depth + 1))
        return true;
  } else if (auto *CI = dyn_cast<CastInst>(U)) {
    Value *Src = CI->getOperand(0);
    if (Src->getType()->isIntegerTy(1) || CI->getType()->isIntegerTy(1))
      if (isFPControlled(Src, Seen, Depth + 1))
        return true;
  } else if (auto *FI = dyn_cast<FreezeInst>(U)) {
    if (isFPControlled(FI->getOperand(0), Seen, Depth + 1))
      return true;
  } else if (auto *PN = dyn_cast<PHINode>(U)) {
    bool sawFCmp = false;
    for (Value *In : PN->incoming_values()) {
      if (isa<FCmpInst>(In))
        sawFCmp = true;
      else if (!isa<ConstantInt>(In))
        return false;
    }
    if (sawFCmp)
      return true;
  }
  return false;
}

// Operand-order walk, therefore deterministic. Used for the site label only.
static FCmpInst *firstFCmp(Value *V, SmallPtrSetImpl<Value *> &Seen,
                           unsigned Depth = 0) {
  if (Depth > 16 || !Seen.insert(V).second)
    return nullptr;
  if (auto *FC = dyn_cast<FCmpInst>(V))
    return FC;
  if (!V->getType()->isIntegerTy(1))
    return nullptr;
  auto *U = dyn_cast<User>(V);
  if (!U)
    return nullptr;
  if (isa<BinaryOperator>(U)) {
    for (Value *Op : U->operands())
      if (FCmpInst *R = firstFCmp(Op, Seen, Depth + 1))
        return R;
  } else if (auto *CI = dyn_cast<CastInst>(U)) {
    Value *Src = CI->getOperand(0);
    if (Src->getType()->isIntegerTy(1) || CI->getType()->isIntegerTy(1))
      return firstFCmp(Src, Seen, Depth + 1);
  } else if (auto *FI = dyn_cast<FreezeInst>(U)) {
    return firstFCmp(FI->getOperand(0), Seen, Depth + 1);
  }
  return nullptr;
}

// Every distinct fcmp reachable through the same i1 chain, in deterministic
// order.
static void collectFCmps(Value *V, SmallVectorImpl<FCmpInst *> &Out,
                         SmallPtrSetImpl<Value *> &Seen, unsigned Depth = 0) {
  if (Depth > 16 || !Seen.insert(V).second)
    return;
  if (auto *FC = dyn_cast<FCmpInst>(V)) {
    for (FCmpInst *E : Out)
      if (E == FC)
        return;
    Out.push_back(FC);
    return;
  }
  if (!V->getType()->isIntegerTy(1))
    return;
  auto *U = dyn_cast<User>(V);
  if (!U)
    return;
  if (isa<BinaryOperator>(U)) {
    for (Value *Op : U->operands())
      collectFCmps(Op, Out, Seen, Depth + 1);
  } else if (auto *CI = dyn_cast<CastInst>(U)) {
    Value *Src = CI->getOperand(0);
    if (Src->getType()->isIntegerTy(1) || CI->getType()->isIntegerTy(1))
      collectFCmps(Src, Out, Seen, Depth + 1);
  } else if (auto *FI = dyn_cast<FreezeInst>(U)) {
    collectFCmps(FI->getOperand(0), Out, Seen, Depth + 1);
  } else if (auto *PN = dyn_cast<PHINode>(U)) {
    bool sawFCmp = false;
    for (Value *In : PN->incoming_values()) {
      if (isa<FCmpInst>(In))
        sawFCmp = true;
      else if (!isa<ConstantInt>(In))
        return;
    }
    if (!sawFCmp)
      return;
    for (Value *In : PN->incoming_values())
      collectFCmps(In, Out, Seen, Depth + 1);
  }
}

//===----------------------------------------------------------------------===//

static bool locOf(const Instruction *I, const Function &F, std::string &S) {
  const DebugLoc &DL = I->getDebugLoc();
  if (!DL)
    return false;
  StringRef File = "?";
  if (auto *Scope = dyn_cast_or_null<DIScope>(DL.getScope()))
    File = Scope->getFilename();
  S = (File + ":" + Twine(DL.getLine()) + ":" + Twine(DL.getCol()) + "\t" +
       F.getName())
          .str();
  return true;
}

// Label anchored on the controlling fcmp, terminator as fallback (brtrace v3).
static std::string locString(const Instruction *I, const Function &F,
                             Value *Cond) {
  std::string S;
  if (Cond) {
    SmallPtrSet<Value *, 16> Seen;
    if (FCmpInst *FC = firstFCmp(Cond, Seen))
      if (locOf(FC, F, S))
        return S;
  }
  if (locOf(I, F, S))
    return S;
  return ("<no-dbg>:0:0\t" + F.getName()).str();
}

struct Row {
  uint32_t Id;
  std::string Loc; // "file:line:col\tfunction"
  unsigned NFCmp;
};

struct NsanBFSitesPass : PassInfoMixin<NsanBFSitesPass> {

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32 = Type::getInt32Ty(Ctx);

    FunctionCallee Tick =
        M.getOrInsertFunction("__nsan_bf_tick", VoidTy, I32, I32, I32);

    std::string ModName = M.getModuleIdentifier();
    if (ModName.empty())
      ModName = "module";
    StringRef Base(ModName);
    size_t Slash = Base.find_last_of("/\\");
    if (Slash != StringRef::npos)
      Base = Base.substr(Slash + 1);
    const uint32_t ModId = moduleHash(Base);

    uint32_t SiteId = 0, SelId = 0;
    std::vector<Row> BranchRows, SelectRows;

    // fcmp -> (site_id, kind); first writer wins, collisions counted.
    std::map<FCmpInst *, std::pair<int32_t, SiteKind>> Owner;
    unsigned SharedFCmps = 0;

    auto claim = [&](FCmpInst *FC, int32_t Id, SiteKind K) {
      if (Owner.find(FC) != Owner.end()) {
        ++SharedFCmps;
        return;
      }
      Owner[FC] = {Id, K};
    };

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      // Selects first, per function; this order is what brtrace uses.
      if (!ClNoSelect) {
        SmallVector<SelectInst *, 32> Sels;
        for (BasicBlock &BB : F)
          for (Instruction &I : BB)
            if (auto *SelI = dyn_cast<SelectInst>(&I))
              Sels.push_back(SelI);

        for (SelectInst *SelI : Sels) {
          Value *Cond = SelI->getCondition();
          if (!Cond->getType()->isIntegerTy(1))
            continue; // vector select: no single outcome
          SmallPtrSet<Value *, 16> Seen;
          if (!isFPControlled(Cond, Seen))
            continue;

          SmallVector<FCmpInst *, 4> FCs;
          SmallPtrSet<Value *, 16> Seen2;
          collectFCmps(Cond, FCs, Seen2);

          SelectRows.push_back(
              {SelId, locString(SelI, F, Cond), (unsigned)FCs.size()});
          for (FCmpInst *FC : FCs)
            claim(FC, (int32_t)SelId, KindSelect);
          ++SelId;
        }
      }

      for (BasicBlock &BB : F) {
        auto *BI = dyn_cast_or_null<BranchInst>(BB.getTerminator());
        if (!BI || !BI->isConditional())
          continue;
        SmallPtrSet<Value *, 16> Seen;
        if (!isFPControlled(BI->getCondition(), Seen))
          continue;

        SmallVector<FCmpInst *, 4> FCs;
        SmallPtrSet<Value *, 16> Seen2;
        collectFCmps(BI->getCondition(), FCs, Seen2);

        BranchRows.push_back({SiteId, locString(BI, F, BI->getCondition()),
                              (unsigned)FCs.size()});
        for (FCmpInst *FC : FCs)
          claim(FC, (int32_t)SiteId, KindBranch);
        ++SiteId;
      }
    }

    // Ticks on every instrumentable fcmp, owned or not.
    unsigned NumOOS = 0, NumTicks = 0;
    for (Function &F : M) {
      if (F.isDeclaration())
        continue;
      for (BasicBlock &BB : F)
        for (Instruction &I : BB) {
          auto *FC = dyn_cast<FCmpInst>(&I);
          if (!FC || !hasNsanShadow(FC->getOperand(0)->getType()))
            continue;
          int32_t Id = -1;
          SiteKind K = KindOutOfScope;
          auto It = Owner.find(FC);
          if (It != Owner.end()) {
            Id = It->second.first;
            K = It->second.second;
          } else {
            ++NumOOS;
          }
          IRBuilder<> B(FC);
          B.SetCurrentDebugLocation(FC->getDebugLoc());
          B.CreateCall(Tick, {ConstantInt::get(I32, ModId),
                              ConstantInt::get(I32, (uint32_t)Id),
                              ConstantInt::get(I32, (uint32_t)K)});
          ++NumTicks;
        }
    }

    writeTable(ModName, ModId, "branch", BranchRows, SharedFCmps, NumOOS);
    if (!ClNoSelect)
      writeTable(ModName, ModId, "select", SelectRows, SharedFCmps, NumOOS);

    if (!ClQuiet) {
      errs() << "[NSanBF] " << Base << " (mod " << ModId << "): " << SiteId
             << " FP-controlled branch sites, " << SelId << " select sites, "
             << NumOOS << " out-of-scope fcmps";
      if (SharedFCmps)
        errs() << ", " << SharedFCmps << " shared";
      errs() << "\n";
    }

    return NumTicks ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  // Same shape as brtrace's .brsites / .brselsites.
  static void writeTable(const std::string &ModName, uint32_t ModId,
                         const char *Kind, const std::vector<Row> &Rows,
                         unsigned Shared, unsigned NumOOS) {
    std::string Ext =
        (StringRef(Kind) == "branch") ? ".nsansites" : ".nsanselsites";
    std::error_code EC;
    raw_fd_ostream OS(ModName + Ext, EC, sys::fs::OF_Text);
    if (EC) {
      errs() << "[NSanBF] cannot write " << ModName << Ext << ": "
             << EC.message() << "\n";
      return;
    }
    const char *Opt = std::getenv("NSAN_BF_OPT_LEVEL");
    OS << "# nsan-table-version " << NSANBF_TABLE_VERSION << "\n"
       << "# kind " << Kind << "\n"
       << "# loc_anchor fcmp\n"
       << "# fp_only 1\n"
       << "# opt " << (Opt ? Opt : "unknown") << "\n"
       << "# module_id " << ModId << "  module " << ModName << "\n"
       << "# n_sites " << Rows.size() << "\n"
       << "# out_of_scope_fcmps " << NumOOS << "\n"
       << "# shared_fcmps " << Shared << "\n"
       << "# site_id\tfile:line:col\tfunction\tn_fcmp\n";
    for (const Row &R : Rows)
      OS << R.Id << "\t" << R.Loc << "\t" << R.NFCmp << "\n";
  }
};

} // namespace

llvm::PassPluginLibraryInfo getNsanBFSitesPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "NsanBFSites", "0.2", [](PassBuilder &PB) {
            // PipelineStartEP: pristine IR, same point brtrace walks from.
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                  MPM.addPass(NsanBFSitesPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "nsan-bf-sites") {
                    MPM.addPass(NsanBFSitesPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getNsanBFSitesPluginInfo();
}
