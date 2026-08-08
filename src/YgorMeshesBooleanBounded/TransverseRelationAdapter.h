#pragma once

#include "FacetFacetRelations.h"
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

inline const source_facet_source_facet_relation_record<float> *
unused_float_record_type_anchor() noexcept {
  return nullptr;
}

template <class T>
const source_facet_source_facet_relation_record<T> *find_stage_relation(
    const candidate_source_facet_relation_stage<T> &stage,
    const relation_feature_key &first,
    const relation_feature_key &second) noexcept {
  const source_facet_source_facet_relation_record<T> *found = nullptr;
  for (const auto &record : stage.relations) {
    if (record.first_feature != first || record.second_feature != second)
      continue;
    if (found != nullptr)
      return nullptr;
    found = &record;
  }
  return found;
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
    const signed_feature_relations<T, I> &relations,
    std::vector<transverse_carrier_proposal> &proposals,
    bounded_boolean_error &error) {
  proposals.clear();

  if (relations.verification() !=
          relation_verification_disposition::independently_verified ||
      !relations.source_facet_stage()) {
    error = transverse_relation_adapter_detail::adapter_error(
        intersection_subcode::predecessor_not_verified,
        "Component 08 transverse adapter requires the verified Component 07 facet/facet stage");
    return false;
  }

  const auto &graph = relations.request_graph();
  const auto &stage = *relations.source_facet_stage();
  const auto &constructions = relations.constructions();
  const auto &feature_relations = relations.relations();

  for (const auto &relation : feature_relations) {
    if (relation.family != feature_relation_family::source_facet_source_facet ||
        relation.status != feature_relation_status::proper_crossing)
      continue;
    if (relation.id.ordinal() >= feature_relations.size() ||
        relation.producer.ordinal() >= graph.requests.size()) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter found malformed Component 07 relation identity");
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

    const auto *stage_relation =
        transverse_relation_adapter_detail::find_stage_relation(
            stage, request.key.first, request.key.second);
    if (stage_relation == nullptr ||
        stage_relation->classification !=
            source_facet_support_relation_class::transverse ||
        !stage_relation->has_transverse_carrier ||
        !stage_relation->transverse_carrier.residuals_accepted) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter could not reconcile detailed carrier support");
      return false;
    }

    const relation_construction_record *construction = nullptr;
    for (const auto &candidate : constructions) {
      if (candidate.source_relation != relation.id ||
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
        construction->component_count != 6 ||
        construction->geometric_lineage == 0 || !construction->finite ||
        !construction->tolerance_compatible ||
        !construction->precision_evidence_complete) {
      error = transverse_relation_adapter_detail::adapter_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse adapter found incomplete authoritative carrier construction");
      return false;
    }

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
    const signed_feature_relations<T, I> &relations,
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

} // namespace ygor::mesh_boolean::bounded
