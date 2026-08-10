#include "IntersectionCodec.h"

#include "Sha256.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace intersection_codec_detail {

constexpr std::uint32_t envelope_magic = 0x38434959U; // YIC8
constexpr std::uint16_t section_count = 8;
constexpr std::uint16_t section_version = 1;

bounded_boolean_error codec_error(const char *summary,
                                  bounded_boolean_error_category category =
                                      bounded_boolean_error_category::input_contract_error) {
  return intersection_error(intersection_subcode::codec_error, category,
                            summary, intersection_checkpoint::canonical_encoding);
}

bounded_boolean_error digest_error(const char *summary) {
  return intersection_error(intersection_subcode::digest_mismatch,
                            bounded_boolean_error_category::input_contract_error,
                            summary, intersection_checkpoint::canonical_encoding);
}

bounded_boolean_error resource_error(const char *summary) {
  return intersection_error(intersection_subcode::resource_preflight,
                            bounded_boolean_error_category::resource_limit,
                            summary, intersection_checkpoint::canonical_encoding);
}

void write_digest(canonical_writer &writer,
                  const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    writer.u8(byte);
}

bool read_digest(canonical_reader &reader, bounded_boolean_digest &digest) {
  for (auto &byte : digest.bytes)
    if (!reader.u8(byte))
      return false;
  return true;
}

void write_range(canonical_writer &writer, const intersection_range &range) {
  writer.u64(range.begin);
  writer.u64(range.count);
}

bool read_range(canonical_reader &reader, intersection_range &range) {
  return reader.u64(range.begin) && reader.u64(range.count);
}

template <class ID>
void write_id(canonical_writer &writer, ID id) {
  writer.u64(id.ordinal());
}

template <class ID>
bool read_id(canonical_reader &reader, ID &id) {
  std::uint64_t ordinal = 0;
  if (!reader.u64(ordinal))
    return false;
  id = ID{ordinal};
  return true;
}

template <class S, class U>
void write_signed(canonical_writer &writer, S value) {
  static_assert(sizeof(S) == sizeof(U));
  U bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  if constexpr (sizeof(U) == 1)
    writer.u8(bits);
  else if constexpr (sizeof(U) == 4)
    writer.u32(bits);
  else
    writer.u64(bits);
}

template <class S, class U>
bool read_signed(canonical_reader &reader, S &value) {
  static_assert(sizeof(S) == sizeof(U));
  U bits = 0;
  bool ok = false;
  if constexpr (sizeof(U) == 1)
    ok = reader.u8(bits);
  else if constexpr (sizeof(U) == 4)
    ok = reader.u32(bits);
  else
    ok = reader.u64(bits);
  if (!ok)
    return false;
  std::memcpy(&value, &bits, sizeof(value));
  return true;
}

template <class E>
void write_enum8(canonical_writer &writer, E value) {
  writer.u8(static_cast<std::uint8_t>(value));
}

template <class E>
bool read_enum8(canonical_reader &reader, E &value) {
  std::uint8_t raw = 0;
  if (!reader.u8(raw))
    return false;
  value = static_cast<E>(raw);
  return true;
}

void write_feature_key(canonical_writer &writer,
                       const relation_feature_key &key) {
  encode_relation_feature_key(writer, key);
}

bool read_feature_key(canonical_reader &reader, relation_feature_key &key,
                      bool allow_none = true) {
  std::uint8_t operand = 0, kind = 0;
  if (!reader.u8(operand) || !reader.u8(kind) || !reader.u64(key.primary) ||
      !reader.u64(key.secondary) || !reader.u32(key.occurrence) ||
      !reader.u16(key.schema_version))
    return false;
  key.operand = static_cast<operand_id>(operand);
  key.kind = static_cast<relation_feature_kind>(kind);
  return valid_relation_feature_key(key, allow_none);
}

void write_seed_key(canonical_writer &writer,
                    const relation_event_seed_key &key) {
  encode_relation_event_seed_key(writer, key);
}

bool read_seed_key(canonical_reader &reader, relation_event_seed_key &key) {
  std::uint8_t family = 0;
  if (!read_digest(reader, key.semantic_namespace) || !reader.u8(family) ||
      !read_feature_key(reader, key.first) ||
      !read_feature_key(reader, key.second) || !reader.u32(key.occurrence) ||
      !reader.u16(key.equivalence_policy_version) ||
      !reader.u16(key.reserved))
    return false;
  key.family = static_cast<feature_relation_family>(family);
  return valid_relation_event_seed_key(key);
}

bool read_occurrence_discriminator(
    canonical_reader &reader, intersection_occurrence_discriminator &value) {
  std::uint8_t role = 0, side = 0;
  if (!reader.u8(role) || !reader.u32(value.component07_occurrence) ||
      !reader.u64(value.first_shell) || !reader.u64(value.second_shell) ||
      !reader.u64(value.first_sheet) || !reader.u64(value.second_sheet) ||
      !reader.u8(side) || !reader.u32(value.symbolic_priority) ||
      !reader.u32(value.multiplicity_slot) ||
      !reader.u64(value.occurrence_lineage))
    return false;
  value.role = static_cast<occurrence_role>(role);
  value.symbolic_side = static_cast<symbolic_relation_side>(side);
  return true;
}

bool read_event_key(canonical_reader &reader, intersection_event_key &key) {
  std::uint8_t event_class = 0, first_operand = 0, second_operand = 0;
  std::uint8_t construction_kind = 0, construction_precedence = 0;
  std::uint8_t carrier_role = 0, contact_status = 0, contact_dimension = 0;
  if (!read_digest(reader, key.semantic_namespace) ||
      !reader.u8(event_class) || !reader.u8(first_operand) ||
      !reader.u8(second_operand) || !read_feature_key(reader, key.first_owner) ||
      !read_feature_key(reader, key.second_owner) ||
      !read_seed_key(reader, key.public_relation) ||
      !reader.u8(construction_kind) || !reader.u8(construction_precedence) ||
      !read_feature_key(reader, key.authoritative_source_feature) ||
      !read_feature_key(reader, key.reused_source_vertex, true) ||
      !reader.u64(key.construction_source_provenance) ||
      !reader.u64(key.construction_geometric_lineage) ||
      !reader.u8(carrier_role) || !reader.u8(contact_status) ||
      !reader.u8(contact_dimension) ||
      !reader.u64(key.symbolic_rule_ordinal) ||
      !reader.u16(key.symbolic_tie_key_schema) ||
      !reader.u16(key.equivalence_policy_version) ||
      !reader.u16(key.schema_version) || !reader.u16(key.reserved))
    return false;
  key.event_class = static_cast<intersection_event_class>(event_class);
  key.first_operand = static_cast<operand_id>(first_operand);
  key.second_operand = static_cast<operand_id>(second_operand);
  key.construction_kind =
      static_cast<relation_construction_kind>(construction_kind);
  key.construction_precedence =
      static_cast<relation_construction_precedence>(construction_precedence);
  key.carrier_role = static_cast<intersection_carrier_role>(carrier_role);
  key.contact_status = static_cast<feature_relation_status>(contact_status);
  key.contact_dimension =
      static_cast<relation_contact_dimension>(contact_dimension);
  return valid_intersection_event_key(key);
}

bool read_occurrence_key(canonical_reader &reader,
                         intersection_occurrence_key &key) {
  return read_event_key(reader, key.event) &&
         read_occurrence_discriminator(reader, key.discriminator) &&
         reader.u16(key.policy_version) && reader.u16(key.schema_version) &&
         reader.u32(key.reserved) && valid_intersection_occurrence_key(key);
}

bool read_incidence_key(canonical_reader &reader, event_incidence_key &key) {
  std::uint8_t kind = 0;
  if (!read_event_key(reader, key.event) ||
      !read_occurrence_key(reader, key.occurrence) ||
      !read_seed_key(reader, key.seed) || !reader.u8(kind) ||
      !read_feature_key(reader, key.feature, true) ||
      !reader.u64(key.predecessor_relation) ||
      !reader.u64(key.predecessor_candidate) ||
      !reader.u64(key.proof_primary) || !reader.u64(key.proof_secondary) ||
      !reader.u32(key.proof_occurrence) ||
      !read_signed<std::int32_t, std::uint32_t>(reader, key.numeric_crossing) ||
      !read_signed<std::int8_t, std::uint8_t>(reader, key.symbolic_crossing) ||
      !read_signed<std::int8_t, std::uint8_t>(reader, key.orientation) ||
      !reader.boolean(key.source_feature_owner) ||
      !reader.boolean(key.bookkeeping_only) ||
      !reader.u16(key.schema_version) || !reader.u16(key.reserved))
    return false;
  key.kind = static_cast<event_incidence_kind>(kind);
  return valid_event_incidence_key(key);
}

bool read_membership_key(canonical_reader &reader,
                         source_edge_membership_key &key) {
  std::uint8_t role = 0, facet_role = 0;
  if (!read_feature_key(reader, key.source_edge) ||
      !read_occurrence_key(reader, key.occurrence) || !reader.u8(role) ||
      !read_id(reader, key.parameter_evidence) ||
      !reader.u64(key.parameter_lineage) || !reader.u64(key.relation_lineage) ||
      !reader.u64(key.overlap_lineage) || !reader.u8(facet_role) ||
      !reader.u16(key.schema_version) || !reader.u16(key.reserved))
    return false;
  key.role = static_cast<intersection_membership_role>(role);
  key.facet_use_role = static_cast<source_facet_use_role>(facet_role);
  return valid_source_edge_membership_key(key);
}

bool read_transverse_key(canonical_reader &reader,
                         transverse_carrier_key &key) {
  std::uint8_t orientation = 0;
  if (!read_feature_key(reader, key.first_facet) ||
      !read_feature_key(reader, key.second_facet) ||
      !read_id(reader, key.construction) ||
      !reader.u64(key.construction_lineage) || !reader.u8(orientation) ||
      !reader.u16(key.support_policy_version) ||
      !reader.u16(key.schema_version) || !reader.u32(key.reserved))
    return false;
  key.orientation = static_cast<carrier_orientation_role>(orientation);
  return valid_transverse_carrier_key(key);
}

bool read_coplanar_support_key(canonical_reader &reader,
                               coplanar_support_key &key) {
  std::uint8_t owner = 0;
  if (!read_feature_key(reader, key.first_facet) ||
      !read_feature_key(reader, key.second_facet) ||
      !reader.u64(key.support_lineage) ||
      !reader.boolean(key.opposite_orientation) || !reader.u8(owner) ||
      !reader.u16(key.support_policy_version) ||
      !reader.u16(key.schema_version) || !reader.u32(key.reserved))
    return false;
  key.symbolic_owner = static_cast<operand_id>(owner);
  return valid_coplanar_support_key(key);
}

bool read_coplanar_overlap_key(canonical_reader &reader,
                               coplanar_overlap_key &key) {
  std::uint8_t kind = 0, owner = 0;
  if (!read_coplanar_support_key(reader, key.support) ||
      !reader.u64(key.component_lineage) || !reader.u8(kind) ||
      !reader.u8(key.sheet_mask) || !reader.u8(owner) ||
      !reader.u16(key.policy_version) || !reader.u16(key.schema_version) ||
      !reader.u32(key.reserved))
    return false;
  key.component_kind = static_cast<relation_coplanar_component_kind>(kind);
  key.symbolic_owner = static_cast<operand_id>(owner);
  return valid_coplanar_overlap_key(key);
}

bool read_overlap_carrier_key(canonical_reader &reader,
                              collinear_overlap_carrier_key &key) {
  std::uint8_t owner = 0;
  if (!read_feature_key(reader, key.first_edge) ||
      !read_feature_key(reader, key.second_edge) ||
      !reader.u64(key.coplanar_support_lineage) ||
      !reader.u64(key.overlap_lineage) ||
      !reader.boolean(key.opposite_direction) || !reader.u8(owner) ||
      !reader.u16(key.half_open_policy_version) ||
      !reader.u16(key.schema_version) || !reader.u32(key.reserved))
    return false;
  key.symbolic_owner = static_cast<operand_id>(owner);
  return valid_collinear_overlap_carrier_key(key);
}

struct decode_budget final {
  const intersection_codec_limits &limits;
  std::uint64_t allocations = 0;

  bool allocate(std::uint64_t count, bool index) {
    const auto maximum = index ? limits.maximum_index_entries
                               : limits.maximum_records_per_table;
    if (count > maximum || allocations == limits.maximum_vector_allocations)
      return false;
    ++allocations;
    return true;
  }
};

template <class T, class Reader>
bool read_record_vector(canonical_reader &reader, std::vector<T> &out,
                        decode_budget &budget, Reader read_record,
                        bool index = false) {
  std::uint64_t count = 0;
  if (!reader.u64(count) || !budget.allocate(count, index) ||
      count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    return false;
  std::vector<T> values;
  values.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t i = 0; i < count; ++i) {
    T value{};
    if (!read_record(reader, value))
      return false;
    values.push_back(std::move(value));
  }
  out = std::move(values);
  return true;
}

template <class T, class Writer>
void write_record_vector(canonical_writer &writer, const std::vector<T> &values,
                         Writer write_record) {
  writer.u64(values.size());
  for (const auto &value : values)
    write_record(writer, value);
}

template <class ID>
void write_id_vector(canonical_writer &writer, const std::vector<ID> &values) {
  writer.u64(values.size());
  for (const auto value : values)
    write_id(writer, value);
}

template <class ID>
bool read_id_vector(canonical_reader &reader, std::vector<ID> &out,
                    decode_budget &budget) {
  std::uint64_t count = 0;
  if (!reader.u64(count) || !budget.allocate(count, true) ||
      count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    return false;
  std::vector<ID> values;
  values.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t i = 0; i < count; ++i) {
    std::uint64_t ordinal = 0;
    if (!reader.u64(ordinal))
      return false;
    values.emplace_back(ordinal);
  }
  out = std::move(values);
  return true;
}

void write_feature_vector(canonical_writer &writer,
                          const std::vector<relation_feature_key> &values) {
  write_record_vector(writer, values, write_feature_key);
}

bool read_feature_vector(canonical_reader &reader,
                         std::vector<relation_feature_key> &out,
                         decode_budget &budget) {
  return read_record_vector(reader, out, budget,
                            [](canonical_reader &r, relation_feature_key &v) {
                              return read_feature_key(r, v, true);
                            }, true);
}

void write_range_vector(canonical_writer &writer,
                        const std::vector<intersection_range> &values) {
  write_record_vector(writer, values, write_range);
}

bool read_range_vector(canonical_reader &reader,
                       std::vector<intersection_range> &out,
                       decode_budget &budget) {
  return read_record_vector(reader, out, budget, read_range, true);
}

bool read_cluster_key(canonical_reader &reader, source_edge_cluster_key &key,
                      decode_budget &budget) {
  std::uint8_t equivalence = 0;
  if (!read_feature_key(reader, key.source_edge) || !reader.u8(equivalence))
    return false;
  key.equivalence = static_cast<intersection_cluster_equivalence>(equivalence);
  if (!read_record_vector(
          reader, key.members, budget,
          [](canonical_reader &r, intersection_occurrence_key &v) {
            return read_occurrence_key(r, v);
          }, true) ||
      !reader.u16(key.tie_policy_version) || !reader.u16(key.schema_version) ||
      !reader.u32(key.reserved))
    return false;
  return valid_source_edge_cluster_key(key);
}

bool read_boundary_key(canonical_reader &reader, source_edge_boundary_key &key,
                       decode_budget &budget) {
  std::uint8_t kind = 0;
  if (!reader.u8(kind))
    return false;
  key.kind = static_cast<boundary_reference_kind>(kind);
  switch (key.kind) {
  case boundary_reference_kind::start_sentinel:
  case boundary_reference_kind::end_sentinel:
    key.cluster = {};
    return true;
  case boundary_reference_kind::cluster:
    return read_cluster_key(reader, key.cluster, budget);
  }
  return false;
}

bool read_interval_key(canonical_reader &reader, source_edge_interval_key &key,
                       decode_budget &budget) {
  std::uint8_t interval_class = 0;
  if (!read_feature_key(reader, key.source_edge) ||
      !read_boundary_key(reader, key.left, budget) ||
      !read_boundary_key(reader, key.right, budget) ||
      !reader.u64(key.canonical_ordinal) || !reader.u8(interval_class) ||
      !reader.u16(key.schema_version) || !reader.u16(key.reserved))
    return false;
  key.interval_class = static_cast<intersection_interval_length>(interval_class);
  return valid_source_edge_interval_key(key);
}

bool read_descriptor_key(canonical_reader &reader,
                         intersection_descriptor_key &key) {
  std::uint8_t locus = 0, category = 0;
  std::vector<std::uint8_t> occurrence_bytes;
  const intersection_occurrence_key empty_occurrence{};
  const auto empty_occurrence_bytes =
      encode_intersection_occurrence_key(empty_occurrence);
  if (!reader.u8(locus) || !reader.u8(category) ||
      !read_feature_key(reader, key.source_feature, true) ||
      !reader.fixed_bytes(empty_occurrence_bytes.size(), occurrence_bytes) ||
      !reader.u64(key.parent_lineage) ||
      !reader.u64(key.boundary_ordinal) ||
      !read_signed<std::int8_t, std::uint8_t>(reader, key.orientation) ||
      !reader.u16(key.schema_version) || !reader.u16(key.reserved))
    return false;
  if (occurrence_bytes == empty_occurrence_bytes) {
    key.occurrence = empty_occurrence;
  } else {
    canonical_reader occurrence_reader(occurrence_bytes);
    if (!read_occurrence_key(occurrence_reader, key.occurrence) ||
        !occurrence_reader.complete())
      return false;
  }
  key.locus = static_cast<intersection_descriptor_locus>(locus);
  key.category = static_cast<intersection_descriptor_category>(category);
  return valid_intersection_descriptor_key(key);
}

void write_point_reference(canonical_writer &writer,
                           const bounded_point_reference &value) {
  write_enum8(writer, value.kind);
  write_feature_key(writer, value.source_vertex);
  write_id(writer, value.construction);
  write_id(writer, value.precision_ledger);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

bool read_point_reference(canonical_reader &reader,
                          bounded_point_reference &value) {
  return read_enum8(reader, value.kind) &&
         read_feature_key(reader, value.source_vertex, true) &&
         read_id(reader, value.construction) &&
         read_id(reader, value.precision_ledger) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16) &&
         reader.u32(value.reserved32);
}

void write_event(canonical_writer &writer,
                 const intersection_event_record &value) {
  write_id(writer, value.id);
  encode_intersection_event_key(writer, value.key);
  write_point_reference(writer, value.point);
  write_range(writer, value.construction_witnesses);
  write_range(writer, value.occurrences);
  write_range(writer, value.seed_bindings);
  write_range(writer, value.incidence);
  write_range(writer, value.crossing_aggregates);
  write_range(writer, value.contact_aggregates);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

bool read_event(canonical_reader &reader, intersection_event_record &value) {
  return read_id(reader, value.id) && read_event_key(reader, value.key) &&
         read_point_reference(reader, value.point) &&
         read_range(reader, value.construction_witnesses) &&
         read_range(reader, value.occurrences) &&
         read_range(reader, value.seed_bindings) &&
         read_range(reader, value.incidence) &&
         read_range(reader, value.crossing_aggregates) &&
         read_range(reader, value.contact_aggregates) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16) &&
         reader.u32(value.reserved32);
}

void write_occurrence(canonical_writer &writer,
                      const intersection_occurrence_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.event);
  encode_intersection_occurrence_key(writer, value.key);
  write_range(writer, value.seed_bindings);
  write_range(writer, value.incidence);
  write_range(writer, value.source_edge_memberships);
  write_range(writer, value.carrier_memberships);
  write_range(writer, value.cluster_memberships);
  write_range(writer, value.aggregate_contributions);
  write_range(writer, value.descriptors);
  writer.boolean(value.may_share_output_coordinate);
  writer.boolean(value.topology_separate);
  writer.boolean(value.local_cluster_compatible);
  writer.boolean(value.requires_contact_separation);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_occurrence(canonical_reader &reader,
                     intersection_occurrence_record &value) {
  return read_id(reader, value.id) && read_id(reader, value.event) &&
         read_occurrence_key(reader, value.key) &&
         read_range(reader, value.seed_bindings) &&
         read_range(reader, value.incidence) &&
         read_range(reader, value.source_edge_memberships) &&
         read_range(reader, value.carrier_memberships) &&
         read_range(reader, value.cluster_memberships) &&
         read_range(reader, value.aggregate_contributions) &&
         read_range(reader, value.descriptors) &&
         reader.boolean(value.may_share_output_coordinate) &&
         reader.boolean(value.topology_separate) &&
         reader.boolean(value.local_cluster_compatible) &&
         reader.boolean(value.requires_contact_separation) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_seed_binding(canonical_writer &writer,
                        const event_seed_binding_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.seed);
  write_seed_key(writer, value.seed_key);
  writer.u64(value.canonical_seed_ordinal);
  write_id(writer, value.event);
  write_id(writer, value.occurrence);
  write_id(writer, value.relation);
  write_id(writer, value.construction);
  write_feature_key(writer, value.accepted_source_vertex);
  write_range(writer, value.incidence);
  write_range(writer, value.candidate_incidence);
  write_enum8(writer, value.expected_membership_role);
  write_enum8(writer, value.expected_carrier_role);
  writer.boolean(value.designated_authority);
  writer.boolean(value.duplicate_consumer);
  writer.boolean(value.compatibility_verified);
  writer.u8(value.reserved8);
  writer.u16(value.schema_version);
  writer.u32(value.reserved32);
}

bool read_seed_binding(canonical_reader &reader,
                       event_seed_binding_record &value) {
  return read_id(reader, value.id) && read_id(reader, value.seed) &&
         read_seed_key(reader, value.seed_key) &&
         reader.u64(value.canonical_seed_ordinal) &&
         read_id(reader, value.event) && read_id(reader, value.occurrence) &&
         read_id(reader, value.relation) && read_id(reader, value.construction) &&
         read_feature_key(reader, value.accepted_source_vertex, true) &&
         read_range(reader, value.incidence) &&
         read_range(reader, value.candidate_incidence) &&
         read_enum8(reader, value.expected_membership_role) &&
         read_enum8(reader, value.expected_carrier_role) &&
         reader.boolean(value.designated_authority) &&
         reader.boolean(value.duplicate_consumer) &&
         reader.boolean(value.compatibility_verified) &&
         reader.u8(value.reserved8) && reader.u16(value.schema_version) &&
         reader.u32(value.reserved32);
}

void write_incidence(canonical_writer &writer,
                     const event_incidence_record &value) {
  write_id(writer, value.id);
  encode_event_incidence_key(writer, value.key);
  write_id(writer, value.event);
  write_id(writer, value.occurrence);
  write_id(writer, value.seed_binding);
  write_enum8(writer, value.kind);
  write_feature_key(writer, value.feature);
  write_id(writer, value.relation);
  write_id(writer, value.candidate);
  writer.u64(value.payload_primary);
  writer.u64(value.payload_secondary);
  writer.u32(value.payload_occurrence);
  write_signed<std::int32_t, std::uint32_t>(writer, value.numeric_crossing);
  write_signed<std::int8_t, std::uint8_t>(writer, value.symbolic_crossing);
  write_signed<std::int8_t, std::uint8_t>(writer, value.orientation);
  writer.boolean(value.source_feature_owner);
  writer.boolean(value.bookkeeping_only);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_incidence(canonical_reader &reader, event_incidence_record &value) {
  return read_id(reader, value.id) && read_incidence_key(reader, value.key) &&
         read_id(reader, value.event) && read_id(reader, value.occurrence) &&
         read_id(reader, value.seed_binding) && read_enum8(reader, value.kind) &&
         read_feature_key(reader, value.feature, true) &&
         read_id(reader, value.relation) && read_id(reader, value.candidate) &&
         reader.u64(value.payload_primary) &&
         reader.u64(value.payload_secondary) &&
         reader.u32(value.payload_occurrence) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.numeric_crossing) &&
         read_signed<std::int8_t, std::uint8_t>(reader,
                                                value.symbolic_crossing) &&
         read_signed<std::int8_t, std::uint8_t>(reader, value.orientation) &&
         reader.boolean(value.source_feature_owner) &&
         reader.boolean(value.bookkeeping_only) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_source_feature_range(
    canonical_writer &writer,
    const source_feature_incidence_range_record &value) {
  write_enum8(writer, value.kind);
  write_feature_key(writer, value.feature);
  write_range(writer, value.incidence);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_source_feature_range(
    canonical_reader &reader, source_feature_incidence_range_record &value) {
  return read_enum8(reader, value.kind) &&
         read_feature_key(reader, value.feature, true) &&
         read_range(reader, value.incidence) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_halfedge_range(
    canonical_writer &writer,
    const oriented_halfedge_incidence_range_record &value) {
  write_enum8(writer, value.operand);
  writer.u64(value.halfedge);
  write_range(writer, value.incidence);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

bool read_halfedge_range(
    canonical_reader &reader, oriented_halfedge_incidence_range_record &value) {
  return read_enum8(reader, value.operand) && reader.u64(value.halfedge) &&
         read_range(reader, value.incidence) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16) &&
         reader.u32(value.reserved32);
}

void write_source_edge_membership(
    canonical_writer &writer, const source_edge_membership_record &value) {
  write_id(writer, value.id);
  encode_source_edge_membership_key(writer, value.key);
  write_id(writer, value.occurrence);
  write_id(writer, value.event);
  write_id(writer, value.parameter);
  write_range(writer, value.contributions);
  write_range(writer, value.incident_facet_uses);
  write_id(writer, value.ordering_certificate);
  writer.boolean(value.exact_equal_eligible);
  writer.boolean(value.cluster_eligible);
  writer.boolean(value.internal_diagonal_discovery);
  writer.boolean(value.bookkeeping_only);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_source_edge_membership(
    canonical_reader &reader, source_edge_membership_record &value) {
  return read_id(reader, value.id) && read_membership_key(reader, value.key) &&
         read_id(reader, value.occurrence) && read_id(reader, value.event) &&
         read_id(reader, value.parameter) &&
         read_range(reader, value.contributions) &&
         read_range(reader, value.incident_facet_uses) &&
         read_id(reader, value.ordering_certificate) &&
         reader.boolean(value.exact_equal_eligible) &&
         reader.boolean(value.cluster_eligible) &&
         reader.boolean(value.internal_diagonal_discovery) &&
         reader.boolean(value.bookkeeping_only) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_sentinel(canonical_writer &writer,
                    const source_edge_endpoint_sentinel_record &value) {
  write_enum8(writer, value.side);
  write_feature_key(writer, value.source_vertex);
  write_feature_key(writer, value.source_edge);
}

bool read_sentinel(canonical_reader &reader,
                   source_edge_endpoint_sentinel_record &value) {
  return read_enum8(reader, value.side) &&
         read_feature_key(reader, value.source_vertex) &&
         read_feature_key(reader, value.source_edge);
}

void write_source_edge_sequence(canonical_writer &writer,
                                const source_edge_sequence_record &value) {
  write_id(writer, value.id);
  write_feature_key(writer, value.source_edge);
  write_sentinel(writer, value.start);
  write_sentinel(writer, value.end);
  write_range(writer, value.clusters);
  write_range(writer, value.memberships);
  write_range(writer, value.intervals);
  write_range(writer, value.aggregates);
  write_range(writer, value.descriptors);
  write_digest(writer, value.sequence_digest);
  writer.u64(value.comparison_count);
  writer.boolean(value.canonical_forward);
  writer.u8(value.reserved8);
  writer.u16(value.schema_version);
  writer.u32(value.reserved32);
}

bool read_source_edge_sequence(canonical_reader &reader,
                               source_edge_sequence_record &value) {
  return read_id(reader, value.id) &&
         read_feature_key(reader, value.source_edge) &&
         read_sentinel(reader, value.start) && read_sentinel(reader, value.end) &&
         read_range(reader, value.clusters) &&
         read_range(reader, value.memberships) &&
         read_range(reader, value.intervals) &&
         read_range(reader, value.aggregates) &&
         read_range(reader, value.descriptors) &&
         read_digest(reader, value.sequence_digest) &&
         reader.u64(value.comparison_count) &&
         reader.boolean(value.canonical_forward) && reader.u8(value.reserved8) &&
         reader.u16(value.schema_version) && reader.u32(value.reserved32);
}

void write_source_edge_cluster(canonical_writer &writer,
                               const source_edge_cluster_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.sequence);
  encode_source_edge_cluster_key(writer, value.key);
  write_range(writer, value.member_occurrences);
  write_range(writer, value.membership_ids);
  write_range(writer, value.contributions);
  write_id(writer, value.predecessor);
  write_id(writer, value.successor);
  write_id(writer, value.ordering_certificate);
  writer.boolean(value.shared_output_coordinate);
  writer.boolean(value.separate_output_occurrences);
  writer.u16(value.schema_version);
}

bool read_source_edge_cluster(canonical_reader &reader,
                              source_edge_cluster_record &value,
                              decode_budget &budget) {
  return read_id(reader, value.id) && read_id(reader, value.sequence) &&
         read_cluster_key(reader, value.key, budget) &&
         read_range(reader, value.member_occurrences) &&
         read_range(reader, value.membership_ids) &&
         read_range(reader, value.contributions) &&
         read_id(reader, value.predecessor) && read_id(reader, value.successor) &&
         read_id(reader, value.ordering_certificate) &&
         reader.boolean(value.shared_output_coordinate) &&
         reader.boolean(value.separate_output_occurrences) &&
         reader.u16(value.schema_version);
}

void write_source_edge_interval(canonical_writer &writer,
                                const source_edge_interval_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.sequence);
  encode_source_edge_interval_key(writer, value.key);
  write_id(writer, value.left_parameter);
  write_id(writer, value.right_parameter);
  write_enum8(writer, value.length_disposition);
  write_signed<std::int32_t, std::uint32_t>(writer, value.left_crossing_delta);
  write_signed<std::int32_t, std::uint32_t>(writer, value.right_crossing_delta);
  write_signed<std::int32_t, std::uint32_t>(writer, value.accumulated_crossing);
  writer.boolean(value.propagation_allowed);
  writer.boolean(value.retention_allowed);
  writer.boolean(value.split_required);
  writer.boolean(value.duplicate_required);
  write_range(writer, value.provenance);
  write_range(writer, value.descriptors);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_source_edge_interval(canonical_reader &reader,
                               source_edge_interval_record &value,
                               decode_budget &budget) {
  return read_id(reader, value.id) && read_id(reader, value.sequence) &&
         read_interval_key(reader, value.key, budget) &&
         read_id(reader, value.left_parameter) &&
         read_id(reader, value.right_parameter) &&
         read_enum8(reader, value.length_disposition) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.left_crossing_delta) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.right_crossing_delta) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.accumulated_crossing) &&
         reader.boolean(value.propagation_allowed) &&
         reader.boolean(value.retention_allowed) &&
         reader.boolean(value.split_required) &&
         reader.boolean(value.duplicate_required) &&
         read_range(reader, value.provenance) &&
         read_range(reader, value.descriptors) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_transverse_carrier(canonical_writer &writer,
                              const transverse_carrier_record &value) {
  write_id(writer, value.id);
  encode_transverse_carrier_key(writer, value.key);
  write_id(writer, value.construction);
  write_range(writer, value.relation_provenance);
  write_range(writer, value.candidate_provenance);
  write_range(writer, value.memberships);
  write_range(writer, value.clusters);
  write_range(writer, value.active_spans);
  write_range(writer, value.region_incidence);
  write_range(writer, value.aggregates);
  write_range(writer, value.descriptors);
  write_digest(writer, value.carrier_digest);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

bool read_transverse_carrier(canonical_reader &reader,
                             transverse_carrier_record &value) {
  return read_id(reader, value.id) && read_transverse_key(reader, value.key) &&
         read_id(reader, value.construction) &&
         read_range(reader, value.relation_provenance) &&
         read_range(reader, value.candidate_provenance) &&
         read_range(reader, value.memberships) &&
         read_range(reader, value.clusters) &&
         read_range(reader, value.active_spans) &&
         read_range(reader, value.region_incidence) &&
         read_range(reader, value.aggregates) &&
         read_range(reader, value.descriptors) &&
         read_digest(reader, value.carrier_digest) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16) &&
         reader.u32(value.reserved32);
}

void write_carrier_membership(canonical_writer &writer,
                              const carrier_membership_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.carrier);
  write_id(writer, value.occurrence);
  write_id(writer, value.event);
  write_id(writer, value.parameter);
  writer.u64(value.relation_lineage);
  write_id(writer, value.ordering_certificate);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

bool read_carrier_membership(canonical_reader &reader,
                             carrier_membership_record &value) {
  return read_id(reader, value.id) && read_id(reader, value.carrier) &&
         read_id(reader, value.occurrence) && read_id(reader, value.event) &&
         read_id(reader, value.parameter) &&
         reader.u64(value.relation_lineage) &&
         read_id(reader, value.ordering_certificate) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16) &&
         reader.u32(value.reserved32);
}

void write_carrier_cluster(canonical_writer &writer,
                           const carrier_cluster_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.carrier);
  write_enum8(writer, value.equivalence);
  write_range(writer, value.occurrence_members);
  write_range(writer, value.membership_members);
  write_id(writer, value.predecessor);
  write_id(writer, value.successor);
  write_id(writer, value.ordering_certificate);
  writer.boolean(value.shared_output_coordinate);
  writer.boolean(value.separate_output_occurrences);
  writer.u16(value.schema_version);
}

bool read_carrier_cluster(canonical_reader &reader,
                          carrier_cluster_record &value) {
  return read_id(reader, value.id) && read_id(reader, value.carrier) &&
         read_enum8(reader, value.equivalence) &&
         read_range(reader, value.occurrence_members) &&
         read_range(reader, value.membership_members) &&
         read_id(reader, value.predecessor) && read_id(reader, value.successor) &&
         read_id(reader, value.ordering_certificate) &&
         reader.boolean(value.shared_output_coordinate) &&
         reader.boolean(value.separate_output_occurrences) &&
         reader.u16(value.schema_version);
}

void write_active_span(canonical_writer &writer,
                       const carrier_active_span_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.carrier);
  write_id(writer, value.left);
  write_id(writer, value.right);
  write_enum8(writer, value.activation);
  writer.u64(value.relation_interval_lineage);
  write_range(writer, value.region_incidence);
  write_range(writer, value.provenance);
  writer.boolean(value.classification_cut);
  writer.boolean(value.contact_delimiter);
  writer.boolean(value.output_edge_allowed);
  writer.u8(value.reserved8);
  writer.u16(value.schema_version);
}

bool read_active_span(canonical_reader &reader,
                      carrier_active_span_record &value) {
  return read_id(reader, value.id) && read_id(reader, value.carrier) &&
         read_id(reader, value.left) && read_id(reader, value.right) &&
         read_enum8(reader, value.activation) &&
         reader.u64(value.relation_interval_lineage) &&
         read_range(reader, value.region_incidence) &&
         read_range(reader, value.provenance) &&
         reader.boolean(value.classification_cut) &&
         reader.boolean(value.contact_delimiter) &&
         reader.boolean(value.output_edge_allowed) &&
         reader.u8(value.reserved8) && reader.u16(value.schema_version);
}

void write_coplanar_support(canonical_writer &writer,
                            const coplanar_support_record &value) {
  write_id(writer, value.id);
  encode_coplanar_support_key(writer, value.key);
  write_feature_key(writer, value.first_facet);
  write_feature_key(writer, value.second_facet);
  writer.u64(value.support_lineage);
  writer.boolean(value.opposite_orientation);
  write_enum8(writer, value.symbolic_owner);
  write_range(writer, value.original_boundary_edges);
  write_range(writer, value.boundary_events);
  write_range(writer, value.boundary_carriers);
  write_range(writer, value.overlap_components);
  write_range(writer, value.region_incidence);
  write_range(writer, value.provenance);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_coplanar_support(canonical_reader &reader,
                           coplanar_support_record &value) {
  return read_id(reader, value.id) &&
         read_coplanar_support_key(reader, value.key) &&
         read_feature_key(reader, value.first_facet) &&
         read_feature_key(reader, value.second_facet) &&
         reader.u64(value.support_lineage) &&
         reader.boolean(value.opposite_orientation) &&
         read_enum8(reader, value.symbolic_owner) &&
         read_range(reader, value.original_boundary_edges) &&
         read_range(reader, value.boundary_events) &&
         read_range(reader, value.boundary_carriers) &&
         read_range(reader, value.overlap_components) &&
         read_range(reader, value.region_incidence) &&
         read_range(reader, value.provenance) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_overlap_carrier(canonical_writer &writer,
                           const collinear_overlap_carrier_record &value) {
  write_id(writer, value.id);
  encode_collinear_overlap_carrier_key(writer, value.key);
  write_id(writer, value.first_parameter_interval);
  write_id(writer, value.second_parameter_interval);
  write_id(writer, value.first_parameter_evidence);
  write_id(writer, value.second_parameter_evidence);
  write_id(writer, value.start_occurrence);
  write_id(writer, value.end_occurrence);
  write_feature_key(writer, value.start_source_vertex);
  write_feature_key(writer, value.end_source_vertex);
  write_enum8(writer, value.symbolic_owner);
  writer.boolean(value.half_open_first);
  writer.boolean(value.half_open_second);
  writer.boolean(value.separate_sheet_required);
  writer.boolean(value.parameter_correspondence_verified);
  writer.boolean(value.zero_length);
  writer.u8(value.reserved8);
  write_range(writer, value.provenance);
  write_range(writer, value.source_provenance);
  write_range(writer, value.contributions);
  write_range(writer, value.descriptors);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_overlap_carrier(canonical_reader &reader,
                          collinear_overlap_carrier_record &value) {
  return read_id(reader, value.id) &&
         read_overlap_carrier_key(reader, value.key) &&
         read_id(reader, value.first_parameter_interval) &&
         read_id(reader, value.second_parameter_interval) &&
         read_id(reader, value.first_parameter_evidence) &&
         read_id(reader, value.second_parameter_evidence) &&
         read_id(reader, value.start_occurrence) &&
         read_id(reader, value.end_occurrence) &&
         read_feature_key(reader, value.start_source_vertex, true) &&
         read_feature_key(reader, value.end_source_vertex, true) &&
         read_enum8(reader, value.symbolic_owner) &&
         reader.boolean(value.half_open_first) &&
         reader.boolean(value.half_open_second) &&
         reader.boolean(value.separate_sheet_required) &&
         reader.boolean(value.parameter_correspondence_verified) &&
         reader.boolean(value.zero_length) && reader.u8(value.reserved8) &&
         read_range(reader, value.provenance) &&
         read_range(reader, value.source_provenance) &&
         read_range(reader, value.contributions) &&
         read_range(reader, value.descriptors) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_coplanar_overlap(canonical_writer &writer,
                            const coplanar_overlap_record &value) {
  write_id(writer, value.id);
  encode_coplanar_overlap_key(writer, value.key);
  write_id(writer, value.support);
  write_id(writer, value.component07_component);
  write_enum8(writer, value.kind);
  write_enum8(writer, value.symbolic_owner);
  writer.u8(value.sheet_mask);
  writer.boolean(value.closed);
  writer.boolean(value.distinct_sheet_occurrences);
  writer.boolean(value.zero_measure);
  write_range(writer, value.boundary_events);
  write_range(writer, value.boundary_carriers);
  write_range(writer, value.provenance);
  write_digest(writer, value.component_digest);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_coplanar_overlap(canonical_reader &reader,
                           coplanar_overlap_record &value) {
  return read_id(reader, value.id) &&
         read_coplanar_overlap_key(reader, value.key) &&
         read_id(reader, value.support) &&
         read_id(reader, value.component07_component) &&
         read_enum8(reader, value.kind) &&
         read_enum8(reader, value.symbolic_owner) &&
         reader.u8(value.sheet_mask) && reader.boolean(value.closed) &&
         reader.boolean(value.distinct_sheet_occurrences) &&
         reader.boolean(value.zero_measure) &&
         read_range(reader, value.boundary_events) &&
         read_range(reader, value.boundary_carriers) &&
         read_range(reader, value.provenance) &&
         read_digest(reader, value.component_digest) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_region_incidence(canonical_writer &writer,
                            const coplanar_region_incidence_record &value) {
  write_id(writer, value.id);
  write_id(writer, value.support);
  write_feature_key(writer, value.first_facet);
  write_feature_key(writer, value.second_facet);
  write_feature_key(writer, value.first_triangle);
  write_feature_key(writer, value.second_triangle);
  write_id(writer, value.component);
  writer.u64(value.component_lineage);
  write_enum8(writer, value.classification);
  write_enum8(writer, value.relation_status);
  write_enum8(writer, value.symbolic_owner);
  writer.u8(value.sheet_mask);
  writer.boolean(value.internal_diagonal_coverage_only);
  writer.boolean(value.coverage_complete);
  writer.u8(value.reserved8);
  write_range(writer, value.boundary_events);
  write_range(writer, value.boundary_carriers);
  write_range(writer, value.coverage_witnesses);
  write_digest(writer, value.source_facet_semantic_digest);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_region_incidence(canonical_reader &reader,
                           coplanar_region_incidence_record &value) {
  return read_id(reader, value.id) && read_id(reader, value.support) &&
         read_feature_key(reader, value.first_facet) &&
         read_feature_key(reader, value.second_facet) &&
         read_feature_key(reader, value.first_triangle) &&
         read_feature_key(reader, value.second_triangle) &&
         read_id(reader, value.component) &&
         reader.u64(value.component_lineage) &&
         read_enum8(reader, value.classification) &&
         read_enum8(reader, value.relation_status) &&
         read_enum8(reader, value.symbolic_owner) &&
         reader.u8(value.sheet_mask) &&
         reader.boolean(value.internal_diagonal_coverage_only) &&
         reader.boolean(value.coverage_complete) &&
         reader.u8(value.reserved8) &&
         read_range(reader, value.boundary_events) &&
         read_range(reader, value.boundary_carriers) &&
         read_range(reader, value.coverage_witnesses) &&
         read_digest(reader, value.source_facet_semantic_digest) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_crossing_subtotal(canonical_writer &writer,
                             const crossing_subtotal_record &value) {
  write_feature_key(writer, value.source_feature);
  write_signed<std::int64_t, std::uint64_t>(writer, value.numeric_signed_sum);
  write_signed<std::int32_t, std::uint32_t>(writer, value.symbolic_signed_sum);
  write_enum8(writer, value.symbolic_owner);
  writer.u8(value.symbolic_owner_mask);
  writer.boolean(value.mixed_symbolic_ownership);
  write_range(writer, value.members);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_crossing_subtotal(canonical_reader &reader,
                            crossing_subtotal_record &value) {
  return read_feature_key(reader, value.source_feature, true) &&
         read_signed<std::int64_t, std::uint64_t>(reader,
                                                  value.numeric_signed_sum) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.symbolic_signed_sum) &&
         read_enum8(reader, value.symbolic_owner) &&
         reader.u8(value.symbolic_owner_mask) &&
         reader.boolean(value.mixed_symbolic_ownership) &&
         read_range(reader, value.members) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_crossing_aggregate(canonical_writer &writer,
                              const crossing_aggregate_record &value) {
  write_id(writer, value.id);
  write_enum8(writer, value.locus);
  write_feature_key(writer, value.source_feature);
  writer.u64(value.locus_ordinal);
  write_signed<std::int64_t, std::uint64_t>(writer, value.numeric_signed_sum);
  write_signed<std::int32_t, std::uint32_t>(writer, value.symbolic_signed_sum);
  write_signed<std::int32_t, std::uint32_t>(writer, value.entering_count);
  write_signed<std::int32_t, std::uint32_t>(writer, value.leaving_count);
  write_enum8(writer, value.symbolic_owner);
  writer.u8(value.symbolic_owner_mask);
  writer.boolean(value.mixed_symbolic_ownership);
  writer.boolean(value.zero_net_contact_retained);
  write_range(writer, value.members);
  write_range(writer, value.facet_subtotals);
  write_range(writer, value.shell_subtotals);
  writer.boolean(value.member_order_verified);
  writer.boolean(value.conserved);
  writer.u16(value.schema_version);
  writer.u8(value.reserved8);
}

bool read_crossing_aggregate(canonical_reader &reader,
                             crossing_aggregate_record &value) {
  return read_id(reader, value.id) && read_enum8(reader, value.locus) &&
         read_feature_key(reader, value.source_feature, true) &&
         reader.u64(value.locus_ordinal) &&
         read_signed<std::int64_t, std::uint64_t>(reader,
                                                  value.numeric_signed_sum) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.symbolic_signed_sum) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.entering_count) &&
         read_signed<std::int32_t, std::uint32_t>(reader,
                                                  value.leaving_count) &&
         read_enum8(reader, value.symbolic_owner) &&
         reader.u8(value.symbolic_owner_mask) &&
         reader.boolean(value.mixed_symbolic_ownership) &&
         reader.boolean(value.zero_net_contact_retained) &&
         read_range(reader, value.members) &&
         read_range(reader, value.facet_subtotals) &&
         read_range(reader, value.shell_subtotals) &&
         reader.boolean(value.member_order_verified) &&
         reader.boolean(value.conserved) && reader.u16(value.schema_version) &&
         reader.u8(value.reserved8);
}

void write_contact_aggregate(canonical_writer &writer,
                             const contact_aggregate_record &value) {
  write_id(writer, value.id);
  write_enum8(writer, value.locus);
  write_feature_key(writer, value.source_feature);
  writer.u64(value.locus_ordinal);
  write_enum8(writer, value.contact_status);
  write_enum8(writer, value.contact_dimension);
  write_enum8(writer, value.symbolic_owner);
  writer.u8(value.symbolic_owner_mask);
  writer.boolean(value.mixed_symbolic_ownership);
  write_range(writer, value.members);
  writer.boolean(value.zero_net_retained);
  writer.boolean(value.tangent_retained);
  writer.boolean(value.coincidence_retained);
  writer.boolean(value.reconstructed);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
}

bool read_contact_aggregate(canonical_reader &reader,
                            contact_aggregate_record &value) {
  return read_id(reader, value.id) && read_enum8(reader, value.locus) &&
         read_feature_key(reader, value.source_feature, true) &&
         reader.u64(value.locus_ordinal) &&
         read_enum8(reader, value.contact_status) &&
         read_enum8(reader, value.contact_dimension) &&
         read_enum8(reader, value.symbolic_owner) &&
         reader.u8(value.symbolic_owner_mask) &&
         reader.boolean(value.mixed_symbolic_ownership) &&
         read_range(reader, value.members) &&
         reader.boolean(value.zero_net_retained) &&
         reader.boolean(value.tangent_retained) &&
         reader.boolean(value.coincidence_retained) &&
         reader.boolean(value.reconstructed) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16);
}

void write_descriptor(canonical_writer &writer,
                      const intersection_descriptor_record &value) {
  write_id(writer, value.id);
  encode_intersection_descriptor_key(writer, value.key);
  write_signed<std::int32_t, std::uint32_t>(writer,
                                            value.signed_crossing_delta);
  write_enum8(writer, value.symbolic_owner);
  writer.u64(value.symbolic_rule_ordinal);
  write_range(writer, value.provenance);
  writer.boolean(value.continuation_allowed);
  writer.boolean(value.occurrence_separation_required);
  writer.boolean(value.classification_consumable);
  writer.boolean(value.selection_consumable);
  writer.boolean(value.topology_consumable);
  writer.u8(value.reserved8);
  writer.u16(value.schema_version);
}

bool read_descriptor(canonical_reader &reader,
                     intersection_descriptor_record &value) {
  return read_id(reader, value.id) && read_descriptor_key(reader, value.key) &&
         read_signed<std::int32_t, std::uint32_t>(
             reader, value.signed_crossing_delta) &&
         read_enum8(reader, value.symbolic_owner) &&
         reader.u64(value.symbolic_rule_ordinal) &&
         read_range(reader, value.provenance) &&
         reader.boolean(value.continuation_allowed) &&
         reader.boolean(value.occurrence_separation_required) &&
         reader.boolean(value.classification_consumable) &&
         reader.boolean(value.selection_consumable) &&
         reader.boolean(value.topology_consumable) &&
         reader.u8(value.reserved8) && reader.u16(value.schema_version);
}

void write_ordering_certificate(canonical_writer &writer,
                                const ordering_certificate_record &value) {
  write_id(writer, value.id);
  write_enum8(writer, value.disposition);
  write_id(writer, value.first_parameter);
  write_id(writer, value.second_parameter);
  writer.u64(value.exact_evidence_lineage);
  writer.u64(value.comparison_evidence_lineage);
  writer.boolean(value.topology_safe);
  writer.u16(value.policy_version);
  writer.u8(value.reserved8);
}

bool read_ordering_certificate(canonical_reader &reader,
                               ordering_certificate_record &value) {
  return read_id(reader, value.id) && read_enum8(reader, value.disposition) &&
         read_id(reader, value.first_parameter) &&
         read_id(reader, value.second_parameter) &&
         reader.u64(value.exact_evidence_lineage) &&
         reader.u64(value.comparison_evidence_lineage) &&
         reader.boolean(value.topology_safe) &&
         reader.u16(value.policy_version) && reader.u8(value.reserved8);
}

void write_verification_evidence(
    canonical_writer &writer, const intersection_verification_evidence &value) {
  writer.u16(value.schema_version);
  writer.u16(value.verifier_version);
  writer.boolean(value.seed_regrouped);
  writer.boolean(value.incidence_reconstructed);
  writer.boolean(value.arrangements_reconstructed);
  writer.boolean(value.descriptors_reconstructed);
  writer.boolean(value.exhaustive_mode);
  writer.u8(value.reserved8);
  write_digest(writer, value.reconstructed_digest);
  writer.u64(value.work_units);
}

bool read_verification_evidence(
    canonical_reader &reader, intersection_verification_evidence &value) {
  return reader.u16(value.schema_version) &&
         reader.u16(value.verifier_version) &&
         reader.boolean(value.seed_regrouped) &&
         reader.boolean(value.incidence_reconstructed) &&
         reader.boolean(value.arrangements_reconstructed) &&
         reader.boolean(value.descriptors_reconstructed) &&
         reader.boolean(value.exhaustive_mode) && reader.u8(value.reserved8) &&
         read_digest(reader, value.reconstructed_digest) &&
         reader.u64(value.work_units);
}

void write_diagnostic(canonical_writer &writer,
                      const intersection_diagnostic_record &value) {
  write_id(writer, value.id);
  writer.u32(static_cast<std::uint32_t>(value.checkpoint));
  writer.u32(static_cast<std::uint32_t>(value.subcode));
  for (const auto witness : value.witnesses)
    writer.u64(witness);
  writer.u8(value.witness_count);
  writer.boolean(value.retained_finding);
  writer.u16(value.schema_version);
  writer.u32(value.reserved32);
}

bool read_diagnostic(canonical_reader &reader,
                     intersection_diagnostic_record &value) {
  std::uint32_t checkpoint = 0, subcode = 0;
  if (!read_id(reader, value.id) || !reader.u32(checkpoint) ||
      !reader.u32(subcode))
    return false;
  value.checkpoint = static_cast<intersection_checkpoint>(checkpoint);
  value.subcode = static_cast<intersection_subcode>(subcode);
  for (auto &witness : value.witnesses)
    if (!reader.u64(witness))
      return false;
  return reader.u8(value.witness_count) &&
         reader.boolean(value.retained_finding) &&
         reader.u16(value.schema_version) && reader.u32(value.reserved32);
}

void write_replay_checkpoint(
    canonical_writer &writer,
    const intersection_replay_checkpoint_record &value) {
  write_id(writer, value.id);
  writer.u32(static_cast<std::uint32_t>(value.checkpoint));
  write_digest(writer, value.input_digest);
  write_digest(writer, value.output_digest);
  writer.u64(value.input_count);
  writer.u64(value.output_count);
  writer.u64(value.cumulative_work_units);
  writer.u16(value.schema_version);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

bool read_replay_checkpoint(
    canonical_reader &reader, intersection_replay_checkpoint_record &value) {
  std::uint32_t checkpoint = 0;
  if (!read_id(reader, value.id) || !reader.u32(checkpoint))
    return false;
  value.checkpoint = static_cast<intersection_checkpoint>(checkpoint);
  return read_digest(reader, value.input_digest) &&
         read_digest(reader, value.output_digest) &&
         reader.u64(value.input_count) && reader.u64(value.output_count) &&
         reader.u64(value.cumulative_work_units) &&
         reader.u16(value.schema_version) && reader.u16(value.reserved16) &&
         reader.u32(value.reserved32);
}

void write_statistics(canonical_writer &writer,
                      const intersection_statistics &value) {
  writer.u64(value.seed_count);
  writer.u64(value.event_count);
  writer.u64(value.occurrence_count);
  writer.u64(value.seed_binding_count);
  writer.u64(value.incidence_count);
  writer.u64(value.source_edge_membership_count);
  writer.u64(value.source_edge_sequence_count);
  writer.u64(value.source_edge_cluster_count);
  writer.u64(value.source_edge_interval_count);
  writer.u64(value.transverse_carrier_count);
  writer.u64(value.carrier_membership_count);
  writer.u64(value.carrier_cluster_count);
  writer.u64(value.carrier_span_count);
  writer.u64(value.coplanar_support_count);
  writer.u64(value.overlap_count);
  writer.u64(value.aggregate_count);
  writer.u64(value.descriptor_count);
  writer.u64(value.ordering_certificate_count);
  writer.u64(value.diagnostic_count);
  writer.u64(value.replay_checkpoint_count);
  writer.u64(value.sort_comparisons);
  writer.u64(value.verifier_work_units);
  writer.u64(value.persistent_bytes);
  writer.u64(value.canonical_bytes);
}

bool read_statistics(canonical_reader &reader, intersection_statistics &value) {
  return reader.u64(value.seed_count) && reader.u64(value.event_count) &&
         reader.u64(value.occurrence_count) &&
         reader.u64(value.seed_binding_count) &&
         reader.u64(value.incidence_count) &&
         reader.u64(value.source_edge_membership_count) &&
         reader.u64(value.source_edge_sequence_count) &&
         reader.u64(value.source_edge_cluster_count) &&
         reader.u64(value.source_edge_interval_count) &&
         reader.u64(value.transverse_carrier_count) &&
         reader.u64(value.carrier_membership_count) &&
         reader.u64(value.carrier_cluster_count) &&
         reader.u64(value.carrier_span_count) &&
         reader.u64(value.coplanar_support_count) &&
         reader.u64(value.overlap_count) &&
         reader.u64(value.aggregate_count) &&
         reader.u64(value.descriptor_count) &&
         reader.u64(value.ordering_certificate_count) &&
         reader.u64(value.diagnostic_count) &&
         reader.u64(value.replay_checkpoint_count) &&
         reader.u64(value.sort_comparisons) &&
         reader.u64(value.verifier_work_units) &&
         reader.u64(value.persistent_bytes) &&
         reader.u64(value.canonical_bytes);
}

void begin_section(canonical_writer &writer, std::uint16_t tag) {
  writer.u16(tag);
  writer.u16(section_version);
  writer.u32(0);
}

bool begin_section(canonical_reader &reader, std::uint16_t expected_tag) {
  std::uint16_t tag = 0, version = 0;
  std::uint32_t reserved = 0;
  return reader.u16(tag) && reader.u16(version) && reader.u32(reserved) &&
         tag == expected_tag && version == section_version && reserved == 0;
}

} // namespace intersection_codec_detail

struct intersection_codec_access final {
  template <class T, class I>
  static bool encode_sections(
      const canonical_intersection_complex<T, I> &artifact,
      std::array<std::vector<std::uint8_t>, 8> &sections) {
    using namespace intersection_codec_detail;
    {
      canonical_writer writer;
      begin_section(writer, 1);
      writer.boolean(static_cast<bool>(artifact.owner_.anchor));
      write_enum8(writer, artifact.operation_);
      write_enum8(writer, artifact.provider_);
      write_enum8(writer, artifact.verification_);
      writer.u8(0);
      writer.u16(artifact.schema_version_);
      writer.u16(artifact.provider_version_);
      writer.u16(artifact.semantic_policy_version_);
      writer.u16(artifact.codec_version_);
      writer.u16(artifact.verifier_version_);
      writer.u16(0);
      writer.u32(artifact.reserved_);
      write_digest(writer, artifact.context_digest_);
      write_digest(writer, artifact.precision_digest_);
      write_digest(writer, artifact.relation_digest_);
      for (const auto &digest : artifact.source_semantic_digests_)
        write_digest(writer, digest);
      for (const auto &digest : artifact.exact_triangulation_digests_)
        write_digest(writer, digest);
      sections[0] = writer.take();
    }
    {
      canonical_writer writer;
      begin_section(writer, 2);
      write_record_vector(writer, artifact.events_, write_event);
      write_record_vector(writer, artifact.occurrences_, write_occurrence);
      write_id_vector(writer, artifact.construction_witness_index_);
      sections[1] = writer.take();
    }
    {
      canonical_writer writer;
      begin_section(writer, 3);
      write_record_vector(writer, artifact.seed_bindings_, write_seed_binding);
      write_id_vector(writer, artifact.event_binding_index_);
      write_id_vector(writer, artifact.occurrence_binding_index_);
      write_id_vector(writer, artifact.seed_to_event_);
      write_id_vector(writer, artifact.seed_to_occurrence_);
      write_record_vector(writer, artifact.incidence_, write_incidence);
      write_id_vector(writer, artifact.incidence_by_event_);
      write_id_vector(writer, artifact.incidence_by_occurrence_);
      write_id_vector(writer, artifact.incidence_by_seed_);
      write_id_vector(writer, artifact.incidence_by_seed_candidate_);
      write_range_vector(writer, artifact.event_incidence_ranges_);
      write_range_vector(writer, artifact.occurrence_incidence_ranges_);
      write_range_vector(writer, artifact.seed_incidence_ranges_);
      write_range_vector(writer, artifact.seed_candidate_incidence_ranges_);
      write_id_vector(writer, artifact.incidence_by_relation_);
      write_range_vector(writer, artifact.relation_incidence_ranges_);
      write_id_vector(writer, artifact.incidence_by_candidate_);
      write_range_vector(writer, artifact.candidate_incidence_ranges_);
      write_id_vector(writer, artifact.incidence_by_source_feature_);
      write_record_vector(writer, artifact.source_feature_incidence_ranges_,
                          write_source_feature_range);
      write_id_vector(writer, artifact.incidence_by_halfedge_);
      write_record_vector(writer, artifact.halfedge_incidence_ranges_,
                          write_halfedge_range);
      sections[2] = writer.take();
    }
    {
      canonical_writer writer;
      begin_section(writer, 4);
      write_record_vector(writer, artifact.source_edge_memberships_,
                          write_source_edge_membership);
      write_record_vector(writer, artifact.source_edge_sequences_,
                          write_source_edge_sequence);
      write_record_vector(writer, artifact.source_edge_clusters_,
                          write_source_edge_cluster);
      write_record_vector(writer, artifact.source_edge_intervals_,
                          write_source_edge_interval);
      write_id_vector(writer, artifact.source_edge_membership_sequence_index_);
      write_id_vector(writer, artifact.source_edge_cluster_occurrence_index_);
      write_id_vector(writer, artifact.source_edge_cluster_membership_index_);
      write_id_vector(writer, artifact.source_edge_sequence_cluster_index_);
      write_id_vector(writer, artifact.source_edge_sequence_interval_index_);
      write_record_vector(writer, artifact.ordering_certificates_,
                          write_ordering_certificate);
      sections[3] = writer.take();
    }
    {
      canonical_writer writer;
      begin_section(writer, 5);
      write_record_vector(writer, artifact.transverse_carriers_,
                          write_transverse_carrier);
      write_record_vector(writer, artifact.carrier_memberships_,
                          write_carrier_membership);
      write_record_vector(writer, artifact.carrier_clusters_,
                          write_carrier_cluster);
      write_record_vector(writer, artifact.carrier_active_spans_,
                          write_active_span);
      write_id_vector(writer, artifact.carrier_relation_provenance_);
      write_id_vector(writer, artifact.carrier_candidate_provenance_);
      write_id_vector(writer, artifact.carrier_membership_index_);
      write_id_vector(writer, artifact.carrier_cluster_occurrence_index_);
      write_id_vector(writer, artifact.carrier_cluster_membership_index_);
      write_id_vector(writer, artifact.transverse_carrier_cluster_index_);
      write_id_vector(writer, artifact.transverse_carrier_span_index_);
      write_id_vector(writer, artifact.carrier_span_relation_provenance_);
      write_id_vector(writer, artifact.carrier_span_region_incidence_);
      sections[4] = writer.take();
    }
    {
      canonical_writer writer;
      begin_section(writer, 6);
      write_record_vector(writer, artifact.coplanar_supports_,
                          write_coplanar_support);
      write_record_vector(writer, artifact.overlap_carriers_,
                          write_overlap_carrier);
      write_record_vector(writer, artifact.coplanar_overlaps_,
                          write_coplanar_overlap);
      write_record_vector(writer, artifact.coplanar_region_incidence_,
                          write_region_incidence);
      write_id_vector(writer, artifact.coplanar_support_relation_provenance_);
      write_id_vector(writer, artifact.coplanar_support_candidate_provenance_);
      write_feature_vector(
          writer, artifact.coplanar_support_original_boundary_edge_index_);
      write_id_vector(writer, artifact.coplanar_support_boundary_event_index_);
      write_id_vector(writer,
                      artifact.coplanar_support_boundary_carrier_index_);
      write_id_vector(writer, artifact.coplanar_support_overlap_index_);
      write_id_vector(writer, artifact.coplanar_support_region_index_);
      write_id_vector(writer, artifact.overlap_carrier_relation_provenance_);
      write_id_vector(writer, artifact.overlap_carrier_candidate_provenance_);
      write_feature_vector(writer,
                           artifact.overlap_carrier_source_provenance_);
      write_id_vector(writer,
                      artifact.coplanar_overlap_boundary_event_index_);
      write_id_vector(writer,
                      artifact.coplanar_overlap_boundary_carrier_index_);
      write_id_vector(writer, artifact.coplanar_overlap_relation_provenance_);
      write_id_vector(writer, artifact.coplanar_region_boundary_event_index_);
      write_id_vector(writer,
                      artifact.coplanar_region_boundary_carrier_index_);
      write_feature_vector(
          writer, artifact.coplanar_region_coverage_witness_index_);
      sections[5] = writer.take();
    }
    {
      canonical_writer writer;
      begin_section(writer, 7);
      write_record_vector(writer, artifact.crossing_aggregates_,
                          write_crossing_aggregate);
      write_id_vector(writer, artifact.crossing_aggregate_members_);
      write_record_vector(writer, artifact.crossing_facet_subtotals_,
                          write_crossing_subtotal);
      write_id_vector(writer, artifact.crossing_facet_subtotal_members_);
      write_record_vector(writer, artifact.crossing_shell_subtotals_,
                          write_crossing_subtotal);
      write_id_vector(writer, artifact.crossing_shell_subtotal_members_);
      write_record_vector(writer, artifact.contact_aggregates_,
                          write_contact_aggregate);
      write_id_vector(writer, artifact.contact_aggregate_members_);
      write_record_vector(writer, artifact.descriptors_, write_descriptor);
      write_id_vector(writer, artifact.descriptor_provenance_);
      sections[6] = writer.take();
    }
    {
      canonical_writer writer;
      begin_section(writer, 8);
      write_record_vector(writer, artifact.diagnostics_, write_diagnostic);
      write_record_vector(writer, artifact.replay_checkpoints_,
                          write_replay_checkpoint);
      write_statistics(writer, artifact.statistics_);
      write_verification_evidence(writer, artifact.verification_evidence_);
      sections[7] = writer.take();
    }
    return true;
  }

  template <class T, class I>
  static bool decode_sections(
      const std::array<std::vector<std::uint8_t>, 8> &sections,
      const intersection_canonicalization_header &expectations,
      const intersection_codec_limits &limits,
      canonical_intersection_complex<T, I> &artifact) {
    using namespace intersection_codec_detail;
    decode_budget budget{limits};
    {
      canonical_reader reader(sections[0]);
      bool owner_bound = false;
      std::uint8_t operation = 0, provider = 0, verification = 0, reserved8 = 0;
      std::uint16_t reserved16 = 0;
      if (!begin_section(reader, 1) || !reader.boolean(owner_bound) ||
          !reader.u8(operation) || !reader.u8(provider) ||
          !reader.u8(verification) || !reader.u8(reserved8) ||
          !reader.u16(artifact.schema_version_) ||
          !reader.u16(artifact.provider_version_) ||
          !reader.u16(artifact.semantic_policy_version_) ||
          !reader.u16(artifact.codec_version_) ||
          !reader.u16(artifact.verifier_version_) || !reader.u16(reserved16) ||
          !reader.u32(artifact.reserved_) ||
          !read_digest(reader, artifact.context_digest_) ||
          !read_digest(reader, artifact.precision_digest_) ||
          !read_digest(reader, artifact.relation_digest_))
        return false;
      artifact.operation_ = static_cast<boolean_operation>(operation);
      artifact.provider_ = static_cast<intersection_provider_kind>(provider);
      artifact.verification_ =
          static_cast<intersection_verification_disposition>(verification);
      for (auto &digest : artifact.source_semantic_digests_)
        if (!read_digest(reader, digest))
          return false;
      for (auto &digest : artifact.exact_triangulation_digests_)
        if (!read_digest(reader, digest))
          return false;
      if (!reader.complete() || !owner_bound || reserved8 != 0 ||
          reserved16 != 0 || artifact.reserved_ != 0 ||
          !expectations.owner.anchor ||
          artifact.operation_ != expectations.operation ||
          artifact.schema_version_ != expectations.schema_version ||
          artifact.provider_version_ != expectations.provider_version ||
          artifact.semantic_policy_version_ !=
              expectations.semantic_policy_version ||
          artifact.codec_version_ != expectations.codec_version ||
          artifact.verifier_version_ != expectations.verifier_version ||
          artifact.context_digest_ != expectations.context_digest ||
          artifact.precision_digest_ != expectations.precision_digest ||
          artifact.relation_digest_ != expectations.relation_digest ||
          artifact.source_semantic_digests_ !=
              expectations.source_semantic_digests ||
          artifact.exact_triangulation_digests_ !=
              expectations.exact_triangulation_digests)
        return false;
      artifact.owner_ = expectations.owner;
    }
    {
      canonical_reader reader(sections[1]);
      if (!begin_section(reader, 2) ||
          !read_record_vector(reader, artifact.events_, budget, read_event) ||
          !read_record_vector(reader, artifact.occurrences_, budget,
                              read_occurrence) ||
          !read_id_vector(reader, artifact.construction_witness_index_, budget) ||
          !reader.complete())
        return false;
    }
    {
      canonical_reader reader(sections[2]);
      if (!begin_section(reader, 3) ||
          !read_record_vector(reader, artifact.seed_bindings_, budget,
                              read_seed_binding) ||
          !read_id_vector(reader, artifact.event_binding_index_, budget) ||
          !read_id_vector(reader, artifact.occurrence_binding_index_, budget) ||
          !read_id_vector(reader, artifact.seed_to_event_, budget) ||
          !read_id_vector(reader, artifact.seed_to_occurrence_, budget) ||
          !read_record_vector(reader, artifact.incidence_, budget,
                              read_incidence) ||
          !read_id_vector(reader, artifact.incidence_by_event_, budget) ||
          !read_id_vector(reader, artifact.incidence_by_occurrence_, budget) ||
          !read_id_vector(reader, artifact.incidence_by_seed_, budget) ||
          !read_id_vector(reader, artifact.incidence_by_seed_candidate_, budget) ||
          !read_range_vector(reader, artifact.event_incidence_ranges_, budget) ||
          !read_range_vector(reader, artifact.occurrence_incidence_ranges_,
                             budget) ||
          !read_range_vector(reader, artifact.seed_incidence_ranges_, budget) ||
          !read_range_vector(reader,
                             artifact.seed_candidate_incidence_ranges_, budget) ||
          !read_id_vector(reader, artifact.incidence_by_relation_, budget) ||
          !read_range_vector(reader, artifact.relation_incidence_ranges_,
                             budget) ||
          !read_id_vector(reader, artifact.incidence_by_candidate_, budget) ||
          !read_range_vector(reader, artifact.candidate_incidence_ranges_,
                             budget) ||
          !read_id_vector(reader, artifact.incidence_by_source_feature_,
                          budget) ||
          !read_record_vector(reader,
                              artifact.source_feature_incidence_ranges_, budget,
                              read_source_feature_range, true) ||
          !read_id_vector(reader, artifact.incidence_by_halfedge_, budget) ||
          !read_record_vector(reader, artifact.halfedge_incidence_ranges_,
                              budget, read_halfedge_range, true) ||
          !reader.complete())
        return false;
    }
    {
      canonical_reader reader(sections[3]);
      if (!begin_section(reader, 4) ||
          !read_record_vector(reader, artifact.source_edge_memberships_, budget,
                              read_source_edge_membership) ||
          !read_record_vector(reader, artifact.source_edge_sequences_, budget,
                              read_source_edge_sequence) ||
          !read_record_vector(
              reader, artifact.source_edge_clusters_, budget,
              [&](canonical_reader &r, source_edge_cluster_record &v) {
                return read_source_edge_cluster(r, v, budget);
              }) ||
          !read_record_vector(
              reader, artifact.source_edge_intervals_, budget,
              [&](canonical_reader &r, source_edge_interval_record &v) {
                return read_source_edge_interval(r, v, budget);
              }) ||
          !read_id_vector(reader,
                          artifact.source_edge_membership_sequence_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.source_edge_cluster_occurrence_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.source_edge_cluster_membership_index_,
                          budget) ||
          !read_id_vector(reader, artifact.source_edge_sequence_cluster_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.source_edge_sequence_interval_index_,
                          budget) ||
          !read_record_vector(reader, artifact.ordering_certificates_, budget,
                              read_ordering_certificate) ||
          !reader.complete())
        return false;
    }
    {
      canonical_reader reader(sections[4]);
      if (!begin_section(reader, 5) ||
          !read_record_vector(reader, artifact.transverse_carriers_, budget,
                              read_transverse_carrier) ||
          !read_record_vector(reader, artifact.carrier_memberships_, budget,
                              read_carrier_membership) ||
          !read_record_vector(reader, artifact.carrier_clusters_, budget,
                              read_carrier_cluster) ||
          !read_record_vector(reader, artifact.carrier_active_spans_, budget,
                              read_active_span) ||
          !read_id_vector(reader, artifact.carrier_relation_provenance_,
                          budget) ||
          !read_id_vector(reader, artifact.carrier_candidate_provenance_,
                          budget) ||
          !read_id_vector(reader, artifact.carrier_membership_index_, budget) ||
          !read_id_vector(reader, artifact.carrier_cluster_occurrence_index_,
                          budget) ||
          !read_id_vector(reader, artifact.carrier_cluster_membership_index_,
                          budget) ||
          !read_id_vector(reader, artifact.transverse_carrier_cluster_index_,
                          budget) ||
          !read_id_vector(reader, artifact.transverse_carrier_span_index_,
                          budget) ||
          !read_id_vector(reader, artifact.carrier_span_relation_provenance_,
                          budget) ||
          !read_id_vector(reader, artifact.carrier_span_region_incidence_,
                          budget) ||
          !reader.complete())
        return false;
    }
    {
      canonical_reader reader(sections[5]);
      if (!begin_section(reader, 6) ||
          !read_record_vector(reader, artifact.coplanar_supports_, budget,
                              read_coplanar_support) ||
          !read_record_vector(reader, artifact.overlap_carriers_, budget,
                              read_overlap_carrier) ||
          !read_record_vector(reader, artifact.coplanar_overlaps_, budget,
                              read_coplanar_overlap) ||
          !read_record_vector(reader, artifact.coplanar_region_incidence_,
                              budget, read_region_incidence) ||
          !read_id_vector(reader,
                          artifact.coplanar_support_relation_provenance_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_support_candidate_provenance_,
                          budget) ||
          !read_feature_vector(
              reader, artifact.coplanar_support_original_boundary_edge_index_,
              budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_support_boundary_event_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_support_boundary_carrier_index_,
                          budget) ||
          !read_id_vector(reader, artifact.coplanar_support_overlap_index_,
                          budget) ||
          !read_id_vector(reader, artifact.coplanar_support_region_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.overlap_carrier_relation_provenance_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.overlap_carrier_candidate_provenance_,
                          budget) ||
          !read_feature_vector(reader,
                               artifact.overlap_carrier_source_provenance_,
                               budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_overlap_boundary_event_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_overlap_boundary_carrier_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_overlap_relation_provenance_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_region_boundary_event_index_,
                          budget) ||
          !read_id_vector(reader,
                          artifact.coplanar_region_boundary_carrier_index_,
                          budget) ||
          !read_feature_vector(
              reader, artifact.coplanar_region_coverage_witness_index_,
              budget) ||
          !reader.complete())
        return false;
    }
    {
      canonical_reader reader(sections[6]);
      if (!begin_section(reader, 7) ||
          !read_record_vector(reader, artifact.crossing_aggregates_, budget,
                              read_crossing_aggregate) ||
          !read_id_vector(reader, artifact.crossing_aggregate_members_,
                          budget) ||
          !read_record_vector(reader, artifact.crossing_facet_subtotals_,
                              budget, read_crossing_subtotal) ||
          !read_id_vector(reader,
                          artifact.crossing_facet_subtotal_members_, budget) ||
          !read_record_vector(reader, artifact.crossing_shell_subtotals_,
                              budget, read_crossing_subtotal) ||
          !read_id_vector(reader,
                          artifact.crossing_shell_subtotal_members_, budget) ||
          !read_record_vector(reader, artifact.contact_aggregates_, budget,
                              read_contact_aggregate) ||
          !read_id_vector(reader, artifact.contact_aggregate_members_,
                          budget) ||
          !read_record_vector(reader, artifact.descriptors_, budget,
                              read_descriptor) ||
          !read_id_vector(reader, artifact.descriptor_provenance_, budget) ||
          !reader.complete())
        return false;
    }
    {
      canonical_reader reader(sections[7]);
      if (!begin_section(reader, 8) ||
          !read_record_vector(reader, artifact.diagnostics_, budget,
                              read_diagnostic) ||
          !read_record_vector(reader, artifact.replay_checkpoints_, budget,
                              read_replay_checkpoint) ||
          !read_statistics(reader, artifact.statistics_) ||
          !read_verification_evidence(reader,
                                      artifact.verification_evidence_) ||
          !reader.complete())
        return false;
    }
    return true;
  }

  template <class T, class I>
  static bool validate_projection(
      const canonical_intersection_complex<T, I> &a,
      bool require_encoded_bytes) noexcept {
    const auto checked_range = [](intersection_range range,
                                  std::size_t size) noexcept {
      return range.begin <= size &&
             range.count <= size - static_cast<std::size_t>(range.begin);
    };
    const auto valid_operation = [](boolean_operation operation) noexcept {
      switch (operation) {
      case boolean_operation::set_union:
      case boolean_operation::intersection:
      case boolean_operation::a_minus_b:
      case boolean_operation::b_minus_a:
      case boolean_operation::symmetric_difference:
        return true;
      }
      return false;
    };
    const auto operand_valid = [](operand_id operand) noexcept {
      return operand == operand_id::a || operand == operand_id::b;
    };
    const auto dense = [](const auto &records) noexcept {
      for (std::size_t i = 0; i < records.size(); ++i)
        if (records[i].id.ordinal() != i)
          return false;
      return true;
    };
    const auto ids_valid = [](const auto &ids, std::size_t size) noexcept {
      for (const auto id : ids)
        if (id.ordinal() >= size)
          return false;
      return true;
    };
    const auto ranges_valid = [&](const auto &ranges,
                                  std::size_t size) noexcept {
      for (const auto range : ranges)
        if (!checked_range(range, size))
          return false;
      return true;
    };
    const auto optional_id_valid = [](const auto id,
                                      std::size_t size) noexcept {
      return id.ordinal() == intersection_invalid_ordinal ||
             id.ordinal() < size;
    };

    if (!a.owner_.anchor || !valid_operation(a.operation_) ||
        a.provider_ !=
            intersection_provider_kind::canonical_lineage_event_arrangement_v1 ||
        (a.verification_ !=
             intersection_verification_disposition::not_verified &&
         a.verification_ !=
             intersection_verification_disposition::independently_verified) ||
        a.schema_version_ != contract_versions::intersection_artifact_schema ||
        a.provider_version_ != contract_versions::intersection_provider ||
        a.semantic_policy_version_ !=
            contract_versions::intersection_semantic_policy ||
        a.codec_version_ != contract_versions::intersection_codec ||
        a.verifier_version_ != contract_versions::intersection_verifier ||
        a.reserved_ != 0)
      return false;

    if (!dense(a.events_) || !dense(a.occurrences_) ||
        !dense(a.seed_bindings_) || !dense(a.incidence_) ||
        !dense(a.source_edge_memberships_) ||
        !dense(a.source_edge_sequences_) || !dense(a.source_edge_clusters_) ||
        !dense(a.source_edge_intervals_) || !dense(a.transverse_carriers_) ||
        !dense(a.carrier_memberships_) || !dense(a.carrier_clusters_) ||
        !dense(a.carrier_active_spans_) || !dense(a.coplanar_supports_) ||
        !dense(a.overlap_carriers_) || !dense(a.coplanar_overlaps_) ||
        !dense(a.coplanar_region_incidence_) ||
        !dense(a.crossing_aggregates_) || !dense(a.contact_aggregates_) ||
        !dense(a.descriptors_) || !dense(a.ordering_certificates_) ||
        !dense(a.diagnostics_) || !dense(a.replay_checkpoints_))
      return false;

    if (a.seed_to_event_.size() != a.seed_bindings_.size() ||
        a.seed_to_occurrence_.size() != a.seed_bindings_.size() ||
        !ids_valid(a.event_binding_index_, a.seed_bindings_.size()) ||
        !ids_valid(a.occurrence_binding_index_, a.seed_bindings_.size()) ||
        !ids_valid(a.seed_to_event_, a.events_.size()) ||
        !ids_valid(a.seed_to_occurrence_, a.occurrences_.size()))
      return false;

    for (const auto &event : a.events_) {
      if (!valid_intersection_event_key(event.key) ||
          event.schema_version != contract_versions::intersection_event_schema ||
          event.reserved16 != 0 || event.reserved32 != 0 ||
          event.point.schema_version !=
              contract_versions::intersection_event_schema ||
          event.point.reserved16 != 0 || event.point.reserved32 != 0 ||
          !checked_range(event.construction_witnesses,
                         a.construction_witness_index_.size()) ||
          !checked_range(event.occurrences, a.occurrences_.size()) ||
          !checked_range(event.seed_bindings, a.event_binding_index_.size()) ||
          !checked_range(event.incidence, a.incidence_by_event_.size()) ||
          !checked_range(event.crossing_aggregates,
                         a.crossing_aggregates_.size()) ||
          !checked_range(event.contact_aggregates,
                         a.contact_aggregates_.size()))
        return false;
    }
    for (const auto &occurrence : a.occurrences_) {
      if (occurrence.event.ordinal() >= a.events_.size() ||
          !valid_intersection_occurrence_key(occurrence.key) ||
          occurrence.schema_version !=
              contract_versions::intersection_occurrence_schema ||
          occurrence.reserved16 != 0 ||
          !checked_range(occurrence.seed_bindings,
                         a.occurrence_binding_index_.size()) ||
          !checked_range(occurrence.incidence,
                         a.incidence_by_occurrence_.size()) ||
          !checked_range(occurrence.source_edge_memberships,
                         a.source_edge_memberships_.size()) ||
          !checked_range(occurrence.carrier_memberships,
                         a.carrier_memberships_.size()) ||
          !checked_range(occurrence.cluster_memberships,
                         a.source_edge_clusters_.size() +
                             a.carrier_clusters_.size()) ||
          !checked_range(occurrence.aggregate_contributions,
                         a.incidence_.size()) ||
          !checked_range(occurrence.descriptors, a.descriptors_.size()))
        return false;
    }
    for (const auto &binding : a.seed_bindings_) {
      if (!valid_relation_event_seed_key(binding.seed_key) ||
          binding.seed.ordinal() >= a.seed_bindings_.size() ||
          binding.event.ordinal() >= a.events_.size() ||
          binding.occurrence.ordinal() >= a.occurrences_.size() ||
          !checked_range(binding.incidence, a.incidence_by_seed_.size()) ||
          !checked_range(binding.candidate_incidence,
                         a.incidence_by_seed_candidate_.size()) ||
          binding.schema_version !=
              contract_versions::intersection_seed_binding_schema ||
          binding.reserved8 != 0 || binding.reserved32 != 0)
        return false;
    }

    if (!ids_valid(a.incidence_by_event_, a.incidence_.size()) ||
        !ids_valid(a.incidence_by_occurrence_, a.incidence_.size()) ||
        !ids_valid(a.incidence_by_seed_, a.incidence_.size()) ||
        !ids_valid(a.incidence_by_seed_candidate_, a.incidence_.size()) ||
        !ids_valid(a.incidence_by_relation_, a.incidence_.size()) ||
        !ids_valid(a.incidence_by_candidate_, a.incidence_.size()) ||
        !ids_valid(a.incidence_by_source_feature_, a.incidence_.size()) ||
        !ids_valid(a.incidence_by_halfedge_, a.incidence_.size()) ||
        !ranges_valid(a.event_incidence_ranges_, a.incidence_by_event_.size()) ||
        !ranges_valid(a.occurrence_incidence_ranges_,
                      a.incidence_by_occurrence_.size()) ||
        !ranges_valid(a.seed_incidence_ranges_, a.incidence_by_seed_.size()) ||
        !ranges_valid(a.seed_candidate_incidence_ranges_,
                      a.incidence_by_seed_candidate_.size()) ||
        !ranges_valid(a.relation_incidence_ranges_,
                      a.incidence_by_relation_.size()) ||
        !ranges_valid(a.candidate_incidence_ranges_,
                      a.incidence_by_candidate_.size()))
      return false;
    for (const auto &record : a.incidence_) {
      if (!valid_event_incidence_key(record.key) ||
          record.event.ordinal() >= a.events_.size() ||
          record.occurrence.ordinal() >= a.occurrences_.size() ||
          record.seed_binding.ordinal() >= a.seed_bindings_.size() ||
          record.schema_version != contract_versions::intersection_incidence_schema ||
          record.reserved16 != 0)
        return false;
    }
    for (const auto &range : a.source_feature_incidence_ranges_)
      if (!checked_range(range.incidence,
                         a.incidence_by_source_feature_.size()) ||
          range.schema_version !=
              contract_versions::intersection_incidence_schema ||
          range.reserved16 != 0)
        return false;
    for (const auto &range : a.halfedge_incidence_ranges_)
      if (!operand_valid(range.operand) ||
          !checked_range(range.incidence, a.incidence_by_halfedge_.size()) ||
          range.schema_version !=
              contract_versions::intersection_incidence_schema ||
          range.reserved16 != 0 || range.reserved32 != 0)
        return false;

    if (!ids_valid(a.source_edge_membership_sequence_index_,
                   a.source_edge_memberships_.size()) ||
        !ids_valid(a.source_edge_cluster_occurrence_index_,
                   a.occurrences_.size()) ||
        !ids_valid(a.source_edge_cluster_membership_index_,
                   a.source_edge_memberships_.size()) ||
        !ids_valid(a.source_edge_sequence_cluster_index_,
                   a.source_edge_clusters_.size()) ||
        !ids_valid(a.source_edge_sequence_interval_index_,
                   a.source_edge_intervals_.size()))
      return false;
    for (const auto &record : a.source_edge_memberships_)
      if (!valid_source_edge_membership_key(record.key) ||
          record.event.ordinal() >= a.events_.size() ||
          record.occurrence.ordinal() >= a.occurrences_.size() ||
          !optional_id_valid(record.ordering_certificate,
                             a.ordering_certificates_.size()) ||
          record.schema_version !=
              contract_versions::intersection_source_edge_membership_schema ||
          record.reserved16 != 0)
        return false;
    for (const auto &record : a.source_edge_sequences_)
      if (!checked_range(record.memberships,
                         a.source_edge_membership_sequence_index_.size()) ||
          !checked_range(record.clusters,
                         a.source_edge_sequence_cluster_index_.size()) ||
          !checked_range(record.intervals,
                         a.source_edge_sequence_interval_index_.size()) ||
          !checked_range(record.aggregates, a.crossing_aggregates_.size()) ||
          !checked_range(record.descriptors, a.descriptors_.size()) ||
          record.reserved8 != 0 || record.reserved32 != 0 ||
          record.schema_version !=
              contract_versions::intersection_source_edge_sequence_schema)
        return false;
    for (const auto &record : a.source_edge_clusters_)
      if (record.sequence.ordinal() >= a.source_edge_sequences_.size() ||
          !valid_source_edge_cluster_key(record.key) ||
          !checked_range(record.member_occurrences,
                         a.source_edge_cluster_occurrence_index_.size()) ||
          !checked_range(record.membership_ids,
                         a.source_edge_cluster_membership_index_.size()) ||
          !checked_range(record.contributions, a.incidence_.size()) ||
          !optional_id_valid(record.predecessor,
                             a.source_edge_clusters_.size()) ||
          !optional_id_valid(record.successor, a.source_edge_clusters_.size()) ||
          !optional_id_valid(record.ordering_certificate,
                             a.ordering_certificates_.size()) ||
          record.schema_version != contract_versions::intersection_cluster_schema)
        return false;
    for (const auto &record : a.source_edge_intervals_)
      if (record.sequence.ordinal() >= a.source_edge_sequences_.size() ||
          !valid_source_edge_interval_key(record.key) ||
          !checked_range(record.provenance, a.incidence_.size()) ||
          !checked_range(record.descriptors, a.descriptors_.size()) ||
          record.schema_version !=
              contract_versions::intersection_source_edge_interval_schema ||
          record.reserved16 != 0)
        return false;

    if (!ids_valid(a.carrier_membership_index_, a.carrier_memberships_.size()) ||
        !ids_valid(a.carrier_cluster_occurrence_index_, a.occurrences_.size()) ||
        !ids_valid(a.carrier_cluster_membership_index_,
                   a.carrier_memberships_.size()) ||
        !ids_valid(a.transverse_carrier_cluster_index_,
                   a.carrier_clusters_.size()) ||
        !ids_valid(a.transverse_carrier_span_index_,
                   a.carrier_active_spans_.size()))
      return false;
    for (const auto &record : a.transverse_carriers_)
      if (!valid_transverse_carrier_key(record.key) ||
          !checked_range(record.relation_provenance,
                         a.carrier_relation_provenance_.size()) ||
          !checked_range(record.candidate_provenance,
                         a.carrier_candidate_provenance_.size()) ||
          !checked_range(record.memberships, a.carrier_membership_index_.size()) ||
          !checked_range(record.clusters,
                         a.transverse_carrier_cluster_index_.size()) ||
          !checked_range(record.active_spans,
                         a.transverse_carrier_span_index_.size()) ||
          !checked_range(record.region_incidence,
                         a.carrier_span_region_incidence_.size()) ||
          !checked_range(record.aggregates, a.crossing_aggregates_.size()) ||
          !checked_range(record.descriptors, a.descriptors_.size()) ||
          record.schema_version != contract_versions::intersection_carrier_schema ||
          record.reserved16 != 0 || record.reserved32 != 0)
        return false;
    for (const auto &record : a.carrier_memberships_)
      if (record.carrier.ordinal() >= a.transverse_carriers_.size() ||
          record.occurrence.ordinal() >= a.occurrences_.size() ||
          record.event.ordinal() >= a.events_.size() ||
          !optional_id_valid(record.ordering_certificate,
                             a.ordering_certificates_.size()) ||
          record.schema_version !=
              contract_versions::intersection_carrier_membership_schema ||
          record.reserved16 != 0 || record.reserved32 != 0)
        return false;
    for (const auto &record : a.carrier_clusters_)
      if (record.carrier.ordinal() >= a.transverse_carriers_.size() ||
          !checked_range(record.occurrence_members,
                         a.carrier_cluster_occurrence_index_.size()) ||
          !checked_range(record.membership_members,
                         a.carrier_cluster_membership_index_.size()) ||
          !optional_id_valid(record.predecessor, a.carrier_clusters_.size()) ||
          !optional_id_valid(record.successor, a.carrier_clusters_.size()) ||
          !optional_id_valid(record.ordering_certificate,
                             a.ordering_certificates_.size()) ||
          record.schema_version != contract_versions::intersection_cluster_schema)
        return false;
    for (const auto &record : a.carrier_active_spans_)
      if (record.carrier.ordinal() >= a.transverse_carriers_.size() ||
          record.left.ordinal() >= a.carrier_clusters_.size() ||
          record.right.ordinal() >= a.carrier_clusters_.size() ||
          !checked_range(record.region_incidence,
                         a.carrier_span_region_incidence_.size()) ||
          !checked_range(record.provenance,
                         a.carrier_span_relation_provenance_.size()) ||
          record.reserved8 != 0 ||
          record.schema_version != contract_versions::intersection_carrier_schema)
        return false;

    for (const auto &record : a.coplanar_supports_)
      if (!valid_coplanar_support_key(record.key) ||
          !checked_range(record.original_boundary_edges,
                         a.coplanar_support_original_boundary_edge_index_.size()) ||
          !checked_range(record.boundary_events,
                         a.coplanar_support_boundary_event_index_.size()) ||
          !checked_range(record.boundary_carriers,
                         a.coplanar_support_boundary_carrier_index_.size()) ||
          !checked_range(record.overlap_components,
                         a.coplanar_support_overlap_index_.size()) ||
          !checked_range(record.region_incidence,
                         a.coplanar_support_region_index_.size()) ||
          !checked_range(record.provenance,
                         a.coplanar_support_relation_provenance_.size()) ||
          record.schema_version != contract_versions::intersection_overlap_schema ||
          record.reserved16 != 0)
        return false;
    for (const auto &record : a.overlap_carriers_)
      if (!valid_collinear_overlap_carrier_key(record.key) ||
          !checked_range(record.provenance,
                         a.overlap_carrier_relation_provenance_.size()) ||
          !checked_range(record.source_provenance,
                         a.overlap_carrier_source_provenance_.size()) ||
          !checked_range(record.contributions, a.incidence_.size()) ||
          !checked_range(record.descriptors, a.descriptors_.size()) ||
          record.reserved8 != 0 || record.reserved16 != 0 ||
          record.schema_version != contract_versions::intersection_overlap_schema)
        return false;
    for (const auto &record : a.coplanar_overlaps_)
      if (!valid_coplanar_overlap_key(record.key) ||
          record.support.ordinal() >= a.coplanar_supports_.size() ||
          !checked_range(record.boundary_events,
                         a.coplanar_overlap_boundary_event_index_.size()) ||
          !checked_range(record.boundary_carriers,
                         a.coplanar_overlap_boundary_carrier_index_.size()) ||
          !checked_range(record.provenance,
                         a.coplanar_overlap_relation_provenance_.size()) ||
          record.schema_version != contract_versions::intersection_overlap_schema ||
          record.reserved16 != 0)
        return false;
    for (const auto &record : a.coplanar_region_incidence_)
      if (record.support.ordinal() >= a.coplanar_supports_.size() ||
          record.component.ordinal() >= a.coplanar_overlaps_.size() ||
          !checked_range(record.boundary_events,
                         a.coplanar_region_boundary_event_index_.size()) ||
          !checked_range(record.boundary_carriers,
                         a.coplanar_region_boundary_carrier_index_.size()) ||
          !checked_range(record.coverage_witnesses,
                         a.coplanar_region_coverage_witness_index_.size()) ||
          record.reserved8 != 0 || record.reserved16 != 0 ||
          record.schema_version != contract_versions::intersection_overlap_schema)
        return false;

    if (!ids_valid(a.crossing_aggregate_members_, a.incidence_.size()) ||
        !ids_valid(a.crossing_facet_subtotal_members_, a.incidence_.size()) ||
        !ids_valid(a.crossing_shell_subtotal_members_, a.incidence_.size()) ||
        !ids_valid(a.contact_aggregate_members_, a.incidence_.size()) ||
        !ids_valid(a.descriptor_provenance_, a.incidence_.size()))
      return false;
    for (const auto &record : a.crossing_aggregates_)
      if (!checked_range(record.members, a.crossing_aggregate_members_.size()) ||
          !checked_range(record.facet_subtotals,
                         a.crossing_facet_subtotals_.size()) ||
          !checked_range(record.shell_subtotals,
                         a.crossing_shell_subtotals_.size()) ||
          !operand_valid(record.symbolic_owner) || record.reserved8 != 0 ||
          record.schema_version != contract_versions::intersection_aggregate_schema)
        return false;
    for (const auto &record : a.crossing_facet_subtotals_)
      if (!checked_range(record.members,
                         a.crossing_facet_subtotal_members_.size()) ||
          !operand_valid(record.symbolic_owner) || record.reserved16 != 0 ||
          record.schema_version != contract_versions::intersection_aggregate_schema)
        return false;
    for (const auto &record : a.crossing_shell_subtotals_)
      if (!checked_range(record.members,
                         a.crossing_shell_subtotal_members_.size()) ||
          !operand_valid(record.symbolic_owner) || record.reserved16 != 0 ||
          record.schema_version != contract_versions::intersection_aggregate_schema)
        return false;
    for (const auto &record : a.contact_aggregates_)
      if (!checked_range(record.members, a.contact_aggregate_members_.size()) ||
          !operand_valid(record.symbolic_owner) || record.reserved16 != 0 ||
          record.schema_version != contract_versions::intersection_aggregate_schema)
        return false;
    for (const auto &record : a.descriptors_)
      if (!valid_intersection_descriptor_key(record.key) ||
          !checked_range(record.provenance, a.descriptor_provenance_.size()) ||
          !operand_valid(record.symbolic_owner) || record.reserved8 != 0 ||
          record.schema_version != contract_versions::intersection_descriptor_schema)
        return false;

    for (const auto &certificate : a.ordering_certificates_) {
      if (certificate.policy_version !=
              contract_versions::intersection_bounded_ordering_policy ||
          certificate.reserved8 != 0 || !certificate.topology_safe ||
          certificate.first_parameter.ordinal() == intersection_invalid_ordinal ||
          certificate.second_parameter.ordinal() == intersection_invalid_ordinal)
        return false;
      switch (certificate.disposition) {
      case intersection_order_disposition::definitely_before:
      case intersection_order_disposition::definitely_after:
        break;
      case intersection_order_disposition::exact_equal:
        if (certificate.exact_evidence_lineage == 0)
          return false;
        break;
      case intersection_order_disposition::unresolved_overlap:
        if (certificate.comparison_evidence_lineage == 0)
          return false;
        break;
      case intersection_order_disposition::invalid:
        return false;
      }
    }
    for (const auto &diagnostic : a.diagnostics_)
      if (diagnostic.witness_count > diagnostic.witnesses.size() ||
          diagnostic.schema_version !=
              contract_versions::intersection_diagnostic_schema ||
          diagnostic.reserved32 != 0)
        return false;
    for (const auto &checkpoint : a.replay_checkpoints_)
      if (checkpoint.schema_version !=
              contract_versions::intersection_replay_schema ||
          checkpoint.reserved16 != 0 || checkpoint.reserved32 != 0)
        return false;
    if (a.verification_evidence_.schema_version !=
            contract_versions::intersection_exhaustive_evidence_schema ||
        a.verification_evidence_.verifier_version !=
            contract_versions::intersection_verifier ||
        a.verification_evidence_.reserved8 != 0)
      return false;

    const auto &s = a.statistics_;
    if (s.seed_count != a.seed_bindings_.size() ||
        s.event_count != a.events_.size() ||
        s.occurrence_count != a.occurrences_.size() ||
        s.seed_binding_count != a.seed_bindings_.size() ||
        s.incidence_count != a.incidence_.size() ||
        s.source_edge_membership_count != a.source_edge_memberships_.size() ||
        s.source_edge_sequence_count != a.source_edge_sequences_.size() ||
        s.source_edge_cluster_count != a.source_edge_clusters_.size() ||
        s.source_edge_interval_count != a.source_edge_intervals_.size() ||
        s.transverse_carrier_count != a.transverse_carriers_.size() ||
        s.carrier_membership_count != a.carrier_memberships_.size() ||
        s.carrier_cluster_count != a.carrier_clusters_.size() ||
        s.carrier_span_count != a.carrier_active_spans_.size() ||
        s.coplanar_support_count != a.coplanar_supports_.size() ||
        s.overlap_count != a.coplanar_overlaps_.size() ||
        s.aggregate_count !=
            a.crossing_aggregates_.size() + a.contact_aggregates_.size() ||
        s.descriptor_count != a.descriptors_.size() ||
        s.ordering_certificate_count != a.ordering_certificates_.size() ||
        s.diagnostic_count != a.diagnostics_.size() ||
        s.replay_checkpoint_count != a.replay_checkpoints_.size() ||
        s.verifier_work_units != a.verification_evidence_.work_units)
      return false;

    std::uint64_t persistent = 0;
    const auto add_bytes = [&](const auto &values) noexcept {
      using value_type = typename std::decay_t<decltype(values)>::value_type;
      if (values.size() >
          std::numeric_limits<std::uint64_t>::max() / sizeof(value_type))
        return false;
      const auto bytes =
          static_cast<std::uint64_t>(values.size()) * sizeof(value_type);
      if (persistent > std::numeric_limits<std::uint64_t>::max() - bytes)
        return false;
      persistent += bytes;
      return true;
    };
#define YGOR_CODEC_ADD(field) if (!add_bytes(a.field)) return false
    YGOR_CODEC_ADD(events_); YGOR_CODEC_ADD(occurrences_);
    YGOR_CODEC_ADD(seed_bindings_); YGOR_CODEC_ADD(construction_witness_index_);
    YGOR_CODEC_ADD(event_binding_index_); YGOR_CODEC_ADD(occurrence_binding_index_);
    YGOR_CODEC_ADD(seed_to_event_); YGOR_CODEC_ADD(seed_to_occurrence_);
    YGOR_CODEC_ADD(incidence_); YGOR_CODEC_ADD(incidence_by_event_);
    YGOR_CODEC_ADD(incidence_by_occurrence_); YGOR_CODEC_ADD(incidence_by_seed_);
    YGOR_CODEC_ADD(incidence_by_seed_candidate_); YGOR_CODEC_ADD(event_incidence_ranges_);
    YGOR_CODEC_ADD(occurrence_incidence_ranges_); YGOR_CODEC_ADD(seed_incidence_ranges_);
    YGOR_CODEC_ADD(seed_candidate_incidence_ranges_); YGOR_CODEC_ADD(incidence_by_relation_);
    YGOR_CODEC_ADD(relation_incidence_ranges_); YGOR_CODEC_ADD(incidence_by_candidate_);
    YGOR_CODEC_ADD(candidate_incidence_ranges_); YGOR_CODEC_ADD(incidence_by_source_feature_);
    YGOR_CODEC_ADD(source_feature_incidence_ranges_); YGOR_CODEC_ADD(incidence_by_halfedge_);
    YGOR_CODEC_ADD(halfedge_incidence_ranges_); YGOR_CODEC_ADD(source_edge_memberships_);
    YGOR_CODEC_ADD(source_edge_sequences_); YGOR_CODEC_ADD(source_edge_clusters_);
    YGOR_CODEC_ADD(source_edge_intervals_); YGOR_CODEC_ADD(source_edge_membership_sequence_index_);
    YGOR_CODEC_ADD(source_edge_cluster_occurrence_index_); YGOR_CODEC_ADD(source_edge_cluster_membership_index_);
    YGOR_CODEC_ADD(source_edge_sequence_cluster_index_); YGOR_CODEC_ADD(source_edge_sequence_interval_index_);
    YGOR_CODEC_ADD(transverse_carriers_); YGOR_CODEC_ADD(carrier_memberships_);
    YGOR_CODEC_ADD(carrier_clusters_); YGOR_CODEC_ADD(carrier_active_spans_);
    YGOR_CODEC_ADD(carrier_relation_provenance_); YGOR_CODEC_ADD(carrier_candidate_provenance_);
    YGOR_CODEC_ADD(carrier_membership_index_); YGOR_CODEC_ADD(carrier_cluster_occurrence_index_);
    YGOR_CODEC_ADD(carrier_cluster_membership_index_); YGOR_CODEC_ADD(transverse_carrier_cluster_index_);
    YGOR_CODEC_ADD(transverse_carrier_span_index_); YGOR_CODEC_ADD(carrier_span_relation_provenance_);
    YGOR_CODEC_ADD(carrier_span_region_incidence_); YGOR_CODEC_ADD(coplanar_supports_);
    YGOR_CODEC_ADD(overlap_carriers_); YGOR_CODEC_ADD(coplanar_overlaps_);
    YGOR_CODEC_ADD(coplanar_region_incidence_); YGOR_CODEC_ADD(coplanar_support_relation_provenance_);
    YGOR_CODEC_ADD(coplanar_support_candidate_provenance_); YGOR_CODEC_ADD(coplanar_support_original_boundary_edge_index_);
    YGOR_CODEC_ADD(coplanar_support_boundary_event_index_); YGOR_CODEC_ADD(coplanar_support_boundary_carrier_index_);
    YGOR_CODEC_ADD(coplanar_support_overlap_index_); YGOR_CODEC_ADD(coplanar_support_region_index_);
    YGOR_CODEC_ADD(overlap_carrier_relation_provenance_); YGOR_CODEC_ADD(overlap_carrier_candidate_provenance_);
    YGOR_CODEC_ADD(overlap_carrier_source_provenance_); YGOR_CODEC_ADD(coplanar_overlap_boundary_event_index_);
    YGOR_CODEC_ADD(coplanar_overlap_boundary_carrier_index_); YGOR_CODEC_ADD(coplanar_overlap_relation_provenance_);
    YGOR_CODEC_ADD(coplanar_region_boundary_event_index_); YGOR_CODEC_ADD(coplanar_region_boundary_carrier_index_);
    YGOR_CODEC_ADD(coplanar_region_coverage_witness_index_); YGOR_CODEC_ADD(crossing_aggregates_);
    YGOR_CODEC_ADD(crossing_aggregate_members_); YGOR_CODEC_ADD(crossing_facet_subtotals_);
    YGOR_CODEC_ADD(crossing_facet_subtotal_members_); YGOR_CODEC_ADD(crossing_shell_subtotals_);
    YGOR_CODEC_ADD(crossing_shell_subtotal_members_); YGOR_CODEC_ADD(contact_aggregates_);
    YGOR_CODEC_ADD(contact_aggregate_members_); YGOR_CODEC_ADD(descriptors_);
    YGOR_CODEC_ADD(descriptor_provenance_); YGOR_CODEC_ADD(ordering_certificates_);
    YGOR_CODEC_ADD(diagnostics_); YGOR_CODEC_ADD(replay_checkpoints_);
#undef YGOR_CODEC_ADD
    if (persistent != s.persistent_bytes)
      return false;
    if (require_encoded_bytes &&
        (a.canonical_bytes_.empty() ||
         s.canonical_bytes != a.canonical_bytes_.size()))
      return false;
    return true;
  }

  template <class T, class I>
  static void clear_encoding(canonical_intersection_complex<T, I> &artifact) {
    artifact.statistics_.canonical_bytes = 0;
    artifact.section_digests_ = {};
    artifact.canonical_bytes_.clear();
    artifact.digest_ = {};
  }

  template <class T, class I>
  static void set_canonical_byte_count(
      canonical_intersection_complex<T, I> &artifact, std::uint64_t count) {
    artifact.statistics_.canonical_bytes = count;
  }

  template <class T, class I>
  static std::uint64_t canonical_byte_count(
      const canonical_intersection_complex<T, I> &artifact) {
    return artifact.statistics_.canonical_bytes;
  }

  template <class T, class I>
  static void publish_encoding(
      canonical_intersection_complex<T, I> &artifact,
      const std::array<bounded_boolean_digest, 8> &section_digests,
      std::vector<std::uint8_t> bytes,
      const bounded_boolean_digest &digest) {
    artifact.section_digests_ = section_digests;
    artifact.canonical_bytes_ = std::move(bytes);
    artifact.digest_ = digest;
  }

  template <class T, class I>
  static bool encoding_matches(
      const canonical_intersection_complex<T, I> &artifact,
      const std::array<bounded_boolean_digest, 8> &section_digests,
      const std::vector<std::uint8_t> &bytes,
      const bounded_boolean_digest &digest) {
    return artifact.section_digests_ == section_digests &&
           artifact.canonical_bytes_ == bytes && artifact.digest_ == digest;
  }
};

namespace intersection_codec_detail {

struct encoded_projection final {
  std::array<std::vector<std::uint8_t>, 8> sections{};
  std::array<bounded_boolean_digest, 8> section_digests{};
  std::vector<std::uint8_t> bytes{};
  bounded_boolean_digest digest{};
};

bool valid_limits(const intersection_codec_limits &limits) noexcept {
  return limits.version == contract_versions::intersection_codec &&
         limits.reserved16 == 0 && limits.reserved32 == 0 &&
         limits.maximum_bytes >= 32 &&
         limits.maximum_records_per_table != 0 &&
         limits.maximum_index_entries != 0 &&
         limits.maximum_vector_allocations >= 64;
}

template <class T, class I>
bool build_projection(const canonical_intersection_complex<T, I> &artifact,
                      const intersection_codec_limits &limits,
                      encoded_projection &projection,
                      bounded_boolean_error &error) {
  projection = encoded_projection{};
  if (!intersection_codec_access::encode_sections(artifact,
                                                   projection.sections)) {
    error = codec_error("intersection section encoding failed",
                        bounded_boolean_error_category::internal_invariant_error);
    return false;
  }
  std::uint64_t total = 12 + 32;
  for (std::size_t i = 0; i < projection.sections.size(); ++i) {
    if (projection.sections[i].size() > limits.maximum_bytes) {
      error = resource_error("intersection encoded section exceeds limit");
      return false;
    }
    projection.section_digests[i] = sha256::digest(projection.sections[i]);
    constexpr std::uint64_t frame_overhead = 2 + 2 + 4 + 32 + 8;
    if (total > limits.maximum_bytes - frame_overhead ||
        projection.sections[i].size() >
            limits.maximum_bytes - total - frame_overhead) {
      error = resource_error("intersection encoded artifact exceeds limit");
      return false;
    }
    total += frame_overhead + projection.sections[i].size();
  }
  if (total > limits.maximum_bytes) {
    error = resource_error("intersection encoded artifact exceeds limit");
    return false;
  }

  canonical_writer writer;
  writer.u32(envelope_magic);
  writer.u16(contract_versions::intersection_codec);
  writer.u16(section_count);
  writer.u32(0);
  for (std::size_t i = 0; i < projection.sections.size(); ++i) {
    writer.u16(static_cast<std::uint16_t>(i + 1));
    writer.u16(section_version);
    writer.u32(0);
    write_digest(writer, projection.section_digests[i]);
    if (!writer.sized_bytes(projection.sections[i])) {
      error = codec_error("intersection section length overflow",
                          bounded_boolean_error_category::internal_invariant_error);
      return false;
    }
  }
  projection.digest = sha256::digest(writer.bytes());
  write_digest(writer, projection.digest);
  projection.bytes = writer.take();
  if (projection.bytes.size() != total) {
    error = codec_error("intersection encoded byte accounting mismatch",
                        bounded_boolean_error_category::internal_invariant_error);
    return false;
  }
  return true;
}

bool parse_projection(const std::vector<std::uint8_t> &bytes,
                      const intersection_codec_limits &limits,
                      encoded_projection &projection,
                      bounded_boolean_error &error) {
  projection = encoded_projection{};
  if (!valid_limits(limits)) {
    error = codec_error("intersection codec limits are malformed");
    return false;
  }
  if (bytes.size() > limits.maximum_bytes) {
    error = resource_error("intersection encoded artifact exceeds limit");
    return false;
  }
  if (bytes.size() < 44) {
    error = codec_error("intersection encoded artifact is truncated");
    return false;
  }
  canonical_reader reader(bytes);
  std::uint32_t magic = 0, reserved = 0;
  std::uint16_t codec_version = 0, count = 0;
  if (!reader.u32(magic) || !reader.u16(codec_version) ||
      !reader.u16(count) || !reader.u32(reserved) ||
      magic != envelope_magic ||
      codec_version != contract_versions::intersection_codec ||
      count != section_count || reserved != 0) {
    error = codec_error("intersection encoded envelope header is invalid");
    return false;
  }
  for (std::size_t i = 0; i < projection.sections.size(); ++i) {
    std::uint16_t tag = 0, version = 0;
    std::uint32_t section_reserved = 0;
    if (!reader.u16(tag) || !reader.u16(version) ||
        !reader.u32(section_reserved) ||
        !read_digest(reader, projection.section_digests[i]) ||
        !reader.sized_bytes(projection.sections[i], limits.maximum_bytes) ||
        tag != i + 1 || version != section_version || section_reserved != 0) {
      error = codec_error("intersection encoded section frame is malformed");
      return false;
    }
    if (sha256::digest(projection.sections[i]) !=
        projection.section_digests[i]) {
      error = digest_error("intersection encoded section digest mismatch");
      return false;
    }
  }
  if (!read_digest(reader, projection.digest) || !reader.complete()) {
    error = codec_error("intersection encoded artifact has trailing or missing data");
    return false;
  }
  sha256 complete;
  complete.update(bytes.data(), bytes.size() - projection.digest.bytes.size());
  if (complete.finish() != projection.digest) {
    error = digest_error("intersection complete artifact digest mismatch");
    return false;
  }
  projection.bytes = bytes;
  return true;
}

} // namespace intersection_codec_detail

template <class T, class I>
bool refresh_intersection_codec(
    canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &limits,
    bounded_boolean_error &error) {
  using namespace intersection_codec_detail;
  if (!valid_limits(limits)) {
    error = codec_error("intersection codec limits are malformed");
    return false;
  }
  try {
    canonical_intersection_complex<T, I> candidate = artifact;
    intersection_codec_access::clear_encoding(candidate);
    if (!intersection_codec_access::validate_projection(candidate, false)) {
      error = codec_error("intersection artifact projection is malformed",
                          bounded_boolean_error_category::internal_invariant_error);
      return false;
    }
    encoded_projection first;
    if (!build_projection(candidate, limits, first, error))
      return false;
    intersection_codec_access::set_canonical_byte_count(
        candidate, static_cast<std::uint64_t>(first.bytes.size()));
    encoded_projection final;
    if (!build_projection(candidate, limits, final, error))
      return false;
    if (final.bytes.size() !=
        intersection_codec_access::canonical_byte_count(candidate)) {
      error = codec_error("intersection canonical byte count did not stabilize",
                          bounded_boolean_error_category::internal_invariant_error);
      return false;
    }
    intersection_codec_access::publish_encoding(
        candidate, final.section_digests, std::move(final.bytes), final.digest);
    if (!intersection_codec_access::validate_projection(candidate, true)) {
      error = codec_error("intersection encoded artifact failed validation",
                          bounded_boolean_error_category::internal_invariant_error);
      return false;
    }
    artifact = std::move(candidate);
    return true;
  } catch (const std::bad_alloc &) {
    error = resource_error("intersection codec allocation failed");
    return false;
  } catch (...) {
    error = codec_error("intersection codec raised an unexpected exception",
                        bounded_boolean_error_category::internal_invariant_error);
    return false;
  }
}

template <class T, class I>
bool verify_intersection_codec(
    const canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &limits,
    bounded_boolean_error &error) {
  using namespace intersection_codec_detail;
  if (!valid_limits(limits)) {
    error = codec_error("intersection codec limits are malformed");
    return false;
  }
  try {
    if (!intersection_codec_access::validate_projection(artifact, true)) {
      error = codec_error("intersection encoded artifact projection is malformed",
                          bounded_boolean_error_category::internal_invariant_error);
      return false;
    }
    encoded_projection expected;
    if (!build_projection(artifact, limits, expected, error))
      return false;
    if (!intersection_codec_access::encoding_matches(
            artifact, expected.section_digests, expected.bytes,
            expected.digest)) {
      error = digest_error("intersection canonical bytes or digest mismatch");
      return false;
    }
    return true;
  } catch (const std::bad_alloc &) {
    error = resource_error("intersection codec verification allocation failed");
    return false;
  } catch (...) {
    error = codec_error("intersection codec verification raised an exception",
                        bounded_boolean_error_category::internal_invariant_error);
    return false;
  }
}

template <class T, class I>
bool decode_intersection_complex_private(
    const std::vector<std::uint8_t> &bytes,
    const intersection_canonicalization_header &expectations,
    const intersection_codec_limits &limits,
    canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error) {
  using namespace intersection_codec_detail;
  try {
    encoded_projection encoded;
    if (!parse_projection(bytes, limits, encoded, error))
      return false;
    canonical_intersection_complex<T, I> candidate;
    if (!intersection_codec_access::decode_sections(
            encoded.sections, expectations, limits, candidate)) {
      error = codec_error("intersection encoded section payload is malformed");
      return false;
    }
    intersection_codec_access::publish_encoding(
        candidate, encoded.section_digests, encoded.bytes, encoded.digest);
    if (!intersection_codec_access::validate_projection(candidate, true)) {
      error = codec_error("intersection decoded artifact failed validation");
      return false;
    }
    encoded_projection reconstructed;
    if (!build_projection(candidate, limits, reconstructed, error))
      return false;
    if (reconstructed.section_digests != encoded.section_digests ||
        reconstructed.bytes != encoded.bytes ||
        reconstructed.digest != encoded.digest) {
      error = digest_error("intersection decoded artifact is not canonical");
      return false;
    }
    artifact = std::move(candidate);
    return true;
  } catch (const std::bad_alloc &) {
    error = resource_error("intersection decoder allocation failed");
    return false;
  } catch (...) {
    error = codec_error("intersection decoder raised an unexpected exception",
                        bounded_boolean_error_category::internal_invariant_error);
    return false;
  }
}

#define YGOR_DEFINE_INTERSECTION_CODEC(T, I)                               \
  template bool refresh_intersection_codec<T, I>(                         \
      canonical_intersection_complex<T, I> &,                              \
      const intersection_codec_limits &, bounded_boolean_error &);         \
  template bool verify_intersection_codec<T, I>(                          \
      const canonical_intersection_complex<T, I> &,                        \
      const intersection_codec_limits &, bounded_boolean_error &);         \
  template bool decode_intersection_complex_private<T, I>(                \
      const std::vector<std::uint8_t> &,                                   \
      const intersection_canonicalization_header &,                        \
      const intersection_codec_limits &,                                   \
      canonical_intersection_complex<T, I> &, bounded_boolean_error &)

YGOR_DEFINE_INTERSECTION_CODEC(float, std::uint32_t);
YGOR_DEFINE_INTERSECTION_CODEC(float, std::uint64_t);
YGOR_DEFINE_INTERSECTION_CODEC(double, std::uint32_t);
YGOR_DEFINE_INTERSECTION_CODEC(double, std::uint64_t);

#undef YGOR_DEFINE_INTERSECTION_CODEC

} // namespace ygor::mesh_boolean::bounded
