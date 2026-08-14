#!/usr/bin/env bash
set -u

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR" || exit 1

BENCHMARK_DIR=$(basename "$SCRIPT_DIR")
BENCHMARK=${BENCHMARK_DIR%_fp64}
BIN=${1:-./${BENCHMARK}}
LINES=${2:-}
SAVE_LOGS=${SAVE_LOGS:-1}
LLVM10_LIB=${LLVM10_LIB:-/g/g90/sharmin1/conda_env/llvm10/lib}
EFT_HOME=${EFT_HOME:-$(cd "$SCRIPT_DIR/../../.." && pwd)}

export LD_LIBRARY_PATH=${EFT_HOME}/runtime/obj:${LLVM10_LIB}:${LD_LIBRARY_PATH:-}

SRC_FILE="${SRC_FILE:-${BENCHMARK}.c}"
FULL_SRC_FILE="$(pwd)/${SRC_FILE}"
OUT="$(pwd)/eftsan_errors.json"

if [ ! -f "$SRC_FILE" ]; then
    echo "ERROR: Source file '${SRC_FILE}' does not exist."
    exit 1
fi

if [ ! -x "$BIN" ]; then
    echo "ERROR: Binary '${BIN}' does not exist or is not executable."
    echo "Build it first with 'make', or pass the correct binary path as the first argument."
    exit 1
fi

if [ -z "$LINES" ]; then
    LINES=$(awk '
      function is_norm_name(name) {
        return name == "norm" || name ~ /^norm_[[:alnum:]_]+$/
      }
      {
        line = $0
        sub(/\/\/.*/, "", line)
        gsub(/[[:space:]]/, "", line)
        sub(/;.*/, ";", line)
        if (line !~ /;$/)
          next
        sub(/;$/, "", line)
        count = split(line, parts, "=")
        if (count != 2)
          next
        lhs = parts[1]
        rhs = parts[2]
        if (!is_norm_name(lhs))
          next
        if (rhs == "0+" lhs || rhs == lhs "+0")
          print NR
      }
    ' "$SRC_FILE" | paste -sd, -)
fi

if [ -z "$LINES" ]; then
    echo "ERROR: Could not find norm assignment lines in ${SRC_FILE}."
    exit 1
fi

rm -f "$OUT"
printf '[\n' > "$OUT"

IFS=',' read -ra LINE_LIST <<< "$LINES"
ENTRY_COUNT=0
EXTRACT_FAILED=0

for LINE in "${LINE_LIST[@]}"; do
    LINE=$(echo "$LINE" | xargs)
    LOG="gdb_eft_line_${LINE}.log"
    rm -f "$LOG"

    NORM_INFO=$(awk -v start="$LINE" '
      NR > start && $0 ~ /^[[:space:]]*(double|long[[:space:]]+double)[[:space:]]+norm_error([_[:alnum:]]*)?[[:space:]]*=/ {
        var = $0
        sub(/^[[:space:]]*/, "", var)
        split(var, fields, /[[:space:]]+/)
        if (fields[1] == "long")
          var = fields[3]
        else
          var = fields[2]
        print NR + 1, var
        exit
      }
    ' "$SRC_FILE")
    NORM_ERROR_LINE=$(echo "$NORM_INFO" | awk '{print $1}')
    NORM_ERROR_VAR=$(echo "$NORM_INFO" | awk '{print $2}')

    GDB_ARGS=(
      -q -batch
      -ex "set pagination off"
      -ex "set breakpoint pending on"
      -ex "break ${SRC_FILE}:${LINE}"
      -ex "run"
      -ex "break eftsan_sum"
      -ex "continue"
    )

    if [ -n "$NORM_ERROR_LINE" ] && [ -n "$NORM_ERROR_VAR" ]; then
      GDB_ARGS+=(
        -ex "break ${SRC_FILE}:${NORM_ERROR_LINE}"
        -ex "disable 2"
        -ex "continue"
        -ex "print ${NORM_ERROR_VAR}"
      )
    fi

    gdb "${GDB_ARGS[@]}" \
      "$BIN" > "$LOG" 2>&1

    ERR=$(grep -m1 "c_err=" "$LOG" | sed -n 's/.*c_err=\([^,)]*\).*/\1/p')
    if [ -z "$ERR" ]; then
        echo "WARNING: Could not extract c_err for line ${LINE}. Check ${LOG}."
        ERR_JSON="null"
        EXTRACT_FAILED=1
    else
        ERR_JSON="$ERR"
    fi

    NORM_ERROR_VALUE=""
    if [ -n "$NORM_ERROR_VAR" ]; then
        NORM_ERROR_VALUE=$(awk '/^\$[0-9]+ = / { value = $0 } END { if (value != "") { sub(/^\$[0-9]+ = /, "", value); print value } }' "$LOG")
        if [ -z "$NORM_ERROR_VALUE" ]; then
            echo "WARNING: Could not extract ${NORM_ERROR_VAR} for line ${LINE}. Check ${LOG}."
            EXTRACT_FAILED=1
        fi
    else
        echo "WARNING: Could not find a norm_error variable after line ${LINE} in ${SRC_FILE}."
        EXTRACT_FAILED=1
    fi

    if [ -z "$NORM_ERROR_VALUE" ]; then
        NORM_ERROR_VALUE_JSON="null"
    else
        NORM_ERROR_VALUE_JSON="${NORM_ERROR_VALUE}"
    fi

    if [ "$ENTRY_COUNT" -gt 0 ]; then
        printf ',\n' >> "$OUT"
    fi

    cat >> "$OUT" <<EOF
  {
    "file": "${FULL_SRC_FILE}",
    "line": ${LINE},
    "error": ${ERR_JSON},
    "baseline_error": ${NORM_ERROR_VALUE_JSON}
  }
EOF

    ENTRY_COUNT=$((ENTRY_COUNT + 1))
    echo "line=${LINE}, error=${ERR_JSON}, baseline_error=${NORM_ERROR_VALUE_JSON}, log=${LOG}"

    if [ "$SAVE_LOGS" -eq 0 ]; then
        rm -f "$LOG"
    fi
done

printf '\n]\n' >> "$OUT"

if [ "$EXTRACT_FAILED" -ne 0 ]; then
    exit 2
fi
