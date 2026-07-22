from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    if old in text:
        target.write_text(text.replace(old, new, 1))
        return
    if new in text:
        return
    raise SystemExit(f"{path}: deterministic replacement not found")


path = Path("src/YgorMeshesBooleanBounded/CanonicalGeometryAttachments.h")
text = path.read_text()
old = '''namespace ygor::mesh_boolean::bounded {\n\n'''
new = '''namespace ygor::mesh_boolean::bounded {\n\nnamespace canonical_geometry_attachment_detail {\n\ntemplate <class T>\ninline void encode_precision_scale(\n    canonical_writer &writer, const precision_scale_descriptor<T> &scale) {\n  for (const auto &axis : scale.axis) {\n    writer.floating(axis.minimum);\n    writer.floating(axis.maximum);\n    writer.floating(axis.maximum_absolute);\n    writer.floating(axis.span);\n    writer.boolean(axis.all_identical);\n  }\n  writer.floating(scale.maximum_absolute);\n  writer.floating(scale.smallest_nonzero);\n  writer.floating(scale.machine_floor);\n  writer.u64(scale.coordinate_count);\n  writer.boolean(scale.has_values);\n  writer.boolean(scale.has_positive_zero);\n  writer.boolean(scale.has_negative_zero);\n  writer.boolean(scale.has_subnormal);\n  writer.boolean(scale.has_normal);\n  writer.boolean(scale.mixed_magnitude);\n  writer.boolean(scale.large_translation);\n  writer.u16(static_cast<std::uint16_t>(scale.normalization_exponent));\n}\n\n} // namespace canonical_geometry_attachment_detail\n\ntemplate <class T>\ninline bounded_boolean_digest canonical_precision_attachment_digest(\n    const precision_context<T> &precision) {\n  canonical_writer writer;\n  writer.u32(0x35504743U); // CGP5\n  writer.u16(contract_versions::canonical_halfedge_geometry_attachment_schema);\n  writer.u16(precision.schema_version());\n  writer.u16(precision.provider_version());\n  writer.u16(precision.scalar_profile_version());\n  writer.u16(precision.arithmetic_profile_version());\n  writer.u8(precision.scalar_bytes());\n  writer.u8(precision.index_bytes());\n  writer.floating(precision.tolerance());\n  writer.floating(precision.declared_input_precision_a());\n  writer.floating(precision.declared_input_precision_b());\n  writer.floating(precision.effective_input_precision_a());\n  writer.floating(precision.effective_input_precision_b());\n  writer.floating(precision.required_machine_floor());\n  writer.boolean(precision.ordinary_success_eligible());\n  canonical_geometry_attachment_detail::encode_precision_scale(\n      writer, precision.operand_a_scale());\n  canonical_geometry_attachment_detail::encode_precision_scale(\n      writer, precision.operand_b_scale());\n  canonical_geometry_attachment_detail::encode_precision_scale(\n      writer, precision.global_scale());\n  return sha256::digest(writer.bytes());\n}\n\n'''
if "canonical_precision_attachment_digest" not in text:
    if old not in text:
        raise SystemExit("CanonicalGeometryAttachments namespace anchor not found")
    text = text.replace(old, new, 1)
path.write_text(text)

replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeOperand.h",
    '''  const bounded_boolean_digest &geometry_attachment_digest() const noexcept {\n    return geometry_attachment_digest_;\n  }\n''',
    '''  const bounded_boolean_digest &geometry_attachment_digest() const noexcept {\n    return geometry_attachment_digest_;\n  }\n  const bounded_boolean_digest &precision_attachment_digest() const noexcept {\n    return precision_attachment_digest_;\n  }\n''',
)
replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeOperand.h",
    '''  bounded_boolean_digest precision_digest_{};\n''',
    '''  bounded_boolean_digest precision_digest_{};\n  bounded_boolean_digest precision_attachment_digest_{};\n''',
)

replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeBuildCore.h",
    '''    if (!source_record.incident_triangles.empty()) {\n      const auto triangle_id = source_record.incident_triangles.front();\n      if (triangle_id >= source_->triangles().size())\n        return fail(canonical_halfedge_subcode::malformed_reference,\n                    bounded_boolean_error_category::internal_invariant_error,\n                    "source vertex incident triangle is invalid",\n                    canonical_halfedge_checkpoint::geometry_attachments);\n      vertex.geometry_basis = source_->triangles()[triangle_id].basis.kind;\n    }\n''',
    '''    bool have_geometry_basis = false;\n    for (const auto triangle_id : source_record.incident_triangles) {\n      if (triangle_id >= source_->triangles().size())\n        return fail(canonical_halfedge_subcode::malformed_reference,\n                    bounded_boolean_error_category::internal_invariant_error,\n                    "source vertex incident triangle is invalid",\n                    canonical_halfedge_checkpoint::geometry_attachments);\n      const auto kind = source_->triangles()[triangle_id].basis.kind;\n      if (have_geometry_basis && vertex.geometry_basis != kind)\n        return fail(canonical_halfedge_subcode::geometry_basis_mismatch,\n                    bounded_boolean_error_category::internal_invariant_error,\n                    "source vertex incident geometry bases disagree",\n                    canonical_halfedge_checkpoint::geometry_attachments);\n      vertex.geometry_basis = kind;\n      have_geometry_basis = true;\n    }\n''',
)

replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeBuildFinalize.h",
    '''    artifact_->precision_digest_ = precision_.digest();\n''',
    '''    artifact_->precision_digest_ = precision_.digest();\n    artifact_->precision_attachment_digest_ =\n        canonical_precision_attachment_digest(precision_);\n''',
)

replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeCodec.h",
    '''  write_digest(writer, artifact.precision_digest_);\n''',
    '''  write_digest(writer, artifact.precision_attachment_digest_);\n''',
)
replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeCodec.h",
    '''    write_digest(writer, triangle.basis.precision_digest);\n''',
    '''''',
)

replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeVerifier.h",
    '''       artifact.precision_digest_ != precision.digest() ||\n       source.predecessor_digest() != validated.digest() ||\n''',
    '''       artifact.precision_digest_ != precision.digest() ||\n       artifact.precision_attachment_digest_ !=\n           canonical_precision_attachment_digest(precision) ||\n       source.predecessor_digest() != validated.digest() ||\n''',
)
replace_once(
    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeVerifier.h",
    '''    const auto &predecessor = source.triangles()[triangle.source_triangle];\n    const auto expected_rotation =\n''',
    '''    const auto &predecessor = source.triangles()[triangle.source_triangle];\n    if (triangle.basis.kind != predecessor.basis.kind ||\n        triangle.basis.operand != predecessor.basis.operand ||\n        triangle.basis.facet != predecessor.basis.facet ||\n        triangle.basis.ring != predecessor.basis.ring ||\n        triangle.basis.shell != predecessor.basis.shell ||\n        triangle.basis.dropped_axis != predecessor.basis.dropped_axis ||\n        triangle.basis.support_vertices != predecessor.basis.support_vertices ||\n        triangle.basis.predecessor_digest != predecessor.basis.predecessor_digest ||\n        triangle.basis.precision_digest != precision.digest() ||\n        triangle.basis.basis_digest != predecessor.basis.basis_digest)\n      return fail(canonical_halfedge_subcode::geometry_basis_mismatch);\n    const auto expected_rotation =\n''',
)

Path(".github/component05_determinism_fix.py").unlink()
