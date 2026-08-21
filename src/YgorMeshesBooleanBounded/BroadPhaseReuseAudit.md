# Component 06 reuse audit

Component 06 reuses the existing bounded-Boolean contracts rather than introducing
an independent geometry or ownership layer.

## Reused directly

- `finite_interval<T>`, `finite_numeric_less`, and Component 03 directed bounds are
  the only pruning arithmetic. No nominal-coordinate or tolerance shortcut is used.
- Component 05 canonical vertex, edge, triangle, halfedge, provenance, bound, and
  precision attachments are copied into immutable primitive records and are
  independently reconstructed by the verifier.
- `canonical_bound_hull` is used for producer bound reconstruction and hierarchy
  construction. The verifier repeats the hull construction through separate control
  flow.
- `strong_id`, `context_owner_token`, canonical byte writers/readers, SHA-256,
  checked arithmetic, resource reservations, cancellation, typed outcomes, and the
  stage transaction are reused from Component 01.
- The strict floating target and runtime precision context remain the capability
  boundary; this component adds no floating-point environment or exact-arithmetic
  dependency.

## Deliberately implemented here

- Dense endpoint-pair ranking and rank-Morton interleaving were not available in
  Ygor with the required owner-free, versioned, most-significant-bit-first contract.
- The flat leaves-first triangle AABB hierarchy has a frozen odd-node carry rule and
  node-ID layout that differs from general-purpose spatial indexes.
- Exact count/prefix/emit traversal is stage-specific because candidate reservation,
  canonical witnesses, and deterministic diagnostics are part of the artifact.
- Candidate canonicalization uses the complete Component 06 semantic key and cannot
  reuse sort/dedup utilities that silently collapse duplicates.
- The mandatory verifier is intentionally independent: it reconstructs primitive
  tables, dense ranks, Morton keys, hierarchy layout, and candidate sets using
  separate breadth-first control flow and a bounded all-pairs oracle.

## Rejected extraction candidates

- General mesh AABB helpers were not reused because they operate on nominal values,
  do not carry Component 03 enclosure evidence, or do not freeze closed-separation
  semantics.
- Existing Morton/BVH code was not reused because it either quantizes coordinates,
  depends on platform integer width, assigns topology-sensitive IDs, or does not
  expose canonical replay bytes.
- Component 02 validation sweep structures were not extracted because their domain,
  ordering, and pruning proofs are validation-specific and do not satisfy the V1
  all-canonical-edge/opposite-source-triangle contract.

No external dependency is introduced. All code remains C++17 and self-contained in
Ygor.
