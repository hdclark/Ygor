# Component 17: Performance and Deterministic Concurrency — Implementation Status

## Status

Component 17 is implemented as the deterministic
`bounded_fifo_worker_service_v1` execution service. It supplies the bounded
worker pool, canonical contiguous partitioning, task-private output ownership,
canonical task-order merge, worker floating-environment qualification,
cancellation coordination, and the serial semantic reference used by the Boolean
pipeline. It does not change any geometric, topological, symbolic, selection,
cleanup, canonicalization, verification, or publication semantics; its serial
reference is authoritative and the parallel path is conforming only when it
produces byte-identical results.

## Frozen V1 providers

- `execution_service`       = `bounded_fifo_worker_service_v1`
- `worker_lifetime`         = `invocation_owned_joinable_pool_v1`
- `worker_selection`        = `qualified_hardware_cap_then_descending_retry_v1`
- `partitioning`            = `canonical_contiguous_ranges_v1`
- `task_output`             = `exclusive_preassigned_slot_v1`
- `merge`                   = `canonical_task_rank_then_record_key_v1`
- `nested_parallelism`      = `serial_nested_fallback_v1`
- `cancellation`            = `cooperative_checkpointed_transactional_v1`
- `floating_environment`    = `establish_verify_restore_each_worker_v1`
- `resource_admission`      = `component01_reservation_before_queue_v1`
- `serial_reference`        = `adapter_owned_executable_serial_v1`

## Files

- `DeterministicExecutionTypes.h` — closed enums, strong ID domains, execution
  task key, capabilities, statistics, report, and subcodes.
- `DeterministicExecution.h` — the typed `execute_parallel` entrypoint with the
  bounded FIFO worker pool, canonical partition, task-order reduce, floating
  environment establishment, and cancellation.

## Implemented normative behavior

- The item range is partitioned into contiguous canonical tasks (one per
  worker); the serial path executes the whole range directly and is the
  normative reference.
- Each task writes into an exclusive preassigned slot; per-task results are
  reduced strictly in ascending task order, so the outcome is independent of
  thread scheduling, worker count, and completion order.
- Each worker establishes and verifies the strict floating-point environment
  before executing any task; an unqualified worker fails the task with a typed
  `unsupported_platform` error.
- Cancellation is cooperative and checkpointed: a pre-cancelled invocation
  returns `cancelled` before any task runs, and an in-flight cancellation
  signals workers, joins them, and returns `cancelled` with no partial output.
- Worker creation failure joins any partial pool and returns `resource_limit`
  without publishing partial results.
- Task exceptions and allocation failures are translated to typed failures.

## V1 fidelity boundary

V1 uses a simple bounded FIFO queue and one-contiguous-range-per-worker grain.
Work stealing, task priorities, runtime subdivision, per-stage child pools, and
concurrent mutation of shared topology are intentionally absent (they would
require a new provider version). The current pipeline components (02-15) execute
serially; Component 17 supplies the service and the serial reference, ready for
parallelizing independent per-facet/per-candidate work under later provider
versions while guaranteeing serial equivalence.

## Tests (`TestDeterministicExecution.cc`)

- `serial` — parallel sums over 1, 2, and 4 workers equal the serial reference.
- `order` — a non-commutative string concatenation reduce proves canonical
  task-order merge byte-for-byte against the serial reference.
- `determinism` — repeated parallel execution is identical.
- `cancellation` — a pre-cancelled invocation returns a typed `cancelled` error.
