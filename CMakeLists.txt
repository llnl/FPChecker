cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

project(fpchecker VERSION 0.6 DESCRIPTION "FPChecker" LANGUAGES CXX C)

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
  add_test(NAME cpu-tests COMMAND Python3::Interpreter -m pytest --tb=no
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/cpu_checking)
  set_tests_properties(cpu-tests PROPERTIES ENVIRONMENT "PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}")
###############################

# Set path of wrappers in config file
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

# Object library for shared source files (compiled once)
add_library(fpchecker_common OBJECT
        src/CodeMatching.cpp
        src/Logging.cpp
        src/Utility.cpp
)
set_target_properties(fpchecker_common PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Library for exception catching in CPU
add_library(fpchecker_cpu SHARED
        src/Instrumentation_cpu.cpp
        src/driver_cpu.cpp
        $<TARGET_OBJECTS:fpchecker_common>
)

# Library for error analysis in CPU
add_library(fpchecker_error SHARED
        src/Instrumentation_error.cpp
        src/driver_error.cpp
        $<TARGET_OBJECTS:fpchecker_common>
)

# !!! We no longer builld this plugin !!!
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
	TARGET_LINK_LIBRARIES(fpchecker_cpu "-undefined dynamic_lookup")
        TARGET_LINK_LIBRARIES(fpchecker_error "-undefined dynamic_lookup")
endif()

if(NOT APPLE)
        TARGET_LINK_LIBRARIES(fpchecker_intercept_lib "-ldl")
endif()

set_target_properties(fpchecker_cpu PROPERTIES
    VERSION ${PROJECT_VERSION}
)

set_target_properties(fpchecker_error PROPERTIES
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

set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

if(APPLE)
install(TARGETS fpchecker_cpu fpchecker_error
        LIBRARY DESTINATION "lib64"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
else()
install(TARGETS fpchecker_intercept_lib fpchecker_cpu fpchecker_error
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

install(FILES "src/Runtime_cpu.h" "src/Runtime_error.h" "src/FPC_Hashtable.h" "src/FPC_Annotations.h" "src/FPC_Hashtable_Error.h" "src/FPC_FloatSeries_List.h"
        DESTINATION "src"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ
)

install(FILES
	"interception_tool/fpchecker.py"
	"interception_tool/colors.py"
	DESTINATION interception 
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

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
install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/cc_frontend.sh ${CMAKE_INSTALL_PREFIX}/bin/clang-fpchecker )"
)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/cxx_frontend.sh ${CMAKE_INSTALL_PREFIX}/bin/clang++-fpchecker )"
)

install(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/cpu_checking/fpc_create_report.py ${CMAKE_INSTALL_PREFIX}/bin/fpc-create-report )"
)

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
