cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(YgorMeshBooleanP69Contracts LANGUAGES CXX)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(YGOR_BUILD_BOOLEAN_TESTS OFF CACHE BOOL "" FORCE)
set(YGOR_BUILD_BOOLEAN_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(WITH_EIGEN OFF CACHE BOOL "" FORCE)
set(WITH_GNU_GSL OFF CACHE BOOL "" FORCE)
set(MEMORY_CONSTRAINED_BUILD ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. ygor)

add_executable(Test_MeshesBooleanQualificationPerformance
    ${CMAKE_CURRENT_LIST_DIR}/../Test_MeshesBooleanQualificationPerformance.cc
    ${CMAKE_CURRENT_LIST_DIR}/../MeshBooleanTestHarness.cc
    ${CMAKE_CURRENT_LIST_DIR}/../MeshBooleanTestConfig.cc)
target_include_directories(Test_MeshesBooleanQualificationPerformance PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../../src
    ${CMAKE_CURRENT_BINARY_DIR}/ygor/src)
target_link_libraries(Test_MeshesBooleanQualificationPerformance PRIVATE ygor Threads::Threads)
target_compile_features(Test_MeshesBooleanQualificationPerformance PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Test_MeshesBooleanQualificationPerformance PRIVATE
        -fno-fast-math -frounding-math -ffp-contract=off -Werror)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Test_MeshesBooleanQualificationPerformance PRIVATE
        -fno-fast-math -ffp-contract=off -Werror)
endif()

enable_testing()
add_test(NAME MeshBoolean.QualificationPerformance
    COMMAND Test_MeshesBooleanQualificationPerformance)
set_tests_properties(MeshBoolean.QualificationPerformance PROPERTIES
    TIMEOUT 900
    LABELS "mesh_boolean;qualification;p6;p6.9;continuous")
