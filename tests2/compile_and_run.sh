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

mkdir -p doctest/
wget -q 'https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h' -O doctest/doctest.h

BUILD_DIR="${REPOROOT}/build-tests2"
cmake -S "${REPOROOT}" -B "${BUILD_DIR}" \
  -DWITH_BOOST=OFF \
  -DWITH_EIGEN=OFF \
  -DWITH_GNU_GSL=OFF
cmake --build "${BUILD_DIR}" -j2

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
  YgorMathIOOBJ.cc \
  YgorMathIOOFF.cc \
  YgorMathIOPLY.cc \
  YgorMathIOSTL.cc \
  YgorMathIOXYZ.cc \
  YgorMathIOCSV.cc \
  YgorMeshesConvexHull.cc \
  YgorMeshesHoles.cc \
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
  -Wl,-rpath,"${BUILD_DIR}/lib" \
  -lygor

rm -rf doctest/

./run_tests #--success

rm ./run_tests
