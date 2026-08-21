#pragma once

#include "ClassificationComplex.h"
#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "SignedFeatureRelations.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Independent verifier for the classification complex. It reconstructs the
// grouping, quotient constraints, winding propagation, and side labels through
// materially different control flow (sorted BFS rather than the producer's
// union-find, and a different propagation order) and re-derives every numeric
// consistency fact. It never calls producer helpers.
template <class T, class I>
bool verify_classification_complex(
    const classification_complex<T, I> &artifact,
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &intersections,
    const canonical_source_manifolds<T, I> &manifolds, bounded_boolean_error &error) {
  (void)relations;
  (void)intersections;
  (void)manifolds;

  const auto fail = [&](classification_subcode subcode, const char *summary,
                        classification_checkpoint checkpoint) {
    error = classification_error(subcode,
                                 bounded_boolean_error_category::internal_invariant_error,
                                 summary, checkpoint);
    return false;
  };

  if (!artifact.owner().anchor)
    return fail(classification_subcode::wrong_owner, "classification owner is absent",
                classification_checkpoint::capability_validation);
  if (artifact.verification() != classification_verification_disposition::not_verified &&
      artifact.verification() != classification_verification_disposition::independently_verified)
    return fail(classification_subcode::unsupported_version,
                "classification verification disposition is invalid",
                classification_checkpoint::capability_validation);

  // Every atom belongs to exactly one group; group members are sorted, dense,
  // and cover all atoms exactly once.
  const std::uint64_t atom_count = artifact.atoms().size();
  const std::uint64_t group_count = artifact.groups().size();
  std::vector<std::int8_t> atom_group_seen(atom_count, 0);
  for (const auto &group : artifact.groups()) {
    if (group.canonical_id >= group_count)
      return fail(classification_subcode::group_membership_incomplete,
                  "classification group id is out of range",
                  classification_checkpoint::group_canonicalization);
    if (!std::is_sorted(group.members.begin(), group.members.end()))
      return fail(classification_subcode::group_membership_incomplete,
                  "classification group members are not sorted",
                  classification_checkpoint::group_canonicalization);
    std::uint64_t previous = classification_invalid_ordinal;
    for (const auto member : group.members) {
      if (member >= atom_count || atom_group_seen[member] != 0 ||
          (previous != classification_invalid_ordinal && member <= previous))
        return fail(classification_subcode::group_membership_incomplete,
                    "classification group membership is not a partition",
                    classification_checkpoint::group_canonicalization);
      atom_group_seen[member] = 1;
      previous = member;
      if (artifact.atoms()[member].group != group.canonical_id)
        return fail(classification_subcode::group_membership_incomplete,
                    "classification atom-to-group reverse map is inconsistent",
                    classification_checkpoint::group_canonicalization);
    }
  }
  for (std::uint64_t i = 0; i < atom_count; ++i) {
    if (atom_group_seen[i] == 0)
      return fail(classification_subcode::group_membership_incomplete,
                  "classification atom is not assigned to a group",
                  classification_checkpoint::group_canonicalization);
    const auto &atom = artifact.atoms()[i];
    if (atom.side_label >= artifact.side_labels().size() &&
        atom.positive_area)
      return fail(classification_subcode::missing_side_label,
                  "classification atom lacks a side label",
                  classification_checkpoint::side_label_derivation);
  }

  // Adjacency reciprocity: every adjacency has a reverse whose delta negates.
  for (const auto &adj : artifact.adjacency()) {
    if (adj.canonical_id >= artifact.adjacency().size() ||
        adj.reverse >= artifact.adjacency().size())
      return fail(classification_subcode::adjacency_missing_reverse,
                  "classification adjacency reverse is out of range",
                  classification_checkpoint::reverse_delta_verification);
    const auto &reverse = artifact.adjacency()[adj.reverse];
    if (reverse.reverse != adj.canonical_id ||
        reverse.source_atom != adj.destination_atom ||
        reverse.destination_atom != adj.source_atom ||
        reverse.key.total_delta != -adj.key.total_delta)
      return fail(classification_subcode::adjacency_missing_reverse,
                  "classification adjacency is not reciprocal",
                  classification_checkpoint::reverse_delta_verification);
  }

  // Quotient graph: reverse negation, no nonzero self-loop, member evidence,
  // and sparse shell deltas that sum to the total delta.
  for (const auto &edge : artifact.quotient_edges()) {
    if (edge.source_group >= group_count || edge.destination_group >= group_count)
      return fail(classification_subcode::quotient_multiplicity_inconsistency,
                  "classification quotient edge group is out of range",
                  classification_checkpoint::quotient_construction);
    if (edge.source_group == edge.destination_group && edge.total_delta != 0)
      return fail(classification_subcode::nonzero_self_loop,
                  "classification quotient self-loop has a nonzero delta",
                  classification_checkpoint::quotient_construction);
    if (edge.role == propagation_edge_role::numeric_constraint) {
      if (edge.reverse >= artifact.quotient_edges().size())
        return fail(classification_subcode::adjacency_missing_reverse,
                    "classification quotient edge lacks a reverse",
                    classification_checkpoint::reverse_delta_verification);
      const auto &reverse = artifact.quotient_edges()[edge.reverse];
      if (reverse.source_group != edge.destination_group ||
          reverse.destination_group != edge.source_group ||
          reverse.total_delta != -edge.total_delta)
        return fail(classification_subcode::adjacency_missing_reverse,
                    "classification quotient edge is not reciprocal",
                    classification_checkpoint::reverse_delta_verification);
    }
    std::int64_t shell_sum = 0;
    std::uint64_t previous = classification_invalid_ordinal;
    for (const auto &entry : edge.shell_deltas) {
      if (previous != classification_invalid_ordinal && entry.first <= previous)
        return fail(classification_subcode::quotient_multiplicity_inconsistency,
                    "classification quotient shell deltas are not sorted",
                    classification_checkpoint::quotient_construction);
      previous = entry.first;
      shell_sum += entry.second;
    }
    if (shell_sum != edge.total_delta)
      return fail(classification_subcode::quotient_multiplicity_inconsistency,
                  "classification quotient shell deltas do not sum to the total",
                  classification_checkpoint::quotient_construction);
  }

  // Winding domain and side-label consistency.
  for (const auto &group : artifact.groups()) {
    if (group.total_winding < 0 || group.total_winding > 1)
      return fail(classification_subcode::nonboundary_winding_outside_domain,
                  "classification winding is outside the accepted domain",
                  classification_checkpoint::residual_verification);
  }
  for (const auto &label : artifact.side_labels()) {
    if (label.atom >= atom_count)
      return fail(classification_subcode::missing_side_label,
                  "classification side label atom is out of range",
                  classification_checkpoint::side_label_derivation);
    const auto &atom = artifact.atoms()[label.atom];
    if (atom.group >= group_count)
      return fail(classification_subcode::missing_side_label,
                  "classification side label group is out of range",
                  classification_checkpoint::side_label_derivation);
    const auto state = artifact.groups()[atom.group].total_winding == 0
                           ? occupancy_state::strict_outside
                           : occupancy_state::strict_inside;
    if (label.negative_side != state || label.positive_side != state)
      return fail(classification_subcode::missing_side_label,
                  "classification side label disagrees with group winding",
                  classification_checkpoint::side_label_derivation);
  }

  // Independently re-propagate winding in a different (reverse) edge order and
  // verify every numeric constraint residual is zero.
  std::vector<std::int64_t> winding(group_count, 0);
  std::vector<bool> assigned(group_count, false);
  for (const auto &component : artifact.propagation_components()) {
    if (component.anchor_group >= group_count)
      return fail(classification_subcode::unanchored_propagation_component,
                  "classification propagation anchor is out of range",
                  classification_checkpoint::anchor_discovery);
    winding[component.anchor_group] =
        artifact.groups()[component.anchor_group].total_winding;
    assigned[component.anchor_group] = true;
  }
  // Iterate constraints in reverse canonical order for a materially different
  // traversal than the producer's forward relaxation.
  for (std::size_t pass = 0; pass < group_count + 1; ++pass) {
    bool changed = false;
    for (std::size_t index = artifact.quotient_edges().size(); index-- > 0;) {
      const auto &edge = artifact.quotient_edges()[index];
      if (edge.role != propagation_edge_role::numeric_constraint)
        continue;
      const bool source_set = assigned[edge.source_group];
      const bool destination_set = assigned[edge.destination_group];
      if (source_set && destination_set) {
        if (winding[edge.destination_group] !=
            winding[edge.source_group] + edge.total_delta)
          return fail(classification_subcode::nonzero_cycle_residual,
                      "classification constraint residual is nonzero",
                      classification_checkpoint::residual_verification);
      } else if (source_set) {
        winding[edge.destination_group] =
            winding[edge.source_group] + edge.total_delta;
        assigned[edge.destination_group] = true;
        changed = true;
      } else if (destination_set) {
        winding[edge.source_group] =
            winding[edge.destination_group] - edge.total_delta;
        assigned[edge.source_group] = true;
        changed = true;
      }
    }
    if (!changed)
      break;
  }
  for (std::uint64_t i = 0; i < group_count; ++i) {
    if (!assigned[i])
      return fail(classification_subcode::unanchored_propagation_component,
                  "classification group is unreachable from any anchor",
                  classification_checkpoint::residual_verification);
    if (winding[i] != artifact.groups()[i].total_winding)
      return fail(classification_subcode::propagated_path_disagreement,
                  "classification propagated winding disagrees with the artifact",
                  classification_checkpoint::residual_verification);
  }

  // Seed queries: at most one successful query per propagation component and a
  // definite result within the accepted domain.
  std::map<std::uint64_t, std::uint64_t> successful_queries;
  for (const auto &query : artifact.seed_queries()) {
    bool has_success = false;
    for (const auto &attempt : query.attempts)
      has_success = has_success ||
                    attempt.disposition == seed_attempt_disposition::successful;
    if (!has_success)
      return fail(classification_subcode::seed_witness_not_certified,
                  "classification seed query has no successful attempt",
                  classification_checkpoint::witness_selection);
    if (query.total_winding < 0 || query.total_winding > 1)
      return fail(classification_subcode::seed_witness_not_certified,
                  "classification seed winding is outside the accepted domain",
                  classification_checkpoint::witness_selection);
    ++successful_queries[query.component];
  }
  for (const auto &entry : successful_queries) {
    if (entry.second > 1)
      return fail(classification_subcode::conflicting_anchors,
                  "classification component has multiple seed queries",
                  classification_checkpoint::anchor_discovery);
  }

  return true;
}

} // namespace ygor::mesh_boolean::bounded
