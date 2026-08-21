#pragma once

#include "EventNormalization.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct event_interning_tables final {
  std::vector<intersection_event_record> events{};
  std::vector<intersection_occurrence_record> occurrences{};
  std::vector<event_seed_binding_record> seed_bindings{};
  std::vector<event_seed_binding_id> event_binding_index{};
  std::vector<event_seed_binding_id> occurrence_binding_index{};
  std::vector<event_id> seed_to_event{};
  std::vector<event_occurrence_id> seed_to_occurrence{};
};

bool intern_normalized_event_seeds(
    const std::vector<normalized_event_seed_proposal> &proposals,
    event_interning_tables &tables, bounded_boolean_error &error);

bool verify_event_interning_tables(
    const std::vector<normalized_event_seed_proposal> &proposals,
    const event_interning_tables &tables,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
