#!/usr/bin/env bash
set -euo pipefail

binary_dir=${1:?missing CMake binary directory}
compiler=${CXX:-c++}
work=/tmp/ygor-component07-rerun-${compiler//[^A-Za-z0-9_.-]/_}
stage=/tmp/ygor-component07-rerun-artifact-${compiler//[^A-Za-z0-9_.-]/_}
rm -rf "$work" "$stage"

git fetch --force --depth=1 origin agent/bounded-manifold-mesh-boolean-plan
git worktree add --detach "$work" FETCH_HEAD
cd "$work"

cat .github/component07-coplanar-event-topology.patch.part-* \
  | base64 -d | gzip -d > /tmp/component07-coplanar-event-topology.patch
echo "0a4b10d4e0046f3d113dc9654ff2953e41e08099f7c4b577631444c177b4096b  /tmp/component07-coplanar-event-topology.patch" \
  | sha256sum -c -
git apply --check /tmp/component07-coplanar-event-topology.patch
git apply --index /tmp/component07-coplanar-event-topology.patch
git diff --check --cached

focused=/tmp/ygor-component07-focused-${compiler//[^A-Za-z0-9_.-]/_}
rm -rf "$focused"
mkdir -p "$focused"
python3 - "$focused" <<'PY'
from pathlib import Path
import sys
repo = Path.cwd()
out_dir = Path(sys.argv[1])
source_cmake = (repo / "src/YgorMeshesBooleanBounded/CMakeLists.txt").read_text()
start = source_cmake.index("add_library(ygor_mesh_boolean_bounded_strict")
end = source_cmake.index("set_property(TARGET ygor_mesh_boolean_bounded_strict", start)
block = source_cmake[start:end]
files = []
for line in block.splitlines()[1:]:
    value = line.strip()
    if not value:
        continue
    if value.endswith(")"):
        value = value[:-1]
        if value:
            files.append(value)
        break
    files.append(value)
output = '''cmake_minimum_required(VERSION 3.16)
project(YgorComponent07Focused LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)
function(strict target)
  target_compile_definitions(${target} PRIVATE YGOR_MESH_BOOLEAN_STRICT_FP_BUILD=1)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE
      -fno-fast-math -frounding-math -fno-associative-math
      -fno-reciprocal-math -fsigned-zeros -fno-finite-math-only
      -ffp-contract=off)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PRIVATE
      -fno-fast-math -frounding-math -fsigned-zeros -ffp-contract=off)
  else()
    message(FATAL_ERROR "Unqualified strict floating profile")
  endif()
endfunction()
set(REPO $ENV{COMPONENT07_WORKTREE})
configure_file(${REPO}/src/YgorDefinitions.h.in
               ${CMAKE_BINARY_DIR}/YgorDefinitions.h)
add_library(core
  ${REPO}/src/YgorMeshesExactFloatExpansionCore.cc
  ${REPO}/src/YgorMeshesAdaptivePredicates.cc)
target_include_directories(core PUBLIC ${REPO}/src ${CMAKE_BINARY_DIR})
strict(core)
add_library(strictlib
'''
for filename in files:
    output += f"  ${{REPO}}/src/YgorMeshesBooleanBounded/{filename}\n"
output += ''')
target_include_directories(strictlib PUBLIC ${REPO}/src ${CMAKE_BINARY_DIR})
target_link_libraries(strictlib PUBLIC core)
strict(strictlib)
function(component07_test target)
  add_executable(${target} ${ARGN})
  target_include_directories(${target} PRIVATE
    ${REPO}/src ${CMAKE_BINARY_DIR}
    ${REPO}/tests/mesh_boolean_bounded)
  target_link_libraries(${target} PRIVATE strictlib Threads::Threads)
  strict(${target})
endfunction()
component07_test(candidate_edge
  ${REPO}/tests/mesh_boolean_bounded/TestCandidateSourceEdgeRelations.cc
  ${REPO}/tests/mesh_boolean_bounded/BroadPhaseFixtures.cc
  ${REPO}/tests/mesh_boolean_bounded/MinimalPublicMeshCarriers.cc
  ${REPO}/src/YgorMeshesBooleanBounded/Cancellation.cc)
component07_test(edge_facet
  ${REPO}/tests/mesh_boolean_bounded/TestEdgeFacetKernel.cc
  ${REPO}/src/YgorMeshesBooleanBounded/Cancellation.cc)
component07_test(edge_facet_integration
  ${REPO}/tests/mesh_boolean_bounded/TestEdgeFacetRelations.cc
  ${REPO}/tests/mesh_boolean_bounded/BroadPhaseFixtures.cc
  ${REPO}/tests/mesh_boolean_bounded/MinimalPublicMeshCarriers.cc
  ${REPO}/src/YgorMeshesBooleanBounded/Cancellation.cc)
component07_test(facet_facet
  ${REPO}/tests/mesh_boolean_bounded/TestFacetFacetKernel.cc
  ${REPO}/src/YgorMeshesBooleanBounded/Cancellation.cc)
component07_test(coplanar
  ${REPO}/tests/mesh_boolean_bounded/TestCoplanarRelationOverlay.cc
  ${REPO}/src/YgorMeshesBooleanBounded/Cancellation.cc)
'''
(out_dir / "CMakeLists.txt").write_text(output)
PY

export COMPONENT07_WORKTREE="$work"
cmake -S "$focused" -B "$focused/build" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$focused/build" -j2 \
  --target candidate_edge edge_facet edge_facet_integration facet_facet coplanar

validation_log=/tmp/component07-validation-${compiler//[^A-Za-z0-9_.-]/_}.txt
: > "$validation_log"
for test in candidate_edge edge_facet edge_facet_integration facet_facet coplanar; do
  echo "===== ${test} =====" | tee -a "$validation_log"
  "$focused/build/${test}" 2>&1 | tee -a "$validation_log"
done

mkdir -p "$stage/src/YgorMeshesBooleanBounded" "$stage/tests/mesh_boolean_bounded"
cp component_07_implementation_status.md "$stage/"
cp tracker.md "$stage/"
cp src/YgorMeshesBooleanBounded/ContractVersions.h "$stage/src/YgorMeshesBooleanBounded/"
cp src/YgorMeshesBooleanBounded/CoplanarRelationOverlay.h "$stage/src/YgorMeshesBooleanBounded/"
cp tests/mesh_boolean_bounded/TestCoplanarRelationOverlay.cc "$stage/tests/mesh_boolean_bounded/"
cp /tmp/component07-coplanar-event-topology.patch "$stage/"
cp "$validation_log" "$stage/validation.txt"
git diff --cached --binary > "$stage/applied.diff"
(
  cd "$stage"
  find . -type f -print0 | sort -z | xargs -0 sha256sum > manifest.sha256
)
tar -C "$stage" -czf /tmp/ygor-component07-validated-postimage.tar.gz .
mkdir -p "$binary_dir/Testing/Temporary"
{
  echo 'COMPONENT07_VALIDATED_POSTIMAGE_BEGIN'
  base64 -w 76 /tmp/ygor-component07-validated-postimage.tar.gz
  echo 'COMPONENT07_VALIDATED_POSTIMAGE_END'
} > "$binary_dir/Testing/Temporary/LastTest.log"

# Fail deliberately so the pre-existing workflow uploads LastTest.log.
exit 97
