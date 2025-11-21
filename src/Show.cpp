#include <iostream>
#include <string>

// The INSTALL_PREFIX macro will be defined by CMake during the build process.
// It will be replaced by a string literal containing the installation path.

int main()
{

#ifndef SHARED_LIB_EXTENSION
    std::cout << "SHARED_LIB_EXTENSION macro not defined. Cannot determine shared library name from build info." << std::endl;
#endif

    // We check if the macro is defined to be safe, though CMake will ensure it.
    std::cout << "========================================" << std::endl;
    std::cout << "         FPChecker Configuration        " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
#ifdef INSTALL_PREFIX
    std::string install_path = INSTALL_PREFIX;
    std::string lib_ext = SHARED_LIB_EXTENSION;
    std::cout << "Installation path: " << install_path << std::endl;
    std::cout << std::endl;
    std::cout << "Add the following to CFLAGS and/or CXXFLAGS:" << std::endl;
    std::cout << std::endl;
    std::cout << "(1) For exceptions checking:" << std::endl;
    std::cout << "-g -include " << install_path << "/src/Runtime_cpu.h -fpass-plugin=" << install_path << "/lib/libfpchecker_cpu" << SHARED_LIB_EXTENSION << std::endl;
    std::cout << std::endl;
    std::cout << "(2) For rounding error tracking:" << std::endl;
    std::cout << "-g -include " << install_path << "/src/Runtime_error.h -fpass-plugin=" << install_path << "/lib/libfpchecker_error" << SHARED_LIB_EXTENSION << std::endl;
    std::cout << std::endl;
    std::cout << "Wrappers are located here:" << std::endl;
    std::cout << install_path << "/bin/clang-fpchecker" << std::endl;
    std::cout << install_path << "/bin/clang++-fpchecker" << std::endl;
    std::cout << install_path << "/bin/mpicc-fpchecker" << std::endl;
    std::cout << install_path << "/bin/mpicxx-fpchecker" << std::endl;
#else
    // This case should theoretically not happen if CMake is configured correctly
    std::cout
        << "INSTALL_PREFIX macro not defined. Cannot determine installation path from build info." << std::endl;
    std::cout << "Note: This program reports the path specified during the CMake configuration step (CMAKE_INSTALL_PREFIX)." << std::endl;
    std::cout << "If the installed directory was moved afterwards, this path will be incorrect." << std::endl;
#endif

    return 0;
}