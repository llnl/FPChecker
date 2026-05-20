
## CUDA support has been dropped!

### Limitations of the CUDA Front-End

The FPChecker CUDA fron-end is work-in-progress and has some limitations:
- It doesn't instrument header files (only source files: .cu, .cpp, .c++, etc.)
- It requires using C++11
- The capabilities to parse macros and commets is good, but not perfect yet.
