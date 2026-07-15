# Component 10: Regularized Boolean Boundary Selection

## 0. Purpose

Apply a tiny, auditable Boolean truth table to labeled arrangement sides and produce the exact oriented boundary of the requested regularized result. All geometric complexity must already have been resolved.

## 1. Input contract

Accept frozen operation contract, verified labeled arrangement, exact patch/coincidence groups, and source provenance.

Every atomic patch has occupancy vectors for its two open sides. Coincident representatives cover identical exact domains with a deterministic common subdivision.

## 2. Required behavior

- Evaluate result occupancy `R` independently on both sides of every atomic patch domain.
- Discard the domain when both sides have equal result occupancy; this removes internal interfaces and lower-dimensional-only contacts under regularization.
- Keep exactly one representative when the two sides differ.
- Orient the kept representative so the `R=true` side is the prescribed interior side.
- Resolve coincident source sheets by side labels, not by operand priority. Provenance selection is deterministic but has no geometric effect.
- Cancel opposite duplicate boundaries only when exact domain equality and side labels prove cancellation.
- Retain seam vertices/edges needed to connect selected patches, but do not emit isolated curves or points.
- Optionally merge adjacent coplanar selected patches only after exact proof that removing their shared edge preserves domain, orientation, embedding, and facet simplicity. Merging is not required for correctness.

Operation-specific geometry code is prohibited. Union, intersection, differences, and symmetric difference differ only through the truth table and resulting orientation.

## 3. Output contract

Produce a `selected_exact_boundary` containing oriented symbolic patches, cycles, adjacency through topological occurrence IDs, exact geometric handles, complete operand/event provenance, both side truth values, and an explicit topology class: empty, closed embedded two-manifold, or closed stratified non-manifold.

Invariants:

- A patch is present if and only if result occupancy changes across its open domain.
- Every selected patch has result interior on the prescribed side.
- No positive-area domain appears more than once.
- Every selected surface-edge occurrence has exactly two opposite patch uses. Several surface occurrences may reference one geometric edge, and several circular link occurrences may reference one geometric point.
- Empty result is represented by an empty patch set, not an error.
- Selection uses no floating-point coordinate or tolerance.

Failure conditions are missing/contradictory labels, an open or contradictory stratified incidence, resource exhaustion, or internal invariant failure. A complete valid non-manifold stratified boundary is classified and published internally; it is not an upstream defect. The manifold-only public policy rejects it later with `result_topology_not_supported`.

## 4. Verification and definition of done

- Exhaust all truth-table inputs for each operation and both patch orientations.
- Verify Boolean identities at the exact-boundary level: idempotence, commutativity where applicable, `A-A=empty`, absorption, and operand-order behavior for difference.
- Equal and coincident operands produce the documented exact outputs for every operation.
- Independent side reclassification confirms every selected and discarded patch decision.
- Selected-boundary topology checks pass before realization.
- Vertex- and edge-touching solids retain distinct surface occurrences over their common geometric strata, and occurrence-welding mutations are rejected.
