from pathlib import Path

path = Path("src/YgorMeshesBooleanBounded/CMakeLists.txt")
text = path.read_text()
old = 'target_include_directories(ygor_mesh_boolean_bounded_facade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")\n'
new = '''target_include_directories(ygor_mesh_boolean_bounded_facade PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_BINARY_DIR}/..")
'''
if text.count(old) != 1:
    raise SystemExit(
        f"facade generated-header include anchor: expected one match, found {text.count(old)}"
    )
path.write_text(text.replace(old, new, 1))
