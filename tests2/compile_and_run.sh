#!/usr/bin/env bash

set -eux

# Move to the test root.
REPOROOT="$(git rev-parse --show-toplevel || true)"
if [ ! -d "${REPOROOT}" ] ; then

    # Fall-back on the source position of this script.
    SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}" )" )"
    if [ ! -d "${SCRIPT_DIR}" ] ; then
        printf "Cannot access repository root or root directory containing this script. Cannot continue.\n" 1>&2
        exit 1
    fi
    REPOROOT="${SCRIPT_DIR}/../"
fi
cd "${REPOROOT}/tests2/"

BUILD_DIR="${YGOR_BUILD_DIR:-${REPOROOT}/build}"
if [ ! -d "${BUILD_DIR}/src" ] ; then
    printf "Cannot access build directory '%s'. Set YGOR_BUILD_DIR to a configured CMake build tree.\n" "${BUILD_DIR}" 1>&2
    exit 1
fi

rm -rf doctest/
mkdir -p doctest/
wget -q 'https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h' -O doctest/doctest.h
sed -i -e 's@^\([#]define INFO[(]\)@//\1@g' doctest/doctest.h
sed -i -e 's@^\([#]define WARN[(]\)@//\1@g' doctest/doctest.h

g++ \
  -std=c++17 \
  -Wall \
  -Wno-unused-variable \
  -Wno-unused-value \
  -I. \
  -I"${REPOROOT}/src" \
  -I"${BUILD_DIR}/src" \
  Main.cc \
  \
  YgorAlgorithms.cc \
  YgorBase64.cc \
  YgorContainers/*.cc \
  YgorFilesDirs.cc \
  YgorImages.cc \
  YgorIndexCells.cc \
  YgorIndexKDTree.cc \
  YgorIndexOctree.cc \
  YgorIndexRTree.cc \
  YgorIO.cc \
  YgorIOgzip.cc \
  YgorMath/*.cc \
  YgorMathDelaunay.cc \
  YgorMathConstrainedDelaunay.cc \
  YgorMathMonotoneDecomposition.cc \
  YgorMathIOOBJ.cc \
  YgorMathIOOFF.cc \
  YgorMathIOPLY.cc \
  YgorMathIOSTL.cc \
  YgorMathIOXYZ.cc \
  YgorMathIOCSV.cc \
  YgorMeshesConvexHull.cc \
  YgorMeshesHoles.cc \
  YgorMeshesBoolean.cc \
  YgorMeshesOrient.cc \
  YgorMeshesRefinement.cc \
  YgorMeshesRemeshing.cc \
  YgorMeshesTetrahedralize.cc \
  YgorNoise.cc \
  YgorOptimizeBFGS.cc \
  YgorOptimizeLM.cc \
  YgorOptimizeSA.cc \
  YgorSerialize.cc \
  YgorStats.cc \
  YgorStatsCITrees.cc \
  YgorStatsConditionalForests.cc \
  YgorStatsStochasticForests.cc \
  YgorString.cc \
  YgorTime/*.cc \
  \
  -o run_tests \
  -L"${BUILD_DIR}/lib" \
  -lygor

rm -rf doctest/

./run_tests #--success

rm ./run_tests
