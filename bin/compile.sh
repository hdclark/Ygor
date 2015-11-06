#!/bin/bash

# P-value cruncher.
g++ -std=c++11 TwoT_PValue.cc -o twot_pvalue -lygor &

# Regex-Tester.
g++ -std=c++11 Regex_Tester.cc -o regex_tester -lygor &


# YgorURITools binaries.
g++ -std=c++11 GetRedditUrls.cc            -o getredditurls           -lygor -lhtmlcxx  &
g++ -std=c++11 GetRedditMusic.cc           -o getredditmusic          -lygor -lhtmlcxx  &
g++ -std=c++11 GetRedditMovies.cc          -o getredditmovies         -lygor -lhtmlcxx  &

wait

g++ -std=c++11 GetVideosiftUrls.cc         -o getvideosifturls        -lygor -lhtmlcxx  &
g++ -std=c++11 GetVimeoUrls.cc             -o getvimeourls            -lygor -lhtmlcxx  &
g++ -std=c++11 GetUrls.cc                  -o geturls                 -lygor -lhtmlcxx  &
g++ -std=c++11 GetNutritionfactsUrls.cc    -o getnutritionfactsurls   -lygor -lhtmlcxx  &

wait

