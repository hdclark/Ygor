
cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)
project(Ygor
    VERSION 0.1.0
    DESCRIPTION "Support library with scientific emphasis"
    HOMEPAGE_URL "https://github.com/hdclark/Ygor"
    LANGUAGES CXX
)

####################################################################################
#                                  User Options
####################################################################################

option(WITH_IWYU                "Compile using clang include-what-you-use."     OFF)

option(WITH_ASAN                "Compile using ASan, LSan, & UBSan."            OFF)
option(WITH_TSAN                "Compile using ThreadSanitizer."                OFF)
option(WITH_MSAN                "Compile using MemorySanitizer."                OFF)

option(WITH_LINUX_SYS "Compile assuming Linux-specific sys interfaces are available." ON)
option(WITH_EIGEN     "Compile assuming Eigen is available." ON)
option(WITH_GNU_GSL   "Compile assuming the GNU GSL is available." ON)
option(WITH_BOOST     "Compile assuming Boost is available (note: won't impact header-only parts)." ON)

option(USE_LTO "Use link-time optimization when available." OFF)

option(BUILD_SHARED_LIBS "Build shared-object/dynamically-loaded binaries." ON)

option(FETCHCONTENT_FALLBACK "Use FetchContent to download missing dependencies." ON)

####################################################################################
#                                  Configuration
####################################################################################

# Modern CMake settings
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as no build type was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(USE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_avail OUTPUT lto_msg)
    if(lto_avail)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "LTO was requested, but is not supported: ${lto_msg}")
    endif()
endif()


####################################################################################
#                                  Dependencies 
####################################################################################
# Note: Dependencies are listed in CPACK list below.

include(FetchContent)

find_package(Threads REQUIRED)

# Boost (optional)
if(WITH_BOOST)
    if(NOT BUILD_SHARED_LIBS)
        set(Boost_USE_STATIC_RUNTIME ON)
        set(Boost_USE_STATIC_LIBS ON)
    endif()
    find_package(Boost QUIET)
    if(NOT Boost_FOUND)
        message(STATUS "Boost not found. Disabling WITH_BOOST.")
        set(WITH_BOOST OFF CACHE BOOL "" FORCE)
    endif()
endif()

# Eigen (optional, with FetchContent fallback)
if(WITH_EIGEN)
    find_package(Eigen3 3.3 QUIET CONFIG)
    if(NOT Eigen3_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(EIGEN3 QUIET eigen3)
        endif()
    endif()

    if(NOT Eigen3_FOUND AND NOT EIGEN3_FOUND)
        if(FETCHCONTENT_FALLBACK)
            message(STATUS "Eigen3 not found locally, attempting FetchContent...")
            # Use URL method which can be more reliable than git
            FetchContent_Declare(
                Eigen
                URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
                URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
            set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
            set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

            FetchContent_GetProperties(Eigen)
            if(NOT eigen_POPULATED)
                message(STATUS "Downloading Eigen3 (this may take a moment)...")
                # Attempt to populate - will fail if network unavailable
                FetchContent_MakeAvailable(Eigen)
                if(TARGET Eigen3::Eigen)
                    set(EIGEN_FETCHED TRUE)
                    message(STATUS "Eigen3 fetched successfully via FetchContent.")
                else()
                    message(WARNING "FetchContent for Eigen3 failed. Disabling WITH_EIGEN.")
                    set(WITH_EIGEN OFF CACHE BOOL "" FORCE)
                endif()
            endif()
        else()
            message(STATUS "Eigen not found and FETCHCONTENT_FALLBACK=OFF. Disabling WITH_EIGEN.")
            set(WITH_EIGEN OFF CACHE BOOL "" FORCE)
        endif()
    endif()
endif()

# GNU GSL (optional)
if(WITH_GNU_GSL)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GNU_GSL QUIET gsl)
    endif()
    if(NOT GNU_GSL_FOUND)
        # Try to find GSL using CMake's FindGSL module
        find_package(GSL QUIET)
        if(GSL_FOUND)
            set(GNU_GSL_FOUND TRUE)
            set(GNU_GSL_LIBRARIES ${GSL_LIBRARIES})
            set(GNU_GSL_INCLUDE_DIRS ${GSL_INCLUDE_DIRS})
        endif()
    endif()
    if(NOT GNU_GSL_FOUND)
        message(STATUS "GNU GSL not found. Disabling WITH_GNU_GSL.")
        set(WITH_GNU_GSL OFF CACHE BOOL "" FORCE)
    endif()
endif()


####################################################################################
#                              Compiler Flags Interface
####################################################################################
# Create an INTERFACE library to encapsulate common compile options

add_library(ygor_compile_options INTERFACE)
add_library(Ygor::CompileOptions ALIAS ygor_compile_options)

# C++ standard
target_compile_features(ygor_compile_options INTERFACE cxx_std_17)

# Position independent code
set_target_properties(ygor_compile_options PROPERTIES
    INTERFACE_POSITION_INDEPENDENT_CODE ON
)

# Static linking setup
if(NOT BUILD_SHARED_LIBS)
    target_link_options(ygor_compile_options INTERFACE "-static")
endif()

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ygor_compile_options INTERFACE
        -ffast-math
        -frounding-math
        -fno-finite-math-only
        -fdiagnostics-color=always
        -fno-var-tracking-assignments
        -Wno-deprecated
        -Wall
        -Wextra
        -Wpedantic
        -Wno-strict-aliasing
    )

    target_compile_options(ygor_compile_options INTERFACE
        $<$<CONFIG:Debug>:-pg>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        $<$<CONFIG:Debug>:-fstack-check>
    )

    if(MEMORY_CONSTRAINED_BUILD)
        target_compile_options(ygor_compile_options INTERFACE
            --param=ggc-min-expand=10
            --param=ggc-min-heapsize=32768
        )
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ygor_compile_options INTERFACE
        -fdiagnostics-color=always
        -Wno-deprecated
        -Wall
        -Wextra
        -Wpedantic
    )

    if(WITH_IWYU)
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "iwyu;-Xiwyu;--no_comments")
    endif()
endif()

# Memory constrained build options
if(MEMORY_CONSTRAINED_BUILD)
    # For memory-constrained builds, minimize optimization but keep Debug assertions enabled
    target_compile_options(ygor_compile_options INTERFACE
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O0 -DNDEBUG>
        $<$<CONFIG:MinSizeRel>:-O0 -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O0 -g>
    )
else()
    target_compile_options(ygor_compile_options INTERFACE
        $<$<CONFIG:Debug>:-O2 -g>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O3 -g>
    )
endif()


####################################################################################
#                                  Sanitizers
####################################################################################

add_library(ygor_sanitizer_options INTERFACE)
add_library(Ygor::SanitizerOptions ALIAS ygor_sanitizer_options)

if(WITH_ASAN OR WITH_TSAN OR WITH_MSAN)
    target_compile_options(ygor_sanitizer_options INTERFACE
        -O2
        -g
        -fno-omit-frame-pointer
        -fno-common
        --coverage
    )
    target_compile_definitions(ygor_sanitizer_options INTERFACE -U_FORTIFY_SOURCE)
endif()

if(WITH_ASAN)
    target_compile_options(ygor_sanitizer_options INTERFACE
        -fsanitize=address
        -fsanitize-address-use-after-scope
        -fsanitize=undefined
        -fno-sanitize-recover=undefined
    )
    target_link_options(ygor_sanitizer_options INTERFACE
        -fsanitize=address
        -fsanitize=undefined
    )
elseif(WITH_TSAN)
    message(WARNING "TSan may not support exceptions (depends on the compiler version and platform).")
    target_compile_options(ygor_sanitizer_options INTERFACE -fsanitize=thread)
    target_link_options(ygor_sanitizer_options INTERFACE -fsanitize=thread)
elseif(WITH_MSAN)
    message(WARNING "MSan may not be available on your system.")
    target_compile_options(ygor_sanitizer_options INTERFACE
        -fsanitize=memory
        -fPIE
        -pie
        -fsanitize-memory-track-origins
    )
    target_link_options(ygor_sanitizer_options INTERFACE -fsanitize=memory)
endif()


####################################################################################
#                              ARM Architecture Support
####################################################################################

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(__arm__     "cstdio" ARCH_IS_ARM)
check_cxx_symbol_exists(__aarch64__ "cstdio" ARCH_IS_ARM64)

if(ARCH_IS_ARM OR ARCH_IS_ARM64)
    message(STATUS "Detected ARM architecture.")
    # Only add flags if not already set by user
    get_target_property(_current_opts ygor_compile_options INTERFACE_COMPILE_OPTIONS)
    if(NOT _current_opts MATCHES "-march=|-mcpu=|-mtune=")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            message(STATUS "No architecture set, adding march=native flag")
            target_compile_options(ygor_compile_options INTERFACE -march=native)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(STATUS "No architecture set, adding mcpu=generic flag")
            target_compile_options(ygor_compile_options INTERFACE -mcpu=generic)
        else()
            message(WARNING "Not able to set architecture, if compilation errors occur set architecture manually")
        endif()
    endif()
endif()


####################################################################################
#                            std::filesystem workaround
####################################################################################
# Workaround for GCCv8 std::filesystem sidecar library -- can be removed when GCCv8 no longer relevant.

set(YGOR_STD_FS_LIB "")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    foreach(lib_dir ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
        file(GLOB_RECURSE _found_fs_lib "${lib_dir}*stdc++fs*")
        if(_found_fs_lib)
            set(YGOR_STD_FS_LIB "stdc++fs")
            message(STATUS "Assuming libstdc++fs library is needed.")
            break()
        endif()
    endforeach()
endif()


####################################################################################
#                                    Definitions
####################################################################################

# Log which features are enabled
if(WITH_LINUX_SYS)
    message(STATUS "Linux-specific sys interfaces: ENABLED")
else()
    message(STATUS "Linux-specific sys interfaces: DISABLED")
endif()

if(WITH_EIGEN)
    message(STATUS "Eigen support: ENABLED")
else()
    message(STATUS "Eigen support: DISABLED")
endif()

if(WITH_GNU_GSL)
    message(STATUS "GNU GSL support: ENABLED")
else()
    message(STATUS "GNU GSL support: DISABLED")
endif()

if(WITH_BOOST)
    message(STATUS "Boost support: ENABLED")
else()
    message(STATUS "Boost support: DISABLED")
endif()


####################################################################################
#                                Output Directories
####################################################################################

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
#                            CMake Package Configuration
####################################################################################

include(CMakePackageConfigHelpers)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/YgorConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Export targets to build tree
export(TARGETS ygor ygor_compile_options ygor_sanitizer_options
    NAMESPACE Ygor::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/YgorTargets.cmake"
)

# Install targets
install(TARGETS ygor ygor_compile_options ygor_sanitizer_options
    EXPORT YgorTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install export file
install(EXPORT YgorTargets
    FILE YgorTargets.cmake
    NAMESPACE Ygor::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ygor
)

# Generate and install config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/YgorConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/YgorConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ygor
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/YgorConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/YgorConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ygor
)


####################################################################################
#                                    Packaging    
####################################################################################

set(CPACK_GENERATOR "DEB")

string(TIMESTAMP INVOCATION_TIMESTAMP "%Y%m%d.%H%M%S")
set(CPACK_PACKAGE_VERSION "${INVOCATION_TIMESTAMP}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "hdeanclark@gmail.com")
set(CPACK_PACKAGE_MAINTAINER "Haley Clark <hdeanclark@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "Science")

# Ygor build dependencies
set(BUILD_DEPENDENCY_PACKAGES "")
if(WITH_BOOST)
    list(APPEND BUILD_DEPENDENCY_PACKAGES "libboost-dev")
endif()
if(WITH_GNU_GSL)
    list(APPEND BUILD_DEPENDENCY_PACKAGES "libgsl-dev")
endif()
list(JOIN BUILD_DEPENDENCY_PACKAGES ", " CPACK_DEBIAN_PACKAGE_DEPENDS)

# Recommended packages
set(RECOMMENDED_PACKAGES "")
if(WITH_EIGEN)
    list(APPEND RECOMMENDED_PACKAGES "libeigen3-dev")
endif()
list(APPEND RECOMMENDED_PACKAGES "gnuplot")
list(APPEND RECOMMENDED_PACKAGES "plotutils")
list(JOIN RECOMMENDED_PACKAGES ", " CPACK_DEBIAN_PACKAGE_RECOMMENDS)

include(CPack)

