set(component08_sources
    IntersectionPreflight.h
    IntersectionBuild.cc
    IntersectionVerifier.h
    IntersectionVerifier.cc
    EventNormalization.h
    EventIncidence.h
    TransverseRelationAdapter.h)

foreach(source IN LISTS component08_sources)
    file(READ "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/${source}" contents)
    if(contents MATCHES "(\\.|->)(producer_)?(predecessor_candidates|candidates|source_edge_stage|source_edge_facet_stage|source_facet_stage|coplanar_overlay_stage)\\(")
        message(FATAL_ERROR "Component 08 boundary violation in ${source}")
    endif()
    if(contents MATCHES [[#include "(CandidateSourceEdgeRelations|EdgeFacetRelations|FacetFacetRelations|CoplanarRelationOverlay|CanonicalSourceManifolds|RelationArtifactInternalAccess)\.h"]])
        message(FATAL_ERROR "Component 08 private-stage include in ${source}")
    endif()
endforeach()

file(READ "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/IntersectionVerifier.h" verifier_header)
if(verifier_header MATCHES "const signed_feature_relations<")
    message(FATAL_ERROR "Component 08 verifier API accepts raw signed_feature_relations")
endif()

file(READ "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/IntersectionBuild.h" build_header)
string(REGEX MATCHALL "signed_feature_relations<" raw_build_mentions "${build_header}")
list(LENGTH raw_build_mentions raw_build_mention_count)
if(NOT raw_build_mention_count EQUAL 2)
    message(FATAL_ERROR "Only the Component 08 outer build declaration/extern adapter may accept raw signed_feature_relations")
endif()

file(READ "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/SignedFeatureRelations.h" artifact_header)
if(artifact_header MATCHES "[ \\n](producer_)?(predecessor_candidates|candidates|source_edge_stage|source_edge_facet_stage|source_facet_stage|coplanar_overlay_stage)[ \\n]*\\(")
    message(FATAL_ERROR "Component 07 artifact retains a public predecessor/private-stage accessor")
endif()

file(GLOB component_sources
    "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/*.h"
    "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/*.cc")
foreach(source IN LISTS component_sources)
    get_filename_component(name "${source}" NAME)
    if(name STREQUAL "RelationReplay.cc" OR
       name STREQUAL "RelationCandidateEvidenceVerifier.cc" OR
       name STREQUAL "RelationArtifactInternalAccess.h")
        continue()
    endif()
    file(READ "${source}" contents)
    if(contents MATCHES [[#include "RelationArtifactInternalAccess\.h"]])
        message(FATAL_ERROR "Component 07 internal artifact access leaked into ${name}")
    endif()
endforeach()
