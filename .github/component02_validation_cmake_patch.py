from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one match, found {count}")
    return text.replace(old, new, 1)


path = Path("src/YgorMeshesBooleanBounded/CMakeLists.txt")
text = path.read_text()
text = replace_once(
    text,
    '''target_include_directories(ygor_exact_float_expansion_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
''',
    '''target_include_directories(ygor_exact_float_expansion_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
''',
    "exact-core generated-header include",
)
text = replace_once(
    text,
    '''target_include_directories(ygor_mesh_boolean_bounded_strict PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
''',
    '''target_include_directories(ygor_mesh_boolean_bounded_strict PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
''',
    "strict-library generated-header include",
)
text = replace_once(
    text,
    'target_include_directories(ygor_mesh_boolean_bounded_facade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")\n',
    '''target_include_directories(ygor_mesh_boolean_bounded_facade PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_BINARY_DIR}/..")
''',
    "facade generated-header include",
)
path.write_text(text)
