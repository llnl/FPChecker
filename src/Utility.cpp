/*
 * Utility.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 */
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "CommonTypes.h"
#include "Utility.h"

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;

namespace CUDAAnalysis
{

  void printMessage(const char *s)
  {
    // errs() << "[ERR_INJ] " << s << "\n";
  }

  std::string inst2str(const Instruction *i)
  {
    std::string s;
    raw_string_ostream rso(s);
    i->print(rso);
    return "{" + rso.str() + "}";
  }

  /// Old Method to get debug information
  /*
  std::string getInstructionInformation(const Instruction *i)
  {
    std::stringstream lineStr("");

    if (DILocation *Loc = i->getDebugLoc()) // only true if dbg info exists
    {
      unsigned Line = Loc->getLine();
      StringRef File = Loc->getFilename();
      StringRef Dir = Loc->getDirectory();
      lineStr << Dir.str() << "/" << File.str() << ":"
          << NumberToString<unsigned>(Line);
    }
    else
    {
      lineStr << "NONE";
    }

    return lineStr.str().c_str();
  }
  */

  /// Get a list of 3 instruction: <a, b, c>
  /// a: current instruction
  /// b: previous instruction (of a)
  /// c: next instruction (of a)
  static InstVector getListOfSurroundingInstructions(const Instruction *i)
  {
    InstVector instructionVector;

    // Return empty list if the instruction is null
    if (!i)
      return instructionVector;

    const BasicBlock *parentBB = i->getParent(); // Get the parent BasicBlock
    if (!parentBB)
    {
      // Return an empty vector if the instruction has no parent BasicBlock
      return instructionVector;
    }

    // Add the current instruction
    instructionVector.push_back(i);

    // Get the previous instruction
    // Check if the instruction is not the first in the BasicBlock
    if (i != &parentBB->front())
    {
      instructionVector.push_back(i->getPrevNode());
    }
    else
    {
      instructionVector.push_back(
          nullptr); // Add nullptr if there is no previous instruction
    }

    // Get the next instruction
    // Check if the instruction is not the last in the BasicBlock
    if (i != &parentBB->back())
    {
      instructionVector.push_back(i->getNextNode());
    }
    else
    {
      instructionVector.push_back(
          nullptr); // Add nullptr if there is no next instruction
    }

    return instructionVector;
  }

  /// Find the best possible debug location:
  /// (1) First, we try on the instruction or it's operands.
  /// (2) If not found, we try on the previous instruction.
  /// (3) If not found, we try on the next instruction.
  /// If no debug info can be found, just return the current one
  /// (which will be empty).
  static DebugLoc findBestDebugLocation(const Instruction *i)
  {
    if (!i)
      return DebugLoc();

    InstVector instructions = getListOfSurroundingInstructions(i);
    for (const Instruction *inst : instructions)
    {
      if (inst)
      {
        DebugLoc Empty;
        if (inst->getDebugLoc() != Empty)
          return inst->getDebugLoc();

        for (const Use &Op : inst->operands())
        {
          if (Instruction *OpInst = dyn_cast<Instruction>(Op))
            if (OpInst->getDebugLoc() != Empty)
              return OpInst->getDebugLoc();
        }
      }
    }
    return i->getDebugLoc();
  }

  /// Get line of code for an instruction (returns integer)
  int getLineOfCode(const Instruction *i)
  {
    int ret = -1;
    // --------- Old version -------------
    // if (DILocation *loc = i->getDebugLoc()) // only true if dbg info exists
    //{
    //  ret = (int)loc->getLine();
    //}
    // -----------------------------------

    DebugLoc debugLoc = findBestDebugLocation(i);
    if (debugLoc)
    {
      ret = debugLoc.getLine();
    }

    return ret;
  }

  // Iterates on instructions, and when it finds the first
  // floating-point, it gets the
  // getFileNameFromInstruction() using that instruction
  std::string getFileNameFromFunction(const Function *f)
  {
    std::string fileName = "UNKNOWN-FILE";
    if (!f)
      return fileName;

    // Prefer the function's own DISubprogram (this avoids picking up inlined callee debug info).
    if (DISubprogram *fsp = f->getSubprogram())
    {
      if (DIFile *file = fsp->getFile())
      {
        errs() << "\t........ (1) Using DISubprogram file for function " << f->getName() << "\n";
        return file->getDirectory().str() + "/" + file->getFilename().str();
      }
    }

    // Fallback: scan for the first FP instruction but ignore debug locations that come from inlined functions.
    DISubprogram *funcSP = f->getSubprogram();
    for (const BasicBlock &BB : *f)
    {
      for (const Instruction &I : BB)
      {
        if (!I.getType()->isFloatingPointTy())
          continue;

        DebugLoc dl = findBestDebugLocation(&I);
        if (!dl)
          continue;

        // If the debug scope is a DISubprogram, accept it only if it belongs to this Function.
        if (MDNode *scopeNode = dl.getScope())
        {
          if (DISubprogram *instSP = dyn_cast<DISubprogram>(scopeNode))
          {
            // If this instruction's subprogram debug info matches our function, use it.
            if (funcSP && instSP == funcSP)
            {
              if (DIFile *file = instSP->getFile())
              {
                errs() << "\t........ (2) Using instruction's DISubprogram file for function " << f->getName() << "\n";
                return file->getDirectory().str() + "/" + file->getFilename().str();
              }
            }
            // Otherwise it's likely inlined from another function; skip.
            continue;
          }

          // If the debug location itself is a DILocation, skip if it's an inlined location.
          if (DILocation *dil = dyn_cast<DILocation>(dl.getAsMDNode()))
          {
            if (dil->getInlinedAt())
              continue; // this instruction was inlined from somewhere else
          }

          // Otherwise try to get the file from a generic DIScope (e.g. lexical block).
          if (DIScope *diScope = dyn_cast<DIScope>(scopeNode))
          {
            if (DIFile *file = diScope->getFile())
            {
              errs() << "\t........ (3) Using generic DIScope file for function " << f->getName() << "\n";
              return file->getDirectory().str() + "/" + file->getFilename().str();
            }
          }
        }
      }
    }

    return fileName;
  }

  std::string getFileNameFromModule(const Module *mod)
  {

    if (!mod)
      return "UNKNOWN-MODULE";

    if (auto *NMD = mod->getNamedMetadata("llvm.dbg.cu"))
    {
      for (unsigned i = 0; i < NMD->getNumOperands(); ++i)
      {
        if (MDNode *node = NMD->getOperand(i))
        {
          if (auto *cu = dyn_cast<DICompileUnit>(node))
          {
            if (DIFile *file = cu->getFile())
              return file->getDirectory().str() + "/" + file->getFilename().str();
          }
        }
      }
    }

    // Fallback to module identifier if no debug compile unit/file was found.
    return mod->getModuleIdentifier();
  }

  std::string getFileNameFromInstruction(const Instruction *i)
  {
    std::stringstream lineStr("");

    DebugLoc debugLoc = findBestDebugLocation(i);
    if (debugLoc)
    {
      // Get the MDNode representing the scope
      MDNode *scopeNode = debugLoc.getScope();

      if (!scopeNode)
      {
        lineStr << "Unknown";
      }

      // Try to cast the scope to DIScope
      if (auto *diScope = dyn_cast<DIScope>(scopeNode))
      {
        // Get the file from the DIScope
        DIFile *file = diScope->getFile();

        if (file)
        {
          // Get the filename (you might want to get the full path)
          StringRef File = file->getFilename();
          StringRef Dir = file->getDirectory();
          lineStr << Dir.str() << "/" << File.str();
        }
        else
        {
          // File information not available in DIScope
          lineStr << "Unknown";
        }
      }
      else
      {
        lineStr << "Unknown";
      }
    }
    else
    {
      lineStr << "Unknown";
    }

    return lineStr.str().c_str();
  }

  /*   std::string getFileNameFromModule(const Module *mod)
    {
      return mod->getModuleIdentifier();
    }
   */
  bool mayModifyMemory(const Instruction *i)
  {
    return (isa<StoreInst>(i) || isa<AtomicCmpXchgInst>(i) ||
            isa<AtomicRMWInst>(i));
  }

  void tokenize(const std::string &str, std::vector<std::string> &tokens,
                const std::string &delimiters)
  {
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
      // Found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(delimiters, pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(delimiters, lastPos);
    }
  }

  bool isFunctionUnwanted(const std::string &str)
  {
    if (str.find("_LOG_FLOATING_POINT_OP_") != std::string::npos)
      return true;
    if (str.find("dumpShadowValues") != std::string::npos)
      return true;
    if (str.find("_PRINT_TABLE_") != std::string::npos)
      return true;
    return false;
  }

  void stop()
  {
    char input;
    scanf("%c", &input);
  }

} // namespace CUDAAnalysis
