# Mesh Boolean independent-backend qualification comparison

P6.4 adds a diagnostic-only qualification layer around the backend adapter
contract. It does not vote on, replace, or silently repair the producer result.
The exact producer remains the sole publisher. The independent backend is used
only to produce durable disagreement evidence and to block an unsupported
qualification claim. The evidence stores canonical engine and product option
bytes, so a request cannot be reinterpreted from a digest alone.

## Frozen workload

`make_qualification_backend_comparison_workload()` defines the version-1
`axis_aligned_box_pair_v1` workload. The workload contains five deterministic,
integer-coordinate, non-empty cases: overlapping boxes exercise union,
intersection, and both directed differences, while disjoint boxes exercise
symmetric difference. This intentionally narrow profile covers every public
operation without conflating independent-backend comparison with the separate
qualification of empty finite-`T` realization.

Every case is canonicalized and content-addressed. The campaign binds the
ordered case digests into one workload digest. A campaign is complete only when
its records exactly match that frozen order and digest. The complete campaign
has a public canonical encoder so it can be stored as a
`qualification_artifact_kind::backend_comparison` artifact.

## Evidence retained per backend attempt

`qualification_backend_attempt_evidence` preserves the backend identity and
maturity, adapter role, request digest, evaluation and verification outcomes,
typed failures, adapter attempt bytes, exact-result bytes, realization bytes,
public-output bytes, diagnostic bytes, and a payload digest. Failed attempts are
records, not missing data.

The comparison runner calls the producer and a candidate-or-qualified maturity
independent axis-box adapter directly so it can retain both complete attempts.
It then calls the comparator's public `compare` contract. The normal backend
publication path is unchanged.

## Guarded exact probes

For every open cell in the independent cut grid, the runner records the grid
shape and one evidence record for every dense cell index:

- exact lower and upper cell bounds;
- the exact rational midpoint, with validation that it lies strictly inside all
  three open intervals;
- comparator occupancy;
- producer point classification against the exact triangulated output; and
- a canonical probe digest.

A midpoint on a producer boundary cannot be called an agreement. Every occupancy
mismatch is represented as an individual semantic difference with the cell
index and expected/observed digests. Volume, connected-component, bounds,
verification, evaluation, and unsupported-comparison differences are preserved
separately.

## Disagreement minimization and classification

A material disagreement is blocking when first produced. The deterministic
minimizer reduces signed integer box coordinates toward zero while repeatedly
calling a supplied reproduction predicate. Every accepted edit binds before and
after case digests, and the final transcript binds the original and minimized
cases.

A disagreement can cease to be unexplained only after attaching both a valid
minimization transcript and an explicit resolution. The exact assessment tokens
are:

- `correct`
- `incorrect`
- `unsupported`
- `policy-different`
- `unresolved`

Valid resolved pairs are correct/incorrect, incorrect/correct,
policy-different/policy-different, or correct/unsupported in either backend
order. Reviewer, rationale, and nonzero evidence digest are mandatory.
`unresolved` is always blocking.

## Qualification gate

`qualification_backend_comparison_gate_passes()` returns true only when:

1. the campaign validates canonically;
2. the full frozen workload is present;
3. no record remains blocking; and
4. the independent backend reported no unsupported case in the declared
   workload.

This implements the P6.4 rule that any material unexplained independent-backend
disagreement blocks qualification. Comparison evidence can be referenced by a
P6.1 result summary using `qualification_artifact_kind::backend_comparison`.

## Tests

`MeshBoolean.QualificationComparison` executes the full workload against
`experimental_exact_v1` and the candidate-or-qualified maturity
`independent_axis_aligned_box_v1` adapter. It checks deterministic canonical
campaign construction, complete payload retention, guarded exact probes,
campaign gating, typed backend
failure retention, forced-disagreement minimization and classification, and
fail-closed mutations of probe and minimization bindings.
