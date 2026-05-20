## Release 0.6
- Added FP32 rounding-error tracking (shadow-value style), including improved accumulation across operations and files.
- Added line-granularity reporting for accumulated rounding error, including per-line visualizations in reports.
- Reports high-error hotspots that can help guide mixed-precision and lower-precision tuning decisions.
- Improved reporting outputs: better HTML layout, shell-friendly rounding-error tables, and support for multi-file rounding sections.
- Expanded error-analysis coverage for math functions, floating-point return values/calls, and MPI-related paths.
- Added substantial regression tests for FP32 error analysis and report generation.
- Fixed multiple bugs and memory leaks in analysis and reporting components.

## Release 0.5
- Improved histrograms of exponent usage (added to main report). Merged branches.
- Support for CUDA has been dropped
- Design of the report changed
- New tests added
- Fixed several bugs

## Release 0.4
- Added histrograms of exponent usage
- Fixed several bugs

## Release 0.3.5
- Fixed various bugs
- Better supporort for strings in compilation commands (e.g., -D\"string\")

## Release 0.3.4
- Fixed some bugs

## Release 0.3.3
- Fixed some bugs

## Release 0.3.2
- Added support to exit on events
- Added support for printing text report
- Saves program inputs in .json trace files
- Fixed minor bugs

## Release 0.3.1
- Fixed a number of minor bugs
- New tests added

## Release 0.3.0
- Added support to check in CPU code
- New documentation available in https://fpchecker.org/
- HTML reports provided
- Added warppers for C/C++ and MPI

## Release 0.2.1
- Added 'fpchecker' tool that intercepts all calls to nvcc from build script (e..g, make)
- The interception tool uses the Linux LD_PRELOAD trick to intercept execve system calls

## Release 0.2.0
- Added a custom front-end that instruments CUDA. This front-end doesn't require clang/LLVM.
- The new front-end is work in progress and has limitations
- Fixed several bugs
- The front-end version doesn't abort on errors by default.

## Release 0.1.2
- Added clang version. It instruments source code via clang plugin. Instrumented code can be compiled with nvcc.
- Added new flags: FPC_DISABLE_SUBNORMAL, FPC_DISABLE_WARNINGS 
- Added clang wrapper to instrument code: clang-fpchecker

## Release 0.1.0
- First stable release
- Tests use pytest

## Release 0.0.4
- When errors-dont-abort, it reports the class of error
- Various code cleaning commits
- Added some tests and cahnge how test config
- Known issue: When errors-dont-abort, we only perform FP64 checks

## Release 0.0.3
- Warning reports printed once only (in ERRORS_DONT_ABORT mode)

## Release 0.0.2
- Supports functionality to avoid aborting the program when an error/warning is found (-D FPC_ERRORS_DONT_ABORT)
- Program prints version when main() starts
- This version is not MPI-aware (all ranks print)
- Includes more tests (e.g., RAJA loop)

## Release 0.0.1
- First complete version
- It supports error and warning reports
- All reports (errors and warnings) abort after detected and printed



