

#ifndef SRC_INSTRUMENTATION_H_
#define SRC_INSTRUMENTATION_H_

#include "CommonTypes.h"
#include "llvm/IR/IRBuilder.h"
#include <string>

using namespace llvm;

namespace CPUAnalysis
{

  class CPUFPInstrumentation
  {
  private:
    Module *mod;

    Function *fp32_check_function;
    Function *fp64_check_function;
    // Function *fpc_init_htable;
    Function *fpc_init;
    Function *fpc_init_args;
    Function *fpc_print_locations;
    Function *fpc_fp32_store_inst;
    Function *fpc_fp32_load_inst;
    Function *fpc_fp32_calculate_function;
    // Function *fpc_fp64_calculate_function;

    bool codeIsAnnotated = false;

    // maximum number for a code line
    // int maxNumLocations = 0;

    /* Host */
    // Function *print_at_main = nullptr;

    // IRBuilder<> createBuilderAfter(Instruction *inst);
    // IRBuilder<> createBuilderBefore(Instruction *inst);
    void setFakeDebugLocation(Instruction *old_inst, Instruction *new_inst, Function *f);
    Instruction *firstInstrution();
    bool selectedBasedOnCondition(Instruction *inst, Function *f, Instruction **select_inst, Value **condition, int *inv);

    // GlobalVariable* generateIntArrayGlobalVariable(ArrayType *arrType);
    // void createReadFunctionForGlobalArray(GlobalVariable *arr, ArrayType *arrType, std::string funcName);
    // void createWriteFunctionForGlobalArray(GlobalVariable *arr, ArrayType *arrType, std::string funcName);

  public:
    CPUFPInstrumentation(Module *M);
    void instrumentFunction(Function *f, long int *c);
    void instrumentFunctionErrorAnalysis(Function *f);
    void instrumentMainFunction(Function *f);
    void setCodeIsAnnotated(bool b);
    // void generateCodeForInterruption();
    // void instrumentErrorArray();
    // void instrumentEndOfKernel(Function *f);
    // InstSet finalInstrutions(Function *f);

    /* Helper functions */
    // static bool isUnwantedFunction(Function *f);
    static bool isFPOperation(const Instruction *inst);
    static bool isDoubleFPOperation(const Instruction *inst);
    static bool isSingleFPOperation(const Instruction *inst);
    // static bool isMainFunction(Function *f);
    // bool errorsDontAbortMode();
    static bool isCmpEqual(const Instruction *inst);
    static bool isFMAOperation(const Instruction *inst);
    static bool basicBlockisAnnotated(const BasicBlock *bb);
    static bool functionisAnnotated(const Function *f, const char *annotation);
  };

}

#endif /* SRC_INSTRUMENTATION_H_ */
