#pragma once

#include "FacetFacetRelations.h"
#include "RelationExecutionAuthorityTypes.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct transverse_relation_evaluation_key final {
  relation_request_key carrier_relation{};
  relation_request_key member_relation{};
  std::uint32_t event_occurrence = 0;

  friend bool operator<(const transverse_relation_evaluation_key &a,
                        const transverse_relation_evaluation_key &b) noexcept {
    return std::tie(a.carrier_relation, a.member_relation, a.event_occurrence) <
           std::tie(b.carrier_relation, b.member_relation, b.event_occurrence);
  }
  friend bool operator==(const transverse_relation_evaluation_key &a,
                         const transverse_relation_evaluation_key &b) noexcept {
    return std::tie(a.carrier_relation, a.member_relation, a.event_occurrence) ==
           std::tie(b.carrier_relation, b.member_relation, b.event_occurrence);
  }
};

template <class T> struct transverse_relation_evaluated_record final {
  transverse_relation_evaluation_key key{};
  T parameter_nominal = T(0);
  finite_interval<T> parameter = finite_interval<T>::singleton(T(0));
  uncertainty_contributors parameter_contributors{};
  std::uint64_t parameter_trace_root = 0;
  construction_operation_certificate<T> parameter_certificate{};
  std::array<finite_interval<T>, 3> point_carrier_residuals{};
  source_facet_point_region_record<T> first_region{};
  source_facet_point_region_record<T> second_region{};
};

template <class T> struct transverse_relation_evaluated_stage final {
  std::vector<transverse_relation_evaluated_record<T>> records;
  std::uint64_t expected_population = 0;
  std::uint64_t evaluation_count = 0;
  bounded_boolean_digest semantic_digest{};
};

namespace transverse_relation_evaluation_detail {

template <class T>
bool accepted_residual(const finite_interval<T> &interval, T boundary) noexcept {
  return finite_bits(boundary) && boundary >= T(0) &&
         finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
         !finite_numeric_less(interval.upper(), interval.lower()) &&
         interval.lower() >= -boundary && interval.upper() <= boundary;
}

template <class T, class Snapshot>
bool restore_issued_vector(const Snapshot &snapshot,
                           const construction_operation_certificate<T> &certificate,
                           std::size_t begin, const context_owner_token &owner,
                           bounded_vec3<T> &out) {
  out = bounded_vec3<T>{};
  out.owner = owner;
  if (!valid_construction_operation_certificate(certificate) ||
      begin > certificate.component_count ||
      certificate.component_count - begin < 3)
    return false;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto &issued = certificate.issued_outputs[begin + axis];
    if (issued.rounded_nominal != snapshot.rounded[axis] ||
        issued.enclosure.lower() != snapshot.lower[axis] ||
        issued.enclosure.upper() != snapshot.upper[axis])
      return false;
    out.components[axis].rounded_nominal = issued.rounded_nominal;
    out.components[axis].uncertainty_enclosure = issued.enclosure;
    out.components[axis].identity = issued.identity;
    out.components[axis].contributors = issued.contributors;
  }
  return bounded_operations_detail::compute_radial_error(out);
}

template <class T, class I>
bool classify_snapshot_against_facet(
    const canonical_candidate_stream<T, I> &candidates,
    const context_owner_token &owner, const source_edge_geometry_snapshot<T> &point,
    const relation_feature_key &facet,
    source_facet_point_region_record<T> &region,
    const relation_feature_key *source_edge) {
  const auto *topology = facet.operand == operand_id::a
                             ? candidates.manifolds()->a().get()
                             : candidates.manifolds()->b().get();
  if (!topology || facet.kind != relation_feature_kind::source_facet ||
      facet.primary >= topology->source_facet_to_group().size())
    return false;
  const auto group_ordinal = topology->source_facet_to_group()[facet.primary];
  if (group_ordinal >= topology->facet_groups().size())
    return false;
  const auto &group = topology->facet_groups()[group_ordinal];
  if (group.ring != facet.secondary || group.basis.dropped_axis > 2)
    return false;
  std::vector<projected_source_point<T>> polygon;
  polygon.reserve(group.source_vertices.size());
  for (std::size_t corner = 0; corner < group.source_vertices.size(); ++corner) {
    const auto source = group.source_vertices[corner];
    if (source >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex = topology->source_vertex_to_vertex()[source];
    bounded_point3<T> bounded;
    if (vertex >= topology->vertices().size() ||
        !candidate_source_edge_relation_detail::import_vertex_point(
            topology->vertices()[vertex], facet.operand, owner, bounded))
      return false;
    polygon.push_back(source_edge_facet_detail::project_point(
        bounded, group.basis.dropped_axis, source, corner));
  }
  const auto orientation =
      bounded_source_polygon_kernel<T>::polygon_orientation(polygon);
  if (!orientation || orientation->bounded_sign == bounded_planar_sign::uncertain)
    return false;
  std::vector<source_facet_boundary_edge_owner> certified_edges;
  if (source_edge && source_edge->operand == facet.operand) {
    std::array<std::uint64_t, 2> vertices{};
    if (!candidate_source_edge_facet_detail::source_edge_vertices(
            candidates, *source_edge, vertices))
      return false;
    for (std::size_t edge = 0; edge < group.source_vertices.size(); ++edge) {
      const auto origin = group.source_vertices[edge];
      const auto destination =
          group.source_vertices[(edge + 1) % group.source_vertices.size()];
      if ((origin == vertices[0] && destination == vertices[1]) ||
          (origin == vertices[1] && destination == vertices[0]))
        certified_edges.push_back({edge, origin, destination});
    }
    if (certified_edges.size() != 1)
      return false;
  }
  auto classified = classify_source_facet_point(
      group.source_facet, group.ring,
      source_edge_facet_detail::project_snapshot(point, group.basis.dropped_axis),
      false, polygon, orientation->bounded_sign, nullptr,
      certified_edges.empty() ? nullptr : &certified_edges);
  if (!classified.has_value())
    return false;
  region = std::move(*classified.value());
  return true;
}

template <class T>
bool evaluate_parameter(const source_edge_geometry_snapshot<T> &point,
                        const construction_operation_certificate<T> &point_certificate,
                        const source_facet_transverse_carrier<T> &carrier,
                        const context_owner_token &owner, T residual_boundary,
                        transverse_relation_evaluated_record<T> &out) {
  bounded_vec3<T> bounded_point;
  bounded_vec3<T> carrier_point;
  bounded_vec3<T> direction;
  bounded_geometry_snapshot3<T> point_snapshot;
  point_snapshot.rounded = point.rounded_nominal;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    point_snapshot.lower[axis] = point.enclosure[axis].lower();
    point_snapshot.upper[axis] = point.enclosure[axis].upper();
  }
  if (!restore_issued_vector(point_snapshot, point_certificate, 0, owner,
                             bounded_point) ||
      !restore_issued_vector(carrier.point, carrier.certificate, 0, owner,
                             carrier_point) ||
      !restore_issued_vector(carrier.direction, carrier.certificate, 3, owner,
                             direction))
    return false;
  auto offset = bounded_vector_subtract(bounded_point, carrier_point);
  if (!offset.has_value())
    return false;
  auto numerator = bounded_dot3(*offset.value(), direction);
  auto denominator = bounded_squared_norm(direction);
  if (!numerator.has_value() || !denominator.has_value() ||
      denominator.value()->uncertainty_enclosure.lower() <= T(0))
    return false;
  auto parameter = bounded_divide(*numerator.value(), *denominator.value());
  if (!parameter.has_value())
    return false;
  const auto below = directed_subtract(
      parameter.value()->rounded_nominal,
      parameter.value()->uncertainty_enclosure.lower());
  const auto above = directed_subtract(
      parameter.value()->uncertainty_enclosure.upper(),
      parameter.value()->rounded_nominal);
  if (!below || !above)
    return false;
  const auto parameter_error =
      std::max(below.value.upper, above.value.upper);
  std::vector<const bounded_scalar<T> *> parameter_outputs{parameter.value()};
  std::vector<const bounded_scalar<T> *> parameter_inputs{
      numerator.value(), denominator.value()};
  auto parameter_certificate = certify_construction_components(
      rounded_operation_code::divide,
      contract_versions::rounded_operation_graphs, owner, parameter_outputs,
      parameter_error, parameter_inputs,
      denominator.value()->uncertainty_enclosure,
      construction_category::stable_interior,
      construction_tolerance_disposition::accepted, residual_boundary);
  if (!parameter_certificate.has_value())
    return false;
  auto scaled = bounded_vector_scale(direction, *parameter.value());
  if (!scaled.has_value())
    return false;
  auto reconstructed = bounded_vector_add(carrier_point, *scaled.value());
  if (!reconstructed.has_value())
    return false;
  auto residual = bounded_vector_subtract(bounded_point, *reconstructed.value());
  if (!residual.has_value())
    return false;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.point_carrier_residuals[axis] =
        residual.value()->components[axis].uncertainty_enclosure;
    if (!accepted_residual(out.point_carrier_residuals[axis], residual_boundary))
      return false;
  }
  out.parameter_nominal = parameter.value()->rounded_nominal;
  out.parameter = parameter.value()->uncertainty_enclosure;
  out.parameter_contributors = parameter.value()->contributors;
  out.parameter_trace_root = parameter.value()->identity.trace_root;
  out.parameter_certificate = std::move(*parameter_certificate.value());
  return out.parameter_trace_root != 0 &&
         bounded_operations_detail::bounded_scalar_valid(*parameter.value());
}

template <class T>
void encode_region(canonical_writer &writer,
                   const source_facet_point_region_record<T> &value) {
  writer.u16(value.schema_version);
  writer.u16(value.policy_version);
  writer.u8(static_cast<std::uint8_t>(value.classification));
  writer.u64(value.source_facet);
  writer.u64(value.ring);
  writer.u8(value.sweep_axis);
  writer.boolean(value.query_source_identity_valid);
  writer.boolean(value.complete_boundary_traversal);
  writer.boolean(value.boundary_ownership_resolved);
  writer.u64(value.boundary_test_count);
  writer.u64(value.parity_crossing_count);
  writer.u64(value.source_vertex_owners.size());
  for (const auto owner : value.source_vertex_owners)
    writer.u64(owner);
  writer.u64(value.source_edge_owners.size());
  for (const auto &owner : value.source_edge_owners) {
    writer.u64(owner.edge_ordinal);
    writer.u64(owner.origin_source_vertex);
    writer.u64(owner.destination_source_vertex);
  }
  writer.u8(static_cast<std::uint8_t>(
      value.polygon_orientation_evidence.bounded_sign));
  writer.u8(static_cast<std::uint8_t>(
      value.polygon_orientation_evidence.exact_sign));
  writer.u64(value.orientation_evidence.size());
  for (const auto &evidence : value.orientation_evidence) {
    writer.floating(evidence.determinant.lower());
    writer.floating(evidence.determinant.upper());
    writer.u8(static_cast<std::uint8_t>(evidence.bounded_sign));
    writer.u8(static_cast<std::uint8_t>(evidence.exact_sign));
    writer.u16(evidence.formula_version);
  }
}

template <class T>
void encode_record(canonical_writer &writer,
                   const transverse_relation_evaluated_record<T> &record) {
  encode_relation_request_key(writer, record.key.carrier_relation);
  encode_relation_request_key(writer, record.key.member_relation);
  writer.u32(record.key.event_occurrence);
  writer.floating(record.parameter_nominal);
  writer.floating(record.parameter.lower());
  writer.floating(record.parameter.upper());
  writer.u64(record.parameter_trace_root);
  encode_construction_operation_certificate(writer,
                                            record.parameter_certificate);
  const double contributors[]{record.parameter_contributors.inherited_a,
                              record.parameter_contributors.inherited_b,
                              record.parameter_contributors.machine_floor,
                              record.parameter_contributors.construction,
                              record.parameter_contributors.conditioning,
                              record.parameter_contributors.conversion,
                              record.parameter_contributors.prior_cleanup,
                              record.parameter_contributors.current_cleanup};
  for (const auto contributor : contributors)
    writer.floating(contributor);
  for (const auto &residual : record.point_carrier_residuals) {
    writer.floating(residual.lower());
    writer.floating(residual.upper());
  }
  encode_region(writer, record.first_region);
  encode_region(writer, record.second_region);
}

} // namespace transverse_relation_evaluation_detail

template <class T, class I>
boolean_outcome<transverse_relation_evaluated_stage<T>>
build_transverse_relation_evaluated_stage(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_execution_authority &authority,
    const candidate_source_edge_facet_relation_stage<T> &edge_facet_stage,
    const candidate_source_facet_relation_stage<T> &facet_stage,
    const relation_capabilities &capabilities, T residual_boundary) {
  using stage_type = transverse_relation_evaluated_stage<T>;
  stage_type out;
  bounded_boolean_error error;
  if (!capabilities.owner.same_owner(candidates.owner()) ||
      !execution_authorizes(authority, edge_facet_stage.request_graph) ||
      !execution_authorizes(authority, facet_stage.request_graph))
    return boolean_outcome<stage_type>::failure(relation_error(
        relation_subcode::predecessor_mismatch,
        bounded_boolean_error_category::internal_invariant_error,
        "Component 07 transverse evaluation predecessor handshake failed",
        relation_checkpoint::predecessor_validation));

  for (std::size_t member_index = 0;
       member_index < edge_facet_stage.relations.size(); ++member_index) {
    if (member_index >= edge_facet_stage.request_graph.requests.size())
      return boolean_outcome<stage_type>::failure(relation_error(
          relation_subcode::source_edge_facet_malformed,
          bounded_boolean_error_category::internal_invariant_error,
          "Component 07 transverse member request is absent",
          relation_checkpoint::construction_validation));
    const auto &member = edge_facet_stage.relations[member_index];
    const auto &member_key = edge_facet_stage.request_graph.requests[member_index].key;
    for (const auto &event : member.events) {
      for (std::size_t carrier_index = 0;
           carrier_index < facet_stage.relations.size(); ++carrier_index) {
        const auto &carrier = facet_stage.relations[carrier_index];
        if (carrier.classification !=
                source_facet_support_relation_class::transverse ||
            !carrier.has_transverse_carrier ||
            !std::binary_search(carrier.edge_facet_consumers.begin(),
                                carrier.edge_facet_consumers.end(),
                                relation_request_id(member_index)))
          continue;
        if (carrier_index >= facet_stage.request_graph.requests.size())
          return boolean_outcome<stage_type>::failure(relation_error(
              relation_subcode::source_facet_relation_malformed,
              bounded_boolean_error_category::internal_invariant_error,
              "Component 07 transverse carrier request is absent",
              relation_checkpoint::construction_validation));
        const auto &carrier_key =
            facet_stage.request_graph.requests[carrier_index].key;
        transverse_relation_evaluated_record<T> record;
        record.key = {carrier_key, member_key, event.occurrence};
        const bool first_is_target = carrier_key.first == member_key.second;
        const bool second_is_target = carrier_key.second == member_key.second;
        if ((!first_is_target && !second_is_target) ||
            !transverse_relation_evaluation_detail::evaluate_parameter(
                event.construction.point, event.construction.certificate,
                carrier.transverse_carrier,
                capabilities.owner, residual_boundary, record) ||
            (first_is_target
                 ? (record.first_region = event.region, false)
                 : !transverse_relation_evaluation_detail::
                       classify_snapshot_against_facet(
                           candidates, capabilities.owner,
                           event.construction.point, carrier_key.first,
                           record.first_region, &member_key.first)) ||
            (second_is_target
                 ? (record.second_region = event.region, false)
                 : !transverse_relation_evaluation_detail::
                       classify_snapshot_against_facet(
                           candidates, capabilities.owner,
                           event.construction.point, carrier_key.second,
                           record.second_region, &member_key.first)) ||
            record.first_region.classification ==
                source_facet_point_region_class::outside ||
            record.second_region.classification ==
                source_facet_point_region_class::outside)
          return boolean_outcome<stage_type>::failure(relation_error(
              relation_subcode::source_facet_carrier_unresolved,
              bounded_boolean_error_category::internal_invariant_error,
              "Component 07 transverse evaluated producer could not certify membership",
              relation_checkpoint::construction_validation));
        out.records.push_back(std::move(record));
      }
    }
  }
  out.expected_population = out.records.size();
  out.evaluation_count = out.records.size();
  if (out.records.size() > capabilities.maximum_transverse_memberships)
    return boolean_outcome<stage_type>::failure(relation_error(
        relation_subcode::work_limit,
        bounded_boolean_error_category::resource_limit,
        "Component 07 transverse evaluated producer limit exceeded",
        relation_checkpoint::count_representability_preflight));
  std::sort(out.records.begin(), out.records.end(), [](const auto &a, const auto &b) {
    return a.key < b.key;
  });
  for (std::size_t i = 1; i < out.records.size(); ++i)
    if (out.records[i - 1].key == out.records[i].key)
      return boolean_outcome<stage_type>::failure(relation_error(
          relation_subcode::duplicate_authoritative_producer,
          bounded_boolean_error_category::internal_invariant_error,
          "Component 07 transverse evaluated producer key is duplicated",
          relation_checkpoint::producer_verification));
  canonical_writer encoded;
  encoded.u64(out.expected_population);
  encoded.u64(out.evaluation_count);
  for (const auto &record : out.records)
    transverse_relation_evaluation_detail::encode_record(encoded, record);
  out.semantic_digest = sha256::digest(encoded.take());
  return boolean_outcome<stage_type>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
