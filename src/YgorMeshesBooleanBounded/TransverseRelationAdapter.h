#pragma once

#include "EventInterning.h"
#include "RelationQueries.h"
#include "TransverseCarrierArrangements.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace transverse_relation_adapter_detail {

inline bounded_boolean_error adapter_error(intersection_subcode subcode,
                                           const char *summary) {
  return intersection_error(subcode,
                            bounded_boolean_error_category::internal_invariant_error,
                            summary,
                            intersection_checkpoint::transverse_carriers);
}

inline bool checked_range(std::uint64_t begin, std::uint64_t count,
                          std::size_t size) noexcept {
  return begin <= size && count <= size - static_cast<std::size_t>(begin);
}

inline bool valid_facet_pair(const relation_feature_key &first,
                             const relation_feature_key &second) noexcept {
  return first.kind == relation_feature_kind::source_facet &&
         second.kind == relation_feature_kind::source_facet &&
         first.operand != second.operand && first < second;
}

} // namespace transverse_relation_adapter_detail

template <class T, class I>
bool collect_component07_transverse_carrier_proposals(
    const signed_feature_relations_view<T, I> &relations,
    std::vector<transverse_carrier_proposal> &proposals,
    bounded_boolean_error &error) {
  proposals.clear();

  if (!relations.valid_owner()) {
    error = transverse_relation_adapter_detail::adapter_error(
        intersection_subcode::predecessor_not_verified,
        "Component 08 transverse adapter requires a checked Component 07 view");
    return false;
  }

  const auto &graph = relations.request_graph();
  const auto &constructions = relations.constructions();
  const auto &feature_relations = relations.relations();

  for (const auto &construction : constructions) {
    if (construction.kind != relation_construction_kind::bounded_carrier)
      continue;
    const auto support_count = std::count_if(
        relations.transverse_carrier_supports().begin(),
        relations.transverse_carrier_supports().end(), [&](const auto &support) {
          return support.construction == construction.id &&
                 support.relation == construction.source_relation;
        });
    if (support_count != 1) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse carrier lacks one public support record");
      return false;
    }
  }
  for (const auto &relation : feature_relations) {
    if (relation.family != feature_relation_family::source_facet_source_facet ||
        relation.status != feature_relation_status::proper_crossing)
      continue;
    const auto support_count = std::count_if(
        relations.transverse_carrier_supports().begin(),
        relations.transverse_carrier_supports().end(),
        [&](const auto &support) { return support.relation == relation.id; });
    if (support_count != 1) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 public transverse relation lacks one support record");
      return false;
    }
  }

  for (const auto &support : relations.transverse_carrier_supports()) {
    if (support.relation.ordinal() >= feature_relations.size()) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter found malformed Component 07 relation identity");
      return false;
    }
    const auto &relation = feature_relations[support.relation.ordinal()];
    if (relation.id != support.relation ||
        relation.family != feature_relation_family::source_facet_source_facet ||
        relation.status != feature_relation_status::proper_crossing ||
        relation.producer.ordinal() >= graph.requests.size()) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse support does not name a public crossing");
      return false;
    }

    const auto &request = graph.requests[relation.producer.ordinal()];
    if (request.id != relation.producer ||
        request.key.family != relation_request_family::source_facet_source_facet ||
        !transverse_relation_adapter_detail::valid_facet_pair(request.key.first,
                                                               request.key.second) ||
        !transverse_relation_adapter_detail::checked_range(
            request.witness_begin, request.witness_count,
            graph.candidate_witnesses.size()) ||
        request.witness_count == 0) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter found malformed facet/facet request lineage");
      return false;
    }

    if (support.first_facet != request.key.first ||
        support.second_facet != request.key.second ||
        !support.support_consistent || !support.orientation_consistent ||
        !support.residuals_accepted || !support.precision_evidence_complete ||
        support.schema_version !=
            contract_versions::relation_transverse_support_schema ||
        support.reserved16 != 0 || support.reserved32 != 0) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter could not reconcile detailed carrier support");
      return false;
    }
    const relation_construction_record *construction = nullptr;
    for (const auto &candidate : constructions) {
      if (candidate.id != support.construction ||
          candidate.source_relation != relation.id ||
          candidate.kind != relation_construction_kind::bounded_carrier)
        continue;
      if (construction != nullptr) {
        error = transverse_relation_adapter_detail::adapter_error(
            intersection_subcode::authoritative_construction_conflict,
            "Component 08 transverse adapter found multiple authoritative carrier constructions");
        return false;
      }
      construction = &candidate;
    }
    if (construction == nullptr ||
        construction->id.ordinal() >= constructions.size() ||
        construction->precedence !=
            relation_construction_precedence::source_facet_source_facet_carrier ||
        construction->coordinate_space !=
            relation_construction_coordinate_space::world_3d ||
        construction->component_count != 6 || !construction->finite ||
        !construction->tolerance_compatible ||
        !construction->precision_evidence_complete) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter found incomplete authoritative carrier construction");
      return false;
    }
    if (construction->geometric_lineage == 0) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 07 transverse carrier construction lacks the nonzero geometric lineage required by the Component 08 carrier key");
      return false;
    }
    const auto actual_memberships = static_cast<std::uint64_t>(std::count_if(
        relations.transverse_carrier_memberships().begin(),
        relations.transverse_carrier_memberships().end(),
        [&](const auto &membership) {
          return membership.carrier_relation == support.relation;
        }));
    if (actual_memberships != support.expected_membership_count) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse carrier membership population is incomplete");
      return false;
    }
    if (support.expected_membership_count == 0)
      continue;

    transverse_carrier_key key;
    key.first_facet = request.key.first;
    key.second_facet = request.key.second;
    key.construction = construction->id;
    key.construction_lineage = construction->geometric_lineage;
    key.orientation = carrier_orientation_role::canonical_forward;
    if (!valid_transverse_carrier_key(key)) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter produced an invalid canonical carrier key");
      return false;
    }

    for (std::uint64_t offset = 0; offset < request.witness_count; ++offset) {
      const auto candidate = graph.candidate_witnesses[request.witness_begin + offset];
      if (candidate.ordinal() == intersection_invalid_ordinal) {
        error = transverse_relation_adapter_detail::adapter_error(
            intersection_subcode::transverse_carrier_invalid,
            "Component 08 transverse adapter found an invalid candidate witness");
        return false;
      }
      transverse_carrier_proposal proposal;
      proposal.key = key;
      proposal.construction = construction->id;
      proposal.relation = relation.id;
      proposal.candidate = candidate;
      proposal.relation_lineage = construction->geometric_lineage;
      proposal.designated_authority = offset == 0;
      proposal.support_consistent = true;
      proposal.orientation_consistent = true;
      proposal.residuals_accepted = true;
      proposal.precision_evidence_complete = true;
      proposal.coplanar = false;
      proposals.push_back(proposal);
    }
  }

  std::sort(proposals.begin(), proposals.end(), [](const auto &a, const auto &b) {
    return std::tie(a.key, a.relation, a.candidate, a.relation_lineage) <
           std::tie(b.key, b.relation, b.candidate, b.relation_lineage);
  });
  return true;
}

template <class T, class I>
bool verify_component07_transverse_carrier_proposals(
    const signed_feature_relations_view<T, I> &relations,
    const std::vector<transverse_carrier_proposal> &proposals,
    bounded_boolean_error &error) {
  std::vector<transverse_carrier_proposal> reconstructed;
  bounded_boolean_error local;
  if (!collect_component07_transverse_carrier_proposals(relations, reconstructed,
                                                        local)) {
    error = transverse_relation_adapter_detail::adapter_error(
        intersection_subcode::verifier_rejection,
        "Component 08 verifier could not reconstruct Component 07 transverse carriers");
    return false;
  }
  const auto equal = [](const transverse_carrier_proposal &a,
                        const transverse_carrier_proposal &b) noexcept {
    return a.key == b.key && a.construction == b.construction &&
           a.relation == b.relation && a.candidate == b.candidate &&
           a.relation_lineage == b.relation_lineage &&
           a.designated_authority == b.designated_authority &&
           a.support_consistent == b.support_consistent &&
           a.orientation_consistent == b.orientation_consistent &&
           a.residuals_accepted == b.residuals_accepted &&
           a.precision_evidence_complete == b.precision_evidence_complete &&
           a.coplanar == b.coplanar;
  };
  if (reconstructed.size() != proposals.size() ||
      !std::equal(reconstructed.begin(), reconstructed.end(), proposals.begin(),
                  equal)) {
    error = transverse_relation_adapter_detail::adapter_error(
        intersection_subcode::verifier_rejection,
        "Component 08 transverse carrier proposal reconstruction mismatch");
    return false;
  }
  return true;
}

template <class T, class I>
bool collect_component07_transverse_membership_proposals(
    const signed_feature_relations_view<T, I> &relations,
    const event_interning_tables &interning,
    std::vector<carrier_membership_proposal> &memberships,
    std::vector<transverse_relation_interval_proposal> &intervals,
    bounded_boolean_error &error) {
  memberships.clear();
  intervals.clear();
  std::vector<transverse_carrier_proposal> carriers;
  if (!collect_component07_transverse_carrier_proposals(relations, carriers,
                                                        error))
    return false;
  for (const auto &record : relations.transverse_carrier_memberships()) {
    if (record.carrier_relation.ordinal() >= relations.relations().size() ||
        record.carrier_construction.ordinal() >=
            relations.constructions().size() ||
        record.member_relation.ordinal() >= relations.relations().size() ||
        record.seed.ordinal() >= interning.seed_to_occurrence.size() ||
        record.seed.ordinal() >= interning.seed_to_event.size() ||
        record.parameter.ordinal() >= relations.interval_evidence().size() ||
        record.first_region.ordinal() >=
            relations.source_facet_regions().size() ||
        record.second_region.ordinal() >=
            relations.source_facet_regions().size()) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::membership_incomplete,
          "Component 08 transverse adapter found incomplete Component 07 membership references");
      return false;
    }
    const auto carrier = std::find_if(
        carriers.begin(), carriers.end(), [&](const auto &candidate) {
          return candidate.relation == record.carrier_relation &&
                 candidate.construction == record.carrier_construction;
        });
    if (carrier == carriers.end()) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse membership has no unique carrier");
      return false;
    }
    const auto occurrence = interning.seed_to_occurrence[record.seed.ordinal()];
    const auto event = interning.seed_to_event[record.seed.ordinal()];
    if (occurrence.ordinal() >= interning.occurrences.size() ||
        event.ordinal() >= interning.events.size()) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::membership_incomplete,
          "Component 08 transverse membership seed was not interned");
      return false;
    }
    const auto &parameter =
        relations.interval_evidence()[record.parameter.ordinal()];
    carrier_membership_proposal proposal;
    proposal.carrier = carrier->key;
    proposal.occurrence_key = interning.occurrences[occurrence.ordinal()].key;
    proposal.occurrence = occurrence;
    proposal.event = event;
    proposal.parameter = parameter.id;
    proposal.nominal_bits = parameter.rounded_nominal_bits;
    proposal.lower_bits = parameter.lower_bits;
    proposal.upper_bits = parameter.upper_bits;
    proposal.parameter_lineage = record.parameter_lineage;
    proposal.event_lineage = record.event_lineage;
    proposal.relation_lineage = record.carrier_lineage;
    proposal.relation = record.carrier_relation;
    const auto shared_point_count = std::count_if(
        relations.transverse_carrier_memberships().begin(),
        relations.transverse_carrier_memberships().end(),
        [&](const auto &candidate) {
          return candidate.carrier_relation == record.carrier_relation &&
                 candidate.carrier_construction == record.carrier_construction &&
                 candidate.point_construction == record.point_construction;
        });
    proposal.exact_equal_eligible = shared_point_count > 1;
    proposal.cluster_eligible = shared_point_count > 1;
    proposal.transition = record.transition;
    proposal.half_open_owner = record.half_open_owner;
    proposal.numeric_owner = record.numeric_owner;
    proposal.first_region_evidence = record.first_region;
    proposal.second_region_evidence = record.second_region;
    proposal.first_region_contains =
        relations.source_facet_regions()[record.first_region.ordinal()]
            .region.classification != source_facet_point_region_class::outside;
    proposal.second_region_contains =
        relations.source_facet_regions()[record.second_region.ordinal()]
            .region.classification != source_facet_point_region_class::outside;
    memberships.push_back(std::move(proposal));
  }

  std::sort(memberships.begin(), memberships.end(), [](const auto &a,
                                                        const auto &b) {
    const auto lower_a = from_bits<T>(
        static_cast<floating_uint_t<T>>(a.lower_bits));
    const auto lower_b = from_bits<T>(
        static_cast<floating_uint_t<T>>(b.lower_bits));
    if (!(a.carrier == b.carrier))
      return a.carrier < b.carrier;
    if (finite_numeric_less(lower_a, lower_b)) return true;
    if (finite_numeric_less(lower_b, lower_a)) return false;
    return a.occurrence_key < b.occurrence_key;
  });
  std::size_t begin = 0;
  while (begin < memberships.size()) {
    std::size_t end = begin + 1;
    while (end < memberships.size() &&
           memberships[end].carrier == memberships[begin].carrier)
      ++end;
    const carrier_membership_proposal *start = nullptr;
    for (std::size_t offset = begin; offset < end;) {
      std::size_t cluster_end = offset + 1;
      while (cluster_end < end &&
             memberships[cluster_end].event_lineage ==
                 memberships[offset].event_lineage)
        ++cluster_end;
      const carrier_membership_proposal *transition = nullptr;
      for (std::size_t member = offset; member < cluster_end; ++member) {
        if (memberships[member].transition ==
            relation_carrier_transition::tangent)
          continue;
        if (transition && transition->transition != memberships[member].transition) {
          error = transverse_relation_adapter_detail::adapter_error(
              intersection_subcode::membership_incomplete,
              "Component 08 transverse boundary lineage has contradictory transitions");
          return false;
        }
        if (!transition || memberships[member].numeric_owner)
          transition = &memberships[member];
      }
      if (!transition) {
        offset = cluster_end;
        continue;
      }
      if (!start) {
        start = transition;
        offset = cluster_end;
        continue;
      }
      const auto &finish = *transition;
      const auto start_upper = from_bits<T>(
          static_cast<floating_uint_t<T>>(start->upper_bits));
      const auto finish_lower = from_bits<T>(
          static_cast<floating_uint_t<T>>(finish.lower_bits));
      if (!finite_numeric_less(start_upper, finish_lower)) {
        error = transverse_relation_adapter_detail::adapter_error(
            intersection_subcode::parameter_invalid,
            "Component 08 transverse endpoint order is not bounded");
        return false;
      }
      transverse_relation_interval_proposal interval;
      interval.carrier = start->carrier;
      interval.relation = start->relation;
      interval.interval_lineage = start->relation_lineage;
      interval.start_parameter = start->parameter;
      interval.end_parameter = finish.parameter;
      interval.start_lower_bits = start->lower_bits;
      interval.start_upper_bits = start->upper_bits;
      interval.end_lower_bits = finish.lower_bits;
      interval.end_upper_bits = finish.upper_bits;
      interval.start_occurrence = start->occurrence;
      interval.end_occurrence = finish.occurrence;
      interval.activation =
          intersection_span_activation::active_transverse_intersection;
      interval.first_region_evidence = start->first_region_evidence;
      interval.second_region_evidence = start->second_region_evidence;
      interval.first_region_contains = start->first_region_contains;
      interval.second_region_contains = start->second_region_contains;
      interval.ownership_verified = start->numeric_owner &&
                                    finish.numeric_owner;
      interval.start_closed =
          start->half_open_owner == start->carrier.first_facet.operand;
      interval.end_closed =
          finish.half_open_owner == finish.carrier.second_facet.operand;
      intervals.push_back(std::move(interval));
      start = nullptr;
      offset = cluster_end;
    }
    if (start) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::membership_incomplete,
          "Component 08 transverse carrier has an unmatched numeric transition");
      return false;
    }
    begin = end;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
