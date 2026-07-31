#include "MeshBooleanOutputFixtures.h"
#include <YgorMeshesBooleanApproximate.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace ygor::mesh_boolean;

namespace {

digest marked(char value) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(value));
  return domain_digest({{'Y', 'G', 'B', 'A', 'F', 'I', 'X', 'T'}},
                       encoder.bytes());
}

product_realization_policy fixture_policy() {
  product_realization_policy policy;
  policy.semantics =
      product_realization_semantics::certified_approximate_embedding_v1;
  policy.search.strategy =
      realization_search_strategy::deterministic_bounded_search;
  policy.search.max_candidates = 27;
  policy.search.max_candidate_evaluations = 100000;
  policy.search.max_search_nodes = 1000000;
  policy.search.max_obligations = 1000000;
  policy.search.max_triangle_pairs = 1000000;
  policy.search.max_predicate_checks = 100000000;
  policy.search.max_verifier_work = 100000000;
  policy.search.max_verifier_records = 10000000;
  policy.search.max_verifier_bytes = 256U * 1024U * 1024U;
  policy.approximation.enabled = true;
  policy.approximation.unit = model_unit::unitless;
  policy.approximation.max_vertex_displacement = 1.0e-12;
  policy.approximation.max_support_plane_deviation = 1.0e-12;
  policy.approximation.declared_model_tolerance = 1.0e-11;
  policy.approximation.candidate_ulp_radius = 1;
  policy.approximation.application_acceptance_metadata = "isolation-fixture";
  return policy;
}

backend_identity fixture_backend() {
  backend_capabilities capabilities;
  for (const auto capability : {
           backend_capability::exact_set_semantics,
           backend_capability::exact_coordinates,
           backend_capability::stratified_output,
           backend_capability::manifold_mesh_output,
           backend_capability::deterministic_canonical_output,
           backend_capability::certified_failure_categories,
           backend_capability::provenance_mapping,
           backend_capability::strict_prepared_operands,
           backend_capability::certified_approximate_output})
    capabilities.set(capability);
  auto backend = make_backend_identity(backend_id::experimental_exact_v1,
                                       {1, 0, 0}, "isolation-fixture",
                                       capabilities,
                                       backend_maturity::experimental);
  if (!backend.has_value())
    throw std::runtime_error("backend");
  return backend.value();
}

std::vector<std::uint8_t> replace_certificate(
    std::vector<std::uint8_t> envelope,
    const std::vector<std::uint8_t> &certificate) {
  auto read_u64 = [&](std::size_t &offset) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i)
      value = (value << 8) | envelope.at(offset++);
    return value;
  };
  std::size_t offset = 12;
  const auto exact_size = read_u64(offset);
  offset += exact_size;
  const auto output_size = read_u64(offset);
  offset += output_size;
  const auto old_size = read_u64(offset);
  if (old_size != certificate.size() || offset + old_size != envelope.size())
    throw std::runtime_error("certificate envelope layout");
  std::copy(certificate.begin(), certificate.end(), envelope.begin() + offset);
  return envelope;
}

void write_blob(std::ofstream &output, const std::vector<std::uint8_t> &bytes) {
  const auto size = static_cast<std::uint64_t>(bytes.size());
  output.write(reinterpret_cast<const char *>(&size), sizeof(size));
  output.write(reinterpret_cast<const char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  auto a = input_test::box<double, std::uint32_t>(0.0, 1.0);
  auto b = input_test::box<double, std::uint32_t>(3.0, 4.0);
  auto context = classification_test::context(
      a, b, output_test::registry(), operation::regularized_union,
      boolean_options{});
  exact_result_backend_binding backend_binding;
  backend_binding.producer = fixture_backend();
  backend_binding.attempted_backends = {backend_id::experimental_exact_v1};
  exact_result_preparation_binding preparation;
  preparation.input_digest = marked('i');
  preparation.prepared_digest = marked('p');
  preparation.policy_digest = marked('o');
  preparation.report_digest = marked('r');
  auto result = evaluate_boolean_product_result(
      *context, std::move(backend_binding), std::move(preparation),
      result_representation::certified_approximate_mesh, fixture_policy());
  if (!result.has_value() || !result.value()->mesh) {
    std::cerr << "fixture product failed";
    if (!result.has_value())
      std::cerr << ": " << result.error().message_key;
    else if (result.value()->realization &&
        result.value()->realization->failure)
      std::cerr << ": " << result.value()->realization->failure->message_key;
    std::cerr << '\n';
    return 3;
  }
  const auto &mesh = *result.value()->mesh;
  auto valid = encode_certified_approximate_embedding(
      result.value()->exact_result, mesh.policy, *mesh.success,
      *mesh.approximate_certificate);
  if (!valid.has_value()) {
    std::cerr << "fixture serialization failed\n";
    return 4;
  }
  auto link_certificate = *mesh.approximate_certificate;
  if (link_certificate.occurrence_maps.empty() ||
      link_certificate.occurrence_maps.front().output_cyclic_triangles.empty())
    return 5;
  auto &link =
      link_certificate.occurrence_maps.front().output_cyclic_triangles.front();
  link = realization_triangle_id::from_canonical_value(
      (link.value_for_debug() + 1) % link_certificate.triangles.size());
  canonicalize_certified_approximate_certificate(link_certificate);
  auto radial_certificate = *mesh.approximate_certificate;
  if (radial_certificate.patch_adjacency.empty() ||
      radial_certificate.patch_adjacency.front().exact_radial_patches.empty())
    return 6;
  auto &radial =
      radial_certificate.patch_adjacency.front().exact_radial_patches.front();
  radial = selected_patch_id::from_canonical_value(
      (radial.value_for_debug() + 1) % radial_certificate.exact_patches);
  canonicalize_certified_approximate_certificate(radial_certificate);
  std::ofstream output(argv[1], std::ios::binary | std::ios::trunc);
  if (!output)
    return 7;
  write_blob(output, valid.value());
  write_blob(output,
             replace_certificate(valid.value(),
                                 link_certificate.canonical_bytes));
  write_blob(output,
             replace_certificate(valid.value(),
                                 radial_certificate.canonical_bytes));
  return output.good() ? 0 : 8;
}
