# Plan 17: Performance and Deterministic Concurrency

## 0. Scope and fixed V1 design

Implement **only Component 17** from `component_17_performance_deterministic_concurrency.md`. Component 17 supplies the bounded execution service, immutable execution plans, task-adapter contract, deterministic partitioning, task-private output ownership, canonical merge and reduction utilities, worker floating-environment qualification, cancellation and failure coordination, structural instrumentation, replay evidence, and qualification seams used by Components 02-16.

Component 17 must not change any geometric, topological, symbolic, selection, cleanup, canonicalization, verification, or publication semantics established by Components 01-16. Its serial semantic references are authoritative. A parallel provider is conforming only when the same frozen input, policy, provider versions, and hard resource ceilings produce the same authoritative artifact or typed primary failure, the same retained canonical findings, the same precision witnesses, the same deterministic reports, the same replay content, and the same canonical digests as the serial reference.

Do not call, adapt, copy from, or depend on `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`.

Freeze V1 as follows:

```text
execution_service:                 bounded_fifo_worker_service_v1
worker_lifetime:                   invocation_owned_joinable_pool_v1
worker_selection:                  qualified_hardware_cap_then_descending_retry_v1
worker_fallback:                   destroy_partial_then_retry_smaller_v1
task_descriptor:                   immutable_adapter_key_range_v1
task_storage:                      canonical_rank_indexed_slots_v1
queue:                             bounded_fifo_task_rank_queue_v1
queue_admission:                   canonical_submit_block_or_cancel_v1
nested_parallelism:                serial_nested_fallback_v1
partitioning:                      canonical_contiguous_ranges_v1
grain_selection:                   versioned_logical_size_thresholds_v1
task_output:                       exclusive_preassigned_slot_v1
completion_tracking:               nonsemantic_bitmap_and_count_v1
merge:                             canonical_task_rank_then_record_key_v1
duplicate_handling:                adapter_declared_closed_policy_v1
prefix_sum:                        checked_serial_canonical_order_v1
integer_reduction:                 checked_serial_canonical_order_v1
maximum_reduction:                 bounded_value_plus_canonical_witness_v1
set_reduction:                     sorted_full_key_union_v1
failure_arbitration:               component01_total_key_after_join_v1
cancellation:                      cooperative_checkpointed_transactional_v1
terminal_hint:                     atomic_nonsemantic_work_suppression_hint_v1
floating_environment:              establish_verify_restore_each_worker_v1
resource_admission:                component01_reservation_before_queue_v1
execution_plan_codec:              component01_canonical_bytes_v1
execution_report_codec:            component01_canonical_bytes_v1
execution_digest:                  component01_sha256_domain_separated_v1
logical_trace:                     canonical_phase_task_event_v1
structural_gates:                  deterministic_counter_envelopes_v1
serial_reference:                  adapter_owned_executable_serial_v1
```

V1 deliberately uses a simple bounded FIFO queue and a canonical-rank-indexed output table. It does **not** use work stealing, task priorities, runtime subdivision, detached threads, per-stage child pools, concurrent mutation of global topology, arrival-order interning, completion-order publication, or schedule-ordered floating accumulation. These may be considered only under a new provider version after the serial-equivalence, progress, resource, replay, and mutation requirements in this plan are satisfied.

Mark Component 17 complete in `tracker.md` only after every requirement below is represented by an exact implementation instruction and qualification gate. The tracker mark records planning completion, not implementation completion.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 `YgorThreadPool.h::work_queue` is not suitable

Preserve `src/YgorThreadPool.h` for existing unrelated callers. Do not use it in the bounded Boolean subsystem and do not broaden its public contract as part of Component 17.

Its current behavior is incompatible because it:

- stores tasks in an unbounded `std::list`;
- admits work without Component 01 resource reservation or backpressure;
- defaults `0` workers through `hardware_concurrency()` and then silently substitutes two workers;
- catches `std::exception` and discards the exception and all witnesses;
- provides no typed failure, cancellation, transaction, task identity, owner token, immutable plan, or canonical merge;
- makes FIFO queue order an implementation behavior without separating it from semantic order;
- has no task-private output schema or exactly disjoint write contract;
- exposes no worker floating-environment establishment or verification;
- has no deterministic primary-failure arbitration;
- has no nested-parallelism budget or progress proof;
- waits in its destructor using polling and queue emptiness rather than an explicit admitted/running task state;
- provides no deterministic structural counters, reports, replay metadata, or verifier; and
- cannot prove that a queue-empty observation means all worker-owned work has completed.

Do not incrementally retrofit this template. Retrofitting would either break existing users or leave Boolean correctness dependent on a generic interface that cannot express the required adapter, ownership, resource, and merge contracts.

### 1.2 `YgorContainers.h::taskqueue` is not suitable

Preserve `taskqueue` for unrelated code. Do not use it as a worker service, serial reference, nested executor, or test scheduler.

It uses one asynchronous thread, mutable list state, manually locked mutexes, logging-based error handling, long-held locks that may block indefinitely, callbacks executed under mutable synchronization state, and no bounded resource, cancellation, transaction, canonical output, floating environment, deterministic error, or replay contract. Its launch and queue behavior is inappropriate for authoritative geometry work.

### 1.3 Other existing threading uses

Existing ad hoc `std::thread`, logging, networking, plotting, or convex-hull concurrency code is non-authoritative. It may provide ordinary C++ usage examples only. Do not reuse any routine unless it is first isolated behind the Component 17 adapter contract and passes the complete qualification matrix. No currently identified routine satisfies that condition.

### 1.4 Standard-library reuse

Use only portable C++17 standard-library facilities:

- `std::thread`;
- `std::mutex`;
- `std::condition_variable`;
- `std::atomic`;
- `std::vector`;
- `std::array`;
- `std::deque` only if its bounded use is proven and hidden;
- `std::unique_ptr`;
- `std::optional`;
- `std::variant`;
- `std::sort`;
- `std::lower_bound`;
- fixed-width integer types; and
- `<cfenv>` through the Component 01/03 qualified wrapper.

Do not use C++20 facilities, compiler task extensions, parallel STL execution policies, OpenMP, TBB, Boost, platform pools, GPU APIs, external allocators, external concurrent containers, external profilers, or downloaded benchmark frameworks.

### 1.5 Mandatory predecessor reuse

Reuse without duplication:

- Component 01 immutable context, normalized execution policy, strong IDs, owner tokens, checked arithmetic, typed outcomes/errors, resource ledger/reservations/leases, cancellation source/token/checkpoints, diagnostics, deterministic finding arbitration, replay framing, canonical bytes, SHA-256, transaction state machine, and immutable artifact publication;
- Component 03 platform qualification, strict floating-point contract, exact scalar bits, bounded values, precision records, and maximum-with-witness comparison semantics;
- each component's complete canonical keys, immutable artifact views, serial producer, independent verifier, structural counters, and transaction boundaries;
- Component 14 execution-report handoff fields and canonical serialization;
- Component 15 independent report and publication audit; and
- Component 16 qualification registry, forced schedule/fault hooks, deterministic campaigns, mutation framework, structural performance fixtures, evidence bundles, and release decision.

Do not create a second cancellation flag API, resource ledger, error order, canonical serializer, digest provider, replay container, floating-point qualification provider, or transaction framework.

### 1.6 Greenfield work required

Add a new execution subsystem under `src/YgorMeshesBooleanBounded/`. The subsystem is greenfield because the existing generic queues cannot satisfy bounded admission, immutable plan validation, private output ownership, deterministic failure arbitration, strict worker floating state, transaction integration, or canonical reports.

The new subsystem may share semantics-free checked helpers with predecessor modules only when the helper has:

- no hidden mutable state;
- no worker-, address-, or timing-dependent output;
- complete owner and range validation where relevant;
- typed failure;
- deterministic tests; and
- no coupling to a particular stage's producer control flow.

## 2. Exact files, targets, and integration points

### 2.1 Production files

Add under `src/YgorMeshesBooleanBounded/`:

- `ExecutionTypes.h` — closed enums, strong IDs, canonical keys, task/range descriptors, counter categories, fixed constants, result records, and failure payloads.
- `ExecutionPolicy.h/.cc` — Component 17 capability validation, actual-worker selection, deterministic fallback rules, stage permission checks, threshold lookup, and normalized execution settings.
- `ExecutionAdapter.h` — non-owning immutable adapter interface, serial-reference contract, task dispatch vtable, output schema contract, duplicate policy, merge hooks, and verifier hook.
- `ExecutionAdapterRegistry.h/.cc` — explicit versioned registration and lookup; no static constructor registration.
- `ExecutionPlan.h/.cc` — immutable plan artifact, canonical partition creation, coverage proof, plan validation, canonical codec, and digest.
- `ExecutionPartition.h/.cc` — checked contiguous-range partitioner, deterministic grain rules, remainder handling, task-count limits, and threshold policy.
- `ExecutionTaskStorage.h/.cc` — preassigned descriptor and private-output slots, completion/failure state, owner tokens, exact range ownership, and destruction checks.
- `BoundedTaskQueue.h/.cc` — bounded FIFO of canonical task ranks, admission backpressure, cancellation-aware waits, wakeup, close, discard, and invariant inspection.
- `BoundedWorkerService.h/.cc` — invocation-owned joinable worker pool, creation retries, worker loop, floating-environment setup, task dispatch, exception capture, shutdown, reuse rules, and join verification.
- `ExecutionStage.h/.cc` — fixed stage orchestration, transaction integration, plan verification, resource reservation, submission, join, merge, independent stage verification, commit, and rollback.
- `ExecutionMerge.h/.cc` — canonical task-output collection, complete-key sorting, duplicate/conflict processing, task-local ID remapping, and merged proposal construction.
- `DeterministicReduction.h/.cc` — checked integer sums, booleans, key minima/maxima, maximum-with-witness, sorted set union, ordered block concatenation, resource summaries, and finding collection.
- `CheckedPrefixSum.h/.cc` — canonical serial checked prefix sums, exact range assignment, representability checks, and count/fill reconciliation.
- `WorkerFloatingEnvironment.h/.cc` — worker establishment, verification, restoration, disturbance detection, and evidence records using Component 01/03.
- `ExecutionCancellation.h/.cc` — stage-local terminal hint, deterministic checkpoint observations, queue wake integration, suppression policy validation, and canonical cancellation evidence.
- `ExecutionCounters.h/.cc` — counter registry, semantic/plan/bounded/non-authoritative categories, checked accumulation, high-water tracking, and schema validation.
- `ExecutionReport.h/.cc` — deterministic per-stage and per-call report, canonical ordering, validation, codec, and digest.
- `ExecutionLogicalTrace.h/.cc` — bounded logical trace events canonicalized by stage/phase/task key; raw timing trace remains optional and non-authoritative.
- `ExecutionReplay.h/.cc` — provider versions, requested/actual workers, limits, plan digests, forced-test settings, cancellation checkpoint, encode/decode, and compatibility checks.
- `ExecutionStructuralGates.h/.cc` — deterministic structural envelope evaluation and canonical failure witnesses.
- `ExecutionVerifier.h/.cc` — independent validation of service state, plans, slots, queue accounting, joins, resource reconciliation, counters, reports, and replay.
- `ExecutionQueries.h` — narrow immutable owner-checked views for Components 02-16, diagnostics, replay, and qualification.
- `ExecutionAdaptersComponent04.cc`.
- `ExecutionAdaptersComponent06.cc`.
- `ExecutionAdaptersComponent07.cc`.
- `ExecutionAdaptersComponent08.cc`.
- `ExecutionAdaptersComponent09.cc`.
- `ExecutionAdaptersComponent10.cc`.
- `ExecutionAdaptersComponent11.cc`.
- `ExecutionAdaptersComponent12.cc`.
- `ExecutionAdaptersComponent13.cc`.
- `ExecutionAdaptersComponent14.cc`.
- `ExecutionAdaptersComponent15.cc`.

Component 16's scheduler adapter is test-only and belongs under the qualification tree. Component 02 and Component 05 remain serial in V1 unless a future adapter is separately specified and qualified.

Extend:

- `ContractVersions.h` with explicit nonzero V1 execution service, policy, adapter, plan, partition, queue, slot, merge, reduction, floating-environment, counter, report, trace, replay, verifier, stage-inventory, and structural-gate versions;
- Component 01 stage/checkpoint/error-subcode/resource-kind/replay-field registries;
- `Context.h/.cc` with an immutable `execution_capabilities` view and one invocation-owned `execution_service_handle`;
- `Transaction.h/.cc` only as needed to register active worker ownership and reject commit with admitted/running/unjoined work;
- `Policies.h/.cc` only to validate Component 17 capability compatibility without changing frozen caller values;
- `YgorMeshesBooleanBounded.h` only for public execution-policy/report fields already promised by Plan 01 and final result reporting;
- Component 14's assembled candidate to carry the deterministic execution report and provider/version evidence;
- Component 15's verification intake to audit execution claims affecting publication; and
- the strict C++17 bounded Boolean object target and explicit instantiation lists.

### 2.2 Test files

Add under `tests/mesh_boolean_bounded/`:

- `TestExecutionPolicy.cc`;
- `TestExecutionPartition.cc`;
- `TestExecutionPlan.cc`;
- `TestBoundedTaskQueue.cc`;
- `TestBoundedWorkerService.cc`;
- `TestExecutionTaskStorage.cc`;
- `TestExecutionMerge.cc`;
- `TestDeterministicReduction.cc`;
- `TestCheckedPrefixSum.cc`;
- `TestWorkerFloatingEnvironment.cc`;
- `TestExecutionCancellation.cc`;
- `TestExecutionResources.cc`;
- `TestExecutionFailureArbitration.cc`;
- `TestExecutionNestedFallback.cc`;
- `TestExecutionTransactions.cc`;
- `TestExecutionCounters.cc`;
- `TestExecutionReports.cc`;
- `TestExecutionReplay.cc`;
- `TestExecutionLogicalTrace.cc`;
- `TestExecutionStructuralGates.cc`;
- `TestExecutionAdapters.cc`;
- `TestExecutionSerialEquivalence.cc`;
- `TestExecutionForcedSchedules.cc`;
- `TestExecutionDeadlockStress.cc`;
- `TestExecutionRaceStress.cc`;
- `TestExecutionExceptionSafety.cc`;
- `TestExecutionMemoryHighWater.cc`;
- `TestExecutionMutation.cc`;
- `TestExecutionProperties.cc`;
- `TestExecutionAdversarial.cc`;
- `ExecutionFixtures.h/.cc`;
- `ExecutionTestAdapter.h/.cc`;
- `ExecutionFaultInjection.h/.cc`; and
- `GoldenExecutionV1.h`.

Register separate CTest cases for unit contracts, queue/service, plans/partitions, merge/reductions, floating environment, cancellation/resources, adapters, serial equivalence, forced schedules, transactions, deadlock/race stress, exception safety, replay/report bytes, structural performance, mutation, properties, and adversarial limits. Apply `ygor_apply_mesh_boolean_strict_fp` to every authoritative production and test target.

### 2.3 Qualification integration

Under `tests/mesh_boolean_bounded/qualification/` add:

- `Component17Qualification.cc`;
- `Component17TestAdapter.h/.cc`;
- Component 17 clause inventory entries;
- closed worker-count, queue-capacity, partition, cancellation, resource, fault, and provider matrices;
- structural envelope fixtures; and
- mutation registrations for every prohibited behavior in Section 20.

No test depends on OS scheduling to produce an expected order. Forced schedules use deterministic barriers, gates, and task-key-controlled delays exposed only under `YGOR_MESH_BOOLEAN_QUALIFICATION_BUILD=1`.

## 3. Stable versions, enums, keys, and failure subcodes

### 3.1 Closed enums

Define explicit fixed-width enums with zero invalid:

- `execution_service_kind`;
- `worker_selection_kind`;
- `worker_fallback_kind`;
- `queue_kind`;
- `queue_admission_kind`;
- `nested_execution_kind`;
- `partition_kind`;
- `grain_policy_kind`;
- `task_storage_kind`;
- `merge_kind`;
- `duplicate_key_policy`;
- `reduction_kind`;
- `execution_stage_status`;
- `execution_task_status`;
- `terminal_hint_kind`;
- `counter_category`;
- `logical_trace_phase`;
- `stage_parallel_capability`;
- `structural_gate_disposition`; and
- `worker_environment_status`.

Use explicit serialized values and compile-time uniqueness checks. Unknown required values, zero values in published records, duplicate singleton fields, incompatible combinations, and nonzero reserved V1 fields are typed failures.

### 3.2 Strong identities

Add owner-bound strong IDs for:

- `execution_plan_id`;
- `execution_task_id`;
- `execution_adapter_id`;
- `execution_output_slot_id`;
- `execution_report_id`;
- `execution_trace_event_id`; and
- `structural_gate_id`.

Task IDs are assigned from canonical task rank only after plan validation. They are not allocated by workers and never derive from thread ID, queue position, pointer, address, submission timing, or completion timing.

### 3.3 Canonical task key

Use a complete key:

```cpp
struct execution_task_key {
    component_id component;
    stage_id stage;
    execution_adapter_id adapter;
    std::uint32_t adapter_version;
    std::uint32_t partition_version;
    canonical_entity_domain domain;
    canonical_entity_key first;
    canonical_entity_key last_exclusive;
    std::uint64_t subdivision_rank;
    std::uint64_t policy_discriminator;
};
```

`canonical_entity_key` is an adapter-owned exact key type encoded into canonical bytes or a bounded fixed tagged tuple. It must include every field needed for a total order. The range endpoints are logical keys or checked canonical ranks, not memory addresses or iterators.

The comparator orders every field explicitly. Equality means identical complete keys, not hash equality. The encoder rejects malformed endpoints, mixed owners, unknown domains, and nonzero reserved fields.

### 3.4 Output-record key contract

Every adapter declares a complete output-record key and one duplicate policy:

1. `unique_required` — duplicate is `internal_invariant_error`;
2. `equivalent_coalesce` — all normative fields must compare equal before one canonical record remains;
3. `explicit_multiset_rank` — the occurrence rank is part of the key;
4. `deterministic_proposal_choice` — all proposals are retained until a documented complete selection rule chooses the serial-equivalent result.

No adapter may use an incomplete comparator and rely on `std::stable_sort` to preserve worker completion order.

### 3.5 Failure subcodes

Reserve Component 17 subcodes for at least:

- unknown execution/provider/adapter/plan/counter/replay version;
- unsupported policy combination;
- invalid requested or maximum worker count;
- worker capability mismatch;
- worker creation resource failure;
- deterministic fallback forbidden;
- partial worker creation rollback failure;
- queue capacity invalid;
- queue closed during admission;
- queue accounting mismatch;
- task descriptor malformed;
- duplicate task key;
- task range omitted/overlapped;
- task count limit;
- descriptor/output reservation failure;
- task owner mismatch;
- task write outside slot/range;
- task-local identity escaped;
- task exception mapped;
- unexpected worker exception;
- worker floating-environment setup/verification/restore failure;
- cancellation checkpoint mismatch;
- unsafe early suppression;
- nested parallelism forbidden;
- prefix overflow;
- count/fill mismatch;
- missing/duplicate/conflicting output record;
- incomplete comparator;
- reduction malformed/overflow/NaN/negative bound;
- maximum witness mismatch;
- primary failure mismatch;
- counter category mismatch;
- structural gate failure;
- report mismatch;
- replay mismatch;
- logical trace overflow/truncation mismatch;
- active or unjoined work at commit;
- reservation leak;
- worker/service shutdown invariant;
- deadlock watchdog infrastructure failure in tests; and
- serial-equivalence failure.

Map expected worker creation, allocation, reservation, cancellation, unsupported platform, and hard work-limit cases to their documented public categories. Reserve `internal_invariant_error` for contradictions and implementation defects.

## 4. Public and internal execution policy

### 4.1 Effective worker selection

Component 01 preserves the caller's `requested_workers` syntax. Component 17 computes `actual_workers` during service creation.

For V1:

1. In `serial_v1`, require one invoking-thread execution lane and create zero background threads.
2. In `deterministic_parallel_v1`:
   - require `maximum_workers >= 1`;
   - query `std::thread::hardware_concurrency()` once during service preflight;
   - treat zero as one qualified logical processor;
   - if `requested_workers != 0`, set the desired count to `min(requested_workers, maximum_workers)`;
   - if `requested_workers == 0`, set it to `min(maximum_workers, max(1, hardware_concurrency))`;
   - reject a caller request above the maximum unless Component 01 explicitly froze a permitted normalization record;
   - cap the desired count by the hard worker resource ceiling; and
   - record requested syntax, hardware result, desired count, actual count, fallback attempts, and capability version.

Background worker count equals `actual_workers`. The invoking thread is not counted as a worker in reports. Serial mode has `actual_workers == 0` and `execution_lanes == 1`. Parallel mode may have one background worker and one submitting thread, but only background workers execute V1 tasks.

### 4.2 Deterministic creation fallback

If the complete desired pool cannot be created:

1. stop admission;
2. signal every successfully created partial worker to exit before any stage task exists;
3. join all partial workers;
4. reconcile all worker-control reservations;
5. discard all partial service state;
6. if fallback is forbidden, return `resource_limit`;
7. otherwise retry with `desired - 1`;
8. continue down to one background worker;
9. if one worker cannot be created, return `resource_limit`; and
10. never keep a partially created pool.

The chosen fallback count must not depend on which particular creation attempt failed. Report every attempted count canonically. Do not fall back after stage work has begun.

### 4.3 V1 nested policy

V1 uses `serial_nested_fallback_v1`.

A worker that calls a parallel-capable stage through the same invocation context must execute that nested logical stage synchronously through its executable serial reference. It may not submit child tasks, create a child pool, or wait while occupying a worker for work that needs the same bounded pool. The nested serial path uses the same resource ledger, cancellation token, floating contract, error order, reports, and transaction semantics.

Caller code outside a worker may run one stage at a time. Concurrent stages within one Boolean invocation are not supported in V1.

### 4.4 Small-work threshold

Each adapter declares a versioned minimum logical work size. Below the threshold, execute the serial reference. Threshold inputs may include only:

- canonical logical item count;
- adapter version;
- frozen execution policy;
- fixed provider constants; and
- hard resource ceilings.

Do not use elapsed time, observed queue depth, worker speed, previous invocation history, or allocation timing. Record the serial/parallel decision and threshold version as plan statistics.

### 4.5 Stage permissions

The execution policy contains a closed set of permitted parallel stage kinds. An adapter not permitted by policy executes serially. Unknown permissions or an adapter claiming a stage not present in the stage inventory fail before worker activity.

## 5. Execution adapter contract

### 5.1 Required interface

Define a non-owning immutable `execution_adapter` descriptor containing:

- stable adapter ID and version;
- component and stage IDs;
- serial/parallel capability;
- logical domain kind;
- serial semantic reference function;
- logical item count function;
- canonical item/range key function;
- deterministic grain policy ID;
- private output slot size/alignment and conservative persistent-output bound;
- task entry function;
- cancellation checkpoint version;
- exception mapping function;
- complete output-record comparator;
- duplicate policy;
- merge function;
- deterministic reduction declarations;
- counter schema;
- independent merged-artifact verifier; and
- canonical plan/report/replay encoders.

Do not place arbitrary caller-owned `std::function` closures in the queue. Queue entries contain only a validated task rank. The service owns one immutable stage invocation object that contains the adapter vtable and owner-checked references to immutable inputs and preallocated output slots.

### 5.2 Serial semantic reference

Every adapter must provide an executable serial function that:

- traverses the complete logical domain in canonical order;
- uses the same producer semantics and bounded arithmetic as the parallel task path;
- constructs the same logical record multiset;
- applies the same duplicate policy;
- uses the same canonical merge/reduction functions or an independently equivalent serial implementation;
- emits the same semantic counters;
- observes cancellation at the same logical checkpoint version;
- applies the same failure arbitration;
- invokes the same independent stage verifier; and
- commits through the same transaction boundary.

The serial reference may use a single whole-domain task internally only when that does not hide task-range coverage tests. Normative bounded fixtures must be able to enumerate the reference records before final publication.

### 5.3 Immutable input contract

The stage invocation object may contain:

- const artifact handles;
- owner-checked const views;
- frozen policy values;
- immutable plan and task descriptors;
- read-only lookup tables;
- read-only precision/resource/cancellation service views; and
- test-only immutable fault-control tables.

It must not contain mutable caller meshes, mutable vectors extended by workers, mutable hash tables, mutable topology, mutable policy state, or references whose lifetime is shorter than the worker join boundary.

### 5.4 Private output contract

Allocate one output slot per canonical task rank before admission. Exactly one task invocation owns each slot. The slot stores:

- task key and owner token;
- status transition;
- private logical output records;
- private failures/findings;
- precision contributors and maxima candidates;
- semantic and plan counters;
- resource usage and high-water candidates;
- bounded logical trace events;
- task-local ID remap tables where required; and
- exact actual persistent-output bytes.

A worker may mutate only its assigned slot and thread-local worker state. Slots are not reused before stage destruction. Merge reads slots only after all admitted tasks are complete or canonically abandoned.

### 5.5 Exactly disjoint direct-fill exception

An adapter may use a two-pass exact direct-fill mode instead of record vectors only when:

- count pass outputs are private;
- checked prefix sums assign non-overlapping complete ranges;
- every task receives an immutable assigned half-open range;
- each task fills every assigned element exactly once;
- no task can write outside the range;
- range ownership is instrumented in tests;
- count/fill inputs and provider versions are identical;
- fill completion is verified before merge; and
- count/fill disagreement is an invariant failure.

Global IDs are assigned from canonical prefix offsets, never worker completion.

### 5.6 Exception contract

Task entrypoints are `noexcept` at the service boundary. The worker wrapper catches:

- `std::bad_alloc`;
- documented adapter typed exceptions, only if an existing API forces their use;
- explicit test-only injected exceptions; and
- all other exceptions.

Expected allocation failure maps to `resource_limit` with task and reservation witnesses. Unexpected exceptions map to `internal_invariant_error`. No exception escapes a worker, destroys a joinable thread, or becomes log-only output.

## 6. Stage inventory and V1 adapter boundaries

Maintain one explicit `StageExecutionInventoryV1` table registered by function call, never static initialization. Each row records capability, serial reference, partition/merge versions, floating arithmetic requirement, resource classes, checkpoints, counters, verifier, and Component 16 test IDs.

### 6.1 Serial-only stages in V1

Keep these serial:

- Component 01 context freeze and transaction commit;
- Component 02 final input topology/shell publication;
- Component 05 canonical halfedge publication;
- Component 08 final event identity assignment and global carrier ordering;
- Component 09 final connectivity quotient publication and winding propagation;
- Component 10 final occurrence partition publication;
- Component 11 paired output-edge assignment and face-cycle publication;
- Component 13 cleanup action selection and mutation application;
- Component 14 final canonical labeling, public index assignment, and facet serialization;
- Component 15 final status transition and ordinary-success publication; and
- all stage transaction commits.

A future parallel provider requires a new inventory and adapter version with exact non-interference and canonical merge proof.

### 6.2 Component 04 adapter

Parallelize only per-source-facet triangulation preparation/count/fill.

Logical key: canonical operand/shell/source-facet key.

Private output: validated projected facet data, triangle proposals, boundary/provenance records, local failures, precision evidence, counts, and counters.

Merge: canonical facet order, checked prefix sums for triangle IDs, complete provenance key sorting, and the existing Component 04 verifier.

Do not publish source triangle IDs from task completion order.

### 6.3 Component 06 adapter

Use canonical contiguous ranges of directed query edges.

Provide separate count and emit task kinds under one execution-plan family.

Private count output: candidate count, traversal counters, failures, and resource estimate.

Prefix merge: canonical query-edge task order with checked sums.

Private emit/direct-fill output: candidates in the assigned range, full overlap witnesses, and traversal counters.

Merge: validate exact fill, canonical candidate sort/dedup according to Plan 06, then invoke `BroadPhaseVerifier`.

The parallel provider must use exactly the same conservative bounds, domain policy, and traversal pruning as the serial semantic reference. No false negative is permitted.

### 6.4 Component 07 adapter

Logical key: canonical relation-key range.

Private output: one relation proposal per key, bounded evidence, symbolic decision, precision contributors, failure findings, and relation counters.

Merge: complete relation key; duplicate inconsistent evaluations are invariant failures; assign published relation IDs only after canonical order is fixed.

Do not choose alternate formulas by worker or schedule.

### 6.5 Component 08 adapter

Parallelize event-seed and consumer-reference derivation only.

Logical key: canonical relation/event-seed source range.

Private output: lineage-keyed seed proposals, construction proposals, consumer references, carrier parameter records, failures, and precision evidence.

Merge: group by complete lineage key, validate equivalent construction claims, retain distinct equal-coordinate lineage keys, assign event IDs canonically, and perform global carrier ordering serially.

Workers must not mutate a shared intern table.

### 6.6 Component 09 adapter

Parallelize primitive zero-delta/signed-delta edge records, independent seed query proposals, and component-local validation.

Private output: canonical graph-edge proposals, seed evidence, local findings, and counters.

Merge: canonical graph keys; construct union/connectivity and quotient identities through the Component 09 serial publication path. Any union-find implementation must canonicalize final groups independently of union order.

### 6.7 Component 10 adapter

Parallelize pure retained-surface-use proposal evaluation over canonical classified source-piece ranges.

Private output: truth-table decision, orientation, multiplicity proposal, coincident ownership evidence, local findings, and counters.

Merge: complete retained-use key, explicit multiplicity rank, canonical occurrence allocation, then run the Component 10 verifier.

Final occurrence partition and global IDs remain serial.

### 6.8 Component 11 adapter

Parallelize immutable carrier endpoint proposal generation, per-retained-face local boundary fragment preparation, and local balance prechecks.

Private output: full carrier/endpoint keys, start/end records, local cycle-edge proposals, findings, and counters.

Merge: canonical carrier grouping and ordering. Actual paired halfedge assignment and globally coupled face-cycle publication remain serial.

No task may claim that different carriers imply independent final topology without the existing Component 11 verifier.

### 6.9 Component 12 adapter

Logical key: canonical output polygon/face key.

Private output: deterministic triangulation proposal, local diagonals, preserved-boundary proof, residual-cell obligations, precision evidence, failures, and counters.

Merge: canonical face order, checked triangle prefix sums, local-to-global ID remapping, complete edge/provenance validation, and Component 12 independent verification.

One face failure participates in canonical failure arbitration; it must not return merely because it completes first.

### 6.10 Component 13 adapter

Parallelize only candidate discovery, immutable-generation eligibility prechecks, displacement-bound proposals, independent no-new-intersection searches, and certificate preparation.

Private output: action-keyed proposals and certificates against an explicit immutable generation ID.

Merge: canonical action key; reject stale/conflicting proposals; apply accepted actions serially in the Component 13 semantic order with complete budget and topology revalidation.

V1 prohibits concurrent topology mutation.

### 6.11 Component 14 adapter

Parallelize component descriptors, refinement proposals, branch encodings, provenance/report block preparation, and other immutable canonicalization support.

Private output: complete branch/component keys, canonical byte proposals, resource findings, and counters.

Merge: compare all required branches in canonical logical order. Select the global canonical minimum and assign public indices/facet order serially. The first completed branch is never privileged.

### 6.12 Component 15 adapter

Parallelize independent verification partitions specifically listed by Plan 15: public edge-use extraction, triangle checks, independent hierarchy construction subwork, non-adjacent pair classification blocks, event audits, cleanup-certificate checks, precision ledger block reductions, and report fragment generation.

Private output: verifier-owned facts/findings, complete pair/probe/ledger keys, bounded evidence, counters, and trace records.

Merge: canonical finding and fact order; reconstruct reports and select primary failure only after join. Check coverage may not shrink with worker count.

### 6.13 Component 16 test-only adapter

The qualification runner may execute independent test cases concurrently through a test-only adapter. Test descriptors are immutable, results are private, and evidence merge follows stable test ID. The serial qualification runner remains normative. Shrink search, corpus transaction, manifest publication, and release decision remain serial unless separately proven.

## 7. Execution-plan construction

### 7.1 Plan inputs

Build a plan from:

- frozen context and execution policy;
- adapter descriptor/version;
- immutable logical domain and digest;
- canonical item count;
- worker capability;
- threshold rule;
- queue/in-flight limits;
- task descriptor/output limits;
- work and byte limits;
- cancellation checkpoint version;
- counter schema; and
- test-only forced partition controls when enabled.

### 7.2 Deterministic contiguous partition

For V1, partition canonical ranks `[0, logical_count)` into contiguous half-open ranges.

1. If count is zero, produce a valid zero-task serial plan.
2. Determine grain from the adapter's fixed threshold table and logical count.
3. Compute `task_count = ceil(count / grain)` with checked arithmetic.
4. Reject if task count exceeds adapter, policy, ID, resource, or `size_t` limits.
5. For task rank `r`, set:
   - `begin = r * grain`;
   - `end = min(count, begin + grain)`.
6. Require `begin < end`.
7. Require the first range begins at zero.
8. Require each range begins at the preceding end.
9. Require final end equals logical count.
10. Assign subdivision rank equal to canonical task rank.
11. Derive complete task keys from adapter and range.
12. Sort-check keys and reject duplicates.

No wall-clock, queue, allocator, worker-speed, or runtime-contention input may influence the ranges.

### 7.3 Plan artifact

`execution_plan` contains:

- schema/provider versions;
- owner/context/stage/adapter IDs;
- logical domain kind/count/digest;
- requested and actual worker fields;
- serial/parallel disposition and reason;
- grain policy/version/value;
- queue and in-flight limits;
- task descriptors and complete keys;
- expected descriptor/private-output/merge/reduction bytes;
- expected work-unit upper bounds;
- resource reservation classes;
- cancellation checkpoint version;
- merge/reduction/counter versions;
- counter schema;
- test-only forced schedule descriptor, absent in production;
- canonical bytes and plan digest; and
- validation evidence.

The verifier independently checks complete coverage, no overlap, task-key order, resource arithmetic, worker-policy compatibility, and digest.

### 7.4 Plan authority

Plan bytes are authoritative for replay and reports in V1. Therefore the same logical input, adapter/provider versions, execution policy, actual worker count, and test controls must produce identical plan bytes. Different actual worker counts may produce semantically equivalent artifacts but distinct explicitly reported plan digests only where the grain policy declares worker-count dependence. Prefer worker-count-independent partitioning; V1 default grain rules must not use worker count.

## 8. Task storage and ownership

### 8.1 Slot state machine

Each slot follows:

```text
uninitialized
-> reserved
-> admitted
-> running
-> completed_success | completed_failure | abandoned_cancelled
-> merge_consumed
-> destroyed
```

Transitions use atomics only for synchronization and are verified against the plan. State timing is non-authoritative. Illegal transition is an invariant failure.

### 8.2 Preallocation

Before admission:

- reserve descriptor bytes;
- reserve output-slot control bytes;
- reserve conservative task-private persistent bytes or a bounded growth allowance;
- construct all descriptors in canonical rank order;
- construct all slot headers;
- validate alignment and offsets;
- bind every slot to one task key and owner token; and
- publish no pointer or reference outside the stage transaction.

Task-local vectors may grow only after an additional Component 01 reservation succeeds. A failed growth produces a private typed failure and no partial published record.

### 8.3 Completion accounting

Use:

- one atomic completed count;
- per-slot terminal state;
- one condition variable for join waits; and
- queue admitted/running counters.

The completed count accelerates waiting but is not sufficient proof. Join verifies every admitted slot terminal state and queue/service accounting. Completion order is never serialized.

### 8.4 Task-local identities

Adapters may allocate compact local IDs within a slot. The merge must map them using `(task_key, local_id, complete_record_key)` and assign public/internal artifact IDs only in canonical order. Any local ID found in a committed artifact is an invariant failure caught by adapter verifier and Component 17 mutation tests.

## 9. Bounded FIFO queue

### 9.1 Queue representation

Use a fixed-capacity ring buffer of task ranks allocated during service/stage preflight. Avoid a node-allocating list.

Store:

- vector of task ranks with fixed capacity;
- head, tail, and count;
- closed/admission-stopped state;
- terminal/cancellation wake flags;
- mutex;
- `not_empty`;
- `not_full`; and
- checked high-water count.

No queue element owns task payload or output storage.

### 9.2 Admission

The sole V1 submitter traverses task ranks in canonical order.

For each task:

1. recheck cancellation and stage terminal policy;
2. reserve/admit descriptor and output resources if not already held;
3. wait on `not_full` with a predicate covering capacity, closure, cancellation, and terminal hint;
4. hold no component or resource-ledger lock while waiting;
5. enqueue the task rank;
6. transition slot to admitted;
7. increment checked admitted/queue counters;
8. notify `not_empty`; and
9. continue.

Spurious wakeups are harmless because all waits use predicates.

### 9.3 Worker pop

Workers:

1. wait on `not_empty` for queue nonempty, closure, cancellation-safe shutdown, or terminal state;
2. pop exactly one rank FIFO;
3. decrement queue count;
4. increment running count;
5. transition the corresponding admitted slot to running;
6. notify `not_full`;
7. release queue lock; and
8. execute the task.

FIFO is a provider property, not an output-order guarantee.

### 9.4 Queue close and cancellation

Closing admission wakes all waiters. On cancellation or terminal failure:

- no new commit-producing task is admitted;
- already queued tasks are either executed to required checkpoints or canonically marked abandoned according to the adapter's suppression policy;
- every admitted task receives a terminal slot state;
- queue counters reconcile;
- all workers join or return to idle only after the stage has no admitted work; and
- no partial stage artifact publishes.

V1 defaults to draining required tasks needed for deterministic failure arbitration and resource evidence. Optional suppression is disabled unless the adapter supplies a monotonic proof accepted by qualification.

## 10. Worker service lifecycle

### 10.1 Ownership

One Boolean invocation owns one `bounded_worker_service`. The context stores a non-copyable handle. The service may be reused sequentially across stages but never outlives the invocation transaction bundle.

No static pool, global singleton, thread-local global service, detached helper, or process-shutdown dependency is allowed.

### 10.2 Worker startup

Each worker receives a stable nonsemantic worker slot index used only for private scratch ownership and diagnostics. It must not enter canonical keys.

At startup:

1. establish Component 01/03 floating environment;
2. run bit-pattern qualification probes;
3. initialize private scratch within reservations;
4. publish startup status to the service;
5. wait for stage work.

Service creation succeeds only after every worker reports qualified startup. A failure stops and joins the entire attempted pool before fallback or return.

### 10.3 Worker loop

For each popped task:

1. verify service/stage/plan/slot owner tokens and adapter version;
2. establish or reverify floating environment;
3. check task-start cancellation checkpoint;
4. invoke the adapter task entrypoint inside catch-all boundary;
5. map exceptions to a private failure;
6. reverify floating environment after task code;
7. finalize resource usage and private counters;
8. transition the slot to a terminal state;
9. decrement running and increment completed counts under synchronization;
10. notify join waiters; and
11. return to queue wait.

Diagnostic formatting must not alter floating state used by subsequent tasks.

### 10.4 Reuse

Between stages:

- queue must be empty;
- no slot may be running/admitted;
- all stage references must be released;
- every worker must be in the idle wait state;
- resource totals must reconcile;
- worker floating environment must remain qualified; and
- the previous stage transaction must be committed or rolled back.

If any invariant fails, destroy the service after joining and return `internal_invariant_error`.

### 10.5 Shutdown

Shutdown is explicit and idempotent at the owning layer:

1. stop admission;
2. set service shutdown under the queue mutex;
3. wake workers and submitters;
4. ensure no stage remains active;
5. join every joinable worker exactly once;
6. verify all worker loops exited;
7. destroy worker scratch;
8. reconcile worker, queue, and service reservations;
9. clear handles; and
10. mark service destroyed.

The destructor invokes this path defensively and cannot throw. Debug/test builds assert no active stage. Production returns any shutdown contradiction through the last controlled boundary before destructor whenever possible.

## 11. Floating-point environment

### 11.1 Worker guard

Implement `worker_floating_environment_guard` using Component 01 `PlatformQualification` and Component 03 strict arithmetic contract.

It must:

- establish nearest-even rounding;
- verify required signed-zero behavior;
- verify subnormal behavior;
- verify contraction/build qualification evidence;
- verify prescribed conversion points;
- clear or classify exception flags according to policy;
- store exact before/after evidence;
- restore the authoritative state after test-only disturbance or reject the worker; and
- never silently proceed after failure.

### 11.2 Verification frequency

Verify:

- at worker startup;
- immediately before every authoritative task;
- immediately after every task;
- after any test-only deliberate disturbance;
- before worker reuse after non-authoritative formatting; and
- before service startup is declared successful.

A task that changes the environment becomes an `internal_invariant_error` unless the adapter explicitly performs a permitted local save/restore operation and post-verification succeeds.

### 11.3 Unsupported platform

If the service cannot establish the contract, return `unsupported_platform` before authoritative output. A partial pool is stopped and joined. Do not silently force serial execution unless the frozen policy explicitly permits serial fallback and the invoking thread separately passes the same qualification.

## 12. Deterministic merge

### 12.1 Join before merge

Merge begins only after:

- all logically required tasks are terminal;
- no worker references stage slots;
- queue count and running count are zero;
- admitted equals completed plus canonically abandoned;
- all task resource usage is finalized; and
- cancellation/failure evidence needed for arbitration is present.

The pool may remain alive and idle, but the stage owns no active worker execution.

### 12.2 Slot collection

Traverse slots by canonical task rank, not completion order. For every slot verify:

- owner, plan, adapter, and task key;
- expected terminal state;
- declared record counts match storage;
- local resource usage is within reservation;
- no malformed precision/failure/counter record;
- no output outside assigned range; and
- no task-local identity escape.

### 12.3 Record ordering

Gather records and sort by the adapter's complete key. Where memory policy requires windows, window boundaries derive from canonical task ranges and fixed policy. Final result must equal a full sort.

Use full-key equality after any hash accelerator. Hash collisions never coalesce records.

### 12.4 Duplicate processing

Apply only the adapter-declared closed policy. Equivalent coalescing compares all normative fields and canonical bytes. Conflicts emit deterministic witnesses including both task keys and record keys. Explicit multiplicity requires contiguous ranks beginning at zero unless the owning component specifies another closed rule.

### 12.5 Local ID remapping

Build remap tables in sorted canonical record order. Validate every local reference resolves exactly once and every remapped ID fits internal and public index domains. Never mutate predecessor artifacts.

### 12.6 Proposed artifact and verifier

Construct one transaction-private proposed artifact. Invoke the adapter's independent verifier. Only a verified proposal may proceed to resource transfer and commit. A verifier rejection discards the proposal and returns the canonical failure after all reservations reconcile.

## 13. Deterministic reductions

### 13.1 Checked integer sums

Combine in canonical task/record order with Component 01 checked arithmetic. Overflow witness is the first canonical input whose addition would overflow, not whichever worker discovers overflow first.

### 13.2 Prefix sums

For counts `c[0..n)`:

- validate each count;
- set offset zero;
- add in canonical order;
- verify representability in internal count type, `uint64_t`, `size_t`, public `I` where relevant, and hard resource limits;
- assign `[offset[i], offset[i+1])`;
- verify disjointness and completeness; and
- verify final total.

A parallel scan provider is out of scope for V1.

### 13.3 Boolean reductions

Logical AND/OR operate over immutable flags in canonical order. If a witness is required, retain the minimum canonical key producing false/true.

### 13.4 Maximum with witness

Use Component 03 bounded comparison and this tie order:

1. greater bounded numerical value;
2. greater nominal value where the bounded type defines it;
3. greater bound/radius according to the owning quantity contract;
4. minimum canonical entity or ledger witness key;
5. minimum provider/policy version tie key.

Reject NaN, negative, inverted, or malformed records. Equal values select the same witness at every worker count.

### 13.5 Set and sequence reductions

Set union:

- canonical-sort full keys;
- compare full normative records;
- apply explicit duplicate policy.

Ordered concatenation:

- concatenate blocks by canonical task/block key;
- verify expected block count and no missing rank;
- do not concatenate completion order.

### 13.6 Floating statistics

Authoritative floating values must use Component 03 bounded operations, a fixed serial order, exact/count representation, or maximum-with-witness. Wall-clock averages and throughput may be non-authoritative and excluded from canonical bytes/digests.

## 14. Failure collection, early stop, and arbitration

### 14.1 Private findings

Each slot retains bounded findings in the Component 01 schema. Findings include stage, task key, feature key, numerical witness, policy/provider versions, and resource/cancellation evidence.

### 14.2 Terminal hint

Use an atomic terminal hint only to wake waits and avoid optional expensive work. It never selects the public error.

The hint states only:

- no hint;
- cancellation observed;
- at least one failure exists;
- service shutdown.

Do not encode failure priority in racing atomics.

### 14.3 Primary failure

After required tasks join:

1. normalize all findings;
2. include service, queue, worker, cancellation, resource, adapter, merge, verifier, and transaction findings;
3. canonical-sort by Component 01 total arbitration key;
4. select the minimum;
5. retain secondary findings within diagnostic count/byte limits;
6. record deterministic truncation; and
7. compare with the serial reference in qualification.

### 14.4 Cancellation race

The frozen Component 01 arbitration rule determines whether cancellation or another failure is primary. A worker that sets a hint first has no privilege. Cancellation checkpoint IDs and canonical progress records are included in findings and replay.

### 14.5 Early suppression

V1 disables adapter task suppression by default.

To enable it later for one adapter, require a proof function and qualification showing no suppressed task can produce:

- a higher-priority finding;
- a required success record;
- a semantic counter;
- resource reconciliation evidence;
- precision evidence;
- or verifier evidence.

The proof version becomes part of adapter, plan, report, and replay bytes. Unsafe suppression is a release-blocking mutation.

## 15. Cancellation and transaction integration

### 15.1 Checkpoints

Register stable checkpoints for:

- service creation;
- worker startup;
- plan construction;
- plan verification;
- resource reservation;
- descriptor/slot construction;
- queue admission wait;
- task start;
- adapter task loop blocks;
- queue pop;
- join wait;
- slot collection;
- canonical sort;
- duplicate processing;
- prefix/reduction;
- proposed artifact construction;
- stage verifier;
- report/replay finalization;
- resource transfer;
- final commit poll;
- worker idle transition; and
- shutdown.

Adapters add versioned inner loop checkpoints without reusing IDs for different semantics.

### 15.2 Stage transaction phases

Use:

```text
created
-> plan_built
-> plan_verified
-> resources_reserved
-> slots_constructed
-> submitting
-> executing
-> joined
-> merging
-> proposed
-> verified
-> resources_reconciled
-> committed
```

Any phase may transition to `rolling_back`, then `rolled_back`, after workers are joined and private state is destroyed. Commit rejects active service references, queued/running tasks, unreconciled reservations, unverified proposal, or pending cancellation.

### 15.3 Rollback

Rollback order:

1. stop admission;
2. set cancellation/terminal wake state;
3. wake all waits;
4. ensure every admitted slot reaches a terminal state;
5. wait for queue/running counts to reach zero;
6. release all worker references to stage state;
7. destroy private outputs;
8. release temporary and persistent reservations;
9. retain bounded canonical diagnostics/replay according to policy;
10. leave predecessor artifacts untouched; and
11. mark transaction rolled back.

No stage destructor may be the first place rollback joins workers.

## 16. Resource admission and memory bounds

### 16.1 Resource classes

Extend Component 01 resource kinds for:

- worker controls;
- thread objects;
- worker scratch;
- queue slots;
- task descriptors;
- output-slot controls;
- task-local temporary bytes;
- task-local persistent bytes;
- merge records;
- merge scratch;
- canonical sort scratch;
- prefix/reduction records;
- failure/finding records;
- logical trace events;
- execution plan bytes;
- execution report bytes;
- replay bytes;
- structural counter records;
- abstract execution work units; and
- optional non-authoritative timing records.

### 16.2 Service preflight

Before worker creation, reserve worker controls, thread objects, queue state, minimum scratch, environment evidence, and service diagnostics. Every count-to-byte multiplication uses Component 01 checked arithmetic.

### 16.3 Stage preflight

Before admission, compute conservative upper bounds for:

- task count;
- descriptor and slot bytes;
- queue capacity;
- task-private outputs;
- merge records and scratch;
- direct-fill arrays;
- failure/trace/counter records;
- work units; and
- final persistent artifact transfer.

If an exact or conservative bound exceeds a hard limit, fail before queueing any task.

### 16.4 Bounded growth

If an adapter cannot precompute exact output, it declares a conservative initial reservation and a maximum number of deterministic growth chunks. Each chunk requires reservation before allocation. Chunk size derives from policy and logical record count, not allocation timing.

### 16.5 High-water values

Track high-water values with checked integer atomics or mutex-protected counters. These are execution statistics; their canonical report category states whether exact equality is required. Memory release and transfer are reconciled after every stage and service shutdown.

### 16.6 Work budgets

Charge deterministic abstract units at adapter-defined operations. Crossing a hard budget emits `resource_limit` at the canonical logical operation key. Do not substitute a weaker algorithm, skip verification, reduce bound inflation, or truncate candidates.

## 17. Counters, reports, traces, and replay

### 17.1 Counter schema

Every counter has:

- stable ID/version;
- component/stage/adapter owner;
- category;
- unit;
- combination rule;
- expected serial/parallel comparison;
- hard/advisory envelope status; and
- canonical witness policy.

Categories:

- semantic invariant;
- plan invariant;
- bounded execution statistic;
- non-authoritative timing statistic.

### 17.2 Required service counters

Record at least:

- requested, desired, attempted, and actual workers;
- logical execution lanes;
- worker startup/qualification failures;
- task count;
- admitted/completed/failed/abandoned tasks;
- queue capacity and high-water;
- peak running/in-flight;
- descriptor/output/merge bytes;
- temporary/persistent high-water;
- cancellation polls and observations;
- exception mappings;
- merge records;
- equivalent/conflicting duplicates;
- sort abstract work;
- reduction records;
- prefix records;
- logical trace events;
- work units; and
- service/stage reuse and shutdown counts.

Adapters add the stage-specific counters required by Component 17 Section 2.36.

### 17.3 Execution report

Produce one immutable per-call report with canonical stage order. Include:

- all provider/schema versions;
- requested syntax, hardware result, desired and actual workers;
- fallback attempts and outcome;
- serial/parallel stage dispositions;
- plan IDs/digests;
- task, queue, work, and memory counters;
- merge/reduction versions;
- worker floating-environment evidence;
- cancellation and terminal state;
- structural gate results;
- resource reconciliation;
- logical trace summary;
- replay compatibility status; and
- optional non-authoritative timings in an explicitly excluded appendix.

Component 14 carries the report in the assembled candidate. Component 15 independently audits facts that affect publication or resource claims.

### 17.4 Logical trace

Canonical events include plan creation, task admission, task logical start, task logical completion, failure emission, cancellation observation, merge phases, verifier disposition, commit, and rollback.

Canonicalize by:

1. component/stage;
2. logical phase;
3. task/record key;
4. event kind;
5. canonical witness.

Do not canonicalize by raw timestamp. A physical trace may contain timestamps/thread slots for debugging but is not encoded into authoritative replay or digest.

### 17.5 Replay

Record:

- service/adapter/partition/grain/queue/merge/reduction versions;
- requested/hardware/desired/actual workers;
- queue/in-flight limits;
- work/memory limits;
- plan digests;
- threshold decisions;
- forced schedule/partition/delay/fault controls in qualification builds;
- cancellation checkpoint/progress;
- provider inventory digest; and
- report/counter schema versions.

Ordinary replay need not reproduce OS scheduling. It must reproduce the same authoritative result or primary failure under any qualified schedule.

## 18. Structural performance gates

### 18.1 Gate principles

Structural counters, not wall-clock time, are normative. Each gate has a stable ID, fixture family, provider version, exact or bounded envelope, canonical witness, and required/advisory status.

A required gate failure is typed and blocks Component 16 release qualification. An advisory timing miss cannot weaken correctness or change output.

### 18.2 Required gate families

Define gates detecting:

- Component 06 disjoint operands reaching narrow phase at all-pairs scale;
- repeated evaluation of one Component 07 canonical relation;
- duplicate Component 08 construction for one event;
- Component 09 reverting to one seed/ray query per vertex when grouping applies;
- Component 11 global quadratic edge pairing despite carrier groups;
- unbounded or non-progressing Component 12 retry loops;
- Component 13 full-mesh rescans after each local action when the provider promises local invalidation;
- Component 14 source-order fallback or uncontrolled canonicalization branching;
- Component 15 omitted candidate/pair/probe work;
- task count above fixed plan limits;
- queue or memory growth above policy;
- duplicate merge records above provider allowance; and
- repeated worker/service creation per stage instead of invocation reuse.

### 18.3 Fixture envelopes

Store deterministic envelopes under Component 16 qualification data. Include large disjoint meshes, clustered overlaps, many relations/few events, many shared event consumers, large untouched classification groups, many independent polygons, cleanup-heavy complexes, symmetric canonicalization inputs, and large verifier outputs.

Every envelope specifies type/policy/provider versions and exact counter meaning. Changing meaning requires a schema/provider version and review.

## 19. Independent verification

### 19.1 Service verifier

Independently verify:

- worker count and creation attempts;
- no detached or joinable-unowned threads;
- worker startup qualification;
- queue capacity/accounting;
- no active stage overlap;
- idle/reuse state;
- shutdown joins; and
- service resource reconciliation.

### 19.2 Plan verifier

Reconstruct:

- logical count;
- threshold decision;
- grain and ranges;
- coverage/no overlap;
- task keys;
- reservation bounds;
- plan bytes and digest.

Do not trust producer counts or digest alone.

### 19.3 Stage verifier

Check:

- every admitted task has one terminal slot;
- every required logical range is represented;
- no slot ownership violation;
- output counts and direct-fill ranges;
- merge duplicate policy;
- local-ID remap completeness;
- reduction witnesses;
- failure arbitration;
- counter categories;
- resource transfer/release;
- report and replay fields; and
- no active work before commit.

The owning component's independent artifact verifier remains mandatory; Component 17's verifier does not replace it.

## 20. Prohibited behavior and mutation requirements

Qualification must include mutations for:

- using `YgorThreadPool.h::work_queue`;
- using `taskqueue`;
- unbounded queue admission;
- detached worker;
- leaked joinable worker;
- per-stage or nested child pool;
- shared mutable output vector;
- write outside assigned range;
- assigning IDs by atomic fetch-add arrival;
- queue/completion order publication;
- first-observed failure selection;
- pointer/thread/time/hash-order tie break;
- schedule-ordered floating accumulation;
- worker without floating qualification;
- failure to restore disturbed floating state;
- unsafe early suppression;
- omitted task/range;
- duplicated task/range;
- partial comparator;
- equivalent duplicate accepted without full comparison;
- conflicting duplicate silently coalesced;
- unchecked count/prefix overflow;
- count/fill disagreement;
- reservation after allocation;
- leaked reservation;
- task admitted after cancellation;
- commit with active/unjoined work;
- cancellation that leaves a wait blocked;
- shutdown depending on static destruction;
- structural gate disabled to pass timing;
- Component 06 reduced bound inflation;
- Component 08 race-ordered interning;
- Component 13 concurrent coupled mutations;
- Component 14 first-completed branch;
- Component 15 skipped verifier pair; and
- external concurrency or benchmark dependency.

Every required mutation must be killed by a named test and traced to a Component 17 clause.

## 21. Test and validation plan

### 21.1 Policy and worker selection

Test serial and deterministic-parallel modes; requested counts zero/one/two/maximum/above maximum; hardware result zero/one/many through test provider; hard worker ceiling; fallback allowed/forbidden; partial creation failure at every creation rank; retry teardown; unsupported platform; and exact report/replay fields.

### 21.2 Queue unit tests

Test capacities zero/invalid, one, below/equal/above worker count, exact hard limit, one above limit, wraparound, repeated fill/drain, spurious wakeups, simultaneous submit/cancel, close while blocked, no lost wakeups, and invariant inspection. Use logical watchdogs in tests so a hang is an infrastructure failure.

### 21.3 Worker lifecycle

Test startup, environment qualification, idle/reuse, multiple sequential stages, stage failure followed by reuse, service shutdown with empty/full queues, exception in task, exception in setup, partial creation, destructor defense, join exactly once, and zero background thread serial mode.

### 21.4 Partition and plan

Test empty, one, grain-1, grain, grain+1, uneven remainder, maximum task count, endpoint overflow, count-to-byte overflow, forced partition controls, key uniqueness, complete coverage, no overlap, worker-count-independent default ranges, canonical bytes, and digest known answers.

### 21.5 Slot ownership

Instrument and test two writers to one slot, write outside direct-fill range, mutation of immutable input, slot reuse before merge, task-local ID escape, use after rollback, output publication before terminal state, malformed counts, and wrong owner/plan/adapter tokens.

### 21.6 Merge permutations

For bounded task counts, enumerate every completion permutation. For larger counts, use deterministic forced schedules.

Cover unique keys, equivalent duplicates, conflicting duplicates, explicit multiplicity ranks, deterministic proposal choice, missing records, duplicate records, hash collisions, partial comparator mutations, local ID remapping, windowed merge, cancellation during merge, and resource failure during sort.

### 21.7 Reduction tests

Test checked sums, booleans with witnesses, prefix sums, maxima with equal values/different witnesses, adjacent bounds, signed zero, malformed NaN/negative/inverted records, set union, ordered concatenation, different input permutations, different task groupings, overflow, and all worker counts.

### 21.8 Floating-environment tests

Deliberately disturb rounding mode, exception flags, signed-zero-sensitive state where portable, and test-provider subnormal/contraction probes before and during tasks. Verify restoration or deterministic rejection, known bit patterns, post-task detection, and no contamination of later tasks.

### 21.9 Cancellation matrix

Cancel before service creation; during partial creation; after service creation before plan; during plan; before/after reservation; while submitter waits on full queue; while queued; at task start; inside adapter loops; during join; sort; merge; reduction; verifier; report; replay; resource transfer; immediately before commit; between stages; and during shutdown.

For every case verify joined workers, terminal slots, reconciled resources, unchanged predecessors, no partial artifact, canonical primary finding, retained findings, report, and replay.

### 21.10 Resource and allocation failure

Inject failure for worker controls, thread objects, queue, descriptors, slots, task-local output, growth chunk, merge records, sort scratch, prefix arrays, findings, trace, report, replay, and final transfer. Test limit-1/limit/limit+1 and arithmetic overflow. No allocation occurs before reservation.

### 21.11 Failure arbitration

Place multiple failures in every completion order: same stage/different keys; different stages; resource plus geometry; cancellation plus failure; equal category/different subcodes; equal bounds/different witnesses; adapter plus verifier failure; service shutdown contradiction; and diagnostic truncation. Compare with Component 01 serial arbitration bytes.

### 21.12 Nested execution

Invoke every parallel-capable adapter from a worker task. Verify serial nested fallback, no child threads, no queue submission, no deadlock, same resources/cancellation/errors/counters, and serial-equivalent artifact. Mutate to child-pool creation and require rejection.

### 21.13 Adapter equivalence

For every V1 adapter and bounded fixtures, run:

- serial reference;
- parallel with one background worker;
- parallel with two workers;
- parallel with maximum configured workers;
- every supported grain boundary;
- forced reverse/striped/random-looking deterministic schedules;
- allocator-layout perturbations;
- hash collision providers where used privately;
- queue capacities one and normal;
- cancellation and resource boundaries; and
- repeated service reuse.

Compare artifact canonical bytes, failures, precision witnesses, semantic counters, required reports, replay, and digests.

### 21.14 Component-specific concurrency tests

Run the complete cases required by Component 17 Sections 5.8-5.15:

- Component 06 exhaustive all-pairs broad-phase oracle;
- Component 07 one record per canonical relation;
- Component 08 equal-coordinate/distinct-lineage and many-consumer cases;
- Component 09 union-order and seed-failure cases;
- Component 12 many complex polygons and one-failure-among-successes;
- Component 13 stale/conflicting candidate and coupled-action mutation;
- Component 14 symmetric components and automorphism branches;
- Component 15 simultaneous topology/geometry/report/digest failures.

### 21.15 Deadlock and progress stress

Use queue capacity one, one worker, nested-capable calls, tight resource limits, blocked submitter, forced worker delays, cancellation, exception, and shutdown. Exercise every wait edge. Tests use deterministic logical progress counters and an outer test-process timeout only as infrastructure protection; production has no wall-clock semantic timeout.

### 21.16 Race stress

Repeatedly exercise queue predicates, cancellation hints, slot state, completion count, resource high-water counters, exception records, reusable idle transitions, simultaneous submit/cancel, stage destruction, and immutable cache initialization. Run under compiler-provided race instrumentation when available; instrumentation is not a dependency and its output is non-canonical.

### 21.17 Reports, traces, and replay

Known-answer encode/decode tests; unknown version/field rejection; duplicate singleton rejection; plan/report digest mismatch; physical trace exclusion; logical trace canonicalization; bounded trace truncation; requested/actual worker recording; forced controls; cancellation checkpoint replay; and semantic replay under different physical schedules.

### 21.18 Structural performance

Run every required fixture and exact counter envelope. Test threshold-1/threshold/threshold+1. Verify no correctness work is skipped at larger worker counts. Compare semantic counters exactly and plan/bounded counters according to their declared schemas.

### 21.19 Memory high-water

Test many small tasks, few large outputs, two-pass direct fill, bounded growth, full queue cancellation, near-limit failures, repeated stages, diagnostics off/on, and service reuse. Component 01 ledger totals must reconcile after each stage and invocation.

### 21.20 Byte stability

Repeat fixed cases under thread counts, forced schedules, allocation layouts, queue capacities, hash collision providers, and repeated runs. Compare every committed stage digest, final mesh bytes, reports, failures, replay, plan digests where worker invariant, and structural counters under schema.

### 21.21 Provider replacement

Any alternate worker, queue, partition, merge, reduction, or stage adapter must run serial equivalence, concurrency matrix, resource/cancellation, floating environment, mutation, replay compatibility, structural counters, and version-change audit before qualification.

## 22. CMake, portability, and build qualification

- Compile all Component 17 production and authoritative tests in the strict bounded Boolean target.
- Require C++17 and disable compiler extensions.
- Reuse `ygor_apply_mesh_boolean_strict_fp`.
- Reject `__FAST_MATH__`.
- Add thread-library linkage through CMake's reviewed standard mechanism without downloading anything.
- Do not use platform-specific thread APIs in normative production code.
- Optional sanitizer/race builds are qualification cells, not runtime dependencies.
- Add compile probes for required `std::thread`, atomic, mutex, condition-variable, and `<cfenv>` behavior.
- Unknown or failed platform qualification returns `unsupported_platform`.
- No benchmark target may alter authoritative code paths with different arithmetic or disabled verification.

## 23. Implementation sequence

Implement in this order. Do not parallelize component stages before the generic service and serial-equivalence harness pass.

1. Extend version, stage, checkpoint, subcode, resource, replay, and counter registries.
2. Implement `ExecutionTypes`, exact keys, codecs, and malformed-input tests.
3. Implement policy capability validation and actual-worker selection with injectable test provider.
4. Implement deterministic partitioning and immutable execution plan plus independent verifier.
5. Implement task slots and exact ownership instrumentation.
6. Implement bounded ring queue and queue tests.
7. Implement worker floating-environment guard.
8. Implement worker creation, startup, loop, reuse, and shutdown without stage adapters.
9. Integrate Component 01 reservations, cancellation, diagnostics, replay, and transaction active-work checks.
10. Implement canonical join, slot collection, merge, duplicate handling, prefix sums, and reductions.
11. Implement counters, reports, logical trace, replay, and independent execution verifier.
12. Add a synthetic adapter and pass all completion-permutation, failure, cancellation, resource, deadlock, race, and mutation tests.
13. Register the V1 stage inventory.
14. Add Component 06 adapter first because its count/prefix/emit structure exercises two-pass direct fill and exhaustive completeness.
15. Add Component 07 and Component 08 adapters.
16. Add Component 04, 09, 10, 11, and 12 adapters.
17. Add Component 13 restrictions and candidate-only adapter.
18. Add Component 14 and Component 15 adapters.
19. Add Component 16 test-only scheduler adapter.
20. Integrate final execution report through Component 14 and audit through Component 15.
21. Add structural gates and qualification data.
22. Run the complete Component 16 closed qualification matrix, mutation suite, byte-stability suite, and release evidence self-audit.

Each step must leave the serial path executable. No commit may make parallel execution the only route before its adapter equivalence tests pass.

## 24. Definition of done

Component 17 implementation is complete only when all of the following are true:

- every parallel-capable stage has an explicit registered adapter and executable serial semantic reference;
- every serial-only V1 stage is explicitly inventoried and cannot become parallel accidentally;
- the worker service is invocation-owned, bounded, joinable, and creates no detached or orphaned work;
- partial worker creation is rolled back before deterministic fallback;
- worker count selection and fallback are validated, reported, and replayable;
- all worker task inputs are immutable;
- every task output is private or exactly disjoint and ownership violations are test-detectable;
- queue admission is bounded, cancellation-aware, and free of component-lock waits;
- partitioning is deterministic, checked, complete, and versioned;
- execution plans are independently verified and canonically encoded;
- task-local identities are remapped before publication;
- canonical merge and duplicate handling are complete-key based;
- checked prefix sums and deterministic reductions produce stable witnesses;
- authoritative floating reductions never depend on completion order;
- every authoritative worker establishes and verifies the qualified floating environment;
- failures are collected privately and the public primary failure follows Component 01 arbitration after join;
- cancellation, resource exhaustion, and exceptions roll back transactionally;
- every exit path reconciles reservations and joins or idles all owned workers before state release;
- nested parallelism uses the one bounded service and V1 serial nested fallback without deadlock;
- serial, one-worker, two-worker, and maximum-worker runs produce identical authoritative artifacts or primary failures;
- reports, logical traces, replay bytes, and required counters are deterministic under their declared comparison rules;
- Component 14 carries complete execution evidence and Component 15 independently audits publication-relevant claims;
- structural gates detect accidental all-pairs, repeated computation, unbounded retries, global rescans, source-order fallback, and omitted verification work;
- every prohibited concurrency, ownership, merge, reduction, environment, cancellation, resource, and publication mutation is rejected;
- provider replacement requires explicit versioning and full requalification;
- all mandatory Component 16 qualification cells pass with no required skip and no surviving mutation; and
- all production and normative-test code is strict portable C++17 with no external dependency.
