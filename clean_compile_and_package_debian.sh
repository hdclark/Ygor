#!/bin/bash
set -e

BUILDDIR="/home/hal/Builds/Ygor/"
BUILTPKGSDIR="/home/hal/Builds/"

mkdir -p "${BUILDDIR}"
rsync -avz --no-links --cvs-exclude --delete ./ "${BUILDDIR}"  # Removes CMake cache files, forcing a fresh rebuild.

pushd .
cd "${BUILDDIR}"
cmake . -DCMAKE_INSTALL_PREFIX=/usr
make -j $(nproc) && make package
mv *.deb "${BUILTPKGSDIR}/"
popd

