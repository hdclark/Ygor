#!/bin/bash
set -e

BUILDDIR="/home/hal/Builds/Ygor/"
BUILTPKGSDIR="/home/hal/Builds/"

mkdir -p "${BUILDDIR}"
rsync -avz --no-links --cvs-exclude --delete ./ "${BUILDDIR}"  # Removes CMake cache files, forcing a fresh rebuild.

pushd .
cd "${BUILDDIR}"
cmake \
  . \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DWITH_LINUX_SYS=ON \
  -DWITH_EIGEN=ON \
  -DWITH_GNU_GSL=ON \
  -DWITH_BOOST=ON 

make -j $(nproc) && make package
mv *.deb "${BUILTPKGSDIR}/"
popd

