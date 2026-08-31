//YgorFileIndexGPX.h

#ifndef YGOR_FILE_INDEX_GPX_HDR_GRD_H
#define YGOR_FILE_INDEX_GPX_HDR_GRD_H

#include <cstdint>
#include <vector>

#include "YgorFileIndex.h"


file_index_measure File_Index_GPX_Latitude_Extent_Measure();
file_index_measure File_Index_GPX_Longitude_Extent_Measure();
file_index_measure File_Index_GPX_Coverage_Cell_Measure(double cell_width_degrees);

void Register_GPX_File_Index_Measures(sqlite_file_index &index,
                                      double cell_width_degrees);

std::vector<int64_t> GPX_Coverage_Cells_For_Bounds(double min_latitude,
                                                   double max_latitude,
                                                   double min_longitude,
                                                   double max_longitude,
                                                   double cell_width_degrees);

file_index_predicate GPX_Coverage_Predicate(double min_latitude,
                                            double max_latitude,
                                            double min_longitude,
                                            double max_longitude,
                                            double cell_width_degrees);


#endif
