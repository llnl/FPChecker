#!/bin/bash

BIN=${1:-./gramschmidt}

# Set to 1 to keep gdb_eft_line_*.log files.
# Set to 0 to delete them after extracting c_err.
SAVE_LOGS=1

export LD_LIBRARY_PATH=/g/g90/sharmin1/tutorial/FPChecker/experiments/EFTSanitizer/runtime/obj:/g/g90/sharmin1/conda_env/llvm10/lib:$LD_LIBRARY_PATH

SRC_FILE="$(basename "$(pwd)").c"
FULL_SRC_FILE="$(pwd)/${SRC_FILE}"
OUT="$(pwd)/eftsan_errors.json"

if [ $# -ge 2 ]; then
    LINES=$2
else
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

echo "[" > "$OUT"

# echo "Running in directory: $(pwd)"
# echo "Binary: $BIN"
# echo "Source file: $FULL_SRC_FILE"
# echo "Output file: $OUT"

IFS=',' read -ra LINE_LIST <<< "$LINES"

ENTRY_COUNT=0

for LINE in "${LINE_LIST[@]}"; do
    LINE=$(echo "$LINE" | xargs)

    LOG="gdb_eft_line_${LINE}.log"
    rm -f "$LOG"

    NORM_INFO=$(awk -v start="$LINE" '
      NR > start && $0 ~ /^[[:space:]]*double[[:space:]]+norm_error([_[:alnum:]]*)?[[:space:]]*=/ {
        var = $0
        sub(/^[[:space:]]*/, "", var)
        split(var, fields, /[[:space:]]+/)
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

    # Extract c_err from the log file.
    # Expected log text contains something like: c_err=VALUE
    ERR=$(grep -m1 "c_err=" "$LOG" | sed -n 's/.*c_err=\([^,)]*\).*/\1/p')

    if [ -z "$ERR" ]; then
        echo "WARNING: Could not extract c_err for line ${LINE}. Check ${LOG}."
        ERR="0.0"
    fi

    NORM_ERROR_VALUE=""
    if [ -n "$NORM_ERROR_VAR" ]; then
        NORM_ERROR_VALUE=$(awk '/^\$[0-9]+ = / { value = $0 } END { if (value != "") { sub(/^\$[0-9]+ = /, "", value); print value } }' "$LOG")
        if [ -z "$NORM_ERROR_VALUE" ]; then
            echo "WARNING: Could not extract ${NORM_ERROR_VAR} for line ${LINE}. Check ${LOG}."
        fi
    fi

    if [ -z "$NORM_ERROR_VALUE" ]; then
        NORM_ERROR_VALUE_JSON="null"
    else
        NORM_ERROR_VALUE_JSON="${NORM_ERROR_VALUE}"
    fi

    if [ "$ENTRY_COUNT" -gt 0 ]; then
        echo "," >> "$OUT"
    fi

    cat >> "$OUT" <<EOF
  {
    "file": "${FULL_SRC_FILE}",
    "line": ${LINE},
    "error": ${ERR},
    "baseline": ${NORM_ERROR_VALUE_JSON}
  }
EOF

    ENTRY_COUNT=$((ENTRY_COUNT + 1))

    echo "line=${LINE}, error=${ERR}, norm_error=${NORM_ERROR_VALUE_JSON}"

    if [ "$SAVE_LOGS" -eq 0 ]; then
        rm -f "$LOG"
    fi
done

# echo "" >> "$OUT"
echo "]" >> "$OUT"

# echo "Created:"
# ls -lh "$OUT"
