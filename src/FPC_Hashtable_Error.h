#ifndef SRC_FPC_HASHTABLE_H_
#define SRC_FPC_HASHTABLE_H_

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <assert.h>

/*----------------------------------------------------------------------------*/
/* Items                                                                      */
/*----------------------------------------------------------------------------*/
// Global clock
uintptr_t _FPC_CLOCK_ = 0;

typedef struct _FPC_ADDRESS_S_
{
  uintptr_t address_value;
  double error;
  double relative_error;
  uint64_t clock; // clock incremented when item is updated/created
  char *file_name;
  int line;
  struct _FPC_ADDRESS_S_ *next;
} _FPC_ADDRESS_T_;

typedef struct _FPC_REGISTER_S_
{
  char *register_name;
  double error;
  double relative_error;
  uint64_t clock; // clock incremented when item is updated/created
  char *file_name;
  int line;
  char *function_name;
  struct _FPC_REGISTER_S_ *next;
} _FPC_REGISTER_T_;

/** Program name and input **/
extern int _FPC_PROG_INPUTS;
extern char **_FPC_PROG_ARGS;

/*----------------------------------------------------------------------------*/
/* Hash tables                                                                */
/*----------------------------------------------------------------------------*/

typedef struct _FPC_ADDRESS_HTABLE_S
{
  uint64_t size;
  uint64_t n; // number of items
  struct _FPC_ADDRESS_S_ **table;
} _FPC_ADDRESS_HTABLE_T;

typedef struct _FPC_REGISTER_HTABLE_S
{
  uint64_t size;
  uint64_t n; // number of items
  struct _FPC_REGISTER_S_ **table;
} _FPC_REGISTER_HTABLE_T;

/*----------------------------------------------------------------------------*/
/* Initialization                                                             */
/*----------------------------------------------------------------------------*/

#define _FPC_HT_CREATE_(prefix, type_name, item_type)                                             \
  prefix##_HTABLE_T *prefix##_HT_CREATE_(int64_t size)                                            \
  {                                                                                               \
    prefix##_HTABLE_T *hashtable = NULL;                                                          \
    int64_t i;                                                                                    \
                                                                                                  \
    if (size < 1)                                                                                 \
      return NULL;                                                                                \
                                                                                                  \
    if ((hashtable = (prefix##_HTABLE_T *)malloc(sizeof(prefix##_HTABLE_T))) == NULL)             \
    {                                                                                             \
      printf("#FPCHECKER: hash table out of memory error!");                                      \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
                                                                                                  \
    if ((hashtable->table =                                                                       \
             (struct type_name **)malloc((size_t)((int64_t)sizeof(item_type *) * size))) == NULL) \
    {                                                                                             \
      printf("#FPCHECKER: hash table out of memory error!");                                      \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
                                                                                                  \
    for (i = 0; i < size; i++)                                                                    \
    {                                                                                             \
      hashtable->table[i] = NULL;                                                                 \
    }                                                                                             \
                                                                                                  \
    hashtable->size = (uint64_t)size;                                                             \
    hashtable->n = 0;                                                                             \
                                                                                                  \
    return hashtable;                                                                             \
  }

_FPC_HT_CREATE_(_FPC_ADDRESS, _FPC_ADDRESS_S_, _FPC_ADDRESS_T_)
_FPC_HT_CREATE_(_FPC_REGISTER, _FPC_REGISTER_S_, _FPC_REGISTER_T_)

/*----------------------------------------------------------------------------*/
/* Hash functions                                                             */
/*----------------------------------------------------------------------------*/

size_t _FPC_HT_HASH_ADDRESS_(_FPC_ADDRESS_HTABLE_T *hashtable, _FPC_ADDRESS_T_ *val)
{
  uint64_t key = (uint64_t)(val->address_value);
  return (int)(key % hashtable->size);
}

// DBJ2 algorithm for hashing a string (does not modify register_name)
size_t _FPC_HT_HASH_REGISTER_(_FPC_REGISTER_HTABLE_T *hashtable, _FPC_REGISTER_T_ *val)
{
  if (!hashtable || hashtable->size == 0 || !val || !val->register_name || !val->function_name)
    return 0;

  unsigned long hash = 5381;

  // Hash register_name and function_name without allocating
  const unsigned char *p = (const unsigned char *)val->register_name;
  int c;
  while ((c = *p++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  // separator
  hash = ((hash << 5) + hash) + ':';
  p = (const unsigned char *)val->function_name;
  while ((c = *p++))
    hash = ((hash << 5) + hash) + c;

  return (size_t)(hash % hashtable->size);
}

/*----------------------------------------------------------------------------*/
/* Key-value pair creation                                                    */
/*----------------------------------------------------------------------------*/

_FPC_ADDRESS_T_ *_FPC_ADDRESS_HT_NEWPAIR_(_FPC_ADDRESS_T_ *val)
{
  _FPC_ADDRESS_T_ *newpair = NULL;

  if ((newpair = (_FPC_ADDRESS_T_ *)malloc(sizeof(_FPC_ADDRESS_T_))) == NULL)
  {
    printf("#FPCHECKER: hash table out of memory error!");
    exit(EXIT_FAILURE);
  }

  newpair->address_value = val->address_value;
  newpair->error = val->error;
  newpair->relative_error = val->relative_error;
  newpair->clock = val->clock;
  newpair->file_name = (char *)malloc((strlen(val->file_name) + 1) * sizeof(char));
  newpair->file_name[0] = '\0';
  strcpy(newpair->file_name, val->file_name);
  newpair->line = val->line;
  newpair->next = NULL;

  return newpair;
}

_FPC_REGISTER_T_ *_FPC_REGISTER_HT_NEWPAIR_(_FPC_REGISTER_T_ *val)
{
  _FPC_REGISTER_T_ *newpair = NULL;

  if ((newpair = (_FPC_REGISTER_T_ *)malloc(sizeof(_FPC_REGISTER_T_))) == NULL)
  {
    printf("#FPCHECKER: hash table out of memory error!");
    exit(EXIT_FAILURE);
  }

  newpair->register_name = (char *)malloc((strlen(val->register_name) + 1) * sizeof(char));
  newpair->register_name[0] = '\0';
  strcpy(newpair->register_name, val->register_name);
  newpair->error = val->error;
  newpair->relative_error = val->relative_error;
  newpair->clock = val->clock;
  newpair->file_name = (char *)malloc((strlen(val->file_name) + 1) * sizeof(char));
  newpair->file_name[0] = '\0';
  strcpy(newpair->file_name, val->file_name);
  newpair->line = val->line;
  newpair->function_name = (char *)malloc((strlen(val->function_name) + 1) * sizeof(char));
  newpair->function_name[0] = '\0';
  strcpy(newpair->function_name, val->function_name);
  newpair->next = NULL;

  return newpair;
}

/*----------------------------------------------------------------------------*/
/* Comparison                                                                 */
/*----------------------------------------------------------------------------*/

static inline int _FPC_ADDRESS_EQUAL_(_FPC_ADDRESS_T_ *x, _FPC_ADDRESS_T_ *y)
{
  return x->address_value == y->address_value;
}

static inline int _FPC_REGISTER_EQUAL_(_FPC_REGISTER_T_ *x, _FPC_REGISTER_T_ *y)
{
  // The register and function names must match
  return (strcmp(x->register_name, y->register_name) == 0 && strcmp(x->function_name, y->function_name) == 0);
}

/*----------------------------------------------------------------------------*/
/* Insert a key-value pair into a hash table                                  */
/*----------------------------------------------------------------------------*/

/* Set for address items (no function_name field) */
void _FPC_ADDRESS_HT_SET_(_FPC_ADDRESS_HTABLE_T *hashtable, _FPC_ADDRESS_T_ *newVal)
{
  if (hashtable == NULL)
    return;

  size_t bin = 0;
  _FPC_ADDRESS_T_ *newpair = NULL;
  _FPC_ADDRESS_T_ *next = NULL;
  _FPC_ADDRESS_T_ *last = NULL;

  bin = _FPC_HT_HASH_ADDRESS_(hashtable, newVal);
  next = hashtable->table[bin];

  while (next != NULL && !_FPC_ADDRESS_EQUAL_(newVal, next))
  {
    last = next;
    next = next->next;
  }

  /* There's already a pair */
  if (next != NULL && _FPC_ADDRESS_EQUAL_(newVal, next))
  {
    next->error = newVal->error;
    next->relative_error = newVal->relative_error;
    next->clock = newVal->clock;
    next->file_name = (char *)realloc(next->file_name, (strlen(newVal->file_name) + 1) * sizeof(char));
    next->file_name[0] = '\0';
    strcpy(next->file_name, newVal->file_name);
    next->line = newVal->line;
  }
  else
  { /* Nope, couldn't find it */
    newpair = _FPC_ADDRESS_HT_NEWPAIR_(newVal);
    (hashtable->n)++;

    if (next == hashtable->table[bin])
    {
      /* We're at the start of the linked list in this bin */
      newpair->next = next;
      hashtable->table[bin] = newpair;
    }
    else if (next == NULL)
    {
      /* We're at the end of the linked list in this bin */
      last->next = newpair;
    }
    else
    {
      /* We're in the middle of the list. */
      newpair->next = next;
      last->next = newpair;
    }
  }
}

/* Set for register items (has function_name field) */
void _FPC_REGISTER_HT_SET_(_FPC_REGISTER_HTABLE_T *hashtable, _FPC_REGISTER_T_ *newVal)
{
  if (hashtable == NULL)
    return;

  size_t bin = 0;
  _FPC_REGISTER_T_ *newpair = NULL;
  _FPC_REGISTER_T_ *next = NULL;
  _FPC_REGISTER_T_ *last = NULL;

  bin = _FPC_HT_HASH_REGISTER_(hashtable, newVal);
  next = hashtable->table[bin];

  while (next != NULL && !_FPC_REGISTER_EQUAL_(newVal, next))
  {
    last = next;
    next = next->next;
  }

  /* There's already a pair */
  if (next != NULL && _FPC_REGISTER_EQUAL_(newVal, next))
  {
    next->error = newVal->error;
    next->relative_error = newVal->relative_error;
    next->clock = newVal->clock;
    next->file_name = (char *)realloc(next->file_name, (strlen(newVal->file_name) + 1) * sizeof(char));
    next->file_name[0] = '\0';
    strcpy(next->file_name, newVal->file_name);
    next->line = newVal->line;
    next->function_name = (char *)realloc(next->function_name, (strlen(newVal->function_name) + 1) * sizeof(char));
    next->function_name[0] = '\0';
    strcpy(next->function_name, newVal->function_name);
  }
  else
  { /* Nope, couldn't find it */
    newpair = _FPC_REGISTER_HT_NEWPAIR_(newVal);
    (hashtable->n)++;

    if (next == hashtable->table[bin])
    {
      /* We're at the start of the linked list in this bin */
      newpair->next = next;
      hashtable->table[bin] = newpair;
    }
    else if (next == NULL)
    {
      /* We're at the end of the linked list in this bin */
      last->next = newpair;
    }
    else
    {
      /* We're in the middle of the list. */
      newpair->next = next;
      last->next = newpair;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* Table updates                                                              */
/* (API used at runtime)                                                      */
/*----------------------------------------------------------------------------*/

void _FPC_ADDRESS_HT_UPDATE_(
    _FPC_ADDRESS_HTABLE_T *hashtable,
    uintptr_t address_value,
    double error,
    double relative_error,
    const char *file_name,
    int line)
{
  _FPC_ADDRESS_T_ temp;
  temp.address_value = address_value;
  temp.error = error;
  temp.relative_error = relative_error;
  temp.clock = ++_FPC_CLOCK_;
  temp.file_name = (char *)malloc((strlen(file_name) + 1) * sizeof(char));
  temp.file_name[0] = '\0';
  strcpy(temp.file_name, file_name);
  temp.line = line;

  _FPC_ADDRESS_HT_SET_(hashtable, &temp);
  free(temp.file_name);
}

void _FPC_REGISTER_HT_UPDATE_(
    _FPC_REGISTER_HTABLE_T *hashtable,
    const char *register_name,
    const char *function_name,
    double error,
    double relative_error,
    const char *file_name,
    int line)
{
  _FPC_REGISTER_T_ temp;
  temp.register_name = (char *)register_name;
  temp.error = error;
  temp.relative_error = relative_error;
  temp.clock = ++_FPC_CLOCK_;
  temp.file_name = (char *)malloc((strlen(file_name) + 1) * sizeof(char));
  temp.file_name[0] = '\0';
  strcpy(temp.file_name, file_name);
  temp.line = line;
  temp.function_name = (char *)malloc((strlen(function_name) + 1) * sizeof(char));
  temp.function_name[0] = '\0';
  strcpy(temp.function_name, function_name);

  _FPC_REGISTER_HT_SET_(hashtable, &temp);
  free(temp.file_name);
  free(temp.function_name);
}

/*----------------------------------------------------------------------------*/
/* Searching                                                                  */
/* (API used at runtime)                                                      */
/*----------------------------------------------------------------------------*/

// Return 1 if found, 0 otherwise
int _FPC_FIND_ERRORS_BY_ADDRESS(_FPC_ADDRESS_HTABLE_T *hashtable,
                                uintptr_t address_value,
                                double *error,
                                double *relative_error)
{
  size_t bin = 0;
  _FPC_ADDRESS_T_ temp;
  _FPC_ADDRESS_T_ *next = NULL;

  temp.address_value = address_value;

  bin = _FPC_HT_HASH_ADDRESS_(hashtable, &temp);
  next = hashtable->table[bin];

  while (next != NULL && !_FPC_ADDRESS_EQUAL_(&temp, next))
  {
    next = next->next;
  }

  if (next != NULL && _FPC_ADDRESS_EQUAL_(&temp, next))
  {
    *error = next->error;
    *relative_error = next->relative_error;
    return 1;
  }
  else
  {
    *error = 0.0;
    *relative_error = 0.0;
    return 0;
  }
  return 0;
}

// Return 1 if found, 0 otherwise
int _FPC_FIND_ERRORS_BY_REGISTER(_FPC_REGISTER_HTABLE_T *hashtable,
                                 const char *register_name,
                                 const char *function_name,
                                 double *error,
                                 double *relative_error)
{
  size_t bin = 0;
  _FPC_REGISTER_T_ temp;
  _FPC_REGISTER_T_ *next = NULL;

  temp.register_name = (char *)register_name;
  temp.function_name = (char *)function_name;

  bin = _FPC_HT_HASH_REGISTER_(hashtable, &temp);
  next = hashtable->table[bin];

  while (next != NULL && !_FPC_REGISTER_EQUAL_(&temp, next))
  {
    next = next->next;
  }

  if (next != NULL && _FPC_REGISTER_EQUAL_(&temp, next))
  {
    *error = next->error;
    *relative_error = next->relative_error;
    return 1;
  }
  else
  {
    *error = 0.0;
    *relative_error = 0.0;
    return 0;
  }
  return 0;
}

/*----------------------------------------------------------------------------*/
/* Memcpy                                                                     */
/* (API used at runtime)                                                      */
/*----------------------------------------------------------------------------*/

// Updates ranges of addresses coing from a memcpy/memmove instruction
void _FPC_ADDRESS_RANGE_UPDATE_(
    _FPC_ADDRESS_HTABLE_T *hashtable,
    uintptr_t address_dst,
    uintptr_t address_src,
    size_t size,
    const char *file_name,
    int line)
{
  // Create temp buffers to copy and hold the errors
  double *error_buffer = (double *)malloc(size * sizeof(double));
  double *relative_error_buffer = (double *)malloc(size * sizeof(double));

  for (size_t offset = 0; offset < size; offset++)
  {
    uintptr_t current_address = address_src + offset;
    double tmp_error = 0.0;
    double tmp_relative_error = 0.0;

    int found = _FPC_FIND_ERRORS_BY_ADDRESS(hashtable, current_address, &tmp_error, &tmp_relative_error);
    if (!found)
    {
      //printf("#FPCHECKER: Trying to update address %lu in memcpy/memmove, but we don't have its error!!\n",
      //       current_address);
    }
    error_buffer[offset] = tmp_error;
    relative_error_buffer[offset] = tmp_relative_error;
  }

  // Update the table with the copied errors
  for (size_t offset = 0; offset < size; offset++)
  {
    uintptr_t dst_address = address_dst + offset;
    _FPC_ADDRESS_HT_UPDATE_(hashtable, dst_address, error_buffer[offset], relative_error_buffer[offset], file_name, line);
  }

  /*   for (size_t offset = 0; offset < size; offset++)
    {
      uintptr_t current_address = address_src + offset;
      double error = 0.0;
      double relative_error = 0.0;

      // Find if this address already has an error
      int found = _FPC_FIND_ERRORS_BY_ADDRESS(hashtable, current_address, &error, &relative_error);
      if (!found)
      {
        printf("#FPCHECKER: Trying to update address %lu in memcpy/memmove, but we don't have its error!!\n",
               current_address);
      }

      // Update table based on the address
      // If address exists, update it
      // If address does not exist, insert new entry
      uintptr_t dst_address = address_dst + offset;
      _FPC_ADDRESS_HT_UPDATE_(hashtable, dst_address, error, relative_error, file_name, line);
    } */

  free(error_buffer);
  free(relative_error_buffer);
}

/*----------------------------------------------------------------------------*/
/* Print hash table                                                           */
/*----------------------------------------------------------------------------*/

/*
Prints the errors to a JSON file.
Example of JSON output:

[
  {
    "file": "/tmp/test_fp32_average/simple_sum.cpp",
    "line": 15,
    "error": 2.98023223876953125e-05,
    "relative_error": 9.93410706781915562e-07
  },
  {
    "file": "/tmp/test_fp32_average/simple_sum.cpp",
    "line": 19,
    "error": 2.98023223876953125e-07,
    "relative_error": 9.93410706781915562e-07
  }
]
*/

void _FPC_WRITE_AND_PRINT_TO_JSON_(_FPC_ADDRESS_HTABLE_T *address_hashtable, _FPC_REGISTER_HTABLE_T *register_hashtable)
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

  typedef struct
  {
    char *file;
    int line;
    double error;
    double relative_error;
    uint64_t clock;
  } ErrorEntry;

  // Set the table to be printed
  size_t max_entries = register_hashtable->n + address_hashtable->n;
  ErrorEntry *ERRORS_LOG = (ErrorEntry *)malloc(max_entries * sizeof(ErrorEntry));

  // Set ERRORS_LOG entries to NULL
  for (size_t i = 0; i < max_entries; i++)
  {
    ERRORS_LOG[i].file = NULL;
    ERRORS_LOG[i].line = 0;
    ERRORS_LOG[i].error = 0.0;
    ERRORS_LOG[i].relative_error = 0.0;
    ERRORS_LOG[i].clock = 0;
  }

  size_t currentEntry = 0;

  /* Iterate address hashtable entries */
  if (address_hashtable != NULL)
  {
    for (uint64_t i = 0; i < address_hashtable->size; ++i)
    {
      _FPC_ADDRESS_T_ *cur = address_hashtable->table[i];
      while (cur != NULL)
      {
        double err = cur->error;
        double rel_err = cur->relative_error;
        int line = cur->line;
        char *file = cur->file_name;
        uint64_t clock = cur->clock;

        int found = 0;
        for (size_t j = 0; j < max_entries; j++)
        {
          if (ERRORS_LOG[j].file != NULL) // found entry
          {
            // Check if file and line are the same
            if (strcmp(ERRORS_LOG[j].file, file) == 0 && ERRORS_LOG[j].line == line)
            {
              found = 1;
              // Check if clock is higher
              if (clock > ERRORS_LOG[j].clock)
              {
                ERRORS_LOG[j].error = err;
                ERRORS_LOG[j].relative_error = rel_err;
                ERRORS_LOG[j].clock = clock;
              }
              break;
            }
          }
        }

        // If entry not found, add new entry
        if (!found)
        {
          assert(currentEntry < max_entries);
          // Element not found in ERRORS_LOG, add new entry
          ERRORS_LOG[currentEntry].file = (char *)malloc((strlen(file) + 1) * sizeof(char));
          ERRORS_LOG[currentEntry].file[0] = '\0';
          strcpy(ERRORS_LOG[currentEntry].file, file);
          ERRORS_LOG[currentEntry].line = line;
          ERRORS_LOG[currentEntry].error = err;
          ERRORS_LOG[currentEntry].relative_error = rel_err;
          ERRORS_LOG[currentEntry].clock = clock;
          currentEntry++;
        }

        cur = cur->next;
      }
    }
  }

  /* Iterate register hashtable entries */
  if (register_hashtable != NULL)
  {
    for (uint64_t i = 0; i < register_hashtable->size; ++i)
    {
      _FPC_REGISTER_T_ *cur = register_hashtable->table[i];
      while (cur != NULL)
      {
        double err = cur->error;
        double rel_err = cur->relative_error;
        int line = cur->line;
        char *file = cur->file_name;
        uint64_t clock = cur->clock;

        int found = 0;
        for (size_t j = 0; j < max_entries; j++)
        {
          if (ERRORS_LOG[j].file != NULL) // found entry
          {
            // Check if file and line are the same
            if (strcmp(ERRORS_LOG[j].file, file) == 0 && ERRORS_LOG[j].line == line)
            {
              found = 1;
              // Check if clock is higher
              if (clock > ERRORS_LOG[j].clock)
              {
                ERRORS_LOG[j].error = err;
                ERRORS_LOG[j].relative_error = rel_err;
                ERRORS_LOG[j].clock = clock;
              }
              break;
            }
          }
        }

        // If entry not found, add new entry
        if (!found)
        {
          assert(currentEntry < max_entries);
          // Element not found in ERRORS_LOG, add new entry
          ERRORS_LOG[currentEntry].file = (char *)malloc((strlen(file) + 1) * sizeof(char));
          ERRORS_LOG[currentEntry].file[0] = '\0';
          strcpy(ERRORS_LOG[currentEntry].file, file);
          ERRORS_LOG[currentEntry].line = line;
          ERRORS_LOG[currentEntry].error = err;
          ERRORS_LOG[currentEntry].relative_error = rel_err;
          ERRORS_LOG[currentEntry].clock = clock;
          currentEntry++;
        }

        cur = cur->next;
      }
    }
  }

  // ----------- Write JSON -----------
  int entries_written = 0;
  fprintf(fp, "[\n");

  for (size_t i = 0; i < max_entries; i++)
  {
    if (ERRORS_LOG[i].file != NULL)
    {
      // Skip empty file names and if line number is zero
      if (ERRORS_LOG[i].file[0] == '\0' || ERRORS_LOG[i].line == 0)
        continue;

      fprintf(fp, "  {\n");
      fprintf(fp, "    \"file\": \"%s\",\n", ERRORS_LOG[i].file);
      fprintf(fp, "    \"line\": %d,\n", ERRORS_LOG[i].line);
      fprintf(fp, "    \"error\": %.17e,\n", ERRORS_LOG[i].error);
      fprintf(fp, "    \"relative_error\": %.17e\n", ERRORS_LOG[i].relative_error);
      fprintf(fp, "  },\n");
      entries_written++;
    }
  }

  // Removes the last comma and newline
  fseek(fp, -2, SEEK_END);
  // Write closing bracket
  fprintf(fp, "\n]\n");
  fclose(fp);
  // ----------- End JSON ----------

  printf("#FPCHECKER: Successfully wrote %d error entries.\n", entries_written);
}

// Function example output
//  ============== Print Tables  ==============
// printf("Address    |  Register Name          |  Function Name          |  Error Value         |  Relative Error     | Clock   | File Name          | Line  \n");
// printf("-----------|-------------------------|-------------------------|---------------------|---------|--------------------|-------\n");
// ...
// ============================================
void _FPC_HT_PRINT_TABLES_(
    _FPC_ADDRESS_HTABLE_T *address_table,
    _FPC_REGISTER_HTABLE_T *register_table)
{
  /* Column widths:
   Address:        18 chars (0x + 16 hex digits)
   Register Name:  25 chars (left-justified, truncated if longer)
  Function Name:  25 chars (left-justified, truncated if longer)
   Error Value:    16 chars (right-justified)
   Relative Error: 16 chars (right-justified)
   Clock:           8 chars (right-justified)
   File Name:      20 chars (left-justified, truncated if longer)
   Line:            5 chars (right-justified)
  */

  /* Header */
  printf("%-18s %-25s %-25s %16s %16s %8s %-20s %5s\n",
         "Address", "Register Name", "Function Name", "Error Value", "Relative Error", "Clock", "File Name", "Line");
  printf("%-18s %-25s %-25s %16s %16s %8s %-20s %5s\n",
         "------------------", "-------------------------", "-------------------------", "----------------", "----------------", "--------", "--------------------", "-------");

  /* Print address table entries */
  if (address_table != NULL)
  {
    for (uint64_t i = 0; i < address_table->size; ++i)
    {
      _FPC_ADDRESS_T_ *cur = address_table->table[i];
      while (cur != NULL)
      {
        // Address in hex, register column empty ("-")
        printf("0x%016llx %-25.25s %-25.25s %16.6g %16.6g %8llu %-20.20s %5d\n",
               (unsigned long long)cur->address_value,
               "-", // register name placeholder
               "-", // function name placeholder
               cur->error,
               cur->relative_error,
               (unsigned long long)cur->clock,
               (cur->file_name ? cur->file_name : "(null)"),
               cur->line);
        cur = cur->next;
      }
    }
  }

  /* Print register table entries */
  if (register_table != NULL)
  {
    for (uint64_t i = 0; i < register_table->size; ++i)
    {
      _FPC_REGISTER_T_ *cur = register_table->table[i];
      while (cur != NULL)
      {
        /* No address for register entries */
        printf("%-18s %-25.25s %-25.25s %16.6g %16.6g %8llu %-20.20s %5d\n",
               "-", /* address placeholder */
               (cur->register_name ? cur->register_name : "(null)"),
               (cur->function_name ? cur->function_name : "(null)"),
               cur->error,
               cur->relative_error,
               (unsigned long long)cur->clock,
               (cur->file_name ? cur->file_name : "(null)"),
               cur->line);

        cur = cur->next;
      }
    }
  }
}

#endif /* SRC_FPC_HASHTABLE_H_ */
