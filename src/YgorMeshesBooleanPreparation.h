#pragma once
#ifndef YGOR_MESHES_BOOLEAN_PREPARATION_H_
#define YGOR_MESHES_BOOLEAN_PREPARATION_H_

#include "YgorMeshesBooleanContract.h"

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t prepared_operand_schema = 1;

enum class preparation_validation_subcode : std::uint32_t {
  malformed_record = 1,
  stale_input_digest = 2,
  stale_policy_digest = 3,
  stale_report_digest = 4,
  stale_certificate = 5,
  type_mismatch = 6,
  semantic_mismatch = 7
};

struct strict_validation_policy {
  std::uint16_t schema = prepared_operand_schema;
  verification_level verification = verification_level::mandatory;
  bool remove_unused_storage = false;
};

struct strict_validation_certificate {
  std::uint16_t schema = prepared_operand_schema;
  std::uint16_t checker_version = 1;
  coordinate_tag coordinate = coordinate_tag::binary32;
  index_tag index = index_tag::uint32;
  digest input_digest;
  digest prepared_digest;
  digest validation_options_digest;
  digest policy_digest;
  digest semantic_digest;
  digest validation_artifact_digest;
  digest validation_report_digest;
  digest invariant_set_digest;
  digest kernel_policy_digest;
  bool geometry_changed = false;
  digest certificate_digest;
};

struct prepared_operand_decode_limits {
  std::uint64_t max_record_bytes = 256ULL * 1024ULL * 1024ULL;
  std::uint64_t max_vertices = 10000000;
  std::uint64_t max_faces = 10000000;
  std::uint64_t max_face_indices = 100000000;
};

template <class T, class I> class prepared_operand {
  struct state {
    fv_surface_mesh<T, I> mesh;
    strict_validation_policy policy;
    strict_validation_certificate certificate;
  };
  std::shared_ptr<const state> state_;

  explicit prepared_operand(std::shared_ptr<const state> state)
      : state_(std::move(state)) {}
  template <class U, class J>
  friend status_or<prepared_operand<U, J>> validate_operand_strict(
      const fv_surface_mesh<U, J> &, const strict_validation_policy &,
      const boolean_options &,
      std::shared_ptr<const exact_kernel_services<U>>,
      std::shared_ptr<const verifier_service>, cancellation_source *);
  template <class U, class J>
  friend status_or<prepared_operand<U, J>> decode_prepared_operand(
      const std::vector<std::uint8_t> &, const prepared_operand_decode_limits &);

public:
  prepared_operand() = delete;
  const fv_surface_mesh<T, I> &mesh() const noexcept { return state_->mesh; }
  const strict_validation_policy &policy() const noexcept {
    return state_->policy;
  }
  const strict_validation_certificate &certificate() const noexcept {
    return state_->certificate;
  }
  std::shared_ptr<const fv_surface_mesh<T, I>> shared_mesh() const {
    return std::shared_ptr<const fv_surface_mesh<T, I>>(state_, &state_->mesh);
  }
};

template <class T, class I> struct strict_validation_result {
  prepared_operand<T, I> operand;
  strict_validation_certificate certificate;
};

template <class T, class I>
status_or<prepared_operand<T, I>> validate_operand_strict(
    const fv_surface_mesh<T, I> &, const strict_validation_policy &,
    const boolean_options &, std::shared_ptr<const exact_kernel_services<T>>,
    std::shared_ptr<const verifier_service>, cancellation_source * = nullptr);

template <class T, class I>
status_or<std::vector<std::uint8_t>>
encode_prepared_operand(const prepared_operand<T, I> &);

template <class T, class I>
status_or<prepared_operand<T, I>> decode_prepared_operand(
    const std::vector<std::uint8_t> &,
    const prepared_operand_decode_limits & = {});

template <class T, class I>
status_or<bool> verify_prepared_operand(
    const prepared_operand<T, I> &, const boolean_options &,
    std::shared_ptr<const exact_kernel_services<T>>,
    std::shared_ptr<const verifier_service>, cancellation_source * = nullptr);

template <class T, class I>
status_or<std::unique_ptr<boolean_context<T, I>>> make_boolean_context(
    const prepared_operand<T, I> &, const prepared_operand<T, I> &, operation,
    const boolean_options &, std::shared_ptr<const exact_kernel_services<T>>,
    std::shared_ptr<const verifier_service>, cancellation_source * = nullptr,
    diagnostic_consumer = {});

extern template class prepared_operand<float, std::uint32_t>;
extern template class prepared_operand<float, std::uint64_t>;
extern template class prepared_operand<double, std::uint32_t>;
extern template class prepared_operand<double, std::uint64_t>;

} // namespace mesh_boolean
} // namespace ygor
#endif
