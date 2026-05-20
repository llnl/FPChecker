<!--
# FPChecker
-->
<img src="figures/logo_fpchecker.png" />

[![Build Status](https://travis-ci.org/LLNL/FPChecker.svg?branch=master)](https://travis-ci.org/LLNL/FPChecker)

**FPChecker** (Floating-Point Checker) is a dynamic analysis tool for profiling floating-point behavior in HPC applications.
It gives developers clear, execution-based insight into how floating-point arithmetic behaves
under real workloads and is currently the only tool in its class tailored to the HPC domain.
Designed for straightforward adoption, FPChecker integrates with existing build workflows and
produces detailed reports that pinpoint the exact file and line locations of numerical issues,
including exceptions, high accumulated rounding error, cancellation, and related events.
Its rounding-error reports also support data-driven mixed-precision tuning by highlighting
code locations where promotion to higher precision is most beneficial.

## Documentation

To see how to install and use FPChecker, visit: [https://fpchecker.org/](https://fpchecker.org/)

## Features

- **Easy adoption:** FPChecker requires only minor build-script updates, such as replacing 
the compiler invocation (for example, `clang++`) with FPChecker wrappers (for example, `clang++-fpchecker`). 
Instrumentation is then applied automatically at build time.
- **Execution-accurate detection:** FPChecker reports issues based on actual execution for the selected inputs, 
reducing false alarms from paths that are not exercised.
- **Built for HPC workflows:** FPChecker supports key HPC languages and programming models, including C/C++, MPI, 
and, in selected modes, OpenMP and Pthreads.
- **Actionable reporting:** FPChecker produces detailed reports that identify the exact source 
location (file and line) of floating-point issues.
- **Mixed-precision guidance:** Rounding-error accumulation reports highlight high-risk lines, 
helping users prioritize selective FP32-to-FP64 promotion based on measured numerical impact.

### Contact
For questions, contact Ignacio Laguna <ilaguna@llnl.gov>.

To cite FPChecker please use

```
Laguna, Ignacio. "FPChecker: Detecting Floating-point Exceptions in GPU Applications."
In 2019 34th IEEE/ACM International Conference on Automated Software Engineering (ASE), 
pp. 1126-1129. IEEE, 2019.
```

or

```
Ignacio Laguna, Tanmay Tirpankar, Xinyi Li, and Ganesh Gopalakrishnan. 
"Fpchecker: Floating-point exception detection tool and benchmark for 
parallel and distributed HPC." In 2022 IEEE International Symposium 
on Workload Characterization (IISWC), pp. 39-50. IEEE, 2022.
```

## License

FPChecker is distributed under the terms of the Apache License (Version 2.0).

All new contributions must be made under the Apache-2.0 license.

See LICENSE and NOTICE for details.

LLNL-CODE-769426
