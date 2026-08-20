#ifndef SRC_RUNTIME_ERROR_FP64_H_
#define SRC_RUNTIME_ERROR_FP64_H_

#include "FPC_Hashtable_Error_FP64.h"
#include "FPC_FloatSeries_List.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <float.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>

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
_FPC_ADDRESS_HTABLE_FP64_T *_FPC_ADDRESS_HT_FP64_;
_FPC_REGISTER_HTABLE_FP64_T *_FPC_REGISTER_HT_FP64_;

// Lines of code to save values from
// Env variable: FPC_SAVE_LINE_ERRORS=3,4
int *_FPC_LINES_TO_KEEP_;
FPC_SeriesManager *FPC_DATA_MANAGER;

// Maximum number of warnings to print
#define MAX_WARNINGS 3
int _FPC_WARNING_COUNT_;
/* The "we don't have its error" store warnings are capped at 3 with no
 * override. Left at 3 by default on purpose -- at -O0 these fire on every
 * spill slot and uncapping them produces gigabytes -- but FPC_MAX_WARNINGS
 * can raise it when you actually need to see them. */
long _FPC_MAX_WARNINGS_ = MAX_WARNINGS;

double _FPC_STABILITY_ETA_ABS_ = 0.0;
double _FPC_STABILITY_ETA_REL_ = 1.0e-14;   /* FP64 default, not 1e-6 */
#define _FPC_STABILITY_TAU_ 1.0e-300        /* FP64 scale, not 1e-30 */
int  _FPC_STABILITY_WARNING_COUNT_ = 0;
long _FPC_STABILITY_MAX_WARNINGS_  = MAX_WARNINGS;
int  _FPC_NONFINITE_WARNING_COUNT_ = 0;
/* Was a hard #define of 10 with no override, so the eleventh and every
 * subsequent non-finite comparison vanished silently and could not even be
 * counted from the log afterwards.
 *
 * Default is now UNLIMITED: every non-finite comparison is reported. These
 * are the overflow-driven comparisons the study is about, and a silent cap
 * at ten makes the per-site distribution unrecoverable -- you cannot tell one
 * site overflowing 400,000 times from 400,000 sites overflowing once.
 *
 * Set FPC_NONFINITE_MAX_WARNINGS=<n> to cap it again (10 restores the old
 * behaviour). On a benchmark that overflows heavily this is a lot of output,
 * so pair it with FPC_BF_LOG=<path> to keep it out of the program's stdout. */
long _FPC_NONFINITE_MAX_WARNINGS_ = 0;   /* 0 or negative = unlimited */

/* What to do when the shadow state is non-finite at a branch-controlling
 * comparison. This is a real methodological choice, not a printing detail:
 *
 *   0 = ABSTAIN  (default, preserves previous numbers). The comparison is
 *       neither classified stable nor unstable. Reported and counted, but the
 *       tool has declined to answer -- which is NOT the same as saying the
 *       branch is stable, and must not be scored as a correct rejection.
 *
 *   1 = UNSTABLE. A non-finite uncertainty window swallows everything, so the
 *       comparison cannot be resolved, which is exactly what UNSTABLE means.
 *       Falls through to the normal warning path and is scored as a detection.
 *
 * The delta between the two settings is the cost of FPChecker's abstention on
 * overflow-driven flips. Run both. */
int _FPC_NONFINITE_POLICY_ = 0;

/* Cause bits for a non-finite comparison. The old code ORed five distinct
 * conditions into one message, so a shadow trajectory that genuinely overflowed
 * could not be told apart from error tracking blowing up on a healthy value. */
#define _FPC_NF_NAN_VAL   0x01  /* shadow operand is NaN                  */
#define _FPC_NF_INF_VAL   0x02  /* shadow operand is Inf                  */
#define _FPC_NF_NAN_ERR   0x04  /* tracked abs/rel error is NaN           */
#define _FPC_NF_INF_ERR   0x08  /* tracked abs/rel error is Inf           */
#define _FPC_NF_INF_WIDTH 0x10  /* derived interval half-width non-finite */
#define _FPC_NF_INF_ETA   0x20  /* eta_rel * magnitude overflowed         */
/* The LOW-PRECISION operand -- the value the program actually computed.
 *
 * This is the bit that matters and it was missing. The shadow runs at HIGHER
 * precision than the program, so when the program overflows the shadow usually
 * does not: 1e38f*1e38f is inf in fp32 but 1e76 in the double shadow. The
 * non-finite state then shows up only indirectly, as err = shadow - inf = -inf,
 * i.e. as INF_ERR -- which reads like an error-model artifact but is in fact a
 * real program overflow. Checking y/z directly says so outright.
 *
 * Read the tags together:
 *   LOW_INF alone           -> the PROGRAM overflowed; the shadow is fine.
 *                              This is the overflow-driven case of interest.
 *   INF_VAL / NAN_VAL       -> the SHADOW trajectory itself went bad.
 *   INF_ERR without LOW_*   -> error tracking blew up on healthy values;
 *                              a tool artifact.                             */
#define _FPC_NF_LOW_NAN   0x40  /* program (low-precision) operand is NaN */
#define _FPC_NF_LOW_INF   0x80  /* program (low-precision) operand is Inf */

static void _FPC_NF_CAUSE_STR_(int why, char *buf, size_t n)
{
  buf[0] = '\0';
  if (why & _FPC_NF_NAN_VAL)   strncat(buf, "NAN_VAL|",   n - strlen(buf) - 1);
  if (why & _FPC_NF_INF_VAL)   strncat(buf, "INF_VAL|",   n - strlen(buf) - 1);
  if (why & _FPC_NF_NAN_ERR)   strncat(buf, "NAN_ERR|",   n - strlen(buf) - 1);
  if (why & _FPC_NF_INF_ERR)   strncat(buf, "INF_ERR|",   n - strlen(buf) - 1);
  if (why & _FPC_NF_INF_WIDTH) strncat(buf, "INF_WIDTH|", n - strlen(buf) - 1);
  if (why & _FPC_NF_INF_ETA)   strncat(buf, "INF_ETA|",   n - strlen(buf) - 1);
  if (why & _FPC_NF_LOW_NAN)   strncat(buf, "LOW_NAN|",   n - strlen(buf) - 1);
  if (why & _FPC_NF_LOW_INF)   strncat(buf, "LOW_INF|",   n - strlen(buf) - 1);
  size_t l = strlen(buf);
  if (l && buf[l - 1] == '|') buf[l - 1] = '\0';
  if (!l) strncat(buf, "UNKNOWN", n - 1);
}

/* Branch-flip diagnostics can be routed to their own file with FPC_BF_LOG.
 * Uncapped output on a real benchmark is large and interleaves with the
 * program's own stdout, which makes both unreadable.
 *
 * NOT static, deliberately. This header is force-included into every TU, so a
 * static here would give each TU its own copy. Only ONE copy of
 * _FPC_INIT_STABILITY_CONFIG_ survives the link (that is what
 * -Wl,--allow-multiple-definition resolves), so it would open the file and set
 * only its own TU's pointer, and every other TU would still see NULL and write
 * to stdout -- splitting the diagnostics across two destinations. It happens to
 * work when the linker takes all the collapsed functions from the same object,
 * which is the usual case, but that is incidental. As a plain global it
 * collapses along with the other counters and there is exactly one. */
FILE *_FPC_BF_LOG_FP_ = NULL;

static FILE *_FPC_BF_OUT_(void)
{
  return _FPC_BF_LOG_FP_ ? _FPC_BF_LOG_FP_ : stdout;
}

static int    _FPC_PERTURB_ENABLE_    = 0;
static double _FPC_PERTURB_DELTA_ABS_ = 0.0;
static double _FPC_PERTURB_DELTA_REL_ = 1.0e-16;   /* FP64 default */
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

// Call/return double precision error propagation stack
#define _FPC_RET_STACK_MAX_ 8192
long double _FPC_RET_SHADOW_STACK_FP64_[_FPC_RET_STACK_MAX_];
long double _FPC_RET_ERR_STACK_FP64_[_FPC_RET_STACK_MAX_];
long double _FPC_RET_REL_ERR_STACK_FP64_[_FPC_RET_STACK_MAX_];
char _FPC_RET_FUNC_STACK_FP64_[_FPC_RET_STACK_MAX_][_FPC_BB_NAME_SIZE_];
int _FPC_RET_STACK_TOP_FP64_;

// Caller-to-callee argument error propagation buffer
#define _FPC_ARG_BUF_MAX_ 256
long double _FPC_ARG_SHADOW_BUF_FP64_[_FPC_ARG_BUF_MAX_];
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

static inline long double _FPC_READ_FP64_VALUE_FROM_ADDRESS_(uintptr_t address)
{
  if (address < 4096)
    return 0.0L;

  double value = 0.0;
  memcpy(&value, (const void *)address, sizeof(value));
  return (long double)value;
}

static inline int _FPC_TRY_PARSE_FP64_LITERAL_(const char *text,
                                               long double *value)
{
  if (text == NULL || text[0] == '\0')
    return 0;

  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
  {
    int has_fp_syntax = 0;
    int hex_digits = 0;
    for (const char *p = text + 2; *p != '\0'; ++p)
    {
      if (*p == '.' || *p == 'p' || *p == 'P')
      {
        has_fp_syntax = 1;
        break;
      }
      if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
          (*p >= 'A' && *p <= 'F'))
      {
        hex_digits++;
        continue;
      }
      hex_digits = 0;
      break;
    }

    if (!has_fp_syntax && hex_digits > 0 && hex_digits <= 16)
    {
      char *endptr = NULL;
      unsigned long long bits = strtoull(text, &endptr, 16);
      if (endptr != text)
      {
        while (*endptr == ' ' || *endptr == '\t' || *endptr == '\n' ||
               *endptr == '\r' || *endptr == '\f' || *endptr == '\v')
        {
          ++endptr;
        }

        if (*endptr == '\0')
        {
          double decoded = 0.0;
          memcpy(&decoded, &bits, sizeof(decoded));
          *value = (long double)decoded;
          return 1;
        }
      }
    }
  }

  char *endptr = NULL;
  long double parsed = strtold(text, &endptr);
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

/*----------------------------------------------------------------------------*/
/* Initialize                                                                 */
/*----------------------------------------------------------------------------*/

enum _FPC_STABILITY_CLASS_
{
  _FPC_STABLE_TRUE  = 0,
  _FPC_STABLE_FALSE = 1,
  _FPC_UNSTABLE     = 2
};

static long double _FPC_STABILITY_WIDTH_FP64_(long double abs_err,
                                              long double rel_err,
                                              long double val)
{
  long double a1 = fabsl(abs_err);
  long double scale = fabsl(val);
  if (scale < (long double)_FPC_STABILITY_TAU_)
    scale = (long double)_FPC_STABILITY_TAU_;
  long double a2 = rel_err * scale;
  return (a1 > a2) ? a1 : a2;
}

/* COMPRESSED predicate code: 0=eq 1=ne 2=lt 3=le 4=gt 5=ge */
static int _FPC_CLASSIFY_STABILITY_COMPRESSED_FP64_(int pred,
                                                    long double a, long double b,
                                                    long double wa, long double wb,
                                                    long double eta)
{
  long double u = wa + wb + eta;
  long double g = b - a;
  long double diff = a - b;

  if (u == 0.0L)
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
    if (fabsl(diff) > u) return _FPC_STABLE_FALSE;
    return _FPC_UNSTABLE;
  case 1:
    if (fabsl(diff) > u) return _FPC_STABLE_TRUE;
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

/* Printed at exit via atexit(), registered in _FPC_INIT_STABILITY_CONFIG_. */
static void _FPC_BF_SUMMARY_(void)
{
  FILE *o = _FPC_BF_OUT_();
  fprintf(o, "#FPCHECKER: branch-flip summary: unstable=%d nonfinite=%d "
             "policy=%s\n",
          _FPC_STABILITY_WARNING_COUNT_, _FPC_NONFINITE_WARNING_COUNT_,
          _FPC_NONFINITE_POLICY_ ? "unstable" : "abstain");
  if (_FPC_NONFINITE_WARNING_COUNT_ && !_FPC_NONFINITE_POLICY_)
    fprintf(o, "#FPCHECKER: NOTE: %d comparison(s) were ABSTAINED, not judged "
               "stable. Exclude them from scoring; do not count them as "
               "correct rejections.\n", _FPC_NONFINITE_WARNING_COUNT_);
  if (_FPC_BF_LOG_FP_) { fflush(_FPC_BF_LOG_FP_); fclose(_FPC_BF_LOG_FP_); }
}

void _FPC_INIT_STABILITY_CONFIG_()
{
  char *eta_abs = getenv("FPC_STABILITY_ETA_ABS");
  if (eta_abs != NULL) _FPC_STABILITY_ETA_ABS_ = atof(eta_abs);
  char *eta_rel = getenv("FPC_STABILITY_ETA_REL");
  if (eta_rel != NULL) _FPC_STABILITY_ETA_REL_ = atof(eta_rel);
  char *smw = getenv("FPC_STABILITY_MAX_WARNINGS");
  if (smw != NULL) _FPC_STABILITY_MAX_WARNINGS_ = atol(smw);
  char *mw = getenv("FPC_MAX_WARNINGS");
  if (mw != NULL) _FPC_MAX_WARNINGS_ = atol(mw);
  char *nfmw = getenv("FPC_NONFINITE_MAX_WARNINGS");
  if (nfmw != NULL) _FPC_NONFINITE_MAX_WARNINGS_ = atol(nfmw);
  char *nfp = getenv("FPC_NONFINITE_POLICY");
  if (nfp != NULL)
    _FPC_NONFINITE_POLICY_ = (strcmp(nfp, "unstable") == 0) ? 1 : 0;
  char *bflog = getenv("FPC_BF_LOG");
  if (bflog != NULL && *bflog)
  {
    _FPC_BF_LOG_FP_ = fopen(bflog, "w");
    if (_FPC_BF_LOG_FP_ == NULL)
      fprintf(stderr, "#FPCHECKER: WARNING: cannot open FPC_BF_LOG=%s; "
                      "falling back to stdout\n", bflog);
    else
      setvbuf(_FPC_BF_LOG_FP_, NULL, _IOFBF, 1 << 20);
  }

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
  printf("#FPCHECKER: Branch-stability tolerances (FP64): eta_abs=%g, eta_rel=%g\n",
         _FPC_STABILITY_ETA_ABS_, _FPC_STABILITY_ETA_REL_);
  printf("#FPCHECKER: Non-finite policy=%s, max warnings=%s\n",
         _FPC_NONFINITE_POLICY_ ? "unstable" : "abstain",
         _FPC_NONFINITE_MAX_WARNINGS_ <= 0 ? "unlimited" : "limited");
#endif

  /* A per-run tally, printed at exit. The per-event lines can be capped or
   * routed elsewhere, but the counts must always survive: an abstention rate
   * is not recoverable from a truncated log, and scoring needs it to exclude
   * those comparisons rather than read them as correct rejections. */
  atexit(_FPC_BF_SUMMARY_);
}

void _FPC_INIT_HASH_TABLE_FP64()
{
#ifndef FPC_QUIET
  printf("#FPCHECKER: Initializing...\n");
#endif

  int64_t size = 65536;
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
  _FPC_INIT_STABILITY_CONFIG_();
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
  _FPC_INIT_STABILITY_CONFIG_();
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

  _FPC_WRITE_AND_PRINT_TO_JSON_FP64_(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);

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
    FPC_append_value(FPC_DATA_MANAGER, line, (double)(relative_error)); // Cast it to double becasue FPC_append_value function definition takes double in FPC_FloatSeries_List.h
  }
}

/*------------------------------------------------------------------*/
/* Store Function with Location Logging                             */
/*------------------------------------------------------------------*/

// *** Error Calculation *** //
// Instrumentation for STORE instructions
void _FPC_FP64_STORE_INST_(const char *reg, const char *function_name,
                           double stored_value, uintptr_t address, int loc,
                           char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP64_STORE_INST_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP64_STORE_INST_:\n");
  printf("reg=%s, address=%lu\n", reg, address);
#endif

  long double shadow_value = 0.0L;
  long double error = 0.0L;
  long double relative_error = 0.0L;

  // Find if this register already has an error
  int found = _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, reg,
                                               function_name, &shadow_value,
                                               &error, &relative_error);
  if (!found)
  {
    if (_FPC_WARNING_COUNT_ < _FPC_MAX_WARNINGS_)
    {
      _FPC_WARNING_COUNT_++;
      printf("#FPCHECKER: Warning: trying to store a register's value (%s) in function %s, but we don't have its error.\n",
             reg, function_name);
    }

    shadow_value = (long double)stored_value;
    error = 0.0L;
    relative_error = 0.0L;
  }

  // Update table based on the address
  // If address exists, update it
  // If address does not exist, insert new entry
  _FPC_ADDRESS_HT_UPDATE_FP64_(_FPC_ADDRESS_HT_FP64_, address, shadow_value,
                               error, relative_error, file_name, loc);

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

  long double shadow_value = 0.0L;
  long double error = 0.0L;
  long double relative_error = 0.0L;

  // Find what's at this memory address
  int found = _FPC_FIND_VALUE_BY_ADDRESS_FP64(_FPC_ADDRESS_HT_FP64_, address,
                                              &shadow_value, &error,
                                              &relative_error);
  if (found)
  {
    long double current_value = _FPC_READ_FP64_VALUE_FROM_ADDRESS_(address);
    long double reconciliation_error = (shadow_value - current_value) - error;
    long double reconciliation_scale = fmaxl(1.0L,
                                            fmaxl(fabsl(shadow_value),
                                                  fmaxl(fabsl(current_value),
                                                        fabsl(error))));

    if (fabsl(reconciliation_error) > (32.0L * DBL_EPSILON * reconciliation_scale))
    {
      shadow_value = current_value;
      error = 0.0L;
      relative_error = 0.0L;
    }
    else if (shadow_value == 0.0L && error == 0.0L && relative_error == 0.0L)
    {
      shadow_value = current_value;
    }

    // Update register entry with this error
    _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, load_reg,
                                  function_name, shadow_value, error,
                                  relative_error, file_name, loc);

    // Log location info if line is in _FPC_LINES_TO_KEEP_
    FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, relative_error);
  }
  else
  {
    // No error found at this address, create register with zero error
    shadow_value = _FPC_READ_FP64_VALUE_FROM_ADDRESS_(address);
    _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, load_reg,
                                  function_name, shadow_value, 0.0L, 0.0L,
                                  file_name, loc);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("LOAD: No data found at address %lu\n", address);
#endif
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_FP64_(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);
#endif

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP64_LOAD_INST_..........\n");
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

void _FPC_FP64_CMP_(int low_cond, double y, double z, int predicate, int loc,
                    char *file_name, const char *result_name,
                    const char *op1_name, const char *op2_name,
                    const char *function_name, int is_branch_controlling)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double shadow_y = (long double)y;
  long double shadow_z = (long double)z;
  long double err_y = 0.0L, rho_y = 0.0L;
  long double err_z = 0.0L, rho_z = 0.0L;

#ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name,
                                   function_name, &shadow_y, &err_y, &rho_y);
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op2_name,
                                   function_name, &shadow_z, &err_z, &rho_z);
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

  _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, result_name,
                                function_name, shadow_cond ? 1.0L : 0.0L,
                                0.0L, 0.0L, file_name, loc);

  /* ---- Phase 1: branch-instability classification ---- */
  if (is_branch_controlling)
  {
    long double wa = _FPC_STABILITY_WIDTH_FP64_(err_y, rho_y, shadow_y);
    long double wb = _FPC_STABILITY_WIDTH_FP64_(err_z, rho_z, shadow_z);

    long double m = fabsl(shadow_y) > fabsl(shadow_z) ? fabsl(shadow_y)
                                                      : fabsl(shadow_z);
    long double eta = (long double)_FPC_STABILITY_ETA_ABS_;
    long double eta_scaled = (long double)_FPC_STABILITY_ETA_REL_ * m;
    if (eta_scaled > eta)
      eta = eta_scaled;

    /* ---- Non-finite shadow state ----
     *
     * Five different conditions used to be ORed into one message and then
     * silently dropped after ten occurrences. Both were wrong:
     *
     *  - The cause matters. A NaN/Inf in the SHADOW VALUE means the shadow
     *    trajectory itself overflowed -- a real numerical event, and exactly
     *    the overflow-driven case this study is about. A NaN/Inf in the
     *    TRACKED ERROR means the value is fine and the error model blew up --
     *    a tool artifact. INF_ETA is eta_rel*magnitude overflowing, also an
     *    artifact. Reporting them as one thing conflates program behaviour
     *    with instrument behaviour.
     *
     *  - The early `return` is an ABSTENTION, not a prediction of stability.
     *    Downstream scoring sees only "reported" vs "silent", so an abstention
     *    at a flipping site was charged as a false negative and at a
     *    non-flipping site credited as a correct rejection. Neither is what
     *    the tool said. It is now counted and reported so it can be excluded.
     */
    int why = 0;
    if (__builtin_isnan(shadow_y) || __builtin_isnan(shadow_z))
      why |= _FPC_NF_NAN_VAL;
    if (__builtin_isinf(shadow_y) || __builtin_isinf(shadow_z))
      why |= _FPC_NF_INF_VAL;
    if (__builtin_isnan(err_y) || __builtin_isnan(err_z) ||
        __builtin_isnan(rho_y) || __builtin_isnan(rho_z))
      why |= _FPC_NF_NAN_ERR;
    if (__builtin_isinf(err_y) || __builtin_isinf(err_z) ||
        __builtin_isinf(rho_y) || __builtin_isinf(rho_z))
      why |= _FPC_NF_INF_ERR;
    if (!__builtin_isfinite(wa) || !__builtin_isfinite(wb))
      why |= _FPC_NF_INF_WIDTH;
    if (!__builtin_isfinite(eta))
      why |= _FPC_NF_INF_ETA;
    /* The program's own operands, as computed at the working precision. */
    if (__builtin_isnan(y) || __builtin_isnan(z))
      why |= _FPC_NF_LOW_NAN;
    if (__builtin_isinf(y) || __builtin_isinf(z))
      why |= _FPC_NF_LOW_INF;

    if (why)
    {
      if (_FPC_NONFINITE_MAX_WARNINGS_ <= 0 ||
          _FPC_NONFINITE_WARNING_COUNT_ < _FPC_NONFINITE_MAX_WARNINGS_)
      {
        char cause[96];
        _FPC_NF_CAUSE_STR_(why, cause, sizeof(cause));
        /* "at FILE:LINE in FUNC" is kept byte-identical to the Unstable
         * branch line so the existing SITE_RE in the runner scripts parses
         * this without modification. Only the leading tag differs. */
        fprintf(_FPC_BF_OUT_(),
                "#FPCHECKER: Nonfinite branch at %s:%d in %s: "
                "(%Lg %s %Lg) observed=%s cause=%s wa=%Lg, wb=%Lg, eta=%Lg -- %s\n",
                file_name ? file_name : "Unknown", loc,
                function_name ? function_name : "Unknown",
                shadow_y, _FPC_PRED_NAME_COMPRESSED_(predicate), shadow_z,
                low_cond ? "true" : "false", cause, wa, wb, eta,
                _FPC_NONFINITE_POLICY_ ? "classified UNSTABLE by policy"
                                       : "NOT classified (abstain)");
      }
      _FPC_NONFINITE_WARNING_COUNT_++;

      if (!_FPC_NONFINITE_POLICY_)
        return;   /* abstain: no verdict for this comparison */
      /* else fall through and treat it as unstable below */
    }

    int cls = why ? _FPC_UNSTABLE
                  : _FPC_CLASSIFY_STABILITY_COMPRESSED_FP64_(predicate,
                                                             shadow_y, shadow_z,
                                                             wa, wb, eta);

    if (cls == _FPC_UNSTABLE)
    {
      if (_FPC_STABILITY_WARNING_COUNT_ < _FPC_STABILITY_MAX_WARNINGS_)
      {
        _FPC_STABILITY_WARNING_COUNT_++;
        printf("#FPCHECKER: Unstable branch at %s:%d in %s: "
               "(%Lg %s %Lg) observed=%s, uncertainty u=%Lg (wa=%Lg, wb=%Lg, eta=%Lg)\n",
               file_name ? file_name : "Unknown", loc,
               function_name ? function_name : "Unknown",
               shadow_y, _FPC_PRED_NAME_COMPRESSED_(predicate), shadow_z,
               low_cond ? "true" : "false",
               wa + wb + eta, wa, wb, eta);
      }
      long double mag = rho_y > rho_z ? rho_y : rho_z;
      FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, mag);
    }
  }
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
            long double old_shadow = 0.0L;
            long double old_error = 0.0L;
            long double old_relative_error = 0.0L;
            int found = _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_,
                                                         first_substr,
                                                         function_name,
                                                         &old_shadow,
                                                         &old_error,
                                                         &old_relative_error);
            if (found)
            {
              _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_,
                                            register_name, function_name,
                                            old_shadow, old_error,
                                            old_relative_error, "", 0);
            }
            else
            {
              long double literal_shadow = 0.0L;
              if (_FPC_TRY_PARSE_FP64_LITERAL_(first_substr, &literal_shadow))
              {
                _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_,
                                              register_name, function_name,
                                              literal_shadow, 0.0L, 0.0L,
                                              "", 0);
              }
              else
              {
                // We don't have its error - create with zero error
                _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_,
                                              register_name, function_name,
                                              0.0L, 0.0L, 0.0L, "", 0);
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
  printf(".........Exiting _FPC_FP64_PHI_..........\n");
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
    printf("_FPC_FP64_MEMCPY_INST_ (memmove): src=0x%016llx, dst=0x%016llx, size=%ld, size_type=%d\n",
           (unsigned long long)address_src, (unsigned long long)address_dst, size, size_type);
#endif
  }

  _FPC_ADDRESS_RANGE_UPDATE_FP64_(
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
void _FPC_FP64_PUSH_ARG_ERROR_(int arg_index, double arg_value,
                               const char *arg_reg,
                               const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double shadow_value = (long double)arg_value;
  long double error = 0.0L;
  long double relative_error = 0.0L;
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, arg_reg,
                                   function_name, &shadow_value, &error,
                                   &relative_error);

  if (arg_index >= 0 && arg_index < _FPC_ARG_BUF_MAX_)
  {
    _FPC_ARG_SHADOW_BUF_FP64_[arg_index] = shadow_value;
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

  long double shadow_value = 0.0L;
  long double error = 0.0L;
  long double relative_error = 0.0L;

  if (param_index >= 0 && param_index < _FPC_ARG_BUF_COUNT_FP64_)
  {
    shadow_value = _FPC_ARG_SHADOW_BUF_FP64_[param_index];
    error = _FPC_ARG_ERR_BUF_FP64_[param_index];
    relative_error = _FPC_ARG_REL_ERR_BUF_FP64_[param_index];
  }

  _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, param_reg, function_name,
                           shadow_value, error, relative_error, "", 0);
}

static void __attribute__((unused))
_FPC_PERTURB_RECONCILE_FP64_(long double val, long double dx, long double sign,
                             long double r_old, long double rho_old,
                             long double *r_new_out, long double *rho_new_out)
{
  long double r_seed = sign * dx;
  long double r_ref  = (r_old != 0.0L) ? r_old : r_seed;
  long double sgn    = (r_ref >= 0.0L) ? 1.0L : -1.0L;
  long double amag   = fabsl(r_old);
  long double bmag   = fabsl(r_seed);
  long double mag    = (amag > bmag) ? amag : bmag;
  long double r_new  = sgn * mag;

  long double scale = fabsl(val);
  if (scale < (long double)_FPC_STABILITY_TAU_)
    scale = (long double)_FPC_STABILITY_TAU_;
  long double rho_new = fabsl(r_new) / scale;
  if (rho_new < rho_old)
    rho_new = rho_old;

  *r_new_out   = r_new;
  *rho_new_out = rho_new;
}

void _FPC_FP64_PERTURB_SCALAR_(double x, const char *param_reg,
                               const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();
  if (!_FPC_PERTURB_ENABLE_)
    return;

  long double xv = (long double)x;
  long double dx = (long double)_FPC_PERTURB_DELTA_REL_ * fabsl(xv);
  if ((long double)_FPC_PERTURB_DELTA_ABS_ > dx)
    dx = (long double)_FPC_PERTURB_DELTA_ABS_;

  long double sign = (long double)_FPC_PERTURB_NEXT_SIGN_();

  long double shadow_old = xv, r_old = 0.0L, rho_old = 0.0L;
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, param_reg,
                                   function_name, &shadow_old, &r_old, &rho_old);

  long double r_new = 0.0L, rho_new = 0.0L;
  _FPC_PERTURB_RECONCILE_FP64_(xv, dx, sign, r_old, rho_old, &r_new, &rho_new);

  /* shadow = value + error  (invariant: shadow - current == error) */
  _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, param_reg, function_name,
                                xv + r_new, r_new, rho_new, "", 0);
}

void _FPC_FP64_PERTURB_POINTER_(const double *p, long int n,
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
    long double xi = (long double)p[i];

    long double dx = (long double)_FPC_PERTURB_DELTA_REL_ * fabsl(xi);
    if ((long double)_FPC_PERTURB_DELTA_ABS_ > dx)
      dx = (long double)_FPC_PERTURB_DELTA_ABS_;

    long double sign = (long double)_FPC_PERTURB_NEXT_SIGN_();

    uintptr_t addr = (uintptr_t)(&p[i]);

    long double shadow_old = xi, r_old = 0.0L, rho_old = 0.0L;
    _FPC_FIND_VALUE_BY_ADDRESS_FP64(_FPC_ADDRESS_HT_FP64_, addr,
                                    &shadow_old, &r_old, &rho_old);

    long double r_new = 0.0L, rho_new = 0.0L;
    _FPC_PERTURB_RECONCILE_FP64_(xi, dx, sign, r_old, rho_old, &r_new, &rho_new);

    /* shadow = value + error */
    _FPC_ADDRESS_HT_UPDATE_FP64_(_FPC_ADDRESS_HT_FP64_, addr,
                                 xi + r_new, r_new, rho_new, "", 0);
  }
}

// Save error of the value being returned by a function.
void _FPC_FP64_PUSH_RET_ERROR_(const char *ret_reg, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double shadow_value = 0.0L;
  long double error = 0.0L;
  long double relative_error = 0.0L;
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, ret_reg,
                                   function_name, &shadow_value, &error,
                                   &relative_error);

  if (_FPC_RET_STACK_TOP_FP64_ < _FPC_RET_STACK_MAX_)
  {
    _FPC_RET_SHADOW_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_] = shadow_value;
    _FPC_RET_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_] = error;
    _FPC_RET_REL_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_] = relative_error;
    strncpy(_FPC_RET_FUNC_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_], function_name, _FPC_BB_NAME_SIZE_ - 1);
    _FPC_RET_FUNC_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_][_FPC_BB_NAME_SIZE_ - 1] = '\0';
    _FPC_RET_STACK_TOP_FP64_++;
  }
}

// Load most recent returned value error into call result register in caller.
void _FPC_FP64_POP_RET_ERROR_(const char *result_reg, double result_value,
                              const char *function_name,
                              const char *callee_name, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  long double shadow_value = (long double)result_value;
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
      shadow_value = _FPC_RET_SHADOW_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_];
      error = _FPC_RET_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_];
      relative_error = _FPC_RET_REL_ERR_STACK_FP64_[_FPC_RET_STACK_TOP_FP64_];
    }
  }

  _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, result_reg, function_name,
                           shadow_value, error, relative_error, file_name, loc);
  FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, relative_error);
}

/*----------------------------------------------------------------------------*/
/* FP64 Error Calculation (Double Precision Error Tracking)                   */
/*----------------------------------------------------------------------------*/
// *** FP64 Error Calculation *** //
// Calculates the error for a given double precision operation
// Re-computes the operation on long double shadow operands and records the
// rounding error of the fp64 result.

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

  long double shadow_y = (long double)y;
  long double shadow_z = (long double)z;
  long double shadow_w = (long double)w;
  long double _tmp_error_ = 0.0L;
  long double _tmp_relative_error_ = 0.0L;

#ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  if (op == 6)
  {
    _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name,
                                     function_name, &shadow_y, &_tmp_error_,
                                     &_tmp_relative_error_);
    _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op2_name,
                                     function_name, &shadow_z, &_tmp_error_,
                                     &_tmp_relative_error_);
    _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, fma_name,
                                     function_name, &shadow_w, &_tmp_error_,
                                     &_tmp_relative_error_);
  }
  else if (op == 7)
  {
    _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name,
                                     function_name, &shadow_y, &_tmp_error_,
                                     &_tmp_relative_error_);
  }
  else
  {
    _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name,
                                     function_name, &shadow_y, &_tmp_error_,
                                     &_tmp_relative_error_);
    _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op2_name,
                                     function_name, &shadow_z, &_tmp_error_,
                                     &_tmp_relative_error_);
    if (op == 8)
    {
      _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, fma_name,
                                       function_name, &shadow_w,
                                       &_tmp_error_, &_tmp_relative_error_);
    }
  }
#endif

  long double r_high = 0.0L;
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
    if (shadow_z != 0.0L)
    {
      r_high = shadow_y / shadow_z;
    }
    else
    {
      if ((long double)z == 0.0L)
      {
        printf("#FPCHECKER_ERROR: Division by zero at %s:%d (low-precision denominator is zero)\n",
               file_name, loc);
      }
      else
      {
        printf("#FPCHECKER_ERROR: Shadow denominator canceled to zero at %s:%d (low-precision denominator=%.21Le, shadow denominator=%.21Le)\n",
               file_name, loc, (long double)z, shadow_z);
      }
      r_high = 0.0L;
    }
    break;
  case 5:
    r_high = fmodl(shadow_y, shadow_z);
    break;
  case 6:
    r_high = fmal(shadow_y, shadow_z, shadow_w);
    break;
  case 7:
    r_high = -shadow_y; // Negation operation
    break;
  case 8: // Select instruction
  {
    long double shadow_cond = (long double)cond;
    _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name,
                                     function_name, &shadow_cond,
                                     &_tmp_error_, &_tmp_relative_error_);
    r_high = (shadow_cond != 0.0L) ? shadow_z : shadow_w;
    break;
  }
  default:
    printf("#FPCHECKER_ERROR: Unknown operation %d\n", op);
  }

  long double r_low = (long double)x;

  long double err_result = r_high - r_low;
  

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("Result (double):         %.17e\n", x);
  printf("Result (double-> long double): %.21Le\n", r_low);
  printf("Result (long double):        %.21Le\n", r_high);     //r_high is not in double precision, so changed it to long double in the print statement
  printf("Error Result:           %.21Le\n", err_result);
#endif

  // Calculate relative error
  long double rel_error = 0.0L;
  long double largest_subnormal_d = nextafterl(LDBL_MIN, 0.0L);
  if (err_result == 0.0)
  {
    rel_error = 0.0L;
  }
  else
  {
    // Only compute relative error if r_high is not zero and
    // is larger than the largest subnormal
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
  printf("\t >>> Relative Error: %.21Le <<< \n", rel_error);
#endif

  // Update register error table
  _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, result_name,
                                function_name, r_high, err_result, rel_error,
                                file_name, loc);

  // Log location info if line is in _FPC_LINES_TO_KEEP_
  FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, rel_error);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_FP64_(_FPC_ADDRESS_HT_FP64_, _FPC_REGISTER_HT_FP64_);
#endif

  // fflush(stdout);

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP64_CALCULATE_ERROR_..........\n");
#endif
}

/*----------------------------------------------------------------------------*/
/* Math Function Error Analysis.                                              */
/*----------------------------------------------------------------------------*/

// Re-computes a math function call in fp64, accounting for propagated
// operand errors, and records the rounding error of the fp64 result.
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

  long double shadow_y = (long double)y;
  long double shadow_z = (long double)z;
  long double shadow_w = (long double)w;
  long double _tmp_error_ = 0.0L;
  long double _tmp_relative_error_ = 0.0L;

#ifndef FPC_CALCULATE_LOCAL_ERRORS_ONLY
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op1_name,
                                   function_name, &shadow_y, &_tmp_error_,
                                   &_tmp_relative_error_);
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op2_name,
                                   function_name, &shadow_z, &_tmp_error_,
                                   &_tmp_relative_error_);
  _FPC_FIND_VALUE_BY_REGISTER_FP64(_FPC_REGISTER_HT_FP64_, op3_name,
                                   function_name, &shadow_w, &_tmp_error_,
                                   &_tmp_relative_error_);
#endif

  long double r_high = 0.0L;

  // Unary functions
  if      (strcmp(math_func_name, "sin") == 0)       r_high = sinl(shadow_y);
  else if (strcmp(math_func_name, "cos") == 0)       r_high = cosl(shadow_y);
  else if (strcmp(math_func_name, "tan") == 0)       r_high = tanl(shadow_y);
  else if (strcmp(math_func_name, "asin") == 0)      r_high = asinl(shadow_y);
  else if (strcmp(math_func_name, "acos") == 0)      r_high = acosl(shadow_y);
  else if (strcmp(math_func_name, "atan") == 0)      r_high = atanl(shadow_y);
  else if (strcmp(math_func_name, "sinh") == 0)      r_high = sinhl(shadow_y);
  else if (strcmp(math_func_name, "cosh") == 0)      r_high = coshl(shadow_y);
  else if (strcmp(math_func_name, "tanh") == 0)      r_high = tanhl(shadow_y);
  else if (strcmp(math_func_name, "asinh") == 0)     r_high = asinhl(shadow_y);
  else if (strcmp(math_func_name, "acosh") == 0)     r_high = acoshl(shadow_y);
  else if (strcmp(math_func_name, "atanh") == 0)     r_high = atanhl(shadow_y);
  else if (strcmp(math_func_name, "exp") == 0)       r_high = expl(shadow_y);
  else if (strcmp(math_func_name, "exp2") == 0)      r_high = exp2l(shadow_y);
  else if (strcmp(math_func_name, "expm1") == 0)     r_high = expm1l(shadow_y);
  else if (strcmp(math_func_name, "log") == 0)       r_high = logl(shadow_y);
  else if (strcmp(math_func_name, "log2") == 0)      r_high = log2l(shadow_y);
  else if (strcmp(math_func_name, "log10") == 0)     r_high = log10l(shadow_y);
  else if (strcmp(math_func_name, "log1p") == 0)     r_high = log1pl(shadow_y);
  else if (strcmp(math_func_name, "logb") == 0)      r_high = logbl(shadow_y);
  else if (strcmp(math_func_name, "sqrt") == 0)      r_high = sqrtl(shadow_y);
  else if (strcmp(math_func_name, "cbrt") == 0)      r_high = cbrtl(shadow_y);
  else if (strcmp(math_func_name, "fabs") == 0)      r_high = fabsl(shadow_y);
  else if (strcmp(math_func_name, "ceil") == 0)      r_high = ceill(shadow_y);
  else if (strcmp(math_func_name, "floor") == 0)     r_high = floorl(shadow_y);
  else if (strcmp(math_func_name, "trunc") == 0)     r_high = truncl(shadow_y);
  else if (strcmp(math_func_name, "round") == 0)     r_high = roundl(shadow_y);
  else if (strcmp(math_func_name, "nearbyint") == 0) r_high = nearbyintl(shadow_y);
  else if (strcmp(math_func_name, "rint") == 0)      r_high = rintl(shadow_y);
  // Binary functions
  else if (strcmp(math_func_name, "pow") == 0)       r_high = powl(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "atan2") == 0)     r_high = atan2l(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "hypot") == 0)     r_high = hypotl(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "fmod") == 0)      r_high = fmodl(shadow_y, shadow_z);
  else if (strcmp(math_func_name, "remainder") == 0) r_high = remainderl(shadow_y, shadow_z);
  // Ternary functions
  else if (strcmp(math_func_name, "fma") == 0)       r_high = fmal(shadow_y, shadow_z, shadow_w);
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
  printf("Math Result (double-> long double): %.21Le\n", r_low);
  printf("Math Result (long double):        %.21Le\n", r_high);
  printf("Math Error Result:           %.21Le\n", err_result);
  printf("\t >>> Math Relative Error: %.21Le <<< \n", rel_error);
#endif

  _FPC_REGISTER_HT_UPDATE_FP64_(_FPC_REGISTER_HT_FP64_, result_name,
                                function_name, r_high, err_result, rel_error,
                                file_name, loc);
  FPC_APPEND_ERROR_LOG_ENTRY_FP64(loc, rel_error);
}

/*----------------------------------------------------------------------------*/
/* Annotation Macros                                                          */
/*----------------------------------------------------------------------------*/
#include "FPC_Annotations.h"

#ifdef _FPC_DEBUG_STDERR_REDIRECT_
#undef printf
#undef _FPC_DEBUG_STDERR_REDIRECT_
#endif

#endif /* SRC_RUNTIME_ERROR_FP64_H_ */