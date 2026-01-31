
# Set configuration variables for YgorDefinitions.h
# These must be set before configure_file is called
if(WITH_LINUX_SYS)
    set(YGOR_USE_LINUX_SYS 1)
endif()
if(WITH_EIGEN)
    set(YGOR_USE_EIGEN 1)
endif()
if(WITH_GNU_GSL)
    set(YGOR_USE_GNU_GSL 1)
endif()
if(WITH_BOOST)
    set(YGOR_USE_BOOST 1)
endif()

# Generate configuration header
configure_file(YgorDefinitions.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h
)

# Source files - explicitly listed for better dependency tracking
set(YGOR_SOURCES
    YgorAlgorithms.cc
    YgorBase64.cc
    YgorCONFIGTools.cc
    YgorContainers.cc
    YgorDICOMTools.cc
    YgorEnvironment.cc
    YgorFilesDirs.cc
    YgorImages.cc
    YgorImagesIO.cc
    YgorLog.cc
    YgorMath.cc
    YgorMathBSpline.cc
    YgorMathChebyshev.cc
    YgorMathChebyshevFunctions.cc
    YgorMathIOOBJ.cc
    YgorMathIOOFF.cc
    YgorMathIOPLY.cc
    YgorMathIOSTL.cc
    YgorMathIOSVG.cc
    YgorMathIOXYZ.cc
    YgorMath_Samples.cc
    YgorNetworking.cc
    YgorNoise.cc
    YgorPerformance.cc
    YgorPlot.cc
    YgorSerialize.cc
    YgorStats.cc
    YgorString.cc
    YgorTAR.cc
    YgorTime.cc
    External/SpookyHash/SpookyV2.cpp
    External/MD5/md5.cc
)

# Header files for installation
set(YGOR_HEADERS
    YgorAlgorithms.h
    YgorArguments.h
    YgorBase64.h
    YgorCONFIGTools.h
    YgorContainers.h
    YgorDICOMTools.h
    YgorEnvironment.h
    YgorFilesDirs.h
    YgorIO.h
    YgorImages.h
    YgorImagesIO.h
    YgorImagesIOBoostSerialization.h
    YgorImagesPlotting.h
    YgorLog.h
    YgorMath.h
    YgorMathBSpline.h
    YgorMathChebyshev.h
    YgorMathChebyshevFunctions.h
    YgorMathChebyshevIOBoostSerialization.h
    YgorMathIOBoostSerialization.h
    YgorMathIOOBJ.h
    YgorMathIOOFF.h
    YgorMathIOPLY.h
    YgorMathIOSTL.h
    YgorMathIOSVG.h
    YgorMathIOXYZ.h
    YgorMathPlotting.h
    YgorMathPlottingGnuplot.h
    YgorMathPlottingVTK.h
    YgorMath_Samples.h
    YgorMisc.h
    YgorNetworking.h
    YgorNoise.h
    YgorPerformance.h
    YgorPlot.h
    YgorSerialize.h
    YgorStats.h
    YgorString.h
    YgorTAR.h
    YgorTime.h
    ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h
)

# Create the library
add_library(ygor ${YGOR_SOURCES})
add_library(Ygor::ygor ALIAS ygor)

# Set library properties
set_target_properties(ygor PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories with proper visibility
target_include_directories(ygor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link compile options
target_link_libraries(ygor
    PUBLIC
        Ygor::CompileOptions
    PRIVATE
        $<$<OR:$<BOOL:${WITH_ASAN}>,$<BOOL:${WITH_TSAN}>,$<BOOL:${WITH_MSAN}>>:Ygor::SanitizerOptions>
)

# Core dependencies
target_link_libraries(ygor
    PUBLIC
        Threads::Threads
    PRIVATE
        m
        $<$<BOOL:${YGOR_STD_FS_LIB}>:${YGOR_STD_FS_LIB}>
)

# Optional Boost dependency
if(WITH_BOOST)
    target_compile_definitions(ygor PUBLIC YGOR_USE_BOOST=1)
    if(TARGET Boost::headers)
        target_link_libraries(ygor PUBLIC Boost::headers)
    elseif(Boost_INCLUDE_DIRS)
        target_include_directories(ygor SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
    endif()
endif()

# Optional Eigen dependency
if(WITH_EIGEN)
    target_compile_definitions(ygor PUBLIC YGOR_USE_EIGEN=1)
    if(TARGET Eigen3::Eigen)
        target_link_libraries(ygor PUBLIC Eigen3::Eigen)
    elseif(EIGEN3_INCLUDE_DIRS)
        target_include_directories(ygor SYSTEM PUBLIC ${EIGEN3_INCLUDE_DIRS})
    endif()
endif()

# Optional GNU GSL dependency
if(WITH_GNU_GSL)
    target_compile_definitions(ygor PUBLIC YGOR_USE_GNU_GSL=1)
    if(GNU_GSL_LIBRARIES)
        target_link_libraries(ygor PRIVATE ${GNU_GSL_LIBRARIES})
    endif()
    if(GNU_GSL_INCLUDE_DIRS)
        target_include_directories(ygor SYSTEM PRIVATE ${GNU_GSL_INCLUDE_DIRS})
    endif()
endif()

# Linux-specific sys interfaces
if(WITH_LINUX_SYS)
    target_compile_definitions(ygor PUBLIC YGOR_USE_LINUX_SYS=1)
endif()

# Install headers
install(FILES ${YGOR_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

