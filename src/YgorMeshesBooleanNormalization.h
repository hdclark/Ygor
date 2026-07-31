#pragma once
#ifndef YGOR_MESHES_BOOLEAN_NORMALIZATION_H_
#define YGOR_MESHES_BOOLEAN_NORMALIZATION_H_

#include "YgorMeshesBooleanPreparation.h"
#include "YgorMeshesBooleanProductContractPolicies.h"

#include <limits>
#include <optional>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t normalization_policy_schema = 4;
constexpr std::uint16_t normalization_report_schema = 4;
constexpr std::uint64_t normalization_removed_ordinal =
    std::numeric_limits<std::uint64_t>::max();

enum class normalization_cancellation_policy : std::uint8_t {
  deterministic_checkpoints = 0
};

enum class normalization_defect_code : std::uint16_t {
  nonfinite_coordinate = 1,
  short_face = 2,
  index_out_of_range = 3,
  consecutive_duplicate_index = 4,
  exact_duplicate_vertex = 5,
  exact_duplicate_facet = 6,
  unused_vertex = 7,
  component2_rejection = 8,
  open_boundary_edge = 9,
  small_gap_candidate = 10,
  nonplanar_facet = 11,
  positive_area_coplanar_facet_overlap = 12,
  opposite_oriented_coplanar_facet_overlap = 13,
  overlapping_facet_attribute_conflict = 14,
  partial_overlapping_facet_requires_remeshing = 15,
  triangular_sliver_below_tolerance = 16,
  self_intersecting_facet_ring = 17,
  adjacent_facet_self_intersection = 18,
  vertex_adjacent_facet_self_intersection = 19,
  nonadjacent_facet_self_intersection = 20
};

enum class normalization_map_status : std::uint8_t {
  total = 0,
  unavailable = 1,
  absent = 2
};

enum class normalization_displacement_claim : std::uint8_t {
  exact_zero = 0,
  records_present = 1
};

enum class normalization_reversibility : std::uint8_t {
  identity = 0,
  fully_reversible = 1,
  partially_reversible = 2,
  irreversible = 3
};

enum class normalization_entity_kind : std::uint8_t {
  vertex = 0,
  edge = 1,
  facet = 2,
  shell = 3,
  attribute = 4
};

enum class normalization_topology_justification : std::uint8_t {
  structural_canonicalization = 0,
  caller_authorized_repair = 1,
  defect_resolution = 2
};

enum class normalization_displacement_kind : std::uint8_t {
  exact = 0,
  bounded = 1
};

struct normalization_resource_limits {
  std::uint64_t max_work_units = 100000000;
  std::uint64_t max_defect_records = 1000000;
  std::uint64_t max_mapping_entries = 100000000;
  std::uint64_t max_report_bytes = 256ULL * 1024ULL * 1024ULL;
};

struct normalization_policy {
  std::uint16_t schema = normalization_policy_schema;
  normalization_mode mode = normalization_mode::diagnosis_only;
  model_unit unit = model_unit::unspecified;
  double model_tolerance = 0.0;
  std::uint64_t enabled_operations = 0;
  nonplanar_facet_policy nonplanar_facets = nonplanar_facet_policy::reject;
  std::uint16_t edit_ordering_version = 1;
  std::uint16_t diagnosis_version = 1;
  normalization_resource_limits resources;
  normalization_cancellation_policy cancellation =
      normalization_cancellation_policy::deterministic_checkpoints;
  std::uint32_t checkpoint_interval = 1024;
};

struct normalization_defect {
  normalization_defect_code code = normalization_defect_code::short_face;
  std::uint64_t primary_ordinal = 0;
  std::uint64_t secondary_ordinal = 0;
  std::uint64_t detail = 0;
};

inline bool operator==(const normalization_defect &a,
                       const normalization_defect &b) noexcept {
  return a.code == b.code && a.primary_ordinal == b.primary_ordinal &&
         a.secondary_ordinal == b.secondary_ordinal && a.detail == b.detail;
}

struct normalization_edit {
  normalization_operation operation =
      normalization_operation::irrelevant_storage_removal;
  std::uint64_t canonical_ordinal = 0;
  normalization_entity_kind entity = normalization_entity_kind::vertex;
  std::uint64_t source_ordinal = 0;
  std::uint64_t prepared_ordinal = 0;
  digest before_evidence_digest;
  digest after_evidence_digest;
  normalization_reversibility reversibility =
      normalization_reversibility::identity;
  digest evidence_digest;
};

struct normalization_topology_change {
  normalization_operation operation =
      normalization_operation::irrelevant_storage_removal;
  std::uint64_t source_ordinal = 0;
  normalization_entity_kind entity = normalization_entity_kind::facet;
  std::uint64_t prepared_ordinal = 0;
  normalization_topology_justification justification =
      normalization_topology_justification::structural_canonicalization;
  std::uint32_t justification_subcode = 0;
  digest before_evidence_digest;
  digest after_evidence_digest;
  normalization_reversibility reversibility =
      normalization_reversibility::identity;
  digest evidence_digest;
};

struct normalization_rational {
  std::int64_t numerator = 0;
  std::uint64_t denominator = 1;
};

inline bool operator==(const normalization_rational &a,
                       const normalization_rational &b) noexcept {
  return a.numerator == b.numerator && a.denominator == b.denominator;
}

struct normalization_displacement_record {
  std::uint64_t source_vertex = 0;
  std::uint64_t prepared_vertex = 0;
  normalization_displacement_kind kind = normalization_displacement_kind::exact;
  std::array<normalization_rational, 3> exact_components;
  normalization_rational squared_distance_bound;
  model_unit unit = model_unit::unspecified;
  digest evidence_digest;
};

struct normalization_mapping {
  normalization_map_status status = normalization_map_status::unavailable;
  std::vector<std::uint64_t> source_to_prepared;
};

struct normalization_attribute_mappings {
  std::uint16_t schema = normalization_report_schema;
  normalization_mapping vertex_normals;
  normalization_mapping vertex_colours;
  normalization_mapping involved_faces;
  normalization_mapping metadata;
};

struct normalization_report {
  std::uint16_t schema = normalization_report_schema;
  std::uint16_t producer_version = 1;
  coordinate_tag coordinate = coordinate_tag::binary32;
  index_tag index = index_tag::uint32;
  normalization_policy policy;
  digest policy_digest;
  digest source_digest;
  digest output_digest;
  bool prepared_operand_available = false;
  std::vector<normalization_edit> edits;
  normalization_displacement_claim displacement =
      normalization_displacement_claim::exact_zero;
  std::vector<normalization_displacement_record> displacements;
  std::vector<normalization_topology_change> topology_changes;
  std::vector<normalization_defect> unresolved_defects;
  normalization_mapping vertices;
  std::vector<std::array<std::uint64_t, 2>> source_edges;
  normalization_mapping edges;
  normalization_mapping facets;
  normalization_mapping shells;
  normalization_attribute_mappings attributes;
  normalization_reversibility reversibility =
      normalization_reversibility::identity;
  std::optional<strict_validation_certificate> strict_certificate;
  digest report_digest;
};

struct normalization_decode_limits {
  std::uint64_t max_record_bytes = 256ULL * 1024ULL * 1024ULL;
  std::uint64_t max_defect_records = 1000000;
  std::uint64_t max_edits = 1000000;
  std::uint64_t max_topology_changes = 1000000;
  std::uint64_t max_displacements = 1000000;
  std::uint64_t max_mapping_entries = 100000000;
};

status_or<std::vector<std::uint8_t>>
encode_normalization_policy(const normalization_policy &);
status_or<normalization_policy>
decode_normalization_policy(const std::vector<std::uint8_t> &);
status_or<digest> normalization_policy_digest(const normalization_policy &);

status_or<std::vector<std::uint8_t>>
encode_normalization_report(const normalization_report &);
status_or<normalization_report> decode_normalization_report(
    const std::vector<std::uint8_t> &, const normalization_decode_limits & = {});
status_or<digest> normalization_report_digest(const normalization_report &);

template <class T, class I>
status_or<prepared_operand<T, I>> normalize_operand(
    const fv_surface_mesh<T, I> &, const normalization_policy &,
    normalization_report &, cancellation_source *);

template <class T, class I>
status_or<prepared_operand<T, I>> normalize_operand(
    const fv_surface_mesh<T, I> &source, const normalization_policy &policy,
    normalization_report &report) {
  return normalize_operand(source, policy, report, nullptr);
}

template <class T, class I>
status_or<bool> verify_normalization_report(
    const std::vector<std::uint8_t> &, const fv_surface_mesh<T, I> &source,
    const fv_surface_mesh<T, I> *output, cancellation_source * = nullptr);

} // namespace mesh_boolean
} // namespace ygor
#endif
