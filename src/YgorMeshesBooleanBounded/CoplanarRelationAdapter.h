#pragma once

#include "CoplanarCarrierArrangements.h"
#include "EventInterning.h"
#include "FloatingBits.h"
#include "RelationQueries.h"
#include "Sha256.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace coplanar_relation_adapter_detail {

inline bounded_boolean_error adapter_error(intersection_subcode subcode,
                                            const char *summary) {
  return intersection_error(subcode,
                            bounded_boolean_error_category::internal_invariant_error,
                            summary,
                            intersection_checkpoint::coplanar_carriers);
}

inline bool classification(relation_coplanar_classification value,
                           coplanar_region_classification &out) noexcept {
  switch (value) {
  case relation_coplanar_classification::point_contact:
    out = coplanar_region_classification::point_contact;
    return true;
  case relation_coplanar_classification::segment_contact:
    out = coplanar_region_classification::segment_contact;
    return true;
  case relation_coplanar_classification::area_overlap:
    out = coplanar_region_classification::area_overlap;
    return true;
  case relation_coplanar_classification::first_contains_second:
    out = coplanar_region_classification::first_contains_second;
    return true;
  case relation_coplanar_classification::second_contains_first:
    out = coplanar_region_classification::second_contains_first;
    return true;
  case relation_coplanar_classification::equal_same_orientation:
    out = coplanar_region_classification::equal_same_orientation;
    return true;
  case relation_coplanar_classification::equal_opposite_orientation:
    out = coplanar_region_classification::equal_opposite_orientation;
    return true;
  case relation_coplanar_classification::disjoint:
    return false;
  }
  return false;
}

inline feature_relation_status relation_status(
    relation_coplanar_classification value) noexcept {
  switch (value) {
  case relation_coplanar_classification::point_contact:
    return feature_relation_status::point_contact;
  case relation_coplanar_classification::segment_contact:
    return feature_relation_status::segment_contact;
  case relation_coplanar_classification::area_overlap:
    return feature_relation_status::overlap;
  case relation_coplanar_classification::first_contains_second:
  case relation_coplanar_classification::second_contains_first:
    return feature_relation_status::containment;
  case relation_coplanar_classification::equal_same_orientation:
    return feature_relation_status::coincidence_same_orientation;
  case relation_coplanar_classification::equal_opposite_orientation:
    return feature_relation_status::coincidence_opposite_orientation;
  case relation_coplanar_classification::disjoint:
    break;
  }
  return feature_relation_status::not_evaluated;
}

inline bounded_boolean_digest support_semantic_digest(
    const coplanar_support_proposal &proposal) {
  canonical_writer writer;
  encode_coplanar_support_key(writer, proposal.key);
  for (const auto byte : proposal.first_source_facet_semantic_digest.bytes)
    writer.u8(byte);
  for (const auto byte : proposal.second_source_facet_semantic_digest.bytes)
    writer.u8(byte);
  return sha256::digest(writer.bytes());
}

template <class T, class I>
bool candidate_witnesses(const signed_feature_relations_view<T, I> &relations,
                         feature_relation_id relation,
                         std::vector<candidate_id> &out) {
  if (relation.ordinal() >= relations.relations().size())
    return false;
  const auto &record = relations.relations()[relation.ordinal()];
  if (record.id != relation ||
      record.producer.ordinal() >= relations.request_graph().requests.size())
    return false;
  const auto &request =
      relations.request_graph().requests[record.producer.ordinal()];
  if (request.id != record.producer ||
      request.witness_begin > relations.request_graph().candidate_witnesses.size() ||
      request.witness_count >
          relations.request_graph().candidate_witnesses.size() -
              request.witness_begin ||
      request.witness_count == 0)
    return false;
  out.assign(relations.request_graph().candidate_witnesses.begin() +
                 static_cast<std::ptrdiff_t>(request.witness_begin),
             relations.request_graph().candidate_witnesses.begin() +
                 static_cast<std::ptrdiff_t>(request.witness_begin +
                                             request.witness_count));
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return !out.empty();
}

template <class T, class I>
bool node_occurrence(const signed_feature_relations_view<T, I> &relations,
                     const event_interning_tables &interning,
                     const relation_coplanar_event_node_record &node,
                     std::uint8_t polygon, event_occurrence_id &out) {
  std::vector<relation_request_id> requests;
  std::vector<relation_feature_key> source_edges;
  std::vector<relation_feature_key> source_vertices;
  for (const auto &occurrence : node.occurrences) {
    if (occurrence.polygon != polygon)
      continue;
    source_edges.push_back(occurrence.source_edge);
    if (occurrence.query_source_vertex_valid)
      source_vertices.push_back(occurrence.endpoint_source_vertex);
    for (const auto &lineage : occurrence.event_lineages) {
      relation_request_id request{intersection_invalid_ordinal};
      if (lineage.contact_lineage == 0 ||
          lineage.request.ordinal() >= relations.request_graph().requests.size() ||
          relations.request_graph().requests[lineage.request.ordinal()].id !=
              lineage.request ||
          relation_stable_lineage(
              relations.request_graph().requests[lineage.request.ordinal()].key,
              5) != lineage.contact_lineage)
        return false;
      requests.push_back(lineage.request);
    }
  }
  std::sort(requests.begin(), requests.end());
  requests.erase(std::unique(requests.begin(), requests.end()), requests.end());
  std::sort(source_edges.begin(), source_edges.end());
  source_edges.erase(std::unique(source_edges.begin(), source_edges.end()),
                     source_edges.end());
  std::sort(source_vertices.begin(), source_vertices.end());
  source_vertices.erase(
      std::unique(source_vertices.begin(), source_vertices.end()),
      source_vertices.end());

  std::vector<event_occurrence_id> matches;
  for (const auto &binding : interning.seed_bindings) {
    if (binding.construction != node.representative ||
        binding.relation.ordinal() >= relations.relations().size())
      continue;
    const auto &relation = relations.relations()[binding.relation.ordinal()];
    if (relation.id != binding.relation)
      return false;
    const bool request_match = !requests.empty() && std::binary_search(
        requests.begin(), requests.end(), relation.producer);
    const bool overlay_match = binding.relation == node.overlay_relation;
    const bool edge_match = std::binary_search(
        source_edges.begin(), source_edges.end(), binding.seed_key.first) ||
        std::binary_search(source_edges.begin(), source_edges.end(),
                           binding.seed_key.second);
    const bool vertex_match = std::binary_search(
        source_vertices.begin(), source_vertices.end(),
        binding.accepted_source_vertex);
    if ((!request_match && !overlay_match) ||
        (!edge_match && !vertex_match && !source_edges.empty()))
      continue;
    matches.push_back(binding.occurrence);
  }
  std::sort(matches.begin(), matches.end());
  matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
  if (matches.size() != 1)
    return false;
  out = matches.front();
  return out.ordinal() < interning.occurrences.size();
}

inline void orient_endpoints(
    const relation_coplanar_arc_occurrence_record &occurrence,
    relation_coplanar_event_node_id canonical_start,
    std::array<std::uint64_t, 2> &nominal,
    std::array<std::uint64_t, 2> &lower,
    std::array<std::uint64_t, 2> &upper,
    std::array<parameter_domain_status, 2> &domains) {
  nominal = occurrence.endpoint_nominal_bits;
  lower = occurrence.endpoint_lower_bits;
  upper = occurrence.endpoint_upper_bits;
  domains = occurrence.endpoint_domains;
  const bool canonical_forward = occurrence.start_node == canonical_start;
  if (canonical_forward != occurrence.forward_along_source_edge) {
    std::swap(nominal[0], nominal[1]);
    std::swap(lower[0], lower[1]);
    std::swap(upper[0], upper[1]);
    std::swap(domains[0], domains[1]);
  }
}

template <class T>
bool endpoint_measure(
    const std::array<std::uint64_t, 2> &lower,
    const std::array<std::uint64_t, 2> &upper, bool &exact_zero,
    bool &definitely_positive) noexcept {
  using bits_type = floating_uint_t<T>;
  const T first_lower = from_bits<T>(static_cast<bits_type>(lower[0]));
  const T first_upper = from_bits<T>(static_cast<bits_type>(upper[0]));
  const T second_lower = from_bits<T>(static_cast<bits_type>(lower[1]));
  const T second_upper = from_bits<T>(static_cast<bits_type>(upper[1]));
  exact_zero = first_lower == first_upper && second_lower == second_upper &&
               first_lower == second_lower;
  definitely_positive = first_upper < second_lower || second_upper < first_lower;
  return exact_zero != definitely_positive;
}

inline bounded_boolean_digest facet_semantic_digest(
    const relation_feature_key &facet,
    const std::vector<relation_feature_key> &boundary,
    const std::vector<relation_coplanar_partition_coverage_record> &coverage,
    std::uint8_t polygon) {
  canonical_writer writer;
  encode_relation_feature_key(writer, facet);
  writer.u64(boundary.size());
  for (const auto &edge : boundary)
    encode_relation_feature_key(writer, edge);
  for (const auto &entry : coverage) {
    if (entry.polygon != polygon)
      continue;
    encode_relation_feature_key(writer, entry.source_edge);
    writer.u64(entry.edge_ordinal);
    writer.u64(entry.breakpoint_count);
    writer.u64(entry.interior_interval_count);
    writer.u64(entry.outside_interval_count);
    writer.u64(entry.original_edge_overlap_interval_count);
    writer.boolean(entry.complete_boundary_contact_set);
    writer.boolean(entry.triangle_reconciliation_complete);
  }
  return sha256::digest(writer.take());
}

inline bool endpoint_correspondence(
    const relation_coplanar_event_node_record &node, std::uint8_t first_polygon,
    std::uint8_t second_polygon,
    const std::vector<relation_request_id> &overlap_requests) {
  for (const auto request : overlap_requests) {
    const relation_coplanar_event_lineage_record *first = nullptr;
    const relation_coplanar_event_lineage_record *second = nullptr;
    for (const auto &occurrence : node.occurrences) {
      for (const auto &lineage : occurrence.event_lineages) {
        if (lineage.request != request)
          continue;
        if (occurrence.polygon == first_polygon)
          first = first ? nullptr : &lineage;
        if (occurrence.polygon == second_polygon)
          second = second ? nullptr : &lineage;
      }
    }
    if (first && second && first->contact_lineage == second->contact_lineage &&
        first->endpoint_role == second->endpoint_role)
      return true;
  }
  return false;
}

} // namespace coplanar_relation_adapter_detail

template <class T, class I>
bool collect_component07_coplanar_arrangement_proposals(
    const signed_feature_relations_view<T, I> &relations,
    const event_interning_tables &interning,
    std::vector<coplanar_support_proposal> &supports,
    std::vector<collinear_overlap_carrier_proposal> &carriers,
    std::vector<coplanar_overlap_component_proposal> &components,
    std::vector<coplanar_region_incidence_proposal> &regions,
    bounded_boolean_error &error) {
  supports.clear();
  carriers.clear();
  components.clear();
  regions.clear();
  if (!relations.valid_owner()) {
    error = coplanar_relation_adapter_detail::adapter_error(
        intersection_subcode::predecessor_not_verified,
        "Component 08 coplanar adapter requires a checked Component 07 view");
    return false;
  }

  const auto fail = [&](const char *summary) {
    supports.clear();
    carriers.clear();
    components.clear();
    regions.clear();
    error = coplanar_relation_adapter_detail::adapter_error(
        intersection_subcode::overlap_carrier_invalid, summary);
    return false;
  };

  for (const auto &source : relations.coplanar_supports()) {
    if (source.id.ordinal() >= relations.coplanar_supports().size() ||
        source.overlay_relation.ordinal() >= relations.relations().size() ||
        source.support_lineage == 0 || source.schema_version !=
            contract_versions::relation_coplanar_topology_schema ||
        source.reserved32 != 0 ||
        !source.complete_boundary_pair_coverage ||
        !source.complete_vertex_coverage ||
        !source.complete_boundary_partition_coverage ||
        !source.complete_event_lineage ||
        !source.complete_authorized_arc_coverage ||
        !source.complete_overlap_component_assembly)
      return fail("Component 08 coplanar support handoff is incomplete");

    coplanar_support_key key;
    key.first_facet = source.support_facets[0];
    key.second_facet = source.support_facets[1];
    key.support_lineage = source.support_lineage;
    key.opposite_orientation =
        source.orientation == relation_coplanar_orientation::opposite;
    key.symbolic_owner = source.half_open_owner;
    std::vector<candidate_id> witnesses;
    if (!valid_coplanar_support_key(key) ||
        !coplanar_relation_adapter_detail::candidate_witnesses(
            relations, source.overlay_relation, witnesses))
      return fail("Component 08 coplanar support provenance is malformed");
    for (std::size_t i = 0; i < witnesses.size(); ++i) {
      coplanar_support_proposal proposal;
      proposal.key = key;
      proposal.relation = source.overlay_relation;
      proposal.candidate = witnesses[i];
      proposal.first_source_facet_semantic_digest =
          coplanar_relation_adapter_detail::facet_semantic_digest(
              key.first_facet, source.original_boundary_edges[0],
              source.partition_coverage, 0);
      proposal.second_source_facet_semantic_digest =
          coplanar_relation_adapter_detail::facet_semantic_digest(
              key.second_facet, source.original_boundary_edges[1],
              source.partition_coverage, 1);
      proposal.original_boundary_edges = source.original_boundary_edges[0];
      proposal.original_boundary_edges.insert(
          proposal.original_boundary_edges.end(),
          source.original_boundary_edges[1].begin(),
          source.original_boundary_edges[1].end());
      // Component 07 publishes a witness set, not an authority bit. Component
      // 08's verified contract designates the lexicographically least complete
      // (relation, candidate) provenance tuple after deduplication.
      proposal.designated_authority = i == 0;
      proposal.support_consistent = true;
      proposal.orientation_consistent = true;
      proposal.symbolic_policy_consistent = true;
      proposal.precision_evidence_complete = true;
      supports.push_back(std::move(proposal));
    }

    for (const auto &arc : relations.coplanar_oriented_arcs()) {
      if (arc.overlay_relation != source.overlay_relation ||
          arc.kind != relation_coplanar_arc_kind::shared_boundary)
        continue;
      const relation_coplanar_arc_occurrence_record *first = nullptr;
      const relation_coplanar_arc_occurrence_record *second = nullptr;
      for (const auto &occurrence : arc.occurrences) {
        if (occurrence.source_edge.operand == key.first_facet.operand)
          first = first ? nullptr : &occurrence;
        else if (occurrence.source_edge.operand == key.second_facet.operand)
          second = second ? nullptr : &occurrence;
      }
      if (!first || !second || arc.source_edge_count != 2 ||
          arc.arc_lineage == 0 || arc.overlap_lineages.empty())
        return fail("Component 08 shared-boundary arc lacks two source-edge occurrences");
      event_occurrence_id start{intersection_invalid_ordinal};
      event_occurrence_id end{intersection_invalid_ordinal};
      if (arc.start_node.ordinal() >= relations.coplanar_event_nodes().size() ||
          arc.end_node.ordinal() >= relations.coplanar_event_nodes().size() ||
          !coplanar_relation_adapter_detail::node_occurrence(
              relations, interning,
              relations.coplanar_event_nodes()[arc.start_node.ordinal()],
              first->polygon, start) ||
          !coplanar_relation_adapter_detail::node_occurrence(
              relations, interning,
              relations.coplanar_event_nodes()[arc.end_node.ordinal()],
              first->polygon, end))
        return fail("Component 08 could not map shared-boundary endpoints by public lineage");

      collinear_overlap_carrier_proposal proposal;
      proposal.key.first_edge = first->source_edge;
      proposal.key.second_edge = second->source_edge;
      proposal.key.coplanar_support_lineage = source.support_lineage;
      proposal.key.overlap_lineage = arc.arc_lineage;
      proposal.key.opposite_direction =
          first->forward_along_source_edge != second->forward_along_source_edge;
      proposal.key.symbolic_owner = source.half_open_owner;
      coplanar_relation_adapter_detail::orient_endpoints(
          *first, arc.start_node, proposal.first_nominal_bits,
          proposal.first_lower_bits, proposal.first_upper_bits,
          proposal.first_domains);
      coplanar_relation_adapter_detail::orient_endpoints(
          *second, arc.start_node, proposal.second_nominal_bits,
          proposal.second_lower_bits, proposal.second_upper_bits,
          proposal.second_domains);
      proposal.start_occurrence = start;
      proposal.end_occurrence = end;
      proposal.start_occurrence_key = interning.occurrences[start.ordinal()].key;
      proposal.end_occurrence_key = interning.occurrences[end.ordinal()].key;
      proposal.relation = source.overlay_relation;
      proposal.candidate = witnesses.front();
      proposal.source_provenance = {key.first_facet, key.second_facet,
                                    first->source_edge, second->source_edge};
      proposal.first_original_source_edge = true;
      proposal.second_original_source_edge = true;
      proposal.first_direction_valid = true;
      proposal.second_direction_valid = true;
      proposal.parameter_correspondence_verified =
          coplanar_relation_adapter_detail::endpoint_correspondence(
              relations.coplanar_event_nodes()[arc.start_node.ordinal()],
              first->polygon, second->polygon, arc.overlap_lineages) &&
          coplanar_relation_adapter_detail::endpoint_correspondence(
              relations.coplanar_event_nodes()[arc.end_node.ordinal()],
              first->polygon, second->polygon, arc.overlap_lineages);
      bool second_zero = false;
      bool second_positive = false;
      if (!proposal.parameter_correspondence_verified ||
          !coplanar_relation_adapter_detail::endpoint_measure<T>(
              proposal.first_lower_bits, proposal.first_upper_bits,
              proposal.exact_zero_length,
              proposal.definitely_positive_length) ||
          !coplanar_relation_adapter_detail::endpoint_measure<T>(
              proposal.second_lower_bits, proposal.second_upper_bits,
              second_zero, second_positive) ||
          proposal.exact_zero_length != second_zero ||
          proposal.definitely_positive_length != second_positive)
        return fail("Component 08 shared-boundary endpoint correspondence or length is unresolved");
      proposal.endpoint_ownership_verified = true;
      proposal.half_open_first = source.half_open_owner ==
                                 proposal.key.first_edge.operand;
      proposal.half_open_second = !proposal.half_open_first;
      if (proposal.exact_zero_length) {
        proposal.half_open_first = false;
        proposal.half_open_second = false;
      }
      proposal.half_open_policy_consistent = true;
      proposal.separate_sheet_required = source.distinct_sheet_occurrences;
      carriers.push_back(std::move(proposal));
    }

    for (const auto &component : relations.coplanar_overlap_components()) {
      if (component.overlay_relation != source.overlay_relation)
        continue;
      coplanar_overlap_component_proposal proposal;
      proposal.key.support = key;
      proposal.key.component_lineage = component.component_lineage;
      proposal.key.component_kind = component.kind;
      proposal.key.sheet_mask = component.sheet_mask;
      proposal.key.symbolic_owner = component.half_open_owner;
      proposal.component07_component = component.id;
      proposal.relation = component.overlay_relation;
      for (const auto node_id : component.node_ids) {
        if (node_id.ordinal() >= relations.coplanar_event_nodes().size())
          return fail("Component 08 coplanar component node is out of range");
        for (std::uint8_t polygon = 0; polygon < 2; ++polygon) {
          if ((component.sheet_mask & (std::uint8_t{1} << polygon)) == 0)
            continue;
          event_occurrence_id occurrence{intersection_invalid_ordinal};
          if (!coplanar_relation_adapter_detail::node_occurrence(
                  relations, interning,
                  relations.coplanar_event_nodes()[node_id.ordinal()], polygon,
                  occurrence))
            return fail("Component 08 could not map a component node by public lineage");
          proposal.boundary_events.push_back(occurrence);
        }
      }
      for (const auto arc_id : component.arc_ids) {
        if (arc_id.ordinal() >= relations.coplanar_oriented_arcs().size())
          return fail("Component 08 coplanar component arc is out of range");
        const auto &arc = relations.coplanar_oriented_arcs()[arc_id.ordinal()];
        if (arc.kind != relation_coplanar_arc_kind::shared_boundary)
          continue;
        const auto found = std::find_if(
            carriers.begin(), carriers.end(), [&](const auto &carrier) {
              return carrier.key.coplanar_support_lineage == source.support_lineage &&
                     carrier.key.overlap_lineage == arc.arc_lineage;
            });
        if (found == carriers.end())
          return fail("Component 08 shared-boundary component arc lacks a carrier");
        proposal.boundary_carriers.push_back(found->key);
      }
      std::sort(proposal.boundary_events.begin(), proposal.boundary_events.end());
      proposal.boundary_events.erase(
          std::unique(proposal.boundary_events.begin(),
                      proposal.boundary_events.end()),
          proposal.boundary_events.end());
      proposal.closed = component.closed;
      proposal.distinct_sheet_occurrences =
          component.distinct_sheet_occurrences && component.sheet_mask == 3;
      proposal.zero_measure = component.zero_measure;
      proposal.component_assembly_complete = true;
      components.push_back(proposal);

      coplanar_region_incidence_proposal region;
      region.support = key;
      region.component = proposal.key;
      region.first_facet = key.first_facet;
      region.second_facet = key.second_facet;
      if (!coplanar_relation_adapter_detail::classification(
              source.classification, region.classification))
        return fail("Component 08 cannot publish a disjoint coplanar region");
      region.relation_status =
          coplanar_relation_adapter_detail::relation_status(source.classification);
      region.symbolic_owner = component.half_open_owner;
      region.sheet_mask = component.sheet_mask;
      region.boundary_events = proposal.boundary_events;
      region.boundary_carriers = proposal.boundary_carriers;
      for (const auto &coverage : source.partition_coverage) {
        coplanar_partition_coverage_commitment commitment;
        commitment.source_edge = coverage.source_edge;
        commitment.polygon = coverage.polygon;
        commitment.edge_ordinal = coverage.edge_ordinal;
        commitment.breakpoint_count = coverage.breakpoint_count;
        commitment.interior_interval_count = coverage.interior_interval_count;
        commitment.outside_interval_count = coverage.outside_interval_count;
        commitment.original_edge_overlap_interval_count =
            coverage.original_edge_overlap_interval_count;
        commitment.complete_boundary_contact_set =
            coverage.complete_boundary_contact_set;
        commitment.triangle_reconciliation_complete =
            coverage.triangle_reconciliation_complete;
        region.partition_coverage.push_back(commitment);
      }
      region.source_facet_semantic_digest =
          coplanar_relation_adapter_detail::support_semantic_digest(
              supports.back());
      region.coverage_complete = true;
      region.internal_diagonals_coverage_only = true;
      region.source_facet_semantics_verified = true;
      regions.push_back(std::move(region));
    }
  }

  std::sort(supports.begin(), supports.end(), [](const auto &a, const auto &b) {
    return std::tie(a.key, a.relation, a.candidate) <
           std::tie(b.key, b.relation, b.candidate);
  });
  std::sort(carriers.begin(), carriers.end(),
            [](const auto &a, const auto &b) { return a.key < b.key; });
  std::sort(components.begin(), components.end(),
            [](const auto &a, const auto &b) { return a.key < b.key; });
  std::sort(regions.begin(), regions.end(), [](const auto &a, const auto &b) {
    return std::tie(a.support, a.component, a.classification, a.sheet_mask) <
           std::tie(b.support, b.component, b.classification, b.sheet_mask);
  });
  return true;
}

template <class T, class I>
bool verify_component07_coplanar_arrangement_proposals(
    const signed_feature_relations_view<T, I> &relations,
    const event_interning_tables &interning,
    const std::vector<coplanar_support_proposal> &supports,
    const std::vector<collinear_overlap_carrier_proposal> &carriers,
    const std::vector<coplanar_overlap_component_proposal> &components,
    const std::vector<coplanar_region_incidence_proposal> &regions,
    bounded_boolean_error &error) {
  std::vector<coplanar_support_proposal> expected_supports;
  std::vector<collinear_overlap_carrier_proposal> expected_carriers;
  std::vector<coplanar_overlap_component_proposal> expected_components;
  std::vector<coplanar_region_incidence_proposal> expected_regions;
  bounded_boolean_error local;
  if (!collect_component07_coplanar_arrangement_proposals(
          relations, interning, expected_supports, expected_carriers,
          expected_components, expected_regions, local)) {
    error = local;
    return false;
  }
  coplanar_carrier_arrangement_tables expected;
  coplanar_carrier_arrangement_tables actual;
  if (!build_coplanar_carrier_arrangements<T>(
          expected_supports, expected_carriers, expected_components,
          expected_regions, expected, local)) {
    error = local;
    return false;
  }
  if (!build_coplanar_carrier_arrangements<T>(supports, carriers, components,
                                              regions, actual, local)) {
    error = local;
    return false;
  }
  if (!verify_coplanar_carrier_arrangements<T>(
          expected_supports, expected_carriers, expected_components,
          expected_regions, actual, local)) {
    error = coplanar_relation_adapter_detail::adapter_error(
        intersection_subcode::verifier_rejection,
        "Component 08 coplanar proposal reconstruction mismatch");
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
