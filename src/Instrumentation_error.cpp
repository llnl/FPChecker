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

#include <vector>
#include <cstdlib>

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
  fpc_init(nullptr), fpc_init_args(nullptr), fpc_print_locations(nullptr),
  fpc_fp32_cmp_function(nullptr),
  fpc_fp32_push_ret_error(nullptr), fpc_fp32_pop_ret_error(nullptr),
  fpc_fp32_push_arg_error(nullptr), fpc_fp32_pop_arg_error(nullptr),
  fpc_fp32_math_error(nullptr)
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
    if (f->getName().str().find("_FPC_FP32_CMP_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_cmp_function,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_CMP_");
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
    if (f->getName().str().find("_FPC_FP32_PUSH_RET_ERROR_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_push_ret_error,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_PUSH_RET_ERROR_");
    }
    if (f->getName().str().find("_FPC_FP32_POP_RET_ERROR_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_pop_ret_error,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_POP_RET_ERROR_");
    }
    if (f->getName().str().find("_FPC_FP32_PUSH_ARG_ERROR_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_push_arg_error,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_PUSH_ARG_ERROR_");
    }
    if (f->getName().str().find("_FPC_FP32_POP_ARG_ERROR_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_pop_arg_error,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_POP_ARG_ERROR_");
    }
    if (f->getName().str().find("_FPC_FP32_MATH_ERROR_") != std::string::npos)
    {
      confFunction(f, &fpc_fp32_math_error,
                   GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                   "_FPC_FP32_MATH_ERROR_");
    }

    SET_ODR_LIKAGE("_FPC_FP32_STORE_INST_")
    SET_ODR_LIKAGE("_FPC_FP32_LOAD_INST_")
    SET_ODR_LIKAGE("_FPC_FP32_CMP_")
    SET_ODR_LIKAGE("_FPC_FP32_CALCULATE_ERROR_")
    SET_ODR_LIKAGE("_FPC_FP32_PHI_")
    SET_ODR_LIKAGE("_FPC_FP32_BRANCH_")
    SET_ODR_LIKAGE("_FPC_PRINT_LOCATIONS_")
    SET_ODR_LIKAGE("_FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED")
    SET_ODR_LIKAGE("FPC_APPEND_ERROR_LOG_ENTRY")
    SET_ODR_LIKAGE("_FPC_FP32_MEMCPY_INST_")
    SET_ODR_LIKAGE("_FPC_FP32_PUSH_RET_ERROR_")
    SET_ODR_LIKAGE("_FPC_FP32_POP_RET_ERROR_")
    SET_ODR_LIKAGE("_FPC_FP32_PUSH_ARG_ERROR_")
    SET_ODR_LIKAGE("_FPC_FP32_POP_ARG_ERROR_")
    SET_ODR_LIKAGE("_FPC_FP32_MATH_ERROR_")

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
    SET_ODR_LIKAGE("_FPC_LINE_MAX_RELATIVE_ERROR_UPDATE_")
    SET_ODR_LIKAGE("_FPC_FIND_LINE_MAX_RELATIVE_ERROR_")
    SET_ODR_LIKAGE("_FPC_REGISTER_RANGE_UPDATE_")   //There is no register range update function in FPC_Hastable_Error.h 
    SET_ODR_LIKAGE("_FPC_FIND_ERRORS_BY_ADDRESS")
    SET_ODR_LIKAGE("_FPC_FIND_ERRORS_BY_REGISTER")
    SET_ODR_LIKAGE("_FPC_HT_PRINT_TABLES_")
    SET_ODR_LIKAGE("_FPC_INIT_HASH_TABLE_")
    SET_ODR_LIKAGE("_FPC_WRITE_AND_PRINT_TO_JSON_")
    SET_ODR_LIKAGE("_FPC_ADDRESS_RANGE_UPDATE_")

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

  GlobalVariable *line_rel_error_head = nullptr;
  line_rel_error_head = mod->getGlobalVariable("_FPC_LINE_REL_ERROR_HEAD_", true);
  assert(line_rel_error_head && "Invalid table!");
  line_rel_error_head->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *line_rel_error_table = nullptr;
  line_rel_error_table = mod->getGlobalVariable("_FPC_LINE_REL_ERROR_TABLE_", true);
  assert(line_rel_error_table && "Invalid table!");
  line_rel_error_table->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *lines_to_keep = nullptr;
  lines_to_keep = mod->getGlobalVariable("_FPC_LINES_TO_KEEP_", true);
  assert(lines_to_keep && "Invalid table!");
  lines_to_keep->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *fpc_data_manager = nullptr;
  fpc_data_manager = mod->getGlobalVariable("FPC_DATA_MANAGER", true);
  assert(fpc_data_manager && "Invalid table!");
  fpc_data_manager->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *ret_error_stack = nullptr;
  ret_error_stack = mod->getGlobalVariable("_FPC_RET_ERR_STACK_", true);
  assert(ret_error_stack && "Invalid table!");
  ret_error_stack->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *ret_shadow_stack = nullptr;
  ret_shadow_stack = mod->getGlobalVariable("_FPC_RET_SHADOW_STACK_", true);
  assert(ret_shadow_stack && "Invalid table!");
  ret_shadow_stack->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *ret_rel_error_stack = nullptr;
  ret_rel_error_stack = mod->getGlobalVariable("_FPC_RET_REL_ERR_STACK_", true);
  assert(ret_rel_error_stack && "Invalid table!");
  ret_rel_error_stack->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *ret_stack_top = nullptr;
  ret_stack_top = mod->getGlobalVariable("_FPC_RET_STACK_TOP_", true);
  assert(ret_stack_top && "Invalid table!");
  ret_stack_top->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *ret_func_stack = nullptr;
  ret_func_stack = mod->getGlobalVariable("_FPC_RET_FUNC_STACK_", true);
  assert(ret_func_stack && "Invalid table!");
  ret_func_stack->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *arg_err_buf = nullptr;
  arg_err_buf = mod->getGlobalVariable("_FPC_ARG_ERR_BUF_", true);
  assert(arg_err_buf && "Invalid table!");
  arg_err_buf->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *arg_shadow_buf = nullptr;
  arg_shadow_buf = mod->getGlobalVariable("_FPC_ARG_SHADOW_BUF_", true);
  assert(arg_shadow_buf && "Invalid table!");
  arg_shadow_buf->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *arg_rel_err_buf = nullptr;
  arg_rel_err_buf = mod->getGlobalVariable("_FPC_ARG_REL_ERR_BUF_", true);
  assert(arg_rel_err_buf && "Invalid table!");
  arg_rel_err_buf->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  GlobalVariable *arg_buf_count = nullptr;
  arg_buf_count = mod->getGlobalVariable("_FPC_ARG_BUF_COUNT_", true);
  assert(arg_buf_count && "Invalid variable!");
  arg_buf_count->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  /* Site counter table: LinkOnceODR like the other runtime globals, so the
   * occurrence index is shared across TUs. Not asserted; a runtime without
   * the counter is still valid. */
  if (GlobalVariable *site_tab =
          mod->getGlobalVariable("_FPC_SITE_TAB_", true))
    site_tab->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);
  if (GlobalVariable *site_init =
          mod->getGlobalVariable("_FPC_SITE_TAB_INIT_", true))
    site_init->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);
  if (GlobalVariable *site_used =
          mod->getGlobalVariable("_FPC_SITE_TAB_USED_", true))
    site_used->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);
  if (GlobalVariable *site_ovf =
          mod->getGlobalVariable("_FPC_SITE_TAB_OVERFLOW_", true))
    site_ovf->setLinkage(GlobalValue::LinkageTypes::LinkOnceODRLinkage);

  // Set module filename
  module_filename = CUDAAnalysis::getFileNameFromModule(M);

  /* Site map, built here on the pristine module: the enumeration is
   * positional, so it must run before any instrumentation is inserted. Site
   * count must match brtrace (check_sites.py). */
  /* Opt level comes from the environment; recorded in the manifest so
   * check_sites.py can refuse a cross-level comparison. */
  const char *fpc_opt_env = getenv("FPC_OPT_LEVEL");
  site_map.reset(new FPCSite::SiteMap(*M, fpc_opt_env ? fpc_opt_env : ""));
  site_map->writeManifest(*M);
  if (getenv("FPC_BRANCH_FLIP") != NULL)
  {
    errs() << "[FPChecker] " << module_filename << " (mod "
           << site_map->getModuleId() << "): " << site_map->getNumSites()
           << " FP-controlled branch sites";
    if (unsigned n_multi = site_map->countMultiFCmpSites())
      errs() << ", " << n_multi << " with multiple fcmps";
    errs() << "\n";
  }
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
  assert((fpc_fp32_cmp_function != nullptr) && "Function not initialized!");
  assert((fpc_fp32_phi_function != nullptr) && "Function not initialized!");
  assert((fpc_fp32_branch_function != nullptr) && "Function not initialized!");
  assert((fp32_memcpy_function != nullptr) && "Function not initialized!");
  assert((fpc_fp32_push_ret_error != nullptr) && "Function not initialized!");
  assert((fpc_fp32_pop_ret_error != nullptr) && "Function not initialized!");
  assert((fpc_fp32_push_arg_error != nullptr) && "Function not initialized!");
  assert((fpc_fp32_pop_arg_error != nullptr) && "Function not initialized!");
  assert((fpc_fp32_math_error != nullptr) && "Function not initialized!");

  // Warning message
  // Check if the function calls other functions with floating-point values
/*   if (functionCallsFunctionWithFloatingPointValues(f))
  {
    CUDAAnalysis::Logging::info(
        ("*** WARNING *** Function " + f->getName() +
         " calls functions that return floating-point values!")
            .str()
            .c_str());
  } */

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
  // Create a constant string for the file name, passed directly to
  // instrumented calls (avoids loading from a global that can be
  // corrupted by relocation issues in shared libraries).
  Constant *loadInst_filename = builder.CreateGlobalStringPtr(module_filename);
  //  -------------------------------------------------------------------------------

  // ============= Pop argument errors into callee parameters ===================
  {
    int fpParamIndex = 0;
    for (auto &arg : f->args())
    {
      if (arg.getType()->isFloatTy() || arg.getType()->isDoubleTy())
      {
        IRBuilder<> popBuilder(first_inst);

        std::string paramRegName;
        llvm::raw_string_ostream rso(paramRegName);
        arg.printAsOperand(rso, false);
        rso.flush();

        ConstantInt *paramIndexVal =
            ConstantInt::get(mod->getContext(), APInt(32, fpParamIndex, true));

        std::vector<Value *> popArgs;
        popArgs.push_back(paramIndexVal);
        {
          // Runtime fallback value.
          Value *av = &arg;
          if (av->getType()->isFloatTy())
            av = popBuilder.CreateFPExt(av, Type::getDoubleTy(mod->getContext()));
          popArgs.push_back(av);
        }
        popArgs.push_back(popBuilder.CreateGlobalStringPtr(paramRegName));
        popArgs.push_back(popBuilder.CreateGlobalStringPtr(f->getName()));

        ArrayRef<Value *> popArgs_ref(popArgs);
        CallInst *hookCall = popBuilder.CreateCall(fpc_fp32_pop_arg_error, popArgs_ref);
        (*insrtrumented_instructions)++;

        setFakeDebugLocation(first_inst, hookCall, f);

        fpParamIndex++;
      }
    }
  }

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
          /*           errs() << "Instrumenting memcpy/memmove in function " << f->getName() << "\n";
                    errs() << "  Instruction: ";
                    inst->print(errs());
                    errs() << "\n"; */

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

      // ============= Propagate error through MPI communication calls ==========
      // MPI calls like MPI_Allreduce, MPI_Reduce, MPI_Bcast copy FP data between
      // buffers. Without this hook the error associated with the source buffer
      // is lost because the MPI library writes to the destination buffer outside
      // of the instrumented store path.
      if (auto *callInst = llvm::dyn_cast<llvm::CallInst>(inst))
      {
        Function *calledFunc = callInst->getCalledFunction();
        if (calledFunc)
        {
          std::string funcName = calledFunc->getName().str();

          // Match *MPI_Allreduce* or *MPI_Reduce* (covers wrappers like hypre_MPI_Allreduce)
          bool isMPIAllreduce = funcName.find("MPI_Allreduce") != std::string::npos;
          bool isMPIReduce    = funcName.find("MPI_Reduce") != std::string::npos &&
                                funcName.find("MPI_Allreduce") == std::string::npos;
          bool isMPIBcast     = funcName.find("MPI_Bcast") != std::string::npos;

          if (isMPIAllreduce && callInst->arg_size() >= 3)
          {
            // MPI_Allreduce(sendbuf, recvbuf, count, ...)
            BasicBlock::iterator nextInst(inst);
            ++nextInst;
            IRBuilder<> builder(&(*nextInst));

            Value *sendbuf = callInst->getArgOperand(0);
            Value *recvbuf = callInst->getArgOperand(1);
            Value *count   = callInst->getArgOperand(2);

            Value *dstAddr = builder.CreatePtrToInt(recvbuf, builder.getInt64Ty());
            Value *srcAddr = builder.CreatePtrToInt(sendbuf, builder.getInt64Ty());

            // Approximate byte size: count * sizeof(float) = count * 4
            Value *countI64 = builder.CreateZExtOrTrunc(count, builder.getInt64Ty());
            Value *byteSize = builder.CreateMul(countI64,
                ConstantInt::get(builder.getInt64Ty(), 4));

            std::vector<Value *> args;
            args.push_back(dstAddr);
            args.push_back(srcAddr);
            args.push_back(byteSize);
            args.push_back(ConstantInt::get(builder.getInt32Ty(), 1)); // size_type: i64
            args.push_back(ConstantInt::get(builder.getInt32Ty(), 0)); // ins_type: memcpy-like
            int lineNumber = CUDAAnalysis::getLineOfCode(inst);
            args.push_back(ConstantInt::get(builder.getInt32Ty(), lineNumber));
            args.push_back(loadInst_filename);

            ArrayRef<Value *> args_ref(args);
            CallInst *hookCall = builder.CreateCall(fp32_memcpy_function, args_ref);
            (*insrtrumented_instructions)++;
            setFakeDebugLocation(inst, hookCall, f);
          }
          else if (isMPIReduce && callInst->arg_size() >= 3)
          {
            // MPI_Reduce(sendbuf, recvbuf, count, ...)
            // Same layout as Allreduce for the first 3 args
            BasicBlock::iterator nextInst(inst);
            ++nextInst;
            IRBuilder<> builder(&(*nextInst));

            Value *sendbuf = callInst->getArgOperand(0);
            Value *recvbuf = callInst->getArgOperand(1);
            Value *count   = callInst->getArgOperand(2);

            Value *dstAddr = builder.CreatePtrToInt(recvbuf, builder.getInt64Ty());
            Value *srcAddr = builder.CreatePtrToInt(sendbuf, builder.getInt64Ty());

            Value *countI64 = builder.CreateZExtOrTrunc(count, builder.getInt64Ty());
            Value *byteSize = builder.CreateMul(countI64,
                ConstantInt::get(builder.getInt64Ty(), 4));

            std::vector<Value *> args;
            args.push_back(dstAddr);
            args.push_back(srcAddr);
            args.push_back(byteSize);
            args.push_back(ConstantInt::get(builder.getInt32Ty(), 1));
            args.push_back(ConstantInt::get(builder.getInt32Ty(), 0));
            int lineNumber = CUDAAnalysis::getLineOfCode(inst);
            args.push_back(ConstantInt::get(builder.getInt32Ty(), lineNumber));
            args.push_back(loadInst_filename);

            ArrayRef<Value *> args_ref(args);
            CallInst *hookCall = builder.CreateCall(fp32_memcpy_function, args_ref);
            (*insrtrumented_instructions)++;
            setFakeDebugLocation(inst, hookCall, f);
          }
          else if (isMPIBcast && callInst->arg_size() >= 2)
          {
            // MPI_Bcast(buffer, count, ...) — in-place, so copy error to itself
            // (re-associate the error with the same address after the opaque call)
            BasicBlock::iterator nextInst(inst);
            ++nextInst;
            IRBuilder<> builder(&(*nextInst));

            Value *buffer = callInst->getArgOperand(0);
            Value *count  = callInst->getArgOperand(1);

            Value *addr = builder.CreatePtrToInt(buffer, builder.getInt64Ty());

            Value *countI64 = builder.CreateZExtOrTrunc(count, builder.getInt64Ty());
            Value *byteSize = builder.CreateMul(countI64,
                ConstantInt::get(builder.getInt64Ty(), 4));

            std::vector<Value *> args;
            args.push_back(addr);  // dst = src (in-place)
            args.push_back(addr);
            args.push_back(byteSize);
            args.push_back(ConstantInt::get(builder.getInt32Ty(), 1));
            args.push_back(ConstantInt::get(builder.getInt32Ty(), 0));
            int lineNumber = CUDAAnalysis::getLineOfCode(inst);
            args.push_back(ConstantInt::get(builder.getInt32Ty(), lineNumber));
            args.push_back(loadInst_filename);

            ArrayRef<Value *> args_ref(args);
            CallInst *hookCall = builder.CreateCall(fp32_memcpy_function, args_ref);
            (*insrtrumented_instructions)++;
            setFakeDebugLocation(inst, hookCall, f);
          }
        }
      }

      // ============= Instrument floating-point compare instructions ==========
      if (auto *cmpInst = llvm::dyn_cast<llvm::FCmpInst>(inst))
      {
        if (cmpInst->getOperand(0)->getType()->isFloatTy() &&
            cmpInst->getOperand(1)->getType()->isFloatTy())
        {
          int predicateCode = -1;
          /* Bit 3 marks an unordered predicate (true on NaN); low bits are the
           * comparison, unchanged. */
          switch (cmpInst->getPredicate())
          {
          case llvm::CmpInst::Predicate::FCMP_OEQ:
            predicateCode = 0;
            break;
          case llvm::CmpInst::Predicate::FCMP_UEQ:
            predicateCode = 0 | 8;
            break;
          case llvm::CmpInst::Predicate::FCMP_ONE:
            predicateCode = 1;
            break;
          case llvm::CmpInst::Predicate::FCMP_UNE:
            predicateCode = 1 | 8;
            break;
          case llvm::CmpInst::Predicate::FCMP_OLT:
            predicateCode = 2;
            break;
          case llvm::CmpInst::Predicate::FCMP_ULT:
            predicateCode = 2 | 8;
            break;
          case llvm::CmpInst::Predicate::FCMP_OLE:
            predicateCode = 3;
            break;
          case llvm::CmpInst::Predicate::FCMP_ULE:
            predicateCode = 3 | 8;
            break;
          case llvm::CmpInst::Predicate::FCMP_OGT:
            predicateCode = 4;
            break;
          case llvm::CmpInst::Predicate::FCMP_UGT:
            predicateCode = 4 | 8;
            break;
          case llvm::CmpInst::Predicate::FCMP_OGE:
            predicateCode = 5;
            break;
          case llvm::CmpInst::Predicate::FCMP_UGE:
            predicateCode = 5 | 8;
            break;
          default:
            predicateCode = -1;
            break;
          }

          if (predicateCode >= 0)
          {
            BasicBlock::iterator nextInst(inst);
            ++nextInst;
            IRBuilder<> builder(&(*nextInst));

            /* File and line from this instruction's own DILocation, walking
             * getInlinedAt(), so a site in an inlined header is not reported
             * under the including TU. */
            int lineNumber = FPCSite::lineOf(inst);
            std::string siteFile = FPCSite::fileOf(inst);
            Value *siteFileStr = builder.CreateGlobalStringPtr(siteFile);
            if (lineNumber != -1)
            {
              std::string resultName;
              llvm::raw_string_ostream resultStream(resultName);
              cmpInst->printAsOperand(resultStream, false);
              resultStream.flush();

              std::string lhsName;
              llvm::raw_string_ostream lhsStream(lhsName);
              cmpInst->getOperand(0)->printAsOperand(lhsStream, false);
              lhsStream.flush();

              std::string rhsName;
              llvm::raw_string_ostream rhsStream(rhsName);
              cmpInst->getOperand(1)->printAsOperand(rhsStream, false);
              rhsStream.flush();

              std::vector<Value *> args;
              args.push_back(builder.CreateZExt(cmpInst, builder.getInt32Ty(), "cmp_cond"));
              args.push_back(cmpInst->getOperand(0));
              args.push_back(cmpInst->getOperand(1));
              args.push_back(ConstantInt::get(builder.getInt32Ty(), predicateCode));
              args.push_back(ConstantInt::get(builder.getInt32Ty(), lineNumber));
              args.push_back(siteFileStr);
              args.push_back(builder.CreateGlobalStringPtr(resultName));
              args.push_back(builder.CreateGlobalStringPtr(lhsName));
              args.push_back(builder.CreateGlobalStringPtr(rhsName));
              args.push_back(builder.CreateGlobalStringPtr(f->getName()));
               // Whether this compare directly controls a branch.
              int isBrCtrl = isBranchControllingFCmp(inst) ? 1 : 0;
              args.push_back(ConstantInt::get(builder.getInt32Ty(), isBrCtrl));

              /* -1: this comparison controls no numbered branch (select,
               * zext-to-bool, call). Reported and counted out of scope, not
               * dropped. */
              int32_t siteId = site_map ? site_map->siteFor(inst) : -1;
              uint32_t modId = site_map ? site_map->getModuleId() : 0u;
              args.push_back(ConstantInt::get(builder.getInt32Ty(), modId));
              args.push_back(ConstantInt::get(builder.getInt32Ty(), siteId));

              ArrayRef<Value *> args_ref(args);
              CallInst *hookCall = builder.CreateCall(fpc_fp32_cmp_function, args_ref);
              (*insrtrumented_instructions)++;

              assert(hookCall && "Invalid call instruction!");
              setFakeDebugLocation(inst, hookCall, f);
            }
          }
        }
      }

      // ============= Push argument errors to callee ============================
      if (auto *callInst = llvm::dyn_cast<llvm::CallBase>(inst))
      {
        Function *calledFunc = callInst->getCalledFunction();
        bool isIntrinsic = calledFunc && calledFunc->isIntrinsic();
        bool isFPCInternal = calledFunc && calledFunc->getName().str().find("_FPC_") != std::string::npos;
        std::string _mathTmp;
        bool isMathCall = isSupportedMathCall(callInst, _mathTmp);
        if (!isIntrinsic && !isFPCInternal && !isMathCall)
        {
          int fpArgIndex = 0;
          for (unsigned argIdx = 0; argIdx < callInst->arg_size(); ++argIdx)
          {
            Value *argVal = callInst->getArgOperand(argIdx);
            if (argVal->getType()->isFloatTy() || argVal->getType()->isDoubleTy())
            {
              IRBuilder<> pushBuilder(inst); // insert before the call

              std::string argRegName;
              llvm::raw_string_ostream rso(argRegName);
              argVal->printAsOperand(rso, false);
              rso.flush();

              ConstantInt *argIndexVal =
                  ConstantInt::get(mod->getContext(), APInt(32, fpArgIndex, true));

              Value *argShadowValue = argVal;
              if (argShadowValue->getType()->isFloatTy())
                argShadowValue = pushBuilder.CreateFPExt(argShadowValue, pushBuilder.getDoubleTy());

              std::vector<Value *> pushArgs;
              pushArgs.push_back(argIndexVal);
              pushArgs.push_back(argShadowValue);
              pushArgs.push_back(pushBuilder.CreateGlobalStringPtr(argRegName));
              pushArgs.push_back(pushBuilder.CreateGlobalStringPtr(f->getName()));

              ArrayRef<Value *> pushArgs_ref(pushArgs);
              CallInst *hookCall = pushBuilder.CreateCall(fpc_fp32_push_arg_error, pushArgs_ref);
              (*insrtrumented_instructions)++;

              setFakeDebugLocation(inst, hookCall, f);

              fpArgIndex++;
            }
          }
        }
      }

      // ============= Propagate callee return error to call result =============
      if (auto *callInst = llvm::dyn_cast<llvm::CallBase>(inst))
      {
        if (callInst->getType()->isFloatTy() || callInst->getType()->isDoubleTy())
        {
          Function *calledFunction = callInst->getCalledFunction();
          if (calledFunction && calledFunction->isIntrinsic())
          {
            // Intrinsics (e.g. llvm.fmuladd) do not participate in
            // interprocedural return propagation, but they may still
            // be FP arithmetic ops that need error calculation below.
            // Do NOT continue here — just skip the pop hook.
          }
          else
          if (calledFunction && calledFunction->getName().str().find("_FPC_") != std::string::npos)
          {
            // Skip internal FPChecker runtime calls.
          }
          else
          {
            // Skip pop-ret for recognized math calls; MATH_ERROR handles
            // their error tracking directly.
            std::string _mathTmp2;
            if (isSupportedMathCall(callInst, _mathTmp2))
            {
              // Skip — math error hook will record the result error.
            }
            else
            {
            IRBuilder<> builder(inst->getContext());
            if (auto *invokeInst = llvm::dyn_cast<llvm::InvokeInst>(callInst))
            {
              BasicBlock *normalDest = invokeInst->getNormalDest();
              builder.SetInsertPoint(&*normalDest->getFirstInsertionPt());
            }
            else
            {
              BasicBlock::iterator nextInst(inst);
              ++nextInst;
              builder.SetInsertPoint(&(*nextInst));
            }

            std::string resultName;
            llvm::raw_string_ostream rso(resultName);
            callInst->printAsOperand(rso, false);
            rso.flush();

            std::vector<Value *> args;
            args.push_back(builder.CreateGlobalStringPtr(resultName));
            // Runtime fallback value; hook takes double.
            {
              Value *rv = callInst;
              if (rv->getType()->isFloatTy())
                rv = builder.CreateFPExt(rv, Type::getDoubleTy(mod->getContext()));
              args.push_back(rv);
            }
            args.push_back(builder.CreateGlobalStringPtr(f->getName()));
            std::string calleeName = "";
            if (calledFunction)
              calleeName = calledFunction->getName().str();
            args.push_back(builder.CreateGlobalStringPtr(calleeName));
            int lineNumber = CUDAAnalysis::getLineOfCode(inst);
            ConstantInt *locId =
                ConstantInt::get(mod->getContext(), APInt(32, lineNumber, true));
            args.push_back(locId);
            args.push_back(loadInst_filename);

            ArrayRef<Value *> args_ref(args);
            CallInst *hookCall = builder.CreateCall(fpc_fp32_pop_ret_error, args_ref);
            (*insrtrumented_instructions)++;

            assert(hookCall && "Invalid call instruction!");
            setFakeDebugLocation(inst, hookCall, f);
            }
          }
        }
      }

      // ============= Instrument math function calls ==========================
      if (auto *callInst = llvm::dyn_cast<llvm::CallInst>(inst))
      {
        std::string mathName;
        if (callInst->getType()->isFloatTy() && isSupportedMathCall(callInst, mathName))
        {
          BasicBlock::iterator nextInst(inst);
          ++nextInst;
          IRBuilder<> builder(&(*nextInst));

          // Collect the result register name
          std::string resultRegName;
          { llvm::raw_string_ostream rso(resultRegName); callInst->printAsOperand(rso, false); rso.flush(); }

          // Collect up to 3 FP operands
          Value *fpArgs[3] = { nullptr, nullptr, nullptr };
          std::string fpArgNames[3] = { "", "", "" };
          int fpCount = 0;
          for (unsigned ai = 0; ai < callInst->arg_size() && fpCount < 3; ++ai)
          {
            Value *argVal = callInst->getArgOperand(ai);
            if (argVal->getType()->isFloatTy() || argVal->getType()->isDoubleTy())
            {
              fpArgs[fpCount] = argVal;
              llvm::raw_string_ostream rso(fpArgNames[fpCount]);
              argVal->printAsOperand(rso, false);
              rso.flush();
              fpCount++;
            }
          }

          // Pad unused operands with 0.0f
          for (int pi = fpCount; pi < 3; ++pi)
            fpArgs[pi] = ConstantFP::get(builder.getFloatTy(), 0.0f);

          // If an FP arg is double, cast it to float for the hook signature
          for (int pi = 0; pi < 3; ++pi)
          {
            if (fpArgs[pi] && fpArgs[pi]->getType()->isDoubleTy())
              fpArgs[pi] = builder.CreateFPTrunc(fpArgs[pi], builder.getFloatTy());
          }

          int lineNumber = CUDAAnalysis::getLineOfCode(inst);
          if (lineNumber == -1)
            goto skip_math;

          {
            ConstantInt *locId =
                ConstantInt::get(mod->getContext(), APInt(32, lineNumber, true));

            std::vector<Value *> args;
            args.push_back(callInst);                                          // result (float x)
            args.push_back(fpArgs[0]);                                         // arg1 (float y)
            args.push_back(fpArgs[1]);                                         // arg2 (float z)
            args.push_back(fpArgs[2]);                                         // arg3 (float w)
            args.push_back(locId);                                             // loc
            args.push_back(loadInst_filename);                                 // file_name
            args.push_back(builder.CreateGlobalStringPtr(mathName));            // math_func_name
            args.push_back(builder.CreateGlobalStringPtr(resultRegName));       // result_name
            args.push_back(builder.CreateGlobalStringPtr(fpArgNames[0]));       // op1_name
            args.push_back(builder.CreateGlobalStringPtr(fpArgNames[1]));       // op2_name
            args.push_back(builder.CreateGlobalStringPtr(fpArgNames[2]));       // op3_name
            args.push_back(builder.CreateGlobalStringPtr(f->getName()));        // function_name

            ArrayRef<Value *> args_ref(args);
            CallInst *hookCall = builder.CreateCall(fpc_fp32_math_error, args_ref);
            (*insrtrumented_instructions)++;

            assert(hookCall && "Invalid call instruction!");
            setFakeDebugLocation(inst, hookCall, f);
          }
          skip_math:;
        }
      }

      // ============= Instrument for FP arithmetic operations ===================
      if ((isFPOperationWithError(inst) ||
           (inst->getOpcode() == Instruction::FNeg) ||
           (inst->getOpcode() == Instruction::Select)) &&
          (inst->getOperand(0)->getType()->isFloatTy() ||
           (inst->getNumOperands() >= 2 && inst->getOperand(1)->getType()->isFloatTy())))
      {
        DebugLoc loc = inst->getDebugLoc();

        // Create builder to add stuff after the instruction
        BasicBlock::iterator nextInst(inst);
        nextInst++;
        if (inst->getOpcode() == Instruction::Select)
        {
          while (nextInst != bb->end())
          {
            auto *nextCall = llvm::dyn_cast<llvm::CallBase>(&*nextInst);
            Function *nextCalled = nextCall ? nextCall->getCalledFunction() : nullptr;
            if (!nextCalled ||
                nextCalled->getName().str().find("_FPC_") == std::string::npos)
              break;
            ++nextInst;
          }
        }

        IRBuilder<> builder(inst->getContext());
        if (nextInst != bb->end())
          builder.SetInsertPoint(&(*nextInst));
        else
          builder.SetInsertPoint(&*bb);

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

        if (inst->getOpcode() == Instruction::Select)
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

      // ============= Track return of floating-point values ====================
      if (auto *retInst = llvm::dyn_cast<llvm::ReturnInst>(inst))
      {
        Value *retVal = retInst->getReturnValue();
        if (retVal && (retVal->getType()->isFloatTy() || retVal->getType()->isDoubleTy()))
        {
          IRBuilder<> builder(inst);
          std::string retRegName;
          llvm::raw_string_ostream rso(retRegName);
          retVal->printAsOperand(rso, false);
          rso.flush();

          std::vector<Value *> args;
          args.push_back(builder.CreateGlobalStringPtr(retRegName));
          args.push_back(builder.CreateGlobalStringPtr(f->getName()));

          ArrayRef<Value *> args_ref(args);
          CallInst *hookCall = builder.CreateCall(fpc_fp32_push_ret_error, args_ref);
          (*insrtrumented_instructions)++;

          assert(hookCall && "Invalid call instruction!");
          setFakeDebugLocation(inst, hookCall, f);
        }
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
          {
            // Runtime fallback value.
            Value *pv = phiInst;
            if (pv->getType()->isFloatTy())
              pv = builder.CreateFPExt(pv, Type::getDoubleTy(mod->getContext()));
            args.push_back(pv);
          }
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

// Returns true if inst is an FCmp on float operands whose result
// directly controls a conditional branch.
bool CPUFPInstrumentation_error::isBranchControllingFCmp(const Instruction *inst)
{
  if (inst->getOpcode() != Instruction::FCmp)
    return false;

  const CmpInst *cmpInst = dyn_cast<CmpInst>(inst);
  if (!cmpInst)
    return false;

  if (!cmpInst->getOperand(0)->getType()->isFloatTy())
    return false;

  for (const User *U : inst->users())
  {
    // Direct: fcmp result is the condition of a conditional branch.
    if (const BranchInst *br = dyn_cast<BranchInst>(U))
    {
      if (br->isConditional() && br->getCondition() == inst)
        return true;
    }
    // Direct: fcmp result is the condition of a select (ternary / min / max).
    if (const SelectInst *sel = dyn_cast<SelectInst>(U))
    {
      if (sel->getCondition() == inst)
        return true;
    }
    // Indirect: fcmp -> zext/sext/trunc -> branch/select/call (e.g. a compare
    // passed to a helper like fbr_emit at -O0).
    if (isa<ZExtInst>(U) || isa<SExtInst>(U) || isa<TruncInst>(U))
    {
      for (const User *U2 : U->users())
      {
        if (isa<BranchInst>(U2) || isa<SelectInst>(U2) || isa<CallInst>(U2))
          return true;
      }
    }
    // fcmp -> phi -> branch/select: the -O0 lowering of a short-circuit `&&`
    // whose right operand is an FP compare. Followed only when every incoming
    // is an fcmp or a constant i1, matching FPC_SiteId.h::isFPControlled().
    if (const PHINode *phi = dyn_cast<PHINode>(U))
    {
      bool onlyFCmpOrConst = true;
      bool sawFCmp = false;
      for (unsigned i = 0, e = phi->getNumIncomingValues(); i != e; ++i)
      {
        const Value *in = phi->getIncomingValue(i);
        if (isa<FCmpInst>(in))
          sawFCmp = true;
        else if (!isa<ConstantInt>(in))
        {
          onlyFCmpOrConst = false;
          break;
        }
      }
      if (onlyFCmpOrConst && sawFCmp)
      {
        for (const User *U2 : phi->users())
        {
          if (isa<BranchInst>(U2) || isa<SelectInst>(U2))
            return true;
        }
      }
    }
    // fcmp result is a direct operand of a select or call.
    if (isa<SelectInst>(U) || isa<CallInst>(U))
      return true;
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

/// Returns true if the call instruction is a supported math function,
/// and fills normalizedName with the canonical name (e.g. "sin", "pow").
bool CPUFPInstrumentation_error::isSupportedMathCall(const CallBase *CI, std::string &normalizedName)
{
  // Check for LLVM math intrinsics
  if (auto *intrin = dyn_cast<IntrinsicInst>(CI))
  {
    auto id = intrin->getIntrinsicID();
    // Skip FMA — it is already handled in the arithmetic instrumentation path
    if (id == Intrinsic::fmuladd || id == Intrinsic::fma)
      return false;

    switch (id)
    {
    case Intrinsic::sin:        normalizedName = "sin"; return true;
    case Intrinsic::cos:        normalizedName = "cos"; return true;
    case Intrinsic::tan:        normalizedName = "tan"; return true;
    case Intrinsic::asin:       normalizedName = "asin"; return true;
    case Intrinsic::acos:       normalizedName = "acos"; return true;
    case Intrinsic::atan:       normalizedName = "atan"; return true;
    case Intrinsic::sinh:       normalizedName = "sinh"; return true;
    case Intrinsic::cosh:       normalizedName = "cosh"; return true;
    case Intrinsic::tanh:       normalizedName = "tanh"; return true;
    case Intrinsic::exp:        normalizedName = "exp"; return true;
    case Intrinsic::exp2:       normalizedName = "exp2"; return true;
    case Intrinsic::log:        normalizedName = "log"; return true;
    case Intrinsic::log2:       normalizedName = "log2"; return true;
    case Intrinsic::log10:      normalizedName = "log10"; return true;
    case Intrinsic::sqrt:       normalizedName = "sqrt"; return true;
    case Intrinsic::fabs:       normalizedName = "fabs"; return true;
    case Intrinsic::pow:        normalizedName = "pow"; return true;
    case Intrinsic::ceil:       normalizedName = "ceil"; return true;
    case Intrinsic::floor:      normalizedName = "floor"; return true;
    case Intrinsic::trunc:      normalizedName = "trunc"; return true;
    case Intrinsic::round:      normalizedName = "round"; return true;
    case Intrinsic::nearbyint:  normalizedName = "nearbyint"; return true;
    case Intrinsic::rint:       normalizedName = "rint"; return true;
    default: return false;
    }
  }

  // Check for libm function calls (e.g. sinf, sin, cosf, cos, ...)
  Function *calledFunc = CI->getCalledFunction();
  if (!calledFunc)
    return false;

  std::string name = calledFunc->getName().str();

  // Table of recognized libm names -> normalized name
  static const std::pair<const char *, const char *> libmTable[] = {
    {"sinf", "sin"}, {"sin", "sin"},
    {"cosf", "cos"}, {"cos", "cos"},
    {"tanf", "tan"}, {"tan", "tan"},
    {"asinf", "asin"}, {"asin", "asin"},
    {"acosf", "acos"}, {"acos", "acos"},
    {"atanf", "atan"}, {"atan", "atan"},
    {"atan2f", "atan2"}, {"atan2", "atan2"},
    {"sinhf", "sinh"}, {"sinh", "sinh"},
    {"coshf", "cosh"}, {"cosh", "cosh"},
    {"tanhf", "tanh"}, {"tanh", "tanh"},
    {"asinhf", "asinh"}, {"asinh", "asinh"},
    {"acoshf", "acosh"}, {"acosh", "acosh"},
    {"atanhf", "atanh"}, {"atanh", "atanh"},
    {"expf", "exp"}, {"exp", "exp"},
    {"exp2f", "exp2"}, {"exp2", "exp2"},
    {"expm1f", "expm1"}, {"expm1", "expm1"},
    {"logf", "log"}, {"log", "log"},
    {"log2f", "log2"}, {"log2", "log2"},
    {"log10f", "log10"}, {"log10", "log10"},
    {"log1pf", "log1p"}, {"log1p", "log1p"},
    {"logbf", "logb"}, {"logb", "logb"},
    {"sqrtf", "sqrt"}, {"sqrt", "sqrt"},
    {"cbrtf", "cbrt"}, {"cbrt", "cbrt"},
    {"powf", "pow"}, {"pow", "pow"},
    {"hypotf", "hypot"}, {"hypot", "hypot"},
    {"fabsf", "fabs"}, {"fabs", "fabs"},
    {"ceilf", "ceil"}, {"ceil", "ceil"},
    {"floorf", "floor"}, {"floor", "floor"},
    {"truncf", "trunc"}, {"trunc", "trunc"},
    {"roundf", "round"}, {"round", "round"},
    {"nearbyintf", "nearbyint"}, {"nearbyint", "nearbyint"},
    {"rintf", "rint"}, {"rint", "rint"},
    {"fmodf", "fmod"}, {"fmod", "fmod"},
    {"remainderf", "remainder"}, {"remainder", "remainder"},
  };

  for (const auto &entry : libmTable)
  {
    if (name == entry.first)
    {
      normalizedName = entry.second;
      return true;
    }
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
