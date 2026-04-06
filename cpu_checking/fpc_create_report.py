#!/usr/bin/env python3

# Description: This script creates an html report of all the events.
#              It assumes that event (json) files are created by each 
#              MPI process indepdently. 

import os
import argparse
import sys
import json
from html import escape
from collections import defaultdict
import shutil 
from line_highlighting import createHTMLCode, createHTMLCode_with_errors
from colors import prGreen, prCyan, prRed
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import math

# -------------------------------------------------------- #
# Insertion points
# -------------------------------------------------------- #
P_INFINITY_POS = '<!-- INFINITY_POS -->'
P_INFINITY_NEG = '<!-- INFINITY_NEG -->'
P_NAN = '<!-- NAN -->'
P_DIV_ZERO = '<!-- DIV_ZERO -->'
P_CANCELLATION = '<!-- CANCELLATION -->'
P_COMPARISON = '<!-- COMPARISON -->'
P_UNDERFLOW = '<!-- UNDERFLOW -->'
P_LATENT_INFINITY_POS = '<!-- LATENT_INFINITY_POS -->'
P_LATENT_INFINITY_NEG = '<!-- LATENT_INFINITY_NEG -->'
P_LATENT_UNDERFLOW = '<!-- LATENT_UNDERFLOW -->'
P_CODE_PATHS = '<!-- CODE_PATHS -->'
P_FILES_AFFECTED = '<!-- FILES_AFFECTED -->'
P_LINES_AFFECTED = '<!-- LINES_AFFECTED -->'
P_REPORT_TITLE = '<!-- REPORT_TITLE -->'
P_FP64_HISTOGRAM = '<!-- FP64_HISTOGRAM -->'
P_FP32_HISTOGRAM = '<!-- FP32_HISTOGRAM -->'
P_FP32_INSTRUCTIONS = '<!-- FP32_INSTRUCTIONS -->'
P_FP64_INSTRUCTIONS = '<!-- FP64_INSTRUCTIONS -->'
P_ERROR_LINE = '<!-- ERROR_LINE -->'
P_ROUNDING_ERRORS_TABLES = '<!-- ROUNDING_ERRORS_TABLES -->'
P_FILE_ERROR_TRACKING = '<!-- FILE_ERROR_TRACKING -->'
P_ERRORS_PER_LINE_PLOTS = '<!-- ERRORS_PER_LINE_PLOTS -->'

# -------------------------------------------------------- #
# PATHS
# -------------------------------------------------------- #

TRACES_DIR = '.fpc_logs'
REPORTS_DIR = './fpc-report'
ROOT_REPORT_NAME = 'index.html'
THIS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_REPORT_TEMPLATE_DIR = THIS_DIR+'/../cpu_checking/report_templates'
ROOT_REPORT_TEMPLATE = ROOT_REPORT_TEMPLATE_DIR+'/index.html' 
EVENT_REPORT_TEMPLATE = ROOT_REPORT_TEMPLATE_DIR+'/event_report_template.html'
SOURCE_REPORT_TEMPLATE = ROOT_REPORT_TEMPLATE_DIR+'/source_report_template.html' 

# -------------------------------------------------------- #
# Globals
# -------------------------------------------------------- #
report_title = ""
events = defaultdict(lambda: defaultdict(list) )
program_inputs = defaultdict(set)
fp64_bin_values = {}
fp32_bin_values = {}
fp64_plot_filename = 'histogram_fp64.svg'
fp32_plot_filename = 'histogram_fp32.svg'
fp64_exp_usage_per_file = defaultdict(int)
fp32_exp_usage_per_file = defaultdict(int)
rounding_errors_per_file_line = defaultdict(lambda: defaultdict(list) ) # ['file'][line] = [1.2e-6, 3.2e-9]
relative_errors_per_line = defaultdict(list) # [line] = [1.2e-6, 3.2e-9, ...], where line is an integer

def getEventFilePaths(p):
  fileList = []
  for root, dirs, files in os.walk(p):
    for file in files:
      fileName = os.path.split(file)[1]
      if fileName.startswith('fpc_') and fileName.endswith(".json"):
        f = str(os.path.join(root, file))
        fileList.append(f)
  return fileList

def getExponentUsageFilePaths(p):
  fileList = []
  for root, dirs, files in os.walk(p):
    for file in files:
      fileName = os.path.split(file)[1]
      if fileName.startswith('exponent_usage_') and fileName.endswith(".json"):
        f = str(os.path.join(root, file))
        fileList.append(f)
  return fileList

def getErrorFilePaths(p):
  fileList = []
  for root, dirs, files in os.walk(p):
    for file in files:
      fileName = os.path.split(file)[1]
      if fileName.startswith('rounding_error_') and fileName.endswith(".json"):
        f = str(os.path.join(root, file))
        fileList.append(f)
  return fileList

def getErrorsPerLineFilePaths(p):
  fileList = []
  for root, dirs, files in os.walk(p):
    for file in files:
      fileName = os.path.split(file)[1]
      if fileName.startswith('errors_per_line_') and fileName.endswith(".json"):
        f = str(os.path.join(root, file))
        fileList.append(f)
  return fileList

def loadReport(fileName):
  f = open(fileName,'r')
  data = json.load(f)
  f.close()
  return data

def loadEvents(files):
  for f in files:
    data = loadReport(f)
    for i in range(len(data)):
      p_input         = data[i]['input']
      fileName        = data[i]['file']
      line            = data[i]['line']
      positive_infinity    = data[i]['infinity_pos']
      negative_infinity    = data[i]['infinity_neg']
      nan             = data[i]['nan']
      division_by_zero   = data[i]['division_zero']
      cancellation    = data[i]['cancellation']
      comparison      = data[i]['comparison']
      underflow       = data[i]['underflow']
      latent_positive_infinity = data[i]['latent_infinity_pos']
      latent_negative_infinity = data[i]['latent_infinity_neg']
      latent_underflow    = data[i]['latent_underflow']

      if positive_infinity != int(0):
        events['positive_infinity'][fileName].append((line,positive_infinity))
        program_inputs['positive_infinity'].add(p_input)
      if negative_infinity != int(0):
        events['negative_infinity'][fileName].append((line,negative_infinity))
        program_inputs['negative_infinity'].add(p_input)
      if nan != int(0): 
        events['nan'][fileName].append((line,nan))
        program_inputs['nan'].add(p_input)
      if division_by_zero != int(0): 
        events['division_by_zero'][fileName].append((line,division_by_zero))
        program_inputs['division_by_zero'].add(p_input)
      if cancellation != int(0):
        events['cancellation'][fileName].append((line,cancellation))
        program_inputs['cancellation'].add(p_input)
      if comparison != int(0): 
        events['comparison'][fileName].append((line,comparison))
        program_inputs['comparison'].add(p_input)
      if underflow != int(0): 
        events['underflow'][fileName].append((line,underflow))
        program_inputs['underflow'].add(p_input)
      if latent_positive_infinity != int(0): 
        events['latent_positive_infinity'][fileName].append((line,latent_positive_infinity))
        program_inputs['latent_positive_infinity'].add(p_input)
      if latent_negative_infinity != int(0): 
        events['latent_negative_infinity'][fileName].append((line,latent_negative_infinity))
        program_inputs['latent_negative_infinity'].add(p_input)
      if latent_underflow != int(0): 
        events['latent_underflow'][fileName].append((line,latent_underflow))
        program_inputs['latent_underflow'].add(p_input)

def getEvents(event_type):
  n = 0
  for f in events[event_type]:
    for l in events[event_type][f]:
      n += int(l[1])
  return n

def getCodePaths():
  files = set([])
  for e in events:
    for f in events[e]:
      files.add(f)
  if len(files) == 0:
    return "Not found"

  return os.path.commonpath(list(files))

def getFilesAffected():
  files = set([])
  for e in events:
    for f in events[e]:
      files.add(f)
  return len(files)

def getLinesAffected():
  lines = set([])
  for e in events:
    for f in events[e]:
      for t in events[e][f]:
        lines.add((f, t[0]))
  return len(lines)

# Approximate ranges for FP precision:
# FP64: 10^-324, 10^+308
# FP32: 10^-45, 10^+38
def loadExponentUsageTraces(files):
    fp64_min_exp = -330
    fp64_max_exp = 330
    fp32_min_exp = -45
    fp32_max_exp = 45

    for key in range(fp64_min_exp, fp64_max_exp + 1):
        fp64_bin_values[key] = 0
    for key in range(fp32_min_exp, fp32_max_exp + 1):
        fp32_bin_values[key] = 0
    
    for f in files:
        data = loadReport(f)
        for i in range(len(data)):
            file_name       = data[i]['file']
            fp32            = data[i]['fp32']
            fp64            = data[i]['fp64']

            for k in fp32:
                value = fp32[k]
                exponent_base2 = int(k)
                exp_base10 = math.floor(math.log10(2)*exponent_base2)
                fp32_bin_values[exp_base10] += value
                fp32_exp_usage_per_file[file_name] += value
            for k in fp64:
                value = fp64[k]
                exponent_base2 = int(k)
                exp_base10 = math.floor(math.log10(2)*exponent_base2)
                fp64_bin_values[exp_base10] += value
                fp64_exp_usage_per_file[file_name] += value

def loadRoundingErrorTraces(files):
  for f in files:
      data = loadReport(f)
      for i in range(len(data)):
          file_name       = data[i]['file']
          line            = data[i]['line']
          error           = data[i]['error']
          relative_error  = data[i]['relative_error']
          if not rounding_errors_per_file_line[file_name][line]:
            rounding_errors_per_file_line[file_name][line] = [0.0, 0.0]
          current_errors = [rounding_errors_per_file_line[file_name][line][0], rounding_errors_per_file_line[file_name][line][1]]
          if abs(error) > current_errors[0]:
            current_errors[0] = error
          if relative_error > current_errors[1]:
            current_errors[1] = relative_error
          rounding_errors_per_file_line[file_name][line] = current_errors

def loadErrorsPerLineTraces(files):
  for f in files:
      data = loadReport(f)
      for i in range(len(data)):
          line            = data[i]['line']
          errors_list    = data[i]['values'] # list of errors for that line
          relative_errors_per_line[line].extend(errors_list)

def plot_exp_usage_bars(data_dict, group_size, filename):
    data_points = list(data_dict.keys())
    counts = list(data_dict.values())

    # Convert to percentages
    total_sum = sum(counts)
    if total_sum == 0:
        percentages = [0.0] * len(counts) #Avoid division by zero
    else:
        percentages = [(quantity / total_sum) * 100 for quantity in counts]

    if group_size <= 0:
        raise ValueError("group_size must be a positive integer.")

    grouped_data_points = []
    grouped_counts = []

    for i in range(0, len(data_points), group_size):
        group_keys = data_points[i:i + group_size]
        group_values = percentages[i:i + group_size]

        middle_index = len(group_keys) // 2
        grouped_data_points.append(group_keys[middle_index])
        grouped_counts.append(sum(group_values))

    plt.figure(figsize=(10, 3))
    plt.bar(grouped_data_points, grouped_counts, width=3, edgecolor = 'gray')
    plt.xlabel(r'Exponent: $10^x$', fontsize=14)
    plt.ylabel("%", fontsize=14)
    #plt.title(title)
    
    # Draw Limit lines
    if 300 in data_dict.keys(): # print if it's the fp64 data
        plt.axvline(x=308, color='red', linestyle='--')
        #plt.axvline(x=-324, color='red', linestyle='--')
        plt.axvline(x=-308, color='red', linestyle='--')
    plt.axvline(x=38, color='gray', linestyle='--')
    #plt.axvline(x=-45, color='gray', linestyle='--')
    plt.axvline(x=-38, color='gray', linestyle='--')

    # Layout & format
    plt.xticks(rotation=45, ha="right") # rotate x axis labels for better readability
    plt.tick_params(axis='x', labelsize=12)  # Change font size for x-axis ticks
    plt.tick_params(axis='y', labelsize=12)  # Change font size for y-axis ticks
    plt.tight_layout() # to prevent labels from being cut off.
    def one_decimal(x, pos): # Format the y-axis to show one decimal place
        return f'{x:.1f}'
    formatter = mticker.FuncFormatter(one_decimal)
    plt.gca().yaxis.set_major_formatter(formatter)
    plt.grid(True)
    plt.savefig(filename, format='svg')
    #plt.show()

#------------------------------------------------------------------------------
#------------------------- Text Reports ---------------------------------------
#------------------------------------------------------------------------------

def createRootReport_Text():
  print('\n')
  print('{:=^50}'.format(' Main Report '))
  print('{:<30}'.format('positive_infinity'), getEvents('positive_infinity'))
  print('{:<30}'.format('negative_infinity'), getEvents('negative_infinity'))
  print('{:<30}'.format('nan'), getEvents('nan'))
  print('{:<30}'.format('division_by_zero'), getEvents('division_by_zero'))
  print('{:<30}'.format('cancellation'), getEvents('cancellation'))
  print('{:<30}'.format('comparison'), getEvents('comparison'))
  print('{:<30}'.format('underflow'), getEvents('underflow'))
  print('{:<30}'.format('latent_positive_infinity'), getEvents('latent_positive_infinity'))
  print('{:<30}'.format('latent_negative_infinity'), getEvents('latent_negative_infinity'))
  print('{:<30}'.format('latent_underflow'), getEvents('latent_underflow'))

def createEventReport_Text(event_name):
  report_name = (' '.join(event_name.split('_'))).title()
  print("\n===== " + report_name + " Report =====")
  locations = set([])
  for file_name in events[event_name]:
    for t in events[event_name][file_name]:
      line = t[0]
      locations.add(file_name+':'+str(line))
  for l in locations:
    print(l)

  print("\n===== Inputs =====")
  for i in program_inputs[event_name]:
    print(i)

def truncateTextForTable(text, max_chars):
  clean_text = str(text).replace('\t', '    ').strip()
  if len(clean_text) <= max_chars:
    return clean_text
  if max_chars <= 3:
    return clean_text[:max_chars]
  return clean_text[:max_chars - 3] + '...'

def getSourceCodeByLine(file_name, lines):
  source_by_line = {}
  try:
    with open(file_name, 'r') as src_file:
      source_lines = src_file.readlines()

    for line in lines:
      source_index = int(line) - 1
      if source_index >= 0 and source_index < len(source_lines):
        source_by_line[line] = source_lines[source_index].rstrip('\n').rstrip('\r')
      else:
        source_by_line[line] = '(line not found)'
  except Exception:
    for line in lines:
      source_by_line[line] = '(source unavailable)'

  return source_by_line

def createRoundingErrorsReport_Text():
  files = getSortedRoundingErrorFiles()
  print("\n===== Rounding Error Report =====")
  if len(files) == 0:
    print('No files with rounding errors.')
    return

  total_lines = 0
  max_abs_error = 0.0
  max_relative_error = 0.0
  for file_name in files:
    for line in rounding_errors_per_file_line[file_name]:
      total_lines += 1
      error = rounding_errors_per_file_line[file_name][line][0]
      relative_error = rounding_errors_per_file_line[file_name][line][1]
      if abs(error) > max_abs_error:
        max_abs_error = abs(error)
      if relative_error > max_relative_error:
        max_relative_error = relative_error

  print('Files with rounding errors:', len(files))
  print('Lines with rounding errors:', total_lines)
  print('Max abs rounding error:   {:.6e}'.format(max_abs_error))
  print('Max relative error:       {:.6e}'.format(max_relative_error))

  # Keep table rows compact for remote terminals.
  line_col_width = 6
  code_col_width = 56
  error_col_width = 13
  rel_error_col_width = 13

  header_row = (
    '{:<{line_w}} | {:<{code_w}} | {:>{err_w}} | {:>{rel_w}}'.format(
      'Line', 'Code', 'Error', 'Rel. Error',
      line_w=line_col_width,
      code_w=code_col_width,
      err_w=error_col_width,
      rel_w=rel_error_col_width
    )
  )
  separator_row = '-' * len(header_row)

  for file_name in files:
    print("\n--- File: " + file_name)
    # Keep shell output deterministic and easier to scan.
    sorted_lines = sorted(rounding_errors_per_file_line[file_name].keys())
    source_by_line = getSourceCodeByLine(file_name, sorted_lines)
    print(header_row)
    print(separator_row)

    for line in sorted_lines:
      error = rounding_errors_per_file_line[file_name][line][0]
      relative_error = rounding_errors_per_file_line[file_name][line][1]
      source_line = truncateTextForTable(source_by_line[line], code_col_width)
      print(
        '{:<{line_w}} | {:<{code_w}} | {:>{err_w}.6e} | {:>{rel_w}.6e}'.format(
          line,
          source_line,
          error,
          relative_error,
          line_w=line_col_width,
          code_w=code_col_width,
          err_w=error_col_width,
          rel_w=rel_error_col_width
        )
      )

#------------------------------------------------------------------------------
#------------------------- HTML Reports ---------------------------------------
#------------------------------------------------------------------------------

def createRootReport():
  if os.path.exists(REPORTS_DIR):
    prRed('Overwriting report dir...')
    shutil.rmtree(REPORTS_DIR)
  os.mkdir(REPORTS_DIR)

  # Load template
  fd = open(ROOT_REPORT_TEMPLATE, 'r')
  templateLines = fd.readlines()
  fd.close()

  # Copy style and other files
  shutil.copy2(ROOT_REPORT_TEMPLATE_DIR+'/sitestyle.css', REPORTS_DIR+'/sitestyle.css')
  if not os.path.exists(REPORTS_DIR+'/icons_4'):
    shutil.copytree(ROOT_REPORT_TEMPLATE_DIR+'/icons_4', REPORTS_DIR+'/icons_4')

  # Create exponent usage plots (if needed)
  if len(set(fp64_bin_values.values())) > 1:
    plot_exp_usage_bars(fp64_bin_values, 5, REPORTS_DIR+'/'+fp64_plot_filename)
  if len(set(fp32_bin_values.values())) > 1:
    plot_exp_usage_bars(fp32_bin_values, 5, REPORTS_DIR+'/'+fp32_plot_filename)
  
  # Copy default plots
  shutil.copy2(ROOT_REPORT_TEMPLATE_DIR+'/default_fp64_plot.svg', REPORTS_DIR+'/default_fp64_plot.svg')
  shutil.copy2(ROOT_REPORT_TEMPLATE_DIR+'/default_fp32_plot.svg', REPORTS_DIR+'/default_fp32_plot.svg')
  shutil.copy2(ROOT_REPORT_TEMPLATE_DIR+'/llnl_logo.png', REPORTS_DIR+'/llnl_logo.png')

  report_full_name = REPORTS_DIR+'/'+ROOT_REPORT_NAME 
  fd = open(report_full_name, 'w')
  for i in range(len(templateLines)):
    if P_INFINITY_POS in templateLines[i]:
      e = getEvents('positive_infinity')
      if e != 0:
        fd.write('<a href="./positive_infinity/positive_infinity.html">'+str(e)+'</a>\n')
        createEventReport('positive_infinity')
      else: fd.write(str(e)+'\n')

    elif P_INFINITY_NEG in templateLines[i]:
      e = getEvents('negative_infinity')
      if e != 0:
        fd.write('<a href="./negative_infinity/negative_infinity.html">'+str(e)+'</a>\n')
        createEventReport('negative_infinity')
      else: fd.write(str(e)+'\n')

    elif P_NAN in templateLines[i]:
      e = getEvents('nan')
      if e != 0: 
        fd.write('<a href="./nan/nan.html">'+str(e)+'</a>\n')
        createEventReport('nan')
      else: fd.write(str(e)+'\n')

    elif P_DIV_ZERO in templateLines[i]:
      e = getEvents('division_by_zero')
      if e != 0:
        fd.write('<a href="./division_by_zero/division_by_zero.html">'+str(e)+'</a>\n')
        createEventReport('division_by_zero')
      else: fd.write(str(e)+'\n')

    elif P_CANCELLATION in templateLines[i]:
      e = getEvents('cancellation')
      if e != 0: 
        fd.write('<a href="./cancellation/cancellation.html">'+str(e)+'</a>\n')
        createEventReport('cancellation')
      else: fd.write(str(e)+'\n')

    elif P_COMPARISON in templateLines[i]:
      e = getEvents('comparison')
      if e != 0:
        fd.write('<a href="./comparison/comparison.html">'+str(e)+'</a>\n')
        createEventReport('comparison')
      else: fd.write(str(e)+'\n')

    elif P_UNDERFLOW in templateLines[i]:
      e = getEvents('underflow')
      if e != 0: 
        fd.write('<a href="./underflow/underflow.html">'+str(e)+'</a>\n')
        createEventReport('underflow')
      else: fd.write(str(e)+'\n')

    elif P_LATENT_INFINITY_POS in templateLines[i]:
      e = getEvents('latent_positive_infinity')
      if e != 0:
        fd.write('<a href="./latent_positive_infinity/latent_positive_infinity.html">'+str(e)+'</a>\n')
        createEventReport('latent_positive_infinity')
      else: fd.write(str(e)+'\n')

    elif P_LATENT_INFINITY_NEG in templateLines[i]:
      e = getEvents('latent_negative_infinity')
      if e != 0:
        fd.write('<a href="./latent_negative_infinity/latent_negative_infinity.html">'+str(e)+'</a>\n')
        createEventReport('latent_negative_infinity')
      else: fd.write(str(e)+'\n')

    elif P_LATENT_UNDERFLOW in templateLines[i]:
      e = getEvents('latent_underflow')
      if e != 0:
        fd.write('<a href="./latent_underflow/latent_underflow.html">'+str(e)+'</a>\n')
        createEventReport('latent_underflow')
      else: fd.write(str(e)+'\n')

    elif P_CODE_PATHS in templateLines[i]:
      fd.write(getCodePaths()+'\n')

    elif P_LINES_AFFECTED in templateLines[i]:
      fd.write(str(getLinesAffected())+'\n')

    elif P_FILES_AFFECTED in templateLines[i]:
      fd.write(str(getFilesAffected())+'\n')
    
    elif P_REPORT_TITLE in templateLines[i]:
      fd.write(report_title+'\n')

    # Exponent usage plots
    elif P_FP64_HISTOGRAM in templateLines[i]:
      if os.path.exists(REPORTS_DIR+'/'+fp64_plot_filename):
        fd.write('<img src="'+fp64_plot_filename+'"\n')
      else:
        fd.write('<img src="default_fp64_plot.svg"\n')
    elif P_FP32_HISTOGRAM in templateLines[i]:
      if os.path.exists(REPORTS_DIR+'/'+fp32_plot_filename):
        fd.write('<img src="'+fp32_plot_filename+'"\n')
      else:
        fd.write('<img src="default_fp32_plot.svg"\n')
    
    elif P_FP64_INSTRUCTIONS in templateLines[i]:
      fd.write(str(sum(fp64_exp_usage_per_file.values()))+'\n')

    elif P_FP32_INSTRUCTIONS in templateLines[i]:
      fd.write(str(sum(fp32_exp_usage_per_file.values()))+'\n')

    # Rounding error report
    elif P_ROUNDING_ERRORS_TABLES in templateLines[i]:
      fd.write(createRoundingErrorsTables()+'\n')

    elif P_ERROR_LINE in templateLines[i]:
      # Backward-compatible fallback if older templates are used.
      fd.write(createRoundingErrorsReport()+'\n')

    elif P_FILE_ERROR_TRACKING in templateLines[i]:
      fd.write(createRoundingErrorFileList()+'\n')

    elif P_ERRORS_PER_LINE_PLOTS in templateLines[i]:
      for line in relative_errors_per_line:
        errors = relative_errors_per_line[line]
        if len(errors) == 0:
          continue
        # Create plot for this line
        plt.figure(figsize=(8, 4))
        x_axis = list(range(len(errors)))
        plt.plot(x_axis, errors, marker='.', linestyle='-', markersize=4)

        # Force scientific notation on the y-axis
        ax = plt.gca()
        ax.ticklabel_format(style='sci', axis='y')

        # switch to logarithmic y-axis (values must be > 0)
        ax.set_yscale('log')

        plt.title(f'Relative Rounding Errors for Line {line}', fontsize=12)
        plt.xlabel('Index of Value', fontsize=12)
        plt.ylabel('Relative Error Value', fontsize=12)
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plot_filename = REPORTS_DIR+f'/relative_errors_line_{line}.svg'
        plt.savefig(plot_filename, format='svg')
        plt.close()

        # Write img tag to report
        fd.write(f'<img src="relative_errors_line_{line}.svg"></img>\n')

        print(f'Created plot for relative errors of line {line}: {plot_filename}')
        fd.write('<div class="separation_class"></div>\n')
        fd.write('<div class="separation_class"></div>\n')

    else:
        fd.write(templateLines[i])

  fd.close()

  prGreen('Report created: ' + report_full_name)

def createEventReport(event_name):
  report_name = (' '.join(event_name.split('_'))).title()
  
  # Load template
  fd = open(EVENT_REPORT_TEMPLATE, 'r')
  templateLines = fd.readlines()
  fd.close()

  if not os.path.exists(REPORTS_DIR+'/'+event_name):
    os.mkdir(REPORTS_DIR+'/'+event_name)

  fd = open(REPORTS_DIR+'/'+event_name+'/'+event_name+'.html', 'w')
  source_id = 0
  for i in range(len(templateLines)):
    if '<!-- PAGE_NAME -->' in templateLines[i]:
      fd.write(report_name)
    elif '<!-- REPORT_NAME -->' in templateLines[i]:
      fd.write(report_name+' Report')
    elif '<!-- FILE_ENTRIES -->' in templateLines[i]:
      for file in events[event_name]:
        lines = set([])
        for t in events[event_name][file]:
          lines.add((file, t[0]))
        source_id += 1
        fd.write('<tr><td class="files_class">'+file+'</td>\n')
        fd.write('<td class="files_class"><a href="./source_'+str(source_id)+'.html">')
        fd.write(str(len(lines))+'</a></td></tr>')
        createCodeReport(event_name, file, source_id)
    elif '<!-- INPUT_ENTRIES -->' in templateLines[i]:
      for i in program_inputs[event_name]:
        fd.write('<tr>\n')
        fd.write('<td class="files_class">'+i+'</td>\n')
        fd.write('</tr>\n')
    elif P_REPORT_TITLE in templateLines[i]:
      fd.write(report_title+'\n')
    else:
      fd.write(templateLines[i])
  fd.close()

def createCodeReport(event_name, file_full_path, id):
  report_name = (' '.join(event_name.split('_'))).title()
  
  # Load template
  fd = open(SOURCE_REPORT_TEMPLATE, 'r')
  templateLines = fd.readlines()
  fd.close()
  
  fd = open(REPORTS_DIR+'/'+event_name+'/source_'+str(id)+'.html', 'w')
  for i in range(len(templateLines)):
    if '<!-- EVENT_REPORT_HTML -->' in templateLines[i]:
      fd.write('<a href="./'+event_name+'.html">')
    elif '<!-- EVENT_REPORT_NAME -->' in templateLines[i]:
      fd.write(report_name+'\n')
    elif '<!-- FILE_FULL_PATH -->' in templateLines[i]:
      fd.write(file_full_path+'\n')
    elif '<!-- FILE_NAME -->' in templateLines[i]:
      fd.write(os.path.split(file_full_path)[1]+'\n')
    elif '<!-- CODE_LINE -->' in templateLines[i]:
      highligth_set = set([])
      for t in events[event_name][file_full_path]:
        highligth_set.add(int(t[0]))
      htmlCode = createHTMLCode(file_full_path, highligth_set)
      for l in htmlCode:
        fd.write(l+'\n')
    elif P_REPORT_TITLE in templateLines[i]:
      fd.write(report_title+'\n')
    else:
      fd.write(templateLines[i])
  fd.close()

def getSortedRoundingErrorFiles():
  return sorted(rounding_errors_per_file_line.keys())

def getShortDisplayPath(path, max_chars=80):
  if len(path) <= max_chars:
    return path
  return '...'+path[-max_chars:]

def createRoundingErrorFileList():
  files = getSortedRoundingErrorFiles()
  if len(files) == 0:
    return 'No files with rounding errors.'

  entries = []
  for idx, file_name in enumerate(files):
    file_label = os.path.split(file_name)[1]
    n_lines = len(rounding_errors_per_file_line[file_name])
    # Show compact file names, preserving full path in title tooltip.
    entries.append(
      '<a href="#rounding_error_file_'+str(idx)+'" title="'+escape(file_name)+'">'
      + escape(file_label) + ' (' + str(n_lines) + ')</a>'
    )
  return ' | '.join(entries)

def createRoundingErrorsReport():
  report_text = ""
  files = getSortedRoundingErrorFiles()
  for idx, file_name in enumerate(files):
    error_dict = {}
    relative_error_dict = {}
    highligth_set = set([])

    report_text += (
      '<tr id="rounding_error_file_'+str(idx)+'">'
      '<td class="code_line_class"></td>'
      '<td colspan="3"><b>File:</b> '+escape(getShortDisplayPath(file_name))+'</td>'
      '</tr>\n'
    )

    for line in rounding_errors_per_file_line[file_name]:
      error = rounding_errors_per_file_line[file_name][line][0]
      relative_error = rounding_errors_per_file_line[file_name][line][1]
      error_dict[line] = error
      relative_error_dict[line] = relative_error
      highligth_set.add(line)
    htmlCode = createHTMLCode_with_errors(file_name, highligth_set, error_dict, relative_error_dict)
    report_text += '\n'.join(htmlCode) + '\n'

    # Spacer between file blocks.
    report_text += (
      '<tr><td class="code_line_class"></td>'
      '<td colspan="3"></td></tr>\n'
    )
  return report_text

def createRoundingErrorsTables():
  files = getSortedRoundingErrorFiles()
  if len(files) == 0:
    return (
      '<table width="600" class="report_box_source">\n'
      '  <tbody>\n'
      '  <tr class="rounding_file_header_row">\n'
      '    <td colspan="4" class="rounding_file_header_cell"><b>File:</b> No files with rounding errors.</td>\n'
      '  </tr>\n'
      '  </tbody>\n'
      '</table>'
    )

  blocks = []
  for idx, file_name in enumerate(files):
    error_dict = {}
    relative_error_dict = {}
    highligth_set = set([])
    for line in rounding_errors_per_file_line[file_name]:
      error = rounding_errors_per_file_line[file_name][line][0]
      relative_error = rounding_errors_per_file_line[file_name][line][1]
      error_dict[line] = error
      relative_error_dict[line] = relative_error
      highligth_set.add(line)

    htmlCode = createHTMLCode_with_errors(file_name, highligth_set, error_dict, relative_error_dict)

    block = []
    block.append('<table width="600" class="report_box_source" id="rounding_error_file_'+str(idx)+'">')
    block.append('  <tbody>')
    block.append('  <tr class="rounding_file_header_row">')
    block.append('    <td colspan="4" class="rounding_file_header_cell"><b>File:</b> '+escape(getShortDisplayPath(file_name))+'</td>')
    block.append('  </tr>')
    block.append('  <tr>')
    block.append('    <th></th>')
    block.append('    <th></th>')
    block.append('    <th class="error_table_header">Rounding Error</th>')
    block.append('    <th class="error_table_header">Relative Error</th>')
    block.append('  </tr>')
    block.extend(htmlCode)
    block.append('  </tbody>')
    block.append('</table>')

    blocks.append('\n'.join(block))
    if idx != len(files) - 1:
      # Reuse existing spacing helper from template styles.
      blocks.append('<div class="separation_class"></div>')
      blocks.append('<div class="separation_class"></div>')

  return '\n'.join(blocks)

def removeReportDir():
  if os.path.exists(REPORTS_DIR):
    prRed('Removing report dir...')
    shutil.rmtree(REPORTS_DIR)
  else:
    prGreen('There is no report directory to remove.')

# Sample query file
#[
#  {
#  "file": "test.cpp",
#  "line": "any",
#  "infinity_pos": 0,
#  "infinity_neg": 1,
#  "nan": 0,
#  "division_zero": 0,
#  "cancellation": 1,
#  "comparison": 0,
#  "underflow": 0,
#  "latent_infinity_pos": 0,
#  "latent_infinity_neg": 0,
#  "latent_underflow": 0
#  }
#]
def executeQuery(fileName):
  prGreen('Loading: ' + fileName)
  fd = open(fileName, 'r')
  data = json.load(fd)
  fd.close()

  # Walk on the dir to find trace files
  current_path = './'
  for root, dirs, files in os.walk(current_path):
    for file in files:
      fname = os.path.split(file)[1]
      if fname.startswith('fpc_') and fname.endswith(".json"):
        f = str(os.path.join(root, file))
        with open(f, 'r') as trace_file:
          trace_data = json.load(trace_file)
          for i in trace_data:
            if i["file"].endswith(data[0]["file"]):
                if (data[0]['infinity_pos'] <= i['infinity_pos'] and
                    data[0]['infinity_neg'] <= i['infinity_neg'] and
                    data[0]['nan'] <= i['nan'] and
                    data[0]['division_zero'] <= i['division_zero'] and
                    data[0]['cancellation'] <= i['cancellation'] and
                    data[0]['comparison'] <= i['comparison'] and
                    data[0]['underflow'] <= i['underflow'] and
                    data[0]['latent_infinity_pos'] <= i['latent_infinity_pos'] and
                    data[0]['latent_infinity_neg'] <= i['latent_infinity_neg'] and
                    data[0]['latent_underflow'] <= i['latent_underflow']
                    ): 
                  print('Trace:', f)

def removeTraces():
  p = './'
  for root, dirs, files in os.walk(p):
    for d in dirs:
      if d.endswith(TRACES_DIR):
        full_path = str(os.path.join(root, d))
        prGreen('Removing: ' + full_path)
        try:
          shutil.rmtree(full_path)
        except Exception as e:
          prRed(e)
 
if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='FPChecker report generator')
  parser.add_argument('-r', '--remove', action='store_true', help='Remove report dir.')
  parser.add_argument('-c', '--clean', action='store_true', help='Remove traces. A report cannot be generated without traces.')
  parser.add_argument('-t', '--title', nargs=1, type=str, help='Title of report.')
  parser.add_argument('-q', '--query', nargs=1, type=str, action='store', help='Query file.')
  parser.add_argument('-s', '--show', action='store', nargs='?', default=0, type=str, help='Show report on screen (main, event, or rounding_error).')
  parser.add_argument('dir', nargs='?', default=os.getcwd())
  args = parser.parse_args()

  if (args.show != 0):
    prCyan('Generating FPChecker report...')
    reports_path = args.dir  
    fileList = getEventFilePaths(reports_path)
    fileListErrors = getErrorFilePaths(reports_path)
    print('Trace files found:', len(fileList))
    print('Error files found:', len(fileListErrors))
    loadEvents(fileList)
    loadRoundingErrorTraces(fileListErrors)
    if (args.show == None ):
      createRootReport_Text()
      createRoundingErrorsReport_Text()
    elif (args.show in ['rounding_error', 'rounding_errors', 'rounding']):
      createRoundingErrorsReport_Text()
    else:
      event_name = args.show
      createEventReport_Text(event_name)
    exit()

  if (args.query):
    fileName = args.query[0]
    executeQuery(fileName)
    exit()
 
  if (args.remove or args.clean):
    if (args.remove):
      removeReportDir()
    if (args.clean):
      removeTraces()
    exit()
    
  if (args.title):
    report_title = args.title[0]
    
  reports_path = args.dir  
  prCyan('Generating FPChecker report...')
  fileList = getEventFilePaths(reports_path)

  # Find files
  fileListExpUsage = getExponentUsageFilePaths(reports_path)
  fileListErrors = getErrorFilePaths(reports_path)
  fileListErrorsPerLine = getErrorsPerLineFilePaths(reports_path)
  print('Trace files found:', len(fileList))
  print('Exponent usage files found:', len(fileListExpUsage))
  print('Error files found:', len(fileListErrors))
  print('Errors per line files found:', len(fileListErrorsPerLine))

  # Load events
  loadEvents(fileList)
  loadExponentUsageTraces(fileListExpUsage)
  loadRoundingErrorTraces(fileListErrors)
  loadErrorsPerLineTraces(fileListErrorsPerLine)

  createRootReport()
