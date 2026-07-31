#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanQualificationIngestion.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

std::uint64_t binary64_bits(double value) {
  std::uint64_t bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "binary64 size mismatch");
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

digest tagged_digest(char kind, std::uint64_t ordinal) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(kind));
  encoder.u64(ordinal);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'I', 'T', '0', '1'}},
                       encoder.bytes());
}

struct artifact_fixture {
  qualification_cad_artifact_reference reference;
  std::vector<std::uint8_t> bytes;
};

artifact_fixture repository_artifact(
    std::string identifier, qualification_cad_artifact_kind kind,
    std::string path, std::string payload) {
  artifact_fixture fixture;
  fixture.bytes.assign(payload.begin(), payload.end());
  fixture.reference.identifier = std::move(identifier);
  fixture.reference.kind = kind;
  fixture.reference.media_type = "text/plain";
  fixture.reference.byte_count = fixture.bytes.size();
  fixture.reference.content_digest =
      qualification_cad_artifact_content_digest(fixture.bytes);
  fixture.reference.repository_path = std::move(path);
  auto made =
      make_qualification_cad_artifact_reference(fixture.reference);
  require(made.has_value(), "repository artifact canonicalizes");
  fixture.reference = std::move(made.value());
  return fixture;
}

artifact_fixture external_artifact(std::string identifier,
                                   qualification_cad_artifact_kind kind,
                                   std::string payload) {
  artifact_fixture fixture;
  fixture.bytes.assign(payload.begin(), payload.end());
  fixture.reference.identifier = std::move(identifier);
  fixture.reference.kind = kind;
  fixture.reference.redistribution =
      qualification_redistribution::content_addressed_external;
  fixture.reference.retrieval =
      qualification_cad_retrieval_kind::content_addressed;
  fixture.reference.media_type = "application/octet-stream";
  fixture.reference.byte_count = fixture.bytes.size();
  fixture.reference.content_digest =
      qualification_cad_artifact_content_digest(fixture.bytes);
  fixture.reference.content_address =
      qualification_cad_content_address(fixture.reference.content_digest);
  fixture.reference.retrieval_procedure = {
      "Resolve the reviewed content address in the controlled corpus store.",
      "Verify byte count and digest before qualification execution."};
  auto made =
      make_qualification_cad_artifact_reference(fixture.reference);
  require(made.has_value(), "external artifact canonicalizes");
  fixture.reference = std::move(made.value());
  return fixture;
}

qualification_cad_artifact_reference private_artifact(
    std::string identifier, qualification_cad_artifact_kind kind,
    std::string payload) {
  std::vector<std::uint8_t> bytes(payload.begin(), payload.end());
  qualification_cad_artifact_reference reference;
  reference.identifier = std::move(identifier);
  reference.kind = kind;
  reference.redistribution = qualification_redistribution::private_digest_only;
  reference.retrieval = qualification_cad_retrieval_kind::unavailable_private;
  reference.media_type = "application/octet-stream";
  reference.byte_count = bytes.size();
  reference.content_digest = qualification_cad_artifact_content_digest(bytes);
  reference.retrieval_permitted = false;
  auto made = make_qualification_cad_artifact_reference(std::move(reference));
  require(made.has_value(), "private artifact canonicalizes");
  return std::move(made.value());
}

qualification_cad_case_record repository_case() {
  auto primary = repository_artifact(
      "primary", qualification_cad_artifact_kind::operand_pair_bundle,
      "tests/mesh_boolean/cad_ingestion/representatives/generated-profile-pair-v1.txt",
      "generated operand pair");
  auto preparation_report = repository_artifact(
      "preparation", qualification_cad_artifact_kind::preparation_report,
      "tests/mesh_boolean/cad_ingestion/evidence/strict-preparation-report.txt",
      "strict preparation report");
  auto expected = repository_artifact(
      "expected", qualification_cad_artifact_kind::expected_outcome_evidence,
      "tests/mesh_boolean/cad_ingestion/evidence/expected-exact-success.txt",
      "verified exact success");
  auto license = repository_artifact(
      "license",
      qualification_cad_artifact_kind::license_or_provenance_evidence,
      "tests/mesh_boolean/cad_ingestion/evidence/internally-generated-license.txt",
      "Ygor generated; repository redistributable");

  qualification_cad_preparation_record preparation;
  preparation.mode = preparation_mode::strict_validation;
  preparation.policy_digest = tagged_digest('P', 1);
  preparation.source_digest = primary.reference.content_digest;
  preparation.output_digest = primary.reference.content_digest;
  preparation.report_digest = preparation_report.reference.content_digest;
  auto prepared = make_qualification_cad_preparation_record(preparation);
  require(prepared.has_value(), "strict preparation record canonicalizes");

  qualification_result_mode_binding exact;
  exact.representation = result_representation::exact_stratified;
  exact.semantics = product_realization_semantics::not_requested;
  exact.policy_digest = tagged_digest('R', 1);

  qualification_cad_case_record record;
  record.identifier = "internally-generated-profile-v1";
  record.source_or_generator_class = "exact-profile-extrusion-v1";
  record.source_system = "Ygor qualification generator";
  record.license_or_provenance =
      "Ygor-generated; repository redistributable";
  record.intended_model_tolerance.unit = model_unit::millimetre;
  record.intended_model_tolerance.binary64_bits = binary64_bits(0.01);
  record.case_count = 32;
  record.preparation = std::move(prepared.value());
  record.geometry_categories = {
      qualification_geometry_category::non_box_intersection,
      qualification_geometry_category::concave_or_reentrant};
  record.operations = {
      {operation::regularized_union, qualification_operand_order::a_then_b},
      {operation::a_minus_b,
       qualification_operand_order::a_then_b}};
  record.result_modes = {exact};
  record.expected_outcomes = {qualification_outcome::verified_exact_success};
  record.artifacts = {primary.reference, preparation_report.reference,
                      expected.reference, license.reference};
  auto made = make_qualification_cad_case_record(std::move(record));
  require(made.has_value(), "repository CAD-like case canonicalizes");
  return std::move(made.value());
}

void artifact_integrity_and_retrieval() {
  auto repository = repository_artifact(
      "repository", qualification_cad_artifact_kind::operand_pair_bundle,
      "tests/mesh_boolean/cad_ingestion/representatives/generated-profile-pair-v1.txt",
      "repository bytes");
  require(validate_qualification_cad_artifact_reference(repository.reference)
              .has_value() &&
              verify_qualification_cad_artifact_bytes(repository.reference,
                                                      repository.bytes)
                  .has_value(),
          "repository artifact verifies exact bytes");
  qualification_cad_artifact_loader repository_loader =
      [&](const auto &) -> product_status_or<std::vector<std::uint8_t>> {
    return repository.bytes;
  };
  require(materialize_qualification_cad_artifact(repository.reference,
                                                 repository_loader)
              .has_value(),
          "repository artifact materializes through caller loader");

  auto external = external_artifact(
      "external", qualification_cad_artifact_kind::source_model_bundle,
      "external controlled bytes");
  qualification_cad_artifact_loader external_loader =
      [&](const auto &) -> product_status_or<std::vector<std::uint8_t>> {
    return external.bytes;
  };
  require(materialize_qualification_cad_artifact(external.reference,
                                                 external_loader)
              .has_value(),
          "content-addressed artifact verifies before publication");
  auto corrupt = external.bytes;
  corrupt.front() ^= 1U;
  require(!verify_qualification_cad_artifact_bytes(external.reference, corrupt)
               .has_value(),
          "artifact corruption fails closed");

  auto private_reference = private_artifact(
      "private", qualification_cad_artifact_kind::operand_pair_bundle,
      "private bytes not committed");
  bool loader_called = false;
  qualification_cad_artifact_loader forbidden_loader =
      [&](const auto &) -> product_status_or<std::vector<std::uint8_t>> {
    loader_called = true;
    return std::vector<std::uint8_t>{1};
  };
  require(!materialize_qualification_cad_artifact(private_reference,
                                                  forbidden_loader)
               .has_value() &&
              !loader_called,
          "private digest-only artifacts cannot be retrieved");

  auto traversing = repository.reference;
  traversing.repository_path = "../private/source";
  traversing.reference_digest = {};
  require(!make_qualification_cad_artifact_reference(std::move(traversing))
               .has_value(),
          "repository path traversal fails closed");
}

void case_cross_binding_contracts() {
  auto record = repository_case();
  require(validate_qualification_cad_case_record(record).has_value(),
          "canonical CAD-like case validates");
  require(record.preparation.source_digest ==
              std::find_if(record.artifacts.begin(), record.artifacts.end(),
                           [](const auto &artifact) {
                             return artifact.kind ==
                                    qualification_cad_artifact_kind::
                                        operand_pair_bundle;
                           })
                  ->content_digest,
          "preparation source binds the retained primary artifact");

  auto stale_report = record;
  stale_report.preparation.report_digest = tagged_digest('X', 2);
  stale_report.preparation.record_digest = {};
  stale_report.record_digest = {};
  require(!make_qualification_cad_case_record(std::move(stale_report))
               .has_value(),
          "preparation report substitution fails case cross-binding");

  auto vague_failure = record;
  vague_failure.identifier = "vague-failure-v1";
  vague_failure.expected_outcomes = {
      qualification_outcome::expected_typed_failure};
  vague_failure.expected_failure_codes.clear();
  vague_failure.record_digest = {};
  require(!make_qualification_cad_case_record(std::move(vague_failure))
               .has_value(),
          "typed-failure expectation requires exact error vocabulary");
}

void manifest_round_trip_and_summary() {
  qualification_cad_ingestion_manifest input;
  input.identifier = "p6.6-controlled-cad-like-v1";
  input.version = "1";
  input.records = {repository_case()};
  auto manifest = make_qualification_cad_ingestion_manifest(std::move(input));
  require(manifest.has_value() &&
              manifest.value().anonymized_summary.total_case_count == 32 &&
              manifest.value().anonymized_summary
                      .repository_embedded_case_count == 32 &&
              manifest.value().manifest_digest != digest{},
          "manifest reconstructs anonymized counts and bindings");
  require(validate_qualification_cad_ingestion_manifest(manifest.value())
              .has_value(),
          "canonical manifest validates");

  auto bytes = encode_qualification_cad_ingestion_manifest(manifest.value());
  require(bytes.has_value(), "manifest encodes");
  auto replay = decode_qualification_cad_ingestion_manifest(bytes.value());
  require(replay.has_value() &&
              replay.value().manifest_digest == manifest.value().manifest_digest,
          "manifest round-trips canonically");
  auto bindings = make_qualification_cad_corpus_bindings(replay.value());
  require(bindings.has_value() && bindings.value().size() == 1 &&
              bindings.value().front().case_count == 32,
          "validated ingestion records bridge to P6.1 corpus bindings");

  auto corrupt = bytes.value();
  corrupt.back() ^= 1U;
  require(!decode_qualification_cad_ingestion_manifest(corrupt).has_value(),
          "manifest corruption fails closed");
  qualification_cad_ingestion_decode_limits limits;
  limits.max_record_bytes = bytes.value().size() - 1;
  require(!decode_qualification_cad_ingestion_manifest(bytes.value(), limits)
               .has_value(),
          "decode resource limits are enforced before publication");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.6.artifacts", artifact_integrity_and_retrieval);
  tests.add("P6.6.cases", case_cross_binding_contracts);
  tests.add("P6.6.manifest", manifest_round_trip_and_summary);
  return tests.run(std::cout, std::cerr);
}
