#include "EFTSanitizer.h"
#include "llvm/IR/CallSite.h"  
#include "llvm/IR/ConstantFolder.h"
#include "llvm/ADT/SCCIterator.h" 
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Support/CommandLine.h"
#include <set>

#define DEBUG 0
//#define SHADOW_STACK_SIZE 6800000
/*As long as NUM_ENTRIES >= 3, we do not need to check if metadata is
 * overwritten to compute the error. To compute the error correctly we only need 
 * metadata for all the operands of an instrunction available. 
 */
#define NUM_ENTRIES 6400

//#define NUM_ENTRIES 4
#define SHADOW_STACK_SIZE NUM_ENTRIES * 24 // 4 entries of sizeof(smem_entry)
#define SHADOW_STACK_SIZE_T NUM_ENTRIES * 56 // 4 entries of sizeof(smem_entry)
#define HASH_SIZE 64 * 1024 * 1024
#define SHADOW_MEM_SIZE HASH_SIZE * 24 // 4 entries of sizeof(smem_entry)
#define SHADOW_MEM_SIZE_T HASH_SIZE * 56 // 4 entries of sizeof(smem_entry)

//By dafault rounding errors and FP exceptions are detected for function return values.
//Enable rounding error detection for every FP instruction
static cl::opt<bool> ClDetectAllRoundingErrors(
    "eftsan-detect-all-rounding-errors",
    cl::desc("use to detect rounding error for every fp instruction"), cl::Hidden,
    cl::init(false));

//Enable FP exeception detection for every FP instruction
static cl::opt<bool> ClDetectExceptions(
    "eftsan-detect-all-exceptions",
    cl::desc("use to detect exceptions for every FP instruction"), cl::Hidden,
    cl::init(false));

//Enable debugging support
static cl::opt<bool> ClTracing(
    "eftsan-enable-debugging",
    cl::desc("use to debug by generating DAG of instructions"), cl::Hidden,
    cl::init(false));

void EFTSanitizer::addFunctionsToList(std::string FN) {
  std::ofstream myfile;
  myfile.open("functions.txt", std::ios::out|std::ios::app);
  if(isListedFunction(FN, "forbid.txt")) return;
  if (myfile.is_open()){
    myfile <<FN;
    myfile << "\n";
    myfile.close();
  }
}
//check name of the function and check if it is in list of functions given by 
//developer and return true else false.
bool EFTSanitizer::isListedFunction(StringRef FN, std::string FileName) {
  std::ifstream infile(FileName);
  std::string line;
  while (std::getline(infile, line)) {
    if (FN.compare(line) == 0){
      return true;
    }
  }
  return false;
}

bool EFTSanitizer::isFloatType(Type *InsType){
  if(InsType->getTypeID() == Type::DoubleTyID ||
      InsType->getTypeID() == Type::FloatTyID)
    return true;
  return false;
}

bool EFTSanitizer::isFloat(Type *InsType){
  if(InsType->getTypeID() == Type::FloatTyID)
    return true;
  return false;
}
bool EFTSanitizer::isDouble(Type *InsType){
  if(InsType->getTypeID() == Type::DoubleTyID)
    return true;
  return false;
}

ConstantInt* EFTSanitizer::GetLineNo(Function *F, Instruction* I) {
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }
  Module *M = F->getParent();
  IntegerType* Int64Ty = Type::getInt64Ty(M->getContext());
  ConstantInt *lineNumber = ConstantInt::get(Int64Ty, lineNum);
  return lineNumber;
}

ConstantInt* EFTSanitizer::GetInstId(Function *F, Value* I) {
  Module *M = F->getParent();
  if(isa<FPTruncInst>(I) || isa<FPExtInst>(I)){
    I = dyn_cast<Instruction>(I)->getOperand(0);
  }
  if(isa<ConstantFP>(I)){
    return ConstantInt::get(Type::getInt64Ty(M->getContext()), 
        ConsInstIdMap.at(I));
  }
  else if(isa<Argument>(I)){
    return ConstantInt::get(Type::getInt64Ty(M->getContext()), 
        ArgInstIdMap.at(dyn_cast<Argument>(I)));
  }
  MDNode* uniqueIdMDNode = dyn_cast<Instruction>(I)->getMetadata("inst_id");
  if (uniqueIdMDNode == NULL) {
    return ConstantInt::get(Type::getInt64Ty(M->getContext()), 0);
    //    exit(1);
  }

  Metadata* uniqueIdMetadata = uniqueIdMDNode->getOperand(0).get();
  ConstantAsMetadata* uniqueIdMD = dyn_cast<ConstantAsMetadata>(uniqueIdMetadata);
  Constant* uniqueIdConstant = uniqueIdMD->getValue();
  return dyn_cast<ConstantInt>(uniqueIdConstant);
}

void EFTSanitizer::SetConstant(Function *F, Value *Op1, Value *BOGEP, Instruction *BO, int *instId){
  Function::iterator Fit = F->begin();
  Module *M = F->getParent();
  BasicBlock &BB = *Fit; 
  BasicBlock::iterator BBit = BB.begin();
  Instruction *I = &*BBit;
  IRBuilder<> IRBI(BO);

  Type* VoidTy = Type::getVoidTy(M->getContext());
  IntegerType* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  ConstantInt* insId = ConstantInt::get(Type::getInt64Ty(M->getContext()), *instId);
  if(ConsInstIdMap.count(Op1) != 0){
    return;
  }
  ConsInstIdMap.insert(std::pair<Value*, int>(Op1, *instId));

  //store err
  Value *ValGep = IRBI.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep_set_c");

  //Set error to 0 for constant
  Value *Err = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);

  Value *StoreIns = IRBI.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));


  //Set computed
  if(isFloat(Op1->getType())){
    Op1 = IRBI.CreateFPExt(Op1, DoubleTy, "my_op");
  }
  ValGep = IRBI.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep_set_c");

  Value *StoreIns1 = IRBI.CreateStore(Op1, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns1));

  //store timestamp
  Value *GTimeStamp = IRBI.CreateLoad(IRBI.getInt64Ty(), TimeStamp, "my_ts");
  Value *Add = IRBI.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRBI.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRBI.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep_set_c");
  Value *StoreIns7 = IRBI.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));

  Value *InstMapGep = IRBI.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep_set_c");
  IRBI.CreateStore(Add, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRBI.CreateCall(SetRealTemp, {insId, Add});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 0); //Constant
    Value *OpCGep = IRBI.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep_set_c");

    Value *StoreIns3 = IRBI.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    ConstantInt *lineNo = GetInstId(F, I);
    Value *InstIdGep = IRBI.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep_set_c");
    //Value *StoreIns4 = IRBI.CreateStore(lineNumber, InstIdGep, "my_store");
    Value *StoreIns4 = IRBI.CreateStore(insId, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));


    //store lhs
    Value *Op1Lhs = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstLHSGep = IRBI.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep_set_c");
    Value *StoreIns5 = IRBI.CreateStore(Op1Lhs, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *Op2Rhs = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstRHSGep = IRBI.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep_set_c");
    Value *StoreIns6 = IRBI.CreateStore(Op2Rhs, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));
  }
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_set_constant", VoidTy, MPtrTy, Int64Ty, Int64Ty);
    IRBI.CreateCall(SetRealTemp, {BOGEP, Add, insId});
  }
}

void EFTSanitizer::createGEP(Function *F, int index, int *instId){
  Function::iterator Fit = F->begin();
  Module *M = F->getParent();
  BasicBlock &BB = *Fit; 
  BasicBlock::iterator BBit = BB.begin();
  Instruction *First = &*BBit;

  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());
  IntegerType* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());

  IRBuilder<> IRB(First);

  ShadowSL = IRB.CreateLoad(MPtrTy, Shadow_Stack, "my_load");
  Value *StackTop = IRB.CreateLoad(IRB.getInt64Ty(), MStackTop, "my_load_stack_top");  
  LoadSMem = IRB.CreateLoad(MPtrTy, Shadow_Mem, "my_load");

  Type* VoidTy = Type::getVoidTy(M->getContext());
  Constant *Num_Entries = ConstantInt::get(Int64Ty, NUM_ENTRIES);
  Value *Add = IRB.CreateAdd(StackTop, ConstantInt::get(Int64Ty, index), "my_stack_top_incr");
  Add = IRB.CreateURem(Add, Num_Entries);
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)){
        switch(BO->getOpcode()) { 
          case Instruction::FAdd:
          case Instruction::FSub:
          case Instruction::FMul:
          case Instruction::FDiv:
          case Instruction::FRem:
            if(BO->getType()->isVectorTy()){ 
              /*
              Value *Indices1[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                ConstantInt::get(Type::getInt32Ty(M->getContext()), index)};
              Value *BOGEP1 = IRB.CreateGEP(ShadowSL, Indices1);
              index++;

              Value *Indices2[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                ConstantInt::get(Type::getInt32Ty(M->getContext()), index)};
              Value *BOGEP2 = IRB.CreateGEP(ShadowSL, Indices2);
              GEPMapPair.insert(std::pair<Instruction*, std::pair<Value*, Value*>>(&I, 
                    std::make_pair(BOGEP1, BOGEP2)));
              index++;
              */
            }
            else
            {
              Value *Op1 = BO->getOperand(0);
              Value *Op2 = BO->getOperand(1);

              if(isa<ConstantFP>(Op1)){
                Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
                Add = IRB.CreateURem(Add, Num_Entries);
                Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
                GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op1), BOGEP));

                ConsMap.insert(std::pair<Value*, Value*>(Op1, BOGEP));
                *instId = *instId + 1;
                SetConstant(F, Op1, BOGEP, First, instId);
              }
              if(isa<ConstantFP>(Op2)){
                Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
                Add = IRB.CreateURem(Add, Num_Entries);
                Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
                GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op2), BOGEP));

                ConsMap.insert(std::pair<Value*, Value*>(Op2, BOGEP));
                *instId = *instId + 1;
                SetConstant(F, Op2, BOGEP, First, instId);
                index++;
              }
            }
        }
      }
      /*
      else if (StoreInst *UI = dyn_cast<StoreInst>(&I)){
        Value *OP = UI->getOperand(0);
        if(isa<ConstantFP>(OP)){
          Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
          Add = IRB.CreateURem(Add, Num_Entries);
          Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
          GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(UI), BOGEP));

          ConsMap.insert(std::pair<Value*, Value*>(UI, BOGEP));
          //Set computed
          *instId = *instId + 1;
          ConsInstIdMap.insert(std::pair<Value*, int>(OP, *instId));
        }
      }
      */
      else if (UnaryOperator *UO = dyn_cast<UnaryOperator>(&I)) {
        if (GEPMap.count(&I) != 0) {
          continue;
        }
        switch (UO->getOpcode()) {
          case Instruction::FNeg: {
            }
        }
      }
      else if (SIToFPInst *UI = dyn_cast<SIToFPInst>(&I)){
        Instruction *Next = getNextInstruction(UI, &BB);
        IRBuilder<> IRBI(Next);
        Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
        Add = IRB.CreateURem(Add, Num_Entries);
        Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
        GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(UI), BOGEP));

        ConsMap.insert(std::pair<Value*, Value*>(UI, BOGEP));
        //Set computed
        Value *OP = dyn_cast<Value>(UI);
        *instId = *instId + 1;
        SetConstant(F, OP, BOGEP, Next, instId);
        index++;
      }
      else if (BitCastInst *UI = dyn_cast<BitCastInst>(&I)){
        if(isFloatType(UI->getType())){
          Instruction *Next = getNextInstruction(UI, &BB);
          IRBuilder<> IRBI(Next);
          Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
          Add = IRB.CreateURem(Add, Num_Entries);
          Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
          GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(UI), BOGEP));

          ConsMap.insert(std::pair<Value*, Value*>(UI, BOGEP));

          //Set computed
          Value *OP = dyn_cast<Value>(UI);
          *instId = *instId + 1;
          SetConstant(F, OP, BOGEP, Next, instId);
          index++;
        }
      }
      else if (UIToFPInst *UI = dyn_cast<UIToFPInst>(&I)){
        Instruction *Next = getNextInstruction(UI, &BB);
        IRBuilder<> IRBI(Next);
        Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
        Add = IRB.CreateURem(Add, Num_Entries);
        Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
        GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(UI), BOGEP));

        ConsMap.insert(std::pair<Value*, Value*>(UI, BOGEP));

        //Set computed
        Value *OP = dyn_cast<Value>(UI);
        *instId = *instId + 1;
        SetConstant(F, OP, BOGEP, Next, instId);
        index++;
      }
      else if (FCmpInst *FCI = dyn_cast<FCmpInst>(&I)){
        Value *Op1 = FCI->getOperand(0);
        Value *Op2 = FCI->getOperand(1);

        bool Op1Call = false;
        bool Op2Call = false;

        if(isa<ConstantFP>(Op1) || Op1Call){
          Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
          Add = IRB.CreateURem(Add, Num_Entries);
          Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
          GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op1), BOGEP));

          ConsMap.insert(std::pair<Value*, Value*>(Op1, BOGEP));

          *instId = *instId + 1;
          SetConstant(F, Op1, BOGEP, First, instId);
          index++;
        }
        if(isa<ConstantFP>(Op2) || Op2Call){
          Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
          Add = IRB.CreateURem(Add, Num_Entries);
          Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
          GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op2), BOGEP));

          ConsMap.insert(std::pair<Value*, Value*>(Op2, BOGEP));

          *instId = *instId + 1;
          SetConstant(F, Op2, BOGEP, First, instId);
          index++;
        }
      }
      else if (LoadInst *LI = dyn_cast<LoadInst>(&I)){
        if(isFloatType(LI->getType())){
        }
      }
      else if (CallInst *CI = dyn_cast<CallInst>(&I)){
        Function *Callee = CI->getCalledFunction();
        IRBuilder<> IRBI(CI);
        if (Callee) {
          if (isListedFunction(Callee->getName(), "functions.txt") ||
              isListedFunction(Callee->getName(), "mathFunc.txt")){
            size_t NumOperands = CI->getNumArgOperands();
            Value *Op[NumOperands];
            Type *OpTy[NumOperands];
            bool Op1Call[NumOperands];
            for(int i = 0; i < NumOperands; i++){
              Op[i] = CI->getArgOperand(i);
              OpTy[i] = Op[i]->getType(); // this should be of float

              if(isFloatType( OpTy[i]) && isa<ConstantFP>(Op[i])){
                Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
                Add = IRB.CreateURem(Add, Num_Entries);
                Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

                GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op[i]), BOGEP));

                ConsMap.insert(std::pair<Value*, Value*>(Op[i], BOGEP));
                //Set computed
                *instId = *instId + 1;
                SetConstant(F, Op[i], BOGEP, First, instId);
                index++;
              }
            }
          }
          else{ //external function
            if(isFloatType(CI->getType())){
              Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
              Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

              GEPMap.insert(std::pair<Instruction*, Value*>(&I, BOGEP));

              index++;
            }
          }
        }
        else{//indirect
          if(isFloatType(CI->getType())){
          }
          size_t NumOperands = CI->getNumArgOperands();
          Value *Op[NumOperands];
          Type *OpTy[NumOperands];
          bool Op1Call[NumOperands];
          for(int i = 0; i < NumOperands; i++){
            Op[i] = CI->getArgOperand(i);
            OpTy[i] = Op[i]->getType(); // this should be of float
            Op1Call[i] = false;

            //handle function call which take as operand another function call,
            //but that function is not defined. It then should be treated a constant.
            if(isa<ConstantFP>(Op[i])){
              Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
              Add = IRB.CreateURem(Add, Num_Entries);
              Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

              GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op[i]), BOGEP));

              ConsMap.insert(std::pair<Value*, Value*>(Op[i], BOGEP));

              //Set computed
              *instId = *instId + 1;
              SetConstant(F, Op[i], BOGEP, First, instId);
              index++;
            }
          }
        }
      }
      else if (InvokeInst *CI = dyn_cast<InvokeInst>(&I)){
        Function *Callee = CI->getCalledFunction();
        IRBuilder<> IRBI(CI);
        BasicBlock *NBB = CI->getNormalDest();
        BasicBlock::iterator BBit = NBB->begin();
        Instruction *First = &*BBit;
        if (Callee) {
          if (isListedFunction(Callee->getName(), "functions.txt") ||
              isListedFunction(Callee->getName(), "mathFunc.txt")){
            size_t NumOperands = CI->getNumArgOperands();
            Value *Op[NumOperands];
            Type *OpTy[NumOperands];
            bool Op1Call[NumOperands];
            for(int i = 0; i < NumOperands; i++){
              Op[i] = CI->getArgOperand(i);
              OpTy[i] = Op[i]->getType(); // this should be of float

              if(isa<ConstantFP>(Op[i])){
                Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
                Add = IRB.CreateURem(Add, Num_Entries);
                Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

                GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op[i]), BOGEP));

                ConsMap.insert(std::pair<Value*, Value*>(Op[i], BOGEP));

                //Set computed
                *instId = *instId + 1;
                SetConstant(F, Op[i], BOGEP, First, instId);
                index++;
              }
            }
          }
          else{ //external function
            if(isFloatType(CI->getType())){
              Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
              Add = IRB.CreateURem(Add, Num_Entries);
              Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

              GEPMap.insert(std::pair<Instruction*, Value*>(&I, BOGEP));

              ConsMap.insert(std::pair<Value*, Value*>(CI, BOGEP));
              *instId = *instId + 1;
              SetConstant(F, CI, BOGEP, First, instId);

              index++;
            }
          }
        }
        else{//indirect
          if(isFloatType(CI->getType())){
          }
          size_t NumOperands = CI->getNumArgOperands();
          Value *Op[NumOperands];
          Type *OpTy[NumOperands];
          bool Op1Call[NumOperands];
          for(int i = 0; i < NumOperands; i++){
            Op[i] = CI->getArgOperand(i);
            OpTy[i] = Op[i]->getType(); // this should be of float
            Op1Call[i] = false;

            //handle function call which take as operand another function call,
            //but that function is not defined. It then should be treated a constant.
            if(isa<ConstantFP>(Op[i])){
              Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
              Add = IRB.CreateURem(Add, Num_Entries);
              Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

              GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op[i]), BOGEP));

              ConsMap.insert(std::pair<Value*, Value*>(Op[i], BOGEP));

              //Set computed
              *instId = *instId + 1;
              SetConstant(F, Op[i], BOGEP, First, instId);
              index++;
            }
          }
        }
      }
      else if (SelectInst *SI = dyn_cast<SelectInst>(&I)){
        if(isFloatType(I.getType())){
          if(isa<ConstantFP>((SI->getOperand(1)))){
            Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
            Add = IRB.CreateURem(Add, Num_Entries);
            Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

            GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(SI->getOperand(1)), BOGEP));

            ConsMap.insert(std::pair<Value*, Value*>(SI->getOperand(1), BOGEP));
            *instId = *instId + 1;
            SetConstant(F, SI->getOperand(1), BOGEP, First, instId);

            index++;
          }
          if(isa<ConstantFP>((SI->getOperand(2)))){
            Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
            Add = IRB.CreateURem(Add, Num_Entries);
            Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

            GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(SI->getOperand(2)), BOGEP));

            ConsMap.insert(std::pair<Value*, Value*>(SI->getOperand(2), BOGEP));
            *instId = *instId + 1;
            SetConstant(F, SI->getOperand(2), BOGEP, First, instId);

            index++;
          }
        }
      }
      else if(PHINode *PN = dyn_cast<PHINode>(&I)){
        Instruction *Next = BB.getFirstNonPHI();
        IRBuilder<> IRBI(Next);
        if(isFloatType(I.getType())){

        }
        for (unsigned PI = 0, PE = PN->getNumIncomingValues(); PI != PE; ++PI) {
          Value *IncValue = PN->getIncomingValue(PI);

          if (IncValue == PN) continue; //TODO
          if(isa<ConstantFP>(IncValue)){
            Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
            Add = IRB.CreateURem(Add, Num_Entries);
            Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
            if(DEBUG){
              SetRealTemp = M->getOrInsertFunction("eftsan_index", VoidTy, Int64Ty, MPtrTy);
              IRB.CreateCall(SetRealTemp, {Add, BOGEP});
            }

            GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(IncValue), BOGEP));

            ConsMap.insert(std::pair<Value*, Value*>(IncValue, BOGEP));

            //Set computed
            *instId = *instId + 1;
            SetConstant(F, IncValue, BOGEP, First, instId);
            index++;
          }
        }
      }
      if(ReturnInst *RT = dyn_cast<ReturnInst>(&I)){
        IRBuilder<> IRBI(RT);
        if (RT->getNumOperands() != 0){
          Value *Op = RT->getOperand(0);
          if(isFloatType(Op->getType())){
            if(isa<ConstantFP>(Op)){
              Add = IRB.CreateAdd(Add, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr");
              Add = IRB.CreateURem(Add, Num_Entries);
              Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

              GEPMap.insert(std::pair<Instruction*, Value*>(dyn_cast<Instruction>(Op), BOGEP));

              ConsMap.insert(std::pair<Value*, Value*>(Op, BOGEP));

              //Set computed
              *instId = *instId + 1;
              SetConstant(F, Op, BOGEP, First, instId);
              index++;
            }
          }
        }
      }
    }
  }
  IRB.CreateStore(Add, MStackTop, "my_store");
  FuncTotalIns.insert(std::pair<Function*, long>(F, index));
}

#if 0
AllocaInst * EFTSanitizer::createAlloca(Function *F, size_t InsCount){
  Function::iterator Fit = F->begin();
  BasicBlock &BB = *Fit; 
  BasicBlock::iterator BBit = BB.begin();
  Instruction *First = &*BBit;
  IRBuilder<> IRB(First);
  Module *M = F->getParent();
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* Int1Ty = Type::getInt1Ty(M->getContext());
  Type* Int8Ty = Type::getInt8Ty(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type *PtrI8Ty = PointerType::getUnqual(Int8Ty);
  Type *PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));

  Instruction *End;
  for (auto &BB : *F) {	
    for (auto &I : BB) {
      if (dyn_cast<ReturnInst>(&I)){
        End = &I;
      }
    }
  }

  AllocaInst *Alloca = IRB.CreateAlloca(ArrayType::get(Real, InsCount), nullptr);
  //memset alloca to 0
  BitCastInst* BCToAddr = new BitCastInst(Alloca, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", First);
  Constant* Zero = ConstantInt::get(Int8Ty, 0);
  Constant* False = ConstantInt::get(Int1Ty, false);
#if TRACING
  Constant* Size = ConstantInt::get(Int64Ty, 48 * InsCount);
#else
  Constant* Size = ConstantInt::get(Int64Ty, 8 * InsCount);
#endif
  Finish = M->getOrInsertFunction("llvm.memset.p0i8.i64", VoidTy, PtrVoidTy, Int8Ty, Int64Ty, Int1Ty);
  IRB.CreateCall(Finish, {BCToAddr, Zero, Size, False});

  return Alloca;
}
#endif

void EFTSanitizer::callGetArgument(Function *F){
  Module *M = F->getParent();
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  for (Function::arg_iterator ait = F->arg_begin(), aend = F->arg_end();
      ait != aend; ++ait) {
    Argument *A = &*ait;
    if(isFloatType(A->getType())){
      Function::iterator Fit = F->begin();
      BasicBlock &BB = *Fit; 
      BasicBlock::iterator BBit = BB.begin();
      Instruction *First = &*BBit;
      IRBuilder<> IRB(First);
      size_t Idx =  ArgMap.at(A);
      Constant* ArgNo = ConstantInt::get(Type::getInt64Ty(M->getContext()), Idx);

      SetRealTemp = M->getOrInsertFunction("eftsan_get_arg", MPtrTy, Int64Ty);
      
      Value* ConsInsIndex = IRB.CreateCall(SetRealTemp, {ArgNo});
      MArgMap.insert(std::pair<Argument*, Instruction*>(A, dyn_cast<Instruction>(ConsInsIndex)));
    }
  }
}

long EFTSanitizer::countConstants(Function *F){
  long TotalAlloca = 0;
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (UnaryOperator *UO = dyn_cast<UnaryOperator>(&I)) {
        switch (UO->getOpcode()) {
          case Instruction::FNeg:
            {
            }
        }
      }
      else if (BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)){
        switch(BO->getOpcode()) {                                                                                                
          case Instruction::FAdd:
          case Instruction::FSub:
          case Instruction::FMul:
          case Instruction::FDiv:
            {
              Value *Op1 = BO->getOperand(0);
              Value *Op2 = BO->getOperand(1);

              if(isa<ConstantFP>(Op1)){
                TotalAlloca++;
              }
              if(isa<ConstantFP>(Op2)){
                TotalAlloca++;
              }
            }
        }
      }
      else if (StoreInst *SI = dyn_cast<StoreInst>(&I)){
        Value *OP = SI->getOperand(0);
        if(isa<ConstantFP>(OP)){
          TotalAlloca++;
        }
      }
      else if (SIToFPInst *UI = dyn_cast<SIToFPInst>(&I)){
        TotalAlloca++;
      }
      else if (BitCastInst *UI = dyn_cast<BitCastInst>(&I)){
        if(isFloatType(UI->getType())){
          TotalAlloca++;
        }
      }
      else if (UIToFPInst *UI = dyn_cast<UIToFPInst>(&I)){
        TotalAlloca++;
      }
      else if (FCmpInst *FCI = dyn_cast<FCmpInst>(&I)){
        Value *Op1 = FCI->getOperand(0);
        Value *Op2 = FCI->getOperand(1);

        bool Op1Call = false;
        bool Op2Call = false;

        if(isa<ConstantFP>(Op1) || Op1Call){
          TotalAlloca++;
        }
        if(isa<ConstantFP>(Op2) || Op2Call){
          TotalAlloca++;
        }
      }
      else if (CallInst *CI = dyn_cast<CallInst>(&I)){
        Function *Callee = CI->getCalledFunction();
        if (Callee) {
          if (isListedFunction(Callee->getName(), "functions.txt") ||
              isListedFunction(Callee->getName(), "mathFunc.txt")){ 
            size_t NumOperands = CI->getNumArgOperands();
            Value *Op[NumOperands];
            Type *OpTy[NumOperands];
            bool Op1Call[NumOperands];
            for(int i = 0; i < NumOperands; i++){
              Op[i] = CI->getArgOperand(i);
              OpTy[i] = Op[i]->getType(); // this should be of float

              if(isa<ConstantFP>(Op[i])){
                TotalAlloca++;
              }
            }
          }
        }
        else{//indirect
          size_t NumOperands = CI->getNumArgOperands();
          Value *Op[NumOperands];
          Type *OpTy[NumOperands];
          bool Op1Call[NumOperands];
          for(int i = 0; i < NumOperands; i++){
            Op[i] = CI->getArgOperand(i);
            OpTy[i] = Op[i]->getType(); // this should be of float

            if(isa<ConstantFP>(Op[i])){
              TotalAlloca++;
            }
          }
        }
      }
      else if (InvokeInst *CI = dyn_cast<InvokeInst>(&I)){
        Function *Callee = CI->getCalledFunction();
        if (Callee) {
          if (isListedFunction(Callee->getName(), "functions.txt") ||
              isListedFunction(Callee->getName(), "mathFunc.txt")){
            size_t NumOperands = CI->getNumArgOperands();
            Value *Op[NumOperands];
            Type *OpTy[NumOperands];
            bool Op1Call[NumOperands];
            for(int i = 0; i < NumOperands; i++){
              Op[i] = CI->getArgOperand(i);
              OpTy[i] = Op[i]->getType(); // this should be of float

              if(isa<ConstantFP>(Op[i])){
                TotalAlloca++;
              }
            }
          }
        }
        else{//indirect
          size_t NumOperands = CI->getNumArgOperands();
          Value *Op[NumOperands];
          Type *OpTy[NumOperands];
          bool Op1Call[NumOperands];
          for(int i = 0; i < NumOperands; i++){
            Op[i] = CI->getArgOperand(i);
            OpTy[i] = Op[i]->getType(); // this should be of float

            if(isa<ConstantFP>(Op[i])){
              TotalAlloca++;
            }
          }
        }
      }
      else if (FCmpInst *FCI = dyn_cast<FCmpInst>(&I)){
        if(isFloatType(I.getType())){
          if(isa<ConstantFP>((FCI->getOperand(0)))){
            TotalAlloca++;
          }
          if(isa<ConstantFP>((FCI->getOperand(1)))){
            TotalAlloca++;
          }
        }
      }
      else if (SelectInst *SI = dyn_cast<SelectInst>(&I)){
        if(isFloatType(I.getType())){
          if(isa<ConstantFP>((SI->getOperand(1)))){
            TotalAlloca++;
          }
          if(isa<ConstantFP>((SI->getOperand(2)))){
            TotalAlloca++;
          }
        }
      }
      else if(PHINode *PN = dyn_cast<PHINode>(&I)){
        for (unsigned PI = 0, PE = PN->getNumIncomingValues(); PI != PE; ++PI) {
          Value *IncValue = PN->getIncomingValue(PI);

          if (IncValue == PN) continue; //TODO
          if(isa<ConstantFP>(IncValue))
            if(isa<ConstantFP>(IncValue)){
              TotalAlloca++;
            }
        }
      }
      else if(ReturnInst *RT = dyn_cast<ReturnInst>(&I)){
        if (RT->getNumOperands() != 0){
          Value *Op = RT->getOperand(0);
          if(isFloatType(Op->getType())){
            if(isa<ConstantFP>(Op)){
              TotalAlloca++;
            }
          }
          else if(Op->getType()->isVectorTy() && 
              isFloatType(Op->getType()->getScalarType())){ //vector
            TotalAlloca++;
          }
        }
      }
    }
  }
  return TotalAlloca;
}

long EFTSanitizer::getTotalFPInst(Function *F){
  long TotalAlloca = 0;
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (UnaryOperator *UO = dyn_cast<UnaryOperator>(&I)) {
        switch (UO->getOpcode()) {
          case Instruction::FNeg:
            {
              TotalAlloca++;
            }
        }
      }
      else if (BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)){
        switch(BO->getOpcode()) {                                                                                                
          case Instruction::FAdd:
          case Instruction::FSub:
          case Instruction::FMul:
          case Instruction::FDiv:
            if(BO->getType()->isVectorTy()){ 
              TotalAlloca++;
              TotalAlloca++;
            }
            else
            {
              Value *Op1 = BO->getOperand(0);
              Value *Op2 = BO->getOperand(1);

              TotalAlloca++;

              if(isa<ConstantFP>(Op1)){
                TotalAlloca++;
              }
              if(isa<ConstantFP>(Op2)){
                TotalAlloca++;
              }
            }
        }
      }
      else if (SIToFPInst *UI = dyn_cast<SIToFPInst>(&I)){
        TotalAlloca++;
      }
      else if (BitCastInst *UI = dyn_cast<BitCastInst>(&I)){
        if(isFloatType(UI->getType())){
          TotalAlloca++;
        }
        else if(UI->getType()->isVectorTy() && 
              isFloatType(UI->getType()->getScalarType())){ //vector
          TotalAlloca++;
        }
      }
      else if (UIToFPInst *UI = dyn_cast<UIToFPInst>(&I)){
        TotalAlloca++;
      }
      else if (FCmpInst *FCI = dyn_cast<FCmpInst>(&I)){
        Value *Op1 = FCI->getOperand(0);
        Value *Op2 = FCI->getOperand(1);

        bool Op1Call = false;
        bool Op2Call = false;

        if(isa<ConstantFP>(Op1) || Op1Call){
          TotalAlloca++;
        }
        if(isa<ConstantFP>(Op2) || Op2Call){
          TotalAlloca++;
        }
      }
      else if (LoadInst *LI = dyn_cast<LoadInst>(&I)){
        if(isFloatType(LI->getType())){
          TotalAlloca++;
        }
        else if(LI->getType()->isVectorTy() && 
              isFloatType(LI->getType()->getScalarType())){ //vector
          TotalAlloca++;
        }
      }
      else if (CallInst *CI = dyn_cast<CallInst>(&I)){
        Function *Callee = CI->getCalledFunction();
        if (Callee) {
          if (isListedFunction(Callee->getName(), "functions.txt") ||
              isListedFunction(Callee->getName(), "mathFunc.txt")){ 
            TotalAlloca++;
            size_t NumOperands = CI->getNumArgOperands();
            Value *Op[NumOperands];
            Type *OpTy[NumOperands];
            bool Op1Call[NumOperands];
            for(int i = 0; i < NumOperands; i++){
              Op[i] = CI->getArgOperand(i);
              OpTy[i] = Op[i]->getType(); // this should be of float

              if(isa<ConstantFP>(Op[i])){
                TotalAlloca++;
              }
            }
          }
          else{
            if(isFloatType(CI->getType())){
              TotalAlloca++;
            }
            else if(CI->getType()->isVectorTy() && 
                isFloatType(CI->getType()->getScalarType())){ //vector
              TotalAlloca++;
            }
          }
        }
        else{//indirect
          if(isFloatType(CI->getType())){
            TotalAlloca++;
          }
          else if(CI->getType()->isVectorTy() && 
              isFloatType(CI->getType()->getScalarType())){ //vector
            TotalAlloca++;
          }
          size_t NumOperands = CI->getNumArgOperands();
          Value *Op[NumOperands];
          Type *OpTy[NumOperands];
          bool Op1Call[NumOperands];
          for(int i = 0; i < NumOperands; i++){
            Op[i] = CI->getArgOperand(i);
            OpTy[i] = Op[i]->getType(); // this should be of float

            if(isa<ConstantFP>(Op[i])){
              TotalAlloca++;
            }
          }
        }
      }
      else if (InvokeInst *CI = dyn_cast<InvokeInst>(&I)){
        Function *Callee = CI->getCalledFunction();
        if (Callee) {
          if (isListedFunction(Callee->getName(), "functions.txt") ||
              isListedFunction(Callee->getName(), "mathFunc.txt")){
            TotalAlloca++;
            size_t NumOperands = CI->getNumArgOperands();
            Value *Op[NumOperands];
            Type *OpTy[NumOperands];
            bool Op1Call[NumOperands];
            for(int i = 0; i < NumOperands; i++){
              Op[i] = CI->getArgOperand(i);
              OpTy[i] = Op[i]->getType(); // this should be of float

              if(isa<ConstantFP>(Op[i])){
                TotalAlloca++;
              }
            }
          }
          else{
            if(isFloatType(CI->getType())){
              TotalAlloca++;
            }
            else if(CI->getType()->isVectorTy() && 
              isFloatType(CI->getType()->getScalarType())){ //vector
              TotalAlloca++;
            }
          }
        }
        else{//indirect
          if(isFloatType(CI->getType())){
            TotalAlloca++;
          }
          else if(CI->getType()->isVectorTy() && 
              isFloatType(CI->getType()->getScalarType())){ //vector
            TotalAlloca++;
          }
          size_t NumOperands = CI->getNumArgOperands();
          Value *Op[NumOperands];
          Type *OpTy[NumOperands];
          bool Op1Call[NumOperands];
          for(int i = 0; i < NumOperands; i++){
            Op[i] = CI->getArgOperand(i);
            OpTy[i] = Op[i]->getType(); // this should be of float

            if(isa<ConstantFP>(Op[i])){
              TotalAlloca++;
            }
          }
        }
      }
      else if (FCmpInst *FCI = dyn_cast<FCmpInst>(&I)){
        if(isFloatType(I.getType())){
          if(isa<ConstantFP>((FCI->getOperand(0)))){
            TotalAlloca++;
          }
          if(isa<ConstantFP>((FCI->getOperand(1)))){
            TotalAlloca++;
          }
        }
      }
      else if (SelectInst *SI = dyn_cast<SelectInst>(&I)){
        if(isFloatType(I.getType())){
          if(isa<ConstantFP>((SI->getOperand(1)))){
            TotalAlloca++;
          }
          if(isa<ConstantFP>((SI->getOperand(2)))){
            TotalAlloca++;
          }
        }
      }
      else if(PHINode *PN = dyn_cast<PHINode>(&I)){
        if(isFloatType(I.getType())){
          TotalAlloca++;
        }
        else if(PN->getType()->isVectorTy() && 
            isFloatType(PN->getType()->getScalarType())){ //vector
          TotalAlloca++;
        }
        for (unsigned PI = 0, PE = PN->getNumIncomingValues(); PI != PE; ++PI) {
          Value *IncValue = PN->getIncomingValue(PI);

          if (IncValue == PN) continue; //TODO
          if(isa<ConstantFP>(IncValue))
            if(isa<ConstantFP>(IncValue)){
              TotalAlloca++;
            }
        }
      }
      else if(ReturnInst *RT = dyn_cast<ReturnInst>(&I)){
        if (RT->getNumOperands() != 0){
          Value *Op = RT->getOperand(0);
          if(isFloatType(Op->getType())){
            if(isa<ConstantFP>(Op)){
              TotalAlloca++;
            }
          }
          else if(Op->getType()->isVectorTy() && 
              isFloatType(Op->getType()->getScalarType())){ //vector
            TotalAlloca++;
          }
        }
      }
    }
  }
  return TotalAlloca;
}

void EFTSanitizer::createMpfrAlloca(Function *F, int *instId){
  long TotalArg = 0;
  long TotalAlloca = 0;
  for (Function::arg_iterator ait = F->arg_begin(), aend = F->arg_end();
      ait != aend; ++ait) {
    Argument *A = &*ait;
    ArgMap.insert(std::pair<Argument*, long>(A, TotalArg));
    TotalArg++;
  }
  FuncTotalArg.insert(std::pair<Function*, long>(F, TotalArg));
  TotalAlloca = getTotalFPInst(F);

  createGEP(F, TotalArg, instId);
}


Instruction*
EFTSanitizer::getNextInstruction(Instruction *I, BasicBlock *BB){
  Instruction *Next;
  for (BasicBlock::iterator BBit = BB->begin(), BBend = BB->end();
      BBit != BBend; ++BBit) {
    Next = &*BBit;
    if(I == Next){
      Next = &*(++BBit);
      break;
    }
  }
  return Next;
}

Instruction*
EFTSanitizer::getNextInstructionNotPhi(Instruction *I, BasicBlock *BB){
  Instruction *Next;
  for (auto &I : *BB) {
    if(!isa<PHINode>(I)){
      Next = &I;
      break;
    }
  }
  return Next;
}

void EFTSanitizer::findInterestingFunctions(Function *F){
  long TotalFPInst = getTotalFPInst(F); 
  if(TotalFPInst > 0){
    std::string name = F->getName();
    addFunctionsToList(name);
  }
}

void EFTSanitizer::handleFuncMainInit(Function *F, int size){
  Function::iterator Fit = F->begin();
  BasicBlock &BB = *Fit; 
  BasicBlock::iterator BBit = BB.begin();                                                                                                                                   
  Instruction *First = &*BBit;

  Module *M = F->getParent();
  IRBuilder<> IRB(First);

  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* Int8Ty = Type::getInt8Ty(M->getContext());
  Type* Int32Ty = Type::getInt64Ty(M->getContext());
  Type *PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));
  Type *PtrI8Ty = PointerType::getUnqual(Int8Ty);

  Value *NullPtr = ConstantPointerNull::get(PointerType::get(Int8Ty, 0));
  Finish = M->getOrInsertFunction("mmap", PtrI8Ty, PtrI8Ty, Int64Ty, Int32Ty, Int32Ty, Int32Ty, Int64Ty);
  if(ClTracing){
    Value *ShadowMem = IRB.CreateCall(Finish, {NullPtr,
        ConstantInt::get(Int64Ty, 3758096384),   
        ConstantInt::get(Int32Ty, 3),
        ConstantInt::get(Int32Ty, 16418),
        ConstantInt::get(Int32Ty, -1),
        ConstantInt::get(Int64Ty, 0)});

    BitCastInst* BCToAddr = new BitCastInst(Shadow_Mem,
        PointerType::getUnqual(PtrI8Ty),"", First);
    IRB.CreateStore(ShadowMem, BCToAddr, "my_shadow_mem");
  }
  else{
    Value *ShadowMem = IRB.CreateCall(Finish, {NullPtr,
        ConstantInt::get(Int64Ty, 1610612736),
        ConstantInt::get(Int32Ty, 3),
        ConstantInt::get(Int32Ty, 16418),
        ConstantInt::get(Int32Ty, -1),
        ConstantInt::get(Int64Ty, 0)});

    BitCastInst* BCToAddr = new BitCastInst(Shadow_Mem,
        PointerType::getUnqual(PtrI8Ty),"", First);
    IRB.CreateStore(ShadowMem, BCToAddr, "my_shadow_mem");
  }
//shadow stack
  Finish = M->getOrInsertFunction("mmap", PtrI8Ty, PtrI8Ty, Int64Ty, Int32Ty, Int32Ty, Int32Ty, Int64Ty);
  if(ClTracing){
    Value *ShadowStack = IRB.CreateCall(Finish, {NullPtr,
        ConstantInt::get(Int64Ty, SHADOW_STACK_SIZE_T),
        ConstantInt::get(Int32Ty, 3),
        ConstantInt::get(Int32Ty, 16418),
        ConstantInt::get(Int32Ty, -1),
        ConstantInt::get(Int64Ty, 0)});

    BitCastInst* BCToAddr1 = new BitCastInst(Shadow_Stack,
        PointerType::getUnqual(PtrI8Ty),"", First);
    IRB.CreateStore(ShadowStack, BCToAddr1, "my_shadow_stack");
  }
  else{
    Value *ShadowStack = IRB.CreateCall(Finish, {NullPtr,
        ConstantInt::get(Int64Ty, SHADOW_STACK_SIZE),
        ConstantInt::get(Int32Ty, 3),
        ConstantInt::get(Int32Ty, 16418),
        ConstantInt::get(Int32Ty, -1),
        ConstantInt::get(Int64Ty, 0)});

    BitCastInst* BCToAddr1 = new BitCastInst(Shadow_Stack,
        PointerType::getUnqual(PtrI8Ty),"", First);
    IRB.CreateStore(ShadowStack, BCToAddr1, "my_shadow_stack");
  }

  Finish = M->getOrInsertFunction("eftsan_init", VoidTy, Int64Ty);
  long TotIns = 0;

  IRB.CreateStore(ConstantInt::get(Int64Ty, 0), TimeStamp, "my_ts");
  IRB.CreateCall(Finish, ConstantInt::get(Int64Ty, size));
}

void EFTSanitizer::handleMainRet(Instruction *I, Function *F){ 
  Module *M = F->getParent();
  IRBuilder<> IRB(I);
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Finish = M->getOrInsertFunction("eftsan_finish", VoidTy);
  IRB.CreateCall(Finish, {});
}


void
EFTSanitizer::handleError2 (InvokeInst *CI,
    BasicBlock *BB,
    Function *F) {

  Instruction *I = dyn_cast<Instruction>(CI);
  Module *M = F->getParent();
  IRBuilder<> IRB(I);


  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());

  size_t NumOperands = CI->getNumArgOperands();
  Value *Op[NumOperands];
  Type *OpTy[NumOperands];
  for(int i = 0; i < NumOperands; i++){
    Op[i] = CI->getArgOperand(i);
    OpTy[i] = Op[i]->getType(); // this should be of float
    if(isFloatType(OpTy[i])){
      Instruction *OpIns = dyn_cast<Instruction>(Op[i]);
      Value *OpIdx;
      bool res = handleOperand(F, OpIns, Op[i], &OpIdx, nullptr);
      if(!res){
        errs()<<"\nError !!! metadata not found for operand:\n";
        Op[i]->dump();
        errs()<<"In Inst:"<<"\n";
        I->dump();
        exit(1);
      }
      Constant* ArgNo = ConstantInt::get(Type::getInt64Ty(M->getContext()), i+1);
      AddFunArg = M->getOrInsertFunction("eftsan_get_error", VoidTy, MPtrTy, OpTy[i]);

      IRB.CreateCall(AddFunArg, {OpIdx, Op[i]});
    }
  }
}

void
EFTSanitizer::handleError (CallInst *CI,
    BasicBlock *BB,
    Function *F) {

  Instruction *I = dyn_cast<Instruction>(CI);
  Module *M = F->getParent();
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRBN(Next);
  IRBuilder<> IRB(I);


  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());

  size_t NumOperands = CI->getNumArgOperands();
  Value *Op[NumOperands];
  Type *OpTy[NumOperands];
  for(int i = 0; i < NumOperands; i++){
    Op[i] = CI->getArgOperand(i);
    OpTy[i] = Op[i]->getType(); // this should be of float
    if(isFloatType(OpTy[i])){
      Instruction *OpIns = dyn_cast<Instruction>(Op[i]);
      Value *OpIdx;
      bool res = handleOperand(F, OpIns, Op[i], &OpIdx, nullptr);
      if(!res){
        errs()<<"\nError !!! metadata not found for operand:\n";
        Op[i]->dump();
        errs()<<"In Inst:"<<"\n";
        I->dump();
        exit(1);
      }
      Constant* ArgNo = ConstantInt::get(Type::getInt64Ty(M->getContext()), i+1);
      AddFunArg = M->getOrInsertFunction("eftsan_get_error", VoidTy, MPtrTy, OpTy[i]);

      IRB.CreateCall(AddFunArg, {OpIdx, Op[i]});
    }
  }
}

void
EFTSanitizer::handleInvokeInst (InvokeInst *CI,
    BasicBlock *BB,
    Function *F) {
  Instruction *I = dyn_cast<Instruction>(CI);
  Module *M = F->getParent();
  BasicBlock *NBB = CI->getNormalDest();
  BasicBlock::iterator BBit = NBB->begin();
  Instruction *First = &*BBit;
  IRBuilder<> IRBN(First);
  IRBuilder<> IRB(I);


  Type* Int8Ty = Type::getInt8Ty(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());

  size_t NumOperands = CI->getNumArgOperands();
  if(isFloatType(CI->getType())){
    long InsIndex;
    long TotalArgs = FuncTotalArg.at(F);
    long TotalIns = FuncTotalIns.at(F);
    Value *StackTop = IRBN.CreateLoad(Int64Ty, MStackTop, "my_load_stack_top");  

    Value *Dest = IRBN.CreateGEP(ShadowSL, StackTop);
    MInsMap.insert(std::pair<Instruction*, Instruction*>(I, dyn_cast<Instruction>(Dest)));

#if TRACING
    /*
    //store instid ts
    ConstantInt* insId = GetInstId(F, CI);
    Value *TSOp1 = IRBN.CreateGEP(Dest, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 7)}, "my_gep");
    Value *TStampOp1 = IRBN.CreateLoad(IRBN.getInt64Ty(), TSOp1, "my_ts");

    Value *InstMapGep = IRBN.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
    insId}, "my_gep");
    IRBN.CreateStore(TStampOp1, InstMapGep, "my_store");

*/
#endif
    FuncInit = M->getOrInsertFunction("eftsan_get_return", MPtrTy, Int64Ty, MPtrTy);
    Value* ConsInsIndex = IRBN.CreateCall(FuncInit, {StackTop, Dest});
  } 
  else if(CI->getType()->isVectorTy() && 
      isFloatType(CI->getType()->getScalarType())){ //vector
    //considering size 2 vector
    /*
       Constant* TotalArgsConst = ConstantInt::get(Type::getInt64Ty(M->getContext()), NumOperands+2); 
       FuncInit = M->getOrInsertFunction("eftsan_get_return", MPtrTy, Int64Ty, Int64Ty);
       Value* ConsInsIndex = IRBN.CreateCall(FuncInit, {TotalArgsConst, MStackTop});

       FuncInit = M->getOrInsertFunction("eftsan_get_return_vec", MPtrTy, Int64Ty, Int64Ty);
       Value* ConsInsIndex2 = IRBN.CreateCall(FuncInit, {TotalArgsConst, MStackTop});
       MInsMapPair.insert(std::pair<Instruction*, std::pair<Instruction*, Instruction*>>(I, 
       std::make_pair(dyn_cast<Instruction>(ConsInsIndex), dyn_cast<Instruction>(ConsInsIndex2))));
       */
  }

  Value *Op[NumOperands];
  Type *OpTy[NumOperands];
  for(int i = 0; i < NumOperands; i++){
    Op[i] = CI->getArgOperand(i);
    OpTy[i] = Op[i]->getType(); // this should be of float
    if(isFloatType(OpTy[i])){
      Instruction *OpIns = dyn_cast<Instruction>(Op[i]);
      Value *OpIdx;
      bool res = handleOperand(F, I, Op[i], &OpIdx, nullptr);
      if(!res){
        errs()<<"\nError !!! metadata not found for operand:\n";
        Op[i]->dump();
        errs()<<"In Inst:"<<"\n";
        I->dump();
        exit(1);
      }
      Constant* ArgNo = ConstantInt::get(Type::getInt64Ty(M->getContext()), i);
      if(isFloat(OpTy[i]))
        AddFunArg = M->getOrInsertFunction("eftsan_set_arg_f", VoidTy, MPtrTy, OpTy[i], Int8Ty, Int64Ty);
      else if(isDouble(OpTy[i]))
        AddFunArg = M->getOrInsertFunction("eftsan_set_arg_d", VoidTy, MPtrTy, OpTy[i], Int8Ty, Int64Ty);

      Constant *ConsFlag;
      if(isa<ConstantFP>(Op[i])){
        ConsFlag = ConstantInt::get(Type::getInt8Ty(M->getContext()), 1);
      }
      else{
        ConsFlag = ConstantInt::get(Type::getInt8Ty(M->getContext()), 0);
      }
      IRB.CreateCall(AddFunArg, {OpIdx, Op[i], ConsFlag, ArgNo});
    }
  }
}

void
EFTSanitizer::handleCallInst (CallInst *CI,
    BasicBlock *BB,
    Function *F) {

  Instruction *I = dyn_cast<Instruction>(CI);
  Module *M = F->getParent();
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRBN(Next);
  IRBuilder<> IRB(I);


  Type* Int8Ty = Type::getInt8Ty(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());

  size_t NumOperands = CI->getNumArgOperands();
  if(isFloatType(CI->getType())){
    long InsIndex;
    long TotalArgs = FuncTotalArg.at(F);
    long TotalIns = FuncTotalIns.at(F);
    Value *StackTop = IRBN.CreateLoad(Int64Ty, MStackTop, "my_load_stack_top");  

    Value *Dest = IRBN.CreateGEP(ShadowSL, StackTop);
    MInsMap.insert(std::pair<Instruction*, Instruction*>(I, dyn_cast<Instruction>(Dest)));

#if TRACING
    /*
    //store instid ts
    ConstantInt* insId = GetInstId(F, CI);
    Value *TSOp1 = IRBN.CreateGEP(Dest, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp1 = IRBN.CreateLoad(IRBN.getInt64Ty(), TSOp1, "my_ts");

    Value *InstMapGep = IRBN.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
    insId}, "my_gep");
    IRBN.CreateStore(TStampOp1, InstMapGep, "my_store");

*/
#endif
    FuncInit = M->getOrInsertFunction("eftsan_get_return", MPtrTy, Int64Ty, MPtrTy);
    Value* ConsInsIndex = IRBN.CreateCall(FuncInit, {StackTop, Dest});
  } 
  else if(CI->getType()->isVectorTy() && 
      isFloatType(CI->getType()->getScalarType())){ //vector
    //considering size 2 vector
    /*
       Constant* TotalArgsConst = ConstantInt::get(Type::getInt64Ty(M->getContext()), NumOperands+2); 
       FuncInit = M->getOrInsertFunction("eftsan_get_return", MPtrTy, Int64Ty, Int64Ty);
       Value* ConsInsIndex = IRBN.CreateCall(FuncInit, {TotalArgsConst, MStackTop});

       FuncInit = M->getOrInsertFunction("eftsan_get_return_vec", MPtrTy, Int64Ty, Int64Ty);
       Value* ConsInsIndex2 = IRBN.CreateCall(FuncInit, {TotalArgsConst, MStackTop});
       MInsMapPair.insert(std::pair<Instruction*, std::pair<Instruction*, Instruction*>>(I, 
       std::make_pair(dyn_cast<Instruction>(ConsInsIndex), dyn_cast<Instruction>(ConsInsIndex2))));
       */
  }

  Value *Op[NumOperands];
  Type *OpTy[NumOperands];
  for(int i = 0; i < NumOperands; i++){
    Op[i] = CI->getArgOperand(i);
    OpTy[i] = Op[i]->getType(); // this should be of float
    if(isFloatType(OpTy[i])){
      Instruction *OpIns = dyn_cast<Instruction>(Op[i]);
      Value *OpIdx;
      bool res = handleOperand(F, I, Op[i], &OpIdx, nullptr);
      if(!res){
        errs()<<"\nError !!! metadata not found for operand:\n";
        Op[i]->dump();
        errs()<<"In Inst:"<<"\n";
        I->dump();
        exit(1);
      }
      Constant* ArgNo = ConstantInt::get(Type::getInt64Ty(M->getContext()), i);
      if(isFloat(OpTy[i]))
        AddFunArg = M->getOrInsertFunction("eftsan_set_arg_f", VoidTy, MPtrTy, OpTy[i], Int8Ty, Int64Ty);
      else if(isDouble(OpTy[i]))
        AddFunArg = M->getOrInsertFunction("eftsan_set_arg_d", VoidTy, MPtrTy, OpTy[i], Int8Ty, Int64Ty);

      Constant *ConsFlag;
      if(isa<ConstantFP>(Op[i])){
        ConsFlag = ConstantInt::get(Type::getInt8Ty(M->getContext()), 1);
      }
      else{
        ConsFlag = ConstantInt::get(Type::getInt8Ty(M->getContext()), 0);
      }
      IRB.CreateCall(AddFunArg, {OpIdx, Op[i], ConsFlag, ArgNo});
    }
  }
}

void EFTSanitizer::handleFuncInit(Function *F){
  Function::iterator Fit = F->begin();
  BasicBlock &BB = *Fit; 
  BasicBlock::iterator BBit = BB.begin();
  Instruction *First = &*BBit;

  Module *M = F->getParent();
  IRBuilder<> IRB(First);
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());

  Type *PtrInt64Ty = PointerType::getUnqual(Int64Ty);
  long TotalArgs = FuncTotalArg.at(F);
  long TotalIns = FuncTotalIns.at(F);
  long TotalSlots = TotalArgs + TotalIns;


  Constant* ConsTotIns = ConstantInt::get(Type::getInt64Ty(M->getContext()), TotalSlots); 
  FuncInit = M->getOrInsertFunction("eftsan_func_init", VoidTy, Int64Ty, PtrInt64Ty);
//  IRB.CreateCall(FuncInit, {ConsTotIns, MStackTop});
}

void EFTSanitizer::handleMemset(CallInst *CI, BasicBlock *BB, Function *F, std::string CallName) {
  Instruction *I = dyn_cast<Instruction>(CI);
  Instruction *Next = getNextInstruction(dyn_cast<Instruction>(CI), BB);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Type *VoidTy = Type::getVoidTy(M->getContext());

  IntegerType *Int32Ty = Type::getInt32Ty(M->getContext());
  IntegerType *Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
  Type *Int64Ty = Type::getInt64Ty(M->getContext());
  Type *PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));

  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0)
      debugInfoAvail = false;
  }
  ConstantInt *debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt *lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt *colNumber = ConstantInt::get(Int32Ty, colNum);

  Value *Op1Addr = CI->getOperand(0);
  Value *Op2Val = CI->getOperand(1);
  Value *size = CI->getOperand(2);
  if (BitCastInst *BI = dyn_cast<BitCastInst>(Op1Addr)) {
    if (checkIfBitcastFromFP(BI)) {
      FuncInit = M->getOrInsertFunction("eftsan_handle_memset", VoidTy,
          PtrVoidTy, Int8Ty, Int64Ty);
//      IRB.CreateCall(FuncInit, {Op1Addr, Op2Val, size});
    }
  }
  if (LoadInst *LI = dyn_cast<LoadInst>(Op1Addr)) {
    Value *Addr = LI->getPointerOperand();
    if (BitCastInst *BI = dyn_cast<BitCastInst>(Addr)) {
      if (checkIfBitcastFromFP(BI)) {
        FuncInit = M->getOrInsertFunction("eftsan_handle_memset", VoidTy,
            PtrVoidTy, Int8Ty, Int64Ty);
//        IRB.CreateCall(FuncInit, {Addr, Op2Val, size});
      }
    }
  }
}

void
EFTSanitizer::handleMemCpy (CallInst *CI,
    BasicBlock *BB,
    Function *F) {

  if (std::find(MemcpyNList.begin(), MemcpyNList.end(), CI) != MemcpyNList.end()) {
    return;
  }
  Instruction *I = dyn_cast<Instruction>(CI);
  Instruction *Next = getNextInstruction(dyn_cast<Instruction>(CI), BB);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Type* VoidTy = Type::getVoidTy(M->getContext());

  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));

  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);

  Value *Op1Addr = CI->getOperand(0);
  Value *Op2Addr = CI->getOperand(1);
  Value *size = CI->getOperand(2);
  BitCastInst*
    BCToAddr1 = new BitCastInst(Op1Addr, 
        PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
  BitCastInst*
    BCToAddr2 = new BitCastInst(Op2Addr, 
        PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
  if (BitCastInst *BI = dyn_cast<BitCastInst>(Op1Addr)){
    if(checkIfBitcastFromFP(BI)){
      FuncInit = M->getOrInsertFunction("eftsan_handle_memcpy", VoidTy, PtrVoidTy, PtrVoidTy, Int64Ty);
      //IRB.CreateCall(FuncInit, {BCToAddr1, BCToAddr2, size});

      IRBuilder<> IRB(I);
      IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
      IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());
      Type* VoidTy = Type::getVoidTy(M->getContext());
      Type* DoubleTy = Type::getDoubleTy(M->getContext());

      Value *PtrToInt = IRB.CreatePtrToInt(BCToAddr1, Int64Ty, "my_ptr_int");
      Value *Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
      Value *Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");
      //load shadow addr
      Value *LoadDest = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");

      PtrToInt = IRB.CreatePtrToInt(BCToAddr2, Int64Ty, "my_ptr_int");
      Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
      Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");
      //load shadow addr
      Value *LoadSrc = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");


      BitCastInst*
        BCDest = new BitCastInst(LoadDest, 
            PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
      BitCastInst*
        BCSrc = new BitCastInst(LoadSrc, 
            PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
      Constant* False = ConstantInt::get(Int1Ty, false);
      Finish = M->getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i64", VoidTy, PtrVoidTy, PtrVoidTy, Int64Ty, Int1Ty);
      IRB.CreateCall(Finish, {BCDest, BCSrc, size, False});
    }
  }
}

void
EFTSanitizer::handleMathLibFunc (CallInst *CI,
    BasicBlock *BB,
    Function *F,
    std::string CallName) {

  Instruction *I = dyn_cast<Instruction>(CI);
  Instruction *Next = getNextInstruction(dyn_cast<Instruction>(CI), BB);
  Module *M = F->getParent();

  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());

  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());

  SmallVector<Type *, 4> ArgsTy;
  SmallVector<Value*, 8> ArgsVal;

  ConstantInt* instId = GetInstId(F, I);
  ConstantInt *lineNo = GetInstId(F, I);
  IRBuilder<> IRBO(Next);
  
  ConstantInt* insId = GetInstId(F, I);

  Value *StackTop = IRBO.CreateLoad(IRBO.getInt64Ty(), MStackTop, "my_load_stack_top_idx");  

  Constant *Num_Entries = ConstantInt::get(Int64Ty, NUM_ENTRIES);
  Value *Add = IRBO.CreateAdd(StackTop, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr_idx");
  Add = IRBO.CreateURem(Add, Num_Entries);
  IRBO.CreateStore(Add, MStackTop, "my_store");
  Value *BOGEP = IRBO.CreateGEP(ShadowSL, Add);

  Value *InstMapGep = IRBO.CreateGEP(MapIns, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRBO.CreateStore(Add, InstMapGep, "my_store");
  MInsMap.insert(std::pair<Instruction*, Instruction*>(I, dyn_cast<Instruction>(BOGEP)));
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRBO.CreateCall(SetRealTemp, {insId, Add});
  }


  IRBuilder<> IRB(Next);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);


  Value *Index1;

  size_t NumOperands = CI->getNumArgOperands();
  Value *Op[NumOperands];
  Type *OpTy[NumOperands];
  bool Op1Call[NumOperands];
  Value* ConsIdx[NumOperands];


  std::string funcName;

  if (CallName == "llvm.cos.f64") {
    funcName = "eftsan_mpfr_llvm_cos_f64";
  }
  else if (CallName == "llvm.sin.f64") {
    funcName = "eftsan_mpfr_llvm_sin_f64";
  }
  else if(CallName == "llvm.ceil.f64"){
    funcName = "eftsan_mpfr_llvm_ceil";
  }
  else if(CallName == "llvm.floor.f64"){
    funcName = "eftsan_mpfr_llvm_floor";
  }
  else if(CallName == "llvm.floor.f32"){
    funcName = "eftsan_mpfr_llvm_floor_f";
  }
  else if(CallName == "llvm.fabs.f64"){
    funcName = "eftsan_mpfr_llvm_fabs";
  }
  else if(CallName == "llvm.powi.f64"){
    funcName = "eftsan_mpfr_llvm_powi";
  }
  else if(CallName == "llvm.fabs.f32"){
    funcName = "eftsan_mpfr_llvm_fabs32";
  }
  else if(CallName == "llvm.exp.f64"){
    funcName = "eftsan_mpfr_llvm_exp64";
  }
  else if(CallName == "llvm.log.f64"){
    funcName = "eftsan_mpfr_llvm_log64";
  }
  else if(CallName == "llvm.sqrt.f64"){
    funcName = "eftsan_mpfr_llvm_sqrt64";
  }
  else {
    funcName = "eftsan_mpfr_"+CallName;
  }  

  for(int i = 0; i < NumOperands; i++){
    Op[i] = CI->getArgOperand(i);
    OpTy[i] = Op[i]->getType(); // this should be of float
    Op1Call[i] = false;
    if(isFloatType(OpTy[i])){
      bool res = handleOperand(F, CI, Op[i], &ConsIdx[i], nullptr);
      if(!res){
        errs()<<"\nError !!! metadata not found for operand:\n";
        Op[i]->dump();
        errs()<<"In Inst:"<<"\n";
        I->dump();
        exit(1);
      }

      Value *LHS = ConsIdx[i];
      ConstantInt* insId1 = GetInstId(F, Op[i]);
      Value *TSOp1 = IRB.CreateGEP(ConsIdx[i], {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
          ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
      Value *TStampOp1 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp1, "my_ts");

      Value *InstMapGep1 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
          insId1}, "my_gep");
      Value *TSInstMap1 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep1, "my_ts");

      if(DEBUG){
        Finish = M->getOrInsertFunction("eftsan_valid", VoidTy, Int64Ty, Int64Ty, Int64Ty);
        IRB.CreateCall(Finish, {TStampOp1, TSInstMap1, insId1});
      }

      Value* Cond1 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp1, TSInstMap1);
      LHS = IRB.CreateSelect(Cond1, ConsIdx[i], ConstantPointerNull::get(cast<PointerType>(MPtrTy)));
      ArgsVal.push_back(LHS);
      ArgsTy.push_back(MPtrTy);
    }
    ArgsVal.push_back(Op[i]);
    ArgsTy.push_back(OpTy[i]);
  }
  ArgsTy.push_back(MPtrTy);
  ArgsTy.push_back(CI->getType());
  ArgsTy.push_back(Int64Ty);
  ArgsTy.push_back(Int1Ty);
  ArgsTy.push_back(Int32Ty);
  ArgsTy.push_back(Int32Ty);
  ArgsTy.push_back(Int64Ty);

  ArgsVal.push_back(BOGEP);
  ArgsVal.push_back(CI);
  ArgsVal.push_back(instId);
  ArgsVal.push_back(debugInfoAvailable);
  ArgsVal.push_back(lineNumber);
  ArgsVal.push_back(colNumber);
  //load timestamp
  Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
  Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRB.CreateStore(Add, TimeStamp, "my_store_idx");
  ArgsVal.push_back(Add);

  if(NumOperands > 1)
    funcName = funcName + std::to_string(NumOperands);

  HandleFunc = M->getOrInsertFunction(funcName, FunctionType::get(IRB.getVoidTy(), ArgsTy, false));
  IRB.CreateCall(HandleFunc, ArgsVal);

  Value *InstTSMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(Add, InstTSMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, TStampOp1});
  }

  if(ClDetectExceptions){
    //detect inf
    Value *NCI = CI;
    if(isFloat(CI->getType())){
      NCI = IRBO.CreateFPExt(CI, DoubleTy, "my_op");
    }
    SetRealTemp = M->getOrInsertFunction("isinf", Int1Ty, DoubleTy);
    Value* Cond = IRBO.CreateCall(SetRealTemp, {NCI});
    BasicBlock *OldBB = I->getParent();
    BasicBlock *Cont = OldBB->splitBasicBlock(Next, "split");
    BasicBlock *NewBB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *BrInst = OldBB->getTerminator();
    BrInst->eraseFromParent();
    BranchInst *BInst = BranchInst::Create(/*ifTrue*/NewBB, /*ifFalse*/Cont, Cond, OldBB);

    IRBuilder<> IRBN(NewBB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_inf", VoidTy, MPtrTy);
    IRBN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *BJCmp = BranchInst::Create(Cont, NewBB);

    //detect nan
    IRBuilder<> IRBS(Next);
    Value* NCond = IRBS.CreateFCmp(FCmpInst::FCMP_UNO, NCI, 
        ConstantFP::get(DoubleTy, 0.0));
    FCmpList.push_back(dyn_cast<FCmpInst>(NCond));

    BasicBlock *NewCont = Cont->splitBasicBlock(Next, "split");
    BasicBlock *NewB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *NBrInst = Cont->getTerminator();
    NBrInst->eraseFromParent();
    BranchInst::Create(/*ifTrue*/NewB, /*ifFalse*/NewCont, NCond, Cont);

    IRBuilder<> IRBNN(NewB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_nan", VoidTy, MPtrTy);
    IRBNN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *NBJCmp = BranchInst::Create(NewCont, NewB);
  }
}

bool EFTSanitizer::handlePhiOperand(Value* OP, Value** ConsInsIndex1, Value** ConsInsIndex2){

  long Idx = 0;

  if(MInsMap.count(dyn_cast<Instruction>(OP)) != 0){
    *ConsInsIndex1 = MInsMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else if(ConsMap.count(OP) != 0){
    *ConsInsIndex1 = ConsMap.at(OP);
    return true;
  }
  else if(isa<Argument>(OP) && (ArgMap.count(dyn_cast<Argument>(OP)) != 0)){
    Idx =  ArgMap.at(dyn_cast<Argument>(OP));
    *ConsInsIndex1 = MArgMap.at(dyn_cast<Argument>(OP));
    return true;
  }
  else if(ExtractValueInst *EV = dyn_cast<ExtractValueInst>(OP)){
    Value *OP1 = EV->getAggregateOperand();
    unsigned idx = *EV->idx_begin();
    if(MInsMapPair.count(dyn_cast<Instruction>(OP1)) != 0){
      std::pair <Instruction*, Instruction*> Ins = MInsMapPair.at(dyn_cast<Instruction>(OP1));
      if(idx == 0){
        *ConsInsIndex1 = Ins.first;
        return true;
      }
      else if(idx == 1){
        *ConsInsIndex1 = Ins.second;
        return true;
      }
      else{
        return false;
      }
    }
    else{
      return false;
    }
  }
  else if(ExtractElementInst *EV = dyn_cast<ExtractElementInst>(OP)){
    Value *OP1 = EV->getVectorOperand();
    ConstantInt *Idx = dyn_cast<ConstantInt>(EV->getIndexOperand());
    if(MInsMapPair.count(dyn_cast<Instruction>(OP1)) != 0){
      std::pair <Instruction*, Instruction*> Ins = MInsMapPair.at(dyn_cast<Instruction>(OP1));
      if(Idx->getValue() == 0){
        *ConsInsIndex1 = Ins.first;
        return true;
      }
      else if(Idx->getValue() == 1){
        *ConsInsIndex1 = Ins.second;
        return true;
      }
      else{
        return false;
      }
    }
    else{
      return false;
    }
  }
  else if(isa<PHINode>(OP)){
    *ConsInsIndex1 = GEPMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else if(isa<Argument>(OP) && (ArgMap.count(dyn_cast<Argument>(OP)) != 0)){
    Idx =  ArgMap.at(dyn_cast<Argument>(OP));
    *ConsInsIndex1 = MArgMap.at(dyn_cast<Argument>(OP));
    return true;
  }
  else if(isa<FPTruncInst>(OP) || isa<FPExtInst>(OP)){
    Instruction *OpIns = dyn_cast<Instruction>(OP);
    Value *OP1 = OpIns->getOperand(0);
    return handlePhiOperand(OP1, ConsInsIndex1, ConsInsIndex2);
    *ConsInsIndex1 = UndefValue::get(MPtrTy);
    return true;
  }
  else if(GEPMap.count(dyn_cast<Instruction>(OP)) != 0){
    *ConsInsIndex1 = GEPMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else if(isa<UndefValue>(OP)){
    *ConsInsIndex1 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    return true;
  }
  else{
    return false;
  }
}

bool EFTSanitizer::handleOperand(Function *F, Instruction *I, Value* OP, Value** ConsInsIndex1, Value** ConsInsIndex2){

  long Idx = 0;
  Module *M = F->getParent();

  if(ConsMap.count(OP) != 0){
    *ConsInsIndex1 = ConsMap.at(OP);
    return true;
  }
  if(isa<PHINode>(OP) && MInsMap.count(dyn_cast<Instruction>(OP)) != 0){
    *ConsInsIndex1 = MInsMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else if(isa<Argument>(OP) && (ArgMap.count(dyn_cast<Argument>(OP)) != 0)){
    Idx =  ArgMap.at(dyn_cast<Argument>(OP));
    *ConsInsIndex1 = MArgMap.at(dyn_cast<Argument>(OP));
    return true;
  }
  else if(isa<UndefValue>(OP)){
    *ConsInsIndex1 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    return true;
  }
  else if(isa<CallInst>(OP) && MInsMap.count(dyn_cast<Instruction>(OP)) != 0){
    *ConsInsIndex1 = MInsMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else{
    IRBuilder<> IRB(I);
    ConstantInt* insId = GetInstId(F, OP);
    Value *InstMapGep = IRB.CreateGEP(MapIns, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId}, "my_gep");
    Value *TSInstMap = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep, "my_ins_map");
    *ConsInsIndex1 = IRB.CreateGEP(ShadowSL, TSInstMap);
    return true;
  }
  /*
  else if(MInsMapPair.count(dyn_cast<Instruction>(OP)) != 0){
    std::pair <Instruction*, Instruction*> Ins = MInsMapPair.at(dyn_cast<Instruction>(OP));
    *ConsInsIndex1 = Ins.first;
    *ConsInsIndex2 = Ins.second;
    return true;
  }
  else if(ExtractValueInst *EV = dyn_cast<ExtractValueInst>(OP)){
    Value *OP1 = EV->getAggregateOperand();
    unsigned idx = *EV->idx_begin();
    if(MInsMapPair.count(dyn_cast<Instruction>(OP1)) != 0){
      std::pair <Instruction*, Instruction*> Ins = MInsMapPair.at(dyn_cast<Instruction>(OP1));
      if(idx == 0){
        *ConsInsIndex1 = Ins.first;
        return true;
      }
      else if(idx == 1){
        *ConsInsIndex1 = Ins.second;
        return true;
      }
      else{
        return false;
      }
    }
    else{
      return false;
    }
  }
  else if(ExtractElementInst *EV = dyn_cast<ExtractElementInst>(OP)){
    Value *OP1 = EV->getVectorOperand();
    ConstantInt *Idx = dyn_cast<ConstantInt>(EV->getIndexOperand());
    if(MInsMapPair.count(dyn_cast<Instruction>(OP1)) != 0){
      std::pair <Instruction*, Instruction*> Ins = MInsMapPair.at(dyn_cast<Instruction>(OP1));
      if(Idx->getValue() == 0){
        *ConsInsIndex1 = Ins.first;
        return true;
      }
      else if(Idx->getValue() == 1){
        *ConsInsIndex1 = Ins.second;
        return true;
      }
      else{
        return false;
      }
    }
    else{
      return false;
    }
  }
  else if(isa<PHINode>(OP)){
    *ConsInsIndex1 = GEPMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else if(MInsMap.count(dyn_cast<Instruction>(OP)) != 0){
    *ConsInsIndex1 = MInsMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else if(isa<Argument>(OP) && (ArgMap.count(dyn_cast<Argument>(OP)) != 0)){
    Idx =  ArgMap.at(dyn_cast<Argument>(OP));
    *ConsInsIndex1 = MArgMap.at(dyn_cast<Argument>(OP));
    return true;
  }
  else if(isa<FPTruncInst>(OP) || isa<FPExtInst>(OP)){
    Value *OP1 = OpIns->getOperand(0);
    return handleOperand(OpIns, OP1, ConsInsIndex1, ConsInsIndex2);
    *ConsInsIndex1 = UndefValue::get(MPtrTy);
    return true;
  }
  else if(GEPMap.count(dyn_cast<Instruction>(OP)) != 0){
    *ConsInsIndex1 = GEPMap.at(dyn_cast<Instruction>(OP));
    return true;
  }
  else if(isa<UndefValue>(OP)){
    *ConsInsIndex1 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    return true;
  }
  else{
    return false;
  }
  */
}

void EFTSanitizer::storeShadowAddrCons(Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
    Function *F, ConstantInt* lineNumber){
  Module *M = F->getParent();
  IRBuilder<> IRB(I);
  IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
  IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());

  Value *PtrToInt = IRB.CreatePtrToInt(Addr, Int64Ty, "my_ptr_int");
  Value *Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
  Value *Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");
  Value *Val = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");

  ConstantInt *instId = GetInstId(F, I);
  ConstantInt *lineNo = GetInstId(F, I);
  //store err
  Value *ValGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  //Set error to 0 for constant
  Value *Err = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));

  //store computed
  if(isFloat(OP->getType())){
    OP = IRB.CreateFPExt(OP, DoubleTy, "my_op");
  }
  Value *CGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
  Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRB.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));

  Value *InstTSMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  instId}, "my_gep");
  IRB.CreateStore(Add, InstTSMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot_store", VoidTy, Int64Ty, Int64Ty);
    IRB.CreateCall(SetRealTemp, {instId, Add});
  }
  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 0); //Constant
    Value *OpCGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    //Value *StoreIns4 = IRB.CreateStore(lineNumber, InstIdGep, "my_store");
    Value *StoreIns4 = IRB.CreateStore(instId, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));


    //store lhs
    Value *Op1 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstLHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(Op1, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *Op2 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstRHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(Op2, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));
  }
}

void EFTSanitizer::storeShadowAddrArg(Value *srcVal, Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
    Function *F, ConstantInt* lineNumber){
  Module *M = F->getParent();
  IRBuilder<> IRB(I);
  IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
  IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  ConstantInt* insId = GetInstId(F, OP);

  if(isFloat(OP->getType())){
    OP = IRB.CreateFPExt(OP, DoubleTy, "my_op");
  }
  ConstantInt* instId;
  if(isa<Argument>(OP)){
    instId = ConstantInt::get(Type::getInt64Ty(M->getContext()), 0);
  }
  else{
    instId = GetInstId(F, dyn_cast<Instruction>(OP));
  }
  Value *PtrToInt = IRB.CreatePtrToInt(Addr, Int64Ty, "my_ptr_int");
  Value *Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
  Value *Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");
  Value *Val = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");

  //store err
  Value *ValGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  //Load the error from the metadata 
  Value *ErrGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");
  Value *Err = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), ErrGep, "my_load");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));
  //store computed
  Value *CGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");

  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *TS = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *TStamp = IRB.CreateLoad(IRB.getInt64Ty(), TS, "my_ts");

  Value *InstTS = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(TStamp, InstTS, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));

  //save ts in instid map
  Value *InstMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(TStamp, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, TStamp});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 

    //load opcode
    Value *OpGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");
    Value *OpCode = IRB.CreateLoad(Type::getInt32Ty(M->getContext()), OpGep, "my_load");

    Value *OpCGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    //load lineno from metadata
    Value *LineNoGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *LineNo = IRB.CreateLoad(Type::getInt64Ty(M->getContext()), LineNoGep, "my_load");
    Value *StoreIns4 = IRB.CreateStore(LineNo, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));


    //store lhs
    //load from metadata
    Value *LHSGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *LHS = IRB.CreateLoad(RealPtr, LHSGep, "my_load");
    Value *InstLHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(LHS, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *RHSGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *RHS = IRB.CreateLoad(RealPtr, RHSGep, "my_load");
    Value *InstRHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));

    if(DEBUG){
      Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));
      SetRealTemp = M->getOrInsertFunction("eftsan_handle_store", VoidTy, MPtrTy, PtrVoidTy, Int64Ty);
      IRB.CreateCall(SetRealTemp, {Val, Addr, LineNo});
    }
  }
}

void EFTSanitizer::storeShadowAddrBO(Value *srcVal, Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
    Function *F, ConstantInt* lineNumber){
  Module *M = F->getParent();
  IRBuilder<> IRB(I);
  IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
  IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());

  ConstantInt* insId = GetInstId(F, OP);
  ConstantInt* instId = GetInstId(F, dyn_cast<Instruction>(OP));
  ConstantInt *lineNo = GetInstId(F, dyn_cast<Instruction>(OP));
  if(isFloat(OP->getType())){
    OP = IRB.CreateFPExt(OP, DoubleTy, "my_op");
  }

  Value *PtrToInt = IRB.CreatePtrToInt(Addr, Int64Ty, "my_ptr_int");
  Value *Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
  Value *Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");
  Value *Val = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");

  //store err
  Value *ValGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  //Load the error from the metadata 
  Value *ErrGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");
  Value *Err = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), ErrGep, "my_load");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));
  //store computed
  Value *CGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *TS = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *TStamp = IRB.CreateLoad(IRB.getInt64Ty(), TS, "my_ts");

  Value *InstTS = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(TStamp, InstTS, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));

  //save ts in instid map
  Value *InstMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(TStamp, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, TStamp});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    size_t op = 0;
    if (BinaryOperator* BO = dyn_cast<BinaryOperator>(Ins)){
      switch(BO->getOpcode()) {                                                                                                    
        case Instruction::FAdd:
          op = 1;
          break;
        case Instruction::FSub:
          op = 2;
          break;
        case Instruction::FMul:
          op = 3;
          break;
        case Instruction::FDiv:
          op = 4;
          break;
      }
    }
    else if (UnaryOperator *UO = dyn_cast<UnaryOperator>(Ins)) {
      switch (UO->getOpcode()) {
        case Instruction::FNeg: 
          op = 2;
          break;
      }
    }

    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), op);
    Value *OpCGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    //Value *StoreIns4 = IRB.CreateStore(lineNumber, InstIdGep, "my_store");
    Value *StoreIns4 = IRB.CreateStore(instId, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));

    //store lhs
    Value *LHSGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *LHS = IRB.CreateLoad(RealPtr, LHSGep, "my_load");
    Value *InstLHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(LHS, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *RHSGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *RHS = IRB.CreateLoad(RealPtr, RHSGep, "my_load");
    Value *InstRHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));

  }
  //debug
  Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_handle_store", VoidTy, MPtrTy, PtrVoidTy, Int64Ty);
    IRB.CreateCall(SetRealTemp, {Val, Addr, lineNumber});
  }

  SetRealTemp = M->getOrInsertFunction("eftsan_trace", VoidTy, MPtrTy);
//  IRB.CreateCall(SetRealTemp, {Val});
}

//fptrunc, fpext
void EFTSanitizer::storeShadowAddr(Value *srcVal, Instruction *I, Value *OP, Value *Addr, BasicBlock *BB,
    Function *F, ConstantInt* lineNumber){
  Module *M = F->getParent();
  IRBuilder<> IRB(I);
  IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
  IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());

  ConstantInt* insId = GetInstId(F, OP);
  if(isFloat(OP->getType())){
    OP = IRB.CreateFPExt(OP, DoubleTy, "my_op");
  }

  Value *PtrToInt = IRB.CreatePtrToInt(Addr, Int64Ty, "my_ptr_int");
  Value *Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
  Value *Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");
  Value *Val = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");


  //store err
  Value *ValGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  //Load the error from the metadata 
  Value *ErrGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");
  Value *Err = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), ErrGep, "my_load");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));

  //store computed
  Value *CGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *TS = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *TStamp = IRB.CreateLoad(IRB.getInt64Ty(), TS, "my_ts");

  Value *InstTS = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(TStamp, InstTS, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));

  //save ts in instid map
  Value *InstMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(TStamp, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, TStamp});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 

    //load opcode
    Value *OpGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");
    Value *OpCode = IRB.CreateLoad(Type::getInt32Ty(M->getContext()), OpGep, "my_load");

    Value *OpCGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    //load lineno from metadata
    Value *LineNoGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *LineNo = IRB.CreateLoad(Type::getInt64Ty(M->getContext()), LineNoGep, "my_load");
    Value *StoreIns4 = IRB.CreateStore(LineNo, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));


    //store lhs
    Value *LHSGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *LHS = IRB.CreateLoad(RealPtr, LHSGep, "my_load");
    Value *InstLHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(LHS, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *RHSGep = IRB.CreateGEP(srcVal, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *RHS = IRB.CreateLoad(RealPtr, RHSGep, "my_load");
    Value *InstRHSGep = IRB.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));


    Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));
    if(DEBUG){
      SetRealTemp = M->getOrInsertFunction("eftsan_handle_store", VoidTy, MPtrTy, PtrVoidTy, Int64Ty);
      IRB.CreateCall(SetRealTemp, {Val, Addr, LineNo});
    }
  }
}

void EFTSanitizer::handleStore(StoreInst *SI, BasicBlock *BB, Function *F){
  if (std::find(StoreList.begin(), StoreList.end(), SI) != StoreList.end()) {
    return;
  }
  Instruction *I = dyn_cast<Instruction>(SI);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(I);
  Module *M = F->getParent();

  LLVMContext &C = F->getContext();
  Value *Addr = SI->getPointerOperand();
  Value *OP = SI->getOperand(0);

  Type *StoreTy = I->getOperand(0)->getType();
  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());
  IntegerType* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());

  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));

  ConstantInt* instId = GetInstId(F, I);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int64Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);

  bool BTFlag = false;
  Type *OpTy = OP->getType();
  //TODO: do we need to check for bitcast for store?
  BitCastInst*
    BCToAddr = new BitCastInst(Addr, 
        PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
  if (BitCastInst *BI = dyn_cast<BitCastInst>(Addr)){
    BTFlag = checkIfBitcastFromFP(BI);
  }
  if((StoreTy->isVectorTy() && 
     isFloatType(StoreTy->getScalarType()))){
    VectorType *VT = dyn_cast<VectorType>(StoreTy);
    unsigned NumElems = VT->getNumElements();
    if(NumElems != 2){
      errs()<<"Vec type not uspported\n";
      errs()<<*SI<<"\n";
 //     exit(1);
      return;
    }
    IRBuilder<> IRBB(I);
    Value *Gep1 = IRBB.CreateGEP(Addr, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");
    Value *Gep2 = IRBB.CreateGEP(Addr, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
    BitCastInst* BCToAddr1 = new BitCastInst(Gep1, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);

    BitCastInst* BCToAddr2 = new BitCastInst(Gep2, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);

    Value *V1 = IRBB.CreateExtractElement(OP, ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "my_extract");
    Value *V2 = IRBB.CreateExtractElement(OP, ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "my_extract");
    handleStr(OP, V1, I, BCToAddr1, BB, F, lineNumber);
    handleStr(OP, V2, I, BCToAddr2, BB, F, lineNumber);
  }
  else if(isFloatType(StoreTy)){
    Value *Flag = ConstantInt::get(Int1Ty, false);
    Value *Size;
    if(ClTracing){
      Size = ConstantInt::get(Int64Ty, 56);
    }
    else{
      Size = ConstantInt::get(Int64Ty, 24);
    }

    if(isa<ConstantFP>(OP)){
      storeShadowAddrCons(I, OP, BCToAddr, BB, F, lineNumber);
    }
    else{
      Value *InsIndex;
      bool res = handleOperand(F, I, OP, &InsIndex, nullptr);
      //get destination in shadow memory
      Value *PtrToInt = IRB.CreatePtrToInt(Addr, Int64Ty, "my_ptr_int");
      Value *Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
      Value *Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");
      Value *Val = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");
      BitCastInst* Dest = new BitCastInst(Val, 
          PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);

      BitCastInst* Src = new BitCastInst(InsIndex, 
          PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
      Finish = M->getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i64", VoidTy, PtrVoidTy, PtrVoidTy, Int64Ty, Int1Ty);
      CallInst *CInst = IRB.CreateCall(Finish, {Dest, Src, Size, Flag});
      MemcpyNList.push_back(CInst);
      if(DEBUG){
        SetRealTemp = M->getOrInsertFunction("eftsan_handle_store", VoidTy, MPtrTy, MPtrTy);
        IRB.CreateCall(SetRealTemp, {Val, InsIndex});
      }
    }
  }
}

void EFTSanitizer::handleStr(Value *OP, Value *EVal, Instruction *I, Value *BCToAddr, 
    BasicBlock *BB, Function *F, ConstantInt* lineNumber){
    Value* InsIndex;
    bool res = handleOperand(F, I, EVal, &InsIndex, nullptr);
    if(res){ //handling registers
       if(isa<Argument>(OP) || isa<CallInst>(OP)){
        storeShadowAddrArg(InsIndex, I, EVal, BCToAddr, BB, F, lineNumber);
       }
       else if(isa<BinaryOperator>(OP) || isa<UnaryOperator>(OP)){
        storeShadowAddrBO(InsIndex, I, EVal, BCToAddr, BB, F, lineNumber);
       }
       else if(isa<ConstantFP>(OP)){
        storeShadowAddrCons(I, EVal, BCToAddr, BB, F, lineNumber);
       }
       else{
        storeShadowAddr(InsIndex, I, EVal, BCToAddr, BB, F, lineNumber);
       }
    }
    else{
      storeShadowAddrCons(I, EVal, BCToAddr, BB, F, lineNumber);
    }
}

void EFTSanitizer::handleNewPhi(Function *F){
  Module *M = F->getParent();
  Instruction* Next;
  long NumPhi = 0;
  BasicBlock *IBB, *BB;
  for(auto it = NewPhiMap.begin(); it != NewPhiMap.end(); ++it)
  {
    if(PHINode *PN = dyn_cast<PHINode>(it->first)){
      PHINode* iPHI = dyn_cast<PHINode>(it->second);
      for (unsigned PI = 0, PE = PN->getNumIncomingValues(); PI != PE; ++PI) {
        IBB = PN->getIncomingBlock(PI);
        Value *IncValue = PN->getIncomingValue(PI);
        BB = PN->getParent();

        if (IncValue == PN) continue; //TODO
        Value* InsIndex;
        bool res = handlePhiOperand(IncValue, &InsIndex, nullptr);
        if(!res){
          errs()<<"handleNewPhi:Error !!! metadata not found for operand:\n";
          IncValue->dump();
          errs()<<"In Inst:"<<"\n";
          it->first->dump();
          exit(1);
        }
        
        iPHI->addIncoming(InsIndex, IBB);
      }
    }
  }
}

void EFTSanitizer::handlePhi(PHINode *PN, BasicBlock *BB, Function *F){
  Instruction *I = dyn_cast<Instruction>(PN);
  Module *M = F->getParent();
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());

  Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));
  IRBuilder<> IRB(dyn_cast<Instruction>(dyn_cast<Instruction>(PN)));

  PHINode* iPHI = IRB.CreatePHI (MPtrTy, 2);
  // Wherever old phi node has been used, we need to replace it with
  //new phi node. That's why need to track it and keep it in RegIdMap
 
  NewPhiMap.insert(std::pair<Instruction*, Instruction*>(dyn_cast<Instruction>(PN), iPHI));

  Instruction* Next;
  Next = getNextInstructionNotPhi(PN, BB);
  //create call to runtime to copy 
  //  Next++;
  IRBuilder<> IRBN(Next);

  //Value *BOGEP = GEPMap.at(PN);
  Value *StackTop = IRBN.CreateLoad(IRBN.getInt64Ty(), MStackTop, "my_load_stack_top_idx");  

  Constant *Num_Entries = ConstantInt::get(Int64Ty, NUM_ENTRIES);
  Value *Add = IRBN.CreateAdd(StackTop, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr_idx");
  Add = IRBN.CreateURem(Add, Num_Entries);
  IRBN.CreateStore(Add, MStackTop, "my_store");
  Value *BOGEP = IRBN.CreateGEP(ShadowSL, Add);

  MInsMap.insert(std::pair<Instruction*, Instruction*>(I, dyn_cast<Instruction>(BOGEP)));
  ConstantInt* insId = GetInstId(F, PN);

  AddFunArg = M->getOrInsertFunction("eftsan_copy_phi", VoidTy, MPtrTy, MPtrTy, Int64Ty);
//  IRBN.CreateCall(AddFunArg, {iPHI, BOGEP, Add});
  Value *Size;
  if(ClTracing){
    Size = ConstantInt::get(Int64Ty, 56);
  }
  else{
    Size = ConstantInt::get(Int64Ty, 24);
  }

  Value *Flag = ConstantInt::get(Int1Ty, false);
  BitCastInst* Dest = new BitCastInst(BOGEP, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", Next);
  BitCastInst* Src = new BitCastInst(iPHI, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", Next);
  Finish = M->getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i64", VoidTy, PtrVoidTy, PtrVoidTy, Int64Ty, Int1Ty);
  CallInst *CInst = IRBN.CreateCall(Finish, {Dest, Src, Size, Flag});
  MemcpyNList.push_back(CInst);

  Value *TS = IRBN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *TStamp = IRBN.CreateLoad(IRB.getInt64Ty(), TS, "my_ts");

  Value *InstTSMapGep = IRBN.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
      insId}, "my_gep");
  IRBN.CreateStore(TStamp, InstTSMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, TStamp});
  }
} 

void EFTSanitizer::handleSelect(SelectInst *SI, BasicBlock *BB, Function *F){
  Instruction *I = dyn_cast<Instruction>(SI);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Value *OP1 = SI->getOperand(0);

  Value* InsIndex2, *InsIndex3;
  bool res1 = handleOperand(F, SI, SI->getOperand(1), &InsIndex2, nullptr);
  if(!res1){
    errs()<<"\nhandleSelect: Error !!! metadata not found for op:"<<"\n";
    SI->getOperand(1)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  bool res2 = handleOperand(F, SI, SI->getOperand(2), &InsIndex3, nullptr);
  if(!res2){
    errs()<<"\nhandleSelect: Error !!! metadata not found for op:"<<"\n";
    SI->getOperand(1)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }

  Value *NewOp2 = dyn_cast<Value>(InsIndex2); 
  Value *NewOp3 = dyn_cast<Value>(InsIndex3);

  Type* FCIOpType = SI->getOperand(0)->getType();

  Value *Select = IRB.CreateSelect(OP1, NewOp2, NewOp3); 
  Instruction *NewIns = dyn_cast<Instruction>(Select);
  MInsMap.insert(std::pair<Instruction*, Instruction*>(I, NewIns));

}

void EFTSanitizer::handleReturn(ReturnInst *RI, BasicBlock *BB, Function *F){

  Instruction *Ins = dyn_cast<Instruction>(RI);
  Module *M = F->getParent();

  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());

  Value* OpIdx;
  //Find first mpfr clear
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&I)){
        Function *Callee = CI->getCalledFunction();
        if(Callee && Callee->getName() == "eftsan_clear_mpfr"){
          Ins = &I;
          break;
        }
      }
    }
  }

  IRBuilder<> IRB(Ins);
  if (RI->getNumOperands() != 0){
    Value *OP = RI->getOperand(0);
    if(isFloatType(OP->getType())){
      bool res = handleOperand(F, RI, OP, &OpIdx, nullptr);
      if(!res){
        errs()<<"\nhandleReturn: Error !!! metadata not found for op:"<<"\n";
        OP->dump();
        errs()<<"In Inst:"<<"\n";
        OP->dump();
        exit(1);
      }
      long TotalArgs = FuncTotalArg.at(F);
      long TotalIns = FuncTotalIns.at(F);
      Constant *Num_Entries = ConstantInt::get(Int64Ty, NUM_ENTRIES);
      Value *StackTop = IRB.CreateLoad(IRB.getInt64Ty(), MStackTop, "my_load_stack_top");  
      Value *Add = IRB.CreateAdd(StackTop, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr_idx");
      Add = IRB.CreateURem(Add, Num_Entries);
      IRB.CreateStore(Add, MStackTop, "my_store");
      
      Value *Dest = IRB.CreateGEP(ShadowSL, Add);

      if(DEBUG){
        SetRealTemp = M->getOrInsertFunction("eftsan_index", VoidTy, Int64Ty, MPtrTy);
        IRB.CreateCall(SetRealTemp, {StackTop, Dest});
      }
      AddFunArg = M->getOrInsertFunction("eftsan_set_return", VoidTy, MPtrTy, MPtrTy, Int64Ty);
      IRB.CreateCall(AddFunArg, {Dest, OpIdx, Add});
      return;
    }
    else if(OP->getType()->isVectorTy() && 
        isFloatType(OP->getType()->getScalarType())){ //vector
      //considering size 2 vector
      Value *V1 = IRB.CreateExtractElement(OP, ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "my_extract");
      Value *V2 = IRB.CreateExtractElement(OP, ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "my_extract");
      bool res1 = handleOperand(F, RI, V1, &OpIdx, nullptr);
      if(!res1){
        errs()<<"\nhandleReturn: Error !!! metadata not found for op:"<<"\n";
        OP->dump();
        errs()<<"In Inst:"<<"\n";
        OP->dump();
        exit(1);
      }
      Value *StackTop = IRB.CreateLoad(IRB.getInt64Ty(), MStackTop, "my_load_stack_top");  
      long TotalArgs = FuncTotalArg.at(F);
      long TotalIns = FuncTotalIns.at(F);
      Constant* TotalArgsConst = ConstantInt::get(Type::getInt64Ty(M->getContext()), TotalArgs + TotalIns); 
      AddFunArg = M->getOrInsertFunction("eftsan_set_return", VoidTy, MPtrTy, Int64Ty, Int64Ty);
      IRB.CreateCall(AddFunArg, {OpIdx, TotalArgsConst, StackTop});

      bool res2 = handleOperand(F, RI, V2, &OpIdx, nullptr);
      if(!res2){
        errs()<<"\nhandleReturn: Error !!! metadata not found for op:"<<"\n";
        OP->dump();
        errs()<<"In Inst:"<<"\n";
        OP->dump();
        exit(1);
      }

      AddFunArg = M->getOrInsertFunction("eftsan_set_return_vec", VoidTy, MPtrTy, Int64Ty, Int64Ty);
      IRB.CreateCall(AddFunArg, {OpIdx, TotalArgsConst, StackTop});
      return;
    }
  }
  FuncInit = M->getOrInsertFunction("eftsan_func_exit", VoidTy, Int64Ty);
  long TotalArgs = FuncTotalArg.at(F);
  Constant* ConsTotIns = ConstantInt::get(Type::getInt64Ty(M->getContext()), TotalArgs); 
//  IRB.CreateCall(FuncInit, {ConsTotIns});
}

void EFTSanitizer::handleFNeg(UnaryOperator *UO, BasicBlock *BB, Function *F) {
  Instruction *I = dyn_cast<Instruction>(UO);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRBI(I);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Value *InsIndex1, *InsIndex2;
  Value *Op1 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);

  bool res2 = handleOperand(F, UO, UO->getOperand(0), &InsIndex2, nullptr);
  if (!res2) {
    errs() << *F << "\n";
    errs() << "handleBinOp: Error !!! metadata not found for op:"
      << "\n";
    errs() << *UO->getOperand(0);
    errs() << "In Inst:"
      << "\n";
    errs() << *I;
    exit(1);
  }
  Type *VoidTy = Type::getVoidTy(M->getContext());
  IntegerType *Int32Ty = Type::getInt32Ty(M->getContext());
  IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());

  ConstantInt *instId = GetInstId(F, I);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0)
      debugInfoAvail = false;
  }
  ConstantInt *lineNumber = ConstantInt::get(Int32Ty, lineNum);

  //Value *BOGEP = GEPMap.at(I);
  ConstantInt* insId = GetInstId(F, I);

  Value *StackTop = IRBI.CreateLoad(IRBI.getInt64Ty(), MStackTop, "my_load_stack_top_idx");  

  Constant *Num_Entries = ConstantInt::get(Int64Ty, NUM_ENTRIES);
  Value *Add = IRBI.CreateAdd(StackTop, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr_idx");
  Add = IRBI.CreateURem(Add, Num_Entries);
  IRBI.CreateStore(Add, MStackTop, "my_store");
  Value *BOGEP = IRBI.CreateGEP(ShadowSL, Add);

  Value *InstMapGep = IRBI.CreateGEP(MapIns, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRBI.CreateStore(Add, InstMapGep, "my_store");
  MInsMap.insert(std::pair<Instruction*, Instruction*>(I, dyn_cast<Instruction>(BOGEP)));
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, Add});
  }

  std::string opName(I->getOpcodeName());

  ComputeReal = M->getOrInsertFunction("eftsan_mpfr_fneg", VoidTy, MPtrTy, MPtrTy, Int32Ty);

  InsIndex1 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
  TwoSubFNeg(I, Op1, I->getOperand(0), I, InsIndex1, InsIndex2, BOGEP);
}

void EFTSanitizer::handleBinOpVec(BinaryOperator* BO, BasicBlock *BB, Function *F){
  Instruction *I = dyn_cast<Instruction>(BO);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(Next);
  IRBuilder<> IRBB(I);
  Module *M = F->getParent();

  Value* InsIndex11, *InsIndex12, *InsIndex21, *InsIndex22; 
  bool res1 = handleOperand(F, BO, BO->getOperand(0), &InsIndex11, &InsIndex12);
  if(!res1){
    errs()<<"handleBinOp: Error !!! metadata not found for op:"<<"\n";
    BO->getOperand(0)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }

  bool res2 = handleOperand(F, BO, BO->getOperand(1), &InsIndex21, &InsIndex22);
  if(!res2){
    errs()<<"handleBinOp: Error !!! metadata not found for op:"<<"\n";
    BO->getOperand(1)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  Type* BOType = BO->getOperand(0)->getType();
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());

  ConstantInt* instId = GetInstId(F, I);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);

  std::pair <Value*, Value*> V = GEPMapPair.at(I);
  Value *BOGEP1 = V.first;
  Value *BOGEP2 = V.second;

  std::string opName(I->getOpcodeName());

  Value *Shadow_Val;

  Value *OP1_1 = IRB.CreateExtractElement(BO->getOperand(0), ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "my_extract");
  Value *OP1_2 = IRB.CreateExtractElement(BO->getOperand(0), ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "my_extract");
  Value *OP2_1 = IRB.CreateExtractElement(BO->getOperand(1), ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "my_extract");
  Value *OP2_2 = IRB.CreateExtractElement(BO->getOperand(1), ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "my_extract");
  Value *V1 = IRB.CreateExtractElement(BO, ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "my_extract");
  Value *V2 = IRB.CreateExtractElement(BO, ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "my_extract");
  if(BO->getOpcode() == Instruction::FAdd) {
    //TwoSum(I, OP1_1, OP2_1, V1, InsIndex11, InsIndex21, BOGEP1);
    //TwoSum(I, OP1_2, OP2_2, V2, InsIndex12, InsIndex22, BOGEP2);
  }
  else if(BO->getOpcode() == Instruction::FSub){
    //TwoSub(I, OP1_1, OP2_1, V1, InsIndex11, InsIndex21, BOGEP1);
    //TwoSub(I, OP1_2, OP2_2, V2, InsIndex12, InsIndex22, BOGEP2);
  }
  else if(BO->getOpcode() == Instruction::FMul){
    //TwoMul(I, OP1_1, OP2_1, V1, InsIndex11, InsIndex21, BOGEP1);
    //TwoMul(I, OP1_2, OP2_2, V2, InsIndex12, InsIndex22, BOGEP2);
  }
  else if(BO->getOpcode() == Instruction::FDiv){
    //TwoDiv(I, OP1_1, OP2_1, V1, InsIndex11, InsIndex21, BOGEP1);
    //TwoDiv(I, OP1_2, OP2_2, V2, InsIndex12, InsIndex22, BOGEP2);
  }
  /*
  else if(BO->getOpcode() == Instruction::FRem){
    //load timestamp
    Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
    Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
    IRB.CreateStore(Add, TimeStamp, "my_store_idx");

    Value *Op1Load = getLoadOperandLHS(BO->getOperand(0));
    Value *Op2Load = getLoadOperandRHS(BO->getOperand(1));

    ComputeReal = M->getOrInsertFunction("eftsan_mpfr_fmod2", VoidTy, MPtrTy, MPtrTy, 
        BO->getOperand(0)->getType(), MPtrTy, MPtrTy,
        BO->getOperand(1)->getType(), MPtrTy, BO->getType(), Int64Ty,
        Int1Ty, Int32Ty, Int32Ty, Int64Ty);

    IRB.CreateCall(ComputeReal, {InsIndex1, Op1Load, BO->getOperand(0),
        InsIndex2, Op2Load, BO->getOperand(1),
        BOGEP, BO, instId, debugInfoAvailable, lineNumber,
        colNumber, Add});
  }
  */
  MInsMapPair.insert(std::pair<Instruction*, std::pair<Instruction*, Instruction*>>(I, 
          std::make_pair(dyn_cast<Instruction>(BOGEP1), dyn_cast<Instruction>(BOGEP2))));
}

void EFTSanitizer::handleBinOp(BinaryOperator* BO, BasicBlock *BB, Function *F){
  Instruction *I = dyn_cast<Instruction>(BO);
  Module *M = F->getParent();

  Value* InsIndex1, *InsIndex2; 
  bool res1 = handleOperand(F, BO, BO->getOperand(0), &InsIndex1, nullptr);
  if(!res1){
    errs()<<"handleBinOp: Error !!! metadata not found for op:"<<"\n";
    BO->getOperand(0)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }

  bool res2 = handleOperand(F, BO, BO->getOperand(1), &InsIndex2, nullptr);
  if(!res2){
    errs()<<"handleBinOp: Error !!! metadata not found for op:"<<"\n";
    BO->getOperand(1)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  Type* BOType = BO->getOperand(0)->getType();
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());

  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);

  //Value *BOGEP = GEPMap.at(I);
  IRBuilder<> IRB(I);
  ConstantInt* insId = GetInstId(F, I);

  Value *StackTop = IRB.CreateLoad(IRB.getInt64Ty(), MStackTop, "my_load_stack_top_idx");  

  Constant *Num_Entries = ConstantInt::get(Int64Ty, NUM_ENTRIES);
  Value *Add = IRB.CreateAdd(StackTop, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr_idx");
  Add = IRB.CreateURem(Add, Num_Entries);
  IRB.CreateStore(Add, MStackTop, "my_store");
  Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);

  Value *InstMapGep = IRB.CreateGEP(MapIns, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(Add, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, Add});
  }
  MInsMap.insert(std::pair<Instruction*, Instruction*>(I, dyn_cast<Instruction>(BOGEP)));

  std::string opName(I->getOpcodeName());

  Value *Shadow_Val;
  if(BO->getType()->isVectorTy()){ //vector
  }
  else{
    if(BO->getOpcode() == Instruction::FAdd) {
      TwoSum(I, BO->getOperand(0), BO->getOperand(1), BO, InsIndex1, InsIndex2, BOGEP, Add);
    }
    else if(BO->getOpcode() == Instruction::FSub){
      TwoSub(I, BO->getOperand(0), BO->getOperand(1), BO, InsIndex1, InsIndex2, BOGEP, Add);
    }
    else if(BO->getOpcode() == Instruction::FMul){
      TwoMul(I, BO->getOperand(0), BO->getOperand(1), BO, InsIndex1, InsIndex2, BOGEP, Add);
    }
    else if(BO->getOpcode() == Instruction::FDiv){
      TwoDiv(I, BO->getOperand(0), BO->getOperand(1), BO, InsIndex1, InsIndex2, BOGEP, Add);
    }
    else if(BO->getOpcode() == Instruction::FRem){
      Instruction *Next = getNextInstruction(I, BB);
      IRBuilder<> IRB(Next);
      //load timestamp
      Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
      Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
      IRB.CreateStore(Add, TimeStamp, "my_store_idx");

      ComputeReal = M->getOrInsertFunction("eftsan_mpfr_fmod2", VoidTy, MPtrTy, MPtrTy, 
          BO->getOperand(0)->getType(), MPtrTy, MPtrTy,
          BO->getOperand(1)->getType(), MPtrTy, BO->getType(), Int64Ty,
          Int1Ty, Int32Ty, Int32Ty, Int64Ty);

      IRB.CreateCall(ComputeReal, {InsIndex1, BO->getOperand(0),
          InsIndex2, BO->getOperand(1),
          BOGEP, BO, instId, debugInfoAvailable, lineNumber,
          colNumber, Add});
    }
  }
}

/*
double two_sum(double a, double b, double x){
  double z = x - a;
  double y1 = (b - z);
  double y2 = (x - z);
  double y3 = (a - y2);
  double y = y3 + y1;

  //add errors from operands
  double newErr = y + a.err + b.err;
  return newErr;
}
*/                                   
void EFTSanitizer::TwoSum(Instruction *I,
                           Value *Op1a,
                           Value *Op2b,
                           Value *Res,
                           Value *InsIndex1,
                           Value *InsIndex2, 
                           Value *BOGEP,
                           Value *idx){

  Module *M = I->getParent()->getParent()->getParent();
  Function *F = I->getParent()->getParent();
  Instruction *Next = getNextInstruction(I, I->getParent());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());

  ConstantInt* insId1 = GetInstId(F, Op1a);
  ConstantInt* insId2 = GetInstId(F, Op2b);
  ConstantInt* insId = GetInstId(F, I);
  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);
  IRBuilder<> IRBO(dyn_cast<Instruction>(Res)->getNextNode());

  if(isFloat(Res->getType())){
    Op1a = IRBO.CreateFPExt(Op1a, DoubleTy, "my_op");
    Op2b = IRBO.CreateFPExt(Op2b, DoubleTy, "my_op");
    Res = IRBO.CreateFPExt(Res, DoubleTy, "my_op");
  }


  IRBuilder<> IRB(dyn_cast<Instruction>(Res)->getNextNode());

  //check nan
 //  Value* Cond = IRB.CreateFCmp(FCmpInst::FCMP_UNE, Res, Res);
  Value *BinOpZ = IRB.CreateBinOp(Instruction::FSub, Res, Op1a, "my_op" );
  Value *BinOpY1 = IRB.CreateBinOp(Instruction::FSub, Op2b, BinOpZ, "my_op" );
  Value *BinOpY2 = IRB.CreateBinOp(Instruction::FSub, Res, BinOpZ, "my_op" );
  Value *BinOpY3 = IRB.CreateBinOp(Instruction::FSub, Op1a, BinOpY2, "my_op" );
  Value *BinOpY = IRB.CreateBinOp(Instruction::FAdd, BinOpY3, BinOpY1, "my_op" );
  
  Value *OP;
  if(isFloat(I->getType())){
    OP = IRB.CreateFPExt(Res, DoubleTy, "my_op");
  }
  else{
    OP = Res;
  }

  //Now add error with operands error
  Value *Indices[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)};
   Value *L1, *L2;
  if(isa<ConstantFP>(Op1a)){
    L1 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op1 = IRB.CreateGEP(InsIndex1, Indices, "my_gep");
    L1 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op1, "my_load");
  }


  if(isa<ConstantFP>(Op2b)){
    L2 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op2 = IRB.CreateGEP(InsIndex2, Indices, "my_gep");
    L2 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op2, "my_load");
  }


  Value *Err1 = IRB.CreateBinOp(Instruction::FAdd, BinOpY, L1, "my_op_err");
  Value *Err = IRB.CreateBinOp(Instruction::FAdd, Err1, L2, "my_op_err");

  //Store err
  Value *ValGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));

  //store computed
  Value *CGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
  Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRB.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));


  Value *InstMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(Add, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, Add});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 1);
    Value *OpCGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *StoreIns4 = IRB.CreateStore(idx, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));

    //store lhs

    Value *TSOp1 = IRB.CreateGEP(InsIndex1, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp1 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp1, "my_ts");

    Value *InstMapGep1 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId1}, "my_gep");
    Value *TSInstMap1 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep1, "my_ts");

    if(DEBUG){
      Finish = M->getOrInsertFunction("eftsan_valid", VoidTy, Int64Ty, Int64Ty, Int64Ty);
      IRB.CreateCall(Finish, {TStampOp1, TSInstMap1, insId1});
    }

    Value* Cond1 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp1, TSInstMap1);
    Value *LHS = IRB.CreateSelect(Cond1, InsIndex1, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));
    //Value *LHS = InsIndex1;

    Value *InstLHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(LHS, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *TSOp2 = IRB.CreateGEP(InsIndex2, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp2 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp2, "my_ts");

    Value *InstMapGep2 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId2}, "my_gep");
    Value *TSInstMap2 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep2, "my_ts");


    if(DEBUG){
      Finish = M->getOrInsertFunction("eftsan_valid", VoidTy, Int64Ty, Int64Ty, Int64Ty);
      IRB.CreateCall(Finish, {TStampOp2, TSInstMap2, insId2});
    }

    Value* Cond2 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp2, TSInstMap2);
    Value *RHS = IRB.CreateSelect(Cond2, InsIndex2, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));
    //  Value *RHS = InsIndex2;

    Value *InstRHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));
  }
  if(ClDetectAllRoundingErrors){
    SetRealTemp = M->getOrInsertFunction("eftsan_sum", VoidTy, DoubleTy, DoubleTy, DoubleTy,
        MPtrTy, MPtrTy, MPtrTy, Int64Ty, DoubleTy, DoubleTy);
    IRB.CreateCall(SetRealTemp, {Res, Op1a, Op2b, BOGEP, InsIndex1, InsIndex2, instId, BinOpY, Err});
  }

  if(ClDetectExceptions){
    //detect inf
    SetRealTemp = M->getOrInsertFunction("isinf", Int1Ty, DoubleTy);
    Value* Cond = IRB.CreateCall(SetRealTemp, {Res});
    BasicBlock *OldBB = I->getParent();
    BasicBlock *Cont = OldBB->splitBasicBlock(Next, "split");
    BasicBlock *NewBB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *BrInst = OldBB->getTerminator();
    BrInst->eraseFromParent();
    BranchInst *BInst = BranchInst::Create(/*ifTrue*/NewBB, /*ifFalse*/Cont, Cond, OldBB);

    IRBuilder<> IRBN(NewBB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_inf", VoidTy, MPtrTy);
    IRBN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *BJCmp = BranchInst::Create(Cont, NewBB);

    //detect nan
    IRBuilder<> IRBS(dyn_cast<Instruction>(Res)->getNextNode());
    Value* NCond = IRBS.CreateFCmp(FCmpInst::FCMP_UNO, Res, 
        ConstantFP::get(DoubleTy, 0.0));
    FCmpList.push_back(dyn_cast<FCmpInst>(NCond));

    BasicBlock *NewCont = Cont->splitBasicBlock(Next, "split");
    BasicBlock *NewB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *NBrInst = Cont->getTerminator();
    NBrInst->eraseFromParent();
    BranchInst::Create(/*ifTrue*/NewB, /*ifFalse*/NewCont, NCond, Cont);

    IRBuilder<> IRBNN(NewB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_nan", VoidTy, MPtrTy);
    IRBNN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *NBJCmp = BranchInst::Create(NewCont, NewB);
  }
}

void EFTSanitizer::TwoSubFNeg(Instruction *I,
                           Value *Op1a,
                           Value *Op2b,
                           Value *Res,
                           Value *InsIndex1,
                           Value *InsIndex2,
                           Value *BOGEP){

  Module *M = I->getParent()->getParent()->getParent();
  Function *F = I->getParent()->getParent();
  IRBuilder<> IRB(dyn_cast<Instruction>(Res)->getNextNode());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());

  if(isFloat(Res->getType())){
    Op1a = IRB.CreateFPExt(Op1a, DoubleTy, "my_op");
    Op2b = IRB.CreateFPExt(Op2b, DoubleTy, "my_op");
    Res = IRB.CreateFPExt(Res, DoubleTy, "my_op");
  }

  Value *OP;
  if(isFloat(I->getType())){
    OP = IRB.CreateFPExt(Res, DoubleTy, "my_op");
  }
  else{
    OP = Res;
  }

  ConstantInt* insId2 = GetInstId(F, Op2b);
  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);
  
  Value *NOp2b = IRB.CreateUnOp(Instruction::FNeg, Op2b, "my_op" );
  Value *BinOpZ = IRB.CreateBinOp(Instruction::FSub, Res, Op1a, "my_op");
  Value *BinOpY1 = IRB.CreateBinOp(Instruction::FSub, NOp2b, BinOpZ, "my_op");
  Value *BinOpY2 = IRB.CreateBinOp(Instruction::FSub, Res, BinOpZ, "my_op");
  Value *BinOpY3 = IRB.CreateBinOp(Instruction::FSub, Op1a, BinOpY2, "my_op");
  Value *BinOpY = IRB.CreateBinOp(Instruction::FAdd, BinOpY3, BinOpY1, "my_op");

  //Now add error with operands error
  Value *Indices[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)};
   Value *L1, *L2;
  if(isa<ConstantFP>(Op1a)){
    L1 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op1 = IRB.CreateGEP(InsIndex1, Indices, "my_gep");
    L1 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op1, "my_load");
  }


  if(isa<ConstantFP>(Op2b)){
    L2 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op2 = IRB.CreateGEP(InsIndex2, Indices, "my_gep");
    L2 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op2, "my_load");
  }

  Value *Err1 = IRB.CreateBinOp(Instruction::FAdd, BinOpY, L1, "my_op");
  Value *Err = IRB.CreateBinOp(Instruction::FAdd, Err1, L2, "my_op");

  //Store err
  Value *ValGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));

  //store computed
  Value *CGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
  Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRB.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 2);
    Value *OpCGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *StoreIns4 = IRB.CreateStore(instId, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));

    //store lhs
    Value *InstLHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(InsIndex1, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *TSOp2 = IRB.CreateGEP(InsIndex2, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp2 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp2, "my_ts");

    Value *InstMapGep2 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId2}, "my_gep");
    Value *TSInstMap2 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep2, "my_ts");

    Value* Cond2 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp2, TSInstMap2);
    Value *RHS = IRB.CreateSelect(Cond2, InsIndex2, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));

    Value *InstRHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));
  }

  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_sub", VoidTy, DoubleTy, DoubleTy, DoubleTy,
                                              MPtrTy, MPtrTy, MPtrTy, Int64Ty, DoubleTy, DoubleTy);
    //IRB.CreateCall(SetRealTemp, {Res, Op1a, Op2b, BOGEP, InsIndex1, InsIndex2, instId, BinOpY, Err});
  }
}

void EFTSanitizer::TwoSub(Instruction *I,
                           Value *Op1a,
                           Value *Op2b,
                           Value *Res,
                           Value *InsIndex1,
                           Value *InsIndex2,
                           Value *BOGEP,
                           Value *idx){

  Module *M = I->getParent()->getParent()->getParent();
  Function *F = I->getParent()->getParent();
  Instruction *Next = getNextInstruction(I, I->getParent());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  Type* Int1Ty = Type::getInt1Ty(M->getContext());

  ConstantInt* insId1 = GetInstId(F, Op1a);
  ConstantInt* insId2 = GetInstId(F, Op2b);
  ConstantInt* insId = GetInstId(F, I);
  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);

  IRBuilder<> IRBO(dyn_cast<Instruction>(Res)->getNextNode());

  if(isFloat(Res->getType())){
    Op1a = IRBO.CreateFPExt(Op1a, DoubleTy, "my_op");
    Op2b = IRBO.CreateFPExt(Op2b, DoubleTy, "my_op");
    Res = IRBO.CreateFPExt(Res, DoubleTy, "my_op");
  }

  IRBuilder<> IRB(dyn_cast<Instruction>(Res)->getNextNode());

  Value *OP;
  if(isFloat(I->getType())){
    OP = IRB.CreateFPExt(Res, DoubleTy, "my_op");
  }
  else{
    OP = Res;
  }

  Value *NOp2b = IRB.CreateUnOp(Instruction::FNeg, Op2b, "my_op" );
  Value *BinOpZ = IRB.CreateBinOp(Instruction::FSub, Res, Op1a, "my_op");
  Value *BinOpY1 = IRB.CreateBinOp(Instruction::FSub, NOp2b, BinOpZ, "my_op");
  Value *BinOpY2 = IRB.CreateBinOp(Instruction::FSub, Res, BinOpZ, "my_op");
  Value *BinOpY3 = IRB.CreateBinOp(Instruction::FSub, Op1a, BinOpY2, "my_op");
  Value *BinOpY = IRB.CreateBinOp(Instruction::FAdd, BinOpY3, BinOpY1, "my_op");

  //Now add error with operands error
  Value *Indices[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)};
   Value *L1, *L2;
  if(isa<ConstantFP>(Op1a)){
    L1 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op1 = IRB.CreateGEP(InsIndex1, Indices, "my_gep");
    L1 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op1, "my_load");
  }


  if(isa<ConstantFP>(Op2b)){
    L2 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op2 = IRB.CreateGEP(InsIndex2, Indices, "my_gep");
    L2 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op2, "my_load");
  }

  Value *Err1 = IRB.CreateBinOp(Instruction::FAdd, BinOpY, L1, "my_op");
  Value *Err = IRB.CreateBinOp(Instruction::FAdd, Err1, L2, "my_op");

  //Store err
  Value *ValGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));

  //store computed
  Value *CGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
  Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRB.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));


  Value *InstMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(Add, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, Add});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 2);
    Value *OpCGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *StoreIns4 = IRB.CreateStore(idx, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));

    //store lhs
    Value *TSOp1 = IRB.CreateGEP(InsIndex1, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp1 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp1, "my_ts");

    Value *InstMapGep1 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId1}, "my_gep");
    Value *TSInstMap1 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep1, "my_ts");

    Value* Cond1 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp1, TSInstMap1);
    Value *LHS = IRB.CreateSelect(Cond1, InsIndex1, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));

    Value *InstLHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(LHS, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *TSOp2 = IRB.CreateGEP(InsIndex2, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp2 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp2, "my_ts");

    Value *InstMapGep2 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId2}, "my_gep");
    Value *TSInstMap2 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep2, "my_ts");

    Value* Cond2 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp2, TSInstMap2);
    Value *RHS = IRB.CreateSelect(Cond2, InsIndex2, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));

    Value *InstRHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));

    if(DEBUG){
      Finish = M->getOrInsertFunction("eftsan_valid", VoidTy, Int64Ty, Int64Ty, Int64Ty);
      IRB.CreateCall(Finish, {TStampOp1, TSInstMap1, insId1});
      Finish = M->getOrInsertFunction("eftsan_valid", VoidTy, Int64Ty, Int64Ty, Int64Ty);
      IRB.CreateCall(Finish, {TStampOp2, TSInstMap2, insId2});
    }
  }

  if(ClDetectAllRoundingErrors){
    SetRealTemp = M->getOrInsertFunction("eftsan_sub", VoidTy, DoubleTy, DoubleTy, DoubleTy,
                                              MPtrTy, MPtrTy, MPtrTy, Int64Ty, DoubleTy, DoubleTy);
    IRB.CreateCall(SetRealTemp, {Res, Op1a, Op2b, BOGEP, InsIndex1, InsIndex2, instId, BinOpY, Err});
  }

  if(ClDetectExceptions){
    //detect inf
    SetRealTemp = M->getOrInsertFunction("isinf", Int1Ty, DoubleTy);
    Value* Cond = IRB.CreateCall(SetRealTemp, {Res});
    BasicBlock *OldBB = I->getParent();
    BasicBlock *Cont = OldBB->splitBasicBlock(Next, "split");
    BasicBlock *NewBB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *BrInst = OldBB->getTerminator();
    BrInst->eraseFromParent();
    BranchInst *BInst = BranchInst::Create(/*ifTrue*/NewBB, /*ifFalse*/Cont, Cond, OldBB);

    IRBuilder<> IRBN(NewBB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_inf", VoidTy, MPtrTy);
    IRBN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *BJCmp = BranchInst::Create(Cont, NewBB);

    //detect nan
    IRBuilder<> IRBS(dyn_cast<Instruction>(Res)->getNextNode());
    Value* NCond = IRBS.CreateFCmp(FCmpInst::FCMP_UNO, Res, 
        ConstantFP::get(DoubleTy, 0.0));
    FCmpList.push_back(dyn_cast<FCmpInst>(NCond));

    BasicBlock *NewCont = Cont->splitBasicBlock(Next, "split");
    BasicBlock *NewB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *NBrInst = Cont->getTerminator();
    NBrInst->eraseFromParent();
    BranchInst::Create(/*ifTrue*/NewB, /*ifFalse*/NewCont, NCond, Cont);

    IRBuilder<> IRBNN(NewB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_nan", VoidTy, MPtrTy);
    IRBNN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *NBJCmp = BranchInst::Create(NewCont, NewB);
  }
}

/*
double two_product(double a, double b, double x){
  double y = fma(a, b, -x);
  double err1 = a*b.err;
  double err2 = b*a.err;
  double err3 = err1 + err2;
  double err = y + err3;
  return err;
}
*/
void EFTSanitizer::TwoMul(Instruction *I,
                           Value *Op1a,
                           Value *Op2b,
                           Value *Res,
                           Value *InsIndex1,
                           Value *InsIndex2,
                           Value *BOGEP,
                           Value *idx){
  Module *M = I->getParent()->getParent()->getParent();
  Function *F = I->getParent()->getParent();
  Instruction *Next = getNextInstruction(I, I->getParent());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());

  IRBuilder<> IRBO(dyn_cast<Instruction>(Res)->getNextNode());
  ConstantInt* insId1 = GetInstId(F, Op1a);
  ConstantInt* insId2 = GetInstId(F, Op2b);
  ConstantInt* insId = GetInstId(F, I);
  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);
  if(isFloat(Res->getType())){
    Op1a = IRBO.CreateFPExt(Op1a, DoubleTy, "my_op");
    Op2b = IRBO.CreateFPExt(Op2b, DoubleTy, "my_op");
    Res = IRBO.CreateFPExt(Res, DoubleTy, "my_op");
  }


  IRBuilder<> IRB(dyn_cast<Instruction>(Res)->getNextNode());

  Value *OP;
  if(isFloat(I->getType())){
    OP = IRB.CreateFPExt(Res, DoubleTy, "my_op");
  }
  else{
    OP = Res;
  }

  Value *NRes = IRB.CreateUnOp(Instruction::FNeg, Res, "my_op");
  Value *Ops[] = { Op1a, Op2b, NRes};
  Value *BinOpY = IRB.CreateCall(Intrinsic::getDeclaration(M, Intrinsic::fma, DoubleTy), Ops, "my_op");

  //Add error
  Value *Indices[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)};
  Value *L1, *L2;
  if(isa<ConstantFP>(Op1a)){
    L1 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op1 = IRB.CreateGEP(InsIndex1, Indices, "my_gep");
    L1 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op1, "my_load");
  }


  if(isa<ConstantFP>(Op2b)){
    L2 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op2 = IRB.CreateGEP(InsIndex2, Indices, "my_gep");
    L2 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op2, "my_load");
  }
  Value *Err1 = IRB.CreateBinOp(Instruction::FMul, Op1a, L2, "my_op");
  Value *Err2 = IRB.CreateBinOp(Instruction::FMul, Op2b, L1, "my_op");
  Value *Err3 = IRB.CreateBinOp(Instruction::FAdd, Err1, Err2, "my_op");
  Value *Err = IRB.CreateBinOp(Instruction::FAdd, BinOpY, Err3, "my_op");

  //Store err
  Value *ValGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));

  //store computed
  Value *CGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
  Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRB.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));


  Value *InstMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(Add, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, Add});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 3);
    Value *OpCGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *StoreIns4 = IRB.CreateStore(idx, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));

    //store lhs
    Value *TSOp1 = IRB.CreateGEP(InsIndex1, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp1 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp1, "my_ts");

    Value *InstMapGep1 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId1}, "my_gep");
    Value *TSInstMap1 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep1, "my_ts");

    Value* Cond1 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp1, TSInstMap1);
    Value *LHS = IRB.CreateSelect(Cond1, InsIndex1, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));
    Value *InstLHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(LHS, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *TSOp2 = IRB.CreateGEP(InsIndex2, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp2 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp2, "my_ts");

    Value *InstMapGep2 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId2}, "my_gep");
    Value *TSInstMap2 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep2, "my_ts");

    Value* Cond2 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp2, TSInstMap2);
    Value *RHS = IRB.CreateSelect(Cond2, InsIndex2, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));
    Value *InstRHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));

  }
  if(ClDetectAllRoundingErrors){
    SetRealTemp = M->getOrInsertFunction("eftsan_mul", VoidTy, DoubleTy, DoubleTy, DoubleTy,
                                              MPtrTy, MPtrTy, MPtrTy, Int64Ty, DoubleTy, DoubleTy);
    IRB.CreateCall(SetRealTemp, {Res, Op1a, Op2b, BOGEP, InsIndex1, InsIndex2, instId, BinOpY, Err});
  }
  if(ClDetectExceptions){
    //detect inf
    SetRealTemp = M->getOrInsertFunction("isinf", Int1Ty, DoubleTy);
    Value* Cond = IRB.CreateCall(SetRealTemp, {Res});
    BasicBlock *OldBB = I->getParent();
    BasicBlock *Cont = OldBB->splitBasicBlock(Next, "split");
    BasicBlock *NewBB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *BrInst = OldBB->getTerminator();
    BrInst->eraseFromParent();
    BranchInst *BInst = BranchInst::Create(/*ifTrue*/NewBB, /*ifFalse*/Cont, Cond, OldBB);

    IRBuilder<> IRBN(NewBB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_inf", VoidTy, MPtrTy);
    IRBN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *BJCmp = BranchInst::Create(Cont, NewBB);

    //detect nan
    IRBuilder<> IRBS(dyn_cast<Instruction>(Res)->getNextNode());
    Value* NCond = IRBS.CreateFCmp(FCmpInst::FCMP_UNO, Res, 
        ConstantFP::get(DoubleTy, 0.0));
    FCmpList.push_back(dyn_cast<FCmpInst>(NCond));

    BasicBlock *NewCont = Cont->splitBasicBlock(Next, "split");
    BasicBlock *NewB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *NBrInst = Cont->getTerminator();
    NBrInst->eraseFromParent();
    BranchInst::Create(/*ifTrue*/NewB, /*ifFalse*/NewCont, NCond, Cont);

    IRBuilder<> IRBNN(NewB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_nan", VoidTy, MPtrTy);
    IRBNN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *NBJCmp = BranchInst::Create(NewCont, NewB);
  }
}

/*
double two_div(double a, double b, double x){
  double y = -fma(b, x, -a);
  double err1 = b + b.err;
  double err2 = y + a.err;
  double err3 = x * b.err;
  double err4 = err2 - err3;
  double err = err4 / err1;
  return err;
}
*/
void EFTSanitizer::TwoDiv(Instruction *I, 
                           Value *Op1a, 
                           Value *Op2b,
                           Value *Res, 
                           Value *InsIndex1, 
                           Value *InsIndex2,
                           Value *BOGEP,
                           Value *idx){
  Module *M = I->getParent()->getParent()->getParent();
  Function *F = I->getParent()->getParent();
  Instruction *Next = getNextInstruction(I, I->getParent());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());

  IRBuilder<> IRBO(dyn_cast<Instruction>(Res)->getNextNode());
  IRBuilder<> IRBC(I);
  ConstantInt* insId1 = GetInstId(F, Op1a);
  ConstantInt* insId2 = GetInstId(F, Op2b);
  ConstantInt* insId = GetInstId(F, I);
  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);

  if(isFloat(Res->getType())){
    Op1a = IRBO.CreateFPExt(Op1a, DoubleTy, "my_op");
    Op2b = IRBO.CreateFPExt(Op2b, DoubleTy, "my_op");
    Res = IRBO.CreateFPExt(Res, DoubleTy, "my_op");
  }

  IRBuilder<> IRB(dyn_cast<Instruction>(Res)->getNextNode());

  Value *OP;
  if(isFloat(I->getType())){
    OP = IRB.CreateFPExt(Res, DoubleTy, "my_op");
  }
  else{
    OP = Res;
  }

  Value *NOp1a = IRB.CreateUnOp(Instruction::FNeg, Op1a, "my_op");

  Value *Ops[] = { Op2b, Res, NOp1a};
  Value *BinOpY = IRB.CreateCall(Intrinsic::getDeclaration(M, Intrinsic::fma, Ops[0]->getType()), Ops, "my_op");

  //Add error
  Value *Indices[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)};
  Value *L1, *L2;
  if(isa<ConstantFP>(Op1a)){
    L1 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op1 = IRB.CreateGEP(InsIndex1, Indices, "my_gep");
    L1 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op1, "my_load");
  }

  if(isa<ConstantFP>(Op2b)){
    L2 = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);
  }
  else{
    Value *Op2 = IRB.CreateGEP(InsIndex2, Indices, "my_gep");
    L2 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op2, "my_load");
  }

  Value *Err1 = IRB.CreateBinOp(Instruction::FAdd, Op2b, L2, "my_op");
  Value *Err2 = IRB.CreateBinOp(Instruction::FAdd, BinOpY, L1, "my_op");
  Value *Err3 = IRB.CreateBinOp(Instruction::FMul, Res, L2, "my_op");
  Value *Err4 = IRB.CreateBinOp(Instruction::FSub, Err2, Err3, "my_op");
  Value *Err = IRB.CreateBinOp(Instruction::FDiv, Err4, Err1, "my_op");

  //Store err
  Value *ValGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));

  //store computed
  Value *CGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
  Value *StoreIns0 = IRB.CreateStore(OP, CGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns0));

  //store timestamp
  Value *GTimeStamp = IRB.CreateLoad(IRB.getInt64Ty(), TimeStamp, "my_ts");
  Value *Add = IRB.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRB.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRB.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));


  Value *InstMapGep = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(Add, InstMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot", VoidTy, Int64Ty, Int64Ty);
//    IRB.CreateCall(SetRealTemp, {insId, Add});
  }

  if(ClTracing){
    //store op
    Instruction *Ins = dyn_cast<Instruction>(I->getOperand(0)); 
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 4);
    Value *OpCGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *StoreIns4 = IRB.CreateStore(idx, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));

    //store lhs
    Value *TSOp1 = IRB.CreateGEP(InsIndex1, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp1 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp1, "my_ts");

    Value *InstMapGep1 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId1}, "my_gep");
    Value *TSInstMap1 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep1, "my_ts");

    Value* Cond1 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp1, TSInstMap1);
    Value *LHS = IRB.CreateSelect(Cond1, InsIndex1, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));
    Value *InstLHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(LHS, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *TSOp2 = IRB.CreateGEP(InsIndex2, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
    Value *TStampOp2 = IRB.CreateLoad(IRB.getInt64Ty(), TSOp2, "my_ts");

    Value *InstMapGep2 = IRB.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
        insId2}, "my_gep");
    Value *TSInstMap2 = IRB.CreateLoad(IRB.getInt64Ty(), InstMapGep2, "my_ts");

    Value* Cond2 = IRB.CreateICmp(ICmpInst::ICMP_EQ, TStampOp2, TSInstMap2);
    Value *RHS = IRB.CreateSelect(Cond2, InsIndex2, ConstantPointerNull::get(cast<PointerType>(MPtrTy)));
    Value *InstRHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(RHS, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));
  }

  if(ClDetectAllRoundingErrors){
    SetRealTemp = M->getOrInsertFunction("eftsan_div", VoidTy, DoubleTy, DoubleTy, DoubleTy,
        MPtrTy, MPtrTy, MPtrTy, Int64Ty, DoubleTy, DoubleTy);
    IRB.CreateCall(SetRealTemp, {Res, Op1a, Op2b, BOGEP, InsIndex1, InsIndex2, instId, BinOpY, Err});
  }
  if(ClDetectExceptions){
    //detect inf
    SetRealTemp = M->getOrInsertFunction("isinf", Int1Ty, DoubleTy);
    Value* Cond = IRB.CreateCall(SetRealTemp, {Res});
    BasicBlock *OldBB = I->getParent();
    BasicBlock *Cont = OldBB->splitBasicBlock(Next, "split");
    BasicBlock *NewBB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *BrInst = OldBB->getTerminator();
    BrInst->eraseFromParent();
    BranchInst *BInst = BranchInst::Create(/*ifTrue*/NewBB, /*ifFalse*/Cont, Cond, OldBB);

    IRBuilder<> IRBN(NewBB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_inf", VoidTy, MPtrTy);
    IRBN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *BJCmp = BranchInst::Create(Cont, NewBB);

    //detect nan
    IRBuilder<> IRBS(dyn_cast<Instruction>(Res)->getNextNode());
    Value* NCond = IRBS.CreateFCmp(FCmpInst::FCMP_UNO, Res, 
        ConstantFP::get(DoubleTy, 0.0));
    FCmpList.push_back(dyn_cast<FCmpInst>(NCond));

    BasicBlock *NewCont = Cont->splitBasicBlock(Next, "split");
    BasicBlock *NewB = BasicBlock::Create(M->getContext(), "error", F);
    Instruction *NBrInst = Cont->getTerminator();
    NBrInst->eraseFromParent();
    BranchInst::Create(/*ifTrue*/NewB, /*ifFalse*/NewCont, NCond, Cont);

    IRBuilder<> IRBNN(NewB);
    SetRealTemp = M->getOrInsertFunction("eftsan_report_nan", VoidTy, MPtrTy);
    IRBNN.CreateCall(SetRealTemp, {BOGEP});
    BranchInst *NBJCmp = BranchInst::Create(NewCont, NewB);
  }
}

#if 0
double EFTSqrt(double op, double res)
{
  double error = -std::fma(res, res, -op);
  return error;
}

void EFTSanitizer::EFTSqrt(Instruction *I, 
                           Value *Op1a, 
                           Value *Res, 
                           Value *InsIndex1, 
                           Value *InsIndex2,
                           Value *BOGEP){
  Module *M = I->getParent()->getParent()->getParent();
  IRBuilder<> IRB(I->getNextNode());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  Type* Int64Ty = Type::getInt64Ty(M->getContext());

  Value *NOp1a = IRB.CreateUnOp(Instruction::FNeg, Op1a, "my_op");

  Value *Ops[] = { Res, Res, NOp1a};
  Value *BinOpY = IRB.CreateCall(Intrinsic::getDeclaration(M, Intrinsic::fma, Ops[0]->getType()), Ops, "my_op");

  //Add error
  Value *Indices[] = {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)};
  Value *Op1 = IRB.CreateGEP(InsIndex1, Indices, "my_gep");
  Value *L1 = IRB.CreateLoad(Type::getDoubleTy(M->getContext()), Op1, "my_load");

  Value *Err1 = IRB.CreateBinOp(Instruction::FAdd, BinOpY, L1, "my_op");
  Value *Err2 = IRB.CreateBinOp(Instruction::FAdd, Res, Res, "my_op");
  Value *Err = IRB.CreateBinOp(Instruction::FDiv, Err1, Err2, "my_op");

  //Store err
  Value *ValGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

  Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));
}
#endif

void EFTSanitizer::handleFPTrunc(FPTruncInst *FPT, BasicBlock *BB, Function *F){

  Instruction *I = dyn_cast<Instruction>(FPT);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Value *InsIndex1;
  bool res1 = handleOperand(F, FPT, FPT->getOperand(0), &InsIndex1, nullptr);
  if(!res1){
    errs()<<"handleFPTrunc: Error !!! metadata not found for op:"<<"\n";
    FPT->getOperand(0)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  Type* VoidTy = Type::getVoidTy(M->getContext());

  CheckBranch = M->getOrInsertFunction("eftsan_handle_fptrunc", VoidTy, FPT->getType(), MPtrTy);
//  IRB.CreateCall(CheckBranch, {FPT, InsIndex1});
}

void EFTSanitizer::handleFPToUI(FPToUIInst *FI, BasicBlock *BB, Function *F){
  Instruction *I = dyn_cast<Instruction>(FI);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Value *InsIndex1, *InsIndex2;
  bool res1 = handleOperand(F, FI, FI->getOperand(0), &InsIndex1, nullptr);
  if(!res1){
    errs()<<"handleFPToUI: Error !!! metadata not found for op:"<<"\n";
    FI->getOperand(0)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  Type* FCIOpType = FI->getOperand(0)->getType();
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());

  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);

  CheckBranch = M->getOrInsertFunction("eftsan_check_conv_ui", VoidTy, FI->getType(), MPtrTy);
  IRB.CreateCall(CheckBranch, {FI, InsIndex1});
}

void EFTSanitizer::handleFPToSI(FPToSIInst *FI, BasicBlock *BB, Function *F){
  Instruction *I = dyn_cast<Instruction>(FI);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Value *InsIndex1, *InsIndex2;
  bool res1 = handleOperand(F, FI, FI->getOperand(0), &InsIndex1, nullptr);
  if(!res1){
    errs()<<"handleFPToSI: Error !!! metadata not found for op:"<<"\n";
    FI->getOperand(0)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  Type* FCIOpType = FI->getOperand(0)->getType();
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());

  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);

  CheckBranch = M->getOrInsertFunction("eftsan_check_conv_si", VoidTy, FI->getType(), MPtrTy);
  IRB.CreateCall(CheckBranch, {FI, InsIndex1});
}

void EFTSanitizer::handleFcmp(FCmpInst *FCI, BasicBlock *BB, Function *F){

  if (std::find(FCmpList.begin(), FCmpList.end(), FCI) != FCmpList.end()) {
    return;
  }
  Instruction *I = dyn_cast<Instruction>(FCI);
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(Next);
  Module *M = F->getParent();

  Value *Op1 = FCI->getOperand(0);
  Value *Op2 = FCI->getOperand(1);
  Value *InsIndex1, *InsIndex2;
  bool res1 = handleOperand(F, FCI, Op1, &InsIndex1, nullptr);
  if(!res1){
    errs()<<"handleFcmp: Error !!! metadata not found for op:"<<"\n";
    FCI->getOperand(0)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  bool res2 = handleOperand(F, FCI, Op2, &InsIndex2, nullptr);
  if(!res2){
    errs()<<"handleFcmp: Error !!! metadata not found for op:"<<"\n";
    FCI->getOperand(1)->dump();
    errs()<<"In Inst:"<<"\n";
    I->dump();
    exit(1);
  }
  Type* FCIOpType = FCI->getOperand(0)->getType();
  Type* Int64Ty = Type::getInt64Ty(M->getContext());
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());

  ConstantInt* instId = GetInstId(F, I);
  ConstantInt* lineNo = GetInstId(F, I);
  const DebugLoc &instDebugLoc = I->getDebugLoc();
  bool debugInfoAvail = false;;
  unsigned int lineNum = 0;
  unsigned int colNum = 0;
  if (instDebugLoc) {
    debugInfoAvail = true;
    lineNum = instDebugLoc.getLine();
    colNum = instDebugLoc.getCol();
    if (lineNum == 0 && colNum == 0) debugInfoAvail = false;
  }

  ConstantInt* debugInfoAvailable = ConstantInt::get(Int1Ty, debugInfoAvail);
  ConstantInt* lineNumber = ConstantInt::get(Int32Ty, lineNum);
  ConstantInt* colNumber = ConstantInt::get(Int32Ty, colNum);

  Constant* OpCode = ConstantInt::get(Type::getInt64Ty(M->getContext()), FCI->getPredicate());
  if(isFloat(FCIOpType)){
    if(isa<ConstantFP>(Op1) && !isa<ConstantFP>(Op2)){
      CheckBranch = M->getOrInsertFunction("eftsan_check_branch_f1", Int1Ty, FCIOpType, FCIOpType, 
          MPtrTy, Int64Ty, Int1Ty, Int32Ty);
      IRB.CreateCall(CheckBranch, {FCI->getOperand(0), FCI->getOperand(1), InsIndex2, 
          OpCode, I, lineNumber});
    }
    else if(!isa<ConstantFP>(Op1) && isa<ConstantFP>(Op2)){
      CheckBranch = M->getOrInsertFunction("eftsan_check_branch_f2", Int1Ty, FCIOpType, MPtrTy, FCIOpType, 
          Int64Ty, Int1Ty, Int32Ty);
      IRB.CreateCall(CheckBranch, {FCI->getOperand(0), InsIndex1, FCI->getOperand(1), 
          OpCode, I, lineNumber});
    }
    else{
      CheckBranch = M->getOrInsertFunction("eftsan_check_branch_f", Int1Ty, FCIOpType, MPtrTy, FCIOpType, 
          MPtrTy, Int64Ty, Int1Ty, Int32Ty);
      IRB.CreateCall(CheckBranch, {FCI->getOperand(0), InsIndex1, FCI->getOperand(1), InsIndex2, 
          OpCode, I, lineNumber});
    }
  }
  else if(isDouble(FCIOpType)){
    if(isa<ConstantFP>(Op1) && !isa<ConstantFP>(Op2)){
      CheckBranch = M->getOrInsertFunction("eftsan_check_branch_d1", Int1Ty, FCIOpType, FCIOpType, 
          MPtrTy, Int64Ty, Int1Ty, Int32Ty);
      IRB.CreateCall(CheckBranch, {FCI->getOperand(0), FCI->getOperand(1), InsIndex2, 
          OpCode, I, lineNumber});
    }
    else if(!isa<ConstantFP>(Op1) && isa<ConstantFP>(Op2)){
      CheckBranch = M->getOrInsertFunction("eftsan_check_branch_d2", Int1Ty, FCIOpType, MPtrTy, FCIOpType, 
          Int64Ty, Int1Ty, Int32Ty);
      IRB.CreateCall(CheckBranch, {FCI->getOperand(0), InsIndex1, FCI->getOperand(1), 
          OpCode, I, lineNumber});
    }
    else{
      CheckBranch = M->getOrInsertFunction("eftsan_check_branch_d", Int1Ty, FCIOpType, MPtrTy, FCIOpType, 
          MPtrTy, Int64Ty, Int1Ty, Int32Ty);
      IRB.CreateCall(CheckBranch, {FCI->getOperand(0), InsIndex1, FCI->getOperand(1), InsIndex2, 
          OpCode, I, lineNumber});
    }
  }
}

bool EFTSanitizer::checkIfBitcastFromFP(BitCastInst *BI){
  bool BTFlag = false;
  Type *BITy = BI->getOperand(0)->getType();
  //check if load operand is bitcast and bitcast operand is float type
  if(isFloatType(BITy)){
    BTFlag = true;
  }
  //check if load operand is bitcast and bitcast operand is struct type.	
  //check if struct has any member of float type
  else if(BITy->getPointerElementType()->getTypeID() == Type::StructTyID){
    StructType *STyL = cast<StructType>(BITy->getPointerElementType());
    int num = STyL->getNumElements();
    for(int i = 0; i < num; i++) {
      if(isFloatType(STyL->getElementType(i)))
        BTFlag = true;
    }
  }
  return BTFlag;
}

Value* EFTSanitizer::loadShadowAddr(Instruction *I, Value *Addr, BasicBlock *BB,
    Function *F){

  Module *M = F->getParent();
  IRBuilder<> IRB(I);
  IRBuilder<> IRBI(I->getNextNode());
  IntegerType *Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
  IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  Type* DoubleTy = Type::getDoubleTy(M->getContext());
  Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(M->getContext()));

  Value *PtrToInt = IRB.CreatePtrToInt(Addr, Int64Ty, "my_ptr_int");
  Value *Addr1 = IRB.CreateLShr(PtrToInt, ConstantInt::get(Type::getInt64Ty(M->getContext()), 2), "my_ls");
  Value *Addr2 = IRB.CreateAnd(Addr1, ConstantInt::get(Type::getInt64Ty(M->getContext()), 67108863), "my_and");

  //load shadow addr
  Value *Val = IRB.CreateGEP(LoadSMem, Addr2, "my_gep");

  ConstantInt* insId = GetInstId(F, I);
  Value *Add;
  //get location in TMS
  Value *StackTop = IRB.CreateLoad(IRB.getInt64Ty(), MStackTop, "my_load_stack_top_idx");  

  Constant *Num_Entries = ConstantInt::get(Int64Ty, NUM_ENTRIES);
  Add = IRB.CreateAdd(StackTop, ConstantInt::get(Int64Ty, 1), "my_stack_top_incr_idx");
  Add = IRB.CreateURem(Add, Num_Entries);
  IRB.CreateStore(Add, MStackTop, "my_store");
  Value *BOGEP = IRB.CreateGEP(ShadowSL, Add);
  MInsMap.insert(std::pair<Instruction*, Instruction*>(I, dyn_cast<Instruction>(BOGEP)));

  Value *InstMapGep2 = IRB.CreateGEP(MapIns, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRB.CreateStore(Add, InstMapGep2, "my_store");

  Value *LoadV = I;
  if(isFloat(I->getType())){
    LoadV = IRBI.CreateFPExt(I, DoubleTy, "my_op");
  }

  //load computed val and check if equal, if not then reset
  Value* ValGep = IRBI.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");

  Value *Computed = IRBI.CreateLoad(DoubleTy, ValGep, "my_load");
  Value* Cond = IRBI.CreateFCmp(FCmpInst::FCMP_UNE, Computed, LoadV);
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_load_reset", VoidTy, DoubleTy, DoubleTy, Int1Ty);
    IRBI.CreateCall(SetRealTemp, {Computed, LoadV, Cond});
  }
  FCmpList.push_back(dyn_cast<FCmpInst>(Cond));

  Instruction *Next = getNextInstruction(dyn_cast<Instruction>(Cond), BB);
  BasicBlock *NewCont = BB->splitBasicBlock(Next, "split");
  BasicBlock *NewB = BasicBlock::Create(M->getContext(), "reset", F);
  Instruction *NBrInst = BB->getTerminator();
  NBrInst->eraseFromParent();
  BranchInst::Create(/*ifTrue*/NewB, /*ifFalse*/NewCont, Cond, BB);

  //Reset
  IRBuilder<> IRBNN(NewB);
  //Set error to 0 for constant
  Value* ErrGep = IRBNN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");
  Value *Err = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);

  Value *StoreIns = IRBNN.CreateStore(Err, ErrGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns));


  //Set computed
  ValGep = IRBNN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");

  Value *StoreIns1 = IRBNN.CreateStore(LoadV, ValGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns1));

  //store timestamp
  Value *GTimeStamp = IRBNN.CreateLoad(IRBNN.getInt64Ty(), TimeStamp, "my_ts");
  Add = IRBNN.CreateAdd(GTimeStamp, ConstantInt::get(Int64Ty, 1), "my_incr_idx");
  IRBNN.CreateStore(Add, TimeStamp, "my_store_idx");

  Value *TSGep = IRBNN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
                      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *StoreIns7 = IRBNN.CreateStore(Add, TSGep, "my_store");
  StoreList.push_back(dyn_cast<StoreInst>(StoreIns7));

  Value *InstTSMapGep = IRBNN.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRBNN.CreateStore(Add, InstTSMapGep, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot_load_reset", VoidTy, Int64Ty, Int64Ty);
    IRBNN.CreateCall(SetRealTemp, {insId, Add});
  }

  if(ClTracing){
    //store op
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 0); //Constant
    Value *OpCGep = IRBNN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");

    Value *StoreIns3 = IRBNN.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    ConstantInt *instId = GetInstId(F, I);
    ConstantInt *lineNo = GetInstId(F, I);
    Value *InstIdGep = IRBNN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *StoreIns4 = IRBNN.CreateStore(instId, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));


    //store lhs
    Value *Op1Lhs = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstLHSGep = IRBNN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns5 = IRBNN.CreateStore(Op1Lhs, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *Op2Rhs = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstRHSGep = IRBNN.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 6)}, "my_gep");
    Value *StoreIns6 = IRBNN.CreateStore(Op2Rhs, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));
  }

  BranchInst *NBJCmp = BranchInst::Create(NewCont, NewB);

  BasicBlock::iterator BBit = NewCont->begin();
  Instruction *First = &*BBit;
  IRBuilder<> IRBO(First);
  Value *TS = IRBO.CreateGEP(Val, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
      ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");
  Value *TStamp = IRBO.CreateLoad(IRB.getInt64Ty(), TS, "my_ts");

  //save ts in instid map
  Value *InstTSMapGep1 = IRBO.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRBO.CreateStore(TStamp, InstTSMapGep1, "my_store");
  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_slot_load", VoidTy, Int64Ty, Int64Ty);
    IRBO.CreateCall(SetRealTemp, {insId, TStamp});
  }
  Value *InstMapGep1 = IRBO.CreateGEP(InstMap, {ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
                  insId}, "my_gep");
  IRBO.CreateStore(TStamp, InstMapGep1, "my_store");


  Value *Size;
  if(ClTracing){
    Size = ConstantInt::get(Int64Ty, 56);
  }
  else
  {
    Size = ConstantInt::get(Int64Ty, 24);
  }

  Value *Flag = ConstantInt::get(Int1Ty, false);
  BitCastInst* Dest = new BitCastInst(BOGEP, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
  BitCastInst* Src = new BitCastInst(Val, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
  Finish = M->getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i64", VoidTy, PtrVoidTy, PtrVoidTy, Int64Ty, Int1Ty);
  CallInst *CInst = IRB.CreateCall(Finish, {Dest, Src, Size, Flag});
  MemcpyNList.push_back(CInst);

  if(DEBUG){
    SetRealTemp = M->getOrInsertFunction("eftsan_handle_load", VoidTy, MPtrTy, MPtrTy);
    IRB.CreateCall(SetRealTemp, {BOGEP, Val});
  }
  return BOGEP;
}

void EFTSanitizer::handleLoad(LoadInst *LI, BasicBlock *BB, Function *F){
  Instruction *I = dyn_cast<Instruction>(LI);
  Module *M = F->getParent();
  Instruction *Next = getNextInstruction(I, BB);
  IRBuilder<> IRB(Next);

  LLVMContext &C = F->getContext();

  Type* PtrVoidTy = PointerType::getUnqual(Type::getInt8Ty(C));
  Type* VoidTy = Type::getVoidTy(C);
  IntegerType* Int1Ty = Type::getInt1Ty(M->getContext());
  IntegerType* Int32Ty = Type::getInt32Ty(M->getContext());

  Value *Addr = LI->getPointerOperand();
  if (isa<GlobalVariable>(Addr)){
  }
  bool BTFlag = false;

  if(LI->getType()->isVectorTy()){
    errs()<<"vectors not supported for now!";
    exit(0);
  }

  BitCastInst* BCToAddr = new BitCastInst(Addr, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
  if(isFloatType(LI->getType()) || BTFlag){
    Value* LoadI = loadShadowAddr(I, BCToAddr, BB, F);
  }
  else if(LI->getType()->isVectorTy() && isFloatType(LI->getType()->getScalarType())){ //vector
    IRBuilder<> IRBB(I);
    Value *ShadowAddr = nullptr;
    int num = LI->getType()->getVectorNumElements();
    Value *Gep1 = IRBB.CreateGEP(Addr, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");
    Value *Gep2 = IRBB.CreateGEP(Addr, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)}, "my_gep");
    BitCastInst* BCToAddr1 = new BitCastInst(Gep1, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
    Value* LoadI1 = loadShadowAddr(I, BCToAddr1, BB, F);

    BitCastInst* BCToAddr2 = new BitCastInst(Gep2, 
      PointerType::getUnqual(Type::getInt8Ty(M->getContext())),"", I);
    Value* Load2 = loadShadowAddr(I, BCToAddr2, BB, F);
    MInsMapPair.insert(std::pair<Instruction*, std::pair<Instruction*, Instruction*>>(I, 
          std::make_pair(dyn_cast<Instruction>(LoadI1), dyn_cast<Instruction>(Load2))));
  }
}

/*
void EFTSanitizer::resetMetadataConstants(Function *F){
  Module *M = F->getParent();
  IntegerType *Int8Ty = Type::getInt8Ty(M->getContext());
  IntegerType *Int64Ty = Type::getInt64Ty(M->getContext());
  Type* VoidTy = Type::getVoidTy(M->getContext());
  std::map<Instruction*, Value*>::iterator it;
  for (it = ConsInsMap.begin(); it != ConsInsMap.end(); it++)
  {
    Instruction *I = it->first;
    IRBuilder<> IRB(I);
    unsigned int lineNum = 0;
    ConstantInt* lineNumber = ConstantInt::get(Int64Ty, lineNum);

    Value *BOGEP = it->second;

    //store err
    Value *ValGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)}, "my_gep");

    //Set error to 0 for constant
    Value *Err = ConstantFP::get(Type::getDoubleTy(M->getContext()), 0.0);

    Value *StoreIns = IRB.CreateStore(Err, ValGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns));
#if TRACING
    //store op
    Constant* OpCode = ConstantInt::get(Type::getInt32Ty(M->getContext()), 0); //Constant
    Value *OpCGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), 
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 2)}, "my_gep");

    Value *StoreIns3 = IRB.CreateStore(OpCode, OpCGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns3));

    //store inst_id
    //store line no
    Value *InstIdGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 3)}, "my_gep");
    Value *StoreIns4 = IRB.CreateStore(lineNumber, InstIdGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns4));


    //store lhs
    Value *Op1 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstLHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 4)}, "my_gep");
    Value *StoreIns5 = IRB.CreateStore(Op1, InstLHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns5));

    //store rhs
    Value *Op2 = ConstantPointerNull::get(cast<PointerType>(MPtrTy));
    Value *InstRHSGep = IRB.CreateGEP(BOGEP, {ConstantInt::get(Type::getInt32Ty(M->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(M->getContext()), 5)}, "my_gep");
    Value *StoreIns6 = IRB.CreateStore(Op2, InstRHSGep, "my_store");
    StoreList.push_back(dyn_cast<StoreInst>(StoreIns6));
#endif
  }
}
*/

void EFTSanitizer::handleIns(Instruction *I, BasicBlock *BB, Function *F){

  Value *V = dyn_cast<Value> (I);
  if(V->getName().startswith("my_")){
    return;
  }
  //instrument interesting instructions
  if (FPToSIInst *FI = dyn_cast<FPToSIInst>(I)){
    handleFPToSI(FI, BB, F);
  }
  else if (FPToUIInst *UI = dyn_cast<FPToUIInst>(I)){
    handleFPToUI(UI, BB, F);
  }
  else if (LoadInst *LI = dyn_cast<LoadInst>(I)){
    handleLoad(LI, BB, F);
  }
  else if (FCmpInst *FCI = dyn_cast<FCmpInst>(I)){
    handleFcmp(FCI, BB, F);
  }
  else if (FPTruncInst *FPT = dyn_cast<FPTruncInst>(I)){
    handleFPTrunc(FPT, BB, F);
  }
  else if (StoreInst *SI = dyn_cast<StoreInst>(I)){
    handleStore(SI, BB, F);
  }
  else if (SelectInst *SI = dyn_cast<SelectInst>(I)){
    if(isFloatType(I->getType())){
      handleSelect(SI, BB, F);
    }
  }
  else if (ExtractValueInst *EVI = dyn_cast<ExtractValueInst>(I)){
  }
  else if (UnaryOperator *UO = dyn_cast<UnaryOperator>(I)) {
    switch (UO->getOpcode()) {
      case Instruction::FNeg: 
        {
          handleFNeg(UO, BB, F);
        }
    }
  }
  else if (BinaryOperator* BO = dyn_cast<BinaryOperator>(I)){
    switch(BO->getOpcode()) {                                                                                                                                         
      case Instruction::FAdd:                                                                        
      case Instruction::FSub:
      case Instruction::FMul:
      case Instruction::FDiv:
      case Instruction::FRem:
        {
          if(BO->getType()->isVectorTy()){ //vector
            handleBinOpVec(BO, BB, F);
          }
          else{
            handleBinOp(BO, BB, F);
          }
        }
    }
  }
  else if (CallInst *CI = dyn_cast<CallInst>(I)){
    Function *Callee = CI->getCalledFunction();
    if (Callee) {
      //handle precision specific constants
      Module *M = F->getParent();
      Type* VoidTy = Type::getVoidTy(M->getContext());
      /*
      if(Callee->getName().startswith("_ZN5Eigen16GenericNumTraitsIfE7epsilonEv")){
        //set a constant
        IRBuilder<> IRB(I);
        Value *BOGEP = GEPMap.at(I);
        SetRealTemp = M->getOrInsertFunction("eftsan_set_epsilon_precision_const", VoidTy, MPtrTy);
        IRB.CreateCall(SetRealTemp, {BOGEP});
      }
      */
      if(Callee->getName().startswith("llvm.memcpy")){
        MemcpyList.push_back(CI);
      }
      else if(Callee->getName().startswith("eftsan_print_error")){
        handleError(CI, BB, F);
        AllInstList.push_back(CI);
      }
      else if (Callee->getName().startswith("llvm.memset")){
        handleMemset(CI, BB, F, Callee->getName());
      }
      else if(isListedFunction(Callee->getName(), "mathFunc.txt")){
        handleMathLibFunc(CI, BB, F, Callee->getName());
      }
      else if(isListedFunction(Callee->getName(), "functions.txt")){
        handleCallInst(CI, BB, F);
      }
      else{
        if(!Callee->getName().startswith("eftsan")){
          if(isFloatType(CI->getType())){
            errs()<<"Check this function call:"<<*CI<<"\n";
          }
        }
      }
    }
    else if(CallSite(I).isIndirectCall()){
      handleCallInst(CI, BB, F);
    }
    else{
      if(Callee && !Callee->getName().startswith("eftsan"))
        if(isFloatType(CI->getType())){
          errs()<<"Check this function call:"<<*CI<<"\n";
        }
    }
  }
  else if (InvokeInst *CI = dyn_cast<InvokeInst>(I)){
    Function *Callee = CI->getCalledFunction();
    if (Callee) {
      if(Callee->getName().startswith("eftsan_print_error")){
        handleError2(CI, BB, F);
      }
      else if(isListedFunction(Callee->getName(), "functions.txt")){
        handleInvokeInst(CI, BB, F);
      }
    }
    else if(CallSite(I).isIndirectCall()){
      handleInvokeInst(CI, BB, F);
    }
    else {
      handleInvokeInst(CI, BB, F);
    }
  }
}

bool EFTSanitizer::runOnModule(Module &M) {

  auto GetTLI = [this](Function &F) -> TargetLibraryInfo & {
    return this->getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(F);
  };
  LLVMContext &C = M.getContext();

  Real = StructType::create(M.getContext(), "struct.smem");
  RealPtr = Real->getPointerTo();
  if (ClTracing){
    Real->setBody({
        Type::getDoubleTy(M.getContext()), //error
        Type::getDoubleTy(M.getContext()), //computed
        Type::getInt64Ty(M.getContext()),  //timestamp
        Type::getInt32Ty(M.getContext()), //op
        Type::getInt64Ty(M.getContext()), //instid
        RealPtr,
        RealPtr,
        });
  }
  else{
    Real->setBody({
        Type::getDoubleTy(M.getContext()), //error
        Type::getDoubleTy(M.getContext()), //computed
        Type::getInt64Ty(M.getContext())  //timestamp
        });
  }

  MPtrTy = Real->getPointerTo();
 
  MStackTop = new GlobalVariable(M, Type::getInt64Ty(M.getContext()), false, GlobalVariable::PrivateLinkage,
            Constant::getNullValue(Type::getInt64Ty(M.getContext())), "m_stack_top");
  MStackTop->setThreadLocalMode(GlobalVariable::LocalExecTLSModel);

  Shadow_Mem = new GlobalVariable(M, MPtrTy, false, GlobalValue::PrivateLinkage,
           Constant::getNullValue(MPtrTy), "m_shadow_mem"); 

  Shadow_Stack = new GlobalVariable(M, MPtrTy, false, GlobalValue::PrivateLinkage,
           Constant::getNullValue(MPtrTy), "m_shadow_stack"); 

  TimeStamp = new GlobalVariable(M, Type::getInt64Ty(M.getContext()), false, GlobalValue::PrivateLinkage, 
            Constant::getNullValue(Type::getInt64Ty(M.getContext())), "my_global_ts");

  // Find functions that perform floating point computation. No
  // instrumentation if the function does not perform any FP
  // computations.
  for (auto &F : M) {
    auto *TLI = &GetTLI(F);
    LibFunc Func;
    if (!F.hasLocalLinkage() && F.hasName() &&
        TLI->getLibFunc(F.getName(), Func)) {
      LibFuncList.insert(F.getFunction().getName());
    }
    else{
      findInterestingFunctions(&F);
    }
  }

  for (auto &F : M) {
    if (F.isDeclaration()) continue;
    if (!isListedFunction(F.getName(), "functions.txt")) continue;
    //All instrumented functions are listed in AllFuncList	
    if(F.getName() != "_ZSt5isnanf"){
      if(F.getName() != "_ZSt5isnand"){
        AllFuncList.push_back(&F);
      }
    }
  } 

  SmallVector<Function *, 8> FuncList; // Ignore returns cloned.
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;
    if (isListedFunction(F.getName(), "forbid.txt"))
      continue;
    if (LibFuncList.count(F.getName()) == 0) {
      FuncList.push_back(&F);
    }
  }
  //TODO start instid from 1, check for constants
  //TODO think about variable function arguments
  int instId = 1;
  //instrument interesting instructions
  Instruction *LastPhi = NULL;
  for (Function *F : reverse(AllFuncList)) {
    for (Function::arg_iterator ait = F->arg_begin(), aend = F->arg_end();
      ait != aend; ++ait) {
        Argument *A = &*ait;
        ArgInstIdMap.insert(std::pair<Argument*, int>(A, instId));
        instId++;
    }
    for (auto &BB : *F) {
      for (auto &I : BB) {
        LLVMContext& instContext = I.getContext();
        ConstantInt* instUniqueId = ConstantInt::get(Type::getInt64Ty(M.getContext()), instId);
        ConstantAsMetadata* uniqueId = ConstantAsMetadata::get(instUniqueId);
        MDNode* md = MDNode::get(instContext, uniqueId);
        I.setMetadata("inst_id", md);
        instId++;
      }
    }
  }
  int totalCons = 0;
  for (Function *F : reverse(AllFuncList)) {
    //give unique indexes to instructions and instrument with call to
    //dynamic lib
    //instId will be used to assign instid for constants as this function handles constants
    //for all instructions
    totalCons = countConstants(F);
  }

  ArrayType *ATy = ArrayType::get(Type::getInt64Ty(M.getContext()), instId + totalCons);

  //TODO: Look into the performance of instmap, how much is the speedup without instmap
  InstMap = new GlobalVariable(M, ATy, false, GlobalValue::PrivateLinkage, 
            ConstantArray::get(ATy, std::vector<Constant *>(instId, 
            ConstantInt::get(Type::getInt64Ty(M.getContext()), 0))), "my_inst_map");
  MapIns = new GlobalVariable(M, ATy, false, GlobalValue::PrivateLinkage, 
            ConstantArray::get(ATy, std::vector<Constant *>(instId, 
            ConstantInt::get(Type::getInt64Ty(M.getContext()), 0))), "my_map_ins");

  for (Function *F : reverse(AllFuncList)) {
    //give unique indexes to instructions and instrument with call to
    //dynamic lib
    //instId will be used to assign instid for constants as this function handles constants
    //for all instructions
    createMpfrAlloca(F, &instId);
    //if argument is used in any floating point computation, then we
    //need to retrieve that argument from shadow stack.  Instead of
    //call __get_arg everytime opearnd is used, it is better to call
    //once in start of the function and remember the address of shadow
    //stack.

    callGetArgument(F);
    //reset metadata for constants
    //resetMetadataConstants(F);

    if(F->getName() != "main"){
      //add func_init and func_exit in the start and end of the
      //function to set shadow stack variables
      handleFuncInit(F);
    }
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if(PHINode *PN = dyn_cast<PHINode>(&I)){
          if(isFloatType(I.getType())){
            //TODO:
            handlePhi(PN, &BB, F);
            LastPhi = &I;
          }
        }
        handleIns(&I, &BB, F);
      }
    }
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if(F->getName() != "main"){
          if (ReturnInst *RI = dyn_cast<ReturnInst>(&I)){
            handleReturn(RI, &BB, F);
          }
        }
      }
    }
    handleNewPhi(F);
    for (CallInst *CI : MemcpyList) {
      handleMemCpy(CI, CI->getParent(), CI->getParent()->getParent());
    }
    for (Instruction *Inst : AllInstList) {
      Inst->eraseFromParent();
    }
    AllInstList.clear();
    StoreList.clear();
    FCmpList.clear();
    NewPhiMap.clear(); 
    MInsMap.clear(); 
    GEPMap.clear(); 
    ConsMap.clear(); 
    ConsInsMap.clear(); 
    ConsInstIdMap.clear();
    MemcpyList.clear(); 
    MemcpyNList.clear(); 
  } 
  FuncList.clear();
  for (auto &F : M) {
    if (F.isDeclaration()) continue;
    if(F.getName() == "main"){
      //add init and finish func in the start and end of the main function to initialize shadow memory
      handleFuncMainInit(&F, instId + totalCons);
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (dyn_cast<ReturnInst>(&I)){
          if(F.getName() == "main"){
            handleMainRet(&I, &F);
          }
        }
      }
    }
  } 
//  errs()<<M;
  return true;
}



void addFPPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
  PM.add(new EFTSanitizer());
}

RegisterStandardPasses SOpt(PassManagerBuilder::EP_OptimizerLast,
    addFPPass);
RegisterStandardPasses S(PassManagerBuilder::EP_EnabledOnOptLevel0,
    addFPPass);

char EFTSanitizer::ID = 0;
static const RegisterPass<EFTSanitizer> Y("eftsan", "instrument with error free transformations", false, false);
