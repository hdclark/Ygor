include(CheckCXXCompilerFlag)

function(ygor_apply_mesh_boolean_strict_fp target)
    target_compile_definitions(${target} PRIVATE YGOR_MESH_BOOLEAN_STRICT_FP_BUILD=1)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -fno-fast-math -frounding-math -fno-associative-math
            -fno-reciprocal-math -fsigned-zeros -fno-finite-math-only
            -ffp-contract=off)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -fno-fast-math -frounding-math -fsigned-zeros -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /fp:strict)
    else()
        message(FATAL_ERROR "No qualified strict floating-point profile for ${CMAKE_CXX_COMPILER_ID}")
    endif()
    set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
endfunction()

add_library(ygor_exact_float_expansion_core
    ../YgorMeshesExactFloatExpansionCore.cc
    ../YgorMeshesAdaptivePredicates.cc)
set_property(TARGET ygor_exact_float_expansion_core PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(ygor_exact_float_expansion_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ygor_apply_mesh_boolean_strict_fp(ygor_exact_float_expansion_core)

add_library(ygor_mesh_boolean_bounded_strict
    BoundedOperations.cc
    ConservativeBounds.cc
    ConstructionConditioning.cc
    DirectedRounding.cc
    ExactFloatExpansion.cc
    FiniteInterval.cc
    PrecisionBootstrap.cc
    PrecisionCodec.cc
    PrecisionContext.cc
    PrecisionImport.cc
    PrecisionLedger.cc
    PrecisionTrace.cc
    PrecisionVerifier.cc
    PredicateResults.cc
    Resources.cc
    Sha256.cc
    SourceFacetGeometryBasis.cc
    BoundedSourcePolygonKernel.cc
    SourceEarTriangulationReference.cc
    SourceEarTriangulationIndexed.cc
    SourceTriangleProvenance.cc
    SourceTriangulationCoverage.cc
    SourceTriangleComplexCodec.cc
    SourceTriangleComplexVerifier.cc
    SourceTriangulation.cc
    CanonicalGeometryAttachments.cc
    CanonicalHalfedgeBuild.cc
    CanonicalHalfedgeCodec.cc
    CanonicalHalfedgePairing.cc
    CanonicalHalfedgePreflight.cc
    CanonicalHalfedgeVerifier.cc
    CanonicalFeatureGroups.cc
    CanonicalVertexFans.cc
    CandidateDomainPolicy.cc
    BroadPhasePrimitiveTables.cc
    RankMortonKey.cc
    TriangleAABBHierarchy.cc
    BroadPhasePreflight.cc
    BroadPhaseTraverse.cc
    CandidateCanonicalization.cc
    BroadPhaseBuild.cc
    BroadPhaseCodec.cc
    BroadPhaseVerifier.cc
    RelationKeys.cc
    IntersectionKeys.cc
    IntersectionPreflight.cc
    EventNormalization.cc
    EventInterning.cc
    EventCoordinates.cc
    EventIncidence.cc
    BoundedCarrierOrdering.cc
    SourceEdgeArrangements.cc
    TransverseCarrierArrangements.cc
    CoplanarCarrierArrangements.cc
    RelationPreflight.cc
    RelationRequestGraph.cc
    PrimitiveRelationKernel.cc
    SourceFacetRegionKernel.cc
    SourceEdgeRelationKernel.cc
    EdgeFacetRelations.cc
    FacetFacetRelations.cc
    CoplanarRelationOverlay.cc
    RelationCanonicalization.cc
    RelationEventSeeds.cc
    SymbolicPerturbation.cc
    RelationBuild.cc
    RelationCodec.cc
    RelationReplay.cc
    RelationCandidateEvidenceVerifier.cc
    RelationVerifier.cc
    ToleranceBudget.cc)
set_property(TARGET ygor_mesh_boolean_bounded_strict PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(ygor_mesh_boolean_bounded_strict PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ygor_mesh_boolean_bounded_strict PUBLIC ygor_exact_float_expansion_core)
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_bounded_strict)

install(TARGETS
        ygor_exact_float_expansion_core
        ygor_mesh_boolean_bounded_strict
        EXPORT YgorTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/YgorMeshesBooleanBounded"
        FILES_MATCHING PATTERN "*.h")

# The public facade uses Ygor's out-of-line mesh/math API. Keep it in ygor while
# the strict provider above remains independently linkable and dependency-minimal.
add_library(ygor_mesh_boolean_bounded_facade OBJECT
    Cancellation.cc
    YgorMeshesBooleanBounded.cc)
set_property(TARGET ygor_mesh_boolean_bounded_facade PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(ygor_mesh_boolean_bounded_facade PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_BINARY_DIR}/..")
ygor_apply_mesh_boolean_strict_fp(ygor_mesh_boolean_bounded_facade)
target_sources(ygor PRIVATE $<TARGET_OBJECTS:ygor_mesh_boolean_bounded_facade>)

if(BUILD_TESTING)
    add_subdirectory("${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded" "${CMAKE_BINARY_DIR}/tests/mesh_boolean_bounded")
    include("${CMAKE_SOURCE_DIR}/tests/mesh_boolean_bounded/Component07SourceEdge.cmake")
endif()
