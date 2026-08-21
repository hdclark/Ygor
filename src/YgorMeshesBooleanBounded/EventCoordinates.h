#pragma once

#include "EventInterning.h"

namespace ygor::mesh_boolean::bounded {

struct event_coordinate_tables final {
  std::vector<relation_construction_ledger_id> construction_witness_index{};
};

bool attach_event_coordinates(
    const std::vector<normalized_event_seed_proposal> &proposals,
    const std::vector<relation_construction_record> &constructions,
    const std::vector<relation_construction_ledger_record> &construction_ledger,
    event_interning_tables &interning,
    event_coordinate_tables &coordinates,
    bounded_boolean_error &error);

bool verify_event_coordinates(
    const std::vector<normalized_event_seed_proposal> &proposals,
    const std::vector<relation_construction_record> &constructions,
    const std::vector<relation_construction_ledger_record> &construction_ledger,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
