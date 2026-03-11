
import os
import tempfile
import sys
import linecache
from collections import defaultdict
from tokenizer import Tokenizer
from match import Match, FunctionType
from match import FunctionType
from deprocess import Deprocess
from fpc_logging import verbose, logMessage
from config_reader import Config

## Assumes we already pre-processed the file
class Instrument:
  def __init__(self, preprocessedFile, srcFile):
    self.preFileContent = None
    self.preFileName = preprocessedFile
    self.sourceFileName = srcFile
    self.deprocessedFile = None
    self.deviceDclLines = []
    self.allTokens = [] # contain tokens of de-preprocessed file
    self.linesOfAssigments = defaultdict(list)
    self.transformedLines = {}
    self.PRE_HOST         = '_FPC_CHECK_HD_'
    self.PRE_DEVICE       = '_FPC_CHECK_D_'
    self.PRE_HOST_DEVICE  = '_FPC_CHECK_HD_'
    self.functionTypeMap = {} # key: token, value: function type
    self.instrumentedFileName = None
    if 'FPC_CONF' in os.environ:
      self.conf = Config(os.environ['FPC_CONF'])
    else:
      self.conf = Config('fpchecker.ini')

  def __del__(self):
    if self.deprocessedFile:
      os.remove(self.deprocessedFile)

  def deprocess(self):
    tmpFd, tmpFname = tempfile.mkstemp(suffix='.txt', text=True)
    if verbose(): print('Temp file (deprocessing):', tmpFname)
    self.deprocessedFile = tmpFname
    dp = Deprocess(self.preFileName, tmpFname)
    if verbose(): print('Running de-processor...')
    dp.run()
    if verbose():
      print('... de-preprocessor done.')
      print('Deprocessed file:', self.deprocessedFile)
      with open(self.deprocessedFile, 'r') as fd:
        i = 1
        for l in fd:
          print("{n:3d}: {line}".format(n=i, line=l[:-1]))
          i += 1

    #os.close(tmpFd)
    #if 'FPC_LEAVE_TEMP_FILES' not in os.environ:
    #  os.remove(tmpFname)

  # Identify all device or host-device code regions
  def findDeviceDeclarations(self):
    t = Tokenizer(self.deprocessedFile)
    for token in t.tokenize():
      self.allTokens.append(token)
    m = Match()
    self.deviceDclLines = m.match_device_function(self.allTokens)

  ## This simply uses the entire file as a big code region
  ## Intended to be used to instrument the entire file
  def findAllDeclarations(self):
    t = Tokenizer(self.deprocessedFile)
    for token in t.tokenize():
      self.allTokens.append(token)
    #m = Match()
    startLine = self.allTokens[0].lineNumber()
    endLine = self.allTokens[-1:][0].lineNumber()
    startIndex = 0
    endIndex = len(self.allTokens) - 1
    func_type = FunctionType.host
    self.deviceDclLines  = [(startLine, endLine, startIndex, endIndex, func_type)]

  ## Add middle lines, i.e., lines in the middle of begin/end
  def addMiddleLines(self, begin_line, end_line):
    for i in range(begin_line, end_line+1):
      if i != begin_line and i != end_line:
        self.linesOfAssigments[i].append((0, 'm')) # token index doesn't matter

  ## Finds ranges of lines that contain assigments
  def findAssigments(self):
    for l in self.deviceDclLines:
      startLine, endLine, startIndex, endIndex, f_type = l # unpack lines and indexes
      m = Match()
      tokenIndexes = m.match_assigment(self.allTokens[startIndex:endIndex])
      for t in tokenIndexes:
        i_abs = startIndex + t[0]
        j_abs = startIndex + t[1]
        i_line = self.allTokens[i_abs].lineNumber()
        j_line = self.allTokens[j_abs].lineNumber()
        self.linesOfAssigments[i_line].append((i_abs, 'b'))
        self.linesOfAssigments[j_line].append((j_abs, 'e'))
        self.addMiddleLines(i_line, j_line)
        if verbose(): print('Lines with assignments:', self.linesOfAssigments)
        self.functionTypeMap[i_abs] = f_type

  ## Adds preamble and end to the operation (i.e., instruments the line)
  ## We add two things:
  ##  (1) Line number (int)
  ##  (2) Source file name (str)
  def transformLine(self, index, currentLine: int):
    beg_tokens = set([])
    end_tokens = set([])
    for elem in self.linesOfAssigments[currentLine]:
      i, kind = elem
      if kind=='b': beg_tokens.add(i)
      if kind=='e': end_tokens.add(i)

    i = index
    newLine = ''
    for token in self.allTokens[index:]:
      if i in beg_tokens:
        # Add preamble
        if self.functionTypeMap[i] == FunctionType.device: pre = self.PRE_DEVICE
        elif self.functionTypeMap[i] == FunctionType.device_host: pre = self.PRE_HOST_DEVICE
        elif self.functionTypeMap[i] == FunctionType.host_device: pre = self.PRE_HOST_DEVICE
        elif self.functionTypeMap[i] == FunctionType.host: pre = self.PRE_HOST
        newLine += pre + '('+str(self.allTokens[i])
      elif i in end_tokens:
        # Add line number
        newLine += ', ' + str(currentLine)
        # Add source file
        newLine += ', "' + self.sourceFileName + '"'
        # Add the end
        newLine += ')'+str(self.allTokens[i])
      else:
        newLine += str(self.allTokens[i])
      if str(token)=='\n':
        break
      i += 1

    return i-index, newLine

  def produceInstrumentedLines(self):
    currentLine = 1
    index = -1
    while True:
      index += 1
      if index >= len(self.allTokens):
        break

      token = self.allTokens[index]
      if str(token)=='\n':
        currentLine += 1
        if currentLine in self.linesOfAssigments.keys():
          self.transformedLines[currentLine] = '\n'
          if verbose(): print('[New Line (empty)]: ==>', self.transformedLines[currentLine])
        continue

      if currentLine in self.linesOfAssigments.keys():
        tokensConsumed, newLine = self.transformLine(index, currentLine)
        index += tokensConsumed-1
        if verbose(): print('[New Line]: ==>', newLine)
        self.transformedLines[currentLine] = newLine

  def is_omitted_line(self, file_name: str, line: int):
    return self.conf.is_line_omitted(file_name, line)

  def isLineInDeviceCode(self, line: int):
    for i in self.deviceDclLines:
      line_begin  = i[0]
      line_end    = i[1]
      if line >= line_begin and line <= line_end:
        return True
    return False

# Old version ----------------
#  def instrument(self):
#    fileName, ext = os.path.splitext(self.sourceFileName)
#    self.instrumentedFileName = fileName+'_inst'+ext
#    with open(self.sourceFileName, 'r') as fd:
#      with open(self.instrumentedFileName, 'w') as outFile:
#        l = 0
#        for line in fd:
#          l += 1
#          if l in self.transformedLines.keys():
#            if not self.is_omitted_line(self.sourceFileName, l):
#              newLine = self.transformedLines[l]
#              if verbose(): print(newLine[:-1])
#              outFile.write(newLine[:-1]+'\n')
#            else:
#              outFile.write(line[:-1]+'\n')
#          else:
#            if verbose(): print(line[:-1])
#            outFile.write(line[:-1]+'\n')

  def instrument(self):
    fileName, ext = os.path.splitext(self.sourceFileName)
    self.instrumentedFileName = fileName+'_inst'+ext
    with open(self.sourceFileName, 'r') as fd:
      with open(self.instrumentedFileName, 'w') as outFile:
        l = 0
        for line in fd:
          l += 1
          if l in self.transformedLines.keys():
            if not self.is_omitted_line(self.sourceFileName, l):
              newLine = self.transformedLines[l]
              if verbose(): print(newLine[:-1])
              outFile.write(newLine[:-1]+'\n')
            else:
              outFile.write(line[:-1]+'\n')
          else:
            if self.isLineInDeviceCode(l):
              newLine = linecache.getline(self.deprocessedFile, l)
              outFile.write(newLine[:-1]+'\n')
              if verbose(): print(newLine[:-1])
            else:
              if verbose(): print(line[:-1])
              outFile.write(line[:-1]+'\n')

  def getInstrumentedFileName(self):
    return self.instrumentedFileName

if __name__ == '__main__':
  preFileName = sys.argv[1]
  sourceFileName = sys.argv[2]
  inst = Instrument(preFileName, sourceFileName)
  inst.deprocess()
  inst.findDeviceDeclarations()
  print(inst.deviceDclLines)
  inst.findAssigments()
  inst.produceInstrumentedLines()
  inst.instrument()
