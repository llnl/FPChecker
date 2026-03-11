#!/usr/bin/env python3

import argparse
import sys
import os

def getLogFiles() -> list:
  fileList = []
  for root, dirs, files in os.walk("./"):
    for file in files:
      if file.endswith(".fpc_log.txt"):
        f = str(os.path.join(root, file))
        fileList.append(f)
        #print(f)
  return fileList

def removeFiles(fileList: list):
  for f in fileList:
    print('Removing:', f)
    os.remove(f)

def getCommandsStatus(fileList: list):
  proc = 0
  failed = 0
  failed_list = []
  for f in fileList:
    with open(f, 'r') as fd:
      for line in fd:
        if line.startswith('Instrumented'):
          proc += 1
        elif line.startswith('Failed:'):
          failed += 1
          failed_list.append(line.split(':')[1].strip())

  return proc, failed, failed_list

def report(proc: int, failed: int):
  print('===== FPChcecker Report =====')
  print('Processed files:', proc)
  print('Failed:', failed)

def reportFailed(failed_list: list):
    print('===== FPChcecker Report =====')
    print('The following commands failed:\n')
    for l in failed_list:
      print(l)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='FPChecker reporting tool')
  parser.add_argument('-r', '--remove', action='store_true', help='Remove log files.')
  parser.add_argument('-f', '--failed', action='store_true', help='Show commands that failed.')
  args = parser.parse_args()

  fileList = getLogFiles()

  if len(sys.argv) > 1:
    if args.remove:
      removeFiles(fileList)
    if args.failed:
      _, _, failed_list = getCommandsStatus(fileList)
      reportFailed(failed_list)

  else:
    proc, failed, _ = getCommandsStatus(fileList)
    report(proc, failed)
