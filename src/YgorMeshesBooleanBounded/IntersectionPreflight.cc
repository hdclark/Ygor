#include "IntersectionPreflight.h"

#include "CheckedArithmetic.h"

#include <algorithm>
#include <cstdint>

namespace ygor::mesh_boolean::bounded {
namespace {

bool range_valid(std::uint64_t begin, std::uint64_t count,
                 std::uint64_t size) noexcept {
  return begin <= size && count <= size - begin;
}

bool add(std::uint64_t a, std::uint64_t b, std::uint64_t &out,
         bounded_boolean_error &error, const char *summary) {
  if (checked_add(a, b, out))
    return true;
  error = intersection_error(intersection_subcode::count_overflow,
                             bounded_boolean_error_category::index_overflow,
                             summary, intersection_checkpoint::count_preflight);
  return false;
}

bool multiply(std::uint64_t a, std::uint64_t b, std::uint64_t &out,
              bounded_boolean_error &error, const char *summary) {
  if (checked_multiply(a, b, out))
    return true;
  error = intersection_error(intersection_subcode::count_overflow,
                             bounded_boolean_error_category::index_overflow,
                             summary, intersection_checkpoint::count_preflight);
  return false;
}

bool capacity(bool condition, bounded_boolean_error &error,
              const char *summary) {
  if (condition)
    return true;
  error = intersection_error(intersection_subcode::capacity_exceeded,
                             bounded_boolean_error_category::resource_limit,
                             summary, intersection_checkpoint::count_preflight);
  return false;
}

} // namespace

bool preflight_intersection_event_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_construction_record> &constructions,
    std::uint64_t construction_ledger_count,
    std::uint64_t seed_incidence_count,
    std::uint64_t candidate_incidence_count,
    const intersection_capabilities &capabilities,
    intersection_preflight_plan &plan,
    bounded_boolean_error &error) {
  plan = intersection_preflight_plan{};
  plan.construction_count = constructions.size();
  plan.construction_ledger_count = construction_ledger_count;
  plan.seed_incidence_count = seed_incidence_count;
  plan.candidate_incidence_count = candidate_incidence_count;

  if (intersection_cancelled(capabilities,
                             intersection_checkpoint::count_preflight)) {
    error = intersection_error(intersection_subcode::cancelled,
                               bounded_boolean_error_category::cancelled,
                               "Component 08 preflight cancelled",
                               intersection_checkpoint::count_preflight);
    return false;
  }

  for (std::size_t i = 0; i < constructions.size(); ++i) {
    const auto &construction = constructions[i];
    if (construction.id.ordinal() != i || construction.reserved != 0 ||
        construction.component_count == 0 ||
        construction.component_count > construction.nominal_bits.size() ||
        !construction.finite || !construction.precision_evidence_complete) {
      error = intersection_error(
          intersection_subcode::malformed_reference,
          bounded_boolean_error_category::input_contract_error,
          "Component 08 preflight rejected malformed construction table",
          intersection_checkpoint::predecessor_validation);
      return false;
    }
    if (!range_valid(construction.ledger_begin, construction.ledger_count,
                     construction_ledger_count)) {
      error = intersection_error(
          intersection_subcode::malformed_reference,
          bounded_boolean_error_category::input_contract_error,
          "Component 08 construction ledger range is invalid",
          intersection_checkpoint::predecessor_validation);
      return false;
    }
  }

  std::uint64_t declared_incidence = 0;
  std::uint64_t declared_candidate_incidence = 0;
  for (std::size_t i = 0; i < seeds.size(); ++i) {
    const auto &seed = seeds[i];
    if (seed.id.ordinal() != i || seed.construction.ordinal() >= constructions.size() ||
        !range_valid(seed.construction_ledger_begin,
                     seed.construction_ledger_count,
                     construction_ledger_count) ||
        !range_valid(seed.incidence_begin, seed.incidence_count,
                     seed_incidence_count) ||
        !range_valid(seed.candidate_incidence_begin,
                     seed.candidate_incidence_count,
                     candidate_incidence_count)) {
      error = intersection_error(
          intersection_subcode::malformed_reference,
          bounded_boolean_error_category::input_contract_error,
          "Component 08 seed range or reference is invalid",
          intersection_checkpoint::predecessor_validation);
      return false;
    }
    if (!add(declared_incidence, seed.incidence_count, declared_incidence,
             error, "Component 08 seed incidence sum overflowed") ||
        !add(declared_candidate_incidence, seed.candidate_incidence_count,
             declared_candidate_incidence, error,
             "Component 08 candidate-incidence sum overflowed"))
      return false;
  }

  const std::uint64_t seed_count = seeds.size();
  plan.estimate.event_count = seed_count;
  plan.estimate.occurrence_count = seed_count;
  plan.estimate.seed_binding_count = seed_count;
  std::uint64_t fixed_incidence = 0;
  if (!multiply(seed_count, std::uint64_t{2}, fixed_incidence, error,
                "Component 08 fixed incidence count overflowed") ||
      !add(declared_incidence, declared_candidate_incidence,
           plan.estimate.incidence_count, error,
           "Component 08 incidence count overflowed") ||
      !add(plan.estimate.incidence_count, fixed_incidence,
           plan.estimate.incidence_count, error,
           "Component 08 incidence count overflowed"))
    return false;

  std::uint64_t sort_levels = 0;
  for (std::uint64_t n = seed_count; n > 1; n = (n + 1) / 2)
    ++sort_levels;
  if (!multiply(seed_count, sort_levels, plan.sort_comparison_bound, error,
                "Component 08 sort comparison bound overflowed") ||
      !add(seed_count, plan.sort_comparison_bound,
           plan.estimate.work_units, error,
           "Component 08 work estimate overflowed"))
    return false;

  std::uint64_t event_bytes = 0;
  std::uint64_t occurrence_bytes = 0;
  std::uint64_t binding_bytes = 0;
  std::uint64_t incidence_bytes = 0;
  std::uint64_t map_bytes = 0;
  if (!multiply(seed_count, sizeof(intersection_event_record), event_bytes,
                error, "Component 08 event byte estimate overflowed") ||
      !multiply(seed_count, sizeof(intersection_occurrence_record),
                occurrence_bytes, error,
                "Component 08 occurrence byte estimate overflowed") ||
      !multiply(seed_count, sizeof(event_seed_binding_record), binding_bytes,
                error, "Component 08 binding byte estimate overflowed") ||
      !multiply(plan.estimate.incidence_count,
                sizeof(event_incidence_record), incidence_bytes, error,
                "Component 08 incidence byte estimate overflowed") ||
      !multiply(seed_count,
                sizeof(event_id) + sizeof(event_occurrence_id) +
                    2 * sizeof(event_seed_binding_id),
                map_bytes, error,
                "Component 08 mapping byte estimate overflowed"))
    return false;
  if (!add(event_bytes, occurrence_bytes, plan.estimate.persistent_bytes,
           error, "Component 08 persistent byte estimate overflowed") ||
      !add(plan.estimate.persistent_bytes, binding_bytes,
           plan.estimate.persistent_bytes, error,
           "Component 08 persistent byte estimate overflowed") ||
      !add(plan.estimate.persistent_bytes, incidence_bytes,
           plan.estimate.persistent_bytes, error,
           "Component 08 persistent byte estimate overflowed") ||
      !add(plan.estimate.persistent_bytes, map_bytes,
           plan.estimate.persistent_bytes, error,
           "Component 08 persistent byte estimate overflowed"))
    return false;

  std::uint64_t proposal_bytes = 0;
  std::uint64_t order_bytes = 0;
  if (!multiply(seed_count, sizeof(normalized_event_seed_proposal),
                proposal_bytes, error,
                "Component 08 proposal byte estimate overflowed") ||
      !multiply(seed_count, 3 * sizeof(std::size_t), order_bytes, error,
                "Component 08 sort workspace estimate overflowed") ||
      !add(proposal_bytes, order_bytes, plan.estimate.temporary_bytes, error,
           "Component 08 temporary byte estimate overflowed"))
    return false;

  if (!capacity(plan.estimate.event_count <= capabilities.maximum_events,
                error, "Component 08 event capacity exceeded") ||
      !capacity(plan.estimate.occurrence_count <=
                    capabilities.maximum_occurrences,
                error, "Component 08 occurrence capacity exceeded") ||
      !capacity(plan.estimate.seed_binding_count <=
                    capabilities.maximum_seed_bindings,
                error, "Component 08 seed-binding capacity exceeded") ||
      !capacity(plan.estimate.incidence_count <=
                    capabilities.maximum_incidence,
                error, "Component 08 incidence capacity exceeded") ||
      !capacity(plan.estimate.work_units <= capabilities.maximum_work_units,
                error, "Component 08 work capacity exceeded"))
    return false;

  std::uint64_t canonical_bytes = 0;
  if (!multiply(plan.estimate.persistent_bytes, std::uint64_t{2},
                canonical_bytes, error,
                "Component 08 canonical byte estimate overflowed") ||
      !add(canonical_bytes, std::uint64_t{4096}, canonical_bytes, error,
           "Component 08 canonical byte estimate overflowed"))
    return false;
  if (!capacity(canonical_bytes <= capabilities.maximum_canonical_bytes,
                error, "Component 08 canonical-byte capacity exceeded"))
    return false;
  return true;
}

} // namespace ygor::mesh_boolean::bounded
