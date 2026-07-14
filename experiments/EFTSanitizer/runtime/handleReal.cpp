#include "handleReal.h"
#include <cmath>
#include <limits>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <iomanip> 
#include <complex.h>
/*
We don't want to call mpfr_init on every add or sub.That's why we keep 
it as global variables and do init once and just update on every add or sub 
*/
mpfr_t temp, tempOp1, tempOp2;
long loadIns = 0;
long storeIns = 0;
long setArg = 0;
long getArg = 0;
long branchIns = 0;
// eftsan_trace: a function that user can set a breakpoint on to
// generate DAGs
static double maxRelError = 0.0;
static double lastRelError = 0.0;
static double maxRelErrorComputedVal = 0.0;
static double maxRelErrorEFTError = 0.0;

#ifdef METADATA_AS_TRIE
smem_entry* m_get_shadowaddress(size_t address){
  size_t addrInt = address >> 2;
  size_t primary_index = (addrInt >> SECONDARY_INDEX_BITS );
  smem_entry* primary_ptr = m_shadow_memory[primary_index];
  if (primary_ptr == NULL) {
    size_t sec_length = (SS_SEC_TABLE_ENTRIES) * sizeof(smem_entry);
    primary_ptr = (smem_entry*)mmap(0, sec_length, PROT_READ| PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
    m_shadow_memory[primary_index] = primary_ptr;
  }
  size_t offset = (addrInt) & SECONDARY_MASK;
  smem_entry* realAddr = primary_ptr + offset;
  return realAddr;
}

#else

smem_entry* m_get_shadowaddress (size_t address){
  size_t addr_int = address >> 2;
  size_t index = addr_int  % HASH_TABLE_ENTRIES;
  smem_entry* realAddr = m_shadow_memory + index;
  realAddr->error = 0.0;
  return realAddr;
}

#endif

#ifdef TRACING
extern "C" void eftsan_fpcore(temp_entry *cur){
  if(cur){
    if(m_lock_key_map[cur->op1_lock] != cur->op1_key ){
      if(m_var_map.count(cur) > 0){
        varString += "( x_"+ m_var_map.at(cur) + ")";
      }
      else{
        varCount++;
        auto var = std::to_string(varCount);
        varString += "( x_"+ var + ")";
        m_var_map.insert( std::pair<temp_entry*, std::string>(cur, var) );
      }
      return;
    }
    if(m_lock_key_map[cur->op2_lock] != cur->op2_key){
      if(m_var_map.count(cur) > 0){
        varString += "( x_"+ m_var_map.at(cur) + ")";
      }
      else{
        varCount++;
        auto var = std::to_string(varCount);
        varString += "( x_"+ var + ")";
        m_var_map.insert( std::pair<temp_entry*, std::string>(cur, var) );
      }
      return;
    }
    if(cur->opcode == CONSTANT){
      if(m_var_map.count(cur) > 0){
        varString += "( x_"+ m_var_map.at(cur);
      }
      else{
        varCount++;
        auto var = std::to_string(varCount);
        varString += "( x_"+ var ;
        m_var_map.insert( std::pair<temp_entry*, std::string>(cur, var) );
      }
    }
    else{
      varString += "(" + m_get_string_opcode_fpcore(cur->opcode);
    }
    if(cur->lhs != NULL){
      if(cur->lhs->timestamp < cur->timestamp){
        eftsan_fpcore(cur->lhs);
      }
    }
    if(cur->rhs != NULL){
      if(cur->rhs->timestamp < cur->timestamp){
        eftsan_fpcore(cur->rhs);
      }
    }
    varString += ")";
  }
}

extern "C" void eftsan_get_fpcore(temp_entry *cur){
  fflush(stdout);
  eftsan_fpcore(cur);
  std::string out_fpcore;
  out_fpcore = "(FPCore ( ";

  while(varCount > 0){
    out_fpcore += "x_"+std::to_string(varCount) + " ";
    varCount--;
  }
  out_fpcore += ")\n";
  out_fpcore += varString; 
  out_fpcore += ")\n";
  fprintf(m_fpcore, "%s",out_fpcore.c_str());
  varString = "";
  varCount = 0;
}

extern "C" void eftsan_trace_less(smem_entry *current){
  m_expr.clear();
  m_expr.push_back(current);
  FILE *fp = popen("/usr/bin/less", "w");
  int level;
  while(!m_expr.empty()){
    level = m_expr.size();
    smem_entry *cur = m_expr.front();
    if(cur == NULL){
      return;
    }
    fprintf(fp, "\n");
//    std::cout<<"\n";
    fprintf(fp, " %d %s ", cur->lineno, m_get_string_opcode(cur->opcode).c_str());
//    std::cout<<" "<<cur->lineno<<" "<<m_get_string_opcode(cur->opcode)<<" ";
    fflush(stdout);
    if(cur->lhs != NULL){
//      std::cout<<" "<<cur->lhs->lineno<<" ";
      fprintf(fp, " %d ", cur->lhs->lineno);
      if(cur->lhs->timestamp < cur->timestamp){
        m_expr.push_back(cur->lhs);
        fflush(stdout);
      }
    }
    if(cur->rhs != NULL){
      //std::cout<<" "<<cur->rhs->lineno<<" ";
      fprintf(fp, " %d ", cur->rhs->lineno);
      if(cur->rhs->timestamp < cur->timestamp){
        m_expr.push_back(cur->rhs);
        fflush(stdout);
      }
    }
    //std::cout<<"(real:"<<std::setprecision(17)<<cur->computed+cur->error;
    fprintf(fp, "(real:%.17g ", cur->computed+cur->error);
    //std::cout<<", computed:"<<std::setprecision(17)<<cur->computed;
    fprintf(fp, ", computed:%.17g ", cur->computed);
//    std::cout<<", error:"<<std::setprecision(17)<<cur->error<<" "<< m_update_error(cur, cur->computed)<<")";
    fprintf(fp, ", error:%.17g  %ld)", cur->error,  m_update_error(cur, cur->computed));
    fflush(stdout);
    m_expr.pop_front();
    level--;
  }
//  std::cout<<"\n\n";
  fprintf(fp, "\n\n");
  pclose(fp);
}

extern "C" void eftsan_trace(smem_entry *current){
  m_expr.clear();
  m_expr.push_back(current);
  int level;
  while(!m_expr.empty()){
    level = m_expr.size();
    smem_entry *cur = m_expr.front();
    if(cur == NULL){
      return;
    }
    std::cout<<"\n";
    std::cout<<" "<<cur->lineno<<" "<<m_get_string_opcode(cur->opcode)<<" ";
    fflush(stdout);
    if(cur->lhs != NULL){
      std::cout<<" "<<cur->lhs->lineno<<" ";
      if(cur->lhs->timestamp < cur->timestamp){
        m_expr.push_back(cur->lhs);
        fflush(stdout);
      }
    }
    if(cur->rhs != NULL){
      std::cout<<" "<<cur->rhs->lineno<<" ";
      if(cur->rhs->timestamp < cur->timestamp){
        m_expr.push_back(cur->rhs);
        fflush(stdout);
      }
    }
//    std::cout<<"(real:"<<std::setprecision(17)<<cur->computed+cur->error;
    printf("(real:: %e", cur->computed+cur->error);
    //std::cout<<", computed:"<<std::setprecision(17)<<cur->computed;
    printf(", computed: %e", cur->computed);
    std::cout<<", error:"<<std::setprecision(17)<<cur->error<<" "<< m_update_error(cur, cur->computed)<<")";
    fflush(stdout);
    m_expr.pop_front();
    level--;
  }
  std::cout<<"\n\n";
}
#endif
// eftsan_check_branch, eftsan_check_conversion, eftsan_check_error are
// functions that user can set breakpoint on
extern "C" void eftsan_handle_fptrunc(float val, smem_entry* op1){
  //op1->computed = val;
}

extern "C" void eftsan_check_conv_si(int val, smem_entry*res){
  double conv = res->computed + res->error;
  int real_val = conv;
  if(real_val != val){
    convCount++;
  }
}

extern "C" void eftsan_check_conv_ui(size_t val, smem_entry*res){
  double conv = res->computed + res->error;
  size_t real_val = conv;
  if(real_val != val){
    convCount++;
  }
}

extern "C" bool eftsan_check_branch_f1(float op1d,
				     float op2d, smem_entry* op2,
				     size_t fcmpFlag, bool computedRes,
				     size_t lineNo){

  bool realRes = m_check_branch(op1d, 
                    op2d + op2->error, fcmpFlag);
  if(realRes != computedRes){
    flipsCount++;
#if STATIC
    m_bf_map[op2->lineno]++;
#endif
  }
  return realRes;
}

extern "C" bool eftsan_check_branch_f2(float op1d, smem_entry* op1,
				     float op2d, 
				     size_t fcmpFlag, bool computedRes,
				     size_t lineNo){

  bool realRes = m_check_branch(op1d + op1->error, 
                    op2d, fcmpFlag);
  if(realRes != computedRes){
    flipsCount++;
#if STATIC
    m_bf_map[op1->lineno]++;
#endif
  }
  return realRes;
}

extern "C" bool eftsan_check_branch_f(float op1d, smem_entry* op1,
				     float op2d, smem_entry* op2,
				     size_t fcmpFlag, bool computedRes,
				     size_t lineNo){

  bool realRes = m_check_branch(op1d + op1->error, 
                    op2d + op2->error, fcmpFlag);
  if(realRes != computedRes){
    flipsCount++;
#if STATIC
    m_bf_map[op1->lineno]++;
#endif
  }
  return realRes;
}

extern "C" bool eftsan_check_branch_d1(double op1d,
				     double op2d, smem_entry* op2,
				     size_t fcmpFlag, bool computedRes,
				     size_t lineNo){

  double op1Total = op1d;
  double op2Total = op2d + op2->error;

  bool realRes = m_check_branch(op1Total, 
                    op2Total, fcmpFlag);
  if(realRes != computedRes){
    flipsCount++;
#if STATIC
    m_bf_map[op2->lineno]++;
#endif
  }
  return realRes;
}

extern "C" bool eftsan_check_branch_d2(double op1d, smem_entry* op1,
				     double op2d,
				     size_t fcmpFlag, bool computedRes,
				     size_t lineNo){

  double op1Total = op1d + op1->error;
  double op2Total = op2d;

  bool realRes = m_check_branch(op1Total, 
                    op2Total, fcmpFlag);
  if(realRes != computedRes){
    flipsCount++;
#if STATIC
    m_bf_map[op1->lineno]++;
#endif
  }
  return realRes;
}

extern "C" bool eftsan_check_branch_d(double op1d, smem_entry* op1,
				     double op2d, smem_entry* op2,
				     size_t fcmpFlag, bool computedRes,
				     size_t lineNo){

  double op1Total = op1d + op1->error;
  double op2Total = op2d + op2->error;

  bool realRes = m_check_branch(op1Total, 
                    op2Total, fcmpFlag);
  if(realRes != computedRes){
    flipsCount++;
#if STATIC
    m_bf_map[op1->lineno]++;
#endif
  }
  return realRes;
}
extern "C" unsigned int  eftsan_check_conversion(long real, long computed,
						smem_entry *realRes){
  if(real != computed){
    return 1;
  }
  return 0;
}

extern "C" void eftsan_init(int size) {
  if (!m_init_flag) {
    m_fpcore = fopen ("eftsan.fpcore","w");
    m_errfile = fopen ("error.log","w");
    
    m_init_flag = true;
    num_inst = size;
#if STATIC
    size_t length = size * sizeof(size_t);

    m_ins_map =
      (size_t *)mmap(0, length, PROT_READ | PROT_WRITE, MMAP_FLAGS, -1, 0);
    m_bf_map =
      (size_t *)mmap(0, length, PROT_READ | PROT_WRITE, MMAP_FLAGS, -1, 0);
    m_inf_map =
      (size_t *)mmap(0, length, PROT_READ | PROT_WRITE, MMAP_FLAGS, -1, 0);
    m_nan_map =
      (size_t *)mmap(0, length, PROT_READ | PROT_WRITE, MMAP_FLAGS, -1, 0);
#endif
  /*
    size_t hash_size = (HASH_TABLE_ENTRIES) * sizeof(smem_entry);
    m_shadow_memory =
      (smem_entry *) mmap(0, hash_size, PROT_READ | PROT_WRITE, MMAP_FLAGS, -1, 0);

    assert(m_arg_stack != (void *)-1);
    */
    mpfr_init2(temp, m_precision);
    mpfr_init2(tempOp1, m_precision);
    mpfr_init2(tempOp2, m_precision);
  }
}

void m_set_mpfr(mpfr_t *val1, mpfr_t *val2) {
  mpfr_set(*val1, *val2, MPFR_RNDN);
}

//primarily used in the LLVM IR for initializing stack metadata

int m_isnan(mpfr_t real){
    return mpfr_nan_p(real);
}

float m_get_float(mpfr_t mpfr_val) { return mpfr_get_flt(mpfr_val, MPFR_RNDN); }

double m_get_double(mpfr_t mpfr_val) { return mpfr_get_d(mpfr_val, MPFR_RNDN); }

long double m_get_longdouble(temp_entry *real) {
  return mpfr_get_ld(real->val, MPFR_RNDN);
}

extern "C" void eftsan_handle_memset(void *toAddr, int val,
    int size) {

  size_t toAddrInt = (size_t)(toAddr);
  for (int i = 0; i < size; i++) {
    smem_entry *dst = m_get_shadowaddress(toAddrInt + i);

  dst->error = 0;
  dst->computed = 0;
  dst->timestamp = 0;
#ifdef TRACING
  dst->opcode = CONSTANT;
  dst->lineno = 0;
  dst->lhs = nullptr;
  dst->rhs = nullptr;
#endif
  }
}

extern "C" void eftsan_handle_memcpy(smem_entry* toAddr, smem_entry* fromAddr, int size){
  
  memcpy(toAddr, fromAddr, size);
  /*
  size_t toAddrInt = (size_t) (toAddr);
  size_t fromAddrInt = (size_t) (fromAddr);
  for(int i=0; i<size; i++){
  }
  */
}

void m_print_real(mpfr_t mpfr_val){

  mpfr_out_str (stdout, 10, 15, mpfr_val, MPFR_RNDN);
}

extern "C" void eftsan_copy_phi(smem_entry* src, smem_entry* dest, size_t slot_idx){
  dest->error = src->error;
  dest->computed = src->computed;
  dest->timestamp = src->timestamp;
#ifdef TRACING
  dest->opcode = src->opcode;
  dest->lineno = src->lineno;
  dest->lhs = src->lhs;
  dest->rhs = src->rhs;
#endif
}

bool m_check_branch(double op1, double op2,
		    size_t fcmpFlag){
  bool realRes = false;

  switch(fcmpFlag){
    case 0:
      realRes = false;
      break;
    case 1: /*oeq*/
      if(!isnan(op1) && !isnan(op2)){
        if(op1 == op2)
          realRes = true;
      }
      break;
    case 2: /*ogt*/
      if(!isnan(op1) && !isnan(op2)){
        if(op1 > op2){
          realRes = true;
        }
      }
      break;
    case 3:
      if(!isnan(op1) && !isnan(op2)){
        if(op1 > op2 || op1 == op2){
          realRes = true;
        }
      }
      break;
    case 4: /*olt*/
      if(!isnan(op1) && !isnan(op2)){
        if(op1 < op2){
          realRes = true;
        }
      }
      break;
    case 5:
      if(!isnan(op1) && !isnan(op2)){
        if(op1 < op2 || op1 == op2){
          realRes = true;
        }
      }
      break;
    case 6:
      if(!isnan(op1) && !isnan(op2)){
        if(op1 != op2){
          realRes = true;
        }
      }
      break;
    case 7:
      if(!isnan(op1) && !isnan(op2)){
        realRes = true;
      }
      break;
    case 8:
      if(isnan(op1) && isnan(op2)){
        realRes = true;
      }
      break;
    case 9:
      if(isnan(op1) || isnan(op2) || op1 == op2)
        realRes = true;
      break;
    case 10:
      if(isnan(op1) || isnan(op2) || op1 > op2)
        realRes = true;
      break;
    case 11:
      if(isnan(op1) || isnan(op2) || op1 >= op2)
        realRes = true;
      break;
    case 12: 
      if(isnan(op1) || isnan(op2) || op1 < op2)
        realRes = true;
      break;
    case 13:
      if(isnan(op1) || isnan(op2) || op1 <= op2)
        realRes = true;
      break;
    case 14:
      if(isnan(op1) || isnan(op2) || op1 != op2){
        realRes = true;
      }
      break;
    case 15:
      realRes = true;
      break;
  }
  return realRes;
}

std::string m_get_string_opcode_fpcore(size_t opcode){
  switch(opcode){
    case FADD:
      return "+";
    case FMUL:
      return "*";
    case FSUB:
      return "-";
    case FDIV:
      return "/";
    case CONSTANT:
      return "CONSTANT";
    case SQRT:  
      return "sqrt";
    case FLOOR:  
      return "floor";
    case TAN:  
      return "tan";
    case SIN:  
      return "sin";
    case COS:  
      return "cos";
    case ACOS:  
      return "acos";
    case ATAN:  
      return "atan";
    case ABS:  
      return "abs";
    case LOG:  
      return "log";
    case ASIN:  
      return "asin";
    case EXP:  
      return "exp";
    case POW:
      return "pow";
    default:
      return "Unknown";
  }
}

std::string m_get_string_opcode(size_t opcode){
  switch(opcode){
    case FADD:
      return "FADD";
    case FMUL:
      return "FMUL";
    case FSUB:
      return "FSUB";
    case FDIV:
      return "FDIV";
    case CONSTANT:
      return "CONSTANT";
    case SQRT:  
      return "SQRT";
    case FLOOR:  
      return "FLOOR";
    case TAN:  
      return "TAN";
    case ATAN:  
      return "ATAN";
    case ATANH:  
      return "ATANH";
    case SIN:  
      return "SIN";
    case ASIN:  
      return "ASIN";
    case ASINH:  
      return "ASINH";
    case COS:  
      return "COS";
    case ACOS:  
      return "ACOS";
    case ACOSH:  
      return "ACOSH";
    case ABS:  
      return "ABS";
    case LOG:  
      return "LOG";
    case EXP:  
      return "EXP";
    case POW:
      return "POW";
    default:
      return "Unknown";
  }
}

#ifdef TRACING
int m_get_depth(smem_entry *current){
  int depth = 0;
  m_expr.push_back(current);
  int level;
  while(!m_expr.empty()){
    level = m_expr.size();
    std::cout<<"\n";
    while(level > 0){
      smem_entry *cur = m_expr.front();
      if(cur == NULL){
        return depth;
      }
      if(cur->lhs != NULL){
        if(cur->lhs->timestamp < cur->timestamp){
          m_expr.push_back(cur->lhs);
        }
      }
      if(cur->rhs != NULL){
        if(cur->rhs->timestamp < cur->timestamp){
          m_expr.push_back(cur->rhs);
        }
      }
      m_expr.pop_front();
      level--;
    }
    depth++;
  }
  return depth;
}
#endif

extern "C" void eftsan_func_init(size_t totalArgs, size_t *m_stack_top) {
//  *m_stack_top = *m_stack_top + totalArgs;

//  if(*m_stack_top >= MAX_STACK_SIZE){
    //*m_stack_top = 1000;
//    *m_stack_top = 0;
//  }
}

extern "C" void eftsan_func_exit(long totalArgs) {
//  m_stack_top = m_stack_top - totalArgs;
}

/* Copy the metadata of the return value of the function and insert
   into the shadow stack. The space for the return value is allocated
   by the caller. This happens in the callee. */

extern "C" void eftsan_set_return_vec(smem_entry *dest, smem_entry* src, size_t totalArgs, size_t m_stack_top) {
  //smem_entry *dest = &(m_shadow_stack[m_stack_top - totalArgs + 1]); 
  dest->error = src->error;
  dest->computed = src->computed;
  dest->timestamp = src->timestamp;
#ifdef TRACING
  dest->opcode = src->opcode;
  dest->lineno = src->lineno;
  dest->lhs = src->lhs;
  dest->rhs = src->rhs;
#endif
}

extern "C" void eftsan_set_return(smem_entry *dest, smem_entry* src, size_t idx) {
  
  dest->error = src->error;
  dest->computed = src->computed;
  dest->timestamp = src->timestamp;
#ifdef TRACING
  dest->opcode = src->opcode;
  dest->lineno = src->lineno;
  dest->lhs = src->lhs;
  dest->rhs = src->rhs;
#endif
//  m_get_error(dest, dest->computed);
}

/* Retrieve the metadata for the return value from the shadow
   stack. This happens in the caller. */
extern "C" smem_entry* eftsan_get_return(size_t m_stack_top, smem_entry* dest) {

//  smem_entry *src = &(m_shadow_stack[m_stack_top - totalArgs]); //save return m_stack_top - totalArgs 
//  return src;
}

extern "C" smem_entry* eftsan_get_return_vec(size_t totalArgs, size_t m_stack_top) {

//  smem_entry *src = &(m_shadow_stack[m_stack_top - totalArgs + 1]); //save return m_stack_top - totalArgs 
 // return src;
}

extern "C" void eftsan_handle_load(smem_entry* dest, smem_entry* src){
  loadIns++;
}

extern "C" void eftsan_handle_store(smem_entry* dest, smem_entry* src){
  storeIns++;
}

/* The callee retrieves the metadata from the shadow stack */

extern "C" smem_entry* eftsan_get_arg(size_t argIdx) {

  smem_entry *dst = m_arg_stack[argIdx];
  return dst;
}

extern "C" void eftsan_set_arg_f(smem_entry* src, float op, bool consFlag, size_t argIdx) {
  m_arg_stack[argIdx] = src;
  assert(argIdx < MAX_STACK_SIZE && "Arg index is more than MAX_STACK_SIZE");
}

extern "C" void eftsan_set_arg_d(smem_entry* src, double op, bool consFlag, size_t argIdx) {

  if(src == nullptr)
    return;
  m_arg_stack[argIdx] = src;

  assert(argIdx < MAX_STACK_SIZE && "Arg index is more than MAX_STACK_SIZE");
}

double two_sum(smem_entry* op1, smem_entry* op2, double computedRes){
  /*
  double z = computedRes - op1->computed;
  double y1 = (op2->computed - z);
  double y2 = (computedRes - z);
  double y3 = (op1->computed - y2);
  double y = y3 + y1;

  //add errors from operands
  double newErr = y + op1->error + op2->error;
  return newErr;
  */
}

double two_product(smem_entry* op1, smem_entry* op2, double computedRes){
  /*
  double y = fma(op1->computed, op2->computed, -computedRes);
  double err1 = op1->computed * op2->error;
  double err2 = op2->computed * op1->error;
  double err3 = err1 + err2;
  double err = y + err3;
  return err;
  */
}

/*
//__muldc3(r1, i1, r2, i2)
//x = a + ib and
//y = c + id
extern "C" void eftsan_mpfr___muldc3(smem_entry* op1, 
				                         smem_entry* lhs, 
                                 double op1d,
                                 smem_entry* res, 
                                 double computedRes,
                                 unsigned long long int instId, 
                                 bool debugInfoAvail, 
                                 unsigned int linenumber, 
                                 unsigned int colnumber,
                                 unsigned long long int ts){
function [p, e, f, g] = TwoProductCplx(x, y)
  double res1 = op1_r * op2_r;
  double err1 = two_product(op1_r, op2_r, res1)
//  [z1, h1] = TwoProduct(a, c)
  double res2 = op1_i * op2_i;
  double err2 = two_product(op1_i, op2_i, res2)
  //[z2, h2] = TwoProduct(b, d)
  double res3 = op1_r * op2_i;
  double err3 = two_product(op1_r, op2_i, res3)
  //[z3, h3] = TwoProduct(a, d)
  double res4 = op1_i * op2_r;
  double err4 = two_product(op1_i, op2_r, res4)
//  [z4, h4] = TwoProduct(b, c)
  double res5 = res1 - res2;
  double err5 = two_sum(op1_i, op2_r, res4)
//  [z5, h5] = TwoSum(z1, −z2)
  [z6, h6] = TwoSum(z3, z4)
  p = z5 + iz6
  e = h1 + ih3
  f = −h2 + ih4
  g = h5 + ih6
}
*/

extern "C" void eftsan_set_epsilon_precision_const(smem_entry* res){
  /*
  res->computed = std::numeric_limits<float>::epsilon();
  res->error = -1.19209289e-7;
  res->lhs = nullptr; 
  res->rhs = nullptr;
  res->lineno = 0;
  res->timestamp = 0;
  res->lineno = 0;
  res->opcode = CONSTANT;
  */
}

extern "C" void eftsan_load_reset(double computed, double load, int cond){
//  m_slot_unique[instId] = timestamp;
}
extern "C" void eftsan_index(size_t idx, smem_entry* res){
//  m_slot_unique[instId] = timestamp;
}
extern "C" void eftsan_slot(size_t instId, size_t timestamp){
//  m_slot_unique[instId] = timestamp;
}
extern "C" void eftsan_slot_load_reset(size_t instId, size_t timestamp){
//  m_slot_unique[instId] = timestamp;
}
extern "C" void eftsan_slot_load(size_t instId, size_t timestamp){
//  m_slot_unique[instId] = timestamp;
}
extern "C" void eftsan_slot_store(size_t instId, size_t timestamp){
//  m_slot_unique[instId] = timestamp;
}
extern "C" smem_entry* eftsan_valid(size_t ts, size_t tsmap, size_t instId){
}
/*
extern "C" smem_entry* eftsan_valid(size_t ts, size_t instId, size_t u_ts, smem_entry* res){
  smem_entry *tmp = NULL;
  
  if(ts == m_slot_unique[instId]){
    return res;
  }
  else{
    return tmp;
  }
}
*/
extern "C" smem_entry* eftsan_get_op_metadata(size_t op1Ts, size_t op2Ts, size_t resTs, size_t op1InstId, size_t op2InstId, 
                                              size_t resInstId, smem_entry* op1, smem_entry* op2){
  m_slot_unique[resInstId] = resTs;

  if(op1Ts != m_slot_unique[op1InstId]){
    op1 = NULL;
  }
  if(op2Ts != m_slot_unique[op2InstId]){
    op2 = NULL;
  }
}

extern "C" void eftsan_set_constant(smem_entry *res, size_t ts, size_t insId){
}

extern "C" void eftsan_sub(double res, double op1, double op2,
                      smem_entry *res_, smem_entry *op1_, smem_entry *op2_, size_t instId, double err, double c_err){
  m_get_error(res_, res);
}

extern "C" void eftsan_sum(double res, double op1, double op2,
                      smem_entry *res_, smem_entry *op1_, smem_entry *op2_, size_t instId, double err, double c_err){
  m_get_error(res_, res);
}

extern "C" void eftsan_mul(double res, double op1, double op2,
                      smem_entry *res_, smem_entry *op1_, smem_entry *op2_, size_t instId, double err, double c_err){
  m_get_error(res_, res);
}

extern "C" void eftsan_div(double res, double op1, double op2,
                      smem_entry *res_, smem_entry *op1_, smem_entry *op2_, size_t instId, double err, double c_err){
  m_get_error(res_, res);
}

unsigned long m_ulpd(double x, double y) {
  if (x == 0)
    x = 0; // -0 == 0
  if (y == 0)
    y = 0; // -0 == 0

  /* if (x != x && y != y) return 0; */
  if (x != x)
    return ULLONG_MAX - 1; // Maximum error
  if (y != y)
    return ULLONG_MAX - 1; // Maximum error

  long long xx = *((long long *)&x);
  xx = xx < 0 ? LLONG_MIN - xx : xx;

  long long yy = *((long long *)&y);
  yy = yy < 0 ? LLONG_MIN - yy : yy;
  return xx >= yy ? xx - yy : yy - xx;
}


extern "C" void eftsan_finish() {
  fprintf(m_errfile, "Error above %d bits found %zd\n", ERRORTHRESHOLD, errorCount);
  fprintf(m_errfile, "Total NaN found %zd\n", nanCount);
  fprintf(m_errfile, "Total Inf found %zd\n", infCount);
  fprintf(m_errfile, "Total branch flips found %zd\n", flipsCount);
  fprintf(m_errfile, "Total conversion errors found %zd\n", convCount);
  fprintf(m_errfile, "Latest relative error %.17e\n", lastRelError);
  fprintf(m_errfile, "Max relative error %.17e\n", maxRelError);
  fprintf(m_errfile, "Computed value at max relative error %.17e\n", maxRelErrorComputedVal);
  fprintf(m_errfile, "EFT error at max relative error %.17e\n", maxRelErrorEFTError);
#if STATIC
  int count = 0;
  int countInf = 0;
  int countNan = 0;
  int countBf = 0;
  for(size_t i = 0; i < num_inst; i++){
    if(m_ins_map[i] > 0){
      count++;
    }
    if(m_inf_map[i] > 0){
      countInf++;
    }
    if(m_nan_map[i] > 0){
      countNan++;
    }
    if(m_bf_map[i] > 0){
      countBf++;
    }
  }
  fprintf(m_errfile, "Error above %d bits found(dynamic) %zd\n", ERRORTHRESHOLD, errorCount);
  fprintf(m_errfile, "Error above %d bits found(static) %zd\n", ERRORTHRESHOLD, count);
  fprintf(m_errfile, "Total Inf found(dynamic) %zd\n", infCount);
  fprintf(m_errfile, "Total Inf found(static) %zd\n", countInf);
  fprintf(m_errfile, "Total NaN found(dynamic) %zd\n", nanCount);
  fprintf(m_errfile, "Total NaN found(static) %zd\n", countNan);
  fprintf(m_errfile, "Total branch flips found(dynamic) %zd\n", flipsCount);
  fprintf(m_errfile, "Total branch flips found(static) %zd\n", countBf);
  fprintf(m_errfile, "Total conversion errors found %zd\n", convCount);
#endif
  fclose(m_errfile);
}

extern "C" void eftsan_print_error(double err){
//  printf("err:%f\n", err);
}

extern "C" void eftsan_print_err(double err){
//  printf("err:%f\n", err);
}

void
print_trace (unsigned long bitsError)
{
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 4);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {

    fprintf (m_errfile, "%d bitsError @ ", bitsError);
    for (i = 0; i < size; i++)
      fprintf(m_errfile, "%s\n", strings[i]);
  }

  free (strings);
}

extern "C" void eftsan_report_inf(smem_entry *real){
    infCount++;
#if STATIC
    m_inf_map[real->lineno]++;
    /*
    if(m_inf_map[real->lineno] == 1){
      fprintf (m_errfile, "Inf detected @ ");
      print_trace (real->error);
    }
    */
#endif
}

extern "C" void eftsan_report_nan(smem_entry *real){
    nanCount++;
#if STATIC
    m_nan_map[real->lineno]++;
    /*
    if(m_nan_map[real->lineno] == 1){
      fprintf (m_errfile, "NaN detected @ ");
      print_trace (real->error);
    }
    */
#endif
}

int count = 0;
extern "C" void eftsan_get_error(smem_entry *real, double computed){
  m_update_error(real, computed);
  //eftsan_trace(real);
}


void m_get_error(smem_entry *real, double computedVal){
  /*
  if(isinf(computedVal)){
    infCount++;
  }
  else if(isnan(computedVal)){
    nanCount++;
  }
  */
  /*
  if(isinf(computedVal)){
    real->error = 0;
  }
#if TRACING
  if(real->lhs && isinf(real->lhs->computed)){
    real->error = 0;
  }
  if(real->rhs != NULL && isinf(real->rhs->computed)){
    real->error = 0;
  }
#endif
  */
  double shadowRounded = real->error + computedVal;
  unsigned long ulpsError = m_ulpd(shadowRounded, computedVal);

  double bitsError = log2(ulpsError + 1);
  if(bitsError >  ERRORTHRESHOLD){
    errorCount++;
#if STATIC
    m_ins_map[real->lineno]++;
#endif
  }

double relError = 0.0;
  if (fabs(shadowRounded) > 0.0) {
  relError = fabs(real->error) / fabs(shadowRounded);
} else if (fabs(real->error) > 0.0) {
  relError = INFINITY;
}
lastRelError = relError;

if (relError > maxRelError) {
maxRelError = relError;
maxRelErrorComputedVal = computedVal;
maxRelErrorEFTError = real->error;
}
}

int m_update_error(smem_entry *real, double computedVal){
  double shadowRounded = real->error + computedVal;
  unsigned long ulpsError = m_ulpd(shadowRounded, computedVal);

  double bitsError = log2(ulpsError + 1);
//  std::cout<<"\nbitsError:"<<bitsError<<"\n";

  if (debugerror){
    std::cout<<"\nThe shadow value is "<<real->error;
    if (computedVal != computedVal){
      std::cout<<", but NaN was computed.\n";
    } else {
      std::cout<<", but ";
      printf("%e", computedVal);
      std::cout<<" was computed.\n";
      std::cout<<"m_update_error: computedVal:"<<computedVal<<"\n";
    }
    printf("%f bits error (%lu ulps)\n",
        bitsError, ulpsError);
    std::cout<<"****************\n\n";
  }
  return bitsError;
}

void handle_math_d_long(fp_op opCode, double op1d, smem_entry *op, 
		   double computedResd, smem_entry *res,
		   unsigned int linenumber, unsigned long long int ts) {

  long double temp = 0.0;
  if(op != NULL){
    temp = op1d + op->error;
  }
  else{
    temp = op1d;
  }

  switch(opCode){
    case CBRT:
      temp = cbrt(temp);
      break;
    case FLOOR:
      temp = floor(temp);
      break;
    case CEIL:
      temp = ceil(temp);
      break;
    case TAN:
      temp = tan(temp);
      break;
    case TANH:
      temp = tanh(temp);
      break;
    case SIN:
      temp = sin(temp);
      break;
    case SINH:
      temp = sinh(temp);
      break;
    case COS:
      temp = cos(temp);
      break;
    case COSH:
      temp = cosh(temp);
      break;
    case ACOS:
      if(temp > 1){
        temp = 2^-52;
      }
      else if (temp < -1){
        temp = M_PI;
      }
      else{
        temp = acos(temp);
      }
      break;
    case ACOSH:
      if(temp < 1.0){
        temp = 2^-52;
      }
      else{
        temp = acosh(temp);
      }
      break;
    case ATAN:
      temp = atan(temp);
      break;
    case ATANH:
      if(temp > 1.0 || temp < 1.0){
        temp = -INFINITY;
      }
      else{
        temp = atanh(temp);
      }
      break;
    case ABS:
      temp = abs(temp);
      break;
    case LOG:
      if(temp < 0){
        temp = -INFINITY;
      }
      else{
        temp = log(temp);
      }
      break;
    case LOG10:
      if(temp < 0){
        temp = -INFINITY;
      }
      else{
        temp = log10(temp);
      }
      break;
    case ASIN: 
      if(temp > 1){
        temp = M_PI/2.;
      }
      else if (temp < -1){
        temp = -M_PI/2;
      }
      else{
        temp = asin(temp);
      }
      break;
    case EXP: 
      temp = exp(temp);
      break;
    default:
      std::cout<<"Error!!! Math function not supported\n\n";
      exit(1);
      break;
  }
  res->error = temp - computedResd;
  if(opCode == LOG || opCode == LOG10){
    if(temp <= 0){
      res->error = 0;
    }
  }
  res->computed = computedResd;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op; 
  res->rhs = nullptr;
  res->opcode = opCode;
  res->lineno = linenumber;
#endif
  m_get_error(res, computedResd);
}

/* Math library functions */
void handle_math_d(fp_op opCode, double op1d, smem_entry *op, 
		   double computedResd, smem_entry *res,
		   unsigned int linenumber, unsigned long long int ts) {

  double op_temp = 0.0;
  if(op != NULL){
    op_temp = op1d + op->error;
  }
  else{
    op_temp = op1d;
  }
  mpfr_set_d(temp, op_temp, MPFR_RNDN);
  switch(opCode){
    case CBRT:
      mpfr_cbrt(temp, temp, MPFR_RNDN);
      break;
    case FLOOR:
      mpfr_floor(temp, temp);
      break;
    case CEIL:
      mpfr_ceil(temp, temp);
      break;
    case TAN:
      mpfr_tan(temp, temp, MPFR_RNDN);
      break;
    case TANH:
      mpfr_tanh(temp, temp, MPFR_RNDN);
      break;
    case SIN:
      mpfr_sin(temp, temp, MPFR_RNDN);
      break;
    case SINH:
      mpfr_sinh(temp, temp, MPFR_RNDN);
      break;
    case COS:
      mpfr_cos(temp, temp, MPFR_RNDN);
      break;
    case COSH:
      mpfr_cosh(temp, temp, MPFR_RNDN);
      break;
    case ACOS:
      if(op_temp > 1){
        mpfr_set_d(temp, 2^-52, MPFR_RNDN);
      }
      else if (op_temp < -1){
        mpfr_set_d(temp, M_PI, MPFR_RNDN);
      }
      else{
        mpfr_acos(temp, temp, MPFR_RNDN);
      }
      break;
    case ACOSH:
      if(op_temp < 1.0){
        mpfr_set_d(temp, 2^-52, MPFR_RNDN);
      }
      else{
        mpfr_acosh(temp, temp, MPFR_RNDN);
      }
      break;
    case ATAN:
      mpfr_atan(temp, temp, MPFR_RNDN);
      break;
    case ATANH:
      if(op_temp > 1.0 || op_temp < 1.0){
        mpfr_set_d(temp, -INFINITY, MPFR_RNDN);
      }
      else{
        mpfr_atanh(temp, temp, MPFR_RNDN);
      }
      break;
    case ABS:
      mpfr_abs(temp, temp, MPFR_RNDN);
      break;
    case LOG:
      if(op_temp < 0){
        mpfr_set_d(temp, -INFINITY, MPFR_RNDN);
      }
      else{
        mpfr_log(temp, temp, MPFR_RNDN);
      }
      break;
    case LOG10:
      if(op_temp < 0){
        mpfr_set_d(temp, -INFINITY, MPFR_RNDN);
      }
      else{
        mpfr_log10(temp, temp, MPFR_RNDN);
      }
      break;
    case ASIN: 
      if(op_temp > 1){
        mpfr_set_d(temp, M_PI/2., MPFR_RNDN);
      }
      else if (op_temp < -1){
        mpfr_set_d(temp, -M_PI/2, MPFR_RNDN);
      }
      else{
        mpfr_asin(temp, temp, MPFR_RNDN);
      }
      break;
    case EXP: 
      mpfr_exp(temp, temp, MPFR_RNDN);
      break;
    default:
      std::cout<<"Error!!! Math function not supported\n\n";
      exit(1);
      break;
  }
  double shadowRounded = m_get_double(temp);
  res->error = shadowRounded - computedResd;
  if(opCode == LOG || opCode == LOG10){
    if(op_temp <= 0){
      res->error = 0;
    }
  }
  res->computed = computedResd;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op; 
  res->rhs = nullptr;
  res->opcode = opCode;
  res->lineno = linenumber;
#endif
  m_get_error(res, computedResd);
}

extern "C" void eftsan_mpfr_cbrt(smem_entry* op1, 
                                 double op1d,
                                 smem_entry* res, 
                                 double computedRes,
                                 unsigned long long int instId, 
                                 bool debugInfoAvail, 
                                 unsigned int linenumber, 
                                 unsigned int colnumber,
                                 unsigned long long int ts){

  handle_math_d_long(CBRT, op1d, op1, computedRes, res, linenumber, ts);  
}

extern "C" void eftsan_mpfr_floor(smem_entry* op1, 
                                 double op1d,
                                 smem_entry* res, 
                                 double computedRes,
                                 unsigned long long int instId, 
                                 bool debugInfoAvail, 
                                 unsigned int linenumber, 
                                 unsigned int colnumber,
                                 unsigned long long int ts){

  handle_math_d_long(FLOOR, op1d, op1, computedRes, res, linenumber, ts);
  
}

extern "C" void eftsan_mpfr_llvm_f(smem_entry* op1, 
                                 float op1d,
                                 smem_entry* res, 
                                 float computedRes,
                                 unsigned long long int instId, 
                                 bool debugInfoAvail, 
                                 unsigned int linenumber, 
                                 unsigned int colnumber,
                                 unsigned long long int ts){

  handle_math_d_long(FLOOR, op1d, op1, computedRes, res, linenumber, ts);
  
}

extern "C" void eftsan_mpfr_atanh(smem_entry* op1, 
				double op1d,
				smem_entry* res, 
				double computedRes,
				unsigned long long int instId, 
				bool debugInfoAvail, 
				unsigned int linenumber, 
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(ATANH, op1d, op1,  computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_tanh(smem_entry* op1, 
				double op1d,
				smem_entry* res, 
				double computedRes,
				unsigned long long int instId, 
				bool debugInfoAvail, 
				unsigned int linenumber, 
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(TANH, op1d, op1, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_tan(smem_entry* op1, 
			       double op1d,
			       smem_entry* res, 
			       double computedRes,
			       unsigned long long int instId, 
			       bool debugInfoAvail, 
			       unsigned int linenumber, 
			       unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(TAN, op1d, op1, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_acosh(smem_entry* op1, 
				double op1d,
				smem_entry* res, 
				double computedRes,
				unsigned long long int instId, 
				bool debugInfoAvail, 
				unsigned int linenumber, 
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(ACOSH, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_acos(smem_entry* op1, 
				double op1d,
				smem_entry* res, 
				double computedRes,
				unsigned long long int instId, 
				bool debugInfoAvail, 
				unsigned int linenumber, 
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(ACOS, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_cosh(smem_entry* op1, 
				double op1d,
				smem_entry* res, 
				double computedRes,
				unsigned long long int instId, 
				bool debugInfoAvail, 
				unsigned int linenumber, 
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(COSH, op1d, op1, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_cos(smem_entry* op1, 
			       double op1d,
			       smem_entry* res, 
			       double computedRes,
			       unsigned long long int instId, 
			       bool debugInfoAvail, 
			       unsigned int linenumber, 
			       unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(COS, op1d, op1, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_llvm_cos_f64(smem_entry* op1, 
			       double op1d,
			       smem_entry* res, 
			       double computedRes,
			       unsigned long long int instId, 
			       bool debugInfoAvail, 
			       unsigned int linenumber, 
			       unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(COS, op1d, op1, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_atan(smem_entry* op1, 
				double op1d,
				smem_entry* res, 
				double computedRes,
				unsigned long long int instId, 
				bool debugInfoAvail, 
				unsigned int linenumber, 
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(ATAN, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_llvm_ceil(smem_entry* op1, 
				     double op1d,
				     smem_entry* res, 
				     double computedRes,
				     unsigned long long int instId, 
				     bool debugInfoAvail, 
				     unsigned int linenumber, 
				     unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(CEIL, op1d, op1, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_llvm_floor(smem_entry* op1Idx, 
				      double op1d,
				      smem_entry* res, 
				      double computedRes,
				      unsigned long long int instId, 
				      bool debugInfoAvail, 
				      unsigned int linenumber, 
				      unsigned int colnumber,
              unsigned long long int ts){

  handle_math_d_long(FLOOR, op1d, op1Idx, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_llvm_floor_f(smem_entry* op1Idx, 
				      double op1d,
				      smem_entry* res, 
				      double computedRes,
				      unsigned long long int instId, 
				      bool debugInfoAvail, 
				      unsigned int linenumber, 
				      unsigned int colnumber,
              unsigned long long int ts){

  handle_math_d_long(FLOOR, op1d, op1Idx, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_expf(smem_entry* op1Idx, 
			   float op1d,
			   smem_entry* res, 
			   float computedRes,
			   unsigned long long int instId, 
			   bool debugInfoAvail, 
			   unsigned int linenumber, 
			   unsigned int colnumber,
         unsigned long long int ts){

  handle_math_d_long(EXP, op1d, op1Idx, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_exp(smem_entry* op1Idx, 
			   double op1d,
			   smem_entry* res, 
			   double computedRes,
			   unsigned long long int instId, 
			   bool debugInfoAvail, 
			   unsigned int linenumber, 
			   unsigned int colnumber,
         unsigned long long int ts){

  handle_math_d_long(EXP, op1d, op1Idx, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_llvm_exp64(smem_entry* op1Idx, 
			   double op1d,
			   smem_entry* res, 
			   double computedRes,
			   unsigned long long int instId, 
			   bool debugInfoAvail, 
			   unsigned int linenumber, 
			   unsigned int colnumber,
         unsigned long long int ts){

  handle_math_d_long(EXP, op1d, op1Idx, computedRes, res, linenumber, ts);

}

extern "C" void eftsan_mpfr_frexpf2(smem_entry* op1Idx, 
			   float op1d,
         int* ex,
			   smem_entry* res, 
			   float computedRes,
			   unsigned long long int instId, 
			   bool debugInfoAvail, 
			   unsigned int linenumber, 
			   unsigned int colnumber,
         unsigned long long int ts){

#if 0
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);
  mpfr_exp_t exp = 0;

  mpfr_frexp(&exp, temp, tempOp1, MPFR_RNDN);
#endif
  long double op1 = op1d + op1Idx->error;
  long double temp = 0;
  int exp = 0;

  temp = frexp(op1, &exp);
//  double shadowRounded = m_get_double(temp);

  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = nullptr;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = MIN;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_GSL_MIN_DBL2(smem_entry* op1Idx, double op1d, 
					smem_entry* op2Idx, double op2d, 
					smem_entry* res, double computedRes,
					unsigned long long int instId, 
					bool debugInfoAvail, 
					unsigned int linenumber, 
					unsigned int colnumber,
          unsigned long long int ts){
#if 0
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);

  double op2 = op2d + op2Idx->error;
  mpfr_set_d(tempOp2, op2, MPFR_RNDN);

  mpfr_min(temp, tempOp1, tempOp2, MPFR_RNDN);
  
  double shadowRounded = m_get_double(temp);
#endif
  long double op1 = op1d + op1Idx->error;

  long double op2 = op2d + op2Idx->error;

  long double temp = std::min(op1, op2);

  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = op2Idx;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = MIN;
#endif
  m_get_error(res, computedRes);
}


extern "C" void eftsan_mpfr_GSL_MAX_DBL2(smem_entry* op1Idx, double op1d, 
					smem_entry* op2Idx, smem_entry* rhs, double op2d, 
					smem_entry* res, double computedRes,
					unsigned long long int instId, 
					bool debugInfoAvail, 
					unsigned int linenumber, 
					unsigned int colnumber,
          unsigned long long int ts){
#if 0 
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);

  double op2 = op2d + op2Idx->error;
  mpfr_set_d(tempOp2, op2, MPFR_RNDN);

  mpfr_max(temp, tempOp1, tempOp2, MPFR_RNDN);
  
  double shadowRounded = m_get_double(temp);
#endif
  long double op1 = op1d + op1Idx->error;

  long double op2 = op2d + op2Idx->error;

  long double temp = std::max(op1, op2);
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = op2Idx;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = MAX;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_ldexp2(smem_entry* op1Idx, double op1d, 
				  int op2d, 
				  smem_entry* res, double computedRes,
				  unsigned long long int instId, 
				  bool debugInfoAvail, 
				  unsigned int linenumber, 
				  unsigned int colnumber,
          unsigned long long int ts){
  
  //op1*2^(op2)
#if 0
  mpfr_t exp;
  mpfr_init2(exp, m_precision);
  mpfr_set_si(exp, op2d, MPFR_RNDN);
  mpfr_exp2(temp, exp, MPFR_RNDN);
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);
  mpfr_mul(temp, tempOp1, temp,  MPFR_RNDN);
  
  mpfr_clear(exp);

  double shadowRounded = m_get_double(temp);
#endif
  //op1*2^(op2)
  long double temp = exp2(op2d);
  long double op1 = op1d + op1Idx->error;
  temp =  op1 * temp;
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = nullptr;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = LDEXP;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_fmod2(smem_entry* op1Idx, double op1d, 
				 smem_entry* op2Idx, double op2d, 
				 smem_entry* res, double computedRes,
				 unsigned long long int instId, 
				 bool debugInfoAvail, 
				 unsigned int linenumber, 
				 unsigned int colnumber,
         unsigned long long int ts){
#if 0 
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);

  double op2 = op2d + op2Idx->error;
  mpfr_set_d(tempOp2, op2, MPFR_RNDN);

  mpfr_fmod(temp, tempOp1, tempOp2, MPFR_RNDN);
  
  double shadowRounded = m_get_double(temp);
#endif
  long double op1 = op1d + op1Idx->error;

  long double op2 = op2d + op2Idx->error;

  long double temp = fmod(op1, op2);
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = op2Idx;
  res->lineno = linenumber;
  res->opcode = FMOD;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_atan22(smem_entry* op1Idx, double op1d, 
				  smem_entry* op2Idx, double op2d, 
				  smem_entry* res, double computedRes,
				  unsigned long long int instId, 
				  bool debugInfoAvail, 
				  unsigned int linenumber, 
				  unsigned int colnumber, 
          unsigned long long int ts){
  
#if 0
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);

  double op2 = op2d + op2Idx->error;
  mpfr_set_d(tempOp2, op2, MPFR_RNDN);

  mpfr_atan2(temp, tempOp1, tempOp2, MPFR_RNDN);
  
  double shadowRounded = m_get_double(temp);
#endif

  long double op1 = 0.0;
  if(op1Idx != NULL){
    op1 = op1d + op1Idx->error;
  }
  else{
    op1 = op1d;
  }

  long double op2 = 0.0;
  if(op2Idx != NULL){
    op2 = op2d + op2Idx->error;
  }
  else{
    op2 = op2d;
  }

  long double temp = atan2(op1, op2);
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = op2Idx;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = ATAN2;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_hypot2(smem_entry* op1Idx, double op1d, 
				  smem_entry* op2Idx, double op2d, 
				  smem_entry* res, double computedRes,
				  unsigned long long int instId, 
				  bool debugInfoAvail, 
				  unsigned int linenumber, 
				  unsigned int colnumber,
          unsigned long long int ts){
  
#if 0
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);

  double op2 = op2d + op2Idx->error;
  mpfr_set_d(tempOp2, op2, MPFR_RNDN);

  mpfr_hypot(temp, tempOp1, tempOp2, MPFR_RNDN);
  
  double shadowRounded = m_get_double(temp);
#endif

  long double op1 = op1d + op1Idx->error;

  long double op2 = op2d + op2Idx->error;

  long double temp = hypot(op1, op2);
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = op2Idx;
  res->lineno = linenumber;
  res->opcode = HYPOT;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_powf2(smem_entry* op1Idx, float op1d, 
				smem_entry* op2Idx, float op2d, 
				smem_entry* res, float computedRes,
				unsigned long long int instId,  
        bool debugInfoAvail,
        unsigned int linenumber,                          
        unsigned int colnumber,
        unsigned long long int ts){
 
#if 0
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);

  double op2 = op2d + op2Idx->error;
  mpfr_set_d(tempOp2, op2, MPFR_RNDN);

  mpfr_pow(temp, tempOp1, tempOp2, MPFR_RNDN);
  
  double shadowRounded = m_get_double(temp);
#endif

  long double op1 = op1d + op1Idx->error;

  long double op2 = op2d + op2Idx->error;

  long double temp = pow(op1, op2);
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = op2Idx;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = HYPOT;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_llvm_powi2(smem_entry* op1Idx, double op1d, 
				int x, smem_entry* res, double computedRes,
				unsigned long long int instId,  
        bool debugInfoAvail,
        unsigned int linenumber,                          
        unsigned int colnumber,
        unsigned long long int ts){
#if 0
  double op1 = op1d + op1Idx->error;
  mpfr_set_d(tempOp1, op1, MPFR_RNDN);

  int op2 = x;
  mpfr_set_d(tempOp2, op2, MPFR_RNDN);

  mpfr_pow(temp, tempOp1, tempOp2, MPFR_RNDN);
  
  double shadowRounded = m_get_double(temp);
#endif

  long double op1 = op1d + op1Idx->error;

  int op2 = x;

  long temp = pow(op1, op2);
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = nullptr;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = HYPOT;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_pow2(smem_entry* op1Idx, double op1d, 
				smem_entry* op2Idx, double op2d, 
				smem_entry* res, double computedRes,
				unsigned long long int instId,  
        bool debugInfoAvail,
        unsigned int linenumber,                          
        unsigned int colnumber,
        unsigned long long int ts){
 
  long double op1 = 0;
  if(op1Idx != NULL){
    op1 = op1d + op1Idx->error;
  }
  else{
    op1 = op1d;
  }
  long double op2 = 0;
  if(op2Idx != NULL){
    op2 = op2d + op2Idx->error;
  }
  else{
    op2 = op2d;
  }

  long double temp = pow(op1, op2);
  
  res->error = temp - computedRes;
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = op2Idx;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = HYPOT;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_sqrtf(smem_entry* op1Idx,
    float op1d,
    smem_entry* res,
    float computedRes,
    unsigned long long int instId,
    bool debugInfoAvail,
    unsigned int linenumber,
    unsigned int colnumber,
    unsigned long long int ts){

  double err = 0;
  if(op1Idx != NULL){
    err = op1Idx->error;
  }

  if (computedRes == 0)
  {
    if(err == 0)
    {
      res->error = 0;
    }
    else
    {
      res->error = std::sqrt(std::abs(err));
    }
  }
  else
  {
    double error = -std::fma(computedRes, computedRes, -op1d);
    res->error = (error + err) / (computedRes + computedRes);
  }
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = nullptr;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = SQRT;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_sqrt(smem_entry* op1Idx,
    double op1d,
    smem_entry* res,
    double computedRes,
    unsigned long long int instId,
    bool debugInfoAvail,
    unsigned int linenumber,
    unsigned int colnumber,
    unsigned long long int ts){

  double err = 0;
  if(op1Idx != NULL){
    err = op1Idx->error;
  }
  if (computedRes == 0)
  {
    if(err == 0)
    {
      res->error = 0;
    }
    else
    {
      res->error = std::sqrt(std::abs(err));
    }
  }
  else
  {
    double error = -std::fma(computedRes, computedRes, -op1d);
    res->error = (error + err) / (computedRes + computedRes);
  }
  
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = nullptr;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = SQRT;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_llvm_sqrt64(smem_entry* op1Idx,
    double op1d,
    smem_entry* res,
    double computedRes,
    unsigned long long int instId,
    bool debugInfoAvail,
    unsigned int linenumber,
    unsigned int colnumber,
    unsigned long long int ts){

  if (computedRes == 0)
  {
    if(op1Idx->error == 0)
    {
      res->error = 0;
    }
    else
    {
      res->error = std::sqrt(std::abs(op1Idx->error));
    }
  }
  else
  {
    double error = -std::fma(computedRes, computedRes, -op1d);
    res->error = (error + op1Idx->error) / (computedRes + computedRes);
  }
  res->computed = computedRes;
  res->timestamp = ts;
#ifdef TRACING
  res->lhs = op1Idx; 
  res->rhs = nullptr;
  res->lineno = linenumber;
  res->lineno = linenumber;
  res->opcode = SQRT;
#endif
  m_get_error(res, computedRes);
}

extern "C" void eftsan_mpfr_llvm_fabs32(smem_entry* op1, 
				     float op1d,
				     smem_entry* res, 
				     float computedRes,
				     unsigned long long int instId, 
				     bool debugInfoAvail, 
				     unsigned int linenumber, 
				     unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(ABS, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_llvm_fabs(smem_entry* op1, 
				     double op1d,
				     smem_entry* res, 
				     double computedRes,
				     unsigned long long int instId, 
				     bool debugInfoAvail, 
				     unsigned int linenumber, 
				     unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(ABS, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_abs(smem_entry* op1,
             smem_entry* res,
			       double op1d, double computedRes,
			       unsigned long long int instId,
			       bool debugInfoAvail, unsigned int linenumber,
			       unsigned int colnumber,
             unsigned long long int ts){


  handle_math_d_long(ABS, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_log10(smem_entry* op1,
             smem_entry* res,
			       double op1d, double computedRes,
			       unsigned long long int instId,
			       bool debugInfoAvail, unsigned int linenumber,
			       unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(LOG10, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_log(smem_entry* op1, 
             smem_entry* res,
			       double op1d, double computedRes,
			       unsigned long long int instId,
			       bool debugInfoAvail, unsigned int linenumber,
			       unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(LOG, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_llvm_log64(smem_entry* op1, 
             smem_entry* res,
			       double op1d, double computedRes,
			       unsigned long long int instId,
			       bool debugInfoAvail, unsigned int linenumber,
			       unsigned int colnumber,
             unsigned long long int ts){

  handle_math_d_long(LOG, op1d, op1, computedRes, res, linenumber, ts);
}
extern "C" void eftsan_mpfr_asinh(smem_entry* op1, 
        smem_entry* res,
				double op1d, double computedRes,
				unsigned long long int instId,
				bool debugInfoAvail, unsigned int linenumber,
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(ASINH, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_sinh(smem_entry* op1,
        smem_entry* res,
				double op1d, double computedRes,
				unsigned long long int instId,
				bool debugInfoAvail, unsigned int linenumber,
				unsigned int colnumber,
        unsigned long long int ts){

  handle_math_d_long(SINH, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_sin(smem_entry* op1,
             smem_entry* res,
			       double op1d, double computedRes,
			       unsigned long long int instId,
			       bool debugInfoAvail, unsigned int linenumber,
			       unsigned int colnumber,
             unsigned long long int ts){
  handle_math_d_long(SIN, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_llvm_sin_f64(smem_entry* op1,
             smem_entry* res,
			       double op1d, double computedRes,
			       unsigned long long int instId,
			       bool debugInfoAvail, unsigned int linenumber,
			       unsigned int colnumber,
             unsigned long long int ts){
  handle_math_d_long(SIN, op1d, op1, computedRes, res, linenumber, ts);
}

extern "C" void eftsan_mpfr_asin(smem_entry* op1,
				smem_entry* res,
				double op1d,
				double computedRes,
				unsigned long long int instId,
				bool debugInfoAvail,
				unsigned int linenumber,
				unsigned int colnumber,
        unsigned long long int ts){
  handle_math_d_long(ASIN, op1d, op1, computedRes, res, linenumber, ts);
}
