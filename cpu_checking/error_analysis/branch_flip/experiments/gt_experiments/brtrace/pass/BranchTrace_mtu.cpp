//===- BranchTrace.cpp - Log conditional-branch directions (multi-TU) -----===//
//
// New-PM module pass. For each conditional BranchInst (and optionally each
// SwitchInst) it inserts, before the terminator, a call to:
//
//     void __brtrace_log(uint32_t module_id, uint32_t site_id, int32_t taken);
//
// It ALSO instruments FP-controlled SelectInst conditions, via a separate hook:
//
//     void __brtrace_log_select(uint32_t module_id, uint32_t sel_id,
//                               int32_t taken);
//
// MULTI-TU DESIGN
// ---------------
// The single-file version numbered sites 0,1,2,... per module, which collides
// across translation units when several instrumented .o files are linked into
// one binary (any multi-TU program). Here each module also
// carries a `module_id` = stable 32-bit hash of its source path, so the pair
// (module_id, site_id) is globally unique across the whole link. site_id is
// still assigned by a deterministic per-module walk, so fp32 and fp64 builds of
// the SAME source get identical (module_id, site_id) for corresponding
// branches -- which is what lets the diff line the two traces up.
//
// `taken`:
//   conditional br : 0 or 1 (i1 condition, zero-extended)
//   switch         : selected successor index (0 = default)
//   select         : 0 or 1 (i1 condition, zero-extended)
//
// FP-ONLY MODE
// ------------
// With -brtrace-fp-only, only branches whose condition is derived from a
// floating-point compare (fcmp) are instrumented. The walk goes one hop
// through boolean logic / casts / selects / phis, so it catches fcmp->br,
// (fcmp && fcmp)->br, and fcmp->zext->... patterns -- not just the direct
// fcmp->br case. Switches are integer-controlled and are skipped in this mode.
// Both builds of a pair MUST use the same flag, or site_ids won't align.
//
// SELECT SITES
// ------------
// FPChecker fires at fcmp->select sites (ternaries, std::max once inlined and
// folded, fmin/fmax idioms). A terminator-only pass cannot see them: a select
// is not a terminator, so a select-flip has no counterpart in the branch trace
// and cannot be adjudicated. Instrumenting them closes that gap and makes
// brtrace's instrumented class a superset of FPChecker's.
//
// Select sites use a SEPARATE id space (SelId), a SEPARATE runtime hook, and a
// SEPARATE output stream. This is deliberate and load-bearing: branch site_ids
// are BIT-IDENTICAL to what they were before selects existed, so traces and
// censuses collected with the older pass remain valid, and lock-step
// adjudication of the branch stream is completely unaffected. Verify after
// rebuilding by checking that a benchmark's branch-site total is unchanged
// (LULESH -O0 fp-only: 75).
//
// Note on interpretation: a SelectInst has no successors. Both arms are
// evaluated and one value is chosen, so a select flip cannot change which site
// executes next, cannot change loop trip count, and cannot end lock-step
// alignment. Select flips are therefore inert with respect to control flow BY
// CONSTRUCTION. They are recorded so the claim can be made empirically rather
// than by argument; they are not scored as TP/FP against the branch oracle.
//
// Side tables emitted next to each module:
//   <module>.brsites    : "site_id \t file:line \t function"
//   <module>.brselsites : "sel_id  \t file:line \t function"
//   <module>.brmods     : "module_id \t module_path"   (one line)
//
// Build (LLVM 19.x):
//   clang++ -fPIC -shared -o libBranchTrace.so BranchTrace.cpp \
//       $(llvm-config --cxxflags --ldflags)
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

// Escape hatch to reproduce pre-select behaviour exactly. Branch site_ids are
// unaffected either way; this only suppresses the select stream and the
// .brselsites table.
static cl::opt<bool> NoSelect(
    "brtrace-no-select", cl::init(false),
    cl::desc("Do not instrument SelectInst conditions"));

// FNV-1a 32-bit over the module identifier -> stable module_id across builds.
static uint32_t moduleHash(StringRef s) {
  uint32_t h = 2166136261u;
  for (unsigned char c : s.bytes()) {
    h ^= c;
    h *= 16777619u;
  }
  return h;
}

// Return true iff a branch condition is a boolean expression built from
// floating-point compares. We walk back ONLY through i1-typed boolean logic
// (and/or/xor on i1) and i1-narrowing/widening casts -- i.e. predicate flow,
// not value flow. We deliberately do NOT recurse through select/phi or
// wider-integer data: those propagate VALUES that may merely depend on an
// fcmp, which is a different question from whether THIS branch is controlled
// by a float compare. Recursing through them mislabels an integer loop
// counter (whose phi can share a block with fcmp-derived selects) as
// FP-controlled -- the false positive seen on the `for (m=0; m<5; m++)`
// tolerance loop in verify().
//
// Coverage: fcmp->br, (fcmp && fcmp)->br, fcmp->zext->trunc->br. Any FP type
// (half/float/double/x86_fp80/fp128/...) yields an fcmp, so long-double
// branches are still caught.
//
// This same predicate is applied to a SelectInst's CONDITION operand, which is
// exactly the right question for a select ("is this choice FP-controlled?") and
// keeps the select class consistent with the branch class -- including the
// refusal to recurse through value flow, so an integer select whose arms happen
// to derive from an fcmp is still correctly excluded.
//
// Cycle-guarded (phis make the use-graph cyclic) and depth-capped.
static bool isFPControlled(Value *V, SmallPtrSetImpl<Value *> &Seen,
                           unsigned Depth = 0) {
  if (Depth > 16 || !Seen.insert(V).second)
    return false;
  if (isa<FCmpInst>(V))
    return true;
  // Only predicate (i1) values can carry FP-comparison control; stop as soon
  // as we leave i1, which is what prevents diving into integer loop counters.
  if (!V->getType()->isIntegerTy(1))
    return false;
  auto *U = dyn_cast<User>(V);
  if (!U)
    return false;
  if (isa<BinaryOperator>(U)) { // and/or/xor on i1
    for (Value *Op : U->operands())
      if (isFPControlled(Op, Seen, Depth + 1))
        return true;
  } else if (auto *CI = dyn_cast<CastInst>(U)) { // trunc iN->i1, zext i1->iN
    Value *Src = CI->getOperand(0);
    if (Src->getType()->isIntegerTy(1) || CI->getType()->isIntegerTy(1))
      if (isFPControlled(Src, Seen, Depth + 1))
        return true;
  } else if (auto *FI = dyn_cast<FreezeInst>(U)) { // freeze i1
    if (isFPControlled(FI->getOperand(0), Seen, Depth + 1))
      return true;
  }
  return false;
}

struct BranchTracePass : PassInfoMixin<BranchTracePass> {

  FunctionCallee getHook(Module &M) {
    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32 = Type::getInt32Ty(Ctx);
    // void __brtrace_log(i32 module_id, i32 site_id, i32 taken)
    FunctionType *FT = FunctionType::get(VoidTy, {I32, I32, I32}, false);
    return M.getOrInsertFunction("__brtrace_log", FT);
  }

  FunctionCallee getSelectHook(Module &M) {
    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32 = Type::getInt32Ty(Ctx);
    // void __brtrace_log_select(i32 module_id, i32 sel_id, i32 taken)
    FunctionType *FT = FunctionType::get(VoidTy, {I32, I32, I32}, false);
    return M.getOrInsertFunction("__brtrace_log_select", FT);
  }

  static std::string locString(const Instruction *I, const Function &F) {
    if (const DebugLoc &DL = I->getDebugLoc()) {
      if (DILocation *Loc = DL.get()) {
        return (Loc->getFilename() + ":" + Twine(Loc->getLine()) + "\t" +
                F.getName())
            .str();
      }
    }
    return ("<no-dbg>:0\t" + F.getName()).str();
  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    LLVMContext &Ctx = M.getContext();
    Type *I32 = Type::getInt32Ty(Ctx);
    FunctionCallee Hook = getHook(M);
    FunctionCallee SelHook = getSelectHook(M);

    std::string ModName = M.getModuleIdentifier();
    if (ModName.empty())
      ModName = "module";
    // Hash the BASENAME, not the full path, so the same source compiled from
    // different directories (e.g. sp_fp32/sp.c, sp_fp64/sp.c, sp_ld/sp.c)
    // gets the SAME module_id and the diff can align them. The full path is
    // still recorded in the side tables for readability.
    StringRef Base = StringRef(ModName);
    size_t Slash = Base.find_last_of("/\\");
    if (Slash != StringRef::npos)
      Base = Base.substr(Slash + 1);
    uint32_t ModId = moduleHash(Base);
    Constant *ModIdC = ConstantInt::get(I32, ModId);

    uint32_t SiteId = 0;
    std::string SiteTable;

    // Selects: separate id space, separate table. Never merge these counters
    // with SiteId -- doing so shifts every branch site_id after the first
    // select and invalidates every previously collected trace.
    uint32_t SelId = 0;
    std::string SelTable;

    bool Changed = false;

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      // --- Selects, BEFORE the terminator walk for this function. ---
      //
      // The ordering is not cosmetic. The switch lowering below synthesises
      // its own SelectInsts (the CreateSelect compare-chain). Collecting the
      // pre-existing selects into a vector first, and finishing with them
      // before any switch in this function is touched, makes it impossible for
      // the pass to instrument its own generated code. In -brtrace-fp-only
      // mode switches are skipped so no selects are synthesised, but the
      // non-fp-only path depends on this ordering. Do not fold this into the
      // terminator loop.
      if (!NoSelect) {
        SmallVector<SelectInst *, 32> Sels;
        for (BasicBlock &BB : F)
          for (Instruction &I : BB)
            if (auto *SelI = dyn_cast<SelectInst>(&I))
              Sels.push_back(SelI);

        for (SelectInst *SelI : Sels) {
          Value *Cond = SelI->getCondition();

          // A vector select has an <N x i1> condition: there is no single
          // outcome to log. Does not arise at -O0; guard regardless so an
          // -O2 build cannot produce a malformed zext.
          if (!Cond->getType()->isIntegerTy(1))
            continue;

          if (FPOnly) {
            SmallPtrSet<Value *, 16> Seen;
            if (!isFPControlled(Cond, Seen))
              continue;
          }

          IRBuilder<> B(SelI); // insert before the select
          Value *Taken = B.CreateZExtOrTrunc(Cond, I32);
          B.CreateCall(SelHook, {ModIdC, ConstantInt::get(I32, SelId), Taken});
          SelTable += (Twine(SelId) + "\t" + locString(SelI, F) + "\n").str();
          ++SelId;
          Changed = true;
        }
      }

      // --- Terminators. Unchanged from the pre-select version. ---
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
          SiteTable += (Twine(SiteId) + "\t" + locString(BI, F) + "\n").str();
          ++SiteId;
          Changed = true;
        } else if (kInstrumentSwitch && !FPOnly) {
          // A switch condition is always integer-controlled, so it is never
          // FP-controlled -- skip switches entirely in FP-only mode.
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
            SiteTable += (Twine(SiteId) + "\t" + locString(SI, F) + "\n").str();
            ++SiteId;
            Changed = true;
          }
        }
      }
    }

    // Emit side tables next to the module.
    if (!SiteTable.empty()) {
      std::error_code EC;
      {
        raw_fd_ostream OS(ModName + ".brsites", EC);
        if (!EC) {
          OS << "# module_id " << ModId << "  module " << ModName << "\n";
          OS << "# site_id\tfile:line\tfunction\n";
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

    // Select table is written independently of the branch table: a module can
    // legitimately have selects but no FP-controlled branches, or the reverse.
    if (!SelTable.empty()) {
      std::error_code EC3;
      raw_fd_ostream OS3(ModName + ".brselsites", EC3);
      if (!EC3) {
        OS3 << "# module_id " << ModId << "  module " << ModName << "\n";
        OS3 << "# sel_id\tfile:line\tfunction\n";
        OS3 << SelTable;
      }
    }

    errs() << "[BranchTrace] " << ModName << " (mod " << ModId
           << "): instrumented " << SiteId << " branch sites";
    if (!NoSelect)
      errs() << ", " << SelId << " select sites";
    errs() << (FPOnly ? " (fp-only)" : "") << "\n";
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