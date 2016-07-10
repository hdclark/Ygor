# Makefile for Project - Ygor.
# Written by hal clark during [2011-2013] with enhancements and additions afterward.

# NOTE: To reduce linker errors on Raspberry Pi:
#    1) make a swap.
#    2) -Wl,--no-keep-memory,--reduce-memory-overheads
#    3) --param ggc-min-expand=0 --param ggc-min-heapsize=8192

XTRA_FLAGS   = 
XTRA_DEFINES = -D_FORTIFY_SOURCE=2  -fstack-check -fstack-protector-all -I/usr/local/include/

XTRA_LIBS    = -L/usr/local/lib/ 
#XTRA_MISC    = -lm 

LINK         = ar rcs

DEBUGSYM     = -g3 -ggdb # Awaiting gcc v.4.9: -fdiagnostics-color=auto

# `-flto -fuse-linker-plugin` will enable the link-time optimizer. It is quite cool and robust,
#                             but can be slow as hell, and substantially add to compile time.
OPTIMIZATION = -O2 #-flto -fuse-linker-plugin #-fast-math -funsafe-loop-optimizations -flto
#WARNINGS     = -Wall -Wextra -Wno-unused-parameter -Wconversion -Wformat=2 -Wformat-security -Wstrict-overflow -pedantic -Woverloaded-virtual -Wreorder -Wsign-promo -Wnon-virtual-dtor #-Wsuggest-attribute=const -Wsuggest-attribute=pure
WARNINGS     = -fdiagnostics-color=always 

USECC           = `type ccache &>/dev/null && echo -n 'ccache '` g++ -std=c++14 ${DEBUGSYM} ${OPTIMIZATION} ${WARNINGS} 
#USECC           = g++      -std=c++1y ${DEBUGSYM} ${OPTIMIZATION} ${WARNINGS}
#USECC           = clang++  -std=c++1y ${DEBUGSYM} ${OPTIMIZATION} ${WARNINGS}

# Use the environmental compiler. Enable this when using clang's static analyzer (scan-build -V make -j 8).
#USECC           = $(CXX) -std=c++1y $(CXXFLAGS) $(CPPFLAGS)

.PHONY: all

#all: libygor.so  libygor.a
all: libygor.so



########################################################################
#####                        Binary Targets                        #####
########################################################################

YgorContainers.o: YgorContainers.h YgorContainers.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorContainers.cc -Wno-strict-aliasing

YgorAlgorithms.o: YgorAlgorithms.h YgorAlgorithms.cc spookyhash.o
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorAlgorithms.cc  #-lm

YgorEnvironment.o: YgorEnvironment.h YgorEnvironment.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorEnvironment.cc

YgorImagesIO.o: YgorImages.h YgorImagesIO.h YgorImagesIO.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorImagesIO.cc  

YgorImages.o: YgorImages.h YgorImages.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorImages.cc  

YgorMath.o: YgorMath.h  YgorMath.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorMath.cc #-lm 

YgorMath_Samples.o: YgorMath_Samples.h  YgorMath_Samples.cc
	${USECC} ${XTRA_DEFINES} -fPIC -fno-var-tracking-assignments -c YgorMath_Samples.cc  

YgorMathChebyshev.o: YgorMathChebyshev.h YgorMathChebyshev.cc
	${USECC} ${XTRA_DEFINES} -fPIC -fno-var-tracking-assignments -c YgorMathChebyshev.cc  

YgorMathChebyshevFunctions.o: YgorMathChebyshevFunctions.h YgorMathChebyshevFunctions.cc
	${USECC} ${XTRA_DEFINES} -fPIC -fno-var-tracking-assignments -c YgorMathChebyshevFunctions.cc  

YgorPlot.o: YgorPlot.h  YgorPlot.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorPlot.cc  #-lplot

YgorSerialize.o: YgorSerialize.h YgorSerialize.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorSerialize.cc

YgorStats.o:  YgorStats.h  YgorStats.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorStats.cc  #-lgsl -lgslcblas

YgorString.o:  YgorString.h  YgorString.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorString.cc 

YgorFilesDirs.o: YgorFilesDirs.h YgorFilesDirs.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorFilesDirs.cc

YgorPerformance.o: YgorPerformance.h  YgorPerformance.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorPerformance.cc

YgorNoise.o: YgorNoise.h  YgorNoise.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorNoise.cc  

YgorNetworking.o: YgorNetworking.h  YgorNetworking.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorNetworking.cc  

YgorTime.o: YgorTime.h YgorTime.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorTime.cc

YgorDICOMTools.o: YgorDICOMTools.h YgorDICOMTools.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorDICOMTools.cc

YgorCONFIGTools.o: YgorCONFIGTools.h YgorCONFIGTools.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorCONFIGTools.cc

YgorURITools.o: YgorURITools.h YgorURITools.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorURITools.cc #-lhtmlcxx

YgorVIDEOTools.o: YgorVIDEOTools.h YgorVIDEOTools.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c YgorVIDEOTools.cc 

# External compilations.
spookyhash.o: External/SpookyHash/SpookyV2.cpp External/SpookyHash/SpookyV2.h
	${USECC} ${XTRA_DEFINES} -fPIC -c External/SpookyHash/SpookyV2.cpp -o spookyhash.o

md5.o: External/MD5/md5.cc
	${USECC} ${XTRA_DEFINES} -fPIC -c External/MD5/md5.cc -o md5.o

# Easiest solution: cram everything into a single library. This drags in a 
#  lot of extra crap. Prune if it will matter.
libygor.so: YgorMisc.h YgorContainers.o YgorAlgorithms.o YgorEnvironment.o YgorMath.o \
               YgorMathChebyshev.o YgorMathChebyshevFunctions.o \
               YgorArguments.h YgorMath_Samples.o YgorTime.o YgorImages.o YgorImagesIO.o \
               YgorSerialize.o          \
               YgorPlot.o YgorString.o YgorStats.o YgorFilesDirs.o YgorPerformance.o YgorNoise.o     \
               YgorNetworking.o YgorDICOMTools.o YgorCONFIGTools.o YgorURITools.o \
               YgorVIDEOTools.o spookyhash.o md5.o  
	${USECC} ${XTRA_DEFINES} -rdynamic -shared -Wl,-soname,libygor.so -o libygor.so   -Wl,-z,now -Wl,-z,noexecstack  \
               YgorMathChebyshev.o YgorMathChebyshevFunctions.o \
               YgorContainers.o YgorAlgorithms.o YgorEnvironment.o YgorMath.o \
               YgorMath_Samples.o YgorTime.o YgorImages.o YgorImagesIO.o \
               YgorSerialize.o                     \
               YgorPlot.o YgorString.o YgorStats.o YgorFilesDirs.o YgorPerformance.o YgorNoise.o     \
               YgorNetworking.o YgorDICOMTools.o YgorCONFIGTools.o YgorURITools.o \
               YgorVIDEOTools.o spookyhash.o md5.o                                    \
               ${XTRA_LIBS} -lhtmlcxx -lgsl -lgslcblas -lm

################### -lregex
################### -lplot  
################### -Wl,-pic -Wl,-z,noexecstack -Wl,-z,noexecheap -Wl,-z,relro -Wl,-z,now \

########################################################################
#####                       Install Targets                        #####
########################################################################
install: all
	install -Dm644 libygor.so                       /usr/lib/
	ldconfig -n                                     /usr/lib/       #Use "-R" instead of "-n" for FreeBSD.
	install -Dm644 YgorContainers.h                 /usr/include/
	install -Dm644 YgorAlgorithms.h                 /usr/include/
	install -Dm644 YgorArguments.h                  /usr/include/   #Header only!
	install -Dm644 YgorEnvironment.h                /usr/include/
	install -Dm644 YgorMisc.h                       /usr/include/
	install -Dm644 YgorImages.h                     /usr/include/
	install -Dm644 YgorImagesIO.h                   /usr/include/
	install -Dm644 YgorImagesPlotting.h             /usr/include/
	install -Dm644 YgorMath.h                       /usr/include/
	install -Dm644 YgorMathChebyshev.h              /usr/include/
	install -Dm644 YgorMathChebyshevFunctions.h     /usr/include/
	install -Dm644 YgorMathPlottingGnuplot.h        /usr/include/
	install -Dm644 YgorMathPlottingVTK.h            /usr/include/
	install -Dm644 YgorMathIOBoostSerialization.h   /usr/include/
	install -Dm644 YgorMathChebyshevIOBoostSerialization.h  /usr/include/
	install -Dm644 YgorImagesIOBoostSerialization.h /usr/include/
	install -Dm644 YgorMath_Samples.h               /usr/include/
	install -Dm644 YgorPlot.h                       /usr/include/
	install -Dm644 YgorSerialize.h                  /usr/include/
	install -Dm644 YgorStats.h                      /usr/include/
	install -Dm644 YgorString.h                     /usr/include/
	install -Dm644 YgorFilesDirs.h                  /usr/include/
	install -Dm644 YgorPerformance.h                /usr/include/
	install -Dm644 YgorNoise.h                      /usr/include/
	install -Dm644 YgorNetworking.h                 /usr/include/
	install -Dm644 YgorTime.h                       /usr/include/
	install -Dm644 YgorDICOMTools.h                 /usr/include/
	install -Dm644 YgorCONFIGTools.h                /usr/include/
	install -Dm644 YgorURITools.h                   /usr/include/
	install -Dm644 YgorVIDEOTools.h                 /usr/include/
	install -Dm777 Ygor_Compiler_Flags.sh           /usr/bin/


########################################################################
#####                      Cleaning Targets                        #####
########################################################################
clean: 
	rm libygor.so libygor.a YgorContainers.o YgorAlgorithms.o \
           YgorEnvironment.o YgorMath.o YgorPlot.o YgorString.o YgorStats.o YgorFilesDirs.o        \
           YgorMathChebyshev.o YgorMathChebyshevFunctions.o        \
           YgorMath_Samples.o YgorTime.o YgorImages.o YgorImagesIO.o         \
           YgorSerialize.o           \
           YgorPerformance.o YgorNoise.o YgorNetworking.o YgorDICOMTools.o             \
           YgorCONFIGTools.o YgorURITools.o YgorVIDEOTools.o \
	   spookyhash.o md5.o           &>/dev/null || true


