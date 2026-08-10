#pragma once

#include "CanonicalCandidateStream.h"

#include <cstdint>
#include <memory>

namespace ygor::mesh_boolean::bounded {

template <class T, class I> class candidate_stream_view final {
public:
  candidate_stream_view(
      std::shared_ptr<const canonical_candidate_stream<T, I>> artifact,
      context_owner_token owner) noexcept
      : artifact_(std::move(artifact)), owner_(std::move(owner)) {}

  bool valid() const noexcept {
    return artifact_ && owner_.anchor && artifact_->owner().same_owner(owner_) &&
           artifact_->verification() ==
               broad_phase_verification_disposition::independently_verified;
  }

  std::uint64_t size() const noexcept {
    return valid() ? static_cast<std::uint64_t>(artifact_->candidates().size()) : 0;
  }

  const canonical_candidate_record<T> *candidate(
      std::uint64_t ordinal) const noexcept {
    return valid() && ordinal < artifact_->candidates().size()
               ? &artifact_->candidates()[ordinal]
               : nullptr;
  }

  const broad_phase_overlap_witness<T> *witness(
      overlap_witness_id id) const noexcept {
    return valid() && id.ordinal() < artifact_->witnesses().size()
               ? &artifact_->witnesses()[id.ordinal()]
               : nullptr;
  }

  const canonical_candidate_partition *partition(
      candidate_partition_id id) const noexcept {
    return valid() && id.ordinal() < artifact_->partitions().size()
               ? &artifact_->partitions()[id.ordinal()]
               : nullptr;
  }

  const broad_phase_edge_primitive<T> *edge(
      const canonical_candidate_record<T> &candidate) const noexcept {
    if (!valid() || candidate.role != candidate.key.role)
      return nullptr;
    const auto &table = artifact_->primitive_table(
        role_edge_operand(candidate.role));
    return candidate.edge.ordinal() < table.edges.size()
               ? &table.edges[candidate.edge.ordinal()]
               : nullptr;
  }

  const broad_phase_triangle_primitive<T> *triangle(
      const canonical_candidate_record<T> &candidate) const noexcept {
    if (!valid() || candidate.role != candidate.key.role)
      return nullptr;
    const auto &table = artifact_->primitive_table(
        role_triangle_operand(candidate.role));
    return candidate.triangle.ordinal() < table.triangles.size()
               ? &table.triangles[candidate.triangle.ordinal()]
               : nullptr;
  }

  const bounded_boolean_digest *digest() const noexcept {
    return valid() ? &artifact_->digest() : nullptr;
  }

  const std::vector<std::uint8_t> *canonical_bytes() const noexcept {
    return valid() ? &artifact_->canonical_bytes() : nullptr;
  }

private:
  std::shared_ptr<const canonical_candidate_stream<T, I>> artifact_;
  context_owner_token owner_{};
};

} // namespace ygor::mesh_boolean::bounded
