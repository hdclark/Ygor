# Component 9: Exact Cell and Side Classification

## 0. Purpose

Determine exact occupancy of both operands in every open arrangement region relevant to boundary selection. Classification must remain correct at tangencies, coincidences, and nested shells and must not sample a numerically offset point.

## 1. Input contract

Accept verified global arrangement, validated oriented operands, exact kernel, stable-ID perturbation policy, and classification diagnostics policy.

Input operands already satisfy the embedded regular-closed-solid contract. Arrangement sectors are complete. Classification is operation-independent and computes `(inside_A, inside_B)`.

## 2. Required behavior

### Frozen classification model

Schema v1 uses only `classification_strategy::independent_patch_side_v1`. Every atomic patch side has its own certified open probe and direct `(inside_A, inside_B)` classification. Region IDs denote conservative arrangement fragments, not proven maximal 3D cells. Preserving/crossing transitions and propagation remain mandatory consistency audits but are never the proof that supplies a missing side label.

### Exact seed classification

- Select deterministic rational/symbolic probe points known to lie in an open region; never use `p + epsilon * normal` in floating point.
- Classify a seed against each operand by exact oriented ray casting, winding/degree, or another proven exact point-location method.
- Handle a ray through a vertex/edge/coplanar facet using symbolic perturbation with stable IDs, while preserving the unperturbed query region.
- Cross-check ambiguous implementation paths with an alternate deterministic ray or local orientation evidence.

### Propagation

- Audit direct occupancy through arrangement adjacency.
- Crossing an oriented non-coincident operand sheet toggles/updates only that operand according to the solid policy.
- Crossing coincident sheet groups applies the net oriented boundary contribution and sector order, not an arbitrary facet count.
- Tangential contact that does not cross occupied volume leaves the appropriate label unchanged.
- If an audited transition or propagation path disagrees with either directly classified endpoint, require an invariant error with both proofs recorded.

### Side labels

For every atomic patch representative, record exact occupancy vectors on its negative and positive open sides. The side itself is boundary and is not mislabeled as inside by a boundary-inclusive point test.

## 3. Output contract

Produce a `labeled_arrangement` containing the frozen strategy/version, conservative fragment IDs, one direct certificate per patch side, both-side labels for every patch, and separate transition-audit provenance.

Invariants:

- Every relevant open region has exactly one occupancy vector.
- Labels are path-independent.
- Crossing behavior agrees with source orientation and operand role.
- Regions connected without crossing a boundary have equal labels.
- Exterior-at-infinity is outside both finite closed operands.
- Classifications are invariant under probe ray choice and exact positive scaling/rigid transformations representable by the test kernel.

Failure conditions are resource exhaustion, incomplete arrangement adjacency, contradictory source orientation/solid contract, or classification invariant failure. A probe degeneracy triggers another exact/symbolic query, not approximate classification.

## 4. Verification and definition of done

- Analytic cases cover disjoint, nested, cavity, equal, touching, tangent, transverse, and coincident opposite-orientation solids.
- Independent exact rays agree for every seed in verification mode.
- Propagation around every closed adjacency cycle returns to the starting label.
- Operand swap swaps label components only.
- Random interior/exterior exact probes agree with their containing cell labels.
- Hybrid artifacts that claim complete-cell propagation while carrying per-side seeds, or omit any direct side certificate, are rejected.
