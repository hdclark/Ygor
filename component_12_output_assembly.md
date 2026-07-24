# Component 12: Output Assembly, Safe Simplification, and Canonicalization

## 0. Purpose

Convert a certified realized boundary into the public `fv_surface_mesh<T, I>`, perform only semantics-preserving simplifications, establish canonical deterministic serialization, and run final publication checks.

## 1. Input contract

Own an early topology preflight over the verified selected boundary, then accept a topology-authorized verified `realized_boundary<T>`, exact selected-boundary provenance, target index type `I`, context output policy, and final verifier.

All coordinates and topology are already certified. This component must not alter coordinates or connectivity heuristically to make checks pass.

## 2. Required behavior

### Indexed assembly

- Assign one output index to each used realized symbolic vertex.
- Check vertex/facet/ring counts and every conversion against `numeric_limits<I>` before publication.
- Emit polygon rings or baseline certified triangles according to realization/output policy.
- Preserve outward orientation and omit unused vertices.

Before Component 11, accept empty and `closed_embedded_two_manifold` selections under the initial public policy. Reject `closed_stratified_nonmanifold` with `result_topology_not_supported` and canonical obstruction records; do not invoke realization or report a coordinate failure. Final assembly rechecks the authorization binding.

### Exact-proof simplification

Permitted optional operations include removing artificial degree-two subdivisions, merging adjacent coplanar facets, and eliminating duplicate indexing only when exact provenance plus realized-geometry checks prove:

- The represented point set and occupied side do not change.
- Facets remain simple, planar, non-zero-area, and correctly oriented.
- Manifold links and embedding are preserved.
- No required intersection/separation certificate is invalidated.

Distinct exact vertices may not merge merely because their `T` coordinates match; such a collision should already have made realization fail. "Small" facets are never deleted.

### Canonical serialization

- Define a total order on vertex coordinate bit patterns augmented by stable exact provenance where needed.
- Canonically rotate each facet ring; preserve required orientation.
- Sort facets by canonical ring keys and connected components by canonical descriptors.
- Reindex vertices from first canonical use or another fully specified rule.
- Normalize signed zero only if certification and the public bit-level policy explicitly permit it.

Canonicalization must make equivalent pipeline schedules byte-identical; it need not make arbitrary geometrically equivalent input tessellations yield identical tessellation unless that stronger normalization is separately specified.

### Final publication

Run mandatory structural, topology, geometry, orientation, provenance, and certificate checks. Construct the public mesh only after success. Empty result emits empty vertex and facet vectors.

## 3. Output contract

Produce `boolean_result<T, I>` success containing canonical `fv_surface_mesh<T, I>`, diagnostics/statistics, operation/input digests, and optionally compact certificates/provenance. On failure produce no public mesh.

Invariants:

- Every index is in range and every vertex is used.
- Every undirected edge has exactly two opposite facet uses; every vertex link is manifold.
- Every facet is simple, planar under policy, non-zero-area, and outward-oriented.
- Non-adjacent facets do not intersect except where the output contract explicitly allows shared topological features.
- Output is subdivision/homeomorphism-equivalent to the selected exact boundary and passes realization constraints.
- Serialization is deterministic.

Failure conditions are result topology not supported, index overflow, resource exhaustion, failed final certificate, or internal invariant error. Assembly cannot downgrade these to warnings.

## 4. Verification and definition of done

- Parse the emitted public vectors from scratch and independently reconstruct all topology.
- Randomly permute internal storage before canonicalization; output bytes remain identical.
- Check output re-ingestion through Component 2.
- Verify optional simplification on/off produces equivalent exact selected boundaries.
- Test maximum `I` boundaries, empty meshes, many components, cavities, and high-valence vertices.

## 5. Assessment-driven product-publication amendment

The product boundary is a tagged `boolean_product_result`, not a mesh-only success alternative.

Component 12 assembles a mesh only when requested and authorized. The result envelope may contain:

- a durable verified exact stratified boundary;
- a canonical strict exact-in-`T` mesh;
- a canonical certified approximate mesh with its displacement/topology certificate;
- the failed realization or topology-publication attempt when exact-result retention was requested;
- backend, preparation, operation, and qualification provenance;
- attribute-transfer mappings and conflict/omission reports; and
- final verification evidence binding every payload to the same exact-result digest.

A stratified non-manifold exact result is not discarded by the manifold mesh gate. The mesh alternative returns `result_topology_not_supported`, while the exact result remains a successful retained artifact. Likewise, finite-`T` realization failure does not convert an independently verified exact result into total operation failure. Product schema 3 retains exact authority unconditionally and rejects mesh-only discard policies.

For certified approximate output, canonicalization and simplification must stay within the accepted realization certificate. Any operation that changes coordinates, topology, displacement maxima, relaxed relations, or attribute mapping requires certificate regeneration and independent verification; otherwise it is prohibited.

Define and test stable attribute/provenance transfer policies for source bodies, shells, facets, materials, normals, sharp edges, texture seams, metadata, and opaque channels. Attributes never control topology. Multi-source derivations require deterministic merge/split/conflict behavior, and silent omission is prohibited in the product API.

Add a one-call service that constructs the default kernel, verifiers, executor, backend adapter, and publication pipeline internally. Expert dependency injection remains separate. Every returned mesh is re-ingested through strict Component 2 validation and bound to the exact result, producing backend, and qualification manifest.
