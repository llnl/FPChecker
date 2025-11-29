
#include "Instrumentation_error.h"
#include "CodeMatching.h"
#include "Logging.h"
#include "Utility.h"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/IntrinsicInst.h>

#include <list>
#include <string>

using namespace CPUAnalysis;
using namespace llvm;

/* This function configures the function found (e.g., calling conventions) and
saves pointer if needed. We also do logging. */
void confFunction(Function *found, Function **saveHere,
                  GlobalValue::LinkageTypes linkage, const char *name)
{
#ifdef FPC_DEBUG
  std::string out = std::string("Found ") + std::string(name);
  CUDAAnalysis::Logging::info(out.c_str());
#endif

  if (saveHere != nullptr) // if we want to save the function pointer
    *saveHere = found;
  if (found->getLinkage() != linkage)
    found->setLinkage(linkage);
}

/** Set linkage as ODR **/
#define SET_ODR_LIKAGE(name)                                      \
  if (f->getName().str().find(name) != std::string::npos)         \
  {                                                               \
    f->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage); \
  }

CPUFPInstrumentation_error::CPUFPInstrumentation_error(Module *M)
    : mod(M),
      fpc_init(nullptr), fpc_init_args(nullptr), fpc_print_locations(nullptr)
{

#ifdef FPC_DEBUG
  CUDAAnalysis::Logging::info("Initializing instrumentation");
#endif

  // Find and configure instrumentation functions
  for (auto F = M->begin(), e = M->end(); F != e; ++F)
  {
    Function *f = &(*F);
    if (f->getName().str().find("_FPC_INIT_FPCHECKER") != std::string::npos)
    {
      confFunction(f, &fpc_init, GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_INIT_FPCHECKER");
    }
    if (f->getName().str().find("_FPC_INIT_ARGS_FPCHECKER") !=
        std::string::npos)
    {
      confFunction(f, &fpc_init_args,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_INIT_ARGS_FPCHECKER");
    }
    if (f->getName().str().find("_FPC_PRINT_LOCATIONS_") != std::string::npos)
    {
      confFunction(f, &fpc_print_locations,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_PRINT_LOCATIONS_");
    }
    if (f->getName().str().find("_FPC_FP32_STORE_INST_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_store_inst,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_STORE_INST_");
    }
    if (f->getName().str().find("_FPC_FP32_LOAD_INST_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_load_inst,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_LOAD_INST_");
    }
    if (f->getName().str().find("_FPC_FP32_CALCULATE_ERROR_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_calculate_function,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_CALCULATE_ERROR_");
    }
    if (f->getName().str().find("_FPC_FP32_PHI_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_phi_function,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_PHI_");
    }
    if (f->getName().str().find("_FPC_FP32_BRANCH_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_branch_function,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_BRANCH_");
    }
    if (f->getName().str().find("_FPC_FP32_MEMCPY_INST_") != std::string::npos)
    {
      confFunction(f, &fp32_memcpy_function,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_MEMCPY_INST_");
    }

    SET_ODR_LIKAGE("_FPC_FP32_STORE_INST_")
    SET_ODR_LIKAGE("_FPC_FP32_LOAD_INST_")
    SET_ODR_LIKAGE("_FPC_FP32_CALCULATE_ERROR_")
    SET_ODR_LIKAGE("_FPC_FP32_PHI_")
    SET_ODR_LIKAGE("_FPC_FP32_BRANCH_")
    SET_ODR_LIKAGE("_FPC_PRINT_LOCATIONS_")
    SET_ODR_LIKAGE("_FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED")
    SET_ODR_LIKAGE("FPC_APPEND_ERROR_LOG_ENTRY")
    SET_ODR_LIKAGE("_FPC_FP32_MEMCPY_INST_")

    // Hash table functions
    SET_ODR_LIKAGE("_FPC_ADDRESS_HT_CREATE_")
    SET_ODR_LIKAGE("_FPC_REGISTER_HT_CREATE_")
    SET_ODR_LIKAGE("_FPC_HT_HASH_ADDRESS_")
    SET_ODR_LIKAGE("_FPC_HT_HASH_REGISTER_")
    SET_ODR_LIKAGE("_FPC_ADDRESS_HT_NEWPAIR_")
    SET_ODR_LIKAGE("_FPC_REGISTER_HT_NEWPAIR_")
    SET_ODR_LIKAGE("_FPC_ADDRESS_EQUAL_")
    SET_ODR_LIKAGE("_FPC_REGISTER_EQUAL_")
    SET_ODR_LIKAGE("_FPC_ADDRESS_HT_SET_")
    SET_ODR_LIKAGE("_FPC_REGISTER_HT_SET_")
    SET_ODR_LIKAGE("_FPC_ADDRESS_HT_UPDATE_")
    SET_ODR_LIKAGE("_FPC_REGISTER_HT_UPDATE_")
    SET_ODR_LIKAGE("_FPC_REGISTER_RANGE_UPDATE_")
    SET_ODR_LIKAGE("_FPC_FIND_ERRORS_BY_ADDRESS")
    SET_ODR_LIKAGE("_FPC_FIND_ERRORS_BY_REGISTER")
    SET_ODR_LIKAGE("_FPC_HT_PRINT_TABLES_")
    SET_ODR_LIKAGE("_FPC_INIT_HASH_TABLE_")
    SET_ODR_LIKAGE("_FPC_WRITE_AND_PRINT_TO_JSON_")

    // Series table functions
    SET_ODR_LIKAGE("FPC_create_manager")
    SET_ODR_LIKAGE("FPC_append_value")
    SET_ODR_LIKAGE("FPC_destroy_manager")
    SET_ODR_LIKAGE("FPC_print_series")
    SET_ODR_LIKAGE("FPC_series_to_json")
  }

  // ---- Globals initializations ---- //

  GlobalVariable *prog_inputs = nullptr;
  prog_inputs = mod->getGlobalVariable("_FPC_PROG_INPUTS", true);
  assert(prog_inputs && "Invalid table!");
  prog_inputs->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *prog_args = nullptr;
  prog_args = mod->getGlobalVariable("_FPC_PROG_ARGS", true);
  assert(prog_args && "Invalid table!");
  prog_args->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  /* -------------------- For error tracking ---------------------- */

  GlobalVariable *addr_table = nullptr;
  addr_table = mod->getGlobalVariable("_FPC_ADDRESS_HT_", true);
  assert(addr_table && "Invalid table!");
  addr_table->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *reg_table = nullptr;
  reg_table = mod->getGlobalVariable("_FPC_REGISTER_HT_", true);
  assert(reg_table && "Invalid table!");
  reg_table->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *war_count = nullptr;
  war_count = mod->getGlobalVariable("_FPC_WARNING_COUNT_", true);
  assert(war_count && "Invalid variable!");
  war_count->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *last_bb = nullptr;
  last_bb = mod->getGlobalVariable("_FPC_LAST_BASIC_BLOCK_", true);
  assert(last_bb && "Invalid table!");
  last_bb->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *clock = nullptr;
  clock = mod->getGlobalVariable("_FPC_CLOCK_", true);
  assert(clock && "Invalid table!");
  clock->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *lines_to_keep = nullptr;
  lines_to_keep = mod->getGlobalVariable("_FPC_LINES_TO_KEEP_", true);
  assert(lines_to_keep && "Invalid table!");
  lines_to_keep->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *fpc_data_manager = nullptr;
  fpc_data_manager = mod->getGlobalVariable("FPC_DATA_MANAGER", true);
  assert(fpc_data_manager && "Invalid table!");
  fpc_data_manager->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  // Set module filename
  module_filename = CUDAAnalysis::getFileNameFromModule(M);
}

// ********************************************************************
// Error analysis instrumentation function
// ********************************************************************
void CPUFPInstrumentation_error::instrumentFunctionErrorAnalysis(Function *f, long int *insrtrumented_instructions)
{
  /*   if (CUDAAnalysis::CodeMatching::isUnwantedFunction(f))
      return;

    if (functionisAnnotated(f, "_FPC_CALCULATE_ERROR_"))
    {
      // Instrument for error calculation
      CUDAAnalysis::Logging::info(
          ("Annotated function for error analysis: " + f->getName()).str().c_str());
    }
    else
    {
      return; // Not annotated, skip
    } */

  // Check that instrumentation functions are initialized
  assert((fpc_fp32_calculate_function != nullptr) && "Function not initialized!");
  assert((fpc_fp32_load_inst != nullptr) && "Function not initialized!");
  assert((fpc_fp32_store_inst != nullptr) && "Function not initialized!");
  assert((fpc_fp32_phi_function != nullptr) && "Function not initialized!");
  assert((fpc_fp32_branch_function != nullptr) && "Function not initialized!");
  assert((fp32_memcpy_function != nullptr) && "Function not initialized!");

  // Warning message
  // Check if the function calls other functions with floating-point values
  if (functionCallsFunctionWithFloatingPointValues(f))
  {
    CUDAAnalysis::Logging::info(
        ("*** WARNING *** Function " + f->getName() +
         " calls functions that return floating-point values!")
            .str()
            .c_str());
  }

#ifdef FPC_DEBUG
  CUDAAnalysis::Logging::info("Entering main loop in instrumentFunctionErrorAnalysis...");
#endif

  // ----- Add Load instruction for file name -----------------------------------------
  // Instrument first instruction in the function
  int load_counter = 0;
  Instruction *first_inst = nullptr;
  // std::string fileName = CUDAAnalysis::getFileNameFromFunction(f);
  for (auto bb = f->begin(), end = f->end(); bb != end; ++bb)
  {
    for (auto i = bb->begin(), bend = bb->end(); i != bend; ++i)
    {
      first_inst = &(*i);
      if (CUDAAnalysis::getFileNameFromInstruction(first_inst) != "Unknown")
      {
        break; // we found the file
      }
    }
    if (first_inst)
      break;
  }
  assert(first_inst && "First instruction not found!");
  IRBuilder<> builder(first_inst);

  // Get global fileName pointer
  // This creates a Load instruction
  GlobalVariable *fName = nullptr;
  fName =
      mod->getGlobalVariable("_ZL15_FPC_FILE_NAME_", true); // C++ binding
  if (fName == nullptr)
    fName =
        mod->getGlobalVariable("_FPC_FILE_NAME_", true); // try C binding
  assert((fName != nullptr) && "Global filename var not found");
  Type *gvType = fName->getType();
  std::string loadName = "my_loaded_" + std::to_string(load_counter++);
  auto loadInst_filename =
      builder.CreateAlignedLoad(gvType, fName, MaybeAlign(), loadName);

  // Push file name
  Constant *c = builder.CreateGlobalStringPtr(module_filename);
  fName->setInitializer(NULL);
  fName->setInitializer(c);
  //  -------------------------------------------------------------------------------

  for (auto bb = f->begin(), end = f->end(); bb != end; ++bb)
  {
    for (auto i = bb->begin(), bend = bb->end(); i != bend; ++i)
    {
      Instruction *inst = &(*i);

      // ============= Instrument for STORE instructions ========================
      if (llvm::isa<llvm::StoreInst>(inst))
      {
        // This is a store instruction
        llvm::Value *storedValue = llvm::cast<llvm::StoreInst>(inst)->getValueOperand();
        if ((storedValue->getType()->isFloatTy() || storedValue->getType()->isDoubleTy()) && !llvm::isa<llvm::Constant>(storedValue))
        {
          BasicBlock::iterator nextInst(inst);
          nextInst++;
          IRBuilder<> builder(&(*nextInst));

          llvm::Value *storeAddr = llvm::cast<StoreInst>(inst)->getPointerOperand(); // ptr %2
          llvm::Value *storeAddrInt = builder.CreatePtrToInt(storeAddr, llvm::Type::getInt64Ty(inst->getContext()), "my_store_addr");

          /*------------------------------------------------------------------*
           *  Get the SSA Name being Stored.                                  *
           *------------------------------------------------------------------*/
          std::string reg;
          llvm::raw_string_ostream rso(reg);
          storedValue->printAsOperand(rso, false);
          rso.flush();
          llvm::Value *regStr = builder.CreateGlobalStringPtr(reg);

          std::vector<Value *> args;
          // Push parameters
          args.push_back(regStr);
          args.push_back(builder.CreateGlobalStringPtr(f->getName()));
          args.push_back(storeAddrInt);

          // Push location parameter (line number)
          int lineNumber = CUDAAnalysis::getLineOfCode(inst);
          ConstantInt *locId =
              ConstantInt::get(mod->getContext(), APInt(32, lineNumber, true));
          args.push_back(locId);
          args.push_back(loadInst_filename);

          ArrayRef<Value *> args_ref(args);
          CallInst *callInst = nullptr;
          callInst = builder.CreateCall(fpc_fp32_store_inst, args_ref);
          (*insrtrumented_instructions)++;

          assert(callInst && "Invalid call instruction!");
          setFakeDebugLocation(inst, callInst, f);
        }
      }
      // ============= Instrument for LOAD instructions =========================
      if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(inst))
      {
        if (loadInst->getType()->isFloatTy() || loadInst->getType()->isDoubleTy())
        {
          BasicBlock::iterator nextInst(inst);
          ++nextInst;
          IRBuilder<> builder(&(*nextInst));

          /*------------------------------------------------------------------*
            Compute the address in uintptr_t                              *
          *------------------------------------------------------------------*/

          llvm::Value *addr = loadInst->getPointerOperand();
          llvm::Value *addrInt = builder.CreatePtrToInt(
              addr, llvm::Type::getInt64Ty(inst->getContext()));

          /*------------------------------------------------------------------*
           * Get the SSA name of the result (%15, %23 …)
           * loadInst itself is the “result”, so print it as an operand.
           *------------------------------------------------------------------*/
          std::string regName;
          llvm::raw_string_ostream rso(regName);
          loadInst->printAsOperand(rso, /*PrintType=*/false); // yields "%15"
          rso.flush();
          llvm::Value *regStr = builder.CreateGlobalStringPtr(regName);

          std::vector<Value *> args;
          args.push_back(regStr);                                      // const char *load_reg
          args.push_back(builder.CreateGlobalStringPtr(f->getName())); // const char *function_name
          args.push_back(addrInt);
          int lineNumber = CUDAAnalysis::getLineOfCode(inst);
          ConstantInt *locId =
              ConstantInt::get(mod->getContext(), APInt(32, lineNumber, true));
          args.push_back(locId);
          args.push_back(loadInst_filename);

          ArrayRef<Value *> args_ref(args);
          CallInst *callInst = nullptr;
          callInst = builder.CreateCall(fpc_fp32_load_inst, args_ref);
          (*insrtrumented_instructions)++;

          assert(callInst && "Invalid call instruction!");
          setFakeDebugLocation(inst, callInst, f);
        }
      }

      // ============= Instrument memcpy and memmove ========================
      /*
      Instrumentation function signature:
      void _FPC_FP32_MEMCPY_INST_(uintptr_t address_dst, uintptr_t address_src,
                            long int size, int size_type, int ins_type, int loc, char *file_name)
      */
      if (auto *II = llvm::dyn_cast<llvm::IntrinsicInst>(inst))
      {
        auto id = II->getIntrinsicID();
        if (id == llvm::Intrinsic::memcpy ||
            id == llvm::Intrinsic::memcpy_inline ||
            id == llvm::Intrinsic::memmove)
        {
          errs() << "Instrumenting memcpy/memmove in function " << f->getName() << "\n";
          errs() << "  Instruction: ";
          inst->print(errs());
          errs() << "\n";

          // Insert instrumentation right after the memcpy/memmove intrinsic.
          BasicBlock::iterator nextInst(inst);
          ++nextInst;
          IRBuilder<> builder(&(*nextInst));

          //   call void @llvm.memcpy.p0.p0.i64(ptr align 4 %93, ptr align 4 %50, i64 %71, i1 false), !dbg !6762

          // destination address
          llvm::Value *addr_dst = II->getArgOperand(0);
          llvm::Value *addr_dstInt = builder.CreatePtrToInt(
              addr_dst, llvm::Type::getInt64Ty(inst->getContext()));
          // source address
          llvm::Value *addr_src = II->getArgOperand(1);
          llvm::Value *addr_srcInt = builder.CreatePtrToInt(
              addr_src, llvm::Type::getInt64Ty(inst->getContext()));
          // size
          llvm::Value *size = II->getArgOperand(2);
          llvm::Value *sizeInt = builder.CreatePtrToInt(
              size, llvm::Type::getInt64Ty(inst->getContext()));

          std::vector<Value *> args;
          args.push_back(addr_dstInt); // uintptr_t dest_address
          args.push_back(addr_srcInt); // uintptr_t src_address
          args.push_back(sizeInt);     // size_t size

          // Size type of the second parameter of the memcpy/memmove:
          // void @llvm.memcpy.inline.p0.p0.i32(ptr <dest>, ptr <src>, i32 <len>, i1 <isvolatile>)
          // type: 0 for i32, 1 for i64
          int size_type = 0;
          if (size->getType()->isIntegerTy(64))
            size_type = 1;
          ConstantInt *sizeTypeId =
              ConstantInt::get(mod->getContext(), APInt(32, size_type, true));
          args.push_back(sizeTypeId);

          // Pass type 0 if the instruction is :
          // ‘llvm.memcpy’ Intrinsic, or ‘llvm.memcpy.inline’ Intrinsic
          // Pass type 1 if the instruction is :
          // ‘llvm.memmove’ Intrinsic
          int ins_type = 0;
          if (id == llvm::Intrinsic::memmove)
            ins_type = 1;
          ConstantInt *insTypeId =
              ConstantInt::get(mod->getContext(), APInt(32, ins_type, true));
          args.push_back(insTypeId);

          // Push location parameter (line number)
          int lineNumber = CUDAAnalysis::getLineOfCode(inst);
          ConstantInt *locId =
              ConstantInt::get(mod->getContext(), APInt(32, lineNumber, true));
          args.push_back(locId);
          args.push_back(loadInst_filename);

          ArrayRef<Value *> args_ref(args);
          CallInst *callInst = nullptr;
          callInst = builder.CreateCall(fp32_memcpy_function, args_ref);
          (*insrtrumented_instructions)++;

          assert(callInst && "Invalid call instruction!");
          setFakeDebugLocation(inst, callInst, f);
        }
      }

      // ============= Instrument for FP arithmetic operations ===================
      if ((isFPOperationWithError(inst) ||
           (inst->getOpcode() == Instruction::FNeg) ||
           (inst->getOpcode() == Instruction::Select)) &&
          (inst->getOperand(0)->getType()->isFloatTy() ||
           inst->getOperand(1)->getType()->isFloatTy()))
      {
        DebugLoc loc = inst->getDebugLoc();

        // Create builder to add stuff after the instruction
        BasicBlock::iterator nextInst(inst);
        nextInst++;
        IRBuilder<> builder(&(*nextInst));

        // Push parameters
        std::vector<Value *> args;
        args.push_back(inst);

        // Every arithmetic instruction has at least one operand (except Select, which has a boolean)
        if (inst->getOpcode() == Instruction::Select)
          args.push_back(ConstantFP::get(builder.getFloatTy(), 0.0f));
        else
          args.push_back(inst->getOperand(0));

        if (inst->getNumOperands() >= 2) // For fneg operation
          args.push_back(inst->getOperand(1));
        else
          args.push_back(ConstantFP::get(builder.getFloatTy(), 0.0f));

        if (inst->getNumOperands() >= 3) // For FMA operation
          args.push_back(inst->getOperand(2));
        else
          args.push_back(ConstantFP::get(builder.getFloatTy(), 0.0f));

        // Push location parameter (line number)
        int lineNumber = CUDAAnalysis::getLineOfCode(inst);
        // Discard if line number is invalid (no debug info for inst)
        if (lineNumber == -1)
          continue;
        ConstantInt *locId =
            ConstantInt::get(mod->getContext(), APInt(32, lineNumber, true));
        args.push_back(locId);
        args.push_back(loadInst_filename);

        // Push operation type
        int operationType = 0;
        if (inst->getOpcode() == Instruction::FAdd)
          operationType = 0;
        else if (inst->getOpcode() == Instruction::FSub)
          operationType = 1;
        else if (inst->getOpcode() == Instruction::FMul)
          operationType = 2;
        else if (inst->getOpcode() == Instruction::FDiv)
          operationType = 3;
        // else if (isCmpEqual(inst))
        //   operationType = 4;
        else if (inst->getOpcode() == Instruction::FRem)
          operationType = 5;
        else if (isFMAOperation(inst))
          operationType = 6;
        else if (inst->getOpcode() == Instruction::FNeg)
          operationType = 7;
        else if (inst->getOpcode() == Instruction::Select)
          operationType = 8;
        else
          operationType = -1;
        assert(operationType >= 0 && "Unknown operation");

        ConstantInt *opType =
            ConstantInt::get(mod->getContext(), APInt(32, operationType, true));
        args.push_back(opType);

        // Check if instruction is selected based on a condition
        Instruction *select_inst = nullptr;
        Value *condition = nullptr;  // condition value
        Value *cond_instr = nullptr; // condition instruction
        int inverse;                 // inverse the semantics of the condition?
        if (selectedBasedOnCondition(inst, f, &select_inst, &condition,
                                     &inverse))
        {
          // Set insertion point after the select instruction
          assert(select_inst && "Invalid select instruction");
          BasicBlock::iterator nextOne(select_inst);
          nextOne++;
          builder.SetInsertPoint(&(*nextOne));
          // Inverse semantics of condition if needed
          if (inverse)
          {
            // Add XOR to negate the condition
            auto neg_inst = builder.CreateXor(condition, 1, "my");
            cond_instr =
                builder.CreateZExt(neg_inst, builder.getInt32Ty(), "my");
            assert(cond_instr && "Invalid extension instruction");
            args.push_back(cond_instr);
          }
          else
          {
            // Add extension of condition (from i1 to i32 integer)
            cond_instr =
                builder.CreateZExt(condition, builder.getInt32Ty(), "my");
            assert(cond_instr && "Invalid extension instruction");
            args.push_back(cond_instr);
          }
        }
        else if (inst->getOpcode() == Instruction::Select)
        {
          // Handle Select instruction
          Value *condVal = inst->getOperand(0);
          Value *condInt = builder.CreateZExt(condVal, builder.getInt32Ty(), "select_cond");
          args.push_back(condInt);
        }
        else
        {
          ConstantInt *cond =
              ConstantInt::get(mod->getContext(), APInt(32, 1, true));
          args.push_back(cond);
        }

        std::string result_name;
        {
          std::string str;
          llvm::raw_string_ostream rso(str);
          inst->printAsOperand(rso, false);
          result_name = rso.str();
        }

        std::string op1_name;
        {
          std::string str;
          llvm::raw_string_ostream rso(str);
          inst->getOperand(0)->printAsOperand(rso, false);
          op1_name = rso.str();
        }

        std::string op2_name = "null";
        if (inst->getNumOperands() >= 2)
        {
          std::string str;
          llvm::raw_string_ostream rso(str);
          inst->getOperand(1)->printAsOperand(rso, false);
          op2_name = rso.str();
        }

        std::string fma_name = "null";
        if (inst->getNumOperands() >= 3)
        {
          std::string str;
          llvm::raw_string_ostream rso(str);
          inst->getOperand(2)->printAsOperand(rso, false);
          fma_name = rso.str();
        }

        args.push_back(builder.CreateGlobalStringPtr(result_name));
        args.push_back(builder.CreateGlobalStringPtr(op1_name));
        args.push_back(builder.CreateGlobalStringPtr(op2_name));
        args.push_back(builder.CreateGlobalStringPtr(fma_name));

        args.push_back(builder.CreateGlobalStringPtr(f->getName()));

        ArrayRef<Value *> args_ref(args);

        CallInst *callInst = nullptr;
        if (inst->getType()->isFloatTy())
        {
          callInst = builder.CreateCall(fpc_fp32_calculate_function, args_ref);
          (*insrtrumented_instructions)++;
        }
        else if (inst->getType()->isDoubleTy())
        {
          return; // Do not instrument fp64 for now
        }

        assert(callInst && "Invalid call instruction!");
        setFakeDebugLocation(inst, callInst, f);
      }
    }
  }

  // ----- Instruments PHI nodes returning float or double -----
  // We instrument in a different loop to avoid changing the names of the registers (%NUMBER)
  // when adding the phi instrumentation functions
  for (auto bb = f->begin(), end = f->end(); bb != end; ++bb)
  {
    for (auto i = bb->begin(), bend = bb->end(); i != bend; ++i)
    {
      Instruction *inst = &(*i);

      if (auto *phiInst = llvm::dyn_cast<llvm::PHINode>(inst))
      {
        if (phiInst->getType()->isFloatTy() || phiInst->getType()->isDoubleTy())
        {
          // Saves the phi instruction register name
          std::string phiRegName;
          llvm::raw_string_ostream phiRegStream(phiRegName);
          phiInst->printAsOperand(phiRegStream, false);
          phiRegStream.flush();
          std::string combinedStr = phiRegName + ":";

          // This is a PHI node returning either float or double
          for (unsigned idx = 0; idx < phiInst->getNumIncomingValues(); ++idx)
          {
            Value *incomingValue = phiInst->getIncomingValue(idx);
            BasicBlock *incomingBlock = phiInst->getIncomingBlock(idx);
            assert(incomingBlock && "Incoming block is null!");

            // Assume you have:
            // BasicBlock *incomingBlock = phiInst->getIncomingBlock(idx);

            std::string blockName;
            llvm::raw_string_ostream ostream(blockName);

            // The second argument 'false' tells it not to print the type (e.g., 'label %entry').
            incomingBlock->printAsOperand(ostream, false);

            std::string incomingValueName;
            llvm::raw_string_ostream incomingValueStream(incomingValueName);
            incomingValue->printAsOperand(incomingValueStream, false);
            incomingValueStream.flush();

            combinedStr += incomingValueName + "|" + ostream.str() + ";";
          }

          // Create builder to add stuff after the last phi instruction
          BasicBlock::iterator insertPt(phiInst);
          ++insertPt;
          while (insertPt != bb->end() && llvm::isa<llvm::PHINode>(&*insertPt))
          {
            ++insertPt;
          }
          IRBuilder<> builder(&*insertPt);

          std::vector<Value *> args;
          args.push_back(builder.CreateGlobalStringPtr(combinedStr));
          args.push_back(builder.CreateGlobalStringPtr(f->getName()));
          ArrayRef<Value *> args_ref(args);
          CallInst *callInst = builder.CreateCall(fpc_fp32_phi_function, args_ref);
          (*insrtrumented_instructions)++;
          assert(callInst && "Invalid call instruction!");
          setFakeDebugLocation(phiInst, callInst, f);
        }
      }
    }
  }

  // ----------- Add BasicBlock instrumentation --------------------
  for (auto bb = f->begin(), end = f->end(); bb != end; ++bb)
  {
    BasicBlock *basicBlock = &(*bb);
    Instruction *terminator = basicBlock->getTerminator();
    IRBuilder<> builder(terminator);

    if (terminator && llvm::isa<llvm::BranchInst>(terminator))
    {
      // llvm::errs() << ">>> Branch instruction found: " << *terminator << "\n";
      std::string bbName;
      llvm::raw_string_ostream bbStream(bbName);
      basicBlock->printAsOperand(bbStream, false);
      bbStream.flush();
      // llvm::errs() << "\t Current BasicBlock: " << bbName << "\n";

      // You can use 'terminator' as the branch instruction here
      std::vector<Value *> args;
      args.push_back(builder.CreateGlobalStringPtr(bbName));
      ArrayRef<Value *> args_ref(args);
      CallInst *callInst = builder.CreateCall(fpc_fp32_branch_function, args_ref);
      (*insrtrumented_instructions)++;

      assert(callInst && "Invalid call instruction!");
      setFakeDebugLocation(terminator, callInst, f);
    }
  }
}

// We check if the instruction inst is used only by a select instruction.
// If that is the case, we set the condition value and select instruction.
// Logic: the function returns true if:
//    - The inst is used only by one select instruction
//    - The inst is not used by any other instruction
bool CPUFPInstrumentation_error::selectedBasedOnCondition(Instruction *inst,
                                                          Function *f,
                                                          Instruction **select_inst,
                                                          Value **condition,
                                                          int *inverse)
{
  bool ret = false;
  int numSelect = 0;
  int others = 0;
  for (User *U : inst->users())
  {
    if (Instruction *use_inst = dyn_cast<Instruction>(U))
    {
      if (SelectInst *s_inst = dyn_cast<SelectInst>(use_inst))
      {
        numSelect++;
        *select_inst = s_inst;
        *condition = s_inst->getOperand(0);
        if (dyn_cast<Instruction>(s_inst->getOperand(1)) == inst)
          *inverse = 0;
        else
          *inverse = 1;
      }
      else
      {
        others++;
      }
    }
  }

  if (numSelect == 1 && others == 0)
    ret = true;

  return ret;
}

bool CPUFPInstrumentation_error::isCmpEqual(const Instruction *inst)
{
  if (inst->getOpcode() == Instruction::FCmp)
  {
    if (const CmpInst *cmpInst = dyn_cast<CmpInst>(inst))
    {
      if (cmpInst->getPredicate() == llvm::CmpInst::Predicate::FCMP_OEQ ||
          cmpInst->getPredicate() == llvm::CmpInst::Predicate::FCMP_UEQ)
        return true;
    }
  }
  return false;
}

/// Returns true if the instruction is a call to the llvm.fmuladd intrinsic.
bool CPUFPInstrumentation_error::isFMAOperation(const Instruction *inst)
{
  if (auto *intrin = dyn_cast<IntrinsicInst>(inst))
  {
    auto id = intrin->getIntrinsicID();
    return id == Intrinsic::fmuladd || id == Intrinsic::fma;
  }
  return false;
}

bool CPUFPInstrumentation_error::isFPOperation(const Instruction *inst)
{

  return ((inst->getOpcode() == Instruction::FMul) ||
          (inst->getOpcode() == Instruction::FDiv) ||
          (inst->getOpcode() == Instruction::FAdd) ||
          (inst->getOpcode() == Instruction::FSub) ||
          (inst->getOpcode() == Instruction::FRem)) ||
         isCmpEqual(inst) ||
         isFMAOperation(inst);
}

bool CPUFPInstrumentation_error::isFPOperationWithError(const Instruction *inst)
{

  return ((inst->getOpcode() == Instruction::FMul) ||
          (inst->getOpcode() == Instruction::FDiv) ||
          (inst->getOpcode() == Instruction::FAdd) ||
          (inst->getOpcode() == Instruction::FSub) ||
          (inst->getOpcode() == Instruction::FRem)) ||
         isFMAOperation(inst);
}

bool CPUFPInstrumentation_error::isDoubleFPOperation(const Instruction *inst)
{
  if (!isFPOperation(inst))
    return false;
  // return inst->getType()->isDoubleTy();
  return inst->getOperand(0)->getType()->isDoubleTy();
}

bool CPUFPInstrumentation_error::isSingleFPOperation(const Instruction *inst)
{
  if (!isFPOperation(inst))
    return false;
  // return inst->getType()->isFloatTy();
  return inst->getOperand(0)->getType()->isFloatTy();
}

void CPUFPInstrumentation_error::setFakeDebugLocation(Instruction *old_inst,
                                                      Instruction *new_inst,
                                                      Function *f)
{
  auto di = old_inst->getDebugLoc();
  if (!di)
  { // couldn't find debug info
    for (auto bb = f->begin(), end = f->end(); bb != end; ++bb)
    {
      for (auto i = bb->begin(), bend = bb->end(); i != bend; ++i)
      {
        Instruction *inst = &(*i);
        auto tmp_di = inst->getDebugLoc();
        if (tmp_di)
        {
          new_inst->setDebugLoc(inst->getDebugLoc());
          return;
        }
      }
    }
  }
  else
  {
    new_inst->setDebugLoc(di);
    return;
  }
  // IF we reach it, it means we couldn't find debug information
  // new_inst->eraseFromParent();
  assert(new_inst->getDebugLoc() && "Invalid debug loc! Please use -g");
}

bool CPUFPInstrumentation_error::functionisAnnotated(const Function *f, const char *annotation)
{
  assert((f != nullptr) && "Function not initialized!");
  const llvm::Module *M = f->getParent();
  if (M == nullptr)
    return false;

  // Access the @llvm.global.annotations global variable (this is an array)
  llvm::GlobalVariable *GV = M->getGlobalVariable("llvm.global.annotations");

  if (!GV || !GV->hasInitializer())
    return false; // No global annotations in this module or it's not initialized

  // The initializer is a constant array of structures
  llvm::Constant *Initializer = GV->getInitializer();
  llvm::ConstantArray *CA = llvm::dyn_cast<llvm::ConstantArray>(Initializer);
  if (!CA)
    return false; // llvm.global.annotations initializer is not a ConstantArray

  // --- Iterate through the array of annotation structures ---
  // Examlple:
  // @llvm.global.annotations = appending global [2 x { ptr, ptr, ptr, i32, ptr }]
  // [{ ptr, ptr, ptr, i32, ptr } { ptr @_Z15matrix_multiplyRKNSt3__16vectorINS0_IdNS_9allocatorIdEEEENS1_IS3_EEEES7_,
  // ptr @.str.65, ptr @.str.66, i32 8, ptr null }, { ptr, ptr, ptr, i32, ptr }
  // { ptr @_Z17subtract_matricesRKNSt3__16vectorINS0_IdNS_9allocatorIdEEEENS1_IS3_EEEES7_, ptr @.str.65,
  // ptr @.str.66, i32 34, ptr null }], section "llvm.metadata"
  for (unsigned i = 0; i < CA->getNumOperands(); ++i)
  {
    llvm::ConstantStruct *CS = llvm::dyn_cast<llvm::ConstantStruct>(CA->getOperand(i));
    if (!CS || CS->getNumOperands() != 5)
      continue; // Expecting a struct with 5 fields: {ptr, ptr, ptr, i32, ptr}

    // Check if the first pointer field points to our function 'f'
    // The first operand is the pointer to the annotated value.
    // Use stripPointerCasts to get the underlying value in case of bitcasts.
    llvm::Value *AnnotatedValue = CS->getOperand(0)->stripPointerCasts();

    // Try to cast the annotated value to a Function*
    llvm::Function *AnnotatedFunc = llvm::dyn_cast<llvm::Function>(AnnotatedValue);

    if (AnnotatedFunc && AnnotatedFunc == f) // Found the annotation entry for our function 'f'
    {
      // Retrieve the second pointer field (annotation string pointer)
      llvm::Constant *AnnotationStrPtrConstant = CS->getOperand(1);

      // The pointer points to a global variable containing the string.
      llvm::GlobalVariable *AnnotationStrGV =
          llvm::dyn_cast<llvm::GlobalVariable>(AnnotationStrPtrConstant->stripPointerCasts());

      if (AnnotationStrGV && AnnotationStrGV->hasInitializer())
      {
        // Access the initializer of the string global variable
        llvm::Constant *StringInitializer = AnnotationStrGV->getInitializer();

        // The initializer should be a ConstantDataArray (for a string literal)
        llvm::ConstantDataArray *CDA = llvm::dyn_cast<llvm::ConstantDataArray>(StringInitializer);

        if (CDA && CDA->isString())
        {
          // Found the string data
          llvm::StringRef AnnotationString = CDA->getAsString();
          // if (AnnotationString.contains("_FPC_INSTRUMENT_FUNCTION_"))
          if (AnnotationString.contains(annotation))
          {
            // Print the annotation string
            // llvm::outs() << "Annotation for function '" << f->getName() << "': " << AnnotationString << "\n";
            return true;
          }
          else // A function could have multiple annotations, so continue the loop.
          {
            continue;
          }
        }
      }
    }
  }

  return false; // No annotation found for this function
}

// Check if the function 'f' calls other functions that return floating-point types
// Except: fmuladd intrinsic, or FMA functions
bool CPUFPInstrumentation_error::functionCallsFunctionWithFloatingPointValues(const Function *f)
{
  // Iterate over all Basic Blocks (BBs) in the Function
  for (auto &bb : *f)
  {
    // Iterate over all Instructions in the Basic Block
    for (auto &i : bb)
    {
      // Use Instruction reference directly
      const Instruction *inst = &i;

      // Use dyn_cast on CallBase for unified handling of Call/Invoke/CallBr
      if (const CallBase *callBase = dyn_cast<CallBase>(inst))
      {
        // If this call is an intrinsic FMA-like operation (fmuladd / fma), skip it.
        if (const IntrinsicInst *II = dyn_cast<IntrinsicInst>(inst))
        {
          auto id = II->getIntrinsicID();
          if (id == Intrinsic::fmuladd || id == Intrinsic::fma)
            continue; // do not consider FMAs
        }

        // Note: The return type is the type of the CallBase instruction itself.
        Type *retType = callBase->getType();
        if (retType && (retType->isFloatTy() || retType->isDoubleTy() || retType->isHalfTy() || retType->isFP128Ty()))
        {
          // errs() << "\n----> Function " << f->getName() << " calls function with floating-point return type: " << *callBase << "\n";
          return true;
        }
      }
    }
  }
  // If no floating-point call instruction was found
  return false;
}

/* Returns the return first (non-phi) instruction of the module */
Instruction *CPUFPInstrumentation_error::firstInstrution()
{
  Instruction *inst = nullptr;
  for (auto f = mod->begin(), e = mod->end(); f != e; ++f)
  {
    // Discard function declarations
    if (f->isDeclaration())
      continue;

    // Function *F = &(*f);
    BasicBlock *bb = &(f->getEntryBlock());
    inst = bb->getFirstNonPHIOrDbgOrLifetime();
    break;
  }

  assert(inst && "Instruction not valid!");
  return inst;
}

void CPUFPInstrumentation_error::instrumentMainFunction(Function *f)
{
  /// ----------------- BEGIN --------------------------
  BasicBlock *bb = &(*(f->begin()));
  Instruction *inst = bb->getFirstNonPHIOrDbg();
  IRBuilder<> builder(inst);
  std::vector<Value *> args;

  CallInst *callInst = nullptr;
  // Check if function has argumments or not
  if (f->arg_size() == 2)
  {
    // Push parameters
    for (auto i = f->arg_begin(); i != f->arg_end(); ++i)
    {
      Value *v = &(*i);
      args.push_back(v);
    }
    ArrayRef<Value *> args_ref(args);
    callInst = builder.CreateCall(fpc_init_args, args_ref);
  }
  else
  {
    // ArrayRef<Value *> args_ref(args);
    callInst = builder.CreateCall(fpc_init, args);
  }
  assert(callInst && "Invalid call instruction!");

  // Set debug location
  for (auto i = bb->begin(), bend = bb->end(); i != bend; ++i)
  {
    Instruction *inst = &(*i);
    if (inst->getDebugLoc())
    {
      callInst->setDebugLoc(inst->getDebugLoc());
      break;
    }
  }
  // callInst->setDebugLoc(inst->getDebugLoc());
  assert(callInst->getDebugLoc() && "Invalid debug loc! Please use -g");

  /// ------------------ END ----------------------------
  /// Print table before end of function
  for (auto bb = f->begin(), end = f->end(); bb != end; ++bb)
  {
    for (auto i = bb->begin(), iend = bb->end(); i != iend; ++i)
    {
      Instruction *inst = &(*i);
      if (isa<ReturnInst>(inst) || isa<ResumeInst>(inst))
      {
        std::vector<Value *> args;
        ArrayRef<Value *> args_ref(args);
        IRBuilder<> builder(inst);
        auto callInst = builder.CreateCall(fpc_print_locations, args_ref);
        assert(callInst && "Invalid call instruction!");
        callInst->setDebugLoc(inst->getDebugLoc());
        assert(callInst->getDebugLoc() && "Invalid debug loc! Please use -g");
      }
    }
  }
}
