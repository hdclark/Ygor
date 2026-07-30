#include "StrictFloatingBuild.h"
#include "IntersectionKeys.h"

#include <algorithm>

namespace ygor::mesh_boolean::bounded {
namespace {

bool valid_operand(operand_id operand) noexcept {
  return operand == operand_id::a || operand == operand_id::b;
}

template <class E>
bool enum_between(E value, std::uint8_t first, std::uint8_t last) noexcept {
  const auto raw = static_cast<std::uint8_t>(value);
  return raw >= first && raw <= last;
}

void encode_digest(canonical_writer &writer,
                   const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    writer.u8(byte);
}

void encode_discriminator(canonical_writer &writer,
                          const intersection_occurrence_discriminator &value) {
  writer.u8(static_cast<std::uint8_t>(value.role));
  writer.u32(value.component07_occurrence);
  writer.u64(value.first_shell);
  writer.u64(value.second_shell);
  writer.u64(value.first_sheet);
  writer.u64(value.second_sheet);
  writer.u8(static_cast<std::uint8_t>(
      static_cast<std::int8_t>(value.symbolic_side)));
  writer.u32(value.symbolic_priority);
  writer.u32(value.multiplicity_slot);
  writer.u64(value.occurrence_lineage);
}

relation_event_seed_key remap_seed_key(relation_event_seed_key key) noexcept {
  key.first = remap_relation_feature_key(key.first);
  key.second = remap_relation_feature_key(key.second);
  return key;
}

intersection_occurrence_discriminator remap_discriminator(
    intersection_occurrence_discriminator value) noexcept {
  std::swap(value.first_shell, value.second_shell);
  std::swap(value.first_sheet, value.second_sheet);
  if (value.symbolic_side == symbolic_relation_side::negative)
    value.symbolic_side = symbolic_relation_side::positive;
  else if (value.symbolic_side == symbolic_relation_side::positive)
    value.symbolic_side = symbolic_relation_side::negative;
  return value;
}

bool valid_occurrence_discriminator(
    const intersection_occurrence_discriminator &value) noexcept {
  const auto symbolic_side = static_cast<std::int8_t>(value.symbolic_side);
  if (!enum_between(value.role, 1, 6) || symbolic_side < -1 ||
      symbolic_side > 1)
    return false;
  const bool shells_present =
      value.first_shell != intersection_invalid_ordinal ||
      value.second_shell != intersection_invalid_ordinal;
  const bool sheets_present =
      value.first_sheet != intersection_invalid_ordinal ||
      value.second_sheet != intersection_invalid_ordinal;
  switch (value.role) {
  case occurrence_role::single_occurrence:
    return !shells_present && !sheets_present &&
           value.symbolic_side == symbolic_relation_side::coincident &&
           value.symbolic_priority == 0 && value.multiplicity_slot == 0;
  case occurrence_role::topology_separated_contact:
  case occurrence_role::overlap_boundary_occurrence:
    return value.occurrence_lineage != 0;
  case occurrence_role::coincident_sheet_member:
    return sheets_present && value.occurrence_lineage != 0;
  case occurrence_role::symbolic_side_occurrence:
    return value.symbolic_side != symbolic_relation_side::coincident &&
           value.symbolic_priority != 0 && value.occurrence_lineage != 0;
  case occurrence_role::multiplicity_occurrence:
    return value.multiplicity_slot != 0 && value.occurrence_lineage != 0;
  }
  return false;
}

} // namespace

bool valid_intersection_event_key(const intersection_event_key &key) noexcept {
  if (!enum_between(key.event_class, 1, 8) ||
      !valid_operand(key.first_operand) || !valid_operand(key.second_operand) ||
      key.first_operand == key.second_operand ||
      !valid_relation_feature_key(key.first_owner) ||
      !valid_relation_feature_key(key.second_owner) ||
      key.first_owner.kind == relation_feature_kind::source_triangle ||
      key.first_owner.kind == relation_feature_kind::facet_internal_diagonal ||
      key.second_owner.kind == relation_feature_kind::source_triangle ||
      key.second_owner.kind == relation_feature_kind::facet_internal_diagonal ||
      key.first_owner.operand != key.first_operand ||
      key.second_owner.operand != key.second_operand ||
      !valid_relation_event_seed_key(key.public_relation) ||
      !enum_between(key.construction_kind, 1, 4) ||
      !enum_between(key.construction_precedence, 1, 6) ||
      !valid_relation_feature_key(key.authoritative_source_feature, true) ||
      !valid_relation_feature_key(key.reused_source_vertex, true) ||
      !enum_between(key.carrier_role, 1, 5) ||
      !enum_between(key.contact_status, 0, 10) ||
      !enum_between(key.contact_dimension, 0, 3) ||
      key.equivalence_policy_version !=
          contract_versions::intersection_semantic_policy ||
      key.schema_version != contract_versions::intersection_event_key_schema ||
      key.reserved != 0)
    return false;

  if (key.reused_source_vertex.kind != relation_feature_kind::none &&
      key.reused_source_vertex.kind != relation_feature_kind::source_vertex)
    return false;
  if (key.construction_kind != relation_construction_kind::bounded_point)
    return false;
  if (key.construction_geometric_lineage == 0 &&
      key.reused_source_vertex.kind == relation_feature_kind::none)
    return false;
  if (key.event_class == intersection_event_class::source_vertex_contact &&
      key.reused_source_vertex.kind != relation_feature_kind::source_vertex)
    return false;
  if (key.event_class != intersection_event_class::source_vertex_contact &&
      key.reused_source_vertex.kind == relation_feature_kind::source_vertex &&
      key.construction_precedence !=
          relation_construction_precedence::accepted_source_vertex)
    return false;
  if (key.event_class == intersection_event_class::symbolic_tie &&
      (key.symbolic_rule_ordinal == 0 || key.symbolic_tie_key_schema == 0))
    return false;
  return true;
}

bool valid_intersection_occurrence_key(
    const intersection_occurrence_key &key) noexcept {
  return valid_intersection_event_key(key.event) &&
         valid_occurrence_discriminator(key.discriminator) &&
         key.policy_version == contract_versions::intersection_occurrence_policy &&
         key.schema_version ==
             contract_versions::intersection_occurrence_key_schema &&
         key.reserved == 0;
}

bool valid_event_incidence_key(const event_incidence_key &key) noexcept {
  const auto encoded_kind = static_cast<std::uint8_t>(key.kind);
  if (!valid_intersection_event_key(key.event) ||
      !valid_intersection_occurrence_key(key.occurrence) ||
      !(key.occurrence.event == key.event) ||
      !valid_relation_event_seed_key(key.seed) || encoded_kind < 1 ||
      encoded_kind > 15 || key.predecessor_relation == intersection_invalid_ordinal ||
      key.symbolic_crossing < -1 || key.symbolic_crossing > 1 ||
      key.orientation < -1 || key.orientation > 1 ||
      key.schema_version != contract_versions::intersection_incidence_schema ||
      key.reserved != 0)
    return false;
  const bool feature_kind = key.kind == event_incidence_kind::source_vertex ||
      key.kind == event_incidence_kind::source_edge ||
      key.kind == event_incidence_kind::source_edge_interval ||
      key.kind == event_incidence_kind::source_facet ||
      key.kind == event_incidence_kind::source_triangle ||
      key.kind == event_incidence_kind::oriented_halfedge ||
      key.kind == event_incidence_kind::source_shell;
  if (feature_kind != valid_relation_feature_key(key.feature))
    return false;
  if ((key.feature.kind == relation_feature_kind::source_triangle ||
       key.feature.kind == relation_feature_kind::facet_internal_diagonal ||
       key.kind == event_incidence_kind::oriented_halfedge) &&
      (!key.bookkeeping_only || key.source_feature_owner))
    return false;
  if (key.feature.kind == relation_feature_kind::facet_internal_diagonal &&
      key.kind != event_incidence_kind::source_edge &&
      key.kind != event_incidence_kind::oriented_halfedge)
    return false;
  const bool candidate_backed =
      key.predecessor_candidate != intersection_invalid_ordinal;
  if ((key.kind == event_incidence_kind::candidate) != candidate_backed &&
      key.kind != event_incidence_kind::source_edge &&
      key.kind != event_incidence_kind::source_triangle &&
      key.kind != event_incidence_kind::oriented_halfedge)
    return false;
  return true;
}

bool valid_source_edge_membership_key(
    const source_edge_membership_key &key) noexcept {
  return valid_relation_feature_key(key.source_edge) &&
         key.source_edge.kind == relation_feature_kind::source_edge &&
         valid_intersection_occurrence_key(key.occurrence) &&
         enum_between(key.role, 1, 5) && key.parameter_lineage != 0 &&
         key.relation_lineage != 0 && enum_between(key.facet_use_role, 1, 4) &&
         key.schema_version ==
             contract_versions::intersection_membership_key_schema &&
         key.reserved == 0;
}

bool valid_transverse_carrier_key(const transverse_carrier_key &key) noexcept {
  return valid_relation_feature_key(key.first_facet) &&
         valid_relation_feature_key(key.second_facet) &&
         key.first_facet.kind == relation_feature_kind::source_facet &&
         key.second_facet.kind == relation_feature_kind::source_facet &&
         key.first_facet.operand != key.second_facet.operand &&
         key.construction_lineage != 0 && enum_between(key.orientation, 1, 2) &&
         key.support_policy_version ==
             contract_versions::intersection_transverse_carrier_policy &&
         key.schema_version == contract_versions::intersection_carrier_key_schema &&
         key.reserved == 0;
}

bool valid_collinear_overlap_carrier_key(
    const collinear_overlap_carrier_key &key) noexcept {
  return valid_relation_feature_key(key.first_edge) &&
         valid_relation_feature_key(key.second_edge) &&
         key.first_edge.kind == relation_feature_kind::source_edge &&
         key.second_edge.kind == relation_feature_kind::source_edge &&
         key.first_edge.operand != key.second_edge.operand &&
         key.coplanar_support_lineage != 0 && key.overlap_lineage != 0 &&
         valid_operand(key.symbolic_owner) &&
         key.half_open_policy_version ==
             contract_versions::intersection_collinear_overlap_policy &&
         key.schema_version == contract_versions::intersection_carrier_key_schema &&
         key.reserved == 0;
}

bool valid_source_edge_cluster_key(const source_edge_cluster_key &key) noexcept {
  if (!valid_relation_feature_key(key.source_edge) ||
      key.source_edge.kind != relation_feature_kind::source_edge ||
      !enum_between(key.equivalence, 1, 4) || key.members.empty() ||
      !std::is_sorted(key.members.begin(), key.members.end()) ||
      std::adjacent_find(key.members.begin(), key.members.end()) !=
          key.members.end() ||
      key.tie_policy_version !=
          contract_versions::intersection_cluster_eligibility_policy ||
      key.schema_version != contract_versions::intersection_cluster_key_schema ||
      key.reserved != 0)
    return false;
  return std::all_of(key.members.begin(), key.members.end(),
                     valid_intersection_occurrence_key);
}

bool valid_source_edge_interval_key(const source_edge_interval_key &key) noexcept {
  if (!valid_relation_feature_key(key.source_edge) ||
      key.source_edge.kind != relation_feature_kind::source_edge ||
      !enum_between(key.left.kind, 1, 3) ||
      !enum_between(key.right.kind, 1, 3) ||
      !enum_between(key.interval_class, 1, 4) ||
      key.schema_version != contract_versions::intersection_interval_key_schema ||
      key.reserved != 0)
    return false;
  if (key.left.kind == boundary_reference_kind::cluster &&
      !valid_source_edge_cluster_key(key.left.cluster))
    return false;
  if (key.right.kind == boundary_reference_kind::cluster &&
      !valid_source_edge_cluster_key(key.right.cluster))
    return false;
  return key.left.kind != boundary_reference_kind::end_sentinel &&
         key.right.kind != boundary_reference_kind::start_sentinel &&
         !(key.right < key.left);
}

bool valid_intersection_descriptor_key(
    const intersection_descriptor_key &key) noexcept {
  if (!enum_between(key.locus, 1, 11) ||
      !enum_between(key.category, 1, 12) ||
      !valid_relation_feature_key(key.source_feature, true) ||
      key.orientation < -1 || key.orientation > 1 ||
      key.schema_version !=
          contract_versions::intersection_descriptor_key_schema ||
      key.reserved != 0)
    return false;
  if (key.locus == intersection_descriptor_locus::separated_contact_occurrence)
    return valid_intersection_occurrence_key(key.occurrence);
  return true;
}

intersection_event_key remap_intersection_event_key(
    intersection_event_key key) noexcept {
  key.first_operand = key.first_operand == operand_id::a ? operand_id::b
                                                         : operand_id::a;
  key.second_operand = key.second_operand == operand_id::a ? operand_id::b
                                                           : operand_id::a;
  key.first_owner = remap_relation_feature_key(key.first_owner);
  key.second_owner = remap_relation_feature_key(key.second_owner);
  key.public_relation = remap_seed_key(key.public_relation);
  if (key.authoritative_source_feature.kind != relation_feature_kind::none)
    key.authoritative_source_feature =
        remap_relation_feature_key(key.authoritative_source_feature);
  if (key.reused_source_vertex.kind != relation_feature_kind::none)
    key.reused_source_vertex =
        remap_relation_feature_key(key.reused_source_vertex);
  return key;
}

intersection_occurrence_key remap_intersection_occurrence_key(
    intersection_occurrence_key key) noexcept {
  key.event = remap_intersection_event_key(std::move(key.event));
  key.discriminator = remap_discriminator(key.discriminator);
  return key;
}

source_edge_membership_key remap_source_edge_membership_key_operands(
    source_edge_membership_key key) noexcept {
  key.source_edge = remap_relation_feature_key(key.source_edge);
  key.occurrence = remap_intersection_occurrence_key(std::move(key.occurrence));
  return key;
}

transverse_carrier_key remap_transverse_carrier_key(
    transverse_carrier_key key) noexcept {
  key.first_facet = remap_relation_feature_key(key.first_facet);
  key.second_facet = remap_relation_feature_key(key.second_facet);
  return key;
}

collinear_overlap_carrier_key remap_collinear_overlap_carrier_key(
    collinear_overlap_carrier_key key) noexcept {
  key.first_edge = remap_relation_feature_key(key.first_edge);
  key.second_edge = remap_relation_feature_key(key.second_edge);
  key.symbolic_owner = key.symbolic_owner == operand_id::a ? operand_id::b
                                                           : operand_id::a;
  return key;
}

intersection_descriptor_key remap_intersection_descriptor_key(
    intersection_descriptor_key key) noexcept {
  if (key.source_feature.kind != relation_feature_kind::none)
    key.source_feature = remap_relation_feature_key(key.source_feature);
  if (valid_intersection_occurrence_key(key.occurrence))
    key.occurrence = remap_intersection_occurrence_key(std::move(key.occurrence));
  key.orientation = static_cast<std::int8_t>(-key.orientation);
  return key;
}

source_edge_membership_key reverse_source_edge_membership_key(
    source_edge_membership_key key) noexcept {
  if (key.role == intersection_membership_role::overlap_start)
    key.role = intersection_membership_role::overlap_end;
  else if (key.role == intersection_membership_role::overlap_end)
    key.role = intersection_membership_role::overlap_start;
  if (key.facet_use_role == source_facet_use_role::left_incident)
    key.facet_use_role = source_facet_use_role::right_incident;
  else if (key.facet_use_role == source_facet_use_role::right_incident)
    key.facet_use_role = source_facet_use_role::left_incident;
  return key;
}

source_edge_cluster_key reverse_source_edge_cluster_key(
    source_edge_cluster_key key) noexcept {
  std::reverse(key.members.begin(), key.members.end());
  std::sort(key.members.begin(), key.members.end());
  return key;
}

source_edge_interval_key reverse_source_edge_interval_key(
    source_edge_interval_key key) noexcept {
  std::swap(key.left, key.right);
  const auto remap_boundary = [](source_edge_boundary_key &boundary) {
    if (boundary.kind == boundary_reference_kind::start_sentinel)
      boundary.kind = boundary_reference_kind::end_sentinel;
    else if (boundary.kind == boundary_reference_kind::end_sentinel)
      boundary.kind = boundary_reference_kind::start_sentinel;
    else
      boundary.cluster = reverse_source_edge_cluster_key(
          std::move(boundary.cluster));
  };
  remap_boundary(key.left);
  remap_boundary(key.right);
  return key;
}

void encode_intersection_event_key(canonical_writer &writer,
                                   const intersection_event_key &key) {
  encode_digest(writer, key.semantic_namespace);
  writer.u8(static_cast<std::uint8_t>(key.event_class));
  writer.u8(static_cast<std::uint8_t>(key.first_operand));
  writer.u8(static_cast<std::uint8_t>(key.second_operand));
  encode_relation_feature_key(writer, key.first_owner);
  encode_relation_feature_key(writer, key.second_owner);
  encode_relation_event_seed_key(writer, key.public_relation);
  writer.u8(static_cast<std::uint8_t>(key.construction_kind));
  writer.u8(static_cast<std::uint8_t>(key.construction_precedence));
  encode_relation_feature_key(writer, key.authoritative_source_feature);
  encode_relation_feature_key(writer, key.reused_source_vertex);
  writer.u64(key.construction_source_provenance);
  writer.u64(key.construction_geometric_lineage);
  writer.u8(static_cast<std::uint8_t>(key.carrier_role));
  writer.u8(static_cast<std::uint8_t>(key.contact_status));
  writer.u8(static_cast<std::uint8_t>(key.contact_dimension));
  writer.u64(key.symbolic_rule_ordinal);
  writer.u16(key.symbolic_tie_key_schema);
  writer.u16(key.equivalence_policy_version);
  writer.u16(key.schema_version);
  writer.u16(key.reserved);
}

void encode_intersection_occurrence_key(
    canonical_writer &writer, const intersection_occurrence_key &key) {
  encode_intersection_event_key(writer, key.event);
  encode_discriminator(writer, key.discriminator);
  writer.u16(key.policy_version);
  writer.u16(key.schema_version);
  writer.u32(key.reserved);
}

void encode_event_incidence_key(canonical_writer &writer,
                                const event_incidence_key &key) {
  encode_intersection_event_key(writer, key.event);
  encode_intersection_occurrence_key(writer, key.occurrence);
  encode_relation_event_seed_key(writer, key.seed);
  writer.u8(static_cast<std::uint8_t>(key.kind));
  encode_relation_feature_key(writer, key.feature);
  writer.u64(key.predecessor_relation);
  writer.u64(key.predecessor_candidate);
  writer.u64(key.proof_primary);
  writer.u64(key.proof_secondary);
  writer.u32(key.proof_occurrence);
  writer.u32(static_cast<std::uint32_t>(key.numeric_crossing));
  writer.u8(static_cast<std::uint8_t>(key.symbolic_crossing));
  writer.u8(static_cast<std::uint8_t>(key.orientation));
  writer.boolean(key.source_feature_owner);
  writer.boolean(key.bookkeeping_only);
  writer.u16(key.schema_version);
  writer.u16(key.reserved);
}

void encode_source_edge_membership_key(
    canonical_writer &writer, const source_edge_membership_key &key) {
  encode_relation_feature_key(writer, key.source_edge);
  encode_intersection_occurrence_key(writer, key.occurrence);
  writer.u8(static_cast<std::uint8_t>(key.role));
  writer.u64(key.parameter_construction.ordinal());
  writer.u64(key.parameter_lineage);
  writer.u64(key.relation_lineage);
  writer.u64(key.overlap_lineage);
  writer.u8(static_cast<std::uint8_t>(key.facet_use_role));
  writer.u16(key.schema_version);
  writer.u16(key.reserved);
}

void encode_transverse_carrier_key(canonical_writer &writer,
                                   const transverse_carrier_key &key) {
  encode_relation_feature_key(writer, key.first_facet);
  encode_relation_feature_key(writer, key.second_facet);
  writer.u64(key.construction.ordinal());
  writer.u64(key.construction_lineage);
  writer.u8(static_cast<std::uint8_t>(key.orientation));
  writer.u16(key.support_policy_version);
  writer.u16(key.schema_version);
  writer.u32(key.reserved);
}

void encode_collinear_overlap_carrier_key(
    canonical_writer &writer, const collinear_overlap_carrier_key &key) {
  encode_relation_feature_key(writer, key.first_edge);
  encode_relation_feature_key(writer, key.second_edge);
  writer.u64(key.coplanar_support_lineage);
  writer.u64(key.overlap_lineage);
  writer.boolean(key.opposite_direction);
  writer.u8(static_cast<std::uint8_t>(key.symbolic_owner));
  writer.u16(key.half_open_policy_version);
  writer.u16(key.schema_version);
  writer.u32(key.reserved);
}

void encode_source_edge_cluster_key(canonical_writer &writer,
                                    const source_edge_cluster_key &key) {
  encode_relation_feature_key(writer, key.source_edge);
  writer.u8(static_cast<std::uint8_t>(key.equivalence));
  writer.u64(key.members.size());
  for (const auto &member : key.members)
    encode_intersection_occurrence_key(writer, member);
  writer.u16(key.tie_policy_version);
  writer.u16(key.schema_version);
  writer.u32(key.reserved);
}

void encode_source_edge_interval_key(canonical_writer &writer,
                                     const source_edge_interval_key &key) {
  const auto encode_boundary = [&writer](const source_edge_boundary_key &boundary) {
    writer.u8(static_cast<std::uint8_t>(boundary.kind));
    if (boundary.kind == boundary_reference_kind::cluster)
      encode_source_edge_cluster_key(writer, boundary.cluster);
  };
  encode_relation_feature_key(writer, key.source_edge);
  encode_boundary(key.left);
  encode_boundary(key.right);
  writer.u64(key.canonical_ordinal);
  writer.u8(static_cast<std::uint8_t>(key.interval_class));
  writer.u16(key.schema_version);
  writer.u16(key.reserved);
}

void encode_intersection_descriptor_key(
    canonical_writer &writer, const intersection_descriptor_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.locus));
  writer.u8(static_cast<std::uint8_t>(key.category));
  encode_relation_feature_key(writer, key.source_feature);
  encode_intersection_occurrence_key(writer, key.occurrence);
  writer.u64(key.parent_lineage);
  writer.u64(key.boundary_ordinal);
  writer.u8(static_cast<std::uint8_t>(key.orientation));
  writer.u16(key.schema_version);
  writer.u16(key.reserved);
}

std::vector<std::uint8_t>
encode_intersection_event_key(const intersection_event_key &key) {
  canonical_writer writer;
  encode_intersection_event_key(writer, key);
  return writer.take();
}

std::vector<std::uint8_t>
encode_intersection_occurrence_key(const intersection_occurrence_key &key) {
  canonical_writer writer;
  encode_intersection_occurrence_key(writer, key);
  return writer.take();
}

} // namespace ygor::mesh_boolean::bounded
