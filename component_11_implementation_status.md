# Component 11: Paired Output Edges and Face-Cycle Construction — Implementation Status

## Status

Component 11 is implemented as the deterministic
`paired_boundary_face_cycle_construction_v1` provider. It consumes the verified
immutable artifacts from Components 01-10 and publishes one immutable
`polygonal_output_complex<T,I>` for Component 12. It never constructs or moves
coordinates, changes occurrence partitions, collapses or welds topology,
triangulates regions, spends cleanup budget, assembles `fv_surface_mesh<T,I>`,
or publishes a public Boolean result.

## Frozen V1 providers

- `incidence_audit_provider`       = `canonical_retained_incidence_audit_v2`
- `vertex_provider`                = `sorted_nonempty_occurrence_materialization_v2`
- `region_provider`                = `exact_positive_area_continuation_components_v2`
- `boundary_provider`              = `oriented_dart_cancellation_v2`
- `edge_provider`                  = `explicit_endpoint_atomic_pair_v2`
- `successor_provider`             = `continuation_skipping_dart_permutation_v2`
- `cycle_provider`                 = `role_independent_canonical_successor_cycles_v2`
- `contour_provider`               = `lineage_first_single_outer_region_v2`
- `witness_provider`               = `certified_contour_side_witness_v1`
- `admissibility_provider`         = `bounded_planar_embedding_audit_v2`
- `vertex_link_provider`           = `halfedge_rotation_link_audit_v1`
- `verification_provider`          = `independent_polygonal_topology_rebuild_v2`

## Files

- `OutputTopologyTypes.h` — closed enums, strong ID domains, canonical keys
  (vertex, region, paired-edge, halfedge, role-independent cycle, contour,
  witness), artifact records, subcodes, cancellation, capabilities, statistics.
- `PolygonalOutputComplex.h` — the immutable artifact with canonical contiguous
  tables, (begin,count) member ranges, dense reverse maps, and narrow views.
- `OutputTopology.h` / `OutputTopology.cc` — typed entrypoint
  `build_polygonal_output_complex` and the phase orchestration.
- `OutputTopologyCodec.h` — canonical encoding with a fixed section order and
  digest.
- `OutputTopologyVerifier.h` — independent reconstruction (reverse-order
  successor derivation, role-independent cycle comparison, pair reciprocity,
  vertex-link closure, re-encoded digest).

## Implemented normative behavior

- Every realized retained incidence receives exactly one audited disposition
  (`paired_boundary` or `paired_zero_measure_boundary`); the audit is a bijection
  over Component 10 incidences.
- Every nonempty Component 10 vertex-occurrence requirement is materialized
  once, with a resolved authoritative bounded coordinate (source vertex nominal
  plus enclosure, or the Component 08 event construction ledger) and preserved
  occurrence separation.
- Each Component 10 planned edge is born as exactly one explicit-endpoint
  reciprocal pair: two halfedges initialized directly from validated occurrence
  IDs with `h0.origin == h1.destination`, `h0.destination == h1.origin`,
  `h0.pair == h1`, and `h1.pair == h0`.
- Zero-measure boundaries (bit-equal or enclosure-overlapping endpoints) are
  recorded as paired zero-measure descriptors with their two audit rows
  reclassified, without welding distinct topological occurrences.
- Final positive-area regions are exact continuation components (one per
  retained use in V1); reciprocal continuation records are honored but V1 emits
  none.
- The successor/predecessor permutation is the exact continuation-skipping dart
  permutation derived from Component 10 face-corner arcs; every halfedge has one
  successor, one predecessor, one region, and one cycle.
- Face cycles are extracted by walking successors and assigned role-independent
  canonical IDs (orientation-preserving least rotation of complete halfedge
  keys); contour-role assignment never renumbers cycles.
- Every positive-area region has exactly one outer contour with a
  predecessor-certified arrangement-cell witness (the Component 09 atom witness).
- Every output occurrence reconstructs one closed vertex link through
  `around_origin(h) = pair(predecessor(h))` and is compared against the
  Component 10 local-link cycle (outgoing count equals half the port count).
- The artifact is canonical, transactional, and independently verified.

## V1 fidelity boundary (fail-closed, not silent)

The current Component 10 predecessor emits only whole-facet retained uses
(planned-edge incidences; no transparent continuations, no support-only
zero-measure evidence, no consumed-owner seams). Component 11 therefore
publishes one final region per retained use with a single outer contour and a
certified predecessor-cell witness. Continuation consumption, hole nesting,
separate positive-area islands, and deferred zero-measure contour records are
represented in the schema and rejected or left empty rather than synthesized.
Endpoint domains of kind `source_edge_interval_endpoint`, `carrier_endpoint`, or
`contact_delimiter` are not materializable in V1 and fail closed with an
`internal_invariant` error rather than publishing an approximate coordinate.

## Tests (`TestOutputTopologyMain.cc`, `OutputTopologyFixtures.*`)

- `contracts` — count equations (pairs, halfedges, regions, cycles, darts,
  vertices, contours, witnesses, audit rows) for a disjoint union.
- `empty` — a fully audited, verified empty artifact for a disjoint
  intersection.
- `containment` — `B-A` with outer shell plus reversed cavity boundary.
- `reciprocity` — pair reciprocity, reversed endpoints, and successor/predecessor
  inversion for every halfedge.
- `links` — box-corner vertex links close with three outgoing halfedges.
- `canonical` — byte and digest determinism across independent builds.
- `mutation` — independent verifier rejects broken pair reciprocity, a forged
  digest, and a duplicated outer contour.
