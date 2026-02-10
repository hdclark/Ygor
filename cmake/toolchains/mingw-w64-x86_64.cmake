# Example CMake toolchain file for cross-compiling to Windows (MinGW-w64)
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake ..
#
# Prerequisites:
#   - MinGW-w64 cross-compiler (x86_64-w64-mingw32-g++)
#   - Cross-compiled dependencies

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Executable suffix
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# Disable Linux-specific features for Windows target
set(WITH_LINUX_SYS OFF CACHE BOOL "Disable Linux-specific sys interfaces for Windows")
