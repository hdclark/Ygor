# Maintainer: Hal Clark <gmail.com[at]hdeanclark>
pkgname=ygor
pkgver=20170327_220203
pkgver() {
  date +%Y%m%d_%H%M%S
}
pkgrel=1

pkgdesc="D.R.Y. support library with scientific emphasis."
url="http://www.halclark.ca"
arch=('x86_64' 'i686' 'armv7h')
license=('unknown')
depends=(
   'htmlcxx'
   'gsl'
)
#optdepends=()
makedepends=(
   'cmake'
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

build() {
  #cmake "${pkgdir}" -DCMAKE_INSTALL_PREFIX=/usr
  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
  make -j 4
}

package() {
  #make -j 4 DESTDIR="${pkgdir}" install
  make -j 4 DESTDIR="${pkgdir}" install
}

# vim:set ts=2 sw=2 et:
