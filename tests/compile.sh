#!/bin/bash

g++ -std=c++14 Report_Machine_Parameters.cc -o report_machine_parameters -lygor -pthread &
g++ -std=c++14 Test_Algorithms.cc -o test_algorithms -lygor -pthread &
g++ -std=c++14 Test_Algorithms2.cc -o test_algorithms2 -lygor -pthread &
g++ -std=c++14 Test_Algorithms3.cc -o test_algorithms3 -lygor -pthread &
wait

g++ -std=c++14 Test_Arguments.cc -o test_arguments -lygor -pthread &
g++ -std=c++14 Test_Containers.cc -o test_containers -lygor -pthread & 
g++ -std=c++14 Test_Containers2.cc -o test_containers2 -lygor -pthread &
g++ -std=c++14 Test_Environment.cc -o test_environment -lygor -pthread &
g++ -std=c++14 Test_Environment2.cc -o test_environment2 -lygor -pthread &
wait

g++ -std=c++14 Test_FilesDirs.cc -o test_filesdirs -lygor -pthread &
g++ -std=c++14 Test_FilesDirs1.cc -o test_filesdirs1 -lygor -pthread &
g++ -std=c++14 Test_FilesDirs2.cc -o test_filesdirs2 -lygor -pthread &
g++ -std=c++14 Test_FilesDirs3.cc -o test_filesdirs3 -lygor -pthread &
g++ -std=c++14 Test_FilesDirs4.cc -o test_filesdirs4 -lygor -pthread &
wait

g++ -std=c++14 Test_Images.cc -o test_images -lygor -pthread &
g++ -std=c++14 Test_Images2.cc -o test_images2 -lygor -pthread -lboost_serialization -lboost_iostreams &
wait

g++ -std=c++14 Test_Math.cc -o test_math -lygor -pthread &
g++ -std=c++14 Test_Math2.cc -o test_math2 -lygor -pthread &
g++ -std=c++14 Test_Math3.cc -o test_math3 -lygor -pthread &
g++ -std=c++14 Test_Math4.cc -o test_math4 -lygor -pthread &
g++ -std=c++14 Test_Math5.cc -o test_math5 -lygor -pthread &
wait

g++ -std=c++14 Test_Math6.cc -o test_math6 -lygor -pthread &
g++ -std=c++14 Test_Math7.cc -o test_math7 -lygor -pthread &
g++ -std=c++14 Test_Math8.cc -o test_math8 -lygor -pthread &
g++ -std=c++14 Test_Math9.cc -o test_math9 -lygor -pthread &
g++ -std=c++14 Test_Math10.cc -o test_math10 -lygor -pthread &
wait

g++ -std=c++14 Test_Math11.cc -o test_math11 -lygor -pthread -lboost_serialization -lboost_iostreams &
g++ -std=c++14 Test_Math12.cc -o test_math12 -lygor -pthread &
g++ -std=c++14 Test_Math13.cc -o test_math13 -lygor -pthread &
g++ -std=c++14 Test_Math14.cc -o test_math14 -lygor -pthread &
wait

g++ -std=c++14 Test_Math15.cc -o test_math15 -lygor -pthread &
g++ -std=c++14 Test_Math16.cc -o test_math16 -lygor -pthread &
g++ -std=c++14 Test_Math17.cc -o test_math17 -lygor -pthread &
g++ -std=c++14 Test_Math18.cc -o test_math18 -lygor -pthread &
g++ -std=c++14 Test_Math19.cc -o test_math19 -lygor -pthread &
wait

g++ -std=c++14 Test_Misc.cc -o test_misc -lygor -pthread  &
g++ -std=c++14 Test_Networking_2.cc -o test_networking_2 -lygor -pthread &
g++ -std=c++14 Test_Networking_Beacon.cc -o test_networking_beacon -lygor -pthread &
g++ -std=c++14 Test_Networking_Radio.cc -o test_networking_radio -lygor -pthread &
wait

g++ -std=c++14 Test_Noise.cc -o test_noise -lygor -pthread &
g++ -std=c++14 Test_Noise2.cc -o test_noise2 -lygor -pthread &
g++ -std=c++14 Test_Serialization.cc -o test_serialization -lygor -pthread &
wait

g++ -std=c++14 Test_Stats.cc -o test_stats -lygor -lgsl -lgslcblas -pthread&
g++ -std=c++14 Test_Stats2.cc -o test_stats2  -lygor -pthread &
g++ -std=c++14 Test_Stats3.cc -o test_stats3  -lygor -pthread &
wait

g++ -std=c++14 Test_String.cc -o test_string -lygor -pthread &
g++ -std=c++14 Test_Video.cc -o test_video -lygor -pthread &
g++ -std=c++14 Test_YgorCONFIGTools.cc -o test_configtools -lygor -pthread &
g++ -std=c++14 Test_YgorDICOMTools.cc -o test_dicomtools -lygor -pthread &
wait


