//YgorVIDEOTools.h - A collection of routines for working with video and video files.

#ifndef VIDEOTOOLS_HDR_GRD_H_
#define VIDEOTOOLS_HDR_GRD_H_

#include <string>
#include <utility>

std::pair<long int, long int> YgorVIDEOTools_Get_Video_Dimensions(const std::string &filename); //W, H. Both are -1 if error is encountered.


#endif
