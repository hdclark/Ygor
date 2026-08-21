# Component 13: Budgeted Cleanup and Topological Simplification — Implementation Status

## Status

Component 13 is implemented as the deterministic
`append_only_generation_checked_cleanup_complex_v1` provider. It consumes the
verified immutable artifacts from Components 01-12 and publishes one immutable
`cleaned_triangle_manifold<T>` for Component 14. It never repeats Boolean
classification, chooses intersection ownership, reconstructs Component 11 face
cycles as a substitute for predecessor data, assembles `fv_surface_mesh<T,I>`,
merges public polygons, or publishes an ordinary Boolean success.

## Frozen V1 providers

- `mutable_complex_provider`  = `append_only_generation_checked_cleanup_complex_v1`
- `obligation_provider`       = `explicit_member_dependency_graph_v1`
- `candidate_provider`        = `obligation_seeded_complete_local_actions_v1`
- `scheduler_provider`        = `serial_global_minimum_phased_scheduler_v1`
- `patch_provider`            = `closed_star_interface_patch_v1`
- `link_provider`             = `reconstructed_closed_surface_link_v1`
- `split_provider`            = `exact_oriented_fan_sector_partition_v1`
- `coordinate_provider`       = `existing_coordinate_first_bounded_segment_v1`
- `retriangulation_provider`  = `interface_preserving_bounded_local_cell_v1`
- `intersection_provider`     = `append_only_rank_morton_triangle_forest_v1`
- `feature_cost_provider`     = `certified_piecewise_affine_patch_deviation_v1`
- `budget_provider`           = `component03_outward_sum_per_lineage_v1`
- `topology_effect_provider`  = `rebuilt_component_link_euler_semantics_v1`
- `certificate_provider`      = `replayable_closed_patch_delta_v1`
- `canonicalization_provider` = `complete_lineage_action_key_remap_v1`
- `verification_provider`     = `independent_replay_rebuild_and_all_pairs_v1`

## Files

- `CleanupTypes.h` — closed enums, strong ID domains, canonical keys (vertex,
  edge, triangle), obligation/action records, subcodes, capabilities,
  statistics.
- `CleanedTriangleManifold.h` — immutable artifact with final vertices, paired
  edges, halfedges, triangles, components, obligation dispositions, and action
  log.
- `MeshCleanup.h` / `MeshCleanup.cc` — typed entrypoint
  `build_cleaned_triangle_manifold` and the phase orchestration.
- `CleanupCodec.h` — canonical encoding with a fixed section order and digest.
- `CleanupVerifier.h` — independent reconstruction (pair reciprocity, triangle
  closure, vertex links, per-component Euler, digest).

## Implemented normative behavior

- Every Component 11 output occurrence becomes one cleaned vertex occurrence
  with its authoritative unchanged bounded coordinate; no coordinate moves.
- Every Component 12 triangle becomes one cleaned triangle with three distinct
  occurrence corners in prescribed outward order and `definite_positive_area`
  category.
- Paired edges and reciprocal halfedges are reconstructed from exact triangle
  corner connectivity: every undirected edge has exactly two opposite directed
  uses and exactly two incident triangles.
- Each cleaned vertex reconstructs one closed oriented fan through
  `around_origin(h) = pair(next(next(h)))`.
- Connected components are reconstructed through shared-edge adjacency, and each
  component's Euler characteristic (`V - E + F`) is computed and published.
- Cleanup obligations are imported and reconciled; the V1 defect scan proves the
  imported manifold has no residual cell, no cleanup-required triangle, and no
  prohibited zero-length edge, so zero actions are required.
- The artifact is canonical, transactional, and independently verified.

## V1 fidelity boundary (fail-closed, not silent)

The current Component 12 predecessor emits only `definite_positive_area`
triangles with no residual cells and no cleanup certificates. Component 13
therefore publishes a zero-action cleaned manifold. The cleanup action machinery
(edge collapse, fan split, retriangulation, collinear-chain replacement,
whole-component removal, Component 03 tolerance-budget reservation) is
represented in the schema but not wired: a cleanup-required triangle, residual
cell, or pending obligation that the V1 defect scan discovers fails closed with a
typed `result_geometry_not_validated` / `internal_invariant` error rather than
publishing an unsimplified or incorrectly welded result.

## Tests (`TestCleanupMain.cc`, `CleanupFixtures.*`)

- `contracts` — count equations (vertices, edges, halfedges, triangles,
  components) and zero-action `verified_clean` status for a disjoint union.
- `empty` — a fully audited, `verified_empty` artifact for a disjoint
  intersection.
- `containment` — `B-A` with outer shell plus reversed cavity shell.
- `links` — every box-corner fan closes into a single cycle.
- `euler` — every genus-0 component has Euler characteristic two.
- `canonical` — byte and digest determinism across independent builds.
- `mutation` — independent verifier rejects broken pair reciprocity and a forged
  digest.
