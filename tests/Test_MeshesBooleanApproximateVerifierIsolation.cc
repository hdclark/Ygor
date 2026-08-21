#include <YgorMeshesBooleanApproximate.h>

#include <cstdint>
#include <fstream>
#include <vector>

using namespace ygor::mesh_boolean;

namespace {
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

std::vector<std::uint8_t> read_blob(std::ifstream &input) {
  std::uint64_t size = 0;
  input.read(reinterpret_cast<char *>(&size), sizeof(size));
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  input.read(reinterpret_cast<char *>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  return bytes;
}
} // namespace

int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  std::ifstream input(argv[1], std::ios::binary);
  if (!input)
    return 3;
  const auto valid = read_blob(input);
  const auto link_mutation = read_blob(input);
  const auto radial_mutation = read_blob(input);
  if (!input)
    return 4;
  const auto policy = fixture_policy();
  const auto replay =
      verify_serialized_certified_approximate_embedding<double, std::uint32_t>(
          valid, policy);
  if (!replay.has_value() || !replay.value())
    return 5;
  if (verify_serialized_certified_approximate_embedding<double, std::uint32_t>(
          link_mutation, policy).has_value())
    return 6;
  if (verify_serialized_certified_approximate_embedding<double, std::uint32_t>(
          radial_mutation, policy).has_value())
    return 7;
  return 0;
}
