if(NOT YGOR_BUILD_BOOLEAN_TESTS)
    return()
endif()

add_library(mesh_boolean_test_support STATIC
    MeshBooleanTestHarness.cc
    MeshBooleanTestConfig.cc)
target_link_libraries(mesh_boolean_test_support PUBLIC ygor Threads::Threads)
target_include_directories(mesh_boolean_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(mesh_boolean_test_support PUBLIC cxx_std_17)

function(mesh_boolean_strict_fp target)
    if(YGOR_BOOLEAN_STRICT_FP AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE -fno-fast-math -frounding-math -ffp-contract=off)
        # Keep GCC from linking crtfastmath and changing the process FP mode.
        set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -fno-fast-math")
    elseif(YGOR_BOOLEAN_STRICT_FP AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
endfunction()
mesh_boolean_strict_fp(mesh_boolean_test_support)

add_executable(Test_MeshesBooleanContract Test_MeshesBooleanContract.cc)
target_link_libraries(Test_MeshesBooleanContract PRIVATE ygor Threads::Threads)
mesh_boolean_strict_fp(Test_MeshesBooleanContract)
add_test(NAME MeshesBooleanContract COMMAND Test_MeshesBooleanContract)
set_tests_properties(MeshesBooleanContract PROPERTIES TIMEOUT 360 LABELS "mesh_boolean;component1;continuous")

function(add_exact_kernel_test target name)
    add_executable(${target} ${target}.cc)
    target_link_libraries(${target} PRIVATE ygor Threads::Threads)
    mesh_boolean_strict_fp(${target})
    add_test(NAME ${name} COMMAND ${target})
    set_tests_properties(${name} PROPERTIES TIMEOUT 360 LABELS "mesh_boolean;component3")
endfunction()
add_exact_kernel_test(Test_MeshesExactArithmetic MeshExactArithmetic.Unit)
add_exact_kernel_test(Test_MeshesExactKernel MeshExactKernel.Unit)
add_exact_kernel_test(Test_MeshesExactKernelProperties MeshExactKernel.Properties)

function(add_boolean_component_test target name labels)
    add_executable(${target} ${target}.cc)
    target_link_libraries(${target} PRIVATE ygor Threads::Threads)
    mesh_boolean_strict_fp(${target})
    add_test(NAME ${name} COMMAND ${target})
    set_tests_properties(${name} PROPERTIES TIMEOUT 360 LABELS "${labels}")
endfunction()
add_boolean_component_test(Test_MeshesBooleanVerification MeshBooleanVerification.Unit "mesh_boolean;component13")
add_boolean_component_test(Test_MeshesBooleanTransaction MeshBooleanTransaction.Unit "mesh_boolean;component1;continuous")
add_boolean_component_test(Test_MeshesBooleanInputTopology MeshBooleanInputTopology.Unit "mesh_boolean;component2")
add_boolean_component_test(Test_MeshesBooleanInputTopologyProperties MeshBooleanInputTopology.Properties "mesh_boolean;component2")
add_boolean_component_test(Test_MeshesBooleanBroadPhase MeshBooleanBroadPhase.Unit "mesh_boolean;component4")
add_boolean_component_test(Test_MeshesBooleanBroadPhaseProperties MeshBooleanBroadPhase.Properties "mesh_boolean;component4")
add_boolean_component_test(Test_MeshesBooleanIntersectionEvents MeshBooleanIntersectionEvents.Unit "mesh_boolean;component5")
add_boolean_component_test(Test_MeshesBooleanIntersectionEventsProperties MeshBooleanIntersectionEvents.Properties "mesh_boolean;component5")
add_boolean_component_test(Test_MeshesBooleanSymbolicRegistry MeshBooleanSymbolicRegistry.Unit "mesh_boolean;component6")
add_boolean_component_test(Test_MeshesBooleanSymbolicRegistryProperties MeshBooleanSymbolicRegistry.Properties "mesh_boolean;component6")
add_boolean_component_test(Test_MeshesBooleanLocalRefinement MeshBooleanLocalRefinement.Unit "mesh_boolean;component7")
add_boolean_component_test(Test_MeshesBooleanLocalRefinementProperties MeshBooleanLocalRefinement.Properties "mesh_boolean;component7")
add_boolean_component_test(Test_MeshesBooleanGlobalArrangement MeshBooleanGlobalArrangement.Unit "mesh_boolean;component8")
add_boolean_component_test(Test_MeshesBooleanGlobalArrangementProperties MeshBooleanGlobalArrangement.Properties "mesh_boolean;component8")
add_boolean_component_test(Test_MeshesBooleanCellClassification MeshBooleanCellClassification.Unit "mesh_boolean;component9")
add_boolean_component_test(Test_MeshesBooleanCellClassificationProperties MeshBooleanCellClassification.Properties "mesh_boolean;component9")
add_boolean_component_test(Test_MeshesBooleanCellClassificationAdversarial MeshBooleanCellClassification.Adversarial "mesh_boolean;component9;adversarial")
add_boolean_component_test(Test_MeshesBooleanSelection MeshBooleanSelection.Unit "mesh_boolean;component10")
add_boolean_component_test(Test_MeshesBooleanSelectionProperties MeshBooleanSelection.Properties "mesh_boolean;component10")
add_boolean_component_test(Test_MeshesBooleanSelectionAdversarial MeshBooleanSelection.Adversarial "mesh_boolean;component10;adversarial")
add_boolean_component_test(Test_MeshesBooleanRealization MeshBooleanRealization.Unit "mesh_boolean;component11")
add_boolean_component_test(Test_MeshesBooleanRealizationProperties MeshBooleanRealization.Properties "mesh_boolean;component11")
add_boolean_component_test(Test_MeshesBooleanOutput MeshBooleanOutput.Unit "mesh_boolean;component12")
add_boolean_component_test(Test_MeshesBooleanOutputProperties MeshBooleanOutput.Properties "mesh_boolean;component12")
set_tests_properties(MeshBooleanCellClassification.Adversarial MeshBooleanSelection.Adversarial PROPERTIES TIMEOUT 600)

function(add_component14_test target name labels timeout)
    add_executable(${target} ${target}.cc)
    target_link_libraries(${target} PRIVATE mesh_boolean_test_support)
    target_compile_definitions(${target} PRIVATE
        YGOR_BOOLEAN_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/mesh_boolean")
    mesh_boolean_strict_fp(${target})
    add_test(NAME ${name} COMMAND ${target})
    set_tests_properties(${name} PROPERTIES
        TIMEOUT ${timeout}
        LABELS "mesh_boolean;component14;${labels}"
        ENVIRONMENT "YGOR_BOOLEAN_TEST_TIER=${YGOR_BOOLEAN_TEST_TIER};YGOR_BOOLEAN_ARTIFACT_DIR=${CMAKE_CURRENT_BINARY_DIR}/mesh_boolean_artifacts")
endfunction()

add_component14_test(Test_MeshesBooleanEndToEnd MeshBoolean.EndToEnd "continuous" 600)
add_component14_test(Test_MeshesBooleanMetamorphic MeshBoolean.Metamorphic "property" 600)
add_component14_test(MeshBooleanReplay MeshBoolean.Replay "replay;continuous" 300)
add_component14_test(Test_MeshesBooleanFuzz MeshBoolean.Fuzz "fuzz;scheduled" 1200)
add_component14_test(Test_MeshesBooleanMutation MeshBoolean.Mutation "mutation;scheduled" 600)
add_component14_test(MeshBooleanQualification MeshBoolean.Qualification "qualification;scheduled" 360)
add_component14_test(Test_MeshesBooleanPerformanceBaselines
    MeshBoolean.PerformanceBaselines "performance" 600)

# Keep the documented end-user example independent of test-only support code.
add_executable(MeshBooleanExample
    ${PROJECT_SOURCE_DIR}/examples/MeshBooleanExample.cc)
target_link_libraries(MeshBooleanExample PRIVATE ygor Threads::Threads)
target_compile_features(MeshBooleanExample PRIVATE cxx_std_17)
mesh_boolean_strict_fp(MeshBooleanExample)
add_test(NAME MeshBoolean.Example COMMAND MeshBooleanExample)
set_tests_properties(MeshBoolean.Example PROPERTIES
    TIMEOUT 600
    LABELS "mesh_boolean;component14;example;continuous")

add_executable(Test_MeshesBooleanPlanGaps Test_MeshesBooleanPlanGaps.cc)
target_link_libraries(Test_MeshesBooleanPlanGaps PRIVATE mesh_boolean_test_support)
mesh_boolean_strict_fp(Test_MeshesBooleanPlanGaps)
foreach(case_id IN ITEMS G1a G1b G2a G3 G4a G4b G5a G6 G7a G7b G8solver G8pairs G9a G9b G9c G9d)
    add_test(NAME MeshBoolean.PlanGap.${case_id}
        COMMAND Test_MeshesBooleanPlanGaps ${case_id})
    set_tests_properties(MeshBoolean.PlanGap.${case_id} PROPERTIES
        TIMEOUT 600
        LABELS "mesh_boolean;component14;plan_gap;continuous")
endforeach()
