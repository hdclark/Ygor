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

g++ \
  -std=c++17 \
  -Wall \
  -I. \
  Main.cc \
  YgorContainers/*.cc \
  YgorFilesDirs.cc \
  YgorIO.cc \
  YgorIndexRTree.cc \
  YgorIndexCells.cc \
  YgorIndexOctree.cc \
  YgorMath/*.cc \
  YgorMathIOOBJ.cc \
  YgorMathIOOFF.cc \
  YgorMathIOPLY.cc \
  YgorMathIOXYZ.cc \
  YgorMeshesHoles.cc \
  YgorTime/*.cc \
  YgorString.cc \
  -o run_tests \
  -lygor

rm -rf doctest/

./run_tests #--success

rm ./run_tests

