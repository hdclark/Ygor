
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language](https://img.shields.io/github/languages/top/hdclark/Ygor.svg)](https://gitlab.com/hdeanclark/Ygor)

[![GitLab CI Pipeline Status](https://gitlab.com/hdeanclark/Ygor/badges/master/pipeline.svg)](https://gitlab.com/hdeanclark/Ygor/-/commits/master)

[![Latest Release DOI](https://zenodo.org/badge/89644588.svg)](https://zenodo.org/badge/latestdoi/89644588)

## Introduction

This supporting library was written by hal clark over the span of many years
(2010-2024) to house bits of code that are convenient for multiple projects.

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
2010-2024 Hal Clark. See [LICENSE.txt](LICENSE.txt) for details about the
license. Informally, `Ygor` is available under a GPLv3 license. 

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
  - Routines from some ad-hoc common 'convenience' headers were amalgamated into
    this project. The conversion to a set of linkable and/or header-only sources
    was started.

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
  - Support for Boost::Serialization 1.74 and earlier versions.
  - Created endian-aware binary IO routines.
  - Added binary PLY support.
  - Improved test portability by removing non-standard math constants (e.g.,
    M\_PI).
  - Added minimal support for vertex colour and vertex normals to point clouds
    and surface meshes, as well as for OBJ, OFF, and PLY IO (where applicable).
    Still not yet fully supported for all formats.
  - Improved interoperability between point clouds and surface meshe.
  - Added 'standard' 32 bit RGBA packing routine for point clouds and surface
    meshes.

- January 2021
  - PLY: increase portability and prefer signed ints for face vert indices.
  - Added common affine transformation factory functions.
  - Added cast conversions between affine transform and num array classes.

- February 2021
  - Added 3D convex hull extractor. Used an incremental algorithm, but at the
    moment the algorithm cannot be resumed. Scaling is reasonable; at the moment
    vertices do not need to be fully loaded into memory, but random access is
    needed. (Support for non-random access iterators is supported, but only
    recommended for a small number of vertices.)

- June 2021
  - Mutate\_Voxels: handle degenerate contour geometry better.
  - Mutate\_Voxels: provide user access to contour overlap mask.
  - Mutate\_Voxels: provide type alias for user-provided functors.

- August 2021
  - YgorTime: remove FUNCERR, add regex-based stringified date-time parsing,
    and provide fractional seconds, if available.
  - YgorString: add GetAllRegex2 variant that accepts a precompiled regex.
  - PLY: support obj\_info statement.

- September 2021
  - YgorArguments: add extra-safe workaround and reference, and capture local
    copy of key.
  - Arm: avoid clobbering user-provided march, mcpu, and mtune flags.
  - YgorArguments: rewrite argument parser, use feature-test macros specified in
    man page.
  - YgorTime: add minimal test cases for IO.

- October 2021
  - YgorImages: added double-double type instantiations.
  - Planar\_adjacency: added min/max index query.

- March 2022
  - ConvexHull: add eps margin for visibility estimation.
  - Bug fix: replace numeric\_limits::min --> lowest.
  - affine\_transform: make read and write symmetric, and provide a stream input
    constructor.
  - Provide base64-aware key-value pair encoder/decoder routines.
  - Add ::invert() to affine\_transform and up to 4x4 for num\_array.
  - Expand\_Macros: add shell-like bracket support for controlling macro scope.
  
- April 2022
  - simplify\_inner\_triangles: add local non-manifold edge check, consolidate
    warning messages, add a few unit tests, add more stringent checks, reject
    candidate faces with negative orientation, add extra check of face
    reduction, and honour transaction semantics.
  - Provide fallback for git CVE-2022-24765.
  - fv\_surface\_mesh: add flat surface vertex removal.
  - ConvexHull: replace face vector with map, prescan for large seed tet and
    more precisely update face adjacency, and revert to exact visibility
    testing.

- May 2022
  - YgorImages: added note about row-major indexing.
  - YgorTAR: add cstring header for std::memcpy.

- June 2022
  - Added yspan<>, a non-owning span-like container that supports stride with
    arrays of composite objects.
  - FITS images: update support for FITSv4.0 spec, and imbue FITS reader and
    writer with support for arbitrary key-value metadata.
  - FITS images: replaced single-image reader and writer with multi-image
    reader and writer using the FITSv4.0 IMAGE extension.

