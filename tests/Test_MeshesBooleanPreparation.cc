#include "MeshBooleanInputTopologyFixtures.h"

#include <YgorMeshesBooleanPreparation.h>
#include <YgorMeshesBooleanNormalization.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>

using namespace input_test;

template <class T, class I>
prepared_operand<T, I> prepare(const fv_surface_mesh<T, I> &mesh,
                               const std::shared_ptr<verifier_registry> &registry) {
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  auto result = validate_operand_strict(mesh, strict_validation_policy{},
                                        boolean_options{}, kernel, verifier);
  require(result.has_value(), "strict preparation succeeds");
  return result.value();
}

template <class T, class I> void round_trip_specialization() {
  auto registry = input_test::registry();
  auto source = tetra<T, I>();
  auto prepared = prepare(source, registry);
  fv_surface_mesh<T, I> empty;
  auto validation_context = context(source, empty, registry);
  auto component2 = validate_operands(*validation_context);
  require(component2.has_value(), "certificate comparison validation");
  require(prepared.certificate().semantic_digest ==
              component2.value()->payload->operands[0].semantic_digest &&
              prepared.certificate().validation_artifact_digest ==
                  component2.value()->payload->artifact_digest &&
              prepared.certificate().validation_report_digest ==
                  component2.value()->report.report_digest,
          "certificate binds actual Component 2 artifact and report");
  auto bytes = encode_prepared_operand(prepared);
  require(bytes.has_value(), "prepared encoding succeeds");
  auto decoded = decode_prepared_operand<T, I>(bytes.value());
  require(decoded.has_value(), "prepared decoding succeeds");
  auto reencoded = encode_prepared_operand(decoded.value());
  require(reencoded.has_value() && reencoded.value() == bytes.value(),
          "prepared reserialization is canonical");
  require(decoded.value().certificate().semantic_digest ==
              prepared.certificate().semantic_digest,
          "prepared semantic binding survives round trip");
}

void ownership_and_equivalence() {
  using T = double;
  using I = std::uint32_t;
  auto registry = input_test::registry();
  auto a = tetra<T, I>();
  auto b = box<T, I>(T(2), T(3));

  auto direct_context = context(a, b, registry);
  auto direct = validate_operands(*direct_context);
  require(direct.has_value(), "raw operands validate");

  auto prepared_a = prepare(a, registry);
  auto prepared_b = prepare(b, registry);
  const auto frozen_x = prepared_a.mesh().vertices[0].x;
  a.vertices[0].x = T(99);
  a.faces.clear();
  require(prepared_a.mesh().vertices[0].x == frozen_x &&
              prepared_a.mesh().faces.size() == 4,
          "prepared operand owns immutable input");

  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  require(verify_prepared_operand(prepared_a, boolean_options{}, kernel,
                                  verifier)
              .has_value(),
          "prepared certificate independently replays Component 2");
  auto prepared_context = make_boolean_context(
      prepared_a, prepared_b, operation::regularized_union, boolean_options{},
      kernel, verifier);
  if (!prepared_context.has_value())
    throw std::runtime_error("prepared request: " +
                             render_error(prepared_context.error()));
  require(prepared_context.value()->owner() != direct_context->owner(),
          "durable preparation is valid in a foreign context");
  auto accepted = validate_operands(*prepared_context.value());
  require(accepted.has_value(), "accepted preparation revalidates");
  require(accepted.value()->payload->operands[0].semantic_digest ==
              direct.value()->payload->operands[0].semantic_digest &&
              accepted.value()->payload->operands[1].semantic_digest ==
                  direct.value()->payload->operands[1].semantic_digest,
          "raw and prepared validation are semantically equivalent");

  auto retained_context = [&]() {
    auto local_a = prepare(tetra<T, I>(), registry);
    auto local_b = prepare(box<T, I>(T(4), T(5)), registry);
    auto made = make_boolean_context(
        local_a, local_b, operation::regularized_union, boolean_options{},
        kernel, verifier);
    require(made.has_value(), "temporary prepared request succeeds");
    return std::move(made.value());
  }();
  require(validate_operands(*retained_context).has_value(),
          "context retains prepared operands after handle destruction");
  require(retained_context->preparation_provenance().has_value(),
          "prepared context retains product provenance");
}

void stale_and_malformed_records() {
  using T = double;
  using I = std::uint32_t;
  auto registry = input_test::registry();
  auto prepared = prepare(tetra<T, I>(), registry);
  auto encoded = encode_prepared_operand(prepared);
  require(encoded.has_value(), "stale fixture encodes");

  auto corrupted = encoded.value();
  corrupted[corrupted.size() - 2] ^= 1U;
  auto stale = decode_prepared_operand<T, I>(corrupted);
  require(!stale.has_value() &&
              stale.error().subcode == static_cast<std::uint32_t>(
                                           preparation_validation_subcode::
                                               stale_certificate),
          "stale certificate is rejected");

  auto stale_mesh = encoded.value();
  stale_mesh[22] ^= 1U;
  auto stale_input = decode_prepared_operand<T, I>(stale_mesh);
  require(!stale_input.has_value() &&
              stale_input.error().subcode == static_cast<std::uint32_t>(
                                              preparation_validation_subcode::
                                                  stale_input_digest),
          "stale prepared mesh binding is rejected");

  auto stale_report = encoded.value();
  stale_report[stale_report.size() - 64] ^= 1U;
  auto report_rejected = decode_prepared_operand<T, I>(stale_report);
  require(!report_rejected.has_value(), "stale report binding is rejected");

  auto truncated = encoded.value();
  truncated.pop_back();
  require(!decode_prepared_operand<T, I>(truncated).has_value(),
          "truncated preparation is rejected");
  auto trailing = encoded.value();
  trailing.push_back(0);
  require(!decode_prepared_operand<T, I>(trailing).has_value(),
          "trailing preparation bytes are rejected");
  prepared_operand_decode_limits limits;
  limits.max_vertices = 1;
  auto limited = decode_prepared_operand<T, I>(encoded.value(), limits);
  require(!limited.has_value() &&
              limited.error().code == boolean_error_code::resource_limit,
          "prepared decode enforces resource limits");
  auto short_counts = encoded.value();
  short_counts.resize(22);
  require(!decode_prepared_operand<T, I>(short_counts).has_value(),
          "truncated counts are rejected before allocation");
}

void failures_and_policy_binding() {
  using T = double;
  using I = std::uint32_t;
  auto registry = input_test::registry();
  auto invalid = tetra<T, I>();
  invalid.faces[0][0] = I(99);
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  auto rejected = validate_operand_strict(
      invalid, strict_validation_policy{}, boolean_options{}, kernel, verifier);
  require(!rejected.has_value() &&
              rejected.error().subcode == static_cast<std::uint32_t>(
                                              input_validation_subcode::
                                                  index_out_of_range),
          "strict service preserves Component 2 failure");

  strict_validation_policy unsupported;
  unsupported.remove_unused_storage = true;
  auto policy_rejected = validate_operand_strict(
      tetra<T, I>(), unsupported, boolean_options{}, kernel, verifier);
  require(!policy_rejected.has_value(),
          "unimplemented storage edits fail closed");

  boolean_options exhaustive_options;
  exhaustive_options.verification = verification_level::exhaustive;
  auto a = prepare(tetra<T, I>(), registry);
  auto b = prepare(fv_surface_mesh<T, I>{}, registry);
  auto mismatch = make_boolean_context(
      a, b, operation::regularized_union, exhaustive_options, kernel, verifier);
  require(!mismatch.has_value() &&
              mismatch.error().subcode == static_cast<std::uint32_t>(
                                              preparation_validation_subcode::
                                                  stale_policy_digest),
          "prepared request verifies policy binding");
}

void normalized_lifetime_and_provenance() {
  using T = double;
  using I = std::uint32_t;
  normalization_report report_a, report_b;
  auto normalized_a =
      normalize_operand(tetra<T, I>(), normalization_policy{}, report_a);
  auto normalized_b = normalize_operand(box<T, I>(T(2), T(3)),
                                        normalization_policy{}, report_b);
  require(normalized_a.has_value() && normalized_b.has_value(),
          "normalized preparation fixtures succeed");
  auto bytes = encode_prepared_operand(normalized_a.value());
  require(bytes.has_value(), "normalized preparation serializes");
  auto decoded = decode_prepared_operand<T, I>(bytes.value());
  require(decoded.has_value() && decoded.value().normalization() &&
              decoded.value().normalization()->report_digest ==
                  report_a.report_digest,
          "normalization report is owned across prepared serialization");
  auto registry = input_test::registry();
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  auto made = make_boolean_context(
      decoded.value(), normalized_b.value(), operation::regularized_union,
      boolean_options{}, kernel, verifier);
  require(made.has_value() && made.value()->preparation_provenance() &&
              made.value()->preparation_provenance()->report_digest != digest{},
          "context provenance binds normalization report digests");

  auto structural_source = tetra<T, I>();
  structural_source.vertices.push_back({T(7), T(8), T(9)});
  normalization_policy structural_policy;
  structural_policy.mode = normalization_mode::structural_only;
  structural_policy.enabled_operations = normalization_operation_bit(
      normalization_operation::irrelevant_storage_removal);
  normalization_report structural_report;
  auto structural = normalize_operand(structural_source, structural_policy,
                                      structural_report);
  require(structural.has_value() &&
              structural.value().mesh() == tetra<T, I>() &&
              structural.value().normalization_source() &&
              *structural.value().normalization_source() == structural_source,
          "structural preparation owns its replay source");
  auto structural_bytes = encode_prepared_operand(structural.value());
  require(structural_bytes.has_value(),
          "structural preparation and replay source serialize");
  auto structural_decoded =
      decode_prepared_operand<T, I>(structural_bytes.value());
  require(structural_decoded.has_value() &&
              structural_decoded.value().normalization_source() &&
              *structural_decoded.value().normalization_source() ==
                  structural_source &&
              verify_prepared_operand(structural_decoded.value(),
                                      boolean_options{}, kernel, verifier)
                  .has_value(),
          "structural preparation independently replays after round trip");
  std::uint64_t seven_bits = 0;
  const T seven = T(7);
  std::memcpy(&seven_bits, &seven, sizeof(seven));
  std::array<std::uint8_t, 8> seven_bytes{};
  for (std::size_t i = 0; i != seven_bytes.size(); ++i)
    seven_bytes[i] = static_cast<std::uint8_t>(seven_bits >> (56 - i * 8));
  auto stale_source_bytes = structural_bytes.value();
  const auto seven_at = std::search(stale_source_bytes.begin(),
                                    stale_source_bytes.end(),
                                    seven_bytes.begin(), seven_bytes.end());
  require(seven_at != stale_source_bytes.end(),
          "serialized normalization source is present");
  *seven_at ^= 1U;
  require(!decode_prepared_operand<T, I>(stale_source_bytes).has_value(),
          "serialized normalization source is digest-bound");
  auto structural_context = make_boolean_context(
      structural_decoded.value(), normalized_b.value(),
      operation::regularized_union, boolean_options{}, kernel, verifier);
  require(structural_context.has_value(),
          "Boolean request accepts verified structural preparation");
}

int main() {
  try {
    round_trip_specialization<float, std::uint32_t>();
    round_trip_specialization<float, std::uint64_t>();
    round_trip_specialization<double, std::uint32_t>();
    round_trip_specialization<double, std::uint64_t>();
    ownership_and_equivalence();
    stale_and_malformed_records();
    failures_and_policy_binding();
    normalized_lifetime_and_provenance();
    std::cout << "PASS strict operand preparation\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
