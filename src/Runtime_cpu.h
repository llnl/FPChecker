#ifndef SRC_RUNTIME_CPU_H_
#define SRC_RUNTIME_CPU_H_

#include "FPC_Hashtable.h"
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <float.h>

#ifdef FPC_MULTI_THREADED
#include <pthread.h>
#endif

#define FPC_MAX(a, b) (((a) > (b)) ? (a) : (b))

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

// We store the file name and directory in this variable (one variable per file)
__attribute__((used)) static char *_FPC_FILE_NAME_;

/** Hash table pointer **/
_FPC_HTABLE_T *_FPC_HTABLE_;

#ifdef FPC_DANGER_ZONE_PERCENT
#define DANGER_ZONE_PERCENTAGE FPC_DANGER_ZONE_PERCENT
#else
#define DANGER_ZONE_PERCENTAGE 0.05
#endif

#ifdef FPC_MULTI_THREADED
pthread_mutex_t fpc_lock;
#endif

/** Program name and input **/
int _FPC_PROG_INPUTS;
char **_FPC_PROG_ARGS;

// *** Error Calculation *** //
// Maximum number of error entries
#define MAX_ERROR_ENTRIES 100000
#define MAX_NAME_SIZE 500

// *** Error Calculation *** //
// saves the line and file name
typedef struct
{
  char file[MAX_NAME_SIZE];
  int line;
} FPC_ERROR_LOG_ENTRY;

FPC_ERROR_LOG_ENTRY ERROR_LOG[MAX_ERROR_ENTRIES];

// *** Error Calculation *** //
uintptr_t _FPC_ADDRESSES_[MAX_ERROR_ENTRIES];           // Runtime addresses
char _FPC_REGISTERS_[MAX_ERROR_ENTRIES][MAX_NAME_SIZE]; // Runtime register names
double _FPC_ERRORS_[MAX_ERROR_ENTRIES];                 // Runtime (rounding) error values
double _FPC_RELATIVE_ERRORS_[MAX_ERROR_ENTRIES];        // Runtime relative error values
int _FPC_OPERATION_CLOCK_[MAX_ERROR_ENTRIES];           // Operation clock
int _FPC_CLOCK_ = 0;                                    // Counter for the order of operations (1, 2, 3, ...)
int _FPC_ENTRY_COUNT_ = 0;                              // Counter of the number of entries in the above arrays

// *** Error Calculation *** //
char _FPC_USED_REG_SET_[MAX_ERROR_ENTRIES][MAX_NAME_SIZE]; // Used registers for child without parent calculation
int _FPC_USED_REG_COUNT_ = 0;                              // Counter for the above array

// *** Error Calculation *** //
uintptr_t _FPC_USED_ADDR_SET_[MAX_ERROR_ENTRIES]; // Used addresses for child without parent calculation
int _FPC_USED_ADDR_COUNT_ = 0;                    // Counter for the above array

// *** Error Calculation *** //
#define _FPC_BB_NAME_SIZE_ 512                   // max size of a basic block name - for example: %bb_26
char _FPC_LAST_BASIC_BLOCK_[_FPC_BB_NAME_SIZE_]; // Last basic block ID

/*----------------------------------------------------------------------------*/
/* Error Calculation Functions                                                */
/*----------------------------------------------------------------------------*/

// *** Error Calculation *** //
// If a register is used, it is added to the use_reg_set array
void _FPC_USED_REG_(const char *reg)
{
  if (!reg || strlen(reg) == 0)
  {
    // This is normal for FMA operations where fma_name can be NULL
    printf("#FPCHECKER: NULL register (normal for FMA) - skipping\n");
    return;
  }

  // printf("#FPCHECKER: Marking register as used: %s\n", reg);

  // Check if already in used set
  for (int i = 0; i < _FPC_USED_REG_COUNT_; i++)
  {
    if (strcmp(_FPC_USED_REG_SET_[i], reg) == 0)
    {
      // printf("#FPCHECKER: Register %s already marked as used at index %d\n", reg, i);
      return;
    }
  }

  // Add to used set
  if (_FPC_USED_REG_COUNT_ < MAX_ERROR_ENTRIES)
  {
    strncpy(_FPC_USED_REG_SET_[_FPC_USED_REG_COUNT_], reg, MAX_NAME_SIZE - 1);
    _FPC_USED_REG_SET_[_FPC_USED_REG_COUNT_][MAX_NAME_SIZE - 1] = '\0';
    // printf("#FPCHECKER: Added %s to used_reg_set[%d]\n", reg, _FPC_USED_REG_COUNT_);
    _FPC_USED_REG_COUNT_++;
  }
  else
  {
    printf("#FPCHECKER: Used register set full\n");
  }
}

// *** Error Calculation *** //
// If an address is used, it is added to the use_addr_set array
void _FPC_USED_ADDR_(uintptr_t addr)
{
  for (int i = 0; i < _FPC_USED_ADDR_COUNT_; ++i)
  {
    if (_FPC_USED_ADDR_SET_[i] == addr)
      return;
  }
  if (_FPC_USED_ADDR_COUNT_ < MAX_ERROR_ENTRIES)
  {
    _FPC_USED_ADDR_SET_[_FPC_USED_ADDR_COUNT_++] = addr;
  }
}

// *** Error Calculation *** //
// This function copies line and file information to the error log
void _FPC_LOG_LOCATION_(const char *file, int line)
{
  if (_FPC_ENTRY_COUNT_ > 0)
  {
    strncpy(ERROR_LOG[_FPC_ENTRY_COUNT_ - 1].file, file, MAX_NAME_SIZE - 1);
    ERROR_LOG[_FPC_ENTRY_COUNT_ - 1].line = line;
  }
}

// *** Error Calculation *** //
// This function removes duplicate entries from the error log
void _FPC_REMOVE_DUPLICATES_()
{
  // printf("\n======== REMOVING DUPLICATES ========\n");

  int unique_count = 0;

  for (int i = 0; i < _FPC_ENTRY_COUNT_; i++)
  {
    int found_duplicate = 0;

    for (int j = 0; j < unique_count; j++)
    {
      if (strcmp(_FPC_REGISTERS_[j], _FPC_REGISTERS_[i]) == 0)
      {
        found_duplicate = 1;
        // printf("Found duplicate: %s at positions %d and %d\n", _FPC_REGISTERS_[i], j, i);

        // Merge information: prefer the entry with more complete data
        if (_FPC_ADDRESSES_[i] != 0 && _FPC_ADDRESSES_[j] == 0)
        {
          // Current entry has address, existing doesn't - update existing
          // printf("  Updating position %d with address %lu from position %d\n", j, _FPC_ADDRESSES_[i], i);
          _FPC_ADDRESSES_[j] = _FPC_ADDRESSES_[i];
          _FPC_ERRORS_[j] = _FPC_ERRORS_[i];
          strcpy(ERROR_LOG[j].file, ERROR_LOG[i].file);
          // ERROR_LOG[j].line = ERROR_LOG[i].line;
        }
        else if (ERROR_LOG[i].file[0] != '\0' && ERROR_LOG[j].file[0] == '\0')
        { // Merge the file and line information
          strcpy(ERROR_LOG[j].file, ERROR_LOG[i].file);
          ERROR_LOG[j].line = ERROR_LOG[i].line;
          _FPC_ERRORS_[j] = _FPC_ERRORS_[i];
        }
        break;
      }
    }

    if (!found_duplicate)
    {
      // No duplicate - keep this entry
      if (unique_count != i)
      {
        // Move entry to write position
        strcpy(_FPC_REGISTERS_[unique_count], _FPC_REGISTERS_[i]);
        _FPC_ADDRESSES_[unique_count] = _FPC_ADDRESSES_[i];
        _FPC_ERRORS_[unique_count] = _FPC_ERRORS_[i];
        strcpy(ERROR_LOG[unique_count].file, ERROR_LOG[i].file);
        ERROR_LOG[unique_count].line = ERROR_LOG[i].line;
        // printf("Moved entry %d to position %d: %s\n", i, unique_count, _FPC_REGISTERS_[i]);
      }
      unique_count++;
    }
  }

  _FPC_ENTRY_COUNT_ = unique_count;
}

// *** Error Calculation *** //
// Here sink == node without child
// This function looks for nodes (registers or addresses) that are not used
int _FPC_IS_FINAL_CHILD_(int index)
{
  if (index >= _FPC_ENTRY_COUNT_)
    return 0;

  const char *reg = _FPC_REGISTERS_[index];
  uintptr_t addr = _FPC_ADDRESSES_[index];

  // Case 1: Register-based
  if (reg && reg[0] != '\0')
  {
    // Check if this register is used as operand later
    for (int i = 0; i < _FPC_USED_REG_COUNT_; ++i)
    {
      if (strcmp(_FPC_USED_REG_SET_[i], reg) == 0)
        return 0; // Not a sink: reg is used
    }
    return 1; // Sink: reg not used
  }

  // Case 2: Address-based sink (if register is empty/null)
  if ((!reg || reg[0] == '\0') && addr != 0)
  {
    // Check if this address is used (loaded from) later
    for (int i = 0; i < _FPC_USED_ADDR_COUNT_; ++i)
    {
      if (_FPC_USED_ADDR_SET_[i] == addr)
        return 0; // Not a sink: address is used
    }
    return 1; // Sink: address not used
  }

  // Otherwise, not a sink
  return 0;
}

// *** Error Calculation *** //
// This function is for debugging purposes. It can be removed in production
void _FPC_DEBUG_PRINT_ALL_TRACKED_DATA_()
{
  printf("\n======== REGISTER TABLE ========\n");
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    printf("%d.  %s\n", i, _FPC_REGISTERS_[i]);
  }

  printf("\n======== UNIQUE REGISTERS ========\n");
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    int is_duplicate = 0;
    for (int j = 0; j < i; ++j)
    {
      if (strcmp(_FPC_REGISTERS_[i], _FPC_REGISTERS_[j]) == 0)
      {
        is_duplicate = 1;
        break;
      }
    }
    if (!is_duplicate)
    {
      printf("%d. %s\n", i, _FPC_REGISTERS_[i]);
    }
  }

  printf("\n======== UNIQUE REGISTERS NOT USED AS OPERANDS ========\n");
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    const char *reg = _FPC_REGISTERS_[i];
    if (!reg || reg[0] == '\0')
      continue;
    // Check if reg is unique up to i
    int is_duplicate = 0;
    for (int j = 0; j < i; ++j)
    {
      if (strcmp(_FPC_REGISTERS_[j], reg) == 0)
      {
        is_duplicate = 1;
        break;
      }
    }
    if (is_duplicate)
      continue;
    // Check if reg is in used set
    int is_used = 0;
    for (int k = 0; k < _FPC_USED_REG_COUNT_; ++k)
    {
      if (strcmp(reg, _FPC_USED_REG_SET_[k]) == 0)
      {
        is_used = 1;
        break;
      }
    }
    if (!is_used)
    {
      printf("%d. %s\n", i, reg);
    }
  }

  printf("\n======== ADDRESS TABLE ========\n");
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    if (_FPC_ADDRESSES_[i] != 0)
      printf("%d. = %ld\n", i, _FPC_ADDRESSES_[i]);
  }

  printf("\n======== USED REGISTERS ========\n");
  for (int i = 0; i < _FPC_USED_REG_COUNT_; ++i)
  {
    printf("Used Reg[%d] = %s\n", i, _FPC_USED_REG_SET_[i]);
  }

  printf("\n======== USED ADDRESSES ========\n");
  for (int i = 0; i < _FPC_USED_ADDR_COUNT_; ++i)
  {
    printf("Used Addr[%d] = %ld\n", i, _FPC_USED_ADDR_SET_[i]);
  }
  printf("=================================\n\n");
}

/*----------------------------------------------------------------------------*/
/* Initialize                                                                 */
/*----------------------------------------------------------------------------*/

void _FPC_INIT_HASH_TABLE_()
{
#ifndef FPC_QUIET
  printf("#FPCHECKER: Initializing...\n");
#endif
  int64_t size = 1000;
  _FPC_HTABLE_ = _FPC_HT_CREATE_(size);

#ifdef FPC_MULTI_THREADED
  if (pthread_mutex_init(&fpc_lock, NULL) != 0)
  {
    printf("#FPCHECKER: Mutex init failed for multi-threading\n");
  }
#endif
}

void _FPC_INIT_FPCHECKER()
{
  _FPC_PROG_INPUTS = 0;
  _FPC_LAST_BASIC_BLOCK_[0] = '\0';
  _FPC_INIT_HASH_TABLE_();
}

void _FPC_INIT_ARGS_FPCHECKER(int argc, char **argv)
{
  _FPC_PROG_INPUTS = argc;
  _FPC_PROG_ARGS = argv;
  _FPC_INIT_HASH_TABLE_();
}

// *** Error Calculation *** //
// Prints the errors to a JSON file
void _FPC_WRITE_AND_PRINT_TO_JSON_()
{
  // Create directory
  struct stat st;
  char dir_name[] = ".fpc_logs";
  if (stat(dir_name, &st) == -1)
  { // dir doesn't exists
    mkdir(dir_name, 0775);
  }

  // Set filename
  // On Linux: The maximum length for a file name is 255 bytes.
  // The maximum combined length of both the file name and path name is 4096 bytes.
  char executionId[5000];
  char fileName[5000];
  char errorFileName[5000];
  errorFileName[0] = '\0';
  strcpy(errorFileName, ".fpc_logs/rounding_error_");

  // ----------- Get execution ID -----------
  // size_t len=256;
  //  According to Linux manual:
  //  Each element of the hostname must be from 1 to 63 characters long
  //  and the entire hostname, including the dots, can be at most 253
  //  characters long.
  executionId[0] = '\0';
  if (gethostname(executionId, 256) != 0)
    strcpy(executionId, "node-unknown");

  // Maximum size for PID: we assume 2,000,000,000
  int pid = (int)getpid();
  char pidStr[11];
  // pidStr[0] = '\0';
  // sprintf(pidStr, "%d", pid);
  snprintf(pidStr, sizeof(pidStr), "%d", pid);
  strcat(executionId, "_");
  strcat(executionId, pidStr);
  strcat(executionId, ".json");

  strcat(fileName, executionId);
  strcat(errorFileName, executionId);

  printf("#FPCHECKER: Writing JSON to: %s\n", errorFileName);

  FILE *fp = fopen(errorFileName, "w");
  if (!fp)
  {
    perror("fopen");
    return;
  }

  fprintf(fp, "[\n");

  /*
  int first = 1;
  int entries_written = 0;
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    // Only include final sinks that have valid file information
    if (_FPC_IS_FINAL_CHILD_(i) && ERROR_LOG[i].file[0] != '\0')
    {
      if (!first)
        fprintf(fp, ",\n");
      first = 0;

      fprintf(fp, "  {\n");
      fprintf(fp, "    \"file\": \"%s\",\n", ERROR_LOG[i].file);
      fprintf(fp, "    \"line\": %d,\n", ERROR_LOG[i].line);
      fprintf(fp, "    \"error\": %.17e\n", _FPC_ERRORS_[i]);
      fprintf(fp, "  }");
      entries_written++;
    }
  }
  */

  int first = 1;
  int entries_written = 0;
  // Array to keep track of printed indices to avoid duplicates
  int *printed_indices = (int *)malloc(sizeof(int) * _FPC_ENTRY_COUNT_); // 0 = not printed, 1 = printed
  if (!printed_indices)
  {
    fprintf(stderr, "Failed to allocate memory for printed_indices\n");
    fclose(fp);
    return;
  }
  for (int i = 0; i < _FPC_ENTRY_COUNT_; i++)
  {
    printed_indices[i] = 0;
  }

  for (int i = 0; i < _FPC_ENTRY_COUNT_; i++)
  {
    // Skip if this entry has already been processed
    if (printed_indices[i])
    {
      continue;
    }

    if (ERROR_LOG[i].file[0] == '\0')
    {
      // Skip this entry if the file name is empty
      continue;
    }

    // Initialize variables to track the highest clock
    int highest_clock = _FPC_OPERATION_CLOCK_[i];
    int highest_clock_index = i;

    // Find other files with the same name and line and check for a higher clock
    for (int j = i + 1; j < _FPC_ENTRY_COUNT_; j++)
    {
      if (strcmp(ERROR_LOG[i].file, ERROR_LOG[j].file) == 0 && ERROR_LOG[i].line == ERROR_LOG[j].line)
      {
        // We found a duplicate. Compare clocks.
        if (_FPC_OPERATION_CLOCK_[j] > highest_clock)
        {
          highest_clock = _FPC_OPERATION_CLOCK_[j];
          highest_clock_index = j;
        }
        // Mark this entry as processed to avoid re-evaluation later
        printed_indices[j] = 1;
      }
    }

    // Print the entry with the highest clock for this unique file
    printf("File: %s, Line: %d, Clock: %d\n", ERROR_LOG[highest_clock_index].file, ERROR_LOG[highest_clock_index].line, highest_clock);

    if (!first)
      fprintf(fp, ",\n");
    first = 0;

    fprintf(fp, "  {\n");
    fprintf(fp, "    \"file\": \"%s\",\n", ERROR_LOG[highest_clock_index].file);
    fprintf(fp, "    \"line\": %d,\n", ERROR_LOG[highest_clock_index].line);
    fprintf(fp, "    \"error\": %.17e,\n", _FPC_ERRORS_[highest_clock_index]);
    fprintf(fp, "    \"relative_error\": %.17e\n", _FPC_RELATIVE_ERRORS_[highest_clock_index]);
    fprintf(fp, "  }");
    entries_written++;

    // Mark the chosen entry as printed
    printed_indices[highest_clock_index] = 1;
  }

  fprintf(fp, "\n]\n");
  fclose(fp);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  FILE *fp_read = fopen(errorFileName, "r");
  if (fp_read)
  {
    char buffer[1024];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp_read)) > 0)
    {
      fwrite(buffer, 1, n, stdout);
    }
    fclose(fp_read);
  }
  else
  {
    printf("#FPCHECKER: Could not open %s for reading\n", errorFileName);
  }
#endif

  printf("#FPCHECKER: Successfully wrote %d final sink errors to %s\n", entries_written, errorFileName);
}

void _FPC_PRINT_LOCATIONS_()
{
#ifndef FPC_QUIET
  printf("#FPCHECKER: Finalizing and writing traces...\n");
#endif
  _FPC_PRINT_HASH_TABLE_(_FPC_HTABLE_);
  //_FPC_REMOVE_DUPLICATES_();
  _FPC_WRITE_AND_PRINT_TO_JSON_(); // *** Error Calculation *** //
}

/*----------------------------------------------------------------------------*/
/* Checking functions for events (FP32)                                       */
/*----------------------------------------------------------------------------*/

uint32_t _FPC_FP32_GET_EXPONENT(float x)
{
  uint32_t val;
  memcpy((void *)&val, (void *)&x, sizeof(val));
  val = val << 1;  // get rid of sign bit
  val = val >> 24; // get rid of the mantissa bits
  return val;
}

uint32_t _FPC_FP32_GET_MANTISSA(float x)
{
  uint32_t val;
  memcpy((void *)&val, (void *)&x, sizeof(val));
  val = val << 9; // get rid of sign bit and exponent
  val = val >> 9;
  return val;
}

int _FPC_FP32_IS_INF(float x)
{
  if (_FPC_FP32_GET_EXPONENT(x) == (uint32_t)(255) &&
      _FPC_FP32_GET_MANTISSA(x) == (uint32_t)(0))
    return 1;
  return 0;
}

int _FPC_FP32_IS_INFINITY_POS(float x)
{
  if (_FPC_FP32_IS_INF(x))
    if (x > 0)
      return 1;
  return 0;
}

int _FPC_FP32_IS_INFINITY_NEG(float x)
{
  if (_FPC_FP32_IS_INF(x))
    if (x < 0)
      return 1;
  return 0;
}

int _FPC_FP32_IS_NAN(float x)
{
  if (isnan(x))
    return 1;
  return 0;
}

int _FPC_FP32_IS_DIVISON_ZERO(float y, float z, int op)
{
  if (op == 3)
    if (y != 0)
      if (z == 0)
        return 1;

  return 0;
}

// Number of cancelled digits calculated as:
//    max{exponent(op1), exponent(op2)} - exponent(res)
// res = result
// A cancellation has happened if the number of canceled digits
// is greater than zero
int _FPC_FP32_IS_CANCELLATION(float x, float y, float z, int op)
{
  if (op == 0 || op == 1)
  {
    uint32_t e1 = _FPC_FP32_GET_EXPONENT(y);
    uint32_t e2 = _FPC_FP32_GET_EXPONENT(z);
    uint32_t re = _FPC_FP32_GET_EXPONENT(x);
    if ((FPC_MAX((int)e1, (int)e2) - (int)re) > 30)
      return 1;
  }

  return 0;
}

int _FPC_FP32_IS_COMPARISON(int op)
{
  if (op == 4)
    return 1;

  return 0;
}

int _FPC_FP32_IS_SUBNORMAL(float x)
{
  int ret = 0;
  uint32_t val = _FPC_FP32_GET_EXPONENT(x);
  if (x != 0.0 && x != -0.0)
  {
    if (val == 0)
      ret = 1;
  }
  return ret;
}

int _FPC_FP32_IS_LATENT_INFINITY(float x)
{
  int ret = 0;
  uint32_t val = _FPC_FP32_GET_EXPONENT(x);
  if (x != 0.0 && x != -0.0)
  {
    uint64_t maxVal = 256 - (uint64_t)(DANGER_ZONE_PERCENTAGE * 256.0);
    if (val >= maxVal)
      ret = 1;
  }
  return ret;
}

int _FPC_FP32_IS_LATENT_INFINITY_POS(float x)
{
  if (_FPC_FP32_IS_LATENT_INFINITY(x))
    if (x > 0)
      return 1;

  return 0;
}

int _FPC_FP32_IS_LATENT_INFINITY_NEG(float x)
{
  if (_FPC_FP32_IS_LATENT_INFINITY(x))
    if (x < 0)
      return 1;

  return 0;
}

int _FPC_FP32_IS_LATENT_SUBNORMAL(float x)
{
  int ret = 0;
  uint32_t val = _FPC_FP32_GET_EXPONENT(x);
  if (x != 0.0 && x != -0.0)
  {
    uint64_t minVal = (uint64_t)(DANGER_ZONE_PERCENTAGE * 256.0);
    if (val <= minVal)
      ret = 1;
  }
  return ret;
}

/*----------------------------------------------------------------------------*/
/* Checking functions for events (FP64)                                       */
/*----------------------------------------------------------------------------*/

uint64_t _FPC_FP64_GET_EXPONENT(double x)
{
  uint64_t val;
  memcpy((void *)&val, (void *)&x, sizeof(val));
  val = val << 1;  // get rid of sign bit
  val = val >> 53; // get rid of the mantissa bits
  return val;
}

uint64_t _FPC_FP64_GET_MANTISSA(double x)
{
  uint64_t val;
  memcpy((void *)&val, (void *)&x, sizeof(val));
  val = val << 12; // get rid of sign bit and exponent
  val = val >> 12;
  return val;
}

int _FPC_FP64_IS_INF(double x)
{
  if (_FPC_FP64_GET_EXPONENT(x) == (uint64_t)(2047) &&
      _FPC_FP64_GET_MANTISSA(x) == (uint64_t)(0))
    return 1;
  return 0;
}

int _FPC_FP64_IS_INFINITY_POS(double x)
{
  if (_FPC_FP64_IS_INF(x))
    if (x > 0)
      return 1;
  return 0;
}

int _FPC_FP64_IS_INFINITY_NEG(double x)
{
  if (_FPC_FP64_IS_INF(x))
    if (x < 0)
      return 1;
  return 0;
}

int _FPC_FP64_IS_NAN(double x)
{
  if (isnan(x))
    return 1;
  return 0;
}

int _FPC_FP64_IS_DIVISON_ZERO(double y, double z, int op)
{
  if (op == 3)
    if (y != 0)
      if (z == 0)
        return 1;

  return 0;
}

// Number of cancelled digits calculated as:
//    max{exponent(op1), exponent(op2)} - exponent(res)
// res = result
// A cancellation has happened if the number of canceled digits
// is greater than zero
// Threshold: 10^9 or 2^30, i.e., 9 decimal digits or 30 binary digits
int _FPC_FP64_IS_CANCELLATION(double x, double y, double z, int op)
{
  if (op == 0 || op == 1)
  {
    uint64_t e1 = _FPC_FP64_GET_EXPONENT(y);
    uint64_t e2 = _FPC_FP64_GET_EXPONENT(z);
    uint64_t re = _FPC_FP64_GET_EXPONENT(x);
    if ((FPC_MAX((int)e1, (int)e2) - (int)re) > 30)
    {
      return 1;
    }
  }

  return 0;
}

int _FPC_FP64_IS_COMPARISON(int op)
{
  if (op == 4)
    return 1;

  return 0;
}

int _FPC_FP64_IS_SUBNORMAL(double x)
{
  int ret = 0;
  uint64_t val = _FPC_FP64_GET_EXPONENT(x);
  if (x != 0.0 && x != -0.0)
  {
    if (val == 0)
      ret = 1;
  }
  return ret;
}

int _FPC_FP64_IS_LATENT_INFINITY(double x)
{
  int ret = 0;
  uint64_t val = _FPC_FP64_GET_EXPONENT(x);
  if (x != 0.0 && x != -0.0)
  {
    uint64_t maxVal = 2048 - (uint64_t)(DANGER_ZONE_PERCENTAGE * 2048.0);
    if (val >= maxVal)
      ret = 1;
  }
  return ret;
}

int _FPC_FP64_IS_LATENT_INFINITY_POS(double x)
{
  if (_FPC_FP64_IS_LATENT_INFINITY(x))
    if (x > 0)
      return 1;

  return 0;
}

int _FPC_FP64_IS_LATENT_INFINITY_NEG(double x)
{
  if (_FPC_FP64_IS_LATENT_INFINITY(x))
    if (x < 0)
      return 1;

  return 0;
}

int _FPC_FP64_IS_LATENT_SUBNORMAL(double x)
{
  int ret = 0;
  uint64_t val = _FPC_FP64_GET_EXPONENT(x);
  if (x != 0.0 && x != -0.0)
  {
    uint64_t minVal = (uint64_t)(DANGER_ZONE_PERCENTAGE * 2048.0);
    if (val <= minVal)
      ret = 1;
  }
  return ret;
}

/*----------------------------------------------------------------------------*/
/* Trap functions                                                             */
/*----------------------------------------------------------------------------*/

/**
 * Trap options and id
 * --------------------
 *  FPC_TRAP_INFINITY_POS     1
 *  FPC_TRAP_INFINITY_NEG     2
 *  FPC_TRAP_NAN              3
 *  FPC_TRAP_DIVISION_ZERO    4
 *  FPC_TRAP_CANCELLATION     5
 *  FPC_TRAP_COMPARISON       6
 *  FPC_TRAP_UNDERFLOW        7
 *  FPC_TRAP_LATENT_INF_POS   8
 *  FPC_TRAP_LATENT_INF_NEG   9
 *  FPC_TRAP_LATENT_UNDERFLOW 10
 *  FPC_TRAP_FILE
 *  FPC_TRAP_LINE
 **/

void _FPC_TRAP_HERE(const char *trap_name, int loc, char *file_name)
{
  printf("#FPCHECKER: Interrupting execution...\n");
  printf("#FPCHECKER: %s\n", trap_name);
  printf("#FPCHECKER: %s:%d\n", file_name, loc);
  fflush(stdout);

  if (getenv("FPC_PRINT_HOSTNAME"))
  {
    char host_name[256];
    host_name[0] = '\0';
    gethostname(host_name, 256);
    pid_t pid = getpid();
    printf("HOST: %s, PID: %d\n", host_name, pid);
  }

  if (getenv("FPC_TRAPS_HANG"))
  {
    sleep(3600);
  }
  else
  {
    raise(SIGABRT);
  }
}

int _FPC_STRING_ENDS_WITH(const char *str, const char *substr)
{
  int len_str = strlen(str);
  int len_substr = strlen(substr);
  if (len_str < len_substr)
    return 0;

  int index = len_str - len_substr;
  if (strcmp(&(str[index]), substr) == 0)
    return 1;

  return 0;
}

void _FPC_CHECK_AND_TRAP(_FPC_ITEM_T_ *item, int loc, char *file_name)
{
  int check = 0;
  if (getenv("FPC_TRAP_FILE") != NULL)
  {
    if (_FPC_STRING_ENDS_WITH(item->file_name, getenv("FPC_TRAP_FILE")))
    {
      check = 1;
    }
  }
  else
  {
    check = 1;
  }

  if (getenv("FPC_TRAP_LINE") != NULL)
  {
    uint64_t line = (uint64_t)atoi(getenv("FPC_TRAP_LINE"));
    if (item->line == line)
      check &= 1;
    else
      check &= 0;
  }
  else
  {
    check &= 1;
  }

  if (check)
  {
    if (getenv("FPC_TRAP_INFINITY_POS") != NULL && item->infinity_pos)
      _FPC_TRAP_HERE("infinity(+)", loc, file_name);
    if (getenv("FPC_TRAP_INFINITY_NEG") != NULL && item->infinity_neg)
      _FPC_TRAP_HERE("infinity(-)", loc, file_name);
    if (getenv("FPC_TRAP_NAN") != NULL && item->nan)
      _FPC_TRAP_HERE("nan", loc, file_name);
    if (getenv("FPC_TRAP_DIVISION_ZERO") != NULL && item->division_zero)
      _FPC_TRAP_HERE("division by zero", loc, file_name);
    if (getenv("FPC_TRAP_CANCELLATION") != NULL && item->cancellation)
      _FPC_TRAP_HERE("cancellation", loc, file_name);
    if (getenv("FPC_TRAP_COMPARISON") != NULL && item->comparison)
      _FPC_TRAP_HERE("comparison", loc, file_name);
    if (getenv("FPC_TRAP_UNDERFLOW") != NULL && item->underflow)
      _FPC_TRAP_HERE("underflow", loc, file_name);
    if (getenv("FPC_TRAP_LATENT_INF_POS") != NULL && item->latent_infinity_pos)
      _FPC_TRAP_HERE("latent infinity(+)", loc, file_name);
    if (getenv("FPC_TRAP_LATENT_INF_NEG") != NULL && item->latent_infinity_neg)
      _FPC_TRAP_HERE("latent infinity(-)", loc, file_name);
    if (getenv("FPC_TRAP_LATENT_UNDERFLOW") != NULL && item->latent_underflow)
      _FPC_TRAP_HERE("latent underflow", loc, file_name);
  }
}

/*----------------------------------------------------------------------------*/
/* Generic checking functions                                                 */
/*----------------------------------------------------------------------------*/

int _FPC_EVENT_OCURRED(_FPC_ITEM_T_ *item)
{
  return (
      item->infinity_pos ||
      item->infinity_neg ||
      item->nan ||
      item->division_zero ||
      item->cancellation ||
      item->comparison ||
      item->underflow ||
      item->latent_infinity_pos ||
      item->latent_infinity_neg ||
      item->latent_underflow);
}

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

// This code is deprecated and will be removed in the next release!
// ==================== Exponent Usage Histograms =====================
// #ifdef FPC_EXPONENT_USAGE
/*
void _FPC_FP32_CHECK_(
    float x, float y, float z, int loc, char *file_name, int op, int cond)
{
  if (!cond)
    return;

  _FPC_ITEM_T_ item;
  for (int i = 0; i < FPC_HISTOGRAM_LEN; ++i)
  {
    item.fp32_exponent_count[i] = 0;
    item.fp64_exponent_count[i] = 0;
  }

  // Set file name and line
  item.file_name = file_name;
  item.line = (uint64_t)loc;

  // Set histogram count
  item.fp32_exponent_count[(int)_FPC_FP32_GET_EXPONENT(x)] = (uint64_t)1;

#ifdef FPC_MULTI_THREADED
  pthread_mutex_lock(&fpc_lock);
#endif
  _FPC_HT_SET_(_FPC_HTABLE_, &item);
#ifdef FPC_MULTI_THREADED
  pthread_mutex_unlock(&fpc_lock);
#endif
}

void _FPC_FP64_CHECK_(
    double x, double y, double z, int loc, char *file_name, int op, int cond)
{
  if (!cond)
    return;

  _FPC_ITEM_T_ item;
  for (int i = 0; i < FPC_HISTOGRAM_LEN; ++i)
  {
    item.fp32_exponent_count[i] = 0;
    item.fp64_exponent_count[i] = 0;
  }

  // Set file name and line
  item.file_name = file_name;
  item.line = (uint64_t)loc;

  // Set histogram count
  item.fp64_exponent_count[(int)_FPC_FP64_GET_EXPONENT(x)] = (uint64_t)1;
#ifdef FPC_MULTI_THREADED
  pthread_mutex_lock(&fpc_lock);
#endif
  _FPC_HT_SET_(_FPC_HTABLE_, &item);
#ifdef FPC_MULTI_THREADED
  pthread_mutex_unlock(&fpc_lock);
#endif
}
// ====================================================
*/
// #else

/*----------------------------------------------------------------------------*/
/* Error Accumulation                                                        */
/*----------------------------------------------------------------------------*/

// Find entry by register name
int _FPC_FP32_FIND_BY_REGISTER_(const char *reg_name)
{
  for (int i = 0; i < _FPC_ENTRY_COUNT_; i++)
  {
    if (strcmp(_FPC_REGISTERS_[i], reg_name) == 0)
    {
      return i;
    }
  }

  return -1; // No register available in the table
}

// Find entry by address
int _FPC_FP32_FIND_BY_ADDRESS_(uintptr_t addr)
{
  for (int i = 0; i < _FPC_ENTRY_COUNT_; i++)
  {
    if (_FPC_ADDRESSES_[i] == addr)
    {
      return i;
    }
  }
  return -1;
}

/**
 * A STORE consumes a register and produces a value at a memory address.
 */

/*----------------------------------------------------------------------------*/
/* Store Function with Location Logging                             */
/*----------------------------------------------------------------------------*/

// *** Error Calculation *** //
// Instrumentation for STORE instructions
void _FPC_FP32_STORE_INST_(const char *reg, uintptr_t address, int loc, char *file_name)
{
#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_STORE_INST_:\n");
  printf("reg=%s, address=%lu\n", reg, address);
#endif

  double error = 0.0;
  double relative_error = 0.0;

  // Find if this register already has an error
  int reg_id = _FPC_FP32_FIND_BY_REGISTER_(reg);
  if (reg_id >= 0)
  {
    error = _FPC_ERRORS_[reg_id];
    relative_error = _FPC_RELATIVE_ERRORS_[reg_id];

    // Increment clock
    _FPC_CLOCK_++;
    _FPC_OPERATION_CLOCK_[reg_id] = _FPC_CLOCK_;
  }
  else
  {
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("\t Trying to STORE the result for this register: %s\n", reg);
    printf("\t But we don't have its error!\n");
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    exit(1);
    // return;
  }

  // Now update or insert based on the *address*
  int addr_id = _FPC_FP32_FIND_BY_ADDRESS_(address);
  if (addr_id >= 0)
  {
    // Address exists — update it
    _FPC_ERRORS_[addr_id] = error;
    _FPC_RELATIVE_ERRORS_[addr_id] = relative_error;
    strncpy(_FPC_REGISTERS_[addr_id], reg, MAX_NAME_SIZE - 1);
    _FPC_REGISTERS_[addr_id][MAX_NAME_SIZE - 1] = '\0';

    // Add location logging for existing address
    strncpy(ERROR_LOG[addr_id].file, file_name, MAX_NAME_SIZE - 1);
    ERROR_LOG[addr_id].file[MAX_NAME_SIZE - 1] = '\0';
    ERROR_LOG[addr_id].line = loc;

    // Increment clock
    _FPC_CLOCK_++;
    _FPC_OPERATION_CLOCK_[addr_id] = _FPC_CLOCK_;
  }
  else if (_FPC_ENTRY_COUNT_ < MAX_ERROR_ENTRIES)
  {
    // Address does not exist — insert new entry
    _FPC_ADDRESSES_[_FPC_ENTRY_COUNT_] = address;
    strncpy(_FPC_REGISTERS_[_FPC_ENTRY_COUNT_], reg, MAX_NAME_SIZE - 1);
    _FPC_REGISTERS_[_FPC_ENTRY_COUNT_][MAX_NAME_SIZE - 1] = '\0';
    _FPC_ERRORS_[_FPC_ENTRY_COUNT_] = error;
    _FPC_RELATIVE_ERRORS_[_FPC_ENTRY_COUNT_] = relative_error;

    // CRITICAL: Add location logging for new entry
    strncpy(ERROR_LOG[_FPC_ENTRY_COUNT_].file, file_name, MAX_NAME_SIZE - 1);
    ERROR_LOG[_FPC_ENTRY_COUNT_].file[MAX_NAME_SIZE - 1] = '\0';
    ERROR_LOG[_FPC_ENTRY_COUNT_].line = loc;

    _FPC_ENTRY_COUNT_++;

    // Increment clock
    _FPC_CLOCK_++;
    _FPC_OPERATION_CLOCK_[_FPC_ENTRY_COUNT_] = _FPC_CLOCK_;
  }
  else
  {
    // This should never happen
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("\t Out of memory!");
    printf("\t Cannot handle this address: %lu\n", address);
    printf("\t addr_id: %d\n", addr_id);
    printf("\t _FPC_ENTRY_COUNT_: %d\n", _FPC_ENTRY_COUNT_);
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    exit(-1);
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  //  ============== Print Tables ==============
  // int max_lines = 20;
  printf("Address    |  Register Name          |  Error Value         |  Relative Error     | Operation Clock  | Line\n");
  printf("-----------|-------------------------|----------------------|---------------------|------------------|------\n");
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    printf("%-10lu | %-23s | %-12.17e | %-14.17e | %d                | %d\n",
           (unsigned long)_FPC_ADDRESSES_[i],
           _FPC_REGISTERS_[i],
           _FPC_ERRORS_[i],
           _FPC_RELATIVE_ERRORS_[i],
           _FPC_OPERATION_CLOCK_[i],
           ERROR_LOG[i].line);

    // print only first max_lines entries
    // if (i == max_lines)
    //  break;
  }
// ============================================
#endif
}

// *** Error Calculation *** //
// Instrumentation for LOAD instructions
void _FPC_FP32_LOAD_INST_(const char *load_reg, uintptr_t address)
{
#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_LOAD_INST_:\n");
  printf("reg=%s, address=%lu\n", load_reg, address);
#endif

  // Mark address as used (this address is being read from)
  _FPC_USED_ADDR_(address);

  // Find if the register already exists
  int reg_id = _FPC_FP32_FIND_BY_REGISTER_(load_reg);

  // Find what's at this memory address
  int addr_id = _FPC_FP32_FIND_BY_ADDRESS_(address);

  // CRITICAL: Only process if different entries (addr_id != reg_id)
  if (addr_id >= 0 && addr_id != reg_id)
  {
    double error = _FPC_ERRORS_[addr_id];
    double relative_error = _FPC_RELATIVE_ERRORS_[addr_id];

    // Increment clock
    _FPC_CLOCK_++;
    _FPC_OPERATION_CLOCK_[reg_id] = _FPC_CLOCK_;

    if (reg_id >= 0)
    {
      // Update existing register
      uintptr_t old_addr = _FPC_ADDRESSES_[reg_id];
      double old_error = _FPC_ERRORS_[reg_id];

      _FPC_ADDRESSES_[reg_id] = address;
      _FPC_ERRORS_[reg_id] = error;
      _FPC_RELATIVE_ERRORS_[reg_id] = relative_error;

      // Increment clock
      _FPC_CLOCK_++;
      _FPC_OPERATION_CLOCK_[reg_id] = _FPC_CLOCK_;
    }
    else if (_FPC_ENTRY_COUNT_ < MAX_ERROR_ENTRIES)
    {
      // Create new register entry
      _FPC_ADDRESSES_[_FPC_ENTRY_COUNT_] = address;
      strncpy(_FPC_REGISTERS_[_FPC_ENTRY_COUNT_], load_reg, MAX_NAME_SIZE - 1);
      _FPC_REGISTERS_[_FPC_ENTRY_COUNT_][MAX_NAME_SIZE - 1] = '\0';
      _FPC_ERRORS_[_FPC_ENTRY_COUNT_] = error;
      _FPC_RELATIVE_ERRORS_[_FPC_ENTRY_COUNT_] = relative_error;

      // No location info for LOAD-created entries
      ERROR_LOG[_FPC_ENTRY_COUNT_].file[0] = '\0';
      ERROR_LOG[_FPC_ENTRY_COUNT_].line = 0;

      _FPC_ENTRY_COUNT_++;

      // Increment clock
      _FPC_CLOCK_++;
      _FPC_OPERATION_CLOCK_[_FPC_ENTRY_COUNT_] = _FPC_CLOCK_;
    }
  }
  else if (addr_id >= 0 && addr_id == reg_id)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("LOAD: Same entry (addr_id=%d == reg_id=%d) - no action needed\n", addr_id, reg_id);
#endif
  }
  else if (addr_id < 0)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("LOAD: No data found at address %lu\n", address);
#endif

    // Create register with zero error if needed
    if (reg_id >= 0)
    {
      _FPC_ADDRESSES_[reg_id] = address;
      _FPC_ERRORS_[reg_id] = 0.0;
    }
    else if (_FPC_ENTRY_COUNT_ < MAX_ERROR_ENTRIES)
    {
      _FPC_ADDRESSES_[_FPC_ENTRY_COUNT_] = address;
      strncpy(_FPC_REGISTERS_[_FPC_ENTRY_COUNT_], load_reg, MAX_NAME_SIZE - 1);
      _FPC_REGISTERS_[_FPC_ENTRY_COUNT_][MAX_NAME_SIZE - 1] = '\0';
      _FPC_ERRORS_[_FPC_ENTRY_COUNT_] = 0.0;

      ERROR_LOG[_FPC_ENTRY_COUNT_].file[0] = '\0';
      ERROR_LOG[_FPC_ENTRY_COUNT_].line = 0;

      _FPC_ENTRY_COUNT_++;
    }
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  //  ============== Print Tables ==============
  // int max_lines = 20;
  printf("Address    |  Register Name          |  Error Value         |  Relative Error     | Operation Clock  | Line\n");
  printf("-----------|-------------------------|----------------------|---------------------|------------------|------\n");
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    printf("%-10lu | %-23s | %-12.17e | %-14.17e | %d                | %d\n",
           (unsigned long)_FPC_ADDRESSES_[i],
           _FPC_REGISTERS_[i],
           _FPC_ERRORS_[i],
           _FPC_RELATIVE_ERRORS_[i],
           _FPC_OPERATION_CLOCK_[i],
           ERROR_LOG[i].line);

    // print only first max_lines entries
    // if (i == max_lines)
    //  break;
  }
// ============================================
#endif
}

// *** Error Calculation *** //
// Finds error for a given register
double _FPC_FP32_FIND_ERROR_(const char *reg_name)
{
  if (!reg_name || strlen(reg_name) == 0)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("#FPCHECKER-FIND: No register name provided → returning 0.0\n");
#endif
    return 0.0;
  }

  int id = _FPC_FP32_FIND_BY_REGISTER_(reg_name);
  if (id >= 0)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("#FPCHECKER-FIND: Found error for [%s || %lu] = %.17e\n",
           reg_name, _FPC_ADDRESSES_[id], _FPC_ERRORS_[id]);
#endif
    return _FPC_ERRORS_[id];
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("#FPCHECKER-FIND: No error found for [%s] -> returning 0.0\n", reg_name);
#endif
  return 0.0;
}

// *** Error Calculation *** //
// Looks for the register names, and updates entry values with errors
void _FPC_FP32_STORE_ERROR_(const char *reg_name, double error, double relative_error)
{
  if (!reg_name || strlen(reg_name) == 0)
  {
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("#FPCHECKER-STORE_ERROR: No register name provided\n");
#endif
    return;
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("#FPCHECKER-STORE_ERROR: Storing error %.17e for register %s\n", error, reg_name);
#endif

  int id = _FPC_FP32_FIND_BY_REGISTER_(reg_name);
  if (id >= 0)
  {
    // Update existing register entry - DON'T CREATE DUPLICATE
    double old_error = _FPC_ERRORS_[id];
    _FPC_ERRORS_[id] = error;
    _FPC_RELATIVE_ERRORS_[id] = relative_error;

    // Increment clock
    _FPC_CLOCK_++;
    _FPC_OPERATION_CLOCK_[id] = _FPC_CLOCK_;
#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("#FPCHECKER-STORE_ERROR: UPDATED existing register [%s] error: %.17e -> %.17e\n",
           reg_name, old_error, error);
#endif
    return;
  }

  // Register doesn't exist - create new entry
  if (_FPC_ENTRY_COUNT_ < MAX_ERROR_ENTRIES)
  {
    _FPC_ADDRESSES_[_FPC_ENTRY_COUNT_] = 0; // No address initially
    strncpy(_FPC_REGISTERS_[_FPC_ENTRY_COUNT_], reg_name, MAX_NAME_SIZE - 1);
    _FPC_REGISTERS_[_FPC_ENTRY_COUNT_][MAX_NAME_SIZE - 1] = '\0';
    _FPC_ERRORS_[_FPC_ENTRY_COUNT_] = error;
    _FPC_RELATIVE_ERRORS_[_FPC_ENTRY_COUNT_] = relative_error;

    // Initialize location (will be set by _FPC_LOG_LOCATION_)
    ERROR_LOG[_FPC_ENTRY_COUNT_].file[0] = '\0';
    ERROR_LOG[_FPC_ENTRY_COUNT_].line = 0;

#ifdef FPC_DEBUG_ERROR_ANALYSIS
    printf("#FPCHECKER-STORE_ERROR: CREATED new register [%s || 0] = %.17e at index %d\n",
           reg_name, error, _FPC_ENTRY_COUNT_);
#endif
    _FPC_ENTRY_COUNT_++;

    // Increment clock
    _FPC_CLOCK_++;
    _FPC_OPERATION_CLOCK_[_FPC_ENTRY_COUNT_] = _FPC_CLOCK_;
  }
}

// *** Error Calculation *** //
void _FPC_FP32_BRANCH_(const char *basic_block_name)
{
  // printf("BRANCH: BasicBlock %s\n", basic_block_name);
  strncpy(_FPC_LAST_BASIC_BLOCK_, basic_block_name, _FPC_BB_NAME_SIZE_ - 1);
  _FPC_LAST_BASIC_BLOCK_[_FPC_BB_NAME_SIZE_ - 1] = '\0';
}

// *** Error Calculation *** //
// This function is called for PHI nodes in SSA form
// It is used to log the values that are being merged
void _FPC_FP32_PHI_(const char *phi_values)
{
  // printf("PHI: Values %s\n", phi_values);
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
      // printf("\t PHI operand: %s\n", token);
      char *pipe_pos = strchr(token, '|');
      if (pipe_pos)
      {
        size_t first_len = pipe_pos - token;
        char first_substr[_FPC_BB_NAME_SIZE_];
        strncpy(first_substr, token, first_len);
        first_substr[first_len] = '\0';
        // printf("\t First substring is: %s\n", first_substr);
        if (pipe_pos)
        {
          if (strcmp(pipe_pos + 1, _FPC_LAST_BASIC_BLOCK_) == 0)
          {
            // Mark register of the original calculation as used
            _FPC_USED_REG_(first_substr);

            int id = _FPC_FP32_FIND_BY_REGISTER_(first_substr);
            if (id >= 0)
            {
              double old_error = _FPC_ERRORS_[id];
              double old_relative_error = _FPC_RELATIVE_ERRORS_[id];
              _FPC_FP32_STORE_ERROR_(register_name, old_error, old_relative_error);
            }
          }
        }
      }
      token = strtok_r(NULL, ";", &saveptr);
    }
  }
}

void _FPC_FP32_CHECK_(
    float x, float y, float z, int loc, char *file_name, int op, int cond)
{
  if (!cond)
    return;

#ifdef FPC_FAST_CHECKING
  // Check for NaN, infinity, or subnormals
  uint64_t exponent = _FPC_FP32_GET_EXPONENT(x);
  if (exponent != (uint64_t)(255))
  {
    if ((exponent != 0) || (x == 0.0 || x == -0.0))
    {
      return;
    }
  }
#endif

  _FPC_ITEM_T_ item;
  // Set file name and line
  item.file_name = file_name;
  item.line = (uint64_t)loc;

  // Set events
  item.infinity_pos = (uint64_t)_FPC_FP32_IS_INFINITY_POS(x);
  item.infinity_neg = (uint64_t)_FPC_FP32_IS_INFINITY_NEG(x);
  item.nan = (uint64_t)_FPC_FP32_IS_NAN(x);
  item.division_zero = (uint64_t)_FPC_FP32_IS_DIVISON_ZERO(y, z, op);
  item.cancellation = (uint64_t)_FPC_FP32_IS_CANCELLATION(x, y, z, op);
  item.comparison = (uint64_t)_FPC_FP32_IS_COMPARISON(op);
  item.underflow = (uint64_t)_FPC_FP32_IS_SUBNORMAL(x);
  item.latent_infinity_pos = (uint64_t)_FPC_FP32_IS_LATENT_INFINITY_POS(x);
  item.latent_infinity_neg = (uint64_t)_FPC_FP32_IS_LATENT_INFINITY_NEG(x);
  item.latent_underflow = (uint64_t)_FPC_FP32_IS_LATENT_SUBNORMAL(x);

  if (getenv("FPC_EXPONENT_USAGE") != NULL)
  {
    // Set exponent usage to zero
    for (int i = 0; i < FPC_HISTOGRAM_LEN; ++i)
    {
      item.fp32_exponent_count[i] = 0;
      item.fp64_exponent_count[i] = 0;
    }
    // Set exponent counts
    item.fp32_exponent_count[(int)_FPC_FP32_GET_EXPONENT(x)] = (uint64_t)1;
  }

  // If FPC_EXPONENT_USAGE is not defined (default), we only save items in the table
  // if an event ocurred. If FPC_EXPONENT_USAGE is defined, we save all items
  // (since we want to profile all instructions).
  if (getenv("FPC_EXPONENT_USAGE") == NULL)
  {
    if (!_FPC_EVENT_OCURRED(&item))
      return;
  }

#ifdef FPC_MULTI_THREADED
  pthread_mutex_lock(&fpc_lock);
#endif
  _FPC_HT_SET_(_FPC_HTABLE_, &item);
#ifdef FPC_MULTI_THREADED
  pthread_mutex_unlock(&fpc_lock);
#endif
  _FPC_CHECK_AND_TRAP(&item, loc, file_name);
}

void _FPC_FP64_CHECK_(
    double x, double y, double z, double error_y, double error_z, int loc, char *file_name, int op, int cond)
{
  if (!cond)
    return;

#ifdef FPC_FAST_CHECKING
  // Check for NaN, infinity, or subnormals
  uint64_t exponent = _FPC_FP64_GET_EXPONENT(x);
  if (exponent != (uint64_t)(2047))
  {
    if ((exponent != 0) || (x == 0.0 || x == -0.0))
    {
      return;
    }
  }
#endif

  _FPC_ITEM_T_ item;
  // Set file name and line
  item.file_name = file_name;
  item.line = (uint64_t)loc;

  // Set events
  item.infinity_pos = (uint64_t)_FPC_FP64_IS_INFINITY_POS(x);
  item.infinity_neg = (uint64_t)_FPC_FP64_IS_INFINITY_NEG(x);
  item.nan = (uint64_t)_FPC_FP64_IS_NAN(x);
  item.division_zero = (uint64_t)_FPC_FP64_IS_DIVISON_ZERO(y, z, op);
  item.cancellation = (uint64_t)_FPC_FP64_IS_CANCELLATION(x, y, z, op);
  item.comparison = (uint64_t)_FPC_FP64_IS_COMPARISON(op);
  item.underflow = (uint64_t)_FPC_FP64_IS_SUBNORMAL(x);
  item.latent_infinity_pos = (uint64_t)_FPC_FP64_IS_LATENT_INFINITY_POS(x);
  item.latent_infinity_neg = (uint64_t)_FPC_FP64_IS_LATENT_INFINITY_NEG(x);
  item.latent_underflow = (uint64_t)_FPC_FP64_IS_LATENT_SUBNORMAL(x);

  if (getenv("FPC_EXPONENT_USAGE") != NULL)
  {
    // Set exponent usage to zero
    for (int i = 0; i < FPC_HISTOGRAM_LEN; ++i)
    {
      item.fp32_exponent_count[i] = 0;
      item.fp64_exponent_count[i] = 0;
    }
    // Set exponent counts
    item.fp64_exponent_count[(int)_FPC_FP64_GET_EXPONENT(x)] = (uint64_t)1;
  }

  // If FPC_EXPONENT_USAGE is not defined (default), we only save items in the table
  // if an event ocurred. If FPC_EXPONENT_USAGE is defined, we save all items
  // (since we want to profile all instructions).
  if (getenv("FPC_EXPONENT_USAGE") == NULL)
  {
    if (!_FPC_EVENT_OCURRED(&item))
      return;
  }

#ifdef FPC_MULTI_THREADED
  pthread_mutex_lock(&fpc_lock);
#endif
  _FPC_HT_SET_(_FPC_HTABLE_, &item);
#ifdef FPC_MULTI_THREADED
  pthread_mutex_unlock(&fpc_lock);
#endif
  _FPC_CHECK_AND_TRAP(&item, loc, file_name);
}

// #endif

/*----------------------------------------------------------------------------*/
/* Error Analysis.                                                            */
/*----------------------------------------------------------------------------*/

// *** Error Calculation *** //
// Calculates the error for a given operation
void _FPC_FP32_CALCULATE_ERROR_(
    float x, float y, float z, float w, int loc, char *file_name, int op, int cond,
    const char *result_name, const char *op1_name, const char *op2_name, const char *fma_name)
{
#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_CALCULATE_ERROR_\n");
  printf("op=%d, x=%.7e, y=%.7e, z=%.7e, w=%.7e, result_name=%s, op1_name=%s, op2_name=%s, fma_name=%s, cond=%d\n", op, x, y, z, w, result_name, op1_name, op2_name, fma_name, cond);
  printf("Line: %d, File Name: %s\n", loc, file_name);
#endif

  double err_y = _FPC_FP32_FIND_ERROR_(op1_name);
  double err_z = _FPC_FP32_FIND_ERROR_(op2_name);
  double err_w = _FPC_FP32_FIND_ERROR_(fma_name);

  double y_high = (double)y + err_y;
  double z_high = (double)z + err_z;
  double w_high = (double)w + err_w;

  double r_high = 0.0;
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
    r_high = fmod(y_high, z_high);
    break;
  case 6:
    r_high = fma(y_high, z_high, w_high);
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

  double r_low = (double)x;

  double err_result = r_high - r_low;

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("Error Result: %.17e\n", err_result);
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

  _FPC_FP32_STORE_ERROR_(result_name, err_result, rel_error);
  _FPC_LOG_LOCATION_(file_name, loc);

  // We mark registers or operands as used
  if (op1_name && strlen(op1_name) > 0)
  {
    _FPC_USED_REG_(op1_name);
  }
  if (op2_name && strlen(op2_name) > 0)
  {
    _FPC_USED_REG_(op2_name);
  }
  if (fma_name && strlen(fma_name) > 0)
  {
    _FPC_USED_REG_(fma_name);
  }

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  //  ============== Print Tables ==============
  // int max_lines = 20;
  printf("Address    |  Register Name          |  Error Value         |  Relative Error     | Operation Clock  | Line\n");
  printf("-----------|-------------------------|----------------------|---------------------|------------------|------\n");
  for (int i = 0; i < _FPC_ENTRY_COUNT_; ++i)
  {
    printf("%-10lu | %-23s | %-12.17e | %-14.17e | %d                | %d\n",
           (unsigned long)_FPC_ADDRESSES_[i],
           _FPC_REGISTERS_[i],
           _FPC_ERRORS_[i],
           _FPC_RELATIVE_ERRORS_[i],
           _FPC_OPERATION_CLOCK_[i],
           ERROR_LOG[i].line);

    // print only first max_lines entries
    // if (i == max_lines)
    //  break;
  }
// ============================================
#endif

  fflush(stdout);
}

/*----------------------------------------------------------------------------*/
/* Annotation Macros                                                          */
/*----------------------------------------------------------------------------*/

// #define FPC_INSTRUMENT_BLOCK __attribute__((annotate("_FPC_INSTRUMENT_BLOCK_"))) int _marker __attribute__((unused)) = 0;
// #define FPC_INSTRUMENT_FUNC __attribute__((annotate("_FPC_INSTRUMENT_FUNCTION_")))
// #define FPC_CALCULATE_ERROR __attribute__((annotate("_FPC_CALCULATE_ERROR_"))) // *** Error Calculation *** //
#include "FPC_Annotations.h"

#endif /* SRC_RUNTIME_CPU_H_ */