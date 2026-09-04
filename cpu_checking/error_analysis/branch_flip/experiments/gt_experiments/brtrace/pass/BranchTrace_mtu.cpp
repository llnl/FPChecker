//===- BranchTrace_mtu.cpp - log conditional-branch directions ------------===//
//
// New-PM module pass. Before every conditional branch (and switch, unless
// -brtrace-fp-only) it inserts
//     void __brtrace_log(uint32_t module_id, uint32_t site_id, int32_t taken);
// and before every FP-controlled select
//     void __brtrace_log_select(uint32_t module_id, uint32_t sel_id, int32_t taken);
//
// module_id is FNV-1a-32 over the module's basename, site_id is the ordinal
// of the branch in a deterministic module walk, so (module_id, site_id) is
// unique across the link and identical between fp32 and fp64 builds of the
// same source. Selects use a separate id space, hook and output stream, so
// branch site_ids are unaffected by them.
//
// With -brtrace-fp-only only branches whose condition derives from an fcmp
// through i1 logic, i1 casts, freeze and a restricted phi (every incoming an
// fcmp or constant i1 -- the -O0 lowering of a short-circuit &&) are
// instrumented. Both builds of a pair must use the same setting.
//
// Site labels are anchored on the controlling fcmp (falling back to the
// terminator), so FPChecker and EFTSan, which read DILocation off the fcmp,
// name the same instruction. Side tables per module:
//   <module>.brsites    site_id \t file:line:col \t function \t n_fcmp
//   <module>.brselsites sel_id  \t file:line:col \t function \t n_fcmp
//   <module>.brmods     module_id \t module_path
// Table version 4.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <string>
#include <system_error>

using namespace llvm;

namespace {

static bool kInstrumentSwitch = true;

static cl::opt<bool> FPOnly(
    "brtrace-fp-only", cl::init(false),
    cl::desc("Only instrument branches controlled by a floating-point compare"));

static cl::opt<bool> NoSelect(
    "brtrace-no-select", cl::init(false),
    cl::desc("Do not instrument SelectInst conditions"));

static cl::opt<bool> LocFromTerminator(
    "brtrace-loc-from-terminator", cl::init(false),
    cl::desc("Label sites by the terminator's debug location (v2 behaviour) "
             "instead of the controlling fcmp's"));

static uint32_t moduleHash(StringRef s) {
  uint32_t h = 2166136261u;
  for (unsigned char c : s.bytes()) {
    h ^= c;
    h *= 16777619u;
  }
  return h;
}

// True iff V is a predicate built from fcmps through i1 logic, i1 casts,
// freeze and a restricted phi. Does not recurse through select or wider
// integers (that would mislabel integer loop counters as FP-controlled).
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
    // Restricted phi: every incoming an fcmp or a constant i1.
    bool sawFCmp = false;
    for (Value *In : PN->incoming_values()) {
      if (isa<FCmpInst>(In)) {
        sawFCmp = true;
      } else if (!isa<ConstantInt>(In)) {
        return false;
      }
    }
    if (sawFCmp)
      return true;
  }
  return false;
}

// The fcmp a site's label names: first one reached in operand order (a
// deterministic walk, unlike iterating the set collectFCmps builds).
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

// Distinct fcmps reachable through the same chain isFPControlled walks;
// n_fcmp > 1 means several comparison sites map onto one branch site.
static void collectFCmps(Value *V, SmallPtrSetImpl<Value *> &Seen,
                         SmallPtrSetImpl<Value *> &Out, unsigned Depth = 0) {
  if (Depth > 16 || !Seen.insert(V).second)
    return;
  if (isa<FCmpInst>(V)) {
    Out.insert(V);
    return;
  }
  if (!V->getType()->isIntegerTy(1))
    return;
  auto *U = dyn_cast<User>(V);
  if (!U)
    return;
  if (isa<BinaryOperator>(U)) {
    for (Value *Op : U->operands())
      collectFCmps(Op, Seen, Out, Depth + 1);
  } else if (auto *CI = dyn_cast<CastInst>(U)) {
    Value *Src = CI->getOperand(0);
    if (Src->getType()->isIntegerTy(1) || CI->getType()->isIntegerTy(1))
      collectFCmps(Src, Seen, Out, Depth + 1);
  } else if (auto *FI = dyn_cast<FreezeInst>(U)) {
    collectFCmps(FI->getOperand(0), Seen, Out, Depth + 1);
  } else if (auto *PN = dyn_cast<PHINode>(U)) {
    // Same phi restriction as isFPControlled(); the two walks must agree.
    bool ok = true;
    for (Value *In : PN->incoming_values())
      if (!isa<FCmpInst>(In) && !isa<ConstantInt>(In))
        ok = false;
    if (ok)
      for (Value *In : PN->incoming_values())
        if (isa<FCmpInst>(In))
          Out.insert(In);
  }
}

static unsigned countFCmps(Value *Cond) {
  SmallPtrSet<Value *, 16> Seen;
  SmallPtrSet<Value *, 8> Out;
  collectFCmps(Cond, Seen, Out);
  return Out.size();
}

struct BranchTracePass : PassInfoMixin<BranchTracePass> {

  FunctionCallee getHook(Module &M) {
    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32 = Type::getInt32Ty(Ctx);
    FunctionType *FT = FunctionType::get(VoidTy, {I32, I32, I32}, false);
    return M.getOrInsertFunction("__brtrace_log", FT);
  }

  FunctionCallee getSelectHook(Module &M) {
    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32 = Type::getInt32Ty(Ctx);
    FunctionType *FT = FunctionType::get(VoidTy, {I32, I32, I32}, false);
    return M.getOrInsertFunction("__brtrace_log_select", FT);
  }

  // file:line:col of one instruction, walking getInlinedAt() to the
  // outermost frame. False if it carries no debug location.
  static bool locOf(const Instruction *I, const Function &F,
                    std::string &Out) {
    if (const DebugLoc &DL = I->getDebugLoc()) {
      if (DILocation *Loc = DL.get()) {
        while (DILocation *Up = Loc->getInlinedAt())
          Loc = Up;
        Out = (Loc->getFilename() + ":" + Twine(Loc->getLine()) + ":" +
               Twine(Loc->getColumn()) + "\t" + F.getName())
                  .str();
        return true;
      }
    }
    return false;
  }

  // Label anchored on the controlling fcmp when Cond is given, else on I.
  static std::string locString(const Instruction *I, const Function &F,
                               Value *Cond = nullptr) {
    std::string S;
    if (Cond && !LocFromTerminator) {
      SmallPtrSet<Value *, 16> Seen;
      if (FCmpInst *FC = firstFCmp(Cond, Seen))
        if (locOf(FC, F, S))
          return S;
    }
    if (locOf(I, F, S))
      return S;
    return ("<no-dbg>:0:0\t" + F.getName()).str();
  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    LLVMContext &Ctx = M.getContext();
    Type *I32 = Type::getInt32Ty(Ctx);
    FunctionCallee Hook = getHook(M);
    FunctionCallee SelHook = getSelectHook(M);

    std::string ModName = M.getModuleIdentifier();
    if (ModName.empty())
      ModName = "module";
    // Hash the basename so the same source built from different directories
    // gets the same module_id.
    StringRef Base = StringRef(ModName);
    size_t Slash = Base.find_last_of("/\\");
    if (Slash != StringRef::npos)
      Base = Base.substr(Slash + 1);
    uint32_t ModId = moduleHash(Base);
    Constant *ModIdC = ConstantInt::get(I32, ModId);

    uint32_t SiteId = 0;
    std::string SiteTable;
    uint32_t SelId = 0;
    std::string SelTable;
    bool Changed = false;

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      // Selects first, collected before any switch lowering below can
      // synthesise its own SelectInsts.
      if (!NoSelect) {
        SmallVector<SelectInst *, 32> Sels;
        for (BasicBlock &BB : F)
          for (Instruction &I : BB)
            if (auto *SelI = dyn_cast<SelectInst>(&I))
              Sels.push_back(SelI);

        for (SelectInst *SelI : Sels) {
          Value *Cond = SelI->getCondition();
          if (!Cond->getType()->isIntegerTy(1))
            continue;
          if (FPOnly) {
            SmallPtrSet<Value *, 16> Seen;
            if (!isFPControlled(Cond, Seen))
              continue;
          }
          IRBuilder<> B(SelI);
          Value *Taken = B.CreateZExtOrTrunc(Cond, I32);
          B.CreateCall(SelHook, {ModIdC, ConstantInt::get(I32, SelId), Taken});
          SelTable += (Twine(SelId) + "\t" + locString(SelI, F, Cond) + "\t" +
                       Twine(countFCmps(Cond)) + "\n")
                          .str();
          ++SelId;
          Changed = true;
        }
      }

      for (BasicBlock &BB : F) {
        Instruction *Term = BB.getTerminator();
        if (!Term)
          continue;

        if (auto *BI = dyn_cast<BranchInst>(Term)) {
          if (!BI->isConditional())
            continue;
          if (FPOnly) {
            SmallPtrSet<Value *, 16> Seen;
            if (!isFPControlled(BI->getCondition(), Seen))
              continue;
          }
          IRBuilder<> B(BI);
          Value *Taken = B.CreateZExtOrTrunc(BI->getCondition(), I32);
          B.CreateCall(Hook, {ModIdC, ConstantInt::get(I32, SiteId), Taken});
          SiteTable += (Twine(SiteId) + "\t" +
                        locString(BI, F, BI->getCondition()) + "\t" +
                        Twine(countFCmps(BI->getCondition())) + "\n")
                           .str();
          ++SiteId;
          Changed = true;
        } else if (kInstrumentSwitch && !FPOnly) {
          if (auto *SI = dyn_cast<SwitchInst>(Term)) {
            IRBuilder<> B(SI);
            Value *CondV = SI->getCondition();
            Value *Sel = ConstantInt::get(I32, 0);
            unsigned idx = 1;
            for (auto Case : SI->cases()) {
              Value *Eq = B.CreateICmpEQ(CondV, Case.getCaseValue());
              Sel = B.CreateSelect(Eq, ConstantInt::get(I32, idx), Sel);
              ++idx;
            }
            B.CreateCall(Hook, {ModIdC, ConstantInt::get(I32, SiteId), Sel});
            SiteTable += (Twine(SiteId) + "\t" + locString(SI, F) +
                          "\t0\n")
                             .str();
            ++SiteId;
            Changed = true;
          }
        }
      }
    }

    const char *LocAnchor = LocFromTerminator ? "terminator" : "fcmp";

    if (!SiteTable.empty()) {
      std::error_code EC;
      {
        raw_fd_ostream OS(ModName + ".brsites", EC);
        if (!EC) {
          OS << "# brtrace-table-version 4\n";
          OS << "# kind branch\n";
          OS << "# loc_anchor " << LocAnchor << "\n";
          OS << "# fp_only " << (FPOnly ? 1 : 0) << "\n";
          OS << "# module_id " << ModId << "  module " << ModName << "\n";
          OS << "# n_sites " << SiteId << "\n";
          OS << "# site_id\tfile:line:col\tfunction\tn_fcmp\n";
          OS << SiteTable;
        }
      }
      {
        std::error_code EC2;
        raw_fd_ostream OS2(ModName + ".brmods", EC2);
        if (!EC2)
          OS2 << ModId << "\t" << ModName << "\n";
      }
    }

    if (!SelTable.empty()) {
      std::error_code EC3;
      raw_fd_ostream OS3(ModName + ".brselsites", EC3);
      if (!EC3) {
        OS3 << "# brtrace-table-version 4\n";
        OS3 << "# kind select\n";
        OS3 << "# loc_anchor " << LocAnchor << "\n";
        OS3 << "# fp_only " << (FPOnly ? 1 : 0) << "\n";
        OS3 << "# module_id " << ModId << "  module " << ModName << "\n";
        OS3 << "# n_sites " << SelId << "\n";
        OS3 << "# sel_id\tfile:line:col\tfunction\tn_fcmp\n";
        OS3 << SelTable;
      }
    }

    errs() << "[BranchTrace] " << ModName << " (mod " << ModId
           << "): instrumented " << SiteId << " branch sites";
    if (!NoSelect)
      errs() << ", " << SelId << " select sites";
    errs() << (FPOnly ? " (fp-only)" : "") << " [loc " << LocAnchor << "]\n";
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

llvm::PassPluginLibraryInfo getBranchTracePluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "BranchTrace", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                  MPM.addPass(BranchTracePass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "branch-trace") {
                    MPM.addPass(BranchTracePass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getBranchTracePluginInfo();
}
