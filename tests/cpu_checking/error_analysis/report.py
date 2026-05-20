import glob
import json
import os
import sys

def loadReport(fileName):
  f = open(fileName,'r')
  data = json.load(f)
  f.close()
  return data

# ----- Regular reports ------
def findReportFile(path):
  reports = glob.glob(path+'/fpc_*.json')
  return reports[0]

def numberReportFiles(path):
  reports = glob.glob(path+'/fpc_*.json')
  return len(reports)

# ------ Histogram reports -------
def findHistogramFile(path):
  reports = glob.glob(path+'/exponent_usage_*.json')
  return reports[0]

# ------ Rounding error reports -------
def findRoundingErrorFile(path):
  reports = glob.glob(path+'/rounding_error_*.json')
  return max(reports, key=os.path.getmtime)

# ------ Error per line reports -------
def findErrorsPerLineFile(path):
  reports = glob.glob(path+'/errors_per_line_*.json')
  return reports[0]

if __name__ == '__main__':
  fileName = sys.argv[1]
  loadReport(fileName)
