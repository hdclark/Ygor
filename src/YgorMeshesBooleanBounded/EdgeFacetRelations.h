#pragma once

#include "CandidateSourceEdgeRelations.h"
#include "SourceFacetRegionSegmentBuild.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class source_edge_facet_support_class : std::uint8_t {
  definitely_separated_same_side = 1,
  transverse_support_crossing = 2,
  endpoint_support_tie = 3,
  coplanar_support = 4,
};

enum class source_edge_facet_contact_class : std::uint8_t {
  none = 1,
  proper_face_crossing = 2,
  boundary_crossing = 3,
  endpoint_contact = 4,
  tangent_contact = 5,
  coplanar_point_contact = 6,
  coplanar_boundary_overlap = 7,
  coplanar_containment = 8,
};

enum class source_edge_facet_event_kind : std::uint8_t {
  proper_face_crossing = 1,
  boundary_crossing = 2,
  endpoint_contact = 3,
  tangent_contact = 4,
};

enum class source_edge_facet_occupancy_state : std::uint8_t {
  unoccupied = 1,
  occupied = 2,
  on_support = 3,
};

template <class T> struct source_edge_facet_point_construction final {
  source_edge_geometry_snapshot<T> point{};
  std::array<finite_interval<T>, 3> edge_carrier_residual{};
  finite_interval<T> support_residual{};
  std::uint8_t edge_endpoint_owner_mask = 0;
  bool accepted_source_vertex = false;
  bool tolerance_compatible = false;
  std::uint8_t reserved8 = 0;
  std::uint32_t reserved32 = 0;
};

template <class T> struct source_edge_facet_event_record final {
  source_edge_facet_event_kind kind =
      source_edge_facet_event_kind::proper_face_crossing;
  source_edge_parameter_evidence<T> parameter{};
  source_edge_facet_point_construction<T> construction{};
  source_facet_point_region_record<T> region{};
  source_edge_facet_occupancy_state before =
      source_edge_facet_occupancy_state::unoccupied;
  source_edge_facet_occupancy_state after =
      source_edge_facet_occupancy_state::unoccupied;
  std::int8_t numeric_crossing = 0;
  std::uint32_t occurrence = 0;
  std::uint32_t reserved = 0;
};

struct source_edge_facet_boundary_binding final {
  relation_request_id request{0};
  relation_feature_key feature{};
  source_facet_boundary_edge_owner owner{};
  std::array<std::uint64_t, 2> parameter_source_vertices{};
  std::uint32_t reserved = 0;
};

template <class T> struct source_edge_facet_boundary_relation final {
  source_edge_facet_boundary_binding binding{};
  source_edge_relation_record<T> relation{};
};

template <class T> struct source_edge_facet_input final {
  source_edge_relation_input<T> edge{};
  std::array<std::uint64_t, 2> edge_source_vertices{};
  relation_feature_key facet_feature{};
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  occupied_side material_side = occupied_side::negative;
  std::uint8_t dropped_axis = 0;
  std::array<bounded_point3<T>, 3> support{};
  std::vector<projected_source_point<T>> polygon;
  bounded_planar_sign polygon_orientation = bounded_planar_sign::uncertain;
  std::vector<source_edge_facet_boundary_relation<T>> boundary_relations;
  std::uint32_t reserved = 0;
};

template <class T> struct source_edge_facet_relation_record final {
  std::uint16_t schema_version =
      contract_versions::relation_source_edge_facet_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_edge_facet_policy;
  context_owner_token owner{};
  relation_feature_key edge_feature{};
  relation_feature_key facet_feature{};
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  occupied_side material_side = occupied_side::negative;
  source_edge_facet_support_class support =
      source_edge_facet_support_class::definitely_separated_same_side;
  source_edge_facet_contact_class contact =
      source_edge_facet_contact_class::none;
  std::array<relation_truth_record, 2> endpoint_support_truth{};
  std::vector<source_edge_facet_event_record<T>> events;
  bool has_coplanar_partition = false;
  source_facet_segment_partition_record<T> coplanar_partition{};
  std::vector<relation_request_id> boundary_relation_requests;
  T residual_boundary = T(0);
  std::uint32_t reserved = 0;
  bounded_boolean_digest semantic_digest{};
};

struct candidate_source_edge_facet_relation_range final {
  candidate_id candidate{0};
  std::uint64_t begin = 0;
  std::uint64_t count = 0;
  std::uint32_t reserved = 0;
};

template <class T> struct candidate_source_edge_facet_relation_stage final {
  std::uint16_t schema_version =
      contract_versions::relation_source_edge_facet_stage_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_edge_facet_stage_policy;
  context_owner_token owner{};
  relation_request_graph request_graph{};
  std::vector<source_edge_facet_relation_record<T>> relations;
  std::vector<relation_request_id> candidate_relations;
  std::vector<candidate_source_edge_facet_relation_range> candidate_ranges;
  std::uint64_t evaluation_count = 0;
  std::uint32_t reserved = 0;
  bounded_boolean_digest semantic_digest{};
};

inline bounded_boolean_error source_edge_facet_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint = relation_checkpoint::edge_facet_evaluation,
    bounded_boolean_error_category category =
        bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(subcode, category, summary, checkpoint);
}

namespace source_edge_facet_detail {

template <class T>
void encode_interval(canonical_writer &writer, const finite_interval<T> &value) {
  writer.floating(value.lower());
  writer.floating(value.upper());
}

inline void encode_edge_owner(canonical_writer &writer,
                              const source_facet_boundary_edge_owner &owner) {
  writer.u64(owner.edge_ordinal);
  writer.u64(owner.origin_source_vertex);
  writer.u64(owner.destination_source_vertex);
}

template <class T>
void encode_orientation(canonical_writer &writer,
                        const source_orientation_evidence<T> &evidence) {
  encode_interval(writer, evidence.determinant);
  writer.u8(static_cast<std::uint8_t>(evidence.exact_sign + 1));
  writer.u8(static_cast<std::uint8_t>(
      static_cast<std::int8_t>(evidence.bounded_sign) + 1));
  writer.u16(evidence.formula_version);
}

template <class T>
void encode_region(canonical_writer &writer,
                   const source_facet_point_region_record<T> &region) {
  writer.u16(region.schema_version);
  writer.u16(region.policy_version);
  writer.u8(static_cast<std::uint8_t>(region.classification));
  writer.u64(region.source_facet);
  writer.u64(region.ring);
  writer.u8(region.sweep_axis);
  writer.boolean(region.query_source_identity_valid);
  writer.boolean(region.complete_boundary_traversal);
  writer.boolean(region.boundary_ownership_resolved);
  writer.u64(region.boundary_test_count);
  writer.u64(region.parity_crossing_count);
  writer.u64(region.source_vertex_owners.size());
  for (const auto owner : region.source_vertex_owners)
    writer.u64(owner);
  writer.u64(region.source_edge_owners.size());
  for (const auto &owner : region.source_edge_owners)
    encode_edge_owner(writer, owner);
  encode_orientation(writer, region.polygon_orientation_evidence);
  writer.u64(region.orientation_evidence.size());
  for (const auto &evidence : region.orientation_evidence)
    encode_orientation(writer, evidence);
  writer.u32(region.reserved);
}

template <class T>
void encode_point(canonical_writer &writer,
                  const source_edge_facet_point_construction<T> &point) {
  source_edge_relation_detail::encode_snapshot(writer, point.point);
  for (const auto &residual : point.edge_carrier_residual)
    encode_interval(writer, residual);
  encode_interval(writer, point.support_residual);
  writer.u8(point.edge_endpoint_owner_mask);
  writer.boolean(point.accepted_source_vertex);
  writer.boolean(point.tolerance_compatible);
  writer.u8(point.reserved8);
  writer.u32(point.reserved32);
}

template <class T>
void encode_event(canonical_writer &writer,
                  const source_edge_facet_event_record<T> &event) {
  writer.u8(static_cast<std::uint8_t>(event.kind));
  source_edge_relation_detail::encode_parameter(writer, event.parameter);
  encode_point(writer, event.construction);
  encode_region(writer, event.region);
  writer.u8(static_cast<std::uint8_t>(event.before));
  writer.u8(static_cast<std::uint8_t>(event.after));
  writer.u8(static_cast<std::uint8_t>(event.numeric_crossing + 1));
  writer.u32(event.occurrence);
  writer.u32(event.reserved);
}

template <class T>
bool valid_point_construction(
    const source_edge_facet_point_construction<T> &point, T boundary) {
  if (!source_edge_relation_detail::valid_snapshot(point.point) ||
      !point.tolerance_compatible || point.edge_endpoint_owner_mask > 3 ||
      point.reserved8 != 0 || point.reserved32 != 0 ||
      !source_edge_relation_detail::residual_accepted(point.support_residual,
                                                       boundary))
    return false;
  for (const auto &residual : point.edge_carrier_residual)
    if (!source_edge_relation_detail::residual_accepted(residual, boundary))
      return false;
  return true;
}

inline bool valid_occupancy(source_edge_facet_occupancy_state state) noexcept {
  return state == source_edge_facet_occupancy_state::unoccupied ||
         state == source_edge_facet_occupancy_state::occupied ||
         state == source_edge_facet_occupancy_state::on_support;
}

template <class T>
bool valid_event(const source_edge_facet_event_record<T> &event, T boundary,
                 std::uint64_t source_facet, std::uint64_t ring) {
  if (!source_edge_relation_detail::parameter_valid(event.parameter) ||
      !valid_point_construction(event.construction, boundary) ||
      !valid_source_facet_point_region_record(event.region) ||
      event.region.source_facet != source_facet || event.region.ring != ring ||
      !valid_occupancy(event.before) || !valid_occupancy(event.after) ||
      event.numeric_crossing < -1 || event.numeric_crossing > 1 ||
      event.reserved != 0)
    return false;
  switch (event.kind) {
  case source_edge_facet_event_kind::proper_face_crossing:
    return event.region.classification ==
               source_facet_point_region_class::interior &&
           event.parameter.domain == parameter_domain_status::stable_interior &&
           event.numeric_crossing != 0;
  case source_edge_facet_event_kind::boundary_crossing:
    return (event.region.classification ==
                source_facet_point_region_class::original_edge ||
            event.region.classification ==
                source_facet_point_region_class::original_vertex) &&
           event.parameter.domain == parameter_domain_status::stable_interior;
  case source_edge_facet_event_kind::endpoint_contact:
  case source_edge_facet_event_kind::tangent_contact:
    return event.parameter.domain == parameter_domain_status::stable_endpoint &&
           event.numeric_crossing == 0;
  }
  return false;
}

template <class T>
std::array<T, 3> snapshot_nominal(
    const source_edge_geometry_snapshot<T> &point) noexcept {
  return point.rounded_nominal;
}

template <class T>
projected_source_point<T> project_point(
    const bounded_point3<T> &point, std::uint8_t dropped_axis,
    std::uint64_t source_vertex = 0, std::uint64_t source_corner = 0) {
  projected_source_point<T> result;
  result.source_vertex = source_vertex;
  result.source_corner = source_corner;
  const std::array<std::array<std::size_t, 2>, 3> axes{{
      {{1, 2}}, {{0, 2}}, {{0, 1}},
  }};
  const auto selected = axes[dropped_axis];
  for (std::size_t axis = 0; axis < 2; ++axis) {
    result.nominal[axis] =
        point.coordinates.components[selected[axis]].rounded_nominal;
    result.enclosure[axis] =
        point.coordinates.components[selected[axis]].uncertainty_enclosure;
  }
  return result;
}

template <class T>
projected_source_point<T> project_snapshot(
    const source_edge_geometry_snapshot<T> &point, std::uint8_t dropped_axis,
    std::uint64_t source_vertex = 0, std::uint64_t source_corner = 0) {
  projected_source_point<T> result;
  result.source_vertex = source_vertex;
  result.source_corner = source_corner;
  const std::array<std::array<std::size_t, 2>, 3> axes{{
      {{1, 2}}, {{0, 2}}, {{0, 1}},
  }};
  const auto selected = axes[dropped_axis];
  for (std::size_t axis = 0; axis < 2; ++axis) {
    result.nominal[axis] = point.rounded_nominal[selected[axis]];
    result.enclosure[axis] = point.enclosure[selected[axis]];
  }
  return result;
}

template <class T>
bool same_truth_side(const relation_truth_record &a,
                     const relation_truth_record &b) noexcept {
  return source_edge_relation_detail::accepted_nonzero<T>(a) &&
         source_edge_relation_detail::accepted_nonzero<T>(b) &&
         a.bounded_sign == b.bounded_sign;
}

template <class T>
bool opposite_truth_side(const relation_truth_record &a,
                         const relation_truth_record &b) noexcept {
  return source_edge_relation_detail::accepted_nonzero<T>(a) &&
         source_edge_relation_detail::accepted_nonzero<T>(b) &&
         a.bounded_sign != b.bounded_sign;
}

inline source_edge_facet_occupancy_state occupancy_for_truth(
    const relation_truth_record &truth, occupied_side material_side) {
  if (truth.exact_relation == exact_relation_status::exact_zero)
    return source_edge_facet_occupancy_state::on_support;
  const bool positive =
      truth.bounded_sign == bounded_sign_status::definitely_positive;
  const bool occupied = material_side == occupied_side::positive
                            ? positive
                            : !positive;
  return occupied ? source_edge_facet_occupancy_state::occupied
                  : source_edge_facet_occupancy_state::unoccupied;
}

inline std::int8_t crossing_delta(source_edge_facet_occupancy_state before,
                                  source_edge_facet_occupancy_state after) {
  if (before == source_edge_facet_occupancy_state::unoccupied &&
      after == source_edge_facet_occupancy_state::occupied)
    return 1;
  if (before == source_edge_facet_occupancy_state::occupied &&
      after == source_edge_facet_occupancy_state::unoccupied)
    return -1;
  return 0;
}

template <class T>
boolean_outcome<relation_truth_record> endpoint_support_truth(
    const source_edge_facet_input<T> &input, const bounded_vec3<T> &normal,
    std::size_t endpoint) {
  const auto &point = endpoint == 0 ? input.edge.start : input.edge.end;
  auto offset = bounded_vector_subtract(point.coordinates,
                                        input.support[0].coordinates);
  if (!offset.has_value())
    return boolean_outcome<relation_truth_record>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_support_unresolved,
            "Component 07 edge/facet endpoint support offset failed"));
  auto bounded = bounded_dot3(*offset.value(), normal);
  if (!bounded.has_value())
    return boolean_outcome<relation_truth_record>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_support_unresolved,
            "Component 07 edge/facet endpoint support residual failed"));
  const auto exact = exact_coplanarity_3d(
      source_edge_relation_detail::nominal(input.support[0]),
      source_edge_relation_detail::nominal(input.support[2]),
      source_edge_relation_detail::nominal(input.support[1]),
      source_edge_relation_detail::nominal(point));
  auto truth = source_edge_relation_detail::make_truth(
      std::move(*bounded.value()), exact, rounded_operation_code::dot3);
  if (!truth.has_value())
    return boolean_outcome<relation_truth_record>::failure(*truth.error());
  return truth;
}

template <class T>
boolean_outcome<source_edge_facet_point_construction<T>> make_construction(
    const source_edge_facet_input<T> &input, const bounded_vec3<T> &normal,
    const bounded_point3<T> &point,
    const source_edge_relation_detail::parameter_work<T> &parameter,
    T residual_boundary, bool accepted_source_vertex,
    std::uint8_t endpoint_mask) {
  auto reconstructed = bounded_interpolate_from_a(
      input.edge.start, input.edge.end,
      source_edge_relation_detail::as_bounded_parameter(parameter,
                                                         input.edge.start.owner));
  if (!reconstructed.has_value())
    return boolean_outcome<source_edge_facet_point_construction<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_residual_rejected,
            "Component 07 edge/facet edge reconstruction failed"));
  auto edge_residual = bounded_vector_subtract(
      point.coordinates, reconstructed.value()->coordinates);
  auto support_offset = bounded_vector_subtract(
      point.coordinates, input.support[0].coordinates);
  if (!edge_residual.has_value() || !support_offset.has_value())
    return boolean_outcome<source_edge_facet_point_construction<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_residual_rejected,
            "Component 07 edge/facet construction residual failed"));
  auto support_residual = bounded_dot3(*support_offset.value(), normal);
  if (!support_residual.has_value())
    return boolean_outcome<source_edge_facet_point_construction<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_residual_rejected,
            "Component 07 edge/facet support reconstruction failed"));

  source_edge_facet_point_construction<T> result;
  result.point = source_edge_relation_detail::snapshot(point);
  result.edge_endpoint_owner_mask = endpoint_mask;
  result.accepted_source_vertex = accepted_source_vertex;
  result.tolerance_compatible = true;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result.edge_carrier_residual[axis] =
        edge_residual.value()->components[axis].uncertainty_enclosure;
    if (!source_edge_relation_detail::residual_accepted(
            result.edge_carrier_residual[axis], residual_boundary))
      return boolean_outcome<source_edge_facet_point_construction<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_residual_rejected,
              "Component 07 edge/facet edge residual exceeds tolerance"));
  }
  result.support_residual =
      support_residual.value()->uncertainty_enclosure;
  if (!source_edge_relation_detail::residual_accepted(result.support_residual,
                                                       residual_boundary))
    return boolean_outcome<source_edge_facet_point_construction<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_residual_rejected,
            "Component 07 edge/facet support residual exceeds tolerance"));
  return boolean_outcome<source_edge_facet_point_construction<T>>::success(
      std::move(result));
}

inline std::uint8_t endpoint_index(std::uint8_t mask) noexcept {
  return mask == 1 ? std::uint8_t{0} : std::uint8_t{1};
}

template <class T>
bool relation_parameter_views(
    const source_edge_facet_input<T> &input,
    const source_edge_facet_boundary_relation<T> &boundary,
    const std::array<source_edge_parameter_evidence<T>, 2> *&query,
    const std::array<source_edge_parameter_evidence<T>, 2> *&facet) {
  if (boundary.relation.first_feature == input.edge.feature &&
      boundary.relation.second_feature == boundary.binding.feature) {
    query = &boundary.relation.first_parameters;
    facet = &boundary.relation.second_parameters;
    return true;
  }
  if (boundary.relation.second_feature == input.edge.feature &&
      boundary.relation.first_feature == boundary.binding.feature) {
    query = &boundary.relation.second_parameters;
    facet = &boundary.relation.first_parameters;
    return true;
  }
  return false;
}

inline std::uint64_t contact_lineage(relation_request_id request,
                                     std::uint32_t endpoint) noexcept {
  const auto base = request.ordinal();
  if (base > (std::numeric_limits<std::uint64_t>::max() - 2) / 2)
    return 0;
  return base * 2 + endpoint + 1;
}

template <class T>
bool parameter_definitely_before(const source_edge_parameter_evidence<T> &a,
                                 const source_edge_parameter_evidence<T> &b) {
  return source_edge_relation_detail::definitely_before(a, b);
}

template <class T>
bool append_coplanar_contacts(
    const source_edge_facet_input<T> &input,
    const source_edge_facet_boundary_relation<T> &boundary,
    std::vector<source_facet_segment_contact_proposal<T>> &contacts,
    bounded_boolean_error &error) {
  const auto &relation = boundary.relation;
  if (!valid_source_edge_relation_record(relation) ||
      boundary.binding.reserved != 0 ||
      boundary.binding.owner.edge_ordinal >= input.polygon.size() ||
      boundary.binding.owner.origin_source_vertex ==
          boundary.binding.owner.destination_source_vertex) {
    error = source_edge_facet_error(
        relation_subcode::source_edge_facet_boundary_coverage,
        "Component 07 coplanar boundary relation is malformed");
    return false;
  }
  if (relation.contact == source_edge_contact_class::none)
    return true;

  const std::array<source_edge_parameter_evidence<T>, 2> *query = nullptr;
  const std::array<source_edge_parameter_evidence<T>, 2> *facet = nullptr;
  if (!relation_parameter_views(input, boundary, query, facet)) {
    error = source_edge_facet_error(
        relation_subcode::source_edge_facet_boundary_coverage,
        "Component 07 coplanar boundary relation feature binding is inconsistent");
    return false;
  }

  const auto add_vertex_owner = [&](
                                    const source_edge_parameter_evidence<T> &p,
                                    std::vector<std::uint64_t> &owners,
                                    projected_source_point<T> &projected,
                                    bool &identity_valid) {
    const auto mask = source_edge_relation_detail::endpoint_mask(p);
    if (mask == 1 || mask == 2) {
      const auto vertex =
          boundary.binding.parameter_source_vertices[endpoint_index(mask)];
      owners.push_back(vertex);
      projected.source_vertex = vertex;
      identity_valid = true;
    }
  };

  if (relation.contact == source_edge_contact_class::proper_crossing ||
      relation.contact == source_edge_contact_class::endpoint_contact ||
      relation.contact == source_edge_contact_class::point_contact) {
    if (relation.parameter_count != 1 || relation.point_count != 1) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_boundary_coverage,
          "Component 07 coplanar point relation cardinality is inconsistent");
      return false;
    }
    source_facet_segment_contact_proposal<T> contact;
    contact.kind = source_facet_segment_contact_kind::point_contact;
    contact.lineage = contact_lineage(boundary.binding.request, 0);
    contact.first_rounded_parameter = (*query)[0].rounded_nominal;
    contact.first_parameter = (*query)[0].enclosure;
    contact.first_point = project_snapshot(relation.points[0].point,
                                           input.dropped_axis);
    contact.first_source_edge_owners = {boundary.binding.owner};
    add_vertex_owner((*facet)[0], contact.first_source_vertex_owners,
                     contact.first_point,
                     contact.first_point_source_identity_valid);
    contact.second_rounded_parameter = contact.first_rounded_parameter;
    contact.second_parameter = contact.first_parameter;
    contact.second_point = contact.first_point;
    if (contact.lineage == 0) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_boundary_coverage,
          "Component 07 coplanar point lineage overflowed");
      return false;
    }
    contacts.push_back(std::move(contact));
    return true;
  }

  if (relation.parameter_count != 2 || relation.point_count != 2) {
    error = source_edge_facet_error(
        relation_subcode::source_edge_facet_boundary_coverage,
        "Component 07 coplanar overlap relation cardinality is inconsistent");
    return false;
  }
  std::size_t first = 0;
  std::size_t second = 1;
  if (!parameter_definitely_before((*query)[first], (*query)[second])) {
    if (parameter_definitely_before((*query)[second], (*query)[first]))
      std::swap(first, second);
    else {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_order_unresolved,
          "Component 07 coplanar overlap parameter order is unresolved");
      return false;
    }
  }

  source_facet_segment_contact_proposal<T> contact;
  contact.kind = source_facet_segment_contact_kind::boundary_overlap;
  contact.lineage = contact_lineage(boundary.binding.request, 0);
  contact.first_rounded_parameter = (*query)[first].rounded_nominal;
  contact.first_parameter = (*query)[first].enclosure;
  contact.first_point =
      project_snapshot(relation.points[first].point, input.dropped_axis);
  contact.first_source_edge_owners = {boundary.binding.owner};
  add_vertex_owner((*facet)[first], contact.first_source_vertex_owners,
                   contact.first_point,
                   contact.first_point_source_identity_valid);
  contact.second_rounded_parameter = (*query)[second].rounded_nominal;
  contact.second_parameter = (*query)[second].enclosure;
  contact.second_point =
      project_snapshot(relation.points[second].point, input.dropped_axis);
  contact.second_source_edge_owners = {boundary.binding.owner};
  add_vertex_owner((*facet)[second], contact.second_source_vertex_owners,
                   contact.second_point,
                   contact.second_point_source_identity_valid);
  contact.overlap_source_edge_owners = {boundary.binding.owner};
  if (contact.lineage == 0) {
    error = source_edge_facet_error(
        relation_subcode::source_edge_facet_boundary_coverage,
        "Component 07 coplanar overlap lineage overflowed");
    return false;
  }
  contacts.push_back(std::move(contact));
  return true;
}

template <class T>
source_edge_facet_contact_class classify_partition_contact(
    const source_facet_segment_partition_record<T> &partition) {
  bool point = !partition.contacts.empty();
  bool overlap = false;
  bool interior = false;
  for (const auto &interval : partition.intervals) {
    overlap = overlap || interval.classification ==
                             source_facet_segment_interval_class::original_edge_overlap;
    interior = interior || interval.classification ==
                               source_facet_segment_interval_class::interior;
  }
  if (overlap)
    return source_edge_facet_contact_class::coplanar_boundary_overlap;
  if (interior)
    return source_edge_facet_contact_class::coplanar_containment;
  if (point)
    return source_edge_facet_contact_class::coplanar_point_contact;
  return source_edge_facet_contact_class::none;
}

template <class T>
bool valid_boundary_relations(const source_edge_facet_input<T> &input) {
  if (input.boundary_relations.size() != input.polygon.size())
    return false;
  std::vector<relation_request_id> requests;
  requests.reserve(input.boundary_relations.size());
  for (std::size_t edge = 0; edge < input.boundary_relations.size(); ++edge) {
    const auto &boundary = input.boundary_relations[edge];
    const auto origin = input.polygon[edge].source_vertex;
    const auto destination =
        input.polygon[(edge + 1) % input.polygon.size()].source_vertex;
    const bool parameter_vertices_match_owner =
        (boundary.binding.parameter_source_vertices[0] == origin &&
         boundary.binding.parameter_source_vertices[1] == destination) ||
        (boundary.binding.parameter_source_vertices[0] == destination &&
         boundary.binding.parameter_source_vertices[1] == origin);
    const std::array<source_edge_parameter_evidence<T>, 2> *query = nullptr;
    const std::array<source_edge_parameter_evidence<T>, 2> *facet = nullptr;
    if (boundary.binding.owner.edge_ordinal != edge ||
        boundary.binding.owner.origin_source_vertex != origin ||
        boundary.binding.owner.destination_source_vertex != destination ||
        boundary.binding.feature.kind != relation_feature_kind::source_edge ||
        boundary.binding.feature.operand != input.facet_feature.operand ||
        boundary.binding.reserved != 0 || !parameter_vertices_match_owner ||
        !valid_source_edge_relation_record(boundary.relation) ||
        !relation_parameter_views(input, boundary, query, facet))
      return false;
    requests.push_back(boundary.binding.request);
  }
  std::sort(requests.begin(), requests.end());
  return std::adjacent_find(requests.begin(), requests.end()) == requests.end();
}

template <class T>
bool certified_non_coplanar_boundary_owners(
    const source_edge_facet_input<T> &input,
    std::vector<std::uint64_t> &source_vertices,
    std::vector<source_facet_boundary_edge_owner> &source_edges,
    bounded_boolean_error &error) {
  source_vertices.clear();
  source_edges.clear();
  for (const auto &boundary : input.boundary_relations) {
    const auto &relation = boundary.relation;
    if (relation.contact == source_edge_contact_class::none)
      continue;
    if (relation.contact == source_edge_contact_class::partial_overlap ||
        relation.contact == source_edge_contact_class::first_contains_second ||
        relation.contact == source_edge_contact_class::second_contains_first ||
        relation.contact == source_edge_contact_class::equal ||
        relation.point_count != 1) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_boundary_coverage,
          "Component 07 non-coplanar facet boundary has incompatible overlap evidence");
      return false;
    }
    const bool boundary_is_first =
        relation.first_feature == boundary.binding.feature;
    const bool boundary_is_second =
        relation.second_feature == boundary.binding.feature;
    const bool query_is_first = relation.first_feature == input.edge.feature;
    const bool query_is_second = relation.second_feature == input.edge.feature;
    if (boundary_is_first == boundary_is_second ||
        query_is_first == query_is_second || boundary_is_first == query_is_first) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_boundary_coverage,
          "Component 07 boundary relation feature roles are inconsistent");
      return false;
    }
    source_edges.push_back(boundary.binding.owner);
    const auto mask = boundary_is_first
                          ? relation.points[0].first_endpoint_owner_mask
                          : relation.points[0].second_endpoint_owner_mask;
    if ((mask & std::uint8_t{1}) != 0)
      source_vertices.push_back(
          boundary.binding.parameter_source_vertices[0]);
    if ((mask & std::uint8_t{2}) != 0)
      source_vertices.push_back(
          boundary.binding.parameter_source_vertices[1]);
    if ((mask & ~std::uint8_t{3}) != 0) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_boundary_coverage,
          "Component 07 boundary endpoint ownership mask is invalid");
      return false;
    }
  }
  source_facet_region_detail::canonicalize_owners(source_vertices, source_edges);
  return true;
}

template <class T>
bool validate_input(const source_edge_facet_input<T> &input,
                    const context_owner_token &owner) {
  if (!owner.anchor || input.reserved != 0 || input.edge.reserved != 0 ||
      !input.edge.original_source_edge ||
      !valid_relation_feature_key(input.edge.feature) ||
      input.edge.feature.kind != relation_feature_kind::source_edge ||
      !valid_relation_feature_key(input.facet_feature) ||
      input.facet_feature.kind != relation_feature_kind::source_facet ||
      input.edge.feature.operand == input.facet_feature.operand ||
      input.facet_feature.primary != input.source_facet ||
      input.facet_feature.secondary != input.ring || input.dropped_axis > 2 ||
      input.polygon.size() < 3 ||
      (input.material_side != occupied_side::negative &&
       input.material_side != occupied_side::positive) ||
      !source_edge_relation_detail::valid_point(input.edge.start, owner) ||
      !source_edge_relation_detail::valid_point(input.edge.end, owner))
    return false;
  for (const auto &point : input.support)
    if (!source_edge_relation_detail::valid_point(point, owner))
      return false;
  for (const auto &point : input.polygon)
    if (!source_facet_region_detail::valid_projected_point(point))
      return false;
  const auto orientation =
      bounded_source_polygon_kernel<T>::polygon_orientation(input.polygon);
  return orientation && valid_source_orientation_evidence(*orientation) &&
         orientation->bounded_sign == input.polygon_orientation &&
         orientation->exact_sign == static_cast<int>(input.polygon_orientation) &&
         valid_boundary_relations(input);
}

template <class T>
bool record_semantics_equal(const source_edge_facet_relation_record<T> &a,
                            const source_edge_facet_relation_record<T> &b);

} // namespace source_edge_facet_detail

template <class T>
std::vector<std::uint8_t> encode_source_edge_facet_relation_semantics(
    const source_edge_facet_relation_record<T> &record) {
  canonical_writer writer;
  writer.u32(0x37464645U); // EFF7
  writer.u16(record.schema_version);
  writer.u16(record.policy_version);
  encode_relation_feature_key(writer, record.edge_feature);
  encode_relation_feature_key(writer, record.facet_feature);
  writer.u64(record.source_facet);
  writer.u64(record.ring);
  writer.u64(record.shell);
  writer.u8(static_cast<std::uint8_t>(record.material_side));
  writer.u8(static_cast<std::uint8_t>(record.support));
  writer.u8(static_cast<std::uint8_t>(record.contact));
  for (const auto &truth : record.endpoint_support_truth)
    source_edge_relation_detail::encode_truth(writer, truth);
  writer.u64(record.events.size());
  for (const auto &event : record.events)
    source_edge_facet_detail::encode_event(writer, event);
  writer.boolean(record.has_coplanar_partition);
  if (record.has_coplanar_partition)
    writer.sized_bytes(source_facet_region_detail::semantic_bytes(
        record.coplanar_partition));
  writer.u64(record.boundary_relation_requests.size());
  for (const auto request : record.boundary_relation_requests)
    writer.u64(request.ordinal());
  writer.floating(record.residual_boundary);
  writer.u32(record.reserved);
  return writer.take();
}

template <class T>
bool valid_source_edge_facet_relation_record(
    const source_edge_facet_relation_record<T> &record) {
  if (record.schema_version !=
          contract_versions::relation_source_edge_facet_schema ||
      record.policy_version !=
          contract_versions::relation_source_edge_facet_policy ||
      !record.owner.anchor || !valid_relation_feature_key(record.edge_feature) ||
      record.edge_feature.kind != relation_feature_kind::source_edge ||
      !valid_relation_feature_key(record.facet_feature) ||
      record.facet_feature.kind != relation_feature_kind::source_facet ||
      record.edge_feature.operand == record.facet_feature.operand ||
      record.facet_feature.primary != record.source_facet ||
      record.facet_feature.secondary != record.ring ||
      (record.material_side != occupied_side::negative &&
       record.material_side != occupied_side::positive) ||
      !finite_bits(record.residual_boundary) || record.residual_boundary < T(0) ||
      record.boundary_relation_requests.size() < 3 || record.reserved != 0)
    return false;
  for (const auto &truth : record.endpoint_support_truth)
    if (!valid_relation_truth_record(truth))
      return false;
  for (std::size_t i = 0; i < record.events.size(); ++i) {
    if (!source_edge_facet_detail::valid_event(
            record.events[i], record.residual_boundary, record.source_facet,
            record.ring) ||
        record.events[i].occurrence != i)
      return false;
    if (i != 0 &&
        !source_edge_relation_detail::definitely_before(
            record.events[i - 1].parameter, record.events[i].parameter))
      return false;
  }
  std::vector<relation_request_id> requests = record.boundary_relation_requests;
  std::sort(requests.begin(), requests.end());
  if (std::adjacent_find(requests.begin(), requests.end()) != requests.end())
    return false;

  switch (record.support) {
  case source_edge_facet_support_class::definitely_separated_same_side:
    if (!source_edge_facet_detail::same_truth_side<T>(
            record.endpoint_support_truth[0],
            record.endpoint_support_truth[1]) ||
        record.contact != source_edge_facet_contact_class::none ||
        !record.events.empty() || record.has_coplanar_partition)
      return false;
    break;
  case source_edge_facet_support_class::transverse_support_crossing:
    if (!source_edge_facet_detail::opposite_truth_side<T>(
            record.endpoint_support_truth[0],
            record.endpoint_support_truth[1]) ||
        record.has_coplanar_partition || record.events.size() > 1 ||
        (record.contact != source_edge_facet_contact_class::none &&
         record.events.size() != 1))
      return false;
    break;
  case source_edge_facet_support_class::endpoint_support_tie: {
    const bool first_tie = source_edge_relation_detail::zero_tie<T>(
        record.endpoint_support_truth[0]);
    const bool second_tie = source_edge_relation_detail::zero_tie<T>(
        record.endpoint_support_truth[1]);
    if (first_tie == second_tie || record.has_coplanar_partition ||
        record.events.size() > 1 ||
        (record.contact != source_edge_facet_contact_class::none &&
         record.events.size() != 1))
      return false;
    break;
  }
  case source_edge_facet_support_class::coplanar_support:
    if (!source_edge_relation_detail::zero_tie<T>(
            record.endpoint_support_truth[0]) ||
        !source_edge_relation_detail::zero_tie<T>(
            record.endpoint_support_truth[1]) ||
        !record.has_coplanar_partition || !record.events.empty() ||
        !valid_source_facet_segment_partition_record(
            record.coplanar_partition) ||
        !record.coplanar_partition.triangle_reconciliation_complete ||
        record.boundary_relation_requests.size() !=
            record.coplanar_partition.boundary_edge_relation_count ||
        record.contact != source_edge_facet_detail::classify_partition_contact(
                              record.coplanar_partition))
      return false;
    break;
  default:
    return false;
  }

  return sha256::digest(encode_source_edge_facet_relation_semantics(record)) ==
         record.semantic_digest;
}

namespace source_edge_facet_detail {

template <class T>
bool record_semantics_equal(const source_edge_facet_relation_record<T> &a,
                            const source_edge_facet_relation_record<T> &b) {
  return encode_source_edge_facet_relation_semantics(a) ==
         encode_source_edge_facet_relation_semantics(b);
}

} // namespace source_edge_facet_detail

template <class T>
boolean_outcome<source_edge_facet_relation_record<T>>
classify_source_edge_facet_relation(const source_edge_facet_input<T> &input,
                                    const context_owner_token &owner,
                                    T residual_boundary) {
  using namespace source_edge_facet_detail;
  static_assert(supported_precision_scalar_v<T>);
  if (!finite_bits(residual_boundary) || residual_boundary < T(0) ||
      !validate_input(input, owner))
    return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_malformed,
            "Component 07 source-edge/source-facet input is malformed"));

  auto first_direction = bounded_vector_subtract(
      input.support[1].coordinates, input.support[0].coordinates);
  auto second_direction = bounded_vector_subtract(
      input.support[2].coordinates, input.support[0].coordinates);
  if (!first_direction.has_value() || !second_direction.has_value())
    return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_support_unresolved,
            "Component 07 source-facet support direction failed"));
  auto normal = bounded_cross3(*first_direction.value(),
                               *second_direction.value());
  if (!normal.has_value())
    return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_support_unresolved,
            "Component 07 source-facet support normal failed"));
  auto normal_norm = bounded_squared_norm(*normal.value());
  if (!normal_norm.has_value() ||
      !source_edge_relation_detail::definite_positive(*normal_norm.value()))
    return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
        source_edge_facet_error(
            relation_subcode::source_edge_facet_support_unresolved,
            "Component 07 source-facet support is not definitely nondegenerate"));

  source_edge_facet_relation_record<T> result;
  result.owner = owner;
  result.edge_feature = input.edge.feature;
  result.facet_feature = input.facet_feature;
  result.source_facet = input.source_facet;
  result.ring = input.ring;
  result.shell = input.shell;
  result.material_side = input.material_side;
  result.residual_boundary = residual_boundary;
  result.boundary_relation_requests.reserve(input.boundary_relations.size());
  for (const auto &boundary : input.boundary_relations)
    result.boundary_relation_requests.push_back(boundary.binding.request);

  for (std::size_t endpoint = 0; endpoint < 2; ++endpoint) {
    auto truth = endpoint_support_truth(input, *normal.value(), endpoint);
    if (!truth.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *truth.error());
    result.endpoint_support_truth[endpoint] = std::move(*truth.value());
  }

  const auto finish = [&]()
      -> boolean_outcome<source_edge_facet_relation_record<T>> {
    result.semantic_digest =
        sha256::digest(encode_source_edge_facet_relation_semantics(result));
    if (!valid_source_edge_facet_relation_record(result))
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_invariant,
              "Component 07 source-edge/source-facet record failed validation"));
    return boolean_outcome<source_edge_facet_relation_record<T>>::success(
        std::move(result));
  };

  if (same_truth_side<T>(result.endpoint_support_truth[0],
                         result.endpoint_support_truth[1])) {
    result.support =
        source_edge_facet_support_class::definitely_separated_same_side;
    result.contact = source_edge_facet_contact_class::none;
    return finish();
  }

  if (opposite_truth_side<T>(result.endpoint_support_truth[0],
                             result.endpoint_support_truth[1])) {
    result.support =
        source_edge_facet_support_class::transverse_support_crossing;
    auto start_offset = bounded_vector_subtract(
        input.edge.start.coordinates, input.support[0].coordinates);
    auto end_offset = bounded_vector_subtract(
        input.edge.end.coordinates, input.support[0].coordinates);
    if (!start_offset.has_value() || !end_offset.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_parameter_unresolved,
              "Component 07 transverse edge/facet support offsets failed"));
    auto start_residual = bounded_dot3(*start_offset.value(), *normal.value());
    auto end_residual = bounded_dot3(*end_offset.value(), *normal.value());
    if (!start_residual.has_value() || !end_residual.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_parameter_unresolved,
              "Component 07 transverse edge/facet support residual failed"));
    auto denominator = bounded_subtract(*start_residual.value(),
                                        *end_residual.value());
    if (!denominator.has_value() ||
        !source_edge_relation_detail::definite_nonzero(*denominator.value()))
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_parameter_unresolved,
              "Component 07 transverse edge/facet denominator is unresolved"));
    auto scalar = bounded_divide(*start_residual.value(),
                                 *denominator.value());
    if (!scalar.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_parameter_unresolved,
              "Component 07 transverse edge/facet parameter division failed"));
    const auto exact_start = exact_coplanarity_3d(
        source_edge_relation_detail::nominal(input.support[0]),
        source_edge_relation_detail::nominal(input.support[2]),
        source_edge_relation_detail::nominal(input.support[1]),
        source_edge_relation_detail::nominal(input.edge.start));
    const auto exact_end = exact_coplanarity_3d(
        source_edge_relation_detail::nominal(input.support[0]),
        source_edge_relation_detail::nominal(input.support[2]),
        source_edge_relation_detail::nominal(input.support[1]),
        source_edge_relation_detail::nominal(input.edge.end));
    auto parameter = source_edge_relation_detail::make_parameter(
        std::move(*scalar.value()), exact_start, exact_end, owner);
    if (!parameter.has_value() ||
        parameter.value()->evidence.domain !=
            parameter_domain_status::stable_interior)
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_parameter_unresolved,
              "Component 07 transverse edge/facet parameter is not stable interior"));
    auto point = bounded_interpolate_from_a(
        input.edge.start, input.edge.end,
        source_edge_relation_detail::as_bounded_parameter(*parameter.value(),
                                                           owner));
    if (!point.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_parameter_unresolved,
              "Component 07 transverse edge/facet construction failed"));
    auto construction = make_construction(
        input, *normal.value(), *point.value(), *parameter.value(),
        residual_boundary, false, 0);
    if (!construction.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *construction.error());
    auto projected = project_point(*point.value(), input.dropped_axis);
    std::vector<std::uint64_t> certified_vertices;
    std::vector<source_facet_boundary_edge_owner> certified_edges;
    bounded_boolean_error ownership_error;
    if (!certified_non_coplanar_boundary_owners(
            input, certified_vertices, certified_edges, ownership_error))
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          ownership_error);
    auto region = classify_source_facet_point(
        input.source_facet, input.ring, projected, false, input.polygon,
        input.polygon_orientation,
        certified_vertices.empty() ? nullptr : &certified_vertices,
        certified_edges.empty() ? nullptr : &certified_edges);
    if (!region.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *region.error());
    if (region.value()->classification ==
        source_facet_point_region_class::outside) {
      result.contact = source_edge_facet_contact_class::none;
      return finish();
    }
    source_edge_facet_event_record<T> event;
    event.parameter = parameter.value()->evidence;
    event.construction = std::move(*construction.value());
    event.region = std::move(*region.value());
    event.before = occupancy_for_truth(result.endpoint_support_truth[0],
                                       input.material_side);
    event.after = occupancy_for_truth(result.endpoint_support_truth[1],
                                      input.material_side);
    event.numeric_crossing = crossing_delta(event.before, event.after);
    event.occurrence = 0;
    if (event.region.classification ==
        source_facet_point_region_class::interior) {
      event.kind = source_edge_facet_event_kind::proper_face_crossing;
      result.contact = source_edge_facet_contact_class::proper_face_crossing;
    } else {
      // A source-boundary hit needs the complete incident source-fan rule before
      // it can own a numeric shell transition. Preserve the local side states,
      // but do not pre-award a crossing to this facet.
      event.kind = source_edge_facet_event_kind::boundary_crossing;
      event.numeric_crossing = 0;
      result.contact = source_edge_facet_contact_class::boundary_crossing;
    }
    result.events.push_back(std::move(event));
    return finish();
  }

  const bool first_tie = source_edge_relation_detail::zero_tie<T>(
      result.endpoint_support_truth[0]);
  const bool second_tie = source_edge_relation_detail::zero_tie<T>(
      result.endpoint_support_truth[1]);
  if (first_tie != second_tie) {
    const std::size_t endpoint = first_tie ? 0 : 1;
    const std::size_t other = endpoint == 0 ? 1 : 0;
    if (!source_edge_relation_detail::accepted_nonzero<T>(
            result.endpoint_support_truth[other]))
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          source_edge_facet_error(
              relation_subcode::source_edge_facet_support_unresolved,
              "Component 07 endpoint edge/facet support state is unresolved"));
    result.support = source_edge_facet_support_class::endpoint_support_tie;
    auto parameter = source_edge_relation_detail::constant_parameter(
        owner, endpoint == 0 ? T(0) : T(1));
    if (!parameter.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *parameter.error());
    const auto &point = endpoint == 0 ? input.edge.start : input.edge.end;
    const auto endpoint_mask = endpoint == 0 ? std::uint8_t{1}
                                             : std::uint8_t{2};
    auto construction = make_construction(
        input, *normal.value(), point, *parameter.value(), residual_boundary,
        true, endpoint_mask);
    if (!construction.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *construction.error());
    auto projected = project_point(point, input.dropped_axis,
                                   input.edge_source_vertices[endpoint]);
    std::vector<std::uint64_t> certified_vertices;
    std::vector<source_facet_boundary_edge_owner> certified_edges;
    bounded_boolean_error ownership_error;
    if (!certified_non_coplanar_boundary_owners(
            input, certified_vertices, certified_edges, ownership_error))
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          ownership_error);
    auto region = classify_source_facet_point(
        input.source_facet, input.ring, projected, false, input.polygon,
        input.polygon_orientation,
        certified_vertices.empty() ? nullptr : &certified_vertices,
        certified_edges.empty() ? nullptr : &certified_edges);
    if (!region.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *region.error());
    if (region.value()->classification ==
        source_facet_point_region_class::outside) {
      result.contact = source_edge_facet_contact_class::none;
      return finish();
    }
    source_edge_facet_event_record<T> event;
    event.parameter = parameter.value()->evidence;
    event.construction = std::move(*construction.value());
    event.region = std::move(*region.value());
    event.before = endpoint == 0
                       ? source_edge_facet_occupancy_state::on_support
                       : occupancy_for_truth(result.endpoint_support_truth[0],
                                             input.material_side);
    event.after = endpoint == 0
                      ? occupancy_for_truth(result.endpoint_support_truth[1],
                                            input.material_side)
                      : source_edge_facet_occupancy_state::on_support;
    event.numeric_crossing = 0;
    event.occurrence = 0;
    if (event.region.classification ==
            source_facet_point_region_class::original_edge ||
        event.region.classification ==
            source_facet_point_region_class::original_vertex) {
      event.kind = source_edge_facet_event_kind::tangent_contact;
      result.contact = source_edge_facet_contact_class::tangent_contact;
    } else {
      event.kind = source_edge_facet_event_kind::endpoint_contact;
      result.contact = source_edge_facet_contact_class::endpoint_contact;
    }
    result.events.push_back(std::move(event));
    return finish();
  }

  if (first_tie && second_tie) {
    result.support = source_edge_facet_support_class::coplanar_support;
    std::vector<source_facet_segment_contact_proposal<T>> contacts;
    contacts.reserve(input.boundary_relations.size());
    bounded_boolean_error contact_error;
    for (const auto &boundary : input.boundary_relations)
      if (!append_coplanar_contacts(input, boundary, contacts, contact_error))
        return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
            contact_error);
    auto partition = partition_source_facet_segment(
        input.source_facet, input.ring,
        project_point(input.edge.start, input.dropped_axis,
                      input.edge_source_vertices[0]),
        false,
        project_point(input.edge.end, input.dropped_axis,
                      input.edge_source_vertices[1]),
        false, input.polygon, input.polygon_orientation,
        input.boundary_relations.size(), true, std::move(contacts));
    if (!partition.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *partition.error());
    auto reconciled = reconcile_source_facet_triangle_local_witnesses(
        std::move(*partition.value()), {});
    if (!reconciled.has_value())
      return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
          *reconciled.error());
    result.has_coplanar_partition = true;
    result.coplanar_partition = std::move(*reconciled.value());
    result.contact = classify_partition_contact(result.coplanar_partition);
    return finish();
  }

  return boolean_outcome<source_edge_facet_relation_record<T>>::failure(
      source_edge_facet_error(
          relation_subcode::source_edge_facet_support_unresolved,
          "Component 07 source-edge/source-facet support pair is unresolved"));
}

namespace candidate_source_edge_facet_detail {

using candidate_source_edge_relation_detail::candidate_operands;
using candidate_source_edge_relation_detail::find_original_edge_primitive;
using candidate_source_edge_relation_detail::make_edge_input;
using candidate_source_edge_relation_detail::operand_topology;
using candidate_source_edge_relation_detail::source_edge_feature;
using candidate_source_edge_relation_detail::valid_original_edge_primitive;

inline relation_feature_key source_facet_feature(operand_id operand,
                                                 std::uint64_t source_facet,
                                                 std::uint64_t ring) {
  relation_feature_key feature;
  feature.operand = operand;
  feature.kind = relation_feature_kind::source_facet;
  feature.primary = source_facet;
  feature.secondary = ring;
  return feature;
}

template <class T, class I>
bool source_edge_vertices(const canonical_candidate_stream<T, I> &candidates,
                          const relation_feature_key &feature,
                          std::array<std::uint64_t, 2> &vertices) {
  const auto *topology = operand_topology(candidates, feature.operand);
  if (!topology)
    return false;
  const auto &table = candidates.primitive_table(feature.operand);
  const broad_phase_edge_primitive<T> *primitive = nullptr;
  for (const auto &candidate : table.edges)
    if (valid_original_edge_primitive(candidate) &&
        source_edge_feature(candidate) == feature) {
      if (primitive)
        return false;
      primitive = &candidate;
    }
  if (!primitive || !valid_original_edge_primitive(*primitive) ||
      primitive->endpoints[0].ordinal() >= topology->vertices().size() ||
      primitive->endpoints[1].ordinal() >= topology->vertices().size())
    return false;
  vertices[0] = topology->vertices()[primitive->endpoints[0].ordinal()].source_vertex;
  vertices[1] = topology->vertices()[primitive->endpoints[1].ordinal()].source_vertex;
  return vertices[0] != vertices[1];
}

inline relation_request_key edge_edge_key(
    const bounded_boolean_digest &semantic_namespace,
    relation_feature_key first, relation_feature_key second) {
  relation_request_key key;
  key.semantic_namespace = semantic_namespace;
  key.family = relation_request_family::source_edge_source_edge;
  key.scope = relation_record_scope::public_source_feature;
  key.first = first;
  key.second = second;
  if (key.second < key.first)
    std::swap(key.first, key.second);
  key.formula_version = contract_versions::exact_relation_formulas;
  key.policy_version = contract_versions::relation_request_key_schema;
  return key;
}

inline const canonical_relation_request *find_request(
    const relation_request_graph &graph, const relation_request_key &key) {
  const auto iterator = std::lower_bound(
      graph.requests.begin(), graph.requests.end(), key,
      [](const canonical_relation_request &record,
         const relation_request_key &candidate) {
        return record.key < candidate;
      });
  return iterator != graph.requests.end() && iterator->key == key
             ? &*iterator
             : nullptr;
}

template <class T, class I>
bool make_facet_input(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const bounded_boolean_digest &semantic_namespace,
    const relation_request_key &request,
    source_edge_facet_input<T> &out) {
  if (request.family != relation_request_family::source_edge_source_facet ||
      request.scope != relation_record_scope::public_source_feature ||
      request.first.kind != relation_feature_kind::source_edge ||
      request.second.kind != relation_feature_kind::source_facet ||
      request.first.operand == request.second.operand ||
      !make_edge_input(candidates, request.first, out.edge) ||
      !source_edge_vertices(candidates, request.first,
                            out.edge_source_vertices))
    return false;

  const auto *topology = operand_topology(candidates, request.second.operand);
  if (!topology || request.second.primary >=
                       topology->source_facet_to_group().size())
    return false;
  const auto group_ordinal =
      topology->source_facet_to_group()[request.second.primary];
  if (group_ordinal >= topology->facet_groups().size())
    return false;
  const auto &group = topology->facet_groups()[group_ordinal];
  if (group.canonical_id != group_ordinal ||
      group.source_facet != request.second.primary ||
      group.ring != request.second.secondary || group.basis.dropped_axis > 2 ||
      group.source_vertices.size() < 3 ||
      group.boundary_halfedges.size() != group.source_vertices.size() ||
      group.shell >= topology->source_shell_to_group().size())
    return false;
  const auto shell_group_ordinal =
      topology->source_shell_to_group()[group.shell];
  if (shell_group_ordinal >= topology->shell_groups().size())
    return false;
  const auto &shell = topology->shell_groups()[shell_group_ordinal];
  if (shell.canonical_id != shell_group_ordinal ||
      shell.source_shell != group.shell)
    return false;

  out.facet_feature = request.second;
  out.source_facet = group.source_facet;
  out.ring = group.ring;
  out.shell = group.shell;
  out.material_side = shell.material_side;
  out.dropped_axis = group.basis.dropped_axis;
  out.polygon.clear();
  out.polygon.reserve(group.source_vertices.size());
  for (std::size_t corner = 0; corner < group.source_vertices.size(); ++corner) {
    const auto source_vertex = group.source_vertices[corner];
    if (source_vertex >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex_ordinal =
        topology->source_vertex_to_vertex()[source_vertex];
    if (vertex_ordinal >= topology->vertices().size())
      return false;
    bounded_point3<T> point;
    if (!candidate_source_edge_relation_detail::import_vertex_point(
            topology->vertices()[vertex_ordinal], request.second.operand,
            candidates.owner(), point))
      return false;
    out.polygon.push_back(source_edge_facet_detail::project_point(
        point, out.dropped_axis, source_vertex, corner));
  }
  const auto orientation =
      bounded_source_polygon_kernel<T>::polygon_orientation(out.polygon);
  if (!orientation || !valid_source_orientation_evidence(*orientation) ||
      orientation->bounded_sign == bounded_planar_sign::uncertain ||
      orientation->exact_sign != static_cast<int>(orientation->bounded_sign))
    return false;
  out.polygon_orientation = orientation->bounded_sign;

  for (std::size_t support = 0; support < 3; ++support) {
    const auto source_vertex = group.basis.support_vertices[support];
    if (source_vertex >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex_ordinal =
        topology->source_vertex_to_vertex()[source_vertex];
    if (vertex_ordinal >= topology->vertices().size() ||
        !candidate_source_edge_relation_detail::import_vertex_point(
            topology->vertices()[vertex_ordinal], request.second.operand,
            candidates.owner(), out.support[support]))
      return false;
  }

  const auto &facet_edge_table =
      candidates.primitive_table(request.second.operand);
  out.boundary_relations.clear();
  out.boundary_relations.reserve(group.boundary_halfedges.size());
  std::vector<bool> consumed_boundary_halfedges(group.boundary_halfedges.size(),
                                                 false);
  for (std::size_t edge_ordinal = 0;
       edge_ordinal < group.source_vertices.size(); ++edge_ordinal) {
    const auto source_origin = group.source_vertices[edge_ordinal];
    const auto source_destination =
        group.source_vertices[(edge_ordinal + 1) % group.source_vertices.size()];
    const canonical_manifold_halfedge_record *ordered_halfedge = nullptr;
    std::size_t ordered_boundary_index = 0;
    for (std::size_t boundary_index = 0;
         boundary_index < group.boundary_halfedges.size(); ++boundary_index) {
      const auto halfedge_ordinal = group.boundary_halfedges[boundary_index];
      if (halfedge_ordinal >= topology->halfedges().size())
        return false;
      const auto &candidate_halfedge = topology->halfedges()[halfedge_ordinal];
      if (candidate_halfedge.canonical_id != halfedge_ordinal ||
          candidate_halfedge.source_facet != group.source_facet ||
          candidate_halfedge.edge >= topology->edges().size())
        return false;
      if (candidate_halfedge.source_origin == source_origin &&
          candidate_halfedge.source_destination == source_destination) {
        if (ordered_halfedge)
          return false;
        ordered_halfedge = &candidate_halfedge;
        ordered_boundary_index = boundary_index;
      }
    }
    if (!ordered_halfedge || consumed_boundary_halfedges[ordered_boundary_index])
      return false;
    consumed_boundary_halfedges[ordered_boundary_index] = true;

    const auto &edge = topology->edges()[ordered_halfedge->edge];
    if (edge.canonical_id != ordered_halfedge->edge ||
        edge.edge_class != canonical_edge_class::source_edge ||
        !edge.source_feature_owner)
      return false;
    const auto *primitive = find_original_edge_primitive(
        facet_edge_table, manifold_edge_id{edge.canonical_id});
    if (!primitive)
      return false;
    const auto feature = source_edge_feature(*primitive);
    const auto dependency_key = edge_edge_key(
        semantic_namespace, request.first, feature);
    const auto *dependency = find_request(edge_stage.request_graph,
                                          dependency_key);
    if (!dependency || dependency->id.ordinal() >= edge_stage.relations.size())
      return false;

    source_edge_facet_boundary_relation<T> boundary;
    boundary.binding.request = dependency->id;
    boundary.binding.feature = feature;
    boundary.binding.owner = {edge_ordinal, source_origin, source_destination};
    if (!source_edge_vertices(candidates, feature,
                              boundary.binding.parameter_source_vertices))
      return false;
    boundary.relation = edge_stage.relations[dependency->id.ordinal()];
    out.boundary_relations.push_back(std::move(boundary));
  }
  if (std::find(consumed_boundary_halfedges.begin(),
                consumed_boundary_halfedges.end(), false) !=
      consumed_boundary_halfedges.end())
    return false;
  return source_edge_facet_detail::validate_input(out, candidates.owner());
}

template <class T, class I>
bool append_candidate_proposals(
    const canonical_candidate_stream<T, I> &candidates,
    const bounded_boolean_digest &semantic_namespace,
    std::vector<relation_request_proposal> &proposals,
    const relation_capabilities &capabilities,
    bounded_boolean_error &error) {
  const auto &records = candidates.candidates();
  for (std::size_t ordinal = 0; ordinal < records.size(); ++ordinal) {
    if (relation_cancelled(capabilities)) {
      error = source_edge_facet_error(
          relation_subcode::cancelled,
          "Component 07 edge/facet request discovery cancelled",
          relation_checkpoint::candidate_scan,
          bounded_boolean_error_category::cancelled);
      return false;
    }
    const auto &candidate = records[ordinal];
    operand_id edge_operand = operand_id::a;
    operand_id triangle_operand = operand_id::b;
    if (candidate.id.ordinal() != ordinal || candidate.ordinal != ordinal ||
        candidate.family !=
            broad_phase_relation_family::canonical_edge_source_triangle ||
        candidate.filter_reason != topological_filter_reason::not_filtered ||
        candidate.reserved != 0 ||
        !candidate_operands<T, I>(candidate, edge_operand, triangle_operand)) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_malformed,
          "Component 07 edge/facet candidate is malformed",
          relation_checkpoint::candidate_scan);
      return false;
    }
    const auto &edge_table = candidates.primitive_table(edge_operand);
    const auto &triangle_table = candidates.primitive_table(triangle_operand);
    if (candidate.edge.ordinal() >= edge_table.edges.size() ||
        candidate.triangle.ordinal() >= triangle_table.triangles.size()) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_malformed,
          "Component 07 edge/facet candidate reference is out of range",
          relation_checkpoint::candidate_scan);
      return false;
    }
    const auto &edge = edge_table.edges[candidate.edge.ordinal()];
    const auto &triangle = triangle_table.triangles[candidate.triangle.ordinal()];
    if (edge.edge_class == canonical_edge_class::facet_internal_diagonal)
      continue;
    if (!valid_original_edge_primitive(edge) ||
        triangle.source_facet == broad_phase_invalid_ordinal) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_malformed,
          "Component 07 edge/facet public feature handshake failed",
          relation_checkpoint::candidate_scan);
      return false;
    }
    relation_request_proposal proposal;
    proposal.key.semantic_namespace = semantic_namespace;
    proposal.key.family = relation_request_family::source_edge_source_facet;
    proposal.key.scope = relation_record_scope::public_source_feature;
    proposal.key.first = source_edge_feature(edge);
    proposal.key.second = source_facet_feature(
        triangle_operand, triangle.source_facet, triangle.ring);
    proposal.key.formula_version = contract_versions::exact_relation_formulas;
    proposal.key.policy_version = contract_versions::relation_request_key_schema;
    proposal.candidate_witnesses.push_back(candidate.id);
    if (!valid_relation_request_key(proposal.key)) {
      error = source_edge_facet_error(
          relation_subcode::source_edge_facet_malformed,
          "Component 07 generated an invalid edge/facet request key",
          relation_checkpoint::candidate_scan);
      return false;
    }
    if (proposals.size() >= capabilities.maximum_requests) {
      error = source_edge_facet_error(
          relation_subcode::work_limit,
          "Component 07 edge/facet request limit exceeded",
          relation_checkpoint::count_representability_preflight,
          bounded_boolean_error_category::resource_limit);
      return false;
    }
    proposals.push_back(std::move(proposal));
  }
  return true;
}

} // namespace candidate_source_edge_facet_detail

template <class T>
std::vector<std::uint8_t>
encode_candidate_source_edge_facet_relation_semantics(
    const candidate_source_edge_facet_relation_stage<T> &stage) {
  canonical_writer writer;
  writer.u32(0x37464643U); // CFF7
  writer.u16(stage.schema_version);
  writer.u16(stage.policy_version);
  writer.sized_bytes(encode_relation_request_graph_semantics(stage.request_graph));
  writer.u64(stage.relations.size());
  for (const auto &relation : stage.relations)
    writer.sized_bytes(encode_source_edge_facet_relation_semantics(relation));
  writer.u64(stage.candidate_relations.size());
  for (const auto request : stage.candidate_relations)
    writer.u64(request.ordinal());
  writer.u64(stage.candidate_ranges.size());
  for (const auto &range : stage.candidate_ranges) {
    writer.u64(range.candidate.ordinal());
    writer.u64(range.begin);
    writer.u64(range.count);
    writer.u32(range.reserved);
  }
  writer.u64(stage.evaluation_count);
  writer.u32(stage.reserved);
  return writer.take();
}

template <class T, class I>
bool verify_candidate_source_edge_facet_relation_stage(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const bounded_boolean_digest &semantic_namespace, T residual_boundary,
    const relation_capabilities &capabilities,
    const candidate_source_edge_facet_relation_stage<T> &stage,
    bounded_boolean_error &error) {
  using namespace candidate_source_edge_facet_detail;
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = source_edge_facet_error(subcode, summary,
                                    relation_checkpoint::producer_verification);
    return false;
  };
  if (stage.schema_version !=
          contract_versions::relation_source_edge_facet_stage_schema ||
      stage.policy_version !=
          contract_versions::relation_source_edge_facet_stage_policy ||
      stage.reserved != 0 || !stage.owner.same_owner(capabilities.owner) ||
      !stage.owner.same_owner(candidates.owner()) ||
      !stage.request_graph.owner.same_owner(stage.owner) ||
      stage.evaluation_count != stage.relations.size() ||
      stage.relations.size() != stage.request_graph.requests.size() ||
      stage.candidate_ranges.size() != candidates.candidates().size())
    return fail(relation_subcode::source_edge_facet_invariant,
                "Component 07 edge/facet integration header is malformed");

  bounded_boolean_error edge_error;
  if (!verify_candidate_source_edge_relation_stage(
          candidates, semantic_namespace, residual_boundary, capabilities,
          edge_stage, edge_error)) {
    error = edge_error;
    return false;
  }
  std::vector<relation_request_proposal> proposals;
  if (!append_candidate_proposals(candidates, semantic_namespace, proposals,
                                  capabilities, error))
    return false;
  auto reconstructed = build_relation_request_graph(std::move(proposals),
                                                     capabilities);
  if (!reconstructed.has_value()) {
    error = *reconstructed.error();
    return false;
  }
  if (encode_relation_request_graph_semantics(*reconstructed.value()) !=
      encode_relation_request_graph_semantics(stage.request_graph))
    return fail(relation_subcode::source_edge_facet_invariant,
                "Component 07 edge/facet request graph did not reconstruct");

  for (std::size_t i = 0; i < stage.relations.size(); ++i) {
    const auto &request = stage.request_graph.requests[i];
    if (request.id.ordinal() != i ||
        request.key.family !=
            relation_request_family::source_edge_source_facet ||
        request.key.first.kind != relation_feature_kind::source_edge ||
        request.key.second.kind != relation_feature_kind::source_facet)
      return fail(relation_subcode::source_edge_facet_invariant,
                  "Component 07 edge/facet producer request is malformed");
    source_edge_facet_input<T> input;
    if (!make_facet_input(candidates, edge_stage, semantic_namespace,
                          request.key, input))
      return fail(relation_subcode::source_edge_facet_malformed,
                  "Component 07 edge/facet verifier could not reconstruct input");
    auto reevaluated = classify_source_edge_facet_relation(
        input, capabilities.owner, residual_boundary);
    if (!reevaluated.has_value()) {
      error = *reevaluated.error();
      return false;
    }
    if (!valid_source_edge_facet_relation_record(stage.relations[i]) ||
        !source_edge_facet_detail::record_semantics_equal(
            stage.relations[i], *reevaluated.value()))
      return fail(relation_subcode::source_edge_facet_invariant,
                  "Component 07 edge/facet numerical record did not reconstruct");
  }

  std::vector<std::vector<relation_request_id>> expected(
      candidates.candidates().size());
  for (const auto &request : stage.request_graph.requests) {
    if (request.witness_begin > stage.request_graph.candidate_witnesses.size() ||
        request.witness_count >
            stage.request_graph.candidate_witnesses.size() -
                request.witness_begin)
      return fail(relation_subcode::source_edge_facet_invariant,
                  "Component 07 edge/facet witness range is malformed");
    for (std::uint64_t offset = 0; offset < request.witness_count; ++offset) {
      const auto witness = stage.request_graph.candidate_witnesses[
          request.witness_begin + offset];
      if (witness.ordinal() >= expected.size())
        return fail(relation_subcode::source_edge_facet_invariant,
                    "Component 07 edge/facet witness is out of range");
      expected[witness.ordinal()].push_back(request.id);
    }
  }
  std::uint64_t expected_begin = 0;
  for (std::size_t candidate = 0; candidate < expected.size(); ++candidate) {
    auto &links = expected[candidate];
    std::sort(links.begin(), links.end());
    links.erase(std::unique(links.begin(), links.end()), links.end());
    const auto &range = stage.candidate_ranges[candidate];
    if (range.candidate.ordinal() != candidate ||
        range.begin != expected_begin || range.count != links.size() ||
        range.reserved != 0 || range.begin > stage.candidate_relations.size() ||
        range.count > stage.candidate_relations.size() - range.begin)
      return fail(relation_subcode::source_edge_facet_invariant,
                  "Component 07 edge/facet candidate range is malformed");
    for (std::size_t offset = 0; offset < links.size(); ++offset)
      if (stage.candidate_relations[range.begin + offset] != links[offset])
        return fail(relation_subcode::source_edge_facet_invariant,
                    "Component 07 edge/facet candidate coverage is incomplete");
    expected_begin += links.size();
  }
  if (expected_begin != stage.candidate_relations.size())
    return fail(relation_subcode::source_edge_facet_invariant,
                "Component 07 edge/facet links contain trailing data");
  if (stage.semantic_digest != sha256::digest(
                                   encode_candidate_source_edge_facet_relation_semantics(
                                       stage)))
    return fail(relation_subcode::digest_mismatch,
                "Component 07 edge/facet integration digest mismatch");
  auto owner_changed = stage;
  owner_changed.owner = context_owner_token::create();
  owner_changed.request_graph.owner = owner_changed.owner;
  if (encode_candidate_source_edge_facet_relation_semantics(owner_changed) !=
      encode_candidate_source_edge_facet_relation_semantics(stage))
    return fail(relation_subcode::owner_in_semantics,
                "Component 07 edge/facet integration encoded a runtime owner");
  return true;
}

template <class T, class I>
boolean_outcome<candidate_source_edge_facet_relation_stage<T>>
build_candidate_source_edge_facet_relations(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const bounded_boolean_digest &semantic_namespace, T residual_boundary,
    const relation_capabilities &capabilities) {
  using stage_type = candidate_source_edge_facet_relation_stage<T>;
  using namespace candidate_source_edge_facet_detail;
  static_assert(supported_precision_scalar_v<T>);
  try {
    if (!capabilities.owner.anchor ||
        !capabilities.owner.same_owner(candidates.owner()) ||
        !capabilities.owner.same_owner(edge_stage.owner) ||
        !finite_bits(residual_boundary) || residual_boundary < T(0))
      return boolean_outcome<stage_type>::failure(source_edge_facet_error(
          relation_subcode::source_edge_facet_malformed,
          "Component 07 edge/facet integration handshake failed",
          relation_checkpoint::predecessor_validation));
    bounded_boolean_error edge_error;
    if (!verify_candidate_source_edge_relation_stage(
            candidates, semantic_namespace, residual_boundary, capabilities,
            edge_stage, edge_error))
      return boolean_outcome<stage_type>::failure(edge_error);

    std::vector<relation_request_proposal> proposals;
    proposals.reserve(std::min<std::uint64_t>(
        candidates.candidates().size(), capabilities.maximum_requests));
    bounded_boolean_error discovery_error;
    if (!append_candidate_proposals(candidates, semantic_namespace, proposals,
                                    capabilities, discovery_error))
      return boolean_outcome<stage_type>::failure(discovery_error);
    auto graph = build_relation_request_graph(std::move(proposals), capabilities);
    if (!graph.has_value())
      return boolean_outcome<stage_type>::failure(*graph.error());
    if (graph.value()->requests.size() > capabilities.maximum_relations)
      return boolean_outcome<stage_type>::failure(source_edge_facet_error(
          relation_subcode::work_limit,
          "Component 07 edge/facet relation limit exceeded",
          relation_checkpoint::count_representability_preflight,
          bounded_boolean_error_category::resource_limit));

    stage_type stage;
    stage.owner = capabilities.owner;
    stage.request_graph = std::move(*graph.value());
    stage.relations.reserve(stage.request_graph.requests.size());
    for (const auto &request : stage.request_graph.requests) {
      if (relation_cancelled(capabilities))
        return boolean_outcome<stage_type>::failure(source_edge_facet_error(
            relation_subcode::cancelled,
            "Component 07 edge/facet evaluation cancelled",
            relation_checkpoint::edge_facet_evaluation,
            bounded_boolean_error_category::cancelled));
      source_edge_facet_input<T> input;
      if (!make_facet_input(candidates, edge_stage, semantic_namespace,
                            request.key, input))
        return boolean_outcome<stage_type>::failure(source_edge_facet_error(
            relation_subcode::source_edge_facet_boundary_coverage,
            "Component 07 edge/facet producer could not reconstruct complete facet input"));
      auto relation = classify_source_edge_facet_relation(
          input, capabilities.owner, residual_boundary);
      if (!relation.has_value())
        return boolean_outcome<stage_type>::failure(*relation.error());
      stage.relations.push_back(std::move(*relation.value()));
      ++stage.evaluation_count;
    }

    std::vector<std::vector<relation_request_id>> candidate_links(
        candidates.candidates().size());
    for (const auto &request : stage.request_graph.requests)
      for (std::uint64_t offset = 0; offset < request.witness_count; ++offset) {
        const auto witness = stage.request_graph.candidate_witnesses[
            request.witness_begin + offset];
        if (witness.ordinal() >= candidate_links.size())
          return boolean_outcome<stage_type>::failure(source_edge_facet_error(
              relation_subcode::source_edge_facet_invariant,
              "Component 07 edge/facet witness is out of range",
              relation_checkpoint::event_seed_and_disposition_reconciliation));
        candidate_links[witness.ordinal()].push_back(request.id);
      }
    stage.candidate_ranges.reserve(candidate_links.size());
    for (std::size_t candidate = 0; candidate < candidate_links.size();
         ++candidate) {
      auto &links = candidate_links[candidate];
      std::sort(links.begin(), links.end());
      links.erase(std::unique(links.begin(), links.end()), links.end());
      candidate_source_edge_facet_relation_range range;
      range.candidate = candidate_id(candidate);
      range.begin = stage.candidate_relations.size();
      range.count = links.size();
      stage.candidate_relations.insert(stage.candidate_relations.end(),
                                       links.begin(), links.end());
      stage.candidate_ranges.push_back(range);
    }
    const auto semantic_bytes =
        encode_candidate_source_edge_facet_relation_semantics(stage);
    if (semantic_bytes.size() > capabilities.maximum_canonical_bytes)
      return boolean_outcome<stage_type>::failure(source_edge_facet_error(
          relation_subcode::byte_count_overflow,
          "Component 07 edge/facet integration bytes exceed capabilities",
          relation_checkpoint::canonical_encoding,
          bounded_boolean_error_category::resource_limit));
    stage.semantic_digest = sha256::digest(semantic_bytes);
    bounded_boolean_error verification_error;
    if (!verify_candidate_source_edge_facet_relation_stage(
            candidates, edge_stage, semantic_namespace, residual_boundary,
            capabilities, stage, verification_error))
      return boolean_outcome<stage_type>::failure(verification_error);
    return boolean_outcome<stage_type>::success(std::move(stage));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<stage_type>::failure(source_edge_facet_error(
        relation_subcode::resource_preflight,
        "Component 07 edge/facet integration allocation failed",
        relation_checkpoint::discovery_resource_reservation,
        bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<stage_type>::failure(source_edge_facet_error(
        relation_subcode::internal_invariant,
        "Component 07 edge/facet integration raised an unexpected exception",
        relation_checkpoint::producer_verification));
  }
}

} // namespace ygor::mesh_boolean::bounded
