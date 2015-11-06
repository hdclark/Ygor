#!/bin/bash

choice="${@}"


echo "NOTE: This script has not been mainted is is lacking many libraries. Touch it up prior to using!"
echo "(Note: using ldd and inspecting the Makefiles in complex projects like operator would be good ways to determine which flagss/libs are required.)"
exit

# We will default to the shared library if nothing else is specified.
if [ "${choice}" = "" ] ; then
    choice="-shared"
fi



# For compiling using the shared library:
if [ "${choice}" = "-shared" ] ; then
    echo   ' -L. -L/usr/lib/ -L/usr/local/lib/ -lygor '
    exit

# For compiling using the static library:
elif [ "${choice}" = "-static" ] ; then
    echo   ' -shared -L. -L/usr/lib/ -L/usr/local/lib/ -lygor -lplot '
    exit

else
    echo "ERROR: Please specify the desired compilation mode!"
fi


