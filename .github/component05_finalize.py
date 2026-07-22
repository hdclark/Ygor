from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one occurrence, found {count}: {old!r}")
    target.write_text(text.replace(old, new, 1))


replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeCodec.h",
    "    writer.u64(vertex.presentation_vertex);\n",
    "",
)

replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeBuildCore.h",
    "  if (persistent > capabilities_.maximum_canonical_bytes +\n"
    "                       counts_.estimated_persistent_bytes + extra &&\n"
    "      capabilities_.maximum_canonical_bytes != 0) {\n"
    "    // The explicit byte cap is checked against the finished canonical stream;\n"
    "    // this branch merely keeps the arithmetic above intentional.\n"
    "  }\n",
    "",
)

replace_once(
    "tests/mesh_boolean_bounded/TestCanonicalHalfedge.cc",
    "  bounded::bounded_boolean_cancellation_source cancellation;\n",
    "  bounded_boolean_cancellation_source cancellation;\n",
)
replace_once(
    "tests/mesh_boolean_bounded/TestCanonicalHalfedge.cc",
    "                         bounded::bounded_boolean_error_category::cancelled,\n",
    "                         bounded_boolean_error_category::cancelled,\n",
)
replace_once(
    "tests/mesh_boolean_bounded/TestCanonicalHalfedge.cc",
    "                         bounded::bounded_boolean_error_category::resource_limit,\n",
    "                         bounded_boolean_error_category::resource_limit,\n",
)

cmake_path = Path("tests/mesh_boolean_bounded/CMakeLists.txt")
cmake = cmake_path.read_text()
old_loop = "foreach(consumer_component 02 04 06 07 13 15 16 17)"
if cmake.count(old_loop) != 1:
    raise SystemExit("precision capability consumer loop did not match")
cmake = cmake.replace(
    old_loop, "foreach(consumer_component 02 04 05 06 07 13 15 16 17)", 1
)
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
if "ygor_mesh_boolean_canonical_halfedge_tests" in cmake:
    raise SystemExit("canonical halfedge CMake target already exists")
cmake_path.write_text(cmake + block)
