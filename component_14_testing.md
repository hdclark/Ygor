# Component 14: Test Oracles, Adversarial Generation, and Qualification

## 0. Purpose

Establish evidence that every component satisfies its contract across ordinary inputs, every specified degeneracy, extreme floating-point values, arbitrary traversal schedules, and composition into complete Booleans. Convert every discovered failure into a permanent deterministic regression.

## 1. Input contract

Accept component APIs and verifiers, replay serialization, deterministic random seeds, operation/domain policies, supported `T`/`I` combinations, and configurable test budgets.

Tests must compare exact topology/set semantics or certified predicates, not rendered images, decimal tolerances, triangle counts, or volume alone.

## 2. Required behavior

### Unit and known-answer suites

- Test arbitrary-precision arithmetic against generated known-answer vectors and algebraic identities.
- Test every predicate relation category, permutation law, filter acceptance path, and exact fallback.
- Test each graph/registry/arrangement operation with hand-auditable fixtures.
- Test all typed failures, resource boundaries, index overflow, and transaction rollback.
- Test `result_topology_not_supported` separately from invalid input, representability, and invariant failures.

### Exact low-complexity oracles

Implement deliberately slow, structurally independent oracles for small inputs:

- Exhaustive pair enumeration instead of broad-phase pruning.
- Direct exact segment/polygon arrangements for simple facets.
- Independent exact point classification using multiple symbolic rays.
- Independent geometric-occurrence partitioning and spherical-link reconstruction.
- Brute-force bounded realization assignments and exhaustive triangle pairs.
- Voxel-free analytic solids and exact halfspace/polytope cases with known occupancy.

An oracle may share the fundamental exact number type but should not reuse the production control flow being checked.

### Degeneracy corpus

Systematically cover:

- Disjoint, containment, equal, and complementary local boundaries.
- Vertex-vertex, vertex-edge, vertex-face, edge-edge, edge-face, and face-face contact.
- Proper crossing, tangency, collinearity, coplanar partial/full overlap, same/opposite orientation.
- Intersection through original vertices/edges, multiple events at one point, and multiple intervals on concave facets.
- Cavities, nested/disconnected shells, high valence, long thin facets, and radically different source subdivisions.
- Signed zero, subnormal and extreme exponents, adjacent floats, cancellation, and distinct exact points with equal nearest-`T` rounding.

Generate exact rational scenarios first and retain only those whose source vertices are representable as required, so expected relations are known independently.

### Property and metamorphic testing

Check, where mathematically applicable:

- Commutativity of union/intersection/xor and correct operand swap for differences.
- Idempotence, identity with empty, `A-A`, absorption, De Morgan relations within the finite-solid domain, and operation consistency.
- Invariance under facet-ring rotation, index/facet/component permutation, legal source subdivision, and internal triangulation change.
- Invariance/equivariance under exactly representable translations, axis permutations, sign flips with corrected orientation, and power-of-two scaling.
- Equivalent output across thread counts, work partition, BVH choices, filters forced on/off, and hash seeds.

Compare canonical exact boundaries when finite-`T` realization legitimately differs or fails.

### Fuzzing and shrinking

- Use topology-preserving generators for valid embedded closed meshes, not only arbitrary corrupt index arrays.
- Add targeted mutators for degeneracy: align planes/edges, duplicate coordinates without topology merge, nest shells, and move by one ULP.
- Also fuzz invalid inputs and require precise rejection without undefined behavior.
- Serialize source float bit patterns, operation, seed, options, and schedule.
- Shrink by provenance-guided removal of shells/facets/events while preserving validity and failure.

### End-to-end qualification

- Re-ingest every successful output through input validation.
- Independently classify exact probes on both sides of output facets against expected Boolean occupancy.
- Compare invariants such as connected components, shell nesting, Euler characteristic, and exact oriented volume when meaningful, but never treat them alone as proof.
- Run sanitizers, compiler warnings, debug standard-library modes where available, and multiple C++17 compilers/architectures.
- Benchmark separately; performance thresholds may reject a release but may never weaken correctness tests.

## 3. Output contract

Produce a versioned test corpus, deterministic replay files, oracle comparisons, coverage by relation/invariant category, minimized regressions, platform matrix, and qualification report.

Invariants:

- Every random failure is exactly replayable from recorded data.
- Expected outcomes are exact relations or explicit typed failures.
- Test tolerances are used only for non-authoritative performance/diagnostic metrics or as explicit inputs to separately typed normalization/approximate-realization policies.
- A representability failure is accepted only when it follows the declared policy and exact internal boundary still verifies.
- An exact-in-`T` success requires bit-decoded equality to every exact target; approximate success has a distinct result kind and nonzero displacement evidence.
- The in-tree exact engine and normative component tests remain independently buildable without an external Boolean dependency. Qualification may use optional external providers as independent comparators or candidate product backends under explicit adapters.

Failure of a test blocks the relevant component/release. Flaky scheduling or non-reproducible output is itself a deterministic-correctness defect.

## 4. Verification and definition of done

- Every component contract clause maps to at least one positive, negative, or property test.
- Every documented degeneracy appears in the coverage inventory for every applicable operation and operand order.
- Mutation testing shows the suite detects wrong predicate signs, omitted candidates/events, bad event merges, reversed patches, incorrect labels, unsafe rounding, and non-manifold assembly.
- Continuous tests run bounded suites on each change; scheduled qualification runs broad fuzz/property/platform matrices.
- A release has zero unexplained oracle disagreements, invariant failures, sanitizer failures, or nondeterministic replays.
- Every G1-G9 case in `tests/MeshBooleanPlanGapCases.md` is executable and passes with its exact success or typed-failure expectation. Standalone verifier targets fail to link if they reference forbidden producer helpers.

## 5. Assessment-driven qualification amendment

`plan_16_qualification_release.md` is the normative P13 completion plan and release gate.

The permanent end-to-end corpus must no longer be box-dominated. Before a production claim it must contain thousands of generated construction-known cases, representative CAD-like tessellated cases, deterministic operation chains, and every minimized regression. Coverage must include rotated/skewed and concave solids, cavities and nested shells, extensive coplanar overlap, high-valence contacts, alternate subdivisions, severe scales, non-dyadic intersections, non-manifold exact results, normalization defects, and attribute/provenance cases.

Fuzzing must generate valid intersecting geometry and operation chains, not only disjoint axis-aligned boxes. Invalid-input and normalization fuzzing must verify precise reports and strict post-preparation validation. Every unique failure or disagreement is serialized, minimized, and added to the permanent corpus.

At least one independently implemented mature backend participates in diagnostic comparison for the candidate workload. Agreement is evidence rather than proof; disagreement is preserved and adjudicated with independent occupancy/topology checks. No majority vote or silent fallback resolves a material disagreement.

Every success is checked for false success by re-ingestion, independent topology reconstruction, guarded side classification, embedding/intersection checks, certificate replay, exact-result binding, and subsequent chain operations. Reports separate safe typed failures from incorrect successes.

Qualification must commit exact commands, compiler/library/platform versions, flags, sanitizer and thread matrices, seeds, fuzz durations, corpus manifests, backend versions, outcome tables, unresolved exclusions, performance, peak memory, exact-number growth, cancellation latency, and verifier overhead. CI definitions without a committed passing report are not release evidence.

A backend/result-mode/workload profile is production-qualified only with zero known false successes, zero unexplained verifier or material backend disagreements, zero nondeterministic canonical outcomes, passing sanitizer/platform matrices, met corpus floors, acceptable product-approved success/failure and resource rates, and a reproducible report bound to a frozen qualification manifest. Discovery of a false success or unexplained disagreement revokes qualification until resolved.
