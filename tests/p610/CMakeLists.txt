cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(YgorMeshBooleanP610Contracts LANGUAGES CXX)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(YGOR_BUILD_BOOLEAN_TESTS OFF CACHE BOOL "" FORCE)
set(YGOR_BUILD_BOOLEAN_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(WITH_EIGEN OFF CACHE BOOL "" FORCE)
set(WITH_GNU_GSL OFF CACHE BOOL "" FORCE)
set(MEMORY_CONSTRAINED_BUILD ON CACHE BOOL "" FORCE)
find_package(Threads REQUIRED)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. ygor)

add_library(ygor_mesh_boolean_p610_support STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../../src/YgorMeshesBooleanContract.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../src/YgorMeshesBooleanPerformance.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../src/YgorMeshesBooleanProductContract.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../src/YgorMeshesBooleanQualification.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../src/YgorMeshesBooleanQualificationCandidate.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../src/YgorMeshesBooleanQualificationCandidateRunner.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../src/YgorMeshesBooleanQualificationMatrix.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../src/External/MD5/md5.cc)
target_include_directories(ygor_mesh_boolean_p610_support PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../src
    ${CMAKE_CURRENT_BINARY_DIR}/ygor/src)
target_compile_features(ygor_mesh_boolean_p610_support PRIVATE cxx_std_17)
target_link_libraries(ygor_mesh_boolean_p610_support PRIVATE Threads::Threads m)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ygor_mesh_boolean_p610_support PRIVATE
        -fno-fast-math -ffp-contract=off -ffunction-sections -fdata-sections)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ygor_mesh_boolean_p610_support PRIVATE -frounding-math)
endif()

add_executable(Test_MeshesBooleanQualificationCandidate
    ${CMAKE_CURRENT_LIST_DIR}/../Test_MeshesBooleanQualificationCandidate.cc
    ${CMAKE_CURRENT_LIST_DIR}/../MeshBooleanTestHarness.cc
    ${CMAKE_CURRENT_LIST_DIR}/../MeshBooleanTestConfig.cc)
target_include_directories(Test_MeshesBooleanQualificationCandidate PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../../src
    ${CMAKE_CURRENT_BINARY_DIR}/ygor/src)
target_link_libraries(Test_MeshesBooleanQualificationCandidate PRIVATE
    ygor_mesh_boolean_p610_support Threads::Threads m)
target_compile_features(Test_MeshesBooleanQualificationCandidate PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Test_MeshesBooleanQualificationCandidate PRIVATE
        -fno-fast-math -frounding-math -ffp-contract=off -ffunction-sections -fdata-sections -Werror)
    target_link_options(Test_MeshesBooleanQualificationCandidate PRIVATE -Wl,--gc-sections)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Test_MeshesBooleanQualificationCandidate PRIVATE
        -fno-fast-math -ffp-contract=off -ffunction-sections -fdata-sections -Werror)
    target_link_options(Test_MeshesBooleanQualificationCandidate PRIVATE -Wl,--gc-sections)
endif()

enable_testing()
add_test(NAME MeshBoolean.QualificationCandidate
    COMMAND Test_MeshesBooleanQualificationCandidate)
set_tests_properties(MeshBoolean.QualificationCandidate PROPERTIES
    TIMEOUT 900
    LABELS "mesh_boolean;qualification;p6;p6.10;continuous")
