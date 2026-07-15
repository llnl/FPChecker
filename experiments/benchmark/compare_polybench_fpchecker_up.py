#!/usr/bin/env python3
"""
Run compare_polybench_fpchecker.py with a command-line optimization override.

This intentionally does not edit the benchmark Makefiles. It relies on GNU
make command-line variable precedence by running builds as:
  make OP="<optimization flags>"
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path
from typing import Optional, Sequence

import compare_polybench_fpchecker as base


DEFAULT_EXTRA_FLAGS = "-fno-vectorize -fno-slp-vectorize"


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    if opt not in {"O0", "O1", "O2", "O3"}:
        raise argparse.ArgumentTypeError("expected one of O0, O1, O2, O3")
    return opt


def parse_wrapper_args(argv: Sequence[str]) -> tuple[argparse.Namespace, list[str]]:
    if any(arg in {"-h", "--help"} for arg in argv):
        print(
            "Optimization override options:\n"
            "  --opt-level {O0,O1,O2,O3}\n"
            "      Optimization level for benchmark builds. Defaults to O2.\n"
            "  --opt-flags FLAGS\n"
            "      Full OP value passed to make; overrides --opt-level.\n"
        )

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--opt-level",
        type=normalize_opt_level,
        default="O2",
        help="Optimization level for benchmark builds: O0, O1, O2, or O3. Defaults to O2.",
    )
    parser.add_argument(
        "--opt-flags",
        default=None,
        help=(
            "Full OP value passed to make. Overrides --opt-level, for example "
            "'-O3 -fno-vectorize -fno-slp-vectorize'."
        ),
    )
    return parser.parse_known_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    raw_args = list(sys.argv[1:] if argv is None else argv)
    wrapper_args, base_args = parse_wrapper_args(raw_args)
    help_requested = any(arg in {"-h", "--help"} for arg in raw_args)

    opt_flags = wrapper_args.opt_flags
    if opt_flags is None:
        opt_flags = f"-{wrapper_args.opt_level} {DEFAULT_EXTRA_FLAGS}"

    original_run_cmd = base.run_cmd
    original_find_executable = base.find_executable

    def run_cmd_with_opt(cmd, cwd: Path, env, timeout):
        if cmd and cmd[0] == "make" and "clean" not in cmd[1:]:
            cmd = [*cmd, f"OP={opt_flags}"]
        return original_run_cmd(cmd, cwd, env, timeout)

    def find_executable_with_chmod(bench_dir: Path):
        executable = original_find_executable(bench_dir)
        if executable is not None:
            return executable

        candidates = [bench_dir / name for name in base.parse_makefile_outputs(bench_dir / "Makefile")]
        candidates.append(bench_dir / bench_dir.name)

        for candidate in candidates:
            if not candidate.is_file():
                continue
            try:
                candidate.chmod(candidate.stat().st_mode | 0o100)
            except OSError:
                pass
            if os.access(candidate, os.X_OK):
                return candidate
        return None

    base.run_cmd = run_cmd_with_opt
    base.find_executable = find_executable_with_chmod
    if not help_requested:
        print(f"Using Makefile OP override: {opt_flags}", file=sys.stderr)
    return base.main(base_args)


if __name__ == "__main__":
    raise SystemExit(main())
