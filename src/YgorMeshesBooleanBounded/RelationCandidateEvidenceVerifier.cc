#include "StrictFloatingBuild.h"
#include "RelationCandidateEvidenceVerifier.h"
#include "CoplanarRelationOverlay.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error candidate_evidence_error(relation_subcode subcode,
                                                 const char *summary) {
  return relation_error(subcode,
                        bounded_boolean_error_category::internal_invariant_error,
                        summary,
                        relation_checkpoint::independent_verification);
}

const canonical_relation_request *find_request(
    const relation_request_graph &graph,
    const relation_request_key &key) noexcept {
  const auto it = std::lower_bound(
      graph.requests.begin(), graph.requests.end(), key,
      [](const canonical_relation_request &record,
         const relation_request_key &candidate) {
        return record.key < candidate;
      });
  return it == graph.requests.end() || it->key != key ? nullptr : &*it;
}

bool request_has_dependency(const relation_request_graph &graph,
                            const canonical_relation_request &consumer,
                            relation_request_id producer) noexcept {
  if (consumer.dependency_begin > graph.dependencies.size() ||
      consumer.dependency_count >
          graph.dependencies.size() - consumer.dependency_begin)
    return false;
  for (std::uint64_t offset = 0; offset < consumer.dependency_count; ++offset)
    if (graph.dependencies[consumer.dependency_begin + offset].producer ==
        producer)
      return true;
  return false;
}

bool request_has_witness(const relation_request_graph &graph,
                         const canonical_relation_request &request,
                         candidate_id candidate) noexcept {
  if (request.witness_begin > graph.candidate_witnesses.size() ||
      request.witness_count >
          graph.candidate_witnesses.size() - request.witness_begin)
    return false;
  for (std::uint64_t offset = 0; offset < request.witness_count; ++offset)
    if (graph.candidate_witnesses[request.witness_begin + offset] == candidate)
      return true;
  return false;
}

bool public_contact(feature_relation_status value) noexcept {
  return value != feature_relation_status::not_evaluated &&
         value != feature_relation_status::definitely_separated;
}

relation_contact_dimension contact_dimension(
    feature_relation_family family, feature_relation_status status) noexcept {
  switch (status) {
  case feature_relation_status::proper_crossing:
  case feature_relation_status::endpoint_crossing:
  case feature_relation_status::point_contact:
  case feature_relation_status::tangency:
    return relation_contact_dimension::point;
  case feature_relation_status::segment_contact:
    return relation_contact_dimension::curve;
  case feature_relation_status::overlap:
  case feature_relation_status::containment:
  case feature_relation_status::coincidence_same_orientation:
  case feature_relation_status::coincidence_opposite_orientation:
    return family == feature_relation_family::source_facet_source_facet
               ? relation_contact_dimension::area
               : relation_contact_dimension::curve;
  case feature_relation_status::not_evaluated:
  case feature_relation_status::definitely_separated:
    return relation_contact_dimension::none;
  }
  return relation_contact_dimension::none;
}

bool valid_candidate_disposition(
    candidate_relation_disposition_kind disposition) noexcept {
  const auto value = static_cast<std::uint8_t>(disposition);
  return value >= static_cast<std::uint8_t>(
                      candidate_relation_disposition_kind::definitely_separated) &&
         value <= static_cast<std::uint8_t>(
                      candidate_relation_disposition_kind::
                          internal_diagonal_bookkeeping_absorbed);
}

template <class T, class I>
relation_feature_key candidate_edge_feature(
    const canonical_candidate_stream<T, I> &candidates,
    const canonical_candidate_record<T> &candidate) noexcept {
  const auto operand =
      candidate.role == directed_candidate_role::a_edge_b_triangle
          ? operand_id::a
          : operand_id::b;
  const auto &table = candidates.primitive_table(operand);
  relation_feature_key out;
  out.operand = operand;
  if (candidate.edge.ordinal() >= table.edges.size())
    return out;
  const auto &edge = table.edges[candidate.edge.ordinal()];
  if (edge.edge_class == canonical_edge_class::source_edge) {
    out.kind = relation_feature_kind::source_edge;
    out.primary = edge.semantic_key.primary;
    out.secondary = edge.semantic_key.secondary;
  } else {
    out.kind = relation_feature_kind::facet_internal_diagonal;
    out.primary = edge.source_facet;
    out.secondary = edge.source_diagonal;
  }
  return out;
}

template <class T, class I>
relation_feature_key candidate_triangle_feature(
    const canonical_candidate_stream<T, I> &candidates,
    const canonical_candidate_record<T> &candidate) noexcept {
  const auto operand =
      candidate.role == directed_candidate_role::a_edge_b_triangle
          ? operand_id::b
          : operand_id::a;
  const auto &table = candidates.primitive_table(operand);
  relation_feature_key out;
  out.operand = operand;
  out.kind = relation_feature_kind::source_triangle;
  if (candidate.triangle.ordinal() < table.triangles.size()) {
    const auto &triangle = table.triangles[candidate.triangle.ordinal()];
    out.primary = triangle.source_triangle;
    out.secondary = triangle.source_facet;
  }
  return out;
}

} // namespace

template <class T, class I>
bool verify_relation_event_candidate_evidence(
    const signed_feature_relations<T, I> &artifact,
    bounded_boolean_error &error) {
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = candidate_evidence_error(subcode, summary);
    return false;
  };
  const auto &graph = artifact.request_graph();
  const auto &event_seeds = artifact.event_seeds();
  const auto &relations = artifact.relations();
  const auto &constructions = artifact.constructions();
  const auto &source_incidence = artifact.event_seed_incidence();
  const auto &candidate_incidence = artifact.event_seed_candidate_incidence();
  const auto &truth_records = artifact.truth_records();
  const auto &construction_ledger = artifact.construction_ledger();
  const auto &symbolic_decisions = artifact.symbolic_decisions();
  const auto &symbolic_eligibility = artifact.symbolic_eligibility();
  const auto &crossings = artifact.crossings();
  const auto &dispositions = artifact.candidate_dispositions();
  const auto &relation_coverage = artifact.candidate_relation_coverage();
  const auto &seed_coverage = artifact.candidate_event_seed_coverage();
  const auto &partitions = artifact.candidate_partitions();
  const auto &candidates = artifact.candidates();
  if (!candidates)
    return fail(relation_subcode::candidate_disposition_missing,
                "Component 07 candidate predecessor is absent");
  std::vector<bool> event_seed_request_seen(
      graph.requests.size(), false);
  std::size_t expected_event_seed_count = 0;
  for (const auto &request : graph.requests)
    if (request.key.family == relation_request_family::event_seed)
      ++expected_event_seed_count;
  if (event_seeds.size() != expected_event_seed_count)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 event-seed table is incomplete");

  if (!std::is_sorted(
          event_seeds.begin(), event_seeds.end(),
          [](const relation_event_seed_record &a,
             const relation_event_seed_record &b) { return a.key < b.key; }))
    return fail(relation_subcode::canonical_order_mismatch,
                "Component 07 event seeds are not ordered");
  std::size_t published_candidate_incidence = 0;
  for (std::size_t i = 0; i < event_seeds.size(); ++i) {
    const auto &record = event_seeds[i];
    if (record.id.ordinal() != i || !valid_relation_event_seed_key(record.key) ||
        record.source_relation.ordinal() >= relations.size() ||
        record.construction.ordinal() >= constructions.size() ||
        record.incidence_begin > source_incidence.size() ||
        record.incidence_count < 2 ||
        record.incidence_count >
            source_incidence.size() - record.incidence_begin ||
        record.candidate_incidence_begin != published_candidate_incidence ||
        record.candidate_incidence_begin >
            candidate_incidence.size() ||
        record.candidate_incidence_count == 0 ||
        record.candidate_incidence_count >
            candidate_incidence.size() -
                record.candidate_incidence_begin ||
        record.schema_version != contract_versions::relation_event_seed_schema ||
        record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event seed is malformed");
    if (i != 0 && !(event_seeds[i - 1].key < record.key))
      return fail(relation_subcode::canonical_order_mismatch,
                  "Component 07 event seeds are not strictly ordered");

    const auto &relation = relations[record.source_relation.ordinal()];
    const auto &construction =
        constructions[record.construction.ordinal()];
    if (record.key.family != relation.family ||
        record.contact_status != relation.status ||
        record.contact_dimension !=
            contact_dimension(relation.family, relation.status) ||
        record.contact_dimension == relation_contact_dimension::none ||
        record.construction_kind != construction.kind ||
        record.truth_begin != relation.truth_begin ||
        record.truth_count != relation.truth_count ||
        record.truth_begin > truth_records.size() ||
        record.truth_count == 0 ||
        record.truth_count >
            truth_records.size() - record.truth_begin ||
        record.construction_ledger_begin != construction.ledger_begin ||
        record.construction_ledger_count != construction.ledger_count ||
        record.construction_ledger_begin > construction_ledger.size() ||
        record.construction_ledger_count == 0 ||
        record.construction_ledger_count >
            construction_ledger.size() -
                record.construction_ledger_begin)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event-seed truth or construction lineage does not reconstruct");

    relation_feature_key expected_source_vertex{};
    expected_source_vertex.operand =
        graph.requests[relation.producer.ordinal()]
            .key.first.operand;
    const bool expected_source_vertex_reused =
        construction.authoritative_source_feature.kind ==
        relation_feature_kind::source_vertex;
    if (expected_source_vertex_reused)
      expected_source_vertex = construction.authoritative_source_feature;
    if (record.accepted_source_vertex_reused !=
            expected_source_vertex_reused ||
        record.accepted_source_vertex != expected_source_vertex ||
        !valid_relation_feature_key(record.accepted_source_vertex, true))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event-seed accepted source vertex does not reconstruct");

    bool precision_complete = construction.finite &&
                              construction.tolerance_compatible;
    bool has_matching_ledger_use = false;
    for (std::uint64_t offset = 0;
         offset < construction.ledger_count; ++offset) {
      const auto &ledger = construction_ledger[
          construction.ledger_begin + offset];
      precision_complete = precision_complete &&
                           ledger.precision_evidence_complete;
      has_matching_ledger_use =
          has_matching_ledger_use ||
          (ledger.source_relation == record.source_relation &&
           ledger.occurrence == record.key.occurrence);
    }
    if (!precision_complete || !record.precision_evidence_complete ||
        !has_matching_ledger_use)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event-seed precision ledger is incomplete");

    bool has_first = false;
    bool has_second = false;
    for (std::uint64_t offset = 0; offset < record.incidence_count; ++offset) {
      const auto index = record.incidence_begin + offset;
      const auto &feature = source_incidence[index];
      if (!valid_relation_feature_key(feature) ||
          (offset != 0 &&
           !(source_incidence[index - 1] < feature)))
        return fail(relation_subcode::canonical_order_mismatch,
                    "Component 07 event-seed source incidence is malformed");
      has_first = has_first || feature == record.key.first;
      has_second = has_second || feature == record.key.second;
    }
    if (!has_first || !has_second)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event seed omits authoritative source incidence");

    const auto &construction_request = graph.requests[
        construction.producer.ordinal()];
    const auto &source_request = graph.requests[
        relation.producer.ordinal()];
    const canonical_relation_request *matched_seed_request = nullptr;
    for (const auto &request : graph.requests)
      if (request.key.family == relation_request_family::event_seed &&
          request.key.first == record.key.first &&
          request.key.second == record.key.second &&
          request.key.directed_use == construction_request.key.directed_use &&
          request.key.occurrence_discriminator == record.key.occurrence &&
          request_has_dependency(graph, request,
                                 source_request.id) &&
          request_has_dependency(graph, request,
                                 construction_request.id)) {
        if (matched_seed_request)
          return fail(relation_subcode::duplicate_authoritative_producer,
                      "Component 07 event seed request is ambiguous");
        matched_seed_request = &request;
      }
    if (!matched_seed_request ||
        matched_seed_request->id.ordinal() >= event_seed_request_seen.size() ||
        event_seed_request_seen[matched_seed_request->id.ordinal()])
      return fail(relation_subcode::missing_dependency,
                  "Component 07 event seed request is absent or duplicated");
    event_seed_request_seen[matched_seed_request->id.ordinal()] = true;

    const symbolic_relation_decision_record *expected_symbolic = nullptr;
    relation_request_id expected_symbolic_request{0};
    for (std::uint64_t offset = 0;
         offset < matched_seed_request->dependency_count; ++offset) {
      const auto dependency = graph.dependencies[
          matched_seed_request->dependency_begin + offset].producer;
      const auto &dependency_request =
          graph.requests[dependency.ordinal()];
      if (dependency_request.key.family !=
          relation_request_family::symbolic_relation_decision)
        continue;
      if (expected_symbolic)
        return fail(relation_subcode::duplicate_authoritative_producer,
                    "Component 07 event seed has multiple symbolic decisions");
      expected_symbolic_request = dependency;
      for (std::size_t symbolic_index = 0;
           symbolic_index < symbolic_decisions.size();
           ++symbolic_index) {
        const auto &eligibility = symbolic_eligibility[symbolic_index];
        const auto *eligibility_request =
            find_request(graph, eligibility.request);
        if (eligibility_request &&
            request_has_dependency(graph,
                                   dependency_request,
                                   eligibility_request->id)) {
          if (expected_symbolic)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 event-seed symbolic decision is ambiguous");
          expected_symbolic = &symbolic_decisions[symbolic_index];
        }
      }
    }
    (void)expected_symbolic_request;
    std::int32_t expected_numeric_crossing = 0;
    std::int8_t expected_symbolic_crossing =
        expected_symbolic
            ? expected_symbolic->symbolic_crossing_contribution
            : 0;
    operand_id expected_half_open_owner =
        expected_symbolic ? expected_symbolic->half_open_owner
                          : source_request.key.first.operand;
    for (const auto &crossing : crossings)
      if (crossing.relation == record.source_relation &&
          crossing.occurrence == record.key.occurrence) {
        expected_numeric_crossing += crossing.numeric_crossing;
        expected_symbolic_crossing = crossing.symbolic_crossing;
        expected_half_open_owner = crossing.half_open_owner;
      }
    if (expected_numeric_crossing < -1 || expected_numeric_crossing > 1 ||
        record.numeric_crossing != expected_numeric_crossing ||
        record.symbolic_crossing != expected_symbolic_crossing ||
        record.half_open_owner != expected_half_open_owner ||
        record.has_symbolic_decision != (expected_symbolic != nullptr))
      return fail(relation_subcode::crossing_conservation_failed,
                  "Component 07 event-seed crossing evidence does not reconstruct");
    bool expected_distinct =
        (relation.family == feature_relation_family::source_edge_source_edge &&
         (relation.status ==
              feature_relation_status::coincidence_same_orientation ||
          relation.status ==
              feature_relation_status::coincidence_opposite_orientation));
    if (source_request.key.family ==
        relation_request_family::coplanar_source_facet_overlay) {
      const auto &overlay_stage = artifact.coplanar_overlay_stage();
      if (!overlay_stage)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 event seed omits its coplanar overlay predecessor");
      const source_facet_coplanar_overlay_record<T> *matched_overlay = nullptr;
      for (const auto &overlay : overlay_stage->overlays)
        if (overlay.facets[0].feature == source_request.key.first &&
            overlay.facets[1].feature == source_request.key.second) {
          if (matched_overlay)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 event seed has ambiguous coplanar occurrence lineage");
          matched_overlay = &overlay;
        }
      if (!matched_overlay)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 event seed coplanar occurrence lineage is absent");
      expected_distinct = matched_overlay->distinct_sheet_occurrences;
    }
    if (expected_symbolic) {
      expected_distinct = expected_distinct ||
                          expected_symbolic->occurrence_separation_required;
      if (record.symbolic_decision != expected_symbolic->id ||
          record.symbolic_rule_ordinal !=
              expected_symbolic->stable_rule_ordinal ||
          record.symbolic_exchange_rule_ordinal !=
              expected_symbolic->exchange_rule_ordinal ||
          record.symbolic_subject_kind != expected_symbolic->subject_kind ||
          record.symbolic_subject_ordinal !=
              expected_symbolic->subject_ordinal ||
          record.symbolic_occurrence_rank !=
              expected_symbolic->feature_priority ||
          record.conceptual_side != expected_symbolic->conceptual_side ||
          record.conceptual_order != expected_symbolic->conceptual_order ||
          record.symbolic_contact != expected_symbolic->contact_class ||
          record.symbolic_expected !=
              expected_symbolic->expected_disposition ||
          record.symbolic_explanation != expected_symbolic->explanation ||
          record.symbolic_tie_key_schema != expected_symbolic->tie_key_schema ||
          record.coincident_owner_rank !=
              expected_symbolic->coincident_owner_rank ||
          record.symbolic_owner_rank_eligible !=
              expected_symbolic->owner_rank_eligible)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 event-seed symbolic evidence does not reconstruct");
    } else if (record.symbolic_rule_ordinal != 0 ||
               record.symbolic_exchange_rule_ordinal != 0 ||
               record.symbolic_subject_kind !=
                   symbolic_relation_subject_kind::relation ||
               record.symbolic_subject_ordinal != 0 ||
               record.symbolic_occurrence_rank != 0 ||
               record.conceptual_side != symbolic_relation_side::coincident ||
               record.conceptual_order !=
                   symbolic_offset_disposition::coincident ||
               record.symbolic_tie_key_schema != 0 ||
               record.symbolic_owner_rank_eligible) {
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event seed invents symbolic evidence");
    }
    if (record.distinct_occurrence_required != expected_distinct)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event-seed occurrence separation does not reconstruct");

    if (record.candidate_incidence_count !=
        matched_seed_request->witness_count)
      return fail(relation_subcode::candidate_disposition_missing,
                  "Component 07 event seed omits candidate incidence");
    for (std::uint64_t offset = 0;
         offset < record.candidate_incidence_count; ++offset) {
      const auto index = record.candidate_incidence_begin + offset;
      const auto &incidence =
          candidate_incidence[index];
      const auto candidate_id = graph.candidate_witnesses[
          matched_seed_request->witness_begin + offset];
      if (!candidates ||
          candidate_id.ordinal() >= candidates->candidates().size())
        return fail(relation_subcode::candidate_disposition_missing,
                    "Component 07 event-seed candidate witness is absent");
      const auto &candidate =
          candidates->candidates()[candidate_id.ordinal()];
      const auto edge_operand =
          candidate.role == directed_candidate_role::a_edge_b_triangle
              ? operand_id::a
              : operand_id::b;
      const auto triangle_operand =
          edge_operand == operand_id::a ? operand_id::b : operand_id::a;
      const auto &edge_table =
          candidates->primitive_table(edge_operand);
      const auto &triangle_table =
          candidates->primitive_table(triangle_operand);
      if (candidate.edge.ordinal() >= edge_table.edges.size() ||
          candidate.triangle.ordinal() >= triangle_table.triangles.size())
        return fail(relation_subcode::candidate_disposition_missing,
                    "Component 07 event-seed candidate primitive is absent");
      const auto &edge = edge_table.edges[candidate.edge.ordinal()];
      const auto &triangle =
          triangle_table.triangles[candidate.triangle.ordinal()];
      const std::array<std::uint64_t, 2> expected_edge_halfedges{
          edge.halfedges[0].ordinal(), edge.halfedges[1].ordinal()};
      const std::array<std::uint64_t, 3> expected_triangle_halfedges{
          triangle.halfedges[0].ordinal(), triangle.halfedges[1].ordinal(),
          triangle.halfedges[2].ordinal()};
      if (incidence.id.ordinal() != index || incidence.seed != record.id ||
          incidence.candidate != candidate.id ||
          incidence.disposition.ordinal() != candidate.id.ordinal() ||
          incidence.candidate_edge != candidate_edge_feature(
                                           *candidates, candidate) ||
          incidence.source_triangle != candidate_triangle_feature(
                                           *candidates, candidate) ||
          incidence.edge_halfedges != expected_edge_halfedges ||
          incidence.triangle_halfedges != expected_triangle_halfedges ||
          incidence.internal_diagonal_witness !=
              (candidate.edge_class ==
               canonical_edge_class::facet_internal_diagonal) ||
          incidence.source_feature_owner != edge.source_feature_owner ||
          incidence.schema_version !=
              contract_versions::relation_event_seed_incidence_schema ||
          incidence.reserved != 0)
        return fail(relation_subcode::candidate_disposition_contradiction,
                    "Component 07 event-seed candidate incidence does not reconstruct");
    }
    published_candidate_incidence += record.candidate_incidence_count;
  }
  if (published_candidate_incidence !=
      candidate_incidence.size())
    return fail(relation_subcode::candidate_disposition_contradiction,
                "Component 07 event-seed candidate incidence has trailing records");
  for (const auto &request : graph.requests)
    if (request.key.family == relation_request_family::event_seed &&
        !event_seed_request_seen[request.id.ordinal()])
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event-seed request has no published record");

  if (!std::is_sorted(
          dispositions.begin(),
          dispositions.end(),
          [](const relation_candidate_disposition_record &a,
             const relation_candidate_disposition_record &b) {
            return a.candidate < b.candidate;
          }))
    return fail(relation_subcode::canonical_order_mismatch,
                "Component 07 candidate dispositions are not ordered");
  const auto expected_candidates = candidates->candidates().size();
  if (dispositions.size() != expected_candidates)
    return fail(relation_subcode::candidate_disposition_missing,
                "Component 07 does not contain exactly one disposition per candidate");
  std::size_t expected_relation_coverage_begin = 0;
  std::size_t expected_seed_coverage_begin = 0;
  for (std::size_t i = 0; i < dispositions.size(); ++i) {
    const auto &record = dispositions[i];
    const auto &candidate = candidates->candidates()[i];
    if (record.id.ordinal() != i || record.candidate.ordinal() != i ||
        !valid_candidate_disposition(record.disposition) ||
        record.bookkeeping_request.ordinal() >=
            graph.requests.size() ||
        graph.requests[record.bookkeeping_request.ordinal()]
                .key.family != relation_request_family::candidate_disposition ||
        !request_has_witness(
            graph,
            graph.requests[record.bookkeeping_request.ordinal()],
            record.candidate) ||
        record.relation_begin != expected_relation_coverage_begin ||
        record.event_seed_begin != expected_seed_coverage_begin ||
        record.relation_begin > relation_coverage.size() ||
        record.relation_count >
            relation_coverage.size() - record.relation_begin ||
        record.event_seed_begin >
            seed_coverage.size() ||
        record.event_seed_count >
            seed_coverage.size() -
                record.event_seed_begin ||
        !record.coverage_complete ||
        (record.coverage_flags & candidate_coverage_complete) == 0 ||
        record.schema_version !=
            contract_versions::relation_candidate_disposition_schema ||
        record.reserved != 0)
      return fail(relation_subcode::candidate_disposition_contradiction,
                  "Component 07 candidate disposition is malformed");

    std::vector<feature_relation_id> expected_relations;
    for (const auto &relation : relations) {
      if (relation.producer.ordinal() >= graph.requests.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 candidate relation producer is absent");
      if (request_has_witness(
              graph,
              graph.requests[relation.producer.ordinal()],
              candidate.id))
        expected_relations.push_back(relation.id);
    }
    std::sort(expected_relations.begin(), expected_relations.end());
    expected_relations.erase(
        std::unique(expected_relations.begin(), expected_relations.end()),
        expected_relations.end());
    std::vector<relation_event_seed_id> expected_seeds;
    for (const auto &incidence : candidate_incidence)
      if (incidence.candidate == candidate.id)
        expected_seeds.push_back(incidence.seed);
    std::sort(expected_seeds.begin(), expected_seeds.end());
    expected_seeds.erase(std::unique(expected_seeds.begin(), expected_seeds.end()),
                         expected_seeds.end());
    if (record.relation_count != expected_relations.size() ||
        record.event_seed_count != expected_seeds.size() ||
        !std::equal(expected_relations.begin(), expected_relations.end(),
                    relation_coverage.begin() +
                        static_cast<std::ptrdiff_t>(record.relation_begin)) ||
        !std::equal(expected_seeds.begin(), expected_seeds.end(),
                    seed_coverage.begin() +
                        static_cast<std::ptrdiff_t>(record.event_seed_begin)))
      return fail(relation_subcode::candidate_disposition_contradiction,
                  "Component 07 candidate coverage does not reconstruct");

    bool has_public_contact = false;
    bool has_coplanar_or_coincident = false;
    bool has_zero_measure = false;
    bool all_separated = !expected_relations.empty();
    bool has_earlier_witness = false;
    bool has_canonical_contribution = false;
    const feature_relation_record *selected = nullptr;
    for (const auto id : expected_relations) {
      const auto &relation = relations[id.ordinal()];
      all_separated = all_separated &&
                      relation.status ==
                          feature_relation_status::definitely_separated;
      if (public_contact(relation.status)) {
        has_public_contact = true;
        if (!selected || relation.producer < selected->producer)
          selected = &relation;
      }
      has_coplanar_or_coincident =
          has_coplanar_or_coincident ||
          relation.status == feature_relation_status::overlap ||
          relation.status == feature_relation_status::containment ||
          relation.status ==
              feature_relation_status::coincidence_same_orientation ||
          relation.status ==
              feature_relation_status::coincidence_opposite_orientation;
      has_zero_measure =
          has_zero_measure ||
          relation.status == feature_relation_status::endpoint_crossing ||
          relation.status == feature_relation_status::point_contact ||
          relation.status == feature_relation_status::segment_contact ||
          relation.status == feature_relation_status::tangency;
      const auto &request =
          graph.requests[relation.producer.ordinal()];
      if (request.witness_count != 0) {
        const auto first =
            graph.candidate_witnesses[request.witness_begin];
        has_earlier_witness =
            has_earlier_witness || first.ordinal() < candidate.id.ordinal();
        has_canonical_contribution =
            has_canonical_contribution || first == candidate.id;
      }
    }
    for (const auto seed : expected_seeds) {
      const auto &seed_record = event_seeds[seed.ordinal()];
      const auto &first = candidate_incidence[
          seed_record.candidate_incidence_begin];
      has_earlier_witness =
          has_earlier_witness || first.candidate.ordinal() < candidate.id.ordinal();
      has_canonical_contribution =
          has_canonical_contribution || first.candidate == candidate.id;
    }
    std::uint16_t expected_flags = candidate_coverage_complete;
    if (!expected_relations.empty())
      expected_flags |= candidate_coverage_relation;
    if (has_public_contact)
      expected_flags |= candidate_coverage_public_contact;
    if (!expected_seeds.empty())
      expected_flags |= candidate_coverage_event_seed;
    if (has_coplanar_or_coincident)
      expected_flags |= candidate_coverage_coplanar_or_coincident;
    if (has_zero_measure)
      expected_flags |= candidate_coverage_zero_measure;
    if (all_separated)
      expected_flags |= candidate_coverage_definitely_separated;
    if (has_earlier_witness && !has_canonical_contribution)
      expected_flags |= candidate_coverage_duplicate_discovery;
    if (candidate.edge_class == canonical_edge_class::facet_internal_diagonal)
      expected_flags |= candidate_coverage_internal_diagonal;

    candidate_relation_disposition_kind expected_disposition =
        candidate_relation_disposition_kind::primitive_dependency_only;
    if (candidate.edge_class == canonical_edge_class::facet_internal_diagonal)
      expected_disposition = candidate_relation_disposition_kind::
          internal_diagonal_bookkeeping_absorbed;
    else if (has_earlier_witness && !has_canonical_contribution)
      expected_disposition =
          candidate_relation_disposition_kind::duplicate_discovery_absorbed;
    else if (has_coplanar_or_coincident)
      expected_disposition = candidate_relation_disposition_kind::
          contributed_coplanar_or_coincident_relation;
    else if (!expected_seeds.empty())
      expected_disposition =
          candidate_relation_disposition_kind::contributed_event_seeds;
    else if (has_zero_measure)
      expected_disposition = candidate_relation_disposition_kind::
          retained_zero_measure_contact;
    else if (all_separated)
      expected_disposition =
          candidate_relation_disposition_kind::definitely_separated;
    if (record.coverage_flags != expected_flags ||
        record.disposition != expected_disposition ||
        (selected && record.public_relation != selected->id) ||
        (!selected && record.public_relation.ordinal() != 0))
      return fail(relation_subcode::candidate_disposition_contradiction,
                  "Component 07 candidate disposition does not reconstruct");
    expected_relation_coverage_begin += expected_relations.size();
    expected_seed_coverage_begin += expected_seeds.size();
  }
  if (expected_relation_coverage_begin !=
          relation_coverage.size() ||
      expected_seed_coverage_begin !=
          seed_coverage.size())
    return fail(relation_subcode::candidate_disposition_contradiction,
                "Component 07 candidate coverage has trailing records");

  if (partitions.size() !=
      candidates->partitions().size())
    return fail(relation_subcode::candidate_disposition_missing,
                "Component 07 candidate partition table is incomplete");
  for (std::size_t i = 0; i < partitions.size(); ++i) {
    const auto &record = partitions[i];
    const auto &source = candidates->partitions()[i];
    std::uint64_t relation_begin = 0, relation_count = 0;
    std::uint64_t event_seed_begin = 0, event_seed_count = 0;
    if (source.count != 0) {
      const auto &first = dispositions[source.begin];
      const auto &last =
          dispositions[source.begin + source.count - 1];
      relation_begin = first.relation_begin;
      relation_count = last.relation_begin + last.relation_count - relation_begin;
      event_seed_begin = first.event_seed_begin;
      event_seed_count =
          last.event_seed_begin + last.event_seed_count - event_seed_begin;
    }
    if (record.id.ordinal() != i || record.source_partition != source.id ||
        record.candidate_begin != source.begin ||
        record.candidate_count != source.count ||
        record.disposition_begin != source.begin ||
        record.disposition_count != source.count ||
        record.relation_begin != relation_begin ||
        record.relation_count != relation_count ||
        record.event_seed_begin != event_seed_begin ||
        record.event_seed_count != event_seed_count ||
        record.maximum_records != source.maximum_records ||
        record.schema_version !=
            contract_versions::relation_candidate_partition_schema ||
        record.reserved != 0)
      return fail(relation_subcode::candidate_disposition_contradiction,
                  "Component 07 candidate partition does not reconstruct");
  }

  return true;
}

template bool verify_relation_event_candidate_evidence<float, std::uint32_t>(
    const signed_feature_relations<float, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_relation_event_candidate_evidence<float, std::uint64_t>(
    const signed_feature_relations<float, std::uint64_t> &,
    bounded_boolean_error &);
template bool verify_relation_event_candidate_evidence<double, std::uint32_t>(
    const signed_feature_relations<double, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_relation_event_candidate_evidence<double, std::uint64_t>(
    const signed_feature_relations<double, std::uint64_t> &,
    bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
