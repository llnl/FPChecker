#!/usr/bin/env python

import subprocess
import os
import sys
# sys.path.append('..')
# sys.path.append('.')
from dynamic import report

sys.path.insert(1, os.path.dirname(os.path.realpath(__file__)) + '/../../../../cpu_checking/histograms')

import fpc_create_exp_usage_report as histogram_plotting

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make clean"]
    cmdutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def test_exponent_usage_basic():
    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    cmd = ["FPC_EXPONENT_USAGE=1 ./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- test data ---
    fileName = report.findHistogramFile('.fpc_logs')
    json_data = report.loadReport(fileName)
    data = histogram_plotting.histogramPerLine('plots', json_data)

    for line_data in data:
        if line_data['line'] == 7:
            assert line_data['fp32'] == {}
            assert line_data['fp64']['0'] == 1
            assert line_data['fp64']['1024'] == 7

        if line_data['line'] == '10':
            assert line_data['fp32'] == {}
            assert line_data['fp64']['-1023'] == 1
            assert line_data['fp64']['1024'] == 15

    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    cmd = ["./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- test data ---
    fileName = report.findHistogramFile('.fpc_logs')
    json_data = report.loadReport(fileName)
    data, meta_data = histogram_plotting.histogramPerFile('plots', json_data)

    for file, file_data in data.items():
        if 'compute.cpp' in file:
            assert file_data['fp32'] == {}
            #assert file_data['fp64']['-1023'] == 1
            assert file_data['fp64']['0'] == 1
            assert file_data['fp64']['1024'] == 22

    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    cmd = ["./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- test data ---
    fileName = report.findHistogramFile('.fpc_logs')
    json_data = report.loadReport(fileName)
    data = histogram_plotting.histogramPerProgram('plots', json_data)

    assert data['fp32'] == {}
    #assert data['fp64']['-1023'] == 1
    assert data['fp64']['0'] == 1
    assert data['fp64']['1024'] == 22
