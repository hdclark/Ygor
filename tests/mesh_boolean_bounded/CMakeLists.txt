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

foreach(consumer_component 02 04 05 06 07 13 15 16 17)
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

add_executable(ygor_mesh_boolean_source_triangulation_tests
    TestSourceTriangulationMain.cc
    TestSourceTriangulationUnit.cc
    TestSourceTriangulationGeometryBasis.cc
    TestSourceTriangulationProjection.cc
    TestSourceTriangulationCoverage.cc
    TestSourceTriangulationDependencies.cc
    TestSourceTriangulationBoundarySharing.cc
    TestSourceTriangulationCanonicalization.cc
    TestSourceTriangulationAlternatives.cc
    TestSourceTriangulationMutation.cc
    TestSourceTriangulationProperties.cc
    TestSourceTriangulationAdversarial.cc
    SourceTriangulationFixtures.cc
    SourceTriangulationExactOracle.cc)
target_include_directories(ygor_mesh_boolean_source_triangulation_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_source_triangulation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_source_triangulation_tests)
foreach(source_triangulation_suite
        unit geometry_basis projection coverage dependencies boundary canonical
        alternatives mutation properties adversarial)
    add_test(NAME mesh_boolean_bounded_component04_${source_triangulation_suite}
        COMMAND ygor_mesh_boolean_source_triangulation_tests
                ${source_triangulation_suite})
endforeach()

add_executable(ygor_mesh_boolean_canonical_halfedge_tests
    TestCanonicalHalfedge.cc)
target_include_directories(ygor_mesh_boolean_canonical_halfedge_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_canonical_halfedge_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_canonical_halfedge_tests)
foreach(canonical_halfedge_suite
        unit pairing fans groups geometry owner canonical codec mutation resources
        profiles structural)
    add_test(NAME mesh_boolean_bounded_component05_${canonical_halfedge_suite}
        COMMAND ygor_mesh_boolean_canonical_halfedge_tests
                ${canonical_halfedge_suite})
endforeach()

add_executable(ygor_mesh_boolean_broad_phase_tests
    TestBroadPhaseMain.cc
    TestBroadPhaseDomain.cc
    TestBroadPhaseRankMorton.cc
    TestBroadPhaseHierarchy.cc
    TestBroadPhaseOverlap.cc
    TestBroadPhaseTraversal.cc
    TestBroadPhaseKnownCandidates.cc
    TestBroadPhaseAllPairsOracle.cc
    TestBroadPhaseProvenance.cc
    TestBroadPhaseCanonicalization.cc
    TestBroadPhaseCodecReplay.cc
    TestBroadPhaseAlternativeTriangulation.cc
    TestBroadPhaseMutation.cc
    TestBroadPhaseProperties.cc
    TestBroadPhaseAdversarial.cc
    TestBroadPhaseResourcesCancellation.cc
    TestBroadPhaseStructuralPerformance.cc
    BroadPhaseFixtures.cc
    BroadPhaseExactOracle.cc)
target_include_directories(ygor_mesh_boolean_broad_phase_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_broad_phase_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_broad_phase_tests)
foreach(broad_phase_suite
        domain rank_morton hierarchy overlap traversal known_candidates all_pairs
        provenance canonicalization codec_replay alternative mutation properties
        adversarial resources structural)
    add_test(NAME mesh_boolean_bounded_component06_${broad_phase_suite}
        COMMAND ygor_mesh_boolean_broad_phase_tests ${broad_phase_suite})
endforeach()

add_executable(ygor_mesh_boolean_relation_foundation_tests
    TestRelationFoundation.cc)
target_include_directories(ygor_mesh_boolean_relation_foundation_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_relation_foundation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_relation_foundation_tests)
foreach(relation_foundation_suite graph owner canonical symbolic)
    add_test(NAME mesh_boolean_bounded_component07_foundation_${relation_foundation_suite}
        COMMAND ygor_mesh_boolean_relation_foundation_tests
                ${relation_foundation_suite})
endforeach()

add_executable(ygor_mesh_boolean_relation_truth_tests
    TestRelationTruthLayers.cc)
target_include_directories(ygor_mesh_boolean_relation_truth_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_relation_truth_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_relation_truth_tests)
add_test(NAME mesh_boolean_bounded_component07_truth_layers
    COMMAND ygor_mesh_boolean_relation_truth_tests)

add_executable(ygor_mesh_boolean_source_facet_region_tests
    TestSourceFacetRegionRelations.cc)
target_include_directories(ygor_mesh_boolean_source_facet_region_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_source_facet_region_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_source_facet_region_tests)
add_test(NAME mesh_boolean_bounded_component07_source_facet_region
    COMMAND ygor_mesh_boolean_source_facet_region_tests)

add_executable(ygor_mesh_boolean_coplanar_overlay_tests
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/TestCoplanarRelationOverlay.cc"
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(ygor_mesh_boolean_coplanar_overlay_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(ygor_mesh_boolean_coplanar_overlay_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_coplanar_overlay_tests)
add_test(NAME mesh_boolean_bounded_component07_coplanar_overlay
    COMMAND ygor_mesh_boolean_coplanar_overlay_tests)

add_executable(ygor_mesh_boolean_intersection_key_tests
    TestIntersectionKeys.cc
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(ygor_mesh_boolean_intersection_key_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_intersection_key_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_intersection_key_tests)
add_test(NAME mesh_boolean_bounded_component08_keys
    COMMAND ygor_mesh_boolean_intersection_key_tests)

add_executable(ygor_mesh_boolean_intersection_interning_tests
    TestEventInterning.cc
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_link_libraries(ygor_mesh_boolean_intersection_interning_tests PRIVATE
    ygor_mesh_boolean_bounded_strict)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_intersection_interning_tests)
add_test(NAME mesh_boolean_bounded_component08_interning
    COMMAND ygor_mesh_boolean_intersection_interning_tests)

add_executable(ygor_mesh_boolean_intersection_preflight_tests
    TestIntersectionPreflight.cc
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_link_libraries(ygor_mesh_boolean_intersection_preflight_tests PRIVATE
    ygor_mesh_boolean_bounded_strict)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_intersection_preflight_tests)
add_test(NAME mesh_boolean_bounded_component08_preflight
    COMMAND ygor_mesh_boolean_intersection_preflight_tests)

add_executable(ygor_mesh_boolean_intersection_coordinate_tests
    TestEventCoordinates.cc
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_link_libraries(ygor_mesh_boolean_intersection_coordinate_tests PRIVATE
    ygor_mesh_boolean_bounded_strict)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_intersection_coordinate_tests)
add_test(NAME mesh_boolean_bounded_component08_coordinates
    COMMAND ygor_mesh_boolean_intersection_coordinate_tests)


add_executable(ygor_mesh_boolean_intersection_incidence_tests
    TestEventIncidence.cc
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(ygor_mesh_boolean_intersection_incidence_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_intersection_incidence_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_intersection_incidence_tests)
add_test(NAME mesh_boolean_bounded_component08_incidence
    COMMAND ygor_mesh_boolean_intersection_incidence_tests)

add_executable(ygor_mesh_boolean_source_edge_arrangement_tests
    TestSourceEdgeArrangements.cc
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(ygor_mesh_boolean_source_edge_arrangement_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ygor_mesh_boolean_source_edge_arrangement_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_source_edge_arrangement_tests)
add_test(NAME mesh_boolean_bounded_component08_source_edge_arrangements
    COMMAND ygor_mesh_boolean_source_edge_arrangement_tests)
