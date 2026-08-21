# Construction-aware qualification generation and chains (P6.3)

Plan 16 P6.3 makes the permanent P6.2 recipe inventory executable. The
implementation is self-contained, C++17-only, deterministic, and fail-closed.
It does not qualify or promote a Boolean backend; it supplies canonical cases,
chain definitions, preparation observations, minimization transcripts, and
promotion artifacts for later qualification campaigns.

## Exact retained recipes

`YgorMeshesBooleanQualificationGeneration.h` defines an exact recipe model.
Coordinates are `exact_rational` values with an explicit signed-zero bit where
required. Faces use canonical 64-bit recipe indices. A recipe retains:

- the P6.2 recipe identifier, version, and recipe digest;
- the exact ordinal and deterministic seed;
- all exact operand vertices and rings;
- exact construction parameters and geometry categories;
- exact halfspaces for halfspace-built convex polytopes;
- valid/invalid preparation expectations and accepted typed failures; and
- a canonical case digest.

Every case also records a derivation kind. A frozen `inventory_ordinal` is
regenerated from its descriptor and ordinal and compared byte-for-byte during
validation. Valid fuzz, invalid/preparation fuzz, and minimized-regression
descendants retain the parent case digest; they cannot be substituted for a
counted inventory ordinal.

Typed materialization rounds each exact coordinate through the in-tree binary
rounding implementation and then proves equality with the exact rational.
Materialization fails with `output_not_representable` rather than silently
rounding. Index capacity is checked before any face index is emitted. Signed
zero is retained by bit pattern.

The executable descriptors bind all P6.2 pair-family ranges:

- ten construction-known families totalling 10,500 operand pairs; and
- six internally generated CAD-like families totalling 1,100 operand pairs.

The construction families cover exact rational halfspace parallelotopes,
concave profile extrusions, coplanar overlays, nested cavities and disconnected
components, feature contacts, legal alternate subdivisions, power-of-two and
adjacent-value transforms, integer inputs with non-dyadic intersections, thin
features, and bounded capacity/replay cases. The halfspace family validates that
every retained mesh vertex satisfies every exact inequality and that every
inequality is active on a face; stale construction certificates are rejected.

## Valid and invalid fuzz families

Valid fuzzing is construction-aware. Each mutator selects an exact family that
preserves the intended invariant: contact, coplanarity, alternate subdivision,
facet splitting, cavity/component addition, one-ULP changes, representable
transforms, increased local valence, or a chain-output seed. Disjoint boxes are
not used as the primary generated workload.

Invalid/preparation fuzzing applies one controlled defect with an exact label:
duplicate vertex use, duplicate facet, inconsistent orientation, open crack,
nonplanar facet, overlapping shells, sliver feature, or self-intersection. The
case contract records whether strict validation must pass, whether
normalization is expected, and the exact accepted error vocabulary.

`validate_qualification_preparation_observation` binds an observed preparation
run back to that contract. A successful normalization must report at least one
edit and its prepared operand must pass strict validation. A failed
normalization must still emit a nonzero report digest and may not claim a valid
prepared operand. Missing, vague, or mismatched defect reports fail closed.

## Operation chains

Five deterministic chain families materialize all 1,100 P6.2 chain ordinals.
Each chain has five to twelve steps and retains every operation, operand order,
preparation boundary, requested result representation, and right-hand recipe.
The chain harness:

1. snapshots the accumulator digest before every Boolean step;
2. optionally performs only a legal quadrilateral diagonal subdivision and
   re-ingests it;
3. invokes a supplied Boolean executor in the retained operand order;
4. retains every nonzero exact-result digest, even when no mesh realization is
   available;
5. re-ingests every successful mesh before it can feed the next step; and
6. proves that an expected typed failure left the accumulator unchanged before
   continuing.

Unexpected failures, missing exact-result digests, rejected re-ingestion, or a
failure that mutates prior state terminate the chain with a typed error. The
complete execution receives a canonical transcript digest.

The harness is callback-based so qualification runners can connect the public
Boolean service, alternative adapters, or a diagnostic comparison executor
without changing the frozen recipe definitions.

## Provenance-guided minimization and promotion

Failure provenance identifies implicated operand faces and vertices, plus an
optional chain step. It is canonicalized, deduplicated, range-checked against
the source case, and digest-bound. The deterministic minimizer attempts edits
in a fixed order, removing non-implicated features before implicated ones. Every
attempt is bounded; every accepted edit records before/after case digests.
Replaying the same source, provenance, predicate, and budget produces the same
minimized case and transcript digest.

A minimized result can be converted into a permanent-regression promotion
artifact. The artifact contains canonical case bytes, a stable `P63-REG-*`
identifier, a stable `P6.3.AUTO.*` permanent test ID, the minimization transcript
digest, and an artifact digest. Promotion occurs only through an explicit sink
after full revalidation. Stale identity, canonical bytes, transcript, or digest
are rejected before the sink is called, providing an atomic integration point
for later corpus tooling.

## Validation target

`Test_MeshesBooleanQualificationGeneration` checks descriptor/range bindings,
canonical replay, all four coordinate/index materializations, representative
strict validation, valid and invalid fuzz contracts, all five chain families,
transactional failure propagation, deterministic provenance-guided
minimization, promotion validation, and fail-closed stale bindings.
