#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_GENERATION_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_GENERATION_H_

#include "YgorMeshesBooleanQualificationCorpus.h"
#include "YgorMeshesExactArithmetic.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <new>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_generation_schema_version = 1;
constexpr std::uint32_t qualification_generator_algorithm_version = 1;
constexpr std::uint64_t qualification_default_minimization_attempt_limit = 10000;

enum class qualification_generator_family : std::uint8_t {
  exact_halfspace_skew_convex,
  exact_profile_extrusion_concave,
  exact_coplanar_overlay,
  exact_nested_shell_cavity,
  exact_feature_alignment_contact,
  exact_subdivision_refinement,
  exact_representable_scale_bits,
  exact_nondyadic_intersection,
  exact_thin_sliver_dense,
  exact_capacity_replay_boundary,
  cadlike_profile_extruded_part,
  cadlike_multibody_cavity,
  cadlike_thin_feature,
  cadlike_dense_tessellation,
  cadlike_attribute_seam_conflict,
  cadlike_controlled_defect,
  count
};

enum class qualification_construction_kind : std::uint8_t {
  exact_halfspace_polytope,
  profile_extrusion,
  coplanar_overlay,
  nested_shell_cavity,
  feature_alignment,
  exact_subdivision,
  representable_transform,
  nondyadic_intersection_inputs,
  thin_or_dense_feature,
  capacity_or_replay_boundary,
  cad_like_model,
  controlled_invalid_preparation
};

enum class qualification_known_relation : std::uint8_t {
  overlapping,
  disjoint,
  a_contains_b,
  b_contains_a,
  touching_only,
  equal_boundaries,
  controlled_invalid_operand
};

enum class qualification_defect_label : std::uint8_t {
  none,
  duplicate_vertex_use,
  duplicate_facet,
  inconsistent_orientation,
  open_crack,
  nonplanar_facet,
  overlapping_shells,
  sliver_feature,
  self_intersection,
  count
};

enum class qualification_recipe_derivation : std::uint8_t {
  inventory_ordinal,
  valid_fuzz_mutation,
  invalid_fuzz_mutation,
  minimized_regression_candidate
};

enum class qualification_valid_mutation : std::uint8_t {
  exact_contact,
  add_coplanarity,
  alternate_subdivision,
  split_or_merge_facets,
  add_cavity,
  add_disconnected_component,
  one_ulp_perturbation,
  representable_transform,
  increase_local_valence,
  chain_output_seed,
  count
};

enum class qualification_minimization_edit_kind : std::uint8_t {
  remove_face,
  remove_unreferenced_vertex,
  remove_chain_suffix,
  replace_coordinate_with_zero,
  halve_coordinate,
  reduce_case_ordinal
};

struct qualification_generator_descriptor {
  std::uint16_t schema = qualification_generation_schema_version;
  qualification_generator_family family =
      qualification_generator_family::exact_halfspace_skew_convex;
  std::string identifier;
  std::string version;
  digest recipe_digest;
  std::uint64_t first_case_ordinal = 0;
  std::uint64_t case_count = 0;
  qualification_corpus_record_kind corpus_kind =
      qualification_corpus_record_kind::generated_pair_family;
  std::vector<qualification_geometry_category> geometry_categories;
  digest implementation_digest;
};

struct qualification_recipe_coordinate {
  exact_rational value;
  bool negative_zero = false;
};

struct qualification_recipe_vertex {
  std::array<qualification_recipe_coordinate, 3> coordinate;
};

struct qualification_recipe_mesh {
  std::vector<qualification_recipe_vertex> vertices;
  std::vector<std::vector<std::uint64_t>> faces;
};

// A halfspace is a*x + b*y + c*z <= d.  The complete halfspace list is
// retained for construction-aware oracles; it is not inferred from the mesh.
struct qualification_exact_halfspace {
  exact_rational a;
  exact_rational b;
  exact_rational c;
  exact_rational d;
};

struct qualification_case_expectation {
  qualification_known_relation relation =
      qualification_known_relation::overlapping;
  qualification_defect_label defect = qualification_defect_label::none;
  std::vector<qualification_outcome> allowed_outcomes;
  std::vector<product_error_code> allowed_failure_codes;
  bool strict_operand_a_expected_valid = true;
  bool strict_operand_b_expected_valid = true;
  bool normalization_report_required = false;
};

struct qualification_case_recipe {
  std::uint16_t schema = qualification_generation_schema_version;
  std::string recipe_identifier;
  std::string recipe_version;
  digest recipe_digest;
  qualification_generator_family family =
      qualification_generator_family::exact_halfspace_skew_convex;
  qualification_construction_kind construction =
      qualification_construction_kind::exact_halfspace_polytope;
  std::uint64_t ordinal = 0;
  std::uint64_t deterministic_seed = 0;
  qualification_recipe_derivation derivation =
      qualification_recipe_derivation::inventory_ordinal;
  std::optional<qualification_valid_mutation> valid_mutation;
  digest parent_case_digest;
  std::vector<std::int64_t> retained_parameters;
  std::vector<qualification_geometry_category> geometry_categories;
  qualification_recipe_mesh operand_a;
  qualification_recipe_mesh operand_b;
  std::vector<qualification_exact_halfspace> operand_a_halfspaces;
  std::vector<qualification_exact_halfspace> operand_b_halfspaces;
  qualification_case_expectation expectation;
  digest case_digest;
};

struct qualification_chain_step_recipe {
  operation selected_operation = operation::regularized_union;
  qualification_operand_order operand_order =
      qualification_operand_order::a_then_b;
  preparation_mode preparation = preparation_mode::strict_validation;
  result_representation requested_result =
      result_representation::exact_stratified;
  qualification_case_recipe rhs_case;
  bool rhs_uses_operand_b = false;
  bool subdivide_accumulator_before_step = false;
  bool continue_after_expected_failure = false;
  bool continue_with_prior_mesh_when_unrealized = false;
  std::vector<product_error_code> expected_failure_codes;
};

struct qualification_operation_chain_recipe {
  std::uint16_t schema = qualification_generation_schema_version;
  std::string identifier;
  std::string version;
  digest definition_digest;
  std::uint64_t ordinal = 0;
  qualification_case_recipe initial_case;
  bool initial_uses_operand_b = false;
  std::vector<qualification_chain_step_recipe> steps;
  digest chain_digest;
};

struct qualification_minimization_edit {
  qualification_minimization_edit_kind kind =
      qualification_minimization_edit_kind::remove_face;
  std::uint8_t operand = 0;
  std::uint64_t primary_index = 0;
  std::uint64_t secondary_index = 0;
  digest before_digest;
  digest after_digest;
};

struct qualification_preparation_observation {
  std::uint16_t schema = qualification_generation_schema_version;
  bool strict_operand_a_valid = false;
  bool strict_operand_b_valid = false;
  bool normalization_attempted = false;
  bool normalization_succeeded = false;
  qualification_defect_label reported_defect = qualification_defect_label::none;
  std::uint64_t normalization_edit_count = 0;
  digest normalization_report_digest;
  bool prepared_operand_strictly_valid = false;
};

struct qualification_failure_provenance {
  std::uint16_t schema = qualification_generation_schema_version;
  std::vector<std::uint64_t> operand_a_faces;
  std::vector<std::uint64_t> operand_b_faces;
  std::vector<std::uint64_t> operand_a_vertices;
  std::vector<std::uint64_t> operand_b_vertices;
  std::optional<std::uint64_t> chain_step;
  digest evidence_digest;
};

struct qualification_minimization_result {
  std::uint16_t schema = qualification_generation_schema_version;
  qualification_case_recipe minimized;
  std::uint64_t attempts = 0;
  std::vector<qualification_minimization_edit> edits;
  digest transcript_digest;
};

struct qualification_regression_promotion {
  std::uint16_t schema = qualification_generation_schema_version;
  std::string identifier;
  std::string permanent_test_id;
  qualification_case_recipe minimized_case;
  std::vector<std::uint8_t> canonical_case_bytes;
  digest minimization_transcript_digest;
  digest artifact_digest;
};

const char *qualification_generator_family_token(
    qualification_generator_family) noexcept;
const char *qualification_defect_label_token(qualification_defect_label) noexcept;

std::vector<qualification_generator_descriptor>
qualification_generator_descriptors();
product_status_or<qualification_generator_descriptor>
find_qualification_generator_descriptor(const std::string &,
                                        const std::string &) noexcept;

product_status_or<qualification_case_recipe>
make_qualification_case_recipe(const std::string &, const std::string &,
                               std::uint64_t);
product_status_or<bool>
validate_qualification_case_recipe(const qualification_case_recipe &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_case_recipe(const qualification_case_recipe &);
product_status_or<qualification_case_recipe>
make_qualification_valid_fuzz_case(qualification_valid_mutation, std::uint64_t);
product_status_or<qualification_case_recipe>
make_qualification_invalid_fuzz_case(qualification_defect_label, std::uint64_t);
product_status_or<bool> validate_qualification_preparation_observation(
    const qualification_case_recipe &,
    const qualification_preparation_observation &) noexcept;

product_status_or<qualification_operation_chain_recipe>
make_qualification_operation_chain_recipe(const std::string &,
                                           const std::string &,
                                           std::uint64_t);
product_status_or<bool> validate_qualification_operation_chain_recipe(
    const qualification_operation_chain_recipe &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_operation_chain_recipe(
    const qualification_operation_chain_recipe &);

product_status_or<qualification_failure_provenance>
make_qualification_failure_provenance(qualification_failure_provenance);
product_status_or<bool> validate_qualification_failure_provenance(
    const qualification_failure_provenance &) noexcept;

using qualification_reproduction_predicate =
    std::function<bool(const qualification_case_recipe &)>;
product_status_or<qualification_minimization_result>
minimize_qualification_case(
    const qualification_case_recipe &, const qualification_failure_provenance &,
    const qualification_reproduction_predicate &,
    std::uint64_t attempt_limit =
        qualification_default_minimization_attempt_limit);
product_status_or<qualification_minimization_result>
minimize_qualification_case(
    const qualification_case_recipe &, const qualification_reproduction_predicate &,
    std::uint64_t attempt_limit =
        qualification_default_minimization_attempt_limit);

product_status_or<qualification_regression_promotion>
make_qualification_regression_promotion(
    const qualification_minimization_result &);
using qualification_regression_sink =
    std::function<product_status_or<bool>(
        const qualification_regression_promotion &)>;
product_status_or<bool> promote_qualification_regression(
    const qualification_regression_promotion &,
    const qualification_regression_sink &);

namespace qualification_generation_detail {
product_error generation_error(product_error_code, const char *);
product_status_or<qualification_case_recipe>
canonicalize_qualification_case(qualification_case_recipe);
product_status_or<qualification_operation_chain_recipe>
canonicalize_qualification_chain(qualification_operation_chain_recipe);
qualification_recipe_mesh subdivide_qualification_mesh(
    const qualification_recipe_mesh &);
} // namespace qualification_generation_detail

template <class T>
product_status_or<T>
materialize_qualification_coordinate(const qualification_recipe_coordinate &c) {
  if (c.negative_zero && !c.value.is_zero())
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_generator.negative_zero_nonzero");
  auto rounded = round_binary_nearest_even<T>(c.value);
  if (!rounded)
    return qualification_generation_detail::generation_error(
        product_error_code::output_not_representable,
        "qualification_generator.coordinate_range");
  auto comparison = compare_binary_bits<T>(*rounded, c.value);
  if (!comparison || *comparison != 0)
    return qualification_generation_detail::generation_error(
        product_error_code::output_not_representable,
        "qualification_generator.coordinate_not_exact_in_type");
  if (c.negative_zero) {
    if constexpr (std::is_same<T, float>::value)
      rounded->bits = std::uint32_t(1) << 31U;
    else if constexpr (std::is_same<T, double>::value)
      rounded->bits = std::uint64_t(1) << 63U;
    else
      return qualification_generation_detail::generation_error(
          product_error_code::unsupported_platform,
          "qualification_generator.unsupported_coordinate_type");
  }
  return value_of_bits<T>(*rounded);
}

template <class T, class I>
product_status_or<fv_surface_mesh<T, I>>
materialize_qualification_mesh(const qualification_recipe_mesh &recipe) {
  if (recipe.vertices.size() >
      static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
    return qualification_generation_detail::generation_error(
        product_error_code::index_overflow,
        "qualification_generator.vertex_index_capacity");
  fv_surface_mesh<T, I> mesh;
  try {
    mesh.vertices.reserve(recipe.vertices.size());
    mesh.faces.reserve(recipe.faces.size());
    for (const auto &v : recipe.vertices) {
      std::array<T, 3> coordinate{};
      for (std::size_t axis = 0; axis != 3; ++axis) {
        auto made = materialize_qualification_coordinate<T>(v.coordinate[axis]);
        if (!made.has_value())
          return made.error();
        coordinate[axis] = made.value();
      }
      mesh.vertices.emplace_back(coordinate);
    }
    for (const auto &face : recipe.faces) {
      if (face.size() < 3)
        return qualification_generation_detail::generation_error(
            product_error_code::qualification_policy_violation,
            "qualification_generator.short_face");
      std::vector<I> emitted;
      emitted.reserve(face.size());
      for (const auto index : face) {
        if (index >= recipe.vertices.size() ||
            index > static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
          return qualification_generation_detail::generation_error(
              product_error_code::index_overflow,
              "qualification_generator.face_index_capacity");
        emitted.push_back(static_cast<I>(index));
      }
      mesh.faces.push_back(std::move(emitted));
    }
  } catch (const std::bad_alloc &) {
    return qualification_generation_detail::generation_error(
        product_error_code::resource_limit,
        "qualification_generator.materialization_allocation");
  }
  return mesh;
}

template <class T, class I>
product_status_or<std::pair<fv_surface_mesh<T, I>, fv_surface_mesh<T, I>>>
materialize_qualification_case(const qualification_case_recipe &recipe) {
  auto valid = validate_qualification_case_recipe(recipe);
  if (!valid.has_value())
    return valid.error();
  auto a = materialize_qualification_mesh<T, I>(recipe.operand_a);
  if (!a.has_value())
    return a.error();
  auto b = materialize_qualification_mesh<T, I>(recipe.operand_b);
  if (!b.has_value())
    return b.error();
  return std::make_pair(std::move(a.value()), std::move(b.value()));
}

template <class T, class I>
digest qualification_materialized_mesh_digest(const fv_surface_mesh<T, I> &mesh) {
  canonical_encoder encoder;
  encoder.u64(mesh.vertices.size());
  for (const auto &v : mesh.vertices) {
    const std::array<T, 3> coordinates{{v.x, v.y, v.z}};
    for (const auto coordinate : coordinates) {
      const auto bits = bits_of<T>(coordinate);
      if constexpr (std::is_same<T, float>::value)
        encoder.u32(bits.bits);
      else
        encoder.u64(bits.bits);
    }
  }
  encoder.u64(mesh.faces.size());
  for (const auto &face : mesh.faces) {
    encoder.u64(face.size());
    for (const auto index : face)
      encoder.u64(static_cast<std::uint64_t>(index));
  }
  return domain_digest({{'Y', 'G', 'B', 'Q', 'M', 'S', '0', '1'}},
                       encoder.bytes());
}

template <class T, class I> struct qualification_chain_step_output {
  qualification_outcome outcome = qualification_outcome::verified_exact_success;
  digest exact_result_digest;
  std::optional<fv_surface_mesh<T, I>> realized_mesh;
};

template <class T, class I> struct qualification_chain_step_record {
  std::uint64_t step_index = 0;
  digest accumulator_before_digest;
  digest rhs_digest;
  digest accumulator_after_digest;
  digest exact_result_digest;
  qualification_outcome outcome = qualification_outcome::infrastructure_failure;
  std::optional<product_error_code> failure_code;
  bool reingested = false;
  bool retained_exact_result = false;
  bool state_unchanged_after_failure = false;
};

template <class T, class I> struct qualification_chain_execution_result {
  std::uint16_t schema = qualification_generation_schema_version;
  fv_surface_mesh<T, I> final_mesh;
  std::vector<digest> retained_exact_results;
  std::vector<qualification_chain_step_record<T, I>> steps;
  digest transcript_digest;
};

template <class T, class I>
using qualification_chain_step_executor = std::function<
    product_status_or<qualification_chain_step_output<T, I>>(
        const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &,
        const qualification_chain_step_recipe &)>;

template <class T, class I>
using qualification_chain_reingestor = std::function<product_status_or<bool>(
    const fv_surface_mesh<T, I> &)>;

template <class T, class I>
product_status_or<qualification_chain_execution_result<T, I>>
execute_qualification_operation_chain(
    const qualification_operation_chain_recipe &recipe,
    const qualification_chain_step_executor<T, I> &executor,
    const qualification_chain_reingestor<T, I> &reingestor) {
  auto valid = validate_qualification_operation_chain_recipe(recipe);
  if (!valid.has_value())
    return valid.error();
  if (!executor || !reingestor)
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_chain.missing_callback");

  auto initial = materialize_qualification_case<T, I>(recipe.initial_case);
  if (!initial.has_value())
    return initial.error();
  fv_surface_mesh<T, I> accumulator =
      recipe.initial_uses_operand_b ? std::move(initial.value().second)
                                    : std::move(initial.value().first);
  qualification_chain_execution_result<T, I> result;
  result.final_mesh = accumulator;

  canonical_encoder transcript;
  transcript.raw(recipe.chain_digest.bytes.data(), recipe.chain_digest.bytes.size());
  transcript.u64(recipe.steps.size());
  for (std::size_t i = 0; i != recipe.steps.size(); ++i) {
    const auto &step = recipe.steps[i];
    qualification_chain_step_record<T, I> record;
    record.step_index = i;

    if (step.subdivide_accumulator_before_step) {
      fv_surface_mesh<T, I> subdivided;
      subdivided.vertices = accumulator.vertices;
      try {
        for (const auto &face : accumulator.faces) {
          if (face.size() != 4) {
            subdivided.faces.push_back(face);
            continue;
          }
          subdivided.faces.push_back({face[0], face[1], face[2]});
          subdivided.faces.push_back({face[0], face[2], face[3]});
        }
      } catch (const std::bad_alloc &) {
        return qualification_generation_detail::generation_error(
            product_error_code::resource_limit,
            "qualification_chain.subdivision_allocation");
      }
      auto checked = reingestor(subdivided);
      if (!checked.has_value())
        return checked.error();
      if (!checked.value())
        return qualification_generation_detail::generation_error(
            product_error_code::verifier_disagreement,
            "qualification_chain.subdivision_reingestion_rejected");
      accumulator = std::move(subdivided);
    }
    record.accumulator_before_digest =
        qualification_materialized_mesh_digest(accumulator);

    auto rhs_case = materialize_qualification_case<T, I>(step.rhs_case);
    if (!rhs_case.has_value())
      return rhs_case.error();
    fv_surface_mesh<T, I> rhs = step.rhs_uses_operand_b
                                    ? std::move(rhs_case.value().second)
                                    : std::move(rhs_case.value().first);
    record.rhs_digest = qualification_materialized_mesh_digest(rhs);

    const fv_surface_mesh<T, I> &first =
        step.operand_order == qualification_operand_order::a_then_b ? accumulator
                                                                    : rhs;
    const fv_surface_mesh<T, I> &second =
        step.operand_order == qualification_operand_order::a_then_b ? rhs
                                                                    : accumulator;
    auto executed = executor(first, second, step);
    if (!executed.has_value()) {
      record.failure_code = executed.error().code;
      record.outcome = qualification_outcome::expected_typed_failure;
      record.accumulator_after_digest =
          qualification_materialized_mesh_digest(accumulator);
      record.state_unchanged_after_failure =
          record.accumulator_after_digest == record.accumulator_before_digest;
      const bool expected =
          std::find(step.expected_failure_codes.begin(),
                    step.expected_failure_codes.end(), executed.error().code) !=
          step.expected_failure_codes.end();
      if (!expected || !record.state_unchanged_after_failure) {
        if (!expected)
          return executed.error();
        return qualification_generation_detail::generation_error(
            product_error_code::verifier_disagreement,
            "qualification_chain.failure_mutated_accumulator");
      }
      result.steps.push_back(record);
      transcript.u64(i);
      transcript.byte(static_cast<std::uint8_t>(record.outcome));
      transcript.u16(static_cast<std::uint16_t>(*record.failure_code));
      transcript.raw(record.accumulator_before_digest.bytes.data(),
                     record.accumulator_before_digest.bytes.size());
      if (!step.continue_after_expected_failure)
        break;
      continue;
    }

    const auto &output = executed.value();
    if (output.exact_result_digest == digest{})
      return qualification_generation_detail::generation_error(
          product_error_code::stale_binding,
          "qualification_chain.missing_exact_result_digest");
    record.outcome = output.outcome;
    record.exact_result_digest = output.exact_result_digest;
    record.retained_exact_result = true;
    result.retained_exact_results.push_back(output.exact_result_digest);
    if (output.realized_mesh) {
      auto checked = reingestor(*output.realized_mesh);
      if (!checked.has_value())
        return checked.error();
      if (!checked.value())
        return qualification_generation_detail::generation_error(
            product_error_code::verifier_disagreement,
            "qualification_chain.reingestion_rejected_success");
      accumulator = *output.realized_mesh;
      record.reingested = true;
    } else if (!step.continue_with_prior_mesh_when_unrealized) {
      return qualification_generation_detail::generation_error(
          product_error_code::output_not_representable,
          "qualification_chain.unrealized_result_cannot_feed_next_step");
    }
    record.accumulator_after_digest =
        qualification_materialized_mesh_digest(accumulator);
    result.steps.push_back(record);
    transcript.u64(i);
    transcript.byte(static_cast<std::uint8_t>(record.outcome));
    transcript.raw(record.exact_result_digest.bytes.data(),
                   record.exact_result_digest.bytes.size());
    transcript.raw(record.accumulator_before_digest.bytes.data(),
                   record.accumulator_before_digest.bytes.size());
    transcript.raw(record.accumulator_after_digest.bytes.data(),
                   record.accumulator_after_digest.bytes.size());
    transcript.boolean(record.reingested);
  }
  result.final_mesh = std::move(accumulator);
  result.transcript_digest =
      domain_digest({{'Y', 'G', 'B', 'Q', 'C', 'H', '0', '1'}},
                    transcript.bytes());
  return result;
}

} // namespace mesh_boolean
} // namespace ygor

#endif
