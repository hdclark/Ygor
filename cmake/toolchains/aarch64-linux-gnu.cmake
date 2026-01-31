# Example CMake toolchain file for cross-compiling to ARM64/aarch64
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake ..
#
# Prerequisites:
#   - aarch64-linux-gnu cross-compiler
#   - Cross-compiled dependencies

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross-compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)

# Search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Architecture flag for aarch64
set(CMAKE_CXX_FLAGS_INIT "-mcpu=generic")
