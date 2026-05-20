---
name: write-tests
description: Guide for writing tests in the project
---

To write new tests in the project follow theese:

1. For tests related to error tracking, add a directory in tests/cpu_checking/error_analysis/ with the name of the test. 
2. Inside that directory, create a file named `main.cpp` and write your test cases using pytest.
3. As an example of tests, see: test_fp32_dot_product, test_fp32_average, test_fp32_nested_loop.
4. For now, only write tests in FP32 precision.
5. There must be a pytest file with the name of the directory and extesion .py, for example: test_fp32_dot_product.py.
6. To test if the tool works correctly, we write a program in FP32 precision and instrument the code. Then we run the same function in FP64 precision and compare the results. The test should fail if the results are different, and pass if they are similar.
7. To build the Mkefile, see the examples.
8. To run the tests, use the following command:

```bash
ml python
pytest tests/cpu_checking/error_analysis/<test_directory>
```

## Enssure the clang tool is installed and available:

```bash
source ~/.bashrc
conda activate tutorial_env
```