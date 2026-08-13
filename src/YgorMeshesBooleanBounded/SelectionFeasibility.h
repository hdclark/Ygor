#pragma once

#include "RetainedSurfaceComplex.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Component 10 complete balance/manifold-feasibility audit. It proves that the
// retained complex is combinatorially capable of forming paired manifold
// output before Component 11 consumes it. It never re-derives selection; it
// audits the published disposition, incidence, edge-occurrence, and
// vertex-occurrence tables for completeness and reciprocity.
template <class T, class I>
bool audit_retained_feasibility(const retained_surface_complex<T, I> &artifact,
                                std::uint64_t positive_area_atom_count,
                                bounded_boolean_error &error) {
  const auto fail = [&](selection_subcode subcode, const char *summary,
                        selection_checkpoint checkpoint) {
    error = selection_error(subcode,
                            bounded_boolean_error_category::internal_invariant_error,
                            summary, checkpoint);
    return false;
  };

  const std::uint64_t atom_count = positive_area_atom_count;
  const std::uint64_t disposition_count = artifact.dispositions().size();
  const std::uint64_t incidence_count = artifact.incidences().size();

  // (1) Exactly one disposition per positive-area atom, dense and in range.
  // The reverse map is sized to the full atom domain; non-positive atoms hold
  // the invalid sentinel.
  if (disposition_count != atom_count)
    return fail(selection_subcode::missing_atom,
                "disposition count does not match the positive-area atom domain",
                selection_checkpoint::disposition_audit);
  std::vector<bool> disposition_seen(disposition_count, false);
  std::uint64_t valid_mapping_count = 0;
  for (std::uint64_t a = 0; a < artifact.disposition_by_atom().size(); ++a) {
    const std::uint64_t d = artifact.disposition_by_atom()[a];
    if (d == selection_invalid_ordinal)
      continue;
    if (d >= disposition_count)
      return fail(selection_subcode::missing_atom,
                  "atom disposition ordinal is out of range",
                  selection_checkpoint::disposition_audit);
    if (disposition_seen[d])
      return fail(selection_subcode::duplicate_atom,
                  "two atoms share one disposition",
                  selection_checkpoint::disposition_audit);
    disposition_seen[d] = true;
    ++valid_mapping_count;
    const auto &disposition = artifact.dispositions()[d];
    if (disposition.atom != a)
      return fail(selection_subcode::reverse_map_error,
                  "disposition does not map back to its atom",
                  selection_checkpoint::disposition_audit);
  }
  if (valid_mapping_count != atom_count)
    return fail(selection_subcode::missing_atom,
                "disposition reverse map does not cover the positive-area atoms",
                selection_checkpoint::disposition_audit);

  // (2) Retained uses are exactly the retained dispositions.
  const std::uint64_t retained_count = artifact.retained_uses().size();
  std::vector<bool> use_seen(retained_count, false);
  for (const auto &disposition : artifact.dispositions()) {
    const bool retained = disposition.disposition == final_disposition::retain_preserve ||
                          disposition.disposition == final_disposition::retain_reverse ||
                          disposition.disposition == final_disposition::represented_by_multiplicity;
    if (retained) {
      if (disposition.retained_use >= retained_count)
        return fail(selection_subcode::retained_use_provenance_mismatch,
                    "retained disposition has no retained use",
                    selection_checkpoint::retained_use_construction);
      if (use_seen[disposition.retained_use])
        return fail(selection_subcode::retained_use_provenance_mismatch,
                    "two dispositions share one retained use",
                    selection_checkpoint::retained_use_construction);
      use_seen[disposition.retained_use] = true;
    } else if (disposition.retained_use != selection_invalid_ordinal) {
      return fail(selection_subcode::retained_use_provenance_mismatch,
                  "discarded disposition owns a retained use",
                  selection_checkpoint::retained_use_construction);
    }
  }
  for (std::uint64_t u = 0; u < retained_count; ++u) {
    if (!use_seen[u])
      return fail(selection_subcode::retained_use_provenance_mismatch,
                  "retained use has no disposition",
                  selection_checkpoint::retained_use_construction);
    const auto &use = artifact.retained_uses()[u];
    if (use.atom >= atom_count)
      return fail(selection_subcode::retained_use_provenance_mismatch,
                  "retained use references an out-of-range atom",
                  selection_checkpoint::retained_use_construction);
  }

  // (3) Continuation reciprocity.
  const std::uint64_t continuation_count = artifact.continuations().size();
  std::vector<bool> continuation_seen(continuation_count, false);
  for (const auto &continuation : artifact.continuations()) {
    if (continuation.forward_incidence >= incidence_count ||
        continuation.reverse_incidence >= incidence_count)
      return fail(selection_subcode::invalid_continuation,
                  "continuation references an out-of-range incidence",
                  selection_checkpoint::continuation_construction);
    const auto &forward = artifact.incidences()[continuation.forward_incidence];
    const auto &reverse = artifact.incidences()[continuation.reverse_incidence];
    if (forward.disposition != incidence_disposition::transparent_continuation ||
        reverse.disposition != incidence_disposition::transparent_continuation)
      return fail(selection_subcode::invalid_continuation,
                  "continuation members are not continuation incidences",
                  selection_checkpoint::continuation_construction);
    if (forward.continuation != continuation.canonical_id ||
        reverse.continuation != continuation.canonical_id)
      return fail(selection_subcode::nonreciprocal_continuation,
                  "continuation is not reciprocal",
                  selection_checkpoint::continuation_construction);
    continuation_seen[continuation.canonical_id] = true;
  }
  for (const auto &incidence : artifact.incidences()) {
    if (incidence.disposition == incidence_disposition::transparent_continuation) {
      if (incidence.continuation >= continuation_count)
        return fail(selection_subcode::invalid_continuation,
                    "continuation incidence references an invalid record",
                    selection_checkpoint::continuation_construction);
    }
  }

  // (4) Planned edges: every edge occurrence is a two-member reciprocal pair.
  const std::uint64_t planned_count = artifact.planned_edges().size();
  const std::uint64_t mate_group_count = artifact.edge_mate_groups().size();
  if (planned_count != mate_group_count)
    return fail(selection_subcode::mate_cardinality_error,
                "planned-edge count does not match mate-group count",
                selection_checkpoint::edge_pairing);
  for (const auto &group : artifact.edge_mate_groups()) {
    if (group.forward_incidence >= incidence_count ||
        group.reverse_incidence >= incidence_count)
      return fail(selection_subcode::mate_cardinality_error,
                  "mate group references an out-of-range incidence",
                  selection_checkpoint::edge_pairing);
    const auto &forward = artifact.incidences()[group.forward_incidence];
    const auto &reverse = artifact.incidences()[group.reverse_incidence];
    if (forward.direction != direction_role::forward ||
        reverse.direction != direction_role::reverse)
      return fail(selection_subcode::mate_direction_error,
                  "mate group directions are not forward/reverse",
                  selection_checkpoint::edge_pairing);
    if (forward.start_domain != reverse.end_domain ||
        forward.end_domain != reverse.start_domain)
      return fail(selection_subcode::mate_endpoint_incompatibility,
                  "mate group endpoints are not reversed",
                  selection_checkpoint::edge_pairing);
    if (!(forward.descriptor == reverse.expected_opposite) ||
        !(reverse.descriptor == forward.expected_opposite))
      return fail(selection_subcode::nonreciprocal_expected_mate,
                  "mate group descriptors are not reciprocal",
                  selection_checkpoint::edge_pairing);
    if (forward.planned_edge != group.canonical_id ||
        reverse.planned_edge != group.canonical_id)
      return fail(selection_subcode::reverse_map_error,
                  "incidence does not map back to its planned edge",
                  selection_checkpoint::edge_pairing);
  }

  // (5) Incidences are consumed exactly once by their disposition.
  std::vector<bool> incidence_consumed(incidence_count, false);
  for (const auto &incidence : artifact.incidences()) {
    if (incidence.disposition == incidence_disposition::planned_edge) {
      if (incidence.planned_edge >= planned_count)
        return fail(selection_subcode::incidence_underconsumption,
                    "planned-edge incidence references an invalid edge",
                    selection_checkpoint::endpoint_remap);
      incidence_consumed[incidence.canonical_id] = true;
    } else if (incidence.disposition == incidence_disposition::transparent_continuation) {
      incidence_consumed[incidence.canonical_id] = true;
    } else if (incidence.disposition == incidence_disposition::zero_measure_support ||
               incidence.disposition == incidence_disposition::suppressed_audit_only ||
               incidence.disposition == incidence_disposition::consumed_owner_seam ||
               incidence.disposition == incidence_disposition::topology_separation_delimiter) {
      // Permitted audit-only dispositions do not consume an output edge.
    } else {
      return fail(selection_subcode::incidence_underconsumption,
                  "incidence has an unknown disposition",
                  selection_checkpoint::incidence_normalization);
    }
  }

  // (6) Local ports, arcs, and vertex-occurrence cycles.
  const std::uint64_t port_count = artifact.local_ports().size();
  const std::uint64_t corner_count = artifact.face_corner_arcs().size();
  const std::uint64_t mate_count = artifact.edge_mate_arcs().size();
  if (port_count != 4 * planned_count)
    return fail(selection_subcode::malformed_local_port,
                "port count does not match planned-edge endpoints",
                selection_checkpoint::local_port_construction);
  if (corner_count != port_count / 2 || mate_count != port_count / 2)
    return fail(selection_subcode::malformed_arc,
                "arc counts do not match port count",
                selection_checkpoint::local_port_construction);

  std::vector<bool> corner_arc_seen(corner_count, false);
  std::vector<bool> mate_arc_seen(mate_count, false);
  for (const auto &port : artifact.local_ports()) {
    if (port.face_corner_arc >= corner_count || port.edge_mate_arc >= mate_count)
      return fail(selection_subcode::malformed_local_port,
                  "port arc ordinal is out of range",
                  selection_checkpoint::local_port_construction);
    corner_arc_seen[port.face_corner_arc] = true;
    mate_arc_seen[port.edge_mate_arc] = true;
  }
  for (const auto &seen : corner_arc_seen)
    if (!seen)
      return fail(selection_subcode::malformed_arc,
                  "face-corner arc has no port",
                  selection_checkpoint::local_port_construction);
  for (const auto &seen : mate_arc_seen)
    if (!seen)
      return fail(selection_subcode::malformed_arc,
                  "edge-mate arc has no port",
                  selection_checkpoint::local_port_construction);

  const std::uint64_t occurrence_count = artifact.vertex_occurrences().size();
  std::uint64_t cycle_port_total = 0;
  std::vector<bool> port_in_cycle(port_count, false);
  for (const auto &component : artifact.link_components()) {
    if (component.vertex_occurrence >= occurrence_count)
      return fail(selection_subcode::open_local_link,
                  "link component references an invalid vertex occurrence",
                  selection_checkpoint::local_link_cycle_extraction);
  }
  for (const auto &occurrence : artifact.vertex_occurrences()) {
    if (occurrence.cycle_begin + occurrence.cycle_count >
        artifact.link_cycle_ports().size())
      return fail(selection_subcode::open_local_link,
                  "vertex occurrence cycle range is out of bounds",
                  selection_checkpoint::local_link_cycle_extraction);
    for (std::uint64_t k = occurrence.cycle_begin;
         k < occurrence.cycle_begin + occurrence.cycle_count; ++k) {
      const std::uint64_t port = artifact.link_cycle_ports()[k];
      if (port >= port_count || port_in_cycle[port])
        return fail(selection_subcode::open_local_link,
                    "vertex occurrence cycle is not a partition",
                    selection_checkpoint::local_link_cycle_extraction);
      port_in_cycle[port] = true;
      ++cycle_port_total;
    }
  }
  if (cycle_port_total != port_count)
    return fail(selection_subcode::open_local_link,
                "vertex occurrence cycles do not cover all ports",
                selection_checkpoint::local_link_cycle_extraction);

  return true;
}

} // namespace ygor::mesh_boolean::bounded
