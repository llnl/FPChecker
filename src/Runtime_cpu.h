
#ifndef SRC_RUNTIME_CPU_H_
#define SRC_RUNTIME_CPU_H_

#include "FPC_Hashtable.h"
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
// #include<string.h>

#ifdef FPC_MULTI_THREADED
#include <pthread.h>
#endif

#define FPC_MAX(a, b) (((a) > (b)) ? (a) : (b))

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

/** We store the file name and directory in this variable **/
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
#define MAX_ERROR_ENTRIES 1000
#define MAX_NAME_SIZE 100
/** Program name and input **/
int _FPC_PROG_INPUTS;
char **_FPC_PROG_ARGS;

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
  _FPC_ERROR_HTABLE_ = _FPC_ERROR_HT_CREATE_(FPC_ERROR_HTABLE_SIZE);


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
  _FPC_INIT_HASH_TABLE_();
}

void _FPC_INIT_ARGS_FPCHECKER(int argc, char **argv)
{
  _FPC_PROG_INPUTS = argc;
  _FPC_PROG_ARGS = argv;
  _FPC_INIT_HASH_TABLE_();
}

void _FPC_PRINT_LOCATIONS_()
{
#ifndef FPC_QUIET
  printf("#FPCHECKER: Finalizing and writing traces...\n");
#endif
  _FPC_PRINT_HASH_TABLE_(_FPC_HTABLE_);
  _FPC_PRINT_ERROR_TABLE_(_FPC_ERROR_HTABLE_);

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

// Simple structure: address || register || error
uintptr_t addresses[MAX_ERROR_ENTRIES];
char registers[MAX_ERROR_ENTRIES][MAX_NAME_SIZE];
double errors[MAX_ERROR_ENTRIES];
int entry_count = 0;

// Find entry by register name
int _FPC_FP32_FIND_BY_REGISTER_(const char *reg_name) {
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(registers[i], reg_name) == 0) {
            return i;
        }
    }
    printf("Register names:\n");
    for (int j = 0; j < entry_count; j++) {
      printf("  [%d] %s\n", j, registers[j]);
    }
    return -1; // No register available in the table
}

// Find entry by address
int _FPC_FP32_FIND_BY_ADDRESS_(uintptr_t addr) {
    for (int i = 0; i < entry_count; i++) {
        if (addresses[i] == addr) {
            return i;
        }
    }
    return -1;
}


void _FPC_FP32_STORE_INST_(const char *reg, uintptr_t address) {
  // printf("Instrumented store registers so far:\n");
  // for (int i = 0; i < entry_count; i++) {
  //   if (strlen(registers[i]) > 0) {
  //     printf("  [%d] %s\n", i, registers[i]);
  //   }
  // }
  //   printf("\n>>> STORE: %s || addr %lu\n", reg, address);
    
    double store_error = 0.0;
    
    // First, check if this register already exists with an error
    int reg_id = _FPC_FP32_FIND_BY_REGISTER_(reg);
    if (reg_id >= 0) {
        store_error = errors[reg_id];
        addresses[reg_id] = address;
        printf("Register  %s exists with error %.17e\n", reg, store_error);
    } else {
        
        int last_non_zero_error_id = -1;
        double last_seen_error = 0.0;
        
          for (int i = entry_count - 1; i >= 0; i--) {
            if (errors[i] != 0.0) {
                // Found a register with error
                last_non_zero_error_id = i;
                last_seen_error = errors[i];
                printf("Propagating Error from %s gets error from value %s (%.17e)\n", 
                       reg, registers[i], last_seen_error);
                break;
            }
        }
        
        store_error = last_seen_error;
        
        // Create new register entry for the pointer
        if (entry_count < MAX_ERROR_ENTRIES) {
            addresses[entry_count] = address;
            strncpy(registers[entry_count], reg, MAX_NAME_SIZE - 1);
            registers[entry_count][MAX_NAME_SIZE - 1] = '\0';
            errors[entry_count] = store_error;
            entry_count++;
        }
    }
    
    int addr_id = _FPC_FP32_FIND_BY_ADDRESS_(address);
    if (addr_id >= 0 && addr_id != reg_id) {
        errors[addr_id] = store_error;
        printf("Updated existing memory location %lu with error %.17e\n", address, store_error);
    }
    else if (addr_id < 0){
      //Create a new memory entry for this address
      if(entry_count < MAX_ERROR_ENTRIES){
        addresses[entry_count] =address;
        errors[entry_count] = store_error;
        printf("Created new memory entry for addr %lu with error %.17e\n", address, store_error);
        entry_count++;
      }
    }
    
    printf(" %s || %lu  <- error %.17e\n", reg, address, store_error);
    for(int i = 0; i< entry_count ; i++){
      printf("[%d] %-10s|| %-10lu = %.17e\n", i, registers[i], addresses[i], errors[i]);
    }
    
    // if (store_error != 0.0) {
    //     printf("ERROR PROPAGATED TO MEMORY!\n");
    // } else {
    //     // DEBUG: Show what we have in the table
    //     printf(" DEBUG: Table Entry\n");
    //     for (int i = 0; i < entry_count; i++) {
    //         printf("[%d] %-10s|| %-10lu = %.17e\n", i, registers[i], addresses[i], errors[i]);
    //     }
    // }
}

// Load instruction
void _FPC_FP32_LOAD_INST_(const char *load_reg, uintptr_t address) {
    printf("\n>>> LOAD: addr %lu -> %s\n", address, load_reg);
    
    // Find the error at this memory address
    double memory_error = 0.0;
    
    int addr_id = _FPC_FP32_FIND_BY_ADDRESS_(address);
    if (addr_id >= 0) {
        memory_error = errors[addr_id];
        printf("Found error %.17e for address %lu\n", memory_error, address);
    }
    
    // Update or create the register entry
    int reg_id = _FPC_FP32_FIND_BY_REGISTER_(load_reg);
    if (reg_id >= 0) {
        addresses[reg_id] = address;
        errors[reg_id] = memory_error;
    } else {
        if (entry_count < MAX_ERROR_ENTRIES) {
            addresses[entry_count] = address;
            strncpy(registers[entry_count], load_reg, MAX_NAME_SIZE - 1);
            registers[entry_count][MAX_NAME_SIZE - 1] = '\0';
            errors[entry_count] = memory_error;
            entry_count++;
        }
    }
    printf(" %s || %lu || %.17e \n", load_reg, address, memory_error);
    
    // if (memory_error != 0.0) {
    //     printf("ERROR INHERITED!\n");
    //}
}

// Find error - unchanged
double _FPC_FP32_FIND_ERROR_(const char *reg_name) {
    if (!reg_name || strlen(reg_name) == 0) {
        printf("#FPCHECKER-FIND: No register name provided → returning 0.0\n");
        return 0.0;
    }

    int id = _FPC_FP32_FIND_BY_REGISTER_(reg_name);
    if (id >= 0) {
        printf("#FPCHECKER-FIND: Found error for [%s || %lu] = %.17e\n", 
               reg_name, addresses[id], errors[id]);
        return errors[id];
    }

    printf("#FPCHECKER-FIND: No error found for [%s] -> returning 0.0\n", reg_name);
    return 0.0;
}

// Store error
void _FPC_FP32_STORE_ERROR_(const char *reg_name, double error) {
    if (!reg_name || strlen(reg_name) == 0) {
        printf("#FPCHECKER-STORE: No register name provided\n");
        return;
    }

    int id = _FPC_FP32_FIND_BY_REGISTER_(reg_name);
    if (id >= 0) {
        // Update existing entry's error
        errors[id] = error;
        printf("#FPCHECKER-STORE: Updated error for [%s || %lu] = %.17e\n", 
               reg_name, addresses[id], error);
    } else {
        // Create new entry (address will be 0 until LOAD/STORE sets it)
        if (entry_count < MAX_ERROR_ENTRIES) {
            addresses[entry_count] = 0;  // Will be reset by LOAD/STORE
            strncpy(registers[entry_count], reg_name, MAX_NAME_SIZE - 1);
            registers[entry_count][MAX_NAME_SIZE - 1] = '\0';
            errors[entry_count] = error;
            printf("#FPCHECKER-STORE: Stored new error for [%s || %lu] = %.17e\n", 
                   reg_name, addresses[entry_count], error);
            entry_count++;
        } else {
            printf("#FPCHECKER-STORE: Error table full\n");
        }
    }
}


void _FPC_FP32_CHECK_(
    float x, float y, float z, float w, int loc, char *file_name, int op, int cond, 
    const char *result_name, const char *op1_name, const char *op2_name, const char *fma_name) {
    
    // printf("\n>>> CHECK: %s = %s op %s\n", 
    //        result_name ? result_name : "null",
    //        op1_name ? op1_name : "null", 
    //        op2_name ? op2_name : "null");
    
    double err_y = _FPC_FP32_FIND_ERROR_(op1_name);
    double err_z = _FPC_FP32_FIND_ERROR_(op2_name);
    // double err_w = (fma_name && strlen(fma_name) > 0) ? _FPC_FP32_FIND_ERROR_(fma_name) : 0.0;
    double err_w = _FPC_FP32_FIND_ERROR_(fma_name);
    
    double y_high = (double)y + err_y;
    double z_high = (double)z + err_z;
    double w_high = (double)w + err_w;
    
    double r_high = 0.0;
    switch (op) {
        case 0: r_high = y_high + z_high; break;
        case 1: r_high = y_high - z_high; break;
        case 2: r_high = y_high * z_high; break;
        case 3: 
            if (z_high != 0.0) {
                r_high = y_high / z_high; 
            } else {
                printf("#FPCHECKER_ERROR: Division by zero\n");
                r_high = 0.0;
            }
            break;
        case 5: r_high = fmod(y_high, z_high); break;
        case 6: r_high = fma(y_high, z_high, w_high); break;
        default:
            printf("FPCHECKER_ERROR: Unknown operation %d\n", op);
    }
    
    double r_low = (double)x;
   
    double err_result = r_high - r_low;
    
    printf("Inputs: %.17e, %.17e -> Error: %.17e\n", y_high, z_high, err_result);
    
    _FPC_FP32_STORE_ERROR_(result_name, err_result);
     printf("Result in runtime (double) : %.17e and (float) %.7f\n", r_low, x);
    fflush(stdout);

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

 _FPC_ERROR_ITEM_T_ new_item;
  new_item.file_name = file_name;
  new_item.line = (uint64_t)loc;
  new_item.error = err_result;
  _FPC_ERROR_HT_SET_(_FPC_ERROR_HTABLE_, &new_item);


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
/* Annotation Macros                                                          */
/*----------------------------------------------------------------------------*/

#define FPC_INSTRUMENT_BLOCK __attribute__((annotate("_FPC_INSTRUMENT_BLOCK_"))) int _marker __attribute__((unused)) = 0;
#define FPC_INSTRUMENT_FUNC __attribute__((annotate("_FPC_INSTRUMENT_FUNCTION_")))

// void _FPC_FP32_STORE_INST_(const char *reg, uintptr_t address)
// {
//   printf("*** Storing %s to address %lu\n", reg, address);
// }

// void _FPC_FP32_LOAD_INST_(const char *load_reg, uintptr_t address)
// {
//   printf("*** Loading address %lu into register %s\n", address, load_reg);
// }

#endif /* SRC_RUNTIME_CPU_H_ */
