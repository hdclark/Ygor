# Plan 03: Precision, Tolerance, and Bounded Arithmetic

## 0. Scope, review status, and non-negotiable constraints

Implement **only Component 03** from `component_03_precision_bounded_arithmetic.md`. Component 03 establishes the immutable precision context and the bounded numerical services consumed by Components 02-15. It owns the qualified arithmetic model, source precision bootstrap, finite enclosures, deterministic rounded operation graphs, exact stored-coordinate algebraic relations, predicate evidence, construction conditioning, precision lineage, tolerance-budget accounting, conservative feature bounds, canonical encoding, replay, and independent verification.

It must not:

- validate source topology, facet incidence, vertex links, shell nesting, or occupied-side semantics;
- assign source topology IDs, intersection-event IDs, output occurrences, edges, faces, or adjacency;
- select Boolean ownership, crossing multiplicity, winding, retained surfaces, or symbolic owners;
- perform cleanup topology mutations;
- assemble or publish `fv_surface_mesh<T,I>`;
- use caller tolerance as a universal equality, coplanarity, ordering, snapping, or welding epsilon;
- infer identity from equal bits, equal coordinates, overlapping intervals, proximity, hashes, pointers, or allocation order;
- call or adapt `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- use `long double`, compiler-specific extended arithmetic, excess register precision, unqualified SIMD, a process-global rounding-mode change, or a transcendental function as an authoritative shortcut;
- use an external interval, exact-arithmetic, geometry, hashing, serialization, test, fuzzing, or concurrency dependency; or
- compile production or normative-test code outside the strict C++17 target established by Component 01.

V1 is fixed to qualified IEC 60559 binary32/binary64 `float` and `double`, nearest-even rounding, prescribed scalar stores, fixed grouping, preserved signed zero, gradual underflow, and prohibited contraction for authoritative operation graphs.

Component 03 must use Component 01 for immutable source views, pending/frozen context state, owner tokens, strong IDs, checked count/byte/index arithmetic, typed outcomes/errors, platform guard, strict-build qualification, resource accounting, cancellation, deterministic error arbitration, transactions, canonical bytes, SHA-256, replay, execution scopes, and immutable publication.

No failed, cancelled, resource-exhausted, budget-rejected, partially encoded, or verifier-rejected artifact may escape a transaction.

`tracker.md` records completion of this planning/review step, not implementation completion. Mark Component 03 complete after this reviewed specification and plan are committed and consistent with the broad plan and adjacent components. Section 25 is the future implementation handoff gate.

## 1. Independent review conclusions and required corrections

The prior plan had a strong numerical, testing, and verification foundation. The independent review identified the following integration and implementability defects; this revision makes their corrections mandatory.

### 1.1 Separate three numerical truth layers

The prior plan used “exact nominal” for two different things:

- the result of a prescribed rounded `T` operation graph; and
- the exact sign of a polynomial evaluated over stored floating-point inputs as exact real numbers.

Those are not equivalent. V1 must keep separate:

1. `rounded_nominal`: exact bits produced by the versioned rounded graph;
2. `exact_relation`: exact sign/zero of a specified algebraic relation over stored nominal bits, when supported; and
3. `uncertainty_enclosure`: all accepted realizations after inherited precision and operation error.

Every operation, codec, verifier, diagnostic, and consumer capability must name these layers explicitly. Expansion arithmetic proves `exact_relation`; it does not redefine the rounded graph.

### 1.2 Do not turn exact stored-coordinate zero into symbolic authorization

Component 07 explicitly forbids resolving ordinary uncertainty through symbolic policy. Therefore Component 03 must return orthogonal bounded-sign and exact-relation evidence. An exact stored-coordinate zero may coexist with inherited uncertainty that permits both signs. Component 03 records that fact; Component 07 decides relation-specific symbolic eligibility under the frozen contact policy.

No Component 03 enum may hide the coexistence of exact zero and unresolved uncertainty.

### 1.3 Resolve non-finite bootstrap ownership

Component 01 preserves exact source bits, including malformed non-finite values, and Component 02 owns full input validation. However, Component 03 must scan coordinates before Component 02 can run because scale and machine-floor bootstrap precede final context freeze.

V1 resolution:

- Component 03 bootstrap rejects NaN/infinity as `input_contract_error` with operand/source-position evidence;
- it publishes no precision preflight or context;
- this is an early numerical-eligibility check, not publication of a validated operand; and
- Component 02 keeps a defensive finiteness check for isolated tests, replay, and invariant protection.

### 1.4 Make outward arithmetic exactness-aware at finite limits

The prior unconditional “round then step both endpoints one ULP” rule is conservative but can reject an exactly representable finite boundary result, such as an exact operation producing maximum finite, even though a finite enclosure exists.

V1 must use error-free or exact residual evidence where available:

- exact operation: keep the exact endpoint;
- inexact operation with known residual direction: step only the required side;
- direction unavailable: conservatively step both sides; and
- required step to infinity or non-finite nominal: typed failure.

This remains fail-closed without creating avoidable false failures.

### 1.5 Reuse and harden Ygor adaptive expansion arithmetic without target cycles

`YgorMeshesAdaptivePredicates` is valuable existing Ygor functionality, but the bounded subsystem must not link the ordinary fast-math `ygor` object or create a bounded-to-general target cycle.

Extract one audited dependency-free strict expansion core into a small internal target compiled with the bounded strict profile. Both:

- legacy `YgorMeshesAdaptivePredicates` compatibility wrappers; and
- Component 03 `ExactFloatExpansion`

may call that strict core through separate narrow adapters. Preserve the public legacy header and return conventions. Apply regression tests before redirecting legacy implementation. The bounded provider adds owner, formula, capacity, scaling, resource, diagnostic, and exact-relation records; the legacy wrapper remains bare for source compatibility.

If a supported build profile cannot link the common strict core without acquiring general-target flags or dependencies, the bounded provider remains independently compiled from the reviewed core source and the legacy implementation remains unchanged for that profile. Component 03 must never call the non-strict legacy object.

### 1.6 Keep ledger completeness without one global record per temporary scalar

The prior wording risked requiring a globally committed ledger entry for every scalar node. That would produce unnecessary memory/work amplification and conflict with Component 17 private fragments.

V1 distinguishes:

- transaction-local immutable operation traces for temporary bounded values; and
- committed precision-ledger records for values crossing component boundaries, participating in event/output lineages, or entering published bytes.

Finalization must preserve every contributor; compacting local traces may not omit arithmetic nodes needed for independent reconstruction.

### 1.7 Make tolerance costs dimensionally valid

Caller tolerance is a length. Cleanup proposals may not compare area, volume, angle, or dimensionless quality directly with it. Every feature-removal or component-removal proposal must provide a versioned conservative length-valued deviation certificate, such as displacement, clearance, thickness, or a Hausdorff-style bound.

### 1.8 Promote, do not duplicate, the test exact oracle

Component 16 owns permanent qualification infrastructure and already requires promotion of Component 03’s base-`2^32` integer/rational oracle. Component 03 must place the initial test-only implementation under the future shared `qualification/` modules or a temporary forwarding layout that Component 16 can promote without a second implementation. Production exact expansions are not the expected-answer oracle.

## 2. Existing Ygor assessment and mandatory reuse decisions

### 2.1 Public math and mesh types

Reuse `vec2<T>` and `vec3<T>` only as nominal coordinate carriers and `fv_surface_mesh<T,I>` only through Component 01 immutable snapshots and later Component 14 output.

Do not use `vec2`/`vec3` arithmetic operators, `Dot`, `Cross`, `length`, `unit`, `distance`, `angle`, `operator==`, or `operator<` for authoritative bounded work. Component 03 implements the prescribed scalar graph explicitly.

### 2.2 Adaptive predicates

The existing `src/YgorMeshesAdaptivePredicates.h/.cc` contains expansion primitives and `orient3d`/`insphere`. It is not directly sufficient because it:

- returns a bare `T` or sign;
- assumes exact source coordinates;
- does not propagate inherited precision;
- has no owner/provenance/operation/formula/conditioning records;
- has fixed arrays without Component 01 resource accounting;
- does not fully protect splitter/product paths against exponent extremes;
- does not distinguish rounded nominal, exact relation, and uncertainty enclosure; and
- is compiled in the ordinary target unless specifically isolated.

Mandatory reuse plan:

1. Add an internal strict core containing audited `two_sum`, `two_diff`, `split`, exponent-safe `two_product`, expansion sum/scale/compress, exact comparison, and determinant assembly.
2. Preserve the existing mathematical approach and public compatibility API.
3. Compile the core once under strict flags in a dependency-free object/static target usable by both legacy wrappers and the bounded target without linking the general `ygor` target into bounded tests.
4. Add exponent normalization, finite checks, capacity checks, stable formula IDs, and exact sign/zero extraction.
5. Keep `insphere` compatibility-tested but do not expose it as a minimum Component 03 capability unless a reviewed consumer requires it.
6. Component 03 computes enclosures separately; exact expansion results never replace inherited uncertainty.

### 2.3 Existing geometry classes

Existing `line<T>`, `line_segment<T>`, and `plane<T>` normalize directions, use square roots, raw comparisons, bare booleans, and optional epsilons. Existing contour projection, point-in-polygon, least-squares plane fitting, duplicate removal, simplification, and intersection helpers lack inherited uncertainty, proof-producing bounds, deterministic lineage, typed failure, and replay.

Do not use their authoritative methods, including plane signed distance, projection, or line/segment intersection. They may be used only for non-authoritative fixtures or visualization.

### 2.4 Existing serializers, hashes, and spatial indexes

Do not use `YgorSerialize`, native struct bytes, text I/O, `std::hash`, SpookyHash, or MD5 for precision artifacts. Use Component 01 `CanonicalBytes` and SHA-256.

Existing R-tree, octree, KD-tree, cells index, and mesh bounding boxes are consumers or benchmark references, not conservative evidence. Component 03 publishes finite closed AABBs; Component 06 chooses the acceleration provider.

### 2.5 No suitable existing ledger or budget provider

Ygor has no owner-bound finite interval, precision-lineage DAG, transactional per-lineage tolerance budget, or independent bounded-artifact verifier. Add these within the bounded subsystem. Do not broaden unrelated general-purpose math APIs with Boolean-specific owner, policy, or replay state.

## 3. Exact file and target layout

### 3.1 Shared strict expansion core

Add:

- `src/YgorMeshesExactFloatExpansionCore.h`
- `src/YgorMeshesExactFloatExpansionCore.cc`

or an equivalently named internal-only pair. It must expose no installed public API. Build it as `ygor_exact_float_expansion_core` under strict floating flags, C++17, no IPO, and no optional dependencies.

Update `YgorMeshesAdaptivePredicates.cc` to call the core through compatibility wrappers only after legacy regression tests pass. Do not change `YgorMeshesAdaptivePredicates.h` signatures.

### 3.2 Component 03 production files

Add under `src/YgorMeshesBooleanBounded/`:

- `PrecisionTypes.h` — closed enums, scale records, truth-layer statuses, conditioning, uncertainty causes, certificate kinds, and fixed constants.
- `PrecisionBootstrap.h/.cc` — exact source-bit scan, finiteness failure, scale derivation, machine floor, preflight bytes, and Component 01 handshake.
- `PrecisionContext.h/.cc` — immutable `precision_context<T>`, owner/version validation, and narrow capability construction.
- `FloatingBits.h` — `memcpy` bit conversion, finite total order, signed-zero rules, adjacent finite values, exponent extraction, and exact power-of-two scaling.
- `DirectedRounding.h/.cc` — exactness-aware scalar endpoint enclosure for add/subtract/multiply/divide and conversion.
- `FiniteInterval.h/.cc` — checked immutable finite closed intervals, hull, intersection proof records, zero containment, and widths.
- `ExactFloatExpansion.h/.cc` — bounded adapter over the strict expansion core, formula registry, normalization, capacity/resources, and exact relation records.
- `BoundedValues.h` — immutable scalar/vector/point/plane/parameter/residual/AABB schemas and transaction-local variants.
- `BoundedOperations.h/.cc` — scalar/vector/point/plane/determinant/projection/parameter operations with fixed graphs.
- `PredicateResults.h/.cc` — orthogonal bounded-sign, exact-relation, disposition, margin, and uncertainty evidence.
- `ConstructionConditioning.h/.cc` — edge-plane, edge-face, carrier, interpolation, projection, and residual classification.
- `PrecisionTrace.h/.cc` — transaction-local immutable operation DAGs and canonical trace roots.
- `PrecisionLedger.h/.cc` — committed lineage records, local-trace finalization, snapshots, and aggregate queries.
- `ToleranceBudget.h/.cc` — length-certificate proposals, reservations, rollback, canonical commit, per-lineage totals, and snapshots.
- `ConservativeBounds.h/.cc` — finite closed AABBs, inflation, overlap, separation, distance lower bounds, and feature constructors.
- `PrecisionImport.h/.cc` — prior-result precision verification, foreign provenance, and affine-transform propagation.
- `PrecisionCodec.h/.cc` — canonical encode/decode for all contexts, records, traces, ledgers, budgets, bounds, and failures.
- `PrecisionVerifier.h/.cc` — independent bootstrap, arithmetic, exact-relation, trace, ledger, budget, bounds, and codec verification.
- `PrecisionCapabilities.h` — narrow owner-checked interfaces exported to Components 02-17.

Extend `ContractVersions.h` and Component 01 stage, checkpoint, resource, strong-ID, subcode, replay, and diagnostic registries. Do not create a second registry.

### 3.3 Tests

Add or extend under `tests/mesh_boolean_bounded/`:

- `TestPrecisionBootstrap.cc`
- `TestPrecisionPlatform.cc`
- `TestFloatingBits.cc`
- `TestDirectedRoundingKnownAnswers.cc`
- `TestDirectedRoundingOracle.cc`
- `TestFiniteInterval.cc`
- `TestExactFloatExpansion.cc`
- `TestBoundedValues.cc`
- `TestBoundedVectorPlane.cc`
- `TestPredicateTruthLayers.cc`
- `TestConstructionConditioning.cc`
- `TestPrecisionTraceLedger.cc`
- `TestToleranceBudget.cc`
- `TestConservativeBounds.cc`
- `TestPrecisionImportTransforms.cc`
- `TestPrecisionCodecReplay.cc`
- `TestPrecisionMutation.cc`
- `TestPrecisionProperties.cc`
- `TestPrecisionAdversarial.cc`
- `PrecisionFixtures.h/.cc`
- `GoldenPrecisionV1.h`

Place the test exact-number implementation directly under `tests/mesh_boolean_bounded/qualification/` using the Component 16 final names where practical:

- `ExactUnsignedInteger.h/.cc`
- `ExactInteger.h/.cc`
- `ExactRational.h/.cc`
- `ExactFloatImport.h/.cc`
- a minimal `ExactGeometryOracle.h/.cc`

If Component 16 files are not yet introduced, Component 03 may add these shared modules now and Component 16 extends them later. Do not add `PrecisionExactOracle` as a separate permanent implementation.

Register focused CTest cases. Apply `ygor_apply_mesh_boolean_strict_fp` to every authoritative production/test target and to the shared exact-expansion core. The arbitrary-precision test oracle is test-only and must not link into production.

## 4. Stable versions, IDs, stages, checkpoints, and errors

### 4.1 Version registry

Add explicit nonzero V1 versions for:

- precision bootstrap schema/provider;
- scalar type/platform profile;
- bit/total-order provider;
- directed-rounding provider;
- finite-interval schema/provider;
- exact-expansion core and bounded adapter;
- rounded operation-graph registry;
- exact-relation formula registry;
- bounded scalar/vector/point/plane/parameter/residual/AABB schemas;
- predicate truth-layer schema;
- conditioning provider;
- local trace and committed ledger schemas/providers;
- tolerance certificate and budget schemas/providers;
- prior-precision import and transform provider;
- precision codec/replay; and
- precision verifier.

Encode every required version in context, replay, records, and verifier evidence. Reject zero, unknown required values, unsupported enum values, and nonzero reserved fields.

### 4.2 Strong IDs

Use Component 01 strong IDs for:

- bounded value;
- operation trace and trace node;
- exact relation record;
- construction;
- precision ledger entry;
- geometric lineage;
- budget proposal/reservation/commit;
- displacement/removal certificate;
- finite bound; and
- verifier finding.

Task-local IDs never cross a transaction or worker boundary. Canonical committed IDs are assigned after complete-key sort/merge.

### 4.3 Rounded operation codes

Define explicit fixed-width codes for at least:

- source import and prior-result import;
- exact scalar constant and checked integer conversion;
- add, subtract, negate, multiply, divide;
- multiply-then-add/subtract with contraction prohibited;
- interval hull and proof-producing intersection;
- vector add/subtract/scale;
- interpolation from endpoint A and B;
- dot2/dot3;
- cross2/cross3;
- squared norm;
- coordinate/radial derivation;
- affine map and projection;
- plane construction and residual;
- 2x2/3x3 determinant rounded graphs;
- carrier parameter;
- AABB build/inflate/union/intersection; and
- serialization rounding/conversion.

### 4.4 Exact relation formula codes

Keep a separate registry for exact relations, at minimum:

- exact finite scalar comparison;
- exact sum/difference residual;
- exact product residual;
- exact division quotient residual numerator;
- exact 2x2 determinant;
- exact 3x3 determinant;
- exact orient2d/orient3d forms needed by consumers;
- exact plane numerator relation;
- exact endpoint numerator relation; and
- exact collinearity/coplanarity relations required by Components 02, 04, 07, 12, and 13.

A rounded operation code must never be decoded as an exact relation formula or vice versa.

### 4.5 Logical checkpoints

Use stable checkpoints in this order:

1. pending-context and capability validation;
2. exact source count/bit scan;
3. non-finite source rejection;
4. operand/global scale derivation;
5. machine-floor derivation;
6. input-precision normalization and tolerance eligibility;
7. preflight encoding/digest;
8. frozen-context handshake validation;
9. immutable precision-context construction;
10. source bounded-value import;
11. primitive rounded operation execution;
12. exact relation evaluation;
13. predicate truth-layer assembly;
14. construction conditioning and residuals;
15. local trace finalization;
16. committed ledger append/snapshot;
17. tolerance reservation;
18. tolerance commit/rollback;
19. conservative bound publication;
20. prior-result import/transform propagation;
21. independent verification;
22. canonical encoding/digest/resource reconciliation; and
23. pre-publication cancellation and transaction commit.

Do not renumber released checkpoints.

### 4.6 Required subcodes and category mapping

Allocate a disjoint Component 03 range covering at least:

- unsupported scalar/profile;
- strict-build marker missing;
- nearest-even/contract/signed-zero/subnormal profile mismatch;
- known-answer mismatch;
- source non-finite bit pattern;
- invalid precision/tolerance;
- scale/floor unavailable or non-representable;
- effective input precision exceeds tolerance;
- bootstrap/frozen-context mismatch;
- wrong/stale owner/provider/version;
- invalid/reversed/non-finite interval;
- nominal outside enclosure;
- directed result non-finite or outward step unavailable;
- division denominator enclosure contains zero;
- unsupported rounded operation/exact formula;
- exact expansion capacity/scaling/finite failure;
- rounded/exact/enclosure contradiction;
- predicate evidence incomplete;
- plane support degenerate or uncertain;
- parameter/domain invalid;
- construction residual exceeded;
- construction ill-conditioned/over tolerance;
- local trace cycle/forward reference/escape;
- ledger ID/count/byte overflow, cycle, missing contributor, or owner mismatch;
- malformed or dimensionally invalid budget certificate;
- duplicate/overlapping budget lineage;
- reservation over tolerance;
- reservation owner/transaction mismatch;
- actual cost above reservation;
- cumulative cost over tolerance;
- feature/component removal unauthorized;
- canonical commit-order mismatch;
- leaked reservation;
- AABB invalid/non-finite/inflation unavailable;
- prior precision import invalid or reset attempt;
- codec unknown/truncated/reordered/overflow;
- digest/replay mismatch;
- verifier rejection; and
- deterministic byte mismatch.

Map:

- unsupported execution semantics to `unsupported_platform`;
- source non-finite to `input_contract_error`;
- invalid caller precision/tolerance to `invalid_tolerance`;
- over-tolerance/ill-conditioned construction to `geometric_condition_exceeds_tolerance`;
- cleanup certificate/budget failure to `cleanup_budget_exceeded`;
- count/byte/entity capacity to `index_overflow` or `resource_limit` as appropriate;
- cancellation to `cancelled`; and
- producer/verifier contradiction or escaped task-local state to `internal_invariant_error`.

## 5. Component 01 bootstrap and freeze handshake

### 5.1 API boundary

Implement a narrow preflight API conceptually equivalent to:

```cpp
template<class T, class I>
boolean_outcome<precision_preflight<T>>
preflight_precision(
    const pending_boolean_context_view<T,I>& pending,
    const immutable_invocation_sources_view<T,I>& sources,
    const precision_bootstrap_capabilities& capabilities);
```

The preflight receives exact source bits and normalized option scalars that passed Component 01 preliminary representation/sign checks. It receives no final owner token and may publish only a transaction-owned preflight record.

### 5.2 Preflight algorithm

1. Validate type/profile/policy versions and strict capability evidence.
2. Validate counts and exact source-section ranges before scanning.
3. Reserve scan, scale, diagnostic, byte, and digest resources.
4. Scan A and B source coordinate bits in stable source-position order.
5. Reject the first canonical non-finite source coordinate as `input_contract_error`; collect bounded secondary findings without changing the primary.
6. Derive operand/global scale descriptors with checked finite arithmetic.
7. Compute per-coordinate representation floors and aggregate source floors.
8. Preserve declared input precision separately.
9. Derive effective precision and ordinary-success eligibility.
10. Encode all inputs/results canonically and compute the domain-separated preflight digest.
11. Independently verify the preflight proposal.
12. Publish one immutable pending preflight record.

Source traversal order is replay order, not canonical topology identity. The preflight digest intentionally binds exact source order because Component 01 replay does.

### 5.3 Context freeze and final construction

Component 01 incorporates the verified preflight record into context bytes/replay and publishes the final owner token.

Implement:

```cpp
template<class T, class I>
boolean_outcome<artifact_handle<const precision_context<T>>>
build_precision_context(
    const precision_preflight<T>& preflight,
    const frozen_boolean_context_view<T,I>& context,
    const precision_runtime_capabilities& capabilities);
```

Revalidate:

- source/preflight/context digests;
- exact scalar/index profiles;
- tolerance/input-precision bits;
- arithmetic/enclosure/formula versions;
- scale/floor summaries;
- ordinary-success eligibility;
- platform evidence;
- owner and transaction; and
- all reserved fields.

No pending object or provisional owner escapes to Components 02-17.

## 6. Qualified floating-point model

### 6.1 Compile-time profile

Require and encode:

- `T` exactly `float` or `double`;
- IEC 60559, radix 2, expected digits/exponents/storage;
- exact `std::uint32_t`/`std::uint64_t` bit carriers;
- `YGOR_MESH_BOOLEAN_STRICT_FP_BUILD == 1`;
- no `__FAST_MATH__`;
- C++17 with extensions disabled;
- no IPO/LTO for V1 strict objects unless separately qualified; and
- no contraction for authoritative graphs.

Do not accept a type from byte width alone.

### 6.2 Runtime and worker profile

At invocation guard establishment and every worker entry, verify:

- `fegetround() == FE_TONEAREST`;
- committed add/subtract/multiply/divide known answers;
- signed-zero operations and bit round trips;
- subnormal input/output behavior and no flush-to-zero;
- prescribed multiply/add is not contracted;
- documented scalar stores occur; and
- `FloatingBits` adjacent-value results match goldens.

Component 03 never sets the rounding mode. Component 01 establishes/restores the guarded environment. A Component 03 mismatch is `unsupported_platform`; restoration failure is a Component 01 transaction invariant.

### 6.3 Prescribed rounded graphs

For V1:

- every scalar node stores to `T`;
- `a*b+c` is rounded multiply then rounded add;
- dot3 is products followed by a fixed left-associated add sequence;
- cross components are two products then subtraction in fixed coordinate order;
- plane residual uses fixed dot grouping then add;
- determinant rounded graphs use an explicit versioned term order; and
- producer/verifier do not substitute algebraically equivalent graphs.

## 7. Floating bits and exactness-aware directed rounding

### 7.1 Bit conversion and total order

Implement `to_bits`/`from_bits` with `std::memcpy`, static size checks, and no union punning. Accept all finite bit patterns including signed zeros/subnormals. Reject NaN/infinity as value inputs.

Provide a stable finite numeric total-order key:

```text
negative finite < -0 < +0 < positive finite
```

Use it for interval validation/canonical sorting only, never identity.

### 7.2 Adjacent finite values

Implement bitwise `next_up_finite` and `next_down_finite` with explicit cases for:

- both zeros;
- smallest/largest subnormal;
- normal/subnormal transition;
- positive/negative normal;
- maximum finite; and
- required outward move to infinity.

Use `std::nextafter` only as a test comparison.

### 7.3 Exact power-of-two scaling

Provide checked exponent-bit or qualified `frexp`/`ldexp` helpers. Preserve sign and exactness when representable. Detect overflow, destructive underflow, and unsupported subnormal behavior. Use these helpers to normalize expansion inputs and avoid splitter/product overflow.

### 7.4 Directed scalar result schema

Each endpoint operation returns:

```cpp
template<class T>
struct directed_scalar_result {
    T rounded;
    T lower;
    T upper;
    rounding_exactness exactness;
    residual_sign direction;
    rounded_operation_code operation;
    exact_relation_id residual_evidence;
};
```

Invariants:

- all values finite;
- `lower <= rounded <= upper`;
- singleton iff exactness is proved;
- one-sided widening iff exact residual direction is proved;
- two-sided widening only when direction is unavailable; and
- no saturated endpoint.

### 7.5 Addition and subtraction

Use error-free `two_sum`/`two_diff` under the qualified profile:

- zero residual => exact singleton;
- positive residual => exact result above rounded, so widen upper;
- negative residual => exact result below rounded, so widen lower.

For interval addition/subtraction, evaluate the mathematically extremal endpoint pairs through the directed primitive.

### 7.6 Multiplication

Use exponent-safe `two_product` from the strict expansion core. The residual determines exactness/direction. For interval multiplication, evaluate all four endpoint products and take the minimum lower and maximum upper under finite total order.

Do not assume sign categories without proving interval signs.

### 7.7 Division

Reject denominator intervals containing either signed zero or straddling zero.

For scalar endpoint `q = round(a/b)`, determine the sign of the exact residual numerator:

```text
r = a - q*b
```

using the strict exact relation core after safe scaling. Account for denominator sign to determine whether `q` lies below or above the exact quotient.

- exact zero residual => singleton quotient;
- known direction => one-sided outward step;
- unavailable direction => conservative two-sided step;
- non-finite q or required step to infinity => failure.

Use one recorded V1 direct-four-quotient interval formula or a recorded reciprocal/multiply formula; producer and verifier use the same formula ID.

### 7.8 Conversion and integers

Import zero, one, powers of two, and integers as singleton intervals only with exact representability proof. Otherwise store the rounded nominal and directed conversion enclosure with an explicit conversion contribution.

## 8. Finite intervals and source precision

### 8.1 Interval schema

Use checked immutable finite closed intervals:

```cpp
template<class T>
struct finite_interval {
    T lower;
    T upper;
};
```

Canonical exact zero interval is `[-0,+0]`; preserve nominal signed zero separately. No NaN, infinity, empty interval, open endpoint, or writable published endpoint is allowed.

### 8.2 Hull and intersection

Hull is the smallest finite interval containing both inputs.

Intersection is the only ordinary narrowing primitive and requires a proof record identifying:

- both parents;
- constraint kind;
- source of the independent guarantee;
- exact resulting endpoints; and
- verifier reconstruction path.

Empty intersection returns an explicit no-value/contradiction outcome; it never silently chooses a parent.

### 8.3 Scale descriptors

Record separately for A, B, and global:

- per-axis min/max;
- per-axis max absolute;
- finite span;
- all-identical-axis flags;
- global max absolute;
- smallest nonzero magnitude;
- zero/subnormal/normal presence;
- mixed-magnitude and translation indicators;
- source count/digest; and
- exact-relation normalization exponent.

### 8.4 Source floor and enclosure

For source coordinate `x`, compute a representation floor from finite adjacent gaps, retaining asymmetric axis bounds when useful. Combine with declared/inherited source precision through directed arithmetic.

V1 keeps both:

- coordinate intervals for conservative box operations; and
- a radial source uncertainty bound reflecting the public precision contract.

Do not derive the radial source bound only by summing three independent axis maxima when the source contract already supplies a tighter radial uncertainty. Store contributors separately.

Ordinary success is initially eligible only when:

- declared/effective input precision is finite and nonnegative;
- required source enclosures are representable;
- effective precision for each operand does not exceed tolerance; and
- tolerance is at least the required machine floor.

A diagnostic-only context may retain ineligibility but all downstream records remain nonpublishable as ordinary success.

## 9. Bounded value schemas

### 9.1 Transaction-local and published values

Provide transaction-local values with owner, local trace, and immutable content. Published values additionally require stable canonical IDs, committed ledger entries, and canonical bytes.

No mutable endpoint/coordinate access is exposed.

### 9.2 Bounded scalar

Store:

- rounded nominal bits;
- finite interval;
- owner/version;
- value/provenance/lineage IDs;
- local trace or committed ledger reference;
- uncertainty cause summary; and
- validity/publication state.

### 9.3 Vector and point

`bounded_vec2/vec3` contain bounded components plus conservative norm/radial queries. A point adds point provenance, geometric lineage, and nominal `vec3<T>` carrier.

Derive axis error by directed subtraction from nominal to interval endpoints. Store a conservative radial bound from the mixed source/construction model; L1 may be used as a fallback but not to discard a tighter independently valid radial contributor.

### 9.4 Plane

Use unnormalized form:

```cpp
template<class T>
struct bounded_plane3 {
    bounded_vec3<T> normal;
    bounded_scalar<T> offset;
    bounded_point3<T> anchor;
    bounded_scalar<T> normal_sq;
    provenance_id provenance;
};
```

Require `normal_sq.lower > 0`. Store anchor residual evidence. Never normalize the plane for authoritative work.

### 9.5 Parameter, residual, exact relation, and AABB

- `bounded_parameter<T>` records interval, carrier, endpoint convention, and domain evidence.
- `bounded_residual<T>` records scale, comparison boundary, pass/uncertain/fail evidence, and contributors.
- `exact_relation_record` records exact formula, ordered input nominal bits/IDs, normalization, capacity usage, and sign/zero.
- `bounded_aabb3<T>` records three finite intervals, feature provenance, inflation contributors, and lineage.

## 10. Vector, plane, determinant, and projection operations

### 10.1 Vector operations

Implement component-wise through bounded scalar operations. One parent trace owns ordered child nodes. No `vec` math helper is authoritative.

### 10.2 Dot and cross

Use the Section 6.3 rounded graph. The interval graph follows the same node structure. Exact relation evidence, when requested, uses the separate exact formula registry.

### 10.3 Squared norm

Compute bounded dot of a vector with itself. If interval widening yields a negative lower endpoint, intersect with `[0,max_finite]` only through a nonnegativity proof record.

Use squared comparisons where possible. A square-root diagnostic may be added only after separate qualification and may not be sole topology authority.

### 10.4 Plane construction

From accepted points `a,b,c`:

1. `u = b-a`;
2. `v = c-a`;
3. `n = cross(u,v)`;
4. `normal_sq = dot(n,n)`;
5. evaluate the exact stored-coordinate collinearity/orientation relation;
6. require a definitely positive lower bound for usable support;
7. `offset = -dot(n,a)`; and
8. evaluate a separately traced anchor residual.

Disposition:

- exact relation zero => exact stored-coordinate degeneracy evidence;
- exact relation nonzero but `normal_sq` overlaps zero => unresolved/ill-conditioned;
- definite positive support => accepted bounded plane;
- contradiction between exact relation and definite enclosure => invariant/verifier failure.

### 10.5 Residual and projection

Residual is fixed-group `dot(normal,point)+offset`.

Orthogonal projection, when explicitly requested, is:

```text
point - normal * residual / normal_sq
```

and requires denominator separation, parameter/displacement bounds, and residual-after-projection evidence. Do not make projection an implicit “repair.”

### 10.6 Determinants

Provide rounded bounded 2x2/3x3 determinant graphs and separate exact relation records over nominal inputs. Exact relation records do not include inherited uncertainty; predicate assembly combines both evidence layers.

## 11. Predicate truth layers and compute-once service

### 11.1 Stable schemas

Use orthogonal enums such as:

```cpp
enum class bounded_sign_status : std::uint8_t {
    definitely_negative = 1,
    overlaps_boundary = 2,
    definitely_positive = 3,
    invalid = 4,
};

enum class exact_relation_status : std::uint8_t {
    exact_negative = 1,
    exact_zero = 2,
    exact_positive = 3,
    unavailable = 4,
    invalid = 5,
};

enum class predicate_disposition : std::uint8_t {
    accept_numeric_sign = 1,
    retain_tie_for_consumer_eligibility = 2,
    try_permitted_alternate = 3,
    route_coplanar_or_coincident = 4,
    fail_condition_or_tolerance = 5,
    fail_invalid = 6,
};
```

A derived legacy five-way view may exist for convenience but cannot discard either orthogonal status.

### 11.2 Assembly rules

For zero comparison:

- `upper < 0` => bounded definitely negative;
- `lower > 0` => bounded definitely positive;
- otherwise bounded overlaps boundary.

Then combine exact relation evidence:

- exact zero + overlap => retain exact tie evidence and uncertainty contributors;
- exact nonzero + overlap => ordinary unresolved uncertainty;
- definite bounded sign conflicting with exact relation sign => internal contradiction;
- exact relation unavailable => disposition depends on consumer contract, never fabricated tie;
- invalid evidence => fail invalid.

Do not recommend “invoke symbolic policy” directly. Recommend retaining tie evidence for consumer eligibility. Component 07 owns the symbolic decision.

### 11.3 Margin and causes

For definite results store a conservative lower separation bound. For overlap store interval width, exact relation, and contributors distinguishing:

- operand A inherited;
- operand B inherited;
- machine floor;
- current construction;
- conditioning amplification;
- conversion/serialization;
- prior cleanup displacement; and
- current cleanup proposal.

### 11.4 Compute-once traces

One canonical producer creates each predicate result and trace root. Consumers receive immutable handles. A permitted alternate formulation is attempted only inside that producer in fixed precedence, after the first attempt, with both attempts recorded.

## 12. Exact expansion provider

### 12.1 Core requirements

The strict core must support:

- exact two-sum/difference;
- exponent-safe two-product;
- expansion grow/sum/scale/compress;
- exact expansion sign and zero;
- fixed-capacity buffers sized by reviewed worst-case formulas;
- checked capacity and work accounting;
- power-of-two input normalization;
- no heap allocation inside primitive determinant evaluation unless explicitly reserved; and
- finite/intermediate validation.

### 12.2 Formula semantics

Each exact formula specifies:

- algebraic expression over stored nominal bits;
- input order and roles;
- translation/subtraction graph where relevant;
- normalization policy;
- maximum expansion capacity;
- sign orientation convention; and
- operand-remapping behavior.

The exact formula may be algebraically chosen for robust sign semantics but must be versioned and consistent across producer/verifier/replay.

### 12.3 Legacy compatibility

Regression-test existing `two_product`, `two_sum`, expansion operations, `orient3d`, and `insphere` before and after wrapper redirection for supported finite inputs.

Legacy wrappers preserve public signatures and return conventions. They do not gain Component 03 owner/enclosure records. Bounded code never calls the wrapper API.

## 13. Construction conditioning

### 13.1 Common record

Every construction record contains:

- rounded graph and exact relation formula IDs;
- ordered source/carrier/plane provenance;
- nominal and enclosure;
- denominator interval and exact residual evidence;
- parameter interval/domain margin;
- endpoint/interior classification;
- cancellation/amplification indicators;
- source-edge/carrier and plane residuals;
- final rounding contribution;
- inherited and prior-cleanup contributors;
- local trace/ledger entry;
- available tolerance;
- required resulting precision; and
- typed category/disposition.

### 13.2 Edge-plane intersection

Given source endpoints `p0,p1` and plane residuals `r0,r1`:

1. compute each residual once;
2. obtain exact residual numerator signs/zeros when supported;
3. compute denominator `r0-r1` once;
4. identify coplanar/endpoint patterns from combined bounded and exact evidence;
5. choose one V1 parameter graph deterministically:
   - `t = r0/(r0-r1)`, or
   - `t = 1-r1/(r1-r0)`;
6. choose the endpoint form with smaller conservative residual upper bound, tie-broken by canonical endpoint identity;
7. interpolate using the matching endpoint graph;
8. compute independent source-carrier and plane residuals for the result;
9. compute precision/tolerance eligibility; and
10. publish only the complete bounded construction.

Both formulas reuse the same residual records and live under one producer trace.

### 13.3 Domain categories

- stable interior: entire parameter interval strictly inside `(0,1)` with positive margin;
- stable endpoint: exact endpoint numerator relation plus acceptable residuals;
- exact stored-coordinate tie: exact zero evidence retained for consumer-specific handling;
- coplanar/coincident: routed to Component 07;
- near-parallel bounded: denominator separated and all bounds within tolerance;
- ill-conditioned: required precision/residual/amplification exceeds tolerance; and
- invalid: owner, provenance, formula, finite, or trace failure.

Nominal `t` alone never determines category.

## 14. Precision traces and committed ledger

### 14.1 Local trace DAG

Each transaction or Component 17 private fragment builds an append-only local DAG containing:

- task-local node ID;
- rounded operation code/formula;
- exact relation reference where applicable;
- ordered parents;
- nominal/enclosure bits;
- contributor summaries;
- source/provenance references;
- resource/checkpoint evidence; and
- complete canonical merge key.

Parents precede children within a fragment. No mutable node is shared across workers.

### 14.2 Finalization

Before a value crosses a component boundary:

1. collect reachable local trace nodes;
2. validate owner, acyclicity, parent completeness, and formula compatibility;
3. canonical sort/group equivalent immutable nodes by complete key;
4. detect incompatible duplicate claims;
5. assign committed trace/ledger IDs;
6. remap task-local references;
7. compute contributor aggregation; and
8. independently verify the resulting records.

Compaction may share identical subgraphs but may not replace semantically different contributors because nominal/enclosure bytes happen to match.

### 14.3 Ledger entry

Store:

- stable ID/owner/version;
- operation and exact formula IDs;
- result value and ordered parents;
- source operands/provenance;
- nominal/enclosure bits;
- inherited precision A/B;
- machine floor;
- construction/conditioning contribution;
- exact tie marker and later consumer eligibility link;
- prior/current cleanup displacement;
- no-motion uncertainty subtotal;
- cumulative displacement subtotal;
- aggregate lineage precision;
- tolerance eligibility;
- trace/replay identity; and
- canonical digest contribution.

### 14.4 Aggregation

For V1 per lineage:

```text
no_motion_uncertainty = outward_sum(required inherited, machine, conversion,
                                    sequential construction/conditioning terms)
cumulative_displacement = outward_sum(committed displacement costs)
lineage_precision = outward_sum(no_motion_uncertainty, cumulative_displacement)
```

A versioned proof may use maximum for truly alternative contributions, but the default for sequential same-lineage terms is sum. The global output precision is the maximum lineage precision.

No historical record is edited. Narrowing affects an interval through proof; it does not erase historical uncertainty contributions unless a separately versioned proof establishes a tighter aggregate bound and the verifier reconstructs it.

## 15. Tolerance-budget service

### 15.1 Certificate units and kinds

Every cost is finite, nonnegative, and length-valued in the same coordinate units as tolerance. Define certificate kinds including:

- vertex displacement upper bound;
- patch correspondence/Hausdorff-style deviation upper bound;
- removable feature thickness/clearance upper bound;
- swept displacement upper bound; and
- whole-component deviation/diameter certificate under explicit policy.

Reject raw area, volume, angle, aspect ratio, or dimensionless quality as tolerance cost.

### 15.2 Proposal

A proposal contains:

- proposal/replay identity;
- cleanup operation kind;
- ordered unique affected lineage IDs;
- requested max length cost per lineage;
- feature/component certificate kind and evidence;
- local before evidence;
- Component 13 topology-authorization reference;
- transaction/stage owner; and
- canonical merge key.

Component 03 verifies arithmetic, units, lineage, and budget. Component 13 remains responsible for topology eligibility.

### 15.3 Reservation

Reservation:

- validates lineages and rejects duplicates;
- conservatively adds requested sequential cost to each committed total;
- checks tolerance and policy;
- reserves Component 01 record/resources;
- returns a move-only owner/transaction-bound token; and
- changes no published geometry.

Rollback/cancellation releases it automatically.

### 15.4 Commit

Commit receives actual after-evidence and proves:

- actual cost finite/nonnegative and `<=` reserved;
- after geometry lies within the certificate;
- feature/component removal does not exceed authorization;
- topology transaction succeeded;
- proposal order is canonical; and
- corresponding precision-lineage records can be committed.

Same-lineage costs outward-sum. Do not credit nominal vector cancellation. Disjoint lineages remain separate; global realized displacement is their maximum.

No stage transaction commits with an active reservation.

## 16. Conservative feature bounds

### 16.1 AABB construction

Construct from coordinate intervals:

- point AABB = component intervals;
- edge = coordinate hull of endpoint AABBs;
- triangle = hull of three point AABBs;
- facet/shell/operand = checked canonical union;
- event/carrier = bounded construction intervals; and
- proposed/swept cleanup patch = union plus explicit verified displacement inflation.

### 16.2 Queries

Expose:

- closed overlap;
- definite separation;
- union;
- proof-producing intersection;
- conservative squared-distance lower bound; and
- explicit additional inflation.

Touching is overlap. Component 06 owns spatial hierarchy and candidate order.

### 16.3 Representability

Exact finite singleton extrema remain representable through exactness-aware directed arithmetic. If genuine uncertainty requires an endpoint beyond finite `T`, fail. Never publish infinity or prune due to failed inflation.

## 17. Prior-result import and affine transforms

### 17.1 Re-ingestion

Accept prior output precision only through a verified versioned import record containing:

- prior output precision;
- per-lineage or conservative aggregate evidence;
- source/inherited contributions;
- construction history digest;
- cumulative cleanup displacement;
- serialization/rounding contribution;
- prior context/replay lineage as foreign provenance; and
- Component 15 publication verification.

Do not trust arbitrary public metadata. The public integration must provide the verified precision record through the frozen contract.

### 17.2 Transforms

- translation: bounded add for nominals; preserve uncertainty and add operation-local roundoff;
- power-of-two scaling: exact nominal scale when representable; scale all uncertainty/displacement by absolute scale;
- general finite affine map: fixed-group bounded matrix/vector operations and conservative operator contribution;
- projective/non-finite/unsupported transform: typed failure.

### 17.3 Tolerance gate

Before ordinary publication, every output lineage precision must be within the current tolerance. A larger tolerance cannot retroactively authorize undocumented prior edits.

## 18. Canonical encoding, diagnostics, and replay

### 18.1 Encoding

Use Component 01 `CanonicalBytes`; never raw structs/padding. Encode:

- all versions/profile IDs;
- owner/context/source linkage;
- exact nominal and interval endpoint bits;
- rounded operation and exact relation formula codes separately;
- ordered parents/provenance/lineages;
- scale and floor records;
- truth-layer predicate evidence;
- conditioning/residual records;
- local-trace finalization maps where retained;
- committed ledger records parent-before-child;
- budget records in canonical proposal order;
- AABBs/import/transform records;
- resource/cancellation summaries; and
- domain-separated digests.

Preserve nominal signed zero; canonicalize interval zero endpoints. No locale-dependent text.

### 18.2 Diagnostics

Every numerical failure records, within frozen capacity:

- component/stage/checkpoint/subcode/category;
- context/source/owner/profile IDs;
- operand and source/feature/provenance references;
- rounded operation and exact relation formula IDs;
- nominal operand/result bits;
- intervals and exact relation evidence;
- denominator/margin/residual/domain evidence;
- inherited/machine/construction/cleanup contributors;
- tolerance and budget state;
- resource/cancellation checkpoint; and
- replay identity.

Core arithmetic does not log. The public boundary renders completed errors.

### 18.3 Replay

Replay reconstructs the same:

- bootstrap and platform decision;
- rounded operation graph and nominal bits;
- directed enclosures;
- exact relation signs/zeros;
- predicate truth layers/disposition;
- construction category/residuals;
- trace finalization and ledger totals;
- budget reservation/commit decision;
- AABBs/import/transforms; and
- primary error/digest.

Thread count, task partition, and traversal order do not change canonical bytes.

## 19. Independent verifier

### 19.1 Independence

`PrecisionVerifier` consumes immutable records, exact bits, formula codes, parent references, and Component 01 capabilities. It must not trust stored `definite`, `within_tolerance`, `committed`, aggregate counts, or digests as sole evidence.

It may share low-level `FloatingBits`, strict exact-expansion core, and canonical-byte primitives. It must have separate record traversal, dispatch, aggregation, and state-machine implementations.

### 19.2 Reconstruction

The verifier must:

- revalidate type/build/runtime/profile/owner/version metadata;
- rescan bootstrap source bits and recompute scale/floor summaries;
- reconstruct each directed scalar endpoint operation;
- recompute rounded nominal graphs;
- recompute exact relation signs/zeros independently from formula records;
- verify nominal containment and finite intervals;
- rebuild predicate truth layers and dispositions;
- recompute construction parameters/residuals/categories;
- validate local trace finalization and no task-local escape;
- rebuild ledger parent graph and per-lineage totals;
- rebuild budget reservations/commits and ensure no active reservations;
- rebuild AABBs and prior-import/transform propagation;
- re-encode canonical bytes and SHA-256; and
- reject unknown, missing, duplicate, reordered, truncated, or contradictory records.

### 19.3 Required mutations

Reject deterministic mutations of:

- one interval endpoint;
- rounded nominal bits only;
- exact relation status only;
- rounded operation or exact formula ID;
- input ordering/orientation convention;
- inherited/machine/construction/displacement contributor;
- trace parent or finalization map;
- ledger parent/order/aggregate;
- budget units/certificate kind/cost/lineage/reservation;
- residual/parameter/domain category;
- AABB inflation;
- prior precision import;
- platform profile/owner/provenance; and
- one digest byte.

## 20. Resources, cancellation, transactions, and concurrency

### 20.1 Resources

Reserve Component 01 resource kinds for:

- source scan and scale records;
- bounded values and interval endpoints;
- directed rounding evidence;
- exact expansion limbs/work;
- local trace nodes/parents;
- committed ledger records;
- budget proposals/reservations/commits;
- AABBs/import/transform records;
- canonical bytes/diagnostics; and
- verifier work.

All count-to-byte arithmetic is checked. Fixed expansion capacity exhaustion is typed; no buffer overrun or silent truncation.

### 20.2 Cancellation

Poll at stable boundaries:

- source scan chunks;
- batch operation/trace construction;
- exact expansion fallback entry for large batches;
- trace finalization and canonical merge batches;
- ledger snapshot batches;
- budget reservation/commit boundaries;
- codec/replay batches; and
- verifier record batches.

A scalar primitive may finish locally, then discard at the next checkpoint. Do not expose half-mutated shared state.

### 20.3 Transactions

Precision preflight, final context, source imports, construction batches, trace finalization, ledger snapshots, budget commits, prior imports, codecs, and verifier proposals remain transaction-owned until verified.

Rollback destroys local values, releases resources/reservations, leaves predecessor artifacts unchanged, and publishes one typed failure/cancellation only.

### 20.4 Component 17 boundary

Implement an executable serial semantic reference first.

Parallel workers receive immutable inputs and private deterministic resource slices. They may produce private bounded values, exact relation records, trace fragments, construction records, and budget proposals. They may not:

- assign committed IDs;
- mutate shared ledger/budget maps;
- publish partial results; or
- choose failure by completion order.

Component 17 canonically merges complete keys, validates duplicates, assigns IDs, replays budget decisions in canonical order, invokes Component 03 verification, and reproduces serial bytes/error.

## 21. Narrow capability contracts

Export owner-checked read-only capabilities:

- **Component 02**: source import, plane/residual/projection, bounded/exact predicate evidence, source AABBs, input-precision and coherent-realization support.
- **Component 04**: projected bounded orientation, segment relations, planarity, source-triangle bounds, and exact relation evidence.
- **Component 05**: conservative normals/bounds only.
- **Component 06**: finite inflated AABBs, closed overlap/separation/distance lower bounds.
- **Component 07**: rounded bounded predicates, exact stored-coordinate relations, construction conditioning, immutable predicate/trace handles; no symbolic owner decision.
- **Component 08**: one bounded coordinate/parameter/lineage per event.
- **Component 09**: immutable margins and crossing evidence; no recomputation capability.
- **Component 10**: precision eligibility attached to retained uses.
- **Components 11-12**: bounded carrier ordering, projection, orientation, construction, and triangulation residuals.
- **Component 13**: length-certificate budget proposal/reservation/commit, displacement certificates, proposed/swept AABBs, and precision update.
- **Component 14**: aggregate output precision and canonical encoding views.
- **Component 15**: independent residual, exact relation, ledger, budget, AABB, import, and replay verification views.
- **Component 16**: test adapters and shared exact-oracle clients only.
- **Component 17**: immutable task inputs, private fragment factories, and canonical finalize APIs.

Every capability validates owner, versions, stage eligibility, transaction state, and required predecessor digest. No consumer receives mutable ledger or budget storage.

## 22. Required tests: platform, bits, and directed rounding

### 22.1 FloatingBits goldens

For both scalar types, commit exact bit expectations for:

- `-0`, `+0`, adjacent values around zero;
- smallest/largest subnormal;
- smallest normal and neighbors;
- adjacent values around `-1`, `1`, and powers of two;
- maximum finite and outward-step rejection;
- total-order keys; and
- exact power-of-two scaling boundaries.

### 22.2 Arithmetic profile

Test nearest-even ties, signed zero, underflow, cancellation, non-contracted multiply/add, prescribed grouping, debug/optimized strict builds, and worker entry.

Build negative probes with unsafe flags/profile changes and require rejection before topology work.

### 22.3 Directed rounding known answers

For add/subtract/multiply/divide test:

- exact singleton result;
- positive and negative residual direction;
- one-sided widening;
- two-sided fallback;
- subnormal result;
- exact maximum-finite result retained;
- genuinely unrepresentable outward result rejected; and
- sign combinations for interval extrema.

Verify exact rational endpoint result lies within every directed enclosure.

## 23. Required tests: exact relations, bounded operations, and conditioning

### 23.1 Shared exact oracle

Implement test-only base-`2^32` unsigned/signed integers, normalized rationals, exact finite-float dyadic import, and required determinant/projection expressions under the Component 16 shared names.

No production linkage. Add self-tests for carries, borrows, signs, normalization, GCD/reduction as available, and bit-to-dyadic boundaries.

### 23.2 Exact expansion tests

Compare strict expansion signs/zeros with the arbitrary-precision oracle across:

- 2x2/3x3 determinants;
- orient2d/orient3d fixtures;
- exponent extremes and normalization;
- exact cancellation;
- signed zeros/subnormals;
- capacity boundaries; and
- legacy wrapper compatibility.

### 23.3 Truth-layer tests

Explicitly cover:

- rounded nominal zero, exact relation nonzero;
- exact relation zero, uncertainty interval spans both signs;
- exact relation nonzero, interval overlaps zero;
- definite interval/exact relation agreement;
- exact relation unavailable;
- formula-ID mismatch; and
- Component 07 mock refusing ordinary uncertainty while accepting eligible tie evidence only after its own rule.

### 23.4 Oracle containment

For bounded integer/rational fixtures verify containment for:

- scalar arithmetic;
- interpolation;
- dot/cross/squared norm;
- plane construction/residual;
- determinants;
- parameter projection;
- edge-plane/edge-face construction;
- affine projection;
- AABBs; and
- displacement/removal certificates.

### 23.5 Conditioning boundaries

Include comfortable, just-inside, exact-threshold, just-outside, denominator-overlap, exact coplanar, exact endpoint, near-parallel bounded, large-translation/small-feature, mixed-magnitude, and finite-limit fixtures. Categories/errors are deterministic and no bare coordinate publishes on failure.

## 24. Required tests: trace, ledger, budget, bounds, import, replay, and mutation

### 24.1 Trace and ledger

Test:

- parent-before-child;
- cycle/forward reference/missing parent;
- wrong owner/stale/task-local escape;
- equivalent subgraph sharing versus false merge by equal nominal bits;
- contributor omission;
- proof narrowing;
- same-lineage outward sums;
- disjoint-lineage global maximum;
- private fragment merge under schedule permutations; and
- re-ingestion without reset.

### 24.2 Budget

Test:

- single/disjoint/repeated lineage edits;
- exact boundary and outward-over-boundary;
- valid displacement, patch-deviation, thickness/clearance, and component certificates;
- rejection of area/volume/angle/quality as length cost;
- rollback/cancellation/leaked reservation;
- actual below/equal/above reserved;
- canonical commit order; and
- nominal vector cancellation receiving no credit.

### 24.3 Conservative bounds

Compare point/edge/triangle/shell/event/carrier/proposed/swept bounds against exhaustive exact fixtures. Include one-ULP gaps, exact contact, subnormal separation, combined uncertainty, extreme scales, exact max-finite singleton, and truly unrepresentable inflation. No false negatives.

### 24.4 Import/transforms

Test prior verified precision import, corrupted/unverified metadata rejection, translation roundoff, exact power-of-two scale, general affine maps, unsupported projective/non-finite transforms, cleanup-before-transform, and tolerance gate.

### 24.5 Codec/replay

Commit golden bytes for representative contexts, values, operations, exact relations, predicates, constructions, traces, ledgers, budgets, AABBs, imports, and failures. Verify decode/re-encode, unknown/truncated/reordered records, full-key hash collision behavior, and thread/traversal stability.

### 24.6 Resource/cancellation/exception

For every resource and major checkpoint test limit-minus-one, limit, limit-plus-one; cancellation before/during/after; allocation/container exception injection; rollback idempotence; no active budget reservation; and zero publication.

### 24.7 Mutation

Apply every Section 19.3 mutation plus:

- change exactness evidence so a widened endpoint becomes singleton;
- force exact max-finite operation to fail despite a valid exactness proof;
- coerce exact zero to symbolic eligibility;
- coerce exact nonzero overlap to tie;
- change certificate units/kind; and
- drop one local trace node while preserving final nominal/enclosure.

Every required mutation must be rejected.

## 25. Implementation sequence and definition of done

Implement in this order. Every gate leaves the branch buildable and retains all previous tests.

1. Extend Component 01 registries, versions, IDs, resources, checkpoints, and subcodes.
2. Add strict shared expansion-core target and legacy baseline regression tests.
3. Add `FloatingBits` and bit-level goldens.
4. Add runtime/worker arithmetic profile qualification and negative probes.
5. Add exactness-aware directed add/subtract/multiply/divide and oracle tests.
6. Add finite intervals, hull, and proof intersection.
7. Add exact expansion adapter, formula registry, exponent normalization, and oracle comparison.
8. Redirect legacy adaptive wrappers to the strict core where the target graph is proven safe; preserve compatibility tests.
9. Add precision bootstrap, non-finite early failure, scale/floor derivation, and Component 01 freeze handshake.
10. Add bounded scalar/vector/point schemas and transaction-local traces.
11. Add vector, dot, cross, squared norm, plane, residual, determinant, projection, and parameter operations.
12. Add orthogonal predicate truth layers and deterministic alternate-formula policy.
13. Add edge-plane/edge-face construction conditioning and threshold tests.
14. Add local trace finalization, committed ledger, monotonic aggregation, and verifier traversal.
15. Add conservative AABBs and exhaustive no-false-negative tests.
16. Add length-certificate tolerance reservation/rollback/commit and mutation tests.
17. Add prior-result import and affine-transform propagation.
18. Add canonical codec, replay, diagnostics, and golden bytes.
19. Complete independent verifier and full mutation suite.
20. Add narrow capability adapters and compile-only integration tests for Components 02, 04, 06, 07, 13, 15, 16, and 17 without implementing those stages.
21. Run Component 01/02 regressions, strict debug/release, sanitizer profiles, supported compiler matrix, and deterministic schedule permutations.

Component 03 implementation is done only when:

- bootstrap/freeze and non-finite failure ownership are consistent with Components 01/02;
- the strict arithmetic profile is qualified at invocation and worker entry;
- exact stored-coordinate relation, rounded nominal, and uncertainty enclosure are separate and independently verified;
- ordinary uncertainty is never automatically sent to symbolic policy;
- directed arithmetic preserves exact finite extrema when proven and fails on genuinely unrepresentable bounds;
- the strict expansion core is exponent-safe, capacity/resource checked, dependency-free, and legacy-compatible;
- every primitive and geometric operation has fixed graph/formula IDs, exact-oracle containment, deterministic bytes, and typed failure;
- every construction has complete parameter, residual, conditioning, precision, and tolerance evidence;
- local traces finalize into complete committed lineages with no task-local escape or omitted contributor;
- precision never resets across cleanup, serialization, import, transform, or repeated Boolean;
- tolerance costs are conservative length-valued certificates and same-lineage displacement outward-sums;
- budget under-reporting, invalid units, and reservation mutations are rejected;
- conservative AABBs have no false negatives and never publish infinity;
- canonical replay is stable under supported builds, traversal permutations, and worker counts;
- independent verification reconstructs arithmetic, exact relations, traces, ledger, budget, bounds, imports, and digests without trusting producer booleans;
- resource, cancellation, wrong-owner, stale-handle, overflow, exception, and rollback tests prove zero partial publication;
- Component 01 and 02 regression suites remain passing; and
- all production and normative-test code is strict portable C++17 with no external dependency.

After this plan is reviewed and committed, `tracker.md` may mark Component 03 planning complete. That mark does not assert that the implementation gates above have been executed.