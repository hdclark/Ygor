# Plan 03: Precision, Tolerance, and Bounded Arithmetic

## 0. Scope and non-negotiable constraints

Implement **only Component 03** from `component_03_precision_bounded_arithmetic.md`. This component establishes the immutable precision context and the bounded numerical services consumed by Components 02-15. It owns the qualified arithmetic model, scale-aware machine floors, source-value import, conservative scalar/vector/point/plane/parameter enclosures, exact-nominal tie evidence, conditioning classifications, precision-ledger records, tolerance-budget reservations/commits, conservative feature bounds, canonical encoding, and independent verification.

This component must not:

- validate source topology or shell semantics;
- choose source-facet triangulations;
- assign source, event, output-occurrence, edge, or face topology;
- select Boolean ownership or winding values;
- create adjacency from coordinate equality or proximity;
- perform cleanup mutations itself;
- publish an ordinary Boolean result;
- use tolerance as a universal equality test;
- silently clamp, snap, normalize, or substitute an arbitrary epsilon for a failed numerical construction;
- call `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- use `long double`, excess register precision, compiler-specific extended types, a change of rounding mode, or a transcendental function as an unqualified authoritative shortcut;
- use an external interval, exact-arithmetic, geometry, hashing, serialization, testing, or concurrency dependency; or
- compile production or normative-test code outside the strict C++17 floating-point target established by Component 01.

The V1 arithmetic policy must use IEC 60559 binary `float` or `double`, nearest-even arithmetic, fixed expression grouping, preserved signed-zero semantics, gradual underflow, and **prohibited floating contraction** for authoritative operations. Every primitive operation must be decomposed into explicitly sequenced scalar operations so the same operation graph is used by the producer, codec, replay, and verifier.

Component 03 must use Component 01 for owner tokens, strong IDs, checked integer/count arithmetic, typed outcomes/errors, strict-build qualification, resource accounting, cancellation, deterministic diagnostics, transactions, canonical bytes, SHA-256 digests, replay, and immutable artifact publication. No failed, cancelled, partially encoded, budget-rejected, or verifier-rejected precision artifact may escape its transaction.

Mark Component 03 complete in `tracker.md` only after Section 25 is fully satisfied.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Reuse unchanged only as narrow data carriers

Reuse:

- `vec3<T>` from `YgorMath.h` only as the nominal public/inter-component three-coordinate carrier;
- `fv_surface_mesh<T,I>` only indirectly through Component 01/02 immutable source descriptions;
- `std::array`, `std::vector`, `std::variant`, `std::optional`, `std::numeric_limits`, `<cmath>` classification functions, `<cstdint>`, `<cstring>`, and other C++17 standard-library facilities under the strict target and Component 01 resource contracts; and
- Component 01 `CanonicalBytes`, SHA-256, checked arithmetic, transaction, resource, cancellation, diagnostic, replay, and owner-validation services.

Do not use `vec3` arithmetic operators, `Dot`, `Cross`, `length`, `unit`, `distance`, `angle`, `operator<`, or `operator==` for an authoritative bounded result. Component 03 may copy nominal coordinates into or out of `vec3<T>`, but every topology-affecting calculation must pass through the bounded operation graph defined below.

### 1.2 Improve and reuse adaptive expansion arithmetic

`YgorMeshesAdaptivePredicates.h/.cc` already contains useful in-tree expansion-arithmetic primitives and adaptive `orient3d`/`insphere` algorithms. The existing implementation is not a Component 03 provider because it:

- returns a bare `T` or sign rather than an enclosure-aware typed predicate result;
- assumes exact source coordinates and does not propagate inherited precision;
- does not expose conditioning, provenance, operation traces, resource usage, or deterministic diagnostics;
- does not protect every splitter/product path against overflow, severe underflow, non-finite intermediate values, or unsupported floating environments;
- is compiled in the ordinary Ygor source target rather than necessarily in the strict Boolean target; and
- cannot distinguish exact-nominal ties from nonzero nominal values whose uncertainty crosses zero.

Do not duplicate this algorithm indefinitely. Perform the following refactor:

1. Add a strict-target internal provider `ExactFloatExpansion.h/.cc` under `src/YgorMeshesBooleanBounded/` containing audited finite-only expansion primitives, exponent-safe product decomposition, fixed-capacity expansion storage, exact nominal sign extraction, and operation/resource diagnostics.
2. Implement the provider from the same expansion-arithmetic approach already present in Ygor, preserving the existing mathematical intent while adding power-of-two exponent normalization where a direct split/product could overflow or underflow.
3. Keep `YgorMeshesAdaptivePredicates.h` source-compatible. Replace the implementation in `YgorMeshesAdaptivePredicates.cc` with thin compatibility wrappers that call the audited strict provider when the strict provider is linked. Preserve existing return conventions and add regression tests before changing the implementation.
4. Component 03 may use the audited provider **only** to establish the exact sign or exact zero of the prescribed nominal floating expression. It must still calculate the conservative uncertainty enclosure independently.
5. Do not use `insphere` unless a later Component 03 consumer explicitly requires it. Its compatibility path must remain tested, but it is not part of the minimum Component 03 production capability set.

If compatibility-wrapper integration would create a cyclic target dependency, keep the existing public implementation temporarily unchanged and compile the new strict provider as the Component 03 source of truth. In that case, add a tracked follow-up in code comments and tests; do not let Component 03 call the non-strict object.

### 1.3 Existing geometry classes are not authoritative bounded providers

The existing `line<T>`, `line_segment<T>`, and `plane<T>` types use normalized directions, square roots, bare booleans, and raw floating comparisons. Existing contour projection, least-squares plane fitting, point-in-polygon, epsilon comparison, duplicate-removal, and intersection helpers similarly do not carry inherited precision or proof-producing enclosures.

Therefore:

- do not use `plane<T>::Intersects_With_Line_Once`, `Intersects_With_Line_Segment_Once`, `Get_Signed_Distance_To_Point`, or `Project_Onto_Plane_Orthogonally` for authoritative work;
- do not construct a bounded plane by normalizing an existing `plane<T>` normal;
- do not use contour epsilon defaults, least-squares fitting, or coordinate-based cleanup helpers; and
- use existing geometry classes only for non-authoritative fixture creation or visualization in tests.

### 1.4 No suitable existing interval, budget, or precision-ledger implementation

Ygor does not currently provide an owner-bound finite interval type, deterministic outward-rounded arithmetic service, inherited/construction/cleanup uncertainty separation, transactional tolerance budget, precision-lineage DAG, or independent bounded-artifact verifier. Implement these as new bounded-Boolean subsystem code rather than broadening unrelated general-purpose math APIs.

Do not use `YgorSerialize`, `std::hash`, `External/SpookyHash`, or `External/MD5` for canonical precision artifacts. Use Component 01 canonical bytes and SHA-256.

### 1.5 Existing spatial indexes are consumers, not providers

Any existing R-tree, octree, KD-tree, or mesh bounding-box utility may be studied for fixture generation and performance baselines, but its raw coordinate comparisons and uninflated bounds are not Component 03 conservative evidence. Component 03 must publish a small finite closed-AABB abstraction that Component 06 can consume without depending on a legacy index implementation.

## 2. Exact file and target layout

Add these files under `src/YgorMeshesBooleanBounded/`:

- `PrecisionTypes.h` — stable enums, finite interval/value records, scale descriptors, operation codes, conditioning categories, uncertainty causes, and capability views;
- `PrecisionBootstrap.h/.cc` — source-bit scan, scale preflight, machine-floor preflight, and Component 01 freeze handshake;
- `PrecisionContext.h/.cc` — immutable `precision_context<T>`, owner/version validation, read-only service views, and construction entrypoint;
- `FloatingBits.h` — C++17 `memcpy`-based scalar bit conversion, total-order keys, finite successor/predecessor, signed-zero handling, and exact power-of-two scaling helpers;
- `OutwardArithmetic.h/.cc` — finite closed-interval arithmetic, explicit rounding points, fixed expression grouping, overflow/underflow checks, and primitive operation traces;
- `ExactFloatExpansion.h/.cc` — audited exact-nominal expansion arithmetic and determinant sign/tie service;
- `BoundedValues.h` — immutable bounded scalar, vector, point, plane, parameter, residual, and AABB schemas with checked accessors;
- `BoundedOperations.h/.cc` — scalar/vector arithmetic, interpolation, dot, cross, squared-norm, affine projection, plane construction, residual, determinant, parameter, and conversion operations;
- `PredicateResults.h/.cc` — five-way predicate outcomes, exact-nominal tie integration, margins, recommended dispositions, and diagnostic evidence;
- `ConstructionConditioning.h/.cc` — edge-plane, line/edge carrier, endpoint/interior, projection, residual, and tolerance sufficiency classification;
- `PrecisionLedger.h/.cc` — append-only operation/precision lineage, immutable committed snapshots, aggregate output-precision queries, and verifier-facing records;
- `ToleranceBudget.h/.cc` — proposal validation, reservation, rollback, commit, per-lineage cumulative accounting, feature/component-removal authorization, and snapshots;
- `ConservativeBounds.h/.cc` — finite closed AABBs, uncertainty inflation, union, intersection, closed overlap, and source/constructed feature bounds;
- `PrecisionCodec.h/.cc` — canonical encoding/decoding of contexts, values, traces, ledger snapshots, budget records, and construction evidence;
- `PrecisionVerifier.h/.cc` — independent reconstruction and mutation rejection for all published Component 03 artifacts; and
- `PrecisionCapabilities.h` — narrow owner-checked interfaces exported to later components.

Extend `ContractVersions.h` and the Component 01 stage/checkpoint/subcode registries; do not create a second version registry.

Extend `tests/mesh_boolean_bounded/` with:

- `TestPrecisionPlatform.cc`;
- `TestFloatingBits.cc`;
- `TestOutwardArithmeticKnownAnswers.cc`;
- `TestOutwardArithmeticOracle.cc`;
- `TestBoundedVectorPlane.cc`;
- `TestPredicateResults.cc`;
- `TestConstructionConditioning.cc`;
- `TestPrecisionLedger.cc`;
- `TestToleranceBudget.cc`;
- `TestConservativeBounds.cc`;
- `TestPrecisionCodecReplay.cc`;
- `TestPrecisionMutation.cc`;
- `TestPrecisionProperties.cc`;
- `TestPrecisionAdversarial.cc`;
- `PrecisionFixtures.h/.cc`;
- `PrecisionExactOracle.h/.cc`; and
- `GoldenPrecisionV1.h`.

Register separate CTest cases for platform, bit operations, known answers, exact-oracle arithmetic, bounded geometry, predicates, conditioning, ledger, budget, bounds, codec/replay, mutation, property, and adversarial suites. Apply `ygor_apply_mesh_boolean_strict_fp` to every production and normative-test target.

## 3. Stable versions, operation codes, stages, and failure subcodes

### 3.1 Version registry

Add explicit nonzero V1 constants for:

- precision bootstrap schema/provider;
- scalar bit/total-order provider;
- arithmetic model;
- outward arithmetic provider;
- exact-float expansion provider;
- bounded scalar/vector/point/plane/parameter/residual/AABB schemas;
- operation-trace schema;
- predicate-result schema;
- construction-conditioning schema/provider;
- precision-ledger schema/provider;
- tolerance-budget schema/provider;
- conservative-bounds provider;
- precision canonical encoding; and
- precision verifier.

Unknown required versions, zero versions, unsupported enum values, or nonzero reserved fields are typed failures. Include all versions in context bytes, construction bytes, ledger/budget snapshots, replay, and verifier checks.

### 3.2 Stable operation codes

Define fixed-width explicit values for every arithmetic node. At minimum include:

- source import;
- exact scalar constant import;
- add, subtract, negate, multiply, divide;
- explicit multiply-then-add and multiply-then-subtract;
- scalar minimum/maximum/hull/intersection;
- vector add/subtract/scale;
- interpolation from endpoint A and interpolation from endpoint B;
- dot3 with prescribed grouping;
- cross3 with prescribed grouping;
- squared norm;
- coordinate/radial envelope derivation;
- affine projection;
- plane from three points;
- plane residual;
- 2x2 and 3x3 determinant;
- carrier parameter projection;
- finite AABB construction/inflation/union/intersection; and
- conversion/rounding to `T`.

Do not serialize compiler-generated type names or function pointers. Operation codes and formula IDs are the replay authority.

### 3.3 Component 03 logical stages and checkpoints

Use stable checkpoints in this order:

1. pending-context/platform capability validation;
2. source-bit/count preflight;
3. operand/global scale scan;
4. machine-floor derivation;
5. input-precision normalization and tolerance eligibility;
6. precision preflight encoding/digest;
7. frozen-context handshake validation;
8. immutable precision-context construction;
9. source bounded-value import;
10. primitive operation execution;
11. exact-nominal sign/tie evaluation when requested;
12. predicate result assembly;
13. construction conditioning/residual evaluation;
14. precision-ledger append/finalization;
15. tolerance reservation;
16. tolerance commit or rollback;
17. conservative feature-bound publication;
18. independent artifact verification;
19. canonical encoding/digest/resource finalization; and
20. pre-publication cancellation and transaction commit.

Do not renumber released checkpoints. Later optional checks require reserved values or a version change.

### 3.4 Required Component 03 subcodes

Allocate a disjoint Component 03 subcode range and define at least:

- unsupported scalar format;
- scalar descriptor mismatch;
- strict-build marker missing;
- nearest-even rounding unavailable or changed;
- contraction policy violated;
- signed-zero behavior unsupported;
- subnormal/gradual-underflow behavior unsupported;
- arithmetic known-answer mismatch;
- non-finite source nominal;
- non-finite declared precision/tolerance;
- negative precision/tolerance;
- source scale unavailable;
- machine floor not finite/representable;
- effective input precision exceeds tolerance;
- precision bootstrap/frozen-context mismatch;
- wrong/stale owner or provider version;
- invalid or inverted interval;
- nominal outside enclosure;
- outward successor/predecessor unavailable;
- finite enclosure overflow;
- arithmetic intermediate non-finite;
- division denominator enclosure contains zero;
- unsupported operation/formula code;
- exact expansion capacity exceeded;
- exact expansion non-finite/scaling failure;
- exact-nominal sign conflicts with definite enclosure;
- predicate evidence incomplete or inconsistent;
- plane support degenerate or uncertain;
- parameter interval invalid;
- construction residual exceeded;
- construction ill-conditioned;
- construction uncertainty exceeds tolerance;
- precision-ledger ID/count/byte overflow;
- precision-ledger cycle, forward reference, or owner mismatch;
- precision contribution omitted or reduced;
- budget proposal malformed;
- duplicate/overlapping budget lineage entry;
- reservation exceeds available tolerance;
- reservation owner/transaction mismatch;
- actual cost exceeds reservation;
- cumulative lineage cost exceeds tolerance;
- feature/component removal not authorized;
- reservation commit-order mismatch;
- reservation leaked at transaction finalization;
- AABB invalid/inverted/non-finite;
- AABB inflation not representable;
- codec unknown version/operation/truncated record;
- canonical digest mismatch;
- precision verifier rejection; and
- deterministic bytes/replay mismatch.

Map unsupported execution behavior to `unsupported_platform`; invalid caller precision/tolerance to `invalid_tolerance`; ill-conditioned constructions to `geometric_condition_exceeds_tolerance`; cleanup accounting failures to `cleanup_budget_exceeded`; representability/count failures to `index_overflow` or `resource_limit` as appropriate; cancellation to `cancelled`; and producer/verifier contradictions to `internal_invariant_error`.

## 4. Component 01 bootstrap and freeze handshake

Component 01 needs the machine floor before it can finish validating tolerance, while Component 03 needs the frozen context owner and policy before it can publish services. Resolve this dependency with an explicit two-phase handshake; do not use a partially initialized global precision singleton.

### 4.1 Preflight input

Implement:

```cpp
template<class T>
boolean_outcome<precision_preflight<T>>
preflight_precision(
    const pending_boolean_context_view<T>& pending,
    const immutable_source_coordinate_bits<T>& operand_a,
    const immutable_source_coordinate_bits<T>& operand_b,
    const precision_bootstrap_capabilities& capabilities);
```

The preflight receives exact source coordinate bit patterns, normalized policy scalars that have passed Component 01's preliminary finite/sign checks, the selected V1 arithmetic/contraction/enclosure policy, strict-build/platform evidence, resources, cancellation, diagnostics, and a pending-context identity that cannot be used as a final owner token.

It must:

- validate all counts before reading coordinate arrays;
- reject non-finite source bit patterns defensively;
- compute operand-local and global scale descriptors;
- compute per-coordinate and aggregate machine-floor requirements;
- preserve declared input precision separately from the machine floor;
- derive effective source precision as the conservative maximum of declared precision and applicable machine floor;
- determine whether ordinary success remains eligible under the caller tolerance;
- encode the complete preflight result canonically; and
- return a digest bound to exact source bits, options, arithmetic policy, and platform conformance evidence.

### 4.2 Context freeze

Component 01 incorporates the preflight digest, effective input precisions, scale summary, and ordinary-success eligibility into the frozen Boolean context. Component 01 remains the authority that rejects invalid options and publishes the final owner token.

### 4.3 Precision context construction

Implement:

```cpp
template<class T>
boolean_outcome<artifact_handle<const precision_context<T>>>
build_precision_context(
    const precision_preflight<T>& preflight,
    const frozen_boolean_context_view<T>& context,
    const precision_runtime_capabilities& capabilities);
```

Recompute and compare the preflight digest and every frozen scalar/policy field. Reject stale preflight objects, different source descriptions, different owner identities, or policy/version drift. Publish an immutable context only after independent verification.

## 5. Qualified V1 arithmetic model

### 5.1 Compile-time requirements

For supported `T`, require and encode:

- `std::is_same<T,float>` or `std::is_same<T,double>`;
- `std::numeric_limits<T>::is_iec559 == true`;
- radix 2;
- expected storage width, significand digits, exponent ranges, infinity, quiet NaN, and denormal support;
- exact `uint32_t`/`uint64_t` bit carrier availability;
- `YGOR_MESH_BOOLEAN_STRICT_FP_BUILD == 1`;
- absence of `__FAST_MATH__`; and
- the V1 no-contraction compile policy.

Do not accept a type merely because it has the same byte width.

### 5.2 Runtime conformance

At context creation and in each Component 17 worker qualification, verify:

- `fegetround() == FE_TONEAREST`;
- basic known-answer additions, subtractions, multiplications, divisions, signed-zero operations, and underflow transitions have expected bits;
- subnormals are not flushed to zero and are not treated as zero on input;
- negative zero survives source-bit round trip and required arithmetic cases;
- the prescribed multiply-then-add expression is not contracted;
- scalar stores occur at the documented `T` boundaries; and
- `FloatingBits` successor/predecessor outputs agree with committed known answers.

The component must never set the process rounding mode. A mismatch returns `unsupported_platform`. The worker check is read-only and must be cheap enough to run once per worker, with the complete conformance suite retained for startup/tests.

### 5.3 Prescribed grouping

Document every authoritative expression as an operation graph. For V1:

- scalar operations round to `T` after each `+`, `-`, `*`, or `/` node;
- `a*b+c` is always `round_T(round_T(a*b)+c)`;
- dot3 is `round_T(round_T(round_T(ax*bx)+round_T(ay*by))+round_T(az*bz))`;
- cross components use one rounded product, one rounded product, then one rounded subtraction;
- 3x3 determinants use a versioned fixed expansion/grouping; and
- producer and verifier do not substitute algebraically equivalent formulas unless a recorded construction formula ID explicitly allows it.

## 6. Floating bit utilities and finite outward stepping

### 6.1 Bit conversion

Implement C++17 `to_bits`/`from_bits` with `std::memcpy`, `static_assert` size checks, and no union type punning. Preserve all finite bit patterns, including signed zero and subnormals. NaN payloads are never accepted as value inputs.

### 6.2 Total numeric order

Provide a stable total-order key for finite values used only for interval validation, canonical sorting, and deterministic diagnostics. The order must place negative finite values before `-0`, then `+0`, then positive finite values. Do not use this order to infer geometric identity.

### 6.3 Successor and predecessor

Implement bitwise `next_up_finite(T)` and `next_down_finite(T)` rather than relying on library `nextafter` in production. Required cases include:

- both signed zeros;
- smallest/largest subnormal;
- normal/subnormal boundaries;
- positive and negative normals;
- maximum finite values; and
- exact rejection when the next outward value would be infinite.

`next_up_finite(-0)` and `next_up_finite(+0)` produce the smallest positive subnormal. `next_down_finite(-0)` and `next_down_finite(+0)` produce the largest-magnitude negative subnormal nearest zero. Interval canonicalization may represent an exact zero interval as `[-0,+0]`, but source nominal signed zero is preserved separately.

Use `std::nextafter` only as a non-authoritative test comparison.

### 6.4 Exact power-of-two scaling

Provide checked `frexp`/`ldexp`-style helpers or direct exponent-bit adjustment for finite power-of-two scaling. Every use must detect overflow, underflow that loses required nonzero information, and unsupported subnormal behavior. These helpers are used to normalize exact-expansion determinants and conditioning calculations without changing sign.

## 7. Core finite interval and bounded-value representations

### 7.1 Closed finite interval

Use an immutable conceptual schema:

```cpp
template<class T>
struct finite_interval {
    T lower;
    T upper;
};
```

Required invariants:

- both endpoints are finite;
- `lower <= upper` under the finite total numeric order;
- exact zero is canonicalized to `[-0,+0]` for interval endpoints;
- no NaN, infinity, empty interval, open endpoint, or reversed interval is representable in a published object; and
- interval construction is available only through checked factories.

Do not expose writable endpoints to consumers.

### 7.2 Bounded scalar

A published `bounded_scalar<T>` contains:

- nominal `T` bits;
- finite enclosure;
- owner token;
- stable `bounded_value_id`;
- provenance ID;
- precision-ledger entry ID;
- operation-trace ID;
- value state/version; and
- uncertainty-cause summary.

The nominal must be finite and lie inside the enclosure. A scalar with invalid provenance, missing ledger evidence, or inconsistent owner cannot cross a component boundary.

### 7.3 Vector and point

A `bounded_vec3<T>` contains three bounded scalar components and one derived conservative radial upper bound. A `bounded_point3<T>` additionally identifies point provenance/lineage and preserves the nominal `vec3<T>` carrier.

Derive per-axis error as the outward-rounded maximum of `nominal-lower` and `upper-nominal`. Derive the V1 radial bound with a conservative L1 sum of axis-error upper bounds; do not require a square root. Later consumers may compare squared bounds through a tighter optional query, but the stored radial bound must remain conservative.

### 7.4 Plane

Represent a plane without normalization:

```cpp
template<class T>
struct bounded_plane3 {
    bounded_vec3<T> normal;
    bounded_scalar<T> offset; // normal dot x + offset = 0
    bounded_point3<T> anchor;
    bounded_scalar<T> normal_sq;
    provenance_id provenance;
    precision_ledger_entry_id ledger;
};
```

The normal must have a definitely positive squared-norm lower bound. Store an anchor so residual plausibility can be independently checked. Do not store only a unit normal or divide by its length.

### 7.5 Parameter, residual, and AABB

- `bounded_parameter<T>` stores a bounded scalar, carrier provenance, endpoint convention, and domain relation to `[0,1]` or an unbounded carrier.
- `bounded_residual<T>` stores the residual scalar, scale metadata, and pass/fail/uncertain comparison evidence.
- `bounded_aabb3<T>` stores three finite closed intervals, source/constructed feature provenance, inflation contributors, and ledger reference.

## 8. Machine floor and scale model

### 8.1 Scale descriptors

The preflight must record, separately for each operand and globally:

- per-axis minimum and maximum finite coordinates;
- per-axis maximum absolute coordinate;
- maximum absolute coordinate overall;
- finite coordinate span per axis using bounded subtraction;
- whether all coordinates on an axis are bit-identical;
- minimum nonzero representable coordinate magnitude observed;
- normal/subnormal/signed-zero presence;
- source count and exact source-bit digest; and
- a power-of-two normalization exponent usable by exact nominal determinants.

Do not define the machine floor only as `epsilon * bounding_box_diagonal`. Large translations with small local features must be handled by operation-local intervals around the actual operands.

### 8.2 Source coordinate floor

For each nominal source coordinate `x`, define a conservative representation floor using at least the adjacent finite values around `x`. The initial source enclosure must contain both:

- the caller-declared inherited uncertainty around the stored nominal coordinate; and
- the applicable one-step representation/roundoff floor.

Construct it with outward interval subtraction/addition, not `x ± p` without widening. Record declared input precision and machine-floor contribution separately in the ledger.

### 8.3 Aggregate floor and tolerance eligibility

The operand aggregate machine floor is the maximum conservative radial source floor over imported source points plus any required source-format conversion bound. The global floor is the maximum of operand floors and context scalar floors.

Ordinary success is eligible only if:

- effective input precision for each operand is no greater than tolerance;
- all required finite source enclosures are representable; and
- the caller tolerance is at least the global machine floor.

A topology-only diagnostic context may retain an ineligible precision context, but every construction and artifact must remain marked ineligible for ordinary publication.

## 9. Outward-rounded primitive arithmetic

### 9.1 General rule

Each interval operation must enclose all exact real results for every value in the input intervals under inherited source uncertainty. Evaluate endpoint expressions in nearest-even `T`, then step the lower endpoint outward with `next_down_finite` and the upper endpoint outward with `next_up_finite` at every rounded arithmetic node. If a required outward step is not finite, fail; do not saturate.

Every operation validates owner, versions, finiteness, interval order, nominal containment, ledger references, resource reservation, cancellation checkpoint, and operation-code/formula compatibility before evaluating.

### 9.2 Addition, subtraction, and negation

- Addition enclosure: outward `a.lower+b.lower` and `a.upper+b.upper`.
- Subtraction enclosure: outward `a.lower-b.upper` and `a.upper-b.lower`.
- Negation swaps/negates endpoints with signed-zero canonicalization.
- Nominal values use the prescribed single scalar operation.

### 9.3 Multiplication

Compute all four endpoint products through the rounded-and-outward scalar product primitive. Select minimum and maximum by total numeric order. Do not assume sign categories without first proving them from the interval. Reject any non-finite product or outward overflow.

### 9.4 Division

If the denominator interval contains either signed zero or straddles zero, return an invalid/uncertain division result with the required disposition; do not clamp the denominator. Otherwise compute the reciprocal interval with outward division and multiply, or compute four quotients directly according to one recorded V1 formula. The producer and verifier must use the same formula ID.

### 9.5 Constants and exact integers

Import zero, one, powers of two, and representable checked integers as exact singleton intervals only when exact representability is proven. Integer-to-`T` conversion must use Component 01 checked range logic and a round-trip or exact-bit proof. Non-exact integer conversion produces a widened bounded scalar with an explicit conversion contribution.

### 9.6 Hull and intersection

- Hull returns the smallest finite closed interval containing both inputs and records both contributors.
- Interval intersection is a proof-producing narrowing operation allowed to shrink a bound only when a separately justified constraint guarantees the true value lies in both intervals.
- Empty or uncertain intersections are typed failures or explicit no-intersection outcomes; never silently choose one input.
- Every narrowing record stores the proof kind and both parent intervals so the verifier can reproduce it.

## 10. Vector, point, plane, and determinant operations

### 10.1 Vector operations

Implement vector add/subtract and scalar scale component-wise through bounded scalar operations. A vector operation creates one parent operation record plus component child records; all share a canonical trace root.

### 10.2 Dot product

Use the V1 grouping from Section 5.3. The interval path applies bounded multiply and add nodes in the same order. Record all three product contributions and both add contributions. Do not call `vec3::Dot`.

### 10.3 Cross product

Compute each component with two bounded products and one bounded subtraction in the fixed coordinate order. Record component formula IDs. Do not call `vec3::Cross`.

### 10.4 Squared norm and norm bounds

Squared norm uses bounded dot of a vector with itself and must have a nonnegative true range. If interval roundoff produces a negative lower endpoint, a proof-producing intersection with `[0,+max_finite]` may raise it to zero. Store that narrowing proof.

Topology decisions must use squared quantities wherever possible. A radial upper bound may use the L1 bound. If a square-root interval is later required for diagnostics, qualify and outward-widen `sqrt` separately and do not make it the sole authority for topology.

### 10.5 Plane construction

Construct a plane from accepted points `a,b,c` as:

1. `u = b-a`;
2. `v = c-a`;
3. `n = cross(u,v)`;
4. `normal_sq = dot(n,n)`;
5. require `normal_sq.lower > 0` for a definitely non-collinear plane;
6. `offset = -dot(n,a)`; and
7. calculate anchor residual `dot(n,a)+offset` through a separately recorded residual path.

If exact nominal expansion proves zero determinant/area while the interval is nonzero only due to widening, return tied/degenerate evidence rather than inventing a normal. If the interval contains zero and the exact nominal is nonzero, return uncertain/ill-conditioned.

### 10.6 Plane residual and affine projection

Residual is `dot(normal,point)+offset` with the fixed grouping. Orthogonal projection, when required, is `point - normal * residual/normal_sq` and requires a denominator interval definitely separated from zero. Store both nominal and residual-after-projection evidence.

### 10.7 Determinants

Provide bounded 2x2 and 3x3 determinant operations with fixed expression graphs. In parallel, the exact-expansion provider may establish exact nominal sign/zero after power-of-two normalization. The exact sign is evidence about the prescribed nominal expression, not a replacement for inherited-uncertainty bounds.

## 11. Predicate result model and exact-nominal ties

### 11.1 Stable categories

Use:

```cpp
enum class bounded_relation : std::uint8_t {
    definitely_negative = 1,
    exact_nominal_tie = 2,
    definitely_positive = 3,
    uncertain = 4,
    invalid = 5,
};
```

The result also stores a recommended disposition:

- accept numeric sign;
- invoke symbolic policy;
- retry the one permitted alternate bounded formula;
- handle as coplanar/coincident relation;
- fail for condition/tolerance; or
- fail invalid.

### 11.2 Classification rules

For a zero comparison:

- `upper < 0` => definitely negative;
- `lower > 0` => definitely positive;
- interval contains zero and exact nominal expansion/sign is exactly zero => exact nominal tie;
- interval contains zero and exact nominal sign is nonzero => uncertain;
- invalid interval, non-finite evidence, missing exact-sign evidence when required, or sign/enclosure contradiction => invalid/internal failure.

An exact nominal tie may still carry inherited uncertainty spanning both signs. The result must explicitly record whether the tie is structurally eligible for Component 07 symbolic handling, whether source uncertainty also permits separated geometries, and whether the caller tolerance is sufficient. Component 07, not Component 03, chooses the symbolic owner.

### 11.3 Margin and uncertainty causes

For definite results, record an outward-rounded lower bound on separation from zero. For tied/uncertain results, record the interval width and contributor summary. Causes must distinguish at least:

- inherited operand A;
- inherited operand B;
- machine roundoff;
- conditioning amplification;
- conversion/serialization;
- prior cleanup displacement; and
- current cleanup proposal.

### 11.4 Compute-once trace

A predicate result has one canonical producer and immutable operation-trace ID. Consumers receive the result or a handle, not the raw inputs plus permission to recompute. Alternate formulas are permitted only within the original producer call, in a fixed precedence, with both attempts recorded under one trace root.

## 12. Construction conditioning

### 12.1 Common construction evidence

Every construction result contains:

- construction formula ID;
- source/carrier/plane provenance;
- nominal value and enclosure;
- denominator interval and separation evidence;
- parameter interval;
- endpoint/interior/domain classification;
- coordinate cancellation indicators;
- source-edge/carrier residuals;
- plane residual;
- final-rounding contribution;
- ledger entry;
- available tolerance before construction;
- required conservative precision after construction; and
- typed conditioning category/disposition.

### 12.2 Edge-plane intersection

Given endpoint points `p0,p1` and plane residuals `r0,r1`:

1. evaluate each residual once;
2. classify exact endpoint ties from exact nominal residual evidence;
3. compute denominator `r0-r1` once;
4. reject/route to coplanar handling when the denominator interval contains zero and both endpoint residuals are tied/uncertain in the coplanar pattern;
5. choose one of two V1 formulas deterministically:
   - from endpoint 0: `t = r0/(r0-r1)`;
   - from endpoint 1: `t = 1 - r1/(r1-r0)`;
6. choose the endpoint formula with the smaller conservative absolute residual upper bound; break equality by canonical endpoint ID;
7. interpolate with the matching endpoint formula;
8. recompute bounded residuals of the resulting point against the source segment carrier and plane; and
9. classify against tolerance.

Both formulas must share the already computed residuals; they are not independent consumer recomputations.

### 12.3 Parameter/domain classification

Classify:

- stable interior only when the entire parameter interval lies strictly inside `(0,1)` with a positive bounded margin;
- stable endpoint when exact nominal evidence ties to `0` or `1` and residuals are acceptable;
- exact/symbolic tie when nominal relation is exact zero under the prescribed formula;
- coplanar/coincident when the plane relation requires Component 07 policy;
- near-parallel within tolerance when the denominator is small but separated from zero and all coordinate/residual bounds remain within tolerance;
- ill-conditioned beyond tolerance when the resulting bound, residual, or amplification exceeds available tolerance; and
- invalid for non-finite, owner, provenance, or operation-graph failures.

Do not classify an interval merely because its nominal `t` lies in `[0,1]`.

### 12.4 Tolerance sufficiency

A construction is eligible for ordinary use only when its aggregate precision contribution, inherited precision, and already committed cleanup displacement remain within caller tolerance. Return `geometric_condition_exceeds_tolerance` before publishing a bare nominal coordinate.

## 13. Precision ledger

### 13.1 Ledger architecture

Implement an append-only transaction-local DAG. Each entry has:

- stable ledger entry ID and owner;
- schema/provider version;
- operation/formula code;
- result bounded-value ID;
- ordered parent entry IDs;
- source operand/provenance contributors;
- exact nominal bits;
- result enclosure bits;
- inherited precision from each operand;
- machine-floor contribution;
- construction contribution;
- conditioning margin/category;
- symbolic/tie marker;
- prior and current cleanup contributions;
- aggregate precision upper bound;
- within-tolerance eligibility;
- operation-trace ID;
- resource/cancellation checkpoint; and
- canonical record digest contribution.

Parent IDs must precede child IDs in canonical committed order. No cycle, forward reference, wrong owner, missing parent, or duplicate ID is permitted.

### 13.2 Monotonic aggregation

For every entry:

```text
aggregate_precision >= inherited_precision_a
aggregate_precision >= inherited_precision_b
aggregate_precision >= machine_floor_contribution
aggregate_precision >= construction_contribution
aggregate_precision >= cumulative_cleanup_displacement
aggregate_precision >= each parent aggregate required by the operation proof
```

Use conservative addition for sequential error terms when cancellation cannot be proven. Use maximum only for genuinely alternative/disjoint contributors under an explicit operation rule. V1 defaults to sum for sequential same-lineage construction/displacement and maximum for a global summary across independent output lineages.

No bound may shrink unless the entry is an explicit proof-producing interval intersection/narrowing operation whose proof is verifier-reconstructible.

### 13.3 Snapshot and publication

Later components receive immutable ledger entry handles and read-only snapshot views. They cannot modify an existing entry. Cleanup adds new entries referencing prior geometry and committed budget records; it never edits historical uncertainty.

The global output precision query returns the conservative maximum aggregate precision over all published output geometric lineages, not merely the largest individual arithmetic node.

## 14. Tolerance-budget service

### 14.1 Separation from uncertainty

The budget service must not count existing input or construction uncertainty as if geometry moved. It records separately:

- no-motion uncertainty;
- actual vertex displacement;
- local feature-removal size;
- cumulative displacement along each geometric lineage;
- component-removal authorization/cost; and
- global caller tolerance.

### 14.2 Proposal schema

A cleanup consumer submits a canonical proposal containing:

- proposal/replay identity;
- operation kind;
- ordered affected lineage IDs;
- proposed maximum displacement per lineage;
- proposed feature-removal measure;
- optional component-removal request;
- local before evidence;
- proof that the proposal is topology-authorized by the cleanup component;
- transaction/stage owner; and
- canonical merge key.

Component 03 validates arithmetic/budget facts, not cleanup topology correctness.

### 14.3 Reservation

Reservation must:

- validate all lineages and reject duplicates;
- reserve worst-case requested cost before mutation;
- conservatively add requested sequential cost to each affected lineage's committed cumulative cost;
- reject if any lineage or required feature/component budget exceeds tolerance/policy;
- reserve Component 01 resources for the eventual record; and
- return a non-copyable owner-bound reservation token.

A reservation changes no published geometry and is released automatically on rollback/cancellation/failure.

### 14.4 Commit

Commit receives actual conservative after-evidence and must prove:

- actual per-lineage cost is finite, nonnegative, and no greater than reserved;
- the resulting point lies within the recorded displacement enclosure;
- actual feature/component removal is no greater than authorized;
- the topology mutation transaction succeeded; and
- commit order follows canonical proposal order.

For V1, sequential displacement on the same lineage combines by outward-rounded sum. Disjoint lineages retain separate cumulative totals; global maximum realized displacement is their maximum. Do not reduce cost because nominal displacement vectors partially cancel unless a future proof-producing vector-path schema is versioned and independently verified.

### 14.5 Finalization

A stage cannot commit while a reservation remains active. Every committed cleanup budget record creates a corresponding precision-ledger entry. Mutation tests must show that omitting a lineage, reducing an actual cost, or committing without a reservation is rejected.

## 15. Conservative feature bounds

### 15.1 Finite closed AABB rules

Build AABBs from bounded coordinate intervals, not nominal coordinates plus an ad hoc epsilon. All overlap tests are closed: touching at an endpoint is overlap. No true interaction permitted by the input enclosures may be pruned.

### 15.2 Required constructors

Provide:

- source/constructed point AABB from coordinate intervals;
- edge AABB as coordinate-wise hull of endpoint intervals;
- triangle AABB as hull of three point intervals;
- shell/operand AABB as checked canonical union;
- event/carrier AABB when later requested; and
- explicit inflation by an additional bounded uncertainty/displacement contribution.

### 15.3 Overflow policy

Every lower/upper expansion uses finite outward stepping. If a conservative finite bound is not representable, return a typed numerical failure; never substitute infinity and never prune the feature.

### 15.4 Queries

Expose closed overlap, definitely separated, enclosure union, proof-producing intersection, and conservative squared-distance lower-bound queries. Keep candidate ordering and acceleration ownership in Component 06.

## 16. Repeated operations, transforms, and public precision import

### 16.1 Re-ingestion

When a prior Boolean output is used as input, accept its published precision metadata only through a versioned verified import record. Preserve:

- prior output precision;
- source/inherited contributions;
- construction history digest;
- cumulative cleanup displacement;
- serialization/rounding contribution; and
- previous owner/replay lineage as foreign provenance.

Never reset uncertainty because public coordinates are exactly representable in `T`.

### 16.2 Affine transforms

Provide precision propagation helpers for transforms accepted by the broader Ygor integration:

- translation: transform nominal coordinates with bounded addition and preserve existing uncertainty;
- power-of-two scaling: exact nominal scale where representable, while scaling all uncertainty and cleanup bounds by absolute scale;
- general finite affine linear maps: bounded matrix-vector operations with fixed grouping and conservative operator contribution; and
- unsupported/non-finite/projective transforms: typed failure.

Translations with large magnitude must add operation-local roundoff even when the translation is exactly representable.

### 16.3 Tolerance gate

Before ordinary publication in a repeated chain, require the new aggregate output precision to remain within the new caller tolerance. A larger tolerance does not retroactively authorize unrecorded prior cleanup.

## 17. Canonical encoding, diagnostics, and replay

### 17.1 Encoding rules

Use Component 01 `CanonicalBytes`; never serialize raw structs or padding. Encode:

- schema/provider versions;
- owner/context identities;
- exact `T` bit patterns for nominal values and interval endpoints;
- stable enum/operation/formula values;
- ordered parent IDs and provenance IDs;
- scale descriptors;
- predicate/conditioning evidence;
- ledger records in parent-before-child canonical order;
- budget records in canonical proposal order;
- AABB records; and
- resource/cancellation summaries.

Preserve nominal signed zero. Canonicalize interval endpoint zeros according to Section 7.1. Encode no locale-dependent text.

### 17.2 Diagnostics

Every typed numerical failure must contain, within policy limits:

- component/stage/checkpoint/subcode;
- context owner and relevant feature/provenance IDs;
- operation/formula ID;
- nominal operand/result bits;
- input/output intervals;
- denominator/margin/residual evidence where applicable;
- inherited, construction, and cleanup contributors;
- tolerance and available budget;
- arithmetic/platform profile digest; and
- deterministic replay identity.

Do not log from core arithmetic. The eventual API boundary may render a completed error.

### 17.3 Replay

Replay must reconstruct the same operation graph, exact nominal bits, intervals, predicate category, conditioning class, ledger bytes, budget decision, and primary error under the same supported platform profile. Thread count and traversal order must not change bytes.

## 18. Independent verifier

### 18.1 Independence requirements

`PrecisionVerifier` must consume only immutable public/inter-component records, context/capability versions, exact bits, operation codes, and parent references. It must not trust producer-owned booleans such as `within_tolerance`, `definite`, `committed`, aggregate counts, or digests as sole evidence.

### 18.2 Reconstruction

The verifier must:

- revalidate owner/version/scalar/platform metadata;
- reconstruct every finite interval operation from parent intervals and operation/formula code;
- recompute prescribed nominal operations and exact nominal signs/ties;
- verify nominal containment and finite outward endpoints;
- verify every ledger parent and monotonic contribution;
- recompute aggregate output precision;
- rebuild per-lineage budget totals from committed records;
- verify reservation/commit evidence and absence of active reservations;
- rebuild AABBs from bounded points/features;
- re-encode canonical bytes and compare SHA-256 digests; and
- reject unknown, truncated, duplicated, reordered, or contradictory records.

The verifier may share `FloatingBits` and canonical-byte primitives, but it must have a separate record traversal and operation-dispatch implementation. It must not call a producer method that simply returns the stored result.

### 18.3 Mutation rejection

Required mutations include shrinking any interval endpoint, removing one inherited contributor, changing exact-tie evidence, altering an operation/formula ID, reordering parents, reducing a budget cost, omitting a lineage, changing a rounding/contraction profile, substituting a wrong provenance/owner, corrupting a residual, widening a parameter domain classification without evidence, and changing one digest byte. Every mutation must be rejected deterministically.

## 19. Resources, cancellation, transactions, and concurrency boundaries

### 19.1 Resources

Preflight and reserve Component 01 resources for:

- source-coordinate scans;
- bounded values;
- operation traces;
- exact-expansion limbs/work;
- ledger entries and parent references;
- budget proposals/reservations/records;
- AABBs;
- diagnostics;
- canonical bytes; and
- verifier work.

All count-to-byte and count arithmetic uses Component 01 checked helpers. Exact expansion storage is fixed-capacity per operation; capacity exhaustion is a typed resource/conditioning failure, not a heap overrun.

### 19.2 Cancellation

Check cancellation at stable loop/checkpoint boundaries:

- source scan chunks;
- ledger batch append;
- exact-expansion fallback entry;
- large codec/replay batches;
- budget reservation/commit boundaries; and
- verifier record batches.

Do not check cancellation in the middle of a scalar primitive after mutation of shared state. Local pure arithmetic may finish, then discard its result at the next checkpoint.

### 19.3 Transactions

Precision context construction, source imports, construction batches, ledger snapshots, budget commits, and encoded artifacts are transaction-owned until independently verified. Rollback destroys local values and releases all reservations/resources. Publication is one immutable handle or one typed failure.

### 19.4 Component 17 boundary

Implement an executable serial semantic reference first. Parallel workers may create private bounded values, traces, ledger fragments, and budget proposals against immutable inputs. They must not commit shared ledger IDs or budgets directly. Component 17 canonically merges private fragments, assigns final IDs/order, replays budget decisions in canonical order, and invokes Component 03 verification before commit.

## 20. Narrow capability contracts for other components

Publish owner-checked read-only capabilities rather than the entire mutable context:

- Component 02: source point import, source plane/residual/projection, bounded predicates, source AABBs, and input-precision evidence;
- Component 04: bounded polygon projection/orientation/planarity operations and source-triangle bounds;
- Component 05: conservative normals/bounds only; topology remains integer-owned;
- Component 06: finite inflated AABBs, closed overlap, and separation queries;
- Component 07: bounded determinants/residuals, exact nominal tie evidence, construction conditioning, and immutable predicate results;
- Component 08: one bounded coordinate/parameter/ledger identity per interned event;
- Component 09: immutable crossing/classification margins, not permission to repeat relations;
- Component 10: precision eligibility attached to retained uses;
- Components 11-12: bounded construction and triangulation residual services;
- Component 13: tolerance proposal/reservation/commit and displacement certificates;
- Component 14: aggregate output precision and canonical encoding inputs;
- Component 15: independent residual, plausibility, ledger, budget, and AABB verification views; and
- Component 17: immutable task inputs, private fragment factories, and canonical merge/finalize APIs.

Every capability validates context owner, provider version, stage eligibility, and transaction state. No consumer receives a mutable ledger vector or budget map.

## 21. Required tests: platform and bit-level known answers

### 21.1 FloatingBits known answers

Commit exact bit expectations for `float` and `double` covering:

- `-0`, `+0`, adjacent values around zero;
- smallest/largest subnormal;
- smallest normal and neighbors;
- adjacent values around `-1`, `1`, and powers of two;
- maximum finite values and outward-overflow rejection;
- total-order keys; and
- exact power-of-two scaling boundaries.

### 21.2 Arithmetic model tests

Test nearest-even tie cases, signed-zero results, underflow, cancellation, non-contracted multiply-add, and prescribed grouping. Build dedicated negative probes with intentionally unsafe flags/rounding settings and require platform rejection before topology work.

### 21.3 Cross-configuration stability

Run debug and optimized strict builds on supported GNU, Clang, and MSVC profiles. Expected nominal bits, interval bits, categories, operation codes, and canonical bytes must match the committed profile. Unknown compilers remain unsupported until a reviewed profile and golden corpus are added.

## 22. Required tests: exact oracle, properties, and conditioning

### 22.1 In-tree exact oracle

Implement a deliberately slow test-only arbitrary-precision unsigned integer, signed integer, and normalized rational type using base-`2^32` limbs. It must support exact conversion from finite `float`/`double` bits and enough `+`, `-`, `*`, comparison, and division/rational normalization to evaluate the required fixtures. Production code must not include or link it.

Component 16 may later adopt this oracle as shared test infrastructure, but Component 03 tests must be self-contained now.

### 22.2 Oracle containment

For bounded integer/rational fixtures, verify exact results lie inside published intervals for:

- scalar arithmetic;
- interpolation;
- dot/cross;
- squared norm;
- 2x2/3x3 determinants;
- plane construction/residual;
- parameter projection;
- edge-plane intersection;
- affine projection; and
- accumulated cleanup displacement.

### 22.3 Property tests

Use a fixed deterministic PRNG and generate finite bit patterns stratified across zeros, subnormals, normals, exponents, signs, and adjacent values. Verify:

- containment;
- monotonic widening when input precision increases;
- no unproved shrinkage;
- operand symmetry/remapped symmetry where mathematically required;
- deterministic operation traces/bytes;
- finite failure instead of infinity/NaN publication;
- exact nominal sign agreement when the interval is definite;
- uncertain classification when exact nonzero values straddle zero; and
- stable exact-tie classification for exact nominal zero.

### 22.4 Conditioning boundaries

Construct edge-plane, line-plane, projection, and carrier-ordering fixtures:

- comfortably conditioned;
- just inside tolerance;
- exactly at the conservative threshold;
- just outside tolerance;
- denominator interval containing zero;
- exact coplanar;
- exact endpoint tie;
- near-parallel but bounded;
- large translation with small local geometry;
- mixed-magnitude operands; and
- result requiring outward step to maximum finite.

Require deterministic categories and typed failures without coordinate jumps.

## 23. Required tests: ledger, budget, bounds, replay, and mutation

### 23.1 Ledger tests

Test parent-before-child ordering, wrong-owner/stale handles, cycles, forward references, omitted contributors, duplicated IDs, narrowing with/without proof, repeated operation chains, and aggregate output precision. Verify precision never resets after serialization/re-ingestion.

### 23.2 Budget tests

Test one displacement, disjoint lineages, repeated same-lineage edits, exact boundary, outward rounding over boundary, rollback, cancellation, actual less than reservation, actual greater than reservation, component removal authorization, leaked reservation, canonical commit order, and nominal-versus-conservative displacement disagreement.

### 23.3 Conservative bounds tests

Compare point/edge/triangle/shell AABB overlap against exhaustive exact rational feature relations for small fixtures. Include one-ULP gaps, exact contact, subnormal separation, combined operand uncertainty, large translations, extreme finite scales, and finite-expansion failure. No true bounded interaction may be excluded.

### 23.4 Codec/replay tests

Commit golden bytes for representative contexts, values, predicates, constructions, ledgers, budgets, AABBs, and failures. Verify decode/re-encode identity, unknown-version rejection, truncation/overflow handling, thread/traversal permutation stability, and replay equivalence.

### 23.5 Mutation tests

Apply every mutation listed in Section 18.3 and require independent verifier rejection. Include mutations that preserve nominal values while shrinking only uncertainty, because nominal-only checks must not pass.

### 23.6 Resource/cancellation tests

For each resource kind and major checkpoint, test limit-minus-one, limit, limit-plus-one; cancellation before/during/after; exception injection at transaction boundaries; and zero publication after failure.

## 24. Implementation sequence and handoff gates

Implement in this order; do not begin a later gate while an earlier gate lacks its required tests:

1. Add version/enum/subcode/checkpoint registry entries and compile-time uniqueness tests.
2. Add `FloatingBits` and all bit-level golden tests.
3. Add the runtime arithmetic-model qualification and negative platform probes.
4. Add finite intervals and scalar outward arithmetic with exact-oracle tests.
5. Add the strict exact-expansion provider and exact nominal sign/tie tests; preserve adaptive-predicate compatibility.
6. Add bounded scalar/vector/point schemas and ledger-free pure operation traces.
7. Add precision bootstrap/context handshake and source import.
8. Add vector, dot, cross, squared-norm, plane, residual, determinant, projection, and parameter operations.
9. Add predicate results and deterministic alternate-formula policy.
10. Add edge-plane/construction conditioning and threshold tests.
11. Add precision ledger, monotonic aggregation, snapshots, and verifier traversal.
12. Add conservative AABBs and exhaustive no-false-negative tests.
13. Add tolerance budget reservation/rollback/commit and mutation tests.
14. Add repeated-operation/affine-transform/public-precision import.
15. Add canonical codec, digests, replay, and golden bytes.
16. Complete the independent verifier and full mutation suite.
17. Integrate narrow Component 03 test providers into Component 02 and later component interfaces without implementing those components.
18. Run all Component 01 and Component 02 tests to prove no contract regression.
19. Run sanitizer/debug/release/strict-FP qualification matrices supported by the repository.
20. Update `tracker.md` only after every Section 25 item is demonstrated.

Each gate must leave the branch buildable and must not weaken a prior invariant to make a later test pass.

## 25. Definition of done

Component 03 is complete only when all of the following are true:

- the two-phase precision bootstrap/frozen-context handshake is implemented and independently verified;
- the V1 arithmetic model is frozen as nearest-even, strict C++17, finite IEC 60559 `float`/`double`, gradual-underflow, signed-zero-preserving, and no-contraction;
- unsupported build/runtime/worker environments fail before topology-affecting work;
- `FloatingBits` has committed known-answer coverage for every boundary class;
- every published bounded value is finite, owner/version valid, nominal-contained, provenance-linked, and ledger-linked;
- every primitive operation has fixed grouping, outward-rounded finite intervals, operation/formula IDs, exact-oracle containment tests, and deterministic bytes;
- exact nominal ties are distinguished from nonzero nominal values whose uncertainty crosses zero;
- the adaptive expansion arithmetic used for exact nominal signs is strict-target, exponent-safe, capacity-checked, and compatibility-tested;
- plane, residual, determinant, interpolation, projection, parameter, and edge-plane construction services satisfy all conditioning categories and typed failures;
- no ill-conditioned or over-tolerance construction publishes a bare coordinate;
- precision ledger propagation is monotonic, proof-producing narrowing is the only permitted shrinkage, and repeated-operation import never resets uncertainty;
- tolerance uncertainty, displacement, feature removal, cumulative lineage cost, and component removal remain separate;
- budget reservation, rollback, canonical commit, and verifier reconstruction reject every under-reporting mutation;
- conservative AABBs have no false negatives in exhaustive bounded tests and fail rather than publish non-finite bounds;
- canonical encoding/replay is bit-stable across supported builds, traversal permutations, and worker counts;
- independent verification reconstructs operation results, exact signs, ledger totals, budget totals, AABBs, and digests without trusting producer booleans;
- resource, cancellation, wrong-owner, stale-handle, overflow, exception, and transaction rollback tests prove zero partial publication;
- Component 01 and Component 02 regression suites remain passing under their required strict targets;
- production and normative-test code use no external dependency and no ignored legacy Boolean source; and
- `tracker.md` marks component 3 complete only after this fully complete plan has been added and reviewed.
