# Component 01: Contract, Context, Identities, Errors, Transactions, and Resources

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. The production implementation and its normative tests must use portable C++17 and the C++ standard library only. No external library may be required, linked, vendored, or invoked.

The terms **must**, **must not**, **required**, **should**, and **may** are normative. A later implementation plan may change data structures and algorithms, but it must preserve the observable contracts and invariants defined here.

## 0. Purpose

This component establishes the immutable execution context within which every other component operates. It is the authority for:

- the requested Boolean operation and its truth table;
- the tolerance, input-precision, contact, output, verification, determinism, execution, diagnostic, and resource policies;
- platform and floating-point-environment qualification;
- stable identities and ownership domains for all pipeline entities;
- typed status and failure reporting;
- stage transactions, cancellation, and rollback;
- resource accounting and limit enforcement;
- deterministic selection of errors when several failures are discovered; and
- replay metadata sufficient to reproduce a run from input bit patterns and options.

This component does not perform geometric reasoning. It ensures that all later geometric and topological reasoning occurs under one frozen, versioned, deterministic contract.

## 1. Input contract

### 1.1 Required public inputs

The component must accept, by value or immutable reference:

1. a supported Boolean operation:
   - union;
   - intersection;
   - `A - B`;
   - `B - A`; or
   - symmetric difference;
2. a complete `bounded_boolean_options<T>` value;
3. immutable descriptions of the two source operands sufficient to compute replay digests, including:
   - the exact bit patterns of every coordinate;
   - facet ring sizes and index sequences;
   - the concrete scalar and index types;
   - any declared input precision metadata; and
   - any versioned solid-semantics metadata;
4. a compile-time and runtime platform descriptor; and
5. an optional cancellation source whose lifetime exceeds the Boolean call.

The component must not retain mutable references to caller-owned meshes or options.

### 1.2 Option validity requirements

Before any downstream component runs, this component must validate at least:

- all scalar options are finite;
- tolerance and input precisions are non-negative;
- tolerance is not smaller than any policy-required machine-error floor once that floor is available from Component 03;
- enum and policy values are recognized and version-compatible;
- resource limits are internally consistent;
- output index limits are compatible with `I`;
- diagnostic and verification modes do not request unsupported behavior;
- deterministic mode does not permit nondeterministic reductions or unordered publication;
- cancellation and execution settings can be implemented without detached work; and
- no option requests behavior outside the frozen public contract.

Validation may be completed in two steps when Component 03 must first provide the platform roundoff floor. The context remains unpublished until all checks succeed.

### 1.3 Floating-point environment requirements

The context must accept only a qualified environment for topology-affecting arithmetic. At minimum, qualification records and verifies:

- IEC 60559 binary representation for supported `T` values;
- nearest-even rounding for authoritative operations;
- preservation of signed zero where the specification distinguishes it;
- supported subnormal behavior, or deterministic rejection if unavailable;
- the configured floating-contraction policy;
- prohibition of fast-math assumptions, reassociation, finite-only assumptions, and unsafe reciprocal approximations;
- prescribed conversion and rounding points to `T`; and
- compiler/platform conformance identifiers used by permanent bit-pattern tests.

The component must return `unsupported_platform` before geometry processing if these requirements are not met.

### 1.4 Identity namespaces

The component must define disjoint, strongly typed identity domains for at least:

- context;
- operand;
- source shell;
- source vertex;
- source facet;
- source ring;
- source directed edge use;
- source undirected edge;
- source triangle;
- internal vertex occurrence;
- internal halfedge;
- broad-phase candidate;
- canonical relation;
- symbolic decision;
- intersection event;
- classification group;
- retained surface use;
- output vertex occurrence;
- output carrier;
- output halfedge;
- output face cycle;
- output triangle;
- cleanup action;
- verification finding; and
- replay record.

An ID must encode or be paired with enough owner information to reject stale handles, wrong-context handles, and accidental cross-operand substitution.

## 2. Required behavior

### 2.1 Context freezing

The component must construct one immutable Boolean context before downstream work begins. Freezing must:

- copy and normalize all options;
- assign policy-version identifiers;
- freeze the operation truth table;
- freeze the symbolic contact-policy matrix supplied to Component 07;
- establish deterministic comparison keys and canonical ordering rules;
- establish resource ceilings and overflow-safe counters;
- establish the requested verification level;
- establish the permitted concurrency model;
- initialize replay hashing from exact input bytes and normalized options; and
- publish a context only after validation succeeds.

No later component may mutate the frozen context. Runtime counters, cancellation state, diagnostics, and transaction state must live in separate controlled services referenced by the context.

### 2.2 Operation truth-table service

The component must expose an operation service that maps classified occupancy on the two sides of a candidate surface to:

- retain or discard;
- preserve or reverse orientation;
- operand ownership for coincident surfaces;
- multiplicity requirements; and
- the corresponding operation under operand exchange.

The truth-table service must be total over all supported classification states. Ambiguous or invalid states must produce a typed failure rather than an arbitrary selection.

The service must not inspect coordinates or apply tolerance comparisons.

### 2.3 Stable identity allocation

Identity allocation must satisfy all of the following:

- IDs are immutable after publication;
- IDs are never reused in one context;
- allocation is checked for overflow before mutation;
- canonical IDs are based on canonical source keys or a deterministic merge step, not worker timing;
- temporary task-local IDs cannot escape into published artifacts;
- published IDs have a defined total order;
- different identity domains are not implicitly convertible; and
- coordinate equality never affects identity allocation.

Parallel stages may allocate task-local records, but publication must remap them to canonical IDs in deterministic key order.

### 2.4 Transaction service

Every pipeline stage must execute transactionally. The transaction service must support:

- creation of a private mutable stage workspace;
- immutable access to all predecessor artifacts;
- resource reservations before large allocations;
- cancellation polling at documented safe points;
- recording of deterministic candidate failures without publishing them immediately;
- verification of the proposed artifact before commit;
- atomic publication of one immutable artifact on success; and
- complete rollback of temporary state on failure or cancellation.

A transaction must never expose partially assembled topology to a later stage. Rollback must release all reservations and join all worker activity before returning.

### 2.5 Typed error model

The error model must include, at minimum:

- `input_contract_error`;
- `input_geometry_not_epsilon_valid`;
- `unsupported_platform`;
- `invalid_tolerance`;
- `ambiguous_shell_semantics`;
- `geometric_condition_exceeds_tolerance`;
- `cleanup_budget_exceeded`;
- `result_geometry_not_validated`;
- `index_overflow`;
- `resource_limit`;
- `cancelled`; and
- `internal_invariant_error`.

Each failure record must contain:

- the error category and stable subcode;
- producing stage and component;
- context and operand identities where applicable;
- canonical feature identities involved;
- relevant nominal values and conservative bounds;
- tolerance and remaining budget at failure;
- policy versions;
- a deterministic human-readable summary;
- a machine-readable diagnostic payload; and
- a replay key or embedded replay payload according to policy.

Expected geometric difficulty, invalid input, unsupported tolerance, and resource exhaustion must never be reported as `internal_invariant_error`.

### 2.6 Deterministic error arbitration

Several parallel tasks may discover failures. The component must select the externally reported primary failure by a frozen total ordering, for example:

1. stage order;
2. failure severity class defined by contract;
3. operand order;
4. canonical feature tuple;
5. stable error subcode; and
6. deterministic numerical witness encoding.

The exact ordering may differ, but it must be documented, total, independent of thread scheduling, and covered by tests. Additional findings may be retained in diagnostics without changing the primary status.

### 2.7 Resource accounting

The component must account separately for at least:

- source and internal vertices;
- facets, triangles, and halfedges;
- broad-phase nodes and candidate pairs;
- canonical relations and intersection events;
- classification groups;
- output occurrences, carriers, cycles, and triangles;
- cleanup operations;
- diagnostic and replay storage;
- temporary bytes;
- persistent artifact bytes; and
- abstract work units used to prevent pathological computation.

Accounting must be overflow-safe and monotonic within a transaction. A component must reserve before allocating or publishing. Crossing a limit returns `resource_limit` or `index_overflow` with no partial output.

Resource policies must distinguish hard correctness limits from advisory performance targets.

### 2.8 Cancellation

Cancellation must be cooperative and deterministic at stage boundaries and documented inner-loop checkpoints. On cancellation:

- no new transaction may commit;
- active workers must observe cancellation and stop at safe points;
- all workers must be joined;
- temporary state must be rolled back;
- immutable predecessor artifacts remain valid;
- the result is `cancelled`; and
- diagnostics state the latest completed transaction and the canonical progress counters.

Cancellation must never produce a topology-only ordinary success.

### 2.9 Replay construction

The component must define a versioned replay format containing enough information to reproduce a failure or success deterministically. It must include:

- exact scalar bit patterns and index data for both operands;
- concrete type identifiers;
- normalized options;
- all policy versions;
- platform qualification identifiers;
- determinism and execution settings;
- resource limits;
- operation;
- canonical input digest; and
- expected primary status or output digest when stored as a regression.

Replay serialization must have a canonical byte representation and explicit versioning. Unknown required fields or versions must fail cleanly.

### 2.10 Cross-component service boundaries

The component must expose narrow services rather than a mutable global singleton. At minimum, later components need read-only access to:

- frozen policies;
- truth tables;
- symbolic-policy version and matrix;
- deterministic comparators;
- typed identity factories or canonicalization services;
- resource reservation interfaces;
- transaction construction;
- cancellation queries;
- diagnostic emission; and
- replay/digest accumulation.

Services must be usable in serial and deterministic concurrent execution without data races.

## 3. Output contract

On success, this component must produce an immutable `boolean_context` artifact and associated controlled runtime services.

The context artifact must contain or reference:

- normalized and validated operation/options;
- concrete type descriptors for `T` and `I`;
- policy and serialization versions;
- operation truth tables and operand-remapping rules;
- symbolic contact-policy definition or stable identifier;
- deterministic ordering definitions;
- resource limits;
- verification requirements;
- qualified floating-point environment metadata;
- canonical source-input digest seeds; and
- owner tokens for all identity domains.

The runtime services must provide:

- transactional stage execution;
- overflow-safe resource accounting;
- cancellation;
- deterministic diagnostic collection;
- identity publication support; and
- replay/digest finalization.

The artifact must be immutable, safely shareable across threads, and valid until the Boolean call and all joined worker activity complete.

On failure, no context is published to downstream components. The caller receives one deterministic typed error with replay information according to policy.

## 4. Required invariants and prohibited behavior

The implementation must preserve these invariants:

- all downstream artifacts belong to exactly one context;
- identity domains cannot be confused accidentally;
- IDs are immutable and not reused;
- all published stage artifacts are immutable and verified;
- a failed or cancelled transaction publishes nothing;
- resource limits are checked before irreversible work;
- error selection is scheduling-independent;
- replay bytes are canonical;
- options and policies cannot change during execution; and
- no service infers topology from floating-point proximity.

The implementation must not:

- use process-global mutable configuration;
- expose unchecked raw integer IDs across component boundaries;
- rely on pointer addresses, hash iteration order, locale, wall-clock time, random-device output, or thread order for canonical results;
- silently clamp invalid tolerances or limits;
- treat cancellation as successful partial completion; or
- use external dependencies in production or normative tests.

## 5. Test and validation specification

### 5.1 Unit tests

Unit tests must cover:

- every operation truth-table state;
- operand-remapping rules for all operations;
- every policy enum and version;
- all typed errors and stable subcodes;
- every identity domain and comparator;
- option normalization and rejection;
- finite/non-finite scalar options;
- resource reservation, release, and overflow;
- transaction commit and rollback;
- cancellation checkpoints; and
- replay encode/decode.

### 5.2 Identity and ownership mutation tests

Construct intentionally corrupt artifacts containing:

- duplicate IDs;
- stale IDs from an earlier context;
- a source-face ID used as a triangle ID;
- wrong-operand ownership;
- IDs reused after rollback;
- task-local IDs leaked into publication; and
- canonical key collisions.

The component verifier must reject each mutation deterministically.

### 5.3 Boundary tests

For every resource counter and every representable ID/index range, test:

- limit minus one;
- exactly the limit;
- limit plus one;
- arithmetic overflow before the declared limit;
- zero limits where meaningful; and
- very large but valid limits that cannot be eagerly allocated.

No test may depend on actually exhausting host memory.

### 5.4 Cancellation tests

Inject cancellation:

- before context freezing;
- immediately after context creation;
- before and after every stage transaction begins;
- during every parallelizable inner loop;
- after a candidate error is discovered but before arbitration;
- while diagnostics are being accumulated; and
- immediately before commit.

Verify complete rollback, joined workers, deterministic `cancelled` reporting, and absence of leaked partial artifacts.

### 5.5 Determinism and replay tests

For identical input bytes and options, vary:

- input container allocation addresses;
- facet and vertex traversal order supplied to internal test adapters;
- worker count;
- forced task delays;
- hash collision patterns;
- diagnostic capacity; and
- repeated execution count.

The frozen context digest, primary error, replay bytes, and all canonical IDs must remain identical.

### 5.6 Platform qualification tests

Run bit-pattern tests for:

- positive and negative zero;
- normals and subnormals;
- infinities and NaNs as rejected inputs/options;
- adjacent representable values;
- conversion round trips;
- rounding mode changes; and
- contraction-sensitive expressions.

Build configurations that enable unsafe floating-point transformations must be detected by compile-time controls, runtime conformance tests, or both, and must not pass qualification.

### 5.7 Transactional integration tests

Provide test doubles for later components that:

- succeed with a verified artifact;
- fail before allocation;
- fail after large reservations;
- emit several competing failures;
- throw only in test builds to simulate unexpected control transfer; and
- request cancellation during commit preparation.

Verify that the context services preserve ownership, rollback, and deterministic error semantics.

### 5.8 Definition of done

Component 01 is complete only when:

- all public policies and error categories are versioned and documented;
- all identity domains are strongly separated;
- transactions and resource accounting have independent verifiers;
- replay is byte-stable across supported builds and thread counts;
- cancellation cannot leak or publish partial work;
- every later component can depend only on the documented read-only services; and
- the implementation and tests compile as strict portable C++17 with no external dependencies.
