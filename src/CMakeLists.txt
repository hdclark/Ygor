

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

# This target intentionally contains no Component 8 producer object. It keeps
# verifier compilation isolated so producer-private helpers cannot be resolved.
add_library(ygor_mesh_boolean_arrangement_verifier_isolation OBJECT
    YgorMeshesBooleanGlobalArrangementVerifier.cc)
target_include_directories(ygor_mesh_boolean_arrangement_verifier_isolation
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(ygor_mesh_boolean_arrangement_verifier_isolation PRIVATE cxx_std_17)

# This checker decodes and substitutes accepted bits without Component 11
# producer helpers.
add_library(ygor_mesh_boolean_realization_verifier_isolation OBJECT
    YgorMeshesBooleanRealizationVerifier.cc)
target_include_directories(ygor_mesh_boolean_realization_verifier_isolation
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(ygor_mesh_boolean_realization_verifier_isolation PRIVATE cxx_std_17)

# The durable-result reader starts from canonical bytes. Build it as a linked
# library so unresolved references cannot be hidden by an OBJECT-only check.
add_library(ygor_mesh_boolean_exact_result_verifier_support STATIC
    YgorMeshesBooleanContract.cc
    YgorMeshesBooleanProductContract.cc
    YgorMeshesExactArithmetic.cc
    YgorMeshesBooleanPerformance.cc
    External/MD5/md5.cc)
target_include_directories(ygor_mesh_boolean_exact_result_verifier_support
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(ygor_mesh_boolean_exact_result_verifier_support PRIVATE cxx_std_17)
set_target_properties(ygor_mesh_boolean_exact_result_verifier_support
    PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(ygor_mesh_boolean_exact_result_verifier_isolation SHARED
    YgorMeshesBooleanExactResult.cc)
target_include_directories(ygor_mesh_boolean_exact_result_verifier_isolation
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(ygor_mesh_boolean_exact_result_verifier_isolation PRIVATE cxx_std_17)
target_link_libraries(ygor_mesh_boolean_exact_result_verifier_isolation PRIVATE
    ygor_mesh_boolean_exact_result_verifier_support
    Threads::Threads
    m
    "${STD_FS_LIB}")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(ygor_mesh_boolean_exact_result_verifier_isolation PRIVATE "LINKER:--no-undefined")
endif()

# Predicate-bearing Boolean sources must override repository-wide fast-math flags.
set(ygor_boolean_strict_arithmetic_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanContract.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanProductContract.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesExactArithmetic.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesExactKernel.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesExactKernelFilters.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanVerification.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanInputTopology.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanPreparation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanBroadPhase.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanIntersectionEvents.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanSymbolicRegistry.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanLocalRefinement.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanGlobalArrangement.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanGlobalArrangementVerifier.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanCellClassification.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanSelection.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanRealization.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanRealizationVerifier.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/YgorMeshesBooleanOutput.cc
)
if(YGOR_BOOLEAN_STRICT_FP AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties(${ygor_boolean_strict_arithmetic_sources} PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-frounding-math;-ffp-contract=off")
    # CMAKE_CXX_FLAGS also reaches the compiler-driver link invocation.  A final
    # -ffast-math there links GCC's crtfastmath startup object, enabling FTZ/DAZ
    # process-wide even though the Boolean translation units compile strictly.
    set_property(TARGET ygor APPEND_STRING PROPERTY LINK_FLAGS " -fno-fast-math")
elseif(YGOR_BOOLEAN_STRICT_FP AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-ffp-model=strict" YGOR_HAS_STRICT_FP_MODEL)
    if(YGOR_HAS_STRICT_FP_MODEL)
        set_source_files_properties(${ygor_boolean_strict_arithmetic_sources} PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-model=strict;-ffp-contract=off")
    else()
        set_source_files_properties(${ygor_boolean_strict_arithmetic_sources} PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    endif()
endif()
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
