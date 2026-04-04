#!/usr/bin/env python3

# Description: This script replaces the clang or clang++ command.
#              It adds the flags required to load the LLVM pass
#              and include the runtime header file.

import os
import pathlib
import subprocess
import platform
import sys
from colors import prRed
from exceptions import CompileException
from fpc_logging import logMessage, verbose

# --------------------------------------------------------------------------- #
# --- Installation Paths ---------------------------------------------------- #
# --------------------------------------------------------------------------- #

# Main installation path
FPCHECKER_PATH      = str(pathlib.Path(__file__).parent.absolute())

# Library and runtime depends on the analysis type
if platform.system() == 'Darwin':
  FPCHECKER_LIB_EXCEPTIONS  = FPCHECKER_PATH+'/../lib/libfpchecker_cpu.dylib'
  FPCHECKER_LIB_ERROR       = FPCHECKER_PATH+'/../lib/libfpchecker_error.dylib'
else:
  FPCHECKER_LIB_EXCEPTIONS  = FPCHECKER_PATH+'/../lib/libfpchecker_cpu.so'
  FPCHECKER_LIB_ERROR       = FPCHECKER_PATH+'/../lib/libfpchecker_error.so'

FPCHECKER_RUNTIME_EXCEPTIONS   = FPCHECKER_PATH+'/../src/Runtime_cpu.h'
FPCHECKER_RUNTIME_ERROR        = FPCHECKER_PATH+'/../src/Runtime_error.h'

LLVM_PASS_EXCEPTIONS           = "-fpass-plugin=" + FPCHECKER_LIB_EXCEPTIONS + " -include " + FPCHECKER_RUNTIME_EXCEPTIONS + ' -g '
LLVM_PASS_ERROR                = "-fpass-plugin=" + FPCHECKER_LIB_ERROR + " -include " + FPCHECKER_RUNTIME_ERROR + ' -g -fno-vectorize -fno-slp-vectorize '

# --------------------------------------------------------------------------- #
# --- Classes --------------------------------------------------------------- #
# --------------------------------------------------------------------------- #

class Command:
  def __init__(self, compiler_name, params):
    self.name = compiler_name
    self.parameters = params
    self.preprocessedFile = None
    self.instrumentedFile = None
    self.outputFile = None

  def executeOriginalCommand(self):
    try:
      cmd = [self.name] + self.parameters
      if verbose(): print('Executing original command:', ' '.join(cmd))
      subprocess.run(' '.join(cmd), shell=True, check=True)
    except subprocess.CalledProcessError as e:
      prRed(e)

  def getOriginalCommand(self):
    return ' '.join([self.name] + self.parameters[1:])

  # It it a compilation command?
  def isCompileCommand(self) -> bool:
    if ('-c' in self.parameters):
      return True
    return False

  # Is the command a link command?
  def isLinkCommand(self) -> bool:
    if ('-c' not in self.parameters and 
        '--compile' not in self.parameters and
        ('-o' in self.parameters or '--output-file' in self.parameters)):
      return True
    return False

  def getOutputFileIfExists(self):
    for i in range(len(self.parameters)):
      p = self.parameters[i]
      if p == '-o' or p == '--output-file':
        self.outputFile = self.parameters[i+1]
        return self.parameters[i+1]
    return None

  def instrumentIR(self):
    if 'FPC_INSTRUMENT' in os.environ:
      LLVM_PASS = LLVM_PASS_EXCEPTIONS
    elif 'FPC_INSTRUMENT_ERR_TRACKING' in os.environ:
      LLVM_PASS = LLVM_PASS_ERROR
    else:
      prRed("Error: No instrumentation type specified!")
  
    new_cmd = [self.name] + LLVM_PASS.split() + self.parameters
    for p in self.parameters:
      if '-fopenmp' in p:
        new_cmd += ['-DFPC_MULTI_THREADED']
    try:
      if verbose(): print('Executing:', ' '.join(new_cmd))
      subprocess.run(' '.join(new_cmd), shell=True, check=True)
    except Exception as e:
      prRed(e)
      raise CompileException(new_cmd) from e

if __name__ == '__main__':
  compiler_name = os.environ['FPC_COMPILER']
  params = os.environ['FPC_COMPILER_PARAMS']
  cmd = Command(compiler_name, params.split())
  
  if 'FPC_INSTRUMENT' not in os.environ and 'FPC_INSTRUMENT_ERR_TRACKING' not in os.environ:
    cmd.executeOriginalCommand()
    exit()
  
  if 'FPC_INSTRUMENT' in os.environ and 'FPC_INSTRUMENT_ERR_TRACKING' in os.environ:
    prRed("Error: FPC_INSTRUMENT and FPC_INSTRUMENT_ERR_TRACKING are set! Only one instrumentation type can be used at a time.")
    exit()

  # Link command
  if cmd.isLinkCommand():
    cmd.executeOriginalCommand()
  else:
    # Compilation command
    try:
      cmd.instrumentIR()
    except Exception as e: # Fall back to original command
      logMessage(str(e))
      prRed(e)     
      logMessage('Failed: ' + ' '.join(sys.argv))
      cmd.executeOriginalCommand()

