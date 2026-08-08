#pragma once

#include "ContractVersions.h"
#include "Policies.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class relation_family : std::uint8_t {
  vertex_vertex = 1,
  vertex_edge = 2,
  vertex_face = 3,
  edge_edge = 4,
  edge_face = 5,
  equal_edge = 6,
  tangent = 7,
  coplanar = 8,
  coincident_face = 9,
};

enum class orientation_relation : std::uint8_t {
  opposite = 1,
  indeterminate = 2,
  same = 3,
};

enum class symbolic_ownership_role : std::uint8_t {
  acting_source_feature = 1,
  opposite_source_feature = 2,
  shared_source_feature = 3,
  coincident_sheet_pair = 4,
};

enum class symbolic_half_open_role : std::uint8_t {
  none = 0,
  interior = 1,
  source_endpoint = 2,
  source_edge = 3,
};

enum class symbolic_transition_orientation : std::uint8_t {
  none = 0,
  negative_to_positive = 1,
  positive_to_negative = 2,
  tangent = 3,
};

enum class symbolic_occurrence_class : std::uint8_t {
  ordinary = 1,
  shared_source_feature = 2,
  lower_dimensional_contact = 3,
  coincident_sheet = 4,
};

enum class symbolic_offset_disposition : std::int8_t {
  negative = -1,
  coincident = 0,
  positive = 1,
};

enum class symbolic_contact_class : std::uint8_t {
  point_contact = 1,
  edge_contact = 2,
  tangent_contact = 3,
  coplanar_overlap = 4,
  coincident_sheet = 5,
};

enum class symbolic_expected_disposition : std::uint8_t {
  classification_only = 1,
  half_open_ownership = 2,
  occurrence_separation = 3,
  coincident_owner_eligibility = 4,
};

enum class symbolic_explanation_code : std::uint16_t {
  exact_vertex_tie = 1,
  exact_edge_tie = 2,
  exact_edge_face_tie = 3,
  exact_tangent_contact = 4,
  exact_coplanar_overlap = 5,
  exact_coincident_sheet = 6,
};

enum class symbolic_tie_key_component : std::uint8_t {
  final_orientation_capability = 1,
  operand_priority = 2,
  symbolic_feature_priority = 3,
  canonical_source_feature = 4,
  directed_use = 5,
  occurrence = 6,
};

inline constexpr std::size_t symbolic_tie_key_component_count = 6;

struct symbolic_tie_key_description final {
  std::array<symbolic_tie_key_component, symbolic_tie_key_component_count>
      components{
          symbolic_tie_key_component::final_orientation_capability,
          symbolic_tie_key_component::operand_priority,
          symbolic_tie_key_component::symbolic_feature_priority,
          symbolic_tie_key_component::canonical_source_feature,
          symbolic_tie_key_component::directed_use,
          symbolic_tie_key_component::occurrence,
      };
  operand_id preferred_operand = operand_id::a;
  std::uint8_t feature_priority = 0;
  std::uint16_t schema_version = contract_versions::symbolic_policy;
  std::uint16_t reserved = 0;

  friend bool operator==(const symbolic_tie_key_description &a,
                         const symbolic_tie_key_description &b) noexcept {
    return std::tie(a.components, a.preferred_operand, a.feature_priority,
                    a.schema_version, a.reserved) ==
           std::tie(b.components, b.preferred_operand, b.feature_priority,
                    b.schema_version, b.reserved);
  }
};

struct symbolic_rule_key final {
  boolean_operation operation = boolean_operation::set_union;
  operand_id acting_operand = operand_id::a;
  relation_family relation = relation_family::vertex_vertex;
  orientation_relation orientation = orientation_relation::indeterminate;
  symbolic_ownership_role ownership_role =
      symbolic_ownership_role::acting_source_feature;
  symbolic_half_open_role half_open_role = symbolic_half_open_role::none;
  symbolic_transition_orientation transition =
      symbolic_transition_orientation::none;
  symbolic_occurrence_class occurrence_class =
      symbolic_occurrence_class::ordinary;
  std::uint16_t schema_version = contract_versions::symbolic_policy;
  std::uint16_t reserved = 0;

  friend bool operator<(const symbolic_rule_key &a,
                        const symbolic_rule_key &b) noexcept {
    return std::tie(a.operation, a.acting_operand, a.relation, a.orientation,
                    a.ownership_role, a.half_open_role, a.transition,
                    a.occurrence_class, a.schema_version, a.reserved) <
           std::tie(b.operation, b.acting_operand, b.relation, b.orientation,
                    b.ownership_role, b.half_open_role, b.transition,
                    b.occurrence_class, b.schema_version, b.reserved);
  }

  friend bool operator==(const symbolic_rule_key &a,
                         const symbolic_rule_key &b) noexcept {
    return std::tie(a.operation, a.acting_operand, a.relation, a.orientation,
                    a.ownership_role, a.half_open_role, a.transition,
                    a.occurrence_class, a.schema_version, a.reserved) ==
           std::tie(b.operation, b.acting_operand, b.relation, b.orientation,
                    b.ownership_role, b.half_open_role, b.transition,
                    b.occurrence_class, b.schema_version, b.reserved);
  }

  friend bool operator!=(const symbolic_rule_key &a,
                         const symbolic_rule_key &b) noexcept {
    return !(a == b);
  }
};

struct symbolic_rule final {
  symbolic_rule_key key{};
  symbolic_offset_disposition conceptual_offset =
      symbolic_offset_disposition::coincident;
  std::uint8_t feature_priority = 0;
  operand_id half_open_owner = operand_id::a;
  std::int8_t crossing_contribution = 0;
  symbolic_contact_class contact_class = symbolic_contact_class::point_contact;
  operand_id coincident_owner = operand_id::a;
  symbolic_expected_disposition expected_disposition =
      symbolic_expected_disposition::classification_only;
  bool owner_rank_eligible = false;
  bool occurrence_separation_required = false;
  symbolic_tie_key_description tie_key{};
  std::uint16_t tie_key_schema = contract_versions::symbolic_policy;
  symbolic_explanation_code explanation =
      symbolic_explanation_code::exact_vertex_tie;
  symbolic_rule_key exchanged_key{};
  std::uint64_t exchange_rule_ordinal = 0;
  std::uint32_t reserved = 0;

  friend bool operator==(const symbolic_rule &a,
                         const symbolic_rule &b) noexcept {
    return std::tie(a.key, a.conceptual_offset, a.feature_priority,
                    a.half_open_owner, a.crossing_contribution,
                    a.contact_class, a.coincident_owner,
                    a.expected_disposition, a.owner_rank_eligible,
                    a.occurrence_separation_required, a.tie_key,
                    a.tie_key_schema, a.explanation, a.exchanged_key, a.exchange_rule_ordinal,
                    a.reserved) ==
           std::tie(b.key, b.conceptual_offset, b.feature_priority,
                    b.half_open_owner, b.crossing_contribution,
                    b.contact_class, b.coincident_owner,
                    b.expected_disposition, b.owner_rank_eligible,
                    b.occurrence_separation_required, b.tie_key,
                    b.tie_key_schema, b.explanation, b.exchanged_key, b.exchange_rule_ordinal,
                    b.reserved);
  }
};

struct symbolic_policy_table final {
  std::vector<symbolic_rule> rules;
  std::vector<std::uint8_t> bytes;
  bounded_boolean_digest digest{};
};

inline constexpr std::uint64_t symbolic_operation_count = 5;
inline constexpr std::uint64_t symbolic_operand_count = 2;
inline constexpr std::uint64_t symbolic_relation_family_count = 9;
inline constexpr std::uint64_t symbolic_orientation_count = 3;
inline constexpr std::uint64_t symbolic_ownership_role_count = 4;
inline constexpr std::uint64_t symbolic_half_open_role_count = 4;
inline constexpr std::uint64_t symbolic_transition_count = 4;
inline constexpr std::uint64_t symbolic_occurrence_class_count = 4;
inline constexpr std::uint64_t symbolic_rule_count =
    symbolic_operation_count * symbolic_operand_count *
    symbolic_relation_family_count * symbolic_orientation_count *
    symbolic_ownership_role_count * symbolic_half_open_role_count *
    symbolic_transition_count * symbolic_occurrence_class_count;

inline operand_id symbolic_opposite_operand(operand_id value) noexcept {
  return value == operand_id::a ? operand_id::b : operand_id::a;
}

inline bool valid_symbolic_rule_key(const symbolic_rule_key &key) noexcept {
  const auto operation = static_cast<std::uint8_t>(key.operation);
  const auto operand = static_cast<std::uint8_t>(key.acting_operand);
  const auto relation = static_cast<std::uint8_t>(key.relation);
  const auto orientation = static_cast<std::uint8_t>(key.orientation);
  const auto ownership = static_cast<std::uint8_t>(key.ownership_role);
  const auto half_open = static_cast<std::uint8_t>(key.half_open_role);
  const auto transition = static_cast<std::uint8_t>(key.transition);
  const auto occurrence = static_cast<std::uint8_t>(key.occurrence_class);
  return operation >= 1 && operation <= symbolic_operation_count &&
         operand < symbolic_operand_count && relation >= 1 &&
         relation <= symbolic_relation_family_count && orientation >= 1 &&
         orientation <= symbolic_orientation_count && ownership >= 1 &&
         ownership <= symbolic_ownership_role_count &&
         half_open < symbolic_half_open_role_count &&
         transition < symbolic_transition_count && occurrence >= 1 &&
         occurrence <= symbolic_occurrence_class_count &&
         key.schema_version == contract_versions::symbolic_policy &&
         key.reserved == 0;
}

inline symbolic_ownership_role exchange_symbolic_ownership_role(
    symbolic_ownership_role value) noexcept {
  switch (value) {
  case symbolic_ownership_role::acting_source_feature:
    return symbolic_ownership_role::opposite_source_feature;
  case symbolic_ownership_role::opposite_source_feature:
    return symbolic_ownership_role::acting_source_feature;
  case symbolic_ownership_role::shared_source_feature:
  case symbolic_ownership_role::coincident_sheet_pair:
    return value;
  }
  return value;
}

inline symbolic_transition_orientation exchange_symbolic_transition(
    symbolic_transition_orientation value) noexcept {
  switch (value) {
  case symbolic_transition_orientation::negative_to_positive:
    return symbolic_transition_orientation::positive_to_negative;
  case symbolic_transition_orientation::positive_to_negative:
    return symbolic_transition_orientation::negative_to_positive;
  case symbolic_transition_orientation::none:
  case symbolic_transition_orientation::tangent:
    return value;
  }
  return value;
}

inline symbolic_rule_key
exchange_symbolic_rule_key(symbolic_rule_key key) noexcept {
  key.operation = swap_operands(key.operation);
  key.acting_operand = symbolic_opposite_operand(key.acting_operand);
  key.ownership_role = exchange_symbolic_ownership_role(key.ownership_role);
  key.transition = exchange_symbolic_transition(key.transition);
  return key;
}

inline std::uint64_t symbolic_rule_ordinal(
    const symbolic_rule_key &key) noexcept {
  if (!valid_symbolic_rule_key(key))
    return symbolic_rule_count;
  std::uint64_t ordinal = static_cast<std::uint8_t>(key.operation) - 1;
  ordinal = ordinal * symbolic_operand_count +
            static_cast<std::uint8_t>(key.acting_operand);
  ordinal = ordinal * symbolic_relation_family_count +
            static_cast<std::uint8_t>(key.relation) - 1;
  ordinal = ordinal * symbolic_orientation_count +
            static_cast<std::uint8_t>(key.orientation) - 1;
  ordinal = ordinal * symbolic_ownership_role_count +
            static_cast<std::uint8_t>(key.ownership_role) - 1;
  ordinal = ordinal * symbolic_half_open_role_count +
            static_cast<std::uint8_t>(key.half_open_role);
  ordinal = ordinal * symbolic_transition_count +
            static_cast<std::uint8_t>(key.transition);
  ordinal = ordinal * symbolic_occurrence_class_count +
            static_cast<std::uint8_t>(key.occurrence_class) - 1;
  return ordinal;
}

inline symbolic_rule_key symbolic_rule_key_from_ordinal(
    std::uint64_t ordinal) noexcept {
  symbolic_rule_key key;
  if (ordinal >= symbolic_rule_count) {
    key.schema_version = 0;
    return key;
  }
  key.occurrence_class = static_cast<symbolic_occurrence_class>(
      ordinal % symbolic_occurrence_class_count + 1);
  ordinal /= symbolic_occurrence_class_count;
  key.transition = static_cast<symbolic_transition_orientation>(
      ordinal % symbolic_transition_count);
  ordinal /= symbolic_transition_count;
  key.half_open_role = static_cast<symbolic_half_open_role>(
      ordinal % symbolic_half_open_role_count);
  ordinal /= symbolic_half_open_role_count;
  key.ownership_role = static_cast<symbolic_ownership_role>(
      ordinal % symbolic_ownership_role_count + 1);
  ordinal /= symbolic_ownership_role_count;
  key.orientation = static_cast<orientation_relation>(
      ordinal % symbolic_orientation_count + 1);
  ordinal /= symbolic_orientation_count;
  key.relation = static_cast<relation_family>(
      ordinal % symbolic_relation_family_count + 1);
  ordinal /= symbolic_relation_family_count;
  key.acting_operand =
      static_cast<operand_id>(ordinal % symbolic_operand_count);
  ordinal /= symbolic_operand_count;
  key.operation = static_cast<boolean_operation>(ordinal + 1);
  return key;
}

inline operand_id symbolic_preferred_operand(boolean_operation operation) noexcept {
  return operation == boolean_operation::b_minus_a ? operand_id::b
                                                    : operand_id::a;
}

inline bool valid_symbolic_tie_key_description(
    const symbolic_tie_key_description &description) noexcept {
  static constexpr std::array<symbolic_tie_key_component,
                              symbolic_tie_key_component_count>
      expected{
          symbolic_tie_key_component::final_orientation_capability,
          symbolic_tie_key_component::operand_priority,
          symbolic_tie_key_component::symbolic_feature_priority,
          symbolic_tie_key_component::canonical_source_feature,
          symbolic_tie_key_component::directed_use,
          symbolic_tie_key_component::occurrence,
      };
  return description.components == expected &&
         (description.preferred_operand == operand_id::a ||
          description.preferred_operand == operand_id::b) &&
         description.schema_version == contract_versions::symbolic_policy &&
         description.reserved == 0;
}

inline symbolic_contact_class
symbolic_contact_class_for(relation_family family) noexcept {
  switch (family) {
  case relation_family::vertex_vertex:
  case relation_family::vertex_edge:
  case relation_family::vertex_face:
    return symbolic_contact_class::point_contact;
  case relation_family::edge_edge:
  case relation_family::edge_face:
  case relation_family::equal_edge:
    return symbolic_contact_class::edge_contact;
  case relation_family::tangent:
    return symbolic_contact_class::tangent_contact;
  case relation_family::coplanar:
    return symbolic_contact_class::coplanar_overlap;
  case relation_family::coincident_face:
    return symbolic_contact_class::coincident_sheet;
  }
  return symbolic_contact_class::point_contact;
}

inline symbolic_explanation_code
symbolic_explanation_for(relation_family family) noexcept {
  switch (family) {
  case relation_family::vertex_vertex:
  case relation_family::vertex_edge:
  case relation_family::vertex_face:
    return symbolic_explanation_code::exact_vertex_tie;
  case relation_family::edge_edge:
  case relation_family::equal_edge:
    return symbolic_explanation_code::exact_edge_tie;
  case relation_family::edge_face:
    return symbolic_explanation_code::exact_edge_face_tie;
  case relation_family::tangent:
    return symbolic_explanation_code::exact_tangent_contact;
  case relation_family::coplanar:
    return symbolic_explanation_code::exact_coplanar_overlap;
  case relation_family::coincident_face:
    return symbolic_explanation_code::exact_coincident_sheet;
  }
  return symbolic_explanation_code::exact_vertex_tie;
}

inline symbolic_offset_disposition symbolic_offset_for(
    const symbolic_rule_key &key) noexcept {
  std::int8_t value = 0;
  switch (key.transition) {
  case symbolic_transition_orientation::negative_to_positive:
    value = 1;
    break;
  case symbolic_transition_orientation::positive_to_negative:
    value = -1;
    break;
  case symbolic_transition_orientation::tangent:
    value = 0;
    break;
  case symbolic_transition_orientation::none:
    value = key.orientation == orientation_relation::same
                ? std::int8_t{1}
            : key.orientation == orientation_relation::opposite
                ? std::int8_t{-1}
                : std::int8_t{0};
    if (key.acting_operand == operand_id::b)
      value = static_cast<std::int8_t>(-value);
    break;
  }
  return value < 0 ? symbolic_offset_disposition::negative
                   : value > 0 ? symbolic_offset_disposition::positive
                               : symbolic_offset_disposition::coincident;
}

inline symbolic_expected_disposition symbolic_expected_disposition_for(
    const symbolic_rule_key &key) noexcept {
  if (key.occurrence_class == symbolic_occurrence_class::coincident_sheet ||
      key.relation == relation_family::coincident_face)
    return symbolic_expected_disposition::coincident_owner_eligibility;
  if (key.occurrence_class != symbolic_occurrence_class::ordinary ||
      key.relation == relation_family::vertex_vertex ||
      key.relation == relation_family::equal_edge ||
      key.relation == relation_family::tangent)
    return symbolic_expected_disposition::occurrence_separation;
  if (key.half_open_role != symbolic_half_open_role::none)
    return symbolic_expected_disposition::half_open_ownership;
  return symbolic_expected_disposition::classification_only;
}

inline symbolic_rule make_symbolic_rule(const symbolic_rule_key &key) noexcept {
  symbolic_rule rule;
  rule.key = key;
  rule.conceptual_offset = symbolic_offset_for(key);
  const auto family = static_cast<std::uint8_t>(key.relation);
  const auto occurrence = static_cast<std::uint8_t>(key.occurrence_class);
  const auto ownership = static_cast<std::uint8_t>(key.ownership_role);
  const auto symmetric_ownership =
      ownership <= 2 ? std::uint8_t{1} : ownership;
  rule.feature_priority = static_cast<std::uint8_t>(
      family * 16U + occurrence * 2U + symmetric_ownership);
  rule.tie_key.preferred_operand = symbolic_preferred_operand(key.operation);
  rule.tie_key.feature_priority = rule.feature_priority;
  switch (key.ownership_role) {
  case symbolic_ownership_role::acting_source_feature:
    rule.half_open_owner = key.acting_operand;
    break;
  case symbolic_ownership_role::opposite_source_feature:
    rule.half_open_owner = symbolic_opposite_operand(key.acting_operand);
    break;
  case symbolic_ownership_role::shared_source_feature:
  case symbolic_ownership_role::coincident_sheet_pair:
    rule.half_open_owner = symbolic_preferred_operand(key.operation);
    break;
  }
  rule.crossing_contribution =
      key.transition == symbolic_transition_orientation::negative_to_positive
          ? std::int8_t{1}
      : key.transition ==
                symbolic_transition_orientation::positive_to_negative
          ? std::int8_t{-1}
          : std::int8_t{0};
  rule.contact_class = symbolic_contact_class_for(key.relation);
  rule.coincident_owner = symbolic_preferred_operand(key.operation);
  rule.expected_disposition = symbolic_expected_disposition_for(key);
  rule.owner_rank_eligible =
      rule.expected_disposition ==
          symbolic_expected_disposition::coincident_owner_eligibility ||
      key.relation == relation_family::equal_edge;
  rule.occurrence_separation_required =
      rule.expected_disposition ==
          symbolic_expected_disposition::occurrence_separation ||
      rule.expected_disposition ==
          symbolic_expected_disposition::coincident_owner_eligibility;
  rule.explanation = symbolic_explanation_for(key.relation);
  rule.exchanged_key = exchange_symbolic_rule_key(key);
  rule.exchange_rule_ordinal = symbolic_rule_ordinal(rule.exchanged_key);
  return rule;
}

inline void encode_symbolic_rule_key(canonical_writer &writer,
                                     const symbolic_rule_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.operation));
  writer.u8(static_cast<std::uint8_t>(key.acting_operand));
  writer.u8(static_cast<std::uint8_t>(key.relation));
  writer.u8(static_cast<std::uint8_t>(key.orientation));
  writer.u8(static_cast<std::uint8_t>(key.ownership_role));
  writer.u8(static_cast<std::uint8_t>(key.half_open_role));
  writer.u8(static_cast<std::uint8_t>(key.transition));
  writer.u8(static_cast<std::uint8_t>(key.occurrence_class));
  writer.u16(key.schema_version);
  writer.u16(key.reserved);
}

inline void encode_symbolic_tie_key_description(
    canonical_writer &writer,
    const symbolic_tie_key_description &description) {
  for (const auto component : description.components)
    writer.u8(static_cast<std::uint8_t>(component));
  writer.u8(static_cast<std::uint8_t>(description.preferred_operand));
  writer.u8(description.feature_priority);
  writer.u16(description.schema_version);
  writer.u16(description.reserved);
}

inline void encode_symbolic_rule(canonical_writer &writer,
                                 const symbolic_rule &rule) {
  encode_symbolic_rule_key(writer, rule.key);
  writer.u8(static_cast<std::uint8_t>(rule.conceptual_offset));
  writer.u8(rule.feature_priority);
  writer.u8(static_cast<std::uint8_t>(rule.half_open_owner));
  writer.u8(static_cast<std::uint8_t>(rule.crossing_contribution));
  writer.u8(static_cast<std::uint8_t>(rule.contact_class));
  writer.u8(static_cast<std::uint8_t>(rule.coincident_owner));
  writer.u8(static_cast<std::uint8_t>(rule.expected_disposition));
  writer.boolean(rule.owner_rank_eligible);
  writer.boolean(rule.occurrence_separation_required);
  encode_symbolic_tie_key_description(writer, rule.tie_key);
  writer.u16(rule.tie_key_schema);
  writer.u16(static_cast<std::uint16_t>(rule.explanation));
  encode_symbolic_rule_key(writer, rule.exchanged_key);
  writer.u64(rule.exchange_rule_ordinal);
  writer.u32(rule.reserved);
}

inline symbolic_policy_table build_symbolic_policy() {
  symbolic_policy_table table;
  table.rules.reserve(static_cast<std::size_t>(symbolic_rule_count));
  canonical_writer writer;
  writer.u16(contract_versions::symbolic_policy);
  writer.u64(symbolic_rule_count);
  for (std::uint64_t ordinal = 0; ordinal < symbolic_rule_count; ++ordinal) {
    const auto rule = make_symbolic_rule(symbolic_rule_key_from_ordinal(ordinal));
    table.rules.push_back(rule);
    encode_symbolic_rule(writer, rule);
  }
  table.bytes = writer.take();
  table.digest = sha256::digest(table.bytes);
  return table;
}

inline const symbolic_policy_table &materialize_symbolic_policy() {
  static const symbolic_policy_table table = build_symbolic_policy();
  return table;
}

inline bool verify_symbolic_policy(const symbolic_policy_table &table) {
  if (table.rules.size() != symbolic_rule_count ||
      sha256::digest(table.bytes) != table.digest)
    return false;
  canonical_writer writer;
  writer.u16(contract_versions::symbolic_policy);
  writer.u64(symbolic_rule_count);
  for (std::uint64_t ordinal = 0; ordinal < symbolic_rule_count; ++ordinal) {
    const auto key = symbolic_rule_key_from_ordinal(ordinal);
    const auto expected = make_symbolic_rule(key);
    const auto &rule = table.rules[static_cast<std::size_t>(ordinal)];
    if (!(rule == expected) ||
        !valid_symbolic_tie_key_description(rule.tie_key) ||
        rule.tie_key.feature_priority != rule.feature_priority ||
        rule.tie_key.preferred_operand != rule.coincident_owner ||
        rule.exchange_rule_ordinal >= symbolic_rule_count)
      return false;
    const auto &exchanged =
        table.rules[static_cast<std::size_t>(rule.exchange_rule_ordinal)];
    if (exchanged.key != rule.exchanged_key ||
        exchanged.exchanged_key != rule.key ||
        exchanged.exchange_rule_ordinal != ordinal ||
        exchanged.feature_priority != rule.feature_priority ||
        exchanged.contact_class != rule.contact_class ||
        exchanged.expected_disposition != rule.expected_disposition ||
        exchanged.tie_key.components != rule.tie_key.components ||
        exchanged.tie_key.feature_priority != rule.tie_key.feature_priority ||
        exchanged.owner_rank_eligible != rule.owner_rank_eligible ||
        exchanged.occurrence_separation_required !=
            rule.occurrence_separation_required ||
        static_cast<std::int8_t>(exchanged.conceptual_offset) !=
            -static_cast<std::int8_t>(rule.conceptual_offset) ||
        exchanged.crossing_contribution != -rule.crossing_contribution)
      return false;
    encode_symbolic_rule(writer, rule);
  }
  return writer.bytes() == table.bytes;
}

} // namespace ygor::mesh_boolean::bounded
