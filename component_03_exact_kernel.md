# Component 3: Exact Arithmetic, Constructions, and Predicates

## 0. Purpose

Provide the sole authority for geometric decisions. Interpret input floating-point values exactly, accelerate common signs with certified filters, fall back to unbounded exact arithmetic, represent constructions exactly, and resolve only genuine combinatorial ties with documented symbolic perturbation.

## 1. Input contract

Accept finite supported binary floating-point scalars, exact integers/rationals produced by the kernel, stable feature IDs for tie-breaking, and bounded-size vectors/matrices required by geometric operations.

Callers must request a semantic operation such as sign, compare, incidence, or intersection construction. Callers may not inspect approximate intermediates to make topology decisions.

## 2. Required behavior

### Number system

Implement in-tree portable C++17:

- Exact decoding of sign, significand, and exponent for supported `T`; each input becomes a dyadic rational without decimal conversion.
- Signed arbitrary-precision integers with canonical zero/sign/limb representation and checked allocation.
- Normalized arbitrary-precision rationals with positive denominator and exact comparison; normalization strategy may be lazy but equality/order must be canonical.
- Expansion or interval arithmetic for fast certified filters, with rigorously derived error bounds and exact fallback.

Unlimited correctness mode imposes no fixed limb cap. Configured resource limits return `resource_limit`, never a guessed sign.

### Predicates

At minimum provide exact/certified:

- Scalar/rational comparison and sign.
- 2D and 3D orientation.
- Point-on-line/segment, point-in-simple-polygon, point-on-plane, and point-in-triangle/polygon classifications with boundary categories.
- Segment/segment, segment/plane, segment/polygon, and polygon/polygon relation classifications.
- Plane-side, collinearity, coplanarity, parallelism, and direction/order comparisons.
- Ray/triangle crossing and solid point-location primitives.
- In-circle or equivalent predicates if a chosen deterministic planar arrangement/triangulation algorithm needs them; in-sphere only if a later algorithm actually requires it.

Predicates return rich enums where boundary cases matter, not booleans that conflate touching and crossing.

### Exact constructions

Represent constructed points and parameters as normalized exact expressions/rationals derived from source geometry. Support line-plane intersection, coplanar line intersection, projected coordinates, plane-plane carriers, and exact affine interpolation. Cache reduced forms and hashes without changing equality semantics.

### Symbolic perturbation

Use simulation of simplicity or lexicographic infinitesimals only where the mathematical query requires choosing a generic probe/ray or deterministic decomposition. It must not change set membership or erase a real coincidence. Tie order derives solely from stable IDs, and APIs distinguish the unperturbed exact relation from the perturbed decision.

### Determinism and safety

- Results are independent of machine rounding mode, fused operations, extended precision, and filter path.
- Filter code explicitly controls assumptions needed for its proof or disables itself.
- Integer arithmetic is checked for allocation/size overflow.
- Caches are thread-safe or stage-local and cannot affect result ordering.

## 3. Output contract

Produce exact number/value types, predicate result enums with certificates or reproducible operands, exact construction handles, and optional conservative approximations explicitly marked as non-authoritative.

Invariants:

- Equal exact values compare equal and hash consistently after canonicalization.
- Every non-zero predicate returns the mathematically exact sign.
- A zero result means exact degeneracy, not numerical uncertainty.
- Filter and fallback paths are observationally identical.
- Every construction carries sufficient provenance to recompute and audit it.

Failure conditions are limited to unsupported scalar representation, resource exhaustion, invalid kernel API preconditions, or internal defects. "Nearly degenerate" is never a failure.

## 4. Verification and definition of done

- Big-integer/rational operations are checked against independently generated known-answer vectors and algebraic identities.
- Predicate tests include cancellation-heavy, subnormal, maximum-exponent, signed-zero, exactly degenerate, and one-ULP-separated cases.
- Every filter is forced through both accept and fallback paths; accepted signs are compared with exact evaluation.
- Permutation identities for orientation and incidence hold exactly.
- Construction substitution proves each result lies on all defining carriers.
- Random tests serialize failures as exact bit patterns, not decimal strings.
