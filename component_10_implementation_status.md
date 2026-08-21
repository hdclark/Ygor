# Component 10: Boolean Selection and Occurrence Accounting — Implementation Status

## Status

Component 10 is implemented as the deterministic `side_truth_and_lineage_selection_v1`
provider. It consumes the verified immutable artifacts from Components 01-09 and
publishes one immutable `retained_surface_complex<T,I>` for Component 11. It
never allocates output vertices, halfedges, face cycles, triangles, cleanup
actions, or a public mesh.

## Frozen V1 providers

- `selection_provider`                 = `side_truth_and_lineage_selection_v1`
- `coincidence_provider`               = `symbolic_sheet_cell_ownership_v1`
- `retained_incidence_provider`        = `semantic_boundary_incidence_v1`
- `edge_occurrence_provider`           = `exact_mate_signature_pairs_v1`
- `vertex_occurrence_provider`         = `degree_two_local_link_cycles_v1`
- `feasibility_provider`               = `complete_retained_complex_audit_v1`

## Files

- `SelectionTypes.h` — closed enums, strong ID domains, canonical keys
  (disposition, sheet-cell, retained-use, surface-occurrence descriptor,
  edge-occurrence slot, local-port), artifact records, subcodes, cancellation,
  capabilities, statistics.
- `RetainedSurfaceComplex.h` — the immutable artifact with canonical contiguous
  tables, (begin,count) member ranges, and dense reverse maps.
- `Selection.h` / `Selection.cc` — typed entrypoint `build_retained_surface_complex`
  and the phase orchestration.
- `SelectionTruth.h` — side-tuple construction from Component 02 shell semantics
  and Component 09 side labels, plus exactly one Component 01 truth-table lookup
  per atom.
- `SelectionCoincidence.h` — sheet-cell reconstruction from Component 07-09
  coplanar lineage and the frozen Component 01 owner rank.
- `SelectionEdgeOccurrences.h` — exact two-member mate grouping with reciprocal
  surface-occurrence descriptors (no general matching).
- `SelectionVertexOccurrences.h` — degree-two local-link graph and closed
  alternating-cycle extraction.
- `SelectionFeasibility.h` — complete balance/manifold-feasibility audit.
- `SelectionCodec.h` — canonical encoding with section order and digest.
- `SelectionVerifier.h` — independent reconstruction (reverse-order truth
  recomputation, descending-lineage sheet-cell reconstruction, re-encoded digest).

## Implemented normative behavior

- Every positive-area Component 09 atom receives exactly one audited
  disposition: `retain_preserve`, `retain_reverse`, `discard_equal_sides`,
  `suppress_internal`, `suppress_non_owner`, or `cancel_coincident`.
- The four-bit A/B side occupancy tuple is built exclusively from Component 02
  shell material-side semantics and Component 09 side labels; the frozen
  Component 01 truth table is evaluated exactly once per atom.
- Retention and preserve/reverse orientation are determined solely by result
  occupancy on the two sides; a missing/contradictory side tuple is a typed
  failure, never a default discard.
- Coincident/coplanar sheet cells are reconstructed from Component 08 coplanar
  support lineage (never coordinates); same/opposite-orientation coincidence,
  cancellation, internal-sheet suppression, and canonical ownership are
  resolved through the frozen Component 01 owner rank.
- Retained uses carry complete provenance, orientation, result occupancy, sheet
  owner lineage, and separation class.
- Directed retained incidences carry exact start/end endpoint domains, carrier
  identity, forward/reverse role, source/event sector lineage, and reciprocal
  `surface_occurrence_descriptor` pairs.
- Planned edges are grouped into exactly two opposite directed uses with
  mutually reciprocal actual/expected descriptors; cross-operand and
  cross-owner pairs are preserved (never rejected by an owner-equality
  assumption).
- Vertex/event occurrences are closed alternating cycles of a degree-two local
  link graph (one face-corner arc + one edge-mate arc per port); distinct cycles
  remain distinct even at bit-identical coordinates.
- Complete feasibility audit proves disposition coverage, retained-use
  bijection, incidence consumption, continuation reciprocity, planned-edge
  cardinality/reciprocity, and vertex-occurrence cycle partition.
- The artifact is canonical, transactional, and independently verified.

## V1 fidelity boundary (fail-closed, not silent)

The current Component 09 predecessor does not persist per-atom ordered boundary
rings. Component 10 reconstructs retained-atom boundaries from the manifold
facet groups and the intersection complex's source-edge sequences. This is
exact for whole-facet atoms (disjoint operands, containment, and equal/coincident
configurations that reduce to whole facets). Proper transverse overlap splits a
facet into sub-facet cells whose carrier-span boundaries require re-deriving the
full cell decomposition; this path is not yet wired and currently fails closed
with `mate_cardinality_error` rather than publishing an incorrect manifold.

Coincident/coplanar ownership machinery is implemented and unit-tested directly;
end-to-end coincident fixtures depend on the same cell-boundary persistence gap
above.

## Tests (`TestSelectionMain.cc`, `SelectionFixtures.*`)

- `truth` — all 80 Component 01 operation/side cells, retention/orientation
  consistency, byte determinism.
- `disjoint` — union/intersection/A-B/symmetric-difference retained-use and
  planned-edge counts.
- `containment` — union/intersection/A-B/B-A including cavity-orientation
  reversal.
- `occupancy` — every retained use is a result-side transition.
- `coincidence` — frozen owner rank (operand priority, feature order).
- `edge_vertex` — reciprocal cross-operand mate grouping, cardinality and
  non-reciprocal rejection, triangular-fan cycle extraction, degenerate-arc
  rejection.
- `canonical` — byte and digest determinism, independent-verification
  disposition.
- `mutation` — independent verifier rejects a flipped disposition and a forged
  digest.
