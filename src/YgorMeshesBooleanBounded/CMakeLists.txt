include(CheckCXXCompilerFlag)

function(ygor_apply_mesh_boolean_strict_fp target)
    target_compile_definitions(${target} PRIVATE YGOR_MESH_BOOLEAN_STRICT_FP_BUILD=1)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -fno-fast-math -frounding-math -fno-associative-math
            -fno-reciprocal-math -fsigned-zeros -fno-finite-math-only
            -ffp-contract=off)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -fno-fast-math -frounding-math -fsigned-zeros -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /fp:strict)
    else()
        message(FATAL_ERROR "No qualified strict floating-point profile for ${CMAKE_CXX_COMPILER_ID}")
    endif()
    set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
endfunction()

add_library(ygor_mesh_boolean_bounded_objects OBJECT
    Cancellation.cc
    Resources.cc
    Sha256.cc
    YgorMeshesBooleanBounded.cc)
set_property(TARGET ygor_mesh_boolean_bounded_objects PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(ygor_mesh_boolean_bounded_objects PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_bounded_objects)
target_sources(ygor PRIVATE $<TARGET_OBJECTS:ygor_mesh_boolean_bounded_objects>)

if(BUILD_TESTING)
    add_subdirectory("${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded" "${CMAKE_BINARY_DIR}/tests/mesh_boolean_bounded")
endif()
