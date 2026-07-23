#include "StrictFloatingBuild.h"
#include "RelationEventSeeds.h"

#include <algorithm>
#include <new>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error seed_error(
    relation_subcode subcode, const char *summary,
    bounded_boolean_error_category category =
        bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(
      subcode, category, summary,
      relation_checkpoint::event_seed_and_disposition_reconciliation);
}

bool proposal_less(const relation_event_seed_proposal &a,
                   const relation_event_seed_proposal &b) noexcept {
  return a.key < b.key;
}

void canonicalize_incidence(std::vector<relation_feature_key> &incidence) {
  std::sort(incidence.begin(), incidence.end());
  incidence.erase(std::unique(incidence.begin(), incidence.end()),
                  incidence.end());
}

} // namespace

std::vector<std::uint8_t>
encode_relation_event_seed_table(const relation_event_seed_table &table) {
  canonical_writer writer;
  writer.u32(0x37534559U); // YES7
  writer.u16(contract_versions::relation_event_seed_schema);
  writer.u64(table.records.size());
  writer.u64(table.incidence.size());
  for (const auto &record : table.records) {
    writer.u64(record.id.ordinal());
    encode_relation_event_seed_key(writer, record.key);
    writer.u64(record.source_relation.ordinal());
    writer.u64(record.construction.ordinal());
    writer.u64(record.incidence_begin);
    writer.u64(record.incidence_count);
    writer.boolean(record.distinct_occurrence_required);
    writer.u32(record.reserved);
  }
  for (const auto &feature : table.incidence)
    encode_relation_feature_key(writer, feature);
  return writer.take();
}

boolean_outcome<relation_event_seed_table> canonicalize_relation_event_seeds(
    std::vector<relation_event_seed_proposal> proposals,
    const relation_capabilities &capabilities) {
  try {
    if (!capabilities.owner.anchor)
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::wrong_owner,
          "event-seed canonicalization owner is not bound"));
    if (relation_cancelled(capabilities))
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::cancelled,
          "event-seed canonicalization cancelled",
          bounded_boolean_error_category::cancelled));
    if (proposals.size() > capabilities.maximum_event_seeds)
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::work_limit, "event-seed limit exceeded",
          bounded_boolean_error_category::resource_limit));
    for (auto &proposal : proposals) {
      if (!valid_relation_event_seed_key(proposal.key))
        return boolean_outcome<relation_event_seed_table>::failure(seed_error(
            relation_subcode::malformed_request_key,
            "event seed has malformed lineage key"));
      for (const auto &feature : proposal.incidence)
        if (!valid_relation_feature_key(feature))
          return boolean_outcome<relation_event_seed_table>::failure(seed_error(
              relation_subcode::malformed_request_key,
              "event seed has malformed incidence feature"));
      canonicalize_incidence(proposal.incidence);
    }
    std::sort(proposals.begin(), proposals.end(), proposal_less);

    relation_event_seed_table table;
    for (std::size_t begin = 0; begin < proposals.size();) {
      std::size_t end = begin + 1;
      while (end < proposals.size() &&
             proposals[end].key == proposals[begin].key)
        ++end;
      const auto source = proposals[begin].source_relation;
      const auto construction = proposals[begin].construction;
      const auto distinct = proposals[begin].distinct_occurrence_required;
      std::vector<relation_feature_key> incidence;
      for (std::size_t i = begin; i < end; ++i) {
        if (proposals[i].source_relation != source ||
            proposals[i].construction != construction ||
            proposals[i].distinct_occurrence_required != distinct)
          return boolean_outcome<relation_event_seed_table>::failure(seed_error(
              relation_subcode::candidate_disposition_contradiction,
              "duplicate event-seed producers disagree"));
        incidence.insert(incidence.end(), proposals[i].incidence.begin(),
                         proposals[i].incidence.end());
      }
      canonicalize_incidence(incidence);
      relation_event_seed_record record;
      record.id = relation_event_seed_id(table.records.size());
      record.key = proposals[begin].key;
      record.source_relation = source;
      record.construction = construction;
      record.incidence_begin = table.incidence.size();
      record.incidence_count = incidence.size();
      record.distinct_occurrence_required = distinct;
      table.records.push_back(record);
      table.incidence.insert(table.incidence.end(), incidence.begin(),
                             incidence.end());
      begin = end;
    }
    if (table.incidence.size() > capabilities.maximum_consumers)
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::work_limit, "event-seed incidence limit exceeded",
          bounded_boolean_error_category::resource_limit));
    table.semantic_digest =
        sha256::digest(encode_relation_event_seed_table(table));
    return boolean_outcome<relation_event_seed_table>::success(std::move(table));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<relation_event_seed_table>::failure(seed_error(
        relation_subcode::resource_preflight,
        "event-seed canonicalization allocation failed",
        bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<relation_event_seed_table>::failure(seed_error(
        relation_subcode::internal_invariant,
        "event-seed canonicalization raised an unexpected exception"));
  }
}

} // namespace ygor::mesh_boolean::bounded
