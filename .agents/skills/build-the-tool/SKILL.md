---
name: build-the-tool
description: Guide for building the tool when asked by the user or testing a code change that requires a new build of the tool
---

To build the tool, follow these steps:

1. Make sure the clang++ compiler is available on your system. You can check this by running `clang++ --version` in your terminal. The current supported version is 19.

2. If not available, a conda enviroment like the following can be loaded:

```bash
source ~/.bashrc
conda activate tutorial_env
```

3. Move to the directory FPChecker/build.

4. If the build directory does not exist, create it and configure it like this:

```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=./install ..
```
5. Build the tool using the following command:

```bash
make -j 4
```

6. Install the tool using the following command:

```bash
make install
```

