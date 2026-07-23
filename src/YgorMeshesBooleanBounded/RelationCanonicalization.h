#pragma once

#include "SignedFeatureRelations.h"
#include "Outcome.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct relation_candidate_disposition_proposal final {
  candidate_id candidate{0};
  candidate_relation_disposition_kind disposition =
      candidate_relation_disposition_kind::no_public_relation;
  feature_relation_id public_relation{0};
  relation_request_id bookkeeping_request{0};
};

boolean_outcome<std::vector<relation_candidate_disposition_record>>
canonicalize_candidate_dispositions(
    std::vector<relation_candidate_disposition_proposal> proposals,
    std::uint64_t candidate_count,
    const relation_capabilities &capabilities);

} // namespace ygor::mesh_boolean::bounded
