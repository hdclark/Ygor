#!/bin/bash

# This script will (attempt) to migrate source code from "Project - Utilities" to
# "Project - Ygor" naming conventions.
#
# Note: should be run like
#   find ./ -type f -exec /home/hal/Dropbox/Project\ -\ Ygor/Migrate.sh '{}' \+
#
echo "This script will alter code directly. Ensure you have made a backup copy."
echo "Press [enter] if willing to proceed"
read dummy



SUBSTITUTIONS+=' -e s/CONFIGTools/YgorCONFIGTools/g '
SUBSTITUTIONS+=' -e s/DICOMTools/YgorDICOMTools/g '
SUBSTITUTIONS+=' -e s/URITools/YgorURITools/g '
SUBSTITUTIONS+=' -e s/VIDEOTools/YgorVIDEOTools/g '

SUBSTITUTIONS+=' -e s/MyAlgorithms/YgorAlgorithms/g '
SUBSTITUTIONS+=' -e s/MYALGORITHMS/YGORALGORITHMS/g '
SUBSTITUTIONS+=' -e s/MyArguments/YgorArguments/g '
SUBSTITUTIONS+=' -e s/MYARGUMENTS/YGORARGUMENTS/g '
SUBSTITUTIONS+=' -e s/MyContainers/YgorContainers/g '
SUBSTITUTIONS+=' -e s/MYCONTAINERS/YGORCONTAINERS/g '
SUBSTITUTIONS+=' -e s/MyEnvironment/YgorEnvironment/g '
SUBSTITUTIONS+=' -e s/MYENVIRONMENT/YGORENVIRONMENT/g '
SUBSTITUTIONS+=' -e s/MyFilesDirs/YgorFilesDirs/g '
SUBSTITUTIONS+=' -e s/MYFILESDIRS/YGORFILESDIRS/g '
SUBSTITUTIONS+=' -e s/MyImages/YgorImages/g '
SUBSTITUTIONS+=' -e s/MyMath/YgorMath/g '
SUBSTITUTIONS+=' -e s/MyMath_Samples/YgorMath_Samples/g '
SUBSTITUTIONS+=' -e s/MyMisc/YgorMisc/g '
SUBSTITUTIONS+=' -e s/MyNetworking/YgorNetworking/g '
SUBSTITUTIONS+=' -e s/MYNETWORKING/YGORNETWORKING/g '
SUBSTITUTIONS+=' -e s/MyNoise/YgorNoise/g '
SUBSTITUTIONS+=' -e s/MyPerformance/YgorPerformance/g '
SUBSTITUTIONS+=' -e s/MyPlot/YgorPlot/g '
SUBSTITUTIONS+=' -e s/MyString/YgorString/g '
SUBSTITUTIONS+=' -e s/MyTime/YgorTime/g '

SUBSTITUTIONS+=' -e s/MYMISCABS/YGORABS/g '
SUBSTITUTIONS+=' -e s/MYMISCMIN/YGORMIN/g '
SUBSTITUTIONS+=' -e s/MYMISCMAX/YGORMAX/g '

SUBSTITUTIONS+=' -e s/MYPERF_TIMESTAMP_DT/YGORPERF_TIMESTAMP_DT/g '
#SUBSTITUTIONS+=' -e s/MY\([_a-zA-Z]*\)/YGOR\1/g    '   # Too broad!
#SUBSTITUTIONS+=' -e s/my\([_a-zA-Z]*\)/Ygor\1/g    '   # Too broad!

SUBSTITUTIONS+=' -e s/libmyutils/libygor/g    '
SUBSTITUTIONS+=' -e s/myutils/ygor/g    '
SUBSTITUTIONS+=' -e s/MYUTILS/YGOR/g    '

SUBSTITUTIONS+=' -e s/YgorYgor/Ygor/g    '
SUBSTITUTIONS+=' -e s/ygorygor/ygor/g    '
SUBSTITUTIONS+=' -e s/YGORYGOR/YGOR/g    '


while [ "$#" != "0" ] ; do
    thefile="$1"
    if [ -f "${thefile}" ] ; then
        echo "--(I) Working on file '$thefile'"

        #First, alter the contents.
        sed -i $SUBSTITUTIONS  "${thefile}"

        sed -i -e 's/YgorYgor/Ygor/g'  -e 's/ygorygor/ygor/g'  -e 's/YGORYGOR/YGOR/g' "${thefile}"
        sed -i -e 's/YgorYgor/Ygor/g'  -e 's/ygorygor/ygor/g'  -e 's/YGORYGOR/YGOR/g' "${thefile}"
        sed -i -e 's/YgorYgor/Ygor/g'  -e 's/ygorygor/ygor/g'  -e 's/YGORYGOR/YGOR/g' "${thefile}"
        sed -i -e 's/YgorYgor/Ygor/g'  -e 's/ygorygor/ygor/g'  -e 's/YGORYGOR/YGOR/g' "${thefile}"
        sed -i -e 's/YgorYgor/Ygor/g'  -e 's/ygorygor/ygor/g'  -e 's/YGORYGOR/YGOR/g' "${thefile}"


        #Now move the file.
        NEWNAME=$( echo "${thefile}" | sed $SUBSTITUTIONS )
        if [ "${thefile}" != "${NEWNAME}" ] ; then 
            echo "Moving file from '${thefile}' to '${NEWNAME}'. Confirm with 'y'."
            read choice
            if [ "${choice}" == "y" ] && [ ! -f "${NEWNAME}" ] ; then
                mv "${thefile}" "${NEWNAME}"
            fi
        fi


    else
        echo "--(E) Argument '$thefile' could not be read!"
    fi
    shift
done

exit

