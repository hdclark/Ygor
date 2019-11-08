
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(Ygor LANGUAGES CXX)
#set(Ygor_VERSION_MAJOR 0)
#set(Ygor_VERSION_MINOR 0)
#set(Ygor_VERSION_PATCH 0)

####################################################################################
#                                  User Options
####################################################################################

option(WITH_LINUX_SYS "Compile assuming Linux-specific sys interfaces are available." ON)
option(WITH_EIGEN     "Compile assuming Eigen is available." ON)
option(WITH_GNU_GSL   "Compile assuming the GNU GSL is available." ON)
option(WITH_BOOST     "Compile assuming Boost is available (note: won't impact header-only parts)." ON)


####################################################################################
#                                  Dependencies
####################################################################################
# Note: dependencies are listed in CPACK list below.

find_package(Threads REQUIRED)

# GNU GSL.

# Header-only: Eigen.
# Header-only: Boost.Serialization.
# Header-only: VTK.

# Optional, at runtime: Gnuplot.


####################################################################################
#                                  Configuration
####################################################################################

# Set the release type. 
set(CMAKE_BUILD_TYPE Release) # Comment this line to use fallback default.
if(NOT CMAKE_BUILD_TYPE)
    # Default to debug builds.
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "default to debug" FORCE)
endif()

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(BUILD_SHARED_LIBS TRUE)
set(POSITION_INDEPENDENT_CODE TRUE)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable GNU extensions (e.g., std=gnu++14).
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -frounding-math -fno-finite-math-only")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always -fno-var-tracking-assignments")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wall -Wextra -Wno-strict-aliasing")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -frounding-math -fno-finite-math-only")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wall -Wextra -Wno-strict-aliasing")
endif()

if(WITH_LINUX_SYS)
    message(STATUS "Assuming Linux-specific sys interfaces are available.")
    add_definitions(-DYGOR_USE_LINUX_SYS=1)
endif()
if(WITH_EIGEN)
    message(STATUS "Assuming Eigen is available.")
    add_definitions(-DYGOR_USE_EIGEN=1)
endif()
if(WITH_GNU_GSL)
    message(STATUS "Assuming GNU GSL is available.")
    add_definitions(-DYGOR_USE_GNU_GSL=1)
endif()
if(WITH_BOOST)
    message(STATUS "Assuming Boost libraries are available.")
    add_definitions(-DYGOR_USE_BOOST=1)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy et al.

# Use the directory where CMakeLists.txt is for inclusions.
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


####################################################################################
#                                 Subdirectories
####################################################################################

add_subdirectory(src)
add_subdirectory(binaries)


####################################################################################
#                                    Packaging    
####################################################################################

set(CPACK_GENERATOR "DEB")
#SET(CPACK_PACKAGE_NAME "ygor")

string(TIMESTAMP INVOCATION_TIMESTAMP "%Y%m%d.%H%M%S") # For a time-based version number.
set(CPACK_PACKAGE_VERSION "${INVOCATION_TIMESTAMP}")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64") # i386, amd64, armel, armhf, ...
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "D.R.Y. support library with scientific emphasis.")
set(CPACK_PACKAGE_CONTACT "hdeanclark@gmail.com")
set(CPACK_PACKAGE_MAINTAINER "Haley Clark <hdeanclark@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "Science")

# Ygor build dependencies, e.g., "libc6 (>= 2.3.1-6), libgcc1 (>= 1:3.4.2-12)"
set(BUILD_DEPENDENCY_PACKAGES "")
if(WITH_BOOST)
    list(APPEND BUILD_DEPENDENCY_PACKAGES "libboost-dev")
endif()
if(WITH_GNU_GSL)
    list(APPEND BUILD_DEPENDENCY_PACKAGES "libgsl-dev")
endif()
list(JOIN BUILD_DEPENDENCY_PACKAGES ", " CPACK_DEBIAN_PACKAGE_DEPENDS)

# Recommended or optional packages, e.g., "liboptional-dev (>= 1.2.3-1), libmaybe-dev (>= 1:1.3.2-10)"
set(RECOMMENDED_PACKAGES "")
if(WITH_EIGEN)
    list(APPEND RECOMMENDED_PACKAGES "libeigen3-dev")
endif()
list(APPEND RECOMMENDED_PACKAGES "gnuplot")
list(JOIN RECOMMENDED_PACKAGES ", " CPACK_DEBIAN_PACKAGE_RECOMMENDS)

include(CPack)

