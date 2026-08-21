# Component 17: Performance and Deterministic Concurrency

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete worker-pool implementation, task container, queue discipline, grain-size provider, partition heuristic, in-tree spatial and sorting providers, memory arena layout, deterministic reduction tree, counter storage, benchmark fixture set, and advisory threshold policy may change. Bounded execution, immutable task inputs, private task outputs, deterministic canonical merge, stable failure arbitration, transactional cancellation, floating-environment conformance in every worker, resource accounting, structural performance evidence, and the input/output contracts in this document are normative.

## 0. Purpose

This component provides the execution services and performance discipline used by the Boolean pipeline after correctness contracts are established. It permits expensive independent work to run concurrently without allowing thread scheduling, task completion order, hash iteration, allocation timing, or reduction order to alter topology, geometry decisions, errors, reports, replay data, or canonical output.

Its purposes are to:

- provide a bounded in-tree C++17 worker service with no detached activity;
- define immutable task descriptors and exclusive ownership of task-local mutable state;
- partition broad-phase, relation, event-preparation, classification-support, per-face, cleanup-candidate, canonicalization-support, and verification work into deterministic units;
- merge task outputs by complete canonical keys rather than completion order;
- preserve an executable serial semantic reference for every parallelized stage;
- establish the required floating-point environment in every worker performing authoritative arithmetic;
- coordinate cancellation, resource reservations, exception capture, rollback, and worker joining;
- select one deterministic primary failure when several tasks fail;
- provide deterministic integer, maximum-with-witness, set, sequence, and report reductions;
- prevent nested unbounded parallelism and memory oversubscription;
- instrument structural work, memory, candidate counts, task counts, and merge behavior;
- define performance gates that detect accidental all-pairs or repeated-computation regressions; and
- permit provider optimization only when every component contract and Component 15 verification result remains unchanged.

This component is not authorized to weaken broad-phase conservatism, bounded-relation accuracy, symbolic semantics, event identity, topology construction, cleanup eligibility, canonical labeling, or final verification. Performance difficulty may cause a typed `resource_limit`, `cancelled`, `unsupported_platform`, or other documented failure; it may not cause a semantically weaker success.

The principal outputs are:

- immutable deterministic execution services referenced by the Boolean context;
- per-stage `execution_plan` artifacts and canonical task descriptors;
- canonical task-output merge artifacts;
- deterministic execution, resource, and structural performance reports; and
- qualification evidence consumed by Component 16.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable Boolean context from Component 01;
- frozen execution, determinism, resource, cancellation, diagnostic, and verification policies;
- supported worker-count limits and caller-requested concurrency settings;
- platform and floating-point-environment qualification metadata;
- immutable stage inputs from Components 02 through 15;
- component-specific task-adapter contracts defining canonical work keys, immutable inputs, private output schemas, merge semantics, and serial reference behavior;
- deterministic comparators and identity-publication services;
- resource reservation and transaction services;
- structural counter definitions and versions;
- replay and diagnostic services; and
- qualification controls from Component 16 for forced schedules, partitions, delays, collisions, limits, and fault injection.

The component must not retain mutable references to caller meshes or options, expose task-queue internals as semantic state, or depend on process-global mutable configuration.

### 1.2 Serial semantic reference contract

Every parallelized operation must have a documented serial semantic reference.

The serial reference defines:

- the complete logical input domain;
- canonical work-item identity;
- required output multiset or sequence;
- canonical output ordering;
- duplicate-key and conflict semantics;
- failure semantics and deterministic primary-error selection;
- resource and cancellation semantics;
- precision and reduction semantics;
- report and counter interpretation; and
- the transaction commit boundary.

A parallel implementation conforms only when, for every supported input and frozen policy, it produces the same authoritative artifact, typed primary failure, canonical diagnostics, report content, replay data, and digest as the serial semantic reference.

The serial reference need not be the production one-worker implementation, but it must be executable in normative tests for bounded cases and must not itself depend on worker scheduling, address order, or unordered-container iteration.

### 1.3 Task-adapter contract

A component may submit parallel work only through an adapter that defines:

- a stable stage and task-kind identifier;
- a complete canonical work key;
- immutable input ranges or entity references;
- exclusive task ownership of private mutable output;
- resource reservations required before execution;
- deterministic cancellation checkpoints;
- exception-to-typed-failure mapping;
- canonical output-record keys;
- merge preconditions and duplicate-key semantics;
- structural work counters; and
- serial-equivalence expectations.

Task adapters must not expose shared mutable component artifacts to workers unless ownership is exact, disjoint, and statically or dynamically verified. A claim that writes are “unlikely to overlap” is not an ownership contract.

### 1.4 Canonical work-key contract

Every published task or task-output record must have a total canonical key independent of:

- worker identity;
- queue position;
- submission timing;
- completion timing;
- pointer values;
- allocator addresses;
- hash-bucket order; and
- task-local temporary IDs.

Canonical keys may contain:

- stage and task kind;
- canonical entity ID or closed entity range;
- canonical candidate or relation key;
- source operand and feature tuple;
- output face, carrier, component, cleanup region, or verification partition key;
- policy version; and
- deterministic subdivision rank derived solely from logical input.

A task key must not use coordinate sorting as a substitute for topological identity. When coordinates are part of a complete key, exact policy-normalized bit patterns and explicit occurrence identities must remain distinguishable.

### 1.5 Execution-policy contract

The frozen execution policy must define, at minimum:

- requested worker count or automatic bounded-selection rule;
- maximum worker count;
- whether deterministic concurrency is enabled;
- permitted parallel stage kinds;
- task-grain policy version;
- private-output byte limits;
- global in-flight work limits;
- queue and backpressure limits;
- nested-parallelism policy;
- floating-environment setup policy;
- cancellation polling requirements;
- advisory performance targets; and
- required deterministic report fields.

Unknown or incompatible execution-policy versions must be rejected before worker activity begins. A worker-count request larger than the supported or configured maximum must be rejected or deterministically clamped only when the public policy explicitly permits and reports that normalization.

### 1.6 Resource input requirements

The component must receive overflow-safe limits for:

- worker objects and thread creation;
- queued tasks;
- in-flight tasks;
- task descriptors;
- task-local temporary bytes;
- task-local persistent outputs;
- merge buffers;
- canonical-sort records;
- reduction records;
- diagnostics and failures;
- structural counters;
- total temporary bytes;
- total persistent bytes; and
- abstract work units.

Reservations must distinguish bytes released after task completion from bytes retained until stage commit. Count-to-byte multiplication, prefix sums, range endpoints, and aggregate reservations must be checked before mutation or allocation.

### 1.7 Floating-point environment input requirements

Every worker that performs topology-affecting arithmetic must receive the qualified Component 01/03 floating-point contract.

The execution service must be able to establish and verify, per worker as required by the platform:

- nearest-even rounding;
- signed-zero behavior required by policy;
- subnormal behavior;
- floating-contraction policy;
- prohibited fast-math and reassociation assumptions as a build qualification;
- prescribed conversion and rounding points to `T`; and
- deterministic exception or status handling.

A worker whose environment cannot be established must emit `unsupported_platform` or a deterministic setup failure before producing authoritative output. The main thread’s floating environment must not be assumed to propagate correctly without qualification.

### 1.8 Lifetime and ownership preconditions

All task input artifacts must remain immutable and alive until every task that references them has joined.

Task-output ownership must be one of:

- entirely task-local until canonical merge;
- a preassigned disjoint range with no cross-task writes;
- an immutable append record published to a private deterministic merge structure after task completion; or
- another ownership model with a proof equivalent to no data race and no schedule-dependent semantic state.

Reference counting, shared ownership, or arenas may be used in-tree, but ownership semantics must be deterministic and must not delay worker activity beyond transaction rollback. No task may retain a reference to transaction-private state after its stage joins.

### 1.9 Component-adapter minimum information

For each parallel-capable stage, the adapter must additionally specify:

- whether the logical result is a sequence, set, multiset, map, graph, reduction, or proposed mutation list;
- whether output keys are unique by contract;
- how duplicate records are proven equivalent or diagnosed as contradictions;
- whether task failure may suppress later work without affecting deterministic failure arbitration;
- which counters are authoritative and how they combine;
- whether task partition changes are permitted without changing replay compatibility;
- which portions of the stage remain serial; and
- the independent verifier that checks the merged artifact.

### 1.10 Portability contract

The execution service may use only portable C++17 language and standard-library facilities in production and normative tests. It must not require OpenMP, TBB, platform thread pools, compiler task extensions, GPU APIs, external concurrent containers, external allocators, or external profilers.

Platform-specific optional instrumentation may be used outside the normative implementation only when absence of that instrumentation does not reduce the required in-tree qualification evidence.

## 2. Required behavior

### 2.1 Bounded worker service

The component must provide a bounded worker service implemented with portable C++17 facilities.

The service must:

- create no more workers than the frozen maximum;
- use joinable worker lifetimes;
- never detach a thread;
- keep all workers owned by one execution service or stage transaction;
- provide bounded task submission with backpressure;
- support clean shutdown and reuse according to policy;
- capture task exceptions and convert them to deterministic failure records;
- stop accepting commit-producing work after cancellation or terminal failure policy; and
- join all worker activity before transaction rollback, destruction, or ordinary return.

A zero-worker implementation may execute work serially on the invoking thread only when the policy defines that behavior. It must preserve the same task and merge semantics.

### 2.2 Worker creation and fallback

Worker creation must be transactional.

Before creating workers, the service must reserve:

- worker-control storage;
- thread objects;
- queue state;
- per-worker floating-environment and diagnostic state; and
- minimum task-output accounting required by the policy.

If creation of the requested bounded pool cannot complete, the service may:

- fail with `resource_limit`; or
- use a documented deterministic smaller worker count when the frozen policy permits fallback.

Fallback must occur before stage work begins, must be reported, and must not depend on which thread creation happened to fail. Partially created workers must be stopped and joined before retry or return.

### 2.3 Task submission and backpressure

Task submission must not permit unbounded descriptor or output growth.

The service must:

- reserve descriptor and expected private-output capacity before admission;
- block, cooperatively execute eligible work, or return a typed resource outcome when the bounded queue is full according to policy;
- use condition-variable predicates robust to spurious wakeups;
- recheck cancellation and terminal failure while waiting;
- avoid holding component locks while waiting for queue capacity; and
- preserve progress with one worker and the smallest supported queue.

Backpressure behavior must not change task identity, output order, or the primary failure.

### 2.4 No detached or orphaned work

A task may not outlive:

- the immutable artifacts it reads;
- the transaction that owns its reservations;
- the execution service that owns its worker;
- the diagnostic collector receiving its records; or
- the stage whose commit depends on it.

Every exit path—success, typed failure, exception, cancellation, resource exhaustion, or failed commit—must join or otherwise synchronously complete all owned tasks before releasing referenced state.

### 2.5 Deterministic partition construction

Partitioning logical work into tasks must be deterministic for a fixed policy version.

Partition construction must use:

- canonical entity or candidate ranges;
- overflow-checked size calculations;
- a versioned grain rule;
- deterministic handling of remainders;
- stable subdivision ranks; and
- an explicit maximum task count.

Worker count may influence physical grouping only if the determinism contract states that task-plan bytes are non-authoritative and canonical merge produces the same artifact and reports. If execution-plan identity is included in replay or reports, the policy must define whether plans are worker-count invariant or merely semantically equivalent.

Adaptive partitioning based on wall-clock timing, worker speed, queue race, or completion order is prohibited for authoritative task identity.

### 2.6 Immutable task inputs

A task descriptor must contain immutable values or handles to immutable artifacts. A task must not read:

- a mutable vector being extended by another task;
- a hash table being rehashed concurrently;
- a cleanup topology being mutated concurrently without exact region ownership;
- a global “next ID” counter whose result escapes publication;
- mutable floating-policy state; or
- non-atomic cancellation state.

Lazy caches are permitted only when their publication is race-free, their contents are semantically unique, and the cache state cannot affect output, errors, counters required by the contract, or replay.

### 2.7 Private task outputs

Each task must write to private output storage or an exactly assigned disjoint range.

Private output must include, as applicable:

- output records;
- task-local temporary identities;
- findings and failures;
- precision contributors;
- maximum-with-witness candidates;
- resource usage;
- structural counters; and
- diagnostic excerpts.

Task-local identities must be remapped during canonical merge. They must never become published IDs merely because their task completed first.

### 2.8 Canonical merge

Task outputs must be merged by a deterministic algorithm over complete canonical keys.

Canonical merge must:

- wait for all logically required tasks unless a proven deterministic early-stop rule applies;
- collect private outputs without using completion order as semantic order;
- validate owner tokens, stage versions, key ranges, and declared counts;
- sort or otherwise order records by a complete total key;
- detect missing, duplicate, and conflicting records;
- remap task-local identities in canonical order;
- combine counters and precision records with defined reductions;
- choose the primary failure through Component 01 arbitration;
- construct one proposed immutable stage artifact; and
- invoke the stage verifier before transaction commit.

A partial comparator whose equal range still depends on input order is prohibited unless a complete deterministic tie resolution follows before publication.

### 2.9 Duplicate-key semantics

Every merge schema must classify duplicate keys as one of:

- impossible and therefore an `internal_invariant_error`;
- equivalent records that must compare equal in all normative fields and coalesce by explicit rule;
- a multiset occurrence requiring an explicit occurrence rank; or
- competing proposals resolved by a documented deterministic selection rule that preserves the serial reference.

Hash equality or coordinate equality is not sufficient proof of record equality.

### 2.10 Deterministic sorting

Any sort affecting authoritative output must use:

- a strict weak ordering over all relevant fields;
- exact integer and policy-normalized bit comparisons where applicable;
- explicit tie fields sufficient to obtain the required total order; and
- a provider whose result does not depend on address, locale, or unstable insertion state.

A stable sort may preserve a previously canonical sequence, but stability must not preserve worker completion order or hash extraction order. The comparator and key schema must be versioned when they affect artifact bytes or replay.

### 2.11 Deterministic reductions

The component must provide deterministic reductions for at least:

- checked integer sums;
- logical AND/OR over immutable flags;
- minimum and maximum canonical keys;
- maximum numerical bound with canonical witness;
- set union by canonical identity;
- ordered concatenation of canonical blocks;
- resource high-water summaries;
- failure and finding collections; and
- optional non-authoritative floating statistics.

Authoritative floating-point quantities must not be accumulated in worker-completion order. They must use a specified serial order, fixed reduction tree, exact/count representation, bounded arithmetic operation, or maximum-with-witness rule appropriate to the quantity.

### 2.12 Maximum-with-witness reductions

Precision, displacement, conditioning, and feature-size maxima must retain a deterministic witness.

The reduction key must define:

1. numerical comparison under the quantity’s bounded representation;
2. treatment of equal nominal values and equal bounds;
3. canonical entity or ledger identity tie-break;
4. policy/version tie-break where required; and
5. rejection of NaN, negative, or malformed records.

The same maximum value and witness must be selected at every worker count.

### 2.13 Failure collection and arbitration

Workers must record failures privately. The execution service must not return the first failure observed in time.

It must:

- normalize each finding to the Component 01 failure schema;
- retain the canonical feature and numerical witness;
- collect all findings required to determine the minimum arbitration key;
- select the primary failure after canonical merge or by a proven monotonic early-stop rule;
- sort retained secondary findings canonically; and
- report explicit truncation if diagnostic limits discard secondary findings.

Cancellation racing with another failure must follow a frozen arbitration rule. The result must not depend on which thread set a flag first.

### 2.14 Early-stop requirements

Early suppression of pending work is permitted only when the provider proves that no suppressed task can produce:

- a higher-priority failure;
- a record required for the successful artifact;
- a counter required by the deterministic report;
- a resource fact required for reconciliation; or
- evidence required by a mandatory verifier.

Otherwise, pending tasks may observe cancellation or terminal state and stop only at a stage policy that defines the resulting primary outcome independently of schedule. Qualification must compare early-stop behavior with complete serial arbitration.

### 2.15 Worker floating-point environment

At worker start and before authoritative arithmetic after any operation that could alter it, the service must establish or verify the frozen floating environment.

The service must:

- reject unsupported environment state;
- prevent task code from silently changing authoritative rounding mode;
- restore required state before reusing a worker;
- record the qualification version in execution evidence; and
- ensure non-authoritative diagnostic formatting does not alter later authoritative arithmetic.

Worker environment verification must be covered by bit-pattern tests, including tasks deliberately submitted after test-only environment disturbance.

### 2.16 Parallel broad-phase construction and traversal

Component 06 may parallelize independent spatial-index construction and node-pair traversal when:

- leaf inputs are in canonical order;
- every task receives conservative immutable bounds;
- partition boundaries cannot omit a node or pair;
- task-local candidate outputs retain full canonical candidate keys;
- adjacency exclusions remain exact topology rules;
- all candidates merge and deduplicate according to the Component 06 contract; and
- exhaustive bounded qualification proves no false negative.

A parallel provider must not use a smaller inflation, different bound arithmetic, or opportunistic pruning than the serial semantic reference.

### 2.17 Parallel relation evaluation

Component 07 relation records are natural independent tasks once canonical candidates exist.

Parallel evaluation must guarantee:

- one logical evaluation per canonical relation key;
- immutable candidate and source-feature access;
- identical bounded arithmetic and symbolic policy in every worker;
- private relation, failure, and precision records;
- canonical merge by relation key;
- detection of duplicate inconsistent evaluations; and
- no publication of task-local relation IDs.

Scheduling must not choose which equivalent expression computes a relation. The canonical relation dependency graph remains authoritative.

### 2.18 Parallel event-registry preparation

Component 08 may derive task-local event seeds and consumer references concurrently, but final event identity, shared construction, carrier ordering, and occurrence separation require canonical merge.

The execution adapter must ensure:

- equal event lineage keys enter one canonical merge group;
- one authoritative construction is selected or verified equal according to Component 08;
- distinct lineage keys remain distinct even at equal coordinates;
- carrier records are globally ordered by complete keys;
- task-local event IDs are remapped canonically; and
- no worker mutates a shared interning table in a way that assigns identity from arrival order.

### 2.19 Parallel classification support

Component 09 may parallelize construction of primitive zero-delta and signed-delta records, independent seed queries, and component-local checks.

Final connectivity groups, quotient-graph identity, winding propagation, and cycle-consistency findings must be equivalent to the serial semantic reference. Concurrent union operations are permitted only if the final published partition and canonical group labels do not depend on union race order and are independently canonicalized.

### 2.20 Parallel per-face and per-component work

Independent source facets, output polygons, disconnected components, or verification partitions may be processed concurrently when all cross-boundary topology is already immutable and explicit.

Examples include:

- source-facet triangulation preparation;
- per-output-face triangulation;
- component signatures;
- local cleanup-candidate discovery;
- provenance block construction;
- per-component canonical-label descriptors; and
- independent verification checks.

Global entity IDs, offsets, component order, report order, and failure selection must be assigned only through checked canonical prefix sums and merge.

### 2.21 Cleanup concurrency restrictions

Component 13 topology mutation is correctness-sensitive and is serial by default.

A conforming provider may parallelize:

- candidate discovery against an immutable generation;
- local eligibility prechecks;
- displacement-bound proposals;
- independent no-new-intersection searches; and
- certificate preparation.

Before mutation, proposals must be merged by canonical action key and revalidated against the current topology and budget state. Atomic actions must be applied in the serial semantic order unless a provider proves exact non-interference including:

- disjoint mutable topology neighborhoods;
- disjoint precision and displacement ledger effects;
- no shared feature-removal or topology-change budget;
- no interaction through no-new-intersection checks;
- deterministic published action order; and
- equivalence under Component 13’s independent verifier.

“Different edges” alone is not proof of safe concurrent mutation.

### 2.22 Canonicalization concurrency restrictions

Component 14 may compute descriptors, component encodings, refinement candidates, and branch encodings concurrently. It must not select:

- the first completed automorphism branch;
- the first discovered component representative;
- the smallest transient task ID; or
- a worker-dependent labeling.

All branch results required by the canonical-minimum provider must be compared in canonical logical order. Final public index assignment and facet serialization must use the selected canonical labeling after all required comparisons complete.

### 2.23 Final-verification concurrency

Component 15 may parallelize independent checks, including:

- public edge-use extraction by facet ranges;
- triangle nondegeneracy checks;
- independent hierarchy construction;
- non-adjacent pair classification;
- event-lineage audits;
- cleanup-certificate checks;
- precision-ledger aggregation; and
- report block generation.

The verifier must merge findings canonically and must not weaken check coverage at higher worker counts. A parallel omission in the independent forbidden-intersection search is a correctness failure, not a performance variation.

### 2.24 Serial topology-assembly preference

Stages that create or mutate globally coupled topology should remain serial unless a parallel provider has a complete deterministic ownership and equivalence proof.

Serial execution is preferred for:

- final occurrence partition publication;
- paired output-edge assignment across shared carriers;
- face-cycle publication;
- cleanup mutation application;
- final canonical public-index assignment;
- final report status transition; and
- transaction commit.

This preference is not a prohibition on future parallel providers. It prevents concurrency from being introduced before ownership and canonical merge contracts are complete.

### 2.25 Nested parallelism

Nested parallelism must use one globally bounded execution budget for the Boolean invocation.

The policy must define whether a worker invoking a parallel-capable operation:

- executes nested work serially;
- helps the same bounded queue while waiting;
- submits only when reserved global capacity exists; or
- uses another deadlock-free bounded strategy.

A task must not create an unbounded child pool. Waiting for child tasks while occupying all workers and preventing those children from running is prohibited.

### 2.26 Resource reservation and admission

Every task must reserve the resources required for its descriptor and conservative private-output allowance before admission.

Providers may use:

- exact pre-counting;
- conservative per-task maxima;
- bounded chunk growth with additional reservations; or
- two-pass count/fill construction.

A task that needs more than its reservation must acquire additional capacity before allocation. Failure must produce a typed resource record and no partial published output.

### 2.27 Backpressure and memory oversubscription

The execution service must bound total memory from queued and running tasks.

It must not admit every broad-phase, face-triangulation, or verification task with worst-case output simultaneously when that exceeds the resource policy. Permitted strategies include:

- bounded queues;
- fixed-size task waves;
- streaming canonical merge windows;
- two-pass output allocation;
- deterministic range ownership; and
- cooperative execution by submitters.

Wave or window boundaries must derive from canonical ranges and policy, not current memory timing.

### 2.28 Two-pass construction

For stages with predictable records, a two-pass design may:

1. count task-local outputs and validate local work;
2. merge checked counts by canonical task order;
3. compute overflow-safe prefix sums;
4. allocate exact global storage;
5. rerun or materialize records into assigned disjoint ranges; and
6. verify every assigned range is filled exactly.

The second pass must use the same immutable inputs and semantic provider version. Count/fill disagreement is an invariant failure. Cancellation between passes publishes nothing.

### 2.29 Checked prefix sums and offsets

All global offsets must be computed in canonical task order with checked arithmetic.

The component must verify:

- every local count is valid;
- cumulative counts fit internal types, `std::size_t`, public `I` where relevant, and resource limits;
- assigned ranges are disjoint and complete;
- no sentinel collision occurs; and
- final count equals the sum of local counts.

Parallel scans are permitted only when their result is identical to the checked serial prefix definition and overflow witnesses are deterministic.

### 2.30 Container and hashing discipline

Concurrent or serial hash tables may be used as private implementation details, but:

- hash iteration order is never publication order;
- collisions are resolved by full-key equality;
- hash values do not define identity;
- randomized seeds are not authoritative unless fully specified and replayed, and deterministic fixed in-tree hashing is preferred;
- rehash timing must not affect output or counters classified as deterministic; and
- records are extracted and canonically sorted before publication.

No external concurrent container may be used.

### 2.31 Cancellation

Cancellation must be cooperative and checked at deterministic safe points in:

- partition construction;
- submission and queue waits;
- task loops;
- bounded spatial traversal;
- relation batches;
- sorting and merge;
- reduction;
- stage verification; and
- shutdown.

After cancellation becomes the selected terminal outcome:

- no new commit-producing task may be admitted;
- workers stop at safe points;
- queued work is discarded or drained according to deterministic policy;
- all workers join;
- private outputs are destroyed;
- reservations return;
- predecessor artifacts remain valid; and
- no partial stage artifact or ordinary success is published.

### 2.32 Terminal failure and work suppression

A task failure may set a shared atomic indication that failure exists, but that flag alone must not select the public error.

Other tasks may suppress expensive optional work only under the early-stop contract. Required cleanup, reservation reconciliation, failure evidence, and higher-priority arbitration work must still complete. The shared flag must not make two worker counts produce different primary failures.

### 2.33 Exception capture

Every worker entry point must catch all exceptions permitted by the build and map them to the frozen typed-failure contract.

The service must distinguish:

- expected allocation or resource failure;
- task-adapter documented typed exceptions, if any;
- cancellation signaling implemented through exceptions, if explicitly supported; and
- unexpected implementation exceptions.

Unexpected exceptions become `internal_invariant_error` with deterministic stage/task witnesses after all workers join. No exception may cause `std::terminate` because a joinable thread was destroyed.

### 2.34 Transaction integration

The execution service must be subordinate to the Component 01 transaction.

A stage transaction must own or reference:

- its execution plan;
- task descriptors;
- task-private output storage;
- reservations;
- worker activity handles;
- finding buffers;
- merge state; and
- proposed artifact.

Commit is permitted only after task completion, canonical merge, resource reconciliation, and stage verification. Rollback must be safe from any phase.

### 2.35 Execution-plan artifact

Before a parallel stage begins, the component must produce an immutable or transaction-private `execution_plan` containing:

- stage and adapter version;
- logical input range and digest;
- worker count;
- canonical partitions and task keys;
- grain policy;
- queue and in-flight limits;
- expected reservation bounds;
- merge and reduction versions;
- cancellation checkpoint version;
- structural counter schema; and
- plan digest where required.

The plan must be verifiable before submission. A plan referencing overlapping exclusive ranges or omitting logical work is invalid.

### 2.36 Instrumentation

Every parallelized stage must emit deterministic structural instrumentation sufficient to explain performance and prove work coverage.

Required counters include, as applicable:

- logical work items;
- task count;
- queued and peak in-flight tasks;
- worker count actually used;
- private-output records and bytes;
- merge records;
- duplicate/equivalent/conflicting keys;
- sort comparisons or abstract sort work;
- broad-phase nodes and candidate pairs;
- bounded relation checks;
- event seeds and merge groups;
- classification edges;
- triangulation work;
- cleanup candidates and actions;
- canonicalization states;
- verification pairs and probes;
- cancellation polls;
- temporary and persistent high-water bytes; and
- abstract work units.

Wall-clock timings may be included as non-authoritative metadata but must not affect canonical artifacts, digests, or typed outcomes.

### 2.37 Counter determinism

Counters used for qualification or canonical reports must define how they compare across worker counts.

A counter is one of:

- **semantic invariant**, required to be identical, such as logical relation count;
- **plan invariant**, identical for one partition-policy version and logical input;
- **bounded execution statistic**, permitted to vary within documented deterministic parameters such as worker-local chunk acquisition count; or
- **non-authoritative timing statistic**.

The execution report must label every counter category. A provider must not silently reclassify a correctness-relevant counter as non-authoritative.

### 2.38 Work budgets

Abstract work counters must protect against pathological computation independently of wall-clock time.

Work units should account for expensive operations such as:

- spatial node-pair visits;
- exact candidate emissions;
- bounded predicate evaluations;
- carrier-order comparisons;
- triangulation candidate checks;
- cleanup eligibility and intersection checks;
- canonicalization refinement and branch states;
- verifier pair classifications; and
- canonical byte comparisons.

Crossing a hard work budget returns `resource_limit` transactionally. It must not trigger an unchecked heuristic fallback.

### 2.39 Structural asymptotic gates

Provider qualification must define structural gates that reject obvious regressions, including:

- large disjoint operands causing all edge-triangle or triangle-triangle pairs to reach narrow phase;
- the same canonical relation evaluated several times;
- duplicate construction of one event;
- classification reverting to one geometric ray test per vertex when grouping should apply;
- output edges paired by global quadratic search despite explicit carrier grouping;
- triangulation retry loops without bounded progress;
- cleanup rescanning the complete mesh after every local action where the selected provider promises local invalidation;
- canonicalization falling back to source order; and
- final verification omitting candidates to improve timing.

The exact envelopes may depend on provider version and fixture family, but they must be deterministic and reviewable.

### 2.40 Small-work serial policy

For small logical ranges, serial execution may be faster and safer.

The threshold rule must be versioned and based on deterministic logical size and policy, not measured timing. Crossing the threshold may change execution counters classified as plan statistics, but must not change authoritative artifacts, failures, reports required to be thread-count invariant, or replay semantics.

### 2.41 Load balancing

Load balancing may use deterministic static partitions, bounded work queues, work helping, or another in-tree strategy.

Dynamic work stealing is permitted only when:

- stolen task identity and inputs are already fixed;
- task output remains private;
- completion order does not affect merge;
- resource ownership transfers safely;
- cancellation remains bounded; and
- replay does not require reproducing the physical steal sequence.

Runtime-generated subdivision based on observed cost is permitted only if subdivision identity is deterministic from immutable progress state and cannot change the logical result or failure arbitration.

### 2.42 Replay execution settings

Replay must record enough execution information to reproduce semantic behavior and investigate concurrency defects, including:

- execution-provider and adapter versions;
- requested and actual worker count;
- partition and grain versions;
- queue and in-flight limits;
- work and memory limits;
- deterministic test hooks such as forced delays when present;
- selected cancellation checkpoint for cancellation replays; and
- execution-plan digest when normative.

Ordinary replay need not reproduce operating-system scheduling. It must reproduce the same authoritative result or primary failure under any qualified schedule.

### 2.43 Logical execution traces

Diagnostic policy may retain a bounded logical trace containing canonical events such as:

- plan creation;
- task admission;
- task logical start and completion;
- output-record count;
- failure-record emission;
- merge phases;
- cancellation observation; and
- commit or rollback.

Trace order must be canonicalized by logical key and phase, not raw timestamp. A raw physical trace may be retained as non-authoritative debug data but must not enter canonical replay or digest domains.

### 2.44 Supported stage inventory

The component must maintain a versioned inventory of every stage adapter, identifying:

- serial-only, parallel-capable, or parallel-required status;
- serial reference implementation;
- ownership model;
- partition and merge versions;
- floating-arithmetic requirements;
- resource classes;
- cancellation points;
- structural counters;
- independent verifier; and
- Component 16 qualification tests.

A stage must not become parallel by an unrecorded implementation detail.

### 2.45 Provider qualification

Every worker, queue, partition, sort, reduction, and stage-adapter provider must pass:

- serial semantic equivalence;
- worker-count invariance;
- forced-schedule tests;
- resource-boundary tests;
- cancellation tests;
- exception and allocation-failure tests;
- race and deadlock stress tests;
- deterministic-report tests;
- replay tests;
- mutation tests; and
- structural performance gates.

Provider difficulty may produce a documented resource failure. It may not fall back to a nondeterministic or semantically weaker path.

### 2.46 Optimization change control

An optimization that changes any of the following requires an explicit provider or schema version review:

- canonical task keys;
- partition boundaries included in replay;
- merge comparator;
- duplicate-key semantics;
- floating reduction order;
- structural counter meaning;
- resource reservation model;
- failure-suppression rule;
- execution report content; or
- canonical artifact bytes.

The affected Component 16 qualification slice and permanent regressions must be rerun.

### 2.47 Worker shutdown

Shutdown must:

- stop task admission;
- communicate terminal state safely;
- wake all blocked workers and submitters;
- permit workers to finish or abandon work only at safe points;
- join every thread exactly once;
- release queue and worker resources;
- verify no outstanding task references remain; and
- leave reusable services in a documented state or destroy them fully.

Shutdown must not depend on static-object destruction order.

### 2.48 Race-detection seam

The implementation must expose enough deterministic stress and ownership diagnostics for compiler-provided race instrumentation to be useful when available, without making such instrumentation a dependency.

In-tree tests must still exercise:

- queue synchronization;
- cancellation state;
- resource counters;
- task-output publication;
- exception records;
- worker shutdown;
- transaction lifetime; and
- shared immutable cache initialization.

### 2.49 Deadlock and progress requirements

The wait graph among submitters, workers, queues, reservations, merge, nested work, and shutdown must have a documented progress argument.

At minimum:

- no worker may wait for work that can run only on itself while holding the last required capacity;
- no submitter may hold a lock needed by workers while waiting for queue space;
- cancellation wakes all relevant waits;
- resource waits, if supported, are bounded or cancelable;
- merge waits only for admitted tasks with known ownership; and
- shutdown cannot wait for a task blocked forever on further admission.

### 2.50 Final execution report

Each completed Boolean call must produce a deterministic execution report containing:

- execution and provider versions;
- requested and actual worker count;
- stages executed serially or concurrently;
- plan versions and canonical plan digests where required;
- task and work counters by stage;
- queue and in-flight high-water values;
- temporary and persistent resource high-water values;
- cancellation and terminal-failure state;
- deterministic merge and reduction versions;
- worker floating-environment qualification status;
- structural performance gate outcomes; and
- non-authoritative timings when enabled.

The report must be included in the Component 14 candidate and independently audited by Component 15 where its facts affect publication or resource claims.

## 3. Output contract

On successful initialization, the component must produce immutable or controlled execution services containing or referencing:

- the frozen execution and determinism policies;
- bounded worker ownership and lifecycle control;
- stage-adapter registry;
- canonical partition and task-plan services;
- bounded queue and backpressure services;
- resource-reservation integration;
- cancellation and terminal-state integration;
- worker floating-environment setup and verification;
- exception and failure capture;
- canonical sorting, merge, prefix-sum, and reduction services;
- structural counter registry;
- logical trace and replay hooks; and
- provider/version metadata.

For every completed parallel-capable stage, the component must produce or contribute:

- one verified `execution_plan` or serial-plan record;
- complete canonical task coverage evidence;
- one canonically merged proposed artifact or deterministic typed failure;
- resource reconciliation evidence;
- deterministic structural counters;
- execution diagnostics according to policy; and
- replay metadata sufficient to reproduce semantic behavior.

A successfully committed stage must guarantee:

- every logical work item required by the serial reference was covered exactly as specified;
- all worker inputs were immutable and all writes privately owned or exactly disjoint;
- no task-local identity or completion order escaped into the artifact;
- canonical merge produced the serial-equivalent artifact;
- deterministic reductions and primary-failure arbitration were used;
- all worker floating environments were qualified;
- resources were reserved and reconciled;
- cancellation and exceptions could not publish partial state;
- all workers joined before commit or rollback completed; and
- structural counters and report fields obey their declared comparison rules.

On failure, no partial execution service requiring unavailable workers and no partial stage artifact is published. The typed error must identify the stage/task key or service operation, provider and policy versions, requested and actual workers, relevant resource or environment witnesses, canonical primary finding, and replay metadata.

## 4. Required invariants and prohibited behavior

Required invariants:

- every parallelized stage has an executable serial semantic reference;
- worker count and scheduling do not change authoritative artifacts or errors;
- task inputs are immutable;
- task outputs are private or exactly disjoint;
- canonical keys and merge, not completion order, determine publication;
- every authoritative worker uses the qualified floating-point environment;
- task-local identities are remapped before publication;
- floating reductions affecting correctness have fixed semantics;
- failures are selected by canonical arbitration;
- nested concurrency and memory are globally bounded;
- cancellation, resource failure, and exceptions are transactional;
- every worker is joined before owned state is released;
- performance counters cannot substitute for correctness verification;
- optimization cannot weaken any predecessor or Component 15 contract; and
- all production and normative-test code is portable C++17 with no external dependency.

Prohibited behavior:

- detached threads;
- unbounded task queues or recursive worker-pool creation;
- shared mutable vectors, maps, topology, or reports without exact ownership and synchronization;
- assigning canonical IDs with a global fetch-add whose arrival order becomes observable;
- publishing task outputs in completion order;
- choosing the first worker-discovered failure;
- using pointer, address, thread ID, wall-clock time, random-device output, or hash iteration as a canonical tie-break;
- schedule-ordered authoritative floating accumulation;
- assuming the main thread’s floating environment is sufficient for workers;
- broad-phase pruning that differs semantically by worker count;
- concurrent event interning that assigns identity by race order;
- concurrent cleanup mutation without a complete non-interference proof;
- selecting the first completed canonicalization branch;
- omitting verification work to meet a performance target;
- continuing after cancellation in a way that can commit output;
- destroying joinable threads;
- leaking reservations, tasks, or transaction references; or
- invoking an external threading, tasking, concurrent-container, allocator, geometry, sorting, or profiling dependency.

## 5. Test and validation specification

### 5.1 Worker-service unit tests

Test:

- zero, one, two, and maximum configured workers;
- worker creation success and deterministic fallback;
- partial creation failure;
- queue admission and backpressure;
- spurious wakeups through test hooks;
- clean reuse and shutdown;
- no detached activity;
- exception capture; and
- floating-environment setup.

### 5.2 Serial-reference equivalence tests

For every parallel-capable stage and bounded fixture, compare serial reference with worker counts 1, 2, and maximum.

Compare:

- logical artifact bytes or documented semantic equivalence;
- primary and retained failures;
- precision and maximum witnesses;
- reports;
- replay metadata;
- resource totals; and
- deterministic counter categories.

### 5.3 Partition tests

For every partition provider, test:

- empty ranges;
- one item;
- grain minus one, grain, and grain plus one;
- uneven remainders;
- maximum task count;
- overflowed endpoints;
- canonical key uniqueness;
- complete coverage;
- no overlap for exclusive ranges; and
- deterministic plan digests.

### 5.4 Private-output ownership tests

Instrument task adapters to detect:

- two tasks writing one record;
- a task writing outside its assigned range;
- mutation of immutable input;
- task-local identity leakage;
- use after transaction rollback;
- output publication before task completion; and
- private-buffer reuse while still referenced.

### 5.5 Canonical-merge tests

Merge task outputs in every completion permutation for bounded task counts and in forced random-looking deterministic schedules for larger counts.

Cover:

- unique keys;
- equivalent duplicates;
- conflicting duplicates;
- explicit multiplicity ranks;
- missing records;
- hash collisions;
- partial comparator mutations;
- task-local ID remapping; and
- cancellation during merge.

### 5.6 Deterministic-reduction tests

Test checked sums, prefix sums, logical reductions, set union, ordered block concatenation, and maximum-with-witness using:

- all input permutations;
- different reduction trees;
- equal maxima;
- adjacent floating-point bounds;
- signed zero where relevant;
- malformed NaN or negative records;
- arithmetic overflow; and
- worker-count changes.

### 5.7 Worker floating-environment tests

Disturb test worker state before tasks and verify restoration or rejection for:

- rounding mode;
- signed-zero-sensitive expressions;
- subnormal handling;
- contraction-sensitive expressions; and
- prescribed conversion points.

Qualified workers must match known bit-pattern results. Unsupported state must fail deterministically before publication.

### 5.8 Broad-phase concurrency tests

Run Component 06 with:

- large disjoint meshes;
- clustered meshes;
- exact-bound contacts;
- inflated uncertainty boxes;
- equal spatial keys;
- reversed node-build order;
- worker counts 1, 2, and maximum; and
- forced task delays.

Compare candidates with the serial provider and exhaustive bounded all-pairs enumeration. No false negative is permitted.

### 5.9 Relation concurrency tests

Evaluate canonical relation sets with:

- mixed relation categories;
- exact ties;
- near-parallel cases;
- several failures;
- duplicate task-submission mutation;
- reordered candidates;
- environment disturbance; and
- cancellation.

Every canonical relation must have one equivalent record and deterministic failure arbitration.

### 5.10 Event-registry concurrency tests

Use cases with:

- many consumers of one event;
- distinct events at one coordinate;
- equal carrier parameters;
- hash collisions;
- reversed task completion;
- duplicate event seeds; and
- inconsistent duplicate construction mutations.

Final event identity and ordering must match the serial semantic reference.

### 5.11 Classification concurrency tests

Construct primitive classification records in parallel for:

- large uncut regions;
- many cut boundaries;
- disconnected shells;
- cavities and islands;
- equal-delta cycles;
- inconsistent cycle mutation;
- varied union order; and
- several seed-query failures.

Canonical groups, winding values, failures, and reports must match the serial reference.

### 5.12 Per-face triangulation concurrency tests

Triangulate many independent output polygons with:

- different complexity;
- holes and nested contours;
- repeated coordinates;
- ambiguous local ears;
- one typed failure among many successes;
- reversed face submission; and
- resource limits.

Global triangle IDs and final ordering must arise from canonical merge, not face completion order.

### 5.13 Cleanup concurrency restriction tests

Verify the default provider:

- discovers candidates in parallel;
- merges by canonical key;
- rejects stale or conflicting candidates;
- applies actions in deterministic order; and
- matches serial cleanup exactly.

Inject a provider that applies two apparently disjoint but budget- or topology-coupled actions concurrently. Qualification must detect divergence or invalid certificates.

### 5.14 Canonicalization concurrency tests

Use symmetric and repeated disconnected components to test:

- parallel descriptor generation;
- parallel component encoding;
- automorphism branch completion in different orders;
- equal component signatures;
- digest collisions;
- resource-limit termination; and
- final public-index assignment.

The first completed branch or component must not determine canonical output.

### 5.15 Final-verification concurrency tests

Run Component 15 with parallel:

- edge-use extraction;
- triangle checks;
- hierarchy construction;
- pair classification;
- event audits;
- cleanup-certificate checks;
- precision aggregation; and
- report generation.

Inject several simultaneous topology, geometry, report, and digest failures. The same primary finding and retained ordered set must result under every schedule.

### 5.16 Queue and backpressure tests

Test queue capacities:

- zero or invalid policy values;
- one;
- below worker count;
- equal to worker count;
- above worker count;
- exact resource limit; and
- one above limit.

Force producers and workers to alternate slowly. Verify progress, no deadlock, bounded memory, and deterministic output.

### 5.17 Nested-parallelism tests

Invoke parallel-capable components from worker tasks and verify:

- use of one global bounded service;
- serial nested fallback where configured;
- no worker explosion;
- no deadlock waiting for child tasks;
- correct resource accounting;
- cancellation propagation; and
- serial-equivalent results.

### 5.18 Resource-reservation tests

For every task resource class, test:

- exact reservation;
- conservative over-reservation and deterministic release;
- reservation failure before task admission;
- task failure after reservation;
- cancellation after reservation;
- persistent-output transfer on commit;
- concurrent contention; and
- arithmetic overflow.

### 5.19 Cancellation tests

Cancel:

- before worker creation;
- after worker creation but before submission;
- while tasks are queued;
- while tasks execute;
- while a submitter is under backpressure;
- during canonical sort;
- during merge;
- during stage verification;
- immediately before commit; and
- during shutdown.

All workers must join, reservations must reconcile, and no partial artifact may publish.

### 5.20 Failure-arbitration tests

Arrange failures with different canonical priorities to be discovered in every possible completion order.

Test:

- same stage, different feature keys;
- different stages;
- equal feature keys, different subcodes;
- equal numerical values, different witnesses;
- resource and geometry failures together;
- cancellation racing with failure; and
- diagnostic truncation.

The primary result must follow the frozen total order.

### 5.21 Early-stop safety tests

For any provider implementing early suppression:

- place a higher-priority failure in an unexecuted later task;
- place only lower-priority failures in suppressed tasks;
- vary partition and worker count;
- cancel during suppression; and
- compare with complete serial arbitration.

Unsafe suppression must be rejected by qualification.

### 5.22 Worker-exception tests

Inject deterministic exceptions at:

- task start;
- allocation;
- bounded-arithmetic adapter seams;
- output append;
- merge preparation;
- condition-variable waits through test hooks; and
- worker shutdown.

Verify typed mapping, joined workers, rollback, deterministic diagnostics, and no process termination.

### 5.23 Race and synchronization stress tests

Repeatedly stress:

- shared cancellation state;
- queue predicates;
- task-output publication;
- worker shutdown;
- resource counters;
- exception and failure records;
- reusable-worker idle transitions;
- stage destruction; and
- simultaneous submit and cancel.

Run under compiler-provided race instrumentation when available without introducing an engine dependency. Any race or undefined behavior is a qualification failure.

### 5.24 Deadlock stress tests

Use tiny queues, tight resource limits, nested-capable calls, forced delays, and cancellation to exercise all wait cycles. Every test must terminate within a deterministic logical work budget or produce a controlled test-infrastructure failure; the engine must not hang.

### 5.25 Two-pass construction tests

Inject:

- count overflow;
- inconsistent second-pass fill count;
- overlapping assigned ranges;
- missing range fill;
- extra records;
- cancellation between passes;
- allocation failure after counts; and
- worker-count changes.

Canonical output and failure witnesses must match serial checked construction.

### 5.26 Hash and container-order tests

Vary:

- hash seed through a test provider;
- bucket count;
- insertion order;
- rehash points;
- collision rate;
- qualified in-tree container alternatives; and
- extraction order.

Published artifacts and diagnostics must be identical after canonical merge.

### 5.27 Small-work threshold tests

For every serial/parallel threshold, test logical size:

- threshold minus one;
- threshold;
- threshold plus one; and
- several sizes around partition boundaries.

Authoritative results must be identical. Counter comparison must follow the declared schema.

### 5.28 Structural performance tests

Measure counters on:

- large disjoint meshes;
- large overlapping meshes;
- many small disconnected components;
- highly clustered candidates;
- many relations with few intersections;
- many intersections on carriers;
- many independent output faces;
- cleanup-heavy artifacts;
- highly symmetric canonicalization inputs; and
- large final-verification outputs.

Assert provider-specific structural envelopes without weakening correctness.

### 5.29 Memory high-water tests

Verify bounded memory behavior under:

- many queued small tasks;
- few large-output tasks;
- two-pass construction;
- streaming merge windows;
- cancellation with full queues;
- resource failure near limits;
- repeated stage reuse; and
- large diagnostics disabled and enabled.

In-tree accounting must reconcile after each run.

### 5.30 Replay tests

Replay executions with recorded:

- one, two, and maximum workers;
- partition versions;
- forced delays;
- queue limits;
- work limits;
- simultaneous-failure cases;
- cancellation checkpoints; and
- provider versions.

Physical scheduling may differ, but authoritative result or primary failure and deterministic reports must match.

### 5.31 Mutation tests

Corrupt execution behavior by:

- publishing in completion order;
- choosing the first worker failure;
- leaking a task-local ID;
- omitting one task output;
- duplicating one output;
- using a partial comparator;
- using pointer order as a tie-break;
- performing schedule-ordered floating accumulation;
- failing to join one worker;
- leaking one reservation;
- admitting work beyond hard limits;
- skipping precision inflation in a parallel broad phase;
- racing event interning;
- racing cleanup mutations; and
- accepting the first canonicalization branch.

Component 16 qualification must reject every mutation.

### 5.32 Determinism byte-stability tests

For fixed inputs, policies, and provider versions, repeat execution under allocator-layout changes, forced schedules, hash collisions, and thread counts. Compare:

- every committed stage artifact digest;
- final public mesh bytes;
- all reports;
- primary and retained failures;
- replay bytes;
- execution-plan digest where required; and
- structural counters under their comparison rules.

### 5.33 Provider replacement tests

For each alternate in-tree worker, partition, sort, reduction, or spatial provider:

- run serial equivalence;
- run the concurrency matrix;
- run resource and cancellation tests;
- run mutation tests;
- run replay compatibility;
- compare structural counters; and
- verify no public or normative artifact changes without an explicit version update.

### 5.34 Definition of done

Component 17 is complete only when:

- every parallel-capable stage has a serial semantic reference and task-adapter contract;
- the bounded C++17 worker service creates no detached or orphaned work;
- task inputs are immutable and outputs are private or exactly disjoint;
- canonical merge removes schedule and completion-order dependence;
- deterministic reductions and failure arbitration are effective;
- every authoritative worker establishes the qualified floating environment;
- nested parallelism and memory remain globally bounded;
- cancellation, resource exhaustion, and exceptions roll back transactionally with all workers joined;
- serial, two-worker, and maximum-worker executions produce identical authoritative results;
- execution reports and replay metadata are deterministic;
- structural counters detect accidental all-pairs and repeated-computation regressions;
- all concurrency and ownership mutations are rejected;
- performance optimization never weakens broad-phase conservatism, topology, precision, cleanup, canonicalization, or final verification; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
