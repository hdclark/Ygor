#pragma once

#include "SourceEdgeRelationDetail.h"

namespace ygor::mesh_boolean::bounded {

template <class T>
std::vector<std::uint8_t>
encode_source_edge_relation_semantics(
    const source_edge_relation_record<T> &record) {
  canonical_writer writer;
  writer.u32(0x37454552U); // REE7
  writer.u16(record.schema_version);
  writer.u16(record.policy_version);
  encode_relation_feature_key(writer, record.first_feature);
  encode_relation_feature_key(writer, record.second_feature);
  source_edge_relation_detail::encode_snapshot(writer, record.first_start);
  source_edge_relation_detail::encode_snapshot(writer, record.first_end);
  source_edge_relation_detail::encode_snapshot(writer, record.second_start);
  source_edge_relation_detail::encode_snapshot(writer, record.second_end);
  writer.u8(static_cast<std::uint8_t>(record.support));
  writer.u8(static_cast<std::uint8_t>(record.contact));
  writer.u8(static_cast<std::uint8_t>(record.orientation));
  source_edge_relation_detail::encode_truth(writer, record.parallel_truth);
  writer.boolean(record.has_coplanarity_truth);
  if (record.has_coplanarity_truth)
    source_edge_relation_detail::encode_truth(writer, record.coplanarity_truth);
  writer.boolean(record.has_collinearity_truth);
  if (record.has_collinearity_truth)
    source_edge_relation_detail::encode_truth(writer, record.collinearity_truth);
  writer.u8(record.parameter_count);
  writer.u8(record.point_count);
  for (std::size_t i = 0; i < record.parameter_count; ++i) {
    source_edge_relation_detail::encode_parameter(writer,
                                                  record.first_parameters[i]);
    source_edge_relation_detail::encode_parameter(writer,
                                                  record.second_parameters[i]);
  }
  for (std::size_t i = 0; i < record.point_count; ++i)
    source_edge_relation_detail::encode_point(writer, record.points[i]);
  writer.floating(record.residual_boundary);
  writer.u8(record.selected_minor_axis);
  writer.u8(record.selected_collinear_axis);
  writer.u16(record.reserved16);
  writer.u32(record.reserved32);
  return writer.take();
}

template <class T>
bool valid_source_edge_relation_record(
    const source_edge_relation_record<T> &record) {
  static_assert(supported_precision_scalar_v<T>);
  if (record.schema_version !=
          contract_versions::relation_source_edge_edge_schema ||
      record.policy_version !=
          contract_versions::relation_source_edge_edge_policy ||
      !record.owner.anchor || !valid_relation_feature_key(record.first_feature) ||
      !valid_relation_feature_key(record.second_feature) ||
      record.first_feature.kind != relation_feature_kind::source_edge ||
      record.second_feature.kind != relation_feature_kind::source_edge ||
      record.first_feature.operand == record.second_feature.operand ||
      !(record.first_feature < record.second_feature) ||
      !source_edge_relation_detail::valid_snapshot(record.first_start) ||
      !source_edge_relation_detail::valid_snapshot(record.first_end) ||
      !source_edge_relation_detail::valid_snapshot(record.second_start) ||
      !source_edge_relation_detail::valid_snapshot(record.second_end) ||
      !valid_relation_truth_record(record.parallel_truth) ||
      (record.has_coplanarity_truth &&
       !valid_relation_truth_record(record.coplanarity_truth)) ||
      (record.has_collinearity_truth &&
       !valid_relation_truth_record(record.collinearity_truth)) ||
      record.parameter_count > 2 || record.point_count > 2 ||
      !finite_bits(record.residual_boundary) ||
      record.residual_boundary < T(0) || record.selected_minor_axis > 3 ||
      record.selected_collinear_axis > 3 || record.reserved16 != 0 ||
      record.reserved32 != 0)
    return false;

  for (std::size_t i = 0; i < record.parameter_count; ++i)
    if (!source_edge_relation_detail::parameter_valid(
            record.first_parameters[i]) ||
        !source_edge_relation_detail::parameter_valid(
            record.second_parameters[i]))
      return false;
  for (std::size_t i = 0; i < record.point_count; ++i)
    if (!source_edge_relation_detail::point_valid(
            record.points[i], record.residual_boundary))
      return false;

  switch (record.support) {
  case source_edge_support_class::skew_separated:
    if (!record.has_coplanarity_truth || record.has_collinearity_truth ||
        record.contact != source_edge_contact_class::none ||
        record.orientation !=
            source_edge_orientation_relation::not_applicable ||
        record.parameter_count != 0 || record.point_count != 0 ||
        record.selected_minor_axis != 3 ||
        !source_edge_relation_detail::accepted_nonzero<T>(
            record.parallel_truth) ||
        !source_edge_relation_detail::accepted_nonzero<T>(
            record.coplanarity_truth))
      return false;
    break;
  case source_edge_support_class::nonparallel_coplanar:
    if (!record.has_coplanarity_truth || record.has_collinearity_truth ||
        record.orientation !=
            source_edge_orientation_relation::not_applicable ||
        record.selected_minor_axis >= 3 ||
        !source_edge_relation_detail::accepted_nonzero<T>(
            record.parallel_truth) ||
        !source_edge_relation_detail::zero_tie<T>(
            record.coplanarity_truth))
      return false;
    break;
  case source_edge_support_class::parallel_separated:
    if (record.has_coplanarity_truth || !record.has_collinearity_truth ||
        record.contact != source_edge_contact_class::none ||
        record.orientation !=
            source_edge_orientation_relation::not_applicable ||
        record.parameter_count != 0 || record.point_count != 0 ||
        record.selected_collinear_axis != 3 ||
        !source_edge_relation_detail::zero_tie<T>(record.parallel_truth) ||
        !source_edge_relation_detail::accepted_nonzero<T>(
            record.collinearity_truth))
      return false;
    break;
  case source_edge_support_class::collinear:
    if (record.has_coplanarity_truth || !record.has_collinearity_truth ||
        record.orientation ==
            source_edge_orientation_relation::not_applicable ||
        record.selected_collinear_axis >= 3 ||
        !source_edge_relation_detail::zero_tie<T>(record.parallel_truth) ||
        !source_edge_relation_detail::zero_tie<T>(
            record.collinearity_truth))
      return false;
    break;
  default:
    return false;
  }

  switch (record.contact) {
  case source_edge_contact_class::none:
    if (record.point_count != 0)
      return false;
    break;
  case source_edge_contact_class::proper_crossing:
    if (record.support !=
            source_edge_support_class::nonparallel_coplanar ||
        record.parameter_count != 1 || record.point_count != 1 ||
        record.first_parameters[0].domain !=
            parameter_domain_status::stable_interior ||
        record.second_parameters[0].domain !=
            parameter_domain_status::stable_interior)
      return false;
    break;
  case source_edge_contact_class::endpoint_contact:
  case source_edge_contact_class::point_contact:
    if (record.parameter_count != 1 || record.point_count != 1)
      return false;
    break;
  case source_edge_contact_class::partial_overlap:
  case source_edge_contact_class::first_contains_second:
  case source_edge_contact_class::second_contains_first:
  case source_edge_contact_class::equal:
    if (record.support != source_edge_support_class::collinear ||
        record.parameter_count != 2 || record.point_count != 2 ||
        !source_edge_relation_detail::definitely_before(
            record.first_parameters[0], record.first_parameters[1]))
      return false;
    break;
  default:
    return false;
  }

  return sha256::digest(encode_source_edge_relation_semantics(record)) ==
         record.semantic_digest;
}

} // namespace ygor::mesh_boolean::bounded
