#ifndef __EFTSAN_RUNTIME_H__
#define __EFTSAN_RUNTIME_H__
#include <iostream>
#include <map>
#include <queue>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <vector>
#include <stack>
#include <list>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <queue>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <execinfo.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <asm/unistd.h>


FILE * m_fpcore;
FILE * m_errfile;
FILE * m_brfile;


enum fp_op{CONSTANT, FADD, FSUB, FMUL, FDIV, SQRT, CEIL, FLOOR, TAN, SIN, CBRT,
	   COS, ATAN, ABS, LOG, LOG10, ASIN, EXP, FREXP, POW, MIN, MAX, LDEXP, FMOD, ATAN2,
	   HYPOT, COSH, SINH, TANH, ACOS, ACOSH, ASINH, ATANH, UNKNOWN};

struct error_info {
  double error;
  unsigned int cbad;
  unsigned int regime;
  unsigned int linenumber;
  unsigned int colnumber;
  bool debugInfoAvail;
};

/* smem_entry: metadata maintained with each shadow memory location.
 * val   : mpfr value for the shadow execution
 * computed: double value 
 * lineno: line number in the source file
 * is_init: is the MPFR initialized
 * opcode: opcode of the operation that created the result

 * error : number of bits in error. Why is it here? (TRACING)
 * lock: CETS style metadata for validity of the temporary metadata pointer (TRACING)
 * key:  CETS style metadata for validity of the temporary metadata pointer (TRACING)
 * tmp_ptr: Pointer to the metadata of the temporary  (TRACING)
 */
struct smem_entry{
  double error;
  double computed;
  size_t timestamp;
#ifdef TRACING  
  enum fp_op opcode; //i32
  size_t lineno;

  struct smem_entry *lhs;
  struct smem_entry *rhs;
#endif
};

struct smem_vec{
  struct smem_entry *op1;
  struct smem_entry *op2;
};

struct temp_entry{
  mpfr_t val;
  double error;
  double computed;
  unsigned int lineno;
  enum fp_op opcode;
  bool is_init;

#ifdef TRACING  
  size_t lock;
  size_t key;

  size_t op1_lock;
  size_t op1_key;
  struct temp_entry* lhs;

  size_t op2_lock;
  size_t op2_key;
  struct temp_entry* rhs;
  size_t timestamp;
#endif
};

#define MMAP_FLAGS (MAP_PRIVATE| MAP_ANONYMOUS| MAP_NORESERVE)
//#define MAX_STACK_SIZE 500000
#define MAX_STACK_SIZE 500

//#define METADATA_AS_TRIE 0

/* Assumption: float types are 4-byte aligned. */

#ifdef METADATA_AS_TRIE

const size_t SS_PRIMARY_TABLE_ENTRIES = ((size_t) 4194304);//2^22
const size_t SS_SEC_TABLE_ENTRIES = ((size_t) 16*(size_t) 1024 * (size_t) 1024); // 2^24
const size_t PRIMARY_INDEX_BITS = 22;
const size_t SECONDARY_INDEX_BITS = 24;
const size_t SECONDARY_MASK = 0xffffff;

smem_entry ** m_shadow_memory;

#else

/* 32 million entries in the hash table */
const size_t HASH_TABLE_ENTRIES = ((size_t) 64 * (size_t) 1024 * (size_t) 1024);

smem_entry * m_shadow_memory;

#endif

size_t *m_ins_map;
size_t *m_inf_map;
size_t *m_nan_map;
size_t *m_bf_map;
smem_entry *m_arg_stack[500];
size_t m_slot_map[500];
size_t m_slot_unique[500];
size_t num_inst = 0;
#define STATIC 0
#define debug 0
#define debugtrace 0
#define debugerror 0
#define ERRORTHRESHOLD 45


# if !defined(PREC_128) && !defined(PREC_256) && !defined(PREC_512) && !defined(PREC_1024) 
#  define PREC_512
# endif

# if !defined(PRECISION) 
# ifdef PREC_128
#   define PRECISION 128
# endif
# ifdef PREC_256
#   define PRECISION 256
# endif
# ifdef PREC_512
#   define PRECISION 512
# endif
# ifdef PREC_1024
#   define PRECISION 1024
# endif
#endif

#ifdef TRACING
size_t * m_lock_key_map;
#endif

int m_prec_bits_f = 0;
int m_prec_bits_d = 0;
size_t m_precision = PRECISION;

#ifdef TRACING
size_t m_key_stack_top = 0;
size_t m_key_counter = 0;
#endif

bool m_init_flag = false;
size_t varCount = 0;
size_t infCount = 0;
size_t nanCount = 0;
size_t errorCount = 0;
size_t flipsCount = 0;
size_t convCount = 0;
size_t ccCount = 0;

std::map<unsigned long long int, struct error_info> m_inst_error_map;
std::map<temp_entry*, std::string> m_var_map;

std::string varString;
std::list <smem_entry*> m_expr;
std::string m_get_string_opcode(size_t);
std::string m_get_string_opcode_fpcore(size_t);
unsigned long m_ulpd(double x, double y);
unsigned long m_ulpf(float x, float y);
int m_update_error(smem_entry *mpfr_val, double computedVal); 
void m_get_error(smem_entry *mpfr_val, double computedVal); 
void m_print_error(size_t opcode, 
		   temp_entry * real, 
		   double d_value, 
		   unsigned int cbad,
		   unsigned long long int instId, 
		   bool debugInfoAvail, 
		   unsigned int linenumber, 
		   unsigned int colnumber);
void m_print_real(mpfr_t);
void m_print_trace (void);
int m_isnan(mpfr_t real);
int m_get_depth(smem_entry*);
void m_compute(fp_op , double, temp_entry *, double,
	       temp_entry *, double, temp_entry *, size_t);

void m_store_shadow_dconst(smem_entry *, double , unsigned int);
void m_store_shadow_fconst(smem_entry *, float , unsigned int);
bool m_check_branch(double, double, size_t);
#endif
