FROM rockylinux:8
LABEL org.opencontainers.image.title="cgo2026-artifact" \
      org.opencontainers.image.description="Branch-flip detection: FPChecker, EFTSanitizer, NSan vs brtrace ground truth" \
      org.opencontainers.image.source="https://github.com/llnl/FPChecker"

ENV ARTIFACT_ROOT=/opt/cgo2026_artifact \
    CONDA_ROOT=/opt/conda \
    FPC_SRC=/opt/cgo2026_artifact/fpchecker_bf \
    FPC_REPO=https://github.com/llnl/FPChecker.git \
    FPC_BRANCH=v0.7_new_runtime_branch_flip \
    EFT_HOME=/opt/cgo2026_artifact/EFTSanitizer \
    EFT_REPO=https://github.com/rutgers-apl/EFTSanitizer.git \
    EFT_SHA=7a77c592a1dd9fc4beace964c04c973de36ad769 \
    MINICONDA=Miniconda3-py312_25.1.1-2-Linux-x86_64.sh \
    PATH=/opt/conda/bin:$PATH
ENV ENV_SETUP=$FPC_SRC/cpu_checking/error_analysis/env_setup \
    EXP=$FPC_SRC/cpu_checking/error_analysis/branch_flip/experiments \
    EFT_PATCH=$FPC_SRC/cpu_checking/error_analysis/branch_flip/experiments/eftsan_experiments/patches/eftsan_branchflip.patch

SHELL ["/bin/bash", "-c"]

# RHEL 8 / glibc 2.28, matching the reference host so libm results are bit-identical
RUN dnf -y install gcc gcc-c++ make git curl bzip2 xz file patch which binutils findutils diffutils \
    && dnf clean all

RUN curl -fsSL "https://repo.anaconda.com/miniconda/${MINICONDA}" -o /tmp/miniconda.sh \
    && bash /tmp/miniconda.sh -b -p "$CONDA_ROOT" && rm /tmp/miniconda.sh \
    && printf 'channels:\n  - conda-forge\nchannel_priority: strict\nauto_activate_base: false\n' > "$CONDA_ROOT/.condarc" \
    && conda clean -afy

# ---------- FPChecker source ----------
RUN mkdir -p "$ARTIFACT_ROOT" \
    && git clone --branch "$FPC_BRANCH" --single-branch "$FPC_REPO" "$FPC_SRC" \
    && cd "$FPC_SRC" && git log -1 --oneline \
    && chmod +x "$ENV_SETUP"/*.sh \
    && for s in fpchecker eftsan nsan; do ln -s "$ENV_SETUP/activate_${s}_env.sh" /usr/local/bin/; done

# ---------- conda environments ----------
RUN "$ENV_SETUP/create_fpchecker_env.sh" && conda clean -afy
RUN "$ENV_SETUP/create_eftsan_env.sh"    && conda clean -afy
RUN "$ENV_SETUP/create_nsan_env.sh"      && conda clean -afy

# ---------- FPChecker: build and install ----------
RUN source activate_fpchecker_env.sh \
    && mkdir -p "$FPC_SRC/build" && cd "$FPC_SRC/build" \
    && cmake .. -DCMAKE_INSTALL_PREFIX="$FPC_SRC/install" \
                -DPython3_ROOT_DIR="$CONDA_ROOT/envs/fpchecker_env" \
                -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    && make -j"$(nproc)" && make install \
    && test -x "$FPC_SRC/install/bin/clang++-fpchecker" \
    && test -f "$FPC_SRC/install/src/FPC_SiteCounter.h"

# ---------- brtrace (ground truth) ----------
RUN source activate_fpchecker_env.sh \
    && cd "$EXP/gt_experiments/brtrace" && bash build_mtu.sh \
    && test -f libBranchTrace_mtu.so && test -f brtrace_runtime_mtu.o

# ---------- EFTSanitizer: pinned SHA + patch, pass, runtime ----------
RUN git clone "$EFT_REPO" "$EFT_HOME" && cd "$EFT_HOME" \
    && git checkout "$EFT_SHA" \
    && git apply "$EFT_PATCH" && git --no-pager diff --stat
RUN source activate_eftsan_env.sh \
    && mkdir -p "$EFT_HOME/llvm_pass/build" && cd "$EFT_HOME/llvm_pass/build" \
    && cmake .. -DLLVM_DIR="$CONDA_PREFIX/lib/cmake/llvm" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    && make && test -f EFTSan/libEFTSanitizer.so
RUN source activate_eftsan_env.sh \
    && cd "$EFT_HOME/runtime" && rm -rf obj && make \
    && test -f obj/libeftsanitizer.so \
    && strings obj/handleReal.o | grep "clang version 10" >/dev/null

# ---------- NSan: compiler-rt runtime, pass plugin, shim ----------
RUN source activate_nsan_env.sh \
    && cd "$EXP/nsan_experiments/nsan" \
    && bash nsan_setup.sh -j "$(nproc)" \
    && bash build_instrumentation.sh \
    && test -f plugin/libNsanBFSites.so && test -f runtime/libnsan_bf.a

# ---------- experiment driver ----------
ENV BF=$ARTIFACT_ROOT/branch_flip
COPY run_experiments.sh branch_flip_tables.py $BF/
RUN mkdir -p "$BF/results" && chmod +x "$BF/run_experiments.sh" "$BF/branch_flip_tables.py" \
    && printf '%s\n' \
        'source /opt/conda/etc/profile.d/conda.sh' \
        'echo "cgo2026-artifact  --  bash run_experiments.sh [--quick] [--tools nsan,eftsan,fpchecker]"' \
        >> /root/.bashrc

WORKDIR $BF
CMD ["/bin/bash", "-l"]