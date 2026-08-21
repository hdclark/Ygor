#include "EventIncidence.h"

#include <algorithm>
#include <limits>
#include <tuple>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error incidence_error(intersection_subcode subcode,
                                      const char *summary,
                                      intersection_checkpoint checkpoint) {
  return intersection_error(subcode,
                            bounded_boolean_error_category::internal_invariant_error,
                            summary, checkpoint);
}

bool checked_range(std::uint64_t begin, std::uint64_t count,
                   std::uint64_t size) noexcept {
  return begin <= size && count <= size - begin;
}

bool is_public_owner_kind(relation_feature_kind kind) noexcept {
  return kind == relation_feature_kind::source_vertex ||
         kind == relation_feature_kind::source_edge ||
         kind == relation_feature_kind::source_facet;
}

event_incidence_kind incidence_kind_for(relation_feature_kind kind) noexcept {
  switch (kind) {
  case relation_feature_kind::source_vertex:
    return event_incidence_kind::source_vertex;
  case relation_feature_kind::source_edge:
  case relation_feature_kind::facet_internal_diagonal:
    return event_incidence_kind::source_edge;
  case relation_feature_kind::source_facet:
    return event_incidence_kind::source_facet;
  case relation_feature_kind::source_triangle:
    return event_incidence_kind::source_triangle;
  case relation_feature_kind::sheet_occurrence:
    return event_incidence_kind::source_shell;
  case relation_feature_kind::none:
    break;
  }
  return event_incidence_kind::descriptor_precursor;
}

struct incidence_proposal final {
  event_incidence_key key{};
  event_id event{0};
  event_occurrence_id occurrence{0};
  event_seed_binding_id seed_binding{0};
  feature_relation_id relation{intersection_invalid_ordinal};
  candidate_id candidate{intersection_invalid_ordinal};
};

bool same_record_semantics(const incidence_proposal &a,
                           const incidence_proposal &b) noexcept {
  return a.key == b.key && a.event == b.event &&
         a.occurrence == b.occurrence &&
         a.seed_binding == b.seed_binding && a.relation == b.relation &&
         a.candidate == b.candidate;
}

void append_proposal(std::vector<incidence_proposal> &out,
                     const event_seed_binding_record &binding,
                     event_incidence_kind kind,
                     relation_feature_key feature,
                     std::uint64_t candidate_ordinal,
                     std::uint64_t proof_primary,
                     std::uint64_t proof_secondary,
                     std::uint32_t proof_occurrence,
                     std::int32_t numeric_crossing,
                     std::int8_t symbolic_crossing,
                     std::int8_t orientation,
                     bool owner, bool bookkeeping) {
  incidence_proposal proposal;
  proposal.event = binding.event;
  proposal.occurrence = binding.occurrence;
  proposal.seed_binding = binding.id;
  proposal.relation = binding.relation;
  if (candidate_ordinal != intersection_invalid_ordinal)
    proposal.candidate = candidate_id{candidate_ordinal};
  proposal.key.kind = kind;
  proposal.key.feature = feature;
  proposal.key.seed = binding.seed_key;
  proposal.key.predecessor_relation = binding.relation.ordinal();
  proposal.key.predecessor_candidate = candidate_ordinal;
  proposal.key.proof_primary = proof_primary;
  proposal.key.proof_secondary = proof_secondary;
  proposal.key.proof_occurrence = proof_occurrence;
  proposal.key.numeric_crossing = numeric_crossing;
  proposal.key.symbolic_crossing = symbolic_crossing;
  proposal.key.orientation = orientation;
  proposal.key.source_feature_owner = owner;
  proposal.key.bookkeeping_only = bookkeeping;
  out.push_back(std::move(proposal));
}

void bind_complete_keys(std::vector<incidence_proposal> &proposals,
                        const event_interning_tables &interning) {
  for (auto &proposal : proposals) {
    proposal.key.event = interning.events[proposal.event.ordinal()].key;
    proposal.key.occurrence =
        interning.occurrences[proposal.occurrence.ordinal()].key;
  }
}

template <class Target>
void build_dense_index(const std::vector<event_incidence_record> &records,
                       std::uint64_t count, Target target,
                       std::vector<event_incidence_id> &index,
                       std::vector<intersection_range> &ranges) {
  index.clear();
  index.reserve(records.size());
  for (const auto &record : records)
    index.push_back(record.id);
  std::sort(index.begin(), index.end(), [&](event_incidence_id a,
                                            event_incidence_id b) {
    const auto &ra = records[a.ordinal()];
    const auto &rb = records[b.ordinal()];
    return std::tuple{target(ra), ra.key} <
           std::tuple{target(rb), rb.key};
  });
  ranges.assign(static_cast<std::size_t>(count), intersection_range{});
  std::size_t begin = 0;
  while (begin < index.size()) {
    const auto ordinal = target(records[index[begin].ordinal()]);
    std::size_t end = begin + 1;
    while (end < index.size() &&
           target(records[index[end].ordinal()]) == ordinal)
      ++end;
    if (ordinal < count)
      ranges[static_cast<std::size_t>(ordinal)] =
          intersection_range{begin, end - begin};
    begin = end;
  }
}

bool source_feature_record(const event_incidence_record &record) noexcept {
  return record.kind == event_incidence_kind::source_vertex ||
         record.kind == event_incidence_kind::source_edge ||
         record.kind == event_incidence_kind::source_edge_interval ||
         record.kind == event_incidence_kind::source_facet ||
         record.kind == event_incidence_kind::source_triangle ||
         record.kind == event_incidence_kind::source_shell;
}

void build_feature_index(const std::vector<event_incidence_record> &records,
                         event_incidence_tables &tables) {
  tables.by_source_feature.clear();
  for (const auto &record : records)
    if (source_feature_record(record))
      tables.by_source_feature.push_back(record.id);
  std::sort(tables.by_source_feature.begin(),
            tables.by_source_feature.end(), [&](event_incidence_id a,
                                                  event_incidence_id b) {
    const auto &ra = records[a.ordinal()];
    const auto &rb = records[b.ordinal()];
    return std::tie(ra.kind, ra.feature, ra.key) <
           std::tie(rb.kind, rb.feature, rb.key);
  });
  tables.source_feature_ranges.clear();
  std::size_t begin = 0;
  while (begin < tables.by_source_feature.size()) {
    const auto &first = records[tables.by_source_feature[begin].ordinal()];
    std::size_t end = begin + 1;
    while (end < tables.by_source_feature.size()) {
      const auto &next = records[tables.by_source_feature[end].ordinal()];
      if (next.kind != first.kind || next.feature != first.feature)
        break;
      ++end;
    }
    source_feature_incidence_range_record range;
    range.kind = first.kind;
    range.feature = first.feature;
    range.incidence = intersection_range{begin, end - begin};
    tables.source_feature_ranges.push_back(range);
    begin = end;
  }
}

void build_halfedge_index(const std::vector<event_incidence_record> &records,
                          event_incidence_tables &tables) {
  tables.by_halfedge.clear();
  for (const auto &record : records)
    if (record.kind == event_incidence_kind::oriented_halfedge)
      tables.by_halfedge.push_back(record.id);
  std::sort(tables.by_halfedge.begin(), tables.by_halfedge.end(),
            [&](event_incidence_id a, event_incidence_id b) {
    const auto &ra = records[a.ordinal()];
    const auto &rb = records[b.ordinal()];
    return std::tie(ra.feature.operand, ra.payload_primary, ra.key) <
           std::tie(rb.feature.operand, rb.payload_primary, rb.key);
  });
  tables.halfedge_ranges.clear();
  std::size_t begin = 0;
  while (begin < tables.by_halfedge.size()) {
    const auto &first = records[tables.by_halfedge[begin].ordinal()];
    std::size_t end = begin + 1;
    while (end < tables.by_halfedge.size()) {
      const auto &next = records[tables.by_halfedge[end].ordinal()];
      if (next.feature.operand != first.feature.operand ||
          next.payload_primary != first.payload_primary)
        break;
      ++end;
    }
    oriented_halfedge_incidence_range_record range;
    range.operand = first.feature.operand;
    range.halfedge = first.payload_primary;
    range.incidence = intersection_range{begin, end - begin};
    tables.halfedge_ranges.push_back(range);
    begin = end;
  }
}

void build_all_indexes(std::uint64_t relation_count,
                       std::uint64_t candidate_count,
                       event_interning_tables &interning,
                       event_incidence_tables &tables) {
  build_dense_index(tables.records, interning.events.size(),
                    [](const event_incidence_record &r) {
                      return r.event.ordinal();
                    }, tables.by_event, tables.event_ranges);
  build_dense_index(tables.records, interning.occurrences.size(),
                    [](const event_incidence_record &r) {
                      return r.occurrence.ordinal();
                    }, tables.by_occurrence, tables.occurrence_ranges);
  build_dense_index(tables.records, interning.seed_bindings.size(),
                    [](const event_incidence_record &r) {
                      return r.seed_binding.ordinal();
                    }, tables.by_seed, tables.seed_ranges);
  build_dense_index(tables.records, relation_count,
                    [](const event_incidence_record &r) {
                      return r.relation.ordinal();
                    }, tables.by_relation, tables.relation_ranges);

  tables.by_candidate.clear();
  for (const auto &record : tables.records)
    if (record.candidate.ordinal() != intersection_invalid_ordinal)
      tables.by_candidate.push_back(record.id);
  std::sort(tables.by_candidate.begin(), tables.by_candidate.end(),
            [&](event_incidence_id a, event_incidence_id b) {
    const auto &ra = tables.records[a.ordinal()];
    const auto &rb = tables.records[b.ordinal()];
    return std::tie(ra.candidate, ra.key) < std::tie(rb.candidate, rb.key);
  });
  tables.candidate_ranges.assign(static_cast<std::size_t>(candidate_count),
                                 intersection_range{});
  std::size_t begin = 0;
  while (begin < tables.by_candidate.size()) {
    const auto ordinal =
        tables.records[tables.by_candidate[begin].ordinal()].candidate.ordinal();
    std::size_t end = begin + 1;
    while (end < tables.by_candidate.size() &&
           tables.records[tables.by_candidate[end].ordinal()].candidate.ordinal() ==
               ordinal)
      ++end;
    if (ordinal < candidate_count)
      tables.candidate_ranges[static_cast<std::size_t>(ordinal)] =
          intersection_range{begin, end - begin};
    begin = end;
  }

  tables.seed_candidate_index.clear();
  for (const auto &record : tables.records)
    if (record.candidate.ordinal() != intersection_invalid_ordinal)
      tables.seed_candidate_index.push_back(record.id);
  std::sort(tables.seed_candidate_index.begin(),
            tables.seed_candidate_index.end(), [&](event_incidence_id a,
                                                     event_incidence_id b) {
    const auto &ra = tables.records[a.ordinal()];
    const auto &rb = tables.records[b.ordinal()];
    return std::tie(ra.seed_binding, ra.key) <
           std::tie(rb.seed_binding, rb.key);
  });
  tables.seed_candidate_ranges.assign(interning.seed_bindings.size(),
                                      intersection_range{});
  begin = 0;
  while (begin < tables.seed_candidate_index.size()) {
    const auto ordinal = tables.records[
        tables.seed_candidate_index[begin].ordinal()].seed_binding.ordinal();
    std::size_t end = begin + 1;
    while (end < tables.seed_candidate_index.size() &&
           tables.records[tables.seed_candidate_index[end].ordinal()]
                   .seed_binding.ordinal() == ordinal)
      ++end;
    tables.seed_candidate_ranges[ordinal] =
        intersection_range{begin, end - begin};
    begin = end;
  }

  build_feature_index(tables.records, tables);
  build_halfedge_index(tables.records, tables);

  for (std::size_t i = 0; i < interning.events.size(); ++i)
    interning.events[i].incidence = tables.event_ranges[i];
  for (std::size_t i = 0; i < interning.occurrences.size(); ++i)
    interning.occurrences[i].incidence = tables.occurrence_ranges[i];
  for (std::size_t i = 0; i < interning.seed_bindings.size(); ++i) {
    interning.seed_bindings[i].incidence = tables.seed_ranges[i];
    interning.seed_bindings[i].candidate_incidence =
        tables.seed_candidate_ranges[i];
  }
}

bool equal_ranges(const std::vector<intersection_range> &a,
                  const std::vector<intersection_range> &b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i].begin != b[i].begin || a[i].count != b[i].count)
      return false;
  return true;
}

bool equal_feature_ranges(
    const std::vector<source_feature_incidence_range_record> &a,
    const std::vector<source_feature_incidence_range_record> &b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i].kind != b[i].kind || a[i].feature != b[i].feature ||
        a[i].incidence.begin != b[i].incidence.begin ||
        a[i].incidence.count != b[i].incidence.count)
      return false;
  return true;
}

bool equal_halfedge_ranges(
    const std::vector<oriented_halfedge_incidence_range_record> &a,
    const std::vector<oriented_halfedge_incidence_range_record> &b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i].operand != b[i].operand || a[i].halfedge != b[i].halfedge ||
        a[i].incidence.begin != b[i].incidence.begin ||
        a[i].incidence.count != b[i].incidence.count)
      return false;
  return true;
}

} // namespace

bool build_event_incidence_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_feature_key> &seed_incidence,
    const std::vector<relation_event_seed_candidate_incidence_record> &
        candidate_incidence,
    std::uint64_t relation_count, std::uint64_t candidate_count,
    event_interning_tables &interning, event_incidence_tables &tables,
    bounded_boolean_error &error) {
  tables = event_incidence_tables{};
  if (seeds.size() != interning.seed_bindings.size()) {
    error = incidence_error(intersection_subcode::seed_mapping_incomplete,
                            "Component 08 incidence seed binding mismatch",
                            intersection_checkpoint::incidence_proposals);
    return false;
  }
  std::vector<incidence_proposal> proposals;
  for (std::size_t i = 0; i < seeds.size(); ++i) {
    const auto &seed = seeds[i];
    const auto &binding = interning.seed_bindings[i];
    if (seed.id.ordinal() != i || binding.seed != seed.id ||
        !(binding.seed_key == seed.key) || binding.relation != seed.source_relation ||
        seed.source_relation.ordinal() >= relation_count ||
        !checked_range(seed.incidence_begin, seed.incidence_count,
                       seed_incidence.size()) ||
        !checked_range(seed.candidate_incidence_begin,
                       seed.candidate_incidence_count,
                       candidate_incidence.size())) {
      error = incidence_error(intersection_subcode::incidence_incomplete,
                              "Component 08 seed incidence range is malformed",
                              intersection_checkpoint::incidence_proposals);
      return false;
    }
    for (std::uint64_t j = 0; j < seed.incidence_count; ++j) {
      const auto &feature = seed_incidence[seed.incidence_begin + j];
      if (!valid_relation_feature_key(feature)) {
        error = incidence_error(intersection_subcode::incidence_incomplete,
                                "Component 08 source incidence is malformed",
                                intersection_checkpoint::incidence_proposals);
        return false;
      }
      const auto kind = incidence_kind_for(feature.kind);
      if (kind == event_incidence_kind::descriptor_precursor) {
        error = incidence_error(intersection_subcode::incidence_incomplete,
                                "Component 08 source incidence kind is unsupported",
                                intersection_checkpoint::incidence_proposals);
        return false;
      }
      const bool bookkeeping =
          feature.kind == relation_feature_kind::source_triangle ||
          feature.kind == relation_feature_kind::facet_internal_diagonal;
      const bool owner = !bookkeeping && is_public_owner_kind(feature.kind) &&
          (feature == seed.key.first || feature == seed.key.second);
      append_proposal(proposals, binding, kind, feature,
                      intersection_invalid_ordinal, j, 0, seed.key.occurrence,
                      0, 0, 0, owner, bookkeeping);
    }

    append_proposal(proposals, binding, event_incidence_kind::relation,
                    relation_feature_key{}, intersection_invalid_ordinal,
                    seed.source_relation.ordinal(), 0, seed.key.occurrence,
                    0, 0, 0, false, false);

    for (std::uint64_t j = 0; j < seed.candidate_incidence_count; ++j) {
      const auto &candidate =
          candidate_incidence[seed.candidate_incidence_begin + j];
      if (candidate.seed != seed.id ||
          candidate.id.ordinal() != seed.candidate_incidence_begin + j ||
          candidate.candidate.ordinal() >= candidate_count ||
          candidate.disposition.ordinal() != candidate.candidate.ordinal() ||
          !valid_relation_feature_key(candidate.candidate_edge) ||
          !valid_relation_feature_key(candidate.source_triangle) ||
          candidate.source_triangle.kind !=
              relation_feature_kind::source_triangle ||
          (candidate.internal_diagonal_witness &&
           candidate.candidate_edge.kind !=
               relation_feature_kind::facet_internal_diagonal) ||
          (candidate.candidate_edge.kind ==
               relation_feature_kind::facet_internal_diagonal &&
           candidate.source_feature_owner)) {
        error = incidence_error(
            candidate.source_feature_owner
                ? intersection_subcode::internal_diagonal_public_ownership
                : intersection_subcode::incidence_incomplete,
            "Component 08 candidate incidence is malformed",
            intersection_checkpoint::incidence_proposals);
        return false;
      }
      const auto candidate_ordinal = candidate.candidate.ordinal();
      append_proposal(proposals, binding, event_incidence_kind::candidate,
                      relation_feature_key{}, candidate_ordinal,
                      candidate.disposition.ordinal(), candidate.id.ordinal(),
                      seed.key.occurrence, 0, 0, 0, false, false);
      const bool edge_bookkeeping = candidate.internal_diagonal_witness ||
          candidate.candidate_edge.kind ==
              relation_feature_kind::facet_internal_diagonal;
      append_proposal(proposals, binding, event_incidence_kind::source_edge,
                      candidate.candidate_edge, candidate_ordinal,
                      candidate.id.ordinal(), 1, seed.key.occurrence,
                      0, 0, 0,
                      candidate.source_feature_owner && !edge_bookkeeping,
                      edge_bookkeeping);
      append_proposal(proposals, binding, event_incidence_kind::source_triangle,
                      candidate.source_triangle, candidate_ordinal,
                      candidate.id.ordinal(), 2, seed.key.occurrence,
                      0, 0, 0, false, true);
      for (std::size_t h = 0; h < candidate.edge_halfedges.size(); ++h)
        append_proposal(proposals, binding,
                        event_incidence_kind::oriented_halfedge,
                        candidate.candidate_edge, candidate_ordinal,
                        candidate.edge_halfedges[h], h,
                        seed.key.occurrence, 0, 0, 0, false, true);
      for (std::size_t h = 0; h < candidate.triangle_halfedges.size(); ++h)
        append_proposal(proposals, binding,
                        event_incidence_kind::oriented_halfedge,
                        candidate.source_triangle, candidate_ordinal,
                        candidate.triangle_halfedges[h], h + 2,
                        seed.key.occurrence, 0, 0, 0, false, true);
    }

    if (seed.numeric_crossing != 0 || seed.symbolic_crossing != 0)
      append_proposal(proposals, binding,
                      event_incidence_kind::crossing_contribution,
                      relation_feature_key{}, intersection_invalid_ordinal,
                      seed.source_relation.ordinal(), seed.symbolic_rule_ordinal,
                      seed.key.occurrence, seed.numeric_crossing,
                      seed.symbolic_crossing, 0, false, false);
    if (seed.has_symbolic_decision)
      append_proposal(proposals, binding,
                      event_incidence_kind::symbolic_decision,
                      relation_feature_key{}, intersection_invalid_ordinal,
                      seed.symbolic_decision.ordinal(),
                      seed.symbolic_rule_ordinal, seed.symbolic_occurrence_rank,
                      0, seed.symbolic_crossing, 0, false, false);
    append_proposal(proposals, binding,
                    event_incidence_kind::descriptor_precursor,
                    relation_feature_key{}, intersection_invalid_ordinal,
                    static_cast<std::uint64_t>(seed.contact_status),
                    static_cast<std::uint64_t>(seed.contact_dimension),
                    seed.key.occurrence, seed.numeric_crossing,
                    seed.symbolic_crossing, 0, false, false);
  }

  bind_complete_keys(proposals, interning);
  for (const auto &proposal : proposals)
    if (!valid_event_incidence_key(proposal.key)) {
      error = incidence_error(intersection_subcode::incidence_incomplete,
                              "Component 08 incidence proposal key is malformed",
                              intersection_checkpoint::incidence_proposals);
      return false;
    }
  std::sort(proposals.begin(), proposals.end(),
            [](const incidence_proposal &a, const incidence_proposal &b) {
              return a.key < b.key;
            });
  for (const auto &proposal : proposals) {
    if (!tables.records.empty() &&
        tables.records.back().key == proposal.key) {
      const auto &last = tables.records.back();
      incidence_proposal prior;
      prior.key = last.key;
      prior.event = last.event;
      prior.occurrence = last.occurrence;
      prior.seed_binding = last.seed_binding;
      prior.relation = last.relation;
      prior.candidate = last.candidate;
      if (!same_record_semantics(prior, proposal)) {
        error = incidence_error(intersection_subcode::incidence_incomplete,
                                "Component 08 duplicate incidence disagrees",
                                intersection_checkpoint::incidence_publication);
        return false;
      }
      continue;
    }
    event_incidence_record record;
    record.id = event_incidence_id{tables.records.size()};
    record.key = proposal.key;
    record.event = proposal.event;
    record.occurrence = proposal.occurrence;
    record.seed_binding = proposal.seed_binding;
    record.kind = proposal.key.kind;
    record.feature = proposal.key.feature;
    record.relation = proposal.relation;
    record.candidate = proposal.candidate;
    record.payload_primary = proposal.key.proof_primary;
    record.payload_secondary = proposal.key.proof_secondary;
    record.payload_occurrence = proposal.key.proof_occurrence;
    record.numeric_crossing = proposal.key.numeric_crossing;
    record.symbolic_crossing = proposal.key.symbolic_crossing;
    record.orientation = proposal.key.orientation;
    record.source_feature_owner = proposal.key.source_feature_owner;
    record.bookkeeping_only = proposal.key.bookkeeping_only;
    tables.records.push_back(std::move(record));
  }
  build_all_indexes(relation_count, candidate_count, interning, tables);
  return verify_event_incidence_records(
      seeds, seed_incidence, candidate_incidence, relation_count,
      candidate_count, interning, tables, error);
}

bool verify_event_incidence_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_feature_key> &seed_incidence,
    const std::vector<relation_event_seed_candidate_incidence_record> &
        candidate_incidence,
    std::uint64_t relation_count, std::uint64_t candidate_count,
    const event_interning_tables &interning,
    const event_incidence_tables &tables, bounded_boolean_error &error) {
  (void)seed_incidence;
  (void)candidate_incidence;
  if (seeds.size() != interning.seed_bindings.size()) {
    error = incidence_error(intersection_subcode::incidence_incomplete,
                            "Component 08 incidence verifier seed mismatch",
                            intersection_checkpoint::incidence_publication);
    return false;
  }
  for (std::size_t i = 0; i < tables.records.size(); ++i) {
    const auto &record = tables.records[i];
    if (record.id.ordinal() != i || !valid_event_incidence_key(record.key) ||
        record.event.ordinal() >= interning.events.size() ||
        record.occurrence.ordinal() >= interning.occurrences.size() ||
        record.seed_binding.ordinal() >= interning.seed_bindings.size() ||
        record.relation.ordinal() >= relation_count ||
        (record.candidate.ordinal() != intersection_invalid_ordinal &&
         record.candidate.ordinal() >= candidate_count) ||
        record.key.event != interning.events[record.event.ordinal()].key ||
        !(record.key.occurrence ==
          interning.occurrences[record.occurrence.ordinal()].key) ||
        !(record.key.seed ==
          interning.seed_bindings[record.seed_binding.ordinal()].seed_key)) {
      error = incidence_error(intersection_subcode::incidence_incomplete,
                              "Component 08 incidence record is malformed",
                              intersection_checkpoint::incidence_publication);
      return false;
    }
    if (i && !(tables.records[i - 1].key < record.key)) {
      error = incidence_error(intersection_subcode::incidence_incomplete,
                              "Component 08 incidence order is noncanonical",
                              intersection_checkpoint::incidence_publication);
      return false;
    }
  }

  event_interning_tables reconstructed_interning = interning;
  event_incidence_tables reconstructed;
  reconstructed.records = tables.records;
  build_all_indexes(relation_count, candidate_count, reconstructed_interning,
                    reconstructed);
  if (reconstructed.by_event != tables.by_event ||
      !equal_ranges(reconstructed.event_ranges, tables.event_ranges) ||
      reconstructed.by_occurrence != tables.by_occurrence ||
      !equal_ranges(reconstructed.occurrence_ranges,
                    tables.occurrence_ranges) ||
      reconstructed.by_seed != tables.by_seed ||
      !equal_ranges(reconstructed.seed_ranges, tables.seed_ranges) ||
      reconstructed.seed_candidate_index != tables.seed_candidate_index ||
      !equal_ranges(reconstructed.seed_candidate_ranges,
                    tables.seed_candidate_ranges) ||
      reconstructed.by_relation != tables.by_relation ||
      !equal_ranges(reconstructed.relation_ranges, tables.relation_ranges) ||
      reconstructed.by_candidate != tables.by_candidate ||
      !equal_ranges(reconstructed.candidate_ranges,
                    tables.candidate_ranges) ||
      reconstructed.by_source_feature != tables.by_source_feature ||
      !equal_feature_ranges(reconstructed.source_feature_ranges,
                            tables.source_feature_ranges) ||
      reconstructed.by_halfedge != tables.by_halfedge ||
      !equal_halfedge_ranges(reconstructed.halfedge_ranges,
                            tables.halfedge_ranges)) {
    error = incidence_error(intersection_subcode::incidence_incomplete,
                            "Component 08 reciprocal incidence index mismatch",
                            intersection_checkpoint::incidence_publication);
    return false;
  }
  for (std::size_t i = 0; i < interning.seed_bindings.size(); ++i) {
    if (tables.seed_ranges[i].count == 0 ||
        interning.seed_bindings[i].incidence.begin !=
            tables.seed_ranges[i].begin ||
        interning.seed_bindings[i].incidence.count !=
            tables.seed_ranges[i].count) {
      error = incidence_error(intersection_subcode::incidence_incomplete,
                              "Component 08 seed incidence is incomplete",
                              intersection_checkpoint::incidence_publication);
      return false;
    }
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
