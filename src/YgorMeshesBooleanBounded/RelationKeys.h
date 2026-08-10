#pragma once

#include "RelationTypes.h"
#include "CanonicalBytes.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class relation_feature_kind : std::uint8_t {
  none = 0,
  source_vertex = 1,
  source_edge = 2,
  source_facet = 3,
  source_triangle = 4,
  facet_internal_diagonal = 5,
  sheet_occurrence = 6,
};

struct relation_feature_key final {
  operand_id operand = operand_id::a;
  relation_feature_kind kind = relation_feature_kind::none;
  std::uint64_t primary = 0;
  std::uint64_t secondary = 0;
  std::uint32_t occurrence = 0;
  std::uint16_t schema_version = contract_versions::relation_feature_key_schema;

  friend bool operator<(const relation_feature_key &a,
                        const relation_feature_key &b) noexcept {
    return std::tie(a.operand, a.kind, a.primary, a.secondary, a.occurrence,
                    a.schema_version) <
           std::tie(b.operand, b.kind, b.primary, b.secondary, b.occurrence,
                    b.schema_version);
  }
  friend bool operator==(const relation_feature_key &a,
                         const relation_feature_key &b) noexcept {
    return std::tie(a.operand, a.kind, a.primary, a.secondary, a.occurrence,
                    a.schema_version) ==
           std::tie(b.operand, b.kind, b.primary, b.secondary, b.occurrence,
                    b.schema_version);
  }
  friend bool operator!=(const relation_feature_key &a,
                         const relation_feature_key &b) noexcept {
    return !(a == b);
  }
};

struct relation_request_key final {
  bounded_boolean_digest semantic_namespace{};
  relation_request_family family =
      relation_request_family::imported_source_geometry;
  relation_record_scope scope = relation_record_scope::public_source_feature;
  relation_feature_key first{};
  relation_feature_key second{};
  std::uint64_t directed_use = 0;
  std::uint16_t formula_version = contract_versions::exact_relation_formulas;
  std::uint16_t policy_version = contract_versions::relation_request_key_schema;
  std::uint32_t occurrence_discriminator = 0;
  std::uint32_t reserved = 0;

  friend bool operator<(const relation_request_key &a,
                        const relation_request_key &b) noexcept {
    return std::tie(a.semantic_namespace.bytes, a.family, a.scope, a.first,
                    a.second, a.directed_use, a.formula_version,
                    a.policy_version, a.occurrence_discriminator, a.reserved) <
           std::tie(b.semantic_namespace.bytes, b.family, b.scope, b.first,
                    b.second, b.directed_use, b.formula_version,
                    b.policy_version, b.occurrence_discriminator, b.reserved);
  }
  friend bool operator==(const relation_request_key &a,
                         const relation_request_key &b) noexcept {
    return std::tie(a.semantic_namespace.bytes, a.family, a.scope, a.first,
                    a.second, a.directed_use, a.formula_version,
                    a.policy_version, a.occurrence_discriminator, a.reserved) ==
           std::tie(b.semantic_namespace.bytes, b.family, b.scope, b.first,
                    b.second, b.directed_use, b.formula_version,
                    b.policy_version, b.occurrence_discriminator, b.reserved);
  }
  friend bool operator!=(const relation_request_key &a,
                         const relation_request_key &b) noexcept {
    return !(a == b);
  }
};

struct relation_event_seed_key final {
  bounded_boolean_digest semantic_namespace{};
  feature_relation_family family =
      feature_relation_family::source_vertex_source_facet;
  relation_feature_key first{};
  relation_feature_key second{};
  std::uint32_t occurrence = 0;
  std::uint16_t equivalence_policy_version =
      contract_versions::relation_event_seed_key_schema;
  std::uint16_t reserved = 0;

  friend bool operator<(const relation_event_seed_key &a,
                        const relation_event_seed_key &b) noexcept {
    return std::tie(a.semantic_namespace.bytes, a.family, a.first, a.second,
                    a.occurrence, a.equivalence_policy_version, a.reserved) <
           std::tie(b.semantic_namespace.bytes, b.family, b.first, b.second,
                    b.occurrence, b.equivalence_policy_version, b.reserved);
  }
  friend bool operator==(const relation_event_seed_key &a,
                         const relation_event_seed_key &b) noexcept {
    return std::tie(a.semantic_namespace.bytes, a.family, a.first, a.second,
                    a.occurrence, a.equivalence_policy_version, a.reserved) ==
           std::tie(b.semantic_namespace.bytes, b.family, b.first, b.second,
                    b.occurrence, b.equivalence_policy_version, b.reserved);
  }
};

bool valid_relation_feature_key(const relation_feature_key &key,
                                bool allow_none = false) noexcept;
bool valid_relation_request_key(const relation_request_key &key) noexcept;
bool valid_relation_event_seed_key(const relation_event_seed_key &key) noexcept;
relation_feature_key remap_relation_feature_key(
    relation_feature_key key) noexcept;
relation_request_key remap_relation_request_key(
    relation_request_key key) noexcept;

void encode_relation_feature_key(canonical_writer &writer,
                                 const relation_feature_key &key);
void encode_relation_request_key(canonical_writer &writer,
                                 const relation_request_key &key);
void encode_relation_event_seed_key(canonical_writer &writer,
                                    const relation_event_seed_key &key);
std::vector<std::uint8_t>
encode_relation_request_key(const relation_request_key &key);

} // namespace ygor::mesh_boolean::bounded
