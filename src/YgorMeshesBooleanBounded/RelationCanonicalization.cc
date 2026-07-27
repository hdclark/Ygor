#include "StrictFloatingBuild.h"
#include "RelationCanonicalization.h"

#include <algorithm>
#include <new>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error disposition_error(
    relation_subcode subcode, const char *summary,
    bounded_boolean_error_category category =
        bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(
      subcode, category, summary,
      relation_checkpoint::event_seed_and_disposition_reconciliation);
}

bool valid_disposition(candidate_relation_disposition_kind value) noexcept {
  const auto raw = static_cast<std::uint8_t>(value);
  return raw >= 1 && raw <= 7;
}

} // namespace

boolean_outcome<std::vector<relation_candidate_disposition_record>>
canonicalize_candidate_dispositions(
    std::vector<relation_candidate_disposition_proposal> proposals,
    std::uint64_t candidate_count,
    const relation_capabilities &capabilities) {
  try {
    if (!capabilities.owner.anchor)
      return boolean_outcome<
          std::vector<relation_candidate_disposition_record>>::failure(
          disposition_error(relation_subcode::wrong_owner,
                            "candidate-disposition owner is not bound"));
    if (relation_cancelled(capabilities))
      return boolean_outcome<
          std::vector<relation_candidate_disposition_record>>::failure(
          disposition_error(relation_subcode::cancelled,
                            "candidate-disposition canonicalization cancelled",
                            bounded_boolean_error_category::cancelled));
    if (candidate_count > capabilities.maximum_relations ||
        proposals.size() > capabilities.maximum_consumers)
      return boolean_outcome<
          std::vector<relation_candidate_disposition_record>>::failure(
          disposition_error(relation_subcode::work_limit,
                            "candidate-disposition limit exceeded",
                            bounded_boolean_error_category::resource_limit));
    std::sort(proposals.begin(), proposals.end(),
              [](const relation_candidate_disposition_proposal &a,
                 const relation_candidate_disposition_proposal &b) {
                return a.candidate < b.candidate;
              });
    if (proposals.size() != candidate_count)
      return boolean_outcome<
          std::vector<relation_candidate_disposition_record>>::failure(
          disposition_error(relation_subcode::candidate_disposition_missing,
                            "exactly one disposition per candidate is required"));

    std::vector<relation_candidate_disposition_record> records;
    records.reserve(proposals.size());
    for (std::uint64_t i = 0; i < candidate_count; ++i) {
      const auto &proposal = proposals[static_cast<std::size_t>(i)];
      if (proposal.candidate.ordinal() != i)
        return boolean_outcome<
            std::vector<relation_candidate_disposition_record>>::failure(
            disposition_error(
                proposal.candidate.ordinal() < i
                    ? relation_subcode::candidate_disposition_duplicate
                    : relation_subcode::candidate_disposition_missing,
                "candidate disposition domain is not complete and unique"));
      if (!valid_disposition(proposal.disposition))
        return boolean_outcome<
            std::vector<relation_candidate_disposition_record>>::failure(
            disposition_error(
                relation_subcode::candidate_disposition_contradiction,
                "candidate disposition enum is unsupported"));
      relation_candidate_disposition_record record;
      record.id = relation_candidate_disposition_id(i);
      record.candidate = proposal.candidate;
      record.disposition = proposal.disposition;
      if (!proposal.coverage_complete ||
          (proposal.coverage_flags & candidate_coverage_complete) == 0)
        return boolean_outcome<
            std::vector<relation_candidate_disposition_record>>::failure(
            disposition_error(
                relation_subcode::candidate_disposition_contradiction,
                "candidate disposition coverage is incomplete"));
      record.public_relation = proposal.public_relation;
      record.bookkeeping_request = proposal.bookkeeping_request;
      record.relation_begin = proposal.relation_begin;
      record.relation_count = proposal.relation_count;
      record.event_seed_begin = proposal.event_seed_begin;
      record.event_seed_count = proposal.event_seed_count;
      record.coverage_flags = proposal.coverage_flags;
      record.coverage_complete = proposal.coverage_complete;
      records.push_back(record);
    }
    return boolean_outcome<
        std::vector<relation_candidate_disposition_record>>::success(
        std::move(records));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<
        std::vector<relation_candidate_disposition_record>>::failure(
        disposition_error(relation_subcode::resource_preflight,
                          "candidate-disposition allocation failed",
                          bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<
        std::vector<relation_candidate_disposition_record>>::failure(
        disposition_error(relation_subcode::internal_invariant,
                          "candidate-disposition canonicalization failed"));
  }
}

} // namespace ygor::mesh_boolean::bounded
