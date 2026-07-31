#ifndef SRC_RUNTIME_ERROR_H_
#define SRC_RUNTIME_ERROR_H_

#include "FPC_Hashtable_Error.h"
#include "FPC_FloatSeries_List.h"
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <float.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>

#if defined(FPC_DEBUG_ERROR_ANALYSIS) || defined(FPDC_DEBUG_CALLSTACK)
static inline int _FPC_DEBUG_OUTPUT_ENABLED_(void)
{
  static int initialized = 0;
  static int enabled = 0;
  if (!initialized)
  {
    enabled = (getenv("FPC_ENABLE_DEBUG_OUTPUT") != NULL);
    initialized = 1;
  }
  return enabled;
}

static inline int _FPC_RUNTIME_DEBUG_PRINTF_(const char *format, ...)
{
  if (!_FPC_DEBUG_OUTPUT_ENABLED_())
    return 0;

  va_list args;
  va_start(args, format);
  int written = vfprintf(stderr, format, args);
  va_end(args);
  return written;
}

#define _FPC_DEBUG_STDERR_REDIRECT_
#define printf(...) _FPC_RUNTIME_DEBUG_PRINTF_(__VA_ARGS__)
#endif

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
_FPC_ADDRESS_HTABLE_T *_FPC_ADDRESS_HT_;
_FPC_REGISTER_HTABLE_T *_FPC_REGISTER_HT_;

// Lines of code to save values from
// Env variable: FPC_SAVE_LINE_ERRORS=3,4
int *_FPC_LINES_TO_KEEP_;
FPC_SeriesManager *FPC_DATA_MANAGER;

// Maximum number of warnings to print
#define MAX_WARNINGS 3
int _FPC_WARNING_COUNT_;

double _FPC_STABILITY_ETA_ABS_ = 0.0;
double _FPC_STABILITY_ETA_REL_ = 1.0e-6;
#define _FPC_STABILITY_TAU_ 1.0e-30
int  _FPC_STABILITY_WARNING_COUNT_ = 0;
long _FPC_STABILITY_MAX_WARNINGS_  = MAX_WARNINGS;
int  _FPC_NONFINITE_WARNING_COUNT_ = 0;
#define _FPC_NONFINITE_MAX_WARNINGS 10

static int    _FPC_PERTURB_ENABLE_    = 0;
static double _FPC_PERTURB_DELTA_ABS_ = 0.0;
static double _FPC_PERTURB_DELTA_REL_ = 1.0e-7;
static int    _FPC_PERTURB_SIGN_MODE_ = 0;
static unsigned long _FPC_PERTURB_SIGN_TICK_ = 0;

static double _FPC_PERTURB_NEXT_SIGN_(void)
{
  switch (_FPC_PERTURB_SIGN_MODE_)
  {
  case 1: return (_FPC_PERTURB_SIGN_TICK_++ % 2 == 0) ? 1.0 : -1.0;
  case 2: return (rand() & 1) ? 1.0 : -1.0;
  case 0:
  default: return 1.0;
  }
}

// Last basic block name
#define _FPC_BB_NAME_SIZE_ 512                   // max size of a basic block name - for example: "%bb_26"
char _FPC_LAST_BASIC_BLOCK_[_FPC_BB_NAME_SIZE_]; // Last basic block ID

// Call/return floating-point error propagation stack
#define _FPC_RET_STACK_MAX_ 8192
double _FPC_RET_SHADOW_STACK_[_FPC_RET_STACK_MAX_];
double _FPC_RET_ERR_STACK_[_FPC_RET_STACK_MAX_];
double _FPC_RET_REL_ERR_STACK_[_FPC_RET_STACK_MAX_];
char _FPC_RET_FUNC_STACK_[_FPC_RET_STACK_MAX_][_FPC_BB_NAME_SIZE_];
int _FPC_RET_STACK_TOP_;

// Caller-to-callee argument error propagation buffer
#define _FPC_ARG_BUF_MAX_ 256
double _FPC_ARG_SHADOW_BUF_[_FPC_ARG_BUF_MAX_];
double _FPC_ARG_ERR_BUF_[_FPC_ARG_BUF_MAX_];
double _FPC_ARG_REL_ERR_BUF_[_FPC_ARG_BUF_MAX_];
int _FPC_ARG_BUF_COUNT_;

// Forward declaration for lazy initialization helper.
void _FPC_INIT_FPCHECKER();
void _FPC_PRINT_LOCATIONS_();

static inline void _FPC_ENSURE_RUNTIME_READY_()
{
  static int fpc_atexit_registered = 0;

  if (_FPC_ADDRESS_HT_ == NULL || _FPC_REGISTER_HT_ == NULL)
  {
    _FPC_INIT_FPCHECKER();

    if (!fpc_atexit_registered)
    {
      atexit(_FPC_PRINT_LOCATIONS_);
      fpc_atexit_registered = 1;
    }
  }
}

static inline double _FPC_READ_FP32_VALUE_FROM_ADDRESS_(uintptr_t address)
{
  if (address < 4096)
    return 0.0;

  float value = 0.0f;
  memcpy(&value, (const void *)address, sizeof(value));
  return (double)value;
}

static inline int _FPC_TRY_PARSE_FP_LITERAL_(const char *text, double *value)
{
  if (text == NULL || text[0] == '\0')
    return 0;

  char *endptr = NULL;
  double parsed = strtod(text, &endptr);
  if (endptr == text)
    return 0;

  while (*endptr == ' ' || *endptr == '\t' || *endptr == '\n' ||
         *endptr == '\r' || *endptr == '\f' || *endptr == '\v')
  {
    ++endptr;
  }

  if (*endptr != '\0')
    return 0;

  *value = parsed;
  return 1;
}

enum _FPC_STABILITY_CLASS_
{
  _FPC_STABLE_TRUE  = 0,
  _FPC_STABLE_FALSE = 1,
  _FPC_UNSTABLE     = 2
};

static double _FPC_STABILITY_WIDTH_(double abs_err, double rel_err, double val)
{
  double a1 = fabs(abs_err);
  double scale = fabs(val);
  if (scale < _FPC_STABILITY_TAU_)
    scale = _FPC_STABILITY_TAU_;
  double a2 = rel_err * scale;
  return (a1 > a2) ? a1 : a2;
}

/* COMPRESSED predicate code: 0=eq 1=ne 2=lt 3=le 4=gt 5=ge */
static int _FPC_CLASSIFY_STABILITY_COMPRESSED_(int pred, double a, double b,
                                               double wa, double wb, double eta)
{
  double u = wa + wb + eta;
  double g = b - a;
  double diff = a - b;

  if (u == 0.0)
    return _FPC_STABLE_TRUE;

  switch (pred)
  {
  case 2:
  case 3:
    if (g > u)  return _FPC_STABLE_TRUE;
    if (g < -u) return _FPC_STABLE_FALSE;
    return _FPC_UNSTABLE;
  case 4:
  case 5:
    if (diff > u)  return _FPC_STABLE_TRUE;
    if (diff < -u) return _FPC_STABLE_FALSE;
    return _FPC_UNSTABLE;
  case 0:
    if (fabs(diff) > u) return _FPC_STABLE_FALSE;
    return _FPC_UNSTABLE;
  case 1:
    if (fabs(diff) > u) return _FPC_STABLE_TRUE;
    return _FPC_UNSTABLE;
  default:
    return _FPC_STABLE_TRUE;
  }
}

static const char *_FPC_PRED_NAME_COMPRESSED_(int pred)
{
  switch (pred)
  {
  case 0: return "==";
  case 1: return "!=";
  case 2: return "<";
  case 3: return "<=";
  case 4: return ">";
  case 5: return ">=";
  default: return "?";
  }
}

/*----------------------------------------------------------------------------*/
/* Initialize                                                                 */
/*----------------------------------------------------------------------------*/

void _FPC_INIT_STABILITY_CONFIG_()
{
  char *eta_abs = getenv("FPC_STABILITY_ETA_ABS");
  if (eta_abs != NULL) _FPC_STABILITY_ETA_ABS_ = atof(eta_abs);
  char *eta_rel = getenv("FPC_STABILITY_ETA_REL");
  if (eta_rel != NULL) _FPC_STABILITY_ETA_REL_ = atof(eta_rel);
  char *smw = getenv("FPC_STABILITY_MAX_WARNINGS");
  if (smw != NULL) _FPC_STABILITY_MAX_WARNINGS_ = atol(smw);

  char *p_en = getenv("FPC_PERTURB_ENABLE");
  if (p_en != NULL) _FPC_PERTURB_ENABLE_ = atoi(p_en);
  char *p_da = getenv("FPC_PERTURB_DELTA_ABS");
  if (p_da != NULL) _FPC_PERTURB_DELTA_ABS_ = atof(p_da);
  char *p_dr = getenv("FPC_PERTURB_DELTA_REL");
  if (p_dr != NULL) _FPC_PERTURB_DELTA_REL_ = atof(p_dr);
  char *p_sg = getenv("FPC_PERTURB_SIGN");
  if (p_sg != NULL)
  {
    if      (strcmp(p_sg, "alternate") == 0) _FPC_PERTURB_SIGN_MODE_ = 1;
    else if (strcmp(p_sg, "random")    == 0) _FPC_PERTURB_SIGN_MODE_ = 2;
    else                                     _FPC_PERTURB_SIGN_MODE_ = 0;
  }

#ifndef FPC_QUIET
  printf("#FPCHECKER: Branch-stability tolerances: eta_abs=%g, eta_rel=%g\n",
         _FPC_STABILITY_ETA_ABS_, _FPC_STABILITY_ETA_REL_);
#endif
}

void _FPC_INIT_HASH_TABLE_()
{
#ifndef FPC_QUIET
  printf("#FPCHECKER: Initializing...\n");
#endif

  int64_t size = 65536;
  _FPC_ADDRESS_HT_ = _FPC_ADDRESS_HT_CREATE_(size);
  _FPC_REGISTER_HT_ = _FPC_REGISTER_HT_CREATE_(size);

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

void _FPC_INIT_FPCHECKER()
{
  if (_FPC_ADDRESS_HT_ != NULL && _FPC_REGISTER_HT_ != NULL)
  {
    return;
  }

  _FPC_PROG_INPUTS = 0;
  _FPC_LAST_BASIC_BLOCK_[0] = '\0';
  _FPC_RET_STACK_TOP_ = 0;
  _FPC_INIT_HASH_TABLE_();
  _FPC_INIT_STABILITY_CONFIG_();
  _FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED();
}

void _FPC_INIT_ARGS_FPCHECKER(int argc, char **argv)
{
  if (_FPC_ADDRESS_HT_ != NULL && _FPC_REGISTER_HT_ != NULL)
  {
    _FPC_PROG_INPUTS = argc;
    _FPC_PROG_ARGS = argv;
    return;
  }

  _FPC_PROG_INPUTS = argc;
  _FPC_PROG_ARGS = argv;
  _FPC_LAST_BASIC_BLOCK_[0] = '\0';
  _FPC_RET_STACK_TOP_ = 0;
  _FPC_INIT_HASH_TABLE_();
  _FPC_INIT_STABILITY_CONFIG_();
  _FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED();
}

void _FPC_PRINT_LOCATIONS_()
{
  static int fpc_finalized = 0;

  if (fpc_finalized)
  {
    return;
  }

  fpc_finalized = 1;

  if (_FPC_ADDRESS_HT_ == NULL || _FPC_REGISTER_HT_ == NULL)
  {
    return;
  }

#ifndef FPC_QUIET
  printf("#FPCHECKER: Finalizing and writing traces...\n");
#endif

  _FPC_WRITE_AND_PRINT_TO_JSON_(_FPC_ADDRESS_HT_, _FPC_REGISTER_HT_);

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
void FPC_APPEND_ERROR_LOG_ENTRY(int line, double relative_error)
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
    FPC_append_value(FPC_DATA_MANAGER, line, relative_error);
  }
}

/*------------------------------------------------------------------*/
/* Store Function with Location Logging                             */
/*------------------------------------------------------------------*/

// *** Error Calculation *** //
// Instrumentation for STORE instructions
void _FPC_FP32_STORE_INST_(const char *reg, const char *function_name, uintptr_t address, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_STORE_INST_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_STORE_INST_:\n");
  printf("reg=%s, address=%lu\n", reg, address);
#endif

  double shadow_value = 0.0;
  double error = 0.0;
  double relative_error = 0.0;

  // Find if this register already has an error
  int found = _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, reg, function_name,
                                          &shadow_value, &error,
                                          &relative_error);
  if (!found)
  {
    if (_FPC_WARNING_COUNT_ < MAX_WARNINGS)
    {
      _FPC_WARNING_COUNT_++;
      printf("#FPCHECKER: Warning: trying to store a register's value (%s) in function %s, but we don't have its error.\n",
             reg, function_name);
    }

    shadow_value = _FPC_READ_FP32_VALUE_FROM_ADDRESS_(address);
    error = 0.0;
    relative_error = 0.0;
  }

  // Update table based on the address
  // If address exists, update it
  // If address does not exist, insert new entry
  _FPC_ADDRESS_HT_UPDATE_(_FPC_ADDRESS_HT_, address, shadow_value, error,
                          relative_error, file_name, loc);

  // Log location info if line is in _FPC_LINES_TO_KEEP_
  FPC_APPEND_ERROR_LOG_ENTRY(loc, relative_error);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_(_FPC_ADDRESS_HT_, _FPC_REGISTER_HT_);
#endif

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP32_STORE_INST_..........\n");
#endif
}

// Instrumentation for LOAD instructions
void _FPC_FP32_LOAD_INST_(const char *load_reg, const char *function_name, uintptr_t address, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_LOAD_INST_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_LOAD_INST_:\n");
  printf("reg=%s, address=0x%016llx, func=%s\n", load_reg, (unsigned long long)address, function_name);
#endif

  double shadow_value = 0.0;
  double error = 0.0;
  double relative_error = 0.0;

  // Find what's at this memory address
  int found = _FPC_FIND_VALUE_BY_ADDRESS(_FPC_ADDRESS_HT_, address,
                                         &shadow_value, &error,
                                         &relative_error);
  if (found)
  {
    double current_value = _FPC_READ_FP32_VALUE_FROM_ADDRESS_(address);
    double reconciliation_error = (shadow_value - current_value) - error;
    double reconciliation_scale = fmax(1.0,
                                       fmax(fabs(shadow_value),
                                            fmax(fabs(current_value),
                                                 fabs(error))));

    if (fabs(reconciliation_error) > (32.0 * FLT_EPSILON * reconciliation_scale))
    {
      // The address appears to have been reused or mutated outside the
      // instrumented shadow path. Fall back to the actual memory value.
      shadow_value = current_value;
      error = 0.0;
      relative_error = 0.0;
    }
    else if (shadow_value == 0.0 && error == 0.0 && relative_error == 0.0)
    {
      shadow_value = current_value;
    }

    // Update register entry with this error
    _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, load_reg, function_name,
                             shadow_value, error, relative_error, file_name,
                             loc);

    // Log location info if line is in _FPC_LINES_TO_KEEP_
    FPC_APPEND_ERROR_LOG_ENTRY(loc, relative_error);
  }
  else
  {
    // No error found at this address, create register with zero error
    shadow_value = _FPC_READ_FP32_VALUE_FROM_ADDRESS_(address);
    _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, load_reg, function_name,
                 shadow_value, 0.0, 0.0, file_name, loc);
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("LOAD: No data found at address %lu\n", address);
#endif
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_(_FPC_ADDRESS_HT_, _FPC_REGISTER_HT_);
#endif

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP32_STORE_INST_..........\n");
#endif
}

void _FPC_FP32_BRANCH_(const char *basic_block_name)
{
#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_BRANCH_..........\n");
#endif

  // printf("BRANCH: BasicBlock %s\n", basic_block_name);
  strncpy(_FPC_LAST_BASIC_BLOCK_, basic_block_name, _FPC_BB_NAME_SIZE_ - 1);
  _FPC_LAST_BASIC_BLOCK_[_FPC_BB_NAME_SIZE_ - 1] = '\0';

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP32_BRANCH_..........\n");
#endif
}

void _FPC_FP32_CMP_(int low_cond, float y, float z, int predicate, int loc,
                    char *file_name, const char *result_name,
                    const char *op1_name, const char *op2_name,
                    const char *function_name, int is_branch_controlling)
{
  _FPC_ENSURE_RUNTIME_READY_();
  
  double shadow_y = (double)y;
  double shadow_z = (double)z;
  double err_y = 0.0, rho_y = 0.0;
  double err_z = 0.0, rho_z = 0.0;

#ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op1_name, function_name,
                              &shadow_y, &err_y, &rho_y);
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op2_name, function_name,
                              &shadow_z, &err_z, &rho_z);
#endif

  int shadow_cond = low_cond;
  switch (predicate)
  {
  case 0: shadow_cond = (shadow_y == shadow_z); break;
  case 1: shadow_cond = (shadow_y != shadow_z); break;
  case 2: shadow_cond = (shadow_y <  shadow_z); break;
  case 3: shadow_cond = (shadow_y <= shadow_z); break;
  case 4: shadow_cond = (shadow_y >  shadow_z); break;
  case 5: shadow_cond = (shadow_y >= shadow_z); break;
  default: shadow_cond = low_cond; break;
  }

  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, result_name, function_name,
                           shadow_cond ? 1.0 : 0.0, 0.0, 0.0, file_name,
                           loc);

  /* ---- Phase 1: branch-instability classification ---- */
  if (is_branch_controlling)
  {
    double wa = _FPC_STABILITY_WIDTH_(err_y, rho_y, shadow_y);
    double wb = _FPC_STABILITY_WIDTH_(err_z, rho_z, shadow_z);

    double m = fabs(shadow_y) > fabs(shadow_z) ? fabs(shadow_y) : fabs(shadow_z);
    double eta = _FPC_STABILITY_ETA_ABS_;
    double eta_scaled = _FPC_STABILITY_ETA_REL_ * m;
    if (eta_scaled > eta)
      eta = eta_scaled;

    // Handle non-finite values: if any of wa, wb, eta, shadow_y, shadow_z are non-finite, we cannot classify the comparison
    if (!isfinite(err_y) || !isfinite(rho_y) ||
        !isfinite(err_z) || !isfinite(rho_z) ||
        !isfinite(wa) || !isfinite(wb) || !isfinite(eta) ||
        !isfinite(shadow_y) || !isfinite(shadow_z))
    {
      if (_FPC_NONFINITE_WARNING_COUNT_ < _FPC_NONFINITE_MAX_WARNINGS)
      {
        _FPC_NONFINITE_WARNING_COUNT_++;
        printf("#FPCHECKER: WARNING: non-finite shadow state at %s:%d in %s: "
               "(%g %s %g) wa=%g, wb=%g, eta=%g -- comparison NOT classified\n",
               file_name ? file_name : "Unknown", loc,
               function_name ? function_name : "Unknown",
               shadow_y, _FPC_PRED_NAME_COMPRESSED_(predicate), shadow_z,
               wa, wb, eta);
      }
      return;
    }

    int cls = _FPC_CLASSIFY_STABILITY_COMPRESSED_(predicate, shadow_y, shadow_z,
                                                  wa, wb, eta);

    if (cls == _FPC_UNSTABLE)
    {
      if (_FPC_STABILITY_WARNING_COUNT_ < _FPC_STABILITY_MAX_WARNINGS_)
      {
        _FPC_STABILITY_WARNING_COUNT_++;
        printf("#FPCHECKER: Unstable branch at %s:%d in %s: "
               "(%g %s %g) observed=%s, uncertainty u=%g (wa=%g, wb=%g, eta=%g)\n",
               file_name ? file_name : "Unknown", loc,
               function_name ? function_name : "Unknown",
               shadow_y, _FPC_PRED_NAME_COMPRESSED_(predicate), shadow_z,
               low_cond ? "true" : "false",
               wa + wb + eta, wa, wb, eta);
      }
      double mag = rho_y > rho_z ? rho_y : rho_z;
      FPC_APPEND_ERROR_LOG_ENTRY(loc, mag);
    }
  }
}

// This function is called for PHI nodes in SSA form
// It is used to log the values that are being merged
void _FPC_FP32_PHI_(const char *phi_values, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_PHI_..........\n");
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
            double old_shadow = 0.0;
            double old_error = 0.0;
            double old_relative_error = 0.0;
            int found = _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_,
                                                    first_substr,
                                                    function_name,
                                                    &old_shadow,
                                                    &old_error,
                                                    &old_relative_error);
            if (found)
            {
              _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, register_name,
                                       function_name, old_shadow, old_error,
                                       old_relative_error, "", 0);
            }
            else
            {
              double literal_shadow = 0.0;
              if (_FPC_TRY_PARSE_FP_LITERAL_(first_substr, &literal_shadow))
              {
                _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, register_name,
                                         function_name, literal_shadow, 0.0,
                                         0.0, "", 0);
              }
              else
              {
                // We don't have its error - create with zero error
                _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, register_name,
                                         function_name, 0.0, 0.0, 0.0, "", 0);
              }
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
void _FPC_FP32_MEMCPY_INST_(uintptr_t address_dst, uintptr_t address_src,
                            long int size, int size_type, int ins_type,
                            int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  if (ins_type == 0)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("_FPC_FP32_MEMCPY_INST_ (memcpy): src=0x%016llx, dst=0x%016llx, size=%zu, size_type=%d\n",
           (unsigned long long)address_src, (unsigned long long)address_dst, size, size_type);
#endif
  }
  else if (ins_type == 1)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("_FPC_FP32_MEMCPY_INST_ (memmove): src=0x%016llx, dst=0x%016llx, size=%zu, size_type=%d\n",
           (unsigned long long)address_src, (unsigned long long)address_dst, size, size_type);
#endif
  }

  _FPC_ADDRESS_RANGE_UPDATE_(
      _FPC_ADDRESS_HT_,
      address_dst,
      address_src,
      size,
      file_name,
      loc);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_(_FPC_ADDRESS_HT_, _FPC_REGISTER_HT_);
#endif
}

// Push argument error from caller's register table into the argument buffer.
// Called once per FP argument, in order, before each user function call.
void _FPC_FP32_PUSH_ARG_ERROR_(int arg_index, double arg_value,
                               const char *arg_reg,
                               const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  double shadow_value = arg_value;
  double error = 0.0;
  double relative_error = 0.0;
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, arg_reg, function_name,
                              &shadow_value, &error, &relative_error);

  if (arg_index >= 0 && arg_index < _FPC_ARG_BUF_MAX_)
  {
    _FPC_ARG_SHADOW_BUF_[arg_index] = shadow_value;
    _FPC_ARG_ERR_BUF_[arg_index] = error;
    _FPC_ARG_REL_ERR_BUF_[arg_index] = relative_error;
    if (arg_index >= _FPC_ARG_BUF_COUNT_)
      _FPC_ARG_BUF_COUNT_ = arg_index + 1;
  }
}

// Pop argument error from the buffer into the callee's parameter register.
// Called once per FP parameter, in order, at callee function entry.
void _FPC_FP32_POP_ARG_ERROR_(int param_index, const char *param_reg, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  double shadow_value = 0.0;
  double error = 0.0;
  double relative_error = 0.0;

  if (param_index >= 0 && param_index < _FPC_ARG_BUF_COUNT_)
  {
    shadow_value = _FPC_ARG_SHADOW_BUF_[param_index];
    error = _FPC_ARG_ERR_BUF_[param_index];
    relative_error = _FPC_ARG_REL_ERR_BUF_[param_index];
  }

  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, param_reg, function_name,
                           shadow_value, error, relative_error, "", 0);
}

static void __attribute__((unused)) _FPC_PERTURB_RECONCILE_(double val, double dx, double sign,
                                    double r_old, double rho_old,
                                    double *r_new_out, double *rho_new_out)
{
  double r_seed = sign * dx;
  double r_ref  = (r_old != 0.0) ? r_old : r_seed;
  double sgn    = (r_ref >= 0.0) ? 1.0 : -1.0;
  double amag   = fabs(r_old);
  double bmag   = fabs(r_seed);
  double mag    = (amag > bmag) ? amag : bmag;
  double r_new  = sgn * mag;

  double scale = fabs(val);
  if (scale < _FPC_STABILITY_TAU_)
    scale = _FPC_STABILITY_TAU_;
  double rho_new = fabs(r_new) / scale;
  if (rho_new < rho_old)
    rho_new = rho_old;

  *r_new_out   = r_new;
  *rho_new_out = rho_new;
}

void _FPC_FP32_PERTURB_SCALAR_(float x, const char *param_reg,
                               const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();
  if (!_FPC_PERTURB_ENABLE_)
    return;

  double dx = _FPC_PERTURB_DELTA_REL_ * fabs((double)x);
  if (_FPC_PERTURB_DELTA_ABS_ > dx)
    dx = _FPC_PERTURB_DELTA_ABS_;

  double sign = _FPC_PERTURB_NEXT_SIGN_();

  double shadow_old = (double)x, r_old = 0.0, rho_old = 0.0;
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, param_reg, function_name,
                              &shadow_old, &r_old, &rho_old);

  double r_new = 0.0, rho_new = 0.0;
  _FPC_PERTURB_RECONCILE_((double)x, dx, sign, r_old, rho_old, &r_new, &rho_new);

  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, param_reg, function_name,
                         (double)x + r_new, r_new, rho_new, "", 0);
}

void _FPC_FP32_PERTURB_POINTER_(const float *p, long int n,
                                const char *param_reg,
                                const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();
  if (!_FPC_PERTURB_ENABLE_)
    return;
  if (p == NULL || n <= 0)
    return;

  for (long int i = 0; i < n; ++i)
  {
    double xi = (double)p[i];

    double dx = _FPC_PERTURB_DELTA_REL_ * fabs(xi);
    if (_FPC_PERTURB_DELTA_ABS_ > dx)
      dx = _FPC_PERTURB_DELTA_ABS_;

    double sign = _FPC_PERTURB_NEXT_SIGN_();

    uintptr_t addr = (uintptr_t)(&p[i]);

    double shadow_old = xi, r_old = 0.0, rho_old = 0.0;
    _FPC_FIND_VALUE_BY_ADDRESS(_FPC_ADDRESS_HT_, addr, &shadow_old, &r_old, &rho_old);

    double r_new = 0.0, rho_new = 0.0;
    _FPC_PERTURB_RECONCILE_(xi, dx, sign, r_old, rho_old, &r_new, &rho_new);

    _FPC_ADDRESS_HT_UPDATE_(_FPC_ADDRESS_HT_, addr, xi + r_new, r_new, rho_new, "", 0);
  }
}

// Save error of the value being returned by a function.
void _FPC_FP32_PUSH_RET_ERROR_(const char *ret_reg, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  double shadow_value = 0.0;
  double error = 0.0;
  double relative_error = 0.0;
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, ret_reg, function_name,
                              &shadow_value, &error, &relative_error);

  if (_FPC_RET_STACK_TOP_ < _FPC_RET_STACK_MAX_)
  {
    _FPC_RET_SHADOW_STACK_[_FPC_RET_STACK_TOP_] = shadow_value;
    _FPC_RET_ERR_STACK_[_FPC_RET_STACK_TOP_] = error;
    _FPC_RET_REL_ERR_STACK_[_FPC_RET_STACK_TOP_] = relative_error;
    strncpy(_FPC_RET_FUNC_STACK_[_FPC_RET_STACK_TOP_], function_name, _FPC_BB_NAME_SIZE_ - 1);
    _FPC_RET_FUNC_STACK_[_FPC_RET_STACK_TOP_][_FPC_BB_NAME_SIZE_ - 1] = '\0';
    _FPC_RET_STACK_TOP_++;
  }
}

// Load most recent returned value error into call result register in caller.
void _FPC_FP32_POP_RET_ERROR_(const char *result_reg, const char *function_name,
                              const char *callee_name, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  double shadow_value = 0.0;
  double error = 0.0;
  double relative_error = 0.0;

  if (_FPC_RET_STACK_TOP_ > 0)
  {
    int top = _FPC_RET_STACK_TOP_ - 1;
    int names_match = 1;
    if (callee_name != NULL && callee_name[0] != '\0')
    {
      names_match = (strcmp(_FPC_RET_FUNC_STACK_[top], callee_name) == 0);
    }

    if (names_match)
    {
      _FPC_RET_STACK_TOP_--;
      shadow_value = _FPC_RET_SHADOW_STACK_[_FPC_RET_STACK_TOP_];
      error = _FPC_RET_ERR_STACK_[_FPC_RET_STACK_TOP_];
      relative_error = _FPC_RET_REL_ERR_STACK_[_FPC_RET_STACK_TOP_];
    }
  }

  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, result_reg, function_name,
                           shadow_value, error, relative_error, file_name, loc);
  FPC_APPEND_ERROR_LOG_ENTRY(loc, relative_error);
}

/*----------------------------------------------------------------------------*/
/* Error Analysis.                                                            */
/*----------------------------------------------------------------------------*/

// *** Error Calculation *** //
// Calculates the error for a given operation
void _FPC_FP32_CALCULATE_ERROR_(
    float x, float y, float z, float w, int loc, char *file_name, int op, int cond,
    const char *result_name, const char *op1_name, const char *op2_name, const char *fma_name, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_CALCULATE_ERROR_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_CALCULATE_ERROR_\n");
  printf("op=%d, x=%.7e, y=%.7e, z=%.7e, w=%.7e, result_name=%s, op1_name=%s, op2_name=%s, fma_name=%s, cond=%d\n", op, x, y, z, w, result_name, op1_name, op2_name, fma_name, cond);
  printf("Line: %d, File Name: %s\n", loc, file_name);
#endif

  double shadow_y = (double)y;
  double shadow_z = (double)z;
  double shadow_w = (double)w;
  double _tmp_error_ = 0.0;
  double _tmp_relative_error_ = 0.0;

#ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  if (op == 6)
  {
    _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op1_name, function_name,
                                &shadow_y, &_tmp_error_,
                                &_tmp_relative_error_);
    _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op2_name, function_name,
                                &shadow_z, &_tmp_error_,
                                &_tmp_relative_error_);
    _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, fma_name, function_name,
                                &shadow_w, &_tmp_error_,
                                &_tmp_relative_error_);
  }
  else if (op == 7)
  {
    _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op1_name, function_name,
                                &shadow_y, &_tmp_error_,
                                &_tmp_relative_error_);
  }
  else
  {
    _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op1_name, function_name,
                                &shadow_y, &_tmp_error_,
                                &_tmp_relative_error_);
    _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op2_name, function_name,
                                &shadow_z, &_tmp_error_,
                                &_tmp_relative_error_);
    if (op == 8)
    {
      _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, fma_name, function_name,
                                  &shadow_w, &_tmp_error_,
                                  &_tmp_relative_error_);
    }
  }
#endif

  double r_high = 0.0;
  switch (op)
  {
  case 0:
    r_high = shadow_y + shadow_z;
    break;
  case 1:
    r_high = shadow_y - shadow_z;
    break;
  case 2:
    r_high = shadow_y * shadow_z;
    break;
  case 3:
    if (shadow_z != 0.0)
    {
      r_high = shadow_y / shadow_z;
    }
    else
    {
      if ((double)z == 0.0)
      {
        printf("#FPCHECKER_ERROR: Division by zero at %s:%d (low-precision denominator is zero)\n",
               file_name, loc);
      }
      else
      {
        printf("#FPCHECKER_ERROR: Shadow denominator canceled to zero at %s:%d (low-precision denominator=%.17e, shadow denominator=%.17e)\n",
               file_name, loc, (double)z, shadow_z);
      }
      r_high = 0.0;
    }
    break;
  case 5:
    r_high = fmod(shadow_y, shadow_z);
    break;
  case 6:
    r_high = fma(shadow_y, shadow_z, shadow_w);
    break;
  case 7:
    r_high = -shadow_y; // Negation operation
    break;
  case 8: // Select instruction
  {
    double shadow_cond = (double)cond;
    _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op1_name, function_name,
                                &shadow_cond, &_tmp_error_,
                                &_tmp_relative_error_);
    if (cond == 1)
      r_high = (shadow_cond != 0.0) ? shadow_z : shadow_w;
    else
      r_high = (shadow_cond != 0.0) ? shadow_z : shadow_w;
    break;
  }
  default:
    printf("#FPCHECKER_ERROR: Unknown operation %d\n", op);
  }

  double r_low = (double)x;

  double err_result = r_high - r_low;

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("Result (float):         %.7e\n", x);
  printf("Result (float->double): %.17e\n", r_low);
  printf("Result (double):        %.17e\n", r_high);
  printf("Error Result:           %.17e\n", err_result);
#endif

  // Calculate relative error
  double rel_error = 0.0;
  double largest_subnormal_d = nextafter(DBL_MIN, 0.0);
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
  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, result_name, function_name,
                           r_high, err_result, rel_error, file_name, loc);

  // Log location info if line is in _FPC_LINES_TO_KEEP_
  FPC_APPEND_ERROR_LOG_ENTRY(loc, rel_error);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_(_FPC_ADDRESS_HT_, _FPC_REGISTER_HT_);
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
void _FPC_FP32_MATH_ERROR_(
    float x, float y, float z, float w,
    int loc, char *file_name,
    const char *math_func_name,
    const char *result_name, const char *op1_name, const char *op2_name,
    const char *op3_name, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_MATH_ERROR_\n");
  printf("func=%s, x=%.7e, y=%.7e, z=%.7e, w=%.7e, result=%s, op1=%s, op2=%s, op3=%s\n",
         math_func_name, x, y, z, w, result_name, op1_name, op2_name, op3_name);
  printf("Line: %d, File Name: %s\n", loc, file_name);
#endif

  double shadow_y = (double)y;
  double shadow_z = (double)z;
  double shadow_w = (double)w;
  double _tmp_error_ = 0.0;
  double _tmp_relative_error_ = 0.0;

#ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op1_name, function_name,
                              &shadow_y, &_tmp_error_,
                              &_tmp_relative_error_);
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op2_name, function_name,
                              &shadow_z, &_tmp_error_,
                              &_tmp_relative_error_);
  _FPC_FIND_VALUE_BY_REGISTER(_FPC_REGISTER_HT_, op3_name, function_name,
                              &shadow_w, &_tmp_error_,
                              &_tmp_relative_error_);
#endif

  double r_high = 0.0;

  // Unary functions
  if      (strcmp(math_func_name, "sin") == 0)       r_high = sin(shadow_y);
  else if (strcmp(math_func_name, "cos") == 0)       r_high = cos(shadow_y);
  else if (strcmp(math_func_name, "tan") == 0)       r_high = tan(shadow_y);
  else if (strcmp(math_func_name, "asin") == 0)      r_high = asin(shadow_y);
  else if (strcmp(math_func_name, "acos") == 0)      r_high = acos(shadow_y);
  else if (strcmp(math_func_name, "atan") == 0)      r_high = atan(shadow_y);
  else if (strcmp(math_func_name, "sinh") == 0)      r_high = sinh(shadow_y);
  else if (strcmp(math_func_name, "cosh") == 0)      r_high = cosh(shadow_y);
  else if (strcmp(math_func_name, "tanh") == 0)      r_high = tanh(shadow_y);
  else if (strcmp(math_func_name, "asinh") == 0)     r_high = asinh(shadow_y);
  else if (strcmp(math_func_name, "acosh") == 0)     r_high = acosh(shadow_y);
  else if (strcmp(math_func_name, "atanh") == 0)     r_high = atanh(shadow_y);
  else if (strcmp(math_func_name, "exp") == 0)       r_high = exp(shadow_y);
  else if (strcmp(math_func_name, "exp2") == 0)      r_high = exp2(shadow_y);
  else if (strcmp(math_func_name, "expm1") == 0)     r_high = expm1(shadow_y);
  else if (strcmp(math_func_name, "log") == 0)       r_high = log(shadow_y);
  else if (strcmp(math_func_name, "log2") == 0)      r_high = log2(shadow_y);
  else if (strcmp(math_func_name, "log10") == 0)     r_high = log10(shadow_y);
  else if (strcmp(math_func_name, "log1p") == 0)     r_high = log1p(shadow_y);
  else if (strcmp(math_func_name, "logb") == 0)      r_high = logb(shadow_y);
  else if (strcmp(math_func_name, "sqrt") == 0)      r_high = sqrt(shadow_y);
  else if (strcmp(math_func_name, "cbrt") == 0)      r_high = cbrt(shadow_y);
  else if (strcmp(math_func_name, "fabs") == 0)      r_high = fabs(shadow_y);
  else if (strcmp(math_func_name, "ceil") == 0)      r_high = ceil(shadow_y);
  else if (strcmp(math_func_name, "floor") == 0)     r_high = floor(shadow_y);
  else if (strcmp(math_func_name, "trunc") == 0)     r_high = trunc(shadow_y);
  else if (strcmp(math_func_name, "round") == 0)     r_high = round(shadow_y);
  else if (strcmp(math_func_name, "nearbyint") == 0) r_high = nearbyint(shadow_y);
  else if (strcmp(math_func_name, "rint") == 0)      r_high = rint(shadow_y);
  // Binary functions
  else if (strcmp(math_func_name, "pow") == 0)       r_high = pow(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "atan2") == 0)     r_high = atan2(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "hypot") == 0)     r_high = hypot(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "fmod") == 0)      r_high = fmod(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "remainder") == 0) r_high = remainder(shadow_y, shadow_z);
  // Ternary functions
  else if (strcmp(math_func_name, "fma") == 0)       r_high = fma(shadow_y, shadow_z, shadow_w);
  else
  {
    printf("#FPCHECKER_WARNING: Unknown math function '%s'\n", math_func_name);
    r_high = (double)x; // Assume no error
  }

  double r_low = (double)x;
  double err_result = r_high - r_low;

  // Calculate relative error
  double rel_error = 0.0;
  double largest_subnormal_d = nextafter(DBL_MIN, 0.0);
  if (err_result == 0.0)
  {
    rel_error = 0.0;
  }
  else
  {
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
  printf("Math Result (float):         %.7e\n", x);
  printf("Math Result (float->double): %.17e\n", r_low);
  printf("Math Result (double):        %.17e\n", r_high);
  printf("Math Error Result:           %.17e\n", err_result);
  printf("\t >>> Math Relative Error: %.7e <<< \n", rel_error);
#endif

  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, result_name, function_name,
                           r_high, err_result, rel_error, file_name, loc);
  FPC_APPEND_ERROR_LOG_ENTRY(loc, rel_error);
}

/*----------------------------------------------------------------------------*/
/* Annotation Macros                                                          */
/*----------------------------------------------------------------------------*/
#include "FPC_Annotations.h"

#ifdef _FPC_DEBUG_STDERR_REDIRECT_
#undef printf
#undef _FPC_DEBUG_STDERR_REDIRECT_
#endif

#endif /* SRC_RUNTIME_ERROR_H_ */
