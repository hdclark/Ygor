# Component 4: Conservative Broad Phase

## 0. Purpose

Reduce exact narrow-phase work by enumerating a deterministic superset of feature pairs that can interact. Broad-phase output affects performance only: omitting a true pair is forbidden, while false positives are harmless.

## 1. Input contract

Accept validated operands, conservative bounds derived from exact source coordinates, feature-type query policy, context resource limits, and stable IDs.

Bounds may be represented by outward-rounded `T` intervals or exact dyadic extrema. Any approximate bound operation must prove containment under all supported platforms.

## 2. Required behavior

- Build a spatial hierarchy or other pruning structure over facet bounds without Boolean-specific semantics.
- Include facet interiors and boundaries exactly; touching boxes overlap.
- Traverse cross-operand structures to enumerate all potentially intersecting facet pairs.
- Support self-query for Component 2 validation without emitting a pair twice or pairing a feature with itself.
- Optionally enumerate edge/facet or lower-dimensional candidates if this demonstrably reduces narrow-phase work; facet-pair coverage remains the correctness baseline.
- Deduplicate and publish candidates sorted by canonical `(operand, facet ID)` keys.
- Permit parallel build/traversal, but merge worker output deterministically.
- Handle empty operands, zero extent, subnormal coordinates, huge dynamic range, and many coincident bounds.

The hierarchy split rule, fanout, and storage layout are replaceable implementation choices. A fallback exhaustive enumerator must remain available for verification and small inputs.

## 3. Output contract

Produce a canonical candidate stream, acceleration statistics, and optional trace proving which bounds caused each candidate.

Invariants:

- Every geometrically interacting pair is present at least once before deduplication and exactly once after publication.
- Candidate order is independent of hierarchy shape, allocation, traversal order, and thread count.
- Bounds contain every point of their feature.
- No narrow-phase conclusion is cached or inferred by this component.

Failure conditions are resource exhaustion or internal bound-invariant failure. When acceleration cannot safely bound a feature, it must conservatively route that feature through exhaustive comparison rather than omit it.

## 4. Verification and definition of done

- For every test mesh, compare candidates with exhaustive exact pair testing and assert zero false negatives.
- Stress touching boxes, one-ULP gaps, signed zero, subnormals, extreme exponents, identical boxes, and pathological hierarchy distributions.
- Randomize insertion and thread scheduling; canonical candidate bytes remain identical.
- Property tests verify that expanding any bound cannot remove candidates.
