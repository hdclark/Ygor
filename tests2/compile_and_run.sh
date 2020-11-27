#!/usr/bin/env bash

set -eu

reporoot=$(git rev-parse --show-toplevel)
cd "${reporoot}/tests2"

mkdir -p doctest/
wget -q 'https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h' -O doctest/doctest.h

g++ \
  -std=c++17 \
  -Wall \
  -I. \
  Main.cc \
  YgorMath/*.cc \
  YgorFilesDirs.cc \
  YgorMathIOXYZ.cc \
  YgorMathIOOBJ.cc \
  YgorMathIOOFF.cc \
  -o run_tests \
  -lygor

rm -rf doctest/

./run_tests #--success

rm ./run_tests

