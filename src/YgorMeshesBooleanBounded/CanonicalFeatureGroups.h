#pragma once

#include "CanonicalHalfedgeOperand.h"

#include <algorithm>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class Range>
inline void canonical_sort_unique(Range &range) {
  std::sort(range.begin(), range.end());
  range.erase(std::unique(range.begin(), range.end()), range.end());
}

} // namespace ygor::mesh_boolean::bounded
