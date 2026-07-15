
#include "CodeMatching.h"
#include "Instrumentation_error.h"
#include "Logging.h"
#include "Utility.h"

#include "llvm/CodeGen/RegAllocRegistry.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

#include <fstream>
#include <iostream>
#include <set>
#include <string>

using namespace llvm;

namespace CPUAnalysis
{

  bool analyzeProgramModule(Module &M)
  {
    Module *m = &M;
    CPUFPInstrumentation_error *fpInstrumentation = new CPUFPInstrumentation_error(m);
    long int instrumented = 0;

#ifdef FPC_DEBUG
    std::string out = "Running Module pass on: " + m->getName().str();
    CUDAAnalysis::Logging::info(out.c_str());
#endif

    auto fileName = CUDAAnalysis::getFileNameFromModule(m);
    // errs() << "\n++++++ Module file name: " << fileName << "\n";

    for (auto f = M.begin(), e = M.end(); f != e; ++f)
    {
      // Discard function declarations
      if (f->isDeclaration())
        continue;

      Function *F = &(*f);

      if (CUDAAnalysis::CodeMatching::isUnwantedFunction(F))
        continue;

#ifdef FPC_DEBUG
      std::string fname = "Instrumenting function: " + F->getName().str();
      CUDAAnalysis::Logging::info(fname.c_str());
#endif
      long int c = 0;
      fpInstrumentation->instrumentFunctionErrorAnalysis(F, &c);
      instrumented += c;

      if (CUDAAnalysis::CodeMatching::isMainFunction(F))
      {
#ifdef FPC_DEBUG
        CUDAAnalysis::Logging::info("main() found");
#endif
        fpInstrumentation->instrumentMainFunction(F);
      }
    }

    std::string out_tmp = "Instrumented " + std::to_string(instrumented) + " @ " +
                          m->getName().str();
    CUDAAnalysis::Logging::info(out_tmp.c_str());

    // This emulates a failure in the pass
    if (getenv("FPC_INJECT_FAULT") != NULL)
      exit(-1);

    delete fpInstrumentation;
    return false;
  }

  // LLVM pass that uses the new pass manager.
  struct CPUKernelAnalysis : public PassInfoMixin<CPUKernelAnalysis>
  {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
    {
      //llvm::outs() << "Running ModulePass on module: " << M.getName() << "\n";
      analyzeProgramModule(M);
      return PreservedAnalyses::none();
    }

    static bool isRequired() { return true; }
  };

  // Clang's -O0 IR is marked optnone, so regular canonicalization passes skip
  // it. Error tracking needs simple stack-local branch diamonds promoted to SSA
  // so select/phi instrumentation can model shadow-side control flow.
  struct CPUPrepareO0ForErrorAnalysis : public PassInfoMixin<CPUPrepareO0ForErrorAnalysis>
  {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
    {
      bool changed = false;
      for (Function &F : M)
      {
        if (F.isDeclaration())
          continue;
        if (CUDAAnalysis::CodeMatching::isUnwantedFunction(&F))
          continue;
        if (F.hasFnAttribute(Attribute::OptimizeNone))
        {
          F.removeFnAttr(Attribute::OptimizeNone);
          changed = true;
        }
      }
      return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }

    static bool isRequired() { return true; }
  };

  PassPluginLibraryInfo getMyModulePassPluginInfo()
  {
    const auto callback = [](PassBuilder &PB)
    {
      PB.registerOptimizerLastEPCallback(
          [&](ModulePassManager &MPM, OptimizationLevel opt)
          {
#ifdef FPC_DEBUG
            std::string fname =
                "Optimzation Level: " + std::to_string(opt.getSpeedupLevel());
            CUDAAnalysis::Logging::info(fname.c_str());
#endif
            if (opt.getSpeedupLevel() == 0)
            {
              MPM.addPass(CPUPrepareO0ForErrorAnalysis());
              MPM.addPass(createModuleToFunctionPassAdaptor(PromotePass()));
              MPM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass()));
            }
            // MPM.addPass(createModuleToFunctionPassAdaptor(CPUKernelAnalysis()));
            MPM.addPass(CPUKernelAnalysis());
            return true;
          });
    };

    return {LLVM_PLUGIN_API_VERSION, "CPUKernelAnalysis", "0.6", callback};
  };

  extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo()
  {
    return getMyModulePassPluginInfo();
  }

} // namespace CPUAnalysis
