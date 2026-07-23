#pragma once

#include "YgorMeshesBooleanBounded/BroadPhaseBuild.h"
#include "YgorMeshesBooleanBounded/BroadPhaseQueries.h"
#include "YgorMeshesBooleanBounded/CanonicalHalfedgeBuild.h"
#include "YgorMeshesBooleanBounded/InputValidation.h"
#include "YgorMeshesBooleanBounded/PrecisionBootstrap.h"
#include "YgorMeshesBooleanBounded/SourceTriangulation.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct broad_phase_test_access final {
  template <class T, class I>
  static canonical_candidate_stream<T, I>
  copy(const canonical_candidate_stream<T, I> &artifact) {
    return artifact;
  }
  template <class T, class I>
  static auto &candidates(canonical_candidate_stream<T, I> &artifact) {
    return artifact.candidates_;
  }
  template <class T, class I>
  static auto &witnesses(canonical_candidate_stream<T, I> &artifact) {
    return artifact.witnesses_;
  }
  template <class T, class I>
  static auto &hierarchy(canonical_candidate_stream<T, I> &artifact,
                         operand_id operand) {
    return artifact.hierarchies_[operand_slot(operand)];
  }
  template <class T, class I>
  static auto &primitive_table(canonical_candidate_stream<T, I> &artifact,
                               operand_id operand) {
    return artifact.primitive_tables_[operand_slot(operand)];
  }
  template <class T, class I>
  static auto &canonical_bytes(canonical_candidate_stream<T, I> &artifact) {
    return artifact.canonical_bytes_;
  }
  template <class T, class I>
  static auto &statistics(canonical_candidate_stream<T, I> &artifact) {
    return artifact.statistics_;
  }
  template <class T, class I>
  static void set_owner(canonical_candidate_stream<T, I> &artifact,
                        context_owner_token owner) {
    artifact.owner_ = std::move(owner);
  }
};

} // namespace ygor::mesh_boolean::bounded

namespace broad_phase_tests {
namespace bounded = ygor::mesh_boolean::bounded;
using scalar = double;
using index_type = std::uint32_t;
using mesh_type = fv_surface_mesh<scalar, index_type>;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

mesh_type box(scalar x0 = 0, scalar y0 = 0, scalar z0 = 0,
              scalar x1 = 1, scalar y1 = 1, scalar z1 = 1);
mesh_type permuted_box(scalar x0 = 0, scalar y0 = 0, scalar z0 = 0,
                       scalar x1 = 1, scalar y1 = 1, scalar z1 = 1);

struct predecessor_fixture final {
  bounded::boolean_context<scalar, index_type> context;
  std::shared_ptr<const bounded::precision_context<scalar>> precision;
  std::shared_ptr<const bounded::canonical_source_manifolds<scalar, index_type>>
      manifolds;
  std::unique_ptr<bounded::resource_manager> resources;
};

struct built_fixture final {
  predecessor_fixture predecessor;
  std::shared_ptr<const bounded::canonical_candidate_stream<scalar, index_type>>
      artifact;
};

predecessor_fixture build_predecessors(
    const mesh_type &a, const mesh_type &b,
    bounded::source_triangulation_provider_kind provider =
        bounded::source_triangulation_provider_kind::indexed_dependency_v1,
    bool compare_reference = true);

built_fixture build(
    const mesh_type &a, const mesh_type &b,
    bounded::source_triangulation_provider_kind provider =
        bounded::source_triangulation_provider_kind::indexed_dependency_v1,
    bool compare_reference = true);

bounded::broad_phase_capabilities capabilities(predecessor_fixture &fixture);
std::string diagnostic(const bounded_boolean_error &error);

void test_domain();
void test_rank_morton();
void test_hierarchy();
void test_overlap();
void test_traversal();
void test_known_candidates();
void test_all_pairs_oracle();
void test_provenance();
void test_canonicalization();
void test_codec_replay();
void test_alternative_triangulation();
void test_mutation();
void test_properties();
void test_adversarial();
void test_resources_cancellation();
void test_structural_performance();

} // namespace broad_phase_tests
