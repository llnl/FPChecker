//===-EFTSanitizer.h  - Interface ---------------------------------*- C++ -*-===//
//
//
//
//===----------------------------------------------------------------------===//
//
// This pass instruments floating point instructions by inserting llvm
// bitcode for error free transformations and calls to the runtime to
// perform shadow execution
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include <fstream>
#include <queue>
#include <set>

using namespace llvm;

namespace {
  struct EFTSanitizer : public ModulePass {
  
  public:
    EFTSanitizer() : ModulePass(ID) {}

    virtual bool runOnModule(Module &module);
    long countConstants(Function *F);
    void InvokeSetArgument (InvokeInst *CI, BasicBlock *BB, Function *F);
    void CallSetArgument (CallInst *CI, BasicBlock *BB, Function *F);
    void createInitMpfr(Value* BOGEP, Function *F, AllocaInst *Alloca, size_t index);
    void createInitAndSetMpfr(Value* BOGEP, Function *F, AllocaInst *Alloca, size_t index, Value *OP);
    void createInitAndSetP32(Value* BOGEP, Function *F, AllocaInst *Alloca, size_t index, Value *OP);
    void instrumentAllFunctions(std::string FN);
    void createMpfrAlloca(Function *F, int *instId);
    void callGetArgument(Function *F);
    AllocaInst *createAlloca(Function *F, size_t InsCount);
    void SetConstant(Function *F, Value *Op1, Value *BOGEP, Instruction *BO, int *instId);
    void createGEP(Function *F, int index, int *instId);
    void clearAlloca(Function *F, size_t InsCount);
    Instruction* getNextInstruction(Instruction *I, BasicBlock *BB);
    Instruction* getNextInstructionNotPhi(Instruction *I, BasicBlock *BB);
    void findInterestingFunctions(Function *F);
    bool handleOperand(Function *F, Instruction *I, Value* OP, Value **InstIdx1, Value **InstIdx2);
    bool handlePhiOperand(Value* OP, Value **InstIdx1, Value **InstIdx2);
    void handleStore(StoreInst *SI, BasicBlock *BB, Function *F);
    void handleStr(Value *OP, Value *EVal, Instruction *I, Value *BCToAddr,                 
            BasicBlock *BB, Function *F, ConstantInt* lineNumber);
    void handleNewPhi(Function *F);
    void handleFPTrunc(FPTruncInst *FPT, BasicBlock *BB, Function *F);
    void copyPhi(Instruction *I, Function *F);
    void handlePhi(PHINode *PN, BasicBlock *BB, Function *F);
    void handleSelect(SelectInst *SI, BasicBlock *BB, Function *F);
    void handleBinOp(BinaryOperator* BO, BasicBlock *BB, Function *F);
    void handleBinOpVec(BinaryOperator* BO, BasicBlock *BB, Function *F);
    void handleFNeg(UnaryOperator *UO, BasicBlock *BB, Function *F);
    void handleFcmp(FCmpInst *FCI, BasicBlock *BB, Function *F);
    void handleFPToSI(FPToSIInst *FI, BasicBlock *BB, Function *F);
    void handleFPToUI(FPToUIInst *FI, BasicBlock *BB, Function *F);
    void handleReturn(ReturnInst *RI, BasicBlock *BB, Function *F);
    bool checkIfBitcastFromFP(BitCastInst *BI);
    void handleLoad(LoadInst *LI, BasicBlock *BB, Function *F);
    void handleMathLibFunc(CallInst *CI, BasicBlock *BB, Function *F, std::string Name);
    void handleMemCpy(CallInst *CI, BasicBlock *BB, Function *F);
    void handleMemset(CallInst *CI, BasicBlock *BB, Function *F, std::string CallName);
    void handlePositLibFunc(CallInst *CI, BasicBlock *BB, Function *F, std::string Name);
		void handleCallInst (CallInst *CI, BasicBlock *BB, Function *F);
		void handleInvokeInst (InvokeInst *CI, BasicBlock *BB, Function *F);
    void TwoSum(Instruction *, Value *, Value *, Value *, Value *, Value *, Value *, Value*);
    void TwoSub(Instruction *, Value *, Value *, Value *, Value *, Value *, Value *, Value*);
    void TwoSubFNeg(Instruction *, Value *, Value *, Value *, Value *, Value *, Value *);
    void TwoMul(Instruction *, Value *, Value *, Value *, Value *, Value *, Value *, Value*);
    void TwoDiv(Instruction *, Value *, Value *, Value *, Value *, Value *, Value *, Value*);
    bool isListedFunction(StringRef FN, std::string FileName);
    void addFunctionsToList(std::string FN);
    bool isFloatType(Type *InsType);
    bool isFloat(Type *InsType);
    bool isDouble(Type *InsType);
    void handleMainRet(Instruction *I, Function *F);
    void handleFuncInit(Function *F);
    void handleFuncMainInit(Function *F, int);
    void handleInit(Module *M);
    void handleIns(Instruction *I, BasicBlock *BB, Function *F);
    long getTotalFPInst(Function *F);
    void storeShadowAddr(Value *srcVal, Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
            Function *F, ConstantInt* lineNumber);
    void storeShadowAddrBO(Value *srcVal, Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
            Function *F, ConstantInt* lineNumber);
    void storeShadowAddrCons(Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
            Function *F, ConstantInt* lineNumber);
    void storeShadowAddrArg(Value *srcVal, Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
            Function *F, ConstantInt* lineNumber);
    Value* loadShadowAddr(Instruction *I, Value *Addr, BasicBlock *BB,
            Function *F);
    ConstantInt* GetInstId(Function *F, Value* I);
    ConstantInt* GetLineNo(Function *F, Instruction* I);
    void handleError (CallInst *CI, BasicBlock *BB, Function *F);
    void handleError2 (InvokeInst *CI, BasicBlock *BB, Function *F);
    Value* getLoadOperandLHS(Value *I);
    Value* getLoadOperandRHS(Value *I);
    void resetMetadataConstants(Function *F);
    StructType *MPFRTy;
    StructType *SMemEntry;
    GlobalVariable *Shadow_Mem;
    GlobalVariable *Shadow_Stack;
    GlobalVariable *TimeStamp;
    GlobalVariable *SlotTimeStamp;
    GlobalVariable *MStackTop;
    GlobalVariable *InstMap;
    GlobalVariable *MapIns;
    Type *SMemPtrTy;
    Type *MPtrTy;
    Type *RealPtr;
    StructType* Real;
    Value* TMSCounter;
    Value* ShadowSL;
    Value* LoadSMem;
    std::map<BasicBlock *, Instruction*> TMSIdxMap;
    std::set<StringRef> LibFuncList;
    std::map<Value*, Value*> ConsMap;
    std::map<Instruction*, Value*> ConsInsMap;
    std::map<Value*, Value*> GEPMap;
    std::map<Value*, Value*> GEPMapSlot;
    std::map<Value*, Value*> SlotTSMap;
    std::map<Value*, Value*> SlotUniqueTSMap;
    std::map<ConstantFP*, Value*> ConstantMap;
    //map new instruction with old esp. for select and phi
    std::map<Instruction*, Instruction*> RegIdMap;
    std::map<Instruction*, Instruction*> NewPhiMap;
    //track unique index for instructions
    std::map<Instruction*, Instruction*> MInsMap;
    std::map<Instruction*, std::pair<Instruction*, Instruction*>> MInsMapPair;
    std::map<Instruction*, std::pair<Value*, Value*>> GEPMapPair;
    std::map<Argument*, Instruction*> MArgMap;
    //Arguments can not be instruction, so we need a seperate map to hold indexes for arguments
    std::map<Argument*, size_t> ArgMap;
    std::map<Argument*, size_t> ArgInstIdMap;
    std::map<Value*, size_t> ConsInstIdMap;
    std::map<Function*, size_t> FuncTotalArg;
    std::map<Function*, size_t> FuncTotalIns;
    SmallVector<Function *, 8> FuncGetArgList;
    SmallVector<Instruction *, 8> StoreList;
    SmallVector<Instruction *, 8> FCmpList;
    SmallVector<CallInst *, 8> MemcpyList;
    SmallVector<CallInst *, 8> MemcpyNList;
    SmallVector<Instruction *, 8> AllInstList;
    //list of all functions need to be instrumented
    SmallVector<Function*, 8> AllFuncList;
    static char ID; // Pass identification
    long InsCount = 0;
    std::function<const TargetLibraryInfo &(Function &F)> GetTLI;

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
      AU.addRequired<TargetLibraryInfoWrapperPass>();
    }
  private:
    FunctionCallee Func;
    FunctionCallee LoadCall;
    FunctionCallee ComputeReal;
    FunctionCallee FuncExit;
    FunctionCallee CheckBranch;
    FunctionCallee FuncInit;
    FunctionCallee Finish;
    FunctionCallee HandleFunc;
    FunctionCallee SetRealTemp;
    FunctionCallee AddFunArg;
  };
}

