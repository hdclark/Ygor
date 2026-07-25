#pragma once
#ifndef YGOR_MESHES_BOOLEAN_ATTRIBUTES_H_
#define YGOR_MESHES_BOOLEAN_ATTRIBUTES_H_

#include "YgorMeshesBooleanProductContractPolicies.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t attribute_source_input_schema = 1;
constexpr std::uint16_t attribute_transfer_report_schema = 1;
constexpr std::uint16_t attribute_transfer_checker_version = 1;
constexpr std::uint16_t attribute_output_binding_schema = 1;
constexpr std::uint64_t attribute_unmapped_id =
    std::numeric_limits<std::uint64_t>::max();

enum class attribute_source_entity_kind : std::uint8_t {
  body,
  shell,
  facet,
  vertex,
  edge
};

enum class attribute_channel_kind : std::uint8_t {
  source_body_id,
  source_shell_id,
  source_facet_id,
  material,
  face_metadata,
  vertex_normal,
  vertex_colour,
  sharp_edge,
  texture_seam,
  opaque,
  construction_provenance
};

enum class attribute_target_kind : std::uint8_t {
  exact_vertex,
  exact_edge,
  exact_patch,
  output_vertex,
  output_face
};

enum class attribute_resolution_kind : std::uint8_t {
  copied,
  split_copy,
  merged_equal,
  representative_copy,
  set_union,
  any_source,
  compact_construction,
  omitted
};

enum class attribute_issue_kind : std::uint8_t { omission, conflict };
enum class attribute_issue_reason : std::uint8_t {
  policy_omits_channel,
  absent_source_value,
  constructed_entity_has_no_source_value,
  removed_internal_entity,
  unequal_source_values,
  texture_seam_mismatch,
  interpolation_prohibited,
  unavailable_source_mapping
};

struct attribute_raw_entity_ref {
  operand_id operand = operand_a();
  attribute_source_entity_kind kind = attribute_source_entity_kind::body;
  std::uint64_t primary = 0;
  std::uint64_t secondary = 0;
};
inline bool operator==(const attribute_raw_entity_ref &a,
                       const attribute_raw_entity_ref &b) noexcept {
  return a.operand == b.operand && a.kind == b.kind &&
         a.primary == b.primary && a.secondary == b.secondary;
}
inline bool operator<(const attribute_raw_entity_ref &a,
                      const attribute_raw_entity_ref &b) noexcept {
  return std::tie(a.operand, a.kind, a.primary, a.secondary) <
         std::tie(b.operand, b.kind, b.primary, b.secondary);
}

struct source_attribute_value_input {
  attribute_raw_entity_ref entity;
  attribute_channel_kind channel = attribute_channel_kind::opaque;
  std::string name;
  std::vector<std::uint8_t> value;
};
struct source_attribute_input {
  std::uint16_t schema = attribute_source_input_schema;
  operand_id operand = operand_a();
  std::string body_id;
  std::vector<source_attribute_value_input> values;
};

struct attribute_source_entity_record {
  attribute_raw_entity_ref source;
  std::uint64_t prepared_id = attribute_unmapped_id;
  bool retained = true;
};
struct attribute_source_value_record {
  std::uint64_t source_entity = 0;
  attribute_channel_kind channel = attribute_channel_kind::opaque;
  std::string name;
  std::vector<std::uint8_t> value;
  digest value_digest;
};
struct attribute_source_catalog {
  std::uint16_t schema = attribute_source_input_schema;
  operand_id operand = operand_a();
  std::string body_id;
  std::vector<attribute_source_entity_record> entities;
  std::vector<attribute_source_value_record> values;
  digest catalog_digest;
};

struct attribute_exact_entity_mapping {
  attribute_target_kind target = attribute_target_kind::exact_vertex;
  std::uint64_t target_id = 0;
  std::vector<std::uint64_t> source_entities;
  std::vector<std::uint64_t> transfers;
  digest provenance_digest;
};
struct attribute_output_entity_mapping {
  attribute_target_kind target = attribute_target_kind::output_vertex;
  std::uint64_t output_id = 0;
  attribute_target_kind exact_target = attribute_target_kind::exact_vertex;
  std::uint64_t exact_id = 0;
  std::vector<std::uint64_t> transfers;
};
struct attribute_transfer_record {
  attribute_target_kind target = attribute_target_kind::exact_vertex;
  std::uint64_t target_id = 0;
  attribute_channel_kind channel = attribute_channel_kind::opaque;
  std::string name;
  attribute_resolution_kind resolution = attribute_resolution_kind::omitted;
  std::vector<std::uint64_t> source_values;
  std::vector<std::uint8_t> value;
  digest value_digest;
};
struct attribute_issue_record {
  attribute_issue_kind kind = attribute_issue_kind::omission;
  attribute_issue_reason reason = attribute_issue_reason::policy_omits_channel;
  attribute_target_kind target = attribute_target_kind::exact_vertex;
  std::uint64_t target_id = 0;
  attribute_channel_kind channel = attribute_channel_kind::opaque;
  std::string name;
  std::vector<std::uint64_t> source_entities;
  std::vector<std::uint64_t> source_values;
};

struct attribute_output_binding {
  std::uint16_t schema = attribute_output_binding_schema;
  coordinate_tag coordinate = coordinate_tag::binary32;
  index_tag index = index_tag::uint32;
  digest exact_result_digest;
  digest output_digest;
  std::vector<std::uint64_t> output_vertex_exact_vertices;
  std::vector<std::uint64_t> output_face_exact_patches;
};

struct attribute_transfer_report {
  std::uint16_t schema = attribute_transfer_report_schema;
  std::uint16_t checker_version = attribute_transfer_checker_version;
  attribute_transfer_policy_contract policy;
  digest policy_digest;
  digest exact_result_digest;
  std::optional<digest> output_digest;
  std::array<attribute_source_catalog, 2> sources;
  std::vector<attribute_exact_entity_mapping> exact_mappings;
  std::vector<attribute_output_entity_mapping> output_mappings;
  std::vector<attribute_transfer_record> transfers;
  std::vector<attribute_issue_record> issues;
  std::uint64_t omissions = 0;
  std::uint64_t conflicts = 0;
  std::vector<std::uint8_t> canonical_bytes;
  digest report_digest;
};

struct attribute_decode_limits {
  std::uint64_t max_record_bytes = 64ULL * 1024ULL * 1024ULL;
  std::uint64_t max_entities = 4000000;
  std::uint64_t max_values = 8000000;
  std::uint64_t max_mappings = 8000000;
  std::uint64_t max_transfers = 8000000;
  std::uint64_t max_issues = 8000000;
  std::uint64_t max_references = 64000000;
  std::uint64_t max_string_bytes = 16ULL * 1024ULL * 1024ULL;
  std::uint64_t max_value_bytes = 64ULL * 1024ULL * 1024ULL;
};

class exact_result_handle;
template <class T, class I> class boolean_context;

template <class T, class I>
product_status_or<std::array<attribute_source_catalog, 2>>
make_attribute_source_catalogs(
    boolean_context<T, I> &,
    const std::array<source_attribute_input, 2> * = nullptr);

product_status_or<std::vector<std::uint8_t>>
encode_source_attribute_input(const source_attribute_input &);
product_status_or<source_attribute_input>
decode_source_attribute_input(const std::vector<std::uint8_t> &,
                              const attribute_decode_limits & = {});

product_status_or<std::vector<std::uint8_t>>
encode_attribute_transfer_report(const attribute_transfer_report &);
product_status_or<attribute_transfer_report>
decode_attribute_transfer_report(const std::vector<std::uint8_t> &,
                                 const attribute_decode_limits & = {});

product_status_or<attribute_transfer_report> make_attribute_transfer_report(
    const exact_result_handle &, const attribute_transfer_policy_contract &,
    std::array<attribute_source_catalog, 2>,
    const attribute_output_binding * = nullptr);

product_status_or<bool> validate_attribute_output_binding(
    const attribute_output_binding &, const exact_result_handle &) noexcept;
product_status_or<bool> verify_attribute_transfer_report(
    const attribute_transfer_report &, const exact_result_handle &,
    const attribute_output_binding * = nullptr) noexcept;
product_status_or<bool> verify_serialized_attribute_transfer_report(
    const std::vector<std::uint8_t> &, const exact_result_handle &,
    const attribute_output_binding * = nullptr,
    const attribute_decode_limits & = {}) noexcept;

#define YGOR_ATTRIBUTE_EXTERN(T, I)                                             \
  extern template product_status_or<                                           \
      std::array<attribute_source_catalog, 2>>                                  \
  make_attribute_source_catalogs(boolean_context<T, I> &,                      \
                                 const std::array<source_attribute_input, 2> *)
YGOR_ATTRIBUTE_EXTERN(float, std::uint32_t);
YGOR_ATTRIBUTE_EXTERN(float, std::uint64_t);
YGOR_ATTRIBUTE_EXTERN(double, std::uint32_t);
YGOR_ATTRIBUTE_EXTERN(double, std::uint64_t);
#undef YGOR_ATTRIBUTE_EXTERN

} // namespace mesh_boolean
} // namespace ygor

#endif
