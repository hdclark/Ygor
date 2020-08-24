#!/usr/bin/env bash

# This script plots a Ygor contour_of_points that has been stringified and written to a file.


cat "$@" |
  sed -e 's/[()]/\n/g' |
  grep ',*,*,' |
  sed -e 's/, / /g' |
  gnuplot -persist -e '
      plot sin(x) ;
      pause 1 ;
      plot "-" u 1:2 w lp ;

      pause 1 ;
      bind all "q" "unset output ; exit gnuplot" ;
      bind "Close" "unset output ; exit gnuplot" ;
      while(1) {
          pause 1 ;
      };
  '

