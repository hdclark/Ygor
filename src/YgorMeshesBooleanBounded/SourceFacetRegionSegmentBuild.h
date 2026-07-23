#pragma once

#include "SourceFacetRegionSegmentValidation.h"

namespace ygor::mesh_boolean::bounded {

namespace source_facet_region_detail {

template <class T>
bool make_declared_overlap_region(
    std::uint64_t source_facet, std::uint64_t ring,
    const projected_source_point<T> &point,
    const std::vector<projected_source_point<T>> &polygon,
    const source_orientation_evidence<T> &polygon_orientation_evidence,
    const std::vector<source_facet_boundary_edge_owner> &owners,
    source_facet_point_region_record<T> &record) {
  if (owners.empty() ||
      !std::is_sorted(owners.begin(), owners.end()) ||
      std::adjacent_find(owners.begin(), owners.end()) != owners.end())
    return false;

  record = {};
  record.classification =
      source_facet_point_region_class::original_edge;
  record.source_facet = source_facet;
  record.ring = ring;
  record.query_source_identity_valid = false;
  record.complete_boundary_traversal = true;
  record.boundary_ownership_resolved = true;
  record.boundary_test_count = polygon.size();
  record.source_edge_owners = owners;
  record.polygon_orientation_evidence =
      polygon_orientation_evidence;
  record.orientation_evidence.reserve(polygon.size());

  for (std::size_t edge = 0; edge < polygon.size(); ++edge) {
    const auto &a = polygon[edge];
    const auto &b = polygon[(edge + 1) % polygon.size()];
    const auto evidence =
        bounded_source_polygon_kernel<T>::orientation(a, b, point);
    if (!valid_source_orientation_evidence(evidence))
      return false;
    record.orientation_evidence.push_back(evidence);
  }

  for (const auto &owner : owners) {
    if (!valid_edge_owner_for_polygon(owner, polygon))
      return false;
    const auto &a = polygon[owner.edge_ordinal];
    const auto &b =
        polygon[(owner.edge_ordinal + 1) % polygon.size()];
    const auto &evidence =
        record.orientation_evidence[owner.edge_ordinal];
    if (!evidence.determinant.contains(T(0)) ||
        source_polygon_kernel_detail::point_box_disjoint(
            point, a, b))
      return false;
  }

  return valid_source_facet_point_region_record(record);
}

} // namespace source_facet_region_detail

template <class T>
boolean_outcome<source_facet_segment_partition_record<T>>
partition_source_facet_segment(
    std::uint64_t source_facet, std::uint64_t ring,
    const projected_source_point<T> &segment_start,
    bool segment_start_source_identity_valid,
    const projected_source_point<T> &segment_end,
    bool segment_end_source_identity_valid,
    const std::vector<projected_source_point<T>> &polygon,
    bounded_planar_sign polygon_orientation,
    std::uint64_t boundary_edge_relation_count,
    bool complete_boundary_contact_set,
    std::vector<source_facet_segment_contact_proposal<T>> contacts) {
  static_assert(supported_precision_scalar_v<T>);
  using namespace source_facet_region_detail;

  if (!complete_boundary_contact_set || polygon.size() < 3 ||
      boundary_edge_relation_count != polygon.size() ||
      !valid_projected_point(segment_start) ||
      !valid_projected_point(segment_end) ||
      same_projected_geometry(segment_start, segment_end))
    return boolean_outcome<
        source_facet_segment_partition_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_malformed,
            "Component 07 source-facet segment query is malformed or incomplete"));

  const auto polygon_orientation_evidence =
      bounded_source_polygon_kernel<T>::polygon_orientation(polygon);
  if (!polygon_orientation_evidence ||
      !valid_source_orientation_evidence(
          *polygon_orientation_evidence) ||
      polygon_orientation_evidence->bounded_sign != polygon_orientation ||
      polygon_orientation_evidence->exact_sign !=
          static_cast<int>(polygon_orientation))
    return boolean_outcome<
        source_facet_segment_partition_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::malformed_source_polygon,
            "Component 07 segment partition polygon orientation is invalid"));

  for (auto &contact : contacts) {
    canonicalize_owners(contact.first_source_vertex_owners,
                        contact.first_source_edge_owners);
    canonicalize_owners(contact.second_source_vertex_owners,
                        contact.second_source_edge_owners);
    std::vector<std::uint64_t> empty_vertices;
    canonicalize_owners(empty_vertices,
                        contact.overlap_source_edge_owners);
    if (!valid_contact_proposal(contact, segment_start, segment_end,
                                polygon))
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_malformed,
              "Component 07 canonical boundary-contact proposal is invalid"));
  }

  // Canonical contact order is proven from complete parameter enclosures.
  std::vector<source_facet_segment_contact_proposal<T>>
      canonical_contacts;
  canonical_contacts.reserve(contacts.size());
  for (auto &contact : contacts) {
    bool inserted = false;
    for (auto iterator = canonical_contacts.begin();
         iterator != canonical_contacts.end(); ++iterator) {
      if (definitely_before(contact.first_parameter,
                            iterator->first_parameter)) {
        canonical_contacts.insert(iterator, std::move(contact));
        inserted = true;
        break;
      }
      if (definitely_before(iterator->first_parameter,
                            contact.first_parameter))
        continue;
      if (!interval_equal_bits(contact.first_parameter,
                               iterator->first_parameter) ||
          !singleton(contact.first_parameter))
        return boolean_outcome<
            source_facet_segment_partition_record<T>>::failure(
            source_facet_region_error(
                relation_subcode::source_facet_segment_order_unresolved,
                "Component 07 contact records cannot be canonically ordered"));
      if (contact.lineage == iterator->lineage)
        return boolean_outcome<
            source_facet_segment_partition_record<T>>::failure(
            source_facet_region_error(
                relation_subcode::source_facet_segment_malformed,
                "Component 07 duplicate contact lineage is forbidden"));
      if (contact.lineage < iterator->lineage) {
        canonical_contacts.insert(iterator, std::move(contact));
        inserted = true;
        break;
      }
    }
    if (!inserted)
      canonical_contacts.push_back(std::move(contact));
  }

  std::vector<segment_position_seed<T>> seeds;
  seeds.reserve(2 + canonical_contacts.size() * 2);
  const auto zero = finite_interval<T>::checked_singleton(T(0));
  const auto one = finite_interval<T>::checked_singleton(T(1));
  if (!zero || !one)
    return boolean_outcome<
        source_facet_segment_partition_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 could not construct the closed segment parameter domain"));
  seeds.push_back({*zero, T(0), segment_start,
                   segment_start_source_identity_valid, {}, {}, {}, 1});
  seeds.push_back({*one, T(1), segment_end,
                   segment_end_source_identity_valid, {}, {}, {}, 2});

  for (const auto &contact : canonical_contacts) {
    segment_position_seed<T> first;
    first.parameter = contact.first_parameter;
    first.rounded_parameter = contact.first_rounded_parameter;
    first.point = contact.first_point;
    first.point_source_identity_valid =
        contact.first_point_source_identity_valid;
    first.expected_vertex_owners =
        contact.first_source_vertex_owners;
    first.expected_edge_owners = contact.first_source_edge_owners;
    first.contact_lineages.push_back(contact.lineage);
    if (contact.kind ==
        source_facet_segment_contact_kind::boundary_overlap) {
      first.expected_edge_owners.insert(
          first.expected_edge_owners.end(),
          contact.overlap_source_edge_owners.begin(),
          contact.overlap_source_edge_owners.end());
    }
    seeds.push_back(std::move(first));

    if (contact.kind ==
        source_facet_segment_contact_kind::boundary_overlap) {
      segment_position_seed<T> second;
      second.parameter = contact.second_parameter;
      second.rounded_parameter = contact.second_rounded_parameter;
      second.point = contact.second_point;
      second.point_source_identity_valid =
          contact.second_point_source_identity_valid;
      second.expected_vertex_owners =
          contact.second_source_vertex_owners;
      second.expected_edge_owners = contact.second_source_edge_owners;
      second.expected_edge_owners.insert(
          second.expected_edge_owners.end(),
          contact.overlap_source_edge_owners.begin(),
          contact.overlap_source_edge_owners.end());
      second.contact_lineages.push_back(contact.lineage);
      seeds.push_back(std::move(second));
    }
  }

  auto canonical_positions = canonicalize_positions(std::move(seeds));
  if (!canonical_positions.has_value())
    return boolean_outcome<
        source_facet_segment_partition_record<T>>::failure(
        *canonical_positions.error());

  source_facet_segment_partition_record<T> result;
  result.source_facet = source_facet;
  result.ring = ring;
  result.segment_start = segment_start;
  result.segment_end = segment_end;
  result.segment_start_source_identity_valid =
      segment_start_source_identity_valid;
  result.segment_end_source_identity_valid =
      segment_end_source_identity_valid;
  result.complete_boundary_contact_set = true;
  result.boundary_edge_relation_count = boundary_edge_relation_count;
  result.polygon_orientation_evidence =
      *polygon_orientation_evidence;
  result.contacts = std::move(canonical_contacts);
  result.breakpoints.reserve(canonical_positions.value()->size());

  for (const auto &position : *canonical_positions.value()) {
    auto region = classify_source_facet_point(
        source_facet, ring, position.point,
        position.point_source_identity_valid, polygon,
        polygon_orientation);
    if (!region.has_value())
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          *region.error());

    auto region_record = *region.value();
    if (!promote_declared_vertex_owner(
            region_record, position.point, polygon,
            position.expected_vertex_owners))
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_boundary_ownership,
              "Component 07 declared segment breakpoint vertex ownership is inconsistent"));
    if (!owner_subset(position.expected_vertex_owners,
                      position.expected_edge_owners,
                      region_record))
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_boundary_ownership,
              "Component 07 segment breakpoint boundary ownership is inconsistent"));

    source_facet_segment_breakpoint<T> breakpoint;
    breakpoint.parameter = position.parameter;
    breakpoint.rounded_parameter = position.rounded_parameter;
    breakpoint.point = position.point;
    breakpoint.region = std::move(region_record);
    breakpoint.contact_lineages = position.contact_lineages;
    breakpoint.segment_endpoint_mask = position.endpoint_mask;
    result.breakpoints.push_back(std::move(breakpoint));
  }

  result.intervals.reserve(result.breakpoints.size() - 1);
  for (std::size_t interval_index = 0;
       interval_index + 1 < result.breakpoints.size();
       ++interval_index) {
    const auto &left = result.breakpoints[interval_index];
    const auto &right = result.breakpoints[interval_index + 1];
    if (!definitely_before(left.parameter, right.parameter))
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_order_unresolved,
              "Component 07 segment interval has unresolved breakpoint ordering"));

    bool accepted = false;
    source_facet_segment_interval<T> interval_record;
    interval_record.left_breakpoint = interval_index;
    interval_record.right_breakpoint = interval_index + 1;

    std::uint32_t attempt = 0;
    for (std::uint32_t depth = 1; depth <= 6 && !accepted; ++depth) {
      const std::uint32_t limit = std::uint32_t{1} << depth;
      for (std::uint32_t numerator = 1;
           numerator < limit && !accepted; numerator += 2) {
        ++attempt;
        auto parameter = dyadic_parameter(
            left.parameter.upper(), right.parameter.lower(),
            depth, numerator);
        if (!parameter.has_value())
          continue;
        const auto &parameter_value = *parameter.value();
        if (!point_strictly_between(parameter_value.second,
                                    left.parameter,
                                    right.parameter))
          continue;

        auto point = interpolate_projected_segment(
            segment_start, segment_end, parameter_value.first,
            parameter_value.second);
        if (!point.has_value())
          continue;

        std::vector<source_facet_boundary_edge_owner>
            overlap_owners;
        bool overlap_decision_unresolved = false;
        for (const auto &contact : result.contacts) {
          if (contact.kind !=
              source_facet_segment_contact_kind::boundary_overlap)
            continue;
          if (parameter_inside_open_interval(
                  parameter_value.second, contact.first_parameter,
                  contact.second_parameter)) {
            overlap_owners.insert(
                overlap_owners.end(),
                contact.overlap_source_edge_owners.begin(),
                contact.overlap_source_edge_owners.end());
            continue;
          }
          const bool before =
              definitely_before(parameter_value.second,
                                contact.first_parameter);
          const bool after =
              definitely_before(contact.second_parameter,
                                parameter_value.second);
          if (!before && !after)
            overlap_decision_unresolved = true;
        }
        if (overlap_decision_unresolved)
          continue;

        std::vector<std::uint64_t> no_vertices;
        canonicalize_owners(no_vertices, overlap_owners);

        source_facet_point_region_record<T> witness_region;
        if (!overlap_owners.empty()) {
          if (!make_declared_overlap_region(
                  source_facet, ring, *point.value(), polygon,
                  *polygon_orientation_evidence, overlap_owners,
                  witness_region))
            continue;
          interval_record.classification =
              source_facet_segment_interval_class::
                  original_edge_overlap;
          interval_record.source_edge_owners =
              overlap_owners;
        } else {
          auto region = classify_source_facet_point(
              source_facet, ring, *point.value(), false, polygon,
              polygon_orientation);
          if (!region.has_value())
            continue;
          witness_region = *region.value();
          if (witness_region.classification ==
              source_facet_point_region_class::interior) {
            interval_record.classification =
                source_facet_segment_interval_class::interior;
          } else if (witness_region.classification ==
                     source_facet_point_region_class::outside) {
            interval_record.classification =
                source_facet_segment_interval_class::outside;
          } else {
            continue;
          }
        }

        interval_record.rounded_witness_parameter =
            parameter_value.first;
        interval_record.witness_parameter =
            parameter_value.second;
        interval_record.witness_point = *point.value();
        interval_record.witness_region =
            std::move(witness_region);
        interval_record.dyadic_attempt_ordinal = attempt;
        accepted = true;
      }
    }

    if (!accepted)
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::
                  source_facet_segment_partition_unresolved,
              "Component 07 could not certify a deterministic open-interval witness"));
    result.intervals.push_back(std::move(interval_record));
  }

  result.semantic_digest =
      sha256::digest(semantic_bytes(result));
  if (!valid_source_facet_segment_partition_record(result))
    return boolean_outcome<
        source_facet_segment_partition_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 source-facet segment partition failed independent invariants"));
  return boolean_outcome<
      source_facet_segment_partition_record<T>>::success(
      std::move(result));
}

template <class T>
boolean_outcome<source_facet_segment_partition_record<T>>
reconcile_source_facet_triangle_local_witnesses(
    source_facet_segment_partition_record<T> partition,
    std::vector<source_facet_triangle_local_witness<T>> witnesses) {
  using namespace source_facet_region_detail;
  if (!valid_source_facet_segment_partition_record(partition) ||
      partition.triangle_reconciliation_complete)
    return boolean_outcome<
        source_facet_segment_partition_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_triangle_reconciliation,
            "Component 07 triangle-local reconciliation input is invalid"));

  for (auto &witness : witnesses)
    canonicalize_owners(witness.source_vertex_owners,
                        witness.source_edge_owners);
  std::sort(witnesses.begin(), witnesses.end());

  for (std::size_t i = 0; i < witnesses.size(); ++i) {
    if (!triangle_witness_valid(witnesses[i], partition))
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_triangle_reconciliation,
              "Component 07 triangle-local witness cannot be absorbed by the semantic partition"));
    if (i != 0 &&
        witnesses[i - 1].triangle == witnesses[i].triangle &&
        witnesses[i - 1].local_witness ==
            witnesses[i].local_witness)
      return boolean_outcome<
          source_facet_segment_partition_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_triangle_reconciliation,
              "Component 07 duplicate triangle-local witness identity is forbidden"));
  }

  partition.triangle_witnesses = std::move(witnesses);
  partition.triangle_reconciliation_complete = true;
  // The semantic digest intentionally excludes triangle-local bookkeeping.
  if (!valid_source_facet_segment_partition_record(partition))
    return boolean_outcome<
        source_facet_segment_partition_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_triangle_reconciliation,
            "Component 07 reconciled triangle-local witness record is invalid"));
  return boolean_outcome<
      source_facet_segment_partition_record<T>>::success(
      std::move(partition));
}

template <class T>
bool equivalent_source_facet_segment_semantics(
    const source_facet_segment_partition_record<T> &a,
    const source_facet_segment_partition_record<T> &b) {
  if (!valid_source_facet_segment_partition_record(a) ||
      !valid_source_facet_segment_partition_record(b))
    return false;
  return source_facet_region_detail::semantic_bytes(a) ==
         source_facet_region_detail::semantic_bytes(b);
}

} // namespace ygor::mesh_boolean::bounded
