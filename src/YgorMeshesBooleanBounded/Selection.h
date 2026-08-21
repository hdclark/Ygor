#pragma once

#include "Classification.h"
#include "SelectionCodec.h"
#include "SelectionCoincidence.h"
#include "SelectionEdgeOccurrences.h"
#include "SelectionFeasibility.h"
#include "SelectionTruth.h"
#include "SelectionVerifier.h"
#include "SelectionVertexOccurrences.h"
#include "Transaction.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const retained_surface_complex<T, I>>>
build_retained_surface_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const classification_complex<T, I>> classification,
    selection_capabilities capabilities,
    selection_codec_limits codec_limits = {});

#define YGOR_DECLARE_SELECTION_BUILD(T, I)                                   \
  extern template boolean_outcome<                                           \
      std::shared_ptr<const retained_surface_complex<T, I>>>                 \
  build_retained_surface_complex<T, I>(                                      \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const signed_feature_relations<T, I>>,                 \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      std::shared_ptr<const classification_complex<T, I>>,                   \
      selection_capabilities, selection_codec_limits)

YGOR_DECLARE_SELECTION_BUILD(float, std::uint32_t);
YGOR_DECLARE_SELECTION_BUILD(float, std::uint64_t);
YGOR_DECLARE_SELECTION_BUILD(double, std::uint32_t);
YGOR_DECLARE_SELECTION_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_SELECTION_BUILD

} // namespace ygor::mesh_boolean::bounded

// ---------------------------------------------------------------------------
// Builder implementation.
// ---------------------------------------------------------------------------
#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"

namespace ygor::mesh_boolean::bounded {

// ---------------------------------------------------------------------------
// The Component 10 builder.
// ---------------------------------------------------------------------------
template <class T, class I> class selection_builder {
public:
  selection_builder(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
      std::shared_ptr<const signed_feature_relations<T, I>> relations,
      std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
      std::shared_ptr<const classification_complex<T, I>> classification,
      selection_capabilities capabilities, selection_codec_limits codec_limits)
      : context_(context), precision_(precision),
        manifolds_(std::move(manifolds)), relations_(std::move(relations)),
        intersections_(std::move(intersections)),
        classification_(std::move(classification)),
        capabilities_(std::move(capabilities)), codec_limits_(codec_limits) {}

  boolean_outcome<std::shared_ptr<const retained_surface_complex<T, I>>> run() {
    bounded_boolean_error error;
    if (!validate_predecessors(error))
      return failure(error);
    if (selection_cancelled(capabilities_,
                            selection_checkpoint::predecessor_validation))
      return fail_cancelled();
    auto artifact = std::make_shared<retained_surface_complex<T, I>>();
    artifact->operation_ = context_.operation;
    artifact->owner_ = context_.owner;
    artifact->context_digest_ = context_.context_digest;
    artifact->precision_digest_ = precision_.digest();
    artifact->manifold_digests_[0] = manifolds_->a()->digest();
    artifact->manifold_digests_[1] = manifolds_->b()->digest();
    artifact->relation_digest_ = relations_->digest();
    artifact->intersection_digest_ = intersections_->digest();
    artifact->classification_digest_ = classification_->digest();

    if (!build_dispositions(*artifact, error) ||
        !build_sheet_cells(*artifact, error) ||
        !build_retained_uses(*artifact, error) ||
        !build_incidences(*artifact, error) ||
        !build_edge_occurrences(*artifact, error) ||
        !build_vertex_occurrences(*artifact, error) ||
        !finalize(*artifact, error))
      return failure(error);

    if (selection_cancelled(capabilities_, selection_checkpoint::commit))
      return fail_cancelled();
    return boolean_outcome<std::shared_ptr<const retained_surface_complex<T, I>>>::success(
        std::move(artifact));
  }

private:
  using artifact_type = retained_surface_complex<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const retained_surface_complex<T, I>>>;

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds_;
  std::shared_ptr<const signed_feature_relations<T, I>> relations_;
  std::shared_ptr<const canonical_intersection_complex<T, I>> intersections_;
  std::shared_ptr<const classification_complex<T, I>> classification_;
  selection_capabilities capabilities_;
  selection_codec_limits codec_limits_;

  outcome_type failure(const bounded_boolean_error &error) {
    return outcome_type::failure(error);
  }
  outcome_type fail_cancelled() {
    return outcome_type::failure(selection_error(
        selection_subcode::cancelled, bounded_boolean_error_category::cancelled,
        "selection cancelled", selection_checkpoint::commit));
  }

  bool validate_predecessors(bounded_boolean_error &error) {
    const auto &a = *manifolds_->a();
    const auto &b = *manifolds_->b();
    if (!a.owner().same_owner(context_.owner) || !b.owner().same_owner(context_.owner) ||
        !relations_->owner().same_owner(context_.owner) ||
        !intersections_->owner().same_owner(context_.owner) ||
        !classification_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner)) {
      error = selection_error(selection_subcode::wrong_owner,
                              bounded_boolean_error_category::internal_invariant_error,
                              "selection predecessor owner mismatch",
                              selection_checkpoint::predecessor_validation);
      return false;
    }
    if (relations_->operation() != context_.operation ||
        intersections_->operation() != context_.operation ||
        classification_->operation() != context_.operation) {
      error = selection_error(selection_subcode::wrong_operation,
                              bounded_boolean_error_category::internal_invariant_error,
                              "selection predecessor operation mismatch",
                              selection_checkpoint::predecessor_validation);
      return false;
    }
    if (relations_->verification() != relation_verification_disposition::independently_verified ||
        intersections_->verification() != intersection_verification_disposition::independently_verified ||
        classification_->verification() != classification_verification_disposition::independently_verified ||
        a.verification() != canonical_halfedge_verification_disposition::independently_verified ||
        b.verification() != canonical_halfedge_verification_disposition::independently_verified) {
      error = selection_error(selection_subcode::predecessor_not_verified,
                              bounded_boolean_error_category::internal_invariant_error,
                              "selection predecessor is not verified",
                              selection_checkpoint::predecessor_validation);
      return false;
    }
    if (capabilities_.provider_version != contract_versions::selection_provider ||
        capabilities_.codec_version != contract_versions::selection_codec ||
        capabilities_.verifier_version != contract_versions::selection_verifier) {
      error = selection_error(selection_subcode::unsupported_version,
                              bounded_boolean_error_category::input_contract_error,
                              "selection capability version mismatch",
                              selection_checkpoint::context_capability_validation);
      return false;
    }
    return true;
  }

  const canonical_halfedge_operand<T, I> &operand_manifold(operand_id id) const {
    return id == operand_id::a ? *manifolds_->a() : *manifolds_->b();
  }

  // -------------------------------------------------------------------------
  // Phase: side tuples and truth dispositions.
  // -------------------------------------------------------------------------
  bool build_dispositions(artifact_type &artifact, bounded_boolean_error &error) {
    const std::uint64_t atom_count = classification_->atoms().size();
    artifact.disposition_by_atom_.resize(atom_count, selection_invalid_ordinal);

    for (std::uint64_t a = 0; a < atom_count; ++a) {
      const auto &atom = classification_->atoms()[a];
      if (!atom.positive_area) {
        artifact.disposition_by_atom_[a] = selection_invalid_ordinal;
        continue;
      }
      if (atom.side_label >= classification_->side_labels().size()) {
        error = selection_error(selection_subcode::invalid_side_tuple,
                                bounded_boolean_error_category::internal_invariant_error,
                                "atom side label is out of range",
                                selection_checkpoint::side_tuple_construction);
        return false;
      }
      const auto &label = classification_->side_labels()[atom.side_label];
      const auto input = selection_truth::build_atom_side_input(
          operand_manifold(atom.operand), *classification_, atom, label);
      if (!input) {
        error = selection_error(selection_subcode::invalid_side_tuple,
                                bounded_boolean_error_category::internal_invariant_error,
                                "atom side tuple cannot be constructed",
                                selection_checkpoint::side_tuple_construction);
        return false;
      }

      // Side tuple record.
      selection_side_tuple_record tuple;
      tuple.canonical_id = artifact.side_tuples_.size();
      tuple.atom = a;
      tuple.source_operand = input->source_operand;
      tuple.source_shell = input->source_shell;
      tuple.occupancy = input->occupancy;
      tuple.origins = input->origins;
      tuple.coincident = input->coincident;

      // Truth record (exactly one Component 01 lookup per atom).
      const auto cell = selection_truth::evaluate_atom_truth(context_.operation,
                                                             input->occupancy);
      selection_truth_record truth;
      truth.canonical_id = artifact.truth_records_.size();
      truth.side_tuple = tuple.canonical_id;
      truth.atom = a;
      truth.operation = context_.operation;
      truth.result_negative = cell.result_negative;
      truth.result_positive = cell.result_positive;
      truth.retain = cell.retain;
      truth.orientation = cell.orientation;
      truth.multiplicity = cell.multiplicity;
      truth.owner_priority = cell.owner_priority;

      selection_disposition_record disposition;
      disposition.canonical_id = artifact.dispositions_.size();
      disposition.key.source_operand = atom.operand;
      disposition.key.atom_ordinal = a;
      disposition.key.shell = atom.shell;
      disposition.key.source_facet = atom.source_facet;
      disposition.key.occupancy = input->occupancy;
      disposition.key.operation = context_.operation;
      disposition.atom = a;
      disposition.side_tuple = tuple.canonical_id;
      disposition.truth = truth.canonical_id;
      if (cell.retain) {
        disposition.disposition =
            cell.orientation == boundary_orientation::preserve
                ? final_disposition::retain_preserve
                : final_disposition::retain_reverse;
      } else {
        disposition.disposition = final_disposition::discard_equal_sides;
      }
      disposition.source_orientation_reversed =
          disposition.disposition == final_disposition::retain_reverse;

      tuple.sheet_cell = selection_invalid_ordinal;
      disposition.sheet_cell = selection_invalid_ordinal;
      disposition.multiplicity = selection_invalid_ordinal;
      disposition.retained_use = selection_invalid_ordinal;

      artifact.side_tuples_.push_back(std::move(tuple));
      artifact.truth_records_.push_back(std::move(truth));
      artifact.dispositions_.push_back(std::move(disposition));
      artifact.disposition_by_atom_[a] = artifact.dispositions_.size() - 1;
    }

    // Reject atoms with unresolved/contradictory side states (never default
    // discard): those would have failed build_atom_side_input above.
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: sheet-cell reconstruction and ownership.
  // -------------------------------------------------------------------------
  bool build_sheet_cells(artifact_type &artifact, bounded_boolean_error &error) {
    const auto coincidence = selection_coincidence::build_atom_coincidence(
        *intersections_, *classification_);

    // Group coincident atoms by support lineage.
    std::map<std::uint64_t, std::vector<std::uint64_t>> cells;
    for (std::uint64_t a = 0; a < coincidence.size(); ++a) {
      if (!coincidence[a].member)
        continue;
      cells[coincidence[a].support_lineage].push_back(a);
    }

    for (const auto &entry : cells) {
      const auto &member_atoms = entry.second;
      const auto &proto = coincidence[member_atoms.front()];

      coincidence_sheet_cell_record cell;
      cell.canonical_id = artifact.sheet_cells_.size();
      cell.relation = proto.relation;
      cell.support_lineage = proto.support_lineage;
      cell.opposite_orientation = proto.opposite_orientation;
      cell.symbolic_owner = proto.symbolic_owner;
      cell.members_begin = artifact.cell_members_.size();
      cell.members_count = member_atoms.size();
      cell.owner_atom = selection_invalid_ordinal;
      cell.owner_decision = selection_invalid_ordinal;
      cell.multiplicity = selection_invalid_ordinal;
      cell.cancelled = false;
      cell.internal = false;
      cell.distinct_occurrences = proto.distinct_occurrences;

      for (const auto a : member_atoms) {
        coincidence_member_record member;
        member.canonical_id = artifact.coincidence_members_.size();
        member.sheet_cell = cell.canonical_id;
        member.atom = a;
        member.source_operand = classification_->atoms()[a].operand;
        member.source_facet = classification_->atoms()[a].source_facet;
        artifact.coincidence_members_.push_back(std::move(member));
        artifact.cell_members_.push_back(member.canonical_id);
        const std::uint64_t d = artifact.disposition_by_atom_[a];
        artifact.dispositions_[d].sheet_cell = cell.canonical_id;
      }

      if (!resolve_sheet_cell(artifact, cell, member_atoms, error))
        return false;
      artifact.sheet_cells_.push_back(std::move(cell));
    }
    return true;
  }

  bool resolve_sheet_cell(
      artifact_type &artifact, coincidence_sheet_cell_record &cell,
      const std::vector<std::uint64_t> &members,
      bounded_boolean_error &error) {
    // Joint decision: determine the required result boundary sheet(s).
    // Collect each member's individual truth result.
    struct member_truth {
      std::uint64_t atom;
      bool retain;
      boundary_orientation orientation;
    };
    std::vector<member_truth> truths;
    for (const auto a : members) {
      const std::uint64_t d = artifact.disposition_by_atom_[a];
      const auto &truth = artifact.truth_records_[artifact.dispositions_[d].truth];
      truths.push_back({a, truth.retain, truth.orientation});
    }

    // Opposite-orientation coincident sheets bound an internal face: the result
    // occupancy is equal on both sides, so regularization suppresses them.
    const bool any_retain =
        std::any_of(truths.begin(), truths.end(),
                    [](const member_truth &m) { return m.retain; });
    if (cell.opposite_orientation && !cell.distinct_occurrences) {
      cell.internal = true;
      for (const auto a : members) {
        const std::uint64_t d = artifact.disposition_by_atom_[a];
        artifact.dispositions_[d].disposition = final_disposition::suppress_internal;
        suppression_record suppression;
        suppression.canonical_id = artifact.suppressions_.size();
        suppression.atom = a;
        suppression.reason = final_disposition::suppress_internal;
        suppression.owner_atom = selection_invalid_ordinal;
        artifact.suppressions_.push_back(std::move(suppression));
      }
      return true;
    }

    // Same-orientation coincidence with no result transition: cancelled.
    if (!any_retain) {
      cell.cancelled = true;
      for (const auto a : members) {
        const std::uint64_t d = artifact.disposition_by_atom_[a];
        artifact.dispositions_[d].disposition = final_disposition::cancel_coincident;
        suppression_record suppression;
        suppression.canonical_id = artifact.suppressions_.size();
        suppression.atom = a;
        suppression.reason = final_disposition::cancel_coincident;
        suppression.owner_atom = selection_invalid_ordinal;
        artifact.suppressions_.push_back(std::move(suppression));
      }
      return true;
    }

    // One required boundary sheet: select exactly one canonical eligible owner.
    const bool required_orientation_reverse =
        std::all_of(truths.begin(), truths.end(),
                    [](const member_truth &m) {
                      return !m.retain || m.orientation == boundary_orientation::reverse;
                    });
    selection_coincidence::owner_rank best;
    std::uint64_t best_atom = selection_invalid_ordinal;
    bool have_best = false;
    for (const auto &m : truths) {
      if (!m.retain)
        continue;
      const auto &atom = classification_->atoms()[m.atom];
      const bool can_realize = required_orientation_reverse
                                   ? (m.orientation == boundary_orientation::reverse)
                                   : (m.orientation == boundary_orientation::preserve);
      const auto rank = selection_coincidence::compute_owner_rank(
          context_.operation, atom.operand, atom.source_facet, m.atom,
          can_realize, 0);
      if (!have_best || rank < best) {
        best = rank;
        best_atom = m.atom;
        have_best = true;
      }
    }
    if (!have_best) {
      error = selection_error(selection_subcode::no_owner,
                              bounded_boolean_error_category::internal_invariant_error,
                              "coincident sheet cell has no eligible owner",
                              selection_checkpoint::owner_resolution);
      return false;
    }

    owner_decision_record decision;
    decision.canonical_id = artifact.owner_decisions_.size();
    decision.sheet_cell = cell.canonical_id;
    decision.owner_atom = best_atom;
    decision.owner_operand = classification_->atoms()[best_atom].operand;
    decision.rank_components = {{
        static_cast<std::uint64_t>(best.orientation_capability),
        static_cast<std::uint64_t>(best.operand_priority),
        static_cast<std::uint64_t>(best.feature_priority),
        best.source_feature, best.atom_ordinal, 0,
    }};
    decision.symbolic_rule_ordinal = 0;
    cell.owner_atom = best_atom;
    cell.owner_decision = decision.canonical_id;

    // The owner is retained; every other member is a suppressed non-owner.
    for (const auto a : members) {
      const std::uint64_t d = artifact.disposition_by_atom_[a];
      if (a == best_atom)
        continue;
      artifact.dispositions_[d].disposition = final_disposition::suppress_non_owner;
      suppression_record suppression;
      suppression.canonical_id = artifact.suppressions_.size();
      suppression.atom = a;
      suppression.reason = final_disposition::suppress_non_owner;
      suppression.owner_atom = best_atom;
      artifact.suppressions_.push_back(std::move(suppression));
    }
    artifact.owner_decisions_.push_back(std::move(decision));
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: retained uses.
  // -------------------------------------------------------------------------
  bool build_retained_uses(artifact_type &artifact, bounded_boolean_error &error) {
    (void)error;
    artifact.retained_use_by_atom_.resize(classification_->atoms().size(),
                                          selection_invalid_ordinal);
    for (auto &disposition : artifact.dispositions_) {
      const bool retained =
          disposition.disposition == final_disposition::retain_preserve ||
          disposition.disposition == final_disposition::retain_reverse;
      if (!retained)
        continue;
      const auto &atom = classification_->atoms()[disposition.atom];

      retained_surface_use_record use;
      use.canonical_id = artifact.retained_uses_.size();
      use.atom = disposition.atom;
      use.source_operand = atom.operand;
      use.source_shell = atom.shell;
      use.source_facet = atom.source_facet;
      use.preserve_source_orientation =
          disposition.disposition == final_disposition::retain_preserve;
      const auto &truth = artifact.truth_records_[disposition.truth];
      use.result_occupancy = disposition.key.occupancy;
      use.sheet_owner_lineage =
          disposition.sheet_cell == selection_invalid_ordinal
              ? 0
              : artifact.sheet_cells_[disposition.sheet_cell].support_lineage;
      use.multiplicity_occurrence = 0;
      use.separation = occurrence_separation_class::ordinary;
      use.separation_lineage = 0;
      use.provenance = selection_invalid_ordinal;
      (void)truth;

      disposition.retained_use = use.canonical_id;
      artifact.retained_use_by_atom_[disposition.atom] = use.canonical_id;

      retained_use_provenance_record provenance;
      provenance.canonical_id = artifact.retained_provenance_.size();
      provenance.retained_use = use.canonical_id;
      provenance.atom = disposition.atom;
      provenance.source_operand = atom.operand;
      provenance.source_shell = atom.shell;
      provenance.source_facet = atom.source_facet;
      provenance.source_triangle =
          atom.source_triangles.empty() ? selection_invalid_ordinal
                                        : atom.source_triangles.front();
      use.provenance = provenance.canonical_id;
      artifact.retained_provenance_.push_back(std::move(provenance));
      artifact.retained_uses_.push_back(std::move(use));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: boundary reconstruction and retained incidences.
  // -------------------------------------------------------------------------
  bool build_incidences(artifact_type &artifact, bounded_boolean_error &error) {
    // Reconstruct each retained atom's ordered boundary ring from the manifold
    // facet group. Source edges are split at the intersection complex's event
    // clusters. Endpoint domains are source vertices or event occurrences.
    std::map<std::uint64_t, std::uint64_t> domain_by_source_vertex;
    std::map<std::uint64_t, std::uint64_t> domain_by_occurrence;
    auto intern_source_vertex = [&](operand_id operand, std::uint64_t source_vertex) {
      const std::uint64_t key = (static_cast<std::uint64_t>(operand) << 48) | source_vertex;
      auto it = domain_by_source_vertex.find(key);
      if (it != domain_by_source_vertex.end())
        return it->second;
      endpoint_domain_record domain;
      domain.canonical_id = artifact.endpoint_domains_.size();
      domain.kind = endpoint_domain_kind::source_vertex;
      domain.source_operand = operand;
      domain.lineage = source_vertex;
      domain.event_occurrence = selection_invalid_ordinal;
      artifact.endpoint_domains_.push_back(domain);
      domain_by_source_vertex.emplace(key, domain.canonical_id);
      return domain.canonical_id;
    };
    auto intern_occurrence = [&](operand_id operand, std::uint64_t occurrence) {
      const std::uint64_t key = (static_cast<std::uint64_t>(operand) << 48) | occurrence;
      auto it = domain_by_occurrence.find(key);
      if (it != domain_by_occurrence.end())
        return it->second;
      endpoint_domain_record domain;
      domain.canonical_id = artifact.endpoint_domains_.size();
      domain.kind = endpoint_domain_kind::event_occurrence;
      domain.source_operand = operand;
      domain.lineage = occurrence;
      domain.event_occurrence = occurrence;
      artifact.endpoint_domains_.push_back(domain);
      domain_by_occurrence.emplace(key, domain.canonical_id);
      return domain.canonical_id;
    };

    // Per-(operand, source edge) ordered event occurrences.
    std::map<std::pair<operand_id, std::uint64_t>, std::vector<std::uint64_t>>
        edge_clusters;
    for (const auto &sequence : intersections_->source_edge_sequences()) {
      const auto &edge = sequence.source_edge;
      std::vector<std::uint64_t> occurrences;
      for (std::uint64_t k = sequence.clusters.begin;
           k < sequence.clusters.begin + sequence.clusters.count; ++k) {
        const auto cluster_id =
            intersections_->source_edge_sequence_cluster_index()[k];
        const auto &cluster = intersections_->source_edge_clusters()[cluster_id.ordinal()];
        for (std::uint64_t m = cluster.member_occurrences.begin;
             m < cluster.member_occurrences.begin + cluster.member_occurrences.count; ++m) {
          const auto occurrence =
              intersections_->source_edge_cluster_occurrence_index()[m];
          occurrences.push_back(occurrence.ordinal());
        }
      }
      edge_clusters[std::make_pair(edge.operand, edge.primary)] =
          std::move(occurrences);
    }

    for (const auto &use : artifact.retained_uses_) {
      const auto &manifold = operand_manifold(use.source_operand);
      const auto &facet_groups = manifold.facet_groups();
      if (use.source_facet >= manifold.source_facet_to_group().size()) {
        error = selection_error(selection_subcode::incidence_carrier_error,
                                bounded_boolean_error_category::internal_invariant_error,
                                "retained atom source facet is out of range",
                                selection_checkpoint::incidence_normalization);
        return false;
      }
      const auto group = manifold.source_facet_to_group()[use.source_facet];
      if (group >= facet_groups.size() || facet_groups[group].boundary_halfedges.empty()) {
        error = selection_error(selection_subcode::incidence_carrier_error,
                                bounded_boolean_error_category::internal_invariant_error,
                                "retained atom facet group has no boundary",
                                selection_checkpoint::incidence_normalization);
        return false;
      }
      const auto &facet_group = facet_groups[group];

      // Walk the facet boundary ring in source orientation.
      std::vector<std::uint64_t> ring_source_vertices;
      std::vector<std::uint64_t> ring_source_edges;
      std::map<std::uint64_t, std::uint64_t> origin_to_halfedge;
      for (const auto he : facet_group.boundary_halfedges)
        origin_to_halfedge[manifold.halfedges()[he].origin] = he;
      const std::uint64_t start_he = facet_group.boundary_halfedges.front();
      std::uint64_t he = start_he;
      std::uint64_t guard = 0;
      do {
        if (he >= manifold.halfedges().size() ||
            guard++ > facet_group.boundary_halfedges.size()) {
          error = selection_error(selection_subcode::incidence_direction_error,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "facet ring traversal failed",
                                  selection_checkpoint::incidence_normalization);
          return false;
        }
        const auto &halfedge = manifold.halfedges()[he];
        const auto &vertex = manifold.vertices()[halfedge.origin];
        ring_source_vertices.push_back(vertex.source_vertex);
        ring_source_edges.push_back(halfedge.source_undirected_edge);
        const auto next = origin_to_halfedge.find(halfedge.destination);
        if (next == origin_to_halfedge.end())
          break;
        he = next->second;
      } while (he != start_he);

      const std::size_t ring_count = ring_source_vertices.size();
      if (ring_count < 3) {
        error = selection_error(selection_subcode::incidence_direction_error,
                                bounded_boolean_error_category::internal_invariant_error,
                                "retained atom facet ring is degenerate",
                                selection_checkpoint::incidence_normalization);
        return false;
      }

      const bool reversed = !use.preserve_source_orientation;
      for (std::size_t i = 0; i < ring_count; ++i) {
        // Source orientation: edge i runs vertices[i] -> vertices[i+1].
        const std::uint64_t forward_start = ring_source_vertices[i];
        const std::uint64_t forward_end = ring_source_vertices[(i + 1) % ring_count];
        const std::uint64_t start_sv = reversed ? forward_end : forward_start;
        const std::uint64_t end_sv = reversed ? forward_start : forward_end;
        const std::uint64_t source_edge = ring_source_edges[i];

        // Ordered point sequence along this edge (start -> end).
        std::vector<std::uint64_t> points;
        points.push_back(start_sv);
        auto clusters_it = edge_clusters.find(
            std::make_pair(use.source_operand, source_edge));
        if (clusters_it != edge_clusters.end())
          for (const auto occurrence : clusters_it->second)
            points.push_back(occurrence);
        points.push_back(end_sv);

        for (std::size_t s = 0; s + 1 < points.size(); ++s) {
          const std::uint64_t p0 = points[s];
          const std::uint64_t p1 = points[s + 1];
          const std::uint64_t domain0 =
              s == 0 ? intern_source_vertex(use.source_operand, p0)
                     : intern_occurrence(use.source_operand, p0);
          const std::uint64_t domain1 =
              s + 2 == points.size()
                  ? intern_source_vertex(use.source_operand, p1)
                  : intern_occurrence(use.source_operand, p1);

          retained_incidence_record incidence;
          incidence.canonical_id = artifact.incidences_.size();
          incidence.retained_use = use.canonical_id;
          incidence.atom = use.atom;
          incidence.start_domain = domain0;
          incidence.end_domain = domain1;
          incidence.carrier.kind = carrier_kind::whole_source_edge;
          incidence.carrier.source_operand = use.source_operand;
          incidence.carrier.lineage = source_edge;
          incidence.direction = direction_role::forward;
          incidence.start_sector_lineage = domain0;
          incidence.end_sector_lineage = domain1;
          incidence.descriptor.source_operand = use.source_operand;
          incidence.descriptor.source_shell = use.source_shell;
          incidence.descriptor.source_facet = use.source_facet;
          incidence.descriptor.sheet_owner_lineage = use.sheet_owner_lineage;
          incidence.descriptor.retained_use_lineage = use.canonical_id;
          incidence.descriptor.multiplicity_occurrence =
              static_cast<std::uint32_t>(use.multiplicity_occurrence);
          incidence.descriptor.result_side_transition =
              use.preserve_source_orientation ? static_cast<std::int8_t>(-1)
                                              : static_cast<std::int8_t>(1);
          incidence.descriptor.start_sector_lineage = domain0;
          incidence.descriptor.end_sector_lineage = domain1;
          incidence.descriptor.separation = use.separation;
          incidence.descriptor.separation_lineage = use.separation_lineage;
          incidence.expected_opposite = incidence.descriptor;
          incidence.disposition = incidence_disposition::planned_edge;
          incidence.continuation = selection_invalid_ordinal;
          incidence.planned_edge = selection_invalid_ordinal;
          incidence.carrier_balance = selection_invalid_ordinal;
          artifact.incidences_.push_back(std::move(incidence));
        }
      }
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: edge occurrences.
  // -------------------------------------------------------------------------
  bool build_edge_occurrences(artifact_type &artifact, bounded_boolean_error &error) {
    // Group directed incidences by carrier identity and endpoint pair. A valid
    // planned edge has exactly two uses with reversed endpoints and reciprocal
    // surface-occurrence descriptors.
    std::map<selection_carrier_identity, std::vector<std::uint64_t>> by_carrier;
    for (std::size_t i = 0; i < artifact.incidences_.size(); ++i)
      by_carrier[artifact.incidences_[i].carrier].push_back(i);

    for (const auto &entry : by_carrier) {
      const auto &members = entry.second;
      std::map<std::pair<std::uint64_t, std::uint64_t>, std::vector<std::uint64_t>>
          by_endpoints;
      for (const auto index : members) {
        const auto &incidence = artifact.incidences_[index];
        const std::uint64_t lo = std::min(incidence.start_domain, incidence.end_domain);
        const std::uint64_t hi = std::max(incidence.start_domain, incidence.end_domain);
        by_endpoints[std::make_pair(lo, hi)].push_back(index);
      }
      for (const auto &endpoint_group : by_endpoints) {
        const auto &group = endpoint_group.second;
        if (group.size() != 2) {
          error = selection_error(selection_subcode::mate_cardinality_error,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "carrier endpoint group does not have two uses",
                                  selection_checkpoint::edge_pairing);
          return false;
        }
        auto &a = artifact.incidences_[group[0]];
        auto &b = artifact.incidences_[group[1]];

        // Reciprocal descriptors.
        a.expected_opposite = b.descriptor;
        b.expected_opposite = a.descriptor;

        // Determine forward/reverse by endpoint reversal. The two retained
        // faces traverse the edge in opposite directions.
        const bool a_forward = a.start_domain == b.end_domain && a.end_domain == b.start_domain;
        const bool b_forward = b.start_domain == a.end_domain && b.end_domain == a.start_domain;
        if (!a_forward && !b_forward) {
          error = selection_error(selection_subcode::mate_endpoint_incompatibility,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "carrier endpoints are not reversed",
                                  selection_checkpoint::edge_pairing);
          return false;
        }
        edge_mate_group_record mate_group;
        mate_group.canonical_id = artifact.edge_mate_groups_.size();
        if (a_forward) {
          a.direction = direction_role::forward;
          b.direction = direction_role::reverse;
          mate_group.forward_incidence = group[0];
          mate_group.reverse_incidence = group[1];
        } else {
          a.direction = direction_role::reverse;
          b.direction = direction_role::forward;
          mate_group.forward_incidence = group[1];
          mate_group.reverse_incidence = group[0];
        }

        planned_edge_occurrence_record planned;
        planned.canonical_id = artifact.planned_edges_.size();
        planned.mate_group = mate_group.canonical_id;
        planned.start_domain = artifact.incidences_[mate_group.forward_incidence].start_domain;
        planned.end_domain = artifact.incidences_[mate_group.forward_incidence].end_domain;
        planned.expected_pair = surface_occurrence_descriptor_pair::ordered(
            a.descriptor, b.descriptor);
        planned.cross_operand = a.descriptor.source_operand != b.descriptor.source_operand;
        planned.cross_owner = a.descriptor.sheet_owner_lineage != b.descriptor.sheet_owner_lineage;

        a.planned_edge = planned.canonical_id;
        b.planned_edge = planned.canonical_id;
        artifact.edge_mate_groups_.push_back(std::move(mate_group));
        artifact.planned_edges_.push_back(std::move(planned));
      }
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: vertex occurrences via local link cycles.
  // -------------------------------------------------------------------------
  bool build_vertex_occurrences(artifact_type &artifact, bounded_boolean_error &error) {
    // One local port per planned-edge endpoint, keyed by (endpoint domain,
    // incidence). Every port has exactly one corner arc and one mate arc.
    std::vector<local_port_record> ports;
    std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t>
        port_index; // (domain, incidence) -> port ordinal

    for (const auto &group : artifact.edge_mate_groups_) {
      const auto &f = artifact.incidences_[group.forward_incidence];
      const auto &r = artifact.incidences_[group.reverse_incidence];
      const std::array<std::pair<std::uint64_t, std::uint64_t>, 4> keys{{
          {f.start_domain, f.canonical_id}, {f.end_domain, f.canonical_id},
          {r.start_domain, r.canonical_id}, {r.end_domain, r.canonical_id},
      }};
      std::array<std::uint64_t, 4> ids{};
      for (std::size_t k = 0; k < 4; ++k) {
        auto it = port_index.find(keys[k]);
        if (it != port_index.end()) {
          ids[k] = it->second;
          continue;
        }
        local_port_record port;
        port.canonical_id = ports.size();
        port.endpoint_domain = keys[k].first;
        port.incidence = keys[k].second;
        port.retained_use = artifact.incidences_[keys[k].second].retained_use;
        port.role = (k == 0 || k == 2) ? endpoint_role::start : endpoint_role::end;
        ports.push_back(port);
        port_index.emplace(keys[k], port.canonical_id);
        ids[k] = port.canonical_id;
      }
      // Edge-mate arcs: f-start <-> r-end, f-end <-> r-start.
      edge_mate_arc_record mate1;
      mate1.canonical_id = artifact.edge_mate_arcs_.size();
      mate1.first_port = ids[0];
      mate1.second_port = ids[3];
      mate1.planned_edge = group.canonical_id;
      artifact.edge_mate_arcs_.push_back(std::move(mate1));
      edge_mate_arc_record mate2;
      mate2.canonical_id = artifact.edge_mate_arcs_.size();
      mate2.first_port = ids[1];
      mate2.second_port = ids[2];
      mate2.planned_edge = group.canonical_id;
      artifact.edge_mate_arcs_.push_back(std::move(mate2));
    }

    // Bind mate arcs onto ports.
    for (const auto &arc : artifact.edge_mate_arcs_) {
      ports[arc.first_port].edge_mate_arc = arc.canonical_id;
      ports[arc.second_port].edge_mate_arc = arc.canonical_id;
    }

    // Face-corner arcs: connect consecutive incidences around each retained
    // use's boundary ring (end of one to start of the next at the same domain).
    std::map<std::uint64_t, std::vector<std::uint64_t>> incidences_by_use;
    for (std::size_t i = 0; i < artifact.incidences_.size(); ++i)
      if (artifact.incidences_[i].disposition == incidence_disposition::planned_edge)
        incidences_by_use[artifact.incidences_[i].retained_use].push_back(i);

    for (auto &entry : incidences_by_use) {
      auto &members = entry.second;
      std::map<std::uint64_t, std::uint64_t> by_start;
      for (const auto index : members)
        by_start[artifact.incidences_[index].start_domain] = index;
      std::vector<std::uint64_t> ordered;
      ordered.reserve(members.size());
      std::uint64_t current = members.front();
      for (std::size_t k = 0; k < members.size(); ++k) {
        ordered.push_back(current);
        const auto next = by_start.find(artifact.incidences_[current].end_domain);
        if (next == by_start.end())
          break;
        current = next->second;
      }
      if (ordered.size() != members.size()) {
        error = selection_error(selection_subcode::open_local_link,
                                bounded_boolean_error_category::internal_invariant_error,
                                "retained use boundary is not a closed ring",
                                selection_checkpoint::local_port_construction);
        return false;
      }
      for (std::size_t k = 0; k < ordered.size(); ++k) {
        const auto in_index = ordered[k];
        const auto out_index = ordered[(k + 1) % ordered.size()];
        const auto &in_inc = artifact.incidences_[in_index];
        const auto &out_inc = artifact.incidences_[out_index];
        const auto in_port = port_index.find(
            std::make_pair(in_inc.end_domain, in_index));
        const auto out_port = port_index.find(
            std::make_pair(out_inc.start_domain, out_index));
        if (in_port == port_index.end() || out_port == port_index.end()) {
          error = selection_error(selection_subcode::malformed_local_port,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "corner port is missing",
                                  selection_checkpoint::local_port_construction);
          return false;
        }
        face_corner_arc_record corner;
        corner.canonical_id = artifact.face_corner_arcs_.size();
        corner.in_port = in_port->second;
        corner.out_port = out_port->second;
        corner.retained_use = in_inc.retained_use;
        ports[corner.in_port].face_corner_arc = corner.canonical_id;
        ports[corner.out_port].face_corner_arc = corner.canonical_id;
        artifact.face_corner_arcs_.push_back(std::move(corner));
      }
    }

    for (auto &port : ports)
      artifact.local_ports_.push_back(std::move(port));

    // Extract closed alternating cycles (vertex occurrences).
    std::vector<selection_vertex_occurrences::link_port> link_ports;
    std::vector<selection_vertex_occurrences::link_arc> corners;
    std::vector<selection_vertex_occurrences::link_arc> mates;
    link_ports.reserve(artifact.local_ports_.size());
    for (const auto &port : artifact.local_ports_) {
      selection_vertex_occurrences::link_port p;
      p.corner_arc = port.face_corner_arc;
      p.mate_arc = port.edge_mate_arc;
      link_ports.push_back(p);
    }
    for (const auto &arc : artifact.face_corner_arcs_)
      corners.push_back({arc.in_port, arc.out_port});
    for (const auto &arc : artifact.edge_mate_arcs_)
      mates.push_back({arc.first_port, arc.second_port});
    std::vector<selection_vertex_occurrences::link_cycle> cycles;
    const auto status = selection_vertex_occurrences::extract_link_cycles(
        link_ports, corners, mates, cycles);
    if (status != selection_vertex_occurrences::link_status::success) {
      error = selection_error(selection_subcode::open_local_link,
                              bounded_boolean_error_category::internal_invariant_error,
                              "local link cycle extraction failed",
                              selection_checkpoint::local_link_cycle_extraction);
      return false;
    }

    for (const auto &cycle : cycles) {
      vertex_occurrence_requirement_record occurrence;
      occurrence.canonical_id = artifact.vertex_occurrences_.size();
      occurrence.cycle_begin = artifact.link_cycle_ports_.size();
      occurrence.cycle_count = cycle.ports.size();
      occurrence.representative_port = cycle.ports.front();
      occurrence.endpoint_domain = artifact.local_ports_[cycle.ports.front()].endpoint_domain;
      for (const auto port : cycle.ports)
        artifact.link_cycle_ports_.push_back(port);

      local_link_component_record component;
      component.canonical_id = artifact.link_components_.size();
      component.endpoint_domain = occurrence.endpoint_domain;
      component.members_begin = occurrence.cycle_begin;
      component.members_count = occurrence.cycle_count;
      component.vertex_occurrence = occurrence.canonical_id;
      artifact.link_components_.push_back(std::move(component));
      artifact.vertex_occurrences_.push_back(std::move(occurrence));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: carrier balance, feasibility, codec, verification.
  // -------------------------------------------------------------------------
  bool finalize(artifact_type &artifact, bounded_boolean_error &error) {
    // Carrier balance records.
    std::map<selection_carrier_identity, std::vector<std::uint64_t>> by_carrier;
    for (std::size_t i = 0; i < artifact.incidences_.size(); ++i)
      by_carrier[artifact.incidences_[i].carrier].push_back(i);
    for (const auto &entry : by_carrier) {
      carrier_balance_record balance;
      balance.canonical_id = artifact.carrier_balances_.size();
      balance.carrier = entry.first;
      balance.members_begin = artifact.balance_members_.size();
      balance.members_count = entry.second.size();
      balance.balanced = true;
      for (const auto member : entry.second)
        artifact.balance_members_.push_back(member);
      artifact.carrier_balances_.push_back(std::move(balance));
    }

    // Positive-area atom count for the feasibility audit.
    std::uint64_t positive_atom_count = 0;
    for (const auto &atom : classification_->atoms())
      if (atom.positive_area)
        ++positive_atom_count;

    // Producer feasibility audit.
    bounded_boolean_error audit_error;
    if (!audit_retained_feasibility(artifact, positive_atom_count, audit_error)) {
      error = audit_error;
      return false;
    }

    // Statistics (must be populated before encoding, since the codec serializes
    // the statistics section).
    artifact.statistics_.atom_count = classification_->atoms().size();
    artifact.statistics_.disposition_count = artifact.dispositions_.size();
    artifact.statistics_.side_tuple_count = artifact.side_tuples_.size();
    artifact.statistics_.sheet_cell_count = artifact.sheet_cells_.size();
    artifact.statistics_.member_count = artifact.coincidence_members_.size();
    artifact.statistics_.owner_decision_count = artifact.owner_decisions_.size();
    artifact.statistics_.suppression_count = artifact.suppressions_.size();
    artifact.statistics_.multiplicity_count = artifact.multiplicities_.size();
    artifact.statistics_.retained_use_count = artifact.retained_uses_.size();
    artifact.statistics_.incidence_count = artifact.incidences_.size();
    artifact.statistics_.continuation_count = artifact.continuations_.size();
    artifact.statistics_.edge_occurrence_count = artifact.planned_edges_.size();
    artifact.statistics_.carrier_balance_count = artifact.carrier_balances_.size();
    artifact.statistics_.endpoint_domain_count = artifact.endpoint_domains_.size();
    artifact.statistics_.local_port_count = artifact.local_ports_.size();
    artifact.statistics_.face_corner_arc_count = artifact.face_corner_arcs_.size();
    artifact.statistics_.edge_mate_arc_count = artifact.edge_mate_arcs_.size();
    artifact.statistics_.link_component_count = artifact.link_components_.size();
    artifact.statistics_.vertex_occurrence_count = artifact.vertex_occurrences_.size();
    artifact.statistics_.persistent_bytes = artifact.canonical_bytes_.size();
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();

    // Canonical encoding and digest.
    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_retained_surface_complex(artifact, bytes, codec_limits_, codec_error)) {
      error = codec_error;
      return false;
    }
    artifact.canonical_bytes_ = std::move(bytes);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
    artifact.statistics_.persistent_bytes = artifact.canonical_bytes_.size();
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();

    // Independent verification.
    if (selection_cancelled(capabilities_,
                            selection_checkpoint::independent_verification))
      return true;
    bounded_boolean_error verification_error;
    if (!verify_retained_surface_complex(
            artifact, context_, *manifolds_, *relations_, *intersections_,
            *classification_, verification_error)) {
      error = verification_error;
      return false;
    }
    artifact.verification_ =
        selection_verification_disposition::independently_verified;
    return true;
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const retained_surface_complex<T, I>>>
build_retained_surface_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const classification_complex<T, I>> classification,
    selection_capabilities capabilities, selection_codec_limits codec_limits) {
  try {
    selection_builder<T, I> builder(context, precision, std::move(manifolds),
                                    std::move(relations), std::move(intersections),
                                    std::move(classification),
                                    std::move(capabilities), codec_limits);
    return builder.run();
  } catch (const std::bad_alloc &) {
    return boolean_outcome<std::shared_ptr<const retained_surface_complex<T, I>>>::failure(
        selection_error(selection_subcode::resource_preflight,
                        bounded_boolean_error_category::resource_limit,
                        "selection allocation failed",
                        selection_checkpoint::count_preflight));
  } catch (...) {
    return boolean_outcome<std::shared_ptr<const retained_surface_complex<T, I>>>::failure(
        selection_error(selection_subcode::internal_invariant,
                        bounded_boolean_error_category::internal_invariant_error,
                        "selection unexpected exception",
                        selection_checkpoint::commit));
  }
}

} // namespace ygor::mesh_boolean::bounded
