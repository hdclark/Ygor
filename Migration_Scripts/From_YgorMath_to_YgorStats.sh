#!/bin/sh

# Inclusion of some common functions.
if [ -f "/home/hal/Dropbox/Code/Common_Bash_Funcs.sh" ] ; then  source /home/hal/Dropbox/Code/Common_Bash_Funcs.sh ;
elif [ -f "Common_Bash_Funcs.sh" ] ; then  source Common_Bash_Funcs.sh ;
else echo "Unable to include common bash functions. Did the file move?" ; exit ; fi

# This script will edit source files to migrate to YgorStats.
# Ensure a backup is made prior to running. This script might destroy stuff!

if [ "$#" == "0" ] ; then
    FUNCWARN "A filename or filenames are required for processing."
    FUNCINFO "This script will attempt to auto-migrate code which uses Ygor. See source"
    exit
fi

# Make a worst-case-scenario backup directory.
BACKUPDIR="/tmp/Migration_From_YgorMath_to_YgorStats/"
mkdir -p "${BACKUPDIR}"


# Cycle over the filenames.
while [ "$#" != "0" ] ; do
    thefilename="$1"
    shift

    FUNCINFO "Processing file '$thefilename' now"

    # Ensure the file exists and can be reached.
    if [ ! -f "$thefilename" ] ; then
        FUNCWARN "Cannot access file '$thefilename'. Skipping it"
        continue
    fi

    # Make a backup copy for good measure.
    cp "$thefilename" "${BACKUPDIR}`get_files_full_base ${thefilename}`_`current_date_time`" 

    ##########################################################################
    # Perform the in-place migration.
    ##########################################################################

    PREMD5SUM=$(md5sum "$thefilename")
    # Func name shortening and namespace introduction.
    sed -i -e 's/PValue_From_StudentT_One_Tail/Stats::P_From_StudT_1Tail/g' \
           -e 's/PValue_From_StudentT_Two_Tail/Stats::P_From_StudT_2Tail/g' \
           -e 's/PValue_From_StudentT_Diff_Means_From_Unequal_Variances/Stats::P_From_StudT_Diff_Means_From_Uneq_Vars/g' \
           -e 's/QValue_From_ChiSquare_Fit/Stats::Q_From_ChiSq_Fit/g' \
           -e 's/Ygor_Unbiased_Estimate_of_Variance/Stats::Unbiased_Var_Est/g' \
           "$thefilename"
    
    POSTMD5SUM=$(md5sum "$thefilename")

    # If no changes occured, this script should not bother doing anything else.
    if [ "$PREMD5SUM" == "$POSTMD5SUM" ] ; then
        FUNCINFO "File '$thefilename' contained no relevant function calls. Not bothering with includes"
        continue
    fi

    # Check for '#include "YgorStats.h"'. Insert after YgorMath.h if none found.
    STATSHEADERINCLUDED=$(grep 'include "YgorStats.h"' "$thefilename")
    MATHHEADERINCLUDED=$(grep 'include "YgorMath.h"' "$thefilename")
    if [ "$STATSHEADERINCLUDED" == "" ] ; then
        FUNCINFO "YgorStats.h not included. Adding after YgorMath.h"
        if [ "$MATHHEADERINCLUDED" == "" ] ; then
            FUNCWARN "Cannot find YgorMath.h include. Add 'include \"YgorStats.h\"' where needed"
            continue
        fi 
   
        sed -i -e 's/[#]include ["]YgorMath[.]h["]/\#include \"YgorMath.h\"\n\#include \"YgorStats.h\"/' \
               "$thefilename"
    fi


    ##########################################################################
done



