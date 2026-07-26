add_executable(ygor_mesh_boolean_source_edge_relation_tests
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/TestSourceEdgeRelations.cc")
target_include_directories(ygor_mesh_boolean_source_edge_relation_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(ygor_mesh_boolean_source_edge_relation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_source_edge_relation_tests)
add_test(NAME mesh_boolean_bounded_component07_source_edge_relations
    COMMAND ygor_mesh_boolean_source_edge_relation_tests)

add_executable(ygor_mesh_boolean_candidate_source_edge_relation_tests
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/TestCandidateSourceEdgeRelations.cc"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/BroadPhaseFixtures.cc"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/MinimalPublicMeshCarriers.cc"
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(
    ygor_mesh_boolean_candidate_source_edge_relation_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(
    ygor_mesh_boolean_candidate_source_edge_relation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(
    ygor_mesh_boolean_candidate_source_edge_relation_tests)
add_test(NAME mesh_boolean_bounded_component07_candidate_source_edge_relations
    COMMAND ygor_mesh_boolean_candidate_source_edge_relation_tests)


add_executable(ygor_mesh_boolean_edge_facet_relation_tests
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/TestEdgeFacetKernel.cc"
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(ygor_mesh_boolean_edge_facet_relation_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(ygor_mesh_boolean_edge_facet_relation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_edge_facet_relation_tests)
add_test(NAME mesh_boolean_bounded_component07_edge_facet_relations
    COMMAND ygor_mesh_boolean_edge_facet_relation_tests)

# This compile-only target instantiates the candidate-stream integration and
# RelationBuild publication gate without dragging the legacy monolithic Ygor
# shared library into the focused kernel test.
add_library(ygor_mesh_boolean_edge_facet_integration_compile OBJECT
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/TestEdgeFacetRelations.cc")
target_include_directories(ygor_mesh_boolean_edge_facet_integration_compile PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(ygor_mesh_boolean_edge_facet_integration_compile PRIVATE
    ygor_mesh_boolean_bounded_strict)
ygor_apply_mesh_boolean_strict_fp(
    ygor_mesh_boolean_edge_facet_integration_compile)

add_executable(ygor_mesh_boolean_edge_facet_integration_runtime_tests
    $<TARGET_OBJECTS:ygor_mesh_boolean_edge_facet_integration_compile>
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/BroadPhaseFixtures.cc"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/MinimalPublicMeshCarriers.cc"
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(
    ygor_mesh_boolean_edge_facet_integration_runtime_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(
    ygor_mesh_boolean_edge_facet_integration_runtime_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(
    ygor_mesh_boolean_edge_facet_integration_runtime_tests)
add_test(NAME mesh_boolean_bounded_component07_edge_facet_integration
    COMMAND ygor_mesh_boolean_edge_facet_integration_runtime_tests)

add_executable(ygor_mesh_boolean_facet_facet_relation_tests
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/TestFacetFacetKernel.cc"
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(ygor_mesh_boolean_facet_facet_relation_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(ygor_mesh_boolean_facet_facet_relation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_facet_facet_relation_tests)
add_test(NAME mesh_boolean_bounded_component07_facet_facet_relations
    COMMAND ygor_mesh_boolean_facet_facet_relation_tests)

add_executable(ygor_mesh_boolean_relation_artifact_tests
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/TestRelationArtifact.cc"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/BroadPhaseFixtures.cc"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/MinimalPublicMeshCarriers.cc"
    "${CMAKE_SOURCE_DIR}/src/YgorMeshesBooleanBounded/Cancellation.cc")
target_include_directories(
    ygor_mesh_boolean_relation_artifact_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(
    ygor_mesh_boolean_relation_artifact_tests PRIVATE
    ygor_mesh_boolean_bounded_strict Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_relation_artifact_tests)
add_test(NAME mesh_boolean_bounded_component07_relation_artifact
    COMMAND ygor_mesh_boolean_relation_artifact_tests)
