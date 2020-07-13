# Maintainer: Hal Clark <gmail.com[at]hdeanclark>
pkgname=ygor
pkgver=20191108_093000
pkgver() {
  date +%Y%m%d_%H%M%S
}
pkgrel=1

pkgdesc="Support library with scientific emphasis."
url="http://www.halclark.ca"
arch=('x86_64' 'i686' 'armv7h')
license=('unknown')
depends=(
   'boost'   # If enabled (see below).
   'gsl'     # If enabled (see below).
)
makedepends=(
   'cmake'
   'eigen'   # If enabled (see below).
   'boost'   # Library dependency, if enabled (see below).
)
optdepends=(
   'gnuplot'   # Runtime optional dependency.
   'boost'     # User build header-only AND/OR library optional dependency.
   'plotutils' # Runtime optional dependency.
)
# conflicts=()
# replaces=()
# backup=()
# install='foo.install'
#source=("http://www.server.tld/${pkgname}-${pkgver}.tar.gz"
#        "foo.desktop")
#md5sums=('a0afa52d60cea6c0363a2a8cb39a4095'
#         'a0afa52d60cea6c0363a2a8cb39a4095')

#options=(!strip staticlibs)
options=(strip staticlibs)
#PKGEXT='.pkg.tar' # Disable compression.

build() {
  # ---------------- Configure -------------------
  # Try use environment variable, but fallback to standard. 
  install_prefix=${INSTALL_PREFIX:-/usr}

  # Default build with default compiler flags.
  cmake \
    -DCMAKE_INSTALL_PREFIX="${install_prefix}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_LINUX_SYS=ON \
    -DWITH_EIGEN=ON \
    -DWITH_GNU_GSL=ON \
    -DWITH_BOOST=ON \
    ../
  make -j 4 VERBOSE=1
}

package() {
  #make -j 4 DESTDIR="${pkgdir}" install
  make -j 4 DESTDIR="${pkgdir}" install
}

# vim:set ts=2 sw=2 et:
