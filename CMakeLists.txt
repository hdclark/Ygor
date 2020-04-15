
cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
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
# Note: Dependencies are listed in CPACK list below.

find_package(Threads REQUIRED)

if(WITH_EIGEN)
    #find_package(Eigen3 3.1.0)
    #include(${EIGEN3_USE_FILE})
    #
    # or via pkg-config method:
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(EIGEN3 REQUIRED eigen3)
    include_directories( ${EIGEN3_INCLUDE_DIRS} )
    #
    # Fallback if the above methods fail:
    #include_directories("/usr/include/eigen3/")
endif()

# GNU GSL.
# Boost.

# Header-only: Boost.Serialization.
# Header-only: VTK.

# Optional, at runtime: Gnuplot.


####################################################################################
#                                  Configuration
####################################################################################

# Override the default CXX flags, which are controlled by the release type.
#
# Note: The '_DEBUG' flags are only applied when the release mode is 'Debug' -- likewise for the other flags.
#
# Note: If you want to fully override the CXX_FLAGS, then do not supply a build type and specify your CXX_FLAGS by
#       defining CMAKE_CXX_FLAGS when calling cmake.
set(CMAKE_CXX_FLAGS_DEBUG           "-O2 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL      "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE         "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O3 -g")

if(MEMORY_CONSTRAINED_BUILD)
    # Do not overwrite user-provided flags, but do provide sane defaults.
    if(NOT CMAKE_CXX_FLAGS)
        set(CMAKE_CXX_FLAGS "-O0 -DNDEBUG")
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG           "-O0 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL      "-O0 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE         "-O0 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O0 -DNDEBUG")
endif()


# Add other appropriate CXX flags.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frounding-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-finite-math-only")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-var-tracking-assignments")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        # Add gprof profiling flag.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
    endif()

    if(MEMORY_CONSTRAINED_BUILD)
        # Trigger garbage collection more frequently.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --param ggc-min-expand=10")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --param ggc-min-heapsize=32768")
    endif()

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
endif()


# Enable to fix linking errors for toolchains that do not auto-detect atomic intrinsics (e.g., some ARM systems).
# Note: Binaries built this way should not be distributed.
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")


# Set the release type. 
if(NOT CMAKE_BUILD_TYPE)
    # Default to debug builds.
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "default to debug" FORCE)
endif()


####################################################################################
#                                    Definitions
####################################################################################
#add_definitions(-U__STRICT_ANSI__)  # Used to compile on MSYS2.
                                     # Alternatively define _GNU_SOURCE or _XOPEN_SOURCE
                                     # or enable GNU extensions via CMake mechanism?

if(WITH_LINUX_SYS)
    message(STATUS "Assuming Linux-specific sys interfaces are available.")
    set(YGOR_USE_LINUX_SYS 1)
endif()
if(WITH_EIGEN)
    message(STATUS "Assuming Eigen is available.")
    set(YGOR_USE_EIGEN 1)
endif()
if(WITH_GNU_GSL)
    message(STATUS "Assuming GNU GSL is available.")
    set(YGOR_USE_GNU_GSL 1)
endif()
if(WITH_BOOST)
    message(STATUS "Assuming Boost libraries are available.")
    set(YGOR_USE_BOOST 1)
endif()


# Add other compiler options.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(BUILD_SHARED_LIBS TRUE)
set(POSITION_INDEPENDENT_CODE TRUE)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable GNU extensions (e.g., std=gnu++14).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For use with clang-tidy et al.


# Use the directory where CMakeLists.txt is for inclusions.
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})


####################################################################################
#                                 Subdirectories
####################################################################################

add_subdirectory(src)
add_subdirectory(binaries)


####################################################################################
#                                    Packaging    
####################################################################################

set(CPACK_GENERATOR "DEB")
#set(CPACK_PACKAGE_NAME "ygor")

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

