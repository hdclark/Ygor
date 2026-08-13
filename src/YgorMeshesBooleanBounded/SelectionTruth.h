#pragma once

#include "CanonicalHalfedgeOperand.h"
#include "ClassificationComplex.h"
#include "SelectionTypes.h"

#include <cstdint>
#include <optional>

namespace ygor::mesh_boolean::bounded {

// Component 10 truth service. This module constructs the four-bit operand
// occupancy tuple on both conceptual sides of a positive-area atom and calls
// the frozen Component 01 truth-table service exactly once. It performs no
// geometric predicate and never duplicates Boolean logic.
namespace selection_truth {

// The atom-side occupancy assembled in canonical A/B roles.
struct atom_side_input final {
  operand_id source_operand = operand_id::a;
  std::uint64_t source_shell = 0;
  selection_side_occupancy occupancy{};
  std::array<side_state_origin, 4> origins{};
  bool coincident = false;
};

inline bool occupancy_to_bool(occupancy_state state) noexcept {
  switch (state) {
  case occupancy_state::strict_inside:
  case occupancy_state::symbolic_positive:
    return true;
  case occupancy_state::strict_outside:
  case occupancy_state::symbolic_negative:
    return false;
  case occupancy_state::boundary:
  case occupancy_state::coincident_owned:
  case occupancy_state::unresolved:
  case occupancy_state::invalid:
    return false;
  }
  return false;
}

inline bool occupancy_is_definite(occupancy_state state) noexcept {
  switch (state) {
  case occupancy_state::strict_inside:
  case occupancy_state::strict_outside:
  case occupancy_state::symbolic_positive:
  case occupancy_state::symbolic_negative:
    return true;
  case occupancy_state::boundary:
  case occupancy_state::coincident_owned:
  case occupancy_state::unresolved:
  case occupancy_state::invalid:
    return false;
  }
  return false;
}

inline side_state_origin origin_for(occupancy_state state) noexcept {
  switch (state) {
  case occupancy_state::strict_inside:
  case occupancy_state::strict_outside:
    return side_state_origin::numeric_classification;
  case occupancy_state::symbolic_positive:
  case occupancy_state::symbolic_negative:
    return side_state_origin::symbolic;
  case occupancy_state::coincident_owned:
    return side_state_origin::coincident_owned;
  case occupancy_state::boundary:
    return side_state_origin::boundary_derived;
  case occupancy_state::unresolved:
  case occupancy_state::invalid:
    return side_state_origin::numeric_classification;
  }
  return side_state_origin::numeric_classification;
}

// Recover the source operand's material side (the side it occupies) for an
// atom from Component 02 shell semantics.
template <class T, class I>
std::optional<occupied_side> atom_material_side(
    const canonical_halfedge_operand<T, I> &manifold,
    const classification_atom_record<T> &atom) {
  const auto &shell_to_group = manifold.source_shell_to_group();
  if (atom.shell >= shell_to_group.size())
    return std::nullopt;
  const auto group = shell_to_group[atom.shell];
  if (group >= manifold.shell_groups().size())
    return std::nullopt;
  const auto side = manifold.shell_groups()[group].material_side;
  if (side != occupied_side::negative && side != occupied_side::positive)
    return std::nullopt;
  return side;
}

// Assemble the canonical A/B side occupancy tuple for a positive-area atom.
// The source operand occupancy is complementary (Component 02 shell semantics);
// the opposite operand occupancy comes exclusively from the Component 09 side
// label. A coincident-owned or unresolved opposite side defers to the sheet
// cell phase via `coincident`.
template <class T, class I>
std::optional<atom_side_input> build_atom_side_input(
    const canonical_halfedge_operand<T, I> &source_manifold,
    const classification_complex<T, I> &classification,
    const classification_atom_record<T> &atom,
    const atom_side_label_record &label) {
  (void)classification;
  atom_side_input input;
  input.source_operand = atom.operand;
  input.source_shell = atom.shell;

  const auto material = atom_material_side(source_manifold, atom);
  if (!material)
    return std::nullopt;
  const bool source_negative = *material == occupied_side::negative;
  const bool source_positive = *material == occupied_side::positive;
  // A valid boundary atom occupies exactly one side.
  if (source_negative == source_positive)
    return std::nullopt;

  if (!occupancy_is_definite(label.negative_side) ||
      !occupancy_is_definite(label.positive_side)) {
    // Coincident / unresolved opposite side: defer to the sheet cell phase.
    const bool any_coincident =
        label.negative_side == occupancy_state::coincident_owned ||
        label.positive_side == occupancy_state::coincident_owned;
    const bool any_unresolved =
        label.negative_side == occupancy_state::unresolved ||
        label.positive_side == occupancy_state::unresolved ||
        label.negative_side == occupancy_state::invalid ||
        label.positive_side == occupancy_state::invalid;
    if (any_unresolved)
      return std::nullopt;
    input.coincident = any_coincident || label.contact == boundary_contact_state::coincident ||
                       label.contact == boundary_contact_state::coplanar ||
                       label.contact == boundary_contact_state::face_contact;
    // Provisional occupancy uses the strict-side interpretation; the sheet-cell
    // phase resolves the actual coincident ownership.
    input.occupancy.a_negative = source_negative;
    input.occupancy.a_positive = source_positive;
    input.occupancy.b_negative = occupancy_to_bool(label.negative_side);
    input.occupancy.b_positive = occupancy_to_bool(label.positive_side);
  } else {
    const bool opposite_negative = occupancy_to_bool(label.negative_side);
    const bool opposite_positive = occupancy_to_bool(label.positive_side);
    if (atom.operand == operand_id::a) {
      input.occupancy.a_negative = source_negative;
      input.occupancy.a_positive = source_positive;
      input.occupancy.b_negative = opposite_negative;
      input.occupancy.b_positive = opposite_positive;
    } else {
      input.occupancy.b_negative = source_negative;
      input.occupancy.b_positive = source_positive;
      input.occupancy.a_negative = opposite_negative;
      input.occupancy.a_positive = opposite_positive;
    }
  }

  // Origins: the source operand bits come from shell semantics; the opposite
  // bits come from the numeric side label.
  input.origins = {{
      side_state_origin::source_shell,      // a_negative
      (atom.operand == operand_id::a) ? origin_for(label.negative_side)
                                      : side_state_origin::source_shell,
      side_state_origin::source_shell,      // a_positive
      (atom.operand == operand_id::a) ? origin_for(label.positive_side)
                                      : side_state_origin::source_shell,
  }};
  if (atom.operand == operand_id::b) {
    input.origins[0] = origin_for(label.negative_side);
    input.origins[2] = origin_for(label.positive_side);
    input.origins[1] = side_state_origin::source_shell;
    input.origins[3] = side_state_origin::source_shell;
  }
  return input;
}

// Evaluate the frozen Component 01 truth table exactly once for an atom.
inline truth_cell evaluate_atom_truth(boolean_operation operation,
                                      const selection_side_occupancy &occupancy) {
  side_occupancy raw;
  raw.a_negative = occupancy.a_negative;
  raw.b_negative = occupancy.b_negative;
  raw.a_positive = occupancy.a_positive;
  raw.b_positive = occupancy.b_positive;
  return evaluate_truth(operation, raw);
}

} // namespace selection_truth

} // namespace ygor::mesh_boolean::bounded
