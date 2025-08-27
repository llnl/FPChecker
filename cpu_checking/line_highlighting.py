
# Padding lines: lines printed but not highlighted
# Dotted lines: only show dots
def calc_lines_to_highligh(file_len:int, highligth_set:set):
  padding_set = set([])
  dots_set = set([])
  for l in highligth_set:
    before = l-1
    after = l+1
    dots_before = before-1
    dots_after = after+1
    if before >= 1:
      padding_set.add(before)
    if after <= file_len:
      padding_set.add(after)
    if dots_before >= 1:
      dots_set.add(dots_before)
    if dots_after <= file_len:
      dots_set.add(dots_after)

  #d = defaultdict(char)
  d = {}
  for i in range(file_len):
    line = i+1
    if line in dots_set:
      d[line] = 'D'
    if line in padding_set:
      d[line] = 'P'
    if line in highligth_set:
      d[line] = 'H'

  return d
  
def replaceCodeChars(line):
  l = line.replace('&', '&amp;')
  l = l.replace('<', '&lt;')
  l = l.replace('>', '&gt;')
  return l
  
def createHTMLCode(file_full_path:str, highligth_set:set):
  fd = open(file_full_path, 'r')
  all_lines = fd.readlines()
  fd.close()
  
  ret = []
  d = calc_lines_to_highligh(len(all_lines), highligth_set)
  for k in d:
    line = '<tr><td class="code_line_class">'+str(k)+'</td>'
    if d[k] == 'D':
      line = line + '<td><code> ... </code></td></tr>'
    elif d[k] == 'P':
      line = line + '<td><code>'+all_lines[k-1][:-1]+'</code></td></tr>'
    elif d[k] == 'H':
      line = line + '<td><span class="highlightme"><code>'+all_lines[k-1][:-1]+'</code></span></td></tr>'
    ret.append(line)
  
  return ret

def createHTMLCode_with_errors(file_full_path:str, highligth_set:set, error_1_dict:dict, error_2_dict:dict):
  fd = open(file_full_path, 'r')
  all_lines = fd.readlines()
  fd.close()
  
  ret = []
  d = calc_lines_to_highligh(len(all_lines), highligth_set)
  for k in d:
    line = '<tr><td class="code_line_class">'+str(k)+'</td>'
    if d[k] == 'D':
      line = line + '<td><code> ... </code></td>'
      line = line + '<td class="error_table_header"></td>' + '<td class="error_table_header"></td>'
      line = line + '</tr>'
    elif d[k] == 'P':
      line = line + '<td><code>'+all_lines[k-1][:-1]+'</code></td>'
      line = line + '<td class="error_table_header"></td>' + '<td class="error_table_header"></td>'
      line = line + '</tr>'
    elif d[k] == 'H':
      line = line + '<td><span class="highlightme_error"><code>'+all_lines[k-1][:-1]+'</code></span></td>'
      error_1 = error_1_dict.get(k, "")
      # Convert to string with 7 decimal places
      if error_1 != "":
        try:
          error_1 = "{:.7e}".format(float(error_1))
        except Exception:
          error_1 = ""
      line = line + '<td class="error_table_header">'+error_1+'</td>'
      error_2 = error_2_dict.get(k, "")
      # Convert to string with 7 decimal places
      if error_2 != "":
        try:
          error_2 = "{:.7e}".format(float(error_2))
        except Exception:
          error_2 = ""
      line = line + '<td class="error_table_header">'+error_2+'</td>'

      line = line + '</tr>'
    ret.append(line)
  
  return ret
  
if __name__ == '__main__':
  highligth_set = set([4,5,6,7,8])
  file_full_path = '../tests/cpu_checking/dynamic/test_fp32_nan/compute.cpp'
  lines = createHTMLCode(file_full_path, highligth_set)
  for l in lines:
    print(l)

  print("\n")
  errors_1 = {4: 1.1472780556687212e-06, 12: 3.767558613316918e-09}
  errors_2 = {4: 1.1472780556687212e-06, 12: 2.58329202364726452e-7}
  lines = createHTMLCode_with_errors(file_full_path, highligth_set, errors_1, errors_2)
  for l in lines:
    print(l)
