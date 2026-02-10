# Example CMake toolchain file for musl libc (Alpine Linux / embedded)
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/musl-linux.cmake ..
#
# Prerequisites:
#   - musl-gcc or a musl-based cross-compiler
#   - Cross-compiled dependencies

set(CMAKE_SYSTEM_NAME Linux)

# Option 1: Using musl-gcc wrapper (on systems with musl installed)
# set(CMAKE_C_COMPILER musl-gcc)
# set(CMAKE_CXX_COMPILER musl-g++)

# Option 2: Using musl cross-compiler (adjust prefix as needed)
set(MUSL_PREFIX "x86_64-linux-musl" CACHE STRING "musl cross-compiler prefix")
set(CMAKE_C_COMPILER "${MUSL_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "${MUSL_PREFIX}-g++")

# Target environment (adjust based on your setup)
set(CMAKE_FIND_ROOT_PATH "/usr/${MUSL_PREFIX}")

# Search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# musl typically prefers static linking
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries for musl")
