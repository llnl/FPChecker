#ifndef SRC_RUNTIME_ERROR_FP64_H_
#define SRC_RUNTIME_ERROR_FP64_H_

#include "FPC_Hashtable_Error_FP64.h"
#include "FPC_FloatSeries_List.h"
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <float.h>
#include <string.h>
#include <sys/stat.h>

#define FPC_MAX(a, b) (((a) > (b)) ? (a) : (b))

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

/**
 * Operations table
 * -------------------------
 * ADD = 0
 * SUB = 1
 * MUL = 2
 * DIV = 3
 * CMP = 4 (comparison)
 * REM = 5 (reminder)
 * FMA = 6 (FMA function call)
 * NEG = 7 (negation)
 * SELECT = 8 (select)
 * -------------------------
 **/

// We store the file name and directory in this variable (one variable per file)
__attribute__((used)) static char *_FPC_FILE_NAME_;

// Program name and input
int _FPC_PROG_INPUTS;
char **_FPC_PROG_ARGS;

// Hash table pointers for address and register errors
_FPC_ADDRESS_HTABLE_FP64_T *_FPC_ADDRESS_HT_FP64_;
_FPC_REGISTER_HTABLE_FP64_T *_FPC_REGISTER_HT_FP64_;

// Lines of code to save values from
// Env variable: FPC_SAVE_LINE_ERRORS=3,4
int *_FPC_LINES_TO_KEEP_;
FPC_SeriesManager *FPC_DATA_MANAGER;

// Maximum number of warnings to print
#define MAX_WARNINGS 3
int _FPC_WARNING_COUNT_;

// Last basic block name
#define _FPC_BB_NAME_SIZE_ 512                   // max size of a basic block name - for example: "%bb_26"
char _FPC_LAST_BASIC_BLOCK_[_FPC_BB_NAME_SIZE_]; // Last basic block ID

// Call/return double precision error propagation stack
#define _FPC_RET_STACK_MAX_ 8192
long double _FPC_RET_ERR_STACK_FP64_[_FPC_RET_STACK_MAX_];
long double _FPC_RET_REL_ERR_STACK_FP64_[_FPC_RET_STACK_MAX_];
char _FPC_RET_FUNC_STACK_FP64_[_FPC_RET_STACK_MAX_][_FPC_BB_NAME_SIZE_];
int _FPC_RET_STACK_TOP_FP64_;

// Caller-to-callee argument error propagation buffer
#define _FPC_ARG_BUF_MAX_ 256
long double _FPC_ARG_ERR_BUF_FP64_[_FPC_ARG_BUF_MAX_];
long double _FPC_ARG_REL_ERR_BUF_FP64_[_FPC_ARG_BUF_MAX_];
int _FPC_ARG_BUF_COUNT_FP64_;

// Forward declaration for lazy initialization helper.
void _FPC_INIT_FPCHECKER_FP64();
void _FPC_PRINT_LOCATIONS_FP64();

static inline void _FPC_ENSURE_RUNTIME_READY_()
{
  static int fpc_atexit_registered = 0;

  if (_FPC_ADDRESS_HT_FP64_ == NULL || _FPC_REGISTER_HT_FP64_ == NULL)
  {
    _FPC_INIT_FPCHECKER_FP64();

    if (!fpc_atexit_registered)
    {
      atexit(_FPC_PRINT_LOCATIONS_FP64);
      fpc_atexit_registered = 1;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* Initialize                                                                 */
/*----------------------------------------------------------------------------*/

void _FPC_INIT_HASH_TABLE_FP64()
{
#ifndef FPC_QUIET
  printf("#FPCHECKER: Initializing...\n");
#endif

  int64_t size = 1024;
  _FPC_ADDRESS_HT_FP64_  = _FPC_ADDRESS_HT_CREATE_FP64_(size);
  _FPC_REGISTER_HT_FP64_ = _FPC_REGISTER_HT_CREATE_FP64_(size);

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Done Initializing..........\n");
#endif
}

void _FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED()
{
  char *env_var = getenv("FPC_SAVE_LINE_ERRORS");
  if (env_var != NULL)
  {
    // Count commas to determine number of lines
    int count = 1;
    for (char *p = env_var; *p; p++)
    {
      if (*p == ',')
        count++;
    }

    _FPC_LINES_TO_KEEP_ = (int *)malloc((count + 1) * sizeof(int));
    if (_FPC_LINES_TO_KEEP_ == NULL)
    {
      fprintf(stderr, "FPCHECKER: ERROR: Failed to allocate memory for line errors.\n");
      exit(EXIT_FAILURE);
    }

    // Parse the line numbers
    char *token = strtok(env_var, ",");
    int index = 0;
    while (token != NULL)
    {
      _FPC_LINES_TO_KEEP_[index++] = atoi(token);
      token = strtok(NULL, ",");
    }
    // Mark the end of the array with -1
    _FPC_LINES_TO_KEEP_[index] = -1;

    FPC_DATA_MANAGER = FPC_create_manager();
    if (FPC_DATA_MANAGER == NULL)
    {
      fprintf(stderr, "FPCHECKER: ERROR: Failed to allocate memory for line errors.\n");
      exit(EXIT_FAILURE);
    }

    // Print _FPC_LINES_TO_KEEP_ for debugging
#ifndef FPC_QUIET
    printf("#FPCHECKER: Saving errors for lines: ");
    for (int i = 0; i < index; i++)
    {
      printf("%d ", _FPC_LINES_TO_KEEP_[i]);
    }
    printf("\n");
#endif
  }
  else
  {
    _FPC_LINES_TO_KEEP_ = NULL;
  }
}

void _FPC_INIT_FPCHECKER_FP64()
{
  if (_FPC_ADDRESS_HT_FP64_ != NULL && _FPC_REGISTER_HT_FP64_ != NULL)
  {
    return;
  }

  _FPC_PROG_INPUTS = 0;
  _FPC_LAST_BASIC_BLOCK_[0] = '\0';
  _FPC_RET_STACK_TOP_FP64_ = 0;
  _FPC_INIT_HASH_TABLE_FP64();
  _FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED();
}

void _FPC_INIT_ARGS_FPCHECKER_FP64(int argc, char **argv)
{
  if (_FPC_ADDRESS_HT_FP64_ != NULL && _FPC_REGISTER_HT_FP64_ != NULL)
  {
    _FPC_PROG_INPUTS = argc;
    _FPC_PROG_ARGS = argv;
    return;
  }

  _FPC_PROG_INPUTS = argc;
  _FPC_PROG_ARGS = argv;
  _FPC_LAST_BASIC_BLOCK_[0] = '\0';
  _FPC_RET_STACK_TOP_FP64_ = 0;
  _FPC_INIT_HASH_TABLE_FP64();
  _FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED();
}

void _FPC_PRINT_LOCATIONS_FP64()
{
  static int fpc_finalized = 0;

  if (fpc_finalized)
  {
    return;
  }

  fpc_finalized = 1;

  if (_FPC_ADDRESS_HT_FP64_ == NULL || _FPC_REGISTER_HT_FP64_ == NULL)
  {
    return;
  }

#ifndef FPC_QUIET
  printf("#FPCHECKER: Finalizing and writing traces...\n");
#endif

  _FPC_WRITE_AND_PRINT_TO_JSON_FP64(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);

  // Print values of al series being tracked
  if (FPC_DATA_MANAGER != NULL)
  {
    FPC_series_to_json(FPC_DATA_MANAGER);
  }
#ifndef FPC_QUIET
  else
  {
    printf("#FPCHECKER: No line error series to print.\n");
  }
#endif
}

/*----------------------------------------------------------------------------*/
/* Error Accumulation                                                        */
/*----------------------------------------------------------------------------*/

// Check that line is in _FPC_LINES_TO_KEEP_//
// If so, append in FPC_DATA_MANAGER
void FPC_APPEND_ERROR_LOG_ENTRY_FP64(int line, long double relative_error)
{
  if (_FPC_LINES_TO_KEEP_ == NULL)
    return;

  // Check if line is in _FPC_LINES_TO_KEEP_
  int found = 0;
  for (int i = 0; _FPC_LINES_TO_KEEP_[i] != -1; i++)
  {
    if (_FPC_LINES_TO_KEEP_[i] == line)
    {
      found = 1;
      break;
    }
  }

  if (found)
  {
    FPC_append_value(FPC_DATA_MANAGER, line, double(relative_error)); // Cast it to double becasue FPC_append_value function definition takes double.
  }
}

/*------------------------------------------------------------------*/
/* Store Function with Location Logging                             */
/*------------------------------------------------------------------*/

// *** Error Calculation *** //
// Instrumentation for STORE instructions
void _FPC_FP64_STORE_INST_(const char *reg, const char *function_name, uintptr_t address, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP64_STORE_INST_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_STORE_INST_:\n");
  printf("reg=%s, address=%lu\n", reg, address);
#endif

  long double error = 0.0;
  long double relative_error = 0.0;

  // Find if this register already has an error
  int found = _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, reg, function_name, &error, &relative_error);
  if (!found)
  {
    if (_FPC_WARNING_COUNT_ < MAX_WARNINGS)
    {
      _FPC_WARNING_COUNT_++;
      printf("#FPCHECKER: Warning: trying to store a register's value (%s) in function %s, but we don't have its error.\n",
             reg, function_name);
    }
  }

  // Update table based on the address
  // If address exists, update it
  // If address does not exist, insert new entry
  _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, reg, function_name, error, relative_error, file_name, loc);

  // Log location info if line is in _FPC_LINES_TO_KEEP_
  FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, relative_error);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_FP64_(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);
#endif

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP64_STORE_INST_..........\n");
#endif
}

// Instrumentation for LOAD instructions
void _FPC_FP64_LOAD_INST_(const char *load_reg, const char *function_name, uintptr_t address, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP64_LOAD_INST_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP64_LOAD_INST_:\n");
  printf("reg=%s, address=0x%016llx, func=%s\n", load_reg, (unsigned long long)address, function_name);
#endif

  long double error = 0.0;
  long double relative_error = 0.0;

  // Find what's at this memory address
  int found = _FPC_FIND_ERRORS_BY_ADDRESS_FP64(_FPC_ADDRESS_HT_FP64_, address, &error, &relative_error);
  if (found)
  {
    // Update register entry with this error
    _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, load_reg, function_name, error, relative_error, file_name, loc);

    // Log location info if line is in _FPC_LINES_TO_KEEP_
    FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, relative_error);
  }
  else
  {
    // No error found at this address, create register with zero error
    _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, load_reg, function_name, 0.0L, 0.0L, file_name, loc);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("LOAD: No data found at address %lu\n", address);
#endif
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_FP64_(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);
#endif

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP64_STORE_INST_..........\n");
#endif
}

void _FPC_FP64_BRANCH_(const char *basic_block_name)
{
#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP64_BRANCH_..........\n");
#endif

  // printf("BRANCH: BasicBlock %s\n", basic_block_name);
  strncpy(_FPC_LAST_BASIC_BLOCK_, basic_block_name, _FPC_BB_NAME_SIZE_ - 1);
  _FPC_LAST_BASIC_BLOCK_[_FPC_BB_NAME_SIZE_ - 1] = '\0';

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP64_BRANCH_..........\n");
#endif
}

// This function is called for PHI nodes in SSA form
// It is used to log the values that are being merged
void _FPC_FP64_PHI_(const char *phi_values, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP64_PHI_..........\n");
#endif

  // printf(">>>>>>>>>>>>>>>>>>>>> PHI: Values %s\n", phi_values);
  char input_copy[_FPC_BB_NAME_SIZE_ * 5];
  strncpy(input_copy, phi_values, sizeof(input_copy) - 1);
  input_copy[sizeof(input_copy) - 1] = '\0';

  char *register_name = strtok(input_copy, ":");
  char *second_token = strtok(NULL, ":");

  if (second_token)
  {
    char *saveptr;
    char *token = strtok_r(second_token, ";", &saveptr);
    while (token)
    {
      char *pipe_pos = strchr(token, '|');
      if (pipe_pos)
      {
        size_t first_len = pipe_pos - token;
        char first_substr[_FPC_BB_NAME_SIZE_];
        strncpy(first_substr, token, first_len);
        first_substr[first_len] = '\0';
        if (pipe_pos)
        {
          if (strcmp(pipe_pos + 1, _FPC_LAST_BASIC_BLOCK_) == 0)
          {
            long double old_error = 0.0;
            long double old_relative_error = 0.0;
            int found = _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, first_substr, function_name, &old_error, &old_relative_error);
            if (found)
            {
              _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, register_name, function_name, old_error, old_relative_error, "", 0);
            }
            else
            {
              // We don't have its error - create with zero error
              _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, register_name, function_name, 0.0L, 0.0L, "", 0);
              // exit(1);
            }
          }
        }
      }
      token = strtok_r(NULL, ";", &saveptr);
    }
  }

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP32_PHI_..........\n");
#endif
}

// inst_type: types of memcpy/memmove:
//    type = 0 -> llvm.memcpy
//    type = 1 -> llvm.memmove
//
// size_type:
//    type = 0 -> i32 (32-bit integer)
//    type = 1 -> i64 (64-bit integer)
void _FPC_FP64_MEMCPY_INST_(uintptr_t address_dst, uintptr_t address_src,
                            long int size, int size_type, int ins_type,
                            int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  if (ins_type == 0)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("_FPC_FP64_MEMCPY_INST_ (memcpy): src=0x%016llx, dst=0x%016llx, size=%zu, size_type=%d\n",
           (unsigned long long)address_src, (unsigned long long)address_dst, size, size_type);
#endif
  }
  else if (ins_type == 1)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("_FPC_FP64_MEMCPY_INST_ (memmove): src=0x%016llx, dst=0x%016llx, size=%zu, size_type=%d\n",
           (unsigned long long)address_src, (unsigned long long)address_dst, size, size_type);
#endif
  }

  _FPC_ADDRESS_RANGE_UPDATE_FP64(
      _FPC_ADDRESS_HT_FP64_,
      address_dst,
      address_src,
      size,
      file_name,
      loc);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_FP64_(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);
#endif
}

// Push argument error from caller's register table into the argument buffer.
// Called once per FP argument, in order, before each user function call.
void _FPC_FP64_PUSH_ARG_ERROR_(int arg_index, const char *arg_reg, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double error = 0.0;
  long double relative_error = 0.0;
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, arg_reg, function_name, &error, &relative_error);

  if (arg_index >= 0 && arg_index < _FPC_ARG_BUF_MAX_)
  {
    _FPC_ARG_ERR_BUF_FP64_[arg_index] = error;
    _FPC_ARG_REL_ERR_BUF_FP64_[arg_index] = relative_error;
    if (arg_index >= _FPC_ARG_BUF_COUNT_FP64_)
      _FPC_ARG_BUF_COUNT_FP64_ = arg_index + 1;
  }
}

// Pop argument error from the buffer into the callee's parameter register.
// Called once per FP parameter, in order, at callee function entry.
void _FPC_FP64_POP_ARG_ERROR_(int param_index, const char *param_reg, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double error = 0.0;
  long double relative_error = 0.0;

  if (param_index >= 0 && param_index < _FPC_ARG_BUF_COUNT_FP64_)
  {
    error = _FPC_ARG_ERR_BUF_FP64_[param_index];
    relative_error = _FPC_ARG_REL_ERR_BUF_FP64_[param_index];
  }

  _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, param_reg, function_name,
                           error, relative_error, "", 0);
}

// Save error of the value being returned by a function.
void _FPC_FP64_PUSH_RET_ERROR_(const char *ret_reg, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double error = 0.0;
  long double relative_error = 0.0;
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, ret_reg, function_name, &error, &relative_error);

  if (_FPC_RET_STACK_TOP_FP64_ < _FPC_RET_STACK_MAX_)
  {
    _FPC_RET_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_] = error;
    _FPC_RET_REL_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_] = relative_error;
    strncpy(_FPC_RET_FUNC_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_], function_name, _FPC_BB_NAME_SIZE_ - 1);
    _FPC_RET_FUNC_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_][_FPC_BB_NAME_SIZE_ - 1] = '\0';
    _FPC_RET_STACK_TOP_FP64_++;
  }
}

// Load most recent returned value error into call result register in caller.
void _FPC_FP64_POP_RET_ERROR_(const char *result_reg, const char *function_name,
                              const char *callee_name, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double error = 0.0L;
  long double relative_error = 0.0L;

  if (_FPC_RET_STACK_TOP_FP64_ > 0)
  {
    int top = _FPC_RET_STACK_TOP_FP64_ - 1;
    int names_match = 1;
    if (callee_name != NULL && callee_name[0] != '\0')
    {
      names_match = (strcmp(_FPC_RET_FUNC_STACK_FP64_[top], callee_name) == 0);
    }

    if (names_match)
    {
      _FPC_RET_STACK_TOP_FP64_--;
      error = _FPC_RET_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_];
      relative_error = _FPC_RET_REL_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_];
    }
  }

  _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, result_reg, function_name,
                           error, relative_error, file_name, loc);
  FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, relative_error);
}

/*----------------------------------------------------------------------------*/
/* FP64 Error Calculation (Double Precision Error Tracking)                   */
/*----------------------------------------------------------------------------*/
// *** FP64 Error Calculation *** //
// Calculates the error for a given double precision operation
// Re-computes the operation in long double (128-bit), accounting for propagated
// operand errors, and records the rounding error of the fp64 result.

void _FPC_FP64_CALCULATE_ERROR_(
    double x, double y, double z, double w, int loc, char *file_name, int op, int cond,
    const char *result_name, const char *op1_name, const char *op2_name, const char *fma_name, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP64_CALCULATE_ERROR_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP64_CALCULATE_ERROR_\n");
  printf("op=%d, x=%.17e, y=%.17e, z=%.17e, w=%.17e, result_name=%s, op1_name=%s, op2_name=%s, fma_name=%s, cond=%d\n", op, x, y, z, w, result_name, op1_name, op2_name, fma_name, cond);
  printf("Line: %d, File Name: %s\n", loc, file_name);
#endif

  long double err_y = 0.0L;
  long double err_z = 0.0L;
  long double err_w = 0.0L;
  long double _tmp_unused_ = 0.0L;

  #ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name, function_name, &err_y, &_tmp_unused_);
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op2_name, function_name, &err_z, &_tmp_unused_);
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, fma_name, function_name, &err_w, &_tmp_unused_);
  #endif

  long double y_high = (long double)y + err_y;
  long double z_high = (long double)z + err_z;
  long double w_high = (long double)w + err_w;

  long double r_high = 0.0;
  switch (op)
  {
  case 0:
    r_high = y_high + z_high;
    break;
  case 1:
    r_high = y_high - z_high;
    break;
  case 2:
    r_high = y_high * z_high;
    break;
  case 3:
    if (z_high != 0.0)
    {
      r_high = y_high / z_high;
    }
    else
    {
      printf("#FPCHECKER_ERROR: Division by zero\n");
      r_high = 0.0;
    }
    break;
  case 5:
    r_high = fmodl(y_high, z_high);
    break;
  case 6:
    r_high = fmal(y_high, z_high, w_high);
    break;
  case 7:
    r_high = -y_high; // Negation operation
    break;
  case 8: // Select instruction
    if (cond == 1)
      r_high = z_high;
    else
      r_high = w_high;
    break;
  default:
    printf("#FPCHECKER_ERROR: Unknown operation %d\n", op);
  }

  long double r_low = (long double)x;

  long double err_result = r_high - r_low;
  

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("Result (double):         %.17e\n", x);
  printf("Result (double-> long double): %.17Le\n", r_low);
  printf("Result (double):        %.17e\n", r_high);
  printf("Error Result:           %.17Le\n", err_result);
#endif

  // Calculate relative error
  long double rel_error = 0.0;
  long double largest_subnormal_d = nextafterl(LDBL_MIN, 0.0L);
  if (err_result == 0.0)
  {
    rel_error = 0.0;
  }
  else
  {
    // Only compute relative error if r_high is not zero and
    // is larger than the largest subnormal
    if (fabs(r_high) > largest_subnormal_d)
    {
      rel_error = fabs(err_result) / fabs(r_high);
    }
    else
    {
      rel_error = INFINITY;
    }
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("\t >>> Relative Error: %.7e <<< \n", rel_error);
#endif

  // Update register error table
  _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, result_name, function_name, err_result, rel_error, file_name, loc);

  // Log location info if line is in _FPC_LINES_TO_KEEP_
  FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, rel_error);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_FP64_(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);
#endif

  // fflush(stdout);

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP32_CALCULATE_ERROR_..........\n");
#endif
}

/*----------------------------------------------------------------------------*/
/* Math Function Error Analysis.                                              */
/*----------------------------------------------------------------------------*/

// Re-computes a math function call in fp64, accounting for propagated
// operand errors, and records the rounding error of the fp32 result.
// Up to 3 FP operands are supported (arg1=y, arg2=z, arg3=w).
void _FPC_FP64_MATH_ERROR_(
    double x, double y, double z, double w,
    int loc, char *file_name,
    const char *math_func_name,
    const char *result_name, const char *op1_name, const char *op2_name,
    const char *op3_name, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP64_MATH_ERROR_\n");
  printf("func=%s, x=%.17e, y=%.17e, z=%.17e, w=%.17e, result=%s, op1=%s, op2=%s, op3=%s\n",
         math_func_name, x, y, z, w, result_name, op1_name, op2_name, op3_name);
  printf("Line: %d, File Name: %s\n", loc, file_name);
#endif

  long double err_y = 0.0L;
  long double err_z = 0.0L;
  long double err_w = 0.0L;
  long double _tmp_unused_ = 0.0L;

#ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name, function_name, &err_y, &_tmp_unused_);
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op2_name, function_name, &err_z, &_tmp_unused_);
  _FPC_FIND_ERRORS_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op3_name, function_name, &err_w, &_tmp_unused_);
#endif

  long double y_high = (long double)y + err_y;
  long double z_high = (long double)z + err_z;
  long double w_high = (long double)w + err_w;

  long double r_high = 0.0;

  // Unary functions
  if      (strcmp(math_func_name, "sin") == 0)       r_high = sinl(y_high);
  else if (strcmp(math_func_name, "cos") == 0)       r_high = cosl(y_high);
  else if (strcmp(math_func_name, "tan") == 0)       r_high = tanl(y_high);
  else if (strcmp(math_func_name, "asin") == 0)      r_high = asinl(y_high);
  else if (strcmp(math_func_name, "acos") == 0)      r_high = acosl(y_high);
  else if (strcmp(math_func_name, "atan") == 0)      r_high = atanl(y_high);
  else if (strcmp(math_func_name, "sinh") == 0)      r_high = sinhl(y_high);
  else if (strcmp(math_func_name, "cosh") == 0)      r_high = coshl(y_high);
  else if (strcmp(math_func_name, "tanh") == 0)      r_high = tanhl(y_high);
  else if (strcmp(math_func_name, "asinh") == 0)     r_high = asinhl(y_high);
  else if (strcmp(math_func_name, "acosh") == 0)     r_high = acoshl(y_high);
  else if (strcmp(math_func_name, "atanh") == 0)     r_high = atanhl(y_high);
  else if (strcmp(math_func_name, "exp") == 0)       r_high = expl(y_high);
  else if (strcmp(math_func_name, "exp2") == 0)      r_high = exp2l(y_high);
  else if (strcmp(math_func_name, "expm1") == 0)     r_high = expm1l(y_high);
  else if (strcmp(math_func_name, "log") == 0)       r_high = logl(y_high);
  else if (strcmp(math_func_name, "log2") == 0)      r_high = log2l(y_high);
  else if (strcmp(math_func_name, "log10") == 0)     r_high = log10l(y_high);
  else if (strcmp(math_func_name, "log1p") == 0)     r_high = log1pl(y_high);
  else if (strcmp(math_func_name, "logb") == 0)      r_high = logbl(y_high);
  else if (strcmp(math_func_name, "sqrt") == 0)      r_high = sqrtl(y_high);
  else if (strcmp(math_func_name, "cbrt") == 0)      r_high = cbrtl(y_high);
  else if (strcmp(math_func_name, "fabs") == 0)      r_high = fabsl(y_high);
  else if (strcmp(math_func_name, "ceil") == 0)      r_high = ceill(y_high);
  else if (strcmp(math_func_name, "floor") == 0)     r_high = floorl(y_high);
  else if (strcmp(math_func_name, "trunc") == 0)     r_high = truncl(y_high);
  else if (strcmp(math_func_name, "round") == 0)     r_high = roundl(y_high);
  else if (strcmp(math_func_name, "nearbyint") == 0) r_high = nearbyintl(y_high);
  else if (strcmp(math_func_name, "rint") == 0)      r_high = rintl(y_high);
  // Binary functions
  else if (strcmp(math_func_name, "pow") == 0)       r_high = powl(y_high, z_high);
  else if (strcmp(math_func_name, "atan2") == 0)     r_high = atan2l(y_high, z_high);
  else if (strcmp(math_func_name, "hypot") == 0)     r_high = hypotl(y_high, z_high);
  else if (strcmp(math_func_name, "fmod") == 0)      r_high = fmodl(y_high, z_high);
  else if (strcmp(math_func_name, "remainder") == 0) r_high = remainderl(y_high, z_high);
  // Ternary functions
  else if (strcmp(math_func_name, "fma") == 0)       r_high = fma(y_high, z_high, w_high);
  else
  {
    printf("#FPCHECKER_WARNING: Unknown math function '%s'\n", math_func_name);
    r_high = (long double)x; // Assume no error
  }

  long double r_low = (long double)x;
  long double err_result = r_high - r_low;

  // Calculate relative error
  long double rel_error = 0.0L;
  long double largest_subnormal_d = nextafterl(LDBL_MIN, 0.0L);
  if (err_result == 0.0L)
  {
    rel_error = 0.0L;
  }
  else
  {
    if (fabsl(r_high) > largest_subnormal_d)
    {
      rel_error = fabsl(err_result) / fabsl(r_high);
    }
    else
    {
      rel_error = INFINITY;
    }
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("Math Result (double):         %.17e\n", x);
  printf("Math Result (double-> long double): %.17Le\n", r_low);
  printf("Math Result (double):        %.17e\n", r_high);
  printf("Math Error Result:           %.17L\n", err_result);
  printf("\t >>> Math Relative Error: %.17Le <<< \n", rel_error);
#endif

  _FPC_REGISTER_HT_FP64_UPDATE_(_FPC_REGISTER_HT_FP64_, result_name, function_name, err_result, rel_error, file_name, loc);
  FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, rel_error);
}

/*----------------------------------------------------------------------------*/
/* Annotation Macros                                                          */
/*----------------------------------------------------------------------------*/
#include "FPC_Annotations.h"

#endif /* SRC_RUNTIME_ERROR_FP64_H_ */