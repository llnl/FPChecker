FROM localhost/cgo2026-artifact:latest

USER root

RUN git clone \
    --branch v0.7_new_runtime \
    --single-branch \
    https://github.com/llnl/FPChecker.git \
    /opt/cgo2026_artifact/fpchecker

WORKDIR /opt/cgo2026_artifact/fpchecker

CMD ["/bin/bash", "-l"]
