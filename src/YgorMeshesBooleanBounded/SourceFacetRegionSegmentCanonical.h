#pragma once

#include "SourceFacetRegionSegmentTypes.h"

namespace ygor::mesh_boolean::bounded {

namespace source_facet_region_detail {

template <class T>
bool valid_edge_owner_for_polygon(
    const source_facet_boundary_edge_owner &owner,
    const std::vector<projected_source_point<T>> &polygon) {
  if (owner.edge_ordinal >= polygon.size())
    return false;
  const auto &a = polygon[owner.edge_ordinal];
  const auto &b = polygon[(owner.edge_ordinal + 1) % polygon.size()];
  return owner.origin_source_vertex == a.source_vertex &&
         owner.destination_source_vertex == b.source_vertex &&
         owner.origin_source_vertex != owner.destination_source_vertex;
}

template <class T>
bool valid_contact_proposal(
    const source_facet_segment_contact_proposal<T> &contact,
    const projected_source_point<T> &segment_start,
    const projected_source_point<T> &segment_end,
    const std::vector<projected_source_point<T>> &polygon) {
  if (contact.schema_version !=
          contract_versions::relation_source_facet_segment_schema ||
      contact.lineage == 0 || contact.reserved != 0 ||
      !unit_parameter(contact.first_parameter,
                      contact.first_rounded_parameter) ||
      !valid_projected_point(contact.first_point) ||
      !exact_point_on_query_segment(segment_start, segment_end,
                                    contact.first_point))
    return false;

  auto first_vertices = contact.first_source_vertex_owners;
  auto first_edges = contact.first_source_edge_owners;
  auto second_vertices = contact.second_source_vertex_owners;
  auto second_edges = contact.second_source_edge_owners;
  auto overlap_edges = contact.overlap_source_edge_owners;
  canonicalize_owners(first_vertices, first_edges);
  canonicalize_owners(second_vertices, second_edges);
  std::vector<std::uint64_t> no_vertices;
  canonicalize_owners(no_vertices, overlap_edges);
  if (first_vertices != contact.first_source_vertex_owners ||
      first_edges != contact.first_source_edge_owners ||
      second_vertices != contact.second_source_vertex_owners ||
      second_edges != contact.second_source_edge_owners ||
      overlap_edges != contact.overlap_source_edge_owners)
    return false;
  for (const auto &edge : first_edges)
    if (!valid_edge_owner_for_polygon(edge, polygon))
      return false;
  for (const auto &edge : second_edges)
    if (!valid_edge_owner_for_polygon(edge, polygon))
      return false;
  for (const auto &edge : overlap_edges)
    if (!valid_edge_owner_for_polygon(edge, polygon))
      return false;

  switch (contact.kind) {
  case source_facet_segment_contact_kind::point_contact:
    return interval_equal_bits(contact.first_parameter,
                               contact.second_parameter) &&
           to_bits(contact.first_rounded_parameter) ==
               to_bits(contact.second_rounded_parameter) &&
           same_projected_geometry(contact.first_point,
                                   contact.second_point) &&
           contact.second_source_vertex_owners.empty() &&
           contact.second_source_edge_owners.empty() &&
           contact.overlap_source_edge_owners.empty() &&
           (!contact.first_source_vertex_owners.empty() ||
            !contact.first_source_edge_owners.empty());
  case source_facet_segment_contact_kind::boundary_overlap:
    return unit_parameter(contact.second_parameter,
                          contact.second_rounded_parameter) &&
           valid_projected_point(contact.second_point) &&
           exact_point_on_query_segment(segment_start, segment_end,
                                        contact.second_point) &&
           definitely_before(contact.first_parameter,
                             contact.second_parameter) &&
           !contact.overlap_source_edge_owners.empty();
  }
  return false;
}

template <class T> struct segment_position_seed final {
  finite_interval<T> parameter{};
  T rounded_parameter = T(0);
  projected_source_point<T> point{};
  bool point_source_identity_valid = false;
  std::vector<std::uint64_t> expected_vertex_owners;
  std::vector<source_facet_boundary_edge_owner> expected_edge_owners;
  std::vector<std::uint64_t> contact_lineages;
  std::uint8_t endpoint_mask = 0;
};

template <class T>
boolean_outcome<std::vector<segment_position_seed<T>>>
canonicalize_positions(std::vector<segment_position_seed<T>> seeds) {
  std::vector<segment_position_seed<T>> positions;
  positions.reserve(seeds.size());
  for (auto &seed : seeds) {
    canonicalize_owners(seed.expected_vertex_owners,
                        seed.expected_edge_owners);
    std::sort(seed.contact_lineages.begin(), seed.contact_lineages.end());
    seed.contact_lineages.erase(
        std::unique(seed.contact_lineages.begin(),
                    seed.contact_lineages.end()),
        seed.contact_lineages.end());

    bool inserted = false;
    for (auto iterator = positions.begin(); iterator != positions.end();
         ++iterator) {
      if (definitely_before(seed.parameter, iterator->parameter)) {
        positions.insert(iterator, std::move(seed));
        inserted = true;
        break;
      }
      if (definitely_before(iterator->parameter, seed.parameter))
        continue;

      if (!interval_equal_bits(seed.parameter, iterator->parameter) ||
          !singleton(seed.parameter) ||
          !same_projected_geometry(seed.point, iterator->point) ||
          to_bits(seed.rounded_parameter) !=
              to_bits(iterator->rounded_parameter))
        return boolean_outcome<std::vector<segment_position_seed<T>>>::failure(
            source_facet_region_error(
                relation_subcode::source_facet_segment_order_unresolved,
                "Component 07 segment contact parameter ordering is unresolved"));

      iterator->endpoint_mask =
          static_cast<std::uint8_t>(iterator->endpoint_mask |
                                    seed.endpoint_mask);
      iterator->point_source_identity_valid =
          iterator->point_source_identity_valid ||
          seed.point_source_identity_valid;
      iterator->expected_vertex_owners.insert(
          iterator->expected_vertex_owners.end(),
          seed.expected_vertex_owners.begin(),
          seed.expected_vertex_owners.end());
      iterator->expected_edge_owners.insert(
          iterator->expected_edge_owners.end(),
          seed.expected_edge_owners.begin(),
          seed.expected_edge_owners.end());
      iterator->contact_lineages.insert(iterator->contact_lineages.end(),
                                        seed.contact_lineages.begin(),
                                        seed.contact_lineages.end());
      canonicalize_owners(iterator->expected_vertex_owners,
                          iterator->expected_edge_owners);
      std::sort(iterator->contact_lineages.begin(),
                iterator->contact_lineages.end());
      iterator->contact_lineages.erase(
          std::unique(iterator->contact_lineages.begin(),
                      iterator->contact_lineages.end()),
          iterator->contact_lineages.end());
      inserted = true;
      break;
    }
    if (!inserted)
      positions.push_back(std::move(seed));
  }

  for (std::size_t i = 1; i < positions.size(); ++i)
    if (!definitely_before(positions[i - 1].parameter,
                           positions[i].parameter))
      return boolean_outcome<std::vector<segment_position_seed<T>>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_order_unresolved,
              "Component 07 canonical segment breakpoint order is unresolved"));
  return boolean_outcome<std::vector<segment_position_seed<T>>>::success(
      std::move(positions));
}

template <class T>
std::vector<std::uint8_t> semantic_bytes(
    const source_facet_segment_partition_record<T> &record) {
  canonical_writer writer;
  writer.u16(record.schema_version);
  writer.u16(record.policy_version);
  writer.u16(record.witness_policy_version);
  writer.u16(record.reconciliation_policy_version);
  writer.u64(record.source_facet);
  writer.u64(record.ring);
  encode_projected_point(writer, record.segment_start);
  encode_projected_point(writer, record.segment_end);
  writer.boolean(record.segment_start_source_identity_valid);
  writer.boolean(record.segment_end_source_identity_valid);
  writer.boolean(record.complete_boundary_contact_set);
  writer.u64(record.boundary_edge_relation_count);
  writer.u8(static_cast<std::uint8_t>(
      record.polygon_orientation_evidence.exact_sign + 1));
  writer.u8(static_cast<std::uint8_t>(
      static_cast<std::int8_t>(
          record.polygon_orientation_evidence.bounded_sign) +
      1));
  encode_interval(writer,
                  record.polygon_orientation_evidence.determinant);

  writer.u64(record.contacts.size());
  for (const auto &contact : record.contacts) {
    writer.u8(static_cast<std::uint8_t>(contact.kind));
    writer.u64(contact.lineage);
    writer.floating(contact.first_rounded_parameter);
    encode_interval(writer, contact.first_parameter);
    encode_projected_point(writer, contact.first_point);
    writer.boolean(contact.first_point_source_identity_valid);
    writer.u64(contact.first_source_vertex_owners.size());
    for (const auto owner : contact.first_source_vertex_owners)
      writer.u64(owner);
    writer.u64(contact.first_source_edge_owners.size());
    for (const auto &owner : contact.first_source_edge_owners)
      encode_edge_owner(writer, owner);
    writer.floating(contact.second_rounded_parameter);
    encode_interval(writer, contact.second_parameter);
    encode_projected_point(writer, contact.second_point);
    writer.boolean(contact.second_point_source_identity_valid);
    writer.u64(contact.second_source_vertex_owners.size());
    for (const auto owner : contact.second_source_vertex_owners)
      writer.u64(owner);
    writer.u64(contact.second_source_edge_owners.size());
    for (const auto &owner : contact.second_source_edge_owners)
      encode_edge_owner(writer, owner);
    writer.u64(contact.overlap_source_edge_owners.size());
    for (const auto &owner : contact.overlap_source_edge_owners)
      encode_edge_owner(writer, owner);
  }

  writer.u64(record.breakpoints.size());
  for (const auto &breakpoint : record.breakpoints) {
    writer.floating(breakpoint.rounded_parameter);
    encode_interval(writer, breakpoint.parameter);
    encode_projected_point(writer, breakpoint.point);
    writer.u8(static_cast<std::uint8_t>(
        breakpoint.region.classification));
    writer.u64(breakpoint.region.source_vertex_owners.size());
    for (const auto owner : breakpoint.region.source_vertex_owners)
      writer.u64(owner);
    writer.u64(breakpoint.region.source_edge_owners.size());
    for (const auto &owner : breakpoint.region.source_edge_owners)
      encode_edge_owner(writer, owner);
    writer.u64(breakpoint.contact_lineages.size());
    for (const auto lineage : breakpoint.contact_lineages)
      writer.u64(lineage);
    writer.u8(breakpoint.segment_endpoint_mask);
  }

  writer.u64(record.intervals.size());
  for (const auto &interval : record.intervals) {
    writer.u64(interval.left_breakpoint);
    writer.u64(interval.right_breakpoint);
    writer.floating(interval.rounded_witness_parameter);
    encode_interval(writer, interval.witness_parameter);
    encode_projected_point(writer, interval.witness_point);
    writer.u8(static_cast<std::uint8_t>(interval.classification));
    writer.u64(interval.source_edge_owners.size());
    for (const auto &owner : interval.source_edge_owners)
      encode_edge_owner(writer, owner);
    writer.u32(interval.dyadic_attempt_ordinal);
  }
  return writer.take();
}

} // namespace source_facet_region_detail

} // namespace ygor::mesh_boolean::bounded
