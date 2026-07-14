import sys
import os
import shutil
import subprocess

def PrintFound() :
    print(u"\u001b[1mFOUND\u001b[0m ", end="")
def PrintExpected() :
    print(u"(\u001b[32mexpected\u001b[0m)")
def PrintUnexpected() :
    print(u"(\u001b[31munexpected\u001b[0m)")

objDir = "correctness_obj"
commonFilesPath = "makefiles"
commonFilesInCorrectness = ["mathFunc.txt", "Makefile"]
totBenchmarks = 0
totCataCancelCount = 0
totBranchFlipCount = 0
totCastErrorCount = 0
totNaNErrorCount = 0
totRErrorCount = 0
totInfCount = 0
unexpectedCount = 0

correctnessPath = "src"
if len(sys.argv) == 2 :
    correctnessPath = str(sys.argv[1])

# Create a separate obj folder to execute correctness test
if not os.path.exists(objDir) :
    os.makedirs(objDir)
    
for path, dirs, files in os.walk(correctnessPath) :
    for filename in files:
#        if filename.endswith(".c") or filename.endswith(".cpp"):
        if filename.endswith(".c"):
        #if filename.endswith(".cpp"):
            totBenchmarks = totBenchmarks + 1
            fileBaseName = os.path.splitext(os.path.basename(filename))[0]
            print("Running test: " + fileBaseName)
            # Create a directory with the filename in correctness_obj
            newDir = os.path.join(objDir, fileBaseName)
            expectedPath = os.path.join("correctness_test_expected", fileBaseName)
            if not os.path.exists(newDir) :
                os.makedirs(newDir)
                
            # copy the c file to the new directory
            shutil.copyfile(os.path.join(path, filename), os.path.join(newDir, filename))

            for commonFiles in commonFilesInCorrectness :
                shutil.copyfile(os.path.join(commonFilesPath, commonFiles), os.path.join(newDir, commonFiles))

            proc = subprocess.Popen(["make"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=newDir)
            proc.communicate()
            proc = subprocess.Popen(["./" + fileBaseName + ".fp.o"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=newDir)
            proc.communicate()

            errorLog = open(os.path.join(newDir, "error.log"), "r")
            logContent = errorLog.readlines()
            expectedExists = False
            if os.path.exists(expectedPath):
                expectedLog = open(os.path.join(expectedPath, "error.log"), "r")
                expectedContent = expectedLog.readlines()
                expectedExists = True

            # Did we catch Rounding as expected?
            foundRCount = int(logContent[0].split()[5])
            if foundRCount > 0:
                totRErrorCount = totRErrorCount + 1
            if expectedExists :
                expectedRCount = int(expectedContent[0].split()[5])

            # Did we catch NaN as expected?
            foundNaNCount = int(logContent[1].split()[3])
            if foundNaNCount > 0:
                totNaNErrorCount = totNaNErrorCount + 1
            if expectedExists :
                expectedNaRCount = int(expectedContent[1].split()[3])

            # Did we catch Inf as expected?
            foundInfCount = int(logContent[2].split()[3])
            if foundInfCount > 0:
                totInfCount = totInfCount + 1
            if expectedExists :
                expectedInfCount = int(expectedContent[2].split()[3])

            # Did we catch branch flip as expected?
            foundBrFlipCount = int(logContent[3].split()[4])
            if foundBrFlipCount > 0:
                totBranchFlipCount = totBranchFlipCount + 1
            if expectedExists :
                expectedBrFlipCount = int(expectedContent[3].split()[4])
'''
            # Did we catch catastrophic cancellation as expected?
            foundCataCancelCount = int(logContent[4].split()[4])
            print("\tCatastrophic Cancellation: ", end="")
            if foundCataCancelCount > 0 :
                totCataCancelCount = totCataCancelCount + 1
                PrintFound()
            else :
                print("NOT found ", end="")
            if expectedExists :
                expectedCataCancelCount = int(expectedContent[4].split()[4])
                if foundCataCancelCount == expectedCataCancelCount :
                    PrintExpected()
                else :
                    PrintUnexpected()
            else :
                print()
            # Did we catch Inf as expected?
            foundInfCount = int(logContent[2].split()[3])
            print("\tInf: ", end="")
            if foundInfCount > 0:
                totInfCount = totInfCount + 1
                PrintFound()
            else :
                print("NOT found ", end="")
            if expectedExists :
                expectedInfCount = int(expectedContent[2].split()[3])
                if foundInfCount == expectedInfCount:
                    PrintExpected()
                else :
                    PrintUnexpected()
                    unexpectedCount = unexpectedCount + 1
            else :
                print()

            # Did we catch branch flip as expected?
            foundBrFlipCount = int(logContent[3].split()[4])
            print("\tBranch Flip: ", end="")
            if foundBrFlipCount > 0:
                totBranchFlipCount = totBranchFlipCount + 1
                PrintFound()
            else :
                print("NOT found ", end="")
            if expectedExists :
                expectedBrFlipCount = int(expectedContent[3].split()[4])
                if foundBrFlipCount == expectedBrFlipCount:
                    PrintExpected()
                else :
                    PrintUnexpected()
                    unexpectedCount = unexpectedCount + 1
            else :
                print()

            errorLog.close()
            if expectedExists :
                expectedLog.close()
'''

print("Total number of benchmarks: " + str(totBenchmarks), end="")
if (totBenchmarks == 46) :
    PrintExpected()
else :
    PrintUnexpected()
    
print("Number of benchmarks with high rounding error: " + str(totRErrorCount), end="")
if (totRErrorCount == 23) :
    PrintExpected()
else :
    PrintUnexpected()

print("Number of benchmarks with NaN computation: " + str(totNaNErrorCount), end="")
if (totNaNErrorCount == 0) :
    PrintExpected()
else :
    PrintUnexpected()
print("Number of benchmarks with Inf computation: " + str(totInfCount), end="")
if (totInfCount == 2) :
    PrintExpected()
else :
    PrintUnexpected()
print("Number of benchmarks with branch flip: " + str(totBranchFlipCount), end="")
if (totBranchFlipCount == 5) :
    PrintExpected()
else :
    PrintUnexpected()
print("Number of benchmarks with integer conversion error: " + str(totCastErrorCount), end="")
if (totCastErrorCount == 0) :
    PrintExpected()
else :
    PrintUnexpected()
