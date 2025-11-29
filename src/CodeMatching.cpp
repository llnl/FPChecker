/*
 * CodeMatching.cpp
 *
 *  Created on: Sep 15, 2018
 *      Author: Ignacio Laguna, ilaguna@llnl.gov
 */

#include "CodeMatching.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ManagedStatic.h"

#include <map>
#include <vector>

using namespace CUDAAnalysis;
using namespace llvm;

namespace
{
  typedef std::map<std::string, std::vector<unsigned>> key_val_pair_t;
  typedef std::map<const GlobalValue *, key_val_pair_t> global_val_annot_t;
  typedef std::map<const Module *, global_val_annot_t> per_module_annot_t;
} // anonymous namespace

bool CodeMatching::isUnwantedFunction(Function *f)
{
  /// We assume all functions in the runtime begin with _FPC_ or FPC_, so we will
  /// not instrument functions that contain this
  if (f->getName().str().find("_FPC_") != std::string::npos)
    return true;
  if (f->getName().str().find("FPC_") != std::string::npos)
    return true;

  return false;
}

bool CodeMatching::isMainFunction(Function *f)
{
  return (f->getName().str().compare("main") == 0);
}
