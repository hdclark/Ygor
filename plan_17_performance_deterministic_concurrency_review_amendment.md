# Plan 17 Review Amendment: Deterministic Evidence, Publication, and Resource Closure

## Status, precedence, and implementation conclusion

This file is the controlling review amendment to `plan_17_performance_deterministic_concurrency.md`.

The base plan remains the implementation handoff except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers must also apply `component_17_performance_deterministic_concurrency_review_amendment.md`, the reviewed Component 14-16 amendments, and the Component 01 runtime/context boundary.

The base plan's overall V1 provider remains appropriate:

- one invocation-owned bounded service;
- zero background threads for serial mode;
- a bounded FIFO rank queue for deterministic parallel mode;
- immutable canonical execution plans;
- preassigned task-private slots or exactly disjoint direct-fill ranges;
- join/quiesce before canonical merge;
- complete-key duplicate handling and canonical failure arbitration;
- serial nested fallback;
- serial publication for globally coupled topology and final status transitions; and
- portable C++17 standard-library concurrency only.

Do not replace that design. Apply the following mandatory implementation corrections.

## 1. Corrected V1 provider inventory

Extend the fixed V1 design with these explicit providers and schemas:

```text
execution_capability_snapshot:       immutable_invocation_profile_v1
thread_creation:                     invocation_owned_thread_factory_v1
stage_generation:                    checked_monotonic_generation_v1
slot_publication:                    terminal_release_merge_acquire_v1
queue_generation_binding:            active_stage_generation_v1
resource_entitlement:                canonical_rank_slice_or_wave_v1
mandatory_local_failure:             fixed_local_minimum_slot_v1
semantic_execution_projection:       semantic_execution_projection_v1
execution_profile_record:             deterministic_execution_profile_v1
physical_observation_appendix:        excluded_physical_observations_v1
semantic_execution_digest:            component01_domain_separated_sha256_v1
execution_profile_digest:             component01_domain_separated_sha256_v1
physical_observation_digest:          optional_nonsemantic_sha256_v1
native_thread_memory_accounting:      profile_observation_not_exact_lease_v1
```

Retain the base plan's remaining V1 choices. Add explicit nonzero versions for every item above to `ContractVersions.h` and to Component 16's effective-contract/provider manifests.

## 2. Existing Ygor assessment and reuse corrections

### 2.1 Keep the existing queue rejections

Continue to preserve, but never use for authoritative bounded Boolean work:

- `src/YgorThreadPool.h::work_queue`; and
- `src/YgorContainers.h::taskqueue`.

Their unbounded admission, exception handling, detach/polling behavior, absent Component 01 integration, and missing deterministic ownership/merge contracts remain disqualifying.

### 2.2 Explicitly reject `For_Each_In_Parallel`

Add `src/YgorAlgorithms.h::For_Each_In_Parallel` to the assessment. Do not call, adapt, or wrap it for Component 17.

Its per-item `std::async`/future behavior is unsuitable because:

- the implementation may choose asynchronous or deferred execution;
- the number and lifetime of asynchronous operations are not controlled by one invocation-owned worker budget;
- it has no immutable execution plan, canonical task identity, bounded queue, reservation, private-output, floating-environment, cancellation, transaction, failure-arbitration, or replay contract; and
- it cannot prove stage-generation isolation or canonical output publication.

Preserve it unchanged for unrelated Ygor users.

### 2.3 Mandatory reuse

Reuse:

- the root project's existing `find_package(Threads REQUIRED)` result and `Threads::Threads` target;
- the strict bounded Boolean target and `ygor_apply_mesh_boolean_strict_fp` established by Plan 01;
- Component 01 checked arithmetic, owner tokens, typed outcomes/errors, failure total order, reservations/leases/slices, cancellation, diagnostics, transactions, canonical bytes, SHA-256, replay framing, and runtime bundle;
- Component 03 strict floating and maximum-with-witness services; and
- every owning component's serial producer and independent verifier.

Do not add another package lookup, task framework, allocator, profiler, benchmark library, concurrent container, or optional external provider.

## 3. Exact file and integration changes

### 3.1 Add two focused production modules

In addition to the files listed by the base plan, add:

- `ExecutionProfile.h/.cc`
  - immutable execution capability snapshot;
  - raw and normalized hardware-concurrency result;
  - supported worker, queue, atomic, condition-variable, and floating-profile capabilities;
  - requested/desired/actual worker admission record;
  - semantic/profile/physical field classification;
  - capability/profile codec, verifier, and digest; and
  - test-provider injection without process-global mutable state.

- `ExecutionThreadFactory.h/.cc`
  - the only Component 17 production location permitted to construct `std::thread`;
  - invocation owner and worker-creation rank binding;
  - startup handoff and exception-safe partial-pool teardown;
  - exactly-once join accounting;
  - no detach operation;
  - immutable post-shutdown ownership evidence; and
  - verifier query surface.

Do not introduce a general-purpose public thread-pool API. These modules remain internal to `YgorMeshesBooleanBounded`.

### 3.2 Amend existing planned modules

#### `ExecutionTypes.h`

Add closed enums and strong types for:

- evidence domain;
- execution field class;
- capability snapshot status;
- service state;
- stage-generation state;
- publication protocol version;
- resource-entitlement mode;
- native-thread-memory disposition; and
- physical-observation retention.

Add owner-bound `execution_stage_generation_id` with checked monotonic creation. Zero is invalid and wrap is a typed resource/invariant failure before generation reuse.

#### `ExecutionPolicy.h/.cc`

Compute desired and actual workers only from:

- frozen caller request;
- one immutable capability snapshot;
- frozen hard ceilings;
- fallback policy; and
- qualification-only forced inputs.

Never query `hardware_concurrency()` in a stage adapter, threshold function, queue, report finalizer, or replay decoder.

#### `ExecutionPlan.h/.cc`

Bind every plan to:

- capability/profile digest;
- stage generation;
- semantic logical-input digest;
- deterministic entitlement mode;
- exact task slices or canonical waves;
- publication protocol version; and
- field-classification schema.

Plan bytes may differ across deliberately different actual-worker profiles only for fields explicitly declared profile-bound. V1 grain and canonical ranges remain worker-count independent by default.

#### `ExecutionTaskStorage.h/.cc`

Implement fixed, non-moving slot storage. Do not rely on relocating slot objects containing atomics or synchronization state. A conforming representation may use a preallocated array/`unique_ptr<slot[]>` or another fixed-address owner-checked block.

Each slot contains:

- owner, service, stage-generation, plan, adapter, and task keys;
- atomic or mutex-protected state;
- task-private payload;
- fixed mandatory local-minimum finding storage;
- optional bounded secondary finding storage;
- cancellation observation;
- deterministic resource entitlement and actual controlled use;
- direct-fill range and fill evidence where applicable;
- semantic/plan counters;
- schedule-observed counters in a separate record; and
- terminal publication evidence.

#### `BoundedTaskQueue.h/.cc`

Add stage-generation binding and separate operations for:

- open stage admission;
- close stage admission;
- drain/quiesce the active generation;
- clear verified abandoned ranks for that generation; and
- service shutdown.

A stage close is not service shutdown. Every rank in the ring is interpreted only with the queue's active generation and validated against the descriptor/slot generation.

#### `BoundedWorkerService.h/.cc`

Create threads only through `ExecutionThreadFactory`. Store no mutable service handle in the canonical context. The Component 01 runtime bundle owns the service.

At stage activation, publish one immutable stage invocation handle and generation under the service mutex. Workers acquire that handle only after the queue predicate confirms an active matching generation. At stage quiescence, prove all workers released the handle before stage storage can be destroyed.

#### `ExecutionStage.h/.cc`

Own the complete corrected transaction sequence:

1. acquire the next checked generation;
2. construct and verify the capability/profile association;
3. construct and verify the execution plan;
4. compute deterministic resource entitlements;
5. reserve the complete service/stage/wave capacity before admission;
6. construct fixed descriptors and slots;
7. activate the generation;
8. submit canonical ranks or one canonical wave at a time;
9. close stage admission;
10. wait for terminal publication and generation quiescence;
11. acquire and inspect all required slots;
12. arbitrate failures and merge semantically;
13. run Component 17 and owning-component verifiers;
14. finalize separately classified reports/replay;
15. reconcile and transfer resources;
16. poll cancellation at the final checkpoint;
17. commit; and
18. retire the generation only after no worker reference remains.

Rollback follows the same generation ownership and cannot destroy stage state until workers have released it.

#### `ExecutionCounters.h/.cc`

Replace any ambiguous `bounded execution statistic` handling with four closed classes:

```text
semantic_invariant
profile_deterministic
schedule_observed_bounded
physical_non_authoritative
```

Every counter definition declares:

- domain;
- combination rule;
- expected comparison relation;
- deterministic upper/lower bound where applicable;
- witness rule; and
- digest inclusion.

#### `ExecutionReport.h/.cc`

Produce three separately encoded records:

- `semantic_execution_projection`;
- `deterministic_execution_profile_record`; and
- `physical_observation_appendix`.

A convenience aggregate view may reference all three, but it must not flatten them into one canonical equality domain.

#### `ExecutionReplay.h/.cc`

Separate semantic replay requirements from profile reproduction. Decode and validate each domain independently. Ordinary replay accepts a compatible qualified physical schedule when the semantic projection matches. Exact profile replay additionally requires the same capability snapshot, actual worker count, limits, provider versions, forced controls, and profile digest.

#### `ExecutionVerifier.h/.cc`

Add independent checks for:

- release/acquire publication;
- stage-generation binding and retirement;
- deterministic entitlement assignment;
- local-minimum failure retention;
- evidence-domain field classification;
- digest-domain separation;
- closed-world thread-factory ownership; and
- honest native-thread-memory disposition.

### 3.3 Correct Component 01 integration wording

Replace the base plan instruction to place “one invocation-owned execution service handle” in `Context.h/.cc` with this exact boundary:

- canonical immutable `boolean_context<T,I>` stores frozen execution request, capability-contract version, semantic determinism requirements, and stable provider compatibility only;
- Component 01's mutable invocation runtime bundle stores the non-copyable execution service handle, live queue/synchronization state, thread factory, capability snapshot instance, and stage-generation factory; and
- later components receive narrow owner-checked const capability views and controlled stage-execution entrypoints, never mutable context fields.

Changing service state must not change `context_digest` or public semantic content.

## 4. Corrected evidence schemas

### 4.1 Semantic execution projection

Define a canonical record containing only facts that must be invariant across admitted profiles:

- semantic schema/version and context owner/digest reference;
- component/stage and owning artifact semantic digest;
- logical domain digest and exact logical item count;
- required-work coverage disposition;
- semantic counter records;
- semantic precision/failure witnesses contributed by the stage;
- owning-component verifier disposition;
- Component 17 execution-semantic verifier disposition; and
- semantic execution digest.

Do not encode requested/actual workers, queue capacity, plan digest, fallback attempts, task completion states, timing, thread slots, physical high-water values, or allocation addresses.

### 4.2 Deterministic execution-profile record

Encode:

- capability snapshot and digest;
- requested syntax, raw hardware result, normalized desired workers, attempted counts, actual workers, and fallback disposition;
- service/provider/adapter/partition/grain/queue/slot/publication/merge/reduction versions;
- serial/parallel stage disposition and reason;
- stage generation's logical identity, without runtime addresses;
- plan digest;
- configured queue, task, in-flight, work, and controlled-byte bounds;
- deterministic preflight reservations and canonical entitlement/wave records;
- profile-deterministic counters;
- worker floating-environment qualification summary;
- resource reconciliation status;
- logical trace digest if retained; and
- execution-profile digest.

The runtime generation ordinal may be present only if it is deterministic for the invocation's canonical stage sequence and is classified profile/presentation evidence. It must not enter semantic artifact identity.

### 4.3 Physical observation appendix

Keep optional and separately bounded:

- elapsed durations;
- raw queue/running/in-flight high-water observations;
- raw controlled-allocation residence observations that vary with schedule;
- physical task timestamps/order;
- nonsemantic worker-slot diagnostics;
- sanitizer/profiler observations; and
- host/runtime/native-stack observations.

Exclude it from semantic and profile digests unless a dedicated appendix integrity digest is requested. Even with such a digest, appendix equality is not a release semantic requirement.

### 4.4 Field audit

Add a compile-time or declarative schema inventory that maps every report/replay/counter field to exactly one domain and comparison class. The encoder and independent verifier reject:

- a field with no class;
- a field appearing in several incompatible domains;
- a physical field in semantic bytes;
- an unknown required class;
- profile data used in semantic failure arbitration; or
- a schema revision that changes classification without a version update.

## 5. One-time capability snapshot and worker admission

### 5.1 Snapshot construction

On the invoking thread, before worker allocation:

1. validate the strict build/platform profile inherited from Components 01/03;
2. query `std::thread::hardware_concurrency()` exactly once through an injectable provider;
3. record the raw result;
4. normalize zero to one logical capability under V1;
5. intersect caller request, maximum, and hard worker ceiling with checked arithmetic;
6. validate required C++ synchronization and floating-environment capabilities;
7. state native-thread-memory accounting disposition;
8. canonically encode and independently verify the snapshot; and
9. publish it into the runtime bundle as immutable profile evidence.

No process-global capability cache is required. If a cache is later introduced, it needs its own lifetime, initialization, profile-freshness, and race proof.

### 5.2 Worker creation

For each attempted worker count:

1. pre-reserve controlled worker records, thread object storage, explicit scratch, startup evidence, and queue service state;
2. construct all factory bookkeeping before launching the first thread;
3. create workers in rank order through `ExecutionThreadFactory`;
4. wait for every created worker's qualified startup disposition;
5. on any creation/startup failure, stop and join the complete partial attempt;
6. reconcile the attempt's controlled resources;
7. record the attempt canonically; and
8. either fail or retry the next lower count according to frozen policy.

Never retain a partially successful pool. Never begin a stage until one complete attempt is qualified.

### 5.3 Thread factory contract

`ExecutionThreadFactory.cc` is the only production source permitted to contain native Component 17 `std::thread` construction. It exposes no detach operation.

For every worker record track:

- canonical creation rank;
- runtime object ownership token;
- created/not-created;
- joinable state;
- startup disposition;
- stop requested;
- loop exited;
- join attempted;
- join succeeded exactly once; and
- controlled resources reconciled.

The factory destructor is defensive and nonthrowing, but ordinary control flow must already have joined workers. A destructor-discovered active worker is an invariant defect captured at the last controlled boundary where a typed result can still be returned.

## 6. Exact C++ publication protocol

### 6.1 Slot state representation

Use one atomic terminal/status field or one mutex-protected state per fixed slot. Document the memory order in code comments and tests.

A suitable atomic protocol is:

```cpp
// Worker owns payload exclusively while status == running.
write_complete_private_payload(slot);
finalize_local_minimum_and_resource_usage(slot);
slot.status.store(terminal_status, std::memory_order_release);
completed_count.fetch_add(1, std::memory_order_relaxed);
notify_join_waiter();
```

The join/merge side performs:

```cpp
wait_until_logical_completion_condition();
for (slot in canonical_task_rank_order) {
    const auto s = slot.status.load(std::memory_order_acquire);
    require_terminal_for_plan(s);
    inspect_payload_after_acquire(slot);
}
```

The exact code may use mutex unlock/lock instead, but qualification must demonstrate an equivalent happens-before edge. Do not use `memory_order_relaxed` for the sole terminal publication edge.

### 6.2 State transitions

Transitions before terminal publication may use compare/exchange or mutex-protected checks. Illegal transitions produce an invariant finding. No observer reads payload while status is `reserved`, `admitted`, or `running`.

The completed count is an efficient wait hint only. It is not proof that:

- every expected slot is terminal;
- payload is visible;
- the queue is empty;
- running count is zero; or
- the generation is quiescent.

### 6.3 Condition-variable discipline

Every wait uses a predicate over state synchronized by the same mutex or atomics with the necessary memory order. Notify after making the predicate true. Spurious wakeups are expected. No production `wait_for` polling loop is used to compensate for missing synchronization.

### 6.4 Direct-fill publication

For two-pass direct fill:

- count slots publish count results first;
- checked canonical prefix sums assign immutable ranges;
- fill workers write only their ranges;
- each fill slot release-publishes terminal state after the final element and fill bitmap/count are complete;
- merge acquire-loads terminal state before reading the range; and
- the verifier checks every element exactly once, no overlap, no hole, and total equality.

## 7. Deterministic task resource entitlements

### 7.1 Preferred V1 order

Use these modes in preference order:

1. exact two-pass count/prefix/fill;
2. full conservative per-task slices reserved in one checked aggregate;
3. canonical fixed waves of consecutive task ranks; and
4. canonical serial authorization of additional growth chunks between execution waves.

Do not let workers independently race against one shared remaining hard-byte or work-unit budget when loss of that race can change required output or failure identity.

### 7.2 Slice construction

For each task rank in canonical order:

1. calculate descriptor, fixed slot, mandatory finding, conservative private temporary, private persistent, and work-unit entitlements;
2. validate every count-to-byte conversion;
3. checked-prefix the slices;
4. verify disjointness and total against Component 01 limits;
5. reserve the aggregate or canonical wave before admission; and
6. bind slice ID/range to the descriptor and slot.

A worker allocation helper requires the bound slice and purpose ID. It cannot consume another task's slice.

### 7.3 Canonical waves

When the aggregate cannot fit but bounded streaming is supported, partition canonical task ranks into deterministic waves based only on:

- task entitlements;
- frozen hard limits;
- adapter/provider version; and
- canonical task order.

Reserve an entire wave before any task in it runs. Complete, acquire, merge or retain its canonical output as specified, reconcile temporary capacity, then admit the next wave. Wave boundaries never derive from current allocator pressure, task speed, completion order, or queue occupancy.

### 7.4 Growth

An adapter with bounded growth declares exact chunk size/count policy. A task exhausts its current slice by producing a private `needs_growth` record. After the wave joins, the stage serially processes growth requests by canonical task key, assigns the next slices, and launches a new canonical phase. No racing `try_reserve` decides which task continues.

### 7.5 Host allocation failure

Perform controllable allocations before worker execution whenever practical. Any worker-side `std::bad_alloc` maps to a private typed profile execution failure with allocation-purpose, task, slice, and generation witnesses. It cannot cause partial record publication or transfer unused semantic entitlement to another worker opportunistically.

## 8. Mandatory local failure storage

Every slot reserves fixed capacity for one `boolean_error`-equivalent normalized local minimum independent of optional diagnostics.

When task code observes a finding:

1. validate and normalize it immediately;
2. compare it with the current local minimum under Component 01's total order;
3. retain the smaller one in mandatory storage;
4. optionally add it to the secondary buffer if capacity permits; and
5. update deterministic truncation evidence if the optional buffer is full.

At join, collect:

- every slot local minimum;
- required cancellation observations;
- worker startup/environment findings;
- queue/service/resource findings;
- merge/reduction/verifier findings; and
- transaction findings.

Canonical-sort and select the global minimum only after required evidence is present. A secondary-buffer limit must never hide a potential primary.

Add a test adapter that emits a low-priority finding, fills optional capacity, and then emits a higher-priority finding. The higher-priority finding must become the task local minimum and participate in the global result under every schedule.

## 9. Stage-generation and reusable-service protocol

### 9.1 Service state

Use a service state machine equivalent to:

```text
constructing
-> starting_workers
-> ready_no_stage
-> ready_active_stage
-> ready_no_stage
-> shutting_down
-> destroyed
```

Only one stage may be active in V1.

### 9.2 Stage state

Each generation uses:

```text
created
-> planned
-> verified
-> resources_reserved
-> slots_ready
-> admission_open
-> admission_closed
-> draining
-> quiescent
-> merging
-> proposed
-> verified_output
-> committed | rolled_back
-> retired
```

Any failure enters controlled rollback, but retirement waits for generation quiescence and worker-handle release.

### 9.3 Generation binding

Bind generation to:

- active stage invocation pointer/owner token;
- queue state;
- every rank and slot;
- completion/admitted/running counters;
- cancellation/terminal hint epoch;
- join predicate;
- logical trace events;
- report/replay records; and
- resource reservations.

Runtime addresses do not enter bytes or ordering.

### 9.4 Activation and retirement

Under the service mutex:

- require `ready_no_stage` and no stale active generation;
- install the immutable active stage handle and generation;
- reset only generation-owned counters/hints;
- open admission; and
- notify workers.

Before retirement independently verify:

- admission closed;
- queue empty for the generation;
- admitted equals completed plus valid abandoned;
- running zero;
- every required slot terminal and acquired;
- no worker owns the stage handle;
- merge/verifier complete;
- resources reconciled; and
- transaction committed or rolled back.

Then clear the active handle under the mutex, advance to `ready_no_stage`, and notify any service waiters.

### 9.5 Stale-state rejection

Qualification must force:

- a delayed old-generation notification;
- an old rank remaining in a mutated queue;
- a worker retaining an old stage handle;
- a reused slot carrying old terminal status;
- an old cancellation hint; and
- generation wrap/duplicate identity.

Every mutation must be rejected before a new stage can commit.

## 10. Queue and progress corrections

### 10.1 Queue predicates

Queue wait predicates include:

- service shutdown;
- active stage generation;
- stage admission state;
- queue nonempty/not-full;
- cancellation/terminal wake state for that generation; and
- exact queue accounting.

A worker may pop only when the queue rank, descriptor, slot, and active generation agree.

### 10.2 Stage close versus service shutdown

`close_stage_admission(generation)` prevents further ranks for that generation and wakes submitters/workers. Workers remain alive and return to the service idle predicate after generation retirement.

`shutdown_service()` is permitted only with no active stage or through controlled rollback. It causes worker loops to exit and the thread factory to join them.

### 10.3 No timeout semantics

Remove any implication that production waits use periodic timeout polling. Tests may use an outer process timeout solely to report a hang. Logical progress watchdogs remain deterministic test infrastructure and do not cause a production semantic timeout.

## 11. Native memory and resource reporting

### 11.1 Exact controlled accounting

Report exact Component 01 reservations and reconciliation for:

- thread object/control records;
- queue storage;
- worker scratch explicitly allocated by the subsystem;
- task descriptors/slots;
- private outputs;
- merge/reduction storage;
- reports/replay/traces; and
- committed artifacts.

### 11.2 Profile observations

Report native thread-stack size only as:

- `unknown_portable_v1`;
- a conservative platform-profile bound with an explicit qualification source; or
- a future exact controlled provider version.

Do not infer stack bytes from worker count times an undocumented constant and call that an exact lease. Whole-process RSS, allocator internals, runtime TLS, sanitizer overhead, and operating-system scheduler storage are physical observations.

### 11.3 High-water classification

For each high-water field state whether it is:

- deterministic configured maximum;
- deterministic logical reservation high-water;
- schedule-observed controlled allocation high-water; or
- external physical observation.

Only the first two may be profile-deterministic canonical fields by default.

## 12. Component-specific adapter corrections

The base plan's adapter boundaries remain effective. Add these requirements to every parallel adapter:

- declare its semantic projection fields;
- declare profile and physical fields separately;
- declare exact slot publication protocol;
- declare deterministic per-task entitlement or wave construction;
- reserve mandatory local-minimum storage;
- bind all task/range records to stage generation;
- state whether any schedule-observed statistic is retained and its deterministic bound;
- state admission-failure versus semantic-failure expectations; and
- add memory-order/resource/generation mutations to Component 16 registration.

For Component 15 verification partitions, omitted work remains a semantic correctness failure. A schedule-observed high-water difference must not be misinterpreted as omitted coverage; coverage is proved by canonical logical keys and semantic counters.

For Component 16's test-only outer scheduler adapter, use the reviewed outer/inner policy. Concurrency-qualification cases run one at a time while exercising inner workers unless a separately qualified nested profile pre-reserves the complete combined budget.

## 13. Component 14 and 15 handoff corrections

### 13.1 Candidate fields

Extend the Component 14 pending candidate by separate immutable references to:

- semantic execution projections in canonical stage order;
- deterministic execution-profile records in canonical stage order;
- logical trace records/digests where retained; and
- optional physical observation appendices.

Do not place the profile record inside the public semantic mesh-content encoding.

### 13.2 Component 15 audit

Component 15 independently checks:

- one semantic execution projection per required stage;
- coverage counts against predecessor artifacts;
- owning-component verifier dispositions;
- profile compatibility and worker qualification;
- publication protocol and generation versions;
- resource entitlement/reconciliation evidence;
- no active/unjoined work;
- field-classification completeness; and
- semantic/profile/physical digest separation.

A worker-count change may alter permitted profile records and physical appendices while the public semantic result remains identical. Component 15 must reject both profile contamination of semantic bytes and semantic fields mislabeled physical.

## 14. Component 16 qualification closure

After implementing Component 17:

1. add both Component 17 review amendments to the explicit effective-contract manifest;
2. mark conflicting base obligations as superseded by stable controlling clause IDs;
3. recompute the effective-contract digest;
4. rebuild the implementation/build manifest with new source and provider versions;
5. register all new tests and mutations;
6. run semantic-equivalence cells across admitted serial/worker profiles;
7. run separate admission/fault cells;
8. produce one profile fragment per actually executed platform/build profile;
9. compare semantic case records across profiles while permitting classified profile differences;
10. run physical-observation exclusion and matched-summary/digest mutations;
11. aggregate the closed required profile set; and
12. run the independent evidence self-audit before release closure.

Earlier Component 16 serial/bootstrap evidence must not be relabeled as final Component 17 evidence.

## 15. Additional test files and focused cases

Add or extend:

- `TestExecutionEvidenceDomains.cc`;
- `TestExecutionProfileSnapshot.cc`;
- `TestExecutionPublicationMemoryOrder.cc`;
- `TestExecutionStageGenerations.cc`;
- `TestExecutionResourceEntitlements.cc`;
- `TestExecutionMandatoryFailures.cc`;
- `TestExecutionThreadFactory.cc`;
- `TestExecutionNativeMemoryDisposition.cc`; and
- existing report/replay/adversarial/mutation suites.

Register focused CTest cases without external frameworks.

### 15.1 Evidence-domain tests

Test:

- semantic bytes equal across serial, one, two, and maximum admitted workers;
- profile bytes differ only in declared fields across worker profiles;
- identical-profile repetitions have identical profile bytes;
- timing and raw high-water observations never enter semantic/profile digests unless explicitly permitted by their schema;
- unknown/duplicate/misclassified fields fail decoding/verifier; and
- Component 14/15 public digest remains unchanged when only execution profile/physical evidence changes.

### 15.2 Capability tests

Use an injectable immutable provider returning zero, one, many, and a value that would change on a second query. Prove exactly one query per service preflight and no stage re-query. Test request above maximum, hard ceiling, fallback allowed/forbidden, unsupported synchronization/floating profile, and replay/profile bytes.

### 15.3 Publication tests

Use qualification hooks that pause a worker between payload writes and terminal publication. Prove merge cannot read early. Mutate release to relaxed, acquire to relaxed, terminal store before payload completion, completed-count-only proof, and direct-fill early read. These mutations must be killed under ordinary tests and remain suitable for compiler race instrumentation.

### 15.4 Generation tests

Run many sequential stages through one service with queue capacity one. Inject stale rank, notification, slot status, cancellation hint, stage handle, and wrong generation in every transition. Test rollback followed by reuse, failure followed by reuse, and shutdown immediately after retirement.

### 15.5 Entitlement tests

Create tasks with unequal deterministic output needs and force every completion order. Test:

- exact aggregate slices;
- limit minus one, limit, limit plus one;
- canonical waves;
- bounded canonical growth phases;
- one task requesting beyond entitlement;
- host allocation failure in each purpose;
- contention mutation using first-racing shared reserve; and
- semantic failure stability across schedules.

### 15.6 Mandatory-failure tests

Exhaust optional secondary storage before emitting a higher-priority local finding. Place higher-priority findings in slow/later tasks. Race cancellation with service/resource/geometry failures. Verify local and global minima, truncation, replay, and no first-observed behavior.

### 15.7 Thread-factory tests

Test zero/one/maximum workers, partial creation at every rank, startup failure, join exactly once, shutdown, destructor defense, no detach, no worker outliving service, and source/build-audit mutations that construct `std::thread` directly or call `std::async`, `work_queue`, `taskqueue`, or `For_Each_In_Parallel`.

### 15.8 Memory-accounting tests

Verify exact controlled reservations. Reject records that claim an exact native stack or whole-process byte total without a qualified provider. Permit profile observations only in the physical domain. Test sanitizer overhead does not alter semantic or deterministic profile fields.

## 16. Structural gate corrections

For each gate, explicitly identify the deterministic measure. Examples:

- Component 06 logical node-pair visits and candidate emissions;
- Component 07 unique canonical relation evaluations;
- Component 08 construction count per event lineage;
- Component 09 classification-group seed queries;
- Component 11 carrier-local pairing work;
- Component 12 bounded candidate/ear attempts;
- Component 13 canonical invalidation/work records;
- Component 14 refinement/branch states;
- Component 15 canonical candidate-pair/probe coverage; and
- Component 17 configured task/queue/memory bounds and canonical reservation totals.

Do not define a release gate solely over elapsed time, raw queue high-water, thread slot utilization, or operating-system scheduling. Such observations may support advisory diagnosis only.

## 17. Corrected implementation sequence

Use the base plan sequence with these insertions and ordering constraints:

1. Extend versions, evidence domains, field classes, generation IDs, entitlement kinds, and failure subcodes.
2. Implement and verify `ExecutionProfile` with a one-time capability snapshot.
3. Implement `ExecutionThreadFactory` and pass lifecycle/partial-creation tests before the worker service creates threads.
4. Implement exact task keys, deterministic partitioning, plan/profile binding, and plan verifier.
5. Implement fixed slot storage, mandatory local-minimum storage, and release/acquire publication tests.
6. Implement stage-generation-bound ring queue and generation/ABA tests.
7. Implement worker floating guard and startup qualification.
8. Implement worker service startup/idle/reuse/shutdown through the thread factory.
9. Integrate Component 01 runtime bundle, transactions, cancellation, and generation retirement.
10. Implement deterministic slices, canonical waves, bounded growth phases, and entitlement tests.
11. Implement join/acquire, slot collection, canonical merge, prefix sums, reductions, and failure arbitration.
12. Implement the three evidence domains, codecs, digests, reports, replay, logical trace, and physical appendix.
13. Implement the independent Component 17 verifier and source/build thread-creation audit.
14. Pass a synthetic adapter through memory-order, generation, resource, failure, deadlock, race, and mutation suites.
15. Register the corrected V1 stage inventory.
16. Add Component 06, then 07/08, then remaining adapters in the base plan's order.
17. Integrate separate Component 14 candidate fields and Component 15 audits.
18. Add corrected structural gates.
19. Add the Component 16 test-only adapter under the reviewed outer/inner policy.
20. Update the Component 16 effective-contract and implementation manifests.
21. Run the complete final qualification/profile aggregation/evidence self-audit.

No parallel adapter may become the only executable path before its semantic-equivalence, memory-publication, resource-entitlement, generation, and failure-arbitration tests pass.

## 18. Corrected definition of done

Plan 17 is implementation-complete only when the base definition of done and all of the following are satisfied:

- `ExecutionProfile` captures one immutable capability snapshot and no stage re-queries physical capability;
- Component 17 thread creation is closed through `ExecutionThreadFactory`, no detach exists, and every factory-owned worker is joined exactly once;
- mutable service state resides only in the Component 01 runtime bundle;
- semantic, profile, logical-trace, and physical evidence schemas are separately encoded, versioned, verified, and digested;
- public semantic output and semantic primary failure are invariant across every admitted qualified worker profile;
- profile byte equality is required only for identical frozen profiles, and physical observations are never semantic authority;
- all slot/direct-fill payload reads follow a documented release/acquire or equivalent mutex happens-before edge;
- the service safely reuses workers across checked stage generations with no stale state or lifetime escape;
- deterministic slices, waves, or serial growth authorization prevent racing workers from deciding semantic resource outcomes;
- every task retains a mandatory local primary-failure candidate despite diagnostic truncation;
- exact resource claims are limited to controlled allocations and native stack/runtime observations are honestly classified;
- structural gates use deterministic logical measures rather than schedule-observed utilization or time alone;
- `work_queue`, `taskqueue`, `For_Each_In_Parallel`, `std::async`, and direct unowned thread creation are absent from authoritative execution paths;
- Component 14 and Component 15 preserve public semantic digest and publication authority independently of worker profile;
- Component 16's effective-contract graph, profile fragments, aggregate evidence, and independent self-audit include the final reviewed Component 17 contract; and
- every added evidence-domain, memory-order, generation, entitlement, mandatory-failure, thread-ownership, accounting, malformed-input, replay, deadlock, race, and mutation test passes with no required survivor.