#pragma once

#include "CanonicalHalfedgeOperand.h"
#include "FloatingBits.h"
#include "PrecisionContext.h"
#include "Sha256.h"

#include <array>
#include <cstddef>
#include <optional>

namespace ygor::mesh_boolean::bounded {

namespace canonical_geometry_attachment_detail {

template <class T>
inline void encode_precision_scale(
    canonical_writer &writer, const precision_scale_descriptor<T> &scale) {
  for (const auto &axis : scale.axis) {
    writer.floating(axis.minimum);
    writer.floating(axis.maximum);
    writer.floating(axis.maximum_absolute);
    writer.floating(axis.span);
    writer.boolean(axis.all_identical);
  }
  writer.floating(scale.maximum_absolute);
  writer.floating(scale.smallest_nonzero);
  writer.floating(scale.machine_floor);
  writer.u64(scale.coordinate_count);
  writer.boolean(scale.has_values);
  writer.boolean(scale.has_positive_zero);
  writer.boolean(scale.has_negative_zero);
  writer.boolean(scale.has_subnormal);
  writer.boolean(scale.has_normal);
  writer.boolean(scale.mixed_magnitude);
  writer.boolean(scale.large_translation);
  writer.u16(static_cast<std::uint16_t>(scale.normalization_exponent));
}

} // namespace canonical_geometry_attachment_detail

template <class T>
inline bounded_boolean_digest canonical_precision_attachment_digest(
    const precision_context<T> &precision) {
  canonical_writer writer;
  writer.u32(0x35504743U); // CGP5
  writer.u16(contract_versions::canonical_halfedge_geometry_attachment_schema);
  writer.u16(precision.schema_version());
  writer.u16(precision.provider_version());
  writer.u16(precision.scalar_profile_version());
  writer.u16(precision.arithmetic_profile_version());
  writer.u8(precision.scalar_bytes());
  writer.u8(precision.index_bytes());
  writer.floating(precision.tolerance());
  writer.floating(precision.declared_input_precision_a());
  writer.floating(precision.declared_input_precision_b());
  writer.floating(precision.effective_input_precision_a());
  writer.floating(precision.effective_input_precision_b());
  writer.floating(precision.required_machine_floor());
  writer.boolean(precision.ordinary_success_eligible());
  canonical_geometry_attachment_detail::encode_precision_scale(
      writer, precision.operand_a_scale());
  canonical_geometry_attachment_detail::encode_precision_scale(
      writer, precision.operand_b_scale());
  canonical_geometry_attachment_detail::encode_precision_scale(
      writer, precision.global_scale());
  return sha256::digest(writer.bytes());
}

template <class T>
inline std::optional<canonical_bound3<T>> canonical_vertex_bound(
    const source_triangle_vertex_ref<T> &source) noexcept {
  canonical_bound3<T> out;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto interval = finite_interval<T>::create(source.lower[axis], source.upper[axis]);
    if (!interval || !interval->contains(from_bits<T>(source.nominal_bits[axis])))
      return std::nullopt;
    out.axes[axis] = *interval;
  }
  return out.valid() ? std::optional<canonical_bound3<T>>(out) : std::nullopt;
}

template <class T>
inline canonical_bound3<T> canonical_edge_bound(
    const canonical_manifold_vertex_record<T> &a,
    const canonical_manifold_vertex_record<T> &b) {
  return canonical_bound_hull(a.bound, b.bound);
}

template <class T>
inline canonical_bound3<T> canonical_triangle_bound(
    const canonical_manifold_vertex_record<T> &a,
    const canonical_manifold_vertex_record<T> &b,
    const canonical_manifold_vertex_record<T> &c) {
  return canonical_bound_hull(canonical_bound_hull(a.bound, b.bound), c.bound);
}

} // namespace ygor::mesh_boolean::bounded
