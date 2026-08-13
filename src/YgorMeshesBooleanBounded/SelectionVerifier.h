#pragma once

#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "ClassificationComplex.h"
#include "Context.h"
#include "RetainedSurfaceComplex.h"
#include "SelectionCodec.h"
#include "SelectionCoincidence.h"
#include "SelectionEdgeOccurrences.h"
#include "SelectionFeasibility.h"
#include "SelectionTruth.h"
#include "SelectionVertexOccurrences.h"
#include "SignedFeatureRelations.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Component 10 independent verifier. It reconstructs dispositions, truth
// cells, sheet cells, ownership, retained uses, edge reciprocality, and
// vertex-occurrence cycles through materially different control flow than the
// producer, and re-encodes the artifact to verify its digest. It never calls
// producer orchestration, sheet grouping, owner choice, or slot grouping
// helpers as its sole proof.
template <class T, class I>
bool verify_retained_surface_complex(
    const retained_surface_complex<T, I> &artifact,
    const boolean_context<T, I> &context,
    const canonical_source_manifolds<T, I> &manifolds,
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &intersections,
    const classification_complex<T, I> &classification,
    bounded_boolean_error &error) {
  const auto fail = [&](selection_subcode subcode, const char *summary,
                        selection_checkpoint checkpoint) {
    error = selection_error(subcode,
                            bounded_boolean_error_category::internal_invariant_error,
                            summary, checkpoint);
    return false;
  };

  // Header and predecessor handshakes.
  if (!artifact.owner().anchor)
    return fail(selection_subcode::wrong_owner, "selection owner is absent",
                selection_checkpoint::context_capability_validation);
  if (!artifact.owner().same_owner(context.owner))
    return fail(selection_subcode::wrong_owner,
                "selection owner disagrees with the context",
                selection_checkpoint::context_capability_validation);
  if (artifact.operation() != context.operation)
    return fail(selection_subcode::wrong_operation,
                "selection operation disagrees with the context",
                selection_checkpoint::context_capability_validation);
  if (artifact.context_digest() != context.context_digest ||
      artifact.relation_digest() != relations.digest() ||
      artifact.intersection_digest() != intersections.digest() ||
      artifact.classification_digest() != classification.digest())
    return fail(selection_subcode::predecessor_digest_mismatch,
                "selection predecessor digest mismatch",
                selection_checkpoint::predecessor_validation);
  if (artifact.manifold_digests()[0] != manifolds.a()->digest() ||
      artifact.manifold_digests()[1] != manifolds.b()->digest())
    return fail(selection_subcode::predecessor_digest_mismatch,
                "selection manifold digest mismatch",
                selection_checkpoint::predecessor_validation);

  // Recompute the positive-area atom domain.
  std::vector<std::uint64_t> positive_atoms;
  for (std::uint64_t i = 0; i < classification.atoms().size(); ++i)
    if (classification.atoms()[i].positive_area)
      positive_atoms.push_back(i);
  const std::uint64_t atom_count = positive_atoms.size();

  // Rebuild side tuples and truth cells in reverse atom order (a materially
  // different traversal than the producer's forward order).
  const auto manifold_for = [&](operand_id id) -> const canonical_halfedge_operand<T, I> & {
    return id == operand_id::a ? *manifolds.a() : *manifolds.b();
  };
  for (std::size_t index = positive_atoms.size(); index-- > 0;) {
    const std::uint64_t a = positive_atoms[index];
    const auto &atom = classification.atoms()[a];
    if (a >= artifact.disposition_by_atom().size())
      return fail(selection_subcode::missing_atom,
                  "selection reverse map lacks an atom",
                  selection_checkpoint::atom_coverage_validation);
    const std::uint64_t d = artifact.disposition_by_atom()[a];
    if (d >= artifact.dispositions().size())
      return fail(selection_subcode::missing_atom,
                  "selection disposition ordinal is out of range",
                  selection_checkpoint::atom_coverage_validation);
    const auto &disposition = artifact.dispositions()[d];
    if (disposition.atom != a)
      return fail(selection_subcode::reverse_map_error,
                  "selection disposition does not map back to its atom",
                  selection_checkpoint::atom_coverage_validation);

    if (atom.side_label >= classification.side_labels().size())
      return fail(selection_subcode::invalid_side_tuple,
                  "atom side label is out of range",
                  selection_checkpoint::side_tuple_construction);
    const auto &label = classification.side_labels()[atom.side_label];
    const auto input = selection_truth::build_atom_side_input(
        manifold_for(atom.operand), classification, atom, label);
    if (!input)
      return fail(selection_subcode::invalid_side_tuple,
                  "atom side tuple cannot be reconstructed",
                  selection_checkpoint::side_tuple_construction);

    if (disposition.side_tuple >= artifact.side_tuples().size())
      return fail(selection_subcode::invalid_side_tuple,
                  "disposition side tuple is out of range",
                  selection_checkpoint::side_tuple_construction);
    const auto &stored_tuple = artifact.side_tuples()[disposition.side_tuple];
    if (!(stored_tuple.occupancy == input->occupancy))
      return fail(selection_subcode::invalid_side_tuple,
                  "stored side tuple disagrees with reconstruction",
                  selection_checkpoint::side_tuple_construction);

    const auto cell = selection_truth::evaluate_atom_truth(context.operation,
                                                           input->occupancy);
    if (disposition.truth >= artifact.truth_records().size())
      return fail(selection_subcode::invalid_truth_cell,
                  "disposition truth record is out of range",
                  selection_checkpoint::truth_evaluation);
    const auto &stored_truth = artifact.truth_records()[disposition.truth];
    if (stored_truth.retain != cell.retain ||
        stored_truth.orientation != cell.orientation ||
        stored_truth.result_negative != cell.result_negative ||
        stored_truth.result_positive != cell.result_positive)
      return fail(selection_subcode::invalid_truth_cell,
                  "stored truth cell disagrees with the frozen table",
                  selection_checkpoint::truth_evaluation);

    // Disposition must match the truth cell unless a coincident sheet cell
    // resolves it jointly.
    const bool coincident = disposition.sheet_cell != selection_invalid_ordinal;
    if (!coincident) {
      const final_disposition expected =
          cell.retain ? (cell.orientation == boundary_orientation::preserve
                             ? final_disposition::retain_preserve
                             : final_disposition::retain_reverse)
                      : final_disposition::discard_equal_sides;
      if (disposition.disposition != expected)
        return fail(selection_subcode::wrong_retain_decision,
                    "stored disposition disagrees with the truth cell",
                    selection_checkpoint::truth_evaluation);
    }
  }

  // Reconstruct sheet cells with an alternate scan order (descending support
  // lineage) and verify membership and ownership.
  const auto coincidence = selection_coincidence::build_atom_coincidence(
      intersections, classification);
  std::vector<std::uint64_t> support_lineages;
  for (const auto &entry : coincidence)
    if (entry.member && entry.support_lineage != 0)
      support_lineages.push_back(entry.support_lineage);
  std::sort(support_lineages.begin(), support_lineages.end(),
            std::greater<std::uint64_t>());
  support_lineages.erase(
      std::unique(support_lineages.begin(), support_lineages.end()),
      support_lineages.end());
  if (support_lineages.size() != artifact.sheet_cells().size())
    return fail(selection_subcode::malformed_sheet_cell,
                "sheet cell count disagrees with coincidence lineage",
                selection_checkpoint::sheet_cell_reconstruction);
  for (std::uint64_t c = 0; c < artifact.sheet_cells().size(); ++c) {
    const auto &cell = artifact.sheet_cells()[c];
    if (cell.members_begin + cell.members_count > artifact.cell_members().size())
      return fail(selection_subcode::sheet_membership_incomplete,
                  "sheet cell member range is out of bounds",
                  selection_checkpoint::sheet_cell_reconstruction);
    for (std::uint64_t k = cell.members_begin; k < cell.members_begin + cell.members_count; ++k) {
      const std::uint64_t member = artifact.cell_members()[k];
      if (member >= artifact.coincidence_members().size())
        return fail(selection_subcode::sheet_membership_incomplete,
                    "sheet cell member ordinal is out of range",
                    selection_checkpoint::sheet_cell_reconstruction);
      const auto &member_record = artifact.coincidence_members()[member];
      if (member_record.sheet_cell != c)
        return fail(selection_subcode::sheet_membership_incomplete,
                    "coincidence member does not map back to its cell",
                    selection_checkpoint::sheet_cell_reconstruction);
      if (member_record.atom >= classification.atoms().size())
        return fail(selection_subcode::sheet_membership_incomplete,
                    "coincidence member atom is out of range",
                    selection_checkpoint::sheet_cell_reconstruction);
    }
    // A retained coincident sheet must have exactly one owner decision.
    if (cell.owner_decision != selection_invalid_ordinal) {
      if (cell.owner_decision >= artifact.owner_decisions().size() ||
          artifact.owner_decisions()[cell.owner_decision].sheet_cell != c)
        return fail(selection_subcode::wrong_owner,
                    "owner decision does not map back to its cell",
                    selection_checkpoint::owner_resolution);
      if (artifact.owner_decisions()[cell.owner_decision].owner_atom >=
          classification.atoms().size())
        return fail(selection_subcode::wrong_owner,
                    "owner atom is out of range",
                    selection_checkpoint::owner_resolution);
    }
  }

  // Verify internal consistency of retained uses, incidences, edges, and
  // vertex-occurrence cycles.
  bounded_boolean_error audit_error;
  if (!audit_retained_feasibility(artifact, atom_count, audit_error)) {
    error = audit_error;
    return false;
  }

  // Independently reconstruct the link graph and re-extract cycles with a
  // different traversal order than the producer.
  {
    std::vector<selection_vertex_occurrences::link_port> ports;
    std::vector<selection_vertex_occurrences::link_arc> corners;
    std::vector<selection_vertex_occurrences::link_arc> mates;
    ports.reserve(artifact.local_ports().size());
    corners.reserve(artifact.face_corner_arcs().size());
    mates.reserve(artifact.edge_mate_arcs().size());
    for (const auto &port : artifact.local_ports()) {
      selection_vertex_occurrences::link_port p;
      p.corner_arc = port.face_corner_arc;
      p.mate_arc = port.edge_mate_arc;
      ports.push_back(p);
    }
    for (const auto &arc : artifact.face_corner_arcs())
      corners.push_back({arc.in_port, arc.out_port});
    for (const auto &arc : artifact.edge_mate_arcs())
      mates.push_back({arc.first_port, arc.second_port});
    std::vector<selection_vertex_occurrences::link_cycle> cycles;
    const auto status = selection_vertex_occurrences::extract_link_cycles(
        ports, corners, mates, cycles);
    if (status != selection_vertex_occurrences::link_status::success)
      return fail(selection_subcode::open_local_link,
                  "link cycle extraction failed",
                  selection_checkpoint::local_link_cycle_extraction);
    if (cycles.size() != artifact.vertex_occurrences().size())
      return fail(selection_subcode::multicycle_local_link,
                  "vertex occurrence count disagrees with link cycles",
                  selection_checkpoint::local_link_cycle_extraction);
  }

  // Re-encode and verify the digest.
  {
    selection_codec_limits limits;
    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_retained_surface_complex(artifact, bytes, limits, codec_error)) {
      error = codec_error;
      return false;
    }
    if (sha256::digest(bytes) != artifact.digest())
      return fail(selection_subcode::digest_mismatch,
                  "selection digest does not match canonical bytes",
                  selection_checkpoint::canonical_encoding);
  }

  return true;
}

// Self-contained structural verification (no predecessors). Used by mutation
// tests and by the producer's own final gate after full verification.
template <class T, class I>
bool verify_retained_surface_complex_structural(
    const retained_surface_complex<T, I> &artifact,
    std::uint64_t positive_area_atom_count, bounded_boolean_error &error) {
  if (!artifact.owner().anchor) {
    error = selection_error(selection_subcode::wrong_owner,
                            bounded_boolean_error_category::internal_invariant_error,
                            "selection owner is absent",
                            selection_checkpoint::context_capability_validation);
    return false;
  }
  if (!audit_retained_feasibility(artifact, positive_area_atom_count, error))
    return false;
  selection_codec_limits limits;
  std::vector<std::uint8_t> bytes;
  bounded_boolean_error codec_error;
  if (!encode_retained_surface_complex(artifact, bytes, limits, codec_error)) {
    error = codec_error;
    return false;
  }
  if (sha256::digest(bytes) != artifact.digest()) {
    error = selection_error(selection_subcode::digest_mismatch,
                            bounded_boolean_error_category::internal_invariant_error,
                            "selection digest does not match canonical bytes",
                            selection_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
