#pragma once

#include "PrecisionContext.h"

namespace ygor::mesh_boolean::bounded {

// Component 07 consumes the numerical semantics of Component 03, not its
// invocation-profile replay identity. This projection deliberately excludes
// the precision preflight and invocation replay digests while retaining every
// scalar, scale, source, and Boolean-context value that can affect relation
// arithmetic.
template <class T>
bounded_boolean_digest relation_precision_semantic_digest(
    const precision_context<T> &precision) {
  canonical_writer writer;
  writer.u32(0x50375259U); // YR7P
  writer.u16(contract_versions::relation_precision_projection);
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
  precision_detail::encode_scale(writer, precision.operand_a_scale());
  precision_detail::encode_scale(writer, precision.operand_b_scale());
  precision_detail::encode_scale(writer, precision.global_scale());
  for (const auto byte : precision.source_digest().bytes)
    writer.u8(byte);
  for (const auto byte : precision.boolean_context_digest().bytes)
    writer.u8(byte);
  return sha256::digest(writer.bytes());
}

} // namespace ygor::mesh_boolean::bounded
