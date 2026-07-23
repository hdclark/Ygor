#pragma once

#include "SourceFacetRegionSegmentCanonical.h"

namespace ygor::mesh_boolean::bounded {

namespace source_facet_region_detail {
template <class T>
source_facet_point_region_class interval_public_point_class(
    source_facet_segment_interval_class classification) {
  switch (classification) {
  case source_facet_segment_interval_class::interior:
    return source_facet_point_region_class::interior;
  case source_facet_segment_interval_class::outside:
    return source_facet_point_region_class::outside;
  case source_facet_segment_interval_class::original_edge_overlap:
    return source_facet_point_region_class::original_edge;
  }
  return source_facet_point_region_class::outside;
}

template <class T>
bool triangle_witness_valid(
    const source_facet_triangle_local_witness<T> &witness,
    const source_facet_segment_partition_record<T> &record) {
  if (witness.schema_version !=
          contract_versions::relation_triangle_local_reconciliation_schema ||
      witness.triangle == 0 || witness.local_witness == 0 ||
      witness.reserved != 0 || !interval_valid(witness.parameter) ||
      !nonzero_digest(witness.exact_triangulation_digest))
    return false;

  auto vertices = witness.source_vertex_owners;
  auto edges = witness.source_edge_owners;
  canonicalize_owners(vertices, edges);
  if (vertices != witness.source_vertex_owners ||
      edges != witness.source_edge_owners)
    return false;

  switch (witness.absorption) {
  case source_facet_triangle_absorption_kind::public_breakpoint: {
    if (witness.local_edge_role !=
            source_triangle_edge_role::source_boundary ||
        witness.internal_diagonal != 0 ||
        witness.public_index >= record.breakpoints.size())
      return false;
    const auto &target = record.breakpoints[witness.public_index];
    return intervals_overlap(witness.parameter, target.parameter) &&
           witness.semantic_classification ==
               target.region.classification &&
           owner_subset(witness.source_vertex_owners,
                        witness.source_edge_owners, target.region);
  }
  case source_facet_triangle_absorption_kind::public_interval: {
    if (witness.local_edge_role !=
            source_triangle_edge_role::source_boundary ||
        witness.internal_diagonal != 0 ||
        witness.public_index >= record.intervals.size())
      return false;
    const auto &target = record.intervals[witness.public_index];
    const auto &left = record.breakpoints[target.left_breakpoint];
    const auto &right = record.breakpoints[target.right_breakpoint];
    return point_strictly_between(witness.parameter, left.parameter,
                                  right.parameter) &&
           witness.semantic_classification ==
               interval_public_point_class<T>(target.classification) &&
           (target.classification ==
                    source_facet_segment_interval_class::original_edge_overlap
                ? std::all_of(
                      witness.source_edge_owners.begin(),
                      witness.source_edge_owners.end(),
                      [&target](const source_facet_boundary_edge_owner &owner) {
                        return std::binary_search(
                            target.source_edge_owners.begin(),
                            target.source_edge_owners.end(), owner);
                      })
                : witness.source_vertex_owners.empty() &&
                      witness.source_edge_owners.empty());
  }
  case source_facet_triangle_absorption_kind::bookkeeping_only: {
    if (witness.local_edge_role !=
            source_triangle_edge_role::facet_internal_diagonal ||
        witness.internal_diagonal == 0 ||
        witness.public_index >= record.intervals.size() ||
        !witness.source_vertex_owners.empty() ||
        !witness.source_edge_owners.empty())
      return false;
    const auto &target = record.intervals[witness.public_index];
    if (target.classification ==
        source_facet_segment_interval_class::original_edge_overlap)
      return false;
    const auto &left = record.breakpoints[target.left_breakpoint];
    const auto &right = record.breakpoints[target.right_breakpoint];
    return point_strictly_between(witness.parameter, left.parameter,
                                  right.parameter) &&
           witness.semantic_classification ==
               interval_public_point_class<T>(target.classification);
  }
  }
  return false;
}

} // namespace source_facet_region_detail

template <class T>
bool valid_source_facet_segment_partition_record(
    const source_facet_segment_partition_record<T> &record) {
  using namespace source_facet_region_detail;
  if (record.schema_version !=
          contract_versions::relation_source_facet_segment_schema ||
      record.policy_version !=
          contract_versions::relation_source_facet_segment_policy ||
      record.witness_policy_version !=
          contract_versions::relation_source_facet_segment_witness_policy ||
      record.reconciliation_policy_version !=
          contract_versions::
              relation_alternative_triangulation_semantics_policy ||
      !record.complete_boundary_contact_set ||
      record.boundary_edge_relation_count == 0 ||
      !valid_projected_point(record.segment_start) ||
      !valid_projected_point(record.segment_end) ||
      !valid_source_orientation_evidence(
          record.polygon_orientation_evidence) ||
      record.polygon_orientation_evidence.bounded_sign ==
          bounded_planar_sign::uncertain ||
      record.breakpoints.size() < 2 ||
      record.intervals.size() + 1 != record.breakpoints.size() ||
      record.reserved != 0)
    return false;

  for (const auto &contact : record.contacts) {
    if (contact.schema_version !=
            contract_versions::relation_source_facet_segment_schema ||
        contact.lineage == 0 || contact.reserved != 0 ||
        !unit_parameter(contact.first_parameter,
                        contact.first_rounded_parameter) ||
        !valid_projected_point(contact.first_point) ||
        !parameter_matches_projected_point(
            record.segment_start, record.segment_end,
            contact.first_rounded_parameter, contact.first_parameter,
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
    const auto owner_ordinals_valid =
        [&record](
            const std::vector<source_facet_boundary_edge_owner> &owners) {
          return std::all_of(
              owners.begin(), owners.end(),
              [&record](const source_facet_boundary_edge_owner &owner) {
                return owner.edge_ordinal <
                           record.boundary_edge_relation_count &&
                       owner.origin_source_vertex !=
                           owner.destination_source_vertex;
              });
        };
    if (!owner_ordinals_valid(first_edges) ||
        !owner_ordinals_valid(second_edges) ||
        !owner_ordinals_valid(overlap_edges))
      return false;

    switch (contact.kind) {
    case source_facet_segment_contact_kind::point_contact:
      if (!interval_equal_bits(contact.first_parameter,
                               contact.second_parameter) ||
          to_bits(contact.first_rounded_parameter) !=
              to_bits(contact.second_rounded_parameter) ||
          !same_projected_geometry(contact.first_point,
                                   contact.second_point) ||
          !contact.second_source_vertex_owners.empty() ||
          !contact.second_source_edge_owners.empty() ||
          !contact.overlap_source_edge_owners.empty() ||
          (contact.first_source_vertex_owners.empty() &&
           contact.first_source_edge_owners.empty()))
        return false;
      break;
    case source_facet_segment_contact_kind::boundary_overlap:
      if (!unit_parameter(contact.second_parameter,
                          contact.second_rounded_parameter) ||
          !valid_projected_point(contact.second_point) ||
          !parameter_matches_projected_point(
              record.segment_start, record.segment_end,
              contact.second_rounded_parameter, contact.second_parameter,
              contact.second_point) ||
          !definitely_before(contact.first_parameter,
                             contact.second_parameter) ||
          contact.overlap_source_edge_owners.empty())
        return false;
      break;
    default:
      return false;
    }
  }
  for (std::size_t i = 1; i < record.contacts.size(); ++i) {
    const auto &a = record.contacts[i - 1];
    const auto &b = record.contacts[i];
    if (definitely_before(b.first_parameter, a.first_parameter) ||
        (!definitely_before(a.first_parameter, b.first_parameter) &&
         !interval_equal_bits(a.first_parameter, b.first_parameter)))
      return false;
    if (interval_equal_bits(a.first_parameter, b.first_parameter) &&
        a.lineage >= b.lineage)
      return false;
  }

  if ((record.breakpoints.front().segment_endpoint_mask & 1U) == 0 ||
      (record.breakpoints.back().segment_endpoint_mask & 2U) == 0)
    return false;

  for (std::size_t i = 0; i < record.breakpoints.size(); ++i) {
    const auto &breakpoint = record.breakpoints[i];
    if (!unit_parameter(breakpoint.parameter,
                        breakpoint.rounded_parameter) ||
        !valid_projected_point(breakpoint.point) ||
        !parameter_matches_projected_point(
            record.segment_start, record.segment_end,
            breakpoint.rounded_parameter, breakpoint.parameter,
            breakpoint.point) ||
        !valid_source_facet_point_region_record(breakpoint.region) ||
        breakpoint.region.source_facet != record.source_facet ||
        breakpoint.region.ring != record.ring ||
        breakpoint.reserved8 != 0 || breakpoint.reserved16 != 0 ||
        !std::is_sorted(breakpoint.contact_lineages.begin(),
                        breakpoint.contact_lineages.end()) ||
        std::adjacent_find(breakpoint.contact_lineages.begin(),
                           breakpoint.contact_lineages.end()) !=
            breakpoint.contact_lineages.end())
      return false;
    if (breakpoint.segment_endpoint_mask > 3 ||
        (breakpoint.segment_endpoint_mask == 0 &&
         breakpoint.contact_lineages.empty()))
      return false;
    if ((breakpoint.segment_endpoint_mask & 1U) != 0 &&
        (!singleton(breakpoint.parameter) ||
         breakpoint.parameter.lower() != T(0) ||
         !same_projected_geometry(breakpoint.point,
                                  record.segment_start)))
      return false;
    if ((breakpoint.segment_endpoint_mask & 2U) != 0 &&
        (!singleton(breakpoint.parameter) ||
         breakpoint.parameter.lower() != T(1) ||
         !same_projected_geometry(breakpoint.point,
                                  record.segment_end)))
      return false;
    if (i != 0 &&
        !definitely_before(record.breakpoints[i - 1].parameter,
                           breakpoint.parameter))
      return false;
  }

  for (std::size_t i = 0; i < record.intervals.size(); ++i) {
    const auto &interval = record.intervals[i];
    if (interval.left_breakpoint != i ||
        interval.right_breakpoint != i + 1 ||
        interval.right_breakpoint >= record.breakpoints.size() ||
        !unit_parameter(interval.witness_parameter,
                        interval.rounded_witness_parameter) ||
        !point_strictly_between(
            interval.witness_parameter,
            record.breakpoints[interval.left_breakpoint].parameter,
            record.breakpoints[interval.right_breakpoint].parameter) ||
        !valid_projected_point(interval.witness_point) ||
        !parameter_matches_projected_point(
            record.segment_start, record.segment_end,
            interval.rounded_witness_parameter,
            interval.witness_parameter, interval.witness_point) ||
        !valid_source_facet_point_region_record(
            interval.witness_region) ||
        interval.witness_region.source_facet != record.source_facet ||
        interval.witness_region.ring != record.ring ||
        interval.dyadic_attempt_ordinal == 0 ||
        interval.reserved != 0 ||
        !std::is_sorted(interval.source_edge_owners.begin(),
                        interval.source_edge_owners.end()) ||
        std::adjacent_find(interval.source_edge_owners.begin(),
                           interval.source_edge_owners.end()) !=
            interval.source_edge_owners.end())
      return false;

    switch (interval.classification) {
    case source_facet_segment_interval_class::interior:
      if (interval.witness_region.classification !=
              source_facet_point_region_class::interior ||
          !interval.source_edge_owners.empty())
        return false;
      break;
    case source_facet_segment_interval_class::outside:
      if (interval.witness_region.classification !=
              source_facet_point_region_class::outside ||
          !interval.source_edge_owners.empty())
        return false;
      break;
    case source_facet_segment_interval_class::original_edge_overlap:
      if (interval.witness_region.classification !=
              source_facet_point_region_class::original_edge ||
          interval.source_edge_owners.empty())
        return false;
      for (const auto &owner : interval.source_edge_owners)
        if (!std::binary_search(
                interval.witness_region.source_edge_owners.begin(),
                interval.witness_region.source_edge_owners.end(), owner))
          return false;
      break;
    }
  }

  if (record.triangle_reconciliation_complete) {
    if (!std::is_sorted(record.triangle_witnesses.begin(),
                        record.triangle_witnesses.end()))
      return false;
    for (std::size_t i = 0; i < record.triangle_witnesses.size(); ++i) {
      if (!triangle_witness_valid(record.triangle_witnesses[i], record))
        return false;
      if (i != 0 &&
          record.triangle_witnesses[i - 1].triangle ==
              record.triangle_witnesses[i].triangle &&
          record.triangle_witnesses[i - 1].local_witness ==
              record.triangle_witnesses[i].local_witness)
        return false;
    }
  } else if (!record.triangle_witnesses.empty()) {
    return false;
  }

  return sha256::digest(semantic_bytes(record)) ==
         record.semantic_digest;
}

} // namespace ygor::mesh_boolean::bounded
