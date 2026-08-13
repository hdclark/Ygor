#pragma once

#include "CanonicalIntersectionComplex.h"
#include "ClassificationComplex.h"
#include "SelectionTypes.h"

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Component 10 coincidence resolution. It reconstructs sheet cells from
// Component 07-09 coplanar lineage (never from coordinates or plane equality)
// and selects exactly one canonical owner per required boundary sheet using
// the frozen Component 01 owner rank. It performs no geometric predicate.
namespace selection_coincidence {

// Per-atom coincidence provenance recovered from the intersection complex.
struct atom_coincidence final {
  std::uint64_t support_lineage = 0;
  bool opposite_orientation = false;
  operand_id symbolic_owner = operand_id::a;
  bool distinct_occurrences = false;
  sheet_relation relation = sheet_relation::ordinary;
  bool member = false;
};

inline bool feature_matches_atom(const relation_feature_key &feature,
                                 operand_id operand,
                                 std::uint64_t source_facet) noexcept {
  return feature.operand == operand &&
         feature.kind == relation_feature_kind::source_facet &&
         feature.primary == source_facet;
}

// Build the per-atom coincidence table. An atom participates in a sheet cell
// exactly when Component 08 records a coplanar support or overlap whose facet
// lineage names that atom's (operand, source_facet). Coordinates and plane
// proximity are never consulted.
template <class T, class I>
std::vector<atom_coincidence> build_atom_coincidence(
    const canonical_intersection_complex<T, I> &intersections,
    const classification_complex<T, I> &classification) {
  const std::uint64_t atom_count = classification.atoms().size();
  std::vector<atom_coincidence> table(atom_count);

  // Map (operand, source_facet) -> the first coplanar support naming it.
  std::map<std::pair<operand_id, std::uint64_t>, std::uint64_t> facet_support;
  for (std::uint64_t s = 0; s < intersections.coplanar_supports().size(); ++s) {
    const auto &support = intersections.coplanar_supports()[s];
    facet_support.emplace(
        std::make_pair(support.first_facet.operand, support.first_facet.primary),
        s);
    facet_support.emplace(
        std::make_pair(support.second_facet.operand,
                       support.second_facet.primary),
        s);
  }

  for (std::uint64_t i = 0; i < atom_count; ++i) {
    const auto &atom = classification.atoms()[i];
    if (!atom.positive_area)
      continue;
    const auto found = facet_support.find(
        std::make_pair(atom.operand, atom.source_facet));
    if (found == facet_support.end())
      continue;
    const auto &support = intersections.coplanar_supports()[found->second];
    table[i].member = true;
    table[i].support_lineage = support.support_lineage;
    table[i].opposite_orientation = support.opposite_orientation;
    table[i].symbolic_owner = support.symbolic_owner;
    if (support.opposite_orientation)
      table[i].relation = sheet_relation::opposite_orientation_coincidence;
    else
      table[i].relation = sheet_relation::same_orientation_coincidence;
  }

  // Distinct-sheet-occurrence marks from coplanar overlaps.
  for (const auto &overlap : intersections.coplanar_overlaps()) {
    if (!overlap.distinct_sheet_occurrences)
      continue;
    const auto &support = intersections.coplanar_supports()[overlap.support.ordinal()];
    for (std::uint64_t i = 0; i < atom_count; ++i) {
      if (!table[i].member)
        continue;
      const auto &atom = classification.atoms()[i];
      if (feature_matches_atom(support.first_facet, atom.operand,
                               atom.source_facet) ||
          feature_matches_atom(support.second_facet, atom.operand,
                               atom.source_facet))
        table[i].distinct_occurrences = true;
    }
  }

  return table;
}

// The frozen Component 01 owner rank. Lower is preferred. Components are, in
// order: ability to realize the required final orientation, operation-specific
// operand priority, symbolic feature priority, canonical source-feature key,
// then the directed occurrence discriminator.
struct owner_rank final {
  std::int8_t orientation_capability = 0; // 0 preferred, 1 cannot realize
  std::int8_t operand_priority = 0;       // 0 preferred operand, 1 other
  std::uint8_t feature_priority = 0;
  std::uint64_t source_feature = 0;
  std::uint64_t atom_ordinal = 0;

  friend bool operator<(const owner_rank &a, const owner_rank &b) noexcept {
    return std::tie(a.orientation_capability, a.operand_priority,
                    a.feature_priority, a.source_feature, a.atom_ordinal) <
           std::tie(b.orientation_capability, b.operand_priority,
                    b.feature_priority, b.source_feature, b.atom_ordinal);
  }
  friend bool operator==(const owner_rank &a, const owner_rank &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Compute the frozen owner rank for one coincident candidate atom. The required
// final orientation is the orientation a retained owner must realize (preserve
// or reverse) as dictated by result occupancy.
inline owner_rank compute_owner_rank(boolean_operation operation,
                                     operand_id candidate_operand,
                                     std::uint64_t source_facet,
                                     std::uint64_t atom_ordinal,
                                     bool candidate_can_realize_orientation,
                                     std::uint8_t feature_priority) {
  owner_rank rank;
  rank.orientation_capability = candidate_can_realize_orientation ? 0 : 1;
  rank.operand_priority =
      candidate_operand == symbolic_preferred_operand(operation) ? 0 : 1;
  rank.feature_priority = feature_priority;
  rank.source_feature = source_facet;
  rank.atom_ordinal = atom_ordinal;
  return rank;
}

} // namespace selection_coincidence

} // namespace ygor::mesh_boolean::bounded
