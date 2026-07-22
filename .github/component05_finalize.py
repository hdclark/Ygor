from pathlib import Path


def ensure_replaced(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    if old in text:
        text = text.replace(old, new)
        target.write_text(text)
        return
    if new and new in text:
        return
    if not new and old not in text:
        return
    raise SystemExit(f"{path}: neither pending nor finalized form found")


ensure_replaced(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeCodec.h",
    "    writer.u64(vertex.presentation_vertex);\n",
    "",
)

ensure_replaced(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeBuildCore.h",
    "  if (persistent > capabilities_.maximum_canonical_bytes +\n"
    "                       counts_.estimated_persistent_bytes + extra &&\n"
    "      capabilities_.maximum_canonical_bytes != 0) {\n"
    "    // The explicit byte cap is checked against the finished canonical stream;\n"
    "    // this branch merely keeps the arithmetic above intentional.\n"
    "  }\n",
    "",
)

ensure_replaced(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeBuildFinalize.h",
    "  artifact_->canonical_bytes_ = encode_canonical_halfedge_complete(\n"
    "      *artifact_, artifact_->source_semantic_bytes_,\n"
    "      artifact_->exact_topology_bytes_, artifact_->geometry_attachment_bytes_);\n"
    "  artifact_->digest_ = sha256::digest(artifact_->canonical_bytes_);\n"
    "  if (artifact_->canonical_bytes_.size() >\n"
    "      capabilities_.maximum_canonical_bytes)\n"
    "    return fail(canonical_halfedge_subcode::resource_preflight,\n"
    "                bounded_boolean_error_category::resource_limit,\n"
    "                \"canonical halfedge byte stream exceeds configured limit\",\n"
    "                canonical_halfedge_checkpoint::canonical_encoding);\n"
    "  if (encode_canonical_halfedge_operand_independent(*artifact_) !=\n"
    "      artifact_->canonical_bytes_)\n"
    "    return fail(canonical_halfedge_subcode::canonical_bytes_mismatch,\n"
    "                bounded_boolean_error_category::internal_invariant_error,\n"
    "                \"independent canonical halfedge encoder disagrees\",\n"
    "                canonical_halfedge_checkpoint::independent_verification);\n"
    "  std::uint32_t finding = 0;\n"
    "  if (!verify_canonical_halfedge_operand(*artifact_, *validated_, *source_,\n"
    "                                         precision_, &finding)) {\n"
    "    fail(canonical_halfedge_subcode::verifier_rejection,\n"
    "         bounded_boolean_error_category::internal_invariant_error,\n"
    "         \"independent canonical halfedge verifier rejected artifact\",\n"
    "         canonical_halfedge_checkpoint::independent_verification);\n"
    "    error_.witnesses[0] = finding;\n"
    "    error_.witness_count = 1;\n"
    "    return false;\n"
    "  }\n"
    "  const auto used = persistent_bytes();\n"
    "  if (used == std::numeric_limits<std::uint64_t>::max() ||\n"
    "      !persistent_reservation_ || used > persistent_reservation_->amount() ||\n"
    "      !work_reservation_ ||\n"
    "      artifact_->statistics_.work_units > work_reservation_->amount())\n"
    "    return fail(canonical_halfedge_subcode::resource_preflight,\n"
    "                bounded_boolean_error_category::resource_limit,\n"
    "                \"canonical halfedge actual resource use exceeds reservation\",\n"
    "                canonical_halfedge_checkpoint::resource_reconciliation);\n"
    "  artifact_->statistics_.persistent_bytes = used;\n",
    "  artifact_->canonical_bytes_ = encode_canonical_halfedge_complete(\n"
    "      *artifact_, artifact_->source_semantic_bytes_,\n"
    "      artifact_->exact_topology_bytes_, artifact_->geometry_attachment_bytes_);\n"
    "  artifact_->statistics_.persistent_bytes = persistent_bytes();\n"
    "  artifact_->canonical_bytes_ = encode_canonical_halfedge_complete(\n"
    "      *artifact_, artifact_->source_semantic_bytes_,\n"
    "      artifact_->exact_topology_bytes_, artifact_->geometry_attachment_bytes_);\n"
    "  artifact_->digest_ = sha256::digest(artifact_->canonical_bytes_);\n"
    "  const auto used = persistent_bytes();\n"
    "  if (used == std::numeric_limits<std::uint64_t>::max() ||\n"
    "      used != artifact_->statistics_.persistent_bytes ||\n"
    "      !persistent_reservation_ || used > persistent_reservation_->amount() ||\n"
    "      !work_reservation_ ||\n"
    "      artifact_->statistics_.work_units > work_reservation_->amount())\n"
    "    return fail(canonical_halfedge_subcode::resource_preflight,\n"
    "                bounded_boolean_error_category::resource_limit,\n"
    "                \"canonical halfedge actual resource use exceeds reservation\",\n"
    "                canonical_halfedge_checkpoint::resource_reconciliation);\n"
    "  if (artifact_->canonical_bytes_.size() >\n"
    "      capabilities_.maximum_canonical_bytes)\n"
    "    return fail(canonical_halfedge_subcode::resource_preflight,\n"
    "                bounded_boolean_error_category::resource_limit,\n"
    "                \"canonical halfedge byte stream exceeds configured limit\",\n"
    "                canonical_halfedge_checkpoint::canonical_encoding);\n"
    "  if (encode_canonical_halfedge_operand_independent(*artifact_) !=\n"
    "      artifact_->canonical_bytes_)\n"
    "    return fail(canonical_halfedge_subcode::canonical_bytes_mismatch,\n"
    "                bounded_boolean_error_category::internal_invariant_error,\n"
    "                \"independent canonical halfedge encoder disagrees\",\n"
    "                canonical_halfedge_checkpoint::independent_verification);\n"
    "  std::uint32_t finding = 0;\n"
    "  if (!verify_canonical_halfedge_operand(*artifact_, *validated_, *source_,\n"
    "                                         precision_, &finding)) {\n"
    "    fail(canonical_halfedge_subcode::verifier_rejection,\n"
    "         bounded_boolean_error_category::internal_invariant_error,\n"
    "         \"independent canonical halfedge verifier rejected artifact\",\n"
    "         canonical_halfedge_checkpoint::independent_verification);\n"
    "    error_.witnesses[0] = finding;\n"
    "    error_.witness_count = 1;\n"
    "    return false;\n"
    "  }\n",
)

ensure_replaced(
    "tests/mesh_boolean_bounded/TestCanonicalHalfedge.cc",
    "  bounded::bounded_boolean_cancellation_source cancellation;\n",
    "  bounded_boolean_cancellation_source cancellation;\n",
)
ensure_replaced(
    "tests/mesh_boolean_bounded/TestCanonicalHalfedge.cc",
    "                         bounded::bounded_boolean_error_category::cancelled,\n",
    "                         bounded_boolean_error_category::cancelled,\n",
)
ensure_replaced(
    "tests/mesh_boolean_bounded/TestCanonicalHalfedge.cc",
    "                         bounded::bounded_boolean_error_category::resource_limit,\n",
    "                         bounded_boolean_error_category::resource_limit,\n",
)
ensure_replaced(
    "tests/mesh_boolean_bounded/TestCanonicalHalfedge.cc",
    "                         statistics.edges + statistics.halfedges * 2,\n",
    "                         statistics.source_edges +\n"
    "                             statistics.internal_diagonals +\n"
    "                             statistics.halfedges * 2,\n",
)

cmake_path = Path("tests/mesh_boolean_bounded/CMakeLists.txt")
cmake = cmake_path.read_text()
old_loop = "foreach(consumer_component 02 04 06 07 13 15 16 17)"
new_loop = "foreach(consumer_component 02 04 05 06 07 13 15 16 17)"
if old_loop in cmake:
    cmake = cmake.replace(old_loop, new_loop, 1)
elif new_loop not in cmake:
    raise SystemExit("precision capability consumer loop did not match")
block = r'''

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
'''
if "ygor_mesh_boolean_canonical_halfedge_tests" not in cmake:
    cmake += block
cmake_path.write_text(cmake)
