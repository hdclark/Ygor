# Component 17 Review Amendment: Deterministic Execution Evidence and C++ Memory-Model Closure

## Status, precedence, and review conclusion

This file is a normative amendment to `component_17_performance_deterministic_concurrency.md` produced by the independent Component 17 review required by `tracker.md`.

The original specification remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, Components 01, 14, 15, and 16, execution-provider authors, and replay/report consumers must read both files together with `broad_plan.md` and every controlling review amendment.

The review found that the original Component 17 architecture is fundamentally aligned with the broad plan and should be retained. In particular:

- the serial semantic reference remains authoritative for every parallel-capable stage;
- one invocation-owned bounded service, immutable task inputs, task-private or exactly disjoint outputs, canonical merge, deterministic reductions, and canonical failure arbitration remain required;
- globally coupled topology publication remains serial in V1 unless a later provider proves complete non-interference and equivalence;
- authoritative arithmetic in every worker remains subject to the qualified Component 01/03 floating environment;
- cancellation, resource exhaustion, exceptions, service shutdown, and transaction rollback remain fail-closed and join all owned work;
- structural counters, rather than wall-clock timing alone, remain the normative performance-regression mechanism; and
- production and normative-test code remain portable C++17 with no external dependency.

The independent review identified thirteen mandatory corrections and clarifications:

1. Semantic result invariance must be separated from execution-profile evidence. A report that contains requested/actual workers, fallback attempts, or plan digests cannot be byte-identical across unlike execution profiles.
2. Schedule-observed queue, running-task, allocation, and memory high-water values must not be treated as canonical semantic facts unless a provider proves a deterministic definition independent of scheduling.
3. Task-output publication requires an explicit C++ memory-model contract. A persistent pool does not obtain a stage publication barrier merely from eventual service destruction or thread join.
4. Shared resource growth must not be awarded to whichever worker races first. Task entitlements or canonical admission waves must make hard-limit outcomes independent of worker timing.
5. Every task must retain a mandatory local primary-failure candidate even when optional diagnostics are truncated.
6. Reused service and queue state require versioned stage generations so stale tasks, notifications, slots, and cancellation state cannot cross stage boundaries.
7. Mutable worker-service ownership belongs in Component 01's invocation runtime bundle, not in canonical immutable `boolean_context` content.
8. Portable verification of “no detached or unowned thread” is a closed-world construction and ownership proof, not an unsupported attempt to enumerate all process threads.
9. `std::thread::hardware_concurrency()` and other execution capabilities form one immutable profile snapshot obtained during preflight; they are not repeatedly queried by stages or included in semantic public-content identity.
10. Exact accounting is limited to memory controlled by the subsystem. Native thread stacks and implementation-owned runtime memory are profile observations, not exact Component 01 byte leases.
11. Existing Ygor assessment must explicitly reject `YgorAlgorithms.h::For_Each_In_Parallel` and its per-item `std::async` execution in addition to the already rejected generic queues.
12. Component 14/15 semantic digest and publication contracts, and Component 16's semantic/profile/release-evidence domains, must remain effective when execution evidence is added.
13. Component 16 release closure must incorporate this controlling amendment, recompute the effective-contract digest, and qualify the corrected evidence, memory-order, ownership, resource, and lifecycle contracts.

No correction authorizes weaker correctness, coordinate-based topology, unchecked shared mutation, schedule-dependent semantic failure, first-completed publication, unbounded work, external dependencies, ordinary success before Component 15, or omission of Component 16 release evidence.

## A. Corrected authority and terminology

### A.1 Semantic authority

Component 17 is an execution provider. It may change physical scheduling and bounded implementation strategy only. It must not change:

- the logical domain processed by a stage;
- relation, event, occurrence, topology, geometry, precision, cleanup, canonicalization, or verification semantics;
- the canonical semantic artifact or public-content digest;
- the canonical semantic primary failure for an admitted execution profile without an intentional execution fault; or
- Component 15's sole authority to publish ordinary success.

An execution provider may return a closed execution-admission failure such as `unsupported_platform` or `resource_limit` before semantic stage work begins. Such a failure is not evidence that the serial semantic algorithm would have produced the same semantic result; it is a profile-bound inability to execute the requested qualified provider.

### A.2 Three evidence domains

Component 17 must define three non-overlapping evidence domains.

1. **Semantic execution projection**
   - canonical committed stage artifacts and their semantic digests;
   - public semantic mesh content and public-content digest;
   - operation, topology, classification, precision, cleanup, and final-verification dispositions;
   - canonical semantic primary and retained findings required by the owning component;
   - logical coverage counts and other counters classified as semantic invariants; and
   - the semantic result/failure projection consumed by Components 14-16.

2. **Deterministic execution-profile record**
   - execution provider, adapter, partition, queue, merge, reduction, and report versions;
   - frozen capability snapshot;
   - requested, desired, attempted, and actual worker counts;
   - admitted serial/parallel disposition;
   - deterministic worker-creation fallback attempts;
   - plan identity and digest;
   - configured queue, in-flight, byte, work, and task bounds;
   - deterministic plan/profile counters; and
   - profile-bound replay controls and qualification evidence.

3. **Physical observation appendix**
   - elapsed time and throughput;
   - raw task start/completion timestamps;
   - thread slot or operating-system scheduling observations;
   - schedule-observed queue, running-task, and in-flight high-water marks;
   - host allocation timing and implementation/runtime memory observations; and
   - optional profiler or sanitizer observations when available.

The physical observation appendix is non-authoritative. It must not enter public semantic bytes, semantic artifact digests, canonical primary-failure selection, ordinary-success status, or semantic replay equivalence.

### A.3 Equality and replay rules

For every admitted execution profile that reaches the same semantic stage and does not inject an execution fault, serial, one-worker, two-worker, and maximum-worker runs must have equal semantic execution projections.

Whole execution-profile records may differ when their frozen profile inputs differ. For one identical capability snapshot, actual worker count, execution policy, provider versions, hard limits, logical input, and qualification controls, the deterministic execution-profile record must be byte-stable.

Ordinary replay must reproduce the same semantic projection or the same closed profile-admission failure under a compatible qualified profile. It need not reproduce raw operating-system scheduling, elapsed time, physical high-water observations, or a different worker-count profile's plan bytes.

Tests and documentation must not use an unqualified phrase such as “the reports are identical across worker counts.” They must name the compared projection and its field-classification schema.

## B. Corrected report, counter, and digest contract

### B.1 Closed counter classes

Every counter field must have one closed class:

- `semantic_invariant` — identical for semantically equivalent admitted runs;
- `deterministic_plan_profile` — deterministic for one frozen execution profile and permitted to differ across explicitly different profiles;
- `schedule_observed_bounded` — may vary with a qualified schedule but must remain within a deterministic configured or derived bound; or
- `physical_non_authoritative` — diagnostic only and excluded from canonical semantic evidence.

The former phrase “bounded execution statistic” is read through these two distinct classes. A schedule-observed value must not be silently upgraded to a deterministic plan/profile field.

### B.2 High-water values

Configured capacities, preflight reservations, canonical task counts, logical work counts, and checked resource reconciliation are deterministic evidence.

Raw queue occupancy, simultaneously running tasks, allocator-resident bytes, and host/runtime high-water values may vary with scheduling. In V1 they are `schedule_observed_bounded` unless the provider defines and proves a schedule-independent logical high-water quantity. Qualification checks:

- observed value is within the frozen deterministic bound;
- accounting never underflows, overflows, or leaks;
- semantic outcomes and semantic counters are unchanged; and
- physical observations are excluded from semantic digest domains.

A structural gate must be stated over deterministic logical work, configured bounds, canonical reservations, or another reviewed schedule-independent measure. It must not fail a correct run merely because a legal operating-system schedule produced a different raw queue high-water mark.

### B.3 Digest separation

Component 17 must define distinct domain-separated digests for:

- semantic execution projection;
- deterministic execution-profile record;
- logical execution trace, when retained canonically; and
- physical observation appendix, if an integrity digest is useful.

The Component 14 public-content digest and Component 15 ordinary-success semantic digest must exclude actual worker count, plan digest, fallback attempts, physical trace, timing, schedule-observed high-water values, and presentation-only execution metadata.

Component 14 may carry the profile record and physical appendix in the pending candidate as separate evidence. Component 15 audits publication-relevant execution claims and domain separation; it must not make public semantic mesh identity depend on the execution profile.

## C. Execution capability snapshot and admission

### C.1 Immutable capability snapshot

Before creating the worker service, Component 17 must construct one immutable `execution_capability_snapshot` containing:

- qualified platform/build/floating profile identifier;
- one captured `std::thread::hardware_concurrency()` result;
- supported maximum background workers;
- Component 01 hard worker/thread/resource ceilings;
- supported queue, atomic, mutex, condition-variable, and floating-environment capabilities;
- native-thread-memory accounting disposition;
- provider versions; and
- test-only forced capability values when qualification is enabled.

The hardware query occurs once per invocation service preflight. A zero result is normalized according to the frozen V1 rule and both raw and normalized values are reported. Stages must not re-query hardware, environment, or provider capability.

The snapshot is profile-bound execution evidence. The immutable `boolean_context` contains only frozen caller requests, capability contract/version, and stable semantic requirements. The mutable service handle and live synchronization state remain in Component 01's invocation-owned runtime service bundle.

### C.2 Admission and semantic equivalence

Worker creation, worker environment qualification, and complete service preflight occur before semantic parallel work. Deterministic fallback is permitted only by frozen policy and only before a stage begins.

Qualification distinguishes:

- **semantic-equivalence cells**, in which every compared profile is successfully admitted and must produce the same semantic projection; and
- **admission/fault cells**, in which worker creation, platform qualification, reservation, or injected execution failure is intentionally exercised and the expected profile-bound typed failure is compared.

A profile-admission failure must not be relabeled as the serial algorithm's semantic failure or used to weaken the requested execution policy silently.

## D. C++ memory-model and output-publication contract

### D.1 Slot publication

Task-private output is not visible to merge merely because task code returned. Each slot must implement a documented happens-before edge:

1. the worker writes all task-private payload, findings, precision records, counters, resource usage, and direct-fill completion evidence;
2. the worker finalizes those records without concurrent mutation;
3. the worker publishes the terminal slot state with release semantics or while holding the mutex whose unlock establishes equivalent release;
4. join/merge observes the terminal state with acquire semantics or through the corresponding mutex acquisition; and
5. only after that acquire may merge read the payload or direct-fill range.

A relaxed atomic terminal store/load pair is insufficient. The advisory completed count cannot publish payload by itself. Condition-variable notification is not a substitute for the predicate's protected or release/acquire state.

### D.2 Queue synchronization

Queue head, tail, count, closed state, stage generation, admitted/running accounting, and wake predicates are protected by one reviewed mutex or an equivalently proved synchronization design. V1 must not introduce a lock-free queue.

The atomic terminal hint is only a wake/suppression hint. It must not publish task payload, choose a failure, establish slot completion, or replace queue predicates.

### D.3 Exactly disjoint direct fill

A direct-fill worker may write only its preassigned range. The task's terminal release publishes completion of the whole range. Merge acquires the terminal state before reading any element and independently verifies exact fill, no overlap, no missing element, and count/fill agreement.

### D.4 Publication mutations

Component 16 must kill mutations that:

- publish terminal state before the last payload write;
- use relaxed terminal publication without another happens-before edge;
- read slot payload before acquiring terminal state;
- treat completed count as sufficient publication;
- reuse a slot while another stage can still observe it; or
- read a direct-fill range before its owner task is terminal.

## E. Deterministic resource entitlements

### E.1 No race-awarded semantic capacity

A shared “reserve more bytes” operation whose first racing worker succeeds can make result or failure depend on scheduling. It is prohibited when the contested capacity can affect required output, the primary failure, retained mandatory evidence, or semantic counters.

Before task admission, the stage must use one of these reviewed deterministic models:

- exact pre-counting and preassignment;
- conservative per-task slices assigned by canonical task rank;
- canonical fixed waves whose complete aggregate reservation succeeds or fails before the wave runs;
- two-pass count/prefix/fill construction; or
- a canonical growth schedule in which entitlement decisions are made serially by complete task key before workers receive the next chunk.

A worker may consume only its assigned entitlement. Needing more than the entitlement produces a private finding at that task's canonical key or requests the next canonical growth phase; it must not race other workers for an undifferentiated remaining pool.

### E.2 Resource failure witnesses

Checked preflight failure records the first canonical task/range whose deterministic entitlement cannot be represented or reserved. Runtime host `std::bad_alloc` remains a typed execution failure with task and allocation-purpose witnesses, but it cannot silently drop a required record or permit another task to consume the failed task's semantic entitlement.

### E.3 Native thread memory

Component 01 accounts exactly for subsystem-controlled C++ allocations, worker-control objects, queue storage, task slots, explicit worker scratch, and retained artifacts. Portable C++ does not expose a reliable exact native thread-stack byte count.

The capability snapshot must state whether native stack size is unknown, platform-qualified as a conservative external bound, or controlled by a future reviewed provider. Worker count is always hard-bounded. Native stack/runtime memory is profile evidence and must not be falsely reported as an exact resource lease.

## F. Mandatory failure retention and arbitration

Each task slot must reserve non-optional capacity for:

- one normalized minimum local finding under the Component 01 total order;
- one cancellation observation when applicable;
- required resource reconciliation evidence; and
- the task terminal status.

Optional secondary findings and verbose diagnostics may be truncated deterministically, but processing a task may not discard a finding that could become the global primary. The task updates its mandatory local minimum as findings arise. After required tasks are terminal, global arbitration combines every task-local minimum, service/queue/worker findings, cancellation findings, merge/verifier findings, and transaction findings.

A task whose optional diagnostic buffer is full continues to maintain its mandatory minimum and explicit truncation summary. First-observed failure, first-filled diagnostic buffer, worker slot, completion timing, and cancellation flag race never select the public error.

## G. Service reuse, stage generations, and shutdown

### G.1 Separate service and stage state

The reusable invocation service has a service lifetime distinct from each stage generation. V1 must distinguish:

- service starting, ready, shutting down, and destroyed;
- no active stage versus one active stage generation;
- stage admission open, admission closed, draining/joining, quiescent, merging, committed, or rolled back; and
- cancellation/terminal state belonging to a specific stage generation or to service shutdown.

Closing one stage queue must not terminate the reusable service. Service shutdown must not be confused with ordinary end-of-stage admission.

### G.2 Generation identity

Every queue entry, task descriptor, slot, stage invocation, wake predicate, finding, trace event, and worker-acquired stage handle must carry or be validated against a monotonically advancing non-wrapping stage-generation identity owned by the invocation runtime.

A new stage generation may begin only after independent quiescence verification proves:

- queue empty and accounting reconciled;
- no admitted/running task;
- all prior slots terminal and merge-consumed or destroyed;
- no worker retains a prior stage handle;
- prior cancellation and terminal hints reset under synchronization;
- prior resources transferred or released; and
- the prior transaction committed or rolled back.

Stale queue ranks, delayed notifications, cached pointers, ABA-like slot reuse, and old cancellation observations must be rejected or rendered unable to match the new generation.

### G.3 Wait and progress rules

Production waits use predicates and explicit notifications; they do not rely on polling timeouts. Test-process timeouts may diagnose a deadlock as infrastructure protection but are not production semantics.

Shutdown and rollback wake every relevant queue, join, reservation, and stage wait. No worker waits for child work on the same bounded pool in V1. No submitter holds a queue, component, transaction, or resource-manager lock while waiting for capacity needed by workers.

## H. Existing Ygor and build integration

The original rejection of `YgorThreadPool.h::work_queue` and `YgorContainers.h::taskqueue` remains correct.

The review additionally rejects `YgorAlgorithms.h::For_Each_In_Parallel` for authoritative bounded Boolean work because its per-element `std::async`/future model:

- does not provide a bounded invocation-owned worker set;
- permits implementation-selected asynchronous or deferred execution;
- has no Component 01 reservation, cancellation, transaction, floating-environment, private-output, canonical-merge, or failure-arbitration contract;
- can create work proportional to the input range without a canonical bounded queue; and
- cannot expose the required deterministic execution plan and ownership proof.

Preserve these existing utilities for unrelated callers. Do not retrofit them as part of Component 17 and do not route Boolean work through them.

Reuse:

- the repository's existing required CMake `Threads` package and `Threads::Threads` linkage;
- the strict bounded target and floating-build isolation established by Component 01;
- portable C++17 `std::thread`, mutex, condition variable, atomics, and standard containers inside the new bounded subsystem; and
- Component 01/03 services rather than duplicating errors, resources, cancellation, transactions, canonical bytes, SHA-256, or floating qualification.

No external task system, allocator, profiler, benchmark framework, concurrent container, or downloaded dependency is permitted.

## I. Closed-world thread ownership and independent verification

Portable C++17 does not provide a reliable API to enumerate every thread in the process. The “no detached or unowned work” guarantee therefore uses a closed-world construction:

- all Component 17 production thread creation goes through one reviewed invocation-owned thread factory;
- the factory records each worker object's creation rank, joinable state, ownership, startup disposition, and exactly-once join disposition;
- no Component 17 source outside that factory may directly construct or detach a `std::thread`;
- worker entrypoints receive only immutable service ownership and never leak a native handle;
- shutdown verifies every factory record is joined and every worker loop exited; and
- build/source audit and mutation tests reject a direct-thread or detach bypass.

The independent verifier audits the complete factory registry and service state. It must not claim to detect arbitrary unrelated application threads or use platform-specific process enumeration as a normative dependency.

## J. Component 14, 15, and 16 integration

### J.1 Component 14 handoff

The pending Component 14 candidate may contain or reference:

- semantic execution projection;
- deterministic execution-profile record;
- logical trace evidence; and
- optional physical observation appendix.

These domains must remain separately versioned and encoded. Execution profile or physical observations must not alter canonical public component/vertex/facet order, public coordinate bits, public-content bytes, correspondence-equivalence classes, or semantic public digest.

### J.2 Component 15 audit

Component 15 independently verifies publication-relevant execution claims, including:

- required logical work coverage;
- no omitted verification partitions;
- semantic counter consistency;
- worker floating qualification;
- plan/profile compatibility;
- resource reconciliation;
- no active/unjoined work at publication; and
- correct semantic/profile/physical digest-domain separation.

Component 15 does not require unlike worker profiles to have identical whole execution reports. It compares semantic projections and validates each profile record under its own frozen profile.

### J.3 Component 16 release closure

The Component 16 effective-contract manifest must add this amendment as controlling Component 17 text, mark conflicting base clauses as superseded where necessary, and recompute the effective-contract digest.

Release closure must include:

- semantic-equivalence matrices across admitted worker counts and forced schedules;
- execution-profile byte stability within identical profiles;
- cross-profile semantic equality with permitted profile-record differences;
- physical-observation exclusion tests;
- memory-order and stale-generation mutations;
- deterministic resource-entitlement and race mutations;
- thread-factory bypass and detach mutations;
- final Component 17 structural gates; and
- aggregate profile-fragment evidence and independent evidence self-audit.

Component 16 outer case concurrency and Component 17 inner engine concurrency remain jointly bounded under the reviewed Component 16 amendment. Cross-case pressure must not reinterpret one case's semantic result.

## K. Additional required tests and mutations

In addition to the original specification, qualification must include:

- semantic projection versus execution-profile versus physical-appendix codec and digest known-answer tests;
- worker-count changes that preserve semantic bytes while changing only permitted profile fields;
- identical-profile repeated runs proving deterministic profile bytes;
- raw queue/running/memory high-water variation proving bounded classification and semantic exclusion;
- Component 14/15 mutations that incorrectly include worker count, plan digest, or timing in public semantic digest;
- one-time hardware-capability snapshot tests, including forced zero and changing test-provider values;
- direct admission-failure versus semantic-failure classification tests;
- release/acquire slot publication and direct-fill happens-before mutations;
- stale stage-generation, delayed notification, slot ABA, old cancellation, and old queue-entry mutations;
- racing resource-growth tests in which completion order is permuted and deterministic task entitlements remain unchanged;
- local diagnostic exhaustion with a later higher-priority local finding;
- thread-factory registry, direct `std::thread`, detach, double-join, missed-join, and leaked-stage-handle mutations;
- native-stack accounting tests that reject false exact-byte claims;
- prohibited use of `work_queue`, `taskqueue`, `For_Each_In_Parallel`, or `std::async` in Component 17 production paths;
- predicate-only waits and no production timeout-polling tests; and
- effective-contract, freshness, aggregate-profile, and evidence-auditor tests incorporating this amendment.

## L. Corrected definition of done

Component 17 implementation is complete only when the original definition of done and all of the following are satisfied:

- semantic, deterministic execution-profile, logical-trace, and physical-observation domains are explicit and non-overlapping;
- semantic projections are identical across all admitted qualified worker counts and schedules, while profile differences are compared only under their declared schema;
- schedule-observed high-water and timing values cannot affect semantic bytes, primary failures, structural correctness gates, or ordinary success;
- every task-output read is preceded by a proved C++ happens-before publication edge;
- direct-fill ranges have exact ownership and acquire-before-read verification;
- hard resource outcomes use preassigned canonical entitlements or canonical waves rather than racing worker acquisition;
- every task retains a mandatory local primary-failure candidate despite optional diagnostic truncation;
- reused services isolate stages with validated generation identities and cannot consume stale queue, slot, cancellation, or notification state;
- mutable service state remains in the invocation runtime bundle and never enters canonical immutable context content;
- the capability snapshot is captured once, replay/profile classified, and not repeatedly queried by stages;
- exact accounting claims cover only subsystem-controlled allocations, with native stack/runtime memory honestly classified;
- all Component 17 threads are created and joined through the closed-world reviewed factory and no production detach exists;
- existing Ygor asynchronous utilities are preserved for unrelated callers but rejected for authoritative Boolean execution;
- Component 14 public semantic identity and Component 15 publication authority remain independent of execution profile;
- Component 16 consumes this controlling amendment and closes the complete semantic/profile/memory-order/resource/lifecycle qualification matrix; and
- all added malformed-input, mutation, race, deadlock, resource, replay, report, and digest-domain tests pass with no required survivor.