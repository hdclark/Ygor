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
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/BroadPhaseFixtures.cc")
target_include_directories(
    ygor_mesh_boolean_candidate_source_edge_relation_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded")
target_link_libraries(
    ygor_mesh_boolean_candidate_source_edge_relation_tests PRIVATE
    ygor_mesh_boolean_bounded_strict ygor Threads::Threads)
ygor_apply_mesh_boolean_strict_fp(
    ygor_mesh_boolean_candidate_source_edge_relation_tests)
add_test(NAME mesh_boolean_bounded_component07_candidate_source_edge_relations
    COMMAND ygor_mesh_boolean_candidate_source_edge_relation_tests)
