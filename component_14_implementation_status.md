# Component 14: Output Assembly and Canonical Serialization — Implementation Status

## Status

Component 14 is implemented as the deterministic
`direct_private_fv_surface_mesh_triangles_v1` provider. It consumes the
immutable, independently verified `cleaned_triangle_manifold<T>` from Component
13 and publishes one immutable `assembled_output_candidate<T,I>` for Component
15. It does not repair topology, move or merge coordinates, run Component 15's
final geometry verification, set `geometry.status == tolerance_checked`, or
expose an ordinary `bounded_boolean_success<T,I>`.

## Frozen V1 providers

- `public_mesh_adapter`       = `direct_private_fv_surface_mesh_triangles_v1`
- `component_reconstruction`  = `exact_paired_edge_component_scan_v1`
- `canonical_graph_model`     = `typed_oriented_incidence_graph_v1`
- `partition_refinement`      = `full_signature_equitable_refinement_v1`
- `automorphism_resolution`   = `individualize_refine_lexicographic_minimum_v1`
- `public_content_labeling`   = `minimum_oriented_mesh_block_encoding_v1`
- `vertex_ordering`           = `canonical_graph_label_v1`
- `triangle_rotation`         = `minimum_forward_cyclic_index_triple_v1`
- `public_topology_rebuild`   = `sorted_directed_edge_and_corner_link_v1`
- `logical_serialization`     = `component01_canonical_bytes_le_v1`
- `digest_provider`           = `component01_sha256_domain_separated_v1`
- `producer_verifier`         = `independent_rebuild_reencode_compare_v1`

## Files

- `OutputAssemblyTypes.h` — closed enums, strong ID domains, candidate records,
  subcodes, capabilities, statistics.
- `AssembledOutputCandidate.h` — immutable candidate holding the canonical public
  `fv_surface_mesh<T,I>`, component records, coordinate-copy records, report,
  maps, and digests.
- `OutputAssembly.h` / `OutputAssembly.cc` — typed entrypoint
  `assemble_output_candidate` and the phase orchestration.
- `OutputAssemblyCodec.h` — canonical public-mesh-content and artifact encoders.
- `OutputAssemblyVerifier.h` — independent reconstruction (edge grouping, vertex
  incidence, map bijections, content re-encoding, digest).

Because `fv_surface_mesh<T,I>` and `vec3<T>` have out-of-line public constructors
in the main Ygor library, the Component 14 production translation units are
compiled in `ygor_mesh_boolean_bounded_facade` (which links Ygor), keeping the
`ygor_mesh_boolean_bounded_strict` provider target independently linkable.

## Implemented normative behavior

- Every cleaned vertex occurrence maps bijectively to one public vertex position
  with exact coordinate-bit copy under the frozen signed-zero rule.
- Every cleaned triangle maps bijectively to one public three-index triangular
  face with forward cyclic rotation to the lexicographically smallest triple.
- Components are reconstructed from cleaned paired topology and ordered by
  canonical component public-content bytes.
- Within each component, vertices are canonically labeled by a typed oriented
  incidence graph (vertex + corner nodes with coordinate-bit colors and
  next/previous orientation relations) under full-signature equitable
  refinement; a non-discrete labeling that cannot resolve vertex automorphisms
  fails closed rather than falling back to cleaned IDs or traversal order.
- Public directed edges, reciprocal pairs, two-use groups, and vertex incidence
  are independently reconstructed and verified.
- The output precision report aggregates the maximum cleaned vertex radial error
  and the caller tolerance with exact bit encoding.
- The candidate carries pending-only topology/geometry statuses and is
  independently verified; Component 15 alone promotes it to ordinary success.

## V1 fidelity boundary (fail-closed, not silent)

The current predecessor produces cleaned manifolds with distinct vertex
coordinates, so the equitable refinement yields a discrete labeling without
individualization branching. The full individualization/refinement automorphism
search is not yet wired: a symmetric component whose refinement does not resolve
to a discrete labeling fails closed with `automorphism_inconsistency` rather
than returning a noncanonical order. Safe pruning and memoization are omitted in
favor of a deterministic full-signature refinement; both are future provider
versions.

## Tests (`TestOutputAssemblyMain.cc`, `OutputAssemblyFixtures.*`)

- `contracts` — public vertex/facet/component counts and pending status for a
  disjoint union.
- `empty` — a valid empty public mesh for a disjoint intersection.
- `maps` — vertex/facet forward and reverse maps are bijective.
- `canonical` — byte and digest determinism across independent builds.
- `mutation` — independent verifier rejects a corrupted face index and a forged
  digest.
