# Component 01: Contract, Context, Invocation Inputs, Identities, Errors, Transactions, and Resources

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production code and normative tests must use portable C++17 and the C++ standard library only. No external library may be required, linked, vendored, downloaded, or invoked by the bounded Boolean subsystem or its normative tests.

The terms **must**, **must not**, **required**, **should**, and **may** are normative. An implementation plan may choose different internal data structures, but it must preserve every observable contract and invariant in this specification.

## 0. Purpose

Component 01 establishes the immutable invocation contract under which Components 02-17 operate. It is the authority for:

- capture of the two public meshes into exact, call-owned immutable source snapshots;
- the requested Boolean operation and its total truth-table service;
- tolerance, input-precision, solid, contact, output, verification, determinism, execution, diagnostic, and resource policies;
- platform, build-mode, and floating-point-environment qualification;
- versioned identity domains, owner tokens, canonical-publication services, and source-position references;
- typed outcomes, errors, diagnostics, and deterministic primary-error selection;
- stage transactions, cancellation, rollback, and immutable publication;
- logical and physical resource accounting;
- canonical bytes, deterministic digests, and replay records; and
- narrow owner-checked capability views for every later component.

Component 01 does not validate source topology, establish shell semantics, evaluate geometric predicates, triangulate facets, enumerate collisions, construct intersections, classify winding, select surfaces, construct output topology, clean geometry, or perform final geometric verification.

Its central guarantee is that all later work consumes the same frozen policies and the same immutable input bytes, and that no failed, cancelled, resource-exhausted, partially verified, or partially committed stage can publish an artifact.

## 1. Public input contract

### 1.1 Required public inputs

The component must accept:

1. two `const fv_surface_mesh<T,I>&` operands;
2. one supported operation: union, intersection, `A - B`, `B - A`, or symmetric difference;
3. one complete `bounded_boolean_options<T>` value;
4. a compile-time and runtime platform descriptor; and
5. either a never-cancelled token or an immutable token obtained from a caller-owned cancellation source.

The ordinary overload without a cancellation argument must be equivalent to the cancellation-aware overload with a canonical never-cancelled token. Cancellation state must not be stored in `bounded_boolean_options<T>` and must not use a process-global registry.

The caller must not concurrently mutate either mesh while the public entrypoint is capturing it. After capture, no later component may borrow caller-owned mesh storage.

### 1.2 V1 scalar and index profiles

The ordinary V1 production profile supports exactly:

- `T == float` with the qualified IEC 60559 binary32 profile;
- `T == double` with the qualified IEC 60559 binary64 profile;
- `I == std::uint32_t`; and
- `I == std::uint64_t`.

Other scalar or index types must fail cleanly under a stable unsupported-type subcode unless a later version adds a complete implementation, explicit instantiations or header-only support, canonical type descriptors, golden replay records, and the full qualification matrix. V1 must not claim support for 8-bit or 16-bit public index types while later components and tests qualify only 32-bit and 64-bit indices.

### 1.3 Semantic public-mesh fields in V1

The V1 Boolean source contract consumes only:

- `mesh.vertices`, preserving every coordinate bit pattern and source position; and
- `mesh.faces`, preserving face order, ring lengths, index order, and index values.

The following existing `fv_surface_mesh` fields are non-semantic for V1 Boolean evaluation and must be ignored and excluded from canonical input, context, and replay bytes:

- `vertex_normals`;
- `vertex_colours`;
- `involved_faces`; and
- arbitrary `metadata`.

Input precision and solid semantics come from the versioned options, not from arbitrary metadata keys. Changing only an ignored field must not change the frozen context, replay digest, canonical diagnostics, or result.

A future version may recognize explicit metadata only by defining stable key names, encodings, precedence, conflict behavior, replay fields, and tests under a new source-contract version. No implementation may silently inspect user metadata for tolerance, precision, orientation, shell role, or identity.

### 1.4 Exact immutable source capture

Before downstream work, Component 01 must create one immutable source snapshot per operand. Each snapshot must contain enough information for Component 02 and replay without retaining caller storage:

- scalar and index type descriptors;
- operand role;
- source vertex count;
- exact x/y/z scalar bits in source order;
- source face count;
- checked face offsets or ring lengths;
- exact widened or typed source indices in source order;
- exact source-position references for diagnostics; and
- a domain-separated source digest.

The snapshot may use contiguous storage instead of the public nested vectors. Construction must preflight counts, sums, byte sizes, `size_t` representability, and configured limits before allocation. It must copy, not normalize, reorder, triangulate, deduplicate, repair, or inspect geometric meaning.

Source snapshots must preserve invalid source bytes as well as valid bytes so that Component 02 can issue the authoritative input failure and replay can reproduce it. In particular, capture does not reject non-finite coordinates merely to avoid recording them.

### 1.5 Option validation

Before a frozen context is published, Component 01 must validate at least:

- policy versions, enum values, and reserved fields;
- finite, non-negative tolerance and input precisions;
- ordinary-success eligibility when either input precision exceeds tolerance;
- the Component 03 machine-floor requirement once available;
- hard/advisory resource consistency and checked arithmetic;
- output-index compatibility with the V1 `I` profile;
- diagnostic and replay retention consistency;
- deterministic execution compatibility;
- cancellation/execution settings that prohibit detached work;
- required verification levels; and
- every V1 field that is fixed or unsupported.

Invalid values must not be silently clamped. Permitted normalization must be explicitly versioned and recorded. Policy negative zero may be normalized to positive zero; source coordinate signed zero must not be normalized by Component 01.

Any V1 output-metadata-preservation or equivalent reserved flag must be fixed to false. Requesting true is a typed contract failure because Component 14 requires empty optional public-mesh arrays and arbitrary metadata in the canonical V1 output.

Validation may use a private two-phase pending context while Component 03 derives the scale-aware machine floor. No pending context may escape to later components.

## 2. Qualified floating-point and build environment

Component 01 must accept only a qualified environment for topology-affecting arithmetic. Qualification records and verifies at least:

- the exact V1 scalar and index profiles;
- nearest-even rounding;
- preserved signed zero;
- gradual underflow and supported subnormal behavior;
- the frozen contraction policy;
- prohibition of fast-math, reassociation, finite-only assumptions, and unsafe reciprocal substitutions;
- prescribed conversion and rounding points to `T`;
- strict behavior under every supported optimization and LTO/IPO profile; and
- stable conformance-profile identifiers covered by permanent bit-pattern tests.

Canonical bytes may contain only stable semantic profile identifiers and observed capability results. Compiler banners, standard-library version strings, build paths, executable names, timestamps, host names, optional dependency versions, and Ygor's time-derived project version are non-authoritative diagnostics and must not affect canonical bytes.

Unsupported environments must return `unsupported_platform` before authoritative geometry processing.

A call-scoped floating-environment guard must establish and verify the required environment and restore the caller environment on every exit. Component 17 must apply the same qualified provider in every worker.

## 3. Frozen policies and services

### 3.1 Context freezing

Component 01 must publish one immutable `boolean_context<T,I>` only after all applicable validation, source capture, platform qualification, Component 03 bootstrap, table materialization, replay preflight, and independent verification succeed.

The frozen context must include or reference:

- operation and normalized options;
- V1 type descriptors;
- source-contract, public-contract, context, codec, digest, error, identity, resource, cancellation, transaction, truth-table, symbolic-policy, platform, and replay versions;
- the immutable source bundle and its digests;
- operation truth-table bytes and digest;
- symbolic contact-policy bytes and digest;
- deterministic comparator descriptions;
- resource ceilings;
- verification requirements;
- requested execution policy and currently available execution capability;
- platform and precision-bootstrap records;
- normalization records;
- context, input, and replay digests; and
- the owner token and fixed operand IDs.

No later component may mutate the context. Runtime counters, transactions, diagnostics, cancellation state, replay retention, and execution state must live in controlled services owned by the top-level invocation.

### 3.2 Operation truth table

The truth-table service must be total for all five operations and all sixteen four-bit side-occupancy tuples. It must return:

- result occupancy on the negative and positive conceptual sides;
- retain or discard;
- preserve, reverse, or not-applicable orientation;
- one-atom multiplicity response; and
- operand-remapped operation and owner-ranking information.

It must not inspect coordinates or apply tolerance. Equal result occupancy is not a regularized boundary. Occupied-negative/empty-positive preserves orientation; empty-negative/occupied-positive reverses it.

The 80 V1 cells must be generated or declared from one reviewed source of truth, canonically encoded, and independently recomputed by the verifier.

### 3.3 Symbolic contact-policy matrix

Component 01 owns the version, complete key domain, canonical representation, operand-remapping transform, total lookup, and digest of the V1 symbolic policy. Component 07 applies the policy only to eligible exact-nominal or exact-lineage ties.

The policy must cover, for every operation and operand role, vertex/vertex, vertex/edge, vertex/face, edge/edge, edge/face, equal-edge, tangent, coplanar, and coincident-face cases; orientation relationships; half-open ownership; crossing contribution; feature priority; coincident owner ranking; and operand exchange.

Symbolic decisions may change classification, ownership, and half-open attribution. They must never change nominal coordinates or create topology from proximity.

### 3.4 Narrow capability views

Later components must receive only the capabilities they need. At minimum Component 01 must expose owner-checked views for:

- immutable invocation sources;
- frozen policies and type/platform records;
- truth-table and symbolic-policy lookup;
- deterministic comparators;
- checked arithmetic for counts, sizes, and index conversion;
- identity publication;
- resource reservation and snapshots;
- cancellation polling and canonical progress;
- diagnostics and primary-error arbitration;
- transactions and immutable artifact publication;
- canonical bytes, digests, and replay accumulation; and
- serial or deterministic execution scopes.

No mutable singleton or process-global registry is permitted.

## 4. Identity and ownership contract

### 4.1 Domains

Component 01 must define disjoint strong identity domains for at least:

- context and operand;
- source shell, vertex, facet, ring, directed edge use, undirected edge, and triangle;
- internal vertex occurrence and halfedge;
- broad-phase candidate and canonical relation;
- symbolic decision and intersection event;
- classification group and retained surface use;
- output vertex occurrence, carrier, halfedge, face cycle, and triangle;
- cleanup action, verification finding, and replay record.

Different domains must not be implicitly convertible. Published IDs must have a total order and be paired with owner, version, and valid-range information sufficient to reject stale, wrong-context, wrong-domain, and wrong-operand handles.

### 4.2 Source positions versus canonical source IDs

A source array position used in replay or an early error is not a canonical topology identity. Component 01 may expose typed source-position references, but Components 02-05 must assign canonical source IDs through the Component 01 publication service after topology-dependent canonical keys are complete.

Component 01 therefore owns the ID schemas and factories; it must not preassign canonical source-vertex, edge, facet, shell, or triangle IDs from caller array order.

### 4.3 Canonical publication

Canonical ID publication must:

- accept only owner-validated task-private records with complete keys;
- preflight count and ID capacity;
- sort by a documented total key;
- compare complete keys rather than hashes alone;
- apply a declared duplicate policy;
- assign dense immutable ordinals only after final order is known;
- produce immutable reverse maps where required;
- reject escaping task-local IDs; and
- occur only inside a stage transaction.

IDs must never derive from coordinates, tolerance, pointer values, allocation order, worker timing, hash iteration order, or wall-clock data.

## 5. Typed outcomes and failures

### 5.1 Required categories

The error model must include at least:

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

Expected invalid input, geometric difficulty, tolerance insufficiency, unsupported environment, index exhaustion, host allocation failure, worker creation failure, container-capacity failure, resource exhaustion, and cancellation must not be reported as `internal_invariant_error`.

### 5.2 Failure evidence

Each public failure must include, where applicable:

- schema version, category, and stable subcode;
- component, logical stage, and checkpoint;
- context/input digest when available;
- operand and typed entity references;
- exact scalar bits, bounded numerical witnesses, tolerance, and remaining budget;
- policy/provider versions;
- a deterministic locale-independent summary;
- machine-readable sorted diagnostic fields;
- replay digest and retention disposition; and
- a precomputed total arbitration key.

A mandatory primary-error representation must remain constructible when optional dynamic allocation is unavailable. Optional findings may be truncated deterministically; the primary category, subcode, stage, checkpoint, bounded witnesses, summary template, and replay key may not be lost.

### 5.3 Exception translation

Expected standard failures must be translated at reviewed boundaries:

- correctly reserved `std::bad_alloc` to `resource_limit` host-allocation failure;
- checked-count `std::length_error` or `max_size()` rejection to container-capacity failure;
- Component 17 worker-creation `std::system_error` to a typed execution/resource failure; and
- producer use exceeding a successful reservation to `internal_invariant_error`.

Raw expected exceptions must not escape the public API. Unexpected exceptions must be caught at transaction/execution boundaries, all work joined, all environments restored, all private state rolled back, and one deterministic invariant error returned.

### 5.4 Deterministic arbitration and cancellation precedence

Ordinary candidate failures must be reduced by a documented total key that includes logical stage, checkpoint, category precedence, component, operand, entity tuple, subcode, and canonical witness bytes. Scheduling and discovery order must not affect the primary error.

Cancellation is not an ordinary candidate in that ordering. Once cancellation is validly observed at a required checkpoint before publication, the public result must be `cancelled` after all work is joined, environments restored, and private state rolled back. Earlier ordinary failures may remain only as secondary findings.

If join, restoration, rollback, or resource reconciliation fails, the corresponding deterministic invariant error overrides clean cancellation and retains cancellation as evidence.

## 6. Resource contract

### 6.1 Resource classes

Component 01 must account separately for at least:

- immutable source-snapshot vertices, face offsets, face indices, and bytes;
- source and internal topological entities;
- broad-phase nodes and candidates;
- relations, symbolic decisions, and events;
- classification and retained-use entities;
- output occurrences, carriers, halfedges, cycles, and triangles;
- cleanup actions and verification findings;
- diagnostics and replay storage;
- canonical-sort records and task descriptors;
- temporary and persistent bytes;
- emergency error storage; and
- abstract work units.

Hard correctness ceilings must be distinct from advisory thresholds.

### 6.2 Accounting semantics

All count and byte arithmetic must be checked before mutation or allocation. A producer must reserve before allocating or publishing. Crossing a hard logical limit returns `resource_limit` or `index_overflow` with no partial artifact.

Cumulative work, allocation attempts, and peak counters are monotonic. Live reserved and committed balances may decrease only through documented release, promotion, destruction, or rollback transitions. The specification must not describe live balances as monotonic while also requiring rollback and release.

Successful logical reservation does not guarantee host allocation. Host failures remain typed expected failures and must not select semantic winners.

Parallel work must use aggregate reservations or deterministic slices. Racing reservations or completion order must not determine which semantic records survive.

### 6.3 Transactions and resources

A transaction must own its private workspace, provisional IDs, reservations, execution scopes, diagnostics, and replay contributions. Rollback must release every private reservation and preserve immutable predecessor leases. Commit must reconcile actual use against reservations and promote only the exact persistent amount.

An independent resource verifier must reconstruct live totals and check owners, arithmetic, hard limits, peaks, transaction state, actual-versus-reserved use, and terminal release.

## 7. Transaction, cancellation, and publication contract

### 7.1 Transaction behavior

Every stage must execute transactionally and support:

- private mutable workspace;
- immutable predecessor access;
- reservation before large allocation;
- documented cancellation checkpoints;
- bounded deterministic failure collection;
- worker registration and mandatory join;
- independent verification of the proposed artifact;
- exact resource reconciliation;
- one atomic immutable publication on success; and
- complete rollback otherwise.

No partially assembled topology or mutable workspace may be visible to a later component.

### 7.2 Commit protocol

A conforming commit must, in order:

1. stop admitting work;
2. poll cancellation;
3. join all execution scopes;
4. canonicalize private records and IDs;
5. construct the proposed immutable artifact using reserved storage;
6. independently verify it;
7. finalize canonical bytes, digests, and replay contribution;
8. reconcile and promote persistent resources;
9. poll cancellation immediately before publication; and
10. publish exactly one immutable handle.

Commit publication must not allocate unreserved memory, execute caller callbacks, or depend on worker completion order.

### 7.3 Rollback

Rollback must join all work, restore qualified environments, discard private state and provisional IDs, release reservations, preserve predecessor artifacts, finalize deterministic failure/cancellation evidence, verify terminal invariants, and complete without publishing.

Rollback must be idempotent after exception capture. Failure to satisfy rollback invariants is an invariant error, not a clean resource or cancellation result.

## 8. Canonical bytes, digests, and replay

### 8.1 Canonical encoding

Canonical encoding must be explicitly versioned and fixed-endian. It must encode fixed-width integers, exact `float`/`double` bits, checked lengths, tagged required/optional fields, and nested framing without serializing native object memory, padding, pointers, `size_t`, container capacity, RTTI names, locale text, or implementation-defined enum layout.

Readers must reject truncation, duplicate singleton fields, invalid booleans/enums, count or offset overflow, unknown required fields, and trailing bytes. Unknown optional fields may be skipped only within configured limits.

### 8.2 Digest provider

The subsystem must provide an in-tree streaming SHA-256 implementation or another equally specified in-tree digest only under a new provider version. Existing MD5, SpookyHash, and `std::hash` are not suitable canonical integrity providers.

Digests may accelerate lookup and identify replay records. Full keys or canonical bytes remain authoritative for semantic equality and ordering.

### 8.3 Replay content

Replay must contain enough canonical information to reproduce the invocation:

- schema/provider versions;
- V1 type descriptors;
- operation and normalized options;
- normalization report;
- stable platform and precision-bootstrap records;
- truth and symbolic tables or their versioned canonical representation;
- resource limits and execution request;
- exact immutable source snapshot A in source order;
- exact immutable source snapshot B in source order;
- input/context digests; and
- expected status or output digest when stored as a regression.

Exact source array permutations intentionally change the invocation replay/input digest because replay reproduces the exact call. Tests for traversal-order independence must vary internal iteration over one frozen snapshot, not mutate the source-order bytes. Components 02 and later may publish separate canonical artifact digests that are invariant under equivalent source permutations.

Replay retention must be preflighted. `digest_only` may retain only the digest in the result but must not eliminate the immutable source snapshots needed by the active invocation. If a policy promises embedded full replay and the reservation cannot be made, the call must fail before downstream work.

## 9. Output contract

On success, Component 01 must produce an immutable invocation foundation containing:

- `artifact_handle<const boolean_context<T,I>>`;
- `artifact_handle<const immutable_invocation_sources<T,I>>` or an equivalent immutable source bundle referenced by the context; and
- a controlled invocation-owned runtime service bundle.

The source bundle and context must remain valid until the call and all joined worker activity complete. Later artifacts may retain immutable predecessor handles as required by their contracts.

On failure, no frozen context is published to downstream components. The public caller receives one deterministic typed error with replay information according to policy.

## 10. Required invariants and prohibited behavior

The implementation must preserve these invariants:

- every downstream artifact belongs to exactly one context owner;
- later components read only immutable call-owned source snapshots;
- canonical source IDs are not caller array positions;
- identity domains cannot be confused accidentally;
- IDs are immutable and not reused;
- all published artifacts are immutable and independently verified;
- failed or cancelled transactions publish nothing;
- hard limits and representability are checked before irreversible work;
- host-resource failures are typed and rollback-complete;
- cancellation precedence is deterministic;
- ordinary failure selection is schedule-independent;
- replay bytes are canonical and include exact V1 source fields only;
- ignored public-mesh fields cannot affect canonical behavior;
- options and policies cannot change during execution;
- precision bounds and cleanup budgets remain owned by Component 03; and
- no service infers topology from coordinate equality or proximity.

The implementation must not:

- retain caller vectors, nested face storage, strings, maps, or mutable mesh references after source capture;
- use process-global mutable configuration, counters, registries, allocators, or cancellation state;
- expose unchecked raw integer IDs across component boundaries;
- use pointer addresses, hash iteration order, locale, wall-clock time, random-device output, thread order, build paths, executable names, invocation timestamps, or project timestamps in canonical decisions;
- inspect arbitrary metadata for Boolean semantics;
- silently clamp invalid options or limits;
- treat cancellation as partial success;
- translate expected host-resource exhaustion to an invariant error;
- use legacy mesh Boolean implementations as providers; or
- use an external dependency in production or normative tests.

## 11. Test and validation specification

### 11.1 Source-capture tests

Tests must cover:

- empty and non-empty meshes for all four V1 type combinations;
- exact preservation of coordinate bits, including positive and negative zero;
- exact face order, ring lengths, and indices;
- invalid source indices and non-finite coordinate bits being captured for Component 02 rather than normalized;
- checked total-ring-length and byte overflow;
- caller storage addresses, capacities, and allocation layouts changing without canonical-byte changes;
- a test hook that mutates or destroys the caller mesh after capture and proves later stages use only the snapshot;
- changes to normals, colours, `involved_faces`, and metadata leaving context/replay bytes unchanged;
- actual vertex/face source-order permutations changing invocation replay bytes while internal traversal permutations do not; and
- unsupported scalar/index profiles failing deterministically.

### 11.2 Policy, truth, and symbolic tests

Tests must cover every operation, operand remap, policy version/enum, reserved field, scalar option boundary, execution combination, all 80 truth-table cells, the complete symbolic key domain, equal operands, point/edge/face contacts, same/opposite coincident faces, half-open owner categories, and operand exchange.

### 11.3 Identity and ownership mutation tests

Corrupt fixtures must include duplicate IDs, stale owners, wrong domains, wrong operands, task-local escape, source positions used as canonical IDs, duplicate canonical keys, forced hash collisions, and IDs reused after rollback. Independent verifiers must reject every mutation deterministically.

### 11.4 Resource and allocation tests

For every resource kind and public-index range, test limit minus one, limit, limit plus one, zero where permitted, checked arithmetic overflow, large ledger-only reservations, advisory crossings, deterministic slices, promotion, release, rollback, and terminal reconciliation.

Inject deterministic host allocation, container capacity, worker creation, optional diagnostic, replay retention, and emergency-error failures without exhausting real host memory. Expected host failures must remain typed; reservation contradictions must be invariants.

### 11.5 Cancellation and transaction tests

Inject cancellation before capture, during source-copy checkpoints, during context phases, before and after every transaction transition, during fake serial and parallel work, after an ordinary error, during diagnostics, and immediately before commit. Require joined work, restored environments, released reservations, intact predecessors, deterministic progress, and no partial publication.

Inject join, environment-restoration, rollback, active-work-at-commit, wrong-owner artifact, verifier rejection, and unexpected-exception contradictions and verify exact terminal states and error precedence.

### 11.6 Platform/build tests

Run bit-pattern and semantic witnesses for signed zero, normals, subnormals, adjacent values, rounding changes, contraction, reassociation, caller-environment restoration, strict-target markers, unsupported profiles, and LTO/IPO behavior.

Normative Boolean tests must build with optional dependencies disabled and without a downloaded test framework. Strict Boolean translation units must remain strict even though the current parent GNU build adds global `-ffast-math`.

### 11.7 Codec, digest, replay, and determinism tests

Provide golden replay records for float/u32, float/u64, double/u32, and double/u64; truncation at every byte boundary for bounded fixtures; invalid lengths, enums, booleans, fields, and trailing bytes; SHA-256 standard vectors and chunkings; collision-forcing lookup tests; exact source signed-zero preservation; policy-zero normalization; and build-metadata exclusion.

For one frozen source snapshot and normalized options, vary internal traversal, allocation addresses, fake workers, delays, hash collisions, optional diagnostic capacity, compiler diagnostic strings, build paths, executable names, and repeated execution. Context bytes, canonical IDs, primary status, and replay bytes must remain identical.

### 11.8 Definition of done

Component 01 is complete only when:

- exact call-owned V1 source snapshots are specified, implemented, and independently verified;
- no later component borrows caller mesh storage;
- ignored public-mesh fields and arbitrary metadata cannot affect canonical behavior;
- float/double with u32/u64 are the only claimed V1 type profiles;
- public operation/options/error/cancellation contracts are versioned and documented;
- ordinary and cancellation-aware entry paths are tested;
- all required ID domains are strong and owner-bound;
- source positions cannot masquerade as canonical topology IDs;
- all 80 truth cells and the complete symbolic matrix are independently verified;
- strict compile/link behavior is proven despite parent fast-math and requested LTO;
- the Component 03 two-phase precision handshake is complete and pending contexts cannot escape;
- expected host failures remain typed and the primary-error path survives optional allocation failure;
- ordinary primary-error bytes are invariant under scheduling and capacity permutations;
- valid cancellation overrides earlier ordinary failures after safe rollback;
- every transaction publishes one verified immutable artifact or nothing;
- canonical/replay bytes are bounded, fixed-endian, versioned, golden-tested, and build-instance-independent;
- unsuitable legacy serializers, hashes, queues, and Boolean implementations are not used;
- unit, property, adversarial, mutation, replay, platform, transaction, resource, cancellation, sanitizer, LTO, dependency-off, and type-matrix tests pass; and
- every later component depends only on documented immutable data and narrow controlled capabilities.
