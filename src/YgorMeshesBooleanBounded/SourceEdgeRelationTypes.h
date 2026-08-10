#pragma once

#include "BoundedOperations.h"
#include "CanonicalBytes.h"
#include "ExactGeometryRelations.h"
#include "PrimitiveRelationKernel.h"
#include "RelationKeys.h"
#include "Sha256.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class source_edge_support_class : std::uint8_t {
  skew_separated = 1,
  nonparallel_coplanar = 2,
  parallel_separated = 3,
  collinear = 4,
};

enum class source_edge_contact_class : std::uint8_t {
  none = 1,
  proper_crossing = 2,
  endpoint_contact = 3,
  point_contact = 4,
  partial_overlap = 5,
  first_contains_second = 6,
  second_contains_first = 7,
  equal = 8,
};

enum class source_edge_orientation_relation : std::uint8_t {
  not_applicable = 0,
  same = 1,
  opposite = 2,
};

template <class T> struct source_edge_relation_input final {
  relation_feature_key feature{};
  bounded_point3<T> start{};
  bounded_point3<T> end{};
  bool original_source_edge = true;
  std::uint32_t reserved = 0;
};

template <class T> struct source_edge_geometry_snapshot final {
  std::array<T, 3> rounded_nominal{};
  std::array<finite_interval<T>, 3> enclosure{};
  std::uint64_t provenance = 0;
  std::uint64_t lineage = 0;
};

template <class T> struct source_edge_parameter_evidence final {
  T rounded_nominal = T(0);
  finite_interval<T> enclosure{};
  parameter_domain_status domain = parameter_domain_status::invalid;
  T domain_margin = T(0);
  exact_relation_status exact_zero = exact_relation_status::unavailable;
  exact_relation_status exact_one = exact_relation_status::unavailable;
  uncertainty_contributors contributors{};
  std::uint64_t trace_root = 0;
};

template <class T> struct source_edge_point_construction final {
  source_edge_geometry_snapshot<T> point{};
  std::array<finite_interval<T>, 3> first_carrier_residual{};
  std::array<finite_interval<T>, 3> second_carrier_residual{};
  std::uint8_t first_endpoint_owner_mask = 0;
  std::uint8_t second_endpoint_owner_mask = 0;
  bool accepted_source_vertex = false;
  bool tolerance_compatible = false;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

template <class T> struct source_edge_relation_record final {
  std::uint16_t schema_version =
      contract_versions::relation_source_edge_edge_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_edge_edge_policy;
  context_owner_token owner{};
  relation_feature_key first_feature{};
  relation_feature_key second_feature{};
  source_edge_geometry_snapshot<T> first_start{};
  source_edge_geometry_snapshot<T> first_end{};
  source_edge_geometry_snapshot<T> second_start{};
  source_edge_geometry_snapshot<T> second_end{};
  source_edge_support_class support =
      source_edge_support_class::skew_separated;
  source_edge_contact_class contact = source_edge_contact_class::none;
  source_edge_orientation_relation orientation =
      source_edge_orientation_relation::not_applicable;
  relation_truth_record parallel_truth{};
  relation_truth_record coplanarity_truth{};
  relation_truth_record collinearity_truth{};
  bool has_coplanarity_truth = false;
  bool has_collinearity_truth = false;
  std::uint8_t parameter_count = 0;
  std::uint8_t point_count = 0;
  std::array<source_edge_parameter_evidence<T>, 2> first_parameters{};
  std::array<source_edge_parameter_evidence<T>, 2> second_parameters{};
  std::array<source_edge_point_construction<T>, 2> points{};
  T residual_boundary = T(0);
  std::uint8_t selected_minor_axis = 3;
  std::uint8_t selected_collinear_axis = 3;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
  bounded_boolean_digest semantic_digest{};
};

bounded_boolean_error source_edge_relation_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint = relation_checkpoint::edge_edge_evaluation);

} // namespace ygor::mesh_boolean::bounded
