# Mesh Boolean application migration

The product-facing entry point is now
`ygor::mesh_boolean::boolean_operation(a, b, op, options)`. The ordinary example
is `examples/MeshBooleanExample.cc`. It intentionally does not construct an
exact kernel, register verifier implementations, create a backend registry, or
manage publication storage.

## Move ordinary callers to the one-call service

Replace the former sequence of manual verifier registration,
`validate_operand_strict`, `make_boolean_context`, and
`assemble_boolean_output` with one service call:

```cpp
#include <YgorMeshesBooleanService.h>

using namespace ygor::mesh_boolean;
auto result = boolean_operation(a, b, operation::regularized_union, options);
```

The return value is a typed
`product_status_or<boolean_product_result_handle<T,I>>`. A successful envelope
always retains the authoritative exact result and records backend, preparation,
realization, attribute, verification, and qualification provenance.

## Choose preparation explicitly

Use the default `strict_validation` preparation only for meshes whose provenance
and construction process justify the strict B-rep contract. Strict validation
checks; it does not heal.

Imported STL, OBJ, scan, or CAD tessellations of unknown provenance are not
implicitly accepted by the Boolean service. Diagnose them first and make an
application decision. Proceeding with normalization requires both:

1. an explicit `normalization_policy` in `boolean_service_options`; and
2. the matching `product.preparation.normalization` contract fields.

Structural normalization may remove proven-irrelevant storage or perform other
authorized exact canonicalizations. Geometry-changing repair additionally
requires declared units, model tolerance, enabled operations, evidence review,
and a qualification profile covering that exact policy. The current examples do
not claim a general unknown-provenance repair workflow.

## Select backend maturity deliberately

Conservative defaults use `qualified_default` and fail with
`backend_unqualified` when no matching qualification manifest/profile is
provided. The built-in `experimental_exact_v1` backend is never selected
silently. Development or diagnostic use must explicitly set:

- `backend_selection_mode::explicit_backend`;
- `requested_backend = backend_id::experimental_exact_v1`;
- `allow_experimental_backend = true`; and
- `qualification_policy_mode::allow_explicit_unqualified`.

This opt-in remains visible in the result provenance and is not a production
qualification claim.

## Handle each result representation separately

`exact_stratified` returns the durable exact boundary without requiring a public
mesh. `exact_in_T_mesh` succeeds only when every output coordinate is exactly
representable in `T`. With
`retain_exact_result_on_realization_failure = true`, an unrepresentable finite
mesh returns an exact-stratified success envelope plus a failed
`realization_attempt_record`; the exact result is not discarded.

`certified_approximate_mesh` is a separately labelled result. It requires an
application-supplied displacement/tolerance policy, bounded deterministic search
limits, acceptance metadata, and an independently verified certificate. It must
never be treated as exact point-set output.

Every caller should inspect `result->representation`, the optional `mesh`, and
the optional `realization` record instead of assuming that success always means
an `fv_surface_mesh` exists.

## Review attribute transfer

Set an explicit attribute policy and inspect `result->attributes`. The report
contains exact/output mappings, transfers, omissions, conflicts, and issues.
Silent attribute loss is not part of the product contract. A mesh payload also
binds its output entities back to exact entities through
`mesh->attribute_binding`.

## Handle typed failures

Do not collapse failures into an empty mesh. At minimum, distinguish malformed
input, preparation requirements/failure, backend unavailable or unqualified,
resource/cancellation limits, unsupported topology, exact finite-coordinate
failure, approximation-policy rejection, attribute conflicts, stale/replay
bindings, and verifier disagreement. `message_key`, `detail`, backend evidence,
and replay binding are available for application diagnostics.

## Expert/internal dependency injection

`examples/MeshBooleanExpertExample.cc` demonstrates the separately named
`boolean_operation_expert` API. It injects an executor factory and immutable
product store while retaining the default kernel, verifier service, and frozen
backend registry. This interface is for tests, adapters, controlled integration,
and internal infrastructure. It is not the ordinary application API, and
ordinary callers should not register internal verifier implementations.
