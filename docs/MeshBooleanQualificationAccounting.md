# Mesh Boolean false-success accounting (P6.5)

P6.5 adds the qualification observer that sits outside Boolean production. It
never repairs a result, changes backend selection, or upgrades backend maturity.
Its job is to preserve the raw evidence for one execution, independently audit
published success, normalize the execution to exactly one Plan 16 outcome, and
aggregate that outcome across the full qualification dimensions.

The public interface is `YgorMeshesBooleanQualificationAccounting.h`.

## Independent success evidence

`observe_qualification_product_success(...)` starts from a frozen
`boolean_product_result` and records separate checks for:

- the product-result contract and durable exact-result binding;
- the declared exact, strict finite-coordinate, or certified-approximate
  representation semantics;
- strict Component 2 re-ingestion of every published mesh;
- an independently reconstructed edge-incidence graph, connected components,
  Euler characteristic, closedness, and opposite edge directions;
- strict or approximate realization-certificate replay from the published output
  bits;
- guarded occupancy probes supplied by an independent oracle;
- embedding, orientation, and shell-nesting acceptance from strict re-ingestion;
- attribute/provenance report and output-binding replay;
- certified-approximate displacement and relaxed-relation bounds; and
- feeding a verified mesh into the next operation-chain consumer when the chain
  profile requires it.

Each check is `passed`, `failed`, or `not_run`. This distinction is deliberate:
a failed correctness check proves that a published success is false, while a
missing independent checker is a verifier disagreement. Qualification therefore
fails closed without falsely accusing the producer when infrastructure did not
run.

`reconstruct_qualification_mesh_topology(...)` does not call producer grouping,
stitching, or canonicalization helpers. It scans public face rings, rebuilds the
undirected edge-use table and face adjacency graph in canonical order, and emits
its own digest. Invalid output is retained as failed evidence instead of being
silently discarded.

## False-success classification

`account_qualification_case(...)` applies one deterministic precedence order and
emits one `qualification_outcome`.

For a published success, any failed semantic check becomes `false_success` with
one or more explicit reasons: semantic mislabeling, stale exact-result binding,
strict re-ingestion rejection, incorrect topology, certificate replay failure,
incorrect occupancy, embedding, orientation, shell nesting, attribute mapping,
or approximate-bound violation. Every false success is blocking.

If no correctness check failed but required evidence was not run, the outcome is
`verifier_disagreement`. Nondeterminism, backend disagreement, resource/timeout,
expected and unexpected typed failures, and infrastructure failure remain
separate outcomes. Exact and certified-approximate successes are never merged.

Expected and unexpected typed failures plus resource/timeout outcomes are also
marked as safe failures for correctness accounting. Unexpected safe failures
remain blocking for product thresholds; “safe” means only that no incorrect mesh
was published.

## Dimensioned campaign accounting

`make_qualification_accounting_campaign(...)` canonicalizes case records and
counts each outcome by:

- backend;
- result representation;
- preparation policy;
- Boolean operation;
- coordinate and index specialization;
- toolchain; and
- geometry category.

The campaign stores total cases, safe failures, false successes, and all blocking
issues separately. Integer overflow, duplicate case identities, non-canonical
ordering, stale digests, altered counts, or canonical-byte corruption fail
validation. `qualification_false_success_gate_passes(...)` requires a complete
campaign with zero false successes and zero blocking outcomes.

`make_qualification_result_summary_from_accounting(...)` is the bridge to the
P6.1 machine-readable result summary. The summary recomputes its blocking and
false-success totals from the dimensioned counts, so a caller cannot hide a
false success by editing aggregate fields.

## Qualification boundary

P6.5 supplies executable evidence and accounting contracts; it does not claim
that a production campaign has run. P6.8 through P6.11 must still execute the
frozen platform, sanitizer, fuzz-duration, performance, and candidate matrices,
resolve every blocking outcome, and publish the reproducible report before any
profile can become `qualified`.
