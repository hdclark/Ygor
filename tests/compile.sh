#!/bin/bash

g++ -std=c++17 Report_Machine_Parameters.cc -o report_machine_parameters -lygor -pthread &
g++ -std=c++17 Test_Algorithms_01.cc -o test_algorithms_01 -lygor -pthread &
g++ -std=c++17 Test_Algorithms_02.cc -o test_algorithms_02 -lygor -pthread &
g++ -std=c++17 Test_Algorithms_03.cc -o test_algorithms_03 -lygor -pthread &
wait

g++ -std=c++17 Test_Arguments_01.cc -o test_arguments_01 -lygor -pthread &
g++ -std=c++17 Test_Base64_01.cc -o test_base64_01 -lygor -pthread &
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
g++ -std=c++17 Test_Math_02.cc -o test_math_02 -lygor -pthread &
g++ -std=c++17 Test_Math_03.cc -o test_math_03 -lygor -pthread &
g++ -std=c++17 Test_Math_04.cc -o test_math_04 -lygor -pthread &
g++ -std=c++17 Test_Math_05.cc -o test_math_05 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_06.cc -o test_math_06 -lygor -pthread &
g++ -std=c++17 Test_Math_07.cc -o test_math_07 -lygor -pthread &
g++ -std=c++17 Test_Math_08.cc -o test_math_08 -lygor -pthread &
g++ -std=c++17 Test_Math_09.cc -o test_math_09 -lygor -pthread &
g++ -std=c++17 Test_Math_10.cc -o test_math_10 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_11.cc -o test_math_11 -lygor -pthread -lboost_serialization -lboost_iostreams &
g++ -std=c++17 Test_Math_12.cc -o test_math_12 -lygor -pthread &
g++ -std=c++17 Test_Math_13.cc -o test_math_13 -lygor -pthread &
g++ -std=c++17 Test_Math_14.cc -o test_math_14 -lygor -pthread &
g++ -std=c++17 Test_Math_15.cc -o test_math_15 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_16.cc -o test_math_16 -lygor -pthread &
g++ -std=c++17 Test_Math_17.cc -o test_math_17 -lygor -pthread &
g++ -std=c++17 Test_Math_18.cc -o test_math_18 -lygor -pthread &
g++ -std=c++17 Test_Math_19.cc -o test_math_19 -lygor -pthread &
g++ -std=c++17 Test_Math_20.cc -o test_math_20 -lygor -pthread &
wait

g++ -std=c++17 Test_Math_21.cc -o test_math_21 -lygor -pthread &
g++ -std=c++17 Test_Math_22.cc -o test_math_22 -lygor -pthread &
g++ -std=c++17 Test_Math_23.cc -o test_math_23 -lygor -pthread &
g++ -std=c++17 Test_Math_24.cc -o test_math_24 -lygor -pthread &
g++ -std=c++17 Test_Math_25.cc -o test_math_25 -lygor -pthread &
g++ -std=c++17 Test_Math_26.cc -o test_math_26 -lygor -pthread &
wait

g++ -std=c++17 Test_Networking_A_01.cc -o test_networking_a_01 -lygor -pthread &
g++ -std=c++17 Test_Networking_B_01.cc -o test_networking_b_01 -lygor -pthread &
g++ -std=c++17 Test_Networking_02.cc -o test_networking_02 -lygor -pthread &
g++ -std=c++17 Test_Networking_Beacon_03.cc -o test_networking_beacon_03 -lygor -pthread &
g++ -std=c++17 Test_Networking_Radio_03.cc -o test_networking_radio_03 -lygor -pthread &
wait

g++ -std=c++17 Test_Misc_01.cc -o test_misc_01 -lygor -pthread  &
g++ -std=c++17 Test_Noise_01.cc -o test_noise_01 -lygor -pthread &
g++ -std=c++17 Test_Noise_02.cc -o test_noise_02 -lygor -pthread &
g++ -std=c++17 Test_Serialization_01.cc -o test_serialization_01 -lygor -pthread &
wait

g++ -std=c++17 Test_Stats_01.cc -o test_stats_01 -lygor -lgsl -lgslcblas -pthread&
g++ -std=c++17 Test_Stats_02.cc -o test_stats_02  -lygor -pthread &
g++ -std=c++17 Test_Stats_03.cc -o test_stats_03  -lygor -pthread &
g++ -std=c++17 Test_Stats_04.cc -o test_stats_04  -lygor -pthread &
wait

g++ -std=c++17 Test_String_01.cc -o test_string_01 -lygor -pthread &
g++ -std=c++17 Test_YgorCONFIGTools_01.cc -o test_configtools_01 -lygor -pthread &
g++ -std=c++17 Test_YgorDICOMTools_01.cc -o test_dicomtools_01 -lygor -pthread &
wait


