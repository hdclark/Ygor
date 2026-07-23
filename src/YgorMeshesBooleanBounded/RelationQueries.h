#pragma once

#include "SignedFeatureRelations.h"

#include <algorithm>
#include <utility>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
std::pair<const relation_candidate_disposition_record *,
          const relation_candidate_disposition_record *>
candidate_disposition_range(const signed_feature_relations<T, I> &artifact,
                            candidate_id candidate,
                            const context_owner_token &owner) noexcept {
  if (!artifact.owner().same_owner(owner))
    return {nullptr, nullptr};
  const auto &records = artifact.candidate_dispositions();
  const auto first = std::lower_bound(
      records.begin(), records.end(), candidate,
      [](const relation_candidate_disposition_record &record,
         candidate_id value) { return record.candidate < value; });
  const auto last = std::upper_bound(
      first, records.end(), candidate,
      [](candidate_id value,
         const relation_candidate_disposition_record &record) {
        return value < record.candidate;
      });
  const auto begin_index = static_cast<std::size_t>(first - records.begin());
  const auto end_index = static_cast<std::size_t>(last - records.begin());
  const auto *base = records.empty() ? nullptr : records.data();
  return {base ? base + begin_index : nullptr, base ? base + end_index : nullptr};
}

} // namespace ygor::mesh_boolean::bounded
