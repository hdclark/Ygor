#pragma once

#include "CanonicalBytes.h"
#include "IntersectionTypes.h"
#include "RelationKeys.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class intersection_carrier_role : std::uint8_t {
  none = 1,
  transverse_endpoint = 2,
  coplanar_boundary = 3,
  overlap_start = 4,
  overlap_end = 5,
};

enum class carrier_orientation_role : std::uint8_t {
  canonical_forward = 1,
  canonical_reverse = 2,
};

enum class source_facet_use_role : std::uint8_t {
  none = 1,
  left_incident = 2,
  right_incident = 3,
  both_incident = 4,
};

enum class boundary_reference_kind : std::uint8_t {
  start_sentinel = 1,
  cluster = 2,
  end_sentinel = 3,
};

struct intersection_event_key final {
  bounded_boolean_digest semantic_namespace{};
  intersection_event_class event_class =
      intersection_event_class::source_vertex_contact;
  operand_id first_operand = operand_id::a;
  operand_id second_operand = operand_id::b;
  relation_feature_key first_owner{};
  relation_feature_key second_owner{};
  relation_event_seed_key public_relation{};
  relation_construction_kind construction_kind =
      relation_construction_kind::bounded_point;
  relation_construction_precedence construction_precedence =
      relation_construction_precedence::verification_witness;
  relation_feature_key authoritative_source_feature{};
  relation_feature_key reused_source_vertex{};
  std::uint64_t construction_source_provenance = 0;
  std::uint64_t construction_geometric_lineage = 0;
  intersection_carrier_role carrier_role = intersection_carrier_role::none;
  feature_relation_status contact_status =
      feature_relation_status::not_evaluated;
  relation_contact_dimension contact_dimension = relation_contact_dimension::none;
  std::uint64_t symbolic_rule_ordinal = 0;
  std::uint16_t symbolic_tie_key_schema = 0;
  std::uint16_t equivalence_policy_version =
      contract_versions::intersection_semantic_policy;
  std::uint16_t schema_version =
      contract_versions::intersection_event_key_schema;
  std::uint16_t reserved = 0;

  friend bool operator<(const intersection_event_key &a,
                        const intersection_event_key &b) noexcept {
    return std::tie(a.semantic_namespace.bytes, a.event_class,
                    a.first_operand, a.second_operand, a.first_owner,
                    a.second_owner, a.public_relation, a.construction_kind,
                    a.construction_precedence,
                    a.authoritative_source_feature, a.reused_source_vertex,
                    a.construction_source_provenance,
                    a.construction_geometric_lineage, a.carrier_role,
                    a.contact_status, a.contact_dimension,
                    a.symbolic_rule_ordinal, a.symbolic_tie_key_schema,
                    a.equivalence_policy_version, a.schema_version,
                    a.reserved) <
           std::tie(b.semantic_namespace.bytes, b.event_class,
                    b.first_operand, b.second_operand, b.first_owner,
                    b.second_owner, b.public_relation, b.construction_kind,
                    b.construction_precedence,
                    b.authoritative_source_feature, b.reused_source_vertex,
                    b.construction_source_provenance,
                    b.construction_geometric_lineage, b.carrier_role,
                    b.contact_status, b.contact_dimension,
                    b.symbolic_rule_ordinal, b.symbolic_tie_key_schema,
                    b.equivalence_policy_version, b.schema_version,
                    b.reserved);
  }
  friend bool operator==(const intersection_event_key &a,
                         const intersection_event_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
  friend bool operator!=(const intersection_event_key &a,
                         const intersection_event_key &b) noexcept {
    return !(a == b);
  }
};

struct intersection_occurrence_discriminator final {
  occurrence_role role = occurrence_role::single_occurrence;
  std::uint32_t component07_occurrence = 0;
  std::uint64_t first_shell = intersection_invalid_ordinal;
  std::uint64_t second_shell = intersection_invalid_ordinal;
  std::uint64_t first_sheet = intersection_invalid_ordinal;
  std::uint64_t second_sheet = intersection_invalid_ordinal;
  symbolic_relation_side symbolic_side = symbolic_relation_side::coincident;
  std::uint32_t symbolic_priority = 0;
  std::uint32_t multiplicity_slot = 0;
  std::uint64_t occurrence_lineage = 0;

  friend bool operator<(const intersection_occurrence_discriminator &a,
                        const intersection_occurrence_discriminator &b) noexcept {
    return std::tie(a.role, a.component07_occurrence, a.first_shell,
                    a.second_shell, a.first_sheet, a.second_sheet,
                    a.symbolic_side, a.symbolic_priority,
                    a.multiplicity_slot, a.occurrence_lineage) <
           std::tie(b.role, b.component07_occurrence, b.first_shell,
                    b.second_shell, b.first_sheet, b.second_sheet,
                    b.symbolic_side, b.symbolic_priority,
                    b.multiplicity_slot, b.occurrence_lineage);
  }
  friend bool operator==(const intersection_occurrence_discriminator &a,
                         const intersection_occurrence_discriminator &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct intersection_occurrence_key final {
  intersection_event_key event{};
  intersection_occurrence_discriminator discriminator{};
  std::uint16_t policy_version = contract_versions::intersection_occurrence_policy;
  std::uint16_t schema_version =
      contract_versions::intersection_occurrence_key_schema;
  std::uint32_t reserved = 0;

  friend bool operator<(const intersection_occurrence_key &a,
                        const intersection_occurrence_key &b) noexcept {
    return std::tie(a.event, a.discriminator, a.policy_version,
                    a.schema_version, a.reserved) <
           std::tie(b.event, b.discriminator, b.policy_version,
                    b.schema_version, b.reserved);
  }
  friend bool operator==(const intersection_occurrence_key &a,
                         const intersection_occurrence_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct source_edge_membership_key final {
  relation_feature_key source_edge{};
  intersection_occurrence_key occurrence{};
  intersection_membership_role role = intersection_membership_role::interior;
  relation_construction_id parameter_construction{0};
  std::uint64_t parameter_lineage = 0;
  std::uint64_t relation_lineage = 0;
  std::uint64_t overlap_lineage = 0;
  source_facet_use_role facet_use_role = source_facet_use_role::none;
  std::uint16_t schema_version =
      contract_versions::intersection_membership_key_schema;
  std::uint16_t reserved = 0;

  friend bool operator<(const source_edge_membership_key &a,
                        const source_edge_membership_key &b) noexcept {
    return std::tie(a.source_edge, a.occurrence, a.role,
                    a.parameter_construction, a.parameter_lineage,
                    a.relation_lineage, a.overlap_lineage,
                    a.facet_use_role, a.schema_version, a.reserved) <
           std::tie(b.source_edge, b.occurrence, b.role,
                    b.parameter_construction, b.parameter_lineage,
                    b.relation_lineage, b.overlap_lineage,
                    b.facet_use_role, b.schema_version, b.reserved);
  }
  friend bool operator==(const source_edge_membership_key &a,
                         const source_edge_membership_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct transverse_carrier_key final {
  relation_feature_key first_facet{};
  relation_feature_key second_facet{};
  relation_construction_id construction{0};
  std::uint64_t construction_lineage = 0;
  carrier_orientation_role orientation =
      carrier_orientation_role::canonical_forward;
  std::uint16_t support_policy_version =
      contract_versions::intersection_transverse_carrier_policy;
  std::uint16_t schema_version =
      contract_versions::intersection_carrier_key_schema;
  std::uint32_t reserved = 0;

  friend bool operator<(const transverse_carrier_key &a,
                        const transverse_carrier_key &b) noexcept {
    return std::tie(a.first_facet, a.second_facet, a.construction,
                    a.construction_lineage, a.orientation,
                    a.support_policy_version, a.schema_version, a.reserved) <
           std::tie(b.first_facet, b.second_facet, b.construction,
                    b.construction_lineage, b.orientation,
                    b.support_policy_version, b.schema_version, b.reserved);
  }
  friend bool operator==(const transverse_carrier_key &a,
                         const transverse_carrier_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct collinear_overlap_carrier_key final {
  relation_feature_key first_edge{};
  relation_feature_key second_edge{};
  std::uint64_t coplanar_support_lineage = 0;
  std::uint64_t overlap_lineage = 0;
  bool opposite_direction = false;
  operand_id symbolic_owner = operand_id::a;
  std::uint16_t half_open_policy_version =
      contract_versions::intersection_collinear_overlap_policy;
  std::uint16_t schema_version =
      contract_versions::intersection_carrier_key_schema;
  std::uint32_t reserved = 0;

  friend bool operator<(const collinear_overlap_carrier_key &a,
                        const collinear_overlap_carrier_key &b) noexcept {
    return std::tie(a.first_edge, a.second_edge,
                    a.coplanar_support_lineage, a.overlap_lineage,
                    a.opposite_direction, a.symbolic_owner,
                    a.half_open_policy_version, a.schema_version,
                    a.reserved) <
           std::tie(b.first_edge, b.second_edge,
                    b.coplanar_support_lineage, b.overlap_lineage,
                    b.opposite_direction, b.symbolic_owner,
                    b.half_open_policy_version, b.schema_version,
                    b.reserved);
  }
  friend bool operator==(const collinear_overlap_carrier_key &a,
                         const collinear_overlap_carrier_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct source_edge_cluster_key final {
  relation_feature_key source_edge{};
  intersection_cluster_equivalence equivalence =
      intersection_cluster_equivalence::exact_parameter_coincidence;
  std::vector<intersection_occurrence_key> members{};
  std::uint16_t tie_policy_version =
      contract_versions::intersection_cluster_eligibility_policy;
  std::uint16_t schema_version = contract_versions::intersection_cluster_key_schema;
  std::uint32_t reserved = 0;

  friend bool operator<(const source_edge_cluster_key &a,
                        const source_edge_cluster_key &b) noexcept {
    return std::tie(a.source_edge, a.equivalence, a.members,
                    a.tie_policy_version, a.schema_version, a.reserved) <
           std::tie(b.source_edge, b.equivalence, b.members,
                    b.tie_policy_version, b.schema_version, b.reserved);
  }
  friend bool operator==(const source_edge_cluster_key &a,
                         const source_edge_cluster_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct source_edge_boundary_key final {
  boundary_reference_kind kind = boundary_reference_kind::start_sentinel;
  source_edge_cluster_key cluster{};

  friend bool operator<(const source_edge_boundary_key &a,
                        const source_edge_boundary_key &b) noexcept {
    return std::tie(a.kind, a.cluster) < std::tie(b.kind, b.cluster);
  }
  friend bool operator==(const source_edge_boundary_key &a,
                         const source_edge_boundary_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct source_edge_interval_key final {
  relation_feature_key source_edge{};
  source_edge_boundary_key left{};
  source_edge_boundary_key right{};
  std::uint64_t canonical_ordinal = 0;
  intersection_interval_length interval_class =
      intersection_interval_length::definitely_positive;
  std::uint16_t schema_version =
      contract_versions::intersection_interval_key_schema;
  std::uint16_t reserved = 0;

  friend bool operator<(const source_edge_interval_key &a,
                        const source_edge_interval_key &b) noexcept {
    return std::tie(a.source_edge, a.left, a.right, a.canonical_ordinal,
                    a.interval_class, a.schema_version, a.reserved) <
           std::tie(b.source_edge, b.left, b.right, b.canonical_ordinal,
                    b.interval_class, b.schema_version, b.reserved);
  }
  friend bool operator==(const source_edge_interval_key &a,
                         const source_edge_interval_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct intersection_descriptor_key final {
  intersection_descriptor_locus locus =
      intersection_descriptor_locus::whole_source_edge;
  intersection_descriptor_category category =
      intersection_descriptor_category::no_influence;
  relation_feature_key source_feature{};
  intersection_occurrence_key occurrence{};
  std::uint64_t parent_lineage = 0;
  std::uint64_t boundary_ordinal = 0;
  std::int8_t orientation = 0;
  std::uint16_t schema_version =
      contract_versions::intersection_descriptor_key_schema;
  std::uint16_t reserved = 0;

  friend bool operator<(const intersection_descriptor_key &a,
                        const intersection_descriptor_key &b) noexcept {
    return std::tie(a.locus, a.category, a.source_feature, a.occurrence,
                    a.parent_lineage, a.boundary_ordinal, a.orientation,
                    a.schema_version, a.reserved) <
           std::tie(b.locus, b.category, b.source_feature, b.occurrence,
                    b.parent_lineage, b.boundary_ordinal, b.orientation,
                    b.schema_version, b.reserved);
  }
  friend bool operator==(const intersection_descriptor_key &a,
                         const intersection_descriptor_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

bool valid_intersection_event_key(const intersection_event_key &key) noexcept;
bool valid_intersection_occurrence_key(
    const intersection_occurrence_key &key) noexcept;
bool valid_source_edge_membership_key(
    const source_edge_membership_key &key) noexcept;
bool valid_transverse_carrier_key(const transverse_carrier_key &key) noexcept;
bool valid_collinear_overlap_carrier_key(
    const collinear_overlap_carrier_key &key) noexcept;
bool valid_source_edge_cluster_key(const source_edge_cluster_key &key) noexcept;
bool valid_source_edge_interval_key(const source_edge_interval_key &key) noexcept;
bool valid_intersection_descriptor_key(
    const intersection_descriptor_key &key) noexcept;

intersection_event_key remap_intersection_event_key(
    intersection_event_key key) noexcept;
intersection_occurrence_key remap_intersection_occurrence_key(
    intersection_occurrence_key key) noexcept;
source_edge_membership_key remap_source_edge_membership_key_operands(
    source_edge_membership_key key) noexcept;
transverse_carrier_key remap_transverse_carrier_key(
    transverse_carrier_key key) noexcept;
collinear_overlap_carrier_key remap_collinear_overlap_carrier_key(
    collinear_overlap_carrier_key key) noexcept;
intersection_descriptor_key remap_intersection_descriptor_key(
    intersection_descriptor_key key) noexcept;

source_edge_membership_key reverse_source_edge_membership_key(
    source_edge_membership_key key) noexcept;
source_edge_cluster_key reverse_source_edge_cluster_key(
    source_edge_cluster_key key) noexcept;
source_edge_interval_key reverse_source_edge_interval_key(
    source_edge_interval_key key) noexcept;

void encode_intersection_event_key(canonical_writer &writer,
                                   const intersection_event_key &key);
void encode_intersection_occurrence_key(
    canonical_writer &writer, const intersection_occurrence_key &key);
void encode_source_edge_membership_key(
    canonical_writer &writer, const source_edge_membership_key &key);
void encode_transverse_carrier_key(canonical_writer &writer,
                                   const transverse_carrier_key &key);
void encode_collinear_overlap_carrier_key(
    canonical_writer &writer, const collinear_overlap_carrier_key &key);
void encode_source_edge_cluster_key(canonical_writer &writer,
                                    const source_edge_cluster_key &key);
void encode_source_edge_interval_key(canonical_writer &writer,
                                     const source_edge_interval_key &key);
void encode_intersection_descriptor_key(
    canonical_writer &writer, const intersection_descriptor_key &key);

std::vector<std::uint8_t>
encode_intersection_event_key(const intersection_event_key &key);
std::vector<std::uint8_t>
encode_intersection_occurrence_key(const intersection_occurrence_key &key);

} // namespace ygor::mesh_boolean::bounded
