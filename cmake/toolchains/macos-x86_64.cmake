# Example CMake toolchain file for cross-compiling to macOS (osxcross)
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/macos-x86_64.cmake ..
#
# Prerequisites:
#   - osxcross toolchain (https://github.com/tpoechtrager/osxcross)
#   - Cross-compiled dependencies

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Adjust these paths based on your osxcross installation
set(OSXCROSS_ROOT "$ENV{HOME}/osxcross" CACHE PATH "Path to osxcross installation")
set(OSXCROSS_TARGET "darwin20.4" CACHE STRING "osxcross target")

# Cross-compiler
set(CMAKE_C_COMPILER "${OSXCROSS_ROOT}/bin/x86_64-apple-${OSXCROSS_TARGET}-clang")
set(CMAKE_CXX_COMPILER "${OSXCROSS_ROOT}/bin/x86_64-apple-${OSXCROSS_TARGET}-clang++")

# Target environment
set(CMAKE_FIND_ROOT_PATH "${OSXCROSS_ROOT}/SDK/MacOSX11.3.sdk")

# Search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# macOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

# Disable Linux-specific features for macOS target
set(WITH_LINUX_SYS OFF CACHE BOOL "Disable Linux-specific sys interfaces for macOS")
