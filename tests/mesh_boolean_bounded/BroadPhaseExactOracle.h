#pragma once

#include "BroadPhaseFixtures.h"

#include <algorithm>
#include <vector>

namespace broad_phase_tests {

std::vector<bounded::canonical_candidate_key>
all_pairs_keys(const bounded::canonical_candidate_stream<scalar, index_type> &artifact);

} // namespace broad_phase_tests
