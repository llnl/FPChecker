# brtrace — fp32-vs-fp64 branch-flip tracer

A general tool for finding branch-decision flips between two builds of the same
program (typically fp32 vs fp64). It instruments conditional branches, runs both
builds, and reports every branch where the two took different directions.

**The tool is independent of any benchmark.** Nothing in the core
(`pass/`, `runtime/`, `tools/`, `brtrace_run.sh`) contains benchmark-specific
code. To run it on a program you write a small *config* that supplies the
build/run commands and calls the generic runner. Example configs live in
`examples/` and are meant to be copied next to the benchmark they drive.

## Layout

    brtrace/
      pass/BranchTrace_mtu.cpp        # LLVM pass: instrument conditional branches
      runtime/brtrace_runtime_mtu.c   # records (module_id, site_id, taken)
      tools/brtrace_diff_mtu.py       # diffs two traces, reports flips
      build_mtu.sh                    # builds the plugin + runtime
      brtrace_run.sh                  # GENERIC runner (no benchmark knowledge)
      examples/                       # sample per-benchmark configs (copy these out)

## Build the tool (once)

    source /usr/workspace/das9/miniconda3/etc/profile.d/conda.sh
    conda activate fpchecker_env
    cd brtrace && bash build_mtu.sh     # -> libBranchTrace_mtu.so, brtrace_runtime_mtu.o

## Run on any program

    BRX=/abs/path/to/brtrace
    $BRX/brtrace_run.sh \
      --brx "$BRX" --out . --label myprog --mods <dir-with-.brsites> \
      --fp32-build '<shell that builds the fp32 binary>' \
      --fp64-build '<shell that builds the fp64 binary>' \
      --fp32-run   '<shell that runs the fp32 binary>' \
      --fp64-run   '<shell that runs the fp64 binary>'

Exported into your build/run commands:

  - $BRX_PLUGIN  absolute path to the pass plugin
  - $BRX_RT      absolute path to the runtime object to link
  - $BRX_CFLAGS  "-O0 -g -fpass-plugin=$BRX_PLUGIN"  (add to every compile)
  - $BRTRACE_OUT set per-run to the right trace file (reference in --*-run)

## Example configs

    cp examples/lulesh.brtrace /path/to/lulesh/run.brtrace
    BRX=/abs/path/to/brtrace  /path/to/lulesh/run.brtrace 10

    cp examples/nas.brtrace /path/to/nas/run.brtrace
    BRX=/abs/path/to/brtrace  /path/to/nas/run.brtrace sp

These only encode the benchmark's build recipe and hand it to brtrace_run.sh.

## How it works

The pass numbers conditional branches deterministically per module and tags each
with a module_id (stable hash of the source path), so (module_id, site_id) is
globally unique across all TUs. Building the same source as fp32 and fp64 gives
identical ids to corresponding branches; the diff walks both traces in lock-step:
same id + different taken = flip; different id = control-flow divergence.

## Caveats (every target)

- -O0 required (identical front-end lowering across builds).
- fcmp+select invisible (ternary / min / max lower to select, not a branch).
- Single-threaded (OMP_NUM_THREADS=1) or trace order scrambles.
- Both builds must process the same input.
- Per-module site counts must match between builds (runner prints them).

## Single-TU variant

The original single-TU tool (libBranchTrace.so, brtrace_runtime.o,
tools/brtrace_diff.py, run_nas.sh) is retained for the single-file NAS runs and
uses an 8-byte record. The multi-TU tool above (12-byte record) supersedes it
and handles the single-file case too (one module). Prefer the multi-TU tool for
new work; the two trace formats are NOT interchangeable.
