file(READ
    "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/RelationVerifier.cc"
    verifier_source)
file(READ
    "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/RelationCandidateEvidenceVerifier.cc"
    candidate_source)
file(READ
    "${SOURCE_ROOT}/src/YgorMeshesBooleanBounded/RelationVerificationRecords.h"
    verification_records_source)

set(forbidden_verifier_tokens
    "verify_candidate_source_edge_relation_stage("
    "verify_candidate_source_edge_facet_relation_stage("
    "verify_candidate_source_facet_relation_stage("
    "verify_candidate_coplanar_overlay_stage("
    "build_source_vertex_facet_evaluated_stage("
    "build_transverse_relation_evaluated_stage("
    "reconstruct_overlay("
    "edge_status(source.contact"
    "edge_facet_status(source.contact"
    "facet_status(source.classification"
    "overlay_status(source.classification"
    "#include \"RelationConstructionPolicy.h\""
    "relation_construction_policy_detail"
    "#include \"CandidateSourceEdgeRelations.h\""
    "#include \"CoplanarRelationOverlay.h\""
    "#include \"EdgeFacetRelations.h\""
    "#include \"FacetFacetRelations.h\""
    "#include \"TransverseRelationEvaluation.h\""
    "relation_construction_policy_detail::edge_point_authority("
    "relation_construction_policy_detail::edge_facet_event_authority("
    "relation_construction_policy_detail::carrier_authority("
    "relation_construction_policy_detail::overlay_node_authority(")
foreach(token IN LISTS forbidden_verifier_tokens)
    string(FIND "${verifier_source}" "${token}" position)
    if(NOT position EQUAL -1)
        message(FATAL_ERROR
            "RelationVerifier contains forbidden producer dependency: ${token}")
    endif()
endforeach()

foreach(token
        "RelationConstructionPolicy.h"
        "relation_construction_policy_detail")
    string(FIND "${verification_records_source}" "${token}" position)
    if(NOT position EQUAL -1)
        message(FATAL_ERROR
            "Verifier record schema boundary exposes producer policy: ${token}")
    endif()
endforeach()

string(FIND "${candidate_source}" "#include \"CoplanarRelationOverlay.h\""
       coplanar_include)
if(NOT coplanar_include EQUAL -1)
    message(FATAL_ERROR
        "Candidate evidence verifier includes coplanar producer implementation")
endif()
