# Component 05 legacy reuse audit

Component 05 was implemented against the immutable Component 02 and Component 04 artifacts. No legacy Boolean, repair, orientation, hole-filling, remeshing, mutable adjacency, coordinate-welding, or generic triangulation provider is called.

## `fv_surface_mesh<T,I>`

Retained only as the public source carrier upstream of Component 02. Component 05 never reads caller-owned mesh arrays and does not mutate the public carrier into a halfedge structure.

## `YgorMeshesVerification.h/.cc`

Rejected as a producer or verifier dependency. Its useful ordered-endpoint and opposite-direction ideas are reproduced locally over complete typed predecessor identities. Existing routines read public mutable arrays, lack source-edge/internal-diagonal role domains, do not publish reciprocal pairs or fans, and do not satisfy owner/version/transaction contracts. No helper was extracted because the required typed provenance payload would have changed the legacy API without benefiting existing callers.

## `YgorMeshesBoolean5.cc`

Rejected. Its fixed-grid `long double` snap key, clamping, coordinate-derived topology, mutable arrangement, and exception-oriented control flow violate the bounded subsystem identity and geometry-basis contracts. No code or control flow was copied.

## Orientation, repair, refinement, hole, remeshing, and adjacency utilities

Rejected. These utilities either choose coordinate representatives, sort adjacency geometrically, repair input, mutate topology, or implement unrelated output-side semantics. Component 05 preserves predecessor incidence exactly and traverses vertex fans only through `pair(previous(h))`.

## Reused bounded-subsystem capabilities

The implementation reuses Component 01 checked arithmetic, owner tokens, typed outcomes, resource reservations, cancellation, canonical primitives, SHA-256, and strict targets; Component 02 canonical source identities, directed uses, source edges, facets, shells, and occupied-side records; Component 03 finite intervals, floating-bit import, and deterministic interval hulls; and Component 04 oriented triangles, local edge roles, internal diagonals, facet coverage, geometry-basis references, and semantic digests.

Producer and independent verifier retain separate grouping and traversal loops. Shared code is limited to closed record definitions, checked arithmetic, finite interval primitives, canonical byte primitives, and immutable query helpers.
