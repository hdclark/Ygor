# Plan 01: Contract, Context, Invocation Inputs, Identities, Errors, Transactions, and Resources

## 0. Scope and non-negotiable constraints

Implement **only Component 01** from `component_01_context_contracts.md`. Component 01 owns the public invocation boundary, exact immutable source capture, frozen policies, owner and identity infrastructure, typed outcomes and errors, checked resource accounting, cancellation, transactions, canonical bytes, deterministic digests, replay, and the narrow controlled services consumed by Components 02-17.

It must not validate mesh topology, determine shell nesting, evaluate bounded geometric predicates, triangulate facets, build halfedges, enumerate collision candidates, construct intersections, classify winding, select Boolean surfaces, construct output cycles, triangulate output polygons, clean geometry, assemble the public result, or perform final geometric verification.

The following rules apply from the first implementation commit:

- production and normative-test code use portable C++17 and the C++ standard library only;
- no external, vendored, downloaded, optional, subprocess, runtime-invoked, or framework dependency is added;
- do not call, adapt, copy, or derive implementation from `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- do not infer identity, adjacency, equivalence, or ordering from coordinates, tolerance, hashes alone, pointers, allocation order, source traversal order, worker completion order, wall-clock time, locale, random-device output, executable paths, build paths, or Ygor's timestamp-derived project version;
- do not retain caller-owned mesh vectors, nested face vectors, maps, strings, or mutable references after the source-capture transaction;
- do not inspect arbitrary `fv_surface_mesh::metadata` for Boolean semantics in V1;
- all expected failures are typed values and no raw expected standard exception escapes the public API;
- cancellation observed before publication returns `cancelled` only after all work is joined, floating environments are restored, and private state is rolled back;
- no failed, cancelled, resource-exhausted, partially verified, or partially committed stage publishes an artifact;
- every public and inter-component schema, provider, enum, stage, checkpoint, resource kind, canonical field, and replay field is explicitly versioned;
- bounded Boolean production code and normative tests are isolated from the repository's global GNU `-ffast-math` flags and from unsafe LTO/IPO transformation;
- normative tests do not link optional Eigen/GSL support or a downloaded test framework; and
- `tracker.md` is marked complete for Component 01 only after the complete plan, its cross-component contracts, and its definition of done are internally consistent. The tracker mark records completion of this planning/review step, not implementation completion.

## 1. Review conclusions and required corrections

The original Component 01 plan had a strong transaction/error/resource foundation, but this review identified four integration defects that must be corrected in the implementation plan:

1. **Component 02 requires an immutable source input owned by Component 01.** The original plan encoded replay input but did not define a reusable immutable source artifact. Add a first-class `immutable_invocation_sources<T,I>` artifact and checked read-only `immutable_source_mesh_view<T,I>` capability. Components 02 and 15 must never reread caller vectors.
2. **V1 index support must match downstream qualification.** Restrict ordinary V1 production to `std::uint32_t` and `std::uint64_t`. Do not claim 8-bit or 16-bit support while Components 02-16 and the public type matrix qualify only u32/u64.
3. **V1 mesh metadata policy must be unambiguous.** Only `vertices` and `faces` are semantic input. Normals, colours, `involved_faces`, and arbitrary metadata are ignored and excluded from canonical source/replay bytes. Input precision and shell policy come from options. V1 output metadata preservation is fixed false.
4. **Replay order and canonical topology identity are different concepts.** Exact source order belongs in replay and the invocation digest; source positions are not canonical source IDs. Components 02-05 assign canonical topology IDs after topology-dependent keys are known. Tests must distinguish source-order mutation from internal traversal permutation.

These corrections preserve the broad plan's requirements: exact topology remains separate from bounded geometry; the invocation is replayable; later stages consume immutable artifacts; canonical identities never arise from coordinate equality; and final success remains gated by Component 15.

## 2. Existing Ygor assessment and reuse decisions

### 2.1 Reuse unchanged as narrow public data carriers

Reuse from `src/YgorMath.h`:

- `fv_surface_mesh<T,I>` only as the public input carrier at call entry and, much later, as the Component 14 output carrier;
- `vec3<T>` only to read nominal x/y/z source values and to expose public result coordinates; and
- standard-library containers and synchronization primitives behind the contracts below.

The public mesh is permissive and mutable. Its fields do not establish Boolean validity. Component 01 must read only:

- `mesh.vertices`; and
- `mesh.faces`.

Preserve exact scalar bit patterns, source vertex positions, face positions, ring lengths, and index values. Do not use `vec3::operator<`, `operator==`, arithmetic methods, `Dot`, `Cross`, normalization, distance, angle, or text conversion for identity or canonical source encoding.

### 2.2 Ignore non-semantic public fields in V1

Do not consult or canonically encode:

- `vertex_normals`;
- `vertex_colours`;
- `involved_faces`; or
- arbitrary `metadata`.

Do not validate the consistency of these ignored fields as part of the Boolean input contract. Component 02 independently reconstructs incidence and geometry from vertices/faces. Tests must prove that changing only ignored fields leaves the context digest, replay bytes, canonical diagnostics, and result unchanged.

A future source-contract version may recognize explicit metadata keys only after names, encoding, precedence, conflicts, replay fields, and tests are specified. V1 must not opportunistically discover precision or shell roles from metadata.

### 2.3 Do not reuse public mesh mutators

Component 01 and its source adapter must not call:

- `merge_duplicate_vertices`;
- `convert_to_triangles`;
- `remove_degenerate_faces`;
- `remove_disconnected_vertices`;
- `simplify_inner_triangles`;
- `compute_vertex_normals`;
- `recreate_involved_face_index`;
- `apply_involved_face_index_diff`; or
- any orientation, remeshing, slicing, hole, BSP, or legacy Boolean helper.

Source capture is a byte-preserving structural copy, not normalization or validation.

### 2.4 Reuse existing adaptive predicates only through Component 03

`YgorMeshesAdaptivePredicates` contains useful in-tree expansion arithmetic, but Component 01 performs no geometric predicate. Platform qualification may share strict low-level floating-environment witnesses with Component 03, but Component 01 must not call `orient_sign` or any geometric predicate.

Component 03 owns scale-aware floor derivation and bounded numerical services. Component 01 only provides the pending-context handshake, frozen platform record, exact source views, and transaction infrastructure.

### 2.5 Existing serializers and hashes are unsuitable

Do not use `YgorSerialize`, `YgorMathIOSerialization`, XML/text mesh serializers, native struct serialization, `std::hash`, `External/SpookyHash`, or `External/MD5` for canonical context, replay, error, identity, or artifact bytes.

Implement one fixed-endian canonical byte codec and one clean-room in-tree streaming SHA-256 provider under the bounded subsystem. Digests are integrity and lookup accelerators; full keys or bytes remain authoritative equality.

### 2.6 Existing queues are unsuitable

Do not use `YgorThreadPool.h::work_queue` or `YgorContainers.h::taskqueue` for authoritative work. Their unbounded admission, generic exception behavior, missing resource/cancellation contracts, and schedule-dependent publication cannot satisfy the broad plan.

Component 01 supplies a serial execution scope and the transaction/execution interface. Component 17 later supplies the invocation-owned bounded deterministic worker service without replacing Component 01 cancellation, resources, errors, or transaction semantics.

### 2.7 Required build isolation

The root build currently:

- derives `PROJECT_VERSION` from an invocation timestamp;
- appends GNU `-ffast-math` globally;
- optionally enables project-wide IPO through `USE_LTO`; and
- optionally locates/links Eigen and GSL.

`src/CMakeLists.txt` globs only top-level `*cc` and links SpookyHash/MD5 into the general `ygor` target.

Do not change unrelated Ygor behavior in Component 01. Add a dedicated bounded Boolean object/static target in a subdirectory, append strict floating flags after inherited global flags, disable IPO by default on strict targets, and link normative tests directly to strict bounded objects plus required toolchain facilities such as `Threads::Threads`. The bounded code may later be added to `ygor`, but canonical services must not call the general target's legacy hashes or optional libraries.

## 3. Exact file and target layout

### 3.1 Installed public header

Create `src/YgorMeshesBooleanBounded.h` containing only stable public declarations:

- explicit `boolean_operation`;
- policy enums and value types;
- `bounded_boolean_options<T>`;
- public error category and immutable public error/result views;
- `bounded_boolean_cancellation_source` and `bounded_boolean_cancellation_token`;
- digest/replay identifiers required by result types;
- forward declarations of `bounded_boolean_result<T,I>`; and
- ordinary and cancellation-aware `bounded_boolean` overloads.

Do not expose mutable service implementations, internal source snapshots, internal IDs, transaction types, or implementation-only includes.

### 3.2 Internal Component 01 files

Create under `src/YgorMeshesBooleanBounded/`:

- `CMakeLists.txt` — strict object/static target, compiler-option probes, IPO isolation, dependency-free test helpers;
- `StrictFloatingBuild.h` — compile-time strict-profile markers and unsafe-mode rejection;
- `ContractVersions.h` — sole version and stable numeric registry;
- `CheckedArithmetic.h` — checked add/subtract/multiply/range/count-to-byte/narrowing helpers;
- `Outcome.h` — C++17 typed success/error carrier;
- `PublicMeshReadView.h` — non-owning call-entry view exposing only vertices/faces and source positions;
- `ImmutableSourceMesh.h/.cc` — exact contiguous source snapshot, checked access, source-position references, digest fields;
- `InvocationSources.h/.cc` — immutable A/B source bundle and capture transaction;
- `InvocationSourcesVerifier.h/.cc` — independent source-copy, range, field-exclusion, and digest verifier;
- `CanonicalBytes.h/.cc` — fixed-endian framed reader/writer and preflight;
- `Sha256.h/.cc` — clean-room streaming SHA-256;
- `PlatformQualification.h/.cc` — type/build/runtime floating qualification and call guard;
- `Policies.h/.cc` — option normalization, execution capability, truth table, operand remapping, symbolic-policy materialization;
- `Identity.h/.cc` — strong IDs, source-position types, owner tokens, canonical publication factories;
- `Errors.h/.cc` — internal records, subcodes, summaries, total keys, emergency primary-error path;
- `Resources.h/.cc` — resource manager, reservations, leases, slices, snapshots, allocation-boundary helpers, verifier support;
- `Cancellation.h/.cc` — public source/token backing state, checkpoints, progress, finalization state;
- `Diagnostics.h/.cc` — mandatory ordinary reducer, cancellation slot, bounded secondary findings;
- `Replay.h/.cc` — replay schema, source embedding, retention, encode/decode, digest linkage;
- `Context.h/.cc` — pending/frozen context, source-bundle linkage, builder, immutable views, runtime bundle;
- `Transaction.h/.cc` — transaction state machine, execution-scope registration, commit/rollback;
- `ContextVerifier.h/.cc` — independent context, policy, service, source-linkage, and canonical-byte verification.

Keep templates header-defined or explicitly instantiate exactly the four V1 type combinations. Unsupported instantiations must fail at the public boundary or be unavailable by design; they must not compile accidentally with incomplete replay/type support.

### 3.3 Normative tests

Create `tests/mesh_boolean_bounded/` with a dependency-free local test harness and at least:

- `TestMain.cc`, `TestSupport.h/.cc`;
- `TestInvocationSources.cc`;
- `TestPublicMeshReadView.cc`;
- `TestContextUnit.cc`;
- `TestContextProperties.cc`;
- `TestContextAdversarial.cc`;
- `TestContextPlatform.cc`;
- `TestContextReplay.cc`;
- `TestContextTransactions.cc`;
- `TestContextResources.cc`;
- `TestContextCancellation.cc`;
- `TestContextBuildIsolation.cc`;
- `TestContextMutation.cc`;
- `GoldenReplayV1.h`; and
- deterministic failure-injection providers.

Register focused CTest cases rather than one monolith so failures identify the contract area. Use a fixed deterministic PRNG and no `random_device`.

### 3.4 CMake integration

1. Add `add_subdirectory(YgorMeshesBooleanBounded)` from `src/CMakeLists.txt`.
2. Because the existing glob is non-recursive, subdirectory `.cc` files will not be compiled twice; preserve that property explicitly in comments/tests.
3. Define `ygor_apply_mesh_boolean_strict_fp(target)` and apply it to every bounded production and normative-test target.
4. For GNU, append at least `-fno-fast-math -frounding-math -fno-associative-math -fno-reciprocal-math -fsigned-zeros -fno-finite-math-only -ffp-contract=off` after global flags, subject to compiler-option checks.
5. For Clang, append `-fno-fast-math -frounding-math -ffp-contract=off` and every supported signed-zero/reassociation option needed by the qualified profile.
6. For MSVC, use `/fp:strict` and add separate qualification before claiming support.
7. Unknown compiler/profile combinations build only when a reviewed strict profile and conformance fixture exists; otherwise the public call returns `unsupported_platform` or the target is not offered.
8. Define `YGOR_MESH_BOOLEAN_STRICT_FP_BUILD=1` only on strict targets and reject `__FAST_MATH__` in strict source.
9. Set `INTERPROCEDURAL_OPTIMIZATION FALSE` on strict targets by default. A later LTO profile requires a separate version and compile/link semantic witnesses.
10. Inspect effective compile commands in a test fixture and verify the last effective floating options are strict.
11. Link normative tests to the bounded object/static target and `Threads::Threads`, not to the general `ygor` target when that would transitively acquire GSL or legacy hash objects.
12. Add configurations with `WITH_GNU_GSL=OFF`, `WITH_EIGEN=OFF`, `USE_LTO=OFF`, and supported `USE_LTO=ON` behavior.

## 4. V1 public types, policies, and options

### 4.1 Supported types

Freeze V1 ordinary production support to:

```text
T: float, double
I: std::uint32_t, std::uint64_t
```

Require IEC 60559 binary32/binary64 properties for `T`. Require unsigned, non-bool, exact 32-bit or 64-bit `I`. Use stable explicit type descriptors; never serialize `typeid().name()`.

Unsupported types use a stable unsupported-type subcode. Adding another type requires a new or explicitly compatible type-profile version, complete instantiation strategy, exact bit codec, index-boundary tests, replay goldens, and all downstream qualification.

### 4.2 Operations

Use an explicit fixed-width enum:

```cpp
enum class boolean_operation : std::uint8_t {
    set_union = 1,
    intersection = 2,
    a_minus_b = 3,
    b_minus_a = 4,
    symmetric_difference = 5,
};
```

Define `swap_operands`: union/intersection/xor map to themselves; the two differences swap. Unknown values fail before source processing beyond required replay/error preflight.

### 4.3 Closed V1 policy set

Expose fixed value policies, not callbacks:

- `solid_policy_kind::outward_oriented_alternating_shells_v1`;
- `contact_policy_kind::regularized_symbolic_v1`;
- `output_policy_kind::triangulated_oriented_manifold_v1`;
- verification levels with a mandatory scalable floor and optional diagnostics/test-only exhaustive mode;
- `determinism_mode::canonical_v1`;
- execution modes `serial_v1` and `deterministic_parallel_v1`;
- replay retention `digest_only`, `full_on_failure`, `full_always`.

Every nested policy has an explicit version and zeroed V1 reserved fields. Unknown required versions or nonzero reserved fields fail.

The V1 output policy must not promise preservation of input normals, colours, `involved_faces`, or arbitrary metadata. If the public struct retains a future-facing `preserve_public_metadata` field, V1 requires `false`; `true` is a typed unsupported-option failure, not a silent no-op.

### 4.4 Options

Implement:

```cpp
template<class T>
struct bounded_boolean_options {
    T tolerance = T(0);
    T input_precision_a = T(0);
    T input_precision_b = T(0);
    solid_policy solids{};
    contact_policy contacts{};
    output_policy output{};
    verification_policy verification{};
    determinism_policy determinism{};
    execution_policy execution{};
    resource_policy resources = resource_policy::conservative_defaults();
    diagnostic_policy diagnostics{};
};
```

Validate finite non-negative tolerance and precisions. For ordinary success require each input precision `<= tolerance` and, after Component 03 bootstrap, tolerance at least the qualified scale-aware floor. Permit a larger precision only for an explicitly internal topology-only diagnostic attempt that is ineligible for ordinary success.

Do not silently clamp workers, limits, diagnostic capacities, precision, or tolerance. `requested_workers == 0` remains a frozen automatic request rather than being replaced in the context by `hardware_concurrency()`. Component 17 records the actual qualified selection.

### 4.5 Cancellation API

Implement caller-owned source/token state with these semantics:

- source and token share lifetime-safe backing state;
- `request_cancel` is idempotent and stores only the first stable reason code;
- token is a cheap immutable polling view safe for concurrent reads;
- no callbacks, detached waiters, signal hooks, process globals, pointer-derived identity, or timestamp enters semantics;
- request time is non-authoritative; the checkpoint where cancellation is observed is authoritative;
- source/token runtime identity is not serialized or hashed; and
- the no-token overload delegates through a canonical never-cancelled token.

## 5. Exact immutable invocation-source capture

### 5.1 Public read view

`PublicMeshReadView<T,I>` is created only at public call entry and exposes:

- vertex count and checked vertex-at-source-position access;
- face count, face ring length, and checked source index access;
- exact scalar bits through a qualified `memcpy` conversion;
- source-position objects for diagnostics; and
- no access to normals, colours, `involved_faces`, or metadata.

The view is non-owning and may exist only during the capture transaction. It must not escape into the frozen context or a later artifact.

### 5.2 Snapshot schema

Implement immutable `immutable_source_mesh<T,I>` with contiguous canonical storage:

- source-contract version and V1 type descriptors;
- fixed operand ID A or B;
- vertex count;
- contiguous exact x/y/z `T` values or exact scalar-bit records in source order;
- face count;
- checked `face_offsets` of length `face_count + 1` or equivalent ring-length table;
- contiguous source indices in source order;
- exact source-position maps;
- section byte counts and resource leases;
- source content bytes/digest; and
- verification disposition.

The snapshot is an exact structural copy, not a validated operand. It may contain out-of-range indices, undersized rings, repeated indices, non-finite coordinate bits, or other data that Component 02 later rejects. Capture itself rejects only unsupported memory representation, unreadable/corrupt public-container state that cannot occur without concurrent mutation/undefined caller behavior, count/byte/index-storage overflow, resource failure, cancellation, or internal contradiction.

### 5.3 Capture algorithm

Use one transaction for the A/B source bundle:

1. validate public type profile and token lifetime;
2. read counts without traversing geometry;
3. check vertex count, face count, each ring length, total index count, offset arithmetic, byte arithmetic, `size_t` representability, configured hard limits, and replay-retention requirements;
4. reserve exact source-snapshot entities and bytes before allocation;
5. allocate contiguous destination storage through the reviewed allocation-boundary helper;
6. copy A vertices in source order by exact bits, polling at deterministic ranges;
7. copy A face offsets and indices in source order;
8. repeat for B;
9. verify actual counts equal preflight counts, detecting caller concurrent mutation as an input-contract/unsupported-use failure without out-of-bounds access;
10. encode domain-separated source sections and compute their digests;
11. independently verify the snapshots against a second read pass while the public view is still valid, or use captured pre/post structural epochs/count witnesses sufficient to detect mutation without borrowing after commit;
12. promote exact persistent resource leases; and
13. publish one immutable `immutable_invocation_sources<T,I>` bundle.

After commit, release all non-owning public views. The top-level invocation and every later component use only immutable snapshots.

### 5.4 Source order, canonical identities, and digests

Define three separate concepts:

- **source position:** exact caller array/ring position, used by replay and early diagnostics;
- **invocation source digest:** exact vertices/faces in source order, so it changes when the public invocation bytes change;
- **canonical source ID/artifact digest:** assigned by Component 02 or later from complete topology-dependent keys and eligible to remain invariant under equivalent input permutations.

Component 01 must not assign canonical source vertex/facet/edge/shell IDs from source positions. It owns strong ID schemas and publication factories only.

Tests that vary internal traversal over one frozen snapshot must preserve Component 01 bytes. Tests that physically permute source arrays create a different invocation replay digest even when later canonical artifacts/results are equivalent.

### 5.5 Ignored-field invariance

Create pairs of public meshes with identical vertices/faces but different:

- valid/invalid/empty normals;
- colours;
- stale or malformed `involved_faces`; and
- arbitrary metadata.

Source snapshots, context bytes, replay bytes, operation decisions, and eventual output must be identical. This test prevents accidental use of convenience caches or undocumented metadata semantics.

## 6. Stable versions and numeric registries

`ContractVersions.h` is the sole authority. Add explicit nonzero V1 values for:

- public API and type profile;
- source public-read contract;
- immutable source mesh and A/B bundle;
- source canonical encoding/digest domains;
- cancellation API;
- options and every policy;
- truth table and symbolic matrix;
- identity and owner tokens;
- error and diagnostic schemas;
- resources, reservations, leases, slices, and snapshots;
- canonical bytes and SHA-256 provider;
- platform/strict-build profile;
- pending/frozen context;
- transaction/execution-scope contract;
- replay and retention;
- context/source verifiers.

Use fixed-width underlying types and explicit numeric assignments. Reserve zero for invalid/uninitialized where applicable. Add compile-time uniqueness tests. Decoders reject unknown required versions, gaps where forbidden, and nonzero reserved V1 fields; they never reinterpret an unknown version as latest.

Define stable stage IDs for public entry/source capture, context preflight, platform/precision bootstrap, Components 02-15 pipeline stages, publication, Component 16 qualification, and Component 17 execution service effects. Later plans extend reserved ranges without renumbering released values.

## 7. Checked outcomes, errors, and deterministic arbitration

### 7.1 Outcome carrier

Implement non-default-constructible `boolean_outcome<T>` over `std::variant<T,boolean_error>` with explicit success/failure factories and a `void` specialization. Expected access does not throw. Payloads should be nothrow-destructible; moving a completed outcome must not introduce an expected allocation failure.

### 7.2 Stable categories and subcodes

Use explicit values for all broad-plan categories. Allocate disjoint Component 01 subcode ranges covering at least:

- unsupported T/I profile;
- unknown version/enum/reserved field;
- non-finite/negative option;
- precision exceeding tolerance;
- tolerance below Component 03 floor;
- unsupported output metadata preservation;
- source count/ring-sum/byte/offset overflow;
- source mutation during capture;
- source snapshot mismatch;
- unsupported build/rounding/signed-zero/subnormal/contraction/IPO profile;
- resource inconsistency, arithmetic overflow, hard limit, host allocation, container capacity, worker creation;
- emergency-error fallback;
- wrong/stale owner, wrong ID domain/operand, duplicate complete key, task-local escape;
- illegal transaction transition, active work at commit, verifier rejection;
- codec truncation/overflow/unknown field/trailing data;
- digest/replay mismatch;
- cancellation checkpoint and cancellation rollback contradiction; and
- unexpected exception.

### 7.3 Error record

`boolean_error` stores stable mandatory fields in fixed/pre-reserved capacity:

- version, category, subcode;
- component, logical stage, checkpoint;
- context/source digest when available;
- optional operand and up to a bounded number of typed entity/source-position references;
- exact tolerance/precision/budget bits and other bounded witnesses;
- sorted typed diagnostic fields;
- stable locale-independent summary template;
- provider/policy versions;
- replay digest/disposition; and
- precomputed total-order key.

Optional text and secondary fields may be dropped deterministically when capacity is unavailable. Pointers, thread IDs, timestamps, allocator addresses, `exception::what()`, build paths, and compiler-generated names never enter canonical fields.

### 7.4 Exception taxonomy

At reviewed allocation/execution boundaries:

- correctly reserved `std::bad_alloc` -> `resource_limit::host_allocation_failed`;
- valid checked count but `std::length_error`/`max_size()` rejection -> `resource_limit::container_capacity_failed`;
- Component 17 worker creation `std::system_error` -> typed worker/execution resource failure;
- producer actual use above reservation or missed checked arithmetic -> `internal_invariant_error`;
- unrelated exception -> deterministic invariant after join/restoration/rollback.

Catch exact expected exception types before generic `std::exception`. Never use `what()` to classify canonically.

### 7.5 Primary ordering and cancellation

For ordinary candidates use a frozen total key:

```text
logical stage
checkpoint
category precedence
component
operand rank
primary entity/source position
secondary entity/source position
subcode
canonical witness bytes
```

The exact category precedence is a contract version. It is reporting order, not severity.

Cancellation has a separate mandatory slot. Once validly observed before publication, it becomes the public outcome after safe finalization regardless of the ordinary reducer minimum. A join/restoration/rollback contradiction becomes the primary invariant error with cancellation retained as evidence.

## 8. Resource accounting and allocation boundaries

### 8.1 Resource kinds

Define explicit resource kinds for at least:

- immutable source vertices;
- immutable source faces/offsets;
- immutable source indices;
- immutable source bytes;
- source/internal vertices, rings, facets, edges, triangles, and halfedges;
- broad-phase nodes/candidates;
- relations, symbolic decisions, and events;
- classification groups and retained uses;
- output occurrences, carriers, halfedges, cycles, and triangles;
- cleanup actions and verification findings;
- diagnostic findings/bytes and replay bytes;
- canonical-sort records and task descriptors;
- temporary bytes, persistent bytes, emergency error storage; and
- abstract work units.

Keep hard correctness ceilings distinct from advisory thresholds.

### 8.2 Manager semantics

A context/invocation-owned thread-safe `resource_manager` tracks for each kind:

- hard and advisory limits;
- currently reserved amount;
- currently committed amount;
- peak live amount;
- cumulative admitted/consumed work where applicable; and
- owner/transaction records needed by the verifier.

Clarify monotonicity:

- cumulative work and peak counters are monotonic;
- precision bounds are not resource counters and remain Component 03's monotonic responsibility;
- live reserved and committed balances may decrease only through documented release, lease destruction, promotion remainder, or rollback.

All arithmetic is checked. Reserve before allocation. Actual use must not exceed reservation. Advisory crossings emit bounded diagnostics but do not alter semantics unless a frozen policy explicitly makes the threshold hard.

### 8.3 Reservations, leases, and slices

Implement:

- move-only `resource_reservation` for private temporary capacity;
- persistent `resource_lease` promoted at commit;
- deterministic `resource_slice` for preassigned parallel ranges; and
- immutable `resource_snapshot` for reports and verification.

Parallel stages reserve aggregate capacity or deterministic slices before workers launch. Racing worker reservations must not select which semantic records are retained.

### 8.4 Conservative defaults

Retain reviewed explicit defaults from the existing plan, but add separate source-snapshot counts/bytes or charge them unambiguously to source entities plus persistent bytes. Defaults are limits, never eager allocations. Callers may lower consistent limits. Raising a limit does not guarantee host allocation.

Changing a default that affects canonical options/replay requires a resource schema or compatible-limit revision and golden update.

### 8.5 Allocation helper

Provide one small reviewed family of reserved-allocation helpers. Each accepts reservation evidence and stable allocation-purpose ID, executes the standard-container allocation, translates expected exceptions, verifies actual capacity/use, and returns a typed outcome.

Use deterministic test providers to fail every allocation purpose without exhausting real memory. Do not scatter ad hoc `try/catch` classification through later components.

## 9. Truth table and symbolic policy

### 9.1 Truth service

Use the four-bit side tuple:

```cpp
struct side_occupancy {
    bool a_negative;
    bool b_negative;
    bool a_positive;
    bool b_positive;
};
```

Evaluate each conceptual side by the requested operation:

- union: `a || b`;
- intersection: `a && b`;
- A-B: `a && !b`;
- B-A: `b && !a`;
- xor: `a != b`.

Return result-side bits, retain/discard, preserve/reverse/not-applicable, one-atom multiplicity, remapped operation, and owner-ranking input. Equal result sides discard. Negative occupied/positive empty preserves; the inverse reverses.

Generate all 80 cells deterministically, encode them, store the digest in context, and independently recompute them without producer helpers. Component 10 calls this frozen service once per atom and must not duplicate Boolean logic.

### 9.2 Coincident owner ranking

Rank already-established equivalent boundary candidates by a complete operation-specific key: ability to realize final orientation, frozen operand priority, symbolic feature priority, full canonical source-feature key, directed-use/occurrence discriminator.

V1 priority is A before B for union/intersection/xor/A-B and B before A for B-A, subject to the operation/remapping table. Equal operands retain one canonical surface for union/intersection and none for differences/xor. The service never discovers geometric coincidence.

### 9.3 Symbolic matrix

Define a complete `symbolic_rule_key` over operation, acting operand, relation family, orientation relation, ownership role, half-open endpoint/edge role, and requested transition orientation where applicable.

Each rule stores conceptual offset disposition, feature priority, half-open owner, crossing contribution, contact/coincidence class, coincident owner preference, expected disposition, full tie-key description, operand-exchange transform, and stable explanation code.

Materialize from one reviewed declarative table or pure generator plus explicit exceptions. Verify domain totality, uniqueness, value validity, and involutive operand exchange. Rules affect classification/ownership only and never alter coordinates.

## 10. Strong identities, owners, and canonical publication

### 10.1 Strong IDs

Implement `strong_id<Tag>` with explicit `std::uint64_t ordinal`, same-tag comparison only, and no implicit integer/cross-tag conversion. Define all domains required by the Component 01 specification and later plans.

Implement a distinct `source_position<Kind>` family for caller/replay positions. Source positions are never accepted where a canonical ID is required.

Implement `task_local_id<Tag>` with no conversion to canonical IDs.

### 10.2 Context owner

Separate deterministic `context_digest` from runtime `context_owner_token`. Create one non-serialized owner anchor per successfully frozen context. Every artifact handle carries owner, schema, and range metadata. Owner pointer values never enter ordering, bytes, diagnostics, replay, or output.

Operand IDs are fixed A=0, B=1. They are roles, not allocated from worker timing.

### 10.3 Canonical factory

`canonical_id_factory<Tag,Key>` must:

1. accept owner-validated private records with complete keys/task-local IDs;
2. preflight counts and ID capacity;
3. sort by full total key;
4. detect full-key duplicates and validate declared duplicate semantics;
5. assign dense `[0,n)` ordinals after final ordering;
6. produce immutable task-local/source-position/key reverse maps where required;
7. prove no task-local ID escaped; and
8. publish only through a transaction.

Hashes may accelerate search only with full-key comparison and canonical sort before publication.

## 11. Cancellation, progress, and diagnostics

### 11.1 Checkpoint polling

`poll_cancellation(token, stage, checkpoint, progress)` returns success or a canonical cancellation observation. Progress contains only stable logical facts:

- last completed and current stage;
- completed canonical range/count;
- checkpoint; and
- committed predecessor digests.

No timing-derived percentage, worker ID, thread ID, or request timestamp enters canonical diagnostics.

### 11.2 Finalization

After observation:

1. close admission;
2. wake blocked bounded queues when Component 17 is present;
3. join all execution scopes;
4. restore every qualified floating environment;
5. roll back private state/resources;
6. verify terminal invariants; and
7. return `cancelled` unless steps 2-6 produce a canonical invariant contradiction.

### 11.3 Diagnostic collector

Maintain:

- a mandatory ordinary primary reducer and candidate count;
- a mandatory cancellation observation slot;
- a bounded optional secondary finding buffer; and
- a pre-reserved truncation record.

Exhaustion of optional capacity cannot change the primary result. Finalization sorts complete keys and coalesces only records whose complete canonical content is equal and whose schema permits it.

## 12. Canonical bytes, SHA-256, and replay

### 12.1 Byte codec

Write fixed little-endian bytes manually. Support:

- u8/u16/u32/u64 and signed fixed-width integers where specified;
- bool exactly 0/1;
- exact float/double bits via `memcpy`;
- checked length-prefixed bytes and UTF-8;
- tagged required/optional framed fields;
- checked sequences and nested sections; and
- size preflight before retained allocation.

Never serialize native structs, padding, `size_t`, pointers, capacities, implementation-defined enum layout, RTTI names, map iteration order, or locale-formatted numbers.

### 12.2 SHA-256

Implement a clean-room streaming SHA-256 provider with fixed 32-byte digest and lowercase hex display. Test standard vectors for empty input, `abc`, multi-block messages, and every meaningful chunk split. Add a collision-forcing test lookup provider to prove digests are not semantic equality.

### 12.3 Replay V1 layout

Encode in canonical field order:

1. magic and replay schema;
2. public/context/source/byte/digest/provider versions;
3. V1 scalar/index descriptors;
4. operation;
5. normalized options;
6. normalization report;
7. stable platform and strict-build profile;
8. Component 03 precision-bootstrap record;
9. truth table bytes/digest;
10. symbolic matrix bytes/digest;
11. resource and execution requests;
12. exact immutable source A section in source order;
13. exact immutable source B section in source order;
14. source, input, and context digests;
15. expected status/output digest for regression records; and
16. optional canonical diagnostics.

Do not encode ignored mesh fields or nonsemantic build metadata. Replay exact source order intentionally reflects the original call.

Preflight full retained size. `digest_only` always streams complete canonical input into SHA-256 but may retain only digest after the invocation. `full_on_failure` must reserve promised full bytes before downstream geometry. `full_always` retains from construction. Failure to honor promised retention returns a typed preflight/resource failure before geometry.

Decoder validates versions, lengths, counts, field uniqueness, type profile, resource limits, and digests. It returns a record; it never automatically executes a Boolean.

## 13. Platform qualification and Component 03 handshake

### 13.1 Build profile

Require the strict-build macro and reject `__FAST_MATH__`. Verify effective flags and a link-time contraction/reassociation witness. Keep bounded objects non-LTO by default even when `USE_LTO=ON`; a later supported IPO profile needs a separate version and evidence.

Canonical platform data stores only stable profile/provider versions and semantic capability results. Compiler/library banners, command excerpts, build paths, executable names, host names, project timestamp, and optional dependency versions are separate noncanonical diagnostics.

### 13.2 Runtime guard

A call/execution-scoped guard:

- captures caller `fenv_t`;
- establishes/verifies `FE_TONEAREST` and the frozen exception/contraction assumptions;
- performs signed-zero, adjacent-value, normal/subnormal, conversion, and contraction-sensitive probes;
- exposes immutable qualification evidence; and
- restores the original environment on every exit.

Failure to establish a supported profile returns `unsupported_platform`. Failure to restore during rollback/cancellation is an invariant contradiction because clean rollback cannot be claimed.

### 13.3 Two-phase precision bootstrap

Use this exact boundary:

1. `build_pending_invocation` captures sources and validates everything not requiring scale-aware precision;
2. Component 03's narrow bootstrap capability receives immutable exact source views, declared input precisions, tolerance, and platform record;
3. Component 03 returns a versioned machine-floor/input-precision bootstrap record;
4. `finalize_context` validates ordinary-success eligibility, includes the record in canonical bytes/replay, independently verifies the proposal, and publishes the frozen context; and
5. pending objects never escape to Components 02-17.

Until Component 03 exists, Component 01 tests may use a deterministic conforming test provider. The production public Boolean pipeline remains unavailable rather than assuming a zero floor.

## 14. Frozen context, source bundle, and capability boundaries

### 14.1 Immutable artifacts

Publish:

- `artifact_handle<const immutable_invocation_sources<T,I>>`;
- `artifact_handle<const boolean_context<T,I>>`; and
- an invocation-owned runtime service bundle.

The context links to the exact source bundle by owner-checked handle and digest. It does not duplicate source bytes unnecessarily. Both artifacts remain valid through all joined work and any predecessor retention required by later artifacts.

### 14.2 Context contents

`boolean_context<T,I>` contains normalized operation/options, V1 types, every provider/schema version, truth/symbolic tables, deterministic comparator descriptions, resource ceilings, verification requirements, requested/available execution capability, stable platform/precision records, normalization report, source/input/context/replay digests, owner token, and fixed operand IDs.

Expose const access only. Noncanonical build diagnostics are not members of canonical context content.

### 14.3 Runtime bundle

Keep mutable controlled services separate:

- resource manager;
- cancellation token view;
- diagnostic collector;
- transaction factory;
- canonical ID publication support;
- replay accumulator/finalizer;
- platform guard/execution-scope factory; and
- Component 17 execution service handle when available.

Later components receive narrow owner-checked interfaces, not the entire mutable bundle.

## 15. Transaction implementation

### 15.1 State machine

Use an explicit state machine, for example:

```text
constructed
workspace_open
work_registered
joining
verifying
commit_ready
committed
```

and failure/cancellation/exception transitions through:

```text
rolling_back
rolled_back
```

Illegal transitions are invariant errors. Destruction of a nonterminal transaction performs noexcept best-effort rollback and records test-visible contradiction evidence without publication.

### 15.2 Workspace and execution scopes

Each transaction owns private workspace, reservations, task-local IDs, diagnostics, replay contributions, and execution scopes. Predecessors are immutable owner-validated handles. No mutable private pointer/reference escapes.

Component 01 provides a serial execution scope. The Component 17 service implements the same interface and must register admitted/running/joined state with the transaction. No scope may detach.

### 15.3 Commit protocol

Implement the Component 01 specification's ten-step protocol exactly. In particular:

- cancellation is polled before and immediately before publication;
- all scopes are joined before canonicalization/verifier execution;
- proposed immutable state is built from reserved storage;
- independent verification does not call producer normalization/materialization helpers;
- exact persistent use is reconciled before lease promotion; and
- atomic publication is a no-fail state transition/move of prepared state.

### 15.4 Rollback and exception boundaries

Rollback joins work, restores environments, discards private data/IDs, releases reservations, keeps predecessors intact, finalizes deterministic error/cancellation evidence, verifies terminal resource state, and becomes idempotent/noexcept after exception capture.

Catch expected standard exceptions at the nearest reviewed boundary, then catch unrelated exceptions at the transaction/execution boundary. Do not continue pipeline work after unexpected exception capture.

## 16. Independent verifiers

### 16.1 Invocation-source verifier

Without trusting producer digests/count booleans alone, verify:

- V1 type descriptors and source-contract version;
- operand roles and owner;
- contiguous offsets, monotonic ranges, and final index count;
- exact coordinate/index bit preservation from the still-live public read view during capture tests/commit preparation;
- no borrowed caller storage in the published representation;
- ignored fields are absent from canonical bytes;
- source-position maps are complete and not canonical IDs;
- resource leases match actual storage;
- source section encoding and digest; and
- A/B bundle linkage.

Provide test-only corrupt constructors for offsets, bits, role, owner, digest, ignored-field inclusion, and caller-pointer retention markers.

### 16.2 Context verifier

Independently recheck versions/enums/reserved fields, V1 type support, scalar relationships, source-bundle linkage, platform/strict-build records, truth table by procedural recomputation, symbolic totality/remapping, resource consistency, comparator descriptors, normalization report, execution capability, canonical-field exclusion, replay linkage, owner uniqueness, and cancellation contract.

Do not call producer table or option normalization helpers as the sole evidence.

### 16.3 Identity verifier

Reconstruct full-key order, dense ranges, duplicate policy, owner/domain/operand correctness, source-position/canonical-ID separation, and absence of task-local IDs. Force hash collisions.

### 16.4 Resource verifier

Recompute live reserved/committed totals from reservation/lease records and verify arithmetic, hard limits, peaks, releases, owners, transaction terminal state, actual-versus-reserved use, source snapshot leases, emergency storage, and no leaked reservation after rollback/commit.

### 16.5 Transaction verifier

Check legal state, joined scopes, restored environments, verified proposal, source/predecessor owner compatibility, lease/resource consistency, digest finalization, cancellation override state, exception translation evidence, and exactly-zero-or-one publication.

## 17. Required tests and exact coverage

### 17.1 Source and public-view tests

Cover all four V1 type combinations, empty sources, large but bounded ring tables, exact signed-zero bits, non-finite source bits, invalid indices, repeated indices, malformed rings, and source counts at representability/resource boundaries.

Prove:

- capture does not validate/repair invalid topology;
- later stages remain valid after caller storage is mutated/destroyed through a test-only lifetime seam after capture;
- ignored normals/colours/involved_faces/metadata do not affect bytes;
- exact source-order permutation changes replay/source digest;
- internal traversal permutation over one snapshot does not;
- unsupported scalar/index profiles fail deterministically; and
- concurrent source mutation during capture is detected at a documented boundary without undefined internal access in the test adapter.

### 17.2 Policy/truth/symbolic tests

Cover every operation/remap, every V1 policy/version/enum/reserved field, finite/non-finite/negative/signed-zero policy scalars, precision/tolerance relations, execution compatibility, output metadata flag rejection, all 80 truth cells, complete symbolic domain, equal/touch/coincident cases, half-open ownership, and operand exchange.

### 17.3 Identity tests

Compile-time nonconvertibility; fixed A/B roles; source-position versus canonical-ID nonconvertibility; deterministic publication under insertion/task permutations; every duplicate policy; forced collisions; wrong/stale owner/domain/operand; task-local escape; rollback isolation; dense range and overflow.

### 17.4 Resource/allocation tests

For every resource kind, test limit-1/limit/limit+1, optional zero, checked overflow, large ledger-only reservation, advisory crossing, promotion/release/rollback, deterministic slices, source-copy byte/count boundaries, and u32/u64 public index representability.

Inject every allocation purpose before/after reservation, during source copy, diagnostics, replay, context proposal, verifier, and transaction commit preparation. Require typed expected failure, mandatory primary error, and complete rollback.

### 17.5 Cancellation tests

Inject cancellation before public-view creation, before/after source preflight, during A/B copy ranges, after source publication but before context freeze, around every context/transaction state, during fake serial/parallel work, after ordinary failure, during diagnostics/replay, and immediately before commit.

Require joined work, restored environment, released private reservations, immutable predecessor preservation, deterministic checkpoint/progress, no partial artifact, and cancellation override. Inject join/restoration/rollback contradictions separately and require invariant reporting.

### 17.6 Codec/replay/digest tests

Golden records for float/u32, float/u64, double/u32, double/u64; exact source signed-zero; policy-zero normalization; truncation at every byte for small records; invalid lengths/counts/bools/enums/required fields/trailing bytes; source section digest mismatch; SHA vectors/chunkings; forced collisions; retention limits; and permanent compatibility bytes.

Vary compiler banner, library string, build path, executable name, project timestamp, optional dependency versions, ignored mesh fields, and caller allocation addresses while preserving the semantic profile and vertices/faces. Canonical context/replay bytes remain identical.

### 17.7 Platform/build tests

Test normals, subnormals, signed zero, adjacent values, NaN/Inf option rejection, caller rounding changes and restoration, contraction/reassociation witnesses, strict macro, unsupported profile injection, global GNU fast-math override, LTO isolation, Debug/Release, sanitizers, and dependency-off linking.

### 17.8 Transaction/exception tests

Test doubles succeed; fail before/after reservation; emit competing failures; throw `bad_alloc`, `length_error`, `system_error`, and unrelated exceptions at source allocation, workspace, join, verifier, digest, and publication preparation; cancel before commit; claim active work after join; and return wrong owner/version/range/resources.

Verify exact terminal state, resource balances, error category/subcode, cancellation precedence, emergency-error availability, and immutable publication.

### 17.9 Determinism/property/mutation tests

Vary private traversal, insertion, allocator addresses, fake workers/delays, collisions, diagnostic capacity, noncanonical build metadata, and repeated runs over one frozen source bundle. Context/source artifact bytes, canonical IDs, primary result, and replay remain identical.

Mutate source offsets, coordinate bits, index bits, operand role, owner, source digest, ignored-field inclusion, borrowed-pointer marker, truth cell, symbolic rule, resource totals, cancellation state, strict profile, project timestamp inclusion, context digest, replay bytes, transaction state, and partial publication. Each intended independent verifier rejects deterministically.

## 18. Implementation sequence and gates

1. **Strict target and dependency-free test harness.** Gate: effective flags and compile/link witnesses pass under normal and requested LTO configurations; dependency-off tests link; unrelated Ygor still builds.
2. **Version registry, checked arithmetic, outcomes, public policy/options/cancellation declarations.** Gate: V1 type profiles, enum/version gaps, scalar and index boundaries pass.
3. **Public mesh read view and immutable invocation-source capture.** Gate: exact bits/rings, ignored-field invariance, no borrowing, invalid-data preservation, source-order semantics, resource/cancellation tests pass.
4. **Canonical bytes and SHA-256.** Gate: primitive goldens, standard vectors, truncation/overflow, collision tests pass.
5. **Errors, summaries, emergency record, arbitration, exception taxonomy.** Gate: exhaustive/random failure permutations and allocation fallback pass.
6. **Strong IDs, source-position types, owners, canonical factories, verifier.** Gate: nonconvertibility, permutation, collision, owner/domain, task-local escape pass.
7. **Resource manager, reservations, leases, slices, allocation helper, verifier.** Gate: every kind's boundary, live-release accounting, source snapshot leases, injected host failures pass.
8. **Cancellation, canonical progress, diagnostics.** Gate: checkpoints, capacity permutations, failure-before-cancel override, rollback contradictions pass.
9. **Truth table and symbolic matrix.** Gate: all 80 cells, complete symbolic domain, equal/contact/remap pass.
10. **Platform/build/runtime qualification.** Gate: strict compile/link, LTO isolation, bits, rounding restoration, subnormal and contraction profiles pass.
11. **Replay schema, source embedding, retention, goldens.** Gate: four-type matrix, exact source replay, ignored-field/build-metadata invariance pass.
12. **Pending/frozen context and Component 03 handshake.** Gate: valid/invalid/mutation tests pass; pending context and caller references cannot escape.
13. **Runtime capabilities, transactions, execution scopes, commit/rollback, verifiers.** Gate: success, expected resource exception, unexpected exception, cancellation, wrong-owner/resource tests pass.
14. **Full property/adversarial/replay/platform/sanitizer/type/dependency/LTO matrix and installed-header check.** Gate: Section 19 complete.

Do not begin production Component 02 integration until the source bundle and Component 03 handshake interfaces are stable and independently verified.

## 19. Definition of done

Component 01 planning and implementation are complete only when:

- the public invocation copies exact vertices/faces into immutable call-owned A/B snapshots;
- no later component borrows caller mesh storage;
- ignored normals, colours, `involved_faces`, and arbitrary metadata cannot affect canonical behavior;
- float/double with u32/u64 are the only claimed V1 profiles and all four are qualified;
- source positions are strongly separated from canonical topology IDs;
- exact source-order replay semantics are documented separately from later permutation-invariant artifacts;
- public operation/options/error/cancellation contracts are versioned and tested;
- V1 output metadata preservation is unambiguously disabled;
- all required identity domains are strong and owner-bound;
- task-local IDs cannot escape and IDs never derive from coordinates, hashes alone, pointers, timing, or insertion order;
- all 80 truth cells and operand remaps are independently verified;
- the complete symbolic matrix is total, immutable, versioned, replayed, and independently verified;
- all scalar/resource/execution/diagnostic validation and normalization behaves exactly as specified;
- strict compile/link behavior is proven despite parent fast-math and requested LTO;
- normative tests link with optional dependencies disabled and no downloaded framework;
- caller/platform floating environment is qualified and restored;
- stable canonical profile data is separated from nonsemantic build diagnostics;
- Component 03's two-phase floor handshake exists and pending contexts cannot escape;
- typed errors contain stable category/subcode/stage/entity/source-position/numeric/policy/replay evidence;
- expected host allocation/container/worker failures remain typed and reservation contradictions are invariants;
- a fixed/pre-reserved primary-error path survives optional allocation failure;
- ordinary primary error bytes are invariant under discovery order, scheduling, collisions, and optional capacity;
- valid cancellation before publication overrides earlier ordinary failures after safe rollback;
- every transaction publishes one independently verified immutable artifact or nothing;
- live resource release/rollback semantics and cumulative/peak monotonic counters are correctly distinguished and verified;
- canonical/replay bytes are fixed-endian, bounded, versioned, golden-tested, exact-source capable, and build-instance independent;
- SHA-256 passes standard vectors and is never authoritative equality;
- unsuitable legacy serializers, hashes, queues, mesh mutators, and Boolean implementations are not used;
- unit, property, adversarial, mutation, replay, platform, transaction, resource-failure, cancellation, sanitizer, LTO, dependency-off, and four-type-matrix tests pass; and
- Components 02-17 require only the documented immutable source/context artifacts and narrow controlled capabilities.
