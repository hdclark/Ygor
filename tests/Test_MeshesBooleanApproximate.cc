#include "MeshBooleanOutputFixtures.h"
#include <YgorMeshesBooleanApproximate.h>
#include <YgorMeshesBooleanPreparation.h>

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ygor::mesh_boolean;

namespace {

void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

digest marked(char value) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(value));
  return domain_digest({{'Y', 'G', 'B', 'A', 'T', 'E', 'S', 'T'}},
                       encoder.bytes());
}

std::vector<std::uint8_t> replace_serialized_exact_and_certificate(
    std::vector<std::uint8_t> envelope,
    const std::vector<std::uint8_t> &exact,
    const std::vector<std::uint8_t> &certificate) {
  auto read_u64 = [&](std::size_t &offset) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i)
      value = (value << 8) | envelope.at(offset++);
    return value;
  };
  std::size_t offset = 12;
  const auto exact_size = read_u64(offset);
  require(exact_size == exact.size(), "forged exact envelope size");
  std::copy(exact.begin(), exact.end(), envelope.begin() + offset);
  offset += exact_size;
  const auto output_size = read_u64(offset);
  offset += output_size;
  const auto certificate_size = read_u64(offset);
  require(certificate_size == certificate.size(),
          "forged certificate envelope size");
  std::copy(certificate.begin(), certificate.end(), envelope.begin() + offset);
  return envelope;
}

backend_identity backend() {
  backend_capabilities capabilities;
  capabilities.set(backend_capability::exact_set_semantics);
  capabilities.set(backend_capability::exact_coordinates);
  capabilities.set(backend_capability::stratified_output);
  capabilities.set(backend_capability::manifold_mesh_output);
  capabilities.set(backend_capability::deterministic_canonical_output);
  capabilities.set(backend_capability::certified_failure_categories);
  capabilities.set(backend_capability::provenance_mapping);
  capabilities.set(backend_capability::strict_prepared_operands);
  capabilities.set(backend_capability::certified_approximate_output);
  auto made = make_backend_identity(backend_id::experimental_exact_v1,
                                    {1, 0, 0}, "approximate-test",
                                    capabilities,
                                    backend_maturity::experimental);
  require(made.has_value(), "approximate backend identity");
  return made.value();
}

product_realization_policy policy(double displacement = 1.0e-12,
                                  std::uint64_t candidates = 27,
                                  std::uint64_t backtracks = 1000000) {
  product_realization_policy result;
  result.semantics =
      product_realization_semantics::certified_approximate_embedding_v1;
  result.search.strategy =
      realization_search_strategy::deterministic_bounded_search;
  result.search.max_candidates = candidates;
  result.search.max_candidate_evaluations = 100000;
  result.search.max_search_nodes = backtracks;
  result.search.max_obligations = 1000000;
  result.search.max_triangle_pairs = 1000000;
  result.search.max_predicate_checks = 100000000;
  result.search.max_verifier_work = 100000000;
  result.search.max_verifier_records = 10000000;
  result.search.max_verifier_bytes = 256U * 1024U * 1024U;
  result.approximation.enabled = true;
  result.approximation.unit = model_unit::unitless;
  result.approximation.max_vertex_displacement = displacement;
  result.approximation.has_max_axis_displacement_x = true;
  result.approximation.max_axis_displacement_x = displacement;
  result.approximation.max_support_plane_deviation = displacement;
  result.approximation.declared_model_tolerance = displacement * 10.0;
  result.approximation.candidate_generation_version = 1;
  result.approximation.candidate_ulp_radius = 1;
  result.approximation.application_acceptance_metadata =
      "adversarial-unitless-model-tolerance";
  return result;
}

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>> run(
    fv_surface_mesh<T, I> a, fv_surface_mesh<T, I> b,
    operation op, product_realization_policy realization,
    boolean_options options = {}, std::uint64_t *used_candidates = nullptr) {
  auto context = classification_test::context(a, b, output_test::registry(), op,
                                              std::move(options));
  exact_result_backend_binding backend_binding;
  backend_binding.producer = backend();
  backend_binding.attempted_backends = {backend_id::experimental_exact_v1};
  exact_result_preparation_binding preparation;
  preparation.input_digest = marked('i');
  preparation.prepared_digest = marked('p');
  preparation.policy_digest = marked('o');
  preparation.report_digest = marked('r');
  auto result = evaluate_boolean_product_result(
      *context, std::move(backend_binding), std::move(preparation),
      result_representation::certified_approximate_mesh,
      std::move(realization));
  if (used_candidates)
    *used_candidates = context->accountant().used(resource_kind::candidates);
  return result;
}

void one_third_and_mutations() {
  auto result = run(input_test::cube<double, std::uint32_t>(),
                    input_test::third_intersection_prism<double, std::uint32_t>(),
                    operation::regularized_intersection, policy());
  require(result.has_value(), "one-third approximate product result");
  require(result.value()->representation ==
                  result_representation::certified_approximate_mesh &&
              result.value()->mesh &&
              result.value()->mesh->approximate_certificate &&
              result.value()->mesh->semantics ==
                  product_realization_semantics::
                      certified_approximate_embedding_v1 &&
              validate_product_result(*result.value()).has_value(),
          "one-third certified approximate publication");
  const auto &mesh = *result.value()->mesh;
  const auto &certificate = *mesh.approximate_certificate;
  require(!certificate.relaxed_relations.empty() &&
              !certificate.defining_relations.empty() &&
              certificate.exact_result_digest ==
                  result.value()->exact_result->canonical_digest &&
              certificate.search.candidate_generation_version == 1 &&
              certificate.maximum_squared_vertex_displacement <
                  exact_scalar(1),
          "one-third approximate evidence is distinct and bounded");

  auto serialized = encode_certified_approximate_embedding(
      result.value()->exact_result, mesh.policy, *mesh.success, certificate);
  require(serialized.has_value() &&
              verify_serialized_certified_approximate_embedding<
                  double, std::uint32_t>(serialized.value(), mesh.policy)
                  .has_value(),
          "canonical serialized approximate replay");
  auto corrupt = serialized.value();
  corrupt[corrupt.size() / 2] ^= 1U;
  require(!verify_serialized_certified_approximate_embedding<
               double, std::uint32_t>(corrupt, mesh.policy)
               .has_value(),
          "serialized corruption rejected");
  auto truncated = serialized.value();
  truncated.pop_back();
  require(!verify_serialized_certified_approximate_embedding<
               double, std::uint32_t>(truncated, mesh.policy)
               .has_value(),
          "serialized truncation rejected");
  auto trailing = serialized.value();
  trailing.push_back(0);
  require(!verify_serialized_certified_approximate_embedding<
               double, std::uint32_t>(trailing, mesh.policy)
               .has_value(),
          "serialized noncanonical trailing bytes rejected");
  approximate_decode_limits decode_limits;
  decode_limits.max_record_bytes = serialized.value().size() - 1;
  auto limited_decode = verify_serialized_certified_approximate_embedding<
      double, std::uint32_t>(serialized.value(), mesh.policy, decode_limits);
  require(!limited_decode.has_value() &&
              limited_decode.error().code == product_error_code::resource_limit,
          "serialized record limit is typed");
  decode_limits = {};
  decode_limits.max_exact_result_bytes =
      result.value()->exact_result->canonical_bytes.size() - 1;
  require(!verify_serialized_certified_approximate_embedding<
               double, std::uint32_t>(serialized.value(), mesh.policy,
                                      decode_limits)
               .has_value(),
          "serialized embedded exact-result limit");
  decode_limits = {};
  decode_limits.max_output_bytes = mesh.output_canonical_bytes.size() - 1;
  require(!verify_serialized_certified_approximate_embedding<
               double, std::uint32_t>(serialized.value(), mesh.policy,
                                      decode_limits)
               .has_value(),
          "serialized output limit");
  decode_limits = {};
  decode_limits.max_certificate_bytes = certificate.canonical_bytes.size() - 1;
  require(!verify_serialized_certified_approximate_embedding<
               double, std::uint32_t>(serialized.value(), mesh.policy,
                                      decode_limits)
               .has_value(),
          "serialized certificate limit");

  std::shared_ptr<const exact_kernel_services<double>> reingest_kernel =
      std::make_shared<exact_kernel<double>>();
  std::shared_ptr<const verifier_service> reingest_verifiers =
      output_test::registry();
  auto reingested = validate_operand_strict(
      mesh.success->mesh, strict_validation_policy{}, boolean_options{},
      std::move(reingest_kernel), std::move(reingest_verifiers));
  require(reingested.has_value(),
          "approximate output passes fresh strict Component 2 re-ingestion");

  auto changed_output = *mesh.success;
  changed_output.mesh.vertices.front().x = std::nextafter(
      changed_output.mesh.vertices.front().x,
      std::numeric_limits<double>::infinity());
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, changed_output,
               certificate)
               .has_value(),
          "emitted-bit mutation rejected");
  auto changed_certificate = certificate;
  changed_certificate.vertices.front().exact_target.x =
      changed_certificate.vertices.front().exact_target.x + exact_scalar(1);
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "exact-target mutation rejected");
  changed_certificate = certificate;
  changed_certificate.obligations.front().passed = false;
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "obligation mutation rejected");
  changed_certificate = certificate;
  changed_certificate.exact_result_digest.bytes[0] ^= 1U;
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "exact-result digest mutation rejected");
  changed_certificate = certificate;
  changed_certificate.search.accepted_candidate_ranks.front() =
      certificate.search.candidate_cap;
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "search transcript mutation rejected");
  changed_certificate = certificate;
  changed_certificate.triangles.front().projection =
      static_cast<projection_axis>(
          (static_cast<unsigned>(
               changed_certificate.triangles.front().projection) +
           1) % 3);
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "authorized triangulation mutation rejected with fresh digest");
  changed_certificate = certificate;
  changed_certificate.occurrence_maps.front().output_cyclic_triangles.front() =
      realization_triangle_id::from_canonical_value(
          (changed_certificate.occurrence_maps.front()
               .output_cyclic_triangles.front()
               .value_for_debug() +
           1) % changed_certificate.triangles.size());
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "occurrence/link mutation rejected with fresh digest");
  changed_certificate = certificate;
  changed_certificate.patch_adjacency.front().exact_radial_patches.front() =
      selected_patch_id::from_canonical_value(
          (changed_certificate.patch_adjacency.front()
               .exact_radial_patches.front()
               .value_for_debug() +
           1) % changed_certificate.exact_patches);
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "edge-radial mutation rejected with fresh digest");
  changed_certificate = certificate;
  changed_certificate.defining_relations.front().emitted_residual =
      changed_certificate.defining_relations.front().emitted_residual +
      exact_scalar(1);
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "defining-relation mutation rejected with fresh digest");
  changed_certificate = certificate;
  changed_certificate.components.front().rejected_prefix_witnesses.push_back(0);
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "rejected-prefix transcript mutation rejected with fresh digest");
  changed_certificate = certificate;
  std::reverse(changed_certificate.components.front().variable_order.begin(),
               changed_certificate.components.front().variable_order.end());
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "variable-order transcript mutation rejected with fresh digest");
  changed_certificate = certificate;
  std::reverse(changed_certificate.components.front().obligations.begin(),
               changed_certificate.components.front().obligations.end());
  canonicalize_certified_approximate_certificate(changed_certificate);
  require(!verify_certified_approximate_embedding(
               result.value()->exact_result, mesh.policy, *mesh.success,
               changed_certificate)
               .has_value(),
          "constraint-order transcript mutation rejected with fresh digest");

  auto exact_candidate_limit = policy();
  exact_candidate_limit.search.max_candidate_evaluations =
      certificate.search.candidate_evaluations;
  auto candidate_exact = run(input_test::cube<double, std::uint32_t>(),
                             input_test::third_intersection_prism<
                                 double, std::uint32_t>(),
                             operation::regularized_intersection,
                             exact_candidate_limit);
  require(candidate_exact.has_value() && candidate_exact.value()->mesh,
          "candidate evaluation exact limit succeeds");
  auto candidate_short_policy = exact_candidate_limit;
  --candidate_short_policy.search.max_candidate_evaluations;
  auto candidate_short = run(input_test::cube<double, std::uint32_t>(),
                             input_test::third_intersection_prism<
                                 double, std::uint32_t>(),
                             operation::regularized_intersection,
                             candidate_short_policy);
  require(candidate_short.has_value() && candidate_short.value()->realization &&
              candidate_short.value()->realization->failure &&
              candidate_short.value()->realization->failure->code ==
                  product_error_code::resource_limit &&
              candidate_short.value()->realization->failure->message_key ==
                  "approximate.candidate_evaluation_limit",
          "candidate evaluation one-under limit rejects");
  auto exact_node_limit = policy();
  exact_node_limit.search.max_search_nodes = certificate.search.visited_nodes;
  auto node_exact = run(input_test::cube<double, std::uint32_t>(),
                        input_test::third_intersection_prism<
                            double, std::uint32_t>(),
                        operation::regularized_intersection, exact_node_limit);
  require(node_exact.has_value() && node_exact.value()->mesh,
          "search node exact limit succeeds");
  --exact_node_limit.search.max_search_nodes;
  auto node_short = run(input_test::cube<double, std::uint32_t>(),
                        input_test::third_intersection_prism<
                            double, std::uint32_t>(),
                        operation::regularized_intersection, exact_node_limit);
  require(node_short.has_value() && node_short.value()->realization &&
              node_short.value()->realization->failure &&
              node_short.value()->realization->failure->code ==
                  product_error_code::resource_limit,
          "search node one-under limit rejects");

  const auto require_resource_failure = [](const auto &result,
                                           const char *message,
                                           const char *key = nullptr) {
    require(result.has_value() && result.value()->realization &&
                result.value()->realization->failure &&
                result.value()->realization->failure->code ==
                    product_error_code::resource_limit &&
                (!key || result.value()->realization->failure->message_key == key),
            message);
  };
  auto run_one_third = [](product_realization_policy limits) {
    return run(input_test::cube<double, std::uint32_t>(),
               input_test::third_intersection_prism<double, std::uint32_t>(),
               operation::regularized_intersection, std::move(limits));
  };
  auto obligation_exact_policy = policy();
  obligation_exact_policy.search.max_obligations = certificate.obligations.size();
  require(run_one_third(obligation_exact_policy).value()->mesh.has_value(),
          "obligation exact limit succeeds");
  --obligation_exact_policy.search.max_obligations;
  require_resource_failure(run_one_third(obligation_exact_policy),
                           "obligation one-under limit is typed");
  const auto triangle_count =
      static_cast<std::uint64_t>(certificate.triangles.size());
  const auto triangle_pairs = triangle_count * (triangle_count - 1) / 2;
  auto pair_exact_policy = policy();
  pair_exact_policy.search.max_triangle_pairs = triangle_pairs;
  require(run_one_third(pair_exact_policy).value()->mesh.has_value(),
          "triangle-pair exact limit succeeds");
  --pair_exact_policy.search.max_triangle_pairs;
  require_resource_failure(run_one_third(pair_exact_policy),
                           "triangle-pair one-under limit is typed");
  approximate_verifier_budget observed(mesh.policy.search);
  auto observed_verification =
      detail::verify_certified_approximate_embedding_with_budget(
          result.value()->exact_result, mesh.policy, *mesh.success, certificate,
          observed);
  require(observed_verification.has_value(), "observe independent verifier work");
  const std::array<std::tuple<std::uint64_t realization_search_policy::*,
                              std::uint64_t, const char *, const char *>, 4>
      verifier_limits{{
          {&realization_search_policy::max_verifier_work,
           observed.verifier_work, "verifier work", "approximate_verifier.work_limit"},
          {&realization_search_policy::max_verifier_records,
           observed.verifier_records, "verifier records",
           "approximate_verifier.record_limit"},
          {&realization_search_policy::max_verifier_bytes,
           observed.verifier_bytes, "verifier bytes",
           "approximate_verifier.byte_limit"},
          {&realization_search_policy::max_predicate_checks,
           observed.predicate_checks, "verifier predicates",
           "approximate_verifier.predicate_limit"},
      }};
  for (const auto &[field, exact_limit, label, key] : verifier_limits) {
    require(exact_limit > 0, "nonzero observed verifier boundary");
    auto exact_policy = policy();
    exact_policy.search.*field = exact_limit;
    auto exact_result = run_one_third(exact_policy);
    require(exact_result.has_value() && exact_result.value()->mesh,
            (std::string(label) + " exact limit succeeds").c_str());
    --(exact_policy.search.*field);
    require_resource_failure(
        run_one_third(exact_policy),
        (std::string(label) + " one-under limit is typed").c_str(), key);
  }
  std::uint64_t observed_candidates = 0;
  auto candidate_observed = run(
      input_test::cube<double, std::uint32_t>(),
      input_test::third_intersection_prism<double, std::uint32_t>(),
      operation::regularized_intersection, policy(), {}, &observed_candidates);
  require(candidate_observed.has_value() && candidate_observed.value()->mesh &&
              observed_candidates > 0,
          "observe context candidate accounting");
  boolean_options candidate_resources;
  candidate_resources.resources.candidates =
      {false, observed_candidates};
  auto candidate_resource_exact = run(
      input_test::cube<double, std::uint32_t>(),
      input_test::third_intersection_prism<double, std::uint32_t>(),
      operation::regularized_intersection, policy(), candidate_resources);
  require(candidate_resource_exact.has_value() &&
              candidate_resource_exact.value()->mesh,
          "context candidate accounting succeeds at exact limit");
  --candidate_resources.resources.candidates.value;
  std::uint64_t rejected_candidate_usage = 1;
  auto candidate_resource_short = run(
      input_test::cube<double, std::uint32_t>(),
      input_test::third_intersection_prism<double, std::uint32_t>(),
      operation::regularized_intersection, policy(), candidate_resources,
      &rejected_candidate_usage);
  require_resource_failure(candidate_resource_short,
                           "context candidate accounting fails one-under",
                           "resource_limit");
  boolean_options baseline_resources;
  baseline_resources.resources.candidates =
      {false, rejected_candidate_usage};
  std::uint64_t baseline_candidate_usage = 0;
  auto baseline_failure = run(
      input_test::cube<double, std::uint32_t>(),
      input_test::third_intersection_prism<double, std::uint32_t>(),
      operation::regularized_intersection, policy(), baseline_resources,
      &baseline_candidate_usage);
  require_resource_failure(baseline_failure,
                           "baseline candidate accounting remains bounded",
                           "resource_limit");
  require(rejected_candidate_usage < observed_candidates &&
              baseline_candidate_usage == rejected_candidate_usage &&
              candidate_resource_short.value()->representation ==
                  result_representation::exact_stratified,
          "candidate reservation failure rolls back without partial mesh");
}

void policy_relative_failures_and_strict_regression() {
  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::third_intersection_prism<double, std::uint32_t>();
  auto exhausted = run(a, b, operation::regularized_intersection,
                       policy(std::numeric_limits<double>::denorm_min(), 3));
  require(exhausted.has_value() && exhausted.value()->exact_result.valid() &&
              exhausted.value()->representation ==
                  result_representation::exact_stratified &&
              exhausted.value()->realization &&
              exhausted.value()->realization->failure &&
              exhausted.value()->realization->failure->code ==
                  product_error_code::output_not_representable &&
              exhausted.value()->realization->failure->subcode ==
                  approximate_search_exhausted_subcode,
          "candidate-boundary exhaustion is policy relative");
  auto limited = run(a, b, operation::regularized_intersection,
                     policy(1.0e-12, 27, 1));
  require(limited.has_value() && limited.value()->realization &&
              limited.value()->realization->failure &&
              limited.value()->realization->failure->code ==
                  product_error_code::resource_limit,
          "solver limit is a resource limit");

  auto context = classification_test::context(
      a, b, output_test::registry(), operation::regularized_intersection,
      boolean_options{});
  exact_result_backend_binding backend_binding;
  backend_binding.producer = backend();
  backend_binding.attempted_backends = {backend_id::experimental_exact_v1};
  exact_result_preparation_binding preparation;
  preparation.input_digest = marked('1');
  preparation.prepared_digest = marked('2');
  preparation.policy_digest = marked('3');
  preparation.report_digest = marked('4');
  auto strict = evaluate_boolean_product_result(
      *context, std::move(backend_binding), std::move(preparation),
      result_representation::exact_in_T_mesh);
  require(strict.has_value() && strict.value()->representation ==
                                    result_representation::exact_stratified &&
              strict.value()->realization->failure->code ==
                  product_error_code::output_not_representable,
          "strict exact-in-T remains exact and rejects one-third");
}

void geometric_adversaries() {
  constexpr std::int64_t n0 = 1501199875789831LL;
  constexpr std::int64_t d0 = 4503599627369496LL;
  constexpr std::int64_t n1 = 1501199875789832LL;
  constexpr std::int64_t d1 = 4503599627369499LL;
  const auto q0 = exact_scalar(n0) / exact_scalar(d0);
  const auto q1 = exact_scalar(n1) / exact_scalar(d1);
  fv_surface_mesh<double, std::uint32_t> wedge;
  wedge.vertices = {{0.0, 0.0, 0.0},
                    {static_cast<double>(n1), static_cast<double>(d1), 0.0},
                    {static_cast<double>(n0), static_cast<double>(d0), 0.0},
                    {0.0, 0.0, 1.0},
                    {static_cast<double>(n1), static_cast<double>(d1), 1.0},
                    {static_cast<double>(n0), static_cast<double>(d0), 1.0}};
  wedge.faces = {{0, 2, 1}, {3, 4, 5}, {0, 1, 4, 3},
                 {1, 2, 5, 4}, {2, 0, 3, 5}};
  auto collision = run(input_test::cube<double, std::uint32_t>(), wedge,
                       operation::regularized_intersection, policy());
  require(collision.has_value() && collision.value()->mesh,
          "rounding-collision approximate assignment");
  bool found0 = false, found1 = false;
  std::array<std::uint64_t, 3> bits0{{0, 0, 0}}, bits1{{0, 0, 0}};
  for (const auto &vertex :
       collision.value()->mesh->approximate_certificate->vertices) {
    if (vertex.exact_target.y == exact_scalar(1) && vertex.exact_target.x == q0) {
      found0 = true;
      for (std::size_t axis = 0; axis < 3; ++axis)
        bits0[axis] = vertex.output_bits[axis].bits;
    }
    if (vertex.exact_target.y == exact_scalar(1) && vertex.exact_target.x == q1) {
      found1 = true;
      for (std::size_t axis = 0; axis < 3; ++axis)
        bits1[axis] = vertex.output_bits[axis].bits;
    }
  }
  require(found0 && found1 && bits0 != bits1,
          "rounding collision remains distinct");
  auto cap_exact_policy = policy();
  cap_exact_policy.search.max_candidates = 4;
  auto cap_exact = run(input_test::cube<double, std::uint32_t>(), wedge,
                       operation::regularized_intersection, cap_exact_policy);
  require(cap_exact.has_value() && cap_exact.value()->mesh,
          "genuine candidate cap succeeds at required retained-domain size");
  auto cap_short_policy = cap_exact_policy;
  --cap_short_policy.search.max_candidates;
  auto cap_short = run(input_test::cube<double, std::uint32_t>(), wedge,
                       operation::regularized_intersection, cap_short_policy);
  require(cap_short.has_value() && cap_short.value()->realization &&
              cap_short.value()->realization->failure &&
              cap_short.value()->realization->failure->code ==
                  product_error_code::output_not_representable,
          "genuine candidate cap fails one-under without displacement change");

  auto disjoint_a = input_test::box<double, std::uint32_t>(0.0, 1.0);
  auto disjoint_b = input_test::box<double, std::uint32_t>(2.0, 3.0);
  auto dyadic = run(disjoint_a, disjoint_b, operation::regularized_union,
                    policy());
  require(dyadic.has_value() && dyadic.value()->mesh,
          "coplanar high-valence dyadic case");

  fv_surface_mesh<double, std::uint32_t> high_valence;
  high_valence.vertices = {{0.0, 0.0, 2.0}, {0.0, 0.0, -2.0},
                           {2.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
                           {0.0, 2.0, 0.0}, {-1.0, 1.0, 0.0},
                           {-2.0, 0.0, 0.0}, {-1.0, -1.0, 0.0},
                           {0.0, -2.0, 0.0}, {1.0, -1.0, 0.0}};
  for (std::uint32_t i = 0; i < 8; ++i) {
    const auto current = static_cast<std::uint32_t>(2 + i);
    const auto next = static_cast<std::uint32_t>(2 + (i + 1) % 8);
    high_valence.faces.push_back({0, current, next});
    high_valence.faces.push_back({1, next, current});
  }
  auto high_valence_shifted = high_valence;
  for (auto &vertex : high_valence_shifted.vertices)
    vertex.x += 10.0;
  auto high_valence_result =
      run(high_valence, high_valence_shifted,
          operation::regularized_union, policy());
  require(high_valence_result.has_value() && high_valence_result.value()->mesh &&
              std::any_of(
                  high_valence_result.value()->mesh->approximate_certificate
                      ->occurrence_maps.begin(),
                  high_valence_result.value()->mesh->approximate_certificate
                      ->occurrence_maps.end(),
                  [](const auto &occurrence) {
                    return occurrence.exact_cyclic_halfedges.size() >= 8;
                  }),
          "real high-valence cyclic-link coverage");

  auto coplanar_a = input_test::box<double, std::uint32_t>(0.0, 2.0);
  auto coplanar_b = input_test::box<double, std::uint32_t>(1.0, 3.0);
  auto coplanar = run(coplanar_a, coplanar_b,
                      operation::regularized_union, policy());
  require(coplanar.has_value() && coplanar.value()->mesh,
          "coplanar source-triangle coverage");

  const double large = std::ldexp(1.0, 500);
  auto large_a = input_test::box<double, std::uint32_t>(large, large * 1.5);
  auto large_b = input_test::box<double, std::uint32_t>(large * 2.0,
                                                       large * 2.5);
  auto large_result = run(large_a, large_b, operation::regularized_union,
                          policy(std::ldexp(1.0, 450)));
  require(large_result.has_value() && large_result.value()->mesh,
          "large-range approximate case");

  const double subnormal = std::numeric_limits<double>::denorm_min();
  auto subnormal_a = input_test::box<double, std::uint32_t>(0.0, subnormal);
  auto subnormal_b = input_test::box<double, std::uint32_t>(subnormal * 2.0,
                                                           subnormal * 3.0);
  auto subnormal_result = run(subnormal_a, subnormal_b,
                              operation::regularized_union,
                              policy(subnormal));
  require(subnormal_result.has_value() && subnormal_result.value()->mesh,
          "subnormal approximate success");

  auto thin_a = input_test::box<double, std::uint32_t>(0.0, 1.0);
  auto thin_b = input_test::box<double, std::uint32_t>(
      1.0 - std::ldexp(1.0, -40), 2.0);
  auto thin = run(thin_a, thin_b, operation::regularized_intersection,
                  policy(1.0e-12));
  require(thin.has_value() && thin.value()->mesh,
          "thin/sliver approximate success");
}

void empty_and_signed_zero() {
  auto empty = run(input_test::box<double, std::uint32_t>(0.0, 1.0),
                   input_test::box<double, std::uint32_t>(2.0, 3.0),
                   operation::regularized_intersection, policy());
  require(empty.has_value() && empty.value()->mesh &&
              empty.value()->mesh->success->mesh.vertices.empty() &&
              empty.value()->mesh->success->mesh.faces.empty() &&
              !empty.value()->mesh->realization_canonical_bytes.empty() &&
              empty.value()->mesh->approximate_certificate->components.empty(),
          "versioned empty approximate publication");
  auto serialized = encode_certified_approximate_embedding(
      empty.value()->exact_result, empty.value()->mesh->policy,
      *empty.value()->mesh->success,
      *empty.value()->mesh->approximate_certificate);
  require(serialized.has_value() &&
              verify_serialized_certified_approximate_embedding<
                  double, std::uint32_t>(serialized.value(),
                                        empty.value()->mesh->policy)
                  .has_value(),
          "empty serialized replay");

  auto negative_zero_box =
      input_test::box<double, std::uint32_t>(-0.0, 1.0);
  auto distant = input_test::box<double, std::uint32_t>(3.0, 4.0);
  auto signed_zero = run(negative_zero_box, distant,
                         operation::regularized_union, policy());
  require(signed_zero.has_value() && signed_zero.value()->mesh,
          "signed-zero approximate publication");
  bool retained_negative_zero = false;
  for (const auto &vertex :
       signed_zero.value()->mesh->approximate_certificate->vertices)
    if (vertex.original_vertex)
      for (const auto bits : vertex.output_bits)
        retained_negative_zero =
            retained_negative_zero || bits.bits == 0x8000000000000000ULL;
  require(retained_negative_zero,
          "original movement false preserves negative-zero bits");
  auto exact_boundary = read_exact_result(signed_zero.value()->exact_result);
  require(exact_boundary.has_value() &&
              std::any_of(exact_boundary.value()->vertices.begin(),
                          exact_boundary.value()->vertices.end(),
                          [](const auto &vertex) {
                            return std::any_of(
                                vertex.original_raw_bits.begin(),
                                vertex.original_raw_bits.end(),
                                [](const auto &raw) {
                                  return std::find(
                                             raw.bits.begin(), raw.bits.end(),
                                             0x8000000000000000ULL) !=
                                         raw.bits.end();
                                });
                          }),
          "durable exact result retains negative-zero source bits");
  auto replayed_exact = decode_exact_stratified_boundary(
      signed_zero.value()->exact_result->canonical_bytes);
  require(replayed_exact.has_value() &&
              std::any_of(replayed_exact.value()->vertices.begin(),
                          replayed_exact.value()->vertices.end(),
                          [](const auto &vertex) {
                            return std::any_of(
                                vertex.original_raw_bits.begin(),
                                vertex.original_raw_bits.end(),
                                [](const auto &raw) {
                                  return std::find(
                                             raw.bits.begin(), raw.bits.end(),
                                             0x8000000000000000ULL) !=
                                         raw.bits.end();
                                });
                          }),
          "negative-zero source bits survive canonical exact-result replay");

  auto coincident = run(negative_zero_box, negative_zero_box,
                        operation::regularized_union, policy());
  require(coincident.has_value() && coincident.value()->mesh,
          "coincident source-bit conflict fixture");
  auto coincident_exact = read_exact_result(coincident.value()->exact_result);
  require(coincident_exact.has_value(), "read coincident exact result");
  auto conflicting = *coincident_exact.value();
  auto shared = std::find_if(
      conflicting.vertices.begin(), conflicting.vertices.end(),
      [](const auto &vertex) { return vertex.original_raw_bits.size() > 1; });
  require(shared != conflicting.vertices.end(),
          "multiple original source-bit fixture");
  const auto shared_index = static_cast<std::size_t>(
      std::distance(conflicting.vertices.begin(), shared));
  shared->original_raw_bits[1].bits[0] ^= 0x8000000000000000ULL;
  auto conflicting_handle =
      freeze_exact_stratified_boundary(std::move(conflicting));
  require(conflicting_handle.has_value(),
          "conflicting original source bits remain durable exact authority");
  auto durable_conflict = make_exact_result_handle(conflicting_handle.value());
  require(durable_conflict.has_value(), "make conflicting exact handle");
  auto conflicting_replay = decode_exact_stratified_boundary(
      durable_conflict.value()->canonical_bytes);
  require(conflicting_replay.has_value() &&
              conflicting_replay.value()->vertices[shared_index]
                      .original_raw_bits.size() > 1 &&
              conflicting_replay.value()->vertices[shared_index]
                      .original_raw_bits[0].bits !=
                  conflicting_replay.value()->vertices[shared_index]
                      .original_raw_bits[1].bits,
          "all conflicting source records survive exact replay");
  auto forged = *coincident.value()->mesh->approximate_certificate;
  forged.exact_result_digest = durable_conflict.value()->canonical_digest;
  canonicalize_certified_approximate_certificate(forged);
  auto forged_in_memory = verify_certified_approximate_embedding(
      durable_conflict.value(), coincident.value()->mesh->policy,
      *coincident.value()->mesh->success, forged);
  require(!forged_in_memory.has_value() &&
              forged_in_memory.error().code ==
                  product_error_code::verifier_disagreement &&
              forged_in_memory.error().message_key ==
                  "approximate_verifier.conflicting_original_raw_bits",
          "canonical first-only fixed certificate is rejected in memory");
  auto coincident_serialized = encode_certified_approximate_embedding(
      coincident.value()->exact_result, coincident.value()->mesh->policy,
      *coincident.value()->mesh->success,
      *coincident.value()->mesh->approximate_certificate);
  require(coincident_serialized.has_value(), "serialize forged base fixture");
  const auto forged_serialized = replace_serialized_exact_and_certificate(
      coincident_serialized.value(), durable_conflict.value()->canonical_bytes,
      forged.canonical_bytes);
  auto forged_replay =
      verify_serialized_certified_approximate_embedding<double, std::uint32_t>(
          forged_serialized, coincident.value()->mesh->policy);
  require(!forged_replay.has_value() &&
              forged_replay.error().code ==
                  product_error_code::verifier_disagreement &&
              forged_replay.error().message_key ==
                  "approximate_verifier.conflicting_original_raw_bits",
          "canonical first-only fixed certificate is rejected from bytes");
  auto fixed_context = classification_test::context(
      negative_zero_box, negative_zero_box, output_test::registry(),
      operation::regularized_union, boolean_options{});
  auto fixed = realize_certified_approximate_embedding(
      *fixed_context, durable_conflict.value(), backend(), policy());
  require(!fixed.has_value() &&
              fixed.error().code == product_error_code::output_not_representable,
          "conflicting fixed source encodings are policy-relative");
  auto movable_policy = policy();
  movable_policy.approximation.allow_original_vertex_movement = true;
  auto movable_context = classification_test::context(
      negative_zero_box, negative_zero_box, output_test::registry(),
      operation::regularized_union, boolean_options{});
  auto movable = realize_certified_approximate_embedding(
      *movable_context, durable_conflict.value(), backend(), movable_policy);
  require(movable.has_value(),
          "conflicting source encodings permit the movement path");
}

template <class T, class I> void runtime_type_case() {
  auto result = run(input_test::box<T, I>(T(0), T(1)),
                    input_test::box<T, I>(T(3), T(4)),
                    operation::regularized_union, policy());
  require(result.has_value() && result.value()->mesh &&
              verify_certified_approximate_embedding(
                  result.value()->exact_result, result.value()->mesh->policy,
                  *result.value()->mesh->success,
                  *result.value()->mesh->approximate_certificate)
                  .has_value(),
          "all T/I runtime instantiations");
}

void all_runtime_types() {
  runtime_type_case<float, std::uint32_t>();
  runtime_type_case<float, std::uint64_t>();
  runtime_type_case<double, std::uint32_t>();
  runtime_type_case<double, std::uint64_t>();
}

} // namespace

int main() {
  try {
    one_third_and_mutations();
    policy_relative_failures_and_strict_regression();
    geometric_adversaries();
    empty_and_signed_zero();
    all_runtime_types();
    std::cout << "ok\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
