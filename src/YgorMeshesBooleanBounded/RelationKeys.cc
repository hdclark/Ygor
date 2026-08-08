#include "StrictFloatingBuild.h"
#include "RelationKeys.h"

namespace ygor::mesh_boolean::bounded {
namespace {

bool valid_operand(operand_id operand) noexcept {
  return operand == operand_id::a || operand == operand_id::b;
}

bool valid_feature_kind(relation_feature_kind kind, bool allow_none) noexcept {
  const auto value = static_cast<std::uint8_t>(kind);
  return (allow_none && value == 0) || (value >= 1 && value <= 6);
}

void encode_digest(canonical_writer &writer,
                   const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    writer.u8(byte);
}

} // namespace

bool valid_relation_feature_key(const relation_feature_key &key,
                                bool allow_none) noexcept {
  if (!valid_operand(key.operand) ||
      !valid_feature_kind(key.kind, allow_none) || key.schema_version != contract_versions::relation_feature_key_schema)
    return false;
  if (key.kind == relation_feature_kind::none)
    return allow_none && key.primary == 0 && key.secondary == 0 &&
           key.occurrence == 0;
  if (key.kind != relation_feature_kind::sheet_occurrence &&
      key.occurrence != 0)
    return false;
  if (key.kind == relation_feature_kind::source_vertex && key.secondary != 0)
    return false;
  return true;
}

bool valid_relation_request_key(const relation_request_key &key) noexcept {
  const auto family = static_cast<std::uint8_t>(key.family);
  const auto scope = static_cast<std::uint8_t>(key.scope);
  if (family < 1 || family > 15 || scope < 1 || scope > 2 ||
      !valid_relation_feature_key(key.first) ||
      !valid_relation_feature_key(key.second, true) ||
      key.formula_version == 0 || key.policy_version == 0 || key.reserved != 0)
    return false;
  if (key.scope == relation_record_scope::public_source_feature &&
      key.second.kind != relation_feature_kind::none &&
      key.first.operand == key.second.operand)
    return false;
  if (key.scope == relation_record_scope::public_source_feature &&
      (key.first.kind == relation_feature_kind::source_triangle ||
       key.first.kind == relation_feature_kind::facet_internal_diagonal ||
       key.second.kind == relation_feature_kind::source_triangle ||
       key.second.kind == relation_feature_kind::facet_internal_diagonal))
    return false;
  return true;
}

bool valid_relation_event_seed_key(const relation_event_seed_key &key) noexcept {
  const auto family = static_cast<std::uint8_t>(key.family);
  return family >= 1 && family <= 6 &&
         valid_relation_feature_key(key.first) &&
         valid_relation_feature_key(key.second) &&
         key.first.operand != key.second.operand &&
         key.equivalence_policy_version == contract_versions::relation_event_seed_key_schema && key.reserved == 0;
}

relation_feature_key remap_relation_feature_key(
    relation_feature_key key) noexcept {
  key.operand = key.operand == operand_id::a ? operand_id::b : operand_id::a;
  return key;
}

relation_request_key remap_relation_request_key(
    relation_request_key key) noexcept {
  key.first = remap_relation_feature_key(key.first);
  if (key.second.kind != relation_feature_kind::none)
    key.second = remap_relation_feature_key(key.second);
  return key;
}

void encode_relation_feature_key(canonical_writer &writer,
                                 const relation_feature_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.operand));
  writer.u8(static_cast<std::uint8_t>(key.kind));
  writer.u64(key.primary);
  writer.u64(key.secondary);
  writer.u32(key.occurrence);
  writer.u16(key.schema_version);
}

void encode_relation_request_key(canonical_writer &writer,
                                 const relation_request_key &key) {
  encode_digest(writer, key.semantic_namespace);
  writer.u8(static_cast<std::uint8_t>(key.family));
  writer.u8(static_cast<std::uint8_t>(key.scope));
  encode_relation_feature_key(writer, key.first);
  encode_relation_feature_key(writer, key.second);
  writer.u64(key.directed_use);
  writer.u16(key.formula_version);
  writer.u16(key.policy_version);
  writer.u32(key.occurrence_discriminator);
  writer.u32(key.reserved);
}

void encode_relation_event_seed_key(canonical_writer &writer,
                                    const relation_event_seed_key &key) {
  encode_digest(writer, key.semantic_namespace);
  writer.u8(static_cast<std::uint8_t>(key.family));
  encode_relation_feature_key(writer, key.first);
  encode_relation_feature_key(writer, key.second);
  writer.u32(key.occurrence);
  writer.u16(key.equivalence_policy_version);
  writer.u16(key.reserved);
}

std::vector<std::uint8_t>
encode_relation_request_key(const relation_request_key &key) {
  canonical_writer writer;
  encode_relation_request_key(writer, key);
  return writer.take();
}

} // namespace ygor::mesh_boolean::bounded
