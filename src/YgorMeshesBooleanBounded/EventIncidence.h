#pragma once

#include "EventInterning.h"
#include "SignedFeatureRelations.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct event_incidence_tables final {
  std::vector<event_incidence_record> records{};
  std::vector<event_incidence_id> by_event{};
  std::vector<intersection_range> event_ranges{};
  std::vector<event_incidence_id> by_occurrence{};
  std::vector<intersection_range> occurrence_ranges{};
  std::vector<event_incidence_id> by_seed{};
  std::vector<intersection_range> seed_ranges{};
  std::vector<event_incidence_id> seed_candidate_index{};
  std::vector<intersection_range> seed_candidate_ranges{};
  std::vector<event_incidence_id> by_relation{};
  std::vector<intersection_range> relation_ranges{};
  std::vector<event_incidence_id> by_candidate{};
  std::vector<intersection_range> candidate_ranges{};
  std::vector<event_incidence_id> by_source_feature{};
  std::vector<source_feature_incidence_range_record> source_feature_ranges{};
  std::vector<event_incidence_id> by_halfedge{};
  std::vector<oriented_halfedge_incidence_range_record> halfedge_ranges{};
};

bool build_event_incidence_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_feature_key> &seed_incidence,
    const std::vector<relation_event_seed_candidate_incidence_record> &
        candidate_incidence,
    std::uint64_t relation_count, std::uint64_t candidate_count,
    event_interning_tables &interning, event_incidence_tables &tables,
    bounded_boolean_error &error);

bool verify_event_incidence_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_feature_key> &seed_incidence,
    const std::vector<relation_event_seed_candidate_incidence_record> &
        candidate_incidence,
    std::uint64_t relation_count, std::uint64_t candidate_count,
    const event_interning_tables &interning,
    const event_incidence_tables &tables, bounded_boolean_error &error);

template <class T, class I>
bool build_event_incidence(
    const signed_feature_relations<T, I> &relations,
    event_interning_tables &interning, event_incidence_tables &tables,
    bounded_boolean_error &error) {
  return build_event_incidence_records(
      relations.event_seeds(), relations.event_seed_incidence(),
      relations.event_seed_candidate_incidence(), relations.relations().size(),
      relations.candidate_dispositions().size(), interning, tables, error);
}

} // namespace ygor::mesh_boolean::bounded
