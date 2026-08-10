# Component 06: Broad-Phase Collision Enumeration

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete spatial-index provider may be a deterministic bounding-volume hierarchy, Morton hierarchy, sweep structure, spatial grid, hybrid, or another in-tree implementation selected later. The conservative-candidate, no-false-negative, topology-filtering, deterministic-order, resource, verification, and failure contracts in this document are normative.

## 0. Purpose

This component constructs conservative deterministic spatial acceleration over the two immutable source halfedge manifolds and emits every edge-triangle pair that may require authoritative relation evaluation.

Its purpose is to reduce the relation workload without changing Boolean semantics. It may reject pairs only when bounded geometry proves that the pair cannot interact. It may emit false positives. It must never omit a pair whose uncertainty envelopes admit an interaction relevant to Component 07.

The component performs no narrow-phase intersection test, no symbolic perturbation, no contact ownership, no crossing classification, no intersection construction, no event interning, and no winding decision. Broad-phase pruning is strictly a conservative scheduling operation.

The principal output is an immutable `canonical_candidate_stream` covering both directed relation domains:

- canonical edges of operand A against source triangles of operand B; and
- canonical edges of operand B against source triangles of operand A.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `canonical_source_manifolds` artifact from Component 05;
- the immutable Boolean context, stable identity services, resource policy, determinism policy, execution policy, cancellation, diagnostics, replay, and transaction services from Component 01;
- the immutable precision context and conservative-bound services from Component 03;
- canonical vertex, edge, halfedge, triangle, source-facet, and shell identities for both operands;
- conservative closed bounds for all indexed edge and triangle features;
- exact topological feature classifications and adjacency queries from Component 05;
- the selected broad-phase provider and artifact-format versions; and
- verification settings governing scalable and exhaustive candidate checks.

The component must not read mutable caller mesh data, reconstruct connectivity from coordinates, or replace Component 05 identities with provider-local identities at its public boundary.

### 1.2 Required predecessor guarantees

The component may rely on Component 05 having established:

- every source triangle appears exactly once in a canonical oriented triangle table;
- every undirected edge has exactly two reciprocal halfedge uses;
- every undirected edge has one stable canonical directed representative;
- source edges and facet-internal triangulation diagonals are distinguishable by identity;
- all feature owner, source-facet, shell, and provenance mappings are total;
- all published feature bounds are finite, closed, and conservative for inherited precision; and
- all feature arrays and identities have deterministic canonical order.

The component must nevertheless validate owner tokens, ranges, bound validity, and artifact digests before using the data. A contradiction in a committed predecessor artifact is an internal invariant failure and must not be hidden by broad-phase omission.

### 1.3 Candidate relation domain

The default Boolean candidate domain consists of two directed passes:

1. each canonical undirected edge feature of A, represented by its stable canonical direction, against every potentially interacting source triangle of B; and
2. each canonical undirected edge feature of B, represented by its stable canonical direction, against every potentially interacting source triangle of A.

A candidate is directed because the edge role and triangle role are not interchangeable in the Component 07 dependency graph. Operand exchange produces a different directed candidate key even when the same nominal geometric sets are involved.

Every undirected edge must enter a pass at most once through its canonical representative. Its two face-oriented halfedges remain available through Component 05 provenance and must not generate duplicate candidates merely because the edge has two incident triangles.

The treatment of facet-internal triangulation diagonals is provider-independent but policy-explicit:

- if Component 07 requires them to guarantee complete triangle-level relation coverage, they must be indexed and labeled as bookkeeping edges;
- if a selected provider omits some or all internal diagonals, it must prove through its candidate-domain contract and exhaustive oracle gates that all required source-facet interactions remain covered by source edges and opposite-operand edges;
- an internal diagonal that is emitted must never acquire original source-feature ownership; and
- legal re-triangulation of a source facet may change bookkeeping candidates but must not change canonical source relations, events, classifications, or final Boolean topology.

The candidate-domain policy must be versioned and recorded in replay metadata.

### 1.4 Accepted geometry and scale cases

The component must support:

- either or both operands empty;
- disjoint and widely separated operands;
- one operand contained in another;
- dense proper overlap;
- exact bound contact;
- point-, edge-, and face-touching configurations;
- coincident or nearly coincident surfaces;
- disconnected and nested shells;
- coordinate-coincident but topologically distinct features;
- zero-width bounds on one or more axes;
- signed zero and subnormal coordinates;
- large translations with small local geometry;
- extreme but finite representable scales; and
- uncertainty envelopes large enough to create many conservative false positives.

A difficult or highly overlapping case may exhaust a caller resource limit. It must not silently omit candidates or weaken bound inflation to continue.

### 1.5 Capacity and lifetime preconditions

Before index construction, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all indexed edge and triangle primitive references;
- all provider nodes, bins, sort keys, ranges, and temporary workspaces;
- all candidate records or deterministic stream partitions;
- all canonical sort and duplicate-removal work;
- verification evidence and structural counters; and
- worst-case counters up to the configured candidate and work limits.

Published streams and indices must refer only to immutable predecessor artifacts and immutable stage-owned storage whose lifetime covers all Component 07 consumption and verification.

## 2. Required behavior

### 2.1 Conservative feature bounds

The component must obtain or reconstruct a conservative closed spatial bound for every indexed edge and triangle through Component 03 services. The exact bound type may be axis-aligned boxes, quantized conservative boxes, or another provider-specific enclosure, but it must support a sound definite-separation test.

For every primitive bound, the component must account for:

- the nominal source coordinates;
- inherited operand precision;
- coordinate-wise and radial point uncertainty;
- rounding introduced while constructing the acceleration bound;
- provider-specific quantization or encoding error;
- any transform or serialization uncertainty already carried by the input; and
- closed-set contact at exact bound endpoints.

Bounds must not be expanded by the user tolerance merely as a universal proximity epsilon. They must be expanded by conservative geometric uncertainty required to avoid false negatives. Tolerance may affect whether a later construction is acceptable, but it is not a license to invent broad-phase adjacency unrelated to the precision model.

If a conservative finite bound cannot be represented, expansion overflows, quantization would become non-conservative, or a bound contains NaN, the component must fail with a typed numerical or invariant error. It must not clamp to a smaller box, replace the feature by an empty bound, or skip the primitive.

### 2.2 No-false-negative contract

Define an edge-triangle pair as **broad-phase relevant** when the conservative feature enclosures do not prove the two closed features separated under the qualified arithmetic model and candidate-domain policy.

The component must emit every broad-phase-relevant pair in both directed operand roles required by Section 1.3.

Equivalently, pruning is permitted only when a bounded separation result is definite. A nominal gap, center distance, open-box test, unchecked plane-side test, or heuristic size threshold is insufficient.

The no-false-negative guarantee applies to:

- proper transverse intersections;
- tangencies;
- endpoint and boundary contact;
- coplanar and coincident configurations;
- interactions admitted only because uncertainty envelopes overlap;
- distinct topological features with identical coordinates; and
- every symbolic tie category that Component 07 may need to resolve.

False positives are expected and permitted. A candidate's existence does not assert that an intersection, contact, or symbolic tie actually occurs.

### 2.3 Deterministic index construction

The component must build one or more conservative spatial indices using canonical primitive inputs. Regardless of provider, construction must satisfy:

- each indexed primitive appears in the required index exactly once unless documented replicated references are provably deduplicated at enumeration;
- every leaf or bucket bound contains all assigned primitive bounds;
- every internal node bound contains all descendant bounds;
- split, axis, key, and partition ties are resolved by frozen total ordering over canonical feature IDs;
- provider-local node IDs and array order are deterministic for canonical input;
- temporary hash or parallel partition order does not affect publication;
- empty and singleton indices have explicit canonical forms; and
- the index format is versioned and independently checkable.

A provider may build separate edge and triangle indices per operand, a mixed hierarchy, or a dual-tree structure. The public candidate semantics must remain identical.

Spatial keys such as Morton codes may guide construction only if their quantization is conservative and deterministic at the supported floating-point extremes. Equal keys must be secondarily ordered by full canonical feature identity.

### 2.4 Hierarchy and bound verification

Before enumeration, the component must verify all provider structure invariants, including as applicable:

- child and primitive ranges are valid and non-overlapping where required;
- no primitive is lost or multiply owned unintentionally;
- node bounds contain children and leaf primitives;
- parent-child graphs are acyclic and rooted;
- empty nodes obey the provider contract;
- sort keys and canonical feature order agree;
- count and byte arithmetic did not overflow; and
- canonical serialization does not depend on padding or uninitialized bytes.

Verification must use reconstructed child and primitive bounds rather than trusting producer summary flags.

### 2.5 Conservative pair traversal

The enumeration algorithm must traverse the relevant edge and triangle structures for both directed operand passes. It may reject a node pair or primitive pair only after a definite closed-bound separation result.

Traversal must:

- include exact-boundary box contact;
- treat interval overlap as closed;
- preserve candidates when any axis comparison is uncertain;
- avoid unchecked subtraction that can overflow or lose a one-ULP overlap;
- use Component 03 bounded comparisons where provider arithmetic is not trivially exact;
- terminate under configured work limits; and
- visit no provider state through data races or mutable shared traversal cursors.

A traversal order may be optimized for locality, but published candidate order must be canonical and independent of traversal order.

### 2.6 Topological filtering

Topological filtering is permitted only from exact Component 05 identities and only when the candidate-domain policy proves the relation is irrelevant.

For the ordinary cross-operand Boolean passes, coordinate coincidence, equal source labels by numeric value, matching spatial keys, same nominal plane, shell contact, or close proximity are not grounds for exclusion.

If same-operand enumeration is used by a verification or diagnostic mode, exclusions such as an edge against its own incident triangle may be applied only through exact incidence queries. A feature may not be declared adjacent because its coordinates match or lie within tolerance.

Internal-diagonal filtering must follow the versioned candidate-domain policy from Section 1.3. It must never suppress a required source-facet relation merely because the current triangulation provides an apparently redundant diagonal.

Every applied filter category must have a stable reason code and structural counter so exhaustive tests can distinguish deliberate exclusion from accidental omission.

### 2.7 Candidate identity

Every emitted pair must have one canonical candidate key containing enough information to distinguish at least:

- the directed operand role (`A-edge/B-triangle` or `B-edge/A-triangle`);
- canonical edge identity;
- canonical triangle identity;
- edge semantic class;
- candidate-domain or provider policy version; and
- any future relation-family discriminator required to avoid key reuse.

Candidate IDs must be allocated only after deterministic key canonicalization. IDs must not depend on discovery order, BVH node order unless that node order is itself canonical and non-semantic, worker number, temporary buffer, or hash insertion order.

Duplicate discovery of the same key through replicated leaves, twin halfedges, overlapping bins, or multiple traversal paths must collapse to one candidate record. Full keys, not hashes alone, must determine equality.

### 2.8 Candidate ordering and deterministic parallel merge

The published candidate stream must have a total canonical order. The default order should be lexicographic over the complete candidate key, but another versioned total order is permitted.

Parallel enumeration may write only to task-local buffers or deterministically owned partitions. Publication must:

- finish or cancel all producers;
- validate task-local records;
- merge by canonical key;
- remove exact duplicate keys deterministically;
- assign final candidate IDs and ordinals in canonical order;
- produce the same primary failure under all schedules; and
- commit only after complete verification.

Use of `std::unordered_*` is permitted only for non-authoritative lookup with explicit deterministic sort before publication. Iterator order may not leak into candidate IDs, diagnostics, digests, or Component 07 work partitioning.

### 2.9 Materialized versus streamed output

The concrete provider may materialize all candidates or expose an immutable deterministic stream over canonical partitions. Either form must provide the same observable contract:

- a stable total candidate count when construction succeeds;
- deterministic iteration from beginning to end;
- stable random-access or partition boundaries if parallel Component 07 consumption is supported;
- no candidate loss when a consumer pauses or retries;
- no hidden dependence on mutable traversal state;
- canonical digestability; and
- complete rollback on failure or cancellation.

A streamed provider must still detect candidate-count and work-limit exhaustion deterministically before ordinary publication, or use reservations and a transaction protocol that guarantees no partial stream is mistaken for success.

### 2.10 Candidate provenance and overlap evidence

Each candidate record must provide or reference enough immutable evidence for Component 07 and independent verification, including:

- edge operand and triangle operand;
- canonical edge and triangle IDs;
- canonical directed edge endpoints;
- the two incident edge halfedges and their source-facet provenance where applicable;
- triangle vertices, orientation, source facet, and shell;
- edge source/internal-diagonal semantics;
- conservative edge and triangle bounds;
- the bound-overlap or traversal witness that admitted the pair;
- combined precision references relevant to the pair;
- topological-filter status and reason code, normally `not_filtered` for emitted pairs;
- canonical candidate key and ID; and
- artifact, provider, and policy versions.

The overlap witness is diagnostic evidence, not a narrow-phase result. It must not contain an invented intersection point or assert a contact class.

### 2.11 Separation from narrow-phase semantics

The component must not:

- evaluate vertex-face, edge-edge, edge-face, or triangle-triangle predicates beyond what is required for conservative bound separation;
- choose a crossing sign or multiplicity;
- classify coplanarity, tangency, coincidence, or sidedness;
- invoke symbolic perturbation;
- decide source-feature ownership;
- create, round, or intern intersection coordinates;
- merge candidate keys because nominal geometry is equal; or
- remove candidates through a heuristic believed likely to be safe.

If an optimization requires a geometry test stronger than conservative bound separation, that test becomes part of the normative broad-phase contract and must return bounded definite/uncertain outcomes through Component 03. Uncertain results must retain the pair.

### 2.12 Resource limits and pathological overlap

The component must account separately for:

- indexed edge and triangle primitive references;
- provider nodes, buckets, keys, and replicated references;
- construction and sorting work;
- node-pair and primitive-pair traversal work;
- emitted pre-deduplication records;
- final unique candidates;
- merge and verification workspaces;
- diagnostics and replay evidence; and
- persistent index and stream bytes.

Worst-case candidate output may be quadratic when many primitives genuinely have overlapping conservative bounds. The component is not required to hide that fact. It must instead:

- use output-sensitive accounting;
- reserve before large allocations;
- stop at deterministic checkpoints when limits are exceeded;
- return `resource_limit` with canonical progress and feature witnesses;
- publish no truncated candidate stream; and
- never reduce precision inflation or omit candidates to meet a target.

Hard correctness limits must be distinguished from advisory performance targets.

### 2.13 Cancellation and transactionality

Index construction, traversal, merge, deduplication, and verification must occur inside one stage transaction or a documented sequence of private subtransactions that publish only one final immutable artifact.

Cancellation must be polled at deterministic safe points during:

- primitive-bound preparation;
- spatial-key generation;
- index construction;
- node-pair traversal;
- candidate-buffer flushes;
- canonical merge and duplicate removal; and
- verification.

On cancellation, all workers must stop at safe points, join, release reservations, and discard every unpublished node and candidate. The result is `cancelled`, never a partial ordinary success.

### 2.14 Structural instrumentation

The artifact and diagnostics must record deterministic structural counters sufficient to assess correctness and scaling, including:

- edge and triangle primitive counts by operand and semantic class;
- provider node and leaf counts;
- replicated primitive references if any;
- node-pair tests;
- primitive bound-overlap tests;
- definite-separation prunes;
- uncertain-overlap retentions;
- topological exclusions by reason;
- pre-deduplication and unique candidate counts;
- maximum deterministic work-queue size or depth;
- resource reservations and peak persistent bytes; and
- cancellation progress counters.

Wall-clock timings may be diagnostic but must not affect correctness, canonical output, or release qualification by themselves.

### 2.15 Independent verification evidence

The component must publish enough structure for a verifier to check:

- primitive coverage of each index;
- leaf and internal-node containment;
- valid and acyclic provider structure;
- candidate owner and ID validity;
- closed overlap of each emitted candidate's conservative bounds;
- canonical key order and duplicate freedom;
- correct directed operand role;
- exact topological filter reasons;
- deterministic counts and digest inputs; and
- no false negatives on bounded verification domains.

For small or policy-bounded artifacts, the verifier must exhaustively enumerate the entire directed edge-triangle domain, apply the same public definite-separation and exact topological-filter contracts through independently implemented control flow, and compare the expected and emitted candidate key sets.

For scalable production artifacts, verification may combine hierarchy containment, complete primitive membership, independent candidate-record checks, structural traversal evidence, and policy-selected independent sweep or sampled exhaustive partitions. The release gate remains the exhaustive all-pairs oracle over all bounded fixtures and generated small meshes.

The verifier must not call the producer's tree traversal, candidate merge, or topological-filter helper as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `canonical_candidate_stream` artifact containing or referencing:

- artifact, provider, candidate-domain, and serialization versions;
- immutable conservative primitive-bound tables or references;
- deterministic spatial-index data required by diagnostics and verification;
- all unique directed candidates for A-edge/B-triangle and B-edge/A-triangle passes;
- canonical candidate IDs, keys, and total order;
- edge and triangle provenance required by Component 07;
- edge semantic classifications;
- bound-overlap evidence and precision references;
- topological-filter reason metadata and counters;
- deterministic partition boundaries for permitted parallel consumption;
- resource and structural statistics;
- independent-verification evidence;
- canonical input and output digests; and
- replay metadata sufficient to reproduce provider construction and candidate order.

Each candidate record must identify, directly or through immutable tables:

- directed operand role;
- canonical edge ID and canonical direction;
- canonical triangle ID and orientation;
- incident halfedge, source-facet, shell, and source-feature provenance;
- source-edge versus internal-diagonal semantics;
- conservative edge and triangle bounds;
- precision-ledger references;
- canonical key, ID, and ordinal; and
- diagnostic overlap witness.

The output must guarantee:

- every pair admitted by the public conservative candidate-domain contract is present;
- no candidate key appears more than once;
- both directed operand passes are represented completely;
- all candidates reference valid immutable Component 05 features;
- each emitted pair survives the public closed-bound overlap test;
- exact-boundary and uncertain overlaps are retained;
- topological filtering used only exact identities and documented reason codes;
- candidate identity and order are independent of input presentation after canonicalization, provider work partition, thread count, and schedule;
- no relation sign, symbolic decision, event coordinate, or Boolean classification has been invented; and
- Component 07 can consume the artifact without recomputing broad-phase discovery.

If either operand has no relevant edges or the opposite operand has no triangles, the component must publish a valid empty stream with canonical metadata and digest.

On failure, no index or candidate stream is published to Component 07. The typed error must include the failing stage, provider version, operand roles, canonical feature or node witnesses, numerical bounds where relevant, candidate and work counters, resource state, and deterministic replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- every prune is justified by definite conservative separation or an exact documented topology exclusion;
- closed-bound contact is never pruned;
- uncertainty retains rather than removes a pair;
- all required edge-triangle roles are enumerated in both operand directions;
- every undirected edge enters a directed pass through one canonical representative;
- candidate keys are unique and totally ordered;
- candidate IDs are assigned after deterministic canonicalization;
- spatial indices contain every required primitive with conservative bounds;
- node bounds contain descendants;
- false positives are permitted and observable;
- false negatives are prohibited;
- broad-phase choices cannot change symbolic tie semantics;
- all artifacts are immutable, context-owned, transactional, and independently verifiable; and
- resource limits cause typed failure rather than candidate omission.

Prohibited behavior:

- using tolerance as a universal broad-phase expansion or equality rule;
- pruning from nominal coordinates when uncertainty envelopes overlap;
- using open interval or open box overlap for contact-sensitive candidates;
- filtering coordinate-coincident features as duplicates;
- inferring adjacency or source identity from coordinates, hashes, Morton codes, or bounding boxes;
- emitting both halfedge twins as duplicate edge candidates;
- allowing internal diagonals to own source-feature semantics;
- relying on random split choices, pointer addresses, unstable sort ties, unordered iteration, or worker discovery order;
- capping a candidate list by truncation;
- narrowing or clamping a conservative bound to avoid overflow;
- performing narrow-phase classification or symbolic perturbation;
- publishing a partial stream after cancellation or resource exhaustion; or
- calling an external BVH, collision, mesh, parallel-runtime, or geometry library.

## 5. Test and validation specification

### 5.1 Bound and overlap unit tests

Unit tests must cover closed conservative bounds for:

- ordinary edges and triangles;
- points and features with zero extent on one or more axes;
- exact-boundary contact;
- one-ULP gaps and overlaps;
- positive and negative zero;
- subnormal coordinates;
- adjacent representable values near large magnitudes;
- extreme finite exponents;
- large translations with small local features;
- inherited uncertainty larger than the nominal gap; and
- provider quantization or expansion boundaries.

For every case, test definite separation, retained overlap, uncertain comparison, overflow, and non-finite rejection paths.

### 5.2 Known-answer candidate sets

Commit hand-auditable fixtures with exact expected candidate keys for:

- empty versus empty and empty versus non-empty operands;
- two disjoint tetrahedra;
- nested boxes with no boundary intersection but overlapping feature boxes where applicable;
- one transverse triangle pair;
- vertex-touching, edge-touching, and face-touching boxes;
- equal boxes;
- thin crossing slivers;
- disconnected shell collections;
- coordinate-coincident but topologically separate shells; and
- concave polygon-derived facets with internal diagonals.

Expected sets must distinguish the A-edge/B-triangle and B-edge/A-triangle roles.

### 5.3 Exhaustive all-pairs oracle tests

For every bounded fixture and generated small manifold pair:

1. enumerate every canonical edge-triangle pair in both directed operand roles;
2. reconstruct conservative feature bounds independently;
3. apply the public definite-separation rule;
4. apply only exact documented topological filters; and
5. compare the complete expected key set with the component output.

Any missing expected key is a release-blocking false negative. Extra keys are allowed but must remain within documented false-positive expectations and resource limits.

Run the oracle across `float` and `double`, supported index widths, all candidate-domain policy versions retained for replay, and every provider used in production.

### 5.4 Floating-point and uncertainty adversarial tests

Include:

- exact box-boundary equality;
- separation by one representable value;
- overlapping uncertainty despite separated nominal features;
- nearly parallel long edges and thin triangles;
- huge boxes containing tiny features;
- subnormal extents;
- signed-zero min/max ordering;
- values near maximum finite magnitude;
- expansion that would overflow without checked arithmetic;
- quantized keys at cell or Morton-code boundaries; and
- distinct features with identical bounds.

Compare all small cases with the in-tree exact or higher-precision test oracle where applicable.

### 5.5 Topology and provenance tests

Verify candidate construction for:

- one undirected source edge with two reciprocal halfedges, emitted only once per directed pass;
- internal triangulation diagonals under each supported candidate-domain policy;
- adjacent source triangles and same-operand diagnostic exclusions;
- duplicate coordinates with distinct source IDs;
- equal source numeric IDs in different operand namespaces;
- source-facet and shell provenance;
- legal alternative triangulations of one source polygon; and
- global source subdivision that preserves the solid.

Alternative triangulation may change bookkeeping candidate counts, but canonical source relations and later Boolean results must remain invariant.

### 5.6 Determinism and metamorphic tests

Apply:

- source vertex, facet, shell, and component permutations;
- canonical primitive input reversal;
- provider leaf-size and deterministic build partition variants allowed by policy;
- build-order permutations;
- axis-tie configurations;
- exactly representable translations;
- power-of-two scales;
- axis permutations;
- operand exchange with directed-role remapping;
- thread counts 1, 2, and maximum;
- forced task delays and reversed merge order; and
- repeated execution.

For a fixed provider and policy version, canonical index serialization where normative, candidate keys, IDs, order, counters, digest, and primary failure must be byte-identical.

### 5.7 Mutation tests

Corrupt valid artifacts or provider evidence by:

- deleting one primitive from an index;
- assigning one primitive twice without supported replication metadata;
- shrinking a leaf or parent bound;
- creating a parent-child cycle;
- changing a primitive owner operand;
- omitting one expected candidate;
- duplicating a candidate through both halfedge twins;
- reversing the directed operand role;
- changing an edge or triangle ID;
- marking an uncertain overlap as definitely separated;
- changing a topological exclusion reason;
- labeling an internal diagonal as a source edge;
- scrambling canonical candidate order;
- assigning IDs before sort and then permuting tasks;
- truncating at a candidate limit; and
- forging counts or digests.

Independent verification must reject every structural mutation. Exhaustive bounded oracles must detect every omitted candidate.

### 5.8 Fuzzing and shrinking

Generate valid manifold operand pairs from exact rational templates, then vary:

- shell count and nesting;
- triangle count and aspect ratio;
- spatial clustering;
- disjoint separation;
- controlled overlap density;
- source triangulation;
- coordinate duplication without identity merging;
- ULP perturbations;
- uncertainty envelopes;
- provider settings; and
- resource limits.

Every discovered false negative, nondeterminism, overflow, invariant failure, or unjustified resource explosion must serialize exact input bits, policies, provider version, and counters and shrink while preserving the failure category.

### 5.9 Resource and cancellation tests

For nodes, primitive references, traversal work, pre-deduplication candidates, unique candidates, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during bound preparation, key generation, index construction, traversal, buffer merge, duplicate removal, and verification. Confirm all workers join, reservations return, and no partial stream is visible.

Dense all-overlap fixtures must fail deterministically with `resource_limit` when configured below the true conservative candidate requirement. Raising the limit must reveal the complete candidate set rather than a different truncated prefix.

### 5.10 Structural performance tests

Use deterministic counters to require:

- near-linear or `O(n log n)` index construction under the selected provider;
- no primitive-pair tests for clearly separated top-level bounds beyond documented constant work;
- subquadratic primitive tests for large disjoint and spatially localized fixtures;
- output-sensitive behavior for clustered overlap;
- no accidental all-pairs enumeration in ordinary sparse scenes;
- bounded memory proportional to provider structures plus emitted candidates; and
- invariant candidate output across optimization settings permitted by one policy version.

Worst-case quadratic candidate output is acceptable only when conservative bounds genuinely admit that many pairs or the provider's documented worst case is triggered. It must be visible in structural counters and controllable through typed resource limits.

### 5.11 Definition of done

Component 06 is complete only when:

- both directed edge-triangle candidate domains are emitted completely;
- conservative feature bounds and closed overlap semantics are independently validated;
- exhaustive small-case comparison finds no false negatives;
- source adjacency and internal-diagonal handling use exact versioned topology rules;
- candidate keys are unique, canonical, and schedule-independent;
- empty, disjoint, touching, coincident, dense-overlap, and extreme-scale cases have explicit tested behavior;
- mutation tests catch omitted candidates and corrupt hierarchy evidence;
- resource and cancellation paths publish no partial stream;
- structural benchmarks prevent accidental all-pairs behavior in sparse scenes;
- Component 07 can consume candidates without re-running broad-phase discovery; and
- production and normative tests are strict portable C++17 with no external dependencies.
