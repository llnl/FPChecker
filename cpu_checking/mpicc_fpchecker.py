#!/usr/bin/env python3

import os
import pathlib
import subprocess
import platform
import sys
from colors import prGreen, prCyan, prRed
from exceptions import CommandException, CompileException, EmptyFileException
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
# --- Global variables ------------------------------------------------------ #
# --------------------------------------------------------------------------- #

C_WRAPPER_NAMES   = ['mpicc', 'mpiclang', 'mpigcc']
CXX_WRAPPER_NAMES = ['mpiCC', 'mpic++', 'mpicxx', 'mpiclang++', 'mpig++']

# --------------------------------------------------------------------------- #
# --- Classes --------------------------------------------------------------- #
# --------------------------------------------------------------------------- #

class Command:
  def __init__(self, compiler_name, params):
    # We assume the command is NAME-fpchecker, where NAME is one of the
    # supported C or CXX wrapper names
    #wrapper_name = os.path.split(cmd[0])[1].split('-')[0]
    wrapper_name = compiler_name
    if (wrapper_name in C_WRAPPER_NAMES):
      self.name = "clang"
    elif (wrapper_name in CXX_WRAPPER_NAMES):
      self.name = "clang++"
    else:
      raise CompileException("Invalid MPI wrapper name")
    self.wrapper_name = wrapper_name
    #self.parameters = cmd[1:]
    self.parameters = params
    self.mpi_params = self.getMPICompileParams()
    self.mpi_link_params = self.getMPILinkParams()

  def getMPICompileParams(self):
    cmd = self.wrapper_name + ' --showme:compile'
    try:
      cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
      params = cmdOutput.decode('utf-8')
      ret = []
      for i in params.split():
        if 'rpath' not in i:
          ret.append(i)
      return ret
    except subprocess.CalledProcessError as e:
      prRed(e)

  def getMPILinkParams(self):
    cmd = self.wrapper_name + ' --showme:link'
    try:
      cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
      params = cmdOutput.decode('utf-8')
      ret = []
      for i in params.split():
        ret.append(i)
      return ret
    except subprocess.CalledProcessError as e:
      prRed(e)

  def executeOriginalCommand(self):
    try:
      cmd = [self.wrapper_name] + self.parameters
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

  def instrumentIR(self):
    if 'FPC_INSTRUMENT_ERR_TRACKING' in os.environ:
      LLVM_PASS = LLVM_PASS_ERROR
    else:
      LLVM_PASS = LLVM_PASS_EXCEPTIONS

    new_cmd = [self.name] + self.mpi_params + LLVM_PASS.split() + self.parameters
    for p in self.parameters:
      if '-fopenmp' in p:
        new_cmd += ['-DFPC_MULTI_THREADED']
    try:
      cmdOutput = subprocess.run(' '.join(new_cmd), shell=True, check=True)
    except Exception as e:
      prRed(e)
      raise CompileException(new_cmd) from e

  def linkMPI(self):
    new_cmd = [self.name] + self.mpi_link_params + self.parameters
    try:
      cmdOutput = subprocess.run(' '.join(new_cmd), shell=True, check=True)
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
    #cmd.executeOriginalCommand()
    cmd.linkMPI()
  else:
    # Compilation command
    try:
      cmd.instrumentIR()
    except Exception as e: # Fall back to original command
      logMessage(str(e))
      prRed(e)     
      logMessage('Failed: ' + ' '.join(sys.argv))
      cmd.executeOriginalCommand()

