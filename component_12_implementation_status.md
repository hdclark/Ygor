# Component 12: Degeneracy-Tolerant Polygon Triangulation — Implementation Status

## Status

Component 12 is implemented as the deterministic
`bounded_indexed_ear_decomposition_v1` provider. It consumes the verified
immutable artifacts from Components 01-11 and publishes one immutable
`triangulated_output_complex<T,I>` for Component 13. It never repeats Boolean
classification or selection, alters output-occurrence identity, moves
coordinates, spends cleanup budget, collapses or welds topology, removes a
component, merges result polygons, or publishes a public Boolean result.

## Frozen V1 providers

- `projection_provider`              = `authoritative_support_frame_projection_v1`
- `region_model_provider`            = `occurrence_preserving_constraint_model_v1`
- `predicate_provider`               = `component03_truth_layers_planar_relations_v2`
- `contour_provider`                 = `canonical_complete_visibility_bridge_v1`
- `triangulation_provider`           = `bounded_indexed_ear_decomposition_v1`
- `orientation_escalation_provider`  = `neighbor_chain_truth_layer_escalation_v2`
- `diagonal_provider`                = `atomic_pending_twin_diagonal_v1`
- `degeneracy_provider`              = `length_certificate_degenerate_cell_handoff_v2`
- `producer_overlap_provider`        = `region_rank_interval_aabb_index_v1`
- `coverage_provider`                = `boundary_euler_area_nonoverlap_v1`
- `verification_provider`            = `independent_truth_layer_cell_rebuild_v2`

## Files

- `OutputTriangulationTypes.h` — closed enums, strong ID domains, canonical keys
  (region, projected occurrence, diagonal, triangle), three-layer predicate
  evidence, cleanup length certificates, subcodes, capabilities, statistics.
- `TriangulatedOutputComplex.h` — immutable artifact with canonical contiguous
  tables, (begin,count) ranges, dense reverse maps, and narrow Component 13
  views.
- `OutputTriangulation.h` / `OutputTriangulation.cc` — typed entrypoint
  `build_triangulated_output_complex` and the phase orchestration.
- `TriangulationCodec.h` — canonical encoding with a fixed section order and
  digest.
- `TriangulationVerifier.h` — independent reconstruction (boundary conservation,
  pair reciprocity, triangle closure, Euler, digest).

## Implemented normative behavior

- Each Component 11 positive-area region is projected through its source-facet
  support frame (`dropped_axis`) exactly once; projected occurrences carry
  nominal bits and conservative enclosures without changing the 3D coordinate.
- Every Component 11 boundary halfedge is preserved unchanged in identity,
  direction, endpoints, pair, region, and provenance, and assigned exactly once
  to one triangle.
- The region boundary is decomposed by a deterministic bounded indexed ear
  algorithm: convex, empty ears are clipped using exact 2D orientation (the
  exact-relation layer) with interval enclosures retained as evidence.
- Every internal diagonal is allocated atomically as a reciprocal halfedge pair
  with exact reversed endpoints; the emitted triangle uses one side while the
  reverse side becomes the remaining active-cell boundary use.
- Every ordinary triangle references three distinct Component 11 output
  occurrences and three oriented edge uses forming one closed cycle with
  `definite_positive_area` category.
- Three-layer planar predicate evidence (rounded nominal, exact relation, and
  uncertainty enclosure) is recorded for each region's polygon-area query.
- Per-region boundary, pair, connectivity, Euler (`V - E + F = 1 - H`), and
  coverage certificates are emitted and independently verified.
- The artifact is canonical, transactional, and independently verified.

## V1 fidelity boundary (fail-closed, not silent)

The current Component 11 predecessor emits regions with exactly one outer
contour and no holes, built from whole source facets (simple polygons without
repeated-coordinate or zero-length degeneracies). Component 12 therefore
triangulates simple polygons directly. Hole bridging, complete bridge candidate
enumeration, orientation escalation beyond the immediate exact relation, and
residual/degenerate-cell handoff are represented in the schema but fail closed
rather than synthesized: an unsupported contour hierarchy, a definite
orientation inversion, an empty occurrence domain, or a stalled ear decomposition
returns a typed `geometric_condition_exceeds_tolerance`/`internal_invariant`
error instead of publishing an approximate triangulation.

## Tests (`TestTriangulationMain.cc`, `TriangulationFixtures.*`)

- `contracts` — count equations (triangles, diagonals, internal halfedges,
  boundary assignments, coverage certificates, support frames) for a disjoint
  union.
- `empty` — a fully audited, verified empty artifact for a disjoint intersection.
- `containment` — `B-A` with outer shell plus reversed cavity boundary.
- `boundary` — every Component 11 boundary halfedge is assigned exactly once.
- `euler` — every quad region satisfies `V - E + F = 1`.
- `canonical` — byte and digest determinism across independent builds.
- `mutation` — independent verifier rejects a missing boundary assignment, a
  broken diagonal pair, and a forged digest.
