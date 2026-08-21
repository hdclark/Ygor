#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_CORPUS_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_CORPUS_H_

#include "YgorMeshesBooleanQualification.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_corpus_inventory_schema_version = 1;
constexpr std::uint64_t qualification_generated_pair_floor = 10000;
constexpr std::uint64_t qualification_cad_like_pair_floor = 1000;
constexpr std::uint64_t qualification_operation_chain_floor = 1000;
constexpr std::uint32_t qualification_operation_chain_step_floor = 5;

enum class qualification_corpus_record_kind : std::uint8_t {
  generated_pair_family,
  cad_like_pair_family,
  operation_chain_family,
  minimized_regression
};

enum class qualification_operand_order : std::uint8_t {
  a_then_b,
  b_then_a
};

enum class qualification_geometry_category : std::uint8_t {
  non_box_intersection,
  rotated_or_skewed_convex,
  concave_or_reentrant,
  disconnected_components,
  nested_shells_or_cavities,
  coplanar_overlay,
  high_valence_contact,
  thin_sliver_or_dense,
  alternate_subdivision,
  scale_extremes,
  floating_point_edge_cases,
  non_dyadic_intersection,
  stratified_non_manifold,
  index_or_resource_boundary,
  attribute_or_provenance_conflict,
  normalization_defect,
  serialization_or_replay,
  count
};

struct qualification_operation_coverage {
  operation selected_operation = operation::regularized_union;
  qualification_operand_order operand_order =
      qualification_operand_order::a_then_b;
};

inline bool operator<(const qualification_operation_coverage &a,
                      const qualification_operation_coverage &b) noexcept {
  return static_cast<unsigned>(a.selected_operation) <
             static_cast<unsigned>(b.selected_operation) ||
         (a.selected_operation == b.selected_operation &&
          static_cast<unsigned>(a.operand_order) <
              static_cast<unsigned>(b.operand_order));
}
inline bool operator==(const qualification_operation_coverage &a,
                       const qualification_operation_coverage &b) noexcept {
  return a.selected_operation == b.selected_operation &&
         a.operand_order == b.operand_order;
}

struct qualification_corpus_record {
  std::uint16_t schema = qualification_corpus_inventory_schema_version;
  std::string identifier;
  qualification_corpus_record_kind kind =
      qualification_corpus_record_kind::generated_pair_family;
  qualification_corpus_source source =
      qualification_corpus_source::generated_construction_known;
  qualification_redistribution redistribution =
      qualification_redistribution::repository_embedded;
  std::string license_or_provenance;
  std::string recipe_identifier;
  std::string recipe_version;
  digest recipe_digest;
  std::uint64_t first_case_ordinal = 0;
  std::uint64_t case_count = 0;
  std::uint32_t minimum_chain_steps = 0;
  std::uint32_t maximum_chain_steps = 0;
  std::vector<qualification_geometry_category> geometry_categories;
  std::vector<qualification_operation_coverage> operations;
  std::vector<qualification_type_binding> type_specializations;
  std::vector<qualification_result_mode_binding> result_modes;
  std::vector<qualification_preparation_binding> preparation_policies;
  std::vector<qualification_outcome> expected_outcomes;
  std::vector<product_error_code> expected_failure_codes;
  std::string expectation_identifier;
  digest expectation_digest;
  std::string permanent_test_id;
  digest record_digest;
};

struct qualification_corpus_inventory {
  std::uint16_t schema = qualification_corpus_inventory_schema_version;
  std::string identifier;
  std::string version;
  std::vector<qualification_corpus_record> records;
  std::uint64_t generated_pair_count = 0;
  std::uint64_t cad_like_pair_count = 0;
  std::uint64_t operation_chain_count = 0;
  std::uint64_t minimized_regression_count = 0;
  digest record_set_digest;
  digest category_coverage_digest;
  digest expected_outcome_digest;
  digest inventory_digest;
};

const char *qualification_corpus_record_kind_token(
    qualification_corpus_record_kind) noexcept;
const char *qualification_operand_order_token(
    qualification_operand_order) noexcept;
const char *qualification_geometry_category_token(
    qualification_geometry_category) noexcept;

std::vector<qualification_geometry_category>
required_qualification_geometry_categories();
std::vector<qualification_operation_coverage>
required_qualification_operation_coverage();

product_status_or<qualification_corpus_inventory>
make_qualification_corpus_inventory(qualification_corpus_inventory);
product_status_or<bool> validate_qualification_corpus_inventory(
    const qualification_corpus_inventory &) noexcept;
product_status_or<std::vector<qualification_corpus_binding>>
make_qualification_corpus_bindings(
    const qualification_corpus_inventory &) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
