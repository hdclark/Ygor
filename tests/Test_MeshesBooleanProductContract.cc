#include <YgorMeshesBooleanExactResult.h>
#include <YgorMeshesBooleanProductContract.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace ygor::mesh_boolean;

namespace {

void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

std::string hex_bytes(const std::vector<std::uint8_t> &bytes) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto byte : bytes)
    out << std::setw(2) << static_cast<unsigned>(byte);
  return out.str();
}

digest test_digest(char suffix) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(suffix));
  return domain_digest({{'Y', 'G', 'B', 'T', 'E', 'S', 'T', suffix}},
                       encoder.bytes());
}

digest sequence_digest(std::uint8_t first) {
  digest out;
  for (std::size_t i = 0; i != out.bytes.size(); ++i)
    out.bytes[i] = static_cast<std::uint8_t>(first + i);
  return out;
}

backend_capabilities experimental_capabilities() {
  backend_capabilities capabilities;
  capabilities.set(backend_capability::exact_set_semantics);
  capabilities.set(backend_capability::exact_coordinates);
  capabilities.set(backend_capability::stratified_output);
  capabilities.set(backend_capability::manifold_mesh_output);
  capabilities.set(backend_capability::deterministic_canonical_output);
  capabilities.set(backend_capability::certified_failure_categories);
  capabilities.set(backend_capability::provenance_mapping);
  capabilities.set(backend_capability::strict_prepared_operands);
  capabilities.set(backend_capability::exact_in_T_output);
  return capabilities;
}

backend_identity
make_test_identity(backend_maturity maturity = backend_maturity::experimental,
                   std::string build = "test-build",
                   backend_version version = {1, 0, 0}) {
  auto made = make_backend_identity(backend_id::experimental_exact_v1, version,
                                    std::move(build),
                                    experimental_capabilities(), maturity);
  require(made.has_value(), "test backend identity");
  return made.value();
}

backend_provenance experimental_provenance() {
  backend_provenance provenance;
  provenance.producer = make_test_identity();
  provenance.selection = backend_selection_mode::explicit_backend;
  provenance.attempted_backends = {backend_id::experimental_exact_v1};
  return provenance;
}

exact_result_handle make_test_exact_result(operation op,
                                           const backend_provenance &backend) {
  exact_stratified_boundary boundary;
  boundary.selected_operation = op;
  boundary.backend.producer = backend.producer;
  boundary.backend.selection = backend.selection;
  boundary.backend.fallback_used = backend.fallback_used;
  boundary.backend.attempted_backends = backend.attempted_backends;
  if (backend.primary_failure)
    boundary.backend.primary_failure = backend.primary_failure->code;
  boundary.preparation.input_digest = test_digest('a');
  boundary.preparation.prepared_digest = test_digest('b');
  boundary.preparation.policy_digest = test_digest('c');
  boundary.preparation.report_digest = test_digest('l');
  boundary.setup_digest = test_digest('n');
  boundary.labeled_digest = test_digest('o');
  boundary.arrangement_digest = test_digest('p');
  boundary.selected_artifact_digest = test_digest('q');
  boundary.selected_semantic_digest = test_digest('r');
  boundary.certificate.id = selection_certificate_id::from_canonical_value(0);
  boundary.certificate.semantic_digest = boundary.selected_semantic_digest;
  auto frozen = freeze_exact_stratified_boundary(std::move(boundary));
  require(frozen.has_value(), "freeze test exact result");
  auto made = make_exact_result_handle(frozen.value());
  require(made.has_value(), "make test exact result handle");
  return made.value();
}

boolean_product_options explicit_experimental_options() {
  boolean_product_options options;
  options.backend.mode = backend_selection_mode::explicit_backend;
  options.backend.requested_backend = backend_id::experimental_exact_v1;
  options.backend.allow_experimental_backend = true;
  options.qualification.mode =
      qualification_policy_mode::allow_explicit_unqualified;
  return options;
}

void product_policy_round_trip() {
  boolean_product_options options;
  require(validate_product_options(options).has_value(),
          "default options are a valid fail-closed request");

  auto encoded = encode_product_options(options);
  require(encoded.has_value(), "product options encode");
  const std::string golden =
      "594742504f503036000600000000000000e7000600060006000600060006010000000000000000000000000000000000000000000000000000000600000600000000000000000000000000000000000000060001000600000600000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000600000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000006000000060000000000000000000001";
  require(hex_bytes(encoded.value()) == golden,
          "schema-6 product option golden vector");

  auto decoded = decode_product_options(encoded.value());
  require(decoded.has_value(), "product options decode");
  require(product_options_digest(decoded.value()) ==
              product_options_digest(options),
          "product options canonical round trip");

  auto stale_schema = options;
  stale_schema.schemas.replay = product_contract_schema_version - 1;
  require(!validate_product_options(stale_schema).has_value(),
          "coordinated schema mismatch rejected");

  auto outer_schema = encoded.value();
  outer_schema[9] =
      static_cast<std::uint8_t>(product_contract_schema_version - 1);
  require(!decode_product_options(outer_schema).has_value(),
          "outer record schema mismatch rejected");

  auto unknown = encoded.value();
  require(unknown.size() > 30, "product option vector size");
  unknown[30] = 0xff;
  require(!decode_product_options(unknown).has_value(),
          "unknown selection enum rejected");

  auto trailing = encoded.value();
  trailing.push_back(0);
  require(!decode_product_options(trailing).has_value(),
          "trailing bytes rejected");

  product_decode_limits limits;
  limits.max_record_bytes = encoded.value().size() - 1;
  require(!decode_product_options(encoded.value(), limits).has_value(),
          "decode limit enforced before allocation");
}

void product_policy_validation() {
  auto strict = boolean_product_options{};
  strict.preparation.normalization.mode = normalization_mode::diagnosis_only;
  require(!validate_product_options(strict).has_value(),
          "strict preparation cannot hide normalization");

  auto diagnosis = boolean_product_options{};
  diagnosis.preparation.mode = preparation_mode::diagnosis_only;
  diagnosis.preparation.normalization.mode = normalization_mode::diagnosis_only;
  require(validate_product_options(diagnosis).has_value(),
          "diagnosis-only preparation is non-mutating");

  auto structural = boolean_product_options{};
  structural.preparation.mode = preparation_mode::normalized;
  structural.preparation.normalization.mode =
      normalization_mode::structural_only;
  structural.preparation.normalization.enabled_operations =
      normalization_operation_bit(
          normalization_operation::exact_duplicate_consolidation);
  require(validate_product_options(structural).has_value(),
          "structural normalization remains geometry preserving");
  structural.preparation.normalization.enabled_operations =
      normalization_operation_bit(normalization_operation::crack_closure);
  require(!validate_product_options(structural).has_value(),
          "structural normalization rejects geometry-changing operations");

  auto normalized = boolean_product_options{};
  normalized.preparation.mode = preparation_mode::normalized;
  normalized.preparation.normalization.mode =
      normalization_mode::geometry_changing;
  normalized.preparation.normalization.unit = model_unit::millimetre;
  normalized.preparation.normalization.model_tolerance = 0.01;
  normalized.preparation.normalization.enabled_operations =
      normalization_operation_bit(normalization_operation::crack_closure);
  require(validate_product_options(normalized).has_value(),
          "explicit geometry-changing normalization contract");
  normalized.preparation.normalization.model_tolerance =
      std::numeric_limits<double>::quiet_NaN();
  require(!validate_product_options(normalized).has_value(),
          "non-finite normalization tolerance rejected");

  auto exact_in_t = boolean_product_options{};
  exact_in_t.result.representation = result_representation::exact_in_T_mesh;
  exact_in_t.realization.semantics = product_realization_semantics::exact_in_T;
  exact_in_t.realization.search.strategy =
      realization_search_strategy::nearest_only;
  require(validate_product_options(exact_in_t).has_value(),
          "strict exact-in-T result contract");
  exact_in_t.realization.search.strategy =
      realization_search_strategy::neighboring_values;
  exact_in_t.realization.search.max_candidates = 32;
  require(validate_product_options(exact_in_t).has_value(),
          "search policy does not change exact semantics");
  require(exact_in_t.result.representation ==
                  result_representation::exact_in_T_mesh &&
              exact_in_t.realization.semantics ==
                  product_realization_semantics::exact_in_T,
          "search and semantic fields remain separate");

  auto approximate = boolean_product_options{};
  approximate.result.representation =
      result_representation::certified_approximate_mesh;
  approximate.realization.semantics =
      product_realization_semantics::certified_approximate_embedding_v1;
  approximate.realization.search.strategy =
      realization_search_strategy::deterministic_bounded_search;
  require(!validate_product_options(approximate).has_value(),
          "approximate output requires an explicit acceptance policy");
  approximate.realization.approximation.enabled = true;
  approximate.realization.approximation.unit = model_unit::millimetre;
  approximate.realization.approximation.max_vertex_displacement = 0.001;
  approximate.realization.approximation.declared_model_tolerance = 0.01;
  approximate.realization.search.max_candidates = 64;
  approximate.realization.search.max_candidate_evaluations = 4096;
  approximate.realization.search.max_search_nodes = 256;
  approximate.realization.search.max_obligations = 4096;
  approximate.realization.search.max_triangle_pairs = 4096;
  approximate.realization.search.max_predicate_checks = 65536;
  approximate.realization.search.max_verifier_work = 65536;
  approximate.realization.search.max_verifier_records = 65536;
  approximate.realization.search.max_verifier_bytes = 1048576;
  approximate.realization.approximation.candidate_ulp_radius = 1;
  approximate.realization.approximation.application_acceptance_metadata =
      "fixture-model-tolerance";
  require(validate_product_options(approximate).has_value(),
          "explicit approximate contract accepted");
  approximate.realization.approximation.max_axis_displacement_y = 0.0005;
  require(!validate_product_options(approximate).has_value(),
          "axis bound value requires explicit presence");
  approximate.realization.approximation.has_max_axis_displacement_y = true;
  require(validate_product_options(approximate).has_value(),
          "explicit optional axis bound accepted");
  approximate.result.retain_exact_result_on_realization_failure = false;
  require(!validate_product_options(approximate).has_value(),
          "exact authority cannot be discarded");

  auto attributes = boolean_product_options{};
  attributes.attributes.mode = attribute_transfer_mode::require_lossless;
  require(!validate_product_options(attributes).has_value(),
          "lossless attributes require conflict rejection");
  attributes.attributes.conflicts = attribute_conflict_policy::reject;
  require(validate_product_options(attributes).has_value(),
          "lossless attribute contract with rejection");

  require(!fallback_permitted_for(product_error_code::internal_invariant_error),
          "invariant errors never fall back");
  require(!fallback_permitted_for(product_error_code::verifier_disagreement),
          "verifier disagreement never falls back");
  require(fallback_permitted_for(product_error_code::backend_unavailable),
          "declared backend unavailability may be listed for fallback");
}

void backend_maturity_and_qualification() {
  const auto experimental = make_test_identity();
  auto backend_bytes = encode_backend_identity(experimental);
  require(backend_bytes.has_value(), "backend identity encode");
  auto backend_round_trip = decode_backend_identity(backend_bytes.value());
  require(backend_round_trip.has_value() &&
              same_backend_identity(backend_round_trip.value(), experimental),
          "backend identity canonical round trip");
  auto backend_trailing = backend_bytes.value();
  backend_trailing.push_back(0);
  require(!decode_backend_identity(backend_trailing).has_value(),
          "backend identity trailing bytes rejected");

  auto explicit_options = explicit_experimental_options();
  require(authorize_backend(explicit_options, experimental).has_value(),
          "experimental backend accepts explicit opt-in");
  explicit_options.backend.allow_experimental_backend = false;
  require(!authorize_backend(explicit_options, experimental).has_value(),
          "experimental backend is not silently selected");

  const auto candidate = make_test_identity(backend_maturity::candidate,
                                            "candidate-test-build", {1, 1, 0});
  explicit_options.backend.allow_experimental_backend = true;
  require(authorize_backend(explicit_options, candidate).has_value(),
          "candidate backend also requires explicit opt-in");

  const auto deprecated = make_test_identity(
      backend_maturity::deprecated, "deprecated-test-build", {1, 0, 1});
  require(!authorize_backend(explicit_options, deprecated).has_value(),
          "deprecated backend is never authorized");

  boolean_product_options default_options;
  require(!authorize_backend(default_options, experimental).has_value(),
          "qualified default rejects experimental backend");

  const auto qualified =
      make_test_identity(backend_maturity::qualified, "qualified-test-build");
  qualification_profile profile;
  profile.backend = qualified.id;
  profile.capability_digest = qualified.capability_digest;
  profile.representation = result_representation::exact_stratified;
  profile.preparation = preparation_mode::strict_validation;
  profile.workload_profile = "contract-fixture";
  auto manifest_made =
      make_qualification_manifest("contract-manifest", {profile});
  require(manifest_made.has_value(), "qualification manifest");
  auto manifest = manifest_made.value();
  default_options.qualification.workload_profile = "contract-fixture";
  default_options.qualification.manifest = qualification_manifest_reference{
      product_contract_schema_version, manifest.identifier,
      manifest.manifest_digest};
  require(authorize_backend(default_options, qualified, &manifest).has_value(),
          "qualified default requires exact manifest profile binding");
  manifest.manifest_digest.bytes[0] ^= 1;
  require(!authorize_backend(default_options, qualified, &manifest).has_value(),
          "mutated qualification manifest rejected");

  auto stale = experimental;
  stale.capability_digest.bytes[0] ^= 1;
  require(!validate_backend_identity(stale).has_value(),
          "stale capability digest rejected");
  stale = experimental;
  stale.capabilities.bits |= std::uint64_t(1) << 63;
  stale.capability_digest = backend_capability_digest(stale.capabilities);
  require(!validate_backend_identity(stale).has_value(),
          "unknown capability rejected");
}

void exact_result_lifetime_and_envelope() {
  auto provenance = experimental_provenance();
  auto exact = make_test_exact_result(operation::regularized_union, provenance);
  auto retained = exact;
  require(exact.use_count() >= 2 && !retained->canonical_bytes.empty(),
          "exact result owns immutable storage independently of context");
  require(!product_digest_is_zero(exact->canonical_digest),
          "exact result digest");
  require(!make_exact_result_handle(operation::regularized_union,
                                    exact_result_topology::empty, provenance,
                                    {})
               .has_value(),
          "empty exact serialization rejected");

  boolean_product_result<double, std::uint64_t> result;
  result.operation = operation_contract(operation::regularized_union);
  result.backend = provenance;
  result.exact_result = exact;
  result.preparation.input_digest = test_digest('a');
  result.preparation.prepared_digest = test_digest('b');
  result.preparation.policy_digest = test_digest('c');
  result.preparation.report_digest = test_digest('l');
  result.attributes.report_digest = test_digest('m');
  result.verification.passed = true;
  result.verification.verifier_set_version = 1;
  result.verification.verifier_set_digest = test_digest('d');
  result.verification.report_digest = test_digest('e');
  require(validate_product_result(result).has_value(),
          "verified exact stratified envelope");

  result.realization = realization_attempt_record{};
  result.realization->requested = result_representation::exact_in_T_mesh;
  result.realization->semantics = product_realization_semantics::exact_in_T;
  result.realization->failure = make_product_error(
      product_error_code::output_not_representable, "fixture");
  require(validate_product_result(result).has_value(),
          "failed mesh realization retains exact success");

  auto success = std::make_shared<boolean_success<double, std::uint64_t>>();
  success->selected_operation = operation::regularized_union;
  auto exact_boundary = read_exact_result(exact);
  require(exact_boundary.has_value(), "read exact result for mesh binding");
  success->selected_boundary_digest =
      exact_boundary.value()->selected_artifact_digest;
  certified_mesh_payload<double, std::uint64_t> payload;
  payload.success = success;
  payload.semantics = product_realization_semantics::exact_in_T;
  payload.exact_result_digest = exact->canonical_digest;
  payload.certificate.semantics = product_realization_semantics::exact_in_T;
  payload.certificate.backend = result.backend.producer;
  payload.certificate.exact_result_digest = exact->canonical_digest;
  payload.certificate.certificate_digest = test_digest('f');
  result.representation = result_representation::exact_in_T_mesh;
  result.mesh = payload;
  result.realization->succeeded = true;
  result.realization->failure.reset();
  require(!validate_product_result(result).has_value(),
          "wrapper-only digest cannot manufacture strict success");
}

void replay_contract() {
  product_replay_binding golden_replay;
  golden_replay.options_digest = sequence_digest(0);
  golden_replay.preparation_policy_digest = sequence_digest(16);
  golden_replay.preparation_report_digest = sequence_digest(32);
  golden_replay.adapter_version = {1, 2, 3};
  golden_replay.build_identifier = "b";
  golden_replay.capability_digest = sequence_digest(48);
  golden_replay.exact_result_digest = sequence_digest(64);
  golden_replay.realization_policy_digest = sequence_digest(80);
  golden_replay.attribute_policy_digest = sequence_digest(96);
  golden_replay.verifier_set_digest = sequence_digest(112);
  golden_replay.qualification_manifest_digest = sequence_digest(128);
  auto golden_encoded = encode_product_replay_binding(golden_replay);
  require(golden_encoded.has_value(), "replay golden encode");
  const std::string replay_golden =
      "5947425052503036000600000000000000ab0006000102030405060708090a0b"
      "0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b"
      "2c2d2e2f0001000100020003000000000000000162303132333435363738393a"
      "3b3c3d3e3f0000000000000000404142434445464748494a4b4c4d4e4f5051"
      "52535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f70"
      "7172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f";
  require(hex_bytes(golden_encoded.value()) == replay_golden,
          "schema-6 replay golden vector");

  auto options = explicit_experimental_options();
  const auto backend = make_test_identity();
  const auto provenance = experimental_provenance();
  auto exact = make_test_exact_result(operation::regularized_union, provenance);

  product_replay_binding replay;
  replay.options_digest = product_options_digest(options);
  replay.preparation_policy_digest = test_digest('g');
  replay.preparation_report_digest = test_digest('h');
  replay.producer = backend.id;
  replay.adapter_version = backend.adapter_version;
  replay.build_identifier = backend.build_identifier;
  replay.capability_digest = backend.capability_digest;
  replay.exact_result_digest = exact->canonical_digest;
  replay.realization_policy_digest = test_digest('i');
  replay.attribute_policy_digest = test_digest('j');
  replay.verifier_set_digest = test_digest('k');
  require(validate_product_replay_binding(replay, options, backend, &exact)
              .has_value(),
          "replay cross-layer bindings");

  auto encoded = encode_product_replay_binding(replay);
  require(encoded.has_value(), "replay encode");
  auto decoded = decode_product_replay_binding(encoded.value());
  require(decoded.has_value(), "replay decode");
  require(
      validate_product_replay_binding(decoded.value(), options, backend, &exact)
          .has_value(),
      "replay canonical round trip");
  decoded.value().capability_digest.bytes[0] ^= 1;
  require(!validate_product_replay_binding(decoded.value(), options, backend,
                                           &exact)
               .has_value(),
          "stale backend replay binding rejected");

  auto trailing = encoded.value();
  trailing.push_back(0);
  require(!decode_product_replay_binding(trailing).has_value(),
          "replay trailing bytes rejected");
}

} // namespace

int main() {
  struct test_case {
    const char *name;
    void (*run)();
  };
  const test_case tests[] = {
      {"round", product_policy_round_trip},
      {"validation", product_policy_validation},
      {"backend", backend_maturity_and_qualification},
      {"exact", exact_result_lifetime_and_envelope},
      {"replay", replay_contract},
  };

  int failures = 0;
  for (const auto &test : tests) {
    try {
      test.run();
      std::cout << "PASS " << test.name << '\n';
    } catch (const std::exception &error) {
      ++failures;
      std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
    }
  }
  return failures == 0 ? 0 : 1;
}
