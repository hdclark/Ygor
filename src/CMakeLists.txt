

configure_file(YgorDefinitions.h.in 
               ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h)

FILE(GLOB ygor_sources "*cc")
FILE(GLOB ygor_headers "*.h"
                       ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h)

add_library(ygor
    ${ygor_sources}
    External/SpookyHash/SpookyV2.cpp
    External/MD5/md5.cc
)
#set_target_properties(ygor PROPERTIES
#    POSITION_INDEPENDENT_CODE TRUE
#)
target_include_directories(ygor
    SYSTEM PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(ygor
    PUBLIC
        Threads::Threads
    PRIVATE
        m
        "${STD_FS_LIB}"
)

set(ygor_robust_fp_sources
    YgorMeshesAdaptivePredicates.cc
    YgorMath.cc
    YgorMathDelaunay.cc
    YgorMathConstrainedDelaunay.cc
    YgorMeshesConvexHull.cc
)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_source_files_properties(${ygor_robust_fp_sources}
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-fexcess-precision=standard"
    )
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set_source_files_properties(${ygor_robust_fp_sources}
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off"
    )
endif()
if(WITH_GNU_GSL)
    target_link_libraries(ygor
        PUBLIC
            ${GNU_GSL_LIBRARIES}
    )
endif()

# Installation info.
install(TARGETS ygor
        EXPORT YgorTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${ygor_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
