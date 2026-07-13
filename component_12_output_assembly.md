# Component 12: Output Assembly, Safe Simplification, and Canonicalization

## 0. Purpose

Convert a certified realized boundary into the public `fv_surface_mesh<T, I>`, perform only semantics-preserving simplifications, establish canonical deterministic serialization, and run final publication checks.

## 1. Input contract

Accept a verified `realized_boundary<T>`, exact selected-boundary provenance, target index type `I`, context output policy, and final verifier.

All coordinates and topology are already certified. This component must not alter coordinates or connectivity heuristically to make checks pass.

## 2. Required behavior

### Indexed assembly

- Assign one output index to each used realized symbolic vertex.
- Check vertex/facet/ring counts and every conversion against `numeric_limits<I>` before publication.
- Emit polygon rings or baseline certified triangles according to realization/output policy.
- Preserve outward orientation and omit unused vertices.

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

Failure conditions are index overflow, resource exhaustion, failed final certificate, or internal invariant error. Assembly cannot downgrade these to warnings.

## 4. Verification and definition of done

- Parse the emitted public vectors from scratch and independently reconstruct all topology.
- Randomly permute internal storage before canonicalization; output bytes remain identical.
- Check output re-ingestion through Component 2.
- Verify optional simplification on/off produces equivalent exact selected boundaries.
- Test maximum `I` boundaries, empty meshes, many components, cavities, and high-valence vertices.
