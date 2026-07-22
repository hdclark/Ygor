add_executable(ygor_mesh_boolean_bounded_tests TestMain.cc)
target_include_directories(ygor_mesh_boolean_bounded_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(ygor_mesh_boolean_bounded_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_bounded_tests)
add_test(NAME mesh_boolean_bounded_component01 COMMAND ygor_mesh_boolean_bounded_tests)

add_executable(ygor_mesh_boolean_input_validation_tests
    TestInputValidation.cc
    qualification/ExactUnsignedInteger.cc
    qualification/ExactInteger.cc
    qualification/ExactRational.cc
    qualification/ExactGeometryOracle.cc)
target_include_directories(ygor_mesh_boolean_input_validation_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(ygor_mesh_boolean_input_validation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_input_validation_tests)
add_test(NAME mesh_boolean_bounded_component02_source_contract COMMAND ygor_mesh_boolean_input_validation_tests)
add_test(NAME mesh_boolean_bounded_component02_topology COMMAND ygor_mesh_boolean_input_validation_tests)
add_test(NAME mesh_boolean_bounded_component02_canonicalization COMMAND ygor_mesh_boolean_input_validation_tests)
add_test(NAME mesh_boolean_bounded_component02_permutation COMMAND ygor_mesh_boolean_input_validation_tests)
add_test(NAME mesh_boolean_bounded_component02_mutation COMMAND ygor_mesh_boolean_input_validation_tests)
add_test(NAME mesh_boolean_bounded_component02_shell_semantics COMMAND ygor_mesh_boolean_input_validation_tests)
add_test(NAME mesh_boolean_bounded_component02_public_gate COMMAND ygor_mesh_boolean_input_validation_tests)

add_executable(ygor_mesh_boolean_precision_tests
    TestPrecision.cc
    qualification/ExactUnsignedInteger.cc
    qualification/ExactInteger.cc
    qualification/ExactRational.cc
    qualification/ExactFloatImport.cc
    qualification/ExactGeometryOracle.cc)
target_include_directories(ygor_mesh_boolean_precision_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_precision_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_precision_tests)
add_test(NAME mesh_boolean_bounded_component03 COMMAND ygor_mesh_boolean_precision_tests)

add_executable(ygor_mesh_boolean_precision_numeric_tests TestPrecisionNumericHardening.cc)
add_executable(ygor_mesh_boolean_precision_operations_tests TestPrecisionOperationsHardening.cc)
add_executable(ygor_mesh_boolean_precision_state_tests TestPrecisionStateHardening.cc)

foreach(hardening_test numeric operations state)
    target_include_directories(ygor_mesh_boolean_precision_${hardening_test}_tests PRIVATE
        "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(ygor_mesh_boolean_precision_${hardening_test}_tests PRIVATE
        ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
    ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_precision_${hardening_test}_tests)
    add_test(NAME mesh_boolean_bounded_component03_${hardening_test}
        COMMAND ygor_mesh_boolean_precision_${hardening_test}_tests)
endforeach()

# These bootstrap cases instantiate Ygor's out-of-line public mesh/vector types.
target_link_libraries(ygor_mesh_boolean_precision_numeric_tests PRIVATE ygor)

add_executable(ygor_mesh_boolean_adaptive_compatibility_tests
    TestAdaptivePredicateCompatibility.cc)
target_include_directories(ygor_mesh_boolean_adaptive_compatibility_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(ygor_mesh_boolean_adaptive_compatibility_tests PRIVATE
    ygor_exact_float_expansion_core Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_adaptive_compatibility_tests)
add_test(NAME mesh_boolean_bounded_component03_adaptive_compatibility
    COMMAND ygor_mesh_boolean_adaptive_compatibility_tests)

add_executable(ygor_mesh_boolean_precision_codec_tests TestPrecisionCodecGaps.cc)
target_include_directories(ygor_mesh_boolean_precision_codec_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(ygor_mesh_boolean_precision_codec_tests PRIVATE ygor_mesh_boolean_bounded_strict)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_precision_codec_tests)
add_test(NAME mesh_boolean_bounded_component03_codecs
    COMMAND ygor_mesh_boolean_precision_codec_tests)

foreach(consumer_component 02 04 06 07 13 15 16 17)
    add_library(ygor_mesh_boolean_component${consumer_component}_capability_compile OBJECT
        PrecisionCapabilityIntegration.cc)
    target_compile_definitions(
        ygor_mesh_boolean_component${consumer_component}_capability_compile
        PRIVATE YGOR_PRECISION_CAPABILITY_CONSUMER=${consumer_component})
    target_include_directories(
        ygor_mesh_boolean_component${consumer_component}_capability_compile
        PRIVATE "${CMAKE_SOURCE_DIR}/src")
    ygor_apply_mesh_boolean_strict_fp(
        ygor_mesh_boolean_component${consumer_component}_capability_compile)
endforeach()
