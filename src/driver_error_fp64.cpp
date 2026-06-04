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

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

using namespace llvm;

namespace CPUAnalysisFP64
{

bool analyzeProgramModule(Module &M)
{
  Module *m = &M;

  CPUFPInstrumentation_error_fp64 *fpInstrumentation =
      new CPUFPInstrumentation_error_fp64(m);

  long int instrumented = 0;

#ifdef FPC_DEBUG
  std::string out = "Running FP64 Module pass on: " + m->getName().str();
  CUDAAnalysis::Logging::info(out.c_str());
#endif

  auto fileName = CUDAAnalysis::getFileNameFromModule(m);
  (void)fileName;

  for (auto f = M.begin(), e = M.end(); f != e; ++f)
  {
    // Discard function declarations.
    if (f->isDeclaration())
      continue;

    Function *F = &(*f);

    if (CUDAAnalysis::CodeMatching::isUnwantedFunction(F))
      continue;

#ifdef FPC_DEBUG
    std::string fname = "Instrumenting FP64 function: " + F->getName().str();
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

  std::string out_tmp = "Instrumented FP64 " + std::to_string(instrumented) +
                        " @ " + m->getName().str();
  CUDAAnalysis::Logging::info(out_tmp.c_str());

  // This emulates a failure in the pass.
  if (getenv("FPC_INJECT_FAULT") != NULL)
    exit(-1);

  delete fpInstrumentation;
  return false;
}

// LLVM pass that uses the new pass manager.
struct CPUKernelAnalysisFP64 : public PassInfoMixin<CPUKernelAnalysisFP64>
{
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
  {
    analyzeProgramModule(M);
    return PreservedAnalyses::none();
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
              "FP64 Optimization Level: " + std::to_string(opt.getSpeedupLevel());
          CUDAAnalysis::Logging::info(fname.c_str());
#endif
          MPM.addPass(CPUKernelAnalysisFP64());
          return true;
        });
  };

  return {LLVM_PLUGIN_API_VERSION, "CPUKernelAnalysisFP64", "0.7", callback};
}

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo()
{
  return getMyModulePassPluginInfo();
}

} // namespace CPUAnalysisFP64
