cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(YgorComponent07RerunHarness LANGUAGES CXX)
include(CTest)

execute_process(
    COMMAND bash "${CMAKE_SOURCE_DIR}/.github/component07-rerun-harness.sh" "${CMAKE_BINARY_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE COMPONENT07_HARNESS_RESULT
)

message(FATAL_ERROR
    "Component 07 rerun harness completed with retained-artifact status ${COMPONENT07_HARNESS_RESULT}")
