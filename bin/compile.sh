#!/bin/bash

# P-value cruncher.
g++ -std=c++14 TwoT_PValue.cc -o twot_pvalue -lygor -pthread &

# Regex-Tester.
g++ -std=c++14 Regex_Tester.cc -o regex_tester -lygor -pthread &

wait
exit


# YgorURITools binaries.
g++ -std=c++14 GetRedditUrls.cc            -o getredditurls           -lygor -lhtmlcxx  &
g++ -std=c++14 GetRedditMusic.cc           -o getredditmusic          -lygor -lhtmlcxx  &
g++ -std=c++14 GetRedditMovies.cc          -o getredditmovies         -lygor -lhtmlcxx  &

wait

g++ -std=c++14 GetVideosiftUrls.cc         -o getvideosifturls        -lygor -lhtmlcxx  &
g++ -std=c++14 GetVimeoUrls.cc             -o getvimeourls            -lygor -lhtmlcxx  &
g++ -std=c++14 GetUrls.cc                  -o geturls                 -lygor -lhtmlcxx  &
g++ -std=c++14 GetNutritionfactsUrls.cc    -o getnutritionfactsurls   -lygor -lhtmlcxx  &

wait

