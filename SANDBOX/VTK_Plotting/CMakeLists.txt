cmake_minimum_required(VERSION 2.8)
 
PROJECT(LinePlot)
 
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})
 
add_executable(LinePlot MACOSX_BUNDLE LinePlot)
 
if(VTK_LIBRARIES)
  target_link_libraries(LinePlot ${VTK_LIBRARIES})
else()
  target_link_libraries(LinePlot vtkHybrid vtkWidgets)
endif()
