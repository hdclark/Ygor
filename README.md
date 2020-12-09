
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language](https://img.shields.io/github/languages/top/hdclark/Ygor.svg)](https://gitlab.com/hdeanclark/Ygor)
[![LOC](https://tokei.rs/b1/gitlab/hdeanclark/Ygor)](https://gitlab.com/hdeanclark/Ygor)

[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/hdclark/Ygor.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/hdclark/Ygor/context:cpp)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/4b2ac86e6fe446a69891e5d61fb3312a)](https://www.codacy.com/app/hdclark/Ygor?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=hdclark/Ygor&amp;utm_campaign=Badge_Grade)

[![GitLab CI Pipeline Status](https://gitlab.com/hdeanclark/Ygor/badges/master/pipeline.svg)](https://gitlab.com/hdeanclark/Ygor/-/commits/master)

[![Latest Release DOI](https://zenodo.org/badge/89644588.svg)](https://zenodo.org/badge/latestdoi/89644588)

## Introduction

This supporting library was written by hal clark over the span of many years
(~2010-2020) to house bits of code that are convenient for multiple projects.

Most, but not all of `Ygor`'s routines are focused on scientific or mathematic
applications (i.e., geometry, simulation, optimization, and statistics). The
routines in this library were all developed for specific projects with specific
needs, but which may (have) become useful for other projects. Ygor's routines
will grow, be replaced, be updated, and may even disappear when their
functionality is superceded by new features in the language/better
libraries/etc. Neither API nor ABI stability is guaranteed.

This library is meant to be 'ugly' -- it houses a ragtag bunch of routines that
would otherwise pollute other projects and be duplicated multiple times. The
focus is on providing functionality rather than optimizing the interface, speed,
or beauty. Additions of new generic routines is welcomed so long as there is
some reasonable chance they will be applicable or broadly useful.


## License and Copying

All materials herein which may be copywrited, where applicable, are. Copyright
2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 Hal Clark. See
[LICENSE.txt](LICENSE.txt) for details about the license. Informally, `Ygor` is
available under a GPLv3 license. 

All liability is herefore disclaimed. The person(s) who use this source and/or
software do so strictly under their own volition. They assume all associated
liability for use and misuse, including but not limited to damages, harm,
injury, and death which may result, including but not limited to that arising
from unforeseen or unanticipated implementation defects.


## Project Home

The `Ygor` homepage can be found at <http://www.halclark.ca/>. Source code is
available at <https://gitlab.com/hdeanclark/Ygor/> and
<https://github.com/hdclark/Ygor/>.


## Dependencies

See [PKGBUILD](PKGBUILD) for a up-to-date list of dependencies.
As of writing, various bits of `Ygor` rely on:

- `STL`/C++17: heavily, throughout; mostly C++11 but some C++14 and C++17.
- `Boost`: various.
- `Eigen`: a few YgorMath routines, optionally.
- `spookyhash`: YgorAlgorithms. Very little - one function at time of writing.
- `GNU Scientific Library`: YgorStats and YgorMathBasisSplines.
- `GNU plotutils`: YgorPlot. Weakly - via shell/pipe.
- `Gnuplot`: YgorPlot. Weakly - via shell/pipe.

All dependencies can be optionally disabled, though this will result in loss of
functionality.

## Installation

This project uses `CMake`. Use the usual commands to compile:

    $>  cd /path/to/source/directory
    $>  mkdir build && cd build/

Then, iff by-passing your package manager:

    $>  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
    $>  make && sudo make install

or, if building for Debian:

    $>  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
    $>  make && make package
    $>  sudo apt install -f ./*.deb

or, if building for Arch Linux:

    $>  rsync -aC --exclude build ../ ./
    $>  makepkg --syncdeps --noconfirm # Optionally also [--install].

A helper script that will auto-detect the system and package or install properly
can be invoked as:

    $>  ./compile_and_install.sh


## Known Issues

- The `Gnuplot` async plotter will cause the main thread to hang on some systems.
  It can be made to block instead, but this is hard-coded at the moment. The
  core issue appears to be communicating via pipes with `Gnuplot` rather than via
  a proper API.


## Rough Historical Timeline

- May/June 2012
  - The routines from some of the common convenience headers were collected into
    (this) project folder. The conversion to a set of linkable and/or
    header-only sources was started.

- Sept 2012
  - Some routines have gradually been modified or added as needed. The inclusion
    of a binaries directory is designed to expose some library functions to the
    shell. It has been difficult to keep the library free of external
    dependencies.

- May 2013 
  - Development in the library has waned somewhat due to functionality existing
    for most of my current use-cases. I haven't bought into unit tests nor
    version control yet. Currently, I'm interested in cleaning and curating what
    I've got to provide as solid a base as possible for future addition.

- June 2013
  - Renamed library from 'Project - Utilities' and 'MyUtils' to 'Project - Ygor'
    in order to release it. Haven't had a chance yet because of ongoing thesis
    work. It'll have to wait until afterward.

- July 2013
  - Lately, library development has mostly focused on minor additions,
    refactoring, and addition of extra error handling for easier use. I expect
    development to pick up a little after the thesis. Specifically, I'd like to
    focus on code quality. It mostly seems to work (I don't often encounter
    bugs), but there are major irritants like non-standard error
    handling/signalling, naming conventions all over the place, and huge files
    which should be broken up into smaller pieces (or several books :) ).

- Aug 2013
  - Small additions, mostly elaborations of existing routines. Major additions
    include parametric fitting (partial migration from Project - PolyCAS) and
    some statistical routines for p-values. The latter introduce a GSL
    dependency. I hope to implement my own beta functions to get rid of it ASAP
    ... but the GSL isn't such a bad dependency. It is nice to have quality
    code publically available.

- Apr 2014
  - Cleanup effort underway. I've tried to fix inconsistent naming schemes,
    performed static analysis to find any bugs which haven't yet been found,
    shuffled code around -- either into more specific files or namespaces, and
    tried to use more consistently handle function behaviour arguments and error
    signaling. I've also spent some time thinking about a better unit testing
    scheme. This was a big undertaking!

- July 2016
  - Migrated code and limited history (~late 2015) to git to ease deployment.
    Some clean-up of junk files was performed. Still relying on GNU Make(files),
    and the project hierarchy is a bit of a mess. Modernization is planned.

- March 2017
  - Migrated to CMake. Removed a few small extraneous files, but the repo needs
    a good clean. The code also needs to be modernized and warnings dealt with.

- April 2017
  - Made sources publically available on GitHub and GitLab at
    [https://gitlab.com/hdeanclark/Ygor] and [https://github.com/hdclark/Ygor].

- April 2019
  - Added integration with static analysis tools and rudimentary CI tooling.

- January 2020
  - Added support for writing many types of mesh file formats (obj, off, stl) 
    over the last few months. Limited reading support since some of the formats
    are fairly open-ended.

- February 2020
  - Improved 3D voxel lookup efficiency for regular images via the 
    `planar_image_adjacency` routine.

- March 2020
  - Added faster projection-based `is_point_in_poly` sub-routine to generic 
    `Mutate_Voxels` interface routine. 
  - Revisited test cases, ensuring they all build.

- April 2020
  - Added generic TAR file read and write support. Small clean-up primarily in
    math and image files.

- May 2020
  - Started writing unit tests in earnest for core math components focusing on
    basic usage and edge behaviour (e.g., handling non-finite data).
  - Added basic Affine transformation class.

- June 2020
  - Added bare-bones arbitrary-dimension matrix class to provide 'polyfill' for
    when external libraries may not be available, to ensure a means of passing
    matrices across library interface with a stable API (and ABI, if needed),
    and as a means of controlling serializability.

- August 2020
  - Expanded testing, added CI build-and-test, and increased static analysis.
  - Re-wrote a few routines to use more modern features to improve portability.
    The port is not comprehensive, but `Ygor` can now be compiled with `MXE`.

- October 2020
  - Encircle\_Images: provide default enum class.
  - contour\_collection: added metadata selector.
  - Document release DOI.

- November 2020
  - Overhauled XYZ, OBJ, and OFF file IO readers to be less stringent when
    reading point sets.
  - Point sets gained support for normals.
  - Added unbiased surface mesh random sampler. This can be used for Monte Carlo
    surface sampling.
  - Factored surface mesh garbage collection into a single cleanup routine.
  - Only link libstdc++fs if it is present.

- December 2020
  - Added basic ASCII PLY read and write support for surface meshes.
  - Added vec2 and vec3 layout checks and cast operators to and from std::array.

