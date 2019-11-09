
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![LOC](https://tokei.rs/b1/gitlab/hdeanclark/Ygor)](https://gitlab.com/hdeanclark/Ygor)
[![Language](https://img.shields.io/github/languages/top/hdclark/Ygor.svg)](https://gitlab.com/hdeanclark/Ygor)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/4b2ac86e6fe446a69891e5d61fb3312a)](https://www.codacy.com/app/hdclark/Ygor?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=hdclark/Ygor&amp;utm_campaign=Badge_Grade)
[![Hit count](http://hits.dwyl.io/hdclark/Ygor.svg)](http://hits.dwyl.io/hdclark/Ygor)

# Introduction

This supporting library was written by hal clark over the span of many years
(~2010-2019) to house bits of code that are useful for multiple projects.

Most, but not all of Ygor's routines are focused on scientific or mathematic
applications. These routines will grow, be replaced, be updated, and may even
disappear when their functionality is superceded by new features in the
language/better libraries/etc. However, many of these routines are not broadly
useful enough for a project like Boost to include, and many are not
comprehensive enough to be submitted to more mature project. The routines in
this library were all developed for specific projects with specific needs, but
which may (have) become useful for other projects.

Pushing (generic) routines from specific projects into these files is
encouraged, so long as the routines has some hope of being applicable in some
other situation.


# License and Copying

All materials herein which may be copywrited, where applicable, are. Copyright
2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 Hal Clark. See
[LICENSE.txt] for details about the license. Informally, Ygor is available under
a GPLv3+ license. 

All liability is herefore disclaimed. The person(s) who use this source and/or
software do so strictly under their own volition. They assume all associated
liability for use and misuse, including but not limited to damages, harm,
injury, and death which may result, including but not limited to that arising
from unforeseen or unanticipated implementation defects.


# Author

The entirety of this project was written by hal clark, unless otherwise noted.
The majority of routines were written with the assistance of websites like
StackOverflow, CppReference, and Bjarne's C++11 FAQ. These resources were
invaluable, and the author thanks those who have made such information available
both free of charge and often with snippets of free-to-use code. Whenever
possible, public domain code has been consulted or used. Insignificant portions
of GPL code have been used verbatim where appropriate.

If a routine existed in a high-quality, widely-available library, it was not
duplicated in Ygor unless including that library was difficult. As of writing,
various bits of Ygor rely on:

- STL/C++17: heavily, throughout; mostly C++11 but some C++14 and C++17.
- Boost: various.
- spookyhash: YgorAlgorithms. Very little - one function at time of writing.
- GNU Scientific Library: YgorStats and YgorMathBasisSplines.
- GNU plotutils: YgorPlot. Weakly - via shell/pipe.
- Gnuplot: YgorPlot. Weakly - via shell/pipe.


# Project Home

The Ygor homepage can be found at [http://www.halclark.ca/]. Source code is
available at [https://gitlab.com/hdeanclark/Ygor/] and
[https://github.com/hdclark/Ygor/].


# Installation

This project uses CMake. Use the usual commands to compile:

     $>  cd /path/to/source/directory
     $>  mkdir build && cd build/

Then, iff by-passing your package manager:

     $>  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
     $>  make && sudo make install

Or, if building for Debian:

     $>  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
     $>  make && make package
     $>  sudo apt install -f ./*.deb

Or, if building for Arch Linux:

     $>  rsync -aC --exclude build ../ ./
     $>  makepkg --syncdeps --noconfirm # Optionally also [--install].


# Known Issues

- The Gnuplot async plotter will cause the main thread to hang on some systems.
  It can be made to block instead, but this is hard-coded at the moment. The
  core issue appears to be communicating via pipes with Gnuplot rather than via
  a proper API.


# Rough Historical Timeline

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

