# Component 16: Test Infrastructure and Qualification

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. All production code, normative tests, test oracles, generators, mutators, shrinkers, replay readers, corpus tools, and qualification utilities must be portable C++17 and use no external dependency.

The concrete test harness, fixture syntax, arbitrary-precision integer representation, rational normalization provider, mesh generator families, fuzz scheduler, shrink search, corpus storage layout, coverage database, benchmark driver, and release-report renderer may change. Contract-to-test traceability, independent in-tree oracles, deterministic generation and shrinking, permanent regression retention, qualification matrices, release evidence, and the input/output contracts in this document are normative.

## 0. Purpose

This component provides the permanent verification and qualification system for the entire Boolean engine. It is not an optional collection of unit tests and it is not deferred until implementation is complete. It defines the machinery by which every normative contract clause, invariant, degeneracy category, policy version, typed failure, deterministic guarantee, resource limit, and performance gate is converted into executable evidence.

Its purposes are to:

- provide a deterministic in-tree C++17 test harness for component, integration, end-to-end, replay, mutation, fuzz, metamorphic, resource, cancellation, and performance tests;
- maintain a versioned traceability inventory from specification clauses to executable tests and qualification evidence;
- provide exact low-complexity arithmetic and geometric oracles implemented entirely in-tree for normative tests;
- provide hand-auditable fixtures and analytic known-answer solids;
- generate valid closed manifold operands with controlled topology, geometry, precision, contact, and degeneracy properties;
- generate invalid inputs and corrupt intermediate artifacts with one or more precisely classified contract violations;
- run deterministic property and fuzz campaigns whose failures are byte-for-byte replayable;
- shrink failing operands, options, artifacts, and schedules while preserving the relevant failure predicate;
- retain every discovered defect as a permanent minimized regression;
- exercise all operations, operand orders, scalar/index types, tolerance boundaries, policy versions, thread counts, and resource boundaries;
- establish release qualification through reproducible manifests, corpus digests, result digests, and coverage evidence; and
- prevent performance optimization or provider replacement from weakening correctness coverage.

This component does not supply production geometric decisions. Test-only exact arithmetic, exhaustive enumeration, artificial schedulers, injected digest collisions, and fault-injection allocators must not become dependencies of the ordinary production pipeline.

The principal outputs are:

- an immutable `qualification_suite_manifest` describing all required tests and matrices;
- a versioned permanent regression corpus;
- deterministic test, fuzz, mutation, benchmark, and replay executables or entry points;
- a `qualification_evidence_bundle` containing pass/fail results, structural counters, version identifiers, and digests; and
- a release qualification decision that cannot be reported as passing while required evidence is missing.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the normative contracts and stable clause identifiers for Components 01 through 17;
- public and internal artifact schemas exposed for testing by each component;
- producer and independently implemented verifier entry points for each component;
- the public `bounded_boolean` API and supported diagnostic/replay APIs;
- all frozen operation, solid, contact, output, verification, determinism, execution, resource, and diagnostic policy versions;
- scalar and index type descriptors for every supported production instantiation;
- platform and floating-point-environment qualification descriptors from Component 01;
- precision and bounded-arithmetic test interfaces from Component 03;
- deterministic execution controls and structural performance counters from Component 17;
- replay encoders/decoders and artifact logical-serialization contracts;
- an in-tree corpus root or equivalent test-resource provider;
- a deterministic campaign seed and campaign configuration; and
- explicit qualification scope, such as local component development, pull-request qualification, nightly stress, or release qualification.

The component must not require network access, an external geometry engine, an external arbitrary-precision package, an external fuzzing library, an external graph package, an external test framework, an external serializer, or an external corpus service.

### 1.2 Specification traceability input

Every normative requirement intended to be testable must have a stable identifier or canonical path.

The traceability input must distinguish:

- public contract clauses;
- component input and output requirements;
- required behavior clauses;
- invariants;
- prohibited behavior;
- typed failure semantics;
- policy/version compatibility rules;
- deterministic requirements;
- resource/cancellation requirements;
- mandatory verification gates;
- performance structural gates; and
- definition-of-done criteria.

A requirement may map to several tests. A test may cover several requirements. The mapping must be explicit and machine-checkable rather than inferred from test names alone.

### 1.3 Component test-adapter contract

Each component must expose a narrow in-tree test adapter sufficient to:

- construct valid immutable predecessor artifacts;
- invoke the component under a frozen context;
- read the resulting artifact or typed failure;
- invoke the independent verifier;
- enumerate logical artifact content needed for mutation;
- replace selected fields through a test-only builder without invoking undefined behavior;
- inspect deterministic structural counters;
- inject resource limits and cancellation at defined checkpoints;
- select provider and policy versions when several qualified implementations exist; and
- serialize a complete replay record.

Test adapters must not bypass production validation in a way that makes successful-path tests unrepresentative. When constructing intentionally corrupt committed artifacts, the adapter must clearly mark them as test-only and prevent them from escaping into production interfaces.

### 1.4 Deterministic seed contract

Every generated test campaign must be controlled by a versioned deterministic seed record containing:

- campaign kind and version;
- root seed bits;
- deterministic pseudo-random generator version;
- generator family and parameter ranges;
- operation and policy distributions;
- scalar/index type selection;
- thread-count and schedule controls;
- resource-limit distribution;
- mutation distribution;
- shrink policy version; and
- expected iteration count or stopping condition.

The same seed record on the same qualified platform and provider versions must generate the same ordered sequence of logical test cases. Standard-library engines whose exact cross-implementation sequence is not guaranteed must not be used as the replay authority unless wrapped by a fully specified in-tree algorithm.

### 1.5 Corpus input contract

Every corpus record must have a versioned canonical representation containing, as applicable:

- exact source coordinate bit patterns;
- source index arrays and facet ring boundaries;
- concrete `T` and `I` identifiers;
- operation and normalized options;
- declared input precisions;
- expected success or typed failure;
- expected stable subcode or finding class where normative;
- expected public topology or content digest;
- expected geometry, precision, cleanup, and verification status;
- accepted result-equivalence policy when exact public bytes are intentionally not normative;
- provider and policy version constraints;
- originating campaign and seed;
- minimized failure predicate;
- links to relevant contract-clause identifiers; and
- record content digest.

Corpus decoding must validate all lengths, counts, indices, scalar encodings, enum values, versions, and digests before allocation or execution.

### 1.6 Oracle-domain contract

The exact test oracle is deliberately bounded in scope. Every oracle invocation must declare and enforce limits for:

- source integer coordinate magnitude;
- arbitrary-precision limb count;
- rational numerator and denominator size;
- vertex, edge, face, shell, and triangle counts;
- candidate relation count;
- exhaustive permutation count;
- winding-ray intersection count;
- Boolean cell or arrangement complexity where used; and
- total oracle work units.

An out-of-domain oracle case is a test configuration error or explicit skipped qualification item, never an approximate exact answer.

### 1.7 Qualification-scope contract

The component must define at least four scopes:

- **component scope**, fast tests for one component and its immediate contracts;
- **integration scope**, cross-component artifact and mutation tests;
- **continuous qualification scope**, deterministic broad matrices suitable for routine development;
- **release scope**, the complete required matrix, permanent corpus, stress budgets, supported types/platforms, and performance gates.

Every scope must have an immutable manifest. A smaller scope must not be mislabeled as release qualification.

### 1.8 Resource and lifetime preconditions

The test system must account for:

- fixture and corpus bytes;
- exact-oracle limbs and rationals;
- generated mesh entities;
- mutation copies;
- replay and shrink candidates;
- retained failure artifacts;
- parallel test workers;
- benchmark data;
- coverage inventory records;
- result logs and evidence bundles;
- temporary and persistent bytes; and
- abstract campaign work units.

A test process must not rely on unbounded memory growth to retain every generated success. It must retain complete information for every failure and only policy-selected evidence for passing cases.

## 2. Required behavior

### 2.1 In-tree test harness

The component must provide or use an in-tree C++17 test harness whose normative behavior includes:

- deterministic test discovery and order;
- stable test identifiers;
- explicit pass, fail, skip-with-reason, and infrastructure-error states;
- typed assertion records with exact scalar-bit and entity witnesses;
- isolation of one test's mutable state from another;
- deterministic output independent of worker completion order;
- resource and cancellation support for long-running cases;
- canonical machine-readable result encoding; and
- nonzero process status when any required test fails or is unexpectedly missing.

The harness may execute tests concurrently, but result order and release evidence must be canonical. Wall-clock duration may be recorded as non-authoritative metadata and must not affect pass/fail except in explicitly versioned benchmark envelopes.

### 2.2 Contract-to-test traceability inventory

The component must maintain a machine-readable inventory mapping every normative clause to executable evidence.

For each clause, the inventory must record:

- clause identifier and source document version;
- responsible component;
- one or more test identifiers;
- test level: unit, known-answer, property, metamorphic, adversarial, mutation, replay, resource, or performance;
- supported type and policy coverage;
- whether the test is mandatory in component, continuous, or release scope;
- most recent result in the evidence bundle; and
- explicit rationale when a clause is not directly executable.

Release qualification must fail when a required clause has no mapped test, when a mapped test is absent, or when evidence is stale relative to the implementation or specification version.

### 2.3 Canonical fixture representation

Hand-authored and generated fixtures must use a canonical logical representation independent of native struct layout.

The representation must preserve:

- exact scalar bit patterns, including signed zero and subnormals;
- facet ring boundaries and index sequences;
- duplicate-coordinate distinct indices;
- operand roles;
- operation and options;
- expected artifact or failure data;
- tolerance and input precision;
- source provenance labels used only for test readability; and
- version identifiers.

Text may be provided for reviewability, but the canonical replay authority must define exact integer and scalar encodings without locale-dependent parsing ambiguity.

### 2.4 Exact arbitrary-precision integer service for tests

The component must provide a test-only exact signed integer type implemented in-tree using portable C++17.

It must support, at minimum:

- canonical zero and sign representation;
- exact comparison;
- addition and subtraction;
- multiplication;
- division with quotient and remainder where required;
- exact divisibility checks;
- greatest common divisor;
- bit-length and limb-limit accounting;
- conversion from supported fixed-width integers;
- deterministic canonical serialization; and
- overflow-free behavior within configured resource limits.

The limb base, multiplication provider, and normalization strategy may change. Arithmetic must be exact, canonical, deterministic, and independently tested against bounded fixed-width cases and algebraic identities.

### 2.5 Exact rational service for tests

The component must provide a test-only exact rational type built on the in-tree exact integer service.

Each rational must have a canonical representation:

- denominator strictly positive;
- numerator and denominator reduced by their greatest common divisor;
- zero represented with denominator one; and
- no negative zero.

Required operations include:

- exact comparison and sign;
- addition, subtraction, multiplication, and division by nonzero values;
- exact conversion from fixed-width integers and exact binary floating-point values;
- determinant and orientation expressions;
- interpolation and plane-intersection expressions required by low-complexity oracles;
- canonical serialization; and
- explicit resource-limit failure.

Production code must not link to or call this service.

### 2.6 Exact conversion of binary floating-point inputs

For oracle comparisons, the component must convert finite supported `float` and `double` bit patterns exactly into dyadic rational values.

The conversion must preserve:

- sign;
- normal and subnormal significands;
- exponent;
- signed-zero distinction in the source record, even though exact rational zero has no sign; and
- rejection of NaN and infinity where the input contract forbids them.

No decimal formatting or parsing may intervene.

### 2.7 Exact low-complexity geometric predicates

Using exact rationals, the test oracle must provide authoritative answers for bounded cases including:

- 2D orientation;
- 3D orientation or signed tetrahedral volume;
- point-plane side;
- point-in-triangle in a chosen exact projection;
- segment-plane relation;
- edge-face crossing category;
- edge-edge relation in coplanar projections;
- triangle-triangle proper intersection and coplanar overlap categories;
- exact parameter order along a source edge or carrier;
- exact polygon area and winding in a projection;
- exact shell ray-crossing or winding for bounded fixtures; and
- containment of exact values inside published bounded floating-point enclosures.

The oracle must expose exact categorical outcomes and rational witnesses. It must not imitate the production predicate dependency graph so closely that identical implementation errors become invisible.

### 2.8 Analytic known-answer solid library

The component must provide a library of hand-auditable and analytically generated solids, including:

- empty meshes;
- tetrahedra;
- axis-aligned and exactly transformed boxes;
- convex polytopes from integer halfspaces or vertices;
- polygonal prisms and extrusions;
- nested boxes or prisms representing cavities and islands;
- disconnected multi-solid operands;
- source subdivisions with the same boundary; and
- controlled contact and overlap configurations.

For each family, expected Boolean occupancy, component counts, shell roles, selected surfaces, or canonical output may be derived analytically or by bounded exact enumeration. The derivation method and domain limits must be recorded.

### 2.9 Valid manifold generator framework

Random and enumerative tests must generate valid closed oriented indexed manifolds rather than relying only on arbitrary soups.

Every generator must provide:

- a proof-by-construction or independent validation reason for manifoldness;
- exact or bounded source geometry metadata;
- stable generation provenance;
- deterministic parameter order;
- explicit shell and occupancy semantics;
- optional source polygonization and triangulation variants;
- exact source coordinate templates before conversion where practical; and
- a generator-specific shrink strategy.

Required generator families include:

- convex polytopes;
- boxes and orthogonal extrusions;
- star-shaped radial solids;
- polygonal extrusions with holes;
- nested shell trees;
- disconnected multi-solid operands;
- topology-preserving edge and face subdivisions;
- triangulation refinements and legal diagonal flips;
- controlled handles and tunnels;
- controlled thin features;
- vertex perturbations by selected ULP counts;
- controlled near-contact pairs; and
- controlled near-parallel edge-face and face-face pairs.

### 2.10 Source-presentation variant generation

For every suitable valid logical solid, the framework should generate equivalent presentations by:

- permuting vertex arrays;
- permuting facet arrays;
- rotating facet rings;
- reversing all rings with corrected shell orientation;
- permuting shells and disconnected components;
- replacing polygons with legal triangulations;
- subdividing source edges and facets without changing the solid;
- selecting alternative legal triangulations;
- duplicating coordinate values while preserving distinct topology; and
- changing internal allocation or traversal order through test adapters.

Expected metamorphic relations must be encoded explicitly. Equivalent presentations must not be assumed to require identical non-authoritative internal diagonals unless the frozen deterministic output policy says so.

### 2.11 Invalid input generator and mutator framework

The component must generate invalid public inputs with precisely classified violations, including:

- out-of-range indices;
- empty or undersized facet rings;
- consecutive duplicate indices;
- repeated directed edges;
- open boundaries;
- three-or-more-use edges;
- bow-tie vertices;
- inconsistent face orientation;
- disconnected vertex links;
- malformed shell nesting;
- ambiguous shell semantics;
- non-planar facets beyond declared precision;
- self-crossing facet rings;
- zero-area facets;
- non-finite coordinates;
- unsupported type or policy metadata;
- severe self-intersections; and
- index/count arithmetic overflow.

Each mutator must record the intended primary violation and whether secondary violations are permitted. Single-fault mutators used for typed-error tests should preserve all unrelated contracts where practical.

### 2.12 Intermediate-artifact mutation framework

Every component artifact must have deterministic mutators capable of producing producer-shaped corruptions that preserve superficial counts and, where useful, recompute non-authoritative summaries or digests.

Mutation families must include:

- wrong owner or stale IDs;
- duplicate or missing entities;
- incorrect reciprocal pairing;
- coordinate-equal identity welding;
- broken provenance lineage;
- omitted candidate or relation;
- changed crossing sign or multiplicity;
- duplicated or merged intersection event;
- false classification union or wrong winding;
- wrong selection, orientation, or occurrence multiplicity;
- unpaired output edge or broken face cycle;
- invalid triangulation diagonal or boundary loss;
- unsafe cleanup action or understated displacement;
- public assembly map corruption;
- verifier-report corruption; and
- concurrency merge-order corruption.

Every mutation must have a stable identifier and intended rejecting verifier. A mutation surviving its intended verifier is a qualification failure.

### 2.13 Required degeneracy matrix

The permanent suite must cover all operations and both operand roles where applicable for:

- disjoint solids;
- strict containment;
- equal operands;
- coincident shells with same orientation;
- coincident shells with opposite orientation;
- vertex-vertex contact;
- vertex-edge contact;
- vertex-face contact;
- edge-edge contact;
- edge-face contact;
- face-face contact;
- proper transverse intersection;
- tangency;
- coplanar partial overlap;
- coplanar complete overlap;
- intersection through original vertices;
- intersection through original edges;
- several event identities at one coordinate;
- distinct events with one rounded coordinate;
- multiple events with equal carrier parameter evidence;
- concave source facets;
- output faces with holes and nested contours;
- cavities, islands, and disconnected shells;
- radically different source triangulations;
- long thin facets and sliver triangles;
- signed zero and subnormal coordinates;
- adjacent floating-point values;
- extreme finite exponents and large translations;
- near-parallel constructions;
- features smaller than, equal to, and larger than tolerance;
- cleanup-induced topology changes; and
- repeated Boolean chains with accumulated precision.

The matrix must identify expected success, expected typed failure, or policy-dependent outcome for every required cell.

### 2.14 Operation and algebraic-law matrix

Where the solid and policy semantics permit, tests must cover:

- commutativity of union;
- commutativity of intersection;
- commutativity of symmetric difference;
- directed-difference operand remapping;
- idempotence;
- `A - A` is empty;
- identity with empty;
- annihilation with empty where applicable;
- absorption;
- containment identities;
- complement-related local truth-table relations where represented by operand remapping rather than an explicit complement mesh; and
- consistency of equal-operand symbolic ownership.

A failed algebraic law must preserve source bits, operation mapping, policy version, and both result or failure records for replay.

### 2.15 Component-local unit and known-answer tests

Every component must have tests for:

- valid minimum-size inputs;
- empty valid artifacts where supported;
- every documented enum and policy branch;
- every stable typed error subcode;
- every identity and owner domain it consumes;
- count and arithmetic boundaries;
- all required invariants;
- all prohibited behavior seams available to test adapters;
- serialization and digest versions; and
- transaction commit, rollback, cancellation, and resource failure.

Known-answer tests must assert full logical artifact content when that content is normative, not only final success status.

### 2.16 Property-test execution

Property tests must derive invariants from generated valid cases and inspect intermediate artifacts where possible.

Required properties include:

- all published artifacts pass their independent verifier;
- IDs are unique and owner-correct;
- precision bounds are monotonic;
- coordinate equality does not create identity;
- event sharing is consistent;
- paired halfedges are reciprocal;
- face cycles close;
- cleanup preserves manifoldness after each atomic action;
- public assembly is bijective;
- final verification accepts every ordinary success; and
- replay reproduces the same result or primary failure.

A property failure must retain the generator seed, fully materialized case, artifact stage, and failing assertion.

### 2.17 Metamorphic-test execution

The component must compare related cases under explicitly defined transformations.

Required transformations include:

- source permutations;
- legal source subdivision;
- alternative triangulation;
- axis permutations;
- sign flips with corrected orientation;
- exactly representable translations;
- power-of-two scaling with corresponding precision and tolerance scaling;
- operand exchange and operation remapping;
- thread-count changes;
- deterministic work-partition changes;
- hash-collision injection;
- allocator/free-list history changes; and
- repeated execution.

Comparison may require exact public bytes, canonical topology, occupied-side evidence, precision dominance, typed-failure equivalence, or another versioned equivalence relation. The required relation must be stated per transformation.

### 2.18 Deterministic fuzz campaign framework

Fuzz campaigns must be deterministic, replayable, and resource-bounded.

A campaign must:

- generate or mutate one case from the versioned seed stream;
- validate generator assumptions independently before using the case as a valid-input oracle;
- execute selected component and end-to-end paths;
- run intended independent verifiers;
- compare scalable paths with exact or exhaustive bounded oracles where in domain;
- retain the complete first canonical failure and optionally additional deterministic failures;
- serialize the case before shrinking;
- enforce per-case and campaign work limits; and
- continue or stop according to an explicit policy.

Fuzz input bytes are not trusted. Every decoder and test adapter must validate counts and lengths before allocation.

### 2.19 Topology-aware shrinking

The shrinker must reduce a failing case while preserving a versioned failure predicate.

Possible shrink operations include:

- remove disconnected components not participating in the witness;
- remove shells not participating in the witness;
- reduce facet count through inverse subdivision where valid;
- reduce polygon ring size;
- remove vertices while preserving manifoldness;
- reduce coordinate magnitude or exponent;
- reduce ULP perturbation distance;
- simplify tolerance and precision values;
- reduce operation or policy complexity only when the predicate permits;
- reduce thread count and schedule controls;
- remove unrelated mutations;
- shorten replay fields; and
- crop intermediate artifacts to the minimal witness closure.

The shrinker must never claim a valid-input reproducer if its own operations made the input invalid unless the preserved predicate is specifically an invalid-input bug.

Candidate selection must be deterministic. When several equally small candidates exist, a canonical logical-byte order must decide.

### 2.20 Geometry-aware numeric shrinking

For floating-point failures, the shrinker should attempt deterministic transformations that preserve exact bit-level meaning, including:

- move finite values toward zero by exponent/significand reduction;
- replace values with adjacent floats;
- normalize translations by subtracting exactly representable offsets;
- reduce power-of-two scale;
- replace general coordinates with exact integers or dyadic rationals when the failure persists;
- simplify signed-zero patterns;
- reduce tolerance and precision to boundary witnesses; and
- isolate one near-parallel or near-contact relation.

Every numeric shrink step must record old and new bit patterns and rerun the complete preserved predicate.

### 2.21 Artifact-aware shrinking

For intermediate-artifact mutations, the shrinker must retain the transitive evidence closure needed by the target component.

It may remove:

- unrelated source features;
- unrelated candidates, relations, events, groups, retained uses, faces, or cleanup actions;
- unrelated report records;
- unrelated worker tasks; and
- unused replay payload sections.

It must preserve owner-token validity, required predecessor references, and all fields necessary for the artifact to remain producer-shaped except for the intended corruption.

### 2.22 Failure-predicate stability

A shrink predicate must state whether it preserves:

- exact primary typed error and stable subcode;
- verifier rejection by a specified check;
- crash or undefined behavior;
- result nondeterminism;
- oracle disagreement;
- replay mismatch;
- performance structural regression; or
- a broader failure class.

The shrinker must not silently weaken the predicate. If exact witness identity changes while the same failure class persists, that change must be permitted explicitly by the shrink policy.

### 2.23 Permanent regression corpus

Every confirmed defect must produce a minimized permanent record containing:

- the earliest affected component;
- source or artifact replay data;
- exact failure predicate;
- original and minimized case digests;
- policy and provider versions;
- contract clauses exercised;
- fixed implementation version or commit metadata supplied by the build;
- expected result after the fix; and
- a stable human-readable explanation.

Regressions must never be silently deleted because they become slow or inconvenient. They may be migrated to a new schema only through a deterministic conversion with old/new digest records.

### 2.24 Corpus organization and deduplication

Corpus records must be organized by logical content and failure semantics rather than filename order alone.

Deduplication may use digests as an accelerator, but full canonical content and predicate metadata must be compared on collisions. Two cases with the same source mesh but different tolerance, operation, policy, expected failure, or witness must remain distinct records.

### 2.25 Replay execution

The test system must execute full and focused replay records.

Replay must:

- validate the record before use;
- reconstruct the frozen context;
- reproduce exact source bits and indices;
- select the recorded provider and policy versions where supported;
- restore resource and deterministic execution settings;
- rerun the requested component or entire pipeline;
- compare the result with the expected public-content digest or primary failure; and
- emit a deterministic compatibility result.

Unsupported obsolete required versions must fail as an explicit replay-compatibility result rather than being interpreted under current semantics.

### 2.26 Type matrix

Release qualification must cover every supported production type combination, initially at least:

- `float` with `std::uint32_t`;
- `float` with `std::uint64_t`;
- `double` with `std::uint32_t`; and
- `double` with `std::uint64_t`.

The matrix must include:

- minimum and maximum valid indices;
- sentinel boundaries;
- signed zero;
- subnormal values;
- adjacent normal values;
- large finite exponents;
- small finite exponents;
- mixed magnitudes; and
- count arithmetic near `std::size_t`, internal count, and configured resource limits through constrained test adapters where full allocation is impractical.

### 2.27 Platform and floating-environment qualification

For every supported compiler/platform configuration, the suite must verify the Component 01/03 arithmetic contract, including:

- binary representation assumptions;
- scalar sizes and bit encodings;
- nearest-even authoritative rounding;
- signed-zero behavior;
- subnormal behavior;
- floating-contraction policy;
- prescribed conversion points;
- rejection of unsupported fast-math or finite-only configurations; and
- canonical logical serialization independent of host endianness and native padding.

A platform that fails qualification must produce `unsupported_platform`; tests must not loosen expected bit patterns to make it pass.

### 2.28 Policy-version matrix

The suite must retain tests for every currently supported version of:

- solid semantics;
- symbolic contact policy;
- output policy;
- verification policy;
- determinism policy;
- execution policy;
- resource accounting schema;
- canonical labeling;
- logical serialization;
- digest domains;
- diagnostics; and
- replay.

When a version is retired, replay compatibility expectations and migration behavior must remain explicit.

### 2.29 Resource-boundary qualification

For every resource class exposed by Component 01 and every component-specific counter, test:

- limit minus one;
- exact limit;
- limit plus one;
- arithmetic overflow before the limit comparison;
- reservation rollback after later failure;
- concurrent reservation contention;
- persistent versus temporary accounting; and
- deterministic primary failure when several limits are crossed.

Tests may use constrained adapters to exercise large-count boundaries without allocating impossible physical storage.

### 2.30 Cancellation qualification

Every documented cancellation checkpoint in every component must be exercised.

Tests must cover cancellation:

- before a stage begins;
- after reservations but before allocation;
- during serial loops;
- during parallel task execution;
- during canonical merge;
- during serialization and digesting;
- during final verification; and
- immediately before transaction commit.

The test must confirm all workers join, all temporary reservations return, no partial artifact is visible, predecessor artifacts remain valid, and the primary result is `cancelled` with deterministic progress metadata.

### 2.31 Allocation and exception fault injection

The component must provide in-tree test allocators or resource adapters capable of deterministic failure at selected allocation/reservation ordinals.

Fault-injection tests must verify strong transaction-level exception safety for:

- containers;
- task-local outputs;
- maps and sorting buffers;
- exact-oracle limbs;
- public mesh storage;
- report and replay storage;
- serialization buffers; and
- worker creation or task-queue storage within the portable execution provider.

Allocation failure must map to the documented typed resource outcome. No test should depend on unspecified native allocator internals as the replay key.

### 2.32 Deterministic concurrency qualification

Using Component 17 controls, the suite must run selected cases under:

- one worker;
- two workers;
- configured maximum workers;
- reversed task submission;
- forced deterministic task delays;
- alternative canonical partitions;
- small task grains;
- large task grains;
- concurrent discovery of several failures;
- cancellation while workers hold private outputs; and
- repeated execution.

Artifacts, primary failures, reports, replay bytes, and deterministic counters must satisfy the frozen thread-count invariance contract.

### 2.33 Race and synchronization qualification

The portable source contract must be testable under compiler-provided race instrumentation when available, without introducing a source or runtime dependency into the engine.

Regardless of instrumentation availability, in-tree stress tests must attempt to expose:

- shared mutable artifact access;
- publication before worker join;
- reservation leaks;
- double commit or rollback;
- stale task-local IDs;
- non-atomic cancellation state misuse;
- incorrect condition-variable predicates;
- task-output lifetime errors; and
- nondeterministic merges.

A race finding is a release blocker.

### 2.34 Structural performance counters

Performance tests must rely primarily on deterministic structural counters rather than wall-clock time.

Required counters include, as applicable:

- source and output entities visited;
- broad-phase nodes;
- candidate pairs;
- exact bounded relation checks;
- event merges;
- classification edges;
- retained uses;
- triangulation ears or candidate diagonals;
- cleanup candidates and accepted actions;
- canonical-label refinement states;
- verifier triangle pairs;
- task count and merge records;
- temporary and persistent bytes; and
- abstract work units.

Counter definitions and versions must be part of the qualification manifest.

### 2.35 Structural complexity gates

The suite must include cases that fail qualification when implementation structure regresses, such as:

- disjoint large meshes causing all-pairs narrow-phase checks;
- repeated recomputation of one canonical relation;
- duplicate event construction;
- per-vertex ray tests replacing grouped classification;
- quadratic carrier ordering where a better provider is required by the selected performance contract;
- unbounded triangulation retries;
- cleanup rescans exceeding documented work bounds;
- canonicalization fallback to source order; or
- verifier omission of required candidates.

A provider may have documented worst-case limits and typed `resource_limit` outcomes. It must not hide structural regression by disabling verification or reducing the test size below the declared qualification threshold.

### 2.36 Wall-clock benchmark discipline

Wall-clock measurements may supplement structural counters.

When used, benchmark records must include:

- logical fixture digest;
- build mode and provider versions;
- platform descriptor;
- worker count;
- warm-up policy;
- sample count;
- deterministic structural counters;
- memory high-water marks from in-tree accounting; and
- an advisory threshold or comparison policy.

Wall-clock variation alone should not reject correctness qualification unless a release policy explicitly defines a stable supported environment and threshold. Structural gate failure remains authoritative.

### 2.37 Test-oracle independence

The test suite must avoid circular validation.

Examples:

- the broad phase is checked against exhaustive all-pairs enumeration;
- canonical ordering is checked against exhaustive bounded permutation minimization or an independently implemented encoding;
- bounded predicates are checked against exact rationals;
- producer edge pairing is checked by independent reconstruction;
- winding groups are checked against exact bounded ray or cell templates;
- cleanup reports are checked from primitive action certificates; and
- final reports are regenerated from primitive records.

A test that merely calls the production producer and production verifier sharing the same helper is insufficient for mutation qualification of that helper's behavior.

### 2.38 Mutation testing execution

The component must run every registered mutation operator against at least one valid fixture in its domain.

For each mutation, evidence must record:

- mutation identifier and version;
- target component and artifact field;
- source fixture digest;
- intended rejecting verifier and clause;
- actual primary finding;
- whether superficial counts/digests were repaired; and
- pass/fail result.

A mutation that survives all intended checks is a release qualification failure. A mutation that becomes malformed before reaching the intended verifier must be corrected or reclassified; it does not count as evidence for the intended clause.

### 2.39 Qualification coverage inventory

The evidence bundle must summarize coverage across:

- components;
- contract clauses;
- operation/operand-order cells;
- contact and degeneracy categories;
- scalar/index types;
- policy versions;
- success and typed-failure categories;
- resource boundaries;
- cancellation checkpoints;
- thread counts and schedule variants;
- exact-oracle relation categories;
- mutation operators;
- generator families;
- replay versions; and
- performance structural gates.

This is logical contract coverage. It must not depend on an external source-code coverage service.

### 2.40 Release evidence bundle

A release qualification run must produce a canonical evidence bundle containing:

- implementation/build identifier supplied by the build;
- specification and manifest versions;
- supported platform/type/policy matrix;
- exact ordered test list;
- result and duration metadata;
- structural counters;
- permanent corpus digest;
- generated campaign seed records;
- mutation survival summary;
- replay compatibility summary;
- contract traceability status;
- performance gate status;
- all failures and skipped-required items;
- canonical logical serialization; and
- aggregate qualification digest.

The bundle must be reproducible except for explicitly non-authoritative fields such as wall-clock timestamps. Non-authoritative fields must be excluded from the qualification digest or normalized.

### 2.41 Release qualification decision

Release qualification passes only when:

- every required test ran and passed;
- no required clause lacks executable evidence;
- no valid test expects `internal_invariant_error`;
- every exact-oracle comparison agrees;
- no broad-phase false negative is found;
- no intended mutation survives;
- every ordinary success re-ingests and passes Component 15;
- replay reproduces all permanent records;
- all required type, policy, resource, cancellation, and concurrency cells pass;
- structural performance gates pass; and
- the evidence bundle is complete and internally consistent.

Missing evidence, unexpected skips, stale corpus versions, or unverifiable digests are failures, not warnings.

### 2.42 Provider replacement qualification

When any algorithm or provider changes, the component must determine the affected qualification slice from explicit dependencies.

At minimum, provider replacement must rerun:

- all direct component tests;
- all independent verifier mutation tests for facts the provider produces;
- all end-to-end permanent regressions touching the provider;
- applicable metamorphic and exact-oracle comparisons;
- serialization/replay compatibility when artifact content or versions change;
- deterministic concurrency tests; and
- structural performance gates.

A provider change may intentionally alter versioned non-authoritative internal artifacts. It must not weaken public contracts or silently invalidate old replay records.

### 2.43 Test infrastructure self-verification

The test system itself must be tested.

Self-tests must include:

- assertion pass/fail behavior;
- stable test ordering;
- seed reproduction;
- corpus decode rejection;
- exact-integer and rational algebraic identities;
- oracle known answers;
- generator validity checks;
- mutator intended-fault checks;
- shrink predicate preservation;
- digest collision handling;
- traceability missing-test detection;
- required-skip rejection;
- evidence bundle canonicalization;
- resource rollback;
- cancellation; and
- deterministic parallel result merge.

A broken test harness cannot certify the engine.

### 2.44 Security and robustness of test inputs

Corpus and replay readers must treat input as untrusted.

They must:

- check sizes before allocation;
- cap recursion and nesting;
- validate all indices and references;
- reject integer overflow;
- reject unsupported required versions;
- avoid undefined shifts and signed overflow;
- avoid unbounded diagnostic formatting;
- reject malformed exact-number encodings; and
- return deterministic typed infrastructure errors.

The test system must include malformed corpus and replay fuzzing.

### 2.45 Transactionality of corpus updates

Adding or migrating permanent regression records must be transactional.

The update process must:

- write proposed records privately;
- validate replay and expected outcome;
- compute canonical digests;
- check deduplication against full content;
- update the manifest and traceability inventory;
- verify the complete corpus index; and
- atomically publish the new corpus state.

A failed update must not leave partial records or a manifest referring to absent files.

### 2.46 Deterministic test result arbitration

When several tests fail in one run, the harness must select a primary displayed failure by stable test identifier and canonical finding order, while retaining all failures subject to explicit storage limits.

Parallel completion order and wall-clock timing must not affect:

- the ordered failure list;
- the evidence bundle;
- the qualification digest; or
- the release decision.

### 2.47 Test data privacy and portability

Corpus records must contain only data required to reproduce engine behavior. They must not embed absolute local paths, pointer values, host usernames, process IDs, locale-specific text, or machine-specific temporary names in canonical content.

All paths in manifests must use a normalized portable logical form.

## 3. Output contract

On successful construction and execution of a qualification scope, the component must produce an immutable `qualification_evidence_bundle` containing or referencing:

- qualification scope and manifest version;
- implementation/build identifier;
- specification and contract-clause versions;
- provider, policy, serialization, digest, and replay versions;
- supported platform and type descriptors;
- exact ordered test and campaign inventory;
- result for every required test;
- canonical primary and complete retained failure lists;
- contract-to-test traceability results;
- operation/contact/degeneracy/type/policy coverage summaries;
- exact-oracle comparison results;
- mutation execution and survival results;
- permanent regression corpus digest;
- fuzz seed and iteration records;
- shrink results for newly discovered failures;
- replay compatibility results;
- resource, cancellation, and concurrency results;
- deterministic structural performance counters and gate results;
- test-infrastructure self-test results;
- non-authoritative timing metadata where retained;
- canonical logical serialization; and
- aggregate qualification digest.

The component must also maintain a versioned permanent regression corpus whose records are:

- canonical;
- replayable;
- independently validated;
- deduplicated by full logical content and predicate semantics;
- mapped to contract clauses;
- migration-aware; and
- never silently dropped.

A release-scope success artifact must guarantee:

- every required clause has current executable evidence;
- every required test ran and passed;
- all exact-oracle comparisons agree within their declared exact domain;
- no intended artifact mutation survived;
- all permanent regressions replay with their expected results;
- all supported type, policy, platform, resource, cancellation, and concurrency matrices pass;
- every ordinary success is accepted by Component 15;
- structural performance gates pass without weakening verification; and
- the complete evidence bundle is deterministic and internally consistent.

On failure, the component must produce a deterministic qualification failure identifying the earliest canonical failing test or missing evidence item, all retained failures within policy, manifest and provider versions, relevant clause identifiers, seed or corpus record, resource counters, and replay information.

## 4. Required invariants and prohibited behavior

Required invariants:

- every normative test and oracle is implemented in-tree with portable C++17 and no external dependency;
- every required contract clause maps to current executable evidence;
- exact test-oracle arithmetic is mathematically exact within explicit resource limits;
- generator validity is established independently before valid-input conclusions rely on it;
- all generated campaigns and shrink sequences are deterministic and replayable;
- every discovered confirmed defect becomes a permanent minimized regression;
- mutation testing proves independent verifiers reject producer-shaped corruption;
- digests accelerate but never replace full corpus-content comparison;
- release qualification cannot pass with missing, stale, or skipped-required evidence;
- structural performance gates cannot be disabled to obtain correctness success;
- test-only exact or exhaustive services do not become production dependencies;
- test and evidence publication is transactional; and
- canonical results do not depend on thread scheduling, locale, filesystem enumeration, or wall-clock time.

Prohibited behavior:

- comparing production output only with another external Boolean engine;
- requiring an external arbitrary-precision, geometry, testing, fuzzing, graph, serialization, database, or benchmarking library;
- using non-replayable random-device output as the campaign authority;
- treating arbitrary index soups as valid-manifold generation without independent validation;
- shrinking a valid-input failure into an invalid input without changing the predicate classification;
- deleting a regression because it is difficult or slow;
- counting a malformed mutation as evidence for a deeper verifier clause;
- allowing test order or parallel completion order to change evidence bytes;
- using digest equality as proof that two corpus records or artifacts are identical;
- accepting approximate arithmetic as the exact oracle;
- silently skipping out-of-domain oracle cases required by the release manifest;
- labeling a partial matrix as release qualification;
- allowing wall-clock noise to override deterministic structural correctness evidence; or
- embedding machine-specific state in canonical corpus or evidence records.

## 5. Test and validation specification

### 5.1 Test-harness self-tests

Verify:

- deterministic discovery and ordering;
- stable identifiers;
- assertion formatting;
- pass/fail/skip/infrastructure states;
- required-skip rejection;
- canonical result encoding;
- parallel result merge;
- resource limits;
- cancellation;
- allocation failure;
- malformed test registration; and
- aggregate process status.

### 5.2 Exact-integer tests

Cover:

- zero and sign normalization;
- carry and borrow across limbs;
- multiplication identities;
- quotient/remainder identities;
- exact divisibility;
- greatest common divisor;
- conversion boundaries;
- canonical serialization;
- limb-limit failure;
- algebraic property tests; and
- comparison with fixed-width arithmetic where no overflow occurs.

### 5.3 Exact-rational tests

Cover:

- sign and denominator normalization;
- reduction to lowest terms;
- zero canonicalization;
- arithmetic identities;
- comparison without overflow;
- division-by-zero rejection;
- exact binary-float conversion;
- determinant expressions;
- canonical serialization; and
- numerator/denominator resource limits.

### 5.4 Geometric-oracle known answers

Commit exact answers for:

- 2D and 3D orientation;
- point-plane side;
- segment-plane intersection;
- edge-face crossing classes;
- coplanar edge-edge relations;
- triangle-triangle relations;
- event parameter order;
- polygon area and winding;
- shell inclusion; and
- enclosure containment.

Include positive, negative, zero, and degenerate cases.

### 5.5 Generator validity tests

For every valid-manifold generator family:

- run independent topology validation;
- verify orientation and shell semantics;
- compare exact template metadata where present;
- permute generation traversal;
- test minimum and maximum configured sizes;
- verify deterministic seed reproduction; and
- verify generator-specific shrinking preserves validity.

### 5.6 Invalid-mutator precision tests

For every invalid-input mutator:

- confirm the intended violation is present;
- confirm unrelated contracts remain valid where promised;
- verify the expected typed failure;
- permute source presentation;
- test mutation composition rules; and
- ensure replay preserves the intended primary violation.

### 5.7 Intermediate-mutation tests

For every artifact mutator:

- verify the source artifact is valid;
- apply exactly the recorded corruption;
- repair superficial summaries/digests when specified;
- invoke the intended independent verifier;
- require the expected finding class; and
- store evidence in the mutation manifest.

### 5.8 Shrinker self-tests

Use synthetic predicates and real engine failures to test:

- deterministic candidate order;
- strict size reduction;
- failure-predicate preservation;
- valid-input preservation;
- numeric bit-pattern reduction;
- topology-aware deletion;
- artifact evidence closure;
- termination under work limits;
- canonical tie resolution; and
- replay of the minimized result.

### 5.9 Corpus codec tests

Cover:

- empty and non-empty records;
- exact scalar bits;
- large ring and count boundaries;
- duplicate coordinates;
- unknown optional fields;
- unknown required versions;
- truncation;
- malformed lengths;
- corrupted digests;
- simulated host endianness;
- locale changes;
- path normalization; and
- deterministic migration.

### 5.10 Traceability tests

Inject:

- an unmapped required clause;
- a missing test identifier;
- stale evidence version;
- a test mapped to the wrong component;
- a required test marked optional;
- a required skip;
- duplicate clause identifiers; and
- inconsistent manifest digests.

Release qualification must reject each case.

### 5.11 Evidence-bundle tests

Verify:

- canonical ordering;
- exclusion or normalization of non-authoritative timestamps;
- complete failure retention within limits;
- aggregate digest stability;
- digest collision fallback;
- platform/type/policy matrix completeness;
- mutation survival summary;
- replay compatibility summary;
- structural counter inclusion; and
- transactional publication.

### 5.12 Deterministic campaign tests

For fixed seed records, verify identical:

- case sequence;
- operation/policy selection;
- source bits and indices;
- mutation sequence;
- thread/schedule controls;
- shrink sequence;
- primary failure;
- retained failure set; and
- evidence bytes.

Test several independent compiler/platform configurations permitted by the qualified contract.

### 5.13 End-to-end qualification smoke set

Maintain a compact deterministic set that covers at least:

- empty identity cases;
- disjoint union/intersection/differences;
- containment;
- proper overlap;
- equal operands;
- all contact dimensions;
- one cavity/island case;
- one concave polygon case;
- one near-parallel case;
- one cleanup case;
- duplicate-coordinate topology separation;
- one repeated Boolean chain;
- all supported type combinations; and
- one thread-count/resource/cancellation matrix.

This smoke set is not release qualification but must detect major integration breakage quickly.

### 5.14 Full degeneracy-matrix tests

Execute every required degeneracy matrix cell under all supported operations and operand orders, with policy-specific expectations and exact or analytic oracles where available.

Missing cells are qualification failures.

### 5.15 Permanent regression replay tests

Replay every permanent record and verify:

- schema compatibility;
- exact input reconstruction;
- expected success or typed failure;
- expected public-content or finding digest;
- deterministic results across required thread counts; and
- current contract-clause mapping.

### 5.16 Mutation survival tests

Run the complete registered mutation set and require zero unintended survivors.

The evidence bundle must distinguish:

- rejected by intended verifier;
- rejected earlier for a valid reason;
- malformed mutation infrastructure;
- unsupported mutation domain; and
- survived.

Only intended-verifier rejection counts as full evidence for the target clause unless the manifest explicitly accepts an earlier independent gate.

### 5.17 Oracle differential tests

For bounded exact-domain cases, compare:

- Component 03 bounds with exact rational values;
- Component 06 candidates with exhaustive pairs;
- Component 07 relations with exact predicates;
- Component 08 event order with exact parameters;
- Component 09 winding with exact ray/cell oracles;
- Component 12 triangulation coverage with exact polygon area/intersection checks;
- Component 14 canonicalization with exhaustive bounded permutations; and
- Component 15 forbidden-intersection conclusions with exhaustive all-pairs checks.

### 5.18 Resource and cancellation matrix tests

Run every component through all registered resource classes and cancellation checkpoints. Confirm deterministic rollback, no partial publication, and stable typed outcomes.

### 5.19 Concurrency matrix tests

Run representative and adversarial cases with all required thread counts, forced schedules, task partitions, simultaneous failures, and cancellation points. Compare logical artifacts and errors byte-for-byte under the frozen determinism policy.

### 5.20 Structural performance regression tests

For each performance fixture, commit expected structural counter envelopes or asymptotic relations.

Test at least:

- large disjoint meshes;
- clustered intersecting meshes;
- many events on a source edge;
- many independent faces;
- high-valence but valid vertices;
- complex output polygons;
- cleanup-heavy meshes;
- symmetric canonicalization cases; and
- final-verification candidate loads.

A structural regression must be review-visible and cannot be hidden by changing fixture order or wall-clock thresholds.

### 5.21 Qualification-scope labeling tests

Attempt to publish component, integration, or continuous evidence as release evidence. The manifest validator must reject the mismatch.

### 5.22 Provider replacement tests

Use test doubles or alternate in-tree providers where available to verify:

- artifact version changes trigger required qualification slices;
- unchanged public contracts retain expected results;
- unsupported replay versions fail explicitly;
- deterministic outputs remain stable when required; and
- performance counters use the correct provider schema.

### 5.23 Malformed corpus and replay fuzzing

Fuzz canonical decoders with arbitrary bytes and structured corruptions. Require:

- no undefined behavior;
- no unchecked allocation;
- deterministic typed infrastructure failure;
- bounded diagnostics;
- no path traversal or machine-specific path use; and
- no accidental execution of partially decoded records.

### 5.24 Release dry-run tests

Execute the release manifest against intentionally altered evidence to confirm release qualification fails for:

- one failed test;
- one missing test;
- one required skip;
- one stale clause version;
- one mutation survivor;
- one replay mismatch;
- one exact-oracle disagreement;
- one structural performance regression;
- one unsupported platform cell; and
- one corrupted qualification digest.

### 5.25 Definition of done

Component 16 is complete only when:

- the in-tree portable C++17 harness, oracles, generators, mutators, shrinkers, replay tools, corpus tools, and evidence tools are operational without external dependencies;
- every normative clause for Components 01 through 17 maps to executable evidence or an explicit justified non-executable record;
- exact integer/rational and geometric oracles pass their self-tests;
- valid generators are independently validated;
- invalid and intermediate mutators produce their intended faults;
- deterministic fuzzing and shrinking reproduce byte-for-byte from seed records;
- every confirmed defect becomes a permanent minimized regression;
- all required operation, degeneracy, type, policy, resource, cancellation, concurrency, and replay matrices are represented;
- mutation survival is zero for required operators;
- release evidence is canonical, complete, and transactional;
- structural performance gates are enforced without weakening correctness; and
- a release cannot be reported qualified while required evidence is missing, stale, skipped, or failing.
