#!/bin/bash

# Step 0 is to figure out which files you'll utimately need. Grepping for #includes helps.
# Or use the preprocessor.

# Step 1 is to cat all needed files together.
cat /home/hal/Dropbox/Project\ -\ Ygor/Ygor{Misc,Math,String,Stats,Images,FilesDirs,Plot,ImagesIO,ImagesPlotting}.h \
    /home/hal/Dropbox/Project\ -\ Ygor/Ygor{Images,Math,Misc,Stats,String,FilesDirs,Plot,ImagesIO,ImagesPlotting}.cc \
    > /tmp/YgorNeeded.h

# Step 2 is to replace all experimental dependencies.
sed -i -e 's/#include <experimental\/optional>/#include <boost\/optional.hpp>/g' \
       -e 's/#include <experimental\/any>/#include <boost\/any.hpp>/g' \
       -e 's/std::experimental::any_cast/boost::any_cast/g' \
       -e 's/std::experimental::any/boost::any/g' \
       -e 's/std::experimental::optional/boost::optional/g' \
       -e 's/std::experimental::make_optional/boost::make_optional/g' \
    /tmp/YgorNeeded.h

# Step 3 is to replace or remove, wholesale, any external dependencies.
# Note that some libraries may never be needed if the code is never referenced...

cat '/home/hal/Dropbox/Project - Ygor/External/MD5/md5.h' \
    '/home/hal/Dropbox/Project - Ygor/External/MD5/md5.cc' \
    /tmp/YgorNeeded.h \
   > /tmp/dummy
mv /tmp/dummy /tmp/YgorNeeded.h

sed -i -e 's/#include "External\/MD5\/md5.h"/\/\//g' \
       -e 's/#include "md5.h"/\/\//g' \
    /tmp/YgorNeeded.h

# Step 4 is to remove all the #include "Ygor..." directives.
sed -i -e '/#include "Ygor/d' \
    /tmp/YgorNeeded.h

# Step 5 is to add template specialization disabling macros.
cat <(printf "#define YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS 1\n") \
    <(printf "#define YGORFILESDIRS_DISABLE_ALL_SPECIALIZATIONS 1\n") \
    <(printf "#define YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS 1\n") \
    <(printf "#define YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS 1\n") \
    <(printf "#define YGORMATH_DISABLE_ALL_SPECIALIZATIONS 1\n") \
    <(printf "#define YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS 1\n") \
    <(printf "#define YGORSTATS_DISABLE_ALL_SPECIALIZATIONS 1\n") \
    <(printf "#define YGORSTRING_DISABLE_ALL_SPECIALIZATIONS 1\n") \
    /tmp/YgorNeeded.h \
   > /tmp/dummy
mv /tmp/dummy /tmp/YgorNeeded.h

