#!/bin/bash
set -e

BUILDDIR="/home/hal/Builds/Ygor/"
BUILTPKGSDIR="/home/hal/Builds/"

mkdir -p "${BUILDDIR}"
rsync -avz --no-links --cvs-exclude ./ "${BUILDDIR}"

pushd .
cd "${BUILDDIR}"
if [ -f CMakeCache.txt ] ; then 
    touch CMakeCache.txt  # To bump CMake-defined compilation time.
else
    cmake \
      . \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DWITH_LINUX_SYS=ON \
      -DWITH_EIGEN=ON \
      -DWITH_GNU_GSL=ON \
      -DWITH_BOOST=ON 

fi
make -j $(nproc) VERBOSE=1 && make package
mv *.deb "${BUILTPKGSDIR}/"
popd

