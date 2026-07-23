#pragma once

#include "BroadPhaseTraverse.h"
#include "CanonicalSourceManifolds.h"
#include "Outcome.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct broad_phase_test_access;
struct broad_phase_codec_access;
struct broad_phase_verifier_access;

template <class T, class I> class canonical_candidate_stream;
template <class T, class I>
std::vector<std::uint8_t> encode_canonical_candidate_stream(
    const canonical_candidate_stream<T, I> &artifact);
template <class T, class I>
bool verify_broad_phase_codec(const canonical_candidate_stream<T, I> &artifact,
                              bounded_boolean_error &error);

template <class T> struct canonical_candidate_record final {
  candidate_id id{0};
  std::uint64_t ordinal = 0;
  canonical_candidate_key key{};
  directed_candidate_role role = directed_candidate_role::a_edge_b_triangle;
  broad_phase_relation_family family =
      broad_phase_relation_family::canonical_edge_source_triangle;
  broad_phase_edge_primitive_id edge{0};
  broad_phase_triangle_primitive_id triangle{0};
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  overlap_witness_id witness{0};
  topological_filter_reason filter_reason =
      topological_filter_reason::not_filtered;
  std::uint16_t domain_policy_version = 1;
  std::uint16_t provider_version = 1;
  std::uint32_t reserved = 0;
};

struct canonical_candidate_partition final {
  candidate_partition_id id{0};
  std::uint64_t ordinal = 0;
  std::uint64_t begin = 0;
  std::uint64_t count = 0;
  std::uint64_t maximum_records = broad_phase_partition_capacity_v1;
  std::uint32_t reserved = 0;
};

struct broad_phase_verification_evidence final {
  broad_phase_verifier_evidence_id id{0};
  std::uint16_t verifier_version = 1;
  bool primitive_reconstruction_complete = false;
  bool rank_reconstruction_complete = false;
  bool hierarchy_reconstruction_complete = false;
  bool breadth_first_candidate_set_complete = false;
  bool exhaustive_all_pairs_performed = false;
  bool exhaustive_all_pairs_complete = false;
  std::uint64_t breadth_first_candidate_count = 0;
  std::uint64_t exhaustive_candidate_count = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t maximum_queue = 0;
  bounded_boolean_digest candidate_set_digest{};
  std::uint32_t reserved = 0;
};

template <class T, class I> class canonical_candidate_stream final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t domain_policy_version() const noexcept {
    return domain_policy_version_;
  }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  broad_phase_provider_kind provider() const noexcept { return provider_; }
  candidate_domain_kind candidate_domain() const noexcept {
    return candidate_domain_;
  }
  broad_phase_verification_disposition verification() const noexcept {
    return verification_;
  }
  const context_owner_token &owner() const noexcept { return owner_; }
  const std::shared_ptr<const canonical_source_manifolds<T, I>> &manifolds()
      const noexcept {
    return manifolds_;
  }
  const broad_phase_primitive_table<T> &primitive_table(
      operand_id operand) const noexcept {
    return primitive_tables_[operand_slot(operand)];
  }
  const triangle_aabb_hierarchy<T> &hierarchy(
      operand_id operand) const noexcept {
    return hierarchies_[operand_slot(operand)];
  }
  const std::vector<broad_phase_count_plan> &count_plans() const noexcept {
    return count_plans_;
  }
  const std::vector<broad_phase_overlap_witness<T>> &witnesses() const noexcept {
    return witnesses_;
  }
  const std::vector<canonical_candidate_record<T>> &candidates() const noexcept {
    return candidates_;
  }
  const std::vector<canonical_candidate_partition> &partitions() const noexcept {
    return partitions_;
  }
  const broad_phase_statistics &statistics() const noexcept {
    return statistics_;
  }
  const broad_phase_verification_evidence &verification_evidence() const noexcept {
    return verification_evidence_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &predecessor_digest() const noexcept {
    return predecessor_digest_;
  }
  const bounded_boolean_digest &primitive_tables_digest() const noexcept {
    return primitive_tables_digest_;
  }
  const std::array<bounded_boolean_digest, 2> &hierarchy_digests() const noexcept {
    return hierarchy_digests_;
  }
  const bounded_boolean_digest &candidate_digest() const noexcept {
    return candidate_digest_;
  }
  const bounded_boolean_digest &partition_digest() const noexcept {
    return partition_digest_;
  }
  const bounded_boolean_digest &evidence_digest() const noexcept {
    return evidence_digest_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }

  const canonical_candidate_record<T> *candidate(
      candidate_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= candidates_.size())
      return nullptr;
    return &candidates_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ = 1;
  std::uint16_t provider_version_ = 1;
  std::uint16_t domain_policy_version_ = 1;
  std::uint16_t codec_version_ = 1;
  std::uint16_t verifier_version_ = 1;
  broad_phase_provider_kind provider_ =
      broad_phase_provider_kind::rank_morton_triangle_aabb_hierarchy_v1;
  candidate_domain_kind candidate_domain_ =
      candidate_domain_kind::all_canonical_edges_against_all_opposite_source_triangles_v1;
  broad_phase_verification_disposition verification_ =
      broad_phase_verification_disposition::unverified;
  context_owner_token owner_{};
  std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds_;
  std::array<broad_phase_primitive_table<T>, 2> primitive_tables_{};
  std::array<triangle_aabb_hierarchy<T>, 2> hierarchies_{};
  std::vector<broad_phase_count_plan> count_plans_;
  std::vector<broad_phase_overlap_witness<T>> witnesses_;
  std::vector<canonical_candidate_record<T>> candidates_;
  std::vector<canonical_candidate_partition> partitions_;
  broad_phase_statistics statistics_{};
  broad_phase_verification_evidence verification_evidence_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest predecessor_digest_{};
  bounded_boolean_digest primitive_tables_digest_{};
  std::array<bounded_boolean_digest, 2> hierarchy_digests_{};
  bounded_boolean_digest candidate_digest_{};
  bounded_boolean_digest partition_digest_{};
  bounded_boolean_digest evidence_digest_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class broad_phase_builder;
  template <class U, class J>
  friend std::vector<std::uint8_t> encode_canonical_candidate_stream(
      const canonical_candidate_stream<U, J> &artifact);
  template <class U, class J>
  friend bool verify_broad_phase_codec(
      const canonical_candidate_stream<U, J> &artifact,
      bounded_boolean_error &error);
  friend struct broad_phase_test_access;
  friend struct broad_phase_codec_access;
  friend struct broad_phase_verifier_access;
};

} // namespace ygor::mesh_boolean::bounded
