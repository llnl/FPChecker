#ifndef SRC_FPC_HASHTABLE_H_
#define SRC_FPC_HASHTABLE_H_

#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define ERROR_TABLE_SIZE 1000
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

/*----------------------------------------------------------------------------*/
/* Hash table item                                                            */
/*----------------------------------------------------------------------------*/

#define FPC_HISTOGRAM_LEN 3000

/** This structure defines different events and the location **/
typedef struct _FPC_ITEM_S_
{
  char *file_name;
  uint64_t line;
  uint64_t infinity_pos;
  uint64_t infinity_neg;
  uint64_t nan;
  uint64_t division_zero;
  uint64_t cancellation;
  uint64_t comparison;
  uint64_t underflow;
  uint64_t latent_infinity_pos;
  uint64_t latent_infinity_neg;
  uint64_t latent_underflow;
  /* Number of times an exponent ocurred */
  uint64_t fp32_exponent_count[FPC_HISTOGRAM_LEN];
  uint64_t fp64_exponent_count[FPC_HISTOGRAM_LEN];
  struct _FPC_ITEM_S_ *next;
} _FPC_ITEM_T_;

typedef struct _FPC_ERROR_ITEM_S_ {
  char *file_name;
  uint64_t line;
  double error;
  struct _FPC_ERROR_ITEM_S_ *next;
} _FPC_ERROR_ITEM_T_;

#define FPC_ERROR_HTABLE_SIZE 1000

typedef struct _FPC_ERROR_HTABLE_S {
  uint64_t size;
  struct _FPC_ERROR_ITEM_S_ **table;
} _FPC_ERROR_HTABLE_T_;

static _FPC_ERROR_HTABLE_T_ *_FPC_ERROR_HTABLE_ = NULL;


// typedef struct _FPC_ITEM_S_ _FPC_ITEM_T_;

/** Program name and input **/
extern int _FPC_PROG_INPUTS;
extern char **_FPC_PROG_ARGS;

/*----------------------------------------------------------------------------*/
/* Hash table type                                                            */
/*----------------------------------------------------------------------------*/

typedef struct _FPC_HTABLE_S
{
  uint64_t size;
  uint64_t n; // number of items
  struct _FPC_ITEM_S_ **table;
} _FPC_HTABLE_T;

// typedef struct _FPC_HTABLE_S _FPC_HTABLE_T;

/*----------------------------------------------------------------------------*/
/* Generating  file identifier: hostName+processID                            */
/*----------------------------------------------------------------------------*/
void _FPC_GET_EXECUTION_ID_(char *executionId)
{
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
}

/*----------------------------------------------------------------------------*/
/* Initialization                                                             */
/*----------------------------------------------------------------------------*/

_FPC_HTABLE_T *_FPC_HT_CREATE_(int64_t size)
{
  _FPC_HTABLE_T *hashtable = NULL;
  int64_t i;

  if (size < 1)
    return NULL;

  // Allocate the table itself
  if ((hashtable = (_FPC_HTABLE_T *)malloc(sizeof(_FPC_HTABLE_T))) == NULL)
  {
    printf("#FPCHECKER: hash table out of memory error!");
    exit(EXIT_FAILURE);
  }

  // Allocate pointers to the head nodes
  if ((hashtable->table =
           (struct _FPC_ITEM_S_ **)malloc((size_t)((int64_t)sizeof(_FPC_ITEM_T_ *) * size))) == NULL)
  {
    //(struct _FPC_ITEM_T_ **)malloc(sizeof(_FPC_ITEM_T_ *) * size)) == NULL) {
    printf("#FPCHECKER: hash table out of memory error!");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < size; i++)
  {
    hashtable->table[i] = NULL;
  }

  hashtable->size = (uint64_t)size;
  hashtable->n = 0;

  return hashtable;
}

/*----------------------------------------------------------------------------*/
/* Hash function                                                              */
/*----------------------------------------------------------------------------*/

int _FPC_HT_HASH_(_FPC_HTABLE_T *hashtable, _FPC_ITEM_T_ *val)
{
  uint64_t key = (uint64_t)(val->file_name);
  key += val->line;
  return (int)(key % hashtable->size);
}

/*----------------------------------------------------------------------------*/
/* Key-value pair creation                                                    */
/*----------------------------------------------------------------------------*/

_FPC_ITEM_T_ *_FPC_HT_NEWPAIR_(_FPC_ITEM_T_ *val)
{
  _FPC_ITEM_T_ *newpair = NULL;

  if ((newpair = (_FPC_ITEM_T_ *)malloc(sizeof(_FPC_ITEM_T_))) == NULL)
  {
    printf("#FPCHECKER: hash table out of memory error!");
    exit(EXIT_FAILURE);
  }

  newpair->file_name = val->file_name;
  newpair->line = val->line;
  newpair->infinity_pos = val->infinity_pos;
  newpair->infinity_neg = val->infinity_neg;
  newpair->nan = val->nan;
  newpair->division_zero = val->division_zero;
  newpair->cancellation = val->cancellation;
  newpair->comparison = val->comparison;
  newpair->underflow = val->underflow;
  newpair->latent_infinity_pos = val->latent_infinity_pos;
  newpair->latent_infinity_neg = val->latent_infinity_neg;
  newpair->latent_underflow = val->latent_underflow;

  /* Copy array of bins for histogram */
  memcpy(newpair->fp32_exponent_count, val->fp32_exponent_count, sizeof(uint64_t) * FPC_HISTOGRAM_LEN);
  memcpy(newpair->fp64_exponent_count, val->fp64_exponent_count, sizeof(uint64_t) * FPC_HISTOGRAM_LEN);

  newpair->next = NULL;

  return newpair;
}

/*----------------------------------------------------------------------------*/
/* Comparison                                                                 */
/*----------------------------------------------------------------------------*/

int _FPC_ITEMS_EQUAL_(_FPC_ITEM_T_ *x, _FPC_ITEM_T_ *y)
{
  if ((x->file_name == y->file_name) && (x->line == y->line))
    return 1;
  return 0;
}

/*----------------------------------------------------------------------------*/
/* Insert a key-value pair into a hash table                                  */
/*----------------------------------------------------------------------------*/

void _FPC_HT_SET_(_FPC_HTABLE_T *hashtable, _FPC_ITEM_T_ *newVal)
{
  if (hashtable == NULL)
    return;

  int bin = 0;
  _FPC_ITEM_T_ *newpair = NULL;
  _FPC_ITEM_T_ *next = NULL;
  _FPC_ITEM_T_ *last = NULL;

  bin = _FPC_HT_HASH_(hashtable, newVal);
  next = hashtable->table[bin];

  while (next != NULL && !_FPC_ITEMS_EQUAL_(newVal, next))
  {
    last = next;
    next = next->next;
  }

  // There's already a pair
  if (next != NULL && _FPC_ITEMS_EQUAL_(newVal, next))
  {
    // Increment values
    next->infinity_pos += newVal->infinity_pos;
    next->infinity_neg += newVal->infinity_neg;
    next->nan += newVal->nan;
    next->division_zero += newVal->division_zero;
    next->cancellation += newVal->cancellation;
    next->comparison += newVal->comparison;
    next->underflow += newVal->underflow;
    next->latent_infinity_pos += newVal->latent_infinity_pos;
    next->latent_infinity_neg += newVal->latent_infinity_neg;
    next->latent_underflow += newVal->latent_underflow;
    /* Update histogram */
    for (int i = 0; i < FPC_HISTOGRAM_LEN; ++i)
    {
      next->fp32_exponent_count[i] += newVal->fp32_exponent_count[i];
      next->fp64_exponent_count[i] += newVal->fp64_exponent_count[i];
    }
  }
  else
  { // Nope, could't find it
    newpair = _FPC_HT_NEWPAIR_(newVal);
    (hashtable->n)++;

    if (next == hashtable->table[bin])
    {
      // We're at the start of the linked list in this bin
      newpair->next = next;
      hashtable->table[bin] = newpair;
    }
    else if (next == NULL)
    {
      // We're at the end of the linked list in this bin
      last->next = newpair;
    }
    else
    {
      // We're in the middle of the list.
      newpair->next = next;
      last->next = newpair;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* Check if item is empty or not                                              */
/*----------------------------------------------------------------------------*/

int _FPC_ITEM_IS_EMPTY_(_FPC_ITEM_T_ *x)
{
  if (x->infinity_pos == 0 && x->infinity_neg == 0 && x->nan == 0 &&
      x->division_zero == 0 && x->cancellation == 0 && x->comparison == 0 &&
      x->underflow == 0 && x->latent_infinity_pos == 0 &&
      x->latent_infinity_neg == 0 && x->latent_underflow == 0)
    return 1;
  return 0;
}

/*----------------------------------------------------------------------------*/
/* Print hash table                                                           */
/*----------------------------------------------------------------------------*/

void _FPC_PRINT_HASH_TABLE_(_FPC_HTABLE_T *hashtable)
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
  char expUsageFileName[5000];
  fileName[0] = '\0';
  expUsageFileName[0] = '\0';
  strcpy(fileName, ".fpc_logs/fpc_");
  strcpy(expUsageFileName, ".fpc_logs/exponent_usage_");

  _FPC_GET_EXECUTION_ID_(executionId);
  strcat(executionId, ".json");

  strcat(fileName, executionId);
  strcat(expUsageFileName, executionId);

  // Get program name and input
  int str_size = 0;
  for (int i = 0; i < _FPC_PROG_INPUTS; ++i)
    str_size += strlen(_FPC_PROG_ARGS[i]) + 1;
  char *prog_input = (char *)malloc((sizeof(char) * str_size) + 1);
  prog_input[0] = '\0';
  for (int i = 0; i < _FPC_PROG_INPUTS; ++i)
  {
    strcat(prog_input, _FPC_PROG_ARGS[i]);
    strcat(prog_input, " ");
  }

  // Prepare to print table
  // uint64_t n = hashtable->n;
  uint64_t printed = 0;
  uint64_t printed_exp_usage = 0;

  FILE *fp;
  FILE *fpe;
  fp = fopen(fileName, "w");
  fprintf(fp, "[\n");
  if (getenv("FPC_EXPONENT_USAGE"))
  {
    fpe = fopen(expUsageFileName, "w");
    fprintf(fpe, "[\n");
  }

  for (int i = 0; (uint64_t)i < hashtable->size; ++i)
  {
    _FPC_ITEM_T_ *next;
    next = hashtable->table[i];

    while (next != NULL)
    {
      if (!_FPC_ITEM_IS_EMPTY_(next))
      {
        // Writing floating-point events data
        fprintf(fp, "  {\n");
        fprintf(fp, "\t\"input\": \"%s\",\n", prog_input);
        fprintf(fp, "\t\"file\": \"%s\",\n", next->file_name);
        fprintf(fp, "\t\"line\": %llu,\n", next->line);
        // Set events
        fprintf(fp, "\t\"infinity_pos\": %llu,\n", next->infinity_pos);
        fprintf(fp, "\t\"infinity_neg\": %llu,\n", next->infinity_neg);
        fprintf(fp, "\t\"nan\": %llu,\n", next->nan);
        fprintf(fp, "\t\"division_zero\": %llu,\n", next->division_zero);
        fprintf(fp, "\t\"cancellation\": %llu,\n", next->cancellation);
        fprintf(fp, "\t\"comparison\": %llu,\n", next->comparison);
        fprintf(fp, "\t\"underflow\": %llu,\n", next->underflow);
        fprintf(fp, "\t\"latent_infinity_pos\": %llu,\n", next->latent_infinity_pos);
        fprintf(fp, "\t\"latent_infinity_neg\": %llu,\n", next->latent_infinity_neg);
        fprintf(fp, "\t\"latent_underflow\": %llu\n", next->latent_underflow);
        fprintf(fp, "  },\n");
        printed++;
      }

      if (getenv("FPC_EXPONENT_USAGE"))
      {
        // Writing exponent usage data
        fprintf(fpe, "  {\n");
        fprintf(fpe, "\t\"input\": \"%s\",\n", prog_input);
        fprintf(fpe, "\t\"file\": \"%s\",\n", next->file_name);
        fprintf(fpe, "\t\"line\": %llu,\n", next->line);

        fprintf(fpe, "\t\"fp32\": {\n");
        int fp32_present = 0;
        for (int j = 0; j < FPC_HISTOGRAM_LEN; ++j)
        {
          if (next->fp32_exponent_count[j] != 0 && j != 0) // do not consider zero exponent
          {
            if (fp32_present)
              fprintf(fpe, ",\n");
            int e = j - 127; // remove bias 2^(k-1)-1, where k is # of bits
            fprintf(fpe, "\t\t\"%d\": %llu", e, next->fp32_exponent_count[j]);
            fp32_present = 1;
          }
        }
        fprintf(fpe, "\n\t},\n");

        fprintf(fpe, "\t\"fp64\": {\n");
        int fp64_present = 0;
        for (int j = 0; j < FPC_HISTOGRAM_LEN; ++j)
        {
          if (next->fp64_exponent_count[j] != 0 && j != 0) // do not consider zero exponent
          {
            if (fp64_present)
              fprintf(fpe, ",\n");
            int e = j - 1023; // remove bias 2^(k-1)-1, where k is # of bits
            fprintf(fpe, "\t\t\"%d\": %llu", e, next->fp64_exponent_count[j]);
            fp64_present = 1;
          }
        }
        fprintf(fpe, "\n\t}\n");
        fprintf(fpe, "  },\n");
        printed_exp_usage++;
      }

      next = next->next;
    }
  }

  if (printed)
  {
    // Move back 5 characters from the current position (the end of the stream)
    fseek(fp, -5, SEEK_CUR);
    // Write "  }\n" from there as the last item
    fprintf(fp, "  }\n");
  }

  fprintf(fp, "]\n");
  fclose(fp);

  if (getenv("FPC_EXPONENT_USAGE"))
  {
    if (printed_exp_usage)
    {
      // Move back 5 characters from the current position (the end of the stream)
      fseek(fpe, -5, SEEK_CUR);
      // Write "  }\n" from there as the last item
      fprintf(fpe, "  }\n");
    }

    fprintf(fpe, "]\n");
    fclose(fpe);
  }
}

_FPC_ERROR_HTABLE_T_ *_FPC_ERROR_HT_CREATE_(int64_t size) {
  _FPC_ERROR_HTABLE_T_ *hashtable = NULL;
  int64_t i;

  if (size < 1)
    return NULL;

  hashtable = (_FPC_ERROR_HTABLE_T_ *)malloc(sizeof(_FPC_ERROR_HTABLE_T_));
  hashtable->size = size;
  hashtable->table = (_FPC_ERROR_ITEM_T_ **)malloc(sizeof(_FPC_ERROR_ITEM_T_ *) * size);
  for (i = 0; i < size; i++)
    hashtable->table[i] = NULL;
  return hashtable;
}
int _FPC_ERROR_HT_HASH_(_FPC_ERROR_HTABLE_T_ *hashtable, _FPC_ERROR_ITEM_T_ *val) {
  uint64_t key = (uint64_t)(val->file_name);
  key += val->line;
  return (int)(key % hashtable->size);
}

void _FPC_ERROR_HT_SET_(_FPC_ERROR_HTABLE_T_ *hashtable, _FPC_ERROR_ITEM_T_ *newVal) {
  if (!hashtable) return;

  int bin = _FPC_ERROR_HT_HASH_(hashtable, newVal);
  _FPC_ERROR_ITEM_T_ *next = hashtable->table[bin];
  _FPC_ERROR_ITEM_T_ *last = NULL;

  // Look for same (file, line)
  while (next != NULL && !(next->file_name == newVal->file_name && next->line == newVal->line)) {
    last = next;
    next = next->next;
  }

  if (next != NULL) {
    // Overwrite error value (or use max, as you prefer)
    next->error = newVal->error;
  } else {
    _FPC_ERROR_ITEM_T_ *newpair = (_FPC_ERROR_ITEM_T_ *)malloc(sizeof(_FPC_ERROR_ITEM_T_));
    newpair->file_name = newVal->file_name;
    newpair->line = newVal->line;
    newpair->error = newVal->error;
    newpair->next = hashtable->table[bin];
    hashtable->table[bin] = newpair;
  }
}
void _FPC_PRINT_ERROR_TABLE_(_FPC_ERROR_HTABLE_T_ *hashtable) {
  // Create directory
  struct stat st;
  char dir_name[] = ".fpc_logs";
  if (stat(dir_name, &st) == -1)
    mkdir(dir_name, 0775);

  // Set filename
  char executionId[5000];
  char fileName[5000];
  fileName[0] = '\0';
  strcpy(fileName, ".fpc_logs/error_");
  _FPC_GET_EXECUTION_ID_(executionId);
  strcat(executionId, ".json");
  strcat(fileName, executionId);

  FILE *fp = fopen(fileName, "w");
  fprintf(fp, "[\n");

  int first = 1;
  for (int i = 0; i < hashtable->size; ++i) {
    _FPC_ERROR_ITEM_T_ *next = hashtable->table[i];
    while (next != NULL) {
      if (!first) fprintf(fp, ",\n");
      first = 0;
      fprintf(fp, "  {\n");
      fprintf(fp, "    \"file\": \"%s\",\n", next->file_name);
      fprintf(fp, "    \"line\": %llu,\n", next->line);
      fprintf(fp, "    \"error\": %.17e\n", next->error);
      fprintf(fp, "  }");
      next = next->next;
    }
  }
  fprintf(fp, "\n]\n");
  fclose(fp);
}

#endif /* SRC_FPC_HASHTABLE_H_ */
