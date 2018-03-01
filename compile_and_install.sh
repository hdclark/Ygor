#!/bin/bash
set -e

BUILDDIR="/home/hal/Builds/Ygor/"
BUILTPKGSDIR="/home/hal/Builds/"

mkdir -p "${BUILDDIR}"
rsync -avz --no-links --cvs-exclude ./ "${BUILDDIR}"

pushd .
cd "${BUILDDIR}"
makepkg --syncdeps --install --noconfirm
mv "${BUILDDIR}/"*.pkg.tar.* "${BUILTPKGSDIR}/"
popd

