#pragma once

#include "FinalVerificationTypes.h"
#include "AssembledOutputCandidate.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct final_verification_artifact_test_access;
template <class T, class I> class final_verification_builder;

// Immutable verified Boolean result published by Component 15. It holds the
// unchanged Component 14 public mesh and all final evidence; a
// bounded_boolean_success may be constructed from it exactly once.
template <class T, class I> class verified_boolean_result final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  boolean_operation operation() const noexcept { return operation_; }
  const context_owner_token &owner() const noexcept { return owner_; }

  const fv_surface_mesh<T, I> &mesh() const noexcept { return mesh_; }
  const topology_report_record &topology_report() const noexcept {
    return topology_report_;
  }
  const geometry_report_record &geometry_report() const noexcept {
    return geometry_report_;
  }
  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &candidate_digest() const noexcept {
    return candidate_digest_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const final_verification_statistics &statistics() const noexcept {
    return statistics_;
  }

private:
  std::uint16_t schema_version_ =
      contract_versions::final_verification_artifact_schema;
  std::uint16_t provider_version_ =
      contract_versions::final_verification_provider;
  std::uint16_t codec_version_ = contract_versions::final_verification_codec;
  std::uint16_t verifier_version_ =
      contract_versions::final_verification_verifier;
  boolean_operation operation_ = boolean_operation::set_union;
  context_owner_token owner_{};

  fv_surface_mesh<T, I> mesh_;
  topology_report_record topology_report_{};
  geometry_report_record geometry_report_{};
  std::vector<check_evidence_record> check_evidence_;
  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest candidate_digest_{};
  final_verification_statistics statistics_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class final_verification_builder;
  friend struct final_verification_artifact_test_access;
};

} // namespace ygor::mesh_boolean::bounded
