# Component 1 implementation plan: operation contract and Boolean context

## 1. Scope and outcome

Implement the invocation-wide contract layer for the robust B-rep Boolean engine. This component must make operation semantics, supported template specializations, IDs, deterministic ordering, ownership, options, failures, accounting, cancellation, diagnostics, tracing, replay metadata, and verified publication available to every later component without making geometric decisions itself.

The completed component must provide:

- a frozen truth-table-based operation contract for all five regularized operations;
- a validated, logically immutable `boolean_context<T, I>` referring to the two immutable operands and shared invocation services;
- strong IDs and deterministic serialization/comparison facilities for every entity domain named by the component requirements;
- a C++17 checked-result and structured-error model;
- overflow-safe resource accounting and cooperative cancellation;
- deterministic diagnostic, trace, digest, and replay structures;
- stage-private transactions and immutable, verified publication;
- executable contract tests, including fault injection and schedule-independent publication tests.

Do not implement input mesh validation/canonicalization, exact arithmetic, intersection discovery, arrangement construction, cell classification, realization, or output assembly here. Define the interfaces those components consume. In particular, Component 1 records the solid and realization policies but does not decide whether a mesh satisfies the solid policy or whether a symbolic point is representable.

All implementation must remain self-contained in Ygor, use only C++17 and the standard library plus in-tree Ygor code, and contain no geometric epsilon or tolerance setting.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse

- Continue to use `fv_surface_mesh<T, I>` from `src/YgorMath.h` as the public operand and eventual result interchange type. Treat `vertices` and `faces` as the geometry payload. The Boolean contract must state that input normals, colours, `involved_faces`, and metadata do not control topology; later components may define separately whether any attributes are propagated.
- Follow the explicit-specialization matrix already used by mesh code: `float` or `double` coordinates with `std::uint32_t` or `std::uint64_t` indices.
- Reuse the canonical undirected-pair idea from `make_undirected_edge` in `src/YgorMeshesVerification.h`, but use strong feature IDs rather than mesh indices for the new internal edge key.
- Reuse the successful pattern in `src/YgorMeshesBoolean5.cc` of sorting provisional records by complete tuple keys before deduplication and publication. Hash-container iteration must never determine IDs or output order. Use `src/YgorMeshesBoolean5.cc` as a pattern reference only, as the file itself will be removed in the near future.
- Wrap the in-tree MD5 implementation behind a safe canonical-byte digest API for replay input digests. First replace its native `unsigned int*` block loads with `memcpy` or explicit byte assembly so unaligned input and strict aliasing are well-defined, and expose a const-byte streaming wrapper. Feed only explicitly encoded bytes to it; do not pass native object representations. The digest is an integrity/replay locator, never an equality proof or entity identity.
- Permit an optional adapter that forwards already-published diagnostics to `ygor::logger`, but only after deterministic recording in the context.

### 2.2 Do not reuse as contract behavior

- Do not route the new engine through `MeshBooleanOperation5`, `BooleanMeshOp5`, or the earlier Boolean generations. Their four-operation APIs, tolerance/snap decisions, approximate classification, and direct mesh returns do not satisfy this contract.
- Do not use `Boolean5SnapCoord`, raw `uint64_t` feature IDs, vector insertion order, pointer values, or input traversal order as canonical identity.
- Do not use `Consistent_Hash_64` for replay bytes: its SpookyHash implementation documents endian-dependent output. Do not use `std::hash` values in serialized data.
- Do not use `YLOGERR` for expected failures; the current logger can terminate the process. Wall-clock times and thread IDs from `YgorLog` are also excluded from canonical diagnostics and replay data.
- Do not use `work_queue` from `src/YgorThreadPool.h` as the invocation executor without a later adapter that propagates failures and cancellation. It currently swallows worker exceptions and has no caller cancellation token.
- Do not call tolerance-based mesh mutation helpers such as `merge_duplicate_vertices`, and do not add tolerance fields to the context.

## 3. Files and integration

Add the following top-level files so the existing `src/CMakeLists.txt` source/header glob compiles and installs them:

- `src/YgorMeshesBooleanContract.h`: public enums, policies, strong IDs, checked-result/error types, replay structures, immutable operation contract, context options, context factory, and the eventual `boolean_result<T, I>` declaration.
- `src/YgorMeshesBooleanContract.cc`: non-template operation semantics, option/platform validation, canonical serialization/digest support, diagnostics, accounting, cancellation, replay construction, and explicit template instantiations.
- `src/YgorMeshesBooleanTransaction.h`: template publication store and stage-transaction definitions. Keep templates in this header rather than adding the existing test-only pattern of including a `.cc` file.
- `src/YgorMeshesBooleanExecutor.h`: the context-owned deterministic executor, task-group, cancellation, and private-shard interfaces. It may use `std::thread` directly but must not inherit `work_queue`'s exception-swallowing behavior.
- `tests/Test_MeshesBooleanContract.cc`: standalone contract tests using the simple pass/fail style of the other `tests/Test_MeshesBoolean*.cc` programs.

Update:

- `src/CMakeLists.txt` so a named list of Boolean strict-arithmetic `.cc` sources receives strict floating-point options. For GCC, place `-fno-fast-math -frounding-math` after repository-wide flags; for Clang, use `-fno-fast-math -ffp-model=strict` when supported. Add a compile-time guard for `__FAST_MATH__`/finite-math-only macros. Keep floating-point arithmetic that depends on these flags out of public templates, and add Component 3 sources to this same list later. Component 3 must check or establish the required thread-local rounding mode at each kernel entry; Component 1 only validates setup facts. Do not remove unrelated project flags in this component.
- `tests/compile.sh` to build `Test_MeshesBooleanContract.cc`.
- root `CMakeLists.txt` and a small `tests/CMakeLists.txt` to use `include(CTest)`, build this test when `BUILD_TESTING` is enabled, and register it with `add_test`.
- `.gitlab-ci.yml` to build and run this CTest by name without `|| true` and with a contract-test-specific timeout. Add available GCC/Clang debug and release jobs; sanitizer jobs may be separate/manual when the runner lacks resources, but their commands and expected use must be documented. Keep unrelated legacy tests unchanged.

Place the new subsystem in namespace `ygor::mesh_boolean`. Existing mesh APIs are mostly global, but a namespace is required to prevent generic names such as `stage`, `result`, and `cell_id` from polluting the global namespace. Public comments must map the conceptual names in the component documents to these concrete C++ names.

## 4. Mathematical operation contract

### 4.1 Public operation and side conventions

Define:

```cpp
enum class operation : std::uint8_t {
    regularized_union,
    regularized_intersection,
    a_minus_b,
    b_minus_a,
    symmetric_difference
};

struct occupancy_pair {
    bool in_a;
    bool in_b;
};
```

Define the truth function once, as a `constexpr` function and a frozen four-entry table in `operation_contract`. Index the table explicitly by `(in_a ? 2 : 0) | (in_b ? 1 : 0)` and test that encoding. The required formulas are:

| operation | `R(in_A, in_B)` |
|---|---|
| regularized union | `in_A || in_B` |
| regularized intersection | `in_A && in_B` |
| A minus B | `in_A && !in_B` |
| B minus A | `!in_A && in_B` |
| symmetric difference | `in_A != in_B` |

Define oriented source facets by right-hand-rule normal. Their positive/open front side lies in the normal direction and their negative/open back side lies opposite it. A final result patch must be oriented so occupied result volume lies on its negative side and unoccupied volume lies on its positive side. Later selection code must call `operation_contract::occupied(...)` for both open sides and select precisely when the values differ; no later component may duplicate a switch on `operation`.

Document that all operations are regularized: evaluate the set truth function on open three-dimensional cells and take `closure(interior(...))`. Therefore an isolated point or curve contact cannot itself produce output boundary.

### 4.2 Required degeneracy semantics

Encode these statements in `operation_contract` documentation and truth-table/contract tests so later components cannot invent case-specific rules:

- An empty operand behaves solely according to the truth table. An empty selected boundary is successful and represents the empty regular closed solid.
- Equal occupied solids produce that same solid for union and intersection, empty for both ordered differences, and empty for symmetric difference, independent of duplicate coincident derivations.
- Disconnected components are evaluated independently by occupancy; component count has no special semantic effect.
- Nested outward shells and inward cavity shells contribute to occupancy according to the selected `solid_policy`; nesting itself is not a Boolean special case.
- Point and curve tangencies that do not separate cells produce no boundary. If a tangency coexists with an area boundary, only the area boundary selected by side occupancy remains.
- Coincident or coplanar regions with the same or opposite source orientation are merged by canonical geometric identity in later components. Their source orientation does not directly decide retention; the occupancy truth values on the two open sides do.
- Difference is ordered. Swapping operands changes `a_minus_b` into the semantics of `b_minus_a`; it must never be implemented as a commutative operation.

### 4.3 Solid policy

Define an explicit, closed enum rather than open-ended callbacks:

```cpp
enum class solid_policy : std::uint8_t {
    outward_oriented_nested_shells
};
```

The initial implementation supports only this policy: each validated connected shell is consistently oriented, outward shells add occupied volume, oppositely oriented nested shells define cavities, and the complete boundary represents a regular closed solid. Component 2 will prove the policy. Reserve no undocumented numeric enum values, and reject deserialized unknown values. Future policies require a replay schema revision or explicit backward-compatible extension.

## 5. Strong identity model

### 5.1 ID representation

Implement a private `strong_id<Tag>` class template over `std::uint64_t` and expose distinct, non-convertible types for:

- `operand_id`;
- `shell_id`;
- `original_vertex_id`;
- `edge_use_id`;
- `undirected_edge_id`;
- `facet_id`;
- `candidate_id`;
- `raw_event_id`;
- `symbolic_vertex_id`;
- `symbolic_curve_id`;
- `local_patch_id`;
- `global_halfedge_id`;
- `global_patch_id`;
- `cell_id`.

Each ID must be trivially copyable, equality comparable, strictly ordered, explicitly constructible only through the appropriate factory/publication store, and serializable as a fixed-width unsigned integer. Do not provide implicit conversion to or from integers. Provide `value_for_debug()` only for diagnostics/serialization, not arithmetic. Use one documented invalid sentinel (`UINT64_MAX`) only for default construction where a type must be default constructible; reject the sentinel at publication and never serialize it as a valid ID.

`operand_id` is the one fixed domain: define canonical IDs `A = 0` and `B = 1`. Every other ID domain is invocation-global, including original shell/vertex/edge/facet IDs: Component 2 sorts keys beginning with operand role and assigns one dense sequence across both operands. Thus no two valid IDs of the same type share a numeric value in one invocation. Provenance records still carry the operand role explicitly where useful.

Bare IDs are meaningful only with their owning context/store. Create a process-local `context_owner_token` from an overflow-checked atomic `uint64_t` sequence at context setup. Stores, transactions, certificates, and owner-bearing handles carry this token and compare it at API boundaries. The token is never used for canonical ordering, IDs, digests, diagnostics, or serialization; equal deterministic invocations still receive different process-local tokens. Token-sequence exhaustion returns `internal_invariant_error` during setup.

Do not specialize `std::hash` as an ordering mechanism. If later components need hash lookup, provide an in-namespace hasher that hashes the integer value only for lookup and require final traversal through sorted records.

### 5.2 Provenance keys and factories

Component 1 owns the generic canonical assignment algorithm; the component that introduces an entity owns its complete provenance-key type and comparator.

Provide a `canonical_id_factory<Id, Key>`/publication utility with this exact process:

1. Accept stage-private provisional records containing a normalized key and payload, never a requested final ID.
2. Sort by the key's explicit lexicographic comparator, not by a hash or payload address. Key normalization and every fallible exact comparison occur before this step; the stored normalized-key comparator itself is a total, strict-weak-order `bool` function and `noexcept`, as required by `std::sort`.
3. Group equal keys. Invoke the owning component's equivalence/merge callback; it must either prove the records are the same mathematical entity and produce one payload, or return `internal_invariant_error` for a conflicting duplicate.
4. Check the group count against the relevant entity resource limit and the ID capacity. With `UINT64_MAX` reserved as invalid, valid values are `0..UINT64_MAX-1`, so at most `UINT64_MAX` entities are representable; perform the comparison through checked conversion from host `size_t` without expressing that count in an overflowing `uint64_t` intermediate.
5. Assign dense IDs `0..n-1` in sorted key order only after all groups and merged payloads verify.
6. Publish an immutable vector indexed by the assigned ID.

Component 2 must construct original shell, vertex, edge-use, undirected-edge, and facet keys only after canonicalizing each operand. Constructed-feature components must use normalized provenance keys, exact comparisons, and this factory. Parallel workers may create provisional records but may not allocate final IDs.

Define provenance references such as `original_vertex_ref { operand_id, original_vertex_id }` and `facet_ref { operand_id, facet_id }`, validate that their operand matches the stored original entity, and order by operand then globally unique ID. All ID aliases are invocation-global; the qualified references communicate provenance rather than repair local uniqueness.

### 5.3 Canonical serialization

Implement a small append-only `canonical_encoder` with methods for bytes, booleans, enums with validated underlying values, unsigned integers in network/big-endian byte order, length-prefixed byte/string/vector fields, ID values, and IEC 60559 floating-point bit patterns. It must never serialize raw structs, `size_t`, native endianness, pointer values, locale-dependent text, padding, timestamps, or thread IDs.

Use one positional canonical grammar. Primitive encodings are: bool as one byte `00`/`01`; enums as their documented one-byte value; `u16/u32/u64` as fixed-width big-endian; byte strings/vectors as `u64 count` followed by exactly that many elements; optional values as a bool followed by the value only when present; float/double as 4/8 bytes holding their IEC 60559 bit pattern in big-endian order. Every top-level record begins with an eight-byte ASCII domain tag, `u16` schema version, and `u64` payload length. Record-specific sections below must list fields in exact order. Decoders check payload/vector lengths against configured decode limits before allocation, reject bool/enum values outside their domain, require the exact field count implied by the recognized version, and reject trailing bytes. Unknown schema versions are errors; there are no skippable or implicit fields in schema version 1.

`+0` and `-0` retain distinct input bit encodings in the replay input digest even though Component 3 may prove them mathematically equal. NaN and infinity bit patterns can be encoded for a diagnostic digest, but Component 2 must reject them before geometry processing.

## 6. Options, platform contract, and context creation

### 6.1 Options

Define value types with deterministic defaults:

```cpp
enum class verification_level : std::uint8_t { mandatory, exhaustive };
enum class trace_level : std::uint8_t { off, failures, stages, full };
enum class determinism_policy : std::uint8_t { strict };
enum class realization_strategy : std::uint8_t {
    nearest_only,
    neighboring_values,
    decline
};

struct execution_policy {
    std::uint32_t max_threads = 1;
    std::uint32_t max_queued_tasks = 1024;
};

struct tracing_policy {
    trace_level level = trace_level::failures;
    bool collect_noncanonical_timings = false;
};

struct realization_policy {
    realization_strategy strategy = realization_strategy::nearest_only;
    std::uint32_t neighboring_value_radius = 0;
};

struct diagnostic_policy {
    bool forward_to_ygor_logger = false;
};

struct resource_limit {
    bool unlimited = true;
    std::uint64_t value = 0;
};

struct resource_policy {
    resource_limit authoritative_bytes;
    resource_limit stage_private_bytes;
    resource_limit work_units;
    resource_limit entities_per_store;
    resource_limit candidates;
    resource_limit raw_events;
    resource_limit symbolic_vertices;
    resource_limit symbolic_curves;
    resource_limit local_patches;
    resource_limit global_halfedges;
    resource_limit global_patches;
    resource_limit cells;
    resource_limit exact_number_bits;
    resource_limit diagnostic_records;
    resource_limit diagnostic_bytes;
    resource_limit trace_records;
    resource_limit trace_bytes;
    resource_limit realization_attempts;
};

struct boolean_options {
    solid_policy solids = solid_policy::outward_oriented_nested_shells;
    determinism_policy determinism = determinism_policy::strict;
    verification_level verification = verification_level::mandatory;
    execution_policy execution;
    tracing_policy tracing;
    realization_policy realization;
    diagnostic_policy diagnostics;
    resource_policy resources;
};
```

All `resource_policy` limits default to explicit unlimited correctness mode; applications may set finite values. The execution defaults are one worker and a bounded pending-task queue, but the queue bound is a scheduling bound rather than a correctness/resource failure threshold: the producer waits when full. `execution_policy` itself is the thread/task budget; report its configured and observed counts, but do not double-account them as byte/entity limits. Diagnostics are always canonical; `diagnostic_policy` controls delivery only. Schedule-independence checking is a test mode, not a runtime option that would repeat user work.

Determinism is mandatory and the only schema-v1 value is `strict`. Mandatory verification cannot be disabled; `exhaustive` adds expensive checks. `nearest_only` and `neighboring_values` still require Component 11 certification, and `decline` deterministically returns `output_not_representable` if realization is needed.

`boolean_options` must contain no distance, angle, area, snapping, welding, or other geometry-specific tolerance. Its schema-v1 canonical field order is exactly the declaration order above; nested fields use their declaration order, and every `resource_limit` encodes `unlimited` followed by `value`, which must be zero when unlimited.

Validate before context publication:

- at least one thread and one queue slot, both representable in their fixed `uint32_t` fields;
- finite trace/diagnostic limits do not exceed finite authoritative-byte limits and tracing `off` does not reserve trace storage;
- neighboring radius is zero unless `neighboring_values` is selected and is bounded by its work budget;
- mandatory verification is present;
- every unlimited limit has value zero and every finite limit has a nonzero value;
- enum values are recognized, including values obtained by deserialization;
- policy combinations do not request an uncertified realization path.

Return `input_contract_error` with stage `context_setup` and option-field diagnostics for invalid/contradictory options. Do not throw.

### 6.2 Supported specializations and platform facts

Define `platform_facts` and populate it without undefined behavior. Record at least:

- engine semantic version and replay schema version;
- coordinate and index type tags and byte widths;
- `CHAR_BIT`, host endian report, and integer width facts;
- coordinate radix, digits, minimum/maximum exponents, IEC 60559 status, and subnormal support;
- whether the compilation unit reports fast-math or finite-math-only assumptions;
- the initial floating-point rounding mode observed at context setup.
- whether runtime arithmetic preserves subnormal inputs and results rather than operating with denormals-are-zero or flush-to-zero modes.

Make schema-v1 fields concrete and ordered: engine major/minor/patch (`u16` each), replay schema (`u16`), coordinate tag (`binary32`/`binary64`), index tag (`uint32`/`uint64`), coordinate/index byte widths (`u8`), `CHAR_BIT` (`u16`), endian enum, `uint32_t`/`uint64_t` widths (`u8`), radix/digits (`u32`), signed min/max exponents encoded as sign byte plus `u32` magnitude, IEC-60559/subnormal booleans, fast-math/finite-math-only booleans, and rounding-mode enum. Add canonical signed-integer encoding as sign byte plus unsigned magnitude with negative zero forbidden. Do not serialize compiler names/versions as compatibility facts unless they are separately noncanonical diagnostics.

Accept exactly `float`/`double` coordinate C++ types and exactly `std::uint32_t`/`std::uint64_t` index C++ types in the first implementation. Enforce that type set through `is_supported_boolean_types<T,I>` and SFINAE/deleted overloads, so unsupported C++ types fail at compile time rather than link time. For those four accepted pairs, return `unsupported_platform` for runtime/compiler representation facts such as non-IEC layout, non-binary radix, non-eight-bit bytes, unsafe arithmetic flags, or unsupported rounding mode.

The context must either require round-to-nearest at setup or preserve/restore a required rounding environment around filtered arithmetic in Component 3. For Component 1, record and validate the selected rule and reject unsupported modes. Validate runtime subnormal arithmetic explicitly because `fegetround()` and `std::numeric_limits<T>::has_denorm` do not detect process-level flush-to-zero or denormals-are-zero modes. Do not claim that `std::numeric_limits<T>::is_iec559` alone defeats compiler fast-math; include the build guard described in Section 3.

Use a stable manually maintained Boolean engine version constant. Do not use the timestamp-derived CMake project version as replay compatibility.

### 6.3 Input ownership and immutable context

Construct contexts only through a checked factory:

```cpp
template<class T, class I>
status_or<std::unique_ptr<boolean_context<T, I>>>
make_boolean_context(const fv_surface_mesh<T, I>& a,
                     const fv_surface_mesh<T, I>& b,
                     operation op,
                     const boolean_options& options,
                     std::shared_ptr<const exact_kernel_services<T>> kernel,
                     std::shared_ptr<const verifier_service> verifiers,
                     cancellation_source* caller_cancellation = nullptr,
                     diagnostic_consumer consumer = {});
```

Define an abstract immutable `exact_kernel_services<T>` in Component 1 with a virtual destructor and nonthrowing methods returning the supported coordinate tag, arithmetic-policy canonical bytes/digest, and a stable implementation-type tag. Require a non-null shared instance in the factory and freeze it in the context. Component 3 implements this interface in `exact_kernel<T>` and provides the checked downcast/accessor used by geometry stages; a tag/type mismatch is `internal_invariant_error`, never an approximate fallback. This metadata-only base lets Component 1 implement and explicitly instantiate context creation before Component 3 exists, using a test implementation of the interface without defining the production kernel.

Require a non-null immutable `verifier_service` whose registry is fully configured before setup; Component 13 supplies the production artifact specifications, and neither its registry nor the context binding can mutate after setup. Component 1 supplies generic store/transaction structural verifiers. Use this explicit type-erased boundary:

```cpp
struct artifact_view {
    context_owner_token owner;       // process-local, not serialized
    artifact_slot slot;
    std::uint64_t artifact_type_tag; // stable, explicitly assigned by its component
    digest artifact_digest;
    std::shared_ptr<const void> lifetime;
    const void* payload;
};

using verifier_callback = status_or<verification_report> (*)(
    const artifact_view&, const verification_spec&) noexcept;
```

The registry key is `(artifact_slot, artifact_type_tag, verification_spec_version)` and maps to one callback plus the required invariant-code set. Registration rejects duplicates/unknown slots and freeze rejects missing mandatory slot registrations. Each component declares a stable type tag and a typed adapter that checks slot/tag, keeps `lifetime` alive, casts `payload` to its immutable artifact type, and invokes its verifier. Raw pointers/type tags are process-local dispatch only and never canonical identity; the callback's report is bound to canonical digests. Registry lookup failure/type mismatch is `internal_invariant_error`.

The context constructs and owns a `deterministic_executor` from `execution_policy`; it propagates task `status_or` values/exceptions, joins all tasks before stage completion, gives each task a deterministic task key and private shard, and never publishes in completion order.

The initial API borrows both meshes for the lifetime of the invocation to avoid an unbudgeted copy. `boolean_context` is non-copyable and non-movable after construction, stores only `const` operand pointers/views, and documents that the caller must keep both objects alive and unmodified until the returned Boolean invocation/result completes. A future owned-input overload may copy only after charging the copy against resource limits; do not add it speculatively.

Compute `input_digest` during setup from a canonical encoding of operand role, coordinate/index type tags, vertex count and coordinate bit patterns, face count, each face length, and each index in original input order. Exclude normals, colours, `involved_faces`, and metadata because they do not control the Boolean geometry. Include the operation and serialized options in the invocation/replay digest, but retain separate per-input geometry digests for diagnostics. Component 2 may add canonicalized-operand digests after validation.

After factory success, expose operation/options/platform/replay/input/kernel/verifier views as `const` accessors only. Logical mutation is restricted to explicitly owned services: executor state, resource counters, cancellation state, stage-private transactions, and append-then-freeze diagnostic/trace collectors. The context must not silently instantiate, replace, or fall back to a legacy approximate kernel or verifier.

## 7. Checked result and structured failure model

### 7.1 Result type

Implement an in-tree C++17 `status_or<Value>` using `std::variant<Value, boolean_error>`. It must:

- always hold exactly one value or error;
- provide explicit `has_value()`, `value()`, and `error()` accessors with checked misuse behavior;
- support move-only values and avoid default-constructing `Value`;
- never convert implicitly to `bool` or silently discard an error;
- provide helpers that preserve the original error when propagating it.

`value()` and `error()` have programmer-precondition semantics: assert in debug and throw `std::logic_error` in release when the wrong alternative is requested. Also provide nonthrowing `value_if()`/`error_if()` pointer accessors. No expected Boolean failure uses these exceptions.

Define the final public shape now so later components do not redesign errors:

```cpp
template<class T, class I>
using boolean_result = status_or<fv_surface_mesh<T, I>>;
```

An empty mesh inside the value alternative is successful empty geometry. It can never represent failure.

### 7.2 Error taxonomy and payload

Define `boolean_error_code` with all required categories:

- `input_contract_error`;
- `unsupported_platform`;
- `resource_limit`;
- `index_overflow`;
- `output_not_representable`;
- `internal_invariant_error`.

Represent cooperative caller cancellation as `resource_limit` with `resource_kind::cancellation`, preserving the six-category external contract while providing an unambiguous structured subcode. Cancellation is not reported as malformed input and never publishes partial state.

Define this closed schema-v1 `boolean_stage : uint8_t` mapping: `context_setup=0`, `input_validation=1`, `broad_phase=2`, `intersection_events=3`, `symbolic_registry=4`, `local_refinement=5`, `global_arrangement=6`, `cell_classification=7`, `boolean_selection=8`, `geometry_realization=9`, `output_assembly=10`, and `final_verification=11`. Exact-kernel failures use the stage whose work requested the predicate/construction. Diagnostic flushing uses its owning stage. Define `feature_ref` as a tagged variant over all strong ID/reference types; never flatten unlike ID types to a bare integer.

Every `boolean_error` contains:

- `boolean_error_code code`;
- a stable detailed subcode appropriate to the category;
- `boolean_stage stage`;
- zero or more sorted/deduplicated `feature_ref` values;
- deterministic numeric facts such as requested/limit/current for resource failures;
- a stable short message key and optional deterministic detail string;
- a tagged successful-setup or provisional-setup digest and replay schema/engine versions;
- an optional canonical trace locator, never a pointer or filesystem path required for correctness.

Do not place localized prose, timestamps, source addresses, allocation addresses, or thread IDs in the canonical error payload. Human rendering is a separate function. Include every stable feature ID available and assigned at the failure point; setup failures legitimately have none. All public failures, including setup failures where a full context does not yet exist, must include the replay metadata available at that point.

Arithmetic filter uncertainty is not an error code. Component 3 must fall back to exact arithmetic. Assertions may remain for developer diagnosis, but every publication invariant has a release-mode checked path returning `internal_invariant_error` before publication.

Normalized-key comparators are total `bool`/`noexcept` functions; fallible key construction/normalization happens before sorting. Merge, verifier, task, and encoder callbacks return `status_or` and are `noexcept` where their operations permit. At every executor task boundary and public/stage coordinator boundary, catch `std::bad_alloc` as allocation `resource_limit`, catch other `std::exception` and unknown exceptions as `internal_invariant_error`, cancel sibling work, join it, and roll back the transaction. User diagnostic consumers are handled separately as described below. Exceptions are allowed to cross the public Boolean API, but should always be descriptive and aid debugging.

## 8. Resource accounting and cancellation

### 8.1 Limits and checked arithmetic

Define `resource_kind` values for at least:

- total committed bytes and stage-private bytes;
- work units;
- candidates, events, and each published entity domain;
- exact-number limbs/bits;
- diagnostics and trace bytes/records;
- realization attempts;
- cancellation.

Represent a limit as `{ bool unlimited; std::uint64_t value; }`; do not use `UINT64_MAX` ambiguously as both a valid bound and unlimited. Provide checked `add`, `multiply`, and integer-conversion helpers that return structured failure rather than wrapping. Every vector count-to-byte calculation and every index conversion in the new engine must use these helpers.

Define byte limits as allocator-requested bytes, not physical RSS: they include requested element storage and explicitly requested scratch blocks but cannot portably include allocator metadata, exception runtime storage, thread stacks, or library-internal overhead. Implement `accounting_allocator<T>` backed by the context accountant and require it for every authoritative published container, transaction shard, canonical byte buffer, diagnostic/trace buffer, and explicitly allocated sort/merge scratch block. Use in-place `std::sort` rather than allocating `stable_sort`; pre-reserve known vector/string capacities through the accounting allocator. Temporary stack objects and the documented excluded runtime overhead remain subject to `bad_alloc` conversion. Tests and documentation must not claim a hard process-RSS cap.

`resource_accountant` supports reservation, commit, and release:

1. Calculate requested units with checked arithmetic.
2. Compare against the per-kind and aggregate limits before allocation.
3. Reserve atomically or under the context accounting mutex.
4. Commit only when storage/publication succeeds; release private reservations on transaction destruction/failure.
5. Keep committed counters monotonic for published data and expose deterministic snapshots.

Never perform an allocation covered by the accounting allocator before its reservation succeeds. Convert `std::bad_alloc` from covered or excluded allocations at public/stage boundaries to `resource_limit` with an allocation subcode when possible, without continuing from partially mutated state.

Before a parallel round, tasks submit conservative allocation envelopes and work charges. The coordinator sorts them by canonical task key, reserves each envelope against context/stage limits in that order, and only then dispatches workers. Worker accounting allocators draw solely from their granted envelope. If unknown-size work needs more capacity, workers stop at a deterministic checkpoint and submit an additional request; after all workers reach the checkpoint or finish, the coordinator grants requests in canonical key order and starts the next round. The first denied canonical request is the reported failure. Unused envelope units return when the round/transaction ends, committed artifact bytes transfer only at publication, and no schedule-dependent direct global reservation is permitted. No worker publishes directly.

### 8.2 Cancellation

Implement C++17 `cancellation_source` and cheap copyable `cancellation_token` over shared atomic state. The context combines an internal failure/cancel source with an optional caller source. Check cancellation:

- before starting each stage;
- at documented bounded intervals in long loops;
- before expensive allocation/exact fallback;
- before verification and immediately before publication.

Workers return private cancellation failures and stop at safe points; they do not mutate published stores. If cancellation races another failure, choose deterministically: an already-established canonical stage failure wins; otherwise report cancellation at the earliest incomplete stage. Tests must not depend on the number of loop iterations completed before observation.

## 9. Diagnostics, trace, and replay

### 9.1 Deterministic diagnostics

Define structured `diagnostic_record` with severity, stable diagnostic code, stage, sorted feature refs, deterministic fields, and an optional canonical text key. Worker diagnostics remain in stage-local buffers. At stage completion:

1. sort by `(stage, canonical work key, diagnostic code, feature refs, fields)`;
2. deduplicate records declared identical;
3. charge the finalized encoding size;
4. append them under a monotonic publication sequence;
5. optionally invoke the user consumer and logger adapter after publication.

Consumer exceptions must not escape or corrupt the invocation. Record one deterministic consumer-failure diagnostic if budget permits, disable that consumer, and continue. Delivery is always observational and never a precondition for geometric success. The global logger is observational only.

### 9.2 Stage trace

Trace records contain stage transition, canonical artifact counts/digests, accounting snapshots, verification outcome, and fault-injection point where applicable. Canonical traces exclude timing. Optional performance timing may be retained in a clearly noncanonical side channel excluded from replay hashes and determinism comparisons.

Respect `trace_level` and charge bytes before appending. If a mandatory failure trace cannot fit its declared budget, return `resource_limit` without replacing an earlier substantive stage failure; attach a deterministic `trace_truncated` fact to that failure instead.

### 9.3 Replay descriptor

Define a versioned `replay_descriptor` containing:

- schema and engine versions;
- operation and complete serialized options/policies/limits;
- both original input digests and encoded geometry byte counts;
- canonicalized operand digests when Component 2 supplies them;
- platform facts and supported specialization tags;
- deterministic execution policy, including requested thread/task budgets;
- enabled verification and trace levels;
- canonical stage/artifact digests and the final descriptor digest.

Use separate domain-tagged values with no circular coverage:

- `input_digest_A/B = MD5("YGBINP01" || encoded geometry payload)`;
- `setup_digest = MD5("YGBSET01" || operation || canonical options || both input digests and byte counts || platform facts || kernel arithmetic-policy digest)`;
- `provisional_setup_digest = MD5("YGBPRE01" || presence-and-value fields for operation, options payload, input A digest/count, input B digest/count, platform facts, kernel policy digest, in exactly that order)`;
- `artifact_digest = MD5("YGBART01" || setup_digest || artifact_slot || canonical geometric/topological artifact payload)`;
- `verification_report_digest = MD5("YGBVER01" || setup_digest || artifact_digest || verifier/spec versions || invariant-set digest || canonical check/evidence payload)`;
- `descriptor_digest = MD5("YGBRPL01" || complete replay descriptor with the descriptor-digest field omitted)`.

Each provisional field begins with the standard optional-value presence bool and, when present, its normal canonical encoding; invalid raw enum/options bytes are represented by a present length-prefixed raw canonical input blob plus the validation subcode, not guessed normalized values. Setup failures carry `provisional_setup_digest`. Once setup succeeds, all later errors and artifacts carry `setup_digest`. The descriptor reproduces deterministic behavior when paired with operands whose canonical input encodings match the recorded digests. State this precondition explicitly; a digest alone is not an input archive. Provide canonical encode/decode and exact round-trip comparison. Reject incompatible schema versions rather than guessing defaults.

Verification reports and certificates are excluded from `artifact_digest`, even though `published_artifact` owns the accepted report for lifetime/convenience. Reports bind back to the artifact through `artifact_digest` and have the separate digest above. Certificates are process-local authority objects and are not serialized or included in either payload digest; replay stores the report and its digest.

Schema-v1 replay payload field order is: engine version, operation, options payload, input A digest/count, input B digest/count, platform facts, kernel arithmetic-policy digest, setup digest, optional canonicalized A/B digests, vector of `(artifact_slot, artifact_digest)` sorted by slot, then descriptor digest. The top-level framing follows Section 5.3. Options and platform facts similarly use their declaration/list order; write golden bytes in tests to make that order normative.

The repaired MD5 wrapper produces the initial fixed-size digest because it is already built in-tree. Verify standard MD5 vectors with optimized and sanitizer builds. Never treat digest equality as proof of geometric equality; when interning entities, compare full normalized keys.

## 10. Stage transactions and immutable publication

### 10.1 Artifact graph, stores, and handles

Define this closed schema-v1 `artifact_slot : uint8_t` graph:

| slot | producing stage | prerequisite slots | bundle contents/owner |
|---|---|---|---|
| `validated_operands=0` | input validation | none | both A and B validated-operand records and all original-feature stores; Component 2 |
| `candidate_stream=1` | broad phase | validated operands | canonical candidates; Component 4 |
| `raw_event_set=2` | intersection events | validated operands, candidate stream | all raw events; Component 5 |
| `symbolic_complex=3` | symbolic registry | raw events | symbolic vertices/curves and incidence/order stores; Component 6 |
| `refined_facet_patches=4` | local refinement | validated operands, symbolic complex | every affected facet's arrangement and patch stores as one bundle; Component 7 |
| `arrangement_complex=5` | global arrangement | refined patches | stitched halfedges/patches/incidence; Component 8 |
| `labeled_arrangement=6` | cell classification | arrangement complex | cells and side labels; Component 9 |
| `selected_exact_boundary=7` | Boolean selection | labeled arrangement | selected oriented exact patches; Component 10 |
| `realized_geometry=8` | geometry realization | selected exact boundary | certified coordinate representatives/certificate; Component 11 |
| `assembled_output=9` | output assembly | selected boundary, realized geometry | immutable candidate `fv_surface_mesh`; Component 12 |
| `final_verification=10` | final verification | assembled output and all prior reports | final report/certificate authorizing public return; Component 13 |

A stage may build many constituent stores, but it publishes exactly one typed artifact bundle to its slot. Component 2 therefore cannot expose operand A while operand B is still private, and local/global arrangement code cannot expose one constituent store at a time. Later component plans define concrete bundle records while preserving this graph. A failed prerequisite prevents opening a dependent transaction.

Input embeddedness validation may need self-pairs before `validated_operands` exists. Component 2 must initially enumerate all relevant nonadjacent feature pairs exhaustively in canonical order inside its private validation transaction, so its implementation does not depend on Component 4. Once Component 4 exists, the same private validation path may optionally use its conservative query primitive only if exhaustive differential tests prove identical validation outcomes. These pairs are validation evidence, not the cross-operand `candidate_stream`, receive no published candidate IDs, and disappear on rollback. The published broad-phase stage still consumes only fully validated operands.

Implement `published_store<Id, Record>` as an immutable building block over a dense vector. It exposes:

- invocation identity and producing stage;
- record count and canonical artifact digest;
- bounds-checked lookup by the matching strong ID;
- ordered iteration in ID order;
- no mutating iterator or mutable record reference.

The store owns its records, so lookups remain valid while the store is alive. An ID alone is not an owning handle. Implement `published_artifact<Artifact>` as the immutable owner of one complete stage bundle, its constituent stores, report, and digest. Both carry `context_owner_token`; reject wrong-stage/wrong-context attachment by that token rather than by digest.

### 10.2 Transaction lifecycle

Implement move-only `stage_transaction<ArtifactDraft, Artifact>` with states `open`, `verified`, `published`, and `failed`. `ArtifactDraft` owns all private constituent-store builders and allocator reservations for the slot. Its exact lifecycle is:

1. The context opens it for one declared stage and reserves private storage.
2. Workers append only to private shards owned by that transaction.
3. The coordinator joins shards, canonicalizes all keys, merges proven duplicates, assigns IDs, freezes every constituent store into one candidate artifact bundle, and computes its digest.
4. A `verifier_service` accepts an immutable candidate-artifact view plus a versioned `verification_spec`. It returns `status_or<verification_report>` containing owner token, stage, slot, artifact digest, verifier/schema version, the exact invariant-code set requested and checked, pass/fail result for each invariant, sorted feature evidence, and evidence digests. Resource exhaustion returns `resource_limit`; a failed invariant returns the category assigned by its spec and no certificate.
5. On a passing complete report, the context privately mints `verification_certificate` bound to report digest, owner token, stage, slot, artifact digest, invariant-set digest, and verifier version. Component 1 supplies structural specifications/checkers for its own generic bundles; Component 13 adds artifact-specific specifications/checkers through this same interface rather than replacing it.
6. `publish(certificate)` checks all bindings, required invariant-set equality, cancellation, limits, complete diagnostics, prerequisite slots, and transaction state. It atomically installs one complete `published_artifact<Artifact>` in the slot and transfers all reservations to committed accounting.
7. Any error, exception, cancellation, failed verification, bad certificate, duplicate publication, or transaction destruction before publication releases private reservations and exposes no constituent store.

Only the context can create a valid certificate after accepting a verifier report; public constructors are unavailable. Reusing a certificate for a changed artifact, stage, context, invariant set, report, or record count returns `internal_invariant_error`. Publishing an unverified artifact is impossible through the typed API and also checked at runtime.

The context permits at most one publication for each artifact slot. Downstream stages receive `shared_ptr<const published_artifact<...>>`, never the transaction. All fallible checks, allocation, digesting, report construction, and observer-independent diagnostics occur before a single publication linearization point under the context lock. At that point, slot installation and accounting reservation-to-commit transfer are prevalidated, nonthrowing, and indivisible. Canonical ordering is established beforehand, and observer notification occurs afterward, so lock acquisition/completion order cannot affect IDs or leave a half-committed artifact.

`final_verification` is the sole special finalization slot and is not recursively verified. Component 13 invokes the verifier service on `assembled_output` with the mandatory final specification. On a passing complete report, the context validates the report binding and invariant-set digest, privately mints the final certificate, and atomically installs the report plus a finalization marker at the normal publication linearization point. That certificate authorizes copying/moving the already immutable assembled mesh into the public success result. A failed/incomplete final report installs nothing and never exposes the mesh publicly.

### 10.3 Fault injection

Add a test-only fault-injection interface, compiled without behavior unless explicitly installed by a test. Name stable points at reservation, worker join, canonicalization, duplicate merge, verification, and pre-publication. The publication linearization point itself is deliberately not injectable because it contains no fallible operation. Provide one post-publication observer-notification point. Injected failures use deterministic subcodes. Tests must prove that every point before publication leaves the context with no visible artifact and no committed artifact/storage charge; canonical failure diagnostics/traces may remain and retain their separately charged monotonic accounting. A post-publication observer failure may only affect nonauthoritative notification and must not unpublish data.

## 11. Implementation sequence

Implement in this order so every layer has tests before the next depends on it:

1. Add namespace, version constants, operation/occupancy enums, side convention, solid/verification/trace/realization policies, and compile-time truth table.
2. Add `strong_id`, all required ID aliases, operand-qualified refs, explicit comparators, invalid-ID checks, and canonical encoding primitives.
3. Add `status_or`, stage/error/resource enums, structured `boolean_error`, feature-ref variant, and deterministic human rendering.
4. Add platform-fact capture, supported-specialization traits, strict-floating-point build guards, options validation, and canonical options serialization.
5. Add safe digest wrapper and input/replay descriptor generation.
6. Add checked arithmetic, resource limits/accountant, cancellation source/token, and deterministic stage-local ledgers.
7. Add diagnostic and trace collectors with canonical merge and optional observer adapters.
8. Add canonical ID factory, immutable published stores, verification certificates, stage transactions, publication slots, and fault injection.
9. Add the checked `make_boolean_context` factory, immutable operand/service views, deterministic executor, and four explicit template instantiations.
10. Integrate the standalone test target/script, CTest target, and authoritative CI execution. Run under release (`NDEBUG`) as well as debug because publication checks must not depend on assertions.

## 12. Validation and tests

### 12.1 Operation contract

- Exhaustively test all five operations over `(false,false)`, `(false,true)`, `(true,false)`, and `(true,true)` against the formulas in Section 4.
- Test ordered-difference operand swapping and noncommutativity.
- Test side selection: equal side occupancy rejects a patch; differing occupancy selects and requests the orientation with result interior on the negative side.
- Use synthetic side labels only to test that empty/equal/nested/tangent/coplanar/disconnected cases have no hidden operation-specific override beyond occupancy and regularization. Assign executable geometric coverage to Components 2 and 7-10 and the Component 14 end-to-end corpus; Component 1 does not claim to prove geometry with these tests.
- Compile-time assert every enum-to-table mapping and reject invalid deserialized operation values.

### 12.2 IDs and deterministic publication

- Static-assert that unlike IDs are not constructible, assignable, or comparable to each other or to integers.
- Test valid/invalid sentinel behavior, upper-bound exhaustion, fixed-width encoding, feature-ref ordering, and operand-qualified ordering.
- Feed the same provisional key set in forward, reverse, shuffled, differently allocated, and differently sharded/threaded orders. Require identical dense IDs, payload order, artifact digest, diagnostics, and serialized bytes.
- Test equal-key merge success and conflicting duplicate rejection without publication.
- Run serialization golden vectors on little-endian hosts and byte-swap simulation to prove host-independent bytes. Include signed zero, min/max finite float/double, index bounds, empty vectors, and malformed/noncanonical input.

### 12.3 Options, platform, and replay

- Instantiate and run context setup for all four supported `<T,I>` pairs. Add compile-time negative trait tests for unsupported coordinate/index types.
- Inject unsupported IEC/radix/width/rounding/fast-math platform facts and require `unsupported_platform` with setup stage and replay metadata.
- Test every invalid or contradictory option independently, including zero threads, invalid enum bytes, inconsistent trace/resource budgets, and uncertified realization combinations.
- Verify canonical options/replay round trips, unknown-version rejection, unknown-enum rejection, trailing-byte rejection, and stable known-byte/digest vectors.
- Verify input digests change for operation-relevant vertex/face/index/order changes, distinguish operand roles and signed zero, and ignore normals, colours, `involved_faces`, and metadata as documented.

### 12.4 Errors, accounting, and cancellation

- Test value/error alternatives, move-only values, successful empty mesh representation, and attempted accessor misuse.
- Construct every public error code and require stage, stable sorted feature refs, deterministic rendering, and replay metadata.
- Test checked add/multiply/conversion at zero, exact limit, one over limit, and integer maximum. No counter may wrap.
- Test reserve/commit/release, aggregate and per-kind limits, transaction rollback, entity-ID exhaustion, trace/diagnostic exhaustion, and converted allocation failure.
- Race deterministic worker ledgers in varied schedules and require the same canonical resource failure and counters.
- Cancel before setup, between stages, during worker work, before verification, and immediately before publication. Require no partial artifact and a structured cancellation resource subcode.

### 12.5 Diagnostics, transactions, and lifetime

- Publish worker diagnostics from varied thread/shard order and require identical canonical order and bytes.
- Verify observer/logger callbacks cannot mutate authoritative records; throwing callbacks are isolated and deterministically disabled.
- Exercise every transaction state transition and reject verify-after-failure, publish-before-verify, duplicate publish, wrong certificate, wrong invocation, wrong stage, changed artifact, and stale certificate.
- Inject each fault point and prove no private records, provisional IDs, charges, or diagnostics leak into published stores.
- Verify published stores remain readable after transaction destruction and expose only const records.
- Destroy a context only after all worker activity is joined; use ASan/TSan runs to check borrowed operand and service lifetime assumptions. Add a debug lifetime guard where practical, while retaining the documented caller precondition in release.
- Repeat publication and replay tests with thread budgets `1`, `2`, and a larger available count and require byte-identical canonical artifacts.

### 12.6 Build verification

- Build with GCC and Clang in C++17 mode with warnings enabled.
- Run the contract test in debug and release/NDEBUG configurations.
- Run ASan/UBSan and TSan variants for transaction/accounting/cancellation tests.
- Inspect compiler command lines and add a test/guard proving the new sources are not compiled with effective fast-math semantics.
- Register the test with CTest and run it explicitly in CI without `|| true`; make `tests/compile.sh` compile it in the foreground or capture/wait for its PID so another background compile cannot mask failure.
- Do not require network access or a newly downloaded testing dependency.

## 13. Completion criteria

Component 1 is complete only when all of the following hold:

- all five regularized truth tables and the result-side orientation rule are centralized, frozen, and exhaustively tested;
- the only accepted initial specializations and platform assumptions are explicit and checked;
- every required entity domain has a strong, canonically serialized ID and deterministic assignment path;
- setup and every expected failure use checked results with stage and replay metadata; empty success is unambiguous;
- options contain no geometric tolerance and cannot select uncertified behavior;
- accounting cannot wrap, every covered authoritative allocation is checked before allocation/publication, excluded runtime overhead is documented, and allocation failures roll back;
- cancellation and every injected stage failure leave no partial published artifact;
- diagnostics, traces, replay descriptors, IDs, and publication are independent of allocation, hash iteration, task partition, and thread schedule;
- stores are immutable after publication and publication requires a matching verification certificate in release builds;
- the context's borrowed-input and service lifetimes are documented and tested;
- tests pass for all four supported `<T,I>` combinations, strict C++17 compilation, release checks, and sanitizer configurations;
- public headers contain concise contract documentation sufficient for later component teams to use the APIs without consulting legacy Boolean implementations.
