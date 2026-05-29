#!/usr/bin/env bash

set -eu

# Move to the test root.
REPOROOT="$(git rev-parse --show-toplevel || true)"
if [ ! -d "${REPOROOT}" ] ; then

    # Fall-back on the source position of this script.
    SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}" )" )"
    if [ ! -d "${SCRIPT_DIR}" ] ; then
        printf "Cannot access repository root or root directory containing this script. Cannot continue.\n" 1>&2
        exit 1
    fi
    REPOROOT="${SCRIPT_DIR}/../"
fi
cd "${REPOROOT}/tests/"


g++ -std=c++17 Report_Machine_Parameters.cc -o report_machine_parameters -lygor -pthread &
g++ -std=c++17 Test_Algorithms_02.cc -o test_algorithms_02 -lygor -pthread &
wait

g++ -std=c++17 Test_Containers_01.cc -o test_containers_01 -lygor -pthread & 
g++ -std=c++17 Test_Containers_02.cc -o test_containers_02 -lygor -pthread &
g++ -std=c++17 Test_Environment_01.cc -o test_environment_01 -lygor -pthread &
g++ -std=c++17 Test_Environment_02.cc -o test_environment_02 -lygor -pthread &
wait

g++ -std=c++17 Test_FilesDirs_00.cc -o test_filesdirs_00 -lygor -pthread &
g++ -std=c++17 Test_FilesDirs_01.cc -o test_filesdirs_01 -lygor -pthread &
g++ -std=c++17 Test_FilesDirs_02.cc -o test_filesdirs_02 -lygor -pthread &
g++ -std=c++17 Test_FilesDirs_03.cc -o test_filesdirs_03 -lygor -pthread &
g++ -std=c++17 Test_FilesDirs_04.cc -o test_filesdirs_04 -lygor -pthread &
wait

g++ -std=c++17 Test_Images_01.cc -o test_images_01 -lygor -pthread &
g++ -std=c++17 Test_Images_02.cc -o test_images_02 -lygor -pthread -lboost_serialization -lboost_iostreams &
wait

g++ -std=c++17 Test_Math_01.cc -o test_math_01 -lygor -pthread &
g++ -std=c++17 Test_Math_04.cc -o test_math_04 -lygor -pthread &
g++ -std=c++17 Test_Math_05.cc -o test_math_05 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_07.cc -o test_math_07 -lygor -pthread &
g++ -std=c++17 Test_Math_08.cc -o test_math_08 -lygor -pthread &
g++ -std=c++17 Test_Math_09.cc -o test_math_09 -lygor -pthread &
g++ -std=c++17 Test_Math_10.cc -o test_math_10 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_11.cc -o test_math_11 -lygor -pthread -lboost_serialization -lboost_iostreams &
g++ -std=c++17 Test_Math_12.cc -o test_math_12 -lygor -pthread &
g++ -std=c++17 Test_Math_13.cc -o test_math_13 -lygor -pthread &
g++ -std=c++17 Test_Math_15.cc -o test_math_15 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_16.cc -o test_math_16 -lygor -pthread &
g++ -std=c++17 Test_Math_19.cc -o test_math_19 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_21.cc -o test_math_21 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_25.cc -o test_math_25 -lygor -pthread &
g++ -std=c++17 Test_Math_26.cc -o test_math_26 -lygor -pthread &
g++ -std=c++17 Test_Math_27.cc -o test_math_27 -lygor -pthread &
wait

g++ -std=c++17 Test_Networking_A_01.cc -o test_networking_a_01 -lygor -pthread &
g++ -std=c++17 Test_Networking_B_01.cc -o test_networking_b_01 -lygor -pthread &
g++ -std=c++17 Test_Networking_02.cc -o test_networking_02 -lygor -pthread &
g++ -std=c++17 Test_Networking_Beacon_03.cc -o test_networking_beacon_03 -lygor -pthread &
g++ -std=c++17 Test_Networking_Radio_03.cc -o test_networking_radio_03 -lygor -pthread &
wait

g++ -std=c++17 Test_Misc_01.cc -o test_misc_01 -lygor -pthread  &
g++ -std=c++17 Test_Noise_01.cc -o test_noise_01 -lygor -pthread &
wait

g++ -std=c++17 Test_TAR_01.cc -o test_tar_01 -lygor -pthread &
g++ -std=c++17 Test_YgorCONFIGTools_01.cc -o test_configtools_01 -lygor -pthread &
g++ -std=c++17 Test_YgorDICOMTools_01.cc -o test_dicomtools_01 -lygor -pthread &
wait

g++ -std=c++17 Test_MeshesBoolean.cc -o test_meshesboolean -lygor -pthread &
wait

