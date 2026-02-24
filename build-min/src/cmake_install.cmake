# Install script for directory: /home/runner/work/Ygor/Ygor/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
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
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libygor.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libygor.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libygor.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/runner/work/Ygor/Ygor/build-min/lib/libygor.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libygor.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libygor.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libygor.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/home/runner/work/Ygor/Ygor/build-min/src/YgorDefinitions.h"
    "/home/runner/work/Ygor/Ygor/src/YgorAlgorithms.h"
    "/home/runner/work/Ygor/Ygor/src/YgorArguments.h"
    "/home/runner/work/Ygor/Ygor/src/YgorBase64.h"
    "/home/runner/work/Ygor/Ygor/src/YgorCONFIGTools.h"
    "/home/runner/work/Ygor/Ygor/src/YgorContainers.h"
    "/home/runner/work/Ygor/Ygor/src/YgorDICOMTools.h"
    "/home/runner/work/Ygor/Ygor/src/YgorEnvironment.h"
    "/home/runner/work/Ygor/Ygor/src/YgorFilesDirs.h"
    "/home/runner/work/Ygor/Ygor/src/YgorIO.h"
    "/home/runner/work/Ygor/Ygor/src/YgorImages.h"
    "/home/runner/work/Ygor/Ygor/src/YgorImagesIO.h"
    "/home/runner/work/Ygor/Ygor/src/YgorImagesIOBoostSerialization.h"
    "/home/runner/work/Ygor/Ygor/src/YgorImagesPlotting.h"
    "/home/runner/work/Ygor/Ygor/src/YgorLog.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMath.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathBSpline.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathChebyshev.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathChebyshevFunctions.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathChebyshevIOBoostSerialization.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathIOBoostSerialization.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathIOOBJ.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathIOOFF.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathIOPLY.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathIOSTL.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathIOSVG.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathIOXYZ.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathPlotting.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathPlottingGnuplot.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMathPlottingVTK.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMath_Samples.h"
    "/home/runner/work/Ygor/Ygor/src/YgorMisc.h"
    "/home/runner/work/Ygor/Ygor/src/YgorNetworking.h"
    "/home/runner/work/Ygor/Ygor/src/YgorNoise.h"
    "/home/runner/work/Ygor/Ygor/src/YgorPerformance.h"
    "/home/runner/work/Ygor/Ygor/src/YgorPlot.h"
    "/home/runner/work/Ygor/Ygor/src/YgorSerialize.h"
    "/home/runner/work/Ygor/Ygor/src/YgorStats.h"
    "/home/runner/work/Ygor/Ygor/src/YgorString.h"
    "/home/runner/work/Ygor/Ygor/src/YgorTAR.h"
    "/home/runner/work/Ygor/Ygor/src/YgorTime.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/runner/work/Ygor/Ygor/build-min/src/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
