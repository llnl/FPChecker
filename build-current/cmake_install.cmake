# Install script for directory: /g/g90/sharmin1/tutorial/FPChecker

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/g/g90/sharmin1/tutorial/FPChecker/build-current/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS)
  set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS "OWNER_READ;OWNER_WRITE;OWNER_EXECUTE;GROUP_READ;GROUP_WRITE;GROUP_EXECUTE;WORLD_READ;WORLD_EXECUTE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_intercept_lib.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_intercept_lib.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_intercept_lib.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/libfpchecker_intercept_lib.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_intercept_lib.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_intercept_lib.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_intercept_lib.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so.0.6" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so.0.6")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so.0.6"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/libfpchecker_cpu.so.0.6")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so.0.6" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so.0.6")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so.0.6")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/libfpchecker_cpu.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_cpu.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so.0.6" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so.0.6")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so.0.6"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/libfpchecker_error.so.0.6")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so.0.6" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so.0.6")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so.0.6")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/libfpchecker_error.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so.0.6" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so.0.6")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so.0.6"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/libfpchecker_error_fp64.so.0.6")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so.0.6" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so.0.6")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so.0.6")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/libfpchecker_error_fp64.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libfpchecker_error_fp64.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process(         COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/lib64 /g/g90/sharmin1/tutorial/FPChecker/build-current/install/lib )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/src" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ FILES
    "/g/g90/sharmin1/tutorial/FPChecker/src/Runtime_cpu.h"
    "/g/g90/sharmin1/tutorial/FPChecker/src/Runtime_error.h"
    "/g/g90/sharmin1/tutorial/FPChecker/src/Runtime_error_FP64.h"
    "/g/g90/sharmin1/tutorial/FPChecker/src/FPC_Hashtable.h"
    "/g/g90/sharmin1/tutorial/FPChecker/src/FPC_Annotations.h"
    "/g/g90/sharmin1/tutorial/FPChecker/src/FPC_Hashtable_Error.h"
    "/g/g90/sharmin1/tutorial/FPChecker/src/FPC_Hashtable_Error_FP64.h"
    "/g/g90/sharmin1/tutorial/FPChecker/src/FPC_FloatSeries_List.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/interception" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES
    "/g/g90/sharmin1/tutorial/FPChecker/interception_tool/fpchecker.py"
    "/g/g90/sharmin1/tutorial/FPChecker/interception_tool/colors.py"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cpu_checking" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/cc_mpi_frontend.sh"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/cc_frontend.sh"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/cxx_mpi_frontend.sh"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/cxx_frontend.sh"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/clang_fpchecker.py"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/colors.py"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/exceptions.py"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/fpc_create_report.py"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/fpc_logging.py"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/line_highlighting.py"
    "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/mpicc_fpchecker.py"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cpu_checking/report_templates" TYPE DIRECTORY FILES "/g/g90/sharmin1/tutorial/FPChecker/cpu_checking/report_templates/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin" TYPE DIRECTORY FILES "" REGEX "/[^/]*$" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process(         COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/front-end/fpc-debug.py /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/fpc-report )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process(         COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/interception/fpchecker.py /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/fpchecker )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process( COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/cpu_checking/cc_frontend.sh /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/clang-fpchecker )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process( COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/cpu_checking/cxx_frontend.sh /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/clang++-fpchecker )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process( COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/cpu_checking/fpc_create_report.py /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/fpc-create-report )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process( COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/cpu_checking/cxx_mpi_frontend.sh /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/mpic++-fpchecker )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process( COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/cpu_checking/cxx_mpi_frontend.sh /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/mpicxx-fpchecker )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  execute_process( COMMAND /usr/tce/backend/installations/linux-rhel8-x86_64/gcc-8.3.1/cmake-3.24.2-ywx52e32uh6gkxzuyubpwkulzgdvxyh6/bin/cmake -E create_symlink         /g/g90/sharmin1/tutorial/FPChecker/build-current/install/cpu_checking/cc_mpi_frontend.sh /g/g90/sharmin1/tutorial/FPChecker/build-current/install/bin/mpicc-fpchecker )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/fpchecker-show" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/fpchecker-show")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/fpchecker-show"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/g/g90/sharmin1/tutorial/FPChecker/build-current/fpchecker-show")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/fpchecker-show" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/fpchecker-show")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/fpchecker-show")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/g/g90/sharmin1/tutorial/FPChecker/build-current/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
