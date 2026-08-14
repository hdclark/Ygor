# Component 15: Independent Verification, Diagnostics, and Replay — Implementation Status

## Status

Component 15 is implemented as the deterministic
`serial_complete_final_verification_v1` provider. It consumes the immutable
`assembled_output_candidate<T,I>` from Component 14 together with the immutable
predecessor artifacts and publishes one immutable
`verified_boolean_result<T,I>`; ordinary `bounded_boolean_success<T,I>`
construction is gated on every mandatory publication check passing. It never
changes coordinates, indices, facet order, orientation, topology, occurrences,
multiplicity, classification, cleanup, or precision, and never calls the legacy
Boolean implementations.

## Frozen V1 providers

- `candidate_intake`         = `complete_dependency_graph_audit_v1`
- `public_lexical_audit`     = `exact_bits_indices_triangles_v1`
- `public_topology_reconstruction` = `sorted_use_pair_link_component_v1`
- `triangle_geometry`        = `bounded_dominant_projection_area_v1`
- `forbidden_pair_relation`  = `bounded_triangle_support_overlay_v1`
- `precision_aggregation`    = `primitive_ledger_path_reduction_v1`
- `report_regeneration`      = `reconstructed_fact_reports_v1`
- `reingestion`              = `fresh_component02_operand_contract_v1`
- `logical_serialization`    = `component01_canonical_bytes_le_v1`
- `digest_provider`          = `component01_sha256_domain_separated_v1`
- `final_self_audit`         = `independent_evidence_codec_resource_audit_v1`
- `execution_reference`      = `serial_complete_final_verification_v1`

## Files

- `FinalVerificationTypes.h` — closed enums, strong ID domains, check/finding
  records, subcodes, capabilities, statistics.
- `VerifiedBooleanResult.h` — immutable verified artifact with the unchanged
  Component 14 mesh, topology/geometry reports, and digest.
- `FinalVerification.h` / `FinalVerification.cc` — typed entrypoint
  `verify_and_publish` and the phase orchestration.

Because the verified artifact carries the public `fv_surface_mesh<T,I>`, the
Component 15 production translation units are compiled in
`ygor_mesh_boolean_bounded_facade`, keeping the strict provider target
dependency-minimal.

## Implemented normative behavior

- Candidate intake validates owner, operation, pending-only statuses, and
  independently-verified predecessor dispositions.
- Public lexical audit requires finite coordinates, three-index facets with
  distinct in-range indices, and empty optional arrays.
- Independent topology reconstruction groups directed facet uses into exact
  undirected edges (two opposite uses each), rejects isolated vertices, and
  derives component counts from the assembled facets.
- Triangle geometry proves every facet is a definite non-degenerate triangle via
  its exact squared cross-product magnitude.
- The independent forbidden-intersection search tests every non-adjacent facet
  pair with an exact-orientation triangle-triangle relation (plane separation,
  coplanar 2D overlay, and intersection-line interval overlap); any forbidden
  non-adjacent interaction fails closed.
- Precision aggregation requires the output precision to be finite,
  nonnegative, and no greater than the caller tolerance.
- The verified result carries a canonical digest; independent re-encoding is
  compared on every semantic equality.

## V1 fidelity boundary (fail-closed, not silent)

The current predecessor chain (Components 09-14, simplified to whole-facet
retained uses) does not persist the full per-atom boundary-ring decomposition
needed to reconstruct the classification quotient potentials, deterministic side
probes, and independent operand winding that the plan's deeper gates require.
Component 15 therefore implements the mandatory floor that is fully reconstructible
from the assembled artifact and cleaned manifold (lexical, topology, bijection,
triangle geometry, forbidden intersection, precision) and fails closed on any
geometric defect it can prove; the deep classification/probe/occupancy and
cleanup-replay gates are represented by the schema but are not yet wired, since
their predecessor data is not produced by the current simplified pipeline.

Notably, Component 15's independent forbidden-intersection search caught and
rejected a vertex-ordering defect in an earlier Component 14 build during
development, demonstrating the intended fail-closed independence.

## Tests (`TestFinalVerificationMain.cc`, `FinalVerificationFixtures.*`)

- `contracts` — verified vertex/facet/component counts for a disjoint union.
- `empty` — a verified empty result for a disjoint intersection.
- `containment` — `B-A` verified with outer and cavity shells.
- `canonical` — verified-result digest determinism across independent builds.
- `mutation` — final verification rejects a corrupted candidate whose triangles
  are all degenerate.
