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
  uint64_t clock; // clock is incremented when item is updated/created
  struct _FPC_ADDRESS_S_ *next;
} _FPC_ADDRESS_T_;

typedef struct _FPC_REGISTER_S_
{
  char *register_name;
  double error;
  double relative_error;
  uint64_t clock; // clock is incremented when item is updated/created
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
  if (!hashtable || hashtable->size == 0 || !val || !val->register_name)
    return 0;

  unsigned long hash = 5381;
  const unsigned char *p = (const unsigned char *)val->register_name;
  int c;

  while ((c = *p++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

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
  newpair->next = NULL;

  return newpair;
}

/*----------------------------------------------------------------------------*/
/* Comparison                                                                 */
/*----------------------------------------------------------------------------*/

inline int _FPC_ADDRESS_EQUAL_(_FPC_ADDRESS_T_ *x, _FPC_ADDRESS_T_ *y)
{
  return x->address_value == y->address_value;
}

inline int _FPC_REGISTER_EQUAL_(_FPC_REGISTER_T_ *x, _FPC_REGISTER_T_ *y)
{
  return strcmp(x->register_name, y->register_name) == 0;
}

/*----------------------------------------------------------------------------*/
/* Insert a key-value pair into a hash table                                  */
/*----------------------------------------------------------------------------*/

#define _FPC_HT_SET_MACRO_(prefix, HTABLE_T, ITEM_T, HASHFN, EQUALFN, NEWPAIRFN) \
  void prefix##_HT_SET_(HTABLE_T *hashtable, ITEM_T *newVal)                     \
  {                                                                              \
    if (hashtable == NULL)                                                       \
      return;                                                                    \
                                                                                 \
    size_t bin = 0;                                                              \
    ITEM_T *newpair = NULL;                                                      \
    ITEM_T *next = NULL;                                                         \
    ITEM_T *last = NULL;                                                         \
                                                                                 \
    bin = HASHFN(hashtable, newVal);                                             \
    next = hashtable->table[bin];                                                \
                                                                                 \
    while (next != NULL && !EQUALFN(newVal, next))                               \
    {                                                                            \
      last = next;                                                               \
      next = next->next;                                                         \
    }                                                                            \
                                                                                 \
    /* There's already a pair */                                                 \
    if (next != NULL && EQUALFN(newVal, next))                                   \
    {                                                                            \
      next->error = newVal->error;                                               \
      next->relative_error = newVal->relative_error;                             \
      next->clock = newVal->clock;                                               \
    }                                                                            \
    else                                                                         \
    { /* Nope, couldn't find it */                                               \
      newpair = NEWPAIRFN(newVal);                                               \
      (hashtable->n)++;                                                          \
                                                                                 \
      if (next == hashtable->table[bin])                                         \
      {                                                                          \
        /* We're at the start of the linked list in this bin */                  \
        newpair->next = next;                                                    \
        hashtable->table[bin] = newpair;                                         \
      }                                                                          \
      else if (next == NULL)                                                     \
      {                                                                          \
        /* We're at the end of the linked list in this bin */                    \
        last->next = newpair;                                                    \
      }                                                                          \
      else                                                                       \
      {                                                                          \
        /* We're in the middle of the list. */                                   \
        newpair->next = next;                                                    \
        last->next = newpair;                                                    \
      }                                                                          \
    }                                                                            \
  }

_FPC_HT_SET_MACRO_(_FPC_ADDRESS, _FPC_ADDRESS_HTABLE_T, _FPC_ADDRESS_T_,
                   _FPC_HT_HASH_ADDRESS_, _FPC_ADDRESS_EQUAL_, _FPC_ADDRESS_HT_NEWPAIR_)

_FPC_HT_SET_MACRO_(_FPC_REGISTER, _FPC_REGISTER_HTABLE_T, _FPC_REGISTER_T_,
                   _FPC_HT_HASH_REGISTER_, _FPC_REGISTER_EQUAL_, _FPC_REGISTER_HT_NEWPAIR_)

/*----------------------------------------------------------------------------*/
/* Table updates                                                              */
/*----------------------------------------------------------------------------*/

void _FPC_ADDRESS_HT_UPDATE_(
    _FPC_ADDRESS_HTABLE_T *hashtable,
    uintptr_t address_value,
    double error,
    double relative_error)
{
  _FPC_ADDRESS_T_ temp;
  temp.address_value = address_value;
  temp.error = error;
  temp.relative_error = relative_error;
  temp.clock = ++_FPC_CLOCK_;

  _FPC_ADDRESS_HT_SET_(hashtable, &temp);
}

void _FPC_REGISTER_HT_UPDATE_(
    _FPC_REGISTER_HTABLE_T *hashtable,
    const char *register_name,
    double error,
    double relative_error)
{
  _FPC_REGISTER_T_ temp;
  temp.register_name = (char *)register_name;
  temp.error = error;
  temp.relative_error = relative_error;
  temp.clock = ++_FPC_CLOCK_;

  _FPC_REGISTER_HT_SET_(hashtable, &temp);
}

/*----------------------------------------------------------------------------*/
/* Searching                                                                  */
/*----------------------------------------------------------------------------*/

void _FPC_FIND_ERRORS_BY_ADDRESS(_FPC_ADDRESS_HTABLE_T *hashtable,
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
  }
  else
  {
    *error = 0.0;
    *relative_error = 0.0;
  }
}

void _FPC_FIND_ERRORS_BY_REGISTER(_FPC_REGISTER_HTABLE_T *hashtable,
                                  const char *register_name,
                                  double *error,
                                  double *relative_error)
{
  size_t bin = 0;
  _FPC_REGISTER_T_ temp;
  _FPC_REGISTER_T_ *next = NULL;

  temp.register_name = (char *)register_name;

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
  }
  else
  {
    *error = 0.0;
    *relative_error = 0.0;
  }
}

/*----------------------------------------------------------------------------*/
/* Print hash table                                                           */
/*----------------------------------------------------------------------------*/

/*
void _FPC_PRINT_HASH_TABLE_(_FPC_HTABLE_T *hashtable)
{

}
*/

// Function example output
//  ============== Print Tables  ==============
// printf("Address    |  Register Name          |  Error Value         |  Relative Error     | Clock  \n");
// printf("-----------|-------------------------|----------------------|---------------------|---------\n");
// ...
// ============================================
void _FPC_HT_PRINT_TABLES_(
    _FPC_ADDRESS_HTABLE_T *address_table,
    _FPC_REGISTER_HTABLE_T *register_table)
{
  /* Column widths:
   Address:        18 chars (0x + 16 hex digits)
   Register Name:  25 chars (left-justified, truncated if longer)
   Error Value:    16 chars (right-justified)
   Relative Error: 16 chars (right-justified)
   Clock:           8 chars (right-justified)
  */

  /* Header */
  printf("%-18s %-25s %16s %16s %8s\n",
         "Address", "Register Name", "Error Value", "Relative Error", "Clock");
  printf("%-18s %-25s %16s %16s %8s\n",
         "------------------", "-------------------------", "----------------", "----------------", "--------");

  /* Print address table entries */
  if (address_table != NULL)
  {
    for (uint64_t i = 0; i < address_table->size; ++i)
    {
      _FPC_ADDRESS_T_ *cur = address_table->table[i];
      while (cur != NULL)
      {
        /* Address in hex, register column empty ("-") */
        printf("0x%016llx %-25.25s %16.6g %16.6g %8llu\n",
               (unsigned long long)cur->address_value,
               "-", /* register name placeholder */
               cur->error,
               cur->relative_error,
               (unsigned long long)cur->clock);
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
        printf("%-18s %-25.25s %16.6g %16.6g %8llu\n",
               "-", /* address placeholder */
               (cur->register_name ? cur->register_name : "(null)"),
               cur->error,
               cur->relative_error,
               (unsigned long long)cur->clock);
        cur = cur->next;
      }
    }
  }
}

#endif /* SRC_FPC_HASHTABLE_H_ */
