# Permanent Mesh Boolean qualification corpus (P6.2)

Plan 16 P6.2 freezes the permanent corpus inventory that later qualification
phases execute. It does not promote a backend and it does not treat a reserved
case count as a passing campaign. The in-tree backend remains experimental.

## Canonical inventory contract

`YgorMeshesBooleanQualificationCorpus.h` defines a versioned inventory with four
record kinds:

1. construction-known operand-pair families;
2. internally generated or licensed CAD-like operand-pair families;
3. deterministic operation-chain families; and
4. singleton minimized regressions.

Each record binds a stable recipe identifier/version/digest, a non-overlapping
ordinal range, source and redistribution provenance, exact expectation digest,
permanent test ID, and explicit coverage dimensions. The constructor sorts and
canonicalizes every set-valued field, rejects duplicate IDs and test bindings,
rejects overlapping ordinal ranges for one recipe version, and computes a
record digest. The inventory then computes independent record-set, category
coverage, expected-outcome, and complete inventory digests.

A validated inventory can be converted into ordinary P6.1
`qualification_corpus_binding` records. Consequently, any recipe, count,
coverage, expectation, source, or provenance change changes the campaign's
material binding and invalidates the prior qualification claim.

## Corpus floors

The checked-in `tests/mesh_boolean/corpus/inventory.tsv` contains compact recipe
ranges rather than 12,000 duplicated mesh files:

| Corpus class | Bound cases | Required floor |
|---|---:|---:|
| Construction-known operand pairs | 10,500 | 10,000 |
| CAD-like operand pairs | 1,100 | 1,000 |
| Operation chains | 1,100 | 1,000 |
| Minimized regressions | 21 | all retained |

Chain records require at least five steps. The checked-in families cap their
recipes at twelve steps so later execution and minimization can retain exact
chain definitions without an unbounded implicit workload.

## Construction-known family contracts

The generated ranges are partitioned by stable recipe version. P6.3 generator
implementations must consume the ordinal directly and reproduce the bound
recipe digest; changing the mapping requires a new recipe version.

- `exact-halfspace-skew-convex-v1`: rational halfspaces with non-axis-aligned
  normals, exact clipping, and construction-known containment/contact facts.
- `exact-profile-extrusion-concave-v1`: rational planar profiles with controlled
  reflex vertices and exact extrusion heights.
- `exact-coplanar-overlay-v1`: exact same/opposite-orientation partial and full
  coplanar overlays.
- `exact-nested-shell-cavity-v1`: disconnected shells, nested cavities, and
  polarity-controlled containment.
- `exact-feature-alignment-contact-v1`: vertex/edge/face contacts and increasing
  local valence, including stratified non-manifold results.
- `exact-subdivision-refinement-v1`: exact alternate triangulations and polygon
  refinements of the same boundary.
- `exact-representable-scale-bits-v1`: power-of-two transforms, signed zero,
  adjacent floats, subnormals, and exponent extremes.
- `exact-nondyadic-intersection-v1`: integer/rational inputs whose exact
  intersections require non-dyadic coordinates.
- `exact-thin-sliver-dense-v1`: controlled thin features, slivers, severe aspect
  ratios, and dense tessellation.
- `exact-capacity-replay-boundary-v1`: index/resource boundaries and canonical
  serialization/replay cases.

The CAD-like ranges use deterministic profile/extrusion, multibody/cavity, thin
feature, dense tessellation, attribute/seam, and controlled-defect recipes. They
are classified as internally generated and project-owned; no external license
claim is implied.

## Coverage gates

For every required geometry category, the inventory must collectively contain:

- union, intersection, both difference directions, and symmetric difference;
- both `A,B` and `B,A` operand orders;
- binary32/binary64 coordinates with uint32/uint64 indices;
- exact stratified, strict exact-in-`T`, and certified approximate results; and
- strict validation, diagnosis-only, and normalized preparation.

The category set includes non-box intersections, rotated/skewed convexity,
concavity, disconnected components, cavities, coplanar overlays, high valence,
thin/sliver/dense features, alternate subdivisions, scale and floating-point
extremes, non-dyadic intersections, stratified results, index/resource limits,
attribute/provenance conflicts, normalization defects, and replay/serialization.

Expected outcomes are restricted to verified exact success, verified certified
approximate success, expected typed failure, or an explicit timeout/resource
outcome. Any record that expects a typed failure must enumerate the accepted
`product_error_code` values.

## Regression retention and fail-closed behavior

The inventory retains the eight legacy manifest records and every distinct
executable G1-G9 plan-gap regression. Regression records are singleton recipes
with stable permanent test IDs. Tests reject deletion, substitution, duplicate
IDs, stale digests, vague failure expectations, missing categories, a corpus
below any floor, or chains shorter than five steps.

P6.3 must implement the construction-aware generators and chain harnesses that
materialize these exact ordinal ranges. P6.10 must execute them under the frozen
campaign. Neither step may reinterpret this inventory without changing its
canonical digest and invalidating prior evidence.


## P6.6 controlled CAD-like source boundary

The P6.2 CAD-like ordinal ranges are internally generated and remain directly
repository reproducible. Additional licensed and private campaign sources enter
through `qualification_cad_ingestion_manifest`, not by editing this inventory or
committing opaque meshes. The ingestion record binds source class, intended
model tolerance, license/provenance, preparation evidence, expected outcome,
redistribution, and content digests. External/private records carry compact,
distinct repository representatives while the full source remains
content-addressed or private digest-only. The resulting P6.1 corpus bindings are
material qualification inputs, so source or evidence changes invalidate the
frozen campaign.
