# CGO 2026 artifact -- branch-flip detection

Three floating-point branch-flip detectors (FPChecker, EFTSanitizer, NSan)
scored against a brtrace ground-truth census on LULESH, AMG, QuickSilver and
the NAS Parallel Benchmarks.

## Build the image

    cd cgo2026_artifact
    podman build -t cgo2026-artifact .        # or: docker build -t cgo2026-artifact .

If the base image cannot be resolved under Podman, use
`--from docker.io/library/ubuntu:22.04` or set `FROM docker.io/library/ubuntu:22.04`.
The build clones FPChecker (branch `v0.7_new_runtime_branch_flip`) and
EFTSanitizer (pinned SHA, patched), creates three conda environments
(LLVM 19.1.7 for FPChecker/brtrace/NSan, LLVM 10 for EFTSanitizer), builds
all four tools and the NSan compiler-rt runtime. Expect 45-90 minutes.

## Run

    podman run -it --rm -v $PWD/results:/opt/cgo2026_artifact/results cgo2026-artifact
    ./run_experiments.sh --quick        # ~1-2 h: everything but QuickSilver and NAS SP
    ./run_experiments.sh                # full sweep; QuickSilver alone is several hours

Outputs in `results/`: `{fpc,eftsan,nsan}_metrics.json` (scorer output),
`table_main.{txt,tex,json,pdf}` (paper Table 4), `table_full.{txt,tex,json,pdf}`
(appendix Tables 11 and 12), `compare.txt`, `run.log`.

Tables can be rebuilt without rerunning anything:

    ./branch_flip_tables.py --main            # Table 4
    ./branch_flip_tables.py --full --pdf      # Tables 11/12
    ./branch_flip_tables.py --score --main    # rerun the scorers first

## What to expect

Every table row is checked against `expected/`: `ok` = identical counts,
`delta` = LULESH only (its non-finite abstention counts move by a few percent
between builds and hosts), `DIFF` = a real mismatch. `RESULT: OK` in
`compare.txt` means no `DIFF`.

## Layout inside the container

    /opt/cgo2026_artifact/FPChecker                     repo (branch v0.7_new_runtime_branch_flip)
      cpu_checking/error_analysis/env_setup/            create_*/activate_* env scripts
      cpu_checking/error_analysis/branch_flip/
        benchmarks/                                     fp32 / fp64 / long-double source trees
        expected/                                       reference metrics JSON
        experiments/gt_experiments/                     brtrace + run_*_brtrace.py + *_exact_metrics.py
        experiments/{fpchecker,eftsan,nsan}_experiments/ per-tool harnesses
    /opt/cgo2026_artifact/EFTSanitizer                  pinned + patched
