#pragma once

#include "SelectionTypes.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Component 10 exact planned-edge occurrence construction. This module groups
// directed retained incidences into exactly-two-member edge occurrences using
// complete reciprocal surface-occurrence descriptors. It performs no
// coordinate, tolerance, or hash-based decision and never runs general or
// minimum-cost matching.
namespace selection_edge_occurrences {

// One directed incidence proposal fed to the mate-grouping phase.
struct directed_proposal final {
  std::uint64_t incidence = selection_invalid_ordinal;
  selection_carrier_identity carrier{};
  std::uint64_t start_domain = selection_invalid_ordinal;
  std::uint64_t end_domain = selection_invalid_ordinal;
  direction_role direction = direction_role::forward;
  surface_occurrence_descriptor descriptor{};
  surface_occurrence_descriptor expected_opposite{};
  std::uint64_t slot_discriminator = 0;
  std::uint64_t start_sector_lineage = 0;
  std::uint64_t end_sector_lineage = 0;
};

// The two directed members of a valid planned edge in canonical order.
struct edge_pair final {
  directed_proposal forward{};
  directed_proposal reverse{};
  edge_occurrence_slot_key slot{};
  bool cross_operand = false;
  bool cross_owner = false;
};

enum class grouping_status : std::uint8_t {
  success = 1,
  mate_cardinality = 2,
  mate_direction = 3,
  endpoint_incompatibility = 4,
  nonreciprocal_descriptor = 5,
  slot_discriminator_mismatch = 6,
};

// Group directed proposals into exactly-two-member edge occurrences. A valid
// slot contains exactly one forward and one reverse member under the carrier
// orientation, matching unordered endpoint-domain lineage, and mutually
// reciprocal actual/expected descriptors. Valid pairs may have different
// source operands or sheet owners (a proper transverse seam normally pairs an
// A-owned use with a B-owned use).
inline grouping_status group_directed_proposals(
    std::vector<directed_proposal> proposals,
    std::vector<edge_pair> &pairs) {
  pairs.clear();
  std::sort(proposals.begin(), proposals.end(),
            [](const directed_proposal &a, const directed_proposal &b) {
              const edge_occurrence_slot_key ka = edge_occurrence_slot_key::canonical(
                  a.carrier, a.start_domain, a.end_domain,
                  a.slot_discriminator,
                  surface_occurrence_descriptor_pair::ordered(
                      a.descriptor, a.expected_opposite));
              const edge_occurrence_slot_key kb = edge_occurrence_slot_key::canonical(
                  b.carrier, b.start_domain, b.end_domain,
                  b.slot_discriminator,
                  surface_occurrence_descriptor_pair::ordered(
                      b.descriptor, b.expected_opposite));
              return ka < kb;
            });

  std::size_t cursor = 0;
  while (cursor < proposals.size()) {
    const auto slot = edge_occurrence_slot_key::canonical(
        proposals[cursor].carrier, proposals[cursor].start_domain,
        proposals[cursor].end_domain, proposals[cursor].slot_discriminator,
        surface_occurrence_descriptor_pair::ordered(
            proposals[cursor].descriptor, proposals[cursor].expected_opposite));
    std::size_t end = cursor;
    while (end < proposals.size()) {
      const auto candidate = edge_occurrence_slot_key::canonical(
          proposals[end].carrier, proposals[end].start_domain,
          proposals[end].end_domain, proposals[end].slot_discriminator,
          surface_occurrence_descriptor_pair::ordered(
              proposals[end].descriptor, proposals[end].expected_opposite));
      if (!(candidate == slot))
        break;
      ++end;
    }
    const std::size_t count = end - cursor;
    if (count != 2)
      return grouping_status::mate_cardinality;

    const auto &a = proposals[cursor];
    const auto &b = proposals[cursor + 1];
    if (a.direction == b.direction)
      return grouping_status::mate_direction;
    if (a.start_domain != b.end_domain || a.end_domain != b.start_domain)
      return grouping_status::endpoint_incompatibility;
    if (!(a.descriptor == b.expected_opposite) ||
        !(b.descriptor == a.expected_opposite))
      return grouping_status::nonreciprocal_descriptor;
    if (a.slot_discriminator != b.slot_discriminator)
      return grouping_status::slot_discriminator_mismatch;

    edge_pair pair;
    if (a.direction == direction_role::forward) {
      pair.forward = a;
      pair.reverse = b;
    } else {
      pair.forward = b;
      pair.reverse = a;
    }
    pair.slot = slot;
    pair.cross_operand = a.descriptor.source_operand != b.descriptor.source_operand;
    pair.cross_owner = a.descriptor.sheet_owner_lineage != b.descriptor.sheet_owner_lineage;
    pairs.push_back(std::move(pair));
    cursor = end;
  }
  return grouping_status::success;
}

} // namespace selection_edge_occurrences

} // namespace ygor::mesh_boolean::bounded
