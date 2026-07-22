#include "CanonicalHalfedgeFixtures.h"

#include "YgorMeshesBooleanBounded/CanonicalHalfedgeCodec.h"
#include "YgorMeshesBooleanBounded/CanonicalHalfedgeVerifier.h"
#include "YgorMeshesBooleanBounded/Sha256.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace tests = canonical_halfedge_tests;
namespace bounded = ygor::mesh_boolean::bounded;

namespace {

template <class T, class I>
void basic_invariants(const tests::built_fixture<T, I> &fixture) {
  const auto &artifact = *fixture.artifact;
  const auto &source = *fixture.source;
  const auto &validated = *fixture.predecessor.operand;
  tests::require(
      bounded::verify_canonical_halfedge_operand(
          artifact, validated, source, *fixture.predecessor.precision),
      "canonical halfedge independent verification");
  tests::require(artifact.verification() ==
                     bounded::canonical_halfedge_verification_disposition::
                         independently_verified,
                 "canonical halfedge verification disposition");
  tests::require(artifact.triangles().size() == source.triangles().size(),
                 "canonical triangle count");
  tests::require(artifact.halfedges().size() ==
                     artifact.triangles().size() * 3,
                 "triangle-major halfedge count");
  tests::require(artifact.edges().size() * 2 == artifact.halfedges().size(),
                 "closed manifold edge handshake");
  tests::require(artifact.source_edge_to_edge().size() ==
                     validated.edges().size(),
                 "source edge projection size");
  tests::require(artifact.source_diagonal_to_edge().size() ==
                     source.diagonals().size(),
                 "source diagonal projection size");
  tests::require(artifact.facet_groups().size() == validated.facets().size(),
                 "facet group count");
  tests::require(artifact.shell_groups().size() == validated.shells().size(),
                 "shell group count");
  tests::require(artifact.digest() ==
                     bounded::sha256::digest(artifact.canonical_bytes()),
                 "canonical halfedge digest");
  tests::require(
      bounded::encode_canonical_halfedge_operand_independent(artifact) ==
          artifact.canonical_bytes(),
      "independent canonical encoder");
}

template <class T, class I> void unit_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::box<T, I>());
  basic_invariants(fixture);
  const auto &artifact = *fixture.artifact;
  for (std::uint64_t triangle_id = 0;
       triangle_id < artifact.triangles().size(); ++triangle_id) {
    const auto &triangle = artifact.triangles()[triangle_id];
    tests::require(triangle.canonical_id == triangle_id,
                   "canonical triangle id");
    for (std::uint8_t slot = 0; slot < 3; ++slot) {
      const auto halfedge_id = triangle_id * 3 + slot;
      const auto &halfedge = artifact.halfedges()[halfedge_id];
      tests::require(triangle.halfedges[slot] == halfedge_id,
                     "triangle-major halfedge layout");
      tests::require(halfedge.next == triangle_id * 3 + ((slot + 1) % 3),
                     "halfedge next cycle");
      tests::require(halfedge.previous ==
                         triangle_id * 3 + ((slot + 2) % 3),
                     "halfedge previous cycle");
      tests::require(halfedge.origin == triangle.vertices[slot] &&
                         halfedge.destination ==
                             triangle.vertices[(slot + 1) % 3],
                     "halfedge endpoints follow triangle cycle");
    }
  }
}

template <class T, class I> void pairing_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::box<T, I>());
  const auto &artifact = *fixture.artifact;
  for (const auto &halfedge : artifact.halfedges()) {
    tests::require(halfedge.pair < artifact.halfedges().size(),
                   "paired halfedge range");
    const auto &pair = artifact.halfedges()[halfedge.pair];
    tests::require(pair.pair == halfedge.canonical_id,
                   "pair reciprocity");
    tests::require(halfedge.origin == pair.destination &&
                       halfedge.destination == pair.origin,
                   "pair canonical endpoints reversed");
    tests::require(halfedge.source_origin == pair.source_destination &&
                       halfedge.source_destination == pair.source_origin,
                   "pair source endpoints reversed");
  }
  for (const auto &edge : artifact.edges()) {
    tests::require(edge.halfedges[0] < edge.halfedges[1],
                   "edge halfedges canonical order");
    const auto &representative = artifact.halfedges()[edge.representative];
    tests::require(representative.origin == edge.endpoints[0] &&
                       representative.destination == edge.endpoints[1],
                   "low-to-high edge representative");
    if (edge.edge_class == bounded::canonical_edge_class::source_edge) {
      tests::require(bounded::canonical_edge_is_source_feature(edge),
                     "source edge semantic ownership");
      tests::require(!bounded::canonical_edge_is_bookkeeping_only(edge),
                     "source edge not bookkeeping");
    } else {
      tests::require(bounded::canonical_edge_is_bookkeeping_only(edge),
                     "diagonal bookkeeping semantics");
      tests::require(!bounded::canonical_edge_is_source_feature(edge),
                     "diagonal not a source feature");
    }
  }
  for (auto edge : artifact.source_edge_to_edge())
    tests::require(edge != bounded::canonical_invalid_ordinal,
                   "all source edges projected");
  for (auto edge : artifact.source_diagonal_to_edge())
    tests::require(edge != bounded::canonical_invalid_ordinal,
                   "all internal diagonals projected");
}

template <class T, class I> void fan_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::box<T, I>());
  const auto &artifact = *fixture.artifact;
  tests::require(artifact.fans().size() == artifact.vertices().size(),
                 "one closed fan per represented vertex");
  std::set<std::uint64_t> covered;
  for (const auto &fan : artifact.fans()) {
    tests::require(!fan.outgoing_halfedges.empty(), "nonempty fan");
    auto current = fan.outgoing_halfedges.front();
    for (auto expected : fan.outgoing_halfedges) {
      tests::require(current == expected, "canonical fan traversal order");
      tests::require(artifact.halfedges()[current].origin == fan.vertex,
                     "fan remains at vertex");
      tests::require(covered.insert(current).second,
                     "fan halfedge unique coverage");
      const auto next = bounded::canonical_fan_next(artifact.halfedges(), current);
      tests::require(
          bounded::canonical_fan_previous(artifact.halfedges(), next) == current,
          "fan transition inverse");
      current = next;
    }
    tests::require(current == fan.outgoing_halfedges.front(),
                   "fan closes");
  }
  tests::require(covered.size() == artifact.halfedges().size(),
                 "fans cover all halfedges exactly once by origin");
}

template <class T, class I> void groups_and_geometry_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::concave_prism<T, I>());
  basic_invariants(fixture);
  const auto &artifact = *fixture.artifact;
  const auto &validated = *fixture.predecessor.operand;
  for (const auto &vertex : artifact.vertices()) {
    tests::require(vertex.bound.valid(), "vertex bound valid");
    tests::require(vertex.bound.axes[0].contains(vertex.committed_point[0]) &&
                       vertex.bound.axes[1].contains(vertex.committed_point[1]) &&
                       vertex.bound.axes[2].contains(vertex.committed_point[2]),
                   "vertex bound contains committed point");
  }
  for (const auto &triangle : artifact.triangles()) {
    tests::require(triangle.bound.valid(), "triangle bound valid");
    for (auto vertex : triangle.vertices)
      tests::require(triangle.bound.contains(artifact.vertices()[vertex].bound),
                     "triangle bound contains vertex bound");
  }
  for (const auto &edge : artifact.edges()) {
    tests::require(edge.bound.valid(), "edge bound valid");
    for (auto vertex : edge.endpoints)
      tests::require(edge.bound.contains(artifact.vertices()[vertex].bound),
                     "edge bound contains endpoint bound");
  }
  for (const auto &group : artifact.facet_groups()) {
    tests::require(group.source_vertices ==
                       validated.facets()[group.source_facet].vertices,
                   "facet group source identity");
    tests::require(group.triangles.size() + 2 ==
                       group.source_vertices.size(),
                   "facet triangulation cardinality");
    tests::require(group.internal_edges.size() + 3 ==
                       group.source_vertices.size(),
                   "facet diagonal cardinality");
    tests::require(group.boundary_halfedges.size() ==
                       group.source_vertices.size(),
                   "facet boundary cardinality");
    for (auto triangle : group.triangles)
      tests::require(group.bound.contains(artifact.triangles()[triangle].bound),
                     "facet group bound contains triangle");
  }
  for (const auto &group : artifact.shell_groups()) {
    tests::require(group.source_shell == group.canonical_id,
                   "shell group identity");
    tests::require(group.facets ==
                       validated.shells()[group.source_shell].facets,
                   "shell group facet projection");
    const auto chi = static_cast<std::int64_t>(group.vertices.size()) -
                     static_cast<std::int64_t>(group.edges.size()) +
                     static_cast<std::int64_t>(group.triangles.size());
    tests::require(chi == 2, "closed genus-zero fixture Euler characteristic");
  }
}

template <class T, class I> void owner_suite() {
  const auto mesh = source_triangulation_tests::box<T, I>();
  auto first = tests::build<T, I>(mesh);
  auto second = tests::build<T, I>(mesh);
  tests::require(!first.artifact->owner().same_owner(second.artifact->owner()),
                 "fixtures use distinct runtime owners");
  tests::require(first.artifact->canonical_bytes() ==
                     second.artifact->canonical_bytes(),
                 "runtime owner excluded from canonical bytes");
  tests::require(first.artifact->digest() == second.artifact->digest(),
                 "runtime owner excluded from digest");
  bounded::canonical_halfedge_query_view<T, I> valid(
      *first.artifact, first.predecessor.context.owner);
  bounded::canonical_halfedge_query_view<T, I> invalid(
      *first.artifact, second.predecessor.context.owner);
  tests::require(valid.valid_owner() &&
                     valid.vertex(bounded::manifold_vertex_id(0)) != nullptr,
                 "owner-checked query succeeds");
  tests::require(!invalid.valid_owner() &&
                     invalid.vertex(bounded::manifold_vertex_id(0)) == nullptr,
                 "foreign owner query rejected");
}

template <class T, class I> void canonicalization_suite() {
  auto mesh = source_triangulation_tests::box<T, I>();
  auto permuted = mesh;
  std::rotate(permuted.faces.begin(), permuted.faces.begin() + 2,
              permuted.faces.end());
  for (auto &face : permuted.faces)
    std::rotate(face.begin(), face.begin() + 1, face.end());
  auto first = tests::build<T, I>(mesh);
  auto second = tests::build<T, I>(permuted);
  tests::require(first.artifact->source_semantic_digest() ==
                     second.artifact->source_semantic_digest(),
                 "presentation permutation preserves source semantics");
  tests::require(first.artifact->exact_topology_digest() ==
                     second.artifact->exact_topology_digest(),
                 "presentation permutation preserves exact topology");
  tests::require(first.artifact->geometry_attachment_digest() ==
                     second.artifact->geometry_attachment_digest(),
                 "presentation permutation preserves geometry attachments");
}

template <class T, class I> void codec_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::box<T, I>());
  const auto &artifact = *fixture.artifact;
  tests::require(
      bounded::canonical_halfedge_codec_header_valid(
          artifact.canonical_bytes(), artifact),
      "canonical codec header");
  auto corrupted = artifact.canonical_bytes();
  corrupted[0] ^= 0x80U;
  tests::require(!bounded::canonical_halfedge_codec_header_valid(corrupted,
                                                                  artifact),
                 "codec rejects corrupt magic");
  corrupted = artifact.canonical_bytes();
  corrupted.pop_back();
  tests::require(!bounded::canonical_halfedge_codec_header_valid(corrupted,
                                                                  artifact),
                 "codec rejects truncation");
}

template <class T, class I> void mutation_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::box<T, I>());
  const auto &artifact = *fixture.artifact;
  auto rejects = [&](auto mutated, const char *message) {
    tests::require(!bounded::verify_canonical_halfedge_operand(
                       mutated, *fixture.predecessor.operand, *fixture.source,
                       *fixture.predecessor.precision),
                   message);
  };
  {
    auto mutated = bounded::canonical_halfedge_test_access::copy(artifact);
    bounded::canonical_halfedge_test_access::halfedges(mutated)[0].pair = 0;
    rejects(std::move(mutated), "verifier rejects pair mutation");
  }
  {
    auto mutated = bounded::canonical_halfedge_test_access::copy(artifact);
    auto &edge = bounded::canonical_halfedge_test_access::edges(mutated)[0];
    edge.source_feature_owner = !edge.source_feature_owner;
    rejects(std::move(mutated), "verifier rejects edge-role mutation");
  }
  {
    auto mutated = bounded::canonical_halfedge_test_access::copy(artifact);
    bounded::canonical_halfedge_test_access::fans(mutated)[0]
        .outgoing_halfedges.clear();
    rejects(std::move(mutated), "verifier rejects fan mutation");
  }
  {
    auto mutated = bounded::canonical_halfedge_test_access::copy(artifact);
    bounded::canonical_halfedge_test_access::facet_groups(mutated)[0]
        .triangles.clear();
    rejects(std::move(mutated), "verifier rejects facet-group mutation");
  }
  {
    auto mutated = bounded::canonical_halfedge_test_access::copy(artifact);
    bounded::canonical_halfedge_test_access::vertices(mutated)[0]
        .bound.schema_version = 0;
    rejects(std::move(mutated), "verifier rejects bound mutation");
  }
}

template <class T, class I> void resources_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::box<T, I>());
  bounded::bounded_boolean_cancellation_source cancellation;
  auto token = cancellation.token();
  cancellation.request_cancel(17);
  auto cancelled_capabilities = tests::capabilities(fixture);
  cancelled_capabilities.cancellation = &token;
  auto cancelled = bounded::build_canonical_halfedge_operand(
      fixture.predecessor.operand, fixture.source,
      fixture.predecessor.context, *fixture.predecessor.precision,
      cancelled_capabilities);
  tests::require(!cancelled.has_value() &&
                     cancelled.error()->category ==
                         bounded_boolean_error_category::cancelled,
                 "pre-cancelled construction fails transactionally");

  auto policy = fixture.predecessor.context.options.resources;
  policy.persistent_bytes.advisory = 1;
  policy.persistent_bytes.hard = 1;
  bounded::resource_manager tiny(policy);
  auto limited_capabilities = tests::capabilities(fixture);
  limited_capabilities.resources = &tiny;
  auto limited = bounded::build_canonical_halfedge_operand(
      fixture.predecessor.operand, fixture.source,
      fixture.predecessor.context, *fixture.predecessor.precision,
      limited_capabilities);
  tests::require(!limited.has_value() &&
                     limited.error()->category ==
                         bounded_boolean_error_category::resource_limit,
                 "resource preflight rejects insufficient budget");
}

template <class T, class I> void structural_suite() {
  auto fixture = tests::build<T, I>(
      source_triangulation_tests::concave_prism<T, I>());
  const auto &statistics = fixture.artifact->statistics();
  tests::require(statistics.fan_transitions == statistics.halfedges,
                 "fan traversal is linear");
  tests::require(statistics.work_units <=
                     statistics.represented_vertices + statistics.triangles +
                         statistics.edges + statistics.halfedges * 2,
                 "work-unit accounting follows structural bound");
  std::uint64_t levels = 0;
  for (auto n = statistics.halfedges; n > 1; n >>= 1)
    ++levels;
  tests::require(statistics.pairing_comparisons <=
                     statistics.halfedges * levels,
                 "pairing comparison structural gate");
  tests::require(statistics.persistent_bytes != 0,
                 "persistent byte accounting published");
}

void run_suite(const std::string &suite) {
  if (suite == "unit") {
    unit_suite<double, std::uint32_t>();
  } else if (suite == "pairing") {
    pairing_suite<double, std::uint32_t>();
  } else if (suite == "fans") {
    fan_suite<double, std::uint32_t>();
  } else if (suite == "groups" || suite == "geometry") {
    groups_and_geometry_suite<double, std::uint32_t>();
  } else if (suite == "owner") {
    owner_suite<double, std::uint32_t>();
  } else if (suite == "canonical") {
    canonicalization_suite<double, std::uint32_t>();
  } else if (suite == "codec") {
    codec_suite<double, std::uint32_t>();
  } else if (suite == "mutation") {
    mutation_suite<double, std::uint32_t>();
  } else if (suite == "resources") {
    resources_suite<double, std::uint32_t>();
  } else if (suite == "profiles") {
    unit_suite<float, std::uint32_t>();
    unit_suite<float, std::uint64_t>();
    unit_suite<double, std::uint32_t>();
    unit_suite<double, std::uint64_t>();
  } else if (suite == "structural") {
    structural_suite<double, std::uint32_t>();
  } else {
    throw std::runtime_error("unknown canonical halfedge suite: " + suite);
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    if (argc == 1) {
      for (const char *suite : {"unit", "pairing", "fans", "groups",
                                "geometry", "owner", "canonical", "codec",
                                "mutation", "resources", "profiles",
                                "structural"})
        run_suite(suite);
    } else {
      run_suite(argv[1]);
    }
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << "canonical halfedge test failure: " << exception.what()
              << '\n';
    return 1;
  }
}
