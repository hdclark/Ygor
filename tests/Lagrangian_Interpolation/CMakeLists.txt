cmake_minimum_required(VERSION 2.8)
PROJECT(LagrangianInterpolation)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always -fno-var-tracking-assignments")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")



add_executable(lagrangian Lagrange.cc)
target_link_libraries(lagrangian ygor)
if(VTK_LIBRARIES)
  target_link_libraries(lagrangian ${VTK_LIBRARIES})
else()
  target_link_libraries(lagrangian vtkHybrid vtkWidgets)
endif()

