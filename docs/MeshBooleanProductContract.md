# Mesh Boolean product contract (schema 3)

This document defines the product boundary introduced by Plan 15 P0. It does
not promote any Boolean backend and it does not implement normalization,
backend fallback, exact-result detachment, or approximate realization. Those
behaviours remain gated by later tracker components.

## Current support boundary

The in-tree symbolic engine is identified as `experimental_exact_v1` and has
maturity `experimental`. It is never selected by `qualified_default` unless a
qualification manifest names the exact backend build, capability digest,
preparation mode, result representation, and workload profile. Experimental or
candidate backends require both `explicit_backend` (or another explicitly named
producer policy) and `allow_experimental_backend=true` under
`allow_explicit_unqualified` qualification policy.

The currently implemented evaluator still requires strict, already-valid
operands. Unknown-provenance STL, OBJ, scan, or CAD tessellations are not an
implicitly supported workflow. `strict_validation`, `diagnosis_only`, and
`normalized` are distinct preparation contracts. No normalization operation is
enabled by default, and a geometry-changing normalization policy must state its
model unit, positive tolerance, and enabled operation set. The P0 types freeze
that boundary; the normalization service itself is P2 work.

## Authoritative result and representations

`exact_result_handle` is an immutable, shared-lifetime owner. It carries no
`boolean_context` owner token and remains valid after the producing invocation
is destroyed. P0 defines ownership, canonical bytes, digest, topology class,
operation, and backend binding. P1 will populate this durable storage from the
verified selected exact boundary and implement its complete canonical reader.

`boolean_product_result<T,I>` is a tagged envelope with exactly three result
representations:

- `exact_stratified`: the authoritative exact boundary, including empty and
  stratified non-manifold topology. It has no mesh payload.
- `exact_in_T_mesh`: a strict mesh realization whose emitted binary floating
  coordinates equal the exact targets. This remains a special-purpose mode,
  not the ordinary CAD-output target.
- `certified_approximate_mesh`: a separately tagged embedding bound to an
  explicit displacement/tolerance policy and certificate. It never claims
  exact point-set equality.

A failed mesh realization may be recorded while the exact result remains a
successful, retained product result. Product-option validation rejects any
policy that permits discarding that exact authority.

## Semantic policy versus search policy

`product_realization_semantics` states what a success means.
`realization_search_policy` states how candidates may be explored. They are
separate fields and are validated against the requested representation.
Changing nearest-value, neighbouring-value, candidate, or backtrack limits
cannot turn an exact request into approximate success.

## Backend selection and fallback

The supported selection contracts are:

- `explicit_backend`: one caller-named producer, no fallback.
- `qualified_default`: only a profile in the bound qualification manifest.
- `diagnostic_compare`: one declared producer plus independent comparators;
  agreement is evidence and never majority-vote publication.
- `explicit_fallback_chain`: a caller-ordered chain and an explicit allow-list
  of failure categories.

Fallback is rejected for internal invariant errors, capability mismatches,
backend or verifier disagreement, stale bindings, replay mismatch, exact-result
serialization errors, and qualification-policy violations. A later fallback
executor must retain the primary failure and all attempted backend identities.
P0 freezes these rules but does not execute a fallback chain.

## Versioning and decoding

Options, artifacts, errors, certificates, and replay bindings use schema 3.
Canonical records contain an eight-byte domain tag, schema, exact payload
length, and positional payload. Decoders reject:

- unknown schema, enum, backend, capability, or error values;
- malformed booleans, truncation, trailing bytes, and non-canonical duplicate
  lists;
- configured record, string, and vector limits before allocation;
- stale capability, manifest, exact-result, and cross-layer digest bindings;
- any reinterpretation of strict `exact_in_T` as approximate output.

Backend provenance contains stable backend ID, adapter semantic version, build
identifier, maturity, capability bits, and capability digest. Product results,
exact results, realization certificate references, fallback records, and replay
bindings carry or validate this provenance.

## Error categories

The product error schema retains the original engine categories and adds:
`normalization_required`, `normalization_failed`, `backend_unavailable`,
`backend_capability_mismatch`, `backend_disagreement`, `backend_unqualified`,
`exact_result_serialization_error`, `attribute_transfer_conflict`,
`approximation_policy_rejected`, and `qualification_policy_violation`.
`stale_binding`, `replay_mismatch`, and `verifier_disagreement` are explicit
fail-closed contract errors and are never fallback-authorized.

## P0 non-goals

P0 deliberately does not:

- change Components 3 through 10 or their exact semantics;
- normalize or repair an operand;
- detach the current selected-boundary implementation into the durable handle;
- implement exact-coordinate export or approximate embedding search;
- wrap backend execution or run independent adapters;
- transfer application attributes; or
- claim any production-qualified backend/result/preparation profile.
