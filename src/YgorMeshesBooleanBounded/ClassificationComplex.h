#pragma once

#include "ClassificationTypes.h"
#include "CanonicalHalfedgeOperand.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct classification_artifact_test_access;
template <class T, class I> class classification_builder;

// A bounded witness point carried inside the immutable artifact as bit patterns
// so the artifact stays free of raw floating payloads and padding.
struct classification_witness final {
  std::array<std::uint64_t, 3> nominal_bits{};
  std::array<std::uint64_t, 6> enclosure_bits{}; // per-axis [lower, upper]
};

template <class T> struct classification_atom_record final {
  std::uint64_t canonical_id = 0;
  classification_atom_key key{};
  operand_id operand = operand_id::a;
  std::uint64_t shell = 0;
  std::uint64_t source_facet = 0;
  std::vector<std::uint64_t> source_triangles;
  bool positive_area = true;
  classification_witness witness{};
  std::uint64_t group = classification_invalid_ordinal;
  std::uint64_t side_label = classification_invalid_ordinal;
};

struct classification_sector_record final {
  std::uint64_t canonical_id = 0;
  operand_id operand = operand_id::a;
  std::uint64_t source_feature = 0;
  boundary_contact_state contact = boundary_contact_state::none;
  std::vector<std::uint64_t> incident_atoms;
  bool occurrence_separated = false;
};

struct classification_occurrence_record final {
  std::uint64_t canonical_id = 0;
  operand_id operand = operand_id::a;
  std::uint64_t event_ordinal = classification_invalid_ordinal;
  std::uint64_t descriptor_lineage = 0;
  bool topology_separate = false;
  std::vector<std::uint64_t> incident_sectors;
};

struct classification_adjacency_record final {
  std::uint64_t canonical_id = 0;
  classification_adjacency_key key{};
  std::uint64_t source_atom = 0;
  std::uint64_t destination_atom = 0;
  std::uint64_t reverse = classification_invalid_ordinal;
  std::uint64_t descriptor_lineage = 0;
};

struct classification_group_record final {
  std::uint64_t canonical_id = 0;
  std::vector<std::uint64_t> members;
  std::uint64_t propagation_component = classification_invalid_ordinal;
  std::int64_t total_winding = 0;
  std::vector<std::pair<std::uint64_t, std::int64_t>> shell_winding;
};

struct quotient_edge_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t source_group = 0;
  std::uint64_t destination_group = 0;
  std::uint64_t reverse = classification_invalid_ordinal;
  std::int32_t total_delta = 0;
  std::vector<std::pair<std::uint64_t, std::int32_t>> shell_deltas;
  std::vector<std::uint64_t> member_adjacency;
  propagation_edge_role role = propagation_edge_role::numeric_constraint;
};

struct propagation_component_record final {
  std::uint64_t canonical_id = 0;
  std::vector<std::uint64_t> groups;
  std::uint64_t anchor_group = classification_invalid_ordinal;
  seed_source_kind anchor_source = seed_source_kind::empty_opposite;
};

struct seed_attempt_record final {
  std::uint64_t canonical_id = 0;
  std::uint32_t direction_index = 0;
  seed_attempt_disposition disposition =
      seed_attempt_disposition::successful;
};

struct seed_query_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t component = 0;
  std::uint64_t group = 0;
  std::uint64_t atom = 0;
  seed_source_kind source_kind = seed_source_kind::bounded_shell_query;
  std::int64_t total_winding = 0;
  std::vector<std::pair<std::uint64_t, std::int64_t>> shell_winding;
  std::vector<seed_attempt_record> attempts;
};

struct atom_side_label_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t atom = 0;
  occupancy_state negative_side = occupancy_state::invalid;
  occupancy_state positive_side = occupancy_state::invalid;
  side_occupancy_origin negative_origin = side_occupancy_origin::numeric;
  side_occupancy_origin positive_origin = side_occupancy_origin::numeric;
  boundary_contact_state contact = boundary_contact_state::none;
  bool occurrence_separated = false;
};

template <class T, class I> class classification_complex final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  classification_provider_kind provider() const noexcept { return provider_; }
  classification_verification_disposition verification() const noexcept {
    return verification_;
  }
  boolean_operation operation() const noexcept { return operation_; }
  const context_owner_token &owner() const noexcept { return owner_; }

  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &relation_digest() const noexcept {
    return relation_digest_;
  }
  const bounded_boolean_digest &intersection_digest() const noexcept {
    return intersection_digest_;
  }
  const std::array<bounded_boolean_digest, 2> &manifold_digests() const noexcept {
    return manifold_digests_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const classification_statistics &statistics() const noexcept {
    return statistics_;
  }

  const std::vector<classification_atom_record<T>> &atoms() const noexcept {
    return atoms_;
  }
  const std::vector<classification_sector_record> &sectors() const noexcept {
    return sectors_;
  }
  const std::vector<classification_occurrence_record> &occurrences()
      const noexcept {
    return occurrences_;
  }
  const std::vector<classification_adjacency_record> &adjacency() const noexcept {
    return adjacency_;
  }
  const std::vector<classification_group_record> &groups() const noexcept {
    return groups_;
  }
  const std::vector<quotient_edge_record> &quotient_edges() const noexcept {
    return quotient_edges_;
  }
  const std::vector<propagation_component_record> &propagation_components()
      const noexcept {
    return propagation_components_;
  }
  const std::vector<seed_query_record> &seed_queries() const noexcept {
    return seed_queries_;
  }
  const std::vector<atom_side_label_record> &side_labels() const noexcept {
    return side_labels_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }

private:
  std::uint16_t schema_version_ = contract_versions::classification_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::classification_provider;
  std::uint16_t codec_version_ = contract_versions::classification_codec;
  std::uint16_t verifier_version_ = contract_versions::classification_verifier;
  classification_provider_kind provider_ =
      classification_provider_kind::lineage_surface_arrangement_v1;
  classification_verification_disposition verification_ =
      classification_verification_disposition::not_verified;
  boolean_operation operation_ = boolean_operation::set_union;
  context_owner_token owner_{};

  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest relation_digest_{};
  bounded_boolean_digest intersection_digest_{};
  std::array<bounded_boolean_digest, 2> manifold_digests_{};

  std::vector<classification_atom_record<T>> atoms_;
  std::vector<classification_sector_record> sectors_;
  std::vector<classification_occurrence_record> occurrences_;
  std::vector<classification_adjacency_record> adjacency_;
  std::vector<classification_group_record> groups_;
  std::vector<quotient_edge_record> quotient_edges_;
  std::vector<propagation_component_record> propagation_components_;
  std::vector<seed_query_record> seed_queries_;
  std::vector<atom_side_label_record> side_labels_;
  classification_statistics statistics_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class classification_builder;
  friend struct classification_artifact_test_access;
};

} // namespace ygor::mesh_boolean::bounded
