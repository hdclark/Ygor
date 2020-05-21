#!/bin/bash

# This script plots a Ygor contour_of_points that has been stringified and written to a file.

cat "$@" |
  sed -e 's/[()]/\n/g' |
  grep ',*,*,' |
  sed -e 's/, / /g' |
  gnuplot -persist -e '
      splot sin(x) ;
      pause 1 ;
      splot "-" u 1:2:3 w lp ;

      pause 1 ;
      bind all "q" "unset output ; exit gnuplot" ;
      while(1) {
          pause 1 ;
      };
  '

