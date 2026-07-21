# Plan 01: Contract, Context, Identities, Errors, Transactions, and Resources

## 0. Scope and non-negotiable constraints

Implement **only Component 01** from `component_01_context_contracts.md`. This component establishes the immutable Boolean contract and the controlled runtime services consumed by Components 02-17. It must not validate mesh topology, establish shell nesting, evaluate geometric predicates, triangulate, enumerate collisions, construct intersections, classify winding, build output topology, clean geometry, or perform final geometric verification.

The implementation must satisfy these rules from the first commit:

- portable C++17 and the C++ standard library only in production and normative tests;
- no new external, vendored, downloaded, runtime-invoked, or optional dependency;
- do not use `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- no process-global mutable Boolean configuration, counters, registries, allocators, or cancellation state;
- no topology or identity inference from coordinates, approximate equality, hashes alone, pointer values, allocation order, thread order, wall-clock time, locale, random-device output, build paths, invocation timestamps, or Ygor's time-derived project version;
- all expected failures are typed values, not log-only outcomes or exceptions;
- expected allocation, container-capacity, and operating-system resource failures are translated to typed expected failures at documented boundaries;
- unexpected exceptions are caught at transaction/execution boundaries, all owned work is joined, and one deterministic `internal_invariant_error` is returned;
- cancellation observed before publication returns `cancelled` after complete rollback unless joining, floating-environment restoration, or rollback itself fails;
- no failed, cancelled, partially verified, or partially committed stage can publish an artifact;
- all public/inter-component schemas and serialized numeric values are explicitly versioned;
- every new Boolean source and normative test is compiled outside the repository's current global `-ffast-math` contract, including under any requested LTO/IPO mode; and
- normative Boolean tests do not link an external test framework or require Ygor's optional external dependencies.

Mark Component 01 complete in `tracker.md` only after Section 21 is fully satisfied.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Reuse unchanged

Reuse `vec3<T>` and `fv_surface_mesh<T,I>` from `YgorMath.h` only as public data carriers and immutable source descriptions. Read `vertices`, `faces`, and recognized Boolean metadata without mutation. Preserve coordinate bit patterns, including signed zero. Do not use `vec3::operator<` or coordinate sorting for canonical identity.

Reuse standard-library facilities such as `<array>`, `<atomic>`, `<cfenv>`, `<condition_variable>`, `<cstring>`, `<memory>`, `<mutex>`, `<new>`, `<stdexcept>`, `<string>`, `<system_error>`, `<thread>`, `<type_traits>`, `<variant>`, and `<vector>` when used under the contracts below.

### 1.2 Reuse only through a new adapter

Ygor logging may display a completed public error at the eventual API boundary, but Component 01 core code must not use `YLOG*` as control flow or emit schedule-dependent diagnostics. Preserve Ygor's installed-header convention and CMake compiler detection, but isolate the new subsystem in a strict object target.

### 1.3 Do not reuse

- Do not call `fv_surface_mesh` mutators such as `merge_duplicate_vertices`, `convert_to_triangles`, or `remove_degenerate_faces`.
- Do not use `YgorSerialize`; it is not a fixed-endian, schema-stable canonical codec.
- Do not use `External/SpookyHash`, `External/MD5`, or `std::hash` as authoritative equality or canonical digest.
- Do not use `work_queue` from `YgorThreadPool.h`; its unbounded queue, swallowed exceptions, and lack of deterministic merge/resource/cancellation semantics violate the contract.
- Do not use `taskqueue` from `YgorContainers.h` as an execution or cancellation provider.
- Do not use `YgorEnvironment` for floating-point qualification; it exposes unrelated platform services rather than a strict floating-point contract. Implement a portable `<cfenv>` provider.
- Do not use `tests2/compile_and_run.sh` or downloaded doctest for normative tests. Add a dependency-free CTest harness.

### 1.4 Required build correction

The root CMake currently appends GNU `-ffast-math` to `CMAKE_CXX_FLAGS`, enables optional project-wide interprocedural optimization through `USE_LTO`, and derives `PROJECT_VERSION` from an invocation timestamp. The `ygor` target can also link optional GSL support when configured.

Do not change unrelated Ygor compilation in this component. Add a dedicated strict object library whose strict floating-point flags occur after inherited global flags. Disable IPO on the strict object and normative-test targets unless a reviewed compiler-specific configuration proves that the strict translation-unit semantics survive link-time optimization. Prove through compile commands, compile-time guards, runtime witnesses, and an LTO configuration test that the Boolean translation units are strict.

Never place `${PROJECT_VERSION}`, invocation timestamps, compiler banners, build directories, executable names, or optional dependency versions in canonical context/replay/error bytes. Retain them only in explicitly non-authoritative diagnostics.

Normative Boolean test executables must link the strict object code and only the standard-library/toolchain facilities they need, such as `Threads::Threads`; they must not acquire GSL, Eigen, a downloaded test framework, or other optional libraries through the general `ygor` link interface. Add a configuration test with `WITH_GNU_GSL=OFF` and `WITH_EIGEN=OFF`.

## 2. Exact file and target layout

### 2.1 Installed public header

Create `src/YgorMeshesBooleanBounded.h` containing:

- stable public operation and policy types;
- `bounded_boolean_options<T>`;
- stable public error category and read-only public error view;
- public `bounded_boolean_cancellation_source` and immutable `bounded_boolean_cancellation_token` declarations and minimal value-semantics interface;
- public digest/replay identifiers required by later result types;
- forward declarations for `bounded_boolean_result<T,I>`;
- the ordinary `bounded_boolean(a,b,operation,options)` overload; and
- an explicit cancellation-aware overload taking a `const bounded_boolean_cancellation_token&` after `options`.

The ordinary overload delegates to the cancellation-aware implementation using a never-cancelled token. Do not put a mutable cancellation source inside `bounded_boolean_options<T>`. The public header contains no mutable runtime-service implementation and no implementation-only includes.

### 2.2 Internal implementation directory

Create `src/YgorMeshesBooleanBounded/` with:

- `CMakeLists.txt` — strict object target, strict-FP helper, IPO isolation, and dependency-free test-link helper;
- `ContractVersions.h` — all schema/provider versions and stable numeric assignments;
- `CheckedArithmetic.h` — checked add/subtract/multiply/count-to-byte/range/narrowing helpers;
- `Outcome.h` — C++17 typed success/error carrier;
- `CanonicalBytes.h/.cc` — fixed-endian writer/reader, framing, exact scalar bits, size preflight;
- `Sha256.h/.cc` — clean-room in-tree SHA-256 used as deterministic digest/integrity key;
- `PlatformQualification.h/.cc` — type, build, IPO, and runtime floating-environment qualification;
- `Policies.h/.cc` — option normalization, truth table, operand remapping, symbolic-policy materialization;
- `Identity.h/.cc` — strongly typed IDs, owner tokens, canonical publication factories;
- `Errors.h/.cc` — internal error records, subcodes, summaries, arbitration keys, and fixed-capacity emergency error construction;
- `Resources.h/.cc` — resource kinds, ledgers, reservations, leases, slices, snapshots, allocation-failure translation, verifier;
- `Cancellation.h/.cc` — public source/token backing state, checkpoint IDs, canonical progress;
- `Diagnostics.h/.cc` — bounded findings and deterministic primary-error reduction;
- `Replay.h/.cc` — replay schema, encoder/decoder, digest and retention;
- `Context.h/.cc` — pending/frozen context, builder, views, runtime service bundle;
- `Transaction.h/.cc` — typed transaction state machine, exception taxonomy, and immutable artifact handle;
- `ContextVerifier.h/.cc` — independent context/service verification.

Keep templates header-defined or explicitly instantiate only supported type combinations. No geometric algorithms belong here.

### 2.3 Tests

Create `tests/mesh_boolean_bounded/` with `CMakeLists.txt`, `TestSupport.h/.cc`, `TestMain.cc`, `TestContextUnit.cc`, `TestContextProperties.cc`, `TestContextAdversarial.cc`, `TestContextPlatform.cc`, `TestContextReplay.cc`, `TestContextTransactions.cc`, `TestContextResources.cc`, `TestContextCancellation.cc`, `TestContextBuildIsolation.cc`, and `GoldenReplayV1.h`.

The harness must be a small local registration/assertion runner with a fixed deterministic PRNG. Register separate CTest cases for unit, property, adversarial, platform, replay, transaction, resource-failure, cancellation, and build-isolation suites.

### 2.4 CMake integration

1. Add the Boolean subdirectory in `src/CMakeLists.txt` and add `$<TARGET_OBJECTS:ygor_mesh_boolean_bounded>` to `ygor`; the existing top-level `*cc` glob must not compile subdirectory files twice.
2. Define `ygor_apply_mesh_boolean_strict_fp(target)` and apply it to the object target and every normative Boolean test.
3. Append:
   - GNU: `-fno-fast-math -frounding-math -fno-associative-math -fno-reciprocal-math -fsigned-zeros -fno-finite-math-only -ffp-contract=off`;
   - Clang: `-fno-fast-math -frounding-math -ffp-contract=off`, plus supported signed-zero/reassociation-disabling options detected with CMake;
   - MSVC: `/fp:strict`;
   - unknown compilers: build only if a reviewed strict option set and conformance fixture exists; otherwise context qualification returns `unsupported_platform`.
4. Set `INTERPROCEDURAL_OPTIMIZATION FALSE` on `ygor_mesh_boolean_bounded` and normative tests by default. If a later supported compiler configuration intentionally enables IPO for these targets, require a separate reviewed profile/version and strict semantic witnesses at both compile and link stages.
5. Define `YGOR_MESH_BOOLEAN_STRICT_FP_BUILD=1` only on strict targets and reject `__FAST_MATH__` in a common internal header.
6. Inspect generated compile commands in a CTest fixture and require the last effective floating options for every bounded Boolean translation unit to be strict. Add a link-time contraction/reassociation witness for `USE_LTO=ON`.
7. Add `include(CTest)` and the new tests when `BUILD_TESTING` is true. Tests may not download or discover an external framework.
8. Link normative tests to the object code or a dedicated internal static test target plus `Threads::Threads`; do not link optional GSL/Eigen merely through `ygor`.
9. Add CI/configuration coverage for `WITH_GNU_GSL=OFF`, `WITH_EIGEN=OFF`, `USE_LTO=OFF`, and the supported `USE_LTO=ON` behavior.

## 3. Stable version and numeric registry

`ContractVersions.h` is the sole authority for serialized versions and enum values. Use explicit fixed-width underlying types and explicit numeric values.

Define V1 constants for public API, cancellation API, context, options, truth table, symbolic policy, identity, errors, resources, replay, canonical bytes, platform contract, strict build profile, transaction contract, and SHA-256 provider. Start each at `1`; reserve `0` for invalid/uninitialized where useful.

Add compile-time uniqueness checks for every serialized enum. Decoders must accept every recognized value and reject gaps/unknown required versions. Never reinterpret an unknown version as the newest version.

Stable conformance-profile IDs are normative. Compiler names/versions, library names/versions, Ygor invocation versions, and build-instance data are not version authorities and must not be serialized into canonical fields.

## 4. Public operation, policies, options, and cancellation

### 4.1 Operations

Use:

```cpp
enum class boolean_operation : std::uint8_t {
    set_union = 1,
    intersection = 2,
    a_minus_b = 3,
    b_minus_a = 4,
    symmetric_difference = 5,
};
```

`swap_operands` maps union/intersection/xor to themselves and swaps the two differences. Invalid values fail decoding/freezing.

### 4.2 Supported V1 policies

Expose only fixed V1 policy kinds, not callbacks or caller-supplied rule tables:

- `solid_policy_kind::outward_oriented_alternating_shells_v1`;
- `contact_policy_kind::regularized_symbolic_v1`;
- `output_policy_kind::triangulated_oriented_manifold_v1`;
- verification levels `required_scalable`, `required_scalable_with_diagnostics`, and `exhaustive_test_only` (test builds only);
- `determinism_mode::canonical_v1`;
- execution modes `serial_v1` and `deterministic_parallel_v1`;
- replay retention `digest_only`, `full_on_failure`, and `full_always`.

Every nested policy begins logically with its schema version, uses fixed-width fields, and requires reserved V1 fields to be zero.

### 4.3 Option layout

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

Required nested fields:

- solid/contact: version and kind;
- output: version, kind, `preserve_public_metadata` (false by default and non-semantic in V1);
- verification: version and level; no `none`;
- determinism: version and mode; canonical only in V1;
- execution: version, mode, requested/max workers, max in-flight tasks, deterministic fallback flag, nested-parallelism flag (false in V1);
- diagnostics: version, max retained findings/bytes, replay retention, retain-secondary flag, internal-topology-only diagnostic flag;
- resources: version and hard/advisory ceiling for every Section 11 resource kind.

Do not silently clamp invalid values. Record every permitted normalization.

### 4.4 Public cancellation API

Implement a caller-owned `bounded_boolean_cancellation_source` with a cheap copyable token:

```cpp
class bounded_boolean_cancellation_token;

class bounded_boolean_cancellation_source {
public:
    bounded_boolean_cancellation_source();
    bounded_boolean_cancellation_token token() const noexcept;
    bool request_cancel(cancellation_reason reason = cancellation_reason::caller_requested) noexcept;
};
```

The exact public names may follow local style, but the semantics are fixed:

- source and token share an internal state whose lifetime covers the call and all joined work;
- `request_cancel` is idempotent and records only the first stable reason code;
- tokens are immutable read-only views and safe for concurrent polling;
- no callback registration, detached waiter, signal handler, global registry, pointer-derived ordering, or timestamp is permitted;
- the no-token `bounded_boolean` overload delegates using a canonical never-cancelled token;
- cancellation request time is non-authoritative; the observed checkpoint and canonical progress are authoritative;
- cancellation source/token identity is not serialized or hashed.

## 5. Deterministic option and source-description normalization

Implement a pure normalizer returning `normalized_boolean_options<T>` or `boolean_error`, never mutating caller data.

### 5.1 Fixed validation phases

Use this order:

1. type descriptors;
2. policy versions/enums/reserved fields;
3. finite scalar checks;
4. scalar sign/relationship checks;
5. resource consistency and checked arithmetic;
6. index compatibility;
7. determinism/execution compatibility;
8. diagnostics/replay capacity;
9. build/runtime platform qualification;
10. Component 03 precision-floor qualification;
11. truth/symbolic table materialization;
12. canonical input/replay encoding and final context verification.

Within a phase, collect bounded candidate errors and choose the canonical minimum; do not let incidental field traversal determine the result.

### 5.2 Scalar rules

- Require finite, non-negative tolerance and input precisions.
- Accept policy scalar negative zero, normalize it to positive zero, and record the exact old/new bits; never normalize coordinate signed zero.
- For ordinary-success-capable modes require each input precision `<= tolerance`.
- Permit larger input precision only for an explicitly requested internal topology-only diagnostic attempt, and mark ordinary publication impossible.
- After Component 03 supplies the scale-aware machine floor, require tolerance at least that floor for ordinary success.
- Component 01 never uses these numbers as general equality epsilons.

### 5.3 Execution rules

`requested_workers == 0` remains a frozen automatic-selection request; Component 01 must not replace it with `hardware_concurrency()`. Maximum workers is nonzero. Serial mode requires one effective worker and one in-flight task. Nested parallelism is false in V1. Deterministic parallel policy may be frozen before Component 17 exists, but provider capability must state that only serial execution is currently realizable; this is not silent mutation.

### 5.4 Resource/index rules

Advisory zero means disabled. Hard zero is valid only for optional resources. Nonzero advisory values are `<=` hard values. Mandatory context/error/truncation/replay-header resources are nonzero. All relationships use checked arithmetic. `I` must be unsigned integral, not bool, and 8/16/32/64 bits; an output count is representable only when zero or `count - 1 <= max(I)`. Encoded sizes must fit both `uint64_t` and `size_t`.

### 5.5 Normalization report

Store canonical entries containing field ID, old exact bits/value, new exact bits/value, and stable reason code. V1 entries are limited to permitted scalar negative-zero normalization and explicit worker-selection syntax. Include this report in replay and the context digest.

## 6. Typed outcomes, errors, and exception taxonomy

### 6.1 Outcome carrier

Implement non-default-constructible `boolean_outcome<T>` over `std::variant<T,boolean_error>` with `success`/`failure` factories and a `void` specialization. Expected access paths do not throw; misuse may assert in debug. Construction/destruction never logs.

Use outcome payloads that are nothrow-destructible. Prefer immutable handles and fixed-capacity/fallible builders so moving a completed outcome cannot introduce a new expected allocation failure.

### 6.2 Stable categories

Give explicit values to all required categories: `input_contract_error`, `input_geometry_not_epsilon_valid`, `unsupported_platform`, `invalid_tolerance`, `ambiguous_shell_semantics`, `geometric_condition_exceeds_tolerance`, `cleanup_budget_exceeded`, `result_geometry_not_validated`, `index_overflow`, `resource_limit`, `cancelled`, and `internal_invariant_error`. `none = 0` is invalid in published errors.

### 6.3 Subcodes

Reserve disjoint numeric ranges by component/service. Component 01 must distinguish unknown version/enum, non-finite/negative option, precision exceeding tolerance, tolerance below floor, unsupported type/build/rounding/signed-zero/subnormal/contraction/IPO profile, resource inconsistency/overflow/hard limit, host allocation failure, container capacity failure, worker creation failure, emergency-error fallback, index capacity, wrong/stale owner, wrong ID domain, duplicate canonical key, task-local escape, illegal transaction transition, active work at commit, verifier rejection, codec truncation/overflow/unknown required field, digest mismatch, replay mismatch, cancellation checkpoint, cancellation rollback failure, and unexpected exception.

### 6.4 Internal error record

`boolean_error` contains schema version, category/subcode, component, stage/checkpoint, context digest when available, optional operand, up to two canonical entity references, exact tolerance/precision/remaining-budget bits where applicable, sorted typed diagnostic fields, stable locale-independent summary, provider/policy versions, replay digest/disposition, and a precomputed total-order key.

Diagnostic values are canonical variants of bool, fixed-width integers, float/double bits, digest, entity reference, bounded bytes, or bounded stable UTF-8. Sort by numeric field ID and reject duplicate singleton fields. Never place pointers, thread IDs, timestamps, allocator addresses, locale text, build paths, project timestamps, or authoritative `exception::what()` in canonical data.

The mandatory canonical fields and summary template must fit a pre-reserved or fixed-capacity emergency record. Optional strings/fields may be dropped deterministically when their reservation is unavailable; the primary category/subcode/key may not be dropped.

### 6.5 Expected exception translation

Translate at the nearest boundary with complete stage/resource evidence:

- `std::bad_alloc` during a correctly reserved allocation -> `resource_limit::host_allocation_failed`;
- `std::length_error` or container `max_size()` rejection despite valid checked counts -> `resource_limit::container_capacity_failed`;
- `std::system_error` from worker/thread creation in Component 17 -> `resource_limit::worker_creation_failed` or a separately versioned execution-capability failure agreed with Component 17;
- codec length/offset/count exceptions must not be used; return typed codec failures directly;
- producer allocation beyond its reservation, allocation after commit-ready, or a supposedly impossible `length_error` caused by missed checked arithmetic -> `internal_invariant_error`.

Catch by exact standard exception type before the generic `std::exception` boundary. Never use `what()` to choose a canonical category or subcode.

### 6.6 Primary-error ordering and cancellation override

For ordinary failure candidates use, in order: logical stage, checkpoint, category precedence, component, operand rank, primary entity domain/ordinal, secondary entity domain/ordinal, subcode, canonical witness bytes.

V1 ordinary category precedence within one stage/checkpoint is:

1. internal invariant;
2. input contract;
3. unsupported platform;
4. invalid tolerance;
5. ambiguous shell semantics;
6. input geometry invalid;
7. geometric condition;
8. cleanup budget;
9. result geometry invalid;
10. index overflow;
11. resource limit.

`cancelled` is deliberately not in the ordinary category precedence. Once a valid cancellation observation occurs before publication, return the canonical cancellation error after joining and rollback. Previously discovered ordinary failures remain secondary findings only. If joining, FP restoration, resource release, or rollback contradicts the contract, return the canonical invariant failure instead of `cancelled` and retain the cancellation observation as evidence.

This is reporting order, not severity. Secondary findings are optional and bounded; the primary failure or cancellation record is mandatory.

## 7. Pipeline-stage and checkpoint registry

Define stable stage IDs now for context preflight, platform/precision bootstrap, operand A/B validation, source triangulation A/B, canonical topology A/B, broad phase, relations, events, classification, selection, output edge/cycle construction, output triangulation, cleanup, public assembly, final verification, and publication.

Define Component 01 checkpoint IDs for public entry/cancellation-token validation, context phases, transactions, resource reservations, host allocation, diagnostics, replay, verification, join, rollback, floating-environment restoration, and commit. Later components extend reserved numeric ranges without renumbering existing IDs. Component 16/17 failures map to the logical stage whose artifact is affected.

## 8. Operation truth-table service

### 8.1 Exact interface and semantics

The immutable service accepts an operation and four open-side occupancy bits:

```cpp
struct side_occupancy {
    bool a_negative;
    bool b_negative;
    bool a_positive;
    bool b_positive;
};
```

Outward-oriented occupied boundaries have occupied material on the negative side. Evaluate each side with:

- union: `a || b`;
- intersection: `a && b`;
- A-B: `a && !b`;
- B-A: `b && !a`;
- xor: `a != b`.

Return retain/discard, `preserve`/`reverse`/`not_applicable`, required multiplicity (V1 0 or 1 for one atom; Component 10 owns grouping into repeated occurrences), and output occupancy on both sides. Equal side occupancy discards. Negative occupied/positive empty preserves; the reverse transition reverses. Invalid wider classification states return a typed failure.

Generate all 80 V1 entries (`5 * 16`) deterministically, store version/table bytes in context, and independently recompute them in the verifier.

### 8.2 Coincident owner ranking

Rank already-established equivalent boundary candidates by: ability to realize final orientation; operation-specific operand priority; symbolic feature-dimension priority; full canonical source-feature key; directed use/occurrence discriminator.

V1 priority is A before B for union/intersection/xor/A-B and B before A for B-A. Operand exchange plus `swap_operands` must exchange ranks. Equal operands retain one canonical surface for union/intersection and none for either difference/xor. This service never discovers coincidence or compares coordinates.

## 9. Frozen symbolic contact-policy matrix

Component 01 owns the total key/value representation, versioning, validation, operand remapping, and replay. Component 07 later applies it geometrically.

### 9.1 Key domain

A `symbolic_rule_key` includes operation, acting operand, relation family (vertex-vertex, vertex-edge, vertex-face, edge-edge, edge-face, equal-edge, coplanar face-face, coincident face-face, tangent), orientation relation, feature ownership role, half-open endpoint/edge role, and requested transition orientation where applicable.

Every valid key has exactly one rule. Invalid combinations are explicitly rejected; lookup has no default fallthrough.

### 9.2 Rule values

Each rule contains conceptual offset disposition for A/B (`none`, toward occupied, toward empty, ordered rank), feature priority, half-open owner selector, conceptual crossing `-1/0/+1`, contact/crossing/coincidence class, coincident owner preference, expected retain/discard consequence, complete tie-key order, operand-exchanged key/value transform, and stable explanatory code. Rules affect classification/ownership only, never nominal coordinates.

### 9.3 Materialization and verification

Materialize V1 from one reviewed declarative table or pure generator plus explicit exceptions. Enumerate the complete domain during context construction and prove totality, no duplicates, valid values, and involutive operand exchange. Store canonical matrix bytes and SHA-256 digest in context/replay. Test every operation for equal operands, point/edge/face contact, same/opposite coincident facets, and every half-open owner category. Do not implement geometric predicates here.

## 10. Strong identities and ownership

### 10.1 Strong types

Implement `strong_id<Tag>` with explicit `uint64_t ordinal`, no implicit integer or cross-tag conversion, same-tag comparisons only. Define distinct tags for context, operand, source shell/vertex/facet/ring/directed edge/undirected edge/triangle, internal vertex occurrence/halfedge, candidate, relation, symbolic decision, event, classification group, retained surface use, output vertex occurrence/carrier/halfedge/face cycle/triangle, cleanup action, verification finding, and replay record.

Define `task_local_id<Tag>` with no conversion to canonical IDs. Internal hash acceleration may exist, but equality compares complete typed IDs/keys.

### 10.2 Context owner

Separate deterministic `context_digest` from runtime `context_owner_token`. Create one non-serialized `shared_ptr<const owner_anchor>` per successfully built context. Every artifact handle carries that owner token and schema/range metadata. Cross-component validation checks owner equality, versions, then ID ranges. Owner pointer values never enter ordering, diagnostics, digest, replay, or output.

Operand IDs are fixed: A=0, B=1. No allocator exists for them.

### 10.3 Canonical publication factory

`canonical_id_factory<Tag,Key>` must:

1. accept private records with complete keys/task-local IDs;
2. validate owner/domain/schema/count;
3. sort by a complete total key;
4. detect duplicate full keys;
5. apply declared duplicate semantics: impossible, equivalent/coalescible, explicit multiset occurrence, or deterministic proposal selection;
6. assign dense `[0,n)` ordinals only after order is final;
7. produce immutable local-to-canonical and key-to-ID maps;
8. verify no task-local ID remains; and
9. publish only through the stage transaction.

Never expose a process-global next-ID counter. Test duplicate/out-of-range/wrong-owner/wrong-domain/wrong-operand/task-local escape/hash collision/rollback reuse mutations.

## 11. Resource accounting and physical allocation

### 11.1 Stable resource kinds

Define explicit values for source vertices/facets/rings/directed edges/undirected edges/triangles, internal occurrences/halfedges, broad-phase nodes/candidates, relations, symbolic decisions, events, classification groups, retained uses, output occurrences/carriers/halfedges/cycles/triangles, cleanup actions, verification findings, diagnostic findings, replay bytes, canonical-sort records, task descriptors, temporary bytes, persistent bytes, emergency error storage, and abstract work units.

Hard correctness limits are distinct from advisory thresholds. Advisory crossings can report diagnostics but cannot change topology unless the frozen policy explicitly makes them hard.

### 11.2 Checked arithmetic

Provide portable checked unsigned add/subtract/multiply/count-to-byte/range-end and narrowing to `size_t`/`I`. Return typed failure before mutation/allocation. Do not rely on wraparound, saturation, container exceptions, or host-memory exhaustion as validation.

### 11.3 Manager, reservations, leases, slices

A context-owned thread-safe `resource_manager` stores hard/advisory/committed/reserved/peak/cumulative-work values per kind. Observations are immutable snapshots.

Implement move-only RAII `resource_reservation`, persistent `resource_lease`, and deterministic preallocated `resource_slice`.

Rules:

- reserve before allocation;
- checked `committed + reserved + request <= hard`;
- failure records kind/request/current/limit/stage/entity;
- destroying an unpromoted reservation releases it;
- commit promotes exact persistent use and releases temporary remainder;
- rollback releases all private reservations but not predecessor leases;
- actual use never exceeds reservation;
- advisory crossings only emit bounded findings;
- representability failures are `index_overflow`;
- parallel stages reserve aggregate capacity or canonical slices before launch; racing worker reservations may not select semantic winners;
- successful logical reservation does not guarantee host allocation;
- host allocation/capacity failure consumes no semantic winner and causes complete rollback;
- producer use beyond reservation is an invariant error, not a resource-limit excuse.

An independent snapshot verifier reconstructs totals from live records, checks limits/peaks/owners/transaction terminal state, and has test-only corrupt constructors.

### 11.4 Exact conservative V1 defaults

Defaults are accounting ceilings, never eager allocations:

| Resource | Hard | Advisory |
|---|---:|---:|
| source vertices/facets/rings | 10,000,000 each | 7,500,000 each |
| source directed/undirected edges | 60,000,000 / 30,000,000 | 45,000,000 / 22,500,000 |
| source triangles | 40,000,000 | 30,000,000 |
| internal occurrences/halfedges | 40,000,000 / 120,000,000 | 30,000,000 / 90,000,000 |
| broad nodes/candidates | 40,000,000 / 200,000,000 | 30,000,000 / 150,000,000 |
| relations/symbolic decisions | 200,000,000 each | 150,000,000 each |
| events | 100,000,000 | 75,000,000 |
| classification groups | 40,000,000 | 30,000,000 |
| retained uses | 80,000,000 | 60,000,000 |
| output occurrences/carriers/cycles/triangles | 80,000,000 each | 60,000,000 each |
| output halfedges | 240,000,000 | 180,000,000 |
| cleanup actions | 20,000,000 | 15,000,000 |
| verification findings | 1,000,000 | 750,000 |
| retained diagnostic findings | 4,096 | 3,072 |
| replay bytes | 536,870,912 (512 MiB) | 402,653,184 (384 MiB) |
| canonical-sort records | 200,000,000 | 150,000,000 |
| task descriptors | 1,048,576 | 786,432 |
| temporary bytes | 2,147,483,648 (2 GiB) | 1,610,612,736 (1.5 GiB) |
| persistent bytes | 2,147,483,648 (2 GiB) | 1,610,612,736 (1.5 GiB) |
| work units | 4,398,046,511,104 (`2^42`) | 3,298,534,883,328 (`3*2^40`) |

Default retained diagnostic bytes are 16,777,216 (16 MiB), charged to temporary and then persistent bytes when published. Reserve a mandatory primary-error slot, truncation slot, replay header, fixed service state, and fixed-capacity emergency error representation even when ordinary finding capacity is zero. Limits are absolute, not based on detected RAM/workers. Callers may lower consistent values; raising a value never guarantees allocation. Golden default-policy bytes detect accidental changes; changing defaults requires a resource schema increment or documented compatible-limit revision.

### 11.5 Allocation boundary helper

Provide one reviewed helper or small family of helpers for reserved standard-container allocation. It accepts reservation evidence and a stable allocation purpose ID, invokes the allocation, translates expected standard exceptions, validates actual capacity/use against the reservation, and returns a typed outcome. Do not scatter ad hoc `try/catch` blocks whose subcodes or witnesses differ by component.

Test providers must deterministically inject failure at each allocation purpose without consuming host memory.

## 12. Cancellation and canonical progress

Implement public `bounded_boolean_cancellation_source` over shared state, copyable read-only `bounded_boolean_cancellation_token`, first-reason capture, atomic request flag, and no callbacks/detached waiter.

`poll_cancellation(token,stage,checkpoint,progress)` returns success or a canonical cancellation observation. The authoritative cancellation location is the checkpoint where observed, not request time. After observation: no new commit-producing work, active scopes stop at safe points, all work joins, private state rolls back, predecessor artifacts remain valid.

Cancellation finalization is separate from ordinary failure reduction:

1. record the canonical observed checkpoint/progress;
2. stop admission and join all work;
3. restore every qualified floating environment;
4. roll back resources/workspace;
5. verify terminal invariants;
6. return `cancelled` and retain earlier failures only as bounded secondary findings.

If steps 2-5 fail, return the deterministic invariant failure and retain cancellation as evidence.

Progress contains only last completed/current stage, completed canonical range/count, checkpoint, and committed artifact digests. No timing/thread-derived percentages enter canonical diagnostics.

## 13. Diagnostics

Maintain:

1. a mandatory reducer that always retains the canonical minimum ordinary failure and checked candidate count;
2. a separate mandatory cancellation observation slot; and
3. an optional bounded finding buffer.

Exhausting optional capacity never changes the primary ordinary failure or cancellation outcome. Pre-reserve one truncation record and report dropped counts by kind. Concurrent submission uses a mutex or proven equivalent. Finalization sorts by complete key, coalesces only fully equal records where allowed, and reports contradictions as internal errors. Summaries use stable templates and locale-independent integer/hex formatting. `exception::what()` may appear only in non-authoritative debug data.

When cancellation is validly finalized, its record becomes the public result regardless of the ordinary reducer's current minimum. When cancellation cleanup is invalid, the ordinary reducer receives the canonical invariant contradiction.

## 14. Canonical bytes, SHA-256, and replay

### 14.1 Codec

Write fixed little-endian bytes manually. Never serialize object memory, padding, `size_t`, pointers, capacity, implicit enum layout, build paths, invocation timestamps, or compiler-generated names. Support fixed-width integers, bool exactly 0/1, exact float/double bits via `memcpy`, checked length-prefixed bytes/UTF-8, tagged required/optional framed fields, checked sequences, and nested sections.

Reader failures include truncation, duplicate singleton fields, invalid bool/enum, length/count overflow, trailing bytes, and unknown required fields. Unknown optional fields may be skipped within limits. No allocation occurs from untrusted lengths before hard/`size_t` checks. Errors include byte offset and field tag.

### 14.2 SHA-256

Implement clean-room streaming SHA-256 with fixed 32-byte digest and lowercase hex display. Test empty, `abc`, multi-block, and chunked updates. Hashes accelerate integrity/lookup only; full keys/bytes determine equality. Add a collision-forcing test provider.

### 14.3 Replay V1

Canonical field order:

1. magic/schema;
2. public/context/byte/digest/provider versions;
3. scalar/index descriptors;
4. operation;
5. normalized options;
6. normalization report;
7. stable platform/strict-build conformance profile and capability bits;
8. precision-floor record;
9. truth table bytes/digest;
10. symbolic matrix bytes/digest;
11. resources;
12. exact operand A description;
13. exact operand B description;
14. input/context digest;
15. expected status/output digest for regression records;
16. optional canonical diagnostics.

Each operand encodes role, type descriptor, vertex count and xyz bits in source order, facet count, ring lengths and indices widened to u64, input precision bits, and only versioned Boolean semantics metadata. Arbitrary mesh metadata is non-semantic in V1.

The canonical platform section stores only stable contract/provider versions, scalar capability facts, strict FP profile, rounding/contraction/subnormal results, and other semantic qualification bits. Compiler/library banners, Ygor project timestamp, build directory, executable path, host name, and optional dependency versions belong in a separate noncanonical debug record that is not hashed into the context or replay digest.

Preflight exact encoded bounds and reserve replay bytes. Always stream bytes into SHA-256. `digest_only` retains digest; `full_on_failure` must pre-reserve full capacity before geometry; `full_always` retains bytes immediately. If promised embedded replay cannot fit, fail before downstream work. Decoder returns a typed record and never automatically executes a Boolean. Commit permanent golden V1 bytes.

## 15. Platform and floating-point qualification

### 15.1 Types

V1 `T` is exactly IEC 60559 binary32/64 (`float` size/digits 4/24; `double` 8/53, radix 2). Qualify denormal declarations/behavior. `I` is unsigned non-bool 8/16/32/64-bit. Use stable descriptors, never `typeid().name()`.

### 15.2 Build and IPO

Require the strict-build macro and reject `__FAST_MATH__`. The strict target must disable inherited IPO by default. For every supported compiler/profile, verify effective compile flags after global flags and verify a link-time contraction/reassociation witness. If Ygor is configured with `USE_LTO=ON`, either keep the bounded object non-LTO and prove it remains opaque to unsafe link-time transformation, or reject/disable that profile for the bounded subsystem.

Record the stable strict-build profile and capability result canonically. Record compiler/library identifiers, command-line excerpts, link mode, and project version for diagnostics only.

### 15.3 Runtime guard

A scoped guard captures the caller environment with `fegetenv`, establishes/verifies `FE_TONEAREST`, applies documented exception handling, and restores the original environment on every exit. Failure to establish or restore returns `unsupported_platform`, except that a restoration contradiction during cancellation/rollback is an invariant failure because clean cancellation cannot be claimed. The guard is call/execution scoped, never global. Component 17 must apply the same provider in every worker.

### 15.4 Probes

Probe positive/negative zero bits, adjacent values, normal/subnormal behavior, bit round trips, rounding changes, and contraction-sensitive expressions. Compile-time and link-time controls detect unsafe transformations; probes supplement but do not excuse unsafe compilation.

### 15.5 Two-phase Component 03 handshake

Because the scale-aware floor belongs to Component 03:

1. `build_pending_context` validates everything not requiring the floor and returns a private pending object;
2. a narrow bootstrap adapter receives exact source coordinate descriptions/input precision/platform record and returns a versioned floor/precision-bootstrap record;
3. `finalize_context` validates tolerance against it, includes it in replay/digest, verifies the proposal, and publishes the frozen context;
4. no pending context is exposed to later components;
5. until Component 03 exists, tests use a conforming deterministic stub; production public Boolean integration remains unavailable rather than assuming zero floor.

## 16. Frozen context and service boundaries

### 16.1 Immutable artifact

`boolean_context<T,I>` contains normalized operation/options, type descriptors, all schema/provider versions, truth table, symbolic matrix, deterministic comparator definitions, resources, verification requirements, execution capability, stable platform/precision qualification, normalization report, context/input/replay digests, owner token, and stable operand IDs. Construction completes before publication; expose only const access.

Noncanonical compiler/build diagnostics are not members of the canonical context artifact. They may be retained in a separate invocation diagnostic bundle whose bytes are explicitly excluded from canonical digests.

### 16.2 Controlled runtime bundle

Separate mutable controlled services from context data: resource manager, cancellation token/source view, diagnostic collector, transaction factory, canonical identity publication support, replay accumulator/finalizer, execution provider interface. Services are owned by one top-level call object and outlive all joined work. Later components receive narrow capability views, never the entire mutable bundle.

### 16.3 Builder workflow

1. capture immutable source descriptions/options and validate the cancellation token;
2. preflight mandatory resources, emergency error storage, and replay size;
3. normalize/validate policy and types;
4. establish strict environment and stable platform profile;
5. materialize/verify truth and symbolic tables;
6. encode source/options and initialize replay digest;
7. obtain Component 03 floor record;
8. validate tolerance/ordinary-publication eligibility;
9. construct proposed context/services;
10. independently verify;
11. atomically publish immutable handle.

Failure publishes no context. Caller mesh/options are not retained mutably; borrowing immutable mesh storage is not permitted in V1 after context construction. Cancellation backing state is retained only through a lifetime-safe token until all work is joined.

### 16.4 Capability interfaces

Define read-only/narrow abstract interfaces or value views for policies, truth lookup, symbolic lookup, comparators, resource reservations, cancellation polling, diagnostic submission, transaction creation, ID publication, replay/digest accumulation, and execution scopes. Avoid a mutable singleton. Every capability validates context ownership.

## 17. Transaction service

### 17.1 State machine

Use explicit states: `constructed -> workspace_open -> work_registered -> joining -> verifying -> commit_ready -> committed`, with failure/cancellation/exception transitions from nonterminal states to `rolling_back -> rolled_back`. Illegal transitions are internal invariant errors. Destruction of a nonterminal transaction performs noexcept rollback and records test-visible invariant evidence without publishing.

### 17.2 Workspace and predecessor rules

Each transaction owns private mutable workspace, reservations, task-local IDs, diagnostics, replay contributions, and execution scopes. It reads predecessor artifacts only through immutable owner-validated handles. No private pointer/reference escapes.

### 17.3 Execution scope

Component 01 supplies a serial execution scope and the interface Component 17 will implement. Every scope reports active work, joins, translates expected standard resource exceptions, converts unexpected exceptions, exposes canonical progress/counters, and cannot detach. Transaction cannot verify/commit until all scopes report joined.

### 17.4 Commit protocol

1. stop admitting work;
2. poll cancellation;
3. join scopes;
4. canonicalize private records/IDs;
5. construct proposed immutable artifact using only reserved allocations;
6. run independent verifier;
7. finalize artifact digest/replay contribution;
8. verify exact persistent resource use and promote leases;
9. re-poll cancellation immediately before publication;
10. atomically publish one `artifact_handle<const T>` and mark committed.

If cancellation is observed at either poll, run the cancellation finalization path and roll back. Commit is a state transition/move of already prepared immutable state; it cannot allocate unreserved memory or execute user callbacks.

### 17.5 Rollback

Join all work, restore qualified floating environments, discard workspace/provisional IDs, release reservations, retain predecessor artifacts unchanged, finalize deterministic primary error/cancellation record, verify terminal resource state, and mark rolled back. Rollback is idempotent/noexcept after owned exception capture.

If rollback cannot satisfy joined-work, environment-restoration, or resource-release invariants, record a stable invariant subcode and do not report clean cancellation.

### 17.6 Exception boundaries

Catch exact expected standard exceptions first and translate them according to Section 6.5. Then catch unrelated exceptions at transaction/execution boundaries, join first, and submit a stable `internal_invariant_error` subcode based on the boundary—not implementation-defined text. Preserve `what()` only as optional non-authoritative debug text.

The emergency primary-error constructor and rollback path must be noexcept after the original exception is captured.

## 18. Independent verifiers

### 18.1 Context verifier

Do not call producer normalization/materialization helpers. Recheck recognized versions/enums/reserved fields, scalar rules, type/platform records, strict-build/IPO profile, truth table by procedural recomputation, symbolic totality/remapping, resource consistency, deterministic comparator descriptors, digest/replay linkage, owner/token uniqueness, cancellation-token contract, absence of mutable caller references, and absence of noncanonical build data from canonical bytes. Emit one deterministic finding and prevent publication on contradiction.

### 18.2 Identity verifier

Reconstruct canonical key order, dense ID ranges, uniqueness, owner/domain/operand correctness, and absence of task-local IDs. Forced hash collisions must not affect results.

### 18.3 Resource verifier

Recompute manager totals from live reservation/lease records and verify arithmetic, limits, peaks, owners, transaction state, exact actual-versus-reserved usage, emergency error storage, and no retained reservation after terminal state.

### 18.4 Transaction verifier

Check legal state, joined scopes, restored environments, verified proposal, lease/resource consistency, owner/version/range metadata, digest finalization, cancellation override state, expected-exception translation evidence, and exactly-one-or-zero publication. Producer-reported counts/digests are not sole evidence.

## 19. Required tests and exact coverage

### 19.1 Unit/options/truth tests

Cover all operations/remaps; all policy/version/enum values and invalid gaps; finite/non-finite/negative/signed-zero scalars; precision/tolerance relationships; execution compatibility; both public cancellation call paths; all 80 truth entries; symbolic complete domain, equal/touch/coincident cases, half-open ownership, operand exchange; outcome traits; every Component 01 category/subcode.

### 19.2 Identity tests

Compile-time nonconvertibility; fixed A/B IDs; deterministic publication under insertion/task permutations; each duplicate semantics class; forced collisions; wrong/stale owner/domain/operand; task-local escape; rollback isolation.

### 19.3 Resource and physical-allocation boundaries

For every kind test limit-1/limit/limit+1, optional zero, overflow before limit, large ledger-only reservation, advisory crossing, promotion/release/rollback, count-to-byte/index boundaries, and permuted canonical slices. Never exhaust host memory.

Use deterministic injectable allocators/factories to fail every allocation purpose before reservation, after reservation, during optional diagnostics, during replay retention, and during proposed-artifact construction. Require typed resource failures, fixed-capacity primary errors, complete rollback, and invariant classification only when actual use exceeds reservation or checked preflight was bypassed.

### 19.4 Cancellation

Inject before preflight, after pending context, around every transaction transition, during fake work, after an ordinary error before arbitration, during diagnostics, immediately before commit, and while a worker is blocked on bounded admission. Require joined work, restored FP environment, no leaked reservation/artifact, deterministic checkpoint, intact predecessors, and public `cancelled` regardless of the earlier ordinary error.

Separately inject join, environment-restoration, and rollback contradictions and require deterministic invariant reporting with cancellation retained as evidence.

### 19.5 Arbitration/diagnostics

Submit category/subcode/stage/operand/entity sets in all small permutations and thousands of fixed-PRNG larger permutations; vary fake thread count/delays/capacity; require identical ordinary primary bytes. Fill optional capacity, inject equal/conflicting duplicates, and test locale changes. Verify cancellation override is independent of ordinary reducer order and diagnostic capacity.

### 19.6 Codec/replay

Golden float/u32, float/u64, double/u32, double/u64 records; coordinate signed-zero preservation/policy-zero normalization; exact round trips; truncation at every byte boundary for small records; invalid lengths/counts/bools/enums/required fields/trailing bytes; retention limits; SHA vectors and chunkings; forced collisions; permanent golden compatibility bytes.

Vary compiler banner, library version string, build path, executable name, project timestamp, and optional dependency versions without changing the stable conformance profile; canonical context/replay bytes and digests must remain identical.

### 19.7 Platform/build isolation

Test zero/normal/subnormal/adjacent values, NaN/Inf rejection, changed rounding before call and restoration, contraction/reassociation witness, strict-target macro, test-provider failures for every qualification field, unknown provider replay versions, and link-time optimization behavior.

Build at least:

- GCC and Clang Debug/Release strict targets;
- `USE_LTO=OFF`;
- `USE_LTO=ON` with bounded-object IPO disabled or a separately qualified strict profile;
- sanitizers where supported;
- `WITH_GNU_GSL=OFF` and `WITH_EIGEN=OFF`;
- a deliberately unsafe fast-math fixture that must fail qualification.

Inspect compile/link commands and run semantic witnesses. Do not rely on option spelling alone.

### 19.8 Transactions and exception taxonomy

Test doubles succeed, fail before/after reservation, emit competing failures, throw `std::bad_alloc`, `std::length_error`, `std::system_error`, and unrelated exceptions at workspace/join/verifier/digest boundaries, cancel before commit, claim active work after join, and return wrong owner/version/range/resources. Verify exact terminal state, rollback, leases, category/subcode, cancellation override, emergency-error availability, and immutable publication.

### 19.9 Determinism/properties

Vary caller allocation/capacity, traversal/insertion order, fake workers/delays, collisions, diagnostic capacity, noncanonical build metadata, and repeated runs. Context digest, table bytes, IDs, primary result, and replay bytes remain identical under the same semantic profile. Never use `random_device`.

### 19.10 Mutation tests

Corrupt duplicate/stale/wrong-domain IDs, task-local escape, truth entry, missing symbolic rule, remap transform, resource totals, actual-versus-reserved use, cancellation finalization state, strict-build profile, canonical inclusion of project timestamp, digest, replay byte with unchanged digest, transaction state, and partial committed artifact. Each intended independent verifier rejects it deterministically.

### 19.11 Build matrix

Run supported GCC/Clang Debug and Release, ASan/UBSan, TSan for thread-safe services, and float/double with u32/u64. Strict Boolean targets remain strict under the parent's GNU fast-math and requested LTO settings. Normative tests remain buildable and linkable with optional external dependencies disabled.

## 20. Implementation sequence and gates

1. File layout, strict object target, IPO isolation, fast-math guard, dependency-free test harness. Gate: compile/link strict witnesses pass under normal and requested LTO configurations; external-dependency-off tests link; unrelated Ygor still builds.
2. Versions, checked arithmetic, public policy/options/cancellation API, outcome. Gate: enum/type/arithmetic boundaries and both public cancellation overloads pass.
3. Canonical codec and SHA-256. Gate: vectors/golden primitives/truncation/overflow and noncanonical-build-data exclusion pass.
4. Errors/subcodes/payloads/emergency record/summaries/arbitration/exception taxonomy. Gate: exhaustive/random permutation, bad-allocation, and fixed-capacity fallback tests pass.
5. Strong IDs/owner/factory/verifier. Gate: separation/permutation/collision/owner/mutation pass.
6. Resource manager/reservations/slices/leases/defaults/allocation helper/verifier. Gate: every kind's boundary, injected host-failure, and rollback tests pass.
7. Cancellation/progress/diagnostics. Gate: checkpoint/capacity schedule permutations, failure-before-cancel override, and rollback-contradiction tests pass.
8. Truth and symbolic tables. Gate: all 80 entries, complete key domain, equal/touch/remap pass.
9. Platform/build/runtime FP qualification. Gate: strict compile/link, IPO isolation, bits, rounding restoration, subnormal, contraction pass.
10. Replay schema/preflight/codec/retention/goldens. Gate: replay/type matrix and build-metadata invariance pass.
11. Pending/frozen context, precision handshake, services, context verifier. Gate: valid/invalid/mutation tests pass with no caller references retained.
12. Transaction/execution scope/artifact commit/rollback/verifier. Gate: success/failure/expected-resource-exception/unexpected-exception/cancel/resource tests pass with no surviving work.
13. Full property/adversarial/replay/platform/sanitizer/type/dependency/LTO matrix and installed-header check. Gate: Section 21 complete.

## 21. Definition of done

Component 01 is complete only when:

- public operation/options/error/cancellation contracts are explicitly versioned/documented;
- the ordinary and explicit cancellation-aware `bounded_boolean` entry paths are defined and tested;
- all required ID domains are distinct C++ types and every artifact has a context owner;
- IDs never derive from timing, pointers, coordinates, hashes alone, or insertion order;
- task-local IDs cannot escape;
- all 80 truth entries and operand remaps are independently verified;
- symbolic key domain is total, immutable, versioned, replayed, and independently verified;
- all scalar/resource/execution/diagnostic validation and normalization behaves exactly as specified;
- strict compilation and link semantics are proven despite parent fast-math and requested LTO;
- normative tests link without optional external dependencies or downloaded frameworks;
- platform/caller FP environment is qualified/restored and unsupported cases fail before geometry;
- stable canonical profile data is separated from compiler/build/timestamp diagnostics;
- Component 03 two-phase floor handshake exists and pending contexts never escape;
- typed errors contain stable category/subcode/stage/entity/numeric/policy/replay evidence;
- expected host allocation/container/worker failures are typed resource failures and producer reservation contradictions are invariants;
- a fixed-capacity or pre-reserved primary-error path survives optional allocation failure;
- ordinary primary error bytes are invariant under order, scheduling, workers, collisions, and diagnostic capacity;
- valid cancellation observed before publication deterministically overrides earlier ordinary failures;
- cancellation joins work, restores environments, rolls back resources, and prevents publication at every required checkpoint;
- every transaction publishes one verified immutable artifact or nothing;
- canonical/replay bytes are fixed-endian, bounded, versioned, golden-tested, build-instance-independent, and byte-stable;
- in-tree SHA-256 passes standard vectors and is never authoritative equality;
- no service depends on mutable globals, addresses, locale, time, random devices, build paths, or project invocation versions;
- unsuitable legacy serializers/hashes/thread pools/Boolean files are not used;
- unit, property, adversarial, mutation, replay, platform, transaction, resource-failure, cancellation, sanitizer, LTO, dependency-off, and type-matrix tests pass;
- public/internal headers compile as strict portable C++17; and
- later components require only the documented read-only context data and controlled capabilities.
