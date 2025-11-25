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

// *** Error Calculation *** //
// Maximum number of error entries
/* #define MAX_ERROR_ENTRIES 100000
#define MAX_NAME_SIZE 500 */

// *** Error Calculation *** //
/* typedef struct
{
  char file[MAX_NAME_SIZE];
  int line;
} FPC_ERROR_LOG_ENTRY;

FPC_ERROR_LOG_ENTRY ERROR_LOG[MAX_ERROR_ENTRIES]; */

// *** Error Calculation *** //
/* uintptr_t _FPC_ADDRESSES_[MAX_ERROR_ENTRIES];           // Runtime addresses
char _FPC_REGISTERS_[MAX_ERROR_ENTRIES][MAX_NAME_SIZE]; // Runtime register names
double _FPC_ERRORS_[MAX_ERROR_ENTRIES];                 // Runtime (rounding) error values
double _FPC_RELATIVE_ERRORS_[MAX_ERROR_ENTRIES];        // Runtime relative error values
int _FPC_OPERATION_CLOCK_[MAX_ERROR_ENTRIES];           // Operation clock
int _FPC_CLOCK_ = 0;                                    // Counter for the order of operations (1, 2, 3, ...)
int _FPC_ENTRY_COUNT_ = 0;                              // Counter of the number of entries in the above arrays */

// Last basic block name
#define _FPC_BB_NAME_SIZE_ 512                   // max size of a basic block name - for example: "%bb_26"
char _FPC_LAST_BASIC_BLOCK_[_FPC_BB_NAME_SIZE_]; // Last basic block ID

/*----------------------------------------------------------------------------*/
/* Error Calculation Functions                                                */
/*----------------------------------------------------------------------------*/

// *** Error Calculation *** //
// This function copies line and file information to the error log
/* void _FPC_LOG_LOCATION_(const char *file, int line)
{
  if (_FPC_ENTRY_COUNT_ > 0)
  {
    strncpy(ERROR_LOG[_FPC_ENTRY_COUNT_ - 1].file, file, MAX_NAME_SIZE - 1);
    ERROR_LOG[_FPC_ENTRY_COUNT_ - 1].line = line;
  }
} */

/*----------------------------------------------------------------------------*/
/* Initialize                                                                 */
/*----------------------------------------------------------------------------*/

void _FPC_INIT_HASH_TABLE_()
{
#ifndef FPC_QUIET
  printf("#FPCHECKER: Initializing...\n");
#endif

  int64_t size = 1024;
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
  _FPC_PROG_INPUTS = 0;
  _FPC_LAST_BASIC_BLOCK_[0] = '\0';
  _FPC_INIT_HASH_TABLE_();
  _FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED();
}

void _FPC_INIT_ARGS_FPCHECKER(int argc, char **argv)
{
  _FPC_PROG_INPUTS = argc;
  _FPC_PROG_ARGS = argv;
  _FPC_LAST_BASIC_BLOCK_[0] = '\0';
  _FPC_INIT_HASH_TABLE_();
  _FPC_CHECK_IF_LINE_ERRORS_ARE_SAVED();
}

// *** Error Calculation *** //
// Prints the errors to a JSON file
/* void _FPC_WRITE_AND_PRINT_TO_JSON_()
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
} */

void _FPC_PRINT_LOCATIONS_()
{
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

// Find entry by register name
/* int _FPC_FP32_FIND_BY_REGISTER_(const char *reg_name)
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
} */

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

/**
 * A STORE consumes a register and produces a value at a memory address.
 */

/*------------------------------------------------------------------*/
/* Store Function with Location Logging                             */
/*------------------------------------------------------------------*/

// *** Error Calculation *** //
// Instrumentation for STORE instructions
void _FPC_FP32_STORE_INST_(const char *reg, uintptr_t address, int loc, char *file_name)
{
#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_STORE_INST_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_STORE_INST_:\n");
  printf("reg=%s, address=%lu\n", reg, address);
#endif

  double error = 0.0;
  double relative_error = 0.0;

  // Find if this register already has an error
  int found = _FPC_FIND_ERRORS_BY_REGISTER(_FPC_REGISTER_HT_, reg, &error, &relative_error);
  if (!found)
  {
    if (_FPC_WARNING_COUNT_ < MAX_WARNINGS)
    {
      _FPC_WARNING_COUNT_++;
      printf("\n");
      printf("#FPCHECKER: Trying to store a register's value (%s), but we don't have its error!!\n", reg);
    }
  }

  // Update table based on the address
  // If address exists, update it
  // If address does not exist, insert new entry
  _FPC_ADDRESS_HT_UPDATE_(_FPC_ADDRESS_HT_, address, error, relative_error, file_name, loc);

  // Log location info if line is in _FPC_LINES_TO_KEEP_
  FPC_APPEND_ERROR_LOG_ENTRY(loc, relative_error);

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  _FPC_HT_PRINT_TABLES_(_FPC_ADDRESS_HT_, _FPC_REGISTER_HT_);
#endif

#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Exiting _FPC_FP32_STORE_INST_..........\n");
#endif
}

// *** Error Calculation *** //
// Instrumentation for LOAD instructions
void _FPC_FP32_LOAD_INST_(const char *load_reg, uintptr_t address, int loc, char *file_name)
{
#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_LOAD_INST_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_LOAD_INST_:\n");
  printf("reg=%s, address=%lu\n", load_reg, address);
#endif

  double error = 0.0;
  double relative_error = 0.0;

  // Find what's at this memory address
  int found = _FPC_FIND_ERRORS_BY_ADDRESS(_FPC_ADDRESS_HT_, address, &error, &relative_error);
  if (found)
  {
    // Update register entry with this error
    _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, load_reg, error, relative_error, file_name, loc);

    // Log location info if line is in _FPC_LINES_TO_KEEP_
    FPC_APPEND_ERROR_LOG_ENTRY(loc, relative_error);
  }
  else
  {
    // No error found at this address, create register with zero error
    _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, load_reg, 0.0, 0.0, file_name, loc);
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

// *** Error Calculation *** //
// Finds error for a given register
/* double _FPC_FP32_FIND_ERROR_(const char *reg_name)
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
} */

// *** Error Calculation *** //
// Looks for the register names, and updates entry values with errors
/* void _FPC_FP32_STORE_ERROR_(const char *reg_name, double error, double relative_error)
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
} */

// *** Error Calculation *** //
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

// *** Error Calculation *** //
// This function is called for PHI nodes in SSA form
// It is used to log the values that are being merged
void _FPC_FP32_PHI_(const char *phi_values)
{
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
            double old_error = 0.0;
            double old_relative_error = 0.0;
            int found = _FPC_FIND_ERRORS_BY_REGISTER(_FPC_REGISTER_HT_, first_substr, &old_error, &old_relative_error);
            if (found)
            {
              _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, register_name, old_error, old_relative_error, "", 0);
            }
            else
            {
              /*               printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                            printf("\t PHI Node: Trying to get error for register: %s\n", first_substr);
                            printf("\t But we don't have its error!\n");
                            printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); */
              // We don't have its error - create with zero error
              _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, register_name, 0.0, 0.0, "", 0);
              // exit(1);
            }

            // --- Old method (using arrays) ---
            /*             int id = _FPC_FP32_FIND_BY_REGISTER_(first_substr);
                        if (id >= 0)
                        {
                          double old_error = _FPC_ERRORS_[id];
                          double old_relative_error = _FPC_RELATIVE_ERRORS_[id];
                          _FPC_FP32_STORE_ERROR_(register_name, old_error, old_relative_error);
                        } */
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

/*----------------------------------------------------------------------------*/
/* Error Analysis.                                                            */
/*----------------------------------------------------------------------------*/

// *** Error Calculation *** //
// Calculates the error for a given operation
void _FPC_FP32_CALCULATE_ERROR_(
    float x, float y, float z, float w, int loc, char *file_name, int op, int cond,
    const char *result_name, const char *op1_name, const char *op2_name, const char *fma_name)
{
#ifdef FPDC_DEBUG_CALLSTACK
  printf(".........Entering _FPC_FP32_CALCULATE_ERROR_..........\n");
#endif

#ifdef FPC_DEBUG_ERROR_ANALYSIS
  printf("_FPC_FP32_CALCULATE_ERROR_\n");
  printf("op=%d, x=%.7e, y=%.7e, z=%.7e, w=%.7e, result_name=%s, op1_name=%s, op2_name=%s, fma_name=%s, cond=%d\n", op, x, y, z, w, result_name, op1_name, op2_name, fma_name, cond);
  printf("Line: %d, File Name: %s\n", loc, file_name);
#endif

  /*   double err_y = _FPC_FP32_FIND_ERROR_(op1_name);
    double err_z = _FPC_FP32_FIND_ERROR_(op2_name);
    double err_w = _FPC_FP32_FIND_ERROR_(fma_name); */

  double err_y = 0.0;
  double err_z = 0.0;
  double err_w = 0.0;
  double _tmp_unused_ = 0.0;
  _FPC_FIND_ERRORS_BY_REGISTER(_FPC_REGISTER_HT_, op1_name, &err_y, &_tmp_unused_);
  _FPC_FIND_ERRORS_BY_REGISTER(_FPC_REGISTER_HT_, op2_name, &err_z, &_tmp_unused_);
  _FPC_FIND_ERRORS_BY_REGISTER(_FPC_REGISTER_HT_, fma_name, &err_w, &_tmp_unused_);

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

  // Update register error table
  _FPC_REGISTER_HT_UPDATE_(_FPC_REGISTER_HT_, result_name, err_result, rel_error, file_name, loc);

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
/* Annotation Macros                                                          */
/*----------------------------------------------------------------------------*/
#include "FPC_Annotations.h"

#endif /* SRC_RUNTIME_ERROR_H_ */