#!/usr/bin/env bash

set -eux

reporoot=$(git rev-parse --show-toplevel)
cd "${reporoot}/tests2"

mkdir -p doctest/
wget -q 'https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h' -O doctest/doctest.h

g++ \
  -std=c++17 \
  -Wall \
  -I. \
  Main.cc \
  YgorFilesDirs.cc \
  YgorIO.cc \
  YgorMath/*.cc \
  YgorMathIOOBJ.cc \
  YgorMathIOOFF.cc \
  YgorMathIOPLY.cc \
  YgorMathIOXYZ.cc \
  YgorTime/*.cc \
  YgorString.cc \
  -o run_tests \
  -lygor

rm -rf doctest/

./run_tests #--success

rm ./run_tests

