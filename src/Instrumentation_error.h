

#ifndef SRC_INSTRUMENTATION_H_
#define SRC_INSTRUMENTATION_H_

#include "CommonTypes.h"
#include "llvm/IR/IRBuilder.h"
#include <string>

using namespace llvm;

namespace CPUAnalysis
{

  class CPUFPInstrumentation_error
  {
  private:
    Module *mod;
    std::string module_filename;

    Function *fpc_init;
    Function *fpc_init_args;
    Function *fpc_print_locations;
    Function *fpc_fp32_store_inst;
    Function *fpc_fp32_load_inst;
    Function *fpc_fp32_calculate_function;
    Function *fpc_fp32_phi_function;
    Function *fpc_fp32_branch_function;
    Function *fp32_memcpy_function;

    void setFakeDebugLocation(Instruction *old_inst, Instruction *new_inst, Function *f);
    Instruction *firstInstrution();
    bool selectedBasedOnCondition(Instruction *inst, Function *f, Instruction **select_inst, Value **condition, int *inv);

  public:
    CPUFPInstrumentation_error(Module *M);
    void instrumentFunctionErrorAnalysis(Function *f, long int *insrtrumented_instructions);
    void instrumentMainFunction(Function *f);

    /* Helper functions */
    static bool isFPOperation(const Instruction *inst);
    static bool isFPOperationWithError(const Instruction *inst);
    static bool isDoubleFPOperation(const Instruction *inst);
    static bool isSingleFPOperation(const Instruction *inst);
    static bool isCmpEqual(const Instruction *inst);
    static bool isFMAOperation(const Instruction *inst);
    static bool functionisAnnotated(const Function *f, const char *annotation);
    static bool functionCallsFunctionWithFloatingPointValues(const Function *f);
  };

}

#endif /* SRC_INSTRUMENTATION_H_ */
