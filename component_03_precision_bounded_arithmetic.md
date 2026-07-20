# Component 03: Precision, Tolerance, and Bounded Arithmetic

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and must not use any external dependency.

A later implementation plan may select different enclosure representations or arithmetic techniques, but it must preserve the contracts, conservative-bound requirements, and failure behavior defined here.

## 0. Purpose

This component provides the numerical foundation for all geometry while keeping numerical uncertainty distinct from topological identity. It defines and enforces:

- the supported floating-point execution model;
- scale-dependent machine roundoff floors;
- operand input precision;
- conservative uncertainty envelopes for source and constructed geometric values;
- conditioning assessment for geometric constructions;
- caller-authorized tolerance budgets;
- cleanup displacement accounting;
- monotonic propagation of output precision; and
- bounded predicates that return definite, tied, uncertain, or invalid outcomes rather than silently guessing.

The component must make it possible for later stages to use fast floating-point arithmetic while knowing when a result is reliable enough for topology, when symbolic policy must resolve an exact tie, and when the requested operation must fail because uncertainty exceeds the contract.

It does not assign topology, choose Boolean ownership, or perform cleanup. It supplies bounded numerical facts and audited budget services to components that do.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the scalar type descriptor for `T`, initially qualified IEC 60559 `float` or `double`;
- the frozen context and platform qualification from Component 01;
- exact source coordinate bit patterns from each operand;
- declared `input_precision_a` and `input_precision_b`;
- the caller's maximum tolerance;
- policy settings that define whether bounds are radial, axis-aligned, mixed, or otherwise represented;
- deterministic rounding/contraction requirements; and
- resource, cancellation, diagnostic, and transaction services.

The component must be initialized before any topology-affecting geometric decision is made.

### 1.2 Required distinctions

The component must represent and preserve the distinction among:

1. **machine roundoff floor**: unavoidable scale-dependent uncertainty from storing and operating in `T`;
2. **input precision**: uncertainty inherited from the source operand;
3. **construction uncertainty**: forward error and inherited error for a newly computed value;
4. **classification margin**: separation between a bounded quantity and a decision boundary;
5. **cleanup displacement**: actual authorized movement or removal introduced by Component 13;
6. **output precision**: conservative uncertainty published with the result; and
7. **user tolerance**: maximum geometric deviation or feature removal authorized by the caller.

No API may collapse these quantities into a single undifferentiated `epsilon`.

### 1.3 Source-value requirements

All source values entering bounded arithmetic must be finite and associated with:

- a nominal value in `T`;
- a stable provenance ID;
- an inherited precision no smaller than the applicable machine floor;
- a coordinate scale or enclosure from which safe operation bounds can be derived; and
- a context owner token.

Non-finite values are rejected by Component 02, but this component must still perform defensive finite checks at every public boundary.

## 2. Required behavior

### 2.1 Qualified arithmetic model

The component must define the exact arithmetic assumptions under which its bounds are valid. At minimum:

- authoritative scalar operations use nearest-even rounding;
- expression grouping is prescribed where it affects bounds;
- fast-math, reassociation, finite-only assumptions, and unsafe reciprocal transformations are forbidden;
- contraction into fused operations is either consistently required, consistently prohibited, or accounted for by bounds and qualification tests;
- conversions to and from `T` occur at documented points;
- integer-to-floating conversions are range-checked;
- subnormal behavior is qualified and included in bounds;
- signed zero is preserved or canonicalized only according to a documented rule; and
- no transcendental function is used as an authoritative topology predicate.

The component must expose a platform-conformance result to Component 01. Unsupported behavior returns `unsupported_platform` before the Boolean pipeline proceeds.

### 2.2 Machine roundoff floor

The component must compute a conservative minimum uncertainty for every scalar or vector quantity from:

- type precision;
- coordinate magnitude;
- local and global spatial scale;
- the number and shape of arithmetic operations;
- cancellation risk; and
- any format conversion.

The floor must remain meaningful for:

- values near zero;
- subnormals;
- very large coordinates with small local features;
- large translations;
- power-of-two scales; and
- mixed-magnitude operands.

A single bounding-box-diagonal multiplier is not sufficient if it underestimates local cancellation or translated-coordinate effects.

### 2.3 Bounded value types

The component must provide bounded scalar, vector, point, plane, parameter, and residual abstractions. Their exact representation is implementation-defined, but they must support at least:

- a nominal `T` value;
- a conservative enclosure;
- inherited and construction contributions;
- provenance;
- finite/valid state;
- deterministic serialization; and
- comparison against zero, intervals, and tolerance budgets.

A point representation must support conservative coordinate-wise and radial uncertainty, conceptually equivalent to:

```cpp
struct bounded_point {
    vec3<T> nominal;
    vec3<T> axis_error;
    T radial_error;
    provenance_id provenance;
};
```

The implementation may store a tighter or more compact enclosure if all required queries remain conservative.

### 2.4 Primitive bounded operations

The component must provide conservative forms of at least:

- addition and subtraction;
- multiplication and division;
- fused or unfused multiply-add according to policy;
- scalar/vector interpolation;
- dot product;
- cross product;
- squared norm and norm bounds without topology-authoritative square-root dependence where avoidable;
- affine projection;
- plane construction from accepted non-collinear points;
- signed plane residual;
- orientation-related determinants required by later components;
- parameter projection along a carrier;
- bounding-box and enclosure union/intersection tests; and
- conversion/rounding to `T`.

Division must reject or return an uncertain/invalid result when the denominator enclosure contains zero and no symbolic special case applies.

Every operation must propagate inherited uncertainty and add a conservative floating-operation bound. Bounds must never shrink merely because two uncertain nominal values happen to cancel.

### 2.5 Predicate result model

Topology-affecting comparisons must not return only `bool`. They must distinguish at least:

- definitely negative;
- exactly or symbolically tied under the qualified floating representation;
- definitely positive;
- uncertain within the current enclosure; and
- invalid/non-finite.

The result must include:

- nominal value;
- conservative enclosure;
- distance or margin from the decision boundary when definite;
- provenance and operation trace sufficient for diagnostics;
- whether uncertainty is inherited, conditioning-driven, or cleanup-driven; and
- a recommended disposition: accept numeric sign, invoke symbolic policy, use an alternate bounded formulation, or fail.

An implementation may retry with an algebraically different but contractually equivalent bounded formulation only if the chosen formulation and precedence are deterministic and do not violate the compute-once lineage requirement. It must not evaluate equivalent formulas independently in different consumers.

### 2.6 Construction conditioning

For every constructed intersection or projection, the component must estimate whether the construction is sufficiently conditioned relative to available tolerance. In particular, edge-plane and edge-face constructions must account for:

- source endpoint uncertainty;
- plane uncertainty;
- denominator separation from zero;
- interpolation-parameter uncertainty;
- coordinate cancellation;
- residual against the source edge;
- residual against the source plane or carrier; and
- final rounding to `T`.

The construction result must classify itself as:

- stable interior construction;
- stable endpoint construction;
- exact or symbolic tie;
- coplanar/coincident relation requiring special handling;
- near-parallel but bounded within tolerance;
- ill-conditioned beyond available tolerance; or
- invalid.

Ill-conditioned constructions must return `geometric_condition_exceeds_tolerance` or a more specific typed status. They must not emit a nominal coordinate with an unbounded or dishonest enclosure.

### 2.7 Precision ledger

The component must maintain a precision ledger for every published geometric artifact. A ledger entry must record:

- source precision inherited from each operand;
- machine-floor contribution;
- construction operations and conservative contribution;
- conditioning margin;
- any symbolic classification marker;
- cleanup displacement contributions when later updated;
- aggregate output precision contribution; and
- whether the value remains within caller tolerance.

The ledger may use compact shared records, but must support deterministic diagnostics and independent verification.

For every stage:

```text
published_precision >= all inherited input precision
published_precision >= all construction uncertainty
published_precision >= all realized cleanup displacement
```

Ordinary success additionally requires the applicable published bound to remain within the user-authorized tolerance.

### 2.8 Tolerance-budget service

The component must expose an auditable budget service used by Components 11-15. The service must distinguish:

- uncertainty that exists without moving geometry;
- actual vertex displacement;
- local feature removal size;
- cumulative displacement along a vertex lineage;
- component removal authorization; and
- global maximum tolerance.

A cleanup proposal must reserve a budget before mutation and commit the actual conservative cost only if the topology operation succeeds. Rollback releases the reservation.

Budget combination must be conservative. Depending on the geometric relation, costs may combine by maximum, sum, or another proven bound; the rule must be explicit and testable. It is not acceptable to report only the largest individual edit if sequential edits can accumulate in the same lineage.

### 2.9 Repeated-operation propagation

When a Boolean result is used as a later input, the component must preserve its published precision. Import must not reset uncertainty because coordinates are exactly representable in `T`.

For chains of transforms and Booleans, precision propagation must account for:

- affine scaling and translation effects;
- previously accumulated construction uncertainty;
- prior cleanup displacement;
- output serialization/rounding; and
- newly introduced uncertainty.

If accumulated precision exceeds the new caller tolerance, the operation must fail before publishing an ordinary success.

### 2.10 Conservative broad-phase bounds

The component must provide Component 06 with conservative feature bounds inflated by all relevant uncertainty. It must support:

- source vertices, edges, triangles, and shells;
- constructed events when later queried;
- combined operand uncertainty;
- closed overlap tests that do not miss contact at exact bounds; and
- safe expansion without overflow to non-finite values.

If a finite conservative bound cannot be represented, the stage must fail with a typed numerical or resource status rather than prune candidates.

### 2.11 Residual and plausibility services

For Component 15, the component must independently support checks such as:

- constructed point lies on its source edge within its enclosure;
- constructed point lies on the defining plane or carrier within its enclosure;
- cleanup output remains within its displacement certificate;
- triangle orientation is definite or accepted under the degeneracy policy;
- non-adjacent feature separation is definite outside uncertainty envelopes; and
- published output precision dominates all contributors.

Verification APIs must not require access to producer-private intermediate booleans.

## 3. Output contract

On successful initialization, the component must produce an immutable `precision_context<T>` plus controlled precision-ledger and tolerance-budget services.

The precision context must contain:

- qualified arithmetic-model metadata;
- machine parameters and conformance digest;
- global and operand-local scale descriptions;
- normalized input precisions;
- caller tolerance;
- bound-combination rules;
- deterministic rounding/contraction policy;
- supported predicate result categories;
- serialization version; and
- owner identity.

For each geometric construction, the component must produce a bounded artifact containing:

- nominal value in `T`;
- conservative enclosure;
- provenance;
- conditioning classification;
- residual evidence;
- precision-ledger reference;
- tolerance disposition; and
- deterministic diagnostic encoding.

For each accepted cleanup action, it must produce a committed budget record containing:

- affected geometric lineages;
- authorized maximum;
- conservative realized displacement or feature-removal measure;
- cumulative cost after the action;
- local before/after evidence; and
- replay identity.

No downstream component may receive a bare constructed coordinate for topology-affecting use without its bounded metadata and identity.

## 4. Required invariants and prohibited behavior

Required invariants:

- every published enclosure contains the mathematical result under the qualified model and inherited source uncertainty;
- bounds are finite or the operation fails;
- bounds never shrink without a documented proof-producing operation;
- tolerance is never used as a universal equality test;
- a numerically uncertain sign is never silently coerced to positive or negative;
- symbolic ties remain distinguishable from uncertainty;
- precision propagation is monotonic across stages and repeated operations;
- cleanup cost is never under-reported; and
- deterministic execution produces identical nominal values, bounds, classifications, and ledger bytes.

Prohibited behavior:

- computing topology from raw unbounded floating-point comparisons;
- hiding near-zero denominators with arbitrary clamping;
- snapping unrelated features because they are within tolerance;
- accepting NaN/infinite intermediate values;
- depending on excess precision in registers;
- changing rounding mode during a transaction without restoring and verifying it;
- using approximate transcendental functions to choose authoritative orientation or ordering; or
- using an external interval, exact-arithmetic, or geometry library.

## 5. Test and validation specification

### 5.1 Bit-pattern known-answer tests

Provide committed expected outputs for operations involving:

- positive and negative zero;
- smallest and largest subnormals;
- smallest normals;
- adjacent representable values around 0, 1, and large magnitudes;
- maximum finite exponents where safe operations remain finite;
- cancellation-prone sums and differences;
- conversion between integer values and `T`; and
- contraction-sensitive expressions.

Expected nominal values, enclosure bits, classifications, and diagnostics must be stable on every supported platform profile.

### 5.2 In-tree exact oracle tests

Normative tests must include a small, deliberately slow, in-tree arbitrary-precision integer/rational oracle implemented in portable C++17. Production code must not depend on it.

For bounded integer-coordinate fixtures, compare exact results against published enclosures for:

- arithmetic primitives;
- dot and cross products;
- determinants;
- plane residuals;
- interpolation parameters;
- edge-plane intersections;
- projection parameters; and
- accumulated cleanup displacements.

Every exact value must lie inside the published enclosure.

### 5.3 Random property tests

Generate representable source values and exact rational templates, then verify:

- enclosure containment;
- monotonicity when input precision is increased;
- no bound shrinkage under repeated propagation;
- symmetry or operand-remapped symmetry where mathematically required;
- deterministic serialization;
- finite-result checks; and
- correct uncertain classification when exact values straddle a decision boundary.

### 5.4 Conditioning threshold tests

Construct line-plane, edge-face, and carrier-ordering cases:

- comfortably conditioned;
- just inside the allowed tolerance;
- exactly at the threshold;
- just beyond the threshold;
- denominator enclosure containing zero;
- coplanar;
- endpoint-tied; and
- affected by large translation with small local geometry.

Verify stable category transitions and typed failures without discontinuous arbitrary coordinate jumps.

### 5.5 Budget tests

Test:

- one displacement;
- several edits on disjoint lineages;
- several edits accumulating on one lineage;
- reservation rollback;
- exact budget boundary;
- budget exceeded by rounding of the bound itself;
- component-removal authorization; and
- disagreement between nominal and conservative realized displacement.

Mutation tests must reduce a recorded cost or omit one lineage contribution and require independent verification failure.

### 5.6 Repeated-chain tests

Build deterministic sequences of:

- exactly representable translations;
- power-of-two scaling;
- non-power-of-two affine transforms where supported;
- repeated Boolean outputs used as inputs; and
- cleanup followed by later construction.

Verify that precision never resets or decreases and that ordinary success stops when accumulated uncertainty exceeds tolerance.

### 5.7 Floating environment tests

Test execution with:

- altered rounding modes;
- compile configurations that request fast math;
- contraction enabled and disabled contrary to policy;
- flush-to-zero behavior where controllable;
- debug and optimized builds; and
- supported compiler families.

The component must either produce the qualified deterministic result or reject the environment before topology processing.

### 5.8 Broad-phase enclosure tests

For small fixtures, compare inflated bounds against exhaustive exact feature relations. Include one-ULP gaps, exact contact, subnormal separations, extreme scales, and combined source uncertainty. No true bounded interaction may be excluded.

### 5.9 Mutation and verifier tests

Corrupt bounded artifacts by:

- shrinking one axis error;
- removing inherited input precision;
- changing a conditioning category;
- reporting a finite bound around a non-finite nominal;
- reducing cumulative cleanup cost;
- changing a rounding-policy identifier; and
- assigning the wrong provenance.

Independent verification must reject every mutation.

### 5.10 Definition of done

Component 03 is complete only when:

- its arithmetic model is frozen and platform-qualified;
- every primitive operation has exact-oracle enclosure tests;
- every topology-affecting predicate has a non-boolean uncertainty-aware result;
- conditioning failures are typed and deterministic;
- precision and cleanup budgets propagate monotonically;
- broad-phase bounds show no false negatives in exhaustive bounded tests;
- replay and serialization are bit-stable; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
