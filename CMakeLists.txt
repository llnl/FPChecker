cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

project(fpchecker VERSION 0.5 DESCRIPTION "FPChecker" LANGUAGES CXX C)

execute_process(COMMAND llvm-config --ldflags 
OUTPUT_VARIABLE CMAKE_SHARED_LINKER_FLAGS 
OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND llvm-config --cxxflags
OUTPUT_VARIABLE CMAKE_CXX_FLAGS
OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND llvm-config --cppflags
OUTPUT_VARIABLE CMAKE_CPP_FLAGS 
OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -g -std=c++17")

include(GNUInstallDirs)
include(CheckLanguage)

###################################
## Check for python3 and packages #
###################################
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(Python3_FOUND AND Python3_Interpreter_FOUND)
  message(STATUS "Found Python3: ${Python3_EXECUTABLE}")

  # Define a variable for the package name
  set(PYTHON_PACKAGE_NAME "matplotlib")

  message(STATUS "Checking for Python package: ${PYTHON_PACKAGE_NAME}")

  # Script to check for the package
set(PYTHON_CHECK_SCRIPT
    "import sys\ntry:\n   import ${PYTHON_PACKAGE_NAME}\n   sys.exit(0)\nexcept ImportError:\n   sys.stderr.write('Python package \\'${PYTHON_PACKAGE_NAME}\\' not found.')\n   sys.exit(1)\n"
)

  # Execute the Python script
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "${PYTHON_CHECK_SCRIPT}"
    RESULT_VARIABLE PYTHON_PACKAGE_CHECK_RESULT
    OUTPUT_VARIABLE PYTHON_PACKAGE_CHECK_OUTPUT
    ERROR_VARIABLE PYTHON_PACKAGE_CHECK_ERROR
  )

  # Check the result of the execution
  if(NOT PYTHON_PACKAGE_CHECK_RESULT EQUAL 0)
    message(NOTICE "Could not find the Python package '${PYTHON_PACKAGE_NAME}'.\n${PYTHON_PACKAGE_CHECK_ERROR}")
    message(WARNING "Python3 and matplotlib are required to create reports.")
  else()
    message(STATUS "Python package '${PYTHON_PACKAGE_NAME}' found.")
  endif()

else()
  message(WARNING "Python3 not found. Python3 and matplotlib are required to create reports.")
endif()

###### Enable testing ##########
enable_testing()
include(CTest)
#find_package(Python3 COMPONENTS Interpreter)
#if(Python3_Interpreter_FOUND)
  add_test(NAME cpu-tests COMMAND Python3::Interpreter -m pytest --tb=no
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/cpu_checking)
  set_tests_properties(cpu-tests PROPERTIES ENVIRONMENT "PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}")
#endif()
###############################

# Set Path pf wrappers in config file
#set(NVCC_WRAPPER "${CMAKE_INSTALL_PREFIX}/bin/nvcc-fpchecker")
set(CLANG_WRAPPER "${CMAKE_INSTALL_PREFIX}/bin/clang-fpchecker")
set(CLANGPP_WRAPPER "${CMAKE_INSTALL_PREFIX}/bin/clang++-fpchecker")
set(MPI_WRAPPER "${CMAKE_INSTALL_PREFIX}/bin/mpicc-fpchecker")
set(MPIPP_WRAPPER "${CMAKE_INSTALL_PREFIX}/bin/mpicxx-fpchecker")
configure_file(interception_tool/intercept.h.in intercept.h)

# !!! We no longer builld the CUDA (old version) !!!
#add_library(fpchecker SHARED 
#	src/CodeMatching.cpp
#	src/Instrumentation.cpp
#	src/Logging.cpp
#	src/Utility.cpp
#	src/driver.cpp
#)

add_executable(fpchecker-show src/Show.cpp)

add_library(fpchecker_cpu SHARED
	src/CodeMatching.cpp
	src/Instrumentation_cpu.cpp
	src/Logging.cpp
	src/Utility.cpp
	src/driver_cpu.cpp
)

#add_library(fpchecker_plugin SHARED
#	plugin/instrumentation_plugin.cpp
#)

if(NOT APPLE)
add_library(fpchecker_intercept_lib SHARED
	interception_tool/intercept.c
)

target_include_directories(fpchecker_intercept_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(APPLE)
	#TARGET_LINK_LIBRARIES(fpchecker "-undefined dynamic_lookup")
	TARGET_LINK_LIBRARIES(fpchecker_cpu "-undefined dynamic_lookup")
	#TARGET_LINK_LIBRARIES(fpchecker_plugin "-Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
endif()

if(NOT APPLE)
TARGET_LINK_LIBRARIES(fpchecker_intercept_lib "-ldl")
endif()

set_target_properties(fpchecker_cpu PROPERTIES
    VERSION ${PROJECT_VERSION}
)

# Set extension of shared library
if(APPLE)
set(LIB_EXTENSION ".dylib")
else()
set(LIB_EXTENSION ".so")
endif()

target_compile_definitions(fpchecker-show
    PRIVATE # This definition is only needed by this target itself
    INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    SHARED_LIB_EXTENSION="${LIB_EXTENSION}"
)

#set_target_properties(fpchecker PROPERTIES
#    VERSION ${PROJECT_VERSION}
#)

#target_include_directories(fpchecker PRIVATE src)

set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

if(APPLE)
install(TARGETS fpchecker_cpu
        LIBRARY DESTINATION "lib64"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
else()
#install(TARGETS fpchecker fpchecker_plugin fpchecker_intercept_lib fpchecker_cpu
install(TARGETS fpchecker_intercept_lib fpchecker_cpu
        LIBRARY DESTINATION "lib64"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
endif()

# Install link for /lib
install(CODE "execute_process( \
        COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/lib64 ${CMAKE_INSTALL_PREFIX}/lib )"
        COMMENT "Creating link: /lib --> /lib64"
)

#install(FILES "src/Runtime.h" "src/Runtime_plugin.h" "src/Runtime_parser.h" "src/Runtime_cpu.h" "src/FPC_Hashtable.h"
install(FILES "src/Runtime_cpu.h" "src/FPC_Hashtable.h" "src/FPC_Annotations.h"
        DESTINATION "src"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ
)

install(FILES
	"interception_tool/fpchecker.py"
	"interception_tool/colors.py"
	DESTINATION interception 
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

#install(FILES
#        "parser/colors.py"
#        "parser/config_reader.py"
#        "parser/deprocess.py"
#        "parser/exceptions.py"
#        "parser/fpc-debug.py"
#        "parser/fpc_logging.py"
#        "parser/g++_fpchecker.py"
#        "parser/instrument.py"
#        "parser/match.py"
#        "parser/nvcc_fpchecker.py"
#        "parser/tokenizer.py"
#        DESTINATION front-end
#        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#)

install(FILES
        "cpu_checking/cc_mpi_frontend.sh"
        "cpu_checking/cc_frontend.sh"
        "cpu_checking/cxx_mpi_frontend.sh"
        "cpu_checking/cxx_frontend.sh"
        "cpu_checking/clang_fpchecker.py"
        "cpu_checking/colors.py"
        "cpu_checking/exceptions.py"
        "cpu_checking/fpc_create_report.py"
        "cpu_checking/fpc_logging.py"
        "cpu_checking/line_highlighting.py"
        "cpu_checking/mpicc_fpchecker.py"
        DESTINATION cpu_checking
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

install(DIRECTORY "cpu_checking/report_templates/" DESTINATION cpu_checking/report_templates)

install(DIRECTORY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        PATTERN "*"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Install link for fpchecker wrappers
#install(CODE "execute_process( \
#        COMMAND ${CMAKE_COMMAND} -E create_symlink \
#        ${CMAKE_INSTALL_PREFIX}/front-end/nvcc_fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/nvcc-fpc )"
#        COMMENT "Creating link: nvcc-fpc -> nvcc_fpchecker.py"
#)

#install(CODE "execute_process( \
#        COMMAND ${CMAKE_COMMAND} -E create_symlink \
#        ${CMAKE_INSTALL_PREFIX}/front-end/nvcc_fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/nvcc-fpchecker )"
#        COMMENT "Creating link: nvcc-fpchecker -> nvcc_fpchecker.py"
#)

# Install link for fpchecker reports tool
install(CODE "execute_process( \
        COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/front-end/fpc-debug.py ${CMAKE_INSTALL_PREFIX}/bin/fpc-report )"
        COMMENT "Creating link: fpc-report -> fpc-debug.py"
)

# Install link for fpchecker reports tool
install(CODE "execute_process( \
        COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/interception/fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/fpchecker )"
        COMMENT "Creating link: fpchecker -> fpchecker.py"
)

# Clang wrappers
#install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
#        ${CMAKE_INSTALL_PREFIX}/cpu_checking/clang_fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/clang-fpchecker )"
#)

#install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
#        ${CMAKE_INSTALL_PREFIX}/cpu_checking/clang_fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/clang++-fpchecker )"
#)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/cc_frontend.sh ${CMAKE_INSTALL_PREFIX}/bin/clang-fpchecker )"
)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/cxx_frontend.sh ${CMAKE_INSTALL_PREFIX}/bin/clang++-fpchecker )"
)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/fpc_create_report.py ${CMAKE_INSTALL_PREFIX}/bin/fpc-create-report )"
)

#install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
#        ${CMAKE_INSTALL_PREFIX}/cpu_checking/mpicc_fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/mpic++-fpchecker )"
#)

#install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
#        ${CMAKE_INSTALL_PREFIX}/cpu_checking/mpicc_fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/mpicxx-fpchecker )"
#)

#install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
#        ${CMAKE_INSTALL_PREFIX}/cpu_checking/mpicc_fpchecker.py ${CMAKE_INSTALL_PREFIX}/bin/mpicc-fpchecker )"
#)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/cxx_mpi_frontend.sh ${CMAKE_INSTALL_PREFIX}/bin/mpic++-fpchecker )"
)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/cxx_mpi_frontend.sh ${CMAKE_INSTALL_PREFIX}/bin/mpicxx-fpchecker )"
)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/cc_mpi_frontend.sh ${CMAKE_INSTALL_PREFIX}/bin/mpicc-fpchecker )"
)

install(TARGETS fpchecker-show
    DESTINATION bin
)

# Tests
#file(COPY tests DESTINATION ${CMAKE_BINARY_DIR})
#file(COPY src/Runtime.h DESTINATION ${CMAKE_BINARY_DIR}/src)
#configure_file(tests/tests_Makefile.in tests/Makefile.config)
#configure_file(tests/llvm/static/_test_config.py.in tests/static/test_config.py)
#configure_file(tests/llvm/dynamic/_test_config.py.in tests/dynamic/test_config.py)
#add_custom_target(tests COMMAND bash ${CMAKE_BINARY_DIR}/tests/tests.sh)
#find_package(Python3 COMPONENTS Interpreter)

if(DEFINED FPC_DEBUG AND "${FPC_DEBUG}" STREQUAL "ON")
	message(STATUS "FPC_DEBUG definition is set to: ${FPC_DEBUG}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFPC_DEBUG")
endif()

message(STATUS "CMAKE_CXX_COMPILE_FLAGS: " ${CMAKE_CXX_COMPILE_FLAGS})
message(STATUS "CMAKE_CXX_COMPILER: " ${CMAKE_CXX_COMPILER})
message(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message(STATUS "CMAKE_CPP_FLAGS: " ${CMAKE_CPP_FLAGS})
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: " ${CMAKE_SHARED_LINKER_FLAGS})
message(STATUS "CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX})
