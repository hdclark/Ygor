#!/bin/bash
set -eu
shopt -s nocasematch

# This script will attempt to build and install Ygor in a distribution-aware way. 
#
# The distribution can be explicitly specified or detected automatically; if a specific distribution is specified, the
# package management system will be honoured, if possible.
# 
# Existing build artifacts can either be retained, causing a faster partial re-build, or purged, causing a complete,
# clean re-build.
#
# Currently supported distributions: Arch Linux, Debian, and generic (i.e., direct-installation without package manager
# support).


# The temporary location in which to build.
#BUILDROOT="/home/hal/Builds/Ygor/"
BUILDROOT="/tmp/ygor_build"
#BUILDROOT="build"

DISTRIBUTION="auto" # used to tailor the build process for specific distributions/environments.

ALSOINSTALL="yes" # install the built binaries.

CLEANBUILD="no" # purge existing (cached) build artifacts before building.


# Argument parsing:
OPTIND=1 # Reset in case getopts has been used previously in the shell.
while getopts "b:d:nch" opt; do
    case "$opt" in
    h)
        printf 'This script attempts to build and optionally install Ygor'
        printf ' in a distribution-aware way using the system package manager.\n'
        printf "\n"
        printf "Usage: \n\t $0 <args> \n"
        printf "\n"
        printf "Available arguments: \n"
        printf "\n"
        printf " -h       : Display usage information and terminate.\n"
        printf "\n"
        printf " -b <dir> : The location to use as the build root; build artifacts are cached here.\n"
        printf "          : Default: '%s'\n" "${BUILDROOT}"
        printf "\n"
        printf " -d <arg> : The distribution/environment to assume for building.\n"
        printf "          : This option controls how the build is controlled, packaged, and installed.\n"
        printf "          : Options are 'auto' (i.e., automatic detection), 'debian', 'arch', and 'generic'.\n"
        printf "          : Default: '%s'\n" "${DISTRIBUTION}"
        printf "\n"
        printf " -n       : No-install; build, but do not install.\n"
        printf "          : Default: binaries will be installed: '%s'\n" "${ALSOINSTALL}"
        printf "\n"
        printf " -c       : Clean-build; purge any existing build artifacts before building.\n"
        printf "          : Default: '%s'\n" "${CLEANBUILD}"
        printf "\n"
        exit 0
        ;;
    b)  BUILDROOT=$(realpath "$OPTARG")
        printf 'Proceeding with user-specified build root "%s".\n' "${BUILDROOT}"
        ;;
    d)  DISTRIBUTION="$OPTARG"
        printf 'Proceeding with user-specified distribution "%s".\n' "${DISTRIBUTION}"
        ;;
    n)  ALSOINSTALL="no"
        printf 'Disabling installation; building only.\n'
        ;;
    c)  CLEANBUILD="yes"
        printf 'Purging cached build artifacts for clean build.\n'
        ;;
    esac
done
#shift $((OPTIND-1))
#[ "$1" = "--" ] && shift
#echo "Leftover arguments: $@"

if [ -z "${BUILDROOT}" ] ; then
    # Proceeding with an empty build root can be dangerous, so refuse to do so.
    printf 'Empty build root directory. Refusing to continue.\n' 1>&2
    exit 1
elif [ "$(readlink -f "${BUILDROOT}")" == "/" ] ; then
    # Proceeding with build root at '/' will destroy the filesystem if the 'clean' option is used.
    printf 'Build root resolves to "/". Refusing to continue.\n' 1>&2
    exit 1
fi

# Determine which distribution/environment to assume.
if [[ "${DISTRIBUTION}" =~ .*auto.* ]] ; then
    DISTRIBUTION=$( bash -c '. /etc/os-release && printf "${NAME}"' )
fi


# Move to the repository root.
REPOROOT=$(git rev-parse --show-toplevel || true) 
if [ ! -d "${REPOROOT}" ] ; then

    # Fall-back on the source position of this script.
    SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}" )" )"
    if [ ! -d "${SCRIPT_DIR}" ] ; then
        printf "Cannot access repository root or root directory containing this script. Cannot continue.\n" 1>&2
        exit 1
    fi
    REPOROOT="${SCRIPT_DIR}"
fi
cd "${REPOROOT}"


# Determine how we will escalate privileges.
SUDO="sudo"
if [[ $EUID -eq 0 ]] ; then
    SUDO="" # no priveleges needed.
fi

#################

# Copy the repository to the build location to eliminate possibility of destroying the repo.
mkdir -p "${BUILDROOT}"
if [ -d "${BUILDROOT}"/.git ] ; then
    printf 'Build root directory "%s" contains a .git subdirectory. Refusing to overwrite it.\n' 1>&2
    exit 1
fi
if [[ "${CLEANBUILD}" =~ ^y.* ]] ; then
    $SUDO find "${BUILDROOT}/" -type f -exec chmod 644 '{}' \+
    $SUDO find "${BUILDROOT}/" -type d -exec chmod 755 '{}' \+
    $SUDO find "${BUILDROOT}/" -exec chown "$( id -n -u ):$( id -n -g )" '{}' \+
    rsync -rpt --delete --no-links --exclude="${BUILDROOT}" --cvs-exclude ./ "${BUILDROOT}/"
else
    rsync -rpt          --no-links --exclude="${BUILDROOT}" --cvs-exclude ./ "${BUILDROOT}/"
fi
cd "${BUILDROOT}"


# Perform the build (+ optional install) for each distribution type.
if [[ "${DISTRIBUTION}" =~ .*debian.* ]] ; then
    printf 'Compiling for Debian...\n'

    mkdir -p build
    cd build

    if [ -f CMakeCache.txt ] ; then
        touch CMakeCache.txt  # To bump CMake-defined compilation time.
    else
        cmake \
          -DCMAKE_INSTALL_PREFIX=/usr \
          -DCMAKE_BUILD_TYPE=Release \
          ../
    fi
    JOBS=$(nproc)
    JOBS=$(( $JOBS < 8 ? $JOBS : 8 )) # Limit to reduce memory use.
    make -j "$JOBS" VERBOSE=1
    make package

    if [[ "${ALSOINSTALL}" =~ ^y.* ]] ; then
        $SUDO apt-get --yes install -f ./*deb
    fi

elif [[ "${DISTRIBUTION}" =~ .*arch.* ]] ; then
    printf 'Compiling for Arch Linux...\n'

    # Arch's makepkg is picky, disallowing being run as root, but also requiring the non-root user to be a sudoer. What
    # a pain ... especially for chroots and controlled environments. So either create a user and dance around using
    # root/sudo, or rely on the current user to have sudo priveleges.

    if [[ $EUID -eq 0 ]] ; then
        useradd -M --shell /bin/bash dummy_build_user || true
        chown -R dummy_build_user .
        runuser dummy_build_user -c "makepkg --syncdeps --needed --noconfirm"
        if [[ "${ALSOINSTALL}" =~ ^y.* ]] ; then
            pacman --noconfirm -U $( ls -t ./*pkg.tar* | head -n 1 )
        fi
        userdel dummy_build_user

    else
        makepkg --syncdeps --needed --noconfirm
        if [[ "${ALSOINSTALL}" =~ ^y.* ]] ; then
            makepkg --syncdeps --needed --noconfirm --install
        fi
    fi


else  # Generic build and install.
    printf 'Compiling for generic Linux distribution...\n'
    printf 'Warning! Bypassing system package management and installing directly!\n'

    mkdir -p build
    cd build
    cmake \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_BUILD_TYPE=Release \
      ../
    JOBS=$(nproc)
    JOBS=$(( $JOBS < 8 ? $JOBS : 8 )) # Limit to reduce memory use.
    make -j "$JOBS" VERBOSE=1

    if [[ "${ALSOINSTALL}" =~ ^y.* ]] ; then
        $SUDO make install
    fi

fi

printf 'Done.\n'

