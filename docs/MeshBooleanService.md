# Mesh Boolean one-call service

`boolean_operation(a, b, op, options)` is the ordinary application-facing entry
point. It owns the exact kernel, mandatory verifier registry, backend registry,
input preparation, backend request, backend selection, execution verification,
and immutable result publication for one invocation.

## Conservative defaults

A default `boolean_service_options` request:

- performs strict validation only;
- enables mandatory verification;
- requests the durable exact stratified result;
- uses `qualified_default` backend selection;
- performs no fallback;
- performs no geometry-changing normalization;
- retains an exact result when a requested mesh realization fails; and
- refuses to run when no matching qualified profile and manifest are supplied.

The in-tree `experimental_exact_v1` backend is therefore never selected by a
default call. A caller must either provide a matching qualification manifest for
a qualified backend or explicitly select the experimental backend, set
`allow_experimental_backend`, and choose
`allow_explicit_unqualified`. That opt-in is recorded in the result envelope.

## Preparation

`strict_validation` is the default preparation mode. `normalized` preparation
requires an explicit `normalization_policy` in `boolean_service_options`, and its
mode, units, tolerance, enabled operations, and non-planar-facet policy must
exactly match the product preparation contract. `diagnosis_only` cannot proceed
to Boolean evaluation and returns `normalization_required`.

The service returns preparation digests and geometry-change status in the same
`boolean_product_result` as backend, exact-result, realization, attribute,
verification, and qualification provenance.

## Results and failures

The return type is
`product_status_or<boolean_product_result_handle<T,I>>`. Publication is
transactional: an error returns no partially published result. Cancellation,
resource exhaustion, invalid preparation, unavailable or unqualified backends,
realization failure, verifier disagreement, and stale bindings remain distinct
typed outcomes.

When `retain_exact_result_on_realization_failure` is enabled, a failed finite
mesh realization is represented by an exact-stratified success envelope with a
failed `realization_attempt_record`; the durable exact result remains available.

## Expert dependency injection

`boolean_operation_expert(...)` is the separately named expert API. It accepts a
`boolean_service_dependencies<T,I>` containing a kernel, frozen verifier
service, frozen backend registry, deterministic-executor factory, and immutable
product store. The executor factory is an implementation substitution only; it
does not change the frozen operation or product policy. The store receives the
already verified immutable result and may persist it, but cannot replace the
published handle. Ordinary callers should use `boolean_operation(...)`; they do
not register internal verifiers, inject executors, manage backend adapters, or
control publication storage.
