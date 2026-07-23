#pragma once

#include "SignedFeatureRelations.h"
#include "Outcome.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct relation_event_seed_proposal final {
  relation_event_seed_key key{};
  feature_relation_id source_relation{0};
  relation_construction_id construction{0};
  std::vector<relation_feature_key> incidence;
  bool distinct_occurrence_required = false;
};

struct relation_event_seed_table final {
  std::vector<relation_event_seed_record> records;
  std::vector<relation_feature_key> incidence;
  bounded_boolean_digest semantic_digest{};
};

boolean_outcome<relation_event_seed_table> canonicalize_relation_event_seeds(
    std::vector<relation_event_seed_proposal> proposals,
    const relation_capabilities &capabilities);

std::vector<std::uint8_t>
encode_relation_event_seed_table(const relation_event_seed_table &table);

} // namespace ygor::mesh_boolean::bounded
