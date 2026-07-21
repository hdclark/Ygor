# Component 03: Precision, Tolerance, and Bounded Arithmetic

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production code and normative tests must use portable C++17 and the C++ standard library only. No external interval, exact-arithmetic, geometry, hashing, serialization, testing, or concurrency dependency may be required, linked, vendored, downloaded, or invoked.

The implementation technique may change, but the observable contracts, numerical distinctions, conservative-bound obligations, failure behavior, evidence requirements, integration boundaries, and tests in this document are normative.

## 0. Purpose

Component 03 is the numerical foundation for Components 02-15. It permits later stages to use bounded floating-point geometry without allowing floating-point coincidence or an arbitrary epsilon to become topology.

It owns:

- qualification of the authoritative floating-point execution model;
- scale-aware machine-roundoff floors;
- normalization and propagation of operand input precision;
- finite conservative enclosures for source and constructed values;
- deterministic rounded nominal operation graphs;
- exact stored-coordinate relation evidence for supported algebraic predicates;
- conditioning and residual evidence for geometric constructions;
- uncertainty-aware predicate results;
- append-only precision lineage;
- transactional tolerance-budget accounting for later cleanup;
- conservative finite feature bounds;
- repeated-operation precision import and propagation;
- deterministic canonical encoding and replay evidence; and
- an independent verifier for every published Component 03 artifact.

Component 03 does not assign topology, validate shell incidence, choose Boolean ownership, resolve operation-specific symbolic ties, perform cleanup mutations, construct output connectivity, or publish an ordinary Boolean result.

Failure is required whenever the component cannot provide a finite conservative enclosure, cannot establish the required execution model, cannot distinguish an eligible exact stored-coordinate tie from unresolved uncertainty, or cannot prove that a construction remains within the caller-authorized contract.

## 1. Input contract

### 1.1 Required inputs

Component 03 must accept, through narrow owner-checked capabilities:

- the Component 01 pending-context view for precision bootstrap;
- the Component 01 frozen Boolean context for final service construction;
- exact source coordinate bit patterns for both immutable operand snapshots;
- the qualified V1 scalar descriptor for `T`, initially `float` or `double`;
- declared or versionedly derived `input_precision_a` and `input_precision_b`;
- the caller's maximum tolerance;
- the selected enclosure, contraction, signed-zero, subnormal, and rounding policies;
- stable provenance, owner, transaction, stage, and replay identities;
- Component 01 checked arithmetic, resources, cancellation, diagnostics, canonical bytes, SHA-256, and immutable publication services; and
- later, immutable source, construction, event, cleanup, and output-lineage records supplied by Components 02-15.

The precision bootstrap must run before Component 01 freezes the final context. The final precision context must be constructed only after Component 01 publishes the frozen owner token and verifies the bootstrap record.

### 1.2 Required numerical distinctions

No API, record, diagnostic, or implementation shortcut may collapse the following into one undifferentiated `epsilon`:

1. **Stored source value**: the exact finite binary value represented by a source `T` bit pattern.
2. **Rounded nominal value**: the deterministic `T` result of one recorded, fixed-grouping operation graph.
3. **Exact stored-coordinate relation**: when available, the exact sign or exact zero of a specified polynomial or rational numerator evaluated over the stored finite binary inputs as exact real numbers, before inherited uncertainty is applied.
4. **Machine-roundoff floor**: unavoidable uncertainty introduced by storage, conversion, and the prescribed arithmetic graph.
5. **Inherited input precision**: uncertainty already attached to a source operand or a re-ingested prior result.
6. **Construction uncertainty**: conservative propagated uncertainty and conditioning amplification for a new geometric value.
7. **Classification margin**: a conservative separation from a decision boundary when a sign or ordering is definite.
8. **Cleanup displacement**: actual authorized movement accumulated along a geometric lineage.
9. **Feature-removal cost**: a conservative length-valued certificate that an authorized removed feature lies within the caller's geometric budget.
10. **Output precision**: the conservative per-lineage and global uncertainty published with a result.
11. **User tolerance**: the maximum authorized geometric deviation or feature removal.

The terms **exact stored-coordinate relation** and **rounded nominal value** are not interchangeable. Expansion arithmetic may prove the exact sign of a determinant over stored floating-point coordinates; it does not retroactively change the prescribed rounded nominal operation graph.

### 1.3 Source bootstrap and finiteness

Component 01 source capture preserves exact coordinate bits, including malformed non-finite values, so replay remains faithful. Component 03 must scan those bits before deriving scales or machine floors.

If any source coordinate is NaN or infinite:

- precision bootstrap must return a deterministic `input_contract_error` identifying the operand and source position;
- no precision preflight or frozen precision context may publish;
- the failure must not be reported as `unsupported_platform`, `invalid_tolerance`, or an internal invariant; and
- Component 02 must retain a defensive finiteness check in its own validator even though the ordinary pipeline will normally fail during bootstrap first.

This early bootstrap rejection is numerical eligibility checking, not publication of a validated operand and not transfer of Component 02's topology responsibilities.

### 1.4 Source-value requirements

Every finite source value imported after context freeze must carry:

- exact nominal bits;
- a stable source provenance identity;
- the applicable declared or inherited precision;
- a conservative machine-floor contribution;
- an owner token and provider/schema versions;
- a finite enclosure containing every accepted source realization; and
- a precision-lineage reference or transaction-local trace that becomes a committed lineage reference before crossing a component boundary.

Duplicate coordinates are ordinary data. Equal nominal bits, overlapping intervals, or distance below tolerance never establish topological identity, event identity, adjacency, ownership, or permission to weld.

## 2. Required behavior

### 2.1 Qualified V1 arithmetic model

The component must define and verify the arithmetic assumptions under which all bounds are valid. V1 requires:

- `T` is qualified IEC 60559 binary32 or binary64;
- nearest-even rounding for authoritative operations;
- fixed and versioned expression grouping;
- no fast-math, reassociation, unsafe reciprocal substitution, or finite-only assumption;
- prohibited floating contraction for authoritative V1 operation graphs;
- documented stores and conversions to `T` at every graph node;
- preserved signed-zero semantics;
- gradual underflow with supported subnormals;
- range-checked integer-to-floating conversion;
- no reliance on `long double`, excess register precision, compiler-specific extended scalars, or unqualified SIMD behavior;
- no process-global rounding-mode mutation by Component 03; and
- no transcendental function as the sole authority for topology.

Component 01 owns the invocation/worker floating-environment guard. Component 03 must verify the active qualified profile at bootstrap, final context construction, and every Component 17 worker entry. A mismatch returns `unsupported_platform` before topology-affecting work.

Every supported compiler/build/runtime profile must have committed bit-pattern witnesses. Unknown profiles are unsupported until explicitly qualified.

### 2.2 Two-phase bootstrap and freeze handshake

The bootstrap phase must:

1. validate the pending context, type descriptor, policy versions, strict-build evidence, counts, resource limits, and cancellation state;
2. scan exact source coordinate bits without using caller-owned storage;
3. reject non-finite source coordinates as specified in Section 1.3;
4. compute operand-local and global scale descriptors;
5. derive per-coordinate and aggregate machine-floor requirements;
6. preserve declared input precision separately from machine-floor contributions;
7. derive effective input precision conservatively;
8. determine ordinary-success eligibility against the caller tolerance without spending cleanup budget;
9. encode the complete preflight record canonically; and
10. return a digest bound to source bits, normalized options, type profile, arithmetic policy, and platform evidence.

Component 01 then freezes the final context, incorporates and independently validates the preflight record, and publishes the owner token.

Final Component 03 context construction must recompute or independently validate all bootstrap-sensitive fields and reject stale preflight objects, different source snapshots, changed policies, changed platform evidence, wrong owners, or digest mismatch.

No partially initialized global precision service is permitted.

### 2.3 Scale and machine-roundoff floors

The component must derive conservative floors from operation-local magnitudes and source representation, not only from one global bounding-box diagonal.

Scale descriptors must cover:

- per-axis finite minima and maxima;
- per-axis maximum absolute coordinate;
- finite spans computed with bounded subtraction;
- global and operand-local maximum absolute values;
- smallest observed non-zero magnitudes;
- normal, subnormal, and signed-zero presence;
- mixed-magnitude and large-translation indicators;
- exact source-bit digests; and
- safe power-of-two normalization exponents for exact algebraic relations.

A source-coordinate enclosure must conservatively include:

- declared or inherited input precision;
- the applicable representation and import floor;
- any source-format conversion contribution; and
- outward rounding used to form the enclosure.

The machine floor must remain meaningful near zero, through the normal/subnormal boundary, at large translations with small local features, under power-of-two scaling, and for mixed-magnitude operands.

If a required finite source enclosure or floor cannot be represented, the operation must fail with a typed numerical status. It must not substitute infinity, clamp to a finite maximum, or continue with an under-sized bound.

### 2.4 Bounded value types

The component must provide immutable bounded abstractions for at least:

- scalar;
- 2D and 3D vector;
- point;
- unnormalized plane;
- carrier parameter;
- residual;
- orientation/determinant evidence;
- finite closed AABB; and
- displacement or feature-removal certificate.

Every published bounded value must contain or reference:

- exact rounded nominal bits;
- a finite conservative enclosure;
- owner and schema/provider versions;
- stable value identity;
- provenance and geometric lineage;
- operation/formula trace;
- inherited, machine, construction, and cleanup contribution summaries;
- precision-ledger evidence;
- deterministic canonical encoding; and
- validity and publication disposition.

A bounded point must support conservative coordinate-wise and radial queries, conceptually equivalent to:

```cpp
struct bounded_point {
    vec3<T> nominal;
    vec3<T> axis_error;
    T radial_error;
    provenance_id provenance;
};
```

The stored representation may be tighter, but every required query must remain conservative.

Planes must be represented without normalization, for example by bounded normal, offset, anchor, and squared-normal evidence. A plane is usable only when the squared-normal lower bound is definitely positive.

### 2.5 Finite outward arithmetic

The component must provide conservative forms of at least:

- import and conversion;
- addition, subtraction, negation, multiplication, and division;
- explicit multiply-then-add and multiply-then-subtract;
- hull and proof-producing interval intersection;
- scalar/vector interpolation from either endpoint;
- vector add, subtract, and scale;
- dot and cross products;
- squared norm and conservative radial bounds;
- affine projection;
- plane construction and plane residual;
- 2x2 and 3x3 determinants required by consumers;
- parameter projection along a carrier;
- finite AABB construction, inflation, union, intersection, overlap, and separation; and
- rounding or serialization conversion to `T`.

Each operation must use the frozen operation graph and propagate every input enclosure. Bounds must never shrink because nominal values cancel.

For a correctly rounded scalar node, the implementation may use either:

- a proved exactness-aware directed enclosure, retaining a singleton endpoint when the operation is proven exact; or
- a conservative predecessor/successor widening when exactness is not proven.

Unconditional one-ULP widening is an acceptable fallback only when it remains finite. The provider must not reject an exactly representable finite boundary result merely because it failed to use an available exactness proof. Tests must cover exact operations at extreme finite values separately from genuinely unrepresentable outward bounds.

Division must return uncertain/invalid evidence when the denominator enclosure contains zero. It must never clamp the denominator or invent a reciprocal.

A narrowing operation is permitted only when an independently checkable proof shows that the true value lies in both parent constraints. The narrowing record must identify the proof kind and parents.

### 2.6 Exact stored-coordinate relation evidence

For supported determinant and algebraic relation families, Component 03 must provide an audited strict-target exact relation service over the finite stored source or constructed nominal bits.

The service must:

- evaluate the specified polynomial, determinant, or rational numerator exactly as a real expression over the stored binary inputs;
- return exact negative, exact zero, exact positive, unavailable, or invalid evidence;
- use exponent-safe power-of-two normalization where required;
- use fixed-capacity, capacity-checked expansion storage or another self-contained exact method;
- reject overflow, unsupported scaling, non-finite input, and capacity exhaustion with typed status;
- record the exact relation formula ID and ordered inputs;
- remain independent from inherited uncertainty; and
- never replace the conservative enclosure path.

The exact relation service must not claim to be the exact value of the rounded nominal graph. The rounded graph and exact relation formula must each have their own stable identifier.

For non-polynomial constructions, exact endpoint or coplanarity ties may be established from an exact supported numerator relation. A rounded parameter equal to zero or one by itself is not proof of an exact tie.

### 2.7 Predicate result model

Topology-affecting comparisons must not return only `bool` and must not encode all evidence in one ambiguous enum.

A predicate result must carry three orthogonal parts:

1. **bounded sign status**: definitely negative, definitely positive, overlaps the decision boundary, or invalid;
2. **exact stored-coordinate relation status**: negative, zero, positive, unavailable, or invalid; and
3. **consumer disposition**: accept definite numeric sign, retain exact-tie evidence for relation-specific eligibility, try one permitted deterministic alternate bounded formulation, route to coplanar/coincident handling, fail for unresolved condition/tolerance, or fail invalid.

A compatibility derived classification may expose:

- definitely negative;
- exact stored-coordinate tie evidence;
- definitely positive;
- uncertain; or
- invalid.

However, an exact stored-coordinate zero whose uncertainty enclosure also permits separated realizations must retain both facts. Component 03 must not automatically authorize symbolic resolution. Component 07 may invoke the operation-specific symbolic policy only after its relation contract proves that the tie is eligible; ordinary uncertainty remains ordinary uncertainty.

Every result must include:

- rounded nominal bits;
- finite enclosure;
- exact relation evidence when requested;
- conservative boundary margin when definite;
- uncertainty width and contributor summary when non-definite;
- provenance, operation trace, formula IDs, and owner;
- conditioning and tolerance evidence; and
- deterministic diagnostics and replay identity.

Equivalent formulations may be attempted only inside the one canonical producer, in a fixed versioned precedence, under one trace root. Consumers must reuse the immutable result rather than recompute the question.

### 2.8 Construction conditioning

For every constructed intersection, projection, parameter, ordering coordinate, or carrier, the component must assess:

- endpoint/source uncertainty;
- plane or carrier uncertainty;
- denominator separation from zero;
- parameter uncertainty and domain margin;
- cancellation and magnitude amplification;
- source-edge or carrier residual;
- support-plane residual;
- final rounding contribution;
- inherited precision and prior cleanup displacement;
- required output precision; and
- available caller tolerance.

At minimum, edge-plane and edge-face constructions must classify as:

- stable interior;
- stable endpoint with exact tie evidence;
- exact stored-coordinate tie requiring consumer-specific handling;
- coplanar/coincident relation requiring Component 07;
- near-parallel but conservatively bounded within tolerance;
- ill-conditioned beyond tolerance; or
- invalid.

The entire parameter interval, not only its nominal value, determines interior/domain classification.

Ill-conditioned or over-tolerance constructions must return `geometric_condition_exceeds_tolerance` or a more specific typed status. No bare nominal coordinate may escape.

### 2.9 Precision ledger and lineage

Component 03 must maintain append-only precision lineage for every geometric artifact that crosses a component boundary or contributes to public output.

A committed ledger entry must identify:

- stable entry and result identities;
- owner, schema, and provider versions;
- operation and exact-relation formula IDs;
- ordered parent entries;
- source operand/provenance contributors;
- rounded nominal bits and result enclosure;
- inherited input precision by operand;
- machine-floor contribution;
- construction/conditioning contribution;
- exact tie marker and eligibility evidence supplied by consumers;
- prior and current cleanup displacement;
- conservative aggregate precision;
- tolerance disposition;
- trace and replay identity; and
- canonical digest contribution.

Transaction-local primitive arithmetic may use compact local traces rather than globally allocating a committed ledger entry for every temporary scalar. Before a value crosses a component boundary, participates in a committed event/output lineage, or is serialized as a published artifact, its complete local trace must be canonicalized into immutable ledger evidence. This distinction is required for bounded resource use and Component 17 private-fragment merging; it must not omit any contributor.

For each geometric lineage, V1 aggregate precision must conservatively include sequential no-motion uncertainty and cumulative displacement. Sequential contributors combine by outward-rounded sum unless a versioned proof justifies a tighter rule. Alternative or disjoint lineages may combine by maximum only for a global summary.

At minimum:

```text
lineage_precision >= inherited precision
lineage_precision >= machine-floor contribution
lineage_precision >= construction uncertainty
lineage_precision >= cumulative cleanup displacement
lineage_precision >= each required parent contribution
```

The global output precision is the maximum conservative lineage precision over all published output geometry.

Historical entries are immutable. Cleanup and re-ingestion append new lineage; they do not edit or reset prior uncertainty.

### 2.10 Tolerance-budget service

The component must expose an auditable transactional budget service used primarily by Component 13 and verified by Components 14-15.

It must distinguish:

- uncertainty that exists without moving geometry;
- actual vertex displacement;
- cumulative displacement along each lineage;
- local feature-removal authorization;
- whole-component removal authorization; and
- global caller tolerance.

Every budget quantity compared directly with `tolerance` must be a conservative non-negative length bound. Area, volume, angle, raw triangle area, or dimensionless quality values must not be compared directly with a length tolerance. A feature-removal proposal must translate its topology-specific proof into a versioned length-valued geometric deviation certificate, such as a conservative displacement, clearance, thickness, or Hausdorff-style upper bound.

A cleanup proposal must:

1. identify all affected geometric lineages and the topology operation kind;
2. provide a conservative requested maximum cost per lineage and feature/component certificate;
3. reserve the worst-case cost before mutation;
4. fail if any sequential lineage total would exceed tolerance or policy;
5. commit only after the topology transaction succeeds and actual conservative after-evidence is supplied; and
6. roll back automatically on failure, cancellation, or transaction abort.

For V1, sequential displacement on the same lineage combines by outward-rounded sum. Nominal vector cancellation does not reduce cost. Disjoint lineages retain separate totals; the global maximum realized displacement is their maximum.

No stage may commit while a reservation remains active. Every committed budget record must create corresponding precision-lineage evidence.

### 2.11 Repeated-operation and transform propagation

When a prior Boolean result is re-ingested, Component 03 must preserve verified published precision, construction lineage digest, cleanup displacement, serialization/rounding contribution, and foreign provenance. Exact representability of public coordinates does not reset uncertainty.

Precision propagation helpers for supported affine transforms must include:

- translation with operation-local roundoff;
- exact representable power-of-two scale with scaled uncertainty and displacement;
- general finite affine maps with fixed-grouping bounded matrix/vector operations; and
- typed failure for non-finite, unsupported, or projective transforms.

A new operation is ordinary-success eligible only when every required input and resulting output lineage remains within the new caller tolerance. A larger later tolerance does not authorize unrecorded prior cleanup.

### 2.12 Conservative finite feature bounds

Component 03 must provide finite closed AABBs constructed from bounded coordinates, not nominal coordinates plus an ad hoc epsilon.

It must support:

- source and constructed point bounds;
- edge bounds as coordinate-wise hulls of endpoint intervals;
- triangle and polygon-support bounds;
- shell and operand bounds;
- constructed event and carrier bounds where requested;
- explicit inflation by additional verified uncertainty or displacement;
- closed overlap, definite separation, union, proof-producing intersection, and conservative squared-distance lower bounds; and
- deterministic encoding of contributors.

Touching at an endpoint counts as overlap. No interaction permitted by the enclosures may be pruned.

If a required finite conservative bound cannot be represented, the stage must fail rather than substitute infinity or prune the feature. Exactness-aware arithmetic must preserve exactly representable finite boundary boxes when possible.

Component 06 owns acceleration structure shape, candidate identity, candidate ordering, and pruning policy.

### 2.13 Residual and verifier-facing services

Component 03 must expose narrow read-only services sufficient for Component 15 to independently check that:

- a constructed point lies on its source edge or carrier within its enclosure;
- a constructed point satisfies its defining plane relation within its enclosure;
- a cleanup output remains within its displacement certificate;
- a triangle orientation is definite or accepted under the explicit degeneracy policy;
- non-adjacent features are definitely separated outside uncertainty envelopes;
- every published output precision dominates all contributors; and
- canonical bytes, relation formula IDs, ledger totals, budget totals, and AABBs are internally consistent.

Verifier APIs must operate on immutable records and exact bits. They must not require producer-private booleans or mutable maps.

## 3. Output contract

On success, Component 03 must publish:

- one immutable `precision_context<T>` linked to the frozen Component 01 context;
- owner-checked bounded arithmetic and exact-relation capabilities;
- transaction-local trace factories and canonical ledger-finalization services;
- immutable precision-ledger snapshots;
- transactional tolerance-budget services and committed snapshots;
- conservative finite-bound capabilities;
- canonical codec and replay records; and
- independent verifier results.

The precision context must contain:

- arithmetic, exact-relation, enclosure, and serialization versions;
- qualified scalar/build/runtime profile metadata;
- rounding, grouping, contraction, signed-zero, and subnormal policies;
- operand-local and global scale descriptors;
- declared and effective input precision;
- machine-floor summaries;
- caller tolerance and ordinary-success eligibility;
- bound-combination rules;
- stable operation and formula registries;
- source/preflight/frozen-context linkage; and
- owner identity.

Every published construction must contain:

- rounded nominal value;
- finite conservative enclosure;
- exact stored-coordinate relation evidence where applicable;
- provenance and lineage;
- conditioning and domain classification;
- residual evidence;
- precision-ledger reference;
- tolerance disposition;
- deterministic diagnostic encoding; and
- replay identity.

Every committed cleanup budget record must contain:

- affected lineages;
- operation kind and length-valued certificate kind;
- authorized maximum;
- reserved and actual conservative costs;
- cumulative per-lineage totals;
- local before/after evidence;
- linked precision-ledger entry; and
- replay identity.

No downstream component may receive a topology-affecting bare coordinate, bare parameter, bare sign, or bare cleanup cost.

## 4. Required invariants and prohibited behavior

Required invariants:

- every published enclosure contains every accepted realization under the qualified arithmetic model and inherited uncertainty;
- all published nominals and bounds are finite;
- every rounded nominal graph and exact relation formula has a stable distinct identity;
- exact stored-coordinate zero is distinguishable from a non-zero exact relation whose uncertainty overlaps zero;
- symbolic tie evidence is distinguishable from authorization to apply symbolic policy;
- bounds never shrink without independently reconstructible proof;
- precision never resets or decreases across stages, cleanup, serialization, transforms, or repeated Booleans;
- every cross-component geometric value has complete provenance and committed precision lineage;
- every tolerance cost compared with tolerance is a conservative length bound;
- cleanup cost is never under-reported;
- deterministic execution produces identical nominal bits, intervals, relation evidence, classifications, ledger/budget bytes, and primary errors; and
- failure, cancellation, resource exhaustion, or verifier rejection publishes no partial artifact.

Prohibited behavior:

- topology from raw unbounded floating comparisons;
- tolerance-based welding or arbitrary snapping;
- treating an interval containing zero as an exact tie;
- treating exact stored-coordinate zero as automatic symbolic eligibility;
- hiding a near-zero denominator by clamping;
- publishing NaN, infinity, or a saturated finite bound;
- using a rounded parameter equal to an endpoint as sole exact-tie proof;
- conflating the exact stored-coordinate relation with the rounded nominal graph;
- depending on register excess precision, unqualified FMA, fast math, reassociation, or process-global rounding mutation;
- dimensionally invalid comparison of area, volume, or quality metrics with a length tolerance;
- using coordinate equality, interval overlap, hash equality, or proximity as identity;
- producer-only verification that trusts stored booleans or digests as sole evidence; or
- use of an external numerical or geometry dependency.

## 5. Integration contracts

### 5.1 Component 01

Component 01 owns source capture, pending/frozen context state, public option normalization, platform guard, owner tokens, resources, cancellation, errors, transactions, canonical bytes, SHA-256, replay, and publication.

Component 03 owns the scale-sensitive precision preflight and final numerical services. The two-phase handshake must match Component 01's `platform/precision bootstrap` contract exactly.

### 5.2 Component 02

Component 02 consumes source-point imports, planes, residuals, projected predicates, conservative bounds, precision lineage, and validation evidence. Component 03 does not validate indexed topology or shell semantics.

Both components defensively check source finiteness. Bootstrap may fail first because Component 02 cannot run without a precision context.

### 5.3 Components 04-06

Component 04 consumes bounded projection, orientation, planarity, segment-relation, and source-triangle bound services. Component 05 consumes conservative normals and bounds without transferring topology authority. Component 06 consumes finite inflated AABBs and closed overlap/separation queries.

### 5.4 Components 07-10

Component 07 is the sole operation-specific relation and symbolic-policy authority. It consumes immutable bounded predicates, exact stored-coordinate relation evidence, constructions, residuals, and conditioning. It must not apply symbolic policy to ordinary unresolved uncertainty.

Component 08 interns one bounded coordinate/parameter/lineage per event. Component 09 consumes immutable crossing and classification margins rather than recomputing relations. Component 10 carries precision eligibility with retained surface uses.

### 5.5 Components 11-15

Components 11-12 consume bounded ordering, construction, projection, and triangulation residual services. Component 13 alone proposes topology-authorized cleanup and uses the budget service. Component 14 consumes aggregate output precision and canonical encoding inputs. Component 15 independently verifies residuals, lineage, budgets, bounds, and publication precision.

### 5.6 Components 16-17

Component 16 owns permanent shared qualification infrastructure. Component 03 must seed, not duplicate, the shared in-tree exact integer/rational oracle and mutation framework.

Component 17 may execute private bounded-operation and ledger fragments against immutable inputs. It must merge in canonical order, assign committed identities deterministically, replay budget decisions serially in canonical order, and invoke Component 03 verification before commit. The executable serial implementation remains the semantic reference.

## 6. Test and validation specification

### 6.1 Platform and bit-pattern tests

Commit known answers for `float` and `double` covering:

- positive and negative zero;
- smallest and largest subnormals;
- normal/subnormal boundaries;
- adjacent values around zero, one, powers of two, and large magnitudes;
- maximum finite values and exact versus unrepresentable outward results;
- cancellation-prone operations;
- integer conversion boundaries;
- fixed-grouping and contraction-sensitive expressions;
- signed-zero arithmetic; and
- power-of-two normalization.

Altered rounding mode, flush-to-zero behavior, unsafe contraction, fast-math, reassociation, unsupported compiler profile, and worker-profile mismatch must fail before topology work.

### 6.2 In-tree exact oracle

Normative tests must use a deliberately slow shared in-tree arbitrary-precision integer/rational oracle implemented in portable C++17. Production code must not include or link it.

For bounded integer/rational fixtures, verify enclosure containment for:

- scalar arithmetic;
- interpolation;
- dot and cross products;
- squared norms;
- determinants and exact stored-coordinate signs;
- plane construction and residuals;
- parameter projection;
- edge-plane and edge-face construction;
- affine projection;
- AABB inflation; and
- accumulated displacement and feature-removal certificates.

The Component 03 implementation must place this oracle where Component 16 can extend it without creating a second implementation.

### 6.3 Truth-layer separation tests

Required tests must distinguish:

- rounded nominal zero with exact stored-coordinate non-zero relation;
- exact stored-coordinate zero with a non-singleton uncertainty enclosure;
- exact stored-coordinate non-zero relation whose enclosure overlaps zero;
- definite interval sign agreeing with exact relation;
- exact relation unavailable for a supported bounded comparison;
- formula-ID mismatch between rounded graph and exact relation; and
- mutation that changes only one truth layer.

Component 07 test doubles must prove that ordinary uncertainty is not sent to symbolic policy.

### 6.4 Arithmetic and property tests

Use a fixed deterministic PRNG stratified over signs, zeros, subnormals, exponents, adjacent values, and extreme finite values. Verify:

- exact oracle containment;
- monotonic widening when inherited precision increases;
- no unproved bound shrinkage;
- deterministic nominal bits and operation traces;
- exactness-aware singleton preservation when proven;
- finite typed failure instead of NaN, infinity, or saturation;
- operand symmetry/remapped symmetry where required; and
- stable serialization under traversal and worker-count permutations.

### 6.5 Conditioning tests

Construct edge-plane, edge-face, projection, and carrier-ordering fixtures that are:

- comfortably conditioned;
- just inside tolerance;
- exactly at the conservative threshold;
- just outside tolerance;
- denominator-overlapping zero;
- exact coplanar;
- exact endpoint-tied;
- near-parallel but bounded;
- large-translation/small-feature;
- mixed-magnitude; and
- close to finite representability limits.

Categories and typed failures must change only according to recorded conservative evidence, without arbitrary coordinate jumps.

### 6.6 Precision-ledger tests

Test:

- parent-before-child order;
- cycles, forward references, duplicate IDs, wrong owner, and stale handles;
- omitted inherited, machine, construction, or displacement contributors;
- local trace finalization into committed lineage;
- proof-producing versus unjustified narrowing;
- same-lineage sequential accumulation;
- disjoint-lineage global maximum;
- serialization and re-ingestion without reset; and
- private Component 17 fragment merge matching the serial reference.

### 6.7 Budget tests

Test:

- one displacement;
- repeated same-lineage edits;
- disjoint lineages;
- exact budget boundary;
- outward rounding beyond the boundary;
- reservation rollback and cancellation;
- actual cost below/equal/above reservation;
- feature removal with valid length certificate;
- rejection of area/volume compared directly with tolerance;
- component-removal authorization;
- leaked reservation;
- canonical commit order; and
- nominal vector cancellation that must not reduce conservative cost.

Mutation tests must omit a lineage, reduce a cost, change certificate units/kind, or commit without a reservation and require deterministic verifier rejection.

### 6.8 Conservative-bound tests

Compare point, edge, triangle, shell, event, and carrier bounds against exhaustive exact fixtures. Include one-ULP gaps, exact contact, subnormal separation, combined source uncertainty, extreme scales, exact maximum-finite singletons, and truly unrepresentable expansions. No permitted interaction may be excluded.

### 6.9 Replay, resource, cancellation, and mutation tests

Commit canonical golden bytes for representative contexts, bounded values, exact relation records, predicates, constructions, ledger entries, budget records, AABBs, and failures.

Verify:

- decode/re-encode identity;
- unknown-version, truncation, duplicate-field, reordering, and overflow rejection;
- deterministic replay of nominal bits, enclosures, exact relation evidence, categories, budgets, and errors;
- limit-minus-one, limit, and limit-plus-one for every resource class;
- cancellation before, during, and after stable checkpoints;
- exception injection at allocation/transaction boundaries; and
- zero publication after every failure path.

Required mutations include shrinking an interval, changing only exact relation evidence, changing only rounded nominal bits, removing inherited precision, changing a formula ID, reducing cleanup cost, changing owner/provenance, corrupting residuals, reordering parents, changing platform profile, and changing one digest byte.

### 6.10 Definition of implementation done

Component 03 implementation is complete only when:

- the two-phase bootstrap/freeze handshake is implemented and independently verified;
- non-finite source bootstrap failure and Component 02 defensive recheck are consistent;
- the V1 strict floating profile is qualified for every supported build/runtime/worker configuration;
- every primitive operation has fixed graph identity, finite conservative bounds, oracle containment, and deterministic bytes;
- exact stored-coordinate relation evidence is separate from rounded nominal and uncertainty evidence;
- no ordinary uncertainty is automatically authorized for symbolic resolution;
- every construction has conditioning, residual, precision, and typed failure evidence;
- every cross-component value has complete committed precision lineage;
- tolerance uncertainty, displacement, feature removal, and component removal remain dimensionally and semantically separate;
- budget reservation/rollback/commit and verifier reconstruction reject every under-reporting mutation;
- conservative bounds have no false negatives in exhaustive bounded tests;
- repeated-operation import never resets uncertainty;
- replay is bit-stable under supported traversal and worker permutations;
- independent verification reconstructs results without trusting producer booleans;
- resource, cancellation, owner, overflow, exception, and rollback tests prove zero partial publication; and
- production and normative-test code remain strict portable C++17 with no external dependency.

## 7. Planning-review completion

`tracker.md` records completion of the specification/plan review workflow, not implementation completion. Component 03 may be marked complete in `tracker.md` after this specification and `plan_03_precision_bounded_arithmetic.md` have been independently reviewed, corrected, and made mutually consistent with the broad plan and Components 01, 02, 04, 06, 07, 13, 15, 16, and 17.

The implementation definition of done in Section 6.10 remains a future handoff gate and is not a prerequisite for marking the planning-review task complete.