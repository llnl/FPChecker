# brtrace -- fp32-vs-fp64 branch-flip tracer

Instruments conditional branches (and FP-controlled selects), runs two builds
of the same program, and reports every branch where the two took different
directions. The tool contains no benchmark-specific code.

    pass/BranchTrace_mtu.cpp        LLVM pass: instrument branches / selects
    runtime/brtrace_runtime_mtu.c   records (module_id, site_id, taken)
    tools/brtrace_diff_mtu.py       diffs two traces; window.csv, flips.csv, sites.txt
    build_mtu.sh                    builds the plugin + runtime
    brtrace_run.sh                  generic runner
    examples/                       per-benchmark configs that call the runner

## Build

    conda activate fpchecker_env      # LLVM 19.1.7
    bash build_mtu.sh                 # -> libBranchTrace_mtu.so, brtrace_runtime_mtu.o

## Run

    BRX=/abs/path/to/brtrace
    $BRX/brtrace_run.sh --brx "$BRX" --out . --label myprog --mods <dir-with-.brsites> \
      --fp32-build '<shell that builds the fp32 binary>' \
      --fp64-build '<shell that builds the fp64 binary>' \
      --fp32-run   '<shell that runs the fp32 binary>' \
      --fp64-run   '<shell that runs the fp64 binary>'

Exported into the build/run commands: `$BRX_PLUGIN`, `$BRX_RT`,
`$BRX_CFLAGS` (`-O0 -g -fpass-plugin=$BRX_PLUGIN`), `$BRTRACE_OUT`.
The gt_experiments harnesses (`run_*_brtrace.py`) drive the pass and diff
tool directly.

## How it works

The pass numbers conditional branches deterministically per module and tags
each with `module_id` = FNV-1a-32 of the source basename, so
`(module_id, site_id)` is unique across the link and identical between fp32
and fp64 builds of the same source. The diff walks both traces in lock-step:
same id, different `taken` = flip; different id = control-flow divergence,
after which nothing is adjudicated. Selects use a separate id space and
stream. Site labels are anchored on the controlling fcmp (table version 4).

## Requirements

- `-O0` for both builds, same `-brtrace-fp-only` setting.
- Single-threaded (`OMP_NUM_THREADS=1`), same input for both runs.
- Per-module site counts must match between builds (the pass prints them).
