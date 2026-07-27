#pragma once

#include "SignedFeatureRelations.h"

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
std::vector<std::uint8_t>
encode_signed_feature_relations(const signed_feature_relations<T, I> &artifact);

template <class T, class I>
bool verify_relation_codec(const signed_feature_relations<T, I> &artifact,
                           bounded_boolean_error &error);

template <class T> struct relation_artifact_envelope final {
  std::uint16_t schema_version = 0;
  std::uint16_t provider_version = 0;
  std::uint16_t graph_policy_version = 0;
  std::uint16_t truth_policy_version = 0;
  std::uint16_t codec_version = 0;
  std::uint16_t verifier_version = 0;
  relation_provider_kind provider =
      relation_provider_kind::canonical_source_feature_relation_graph_v1;
  relation_verification_disposition verification =
      relation_verification_disposition::unverified;
  bounded_boolean_digest context_digest{};
  bounded_boolean_digest precision_digest{};
  bounded_boolean_digest candidate_digest{};
  bounded_boolean_digest graph_digest{};
  boolean_operation operation = boolean_operation::set_union;
  T residual_boundary = T(0);
  bounded_boolean_digest symbolic_policy_digest{};
  std::array<bool, 4> detailed_stage_present{};
  bounded_boolean_digest detailed_stage_digest{};
  bounded_boolean_digest graph_section_digest{};
  std::uint64_t imported_geometry_count = 0;
  std::uint64_t bounded_primitive_count = 0;
  std::uint64_t exact_relation_count = 0;
  std::uint64_t truth_lineage_count = 0;
  std::uint16_t interval_evidence_schema = 0;
  std::uint16_t source_facet_region_schema = 0;
  std::uint64_t interval_evidence_count = 0;
  std::uint64_t source_facet_region_count = 0;
  std::uint64_t truth_count = 0;
  std::uint64_t relation_count = 0;
  std::uint16_t construction_schema = 0;
  std::uint16_t construction_registry_policy = 0;
  std::uint16_t construction_ledger_schema = 0;
  std::uint64_t construction_count = 0;
  std::uint64_t construction_ledger_count = 0;
  std::uint16_t coplanar_topology_schema = 0;
  std::uint16_t coplanar_topology_policy = 0;
  std::uint64_t coplanar_event_node_count = 0;
  std::uint64_t coplanar_oriented_arc_count = 0;
  std::uint64_t coplanar_overlap_component_count = 0;
  std::uint64_t symbolic_eligibility_count = 0;
  std::uint64_t symbolic_decision_count = 0;
  std::uint64_t crossing_count = 0;
  std::uint64_t event_seed_count = 0;
  std::uint64_t incidence_count = 0;
  std::uint64_t candidate_disposition_count = 0;
  relation_statistics statistics{};
};

template <class T>
bool parse_relation_artifact_envelope(
    const std::vector<std::uint8_t> &bytes,
    const relation_capabilities &capabilities,
    relation_artifact_envelope<T> &envelope,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
