
cmake_minimum_required (VERSION 3.1.0 FATAL_ERROR)
project (Ygor)

set (Ygor_VERSION_MAJOR 1)
set (Ygor_VERSION_MINOR 0)
set (Ygor_VERSION_PATCH 0)

# Set the release type. 
set(CMAKE_BUILD_TYPE Release) # Comment this line to use fallback default.
if(NOT CMAKE_BUILD_TYPE)
    # Default to debug builds.
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "default to debug" FORCE)
endif()

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(BUILD_SHARED_LIBS TRUE)
SET(POSITION_INDEPENDENT_CODE TRUE)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -frounding-math -fno-finite-math-only")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always -fno-var-tracking-assignments")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wall -Wextra -Wno-strict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy et al.

# Use the directory where CMakeLists.txt is for inclusions.
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(binaries)


set(CPACK_GENERATOR "DEB")
#SET(CPACK_PACKAGE_NAME "ygor")

STRING(TIMESTAMP INVOCATION_TIMESTAMP "%Y%m%d.%H%M%S") # For a time-based version number.
SET(CPACK_PACKAGE_VERSION "${INVOCATION_TIMESTAMP}")

# Dependencies, e.g., "libc6 (>= 2.3.1-6), libgcc1 (>= 1:3.4.2-12)"
SET(CPACK_DEBIAN_PACKAGE_DEPENDS 
   "libhtmlcxx-dev,
    libboost-dev,
    libgsl-dev")

# Recommended or optional packages, e.g., "liboptional-dev (>= 1.2.3-1), libmaybe-dev (>= 1:1.3.2-10)"
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS 
   "libeigen3-dev")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64") # i386, amd64, armel, armhf, ...
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "D.R.Y. support library with scientific emphasis.")
SET(CPACK_PACKAGE_CONTACT "hdeanclark@gmail.com")
SET(CPACK_PACKAGE_MAINTAINER "Haley Clark <hdeanclark@gmail.com>")
SET(CPACK_DEBIAN_PACKAGE_SECTION "Science")

include(CPack)

