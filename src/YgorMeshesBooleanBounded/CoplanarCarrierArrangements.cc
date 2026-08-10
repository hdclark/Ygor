#include "CoplanarCarrierArrangements.h"

#include "FloatingBits.h"
#include "Sha256.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <tuple>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error coplanar_error(intersection_subcode subcode,
                                     const char *summary) {
  return intersection_error(subcode,
                            bounded_boolean_error_category::input_contract_error,
                            summary,
                            intersection_checkpoint::coplanar_carriers);
}

void write_digest(canonical_writer &writer,
                  const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    writer.u8(byte);
}

bool digest_zero(const bounded_boolean_digest &digest) noexcept {
  return std::all_of(digest.bytes.begin(), digest.bytes.end(),
                     [](std::uint8_t value) { return value == 0; });
}

bool valid_relation_id(feature_relation_id id) noexcept {
  return id.ordinal() != intersection_invalid_ordinal;
}
bool valid_candidate_id(candidate_id id) noexcept {
  return id.ordinal() != intersection_invalid_ordinal;
}
bool valid_occurrence_id(event_occurrence_id id) noexcept {
  return id.ordinal() != intersection_invalid_ordinal;
}
bool valid_construction_id(relation_construction_id id) noexcept {
  return id.ordinal() != intersection_invalid_ordinal;
}
bool valid_interval_id(relation_interval_evidence_id id) noexcept {
  return id.ordinal() != intersection_invalid_ordinal;
}

template <class T>
bool valid_parameter_interval(std::uint64_t lower_bits,
                              std::uint64_t upper_bits,
                              parameter_domain_status domain) noexcept {
  if (domain == parameter_domain_status::outside ||
      domain == parameter_domain_status::invalid)
    return false;
  using bits_type = floating_uint_t<T>;
  const T lower = from_bits<T>(static_cast<bits_type>(lower_bits));
  const T upper = from_bits<T>(static_cast<bits_type>(upper_bits));
  return finite_bits(lower) && finite_bits(upper) && lower <= upper &&
         lower >= T(0) && upper <= T(1);
}

bounded_boolean_digest support_semantic_digest(
    const coplanar_support_proposal &proposal) {
  canonical_writer writer;
  encode_coplanar_support_key(writer, proposal.key);
  write_digest(writer, proposal.first_source_facet_semantic_digest);
  write_digest(writer, proposal.second_source_facet_semantic_digest);
  return sha256::digest(writer.bytes());
}

bool classification_matches(coplanar_region_classification classification,
                            feature_relation_status status) noexcept {
  switch (classification) {
  case coplanar_region_classification::point_contact:
    return status == feature_relation_status::point_contact;
  case coplanar_region_classification::segment_contact:
    return status == feature_relation_status::segment_contact ||
           status == feature_relation_status::overlap;
  case coplanar_region_classification::area_overlap:
    return status == feature_relation_status::overlap;
  case coplanar_region_classification::first_contains_second:
  case coplanar_region_classification::second_contains_first:
    return status == feature_relation_status::containment;
  case coplanar_region_classification::equal_same_orientation:
    return status == feature_relation_status::coincidence_same_orientation;
  case coplanar_region_classification::equal_opposite_orientation:
  case coplanar_region_classification::coincident_sheet_boundary:
    return status == feature_relation_status::coincidence_opposite_orientation ||
           status == feature_relation_status::coincidence_same_orientation;
  }
  return false;
}

template <class T>
bool build_impl(
    const std::vector<coplanar_support_proposal> &support_proposals,
    const std::vector<collinear_overlap_carrier_proposal> &carrier_proposals,
    const std::vector<coplanar_overlap_component_proposal> &overlap_proposals,
    const std::vector<coplanar_region_incidence_proposal> &region_proposals,
    coplanar_carrier_arrangement_tables &tables,
    bounded_boolean_error &error) {
  tables = {};

  std::map<coplanar_support_key, std::vector<coplanar_support_proposal>>
      supports_by_key;
  for (const auto &proposal : support_proposals) {
    if (!valid_coplanar_support_key(proposal.key) ||
        !valid_relation_id(proposal.relation) ||
        !valid_candidate_id(proposal.candidate) ||
        digest_zero(proposal.first_source_facet_semantic_digest) ||
        digest_zero(proposal.second_source_facet_semantic_digest) ||
        proposal.original_boundary_edges.empty() ||
        !proposal.support_consistent || !proposal.orientation_consistent ||
        !proposal.symbolic_policy_consistent ||
        !proposal.precision_evidence_complete) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 coplanar support proposal is invalid");
      return false;
    }
    supports_by_key[proposal.key].push_back(proposal);
  }

  std::map<coplanar_support_key, coplanar_support_id> support_ids;
  std::map<coplanar_support_key, bounded_boolean_digest> support_digests;
  for (auto &[key, proposals] : supports_by_key) {
    const auto authority_count = std::count_if(
        proposals.begin(), proposals.end(),
        [](const auto &proposal) { return proposal.designated_authority; });
    if (authority_count != 1) {
      error = coplanar_error(
          intersection_subcode::overlap_carrier_invalid,
          "Component 08 coplanar support has no unique authority");
      return false;
    }
    const auto expected_first = proposals.front().first_source_facet_semantic_digest;
    const auto expected_second = proposals.front().second_source_facet_semantic_digest;
    auto expected_boundary_edges = proposals.front().original_boundary_edges;
    std::sort(expected_boundary_edges.begin(), expected_boundary_edges.end());
    if (std::adjacent_find(expected_boundary_edges.begin(),
                           expected_boundary_edges.end()) !=
        expected_boundary_edges.end() ||
        std::any_of(expected_boundary_edges.begin(), expected_boundary_edges.end(),
                    [](const auto &edge) {
                      return !valid_relation_feature_key(edge) ||
                             edge.kind != relation_feature_kind::source_edge;
                    })) {
      error = coplanar_error(
          intersection_subcode::overlap_carrier_invalid,
          "Component 08 coplanar support boundary edge set is invalid");
      return false;
    }
    for (const auto &proposal : proposals) {
      auto boundary_edges = proposal.original_boundary_edges;
      std::sort(boundary_edges.begin(), boundary_edges.end());
      if (proposal.first_source_facet_semantic_digest != expected_first ||
          proposal.second_source_facet_semantic_digest != expected_second ||
          boundary_edges != expected_boundary_edges) {
        error = coplanar_error(
            intersection_subcode::overlap_carrier_invalid,
            "Component 08 coplanar support semantic digests disagree");
        return false;
      }
    }
    std::sort(proposals.begin(), proposals.end(), [](const auto &a, const auto &b) {
      return std::tie(a.relation, a.candidate, a.designated_authority) <
             std::tie(b.relation, b.candidate, b.designated_authority);
    });
    const auto duplicate = std::adjacent_find(
        proposals.begin(), proposals.end(), [](const auto &a, const auto &b) {
          return a.relation == b.relation && a.candidate == b.candidate;
        });
    if (duplicate != proposals.end()) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 coplanar support duplicates provenance");
      return false;
    }

    coplanar_support_record record;
    record.id = coplanar_support_id{tables.supports.size()};
    record.key = key;
    record.first_facet = key.first_facet;
    record.second_facet = key.second_facet;
    record.support_lineage = key.support_lineage;
    record.opposite_orientation = key.opposite_orientation;
    record.symbolic_owner = key.symbolic_owner;
    record.provenance.begin = tables.support_relation_provenance.size();
    for (const auto &proposal : proposals) {
      tables.support_relation_provenance.push_back(proposal.relation);
      tables.support_candidate_provenance.push_back(proposal.candidate);
    }
    record.provenance.count = proposals.size();
    record.original_boundary_edges.begin =
        tables.support_original_boundary_edge_index.size();
    tables.support_original_boundary_edge_index.insert(
        tables.support_original_boundary_edge_index.end(),
        expected_boundary_edges.begin(), expected_boundary_edges.end());
    record.original_boundary_edges.count = expected_boundary_edges.size();
    support_ids.emplace(key, record.id);
    support_digests.emplace(key, support_semantic_digest(proposals.front()));
    tables.supports.push_back(record);
  }

  auto find_support_for_carrier = [&](const collinear_overlap_carrier_key &key)
      -> std::map<coplanar_support_key, coplanar_support_id>::const_iterator {
    auto found = support_ids.end();
    for (auto it = support_ids.begin(); it != support_ids.end(); ++it) {
      const auto &support = it->first;
      if (support.support_lineage == key.coplanar_support_lineage &&
          support.first_facet.operand == key.first_edge.operand &&
          support.second_facet.operand == key.second_edge.operand) {
        if (found != support_ids.end())
          return support_ids.end();
        found = it;
      }
    }
    return found;
  };

  std::vector<collinear_overlap_carrier_proposal> sorted_carriers =
      carrier_proposals;
  std::sort(sorted_carriers.begin(), sorted_carriers.end(),
            [](const auto &a, const auto &b) { return a.key < b.key; });
  if (std::adjacent_find(sorted_carriers.begin(), sorted_carriers.end(),
                         [](const auto &a, const auto &b) {
                           return a.key == b.key;
                         }) != sorted_carriers.end()) {
    error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                           "Component 08 collinear overlap carrier is duplicated");
    return false;
  }

  std::map<collinear_overlap_carrier_key, collinear_overlap_carrier_id>
      carrier_ids;
  std::map<coplanar_support_id, std::vector<collinear_overlap_carrier_id>>
      support_carriers;
  for (auto proposal : sorted_carriers) {
    if (!valid_collinear_overlap_carrier_key(proposal.key) ||
        !valid_construction_id(proposal.first_parameter_interval) ||
        !valid_construction_id(proposal.second_parameter_interval) ||
        !valid_interval_id(proposal.first_parameter_evidence) ||
        !valid_interval_id(proposal.second_parameter_evidence) ||
        !valid_parameter_interval<T>(proposal.first_lower_bits,
                                     proposal.first_upper_bits,
                                     proposal.first_domain) ||
        !valid_parameter_interval<T>(proposal.second_lower_bits,
                                     proposal.second_upper_bits,
                                     proposal.second_domain) ||
        !valid_occurrence_id(proposal.start_occurrence) ||
        !valid_occurrence_id(proposal.end_occurrence) ||
        !valid_intersection_occurrence_key(proposal.start_occurrence_key) ||
        !valid_intersection_occurrence_key(proposal.end_occurrence_key) ||
        !valid_relation_feature_key(proposal.start_source_vertex) ||
        !valid_relation_feature_key(proposal.end_source_vertex) ||
        proposal.start_source_vertex.kind != relation_feature_kind::source_vertex ||
        proposal.end_source_vertex.kind != relation_feature_kind::source_vertex ||
        !valid_relation_id(proposal.relation) ||
        !valid_candidate_id(proposal.candidate) ||
        !proposal.first_original_source_edge ||
        !proposal.second_original_source_edge || !proposal.first_direction_valid ||
        !proposal.second_direction_valid ||
        !proposal.parameter_correspondence_verified ||
        !proposal.endpoint_ownership_verified ||
        !proposal.half_open_policy_consistent ||
        proposal.source_provenance.empty()) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 collinear overlap proposal is invalid");
      return false;
    }
    if (proposal.exact_zero_length == proposal.definitely_positive_length ||
        (proposal.exact_zero_length &&
         (proposal.start_occurrence != proposal.end_occurrence ||
          proposal.half_open_first || proposal.half_open_second)) ||
        (proposal.definitely_positive_length &&
         proposal.start_occurrence == proposal.end_occurrence) ||
        (proposal.half_open_first && proposal.half_open_second)) {
      error = coplanar_error(
          intersection_subcode::overlap_carrier_invalid,
          "Component 08 overlap measure or half-open ownership is inconsistent");
      return false;
    }
    const auto support_it = find_support_for_carrier(proposal.key);
    if (support_it == support_ids.end()) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 overlap carrier support is ambiguous or absent");
      return false;
    }
    std::sort(proposal.source_provenance.begin(), proposal.source_provenance.end());
    if (std::adjacent_find(proposal.source_provenance.begin(),
                           proposal.source_provenance.end()) !=
        proposal.source_provenance.end()) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 overlap source provenance is duplicated");
      return false;
    }
    for (const auto &feature : proposal.source_provenance) {
      if (!valid_relation_feature_key(feature) ||
          feature.kind == relation_feature_kind::facet_internal_diagonal) {
        error = coplanar_error(
            intersection_subcode::overlap_carrier_invalid,
            "Component 08 internal diagonals cannot own overlap carriers");
        return false;
      }
    }

    collinear_overlap_carrier_record record;
    record.id = collinear_overlap_carrier_id{tables.carriers.size()};
    record.key = proposal.key;
    record.first_parameter_interval = proposal.first_parameter_interval;
    record.second_parameter_interval = proposal.second_parameter_interval;
    record.first_parameter_evidence = proposal.first_parameter_evidence;
    record.second_parameter_evidence = proposal.second_parameter_evidence;
    record.start_occurrence = proposal.start_occurrence;
    record.end_occurrence = proposal.end_occurrence;
    record.start_source_vertex = proposal.start_source_vertex;
    record.end_source_vertex = proposal.end_source_vertex;
    record.symbolic_owner = proposal.key.symbolic_owner;
    record.half_open_first = proposal.half_open_first;
    record.half_open_second = proposal.half_open_second;
    record.separate_sheet_required = proposal.separate_sheet_required;
    record.parameter_correspondence_verified = true;
    record.zero_length = proposal.exact_zero_length;
    record.provenance.begin = tables.carrier_relation_provenance.size();
    tables.carrier_relation_provenance.push_back(proposal.relation);
    tables.carrier_candidate_provenance.push_back(proposal.candidate);
    record.provenance.count = 1;
    record.source_provenance.begin = tables.carrier_source_provenance.size();
    tables.carrier_source_provenance.insert(
        tables.carrier_source_provenance.end(), proposal.source_provenance.begin(),
        proposal.source_provenance.end());
    record.source_provenance.count = proposal.source_provenance.size();
    carrier_ids.emplace(proposal.key, record.id);
    support_carriers[support_it->second].push_back(record.id);
    tables.carriers.push_back(record);
  }

  std::vector<coplanar_overlap_component_proposal> sorted_overlaps =
      overlap_proposals;
  std::sort(sorted_overlaps.begin(), sorted_overlaps.end(),
            [](const auto &a, const auto &b) { return a.key < b.key; });
  if (std::adjacent_find(sorted_overlaps.begin(), sorted_overlaps.end(),
                         [](const auto &a, const auto &b) {
                           return a.key == b.key;
                         }) != sorted_overlaps.end()) {
    error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                           "Component 08 coplanar overlap component is duplicated");
    return false;
  }

  std::map<coplanar_overlap_key, coplanar_overlap_record_id> overlap_ids;
  std::map<coplanar_support_id, std::vector<coplanar_overlap_record_id>>
      support_overlaps;
  for (auto proposal : sorted_overlaps) {
    const auto support_it = support_ids.find(proposal.key.support);
    if (!valid_coplanar_overlap_key(proposal.key) ||
        support_it == support_ids.end() ||
        proposal.component07_component.ordinal() == intersection_invalid_ordinal ||
        !valid_relation_id(proposal.relation) ||
        !proposal.component_assembly_complete ||
        proposal.closed !=
            (proposal.key.component_kind ==
                 relation_coplanar_component_kind::area_boundary ||
             proposal.key.component_kind ==
                 relation_coplanar_component_kind::coincident_sheet_boundary) ||
        proposal.distinct_sheet_occurrences !=
            (proposal.key.sheet_mask != 1 && proposal.key.sheet_mask != 2)) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 coplanar overlap component is invalid");
      return false;
    }
    std::sort(proposal.boundary_events.begin(), proposal.boundary_events.end());
    std::sort(proposal.boundary_carriers.begin(),
              proposal.boundary_carriers.end());
    if (std::adjacent_find(proposal.boundary_events.begin(),
                           proposal.boundary_events.end()) !=
            proposal.boundary_events.end() ||
        std::adjacent_find(proposal.boundary_carriers.begin(),
                           proposal.boundary_carriers.end()) !=
            proposal.boundary_carriers.end()) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 overlap component members are duplicated");
      return false;
    }
    if ((proposal.key.component_kind ==
             relation_coplanar_component_kind::isolated_point &&
         (proposal.boundary_events.size() != 1 ||
          !proposal.boundary_carriers.empty() || !proposal.zero_measure)) ||
        (proposal.key.component_kind !=
             relation_coplanar_component_kind::isolated_point &&
         proposal.boundary_events.empty())) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 overlap component topology is invalid");
      return false;
    }

    coplanar_overlap_record record;
    record.id = coplanar_overlap_record_id{tables.overlaps.size()};
    record.key = proposal.key;
    record.support = support_it->second;
    record.component07_component = proposal.component07_component;
    record.kind = proposal.key.component_kind;
    record.symbolic_owner = proposal.key.symbolic_owner;
    record.sheet_mask = proposal.key.sheet_mask;
    record.closed = proposal.closed;
    record.distinct_sheet_occurrences = proposal.distinct_sheet_occurrences;
    record.zero_measure = proposal.zero_measure;
    record.boundary_events.begin = tables.overlap_boundary_event_index.size();
    tables.overlap_boundary_event_index.insert(
        tables.overlap_boundary_event_index.end(), proposal.boundary_events.begin(),
        proposal.boundary_events.end());
    record.boundary_events.count = proposal.boundary_events.size();
    record.boundary_carriers.begin =
        tables.overlap_boundary_carrier_index.size();
    for (const auto &key : proposal.boundary_carriers) {
      const auto carrier = carrier_ids.find(key);
      if (carrier == carrier_ids.end()) {
        error = coplanar_error(
            intersection_subcode::overlap_carrier_invalid,
            "Component 08 overlap component references an absent carrier");
        return false;
      }
      tables.overlap_boundary_carrier_index.push_back(carrier->second);
    }
    record.boundary_carriers.count = proposal.boundary_carriers.size();
    record.provenance.begin = tables.overlap_relation_provenance.size();
    tables.overlap_relation_provenance.push_back(proposal.relation);
    record.provenance.count = 1;
    canonical_writer writer;
    encode_coplanar_overlap_key(writer, record.key);
    writer.u64(record.component07_component.ordinal());
    writer.u64(record.boundary_events.count);
    for (std::uint64_t i = 0; i < record.boundary_events.count; ++i)
      writer.u64(tables.overlap_boundary_event_index[
                     record.boundary_events.begin + i]
                     .ordinal());
    writer.u64(record.boundary_carriers.count);
    for (std::uint64_t i = 0; i < record.boundary_carriers.count; ++i)
      writer.u64(tables.overlap_boundary_carrier_index[
                     record.boundary_carriers.begin + i]
                     .ordinal());
    record.component_digest = sha256::digest(writer.bytes());
    overlap_ids.emplace(proposal.key, record.id);
    support_overlaps[record.support].push_back(record.id);
    tables.overlaps.push_back(record);
  }

  std::vector<coplanar_region_incidence_proposal> sorted_regions =
      region_proposals;
  std::sort(sorted_regions.begin(), sorted_regions.end(), [](const auto &a,
                                                             const auto &b) {
    return std::tie(a.support, a.component, a.first_triangle,
                    a.second_triangle, a.classification, a.sheet_mask) <
           std::tie(b.support, b.component, b.first_triangle,
                    b.second_triangle, b.classification, b.sheet_mask);
  });
  std::map<coplanar_support_id, std::vector<coplanar_region_incidence_id>>
      support_regions;
  for (auto proposal : sorted_regions) {
    const auto support_it = support_ids.find(proposal.support);
    const auto overlap_it = overlap_ids.find(proposal.component);
    if (support_it == support_ids.end() || overlap_it == overlap_ids.end() ||
        !(proposal.component.support == proposal.support) ||
        proposal.first_facet != proposal.support.first_facet ||
        proposal.second_facet != proposal.support.second_facet ||
        !valid_relation_feature_key(proposal.first_triangle) ||
        !valid_relation_feature_key(proposal.second_triangle) ||
        proposal.first_triangle.kind != relation_feature_kind::source_triangle ||
        proposal.second_triangle.kind != relation_feature_kind::source_triangle ||
        proposal.first_triangle.operand != proposal.first_facet.operand ||
        proposal.second_triangle.operand != proposal.second_facet.operand ||
        proposal.symbolic_owner != proposal.support.symbolic_owner ||
        proposal.sheet_mask != proposal.component.sheet_mask ||
        proposal.boundary_events.empty() ||
        !classification_matches(proposal.classification, proposal.relation_status) ||
        !proposal.coverage_complete ||
        !proposal.internal_diagonals_coverage_only ||
        !proposal.source_facet_semantics_verified ||
        proposal.source_facet_semantic_digest !=
            support_digests.at(proposal.support)) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 coplanar region incidence is invalid");
      return false;
    }
    const auto classification_value =
        static_cast<std::uint8_t>(proposal.classification);
    if (classification_value < 1 || classification_value > 8) {
      error = coplanar_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 coplanar classification is invalid");
      return false;
    }
    std::sort(proposal.boundary_events.begin(), proposal.boundary_events.end());
    std::sort(proposal.boundary_carriers.begin(),
              proposal.boundary_carriers.end());
    std::sort(proposal.coverage_witnesses.begin(),
              proposal.coverage_witnesses.end());
    for (const auto &witness : proposal.coverage_witnesses) {
      if (!valid_relation_feature_key(witness) ||
          (witness.kind != relation_feature_kind::source_triangle &&
           witness.kind != relation_feature_kind::facet_internal_diagonal)) {
        error = coplanar_error(
            intersection_subcode::overlap_carrier_invalid,
            "Component 08 coplanar coverage witness is invalid");
        return false;
      }
    }

    coplanar_region_incidence_record record;
    record.id = coplanar_region_incidence_id{tables.regions.size()};
    record.support = support_it->second;
    record.first_facet = proposal.first_facet;
    record.second_facet = proposal.second_facet;
    record.first_triangle = proposal.first_triangle;
    record.second_triangle = proposal.second_triangle;
    record.component = overlap_it->second;
    record.component_lineage = proposal.component.component_lineage;
    record.classification = proposal.classification;
    record.relation_status = proposal.relation_status;
    record.symbolic_owner = proposal.symbolic_owner;
    record.sheet_mask = proposal.sheet_mask;
    record.internal_diagonal_coverage_only = true;
    record.coverage_complete = true;
    record.boundary_events.begin = tables.region_boundary_event_index.size();
    tables.region_boundary_event_index.insert(
        tables.region_boundary_event_index.end(), proposal.boundary_events.begin(),
        proposal.boundary_events.end());
    record.boundary_events.count = proposal.boundary_events.size();
    record.boundary_carriers.begin =
        tables.region_boundary_carrier_index.size();
    for (const auto &key : proposal.boundary_carriers) {
      const auto carrier = carrier_ids.find(key);
      if (carrier == carrier_ids.end()) {
        error = coplanar_error(
            intersection_subcode::overlap_carrier_invalid,
            "Component 08 region references an absent boundary carrier");
        return false;
      }
      tables.region_boundary_carrier_index.push_back(carrier->second);
    }
    record.boundary_carriers.count = proposal.boundary_carriers.size();
    record.coverage_witnesses.begin =
        tables.region_coverage_witness_index.size();
    tables.region_coverage_witness_index.insert(
        tables.region_coverage_witness_index.end(),
        proposal.coverage_witnesses.begin(), proposal.coverage_witnesses.end());
    record.coverage_witnesses.count = proposal.coverage_witnesses.size();
    record.source_facet_semantic_digest = proposal.source_facet_semantic_digest;
    support_regions[record.support].push_back(record.id);
    tables.regions.push_back(record);
  }

  for (auto &support : tables.supports) {
    auto &carriers = support_carriers[support.id];
    auto &overlaps = support_overlaps[support.id];
    auto &regions = support_regions[support.id];
    std::vector<event_occurrence_id> boundary_events;
    for (const auto overlap_id : overlaps) {
      const auto &overlap = tables.overlaps[overlap_id.ordinal()];
      for (std::uint64_t offset = 0; offset < overlap.boundary_events.count;
           ++offset)
        boundary_events.push_back(tables.overlap_boundary_event_index[
            overlap.boundary_events.begin + offset]);
    }
    std::sort(boundary_events.begin(), boundary_events.end());
    boundary_events.erase(
        std::unique(boundary_events.begin(), boundary_events.end()),
        boundary_events.end());
    support.boundary_events.begin = tables.support_boundary_event_index.size();
    tables.support_boundary_event_index.insert(
        tables.support_boundary_event_index.end(), boundary_events.begin(),
        boundary_events.end());
    support.boundary_events.count = boundary_events.size();
    support.boundary_carriers.begin = tables.support_boundary_carrier_index.size();
    tables.support_boundary_carrier_index.insert(
        tables.support_boundary_carrier_index.end(), carriers.begin(),
        carriers.end());
    support.boundary_carriers.count = carriers.size();
    support.overlap_components.begin = tables.support_overlap_index.size();
    tables.support_overlap_index.insert(tables.support_overlap_index.end(),
                                        overlaps.begin(), overlaps.end());
    support.overlap_components.count = overlaps.size();
    support.region_incidence.begin = tables.support_region_index.size();
    tables.support_region_index.insert(tables.support_region_index.end(),
                                       regions.begin(), regions.end());
    support.region_incidence.count = regions.size();
    if (support.overlap_components.count == 0 ||
        support.region_incidence.count == 0) {
      error = coplanar_error(
          intersection_subcode::overlap_carrier_invalid,
          "Component 08 coplanar support has no overlap component or region");
      return false;
    }
  }

  return true;
}

void encode_tables(canonical_writer &writer,
                   const coplanar_carrier_arrangement_tables &tables) {
  writer.u64(tables.supports.size());
  for (const auto &record : tables.supports) {
    writer.u64(record.id.ordinal());
    encode_coplanar_support_key(writer, record.key);
    writer.u64(record.original_boundary_edges.begin);
    writer.u64(record.original_boundary_edges.count);
    writer.u64(record.boundary_events.begin);
    writer.u64(record.boundary_events.count);
    writer.u64(record.boundary_carriers.begin);
    writer.u64(record.boundary_carriers.count);
    writer.u64(record.overlap_components.begin);
    writer.u64(record.overlap_components.count);
    writer.u64(record.region_incidence.begin);
    writer.u64(record.region_incidence.count);
    writer.u64(record.provenance.begin);
    writer.u64(record.provenance.count);
  }
  auto ids = [&](const auto &values) {
    writer.u64(values.size());
    for (const auto value : values)
      writer.u64(value.ordinal());
  };
  ids(tables.support_relation_provenance);
  ids(tables.support_candidate_provenance);
  writer.u64(tables.support_original_boundary_edge_index.size());
  for (const auto &edge : tables.support_original_boundary_edge_index)
    encode_relation_feature_key(writer, edge);
  ids(tables.support_boundary_event_index);
  ids(tables.support_boundary_carrier_index);
  ids(tables.support_overlap_index);
  ids(tables.support_region_index);
  writer.u64(tables.carriers.size());
  for (const auto &record : tables.carriers) {
    writer.u64(record.id.ordinal());
    encode_collinear_overlap_carrier_key(writer, record.key);
    writer.u64(record.first_parameter_interval.ordinal());
    writer.u64(record.second_parameter_interval.ordinal());
    writer.u64(record.first_parameter_evidence.ordinal());
    writer.u64(record.second_parameter_evidence.ordinal());
    writer.u64(record.start_occurrence.ordinal());
    writer.u64(record.end_occurrence.ordinal());
    encode_relation_feature_key(writer, record.start_source_vertex);
    encode_relation_feature_key(writer, record.end_source_vertex);
    writer.boolean(record.half_open_first);
    writer.boolean(record.half_open_second);
    writer.boolean(record.separate_sheet_required);
    writer.boolean(record.parameter_correspondence_verified);
    writer.boolean(record.zero_length);
    writer.u64(record.provenance.begin);
    writer.u64(record.provenance.count);
    writer.u64(record.source_provenance.begin);
    writer.u64(record.source_provenance.count);
  }
  ids(tables.carrier_relation_provenance);
  ids(tables.carrier_candidate_provenance);
  writer.u64(tables.carrier_source_provenance.size());
  for (const auto &feature : tables.carrier_source_provenance)
    encode_relation_feature_key(writer, feature);
  writer.u64(tables.overlaps.size());
  for (const auto &record : tables.overlaps) {
    writer.u64(record.id.ordinal());
    encode_coplanar_overlap_key(writer, record.key);
    writer.u64(record.support.ordinal());
    writer.u64(record.component07_component.ordinal());
    writer.boolean(record.closed);
    writer.boolean(record.distinct_sheet_occurrences);
    writer.boolean(record.zero_measure);
    writer.u64(record.boundary_events.begin);
    writer.u64(record.boundary_events.count);
    writer.u64(record.boundary_carriers.begin);
    writer.u64(record.boundary_carriers.count);
    write_digest(writer, record.component_digest);
  }
  ids(tables.overlap_boundary_event_index);
  ids(tables.overlap_boundary_carrier_index);
  ids(tables.overlap_relation_provenance);
  writer.u64(tables.regions.size());
  for (const auto &record : tables.regions) {
    writer.u64(record.id.ordinal());
    writer.u64(record.support.ordinal());
    writer.u64(record.component.ordinal());
    writer.u64(record.component_lineage);
    writer.u8(static_cast<std::uint8_t>(record.classification));
    writer.u8(static_cast<std::uint8_t>(record.relation_status));
    writer.u8(static_cast<std::uint8_t>(record.symbolic_owner));
    writer.u8(record.sheet_mask);
    writer.boolean(record.internal_diagonal_coverage_only);
    writer.boolean(record.coverage_complete);
    writer.u64(record.boundary_events.begin);
    writer.u64(record.boundary_events.count);
    writer.u64(record.boundary_carriers.begin);
    writer.u64(record.boundary_carriers.count);
    writer.u64(record.coverage_witnesses.begin);
    writer.u64(record.coverage_witnesses.count);
    write_digest(writer, record.source_facet_semantic_digest);
  }
  ids(tables.region_boundary_event_index);
  ids(tables.region_boundary_carrier_index);
  writer.u64(tables.region_coverage_witness_index.size());
  for (const auto &feature : tables.region_coverage_witness_index)
    encode_relation_feature_key(writer, feature);
}

} // namespace

template <class T>
bool build_coplanar_carrier_arrangements(
    const std::vector<coplanar_support_proposal> &support_proposals,
    const std::vector<collinear_overlap_carrier_proposal> &carrier_proposals,
    const std::vector<coplanar_overlap_component_proposal> &overlap_proposals,
    const std::vector<coplanar_region_incidence_proposal> &region_proposals,
    coplanar_carrier_arrangement_tables &tables,
    bounded_boolean_error &error) {
  return build_impl<T>(support_proposals, carrier_proposals, overlap_proposals,
                       region_proposals, tables, error);
}

template <class T>
bool verify_coplanar_carrier_arrangements(
    const std::vector<coplanar_support_proposal> &support_proposals,
    const std::vector<collinear_overlap_carrier_proposal> &carrier_proposals,
    const std::vector<coplanar_overlap_component_proposal> &overlap_proposals,
    const std::vector<coplanar_region_incidence_proposal> &region_proposals,
    const coplanar_carrier_arrangement_tables &tables,
    bounded_boolean_error &error) {
  coplanar_carrier_arrangement_tables rebuilt;
  if (!build_impl<T>(support_proposals, carrier_proposals, overlap_proposals,
                     region_proposals, rebuilt, error))
    return false;
  canonical_writer expected;
  canonical_writer actual;
  encode_tables(expected, rebuilt);
  encode_tables(actual, tables);
  if (expected.bytes() != actual.bytes()) {
    error = coplanar_error(
        intersection_subcode::verifier_rejection,
        "Component 08 coplanar arrangement reconstruction disagrees");
    return false;
  }
  return true;
}

template bool build_coplanar_carrier_arrangements<float>(
    const std::vector<coplanar_support_proposal> &,
    const std::vector<collinear_overlap_carrier_proposal> &,
    const std::vector<coplanar_overlap_component_proposal> &,
    const std::vector<coplanar_region_incidence_proposal> &,
    coplanar_carrier_arrangement_tables &, bounded_boolean_error &);
template bool build_coplanar_carrier_arrangements<double>(
    const std::vector<coplanar_support_proposal> &,
    const std::vector<collinear_overlap_carrier_proposal> &,
    const std::vector<coplanar_overlap_component_proposal> &,
    const std::vector<coplanar_region_incidence_proposal> &,
    coplanar_carrier_arrangement_tables &, bounded_boolean_error &);
template bool verify_coplanar_carrier_arrangements<float>(
    const std::vector<coplanar_support_proposal> &,
    const std::vector<collinear_overlap_carrier_proposal> &,
    const std::vector<coplanar_overlap_component_proposal> &,
    const std::vector<coplanar_region_incidence_proposal> &,
    const coplanar_carrier_arrangement_tables &, bounded_boolean_error &);
template bool verify_coplanar_carrier_arrangements<double>(
    const std::vector<coplanar_support_proposal> &,
    const std::vector<collinear_overlap_carrier_proposal> &,
    const std::vector<coplanar_overlap_component_proposal> &,
    const std::vector<coplanar_region_incidence_proposal> &,
    const coplanar_carrier_arrangement_tables &, bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
