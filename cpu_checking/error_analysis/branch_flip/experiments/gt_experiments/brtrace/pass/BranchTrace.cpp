//===- BranchTrace.cpp - Log the direction of every conditional branch ---===//
//
// New-PM module pass. For each conditional BranchInst (and optionally each
// SwitchInst) it inserts, immediately before the terminator, a call to:
//
//     void __brtrace_log(uint32_t site_id, int32_t taken);
//
// It ALSO instruments FP-controlled SelectInst conditions, via a separate hook:
//
//     void __brtrace_log_select(uint32_t sel_id, int32_t taken);
//
// Selects matter because FPChecker fires at fcmp->select sites, and a
// terminator-only pass cannot see them (a select is not a terminator). Without
// this, any FPChecker detection at a ternary / std::max-at-O1+ / fmin-style
// site has no counterpart in the trace and cannot be adjudicated.
//
// Select sites use a SEPARATE id space, a SEPARATE runtime hook, and a
// SEPARATE output stream. That is deliberate: branch site_ids are therefore
// unchanged by this feature, so traces and censuses collected before it was
// added remain valid and do not need re-running.
//
// site_id is a deterministic, per-branch identifier assigned during a stable
// walk of the module (function order as in the IR, block order, instruction
// order). Because the walk is deterministic and driven by source structure,
// compiling the fp32 and fp64 variants of the SAME source with this pass
// assigns the SAME site_id to corresponding branches -- which is what lets the
// diff tool line the two traces up. sel_id is assigned by the same rule.
//
// `taken`:
//   - conditional br : 0 or 1 (the i1 condition, zero-extended)
//   - switch         : the matched case's successor index (0 = default)
//   - select         : 0 or 1 (the i1 condition, zero-extended)
//
// Companion side-tables (id -> "file:line  funcname") are emitted next to the
// module as <module>.brsites (branches) and <module>.brselsites (selects) so
// flips can be mapped back to source.
//
// Build (LLVM 19.x, New PM plugin):
//   clang++ -fPIC -shared -o libBranchTrace.so BranchTrace.cpp \
//       $(llvm-config --cxxflags --ldflags)
//
// Use:
//   clang -O0 -g -fpass-plugin=./libBranchTrace.so foo.c -c -o foo.o
//   (then link with brtrace_runtime.o)
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <system_error>

using namespace llvm;

namespace {

// Whether to also instrument switch instructions.
static bool kInstrumentSwitch = true;

// Whether to instrument SelectInst conditions at all.
static bool kInstrumentSelect = true;

// If true, only instrument selects whose condition is derived from an fcmp.
// This is what makes the select class match FPChecker's: FPChecker fires at
// fcmp->select, not at integer ternaries. Setting this false also picks up
// integer selects, which inflates the stream with events no precision study
// cares about.
static bool kSelectFPOnly = true;

struct BranchTracePass : PassInfoMixin<BranchTracePass> {

  // Look up (or declare) the runtime hook:
  //   void __brtrace_log(i32 site_id, i32 taken)
  FunctionCallee getHook(Module &M) {
    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32 = Type::getInt32Ty(Ctx);
    FunctionType *FT = FunctionType::get(VoidTy, {I32, I32}, /*vararg=*/false);
    return M.getOrInsertFunction("__brtrace_log", FT);
  }

  // Look up (or declare) the select runtime hook:
  //   void __brtrace_log_select(i32 sel_id, i32 taken)
  FunctionCallee getSelectHook(Module &M) {
    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32 = Type::getInt32Ty(Ctx);
    FunctionType *FT = FunctionType::get(VoidTy, {I32, I32}, /*vararg=*/false);
    return M.getOrInsertFunction("__brtrace_log_select", FT);
  }

  // Is this i1 value derived from a floating-point comparison?
  //
  // A direct `fcmp` covers most -O0 cases, but not all: a _Bool temporary
  // round-trips through an alloca and arrives as trunc(load(...)), and
  // combined predicates arrive through and/or. Bounded backward walk so a
  // pathological chain cannot blow up compile time.
  static bool isFPControlled(Value *V, unsigned Depth = 0) {
    if (!V || Depth > 8)
      return false;
    if (isa<FCmpInst>(V))
      return true;

    auto *I = dyn_cast<Instruction>(V);
    if (!I)
      return false;

    switch (I->getOpcode()) {
    // Value-preserving reshaping of the predicate.
    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt:
    case Instruction::BitCast:
    case Instruction::Freeze:
      return isFPControlled(I->getOperand(0), Depth + 1);

    // Combined predicates: FP-controlled if EITHER side is.
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
      return isFPControlled(I->getOperand(0), Depth + 1) ||
             isFPControlled(I->getOperand(1), Depth + 1);

    case Instruction::Select:
      return isFPControlled(I->getOperand(1), Depth + 1) ||
             isFPControlled(I->getOperand(2), Depth + 1);

    case Instruction::PHI: {
      auto *PN = cast<PHINode>(I);
      for (Value *In : PN->incoming_values())
        if (isFPControlled(In, Depth + 1))
          return true;
      return false;
    }

    // -O0 _Bool spill: load from an alloca that some fcmp was stored into.
    case Instruction::Load: {
      Value *Ptr = I->getOperand(0);
      if (auto *AI = dyn_cast<AllocaInst>(Ptr->stripPointerCasts()))
        for (User *U : AI->users())
          if (auto *SI = dyn_cast<StoreInst>(U))
            if (isFPControlled(SI->getValueOperand(), Depth + 1))
              return true;
      return false;
    }

    default:
      return false;
    }
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

    uint32_t SiteId = 0;
    std::string SiteTable; // "id\tfile:line\tfunc\n"
    bool Changed = false;

    // Select sites live in their own id space with their own hook. Keeping
    // them separate is what makes this change non-breaking: branch site_ids
    // are identical to what they were before selects were instrumented.
    FunctionCallee SelHook = getSelectHook(M);
    uint32_t SelId = 0;
    std::string SelTable;

    // Deterministic walk: functions in module order, blocks in layout order,
    // instructions in order. Do NOT reorder -- both fp32/fp64 builds must see
    // the identical sequence so IDs line up.
    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      // --- Selects, BEFORE the terminator loop for this function. ---
      //
      // Order matters and is not cosmetic. The switch lowering below
      // synthesises its own SelectInsts. Collecting the pre-existing selects
      // into a vector first, and finishing with them before any switch is
      // touched, makes it impossible for the pass to instrument its own
      // generated code. Do not merge this into the loop below.
      if (kInstrumentSelect) {
        SmallVector<SelectInst *, 32> Sels;
        for (BasicBlock &BB : F)
          for (Instruction &I : BB)
            if (auto *SelI = dyn_cast<SelectInst>(&I))
              Sels.push_back(SelI);

        for (SelectInst *SelI : Sels) {
          Value *Cond = SelI->getCondition();

          // A vector select has an <N x i1> condition: there is no single
          // outcome to log. Does not arise at -O0; guard anyway.
          if (!Cond->getType()->isIntegerTy(1))
            continue;

          if (kSelectFPOnly && !isFPControlled(Cond))
            continue;

          IRBuilder<> B(SelI); // insert before the select
          Value *Taken = B.CreateZExtOrTrunc(Cond, I32);
          Value *Id = ConstantInt::get(I32, SelId);
          B.CreateCall(SelHook, {Id, Taken});

          SelTable += (Twine(SelId) + "\t" + locString(SelI, F) + "\n").str();
          ++SelId;
          Changed = true;
        }
      }

      // --- Terminators. ---
      for (BasicBlock &BB : F) {
        Instruction *Term = BB.getTerminator();
        if (!Term)
          continue;

        if (auto *BI = dyn_cast<BranchInst>(Term)) {
          if (!BI->isConditional())
            continue;

          IRBuilder<> B(BI); // insert before the branch
          Value *Cond = BI->getCondition();
          // Condition is i1; zext to i32 for the hook.
          Value *Taken = B.CreateZExtOrTrunc(Cond, I32);
          Value *Id = ConstantInt::get(I32, SiteId);
          B.CreateCall(Hook, {Id, Taken});

          SiteTable +=
              (Twine(SiteId) + "\t" + locString(BI, F) + "\n").str();
          ++SiteId;
          Changed = true;
        } else if (kInstrumentSwitch) {
          if (auto *SI = dyn_cast<SwitchInst>(Term)) {
            // Compute which successor index the condition selects, at runtime,
            // as a small chain of compares. Successor 0 is the default.
            IRBuilder<> B(SI);
            Value *CondV = SI->getCondition();
            Value *Sel = ConstantInt::get(I32, 0); // default index
            unsigned idx = 1;
            for (auto Case : SI->cases()) {
              Value *Eq = B.CreateICmpEQ(CondV, Case.getCaseValue());
              Sel = B.CreateSelect(Eq, ConstantInt::get(I32, idx), Sel);
              ++idx;
            }
            Value *Id = ConstantInt::get(I32, SiteId);
            B.CreateCall(Hook, {Id, Sel});

            SiteTable +=
                (Twine(SiteId) + "\t" + locString(SI, F) + "\n").str();
            ++SiteId;
            Changed = true;
          }
        }
      }
    }

    // Emit the site table next to the module. Use the module identifier as a
    // base; if empty, fall back to a fixed name.
    if (!SiteTable.empty()) {
      std::string Base = M.getModuleIdentifier();
      if (Base.empty())
        Base = "module";
      std::string Path = Base + ".brsites";
      std::error_code EC;
      raw_fd_ostream OS(Path, EC);
      if (!EC) {
        OS << "# site_id\tfile:line\tfunction\n";
        OS << SiteTable;
      } else {
        errs() << "[BranchTrace] could not write " << Path << ": "
               << EC.message() << "\n";
      }
    }

    // Emit the select side table alongside it.
    if (!SelTable.empty()) {
      std::string Base = M.getModuleIdentifier();
      if (Base.empty())
        Base = "module";
      std::string Path = Base + ".brselsites";
      std::error_code EC;
      raw_fd_ostream OS(Path, EC);
      if (!EC) {
        OS << "# sel_id\tfile:line\tfunction\n";
        OS << SelTable;
      } else {
        errs() << "[BranchTrace] could not write " << Path << ": "
               << EC.message() << "\n";
      }
    }

    errs() << "[BranchTrace] instrumented " << SiteId << " branch sites, "
           << SelId << " select sites"
           << (kSelectFPOnly ? " (selects: fp-only)" : "") << "\n";
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
// Plugin registration (New PM)
//===----------------------------------------------------------------------===//
llvm::PassPluginLibraryInfo getBranchTracePluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "BranchTrace", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // Run automatically via -fpass-plugin at the very start of the
            // pipeline so branch structure is still source-faithful.
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                  MPM.addPass(BranchTracePass());
                });
            // Also allow explicit `opt -passes=branch-trace`.
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