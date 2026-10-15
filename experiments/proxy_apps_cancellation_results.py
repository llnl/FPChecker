#!/usr/bin/env python3

import argparse
import atexit
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


EXPERIMENTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = EXPERIMENTS_DIR.parent
PROXY_APPS_DIR = EXPERIMENTS_DIR / "proxy_apps"
FPCHECKER_BIN = REPO_ROOT / "bin"
CPU_CHECKING_DIR = REPO_ROOT / "cpu_checking"

DEFAULT_ROWS_PER_APP = 4
DEFAULT_ROWS_PER_TYPE = 2
INJECTION_TYPES = ("standalone", "state")
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
DEFAULT_FPCHECKER_ENVS = ("tutorial_env", "fpchecker_env")
CONDA_ENV_PREFIX_CACHE = None

APP_RUNS = [
    {
        "name": "AMG",
        "cwd": PROXY_APPS_DIR / "AMG",
        "command": ["./run_cancellation_experiment.py"],
    },
    {
        "name": "HPCCG",
        "cwd": PROXY_APPS_DIR / "HPCCG",
        "command": ["./run_cancellation_experiment.py"],
    },
    {
        "name": "LULESH",
        "cwd": PROXY_APPS_DIR / "LULESH_modified",
        "command": ["./run_cancellation_experiment.py"],
    },
    {
        "name": "miniAMR",
        "cwd": PROXY_APPS_DIR / "miniAMR",
        "command": ["./run_cancellation_experiment.py"],
    },
    {
        "name": "Quicksilver",
        "cwd": PROXY_APPS_DIR / "Quicksilver" / "src",
        "command": ["./run_cancellation_exp_multiple_file.py"],
    },
]


def make_frontend_wrapper(wrapper_dir, wrapper_name, compiler_name, frontend_script):
    wrapper_path = wrapper_dir / wrapper_name
    wrapper_path.write_text(
        "#!/bin/bash\n"
        "export FPC_COMPILER={compiler!r}\n"
        "export FPC_COMPILER_PARAMS=`printf \"%q \" \"$@\"`\n"
        "exec {frontend!r}\n".format(
            compiler=compiler_name,
            frontend=str(frontend_script),
        )
    )
    wrapper_path.chmod(0o700)


def create_local_fpchecker_bin():
    clang_frontend = CPU_CHECKING_DIR / "clang_fpchecker.py"
    mpi_frontend = CPU_CHECKING_DIR / "mpicc_fpchecker.py"
    if not clang_frontend.is_file() or not mpi_frontend.is_file():
        raise RuntimeError("local FPChecker CPU frontends are missing in " + str(CPU_CHECKING_DIR))

    wrapper_dir = Path(tempfile.mkdtemp(prefix="fpchecker-bin-"))
    atexit.register(lambda: shutil.rmtree(str(wrapper_dir), ignore_errors=True))

    make_frontend_wrapper(wrapper_dir, "clang-fpchecker", "clang", clang_frontend)
    make_frontend_wrapper(wrapper_dir, "clang++-fpchecker", "clang++", clang_frontend)
    make_frontend_wrapper(wrapper_dir, "mpicc-fpchecker", "mpicc", mpi_frontend)
    make_frontend_wrapper(wrapper_dir, "mpic++-fpchecker", "mpic++", mpi_frontend)
    make_frontend_wrapper(wrapper_dir, "mpicxx-fpchecker", "mpicxx", mpi_frontend)
    return wrapper_dir


def prepend_path(env, name, value):
    old = env.get(name, "")
    env[name] = str(value) + os.pathsep + old if old else str(value)


def conda_env_prefixes_by_name():
    global CONDA_ENV_PREFIX_CACHE
    if CONDA_ENV_PREFIX_CACHE is not None:
        return CONDA_ENV_PREFIX_CACHE

    prefixes = {}
    conda = shutil.which("conda")
    if conda is None:
        CONDA_ENV_PREFIX_CACHE = prefixes
        return prefixes

    try:
        result = subprocess.run(
            [conda, "env", "list", "--json"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            universal_newlines=True,
            check=False,
            timeout=15,
        )
    except (OSError, subprocess.TimeoutExpired):
        CONDA_ENV_PREFIX_CACHE = prefixes
        return prefixes

    if result.returncode == 0:
        try:
            data = json.loads(result.stdout)
        except ValueError:
            data = {}
        for item in data.get("envs", []):
            prefix = Path(str(item)).expanduser()
            prefixes.setdefault(prefix.name, prefix)

    CONDA_ENV_PREFIX_CACHE = prefixes
    return prefixes


def candidate_env_prefixes(env_name):
    value = Path(env_name).expanduser()
    if value.is_absolute() or "/" in env_name:
        return [value]

    candidates = []
    conda_prefix = os.environ.get("CONDA_PREFIX")
    if conda_prefix:
        current_prefix = Path(conda_prefix)
        if current_prefix.name == env_name:
            candidates.append(current_prefix)
        candidates.append(current_prefix.parent / "envs" / env_name)

    home = Path.home()
    candidates.extend(
        [
            home / "conda_env" / env_name,
            home / "miniconda3" / "envs" / env_name,
            home / "anaconda3" / "envs" / env_name,
            Path("/opt/anaconda3/envs") / env_name,
        ]
    )
    conda_env_prefix = conda_env_prefixes_by_name().get(env_name)
    if conda_env_prefix is not None:
        candidates.append(conda_env_prefix)
    return candidates


def resolve_env_prefix(env_name):
    for candidate in candidate_env_prefixes(env_name):
        if candidate.is_dir():
            return candidate.resolve()
    return None


def choose_fpchecker_env_name():
    for env_name in DEFAULT_FPCHECKER_ENVS:
        if resolve_env_prefix(env_name) is not None:
            return env_name
    current_env = os.environ.get("CONDA_DEFAULT_ENV")
    if current_env in DEFAULT_FPCHECKER_ENVS:
        return current_env
    return DEFAULT_FPCHECKER_ENVS[0]


def default_conda_sh():
    candidates = []
    conda_sh = os.environ.get("CONDA_SH")
    if conda_sh:
        candidates.append(Path(conda_sh).expanduser())
    conda_prefix = os.environ.get("CONDA_PREFIX")
    if conda_prefix:
        candidates.append(Path(conda_prefix) / "etc" / "profile.d" / "conda.sh")
    candidates.extend(
        [
            Path.home() / "miniconda3" / "etc" / "profile.d" / "conda.sh",
            Path.home() / "anaconda3" / "etc" / "profile.d" / "conda.sh",
            Path("/opt/anaconda3/etc/profile.d/conda.sh"),
        ]
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def shell_join(command):
    return " ".join(shlex.quote(item) for item in command)


def searched_fpchecker_envs_message():
    pieces = []
    for env_name in DEFAULT_FPCHECKER_ENVS:
        locations = ", ".join(str(path) for path in candidate_env_prefixes(env_name))
        pieces.append(env_name + " in " + locations)
    return "; ".join(pieces)


def conda_activation_shell(env_name, prefix):
    if prefix is not None:
        return []

    conda_sh = default_conda_sh()
    if conda_sh is None:
        raise RuntimeError(
            "could not find FPChecker conda environment. Searched "
            + searched_fpchecker_envs_message()
            + ". Also could not find conda activation support."
        )
    return [
        ". " + shlex.quote(str(conda_sh)),
        "conda activate " + shlex.quote(env_name),
    ]


class FPCheckerEnvironment:
    def __init__(self, name, env, activation_shell):
        self.name = name
        self.env = env
        self.activation_shell = activation_shell


def fpchecker_env():
    env = os.environ.copy()
    env_name = choose_fpchecker_env_name()
    prefix = resolve_env_prefix(env_name)
    activation_shell = conda_activation_shell(env_name, prefix)
    if prefix is not None:
        env["CONDA_PREFIX"] = str(prefix)
        env["CONDA_DEFAULT_ENV"] = env_name
        prepend_path(env, "PATH", prefix / "bin")
        if (prefix / "lib").is_dir():
            prepend_path(env, "LD_LIBRARY_PATH", prefix / "lib")

    current_path = env.get("PATH", "")

    has_clang_wrapper = shutil.which("clang++-fpchecker", path=current_path) is not None
    has_mpi_wrapper = shutil.which("mpicc-fpchecker", path=current_path) is not None

    local_bin_has_wrappers = (
        FPCHECKER_BIN.is_dir()
        and shutil.which("clang++-fpchecker", path=str(FPCHECKER_BIN)) is not None
        and shutil.which("mpicc-fpchecker", path=str(FPCHECKER_BIN)) is not None
    )

    if not (has_clang_wrapper and has_mpi_wrapper):
        enabled_bin = FPCHECKER_BIN if local_bin_has_wrappers else create_local_fpchecker_bin()
        env["PATH"] = str(enabled_bin) + os.pathsep + current_path
        env["FPCHECKER_INSTALL_BIN"] = str(enabled_bin)
        env.setdefault("FPCHECKER_HOME", str(REPO_ROOT))

    missing = [
        tool
        for tool in ("clang++-fpchecker", "mpicc-fpchecker")
        if shutil.which(tool, path=env.get("PATH", "")) is None
    ]
    if missing:
        raise RuntimeError("FPChecker environment is not enabled; missing " + ", ".join(missing))

    return FPCheckerEnvironment(env_name, env, activation_shell)


def run_app(app, fpchecker_runtime):
    cwd = app["cwd"]
    command = app["command"]
    # print("Running {name} in {cwd} with {env_name}".format(name=app["name"],cwd=cwd,env_name=fpchecker_runtime.name,),
    #     file=sys.stderr,)
    if not cwd.is_dir():
        raise RuntimeError("missing proxy app directory: " + str(cwd))

    effective_command = command
    if fpchecker_runtime.activation_shell:
        effective_command = [
            "bash",
            "-lc",
            " && ".join(fpchecker_runtime.activation_shell + [shell_join(command)]),
        ]

    completed = subprocess.run(
        effective_command,
        cwd=str(cwd),
        env=fpchecker_runtime.env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )

    if completed.returncode != 0:
        raise RuntimeError(
            "command failed for {name}: {cmd}\n{output}".format(
                name=app["name"],
                cmd=" ".join(command),
                output=completed.stdout,
            )
        )

    return completed.stdout


def injection_line_map(cwd):
    mapping = {}
    skip_dirs = {".git", ".fpc_logs", "__pycache__"}

    for path in cwd.rglob("*"):
        if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
            continue
        if any(part in skip_dirs for part in path.parts):
            continue

        try:
            lines = path.read_text(errors="ignore").splitlines()
        except OSError:
            continue

        relative_path = path.relative_to(cwd).as_posix()
        for line_number, line in enumerate(lines, 1):
            if "Injection" not in line:
                continue
            marker = line.split("Injection", 1)[1].strip(" */")
            injection_type = marker.split()[0].lower() if marker else "unknown"
            mapping[(path.name, line_number, injection_type)] = relative_path
            mapping[(path.name, line_number, None)] = relative_path

    return mapping


def resolve_injection_line(injection_line, injection_type, path_map):
    try:
        filename, line_number_text = injection_line.rsplit(":", 1)
        line_number = int(line_number_text)
    except ValueError:
        return injection_line

    resolved_path = path_map.get(
        (filename, line_number, injection_type.lower()),
        path_map.get((filename, line_number, None)),
    )
    if resolved_path is None:
        return injection_line
    return "{}:{}".format(resolved_path, line_number)


def parse_result_rows(output, fallback_app_name, path_map):
    rows = []
    in_table = False

    for line in output.splitlines():
        stripped = line.strip()
        if not stripped:
            if in_table and rows:
                break
            continue

        if stripped.startswith("Proxy app") and "Injection line" in stripped:
            in_table = True
            continue

        if not in_table:
            continue

        if set(stripped.replace(" ", "")) == {"-"}:
            continue

        fields = stripped.split()
        if len(fields) < 7:
            if rows:
                break
            continue

        proxy_app = fields[0]
        injection_line = fields[1]
        injection_type = fields[2]
        top_5 = fields[-2]
        top_10 = fields[-1]
        resolved_line = resolve_injection_line(injection_line, injection_type, path_map)

        rows.append(
            {
                "proxy_app": proxy_app or fallback_app_name,
                "injection_line": resolved_line,
                "injection_type": injection_type,
                "top_5": top_5,
                "top_10": top_10,
            }
        )

    return rows


def select_balanced_rows(rows, rows_per_type):
    selected = []
    selected_ids = set()

    for injection_type in INJECTION_TYPES:
        type_rows = [
            (index, row)
            for index, row in enumerate(rows)
            if row["injection_type"].lower() == injection_type
        ]
        used_files = set()

        for index, row in type_rows:
            file_name = row["injection_line"].rsplit(":", 1)[0]
            if file_name in used_files:
                continue
            selected.append(row)
            selected_ids.add(index)
            used_files.add(file_name)
            if len([r for r in selected if r["injection_type"].lower() == injection_type]) == rows_per_type:
                break

        if len([r for r in selected if r["injection_type"].lower() == injection_type]) == rows_per_type:
            continue

        for index, row in type_rows:
            if index in selected_ids:
                continue
            selected.append(row)
            selected_ids.add(index)
            if len([r for r in selected if r["injection_type"].lower() == injection_type]) == rows_per_type:
                break

    if len(selected) < rows_per_type * len(INJECTION_TYPES):
        for index, row in enumerate(rows):
            if index in selected_ids:
                continue
            selected.append(row)
            if len(selected) == rows_per_type * len(INJECTION_TYPES):
                break

    return selected


def print_table(rows):
    headers = ["Proxy app", "Injected line", "Injection type", "Top-5", "Top-10"]
    widths = [len(header) for header in headers]

    for row in rows:
        values = [
            row["proxy_app"],
            row["injection_line"],
            row["injection_type"],
            row["top_5"],
            row["top_10"],
        ]
        widths = [max(width, len(value)) for width, value in zip(widths, values)]

    separator = "+-" + "-+-".join("-" * width for width in widths) + "-+"

    def format_row(values):
        return "| " + " | ".join(value.ljust(widths[index]) for index, value in enumerate(values)) + " |"

    print(separator)
    print(format_row(headers))
    print(separator)
    previous_proxy_app = None
    for row in rows:
        if previous_proxy_app is not None and row["proxy_app"] != previous_proxy_app:
            print(separator)
        print(
            format_row(
                [
                    row["proxy_app"],
                    row["injection_line"],
                    row["injection_type"],
                    row["top_5"],
                    row["top_10"],
                ]
            )
        )
        previous_proxy_app = row["proxy_app"]
    print(separator)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run proxy-app cancellation experiments and summarize top-k results."
    )
    parser.add_argument(
        "--rows-per-app",
        type=int,
        default=DEFAULT_ROWS_PER_APP,
        help="Maximum result rows to keep from each proxy app table.",
    )
    parser.add_argument(
        "--rows-per-type",
        type=int,
        default=DEFAULT_ROWS_PER_TYPE,
        help="Number of standalone and state rows to keep from each proxy app table.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    if args.rows_per_app < 1:
        raise RuntimeError("--rows-per-app must be at least 1")
    if args.rows_per_type < 1:
        raise RuntimeError("--rows-per-type must be at least 1")

    fpchecker_runtime = fpchecker_env()
    all_rows = []

    for app in APP_RUNS:
        output = run_app(app, fpchecker_runtime)
        path_map = injection_line_map(app["cwd"])
        app_rows = parse_result_rows(output, app["name"], path_map)
        if not app_rows:
            raise RuntimeError("could not find result table in output for " + app["name"])
        selected_rows = select_balanced_rows(app_rows, args.rows_per_type)
        all_rows.extend(selected_rows[: args.rows_per_app])

    print_table(all_rows)


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        sys.exit(1)
