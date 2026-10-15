ARG BASE_IMAGE=localhost/cgo2026-artifact:latest
FROM ${BASE_IMAGE}

USER root

RUN /bin/bash -lc 'set -e; \
    if command -v gdb >/dev/null 2>&1; then exit 0; fi; \
    if command -v apt-get >/dev/null 2>&1; then \
        apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends gdb && rm -rf /var/lib/apt/lists/*; \
    elif command -v dnf >/dev/null 2>&1; then \
        dnf install -y gdb && dnf clean all; \
    elif command -v yum >/dev/null 2>&1; then \
        yum install -y gdb && yum clean all; \
    elif command -v microdnf >/dev/null 2>&1; then \
        microdnf install -y gdb && microdnf clean all; \
    elif command -v conda >/dev/null 2>&1; then \
        conda install -y -c conda-forge gdb && conda clean -afy; \
    else \
        echo "gdb not found and no supported package manager is available; EFTSan error extraction requires gdb." >&2; \
        exit 1; \
    fi'

RUN /bin/bash -lc 'set -e; \
    if ! command -v conda >/dev/null 2>&1; then \
        echo "conda not found; cannot install EFTSan MPFR/GMP dependencies." >&2; \
        exit 1; \
    fi; \
    target_env=""; \
    for candidate in /opt/conda/envs/llvm10 /opt/conda/envs/eftsan_env; do \
        if [ -d "$candidate" ]; then target_env="$candidate"; break; fi; \
    done; \
    if [ -z "$target_env" ]; then \
        echo "proper environment not found; running is not working for EFTSan. Expected llvm10 or eftsan_env." >&2; \
        exit 1; \
    fi; \
    if [ -e "$target_env/lib/libmpfr.so" ] && [ -e "$target_env/lib/libgmp.so" ]; then \
        exit 0; \
    fi; \
    conda install -y -p "$target_env" -c conda-forge mpfr gmp; \
    conda clean -afy'

RUN rm -rf /opt/cgo2026_artifact_fpchecker/fpchecker \
    && mkdir -p /opt/cgo2026_artifact_fpchecker/fpchecker

WORKDIR /opt/cgo2026_artifact_fpchecker/fpchecker

COPY . /opt/cgo2026_artifact_fpchecker/fpchecker

RUN test -f experiments/compare_error_baselines.py \
    && test -f experiments/EFTSanitizer/generate_eftsan_error_report_up.py \
    || (echo "Build this image from the FPChecker repo root, for example: docker build -f cgo2026_artifact_fpchecker/Dockerfile ." >&2; exit 1)

RUN /bin/bash -lc 'set -e; \
    fp_env=""; \
    for candidate in /opt/conda/envs/tutorial_env /opt/conda/envs/fpchecker_env; do \
        if [ -d "$candidate" ]; then fp_env="$candidate"; break; fi; \
    done; \
    if [ -z "$fp_env" ]; then \
        echo "proper environment not found; running is not working for FPChecker. Expected tutorial_env or fpchecker_env." >&2; \
        exit 1; \
    fi; \
    export PATH="$fp_env/bin:$PATH"; \
    export LD_LIBRARY_PATH="$fp_env/lib:${LD_LIBRARY_PATH:-}"; \
    if ! command -v clang++ >/dev/null 2>&1; then \
        echo "proper environment not found; running is not working for FPChecker. clang++ is missing from $fp_env." >&2; \
        exit 1; \
    fi; \
    cmake -S . -B build-cgo2026 -DCMAKE_INSTALL_PREFIX=/opt/cgo2026_artifact_fpchecker/fpchecker/install; \
    cmake --build build-cgo2026 --parallel 4; \
    cmake --install build-cgo2026; \
    ln -sf /opt/cgo2026_artifact_fpchecker/fpchecker/install/bin/fpchecker /usr/local/bin/fpchecker; \
    ln -sf /opt/cgo2026_artifact_fpchecker/fpchecker/install/bin/clang-fpchecker /usr/local/bin/clang-fpchecker; \
    ln -sf /opt/cgo2026_artifact_fpchecker/fpchecker/install/bin/clang++-fpchecker /usr/local/bin/clang++-fpchecker; \
    ln -sf /opt/cgo2026_artifact_fpchecker/fpchecker/install/bin/fpc-create-report /usr/local/bin/fpc-create-report; \
    ln -sf /opt/cgo2026_artifact_fpchecker/fpchecker/install/bin/fpchecker-show /usr/local/bin/fpchecker-show'

ENV PATH="/opt/cgo2026_artifact_fpchecker/fpchecker/install/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/cgo2026_artifact_fpchecker/fpchecker/install/lib64:${LD_LIBRARY_PATH}"

CMD ["/bin/bash", "-l"]
