# Mesh Boolean backend adapters (Plan 15 P4)

`YgorMeshesBooleanBackend.h` is the executable backend boundary for the mesh
Boolean product contract. It accepts only immutable `prepared_operand<T,I>`
values, retains the exact-kernel and verifier services for the complete request
lifetime, owns a cancellation source, and binds engine options, product options,
operation, preparation certificates, resource limits, and diagnostics into one
canonical request digest.

## Registered adapters

The default frozen registry contains two adapters:

- `experimental_exact_v1` is the sole producer. It wraps Components 3 through
  10 without changing their semantics, constructs the ordinary verified Boolean
  context from prepared operands, publishes the durable exact result, performs
  the requested strict or certified-approximate realization, and reruns product
  result verification before the attempt can be accepted.
- `independent_axis_aligned_box_v1` is diagnostic-only. It is intentionally
  limited to the declared `axis_aligned_box_pair_v1` workload, strict prepared
  operands, and an `exact_in_T_mesh` producer result. It never publishes a
  product result and cannot appear as a successful fallback producer.

A registry is mutable only during construction. `freeze()` records canonical
identity, semantic version, build identifier, maturity, capability bits,
capability digest, and adapter role in deterministic backend-ID order. Every
lookup compares the live adapter against the frozen snapshot. Version, build,
capability, maturity, and role drift fail closed before execution.

## Selection modes

`evaluate_backend_request` executes the product policy literally:

- `explicit_backend` runs exactly the named producer and never falls back.
- `qualified_default` considers producer entries in canonical registry order
  and accepts only a backend authorized by the caller-bound qualification
  manifest. An absent, stale, nonmatching, or unqualified profile is a failure.
- `diagnostic_compare` runs one named producer, then each named diagnostic-only
  adapter. The producer remains the only publication authority. Comparison
  disagreements are retained in `backend_comparison_record` evidence and never
  trigger voting or geometry replacement.
- `explicit_fallback_chain` tries the caller-ordered list only after a failure
  category is both globally fallback-permitted and explicitly present in
  `fallback_on`. The first failure, all attempted backend IDs, final producer,
  selection mode, and fallback decision are bound into the durable result and
  the execution envelope.

Fallback remains prohibited for internal invariants, stale bindings, replay or
serialization mismatches, verifier or backend disagreements, capability or
qualification-policy violations, normalization failures, and approximation
policy rejection. Diagnostic-only adapters are also prohibited as producers,
even when named in a fallback chain.

## Independent box reference

The box reference does not sample with tolerances. It validates that each input
is exactly one outward-oriented axis-aligned hexahedron with eight distinct
corners and six quadrilateral support planes. It then constructs the exact
Cartesian cut grid from both boxes and evaluates the requested Boolean truth
table in every open cell using exact dyadic rationals.

The canonical diagnostic payload records all axis cuts, every occupied cell,
occupied-cell count, exact volume, six-neighbour connected-component count,
boundary rectangle count, request digest, operation, workload profile, canonical
bytes, and semantic digest. Verification independently recomputes the payload
from immutable prepared operands; mutated occupancy, counts, exact volume,
canonical bytes, or digests produce `verifier_disagreement`.

Comparison against the producer independently decodes the emitted coordinates,
requires triangulated faces and valid indices, classifies every exact cut-cell
midpoint against the closed output shell, computes exact signed volume, checks
component count, and checks output bounds. The comparison record contains
expected and observed digests, mismatch count, individual invariant results,
outcome, and a canonical report digest. `disagree` remains a successful
execution result containing blocking-quality evidence for callers; it does not
replace or silently reject the producer geometry. Corrupt comparison evidence,
by contrast, fails verification and prevents publication. An adapter verifier
that returns `false` without an error is also treated as `verifier_disagreement`;
verification hooks cannot silently waive publication checks.

## Failure and ownership rules

Adapter evaluation is transactional. A failed or cancelled attempt publishes no
partial product. Resource limits cover diagnostic cells, records, and canonical
bytes; cancellation and arithmetic/allocation failures are typed failures.
Backend errors are mapped to product error categories and carry the responsible
backend identity when available.

Every successful execution names exactly one producer and verifies:

- the immutable request digest;
- preparation mode and normalization provenance;
- backend identity and capability snapshot;
- durable exact-result and product-result bindings;
- fallback provenance and retained primary failure;
- diagnostic attempt and comparison digests; and
- the final execution digest.

No adapter entry point bypasses mandatory Component 2 validation, stage
verification, durable exact-result verification, realization verification, or
product-envelope verification.
