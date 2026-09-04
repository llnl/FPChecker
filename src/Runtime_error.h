#ifndef SRC_RUNTIME_ERROR_H_
#define SRC_RUNTIME_ERROR_H_

#include "FPC_Hashtable_Error.h"
#include "FPC_FloatSeries_List.h"
#include "FPC_SiteCounter.h"
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
/* Store-miss warnings; FPC_MAX_WARNINGS overrides. */
long _FPC_MAX_WARNINGS_ = MAX_WARNINGS;

double _FPC_STABILITY_ETA_ABS_ = 0.0;
double _FPC_STABILITY_ETA_REL_ = 1.0e-6;
#define _FPC_STABILITY_TAU_ 1.0e-30
int  _FPC_STABILITY_WARNING_COUNT_ = 0;
/* Printed-line cap only; 0 or negative = unlimited. #FPC_EVENT records and
 * #FPC_SITE tables are never capped. */
long _FPC_STABILITY_MAX_WARNINGS_  = MAX_WARNINGS;
int  _FPC_NONFINITE_WARNING_COUNT_ = 0;
/* Same convention as FPC_STABILITY_MAX_WARNINGS. */
long _FPC_NONFINITE_MAX_WARNINGS_ = MAX_WARNINGS;

/* Non-finite shadow at a branch-controlling comparison: 0 = abstain
 * (default), 1 = classify UNSTABLE. An abstention is not a verdict and must
 * not be scored as a correct rejection. */
int _FPC_NONFINITE_POLICY_ = 0;

/* Shadow-rule tallies. */
int _FPC_SHADOW_FLIP_COUNT_ = 0;      /* all SFLIP + SFLIPNF          */
int _FPC_SHADOW_FLIP_NF_COUNT_ = 0;   /* SFLIPNF: shadow non-finite, native finite */
/* On a shadow miss, seed from the native value (1) or from zero (0). */
#ifndef FPC_SHADOW_FALLBACK_NATIVE
#define FPC_SHADOW_FALLBACK_NATIVE 1
#endif
#define _FPC_SHADOW_FALLBACK_(v) (FPC_SHADOW_FALLBACK_NATIVE ? (v) : 0.0)

/* Shadow-miss counters: places where no shadow state was found for a value
 * and it was re-seeded. Printed in the exit summary. */
long _FPC_SHADOW_MISS_LOAD_  = 0;   /* load: stored shadow failed reconciliation */
long _FPC_SHADOW_MISS_STORE_ = 0;   /* store: register had no shadow             */
long _FPC_SHADOW_MISS_RET_   = 0;   /* call result: return stack did not match   */
long _FPC_SHADOW_MISS_ARG_   = 0;   /* parameter: argument buffer had no entry   */
long _FPC_SHADOW_MISS_PHI_   = 0;   /* phi: incoming value untracked, not literal */

/* Cause bits for a non-finite comparison. */
#define _FPC_NF_NAN_VAL   0x01  /* shadow operand is NaN                  */
#define _FPC_NF_INF_VAL   0x02  /* shadow operand is Inf                  */
#define _FPC_NF_NAN_ERR   0x04  /* tracked abs/rel error is NaN           */
#define _FPC_NF_INF_ERR   0x08  /* tracked abs/rel error is Inf           */
#define _FPC_NF_INF_WIDTH 0x10  /* derived interval half-width non-finite */
#define _FPC_NF_INF_ETA   0x20  /* eta_rel * magnitude overflowed         */
/* The program's own operand is non-finite. LOW_INF alone means the program
 * overflowed while the wider shadow did not. */
#define _FPC_NF_LOW_NAN   0x40  /* program (low-precision) operand is NaN */
#define _FPC_NF_LOW_INF   0x80  /* program (low-precision) operand is Inf */

/* Per-cause abstention tallies, indexed by bit position. */
long _FPC_NF_CAUSE_COUNT_[8];
static const char *_FPC_NF_CAUSE_NAMES_[8] = {"NAN_VAL", "INF_VAL", "NAN_ERR", "INF_ERR", "INF_WIDTH", "INF_ETA", "LOW_NAN", "LOW_INF"};

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

/* FPC_BF_LOG=<path> routes branch-flip diagnostics to a file. Plain global,
 * not static, so there is one instance per link. */
FILE *_FPC_BF_LOG_FP_ = NULL;

static FILE *_FPC_BF_OUT_(void)
{
  return _FPC_BF_LOG_FP_ ? _FPC_BF_LOG_FP_ : stdout;
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
/* Valid bit per slot, so a callee whose caller did not push does not read a
 * stale entry. */
unsigned char _FPC_ARG_VALID_[_FPC_ARG_BUF_MAX_];
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

  /* LLVM prints non-short float constants as the hex bit pattern of the
   * double ("0x3FB99999A0000000"); decode those. True hex floats ("0x1.8p3")
   * still go to strtod. */
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
  {
    int has_fp_syntax = 0, hex_digits = 0;
    for (const char *p = text + 2; *p != '\0'; ++p)
    {
      if (*p == '.' || *p == 'p' || *p == 'P') { has_fp_syntax = 1; break; }
      if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
          (*p >= 'A' && *p <= 'F')) { hex_digits++; continue; }
      hex_digits = 0;
      break;
    }
    if (!has_fp_syntax && hex_digits > 0 && hex_digits <= 16)
    {
      char *end = NULL;
      unsigned long long bits = strtoull(text, &end, 16);
      if (end != text)
      {
        while (*end == ' ' || *end == '\t' || *end == '\n' ||
               *end == '\r' || *end == '\f' || *end == '\v')
          ++end;
        if (*end == '\0')
        {
          double decoded = 0.0;
          memcpy(&decoded, &bits, sizeof(decoded));
          *value = decoded;
          return 1;
        }
      }
    }
  }

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
  /* The unordered bit is inert here (this rule abstains on NaN); strip it for
   * the switch. */
  pred = _FPC_PRED_BASE_(pred);

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

/* LLVM fcmp semantics: ordered predicates are false on NaN, unordered are
 * true. */
static int _FPC_EVAL_PRED_(int pred, double a, double b)
{
  if (isnan((double)a) || isnan((double)b))
    return (pred & _FPC_PRED_UNORD_) ? 1 : 0;
  switch (_FPC_PRED_BASE_(pred))
  {
  case 0: return a == b;
  case 1: return a != b;
  case 2: return a <  b;
  case 3: return a <= b;
  case 4: return a >  b;
  case 5: return a >= b;
  default: return 0;
  }
}

static const char *_FPC_PRED_NAME_COMPRESSED_(int pred)
{
  switch (_FPC_PRED_BASE_(pred))
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

/* Printed at exit via atexit(), registered in _FPC_INIT_STABILITY_CONFIG_. */
#define _FPC_BF_PRECISION_ "fp32"

/* .fpc_logs/branch_flip_<host>_<pid>.json: config, counters, per-site table.
 * Events stay in the text log. */
static void _FPC_BF_WRITE_JSON_(void)
{
  struct stat st = {0};
  if (stat(".fpc_logs", &st) == -1)
    mkdir(".fpc_logs", 0775);
  char host[256];
  if (gethostname(host, sizeof(host)) != 0)
    strcpy(host, "node-unknown");
  char path[512];
  snprintf(path, sizeof(path), ".fpc_logs/branch_flip_%s_%d.json", host,
           (int)getpid());
  FILE *fp = fopen(path, "w");
  if (!fp)
    return;
  fprintf(fp, "{\n");
  fprintf(fp, "  \"precision\": \"%s\",\n", _FPC_BF_PRECISION_);
  fprintf(fp, "  \"config\": {\"rule\": \"%s\", \"eta_abs\": %g, \"eta_rel\": %g, "
              "\"nonfinite_policy\": \"%s\", \"shadow_fallback_native\": %d, "
              "\"stability_max_warnings\": %ld, \"nonfinite_max_warnings\": %ld},\n",
          _FPC_BF_MODE_ == (_FPC_RULE_INTERVAL | _FPC_RULE_SHADOW) ? "both"
          : _FPC_BF_MODE_ == _FPC_RULE_SHADOW ? "shadow" : "interval",
          _FPC_STABILITY_ETA_ABS_, _FPC_STABILITY_ETA_REL_,
          _FPC_NONFINITE_POLICY_ ? "unstable" : "abstain",
          FPC_SHADOW_FALLBACK_NATIVE ? 1 : 0,
          _FPC_STABILITY_MAX_WARNINGS_, _FPC_NONFINITE_MAX_WARNINGS_);
  fprintf(fp, "  \"counts\": {\"unstable\": %d, \"nonfinite\": %d, "
              "\"shadow_flip\": %d, \"shadow_flip_nonfinite\": %d},\n",
          _FPC_STABILITY_WARNING_COUNT_, _FPC_NONFINITE_WARNING_COUNT_,
          _FPC_SHADOW_FLIP_COUNT_, _FPC_SHADOW_FLIP_NF_COUNT_);
  fprintf(fp, "  \"nonfinite_causes\": {");
  for (int b = 0; b < 8; ++b)
    fprintf(fp, "%s\"%s\": %ld", b ? ", " : "", _FPC_NF_CAUSE_NAMES_[b],
            _FPC_NF_CAUSE_COUNT_[b]);
  fprintf(fp, "},\n");
  fprintf(fp, "  \"shadow_misses\": {\"load\": %ld, \"store\": %ld, \"ret\": %ld, "
              "\"arg\": %ld, \"phi\": %ld},\n",
          _FPC_SHADOW_MISS_LOAD_, _FPC_SHADOW_MISS_STORE_, _FPC_SHADOW_MISS_RET_,
          _FPC_SHADOW_MISS_ARG_, _FPC_SHADOW_MISS_PHI_);
  fprintf(fp, "  \"sites\": ");
  _FPC_SITE_DUMP_JSON_(fp);
  fprintf(fp, ",\n  \"site_table_overflow\": %llu\n}\n",
          (unsigned long long)_FPC_SITE_TAB_OVERFLOW_);
  fclose(fp);
  printf("#FPCHECKER: Writing branch-flip JSON to: %s\n", path);
}

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
  if (_FPC_BF_MODE_ & _FPC_RULE_SHADOW)
    fprintf(o, "#FPCHECKER: shadow-rule summary: sflip=%d sflip_nonfinite=%d\n",
            _FPC_SHADOW_FLIP_COUNT_, _FPC_SHADOW_FLIP_NF_COUNT_);
  fprintf(o, "#FPCHECKER: nonfinite causes:");
  for (int b = 0; b < 8; ++b)
    fprintf(o, " %s=%ld", _FPC_NF_CAUSE_NAMES_[b], _FPC_NF_CAUSE_COUNT_[b]);
  fprintf(o, "\n");
  fprintf(o, "#FPCHECKER: shadow misses: load=%ld store=%ld ret=%ld arg=%ld "
             "phi=%ld\n", _FPC_SHADOW_MISS_LOAD_, _FPC_SHADOW_MISS_STORE_,
          _FPC_SHADOW_MISS_RET_, _FPC_SHADOW_MISS_ARG_, _FPC_SHADOW_MISS_PHI_);
  /* Per-site full-run execution totals. */
  _FPC_SITE_DUMP_(o);
  _FPC_BF_WRITE_JSON_();
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
  /* FPC_BF_MODE = interval (default) | shadow | both */
  char *bfm = getenv("FPC_BF_MODE");
  if (bfm != NULL)
  {
    if (strcmp(bfm, "shadow") == 0)
      _FPC_BF_MODE_ = _FPC_RULE_SHADOW;
    else if (strcmp(bfm, "both") == 0)
      _FPC_BF_MODE_ = _FPC_RULE_INTERVAL | _FPC_RULE_SHADOW;
    else
      _FPC_BF_MODE_ = _FPC_RULE_INTERVAL;
  }

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

#ifndef FPC_QUIET
  printf("#FPCHECKER: Branch-stability tolerances: eta_abs=%g, eta_rel=%g\n",
         _FPC_STABILITY_ETA_ABS_, _FPC_STABILITY_ETA_REL_);
  printf("#FPCHECKER: Branch-flip rule=%s\n",
         _FPC_BF_MODE_ == (_FPC_RULE_INTERVAL | _FPC_RULE_SHADOW) ? "both"
         : _FPC_BF_MODE_ == _FPC_RULE_SHADOW ? "shadow" : "interval");
  printf("#FPCHECKER: Non-finite policy=%s, max warnings=%s, "
         "stability max warnings=%s\n",
         _FPC_NONFINITE_POLICY_ ? "unstable" : "abstain",
         _FPC_NONFINITE_MAX_WARNINGS_ <= 0 ? "unlimited" : "limited",
         _FPC_STABILITY_MAX_WARNINGS_ <= 0 ? "unlimited" : "limited");
#endif

  /* Per-run tallies, printed at exit; never capped. */
  atexit(_FPC_BF_SUMMARY_);
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
    if (_FPC_WARNING_COUNT_ < _FPC_MAX_WARNINGS_)
    {
      _FPC_WARNING_COUNT_++;
      printf("#FPCHECKER: Warning: trying to store a register's value (%s) in function %s, but we don't have its error.\n",
             reg, function_name);
    }

    _FPC_SHADOW_MISS_STORE_++;
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
      _FPC_SHADOW_MISS_LOAD_++;
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

/* Rule I: interval overlap (FPChecker, Sec 3.4). y/z are the program's own
 * low-precision operands. */
static void _FPC_BF_RULE_INTERVAL_(int low_cond, double y, double z,
                                   double shadow_y, double shadow_z,
                                   double err_y, double rho_y,
                                   double err_z, double rho_z,
                                   int predicate, int loc, char *file_name,
                                   const char *function_name,
                                   unsigned mod_id, int site_id, uint64_t bf_k)
{
  double wa = _FPC_STABILITY_WIDTH_(err_y, rho_y, shadow_y);
  double wb = _FPC_STABILITY_WIDTH_(err_z, rho_z, shadow_z);

  double m = fabs(shadow_y) > fabs(shadow_z) ? fabs(shadow_y) : fabs(shadow_z);
  double eta = _FPC_STABILITY_ETA_ABS_;
  double eta_scaled = _FPC_STABILITY_ETA_REL_ * m;
  if (eta_scaled > eta)
    eta = eta_scaled;

  /* Non-finite shadow state: record the cause, then abstain (not "stable")
   * unless the policy says UNSTABLE. */
  int why = 0;
  if (isnan(shadow_y) || isnan(shadow_z))              why |= _FPC_NF_NAN_VAL;
  if (isinf(shadow_y) || isinf(shadow_z))              why |= _FPC_NF_INF_VAL;
  if (isnan(err_y) || isnan(err_z) ||
      isnan(rho_y) || isnan(rho_z))                    why |= _FPC_NF_NAN_ERR;
  if (isinf(err_y) || isinf(err_z) ||
      isinf(rho_y) || isinf(rho_z))                    why |= _FPC_NF_INF_ERR;
  if (!isfinite(wa) || !isfinite(wb))                  why |= _FPC_NF_INF_WIDTH;
  if (!isfinite(eta))                                  why |= _FPC_NF_INF_ETA;
  /* The program's own operands, as computed at the working precision. */
  if (isnan(y) || isnan(z))                            why |= _FPC_NF_LOW_NAN;
  if (isinf(y) || isinf(z))                            why |= _FPC_NF_LOW_INF;

  if (why)
  {
    for (int b = 0; b < 8; ++b)
      if (why & (1 << b))
        _FPC_NF_CAUSE_COUNT_[b]++;
    char cause[96];
    _FPC_NF_CAUSE_STR_(why, cause, sizeof(cause));
    if (_FPC_NONFINITE_MAX_WARNINGS_ <= 0 ||
        _FPC_NONFINITE_WARNING_COUNT_ < _FPC_NONFINITE_MAX_WARNINGS_)
    {
      /* Same "at FILE:LINE in FUNC" shape as the Unstable line so SITE_RE
       * parses it. */
      fprintf(_FPC_BF_OUT_(),
              "#FPCHECKER: Nonfinite branch at %s:%d in %s: "
              "(%g %s %g) observed=%s cause=%s wa=%g, wb=%g, eta=%g -- %s\n",
              file_name ? file_name : "Unknown", loc,
              function_name ? function_name : "Unknown",
              shadow_y, _FPC_PRED_NAME_COMPRESSED_(predicate), shadow_z,
              low_cond ? "true" : "false", cause, wa, wb, eta,
              _FPC_NONFINITE_POLICY_ ? "classified UNSTABLE by policy"
                                     : "NOT classified (abstain)");
    }
    _FPC_NONFINITE_WARNING_COUNT_++;

    /* Event record is never capped. */
    if (!_FPC_NONFINITE_POLICY_)
    {
      _FPC_SITE_MARK_(mod_id, site_id, 1);
      _FPC_SITE_EMIT_(_FPC_BF_OUT_(), mod_id, site_id, bf_k, "DECLINED",
                      file_name, loc, function_name, cause);
    }

    if (!_FPC_NONFINITE_POLICY_)
      return;   /* abstain: no verdict for this comparison */
    /* else fall through and treat it as unstable below */
  }

  int cls = why ? _FPC_UNSTABLE
                : _FPC_CLASSIFY_STABILITY_COMPRESSED_(predicate,
                                                      shadow_y, shadow_z,
                                                      wa, wb, eta);

  if (cls == _FPC_UNSTABLE)
  {
    _FPC_SITE_MARK_(mod_id, site_id, 0);
    _FPC_SITE_EMIT_(_FPC_BF_OUT_(), mod_id, site_id, bf_k, "UNSTABLE",
                    file_name, loc, function_name, NULL);
    /* Counted outside the print cap. */
    _FPC_STABILITY_WARNING_COUNT_++;
    if (_FPC_STABILITY_MAX_WARNINGS_ <= 0 ||
        _FPC_STABILITY_WARNING_COUNT_ <= _FPC_STABILITY_MAX_WARNINGS_)
    {
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

/* Rule S: shadow-predicate disagreement (Chevalier et al., CC'21). Same predicate
 * on the shadow operands; a flip iff the two booleans differ. No threshold,
 * no abstention. Our shadow is one step wider (float->double, double->long
 * double), not two. */
static void _FPC_BF_RULE_SHADOW_(int low_cond, int shadow_cond,
                                 double y, double z,
                                 double shadow_y, double shadow_z,
                                 int predicate, int loc, char *file_name,
                                 const char *function_name,
                                 unsigned mod_id, int site_id, uint64_t bf_k)
{
  if (shadow_cond == low_cond)
    return;              /* agreement: silent, and NOT an abstention */

  /* SFLIPNF: shadow operand non-finite while native is finite. A flip under
   * the rule, but caused by the shadow's own pathology; the interval rule
   * abstains on the same state. */
  int nf = (!isfinite((double)shadow_y) && isfinite((double)y)) ||
           (!isfinite((double)shadow_z) && isfinite((double)z));
  _FPC_SITE_MARK_S_(mod_id, site_id);
  _FPC_SITE_EMIT_(_FPC_BF_OUT_(), mod_id, site_id, bf_k,
                  nf ? "SFLIPNF" : "SFLIP", file_name, loc, function_name,
                  NULL);
  if (nf)
    _FPC_SHADOW_FLIP_NF_COUNT_++;
  _FPC_SHADOW_FLIP_COUNT_++;
  if (_FPC_STABILITY_MAX_WARNINGS_ <= 0 ||
      _FPC_SHADOW_FLIP_COUNT_ <= _FPC_STABILITY_MAX_WARNINGS_)
    fprintf(_FPC_BF_OUT_(),
            "#FPCHECKER: Shadow branch flip%s at %s:%d in %s: "
            "native (%.17g %s %.17g)=%s, shadow (%.17g %s %.17g)=%s\n",
            nf ? " (non-finite shadow)" : "",
            file_name ? file_name : "Unknown", loc,
            function_name ? function_name : "Unknown",
            y, _FPC_PRED_NAME_COMPRESSED_(predicate), z,
            low_cond ? "true" : "false",
            shadow_y, _FPC_PRED_NAME_COMPRESSED_(predicate), shadow_z,
            shadow_cond ? "true" : "false");
}

void _FPC_FP32_CMP_(int low_cond, float y, float z, int predicate, int loc,
                    char *file_name, const char *result_name,
                    const char *op1_name, const char *op2_name,
                    const char *function_name, int is_branch_controlling,
                    unsigned mod_id, int site_id)
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

  int shadow_cond = _FPC_EVAL_PRED_(predicate, shadow_y, shadow_z);

  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, result_name, function_name,
                           shadow_cond ? 1.0 : 0.0, 0.0, 0.0, file_name,
                           loc);

  /* ---- Branch-instability classification ---- */
  if (is_branch_controlling)
  {
    /* Occurrence index k. Ticked before any verdict or early return so it
     * counts executions, not reports. site_id == -1: this comparison controls
     * no numbered branch. */
    uint64_t bf_k = _FPC_SITE_TICK_(mod_id, site_id);

    /* Rule S first: the interval rule may abstain and return early. Both get
     * the same bf_k. */
    if (_FPC_BF_MODE_ & _FPC_RULE_SHADOW)
      _FPC_BF_RULE_SHADOW_(low_cond, shadow_cond, (double)y, (double)z,
                           shadow_y, shadow_z, predicate, loc,
                           file_name, function_name, mod_id, site_id,
                           bf_k);

    if (_FPC_BF_MODE_ & _FPC_RULE_INTERVAL)
      _FPC_BF_RULE_INTERVAL_(low_cond, (double)y, (double)z, shadow_y, shadow_z,
                             err_y, rho_y, err_z, rho_z, predicate, loc,
                             file_name, function_name, mod_id, site_id,
                             bf_k);
  }
}

// This function is called for PHI nodes in SSA form
// It is used to log the values that are being merged
void _FPC_FP32_PHI_(const char *phi_values, double phi_value,
                    const char *function_name)
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
                /* Untracked and not a literal: fall back. */
                _FPC_SHADOW_MISS_PHI_++;
                _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, register_name,
                                         function_name, _FPC_SHADOW_FALLBACK_(phi_value), 0.0, 0.0,
                                         "", 0);
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
    _FPC_ARG_VALID_[arg_index] = 1;
    if (arg_index >= _FPC_ARG_BUF_COUNT_)
      _FPC_ARG_BUF_COUNT_ = arg_index + 1;
  }
}

// Pop argument error from the buffer into the callee's parameter register.
// Called once per FP parameter, in order, at callee function entry.
void _FPC_FP32_POP_ARG_ERROR_(int param_index, double param_value,
                              const char *param_reg, const char *function_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  double shadow_value = _FPC_SHADOW_FALLBACK_(param_value);
  double error = 0.0;
  double relative_error = 0.0;

  if (param_index >= 0 && param_index < _FPC_ARG_BUF_COUNT_ &&
      _FPC_ARG_VALID_[param_index])
  {
    shadow_value = _FPC_ARG_SHADOW_BUF_[param_index];
    error = _FPC_ARG_ERR_BUF_[param_index];
    relative_error = _FPC_ARG_REL_ERR_BUF_[param_index];
    _FPC_ARG_VALID_[param_index] = 0;   /* consumed */
  }
  else
    _FPC_SHADOW_MISS_ARG_++;

  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, param_reg, function_name,
                           shadow_value, error, relative_error, "", 0);
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
void _FPC_FP32_POP_RET_ERROR_(const char *result_reg, double result_value,
                              const char *function_name,
                              const char *callee_name, int loc, char *file_name)
{
  _FPC_ENSURE_RUNTIME_READY_();

  double shadow_value = _FPC_SHADOW_FALLBACK_(result_value);
  double error = 0.0;
  double relative_error = 0.0;
  int popped = 0;

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
      popped = 1;
    }
  }
  if (!popped)
    _FPC_SHADOW_MISS_RET_++;

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
