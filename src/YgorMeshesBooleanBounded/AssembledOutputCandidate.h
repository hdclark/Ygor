#pragma once

#include "OutputAssemblyTypes.h"
#include "../YgorMath.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct output_assembly_artifact_test_access;
template <class T, class I> class output_assembly_builder;

// Immutable assembled output candidate published by Component 14 and consumed
// by Component 15. The public mesh is canonical and structurally round-trip
// verified; Component 15 alone promotes it to ordinary success.
template <class T, class I> class assembled_output_candidate final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  output_assembly_provider_kind provider() const noexcept { return provider_; }
  output_assembly_verification_disposition verification() const noexcept {
    return verification_;
  }
  boolean_operation operation() const noexcept { return operation_; }
  candidate_topology_status topology_status() const noexcept {
    return topology_status_;
  }
  candidate_geometry_status geometry_status() const noexcept {
    return geometry_status_;
  }
  const context_owner_token &owner() const noexcept { return owner_; }

  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &cleaned_digest() const noexcept {
    return cleaned_digest_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const output_assembly_statistics &statistics() const noexcept {
    return statistics_;
  }

  const fv_surface_mesh<T, I> &mesh() const noexcept { return mesh_; }
  const std::vector<assembly_component_record> &components() const noexcept {
    return components_;
  }
  const std::vector<coordinate_copy_record> &coordinate_copies() const noexcept {
    return coordinate_copies_;
  }
  const output_report_record &report() const noexcept { return report_; }

  // Dense reverse maps.
  const std::vector<std::uint64_t> &vertex_by_occurrence() const noexcept {
    return vertex_by_occurrence_;
  }
  const std::vector<std::uint64_t> &occurrence_by_vertex() const noexcept {
    return occurrence_by_vertex_;
  }
  const std::vector<std::uint64_t> &facet_by_triangle() const noexcept {
    return facet_by_triangle_;
  }
  const std::vector<std::uint64_t> &triangle_by_facet() const noexcept {
    return triangle_by_facet_;
  }
  const std::vector<std::uint8_t> &public_content_bytes() const noexcept {
    return public_content_bytes_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }

private:
  std::uint16_t schema_version_ = contract_versions::output_assembly_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::output_assembly_provider;
  std::uint16_t codec_version_ = contract_versions::output_assembly_codec;
  std::uint16_t verifier_version_ = contract_versions::output_assembly_verifier;
  output_assembly_provider_kind provider_ =
      output_assembly_provider_kind::direct_private_fv_surface_mesh_triangles_v1;
  output_assembly_verification_disposition verification_ =
      output_assembly_verification_disposition::not_verified;
  boolean_operation operation_ = boolean_operation::set_union;
  candidate_topology_status topology_status_ =
      candidate_topology_status::assembled_pending_independent_verification;
  candidate_geometry_status geometry_status_ =
      candidate_geometry_status::finite_and_bounded_pending_independent_verification;
  context_owner_token owner_{};

  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest cleaned_digest_{};

  fv_surface_mesh<T, I> mesh_;
  std::vector<assembly_component_record> components_;
  std::vector<coordinate_copy_record> coordinate_copies_;
  output_report_record report_{};

  std::vector<std::uint64_t> vertex_by_occurrence_;
  std::vector<std::uint64_t> occurrence_by_vertex_;
  std::vector<std::uint64_t> facet_by_triangle_;
  std::vector<std::uint64_t> triangle_by_facet_;

  std::vector<std::uint8_t> public_content_bytes_;
  output_assembly_statistics statistics_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class output_assembly_builder;
  friend struct output_assembly_artifact_test_access;
};

} // namespace ygor::mesh_boolean::bounded
