# Mesh Boolean Engine Performance Acceleration Plan

## 0. Status, authority, and scope

This document is the implementation sequence for improving the performance of the symbolic surface-mesh Boolean engine in pull request #89.

It supplements, but does not weaken or replace:

- `broad_plan.md`;
- `component_01_contract_context.md` through `component_14_testing.md`;
- the artifact schemas, mandatory verifier contracts, exact-kernel contracts, and typed failure model in `src/YgorMeshesBooleanContract.h`;
- the permanent regression corpus, including `tests/MeshBooleanPlanGapCases.md` and the exact annular-patch witness regression.

If this document conflicts with a correctness, exactness, determinism, verifier-independence, publication, or failure-model requirement in those documents, the existing requirement wins and the proposed optimization must be redesigned.

The purpose of this plan is to reduce elapsed time, peak memory, exact-arithmetic work, allocation count, and repeated verification work **without changing the mathematical result, the accepted input domain, canonical artifact semantics, deterministic ordering, mandatory verification coverage, or typed result**.

The implementation order below is mandatory. Do not begin a later work package until the preceding package has met its exit gate. A performance change that cannot demonstrate an unchanged exact result and verifier behavior is incomplete.

## 1. Non-negotiable rules for every performance change

Every implementation commit in this workstream shall satisfy all of the following.

1. **Preserve exact authority.** Approximate values, floating-point filters, hashes, bounds, caches, and spatial indices may reject work only when their proof permits it. An uncertain acceleration result must retain the candidate or fall back to the current exact path.
2. **Preserve the frozen public semantics.** `classification_strategy::independent_patch_side_v1`, `realization_semantics::exact_in_T`, the manifold-result policy, and all typed failures remain unchanged unless a separately reviewed specification change explicitly versions them.
3. **Preserve deterministic output.** Worker scheduling, hash bucket order, allocator behavior, hierarchy shape, filter acceptance, and platform-specific sort behavior must not change canonical IDs, semantic bytes, artifact digests, result meshes, replay data, or the selected first diagnostic.
4. **Preserve independent verification.** A producer and its mandatory verifier may share exact number types and immutable source artifacts, but must not share producer grouping, ordering, candidate-pruning, triangulation, probe-selection, realization-assignment, obligation-generation, or canonical-encoding helpers where Components 8, 11, and 13 prohibit it.
5. **Do not replace proof with timing.** Wall-clock measurements are evidence of speed only. Correctness acceptance is based on exact artifact equivalence, independent oracles, invariant reports, mutation tests, and deterministic structural counters.
6. **Add the test before or with the optimization.** Each optimization must add a focused regression or differential test that exercises the path being changed. A later broad end-to-end run is not a substitute for a component-level test.
7. **Keep exhaustive oracles.** A faster mandatory implementation may replace an accidentally exhaustive production or verifier algorithm, but the old exhaustive method must remain available as a bounded independent oracle in Component 14 tests.
8. **Account for resources before doing work.** Batching resource-accounting calls is allowed; under-counting, post-facto accounting, or work performed after a failed reservation is not.
9. **Version semantic changes.** If an artifact field, canonical byte stream, replay interpretation, certificate formula, filter proof, solver transcript, or policy changes semantically, update the relevant schema/version and add backward/stale-evidence tests. Purely diagnostic performance counters must not enter semantic digests.
10. **One optimization class per commit.** Do not combine an arithmetic rewrite, a new broad phase, verifier changes, and parallel scheduling in one commit. Small reviewable commits are required so regressions and speedups can be bisected.

## 2. Required performance evidence

### 2.1 Separate producer and verifier cost

The current end-to-end timing hides whether time is spent producing an artifact or independently verifying it. Add non-authoritative timing and operation counters around every stage transaction:

- input validation;
- broad phase;
- intersection events;
- symbolic registry;
- local refinement;
- global arrangement;
- cell classification;
- Boolean selection and topology preflight;
- geometry realization;
- output assembly;
- final verification.

For each stage, report producer time and verifier time separately. Timings and counters must be collected only when requested by the benchmark or tracing policy, must not affect the setup digest or artifact digests beyond existing explicitly versioned diagnostic options, and must not affect control flow.

Use thread-local or task-local counters and merge them in canonical task-key order. Do not add contended atomics to exact-predicate hot loops.

### 2.2 Minimum structural counters

Add counters sufficient to explain a change, not merely observe it. At minimum record:

- exact arithmetic: small/large integer operations, limb additions, limb multiplications, division calls, divided limbs, GCD calls, rational normalizations, cross-cancellations, and maximum numerator/denominator limb counts;
- exact kernel: predicate calls by kind, filter accepts, filter fallbacks, exact fallbacks, exact divisions performed by geometric queries, support-plane constructions, and point-in-polygon edge tests;
- input validation: ring-edge candidate pairs, exact ring-edge tests, ear candidates, exact ear tests, self-embedding candidate pairs, exact facet-pair tests, shell-location queries, and canonicalization refinement/branch counts;
- broad phase: node pairs, leaf facet pairs, final candidate pairs, false positives on bounded oracle fixtures, build comparisons, and verifier candidate checks;
- events and registry: candidate facet pairs, plane-relation classes, exact carrier/polygon tests, raw events, duplicate derivations, exact equality checks, hash-bucket probes, canonical-key encodings, symbolic vertices, and symbolic curves;
- local/global arrangement: constraints per facet, candidate constraint pairs, exact constraint intersections, DCEL entities, reconciliation passes, global patch/edge/use counts, link entities, patch-witness slabs, patch-witness crossings, and probe constraints;
- classification: source facets/triangles, probes, ray-box candidates, exact ray-facet or ray-triangle tests, accepted hits, alternate rays, independently reconstructed rays, and exterior-attachment patch candidates;
- realization: realized variables, axis candidates, obligations by kind, pair boxes, pair candidates, exact pair checks, constraint components, solver nodes, rejected prefixes, and complete assignments;
- memory/allocation: authoritative bytes, peak stage-private bytes, verifier scratch bytes, allocation count where measurable, and copied artifact bytes.

Counters used as CI gates must be integer and deterministic. Wall-clock values are never deterministic CI gates.

### 2.3 Benchmark output schema

Extend `benchmarks/MeshBooleanBenchmark.cc` instead of creating unrelated ad-hoc benchmark programs. Introduce a versioned machine-readable line format, for example `BENCH schema=2`, containing:

- fixture and size;
- operation;
- `T` and `I` tags;
- verification level;
- thread count;
- repetition count;
- input and setup digests;
- typed outcome;
- selected/output semantic digest when available;
- total, producer, and verifier durations;
- peak bytes and the structural counters above.

Keep the current overlapping-box case as the smallest smoke workload, but do not use it as the only performance signal.

### 2.4 Required workload families

Add deterministic exact fixture generators to `tests/MeshBooleanAnalyticFixtures.*` or a dedicated Component 14 support file. Every size parameter must produce byte-identical input meshes across runs.

1. **B0: current overlapping boxes.** Preserve the current benchmark for continuity.
2. **B1: disjoint subdivided shells.** Two spatially separated boxes or closed grids with increasing facet subdivision. This isolates validation, canonicalization, and broad-phase rejection.
3. **B2: transverse subdivided intersection.** Two boxes or prisms whose intersecting faces are independently subdivided into increasing grids. This stresses candidate generation, event deduplication, local refinement, and topology invariance under subdivision.
4. **B3: coplanar overlap grid.** Coincident or partially overlapping planar face grids with same and opposite orientations. This stresses coplanar events, interval/region interning, and common atomic subdivision.
5. **B4: repeated derivation registry.** Many facet pairs derive the same exact points and carriers while nearby distinct exact points round to the same `T`. This stresses exact identity without permitting approximate merging.
6. **B5: holed patch witnesses.** The through-column square annulus plus a deterministic family with multiple holes and increasing ring complexity. This permanently protects the complete exact vertical-decomposition witness algorithm.
7. **B6: classification-heavy nested shells.** Increasing numbers of nested/disconnected shells and many arrangement patches with few intersections. This isolates exact point location and verifier reconstruction.
8. **B7: realization components.** Many disconnected selected components and a separate tightly coupled component. This verifies constraint-component decomposition and pair broad-phase scaling.
9. **B8: adversarial exact arithmetic.** Subnormal, extreme-exponent, cancellation-heavy, large-limb, and one-ULP-separated fixtures from Components 3 and 14.

For every workload, freeze the exact typed outcome and all available stage semantic digests before beginning P1. If a fixture legitimately reaches a resource limit, freeze the precise limit and replay evidence too.

## 3. P0 — Freeze the baseline before optimizing

### Required implementation

1. Add the timing/counter facility described in section 2 behind a benchmark/tracing switch.
2. Extend `MeshBooleanBenchmark` to run B0-B8 by fixture name and size, and to select operation, verification level, type pair, thread count, warm-up count, and measured repetitions.
3. Add `tests/Test_MeshesBooleanPerformanceBaselines.cc` and register it in `tests/CMakeLists.txt` with Component 14 and performance labels.
4. The test shall not assert wall-clock time. It shall assert:
   - the frozen typed outcome;
   - stage semantic/artifact digests where the public test harness exposes them;
   - canonical output bytes on successful cases;
   - deterministic counter identities that describe correctness, such as broad-phase final candidates and selected patch counts;
   - equality across one-thread and configured multi-thread execution;
   - equality with filters forced off and on once filters exist.
5. Add a benchmark README describing compiler, build type, CPU pinning, repetition policy, and how to compare two benchmark outputs. Use median and median absolute deviation; do not report a single run as a result.
6. Record a baseline result for strict-FP Release builds. Keep machine-specific timing output outside normative source tests; commit the workload definitions, exact outcomes, and structural baselines.

### Required tests

Run all current exact-kernel and mesh-Boolean tests before recording the baseline. In particular:

- `MeshExactArithmetic.Unit`;
- `MeshExactKernel.Unit` and `MeshExactKernel.Properties`;
- all Component 2-13 unit/property/adversarial tests;
- `MeshBoolean.EndToEnd`, `MeshBoolean.Metamorphic`, `MeshBoolean.Replay`, `MeshBoolean.Mutation`, and `MeshBoolean.Qualification`;
- every `MeshBoolean.PlanGap.*` test;
- `MeshBoolean.Example`.

### Exit gate

P0 is complete only when B0-B8 can be reproduced, the current branch passes the full suite, and every later change can be compared against an exact pre-optimization outcome and a producer/verifier cost breakdown.

## 4. P1 — Remove redundant copies, encodings, allocations, and linear lookups

This package intentionally precedes arithmetic and algorithm changes because it is low semantic risk and makes later profiles more representative.

### P1.1 Eliminate artifact copies at publication

`src/YgorMeshesBooleanTransaction.h` currently permits producers to construct a `shared_ptr<const Artifact>` and then copy the artifact into the transaction draft, as seen in stage code such as `enumerate_broad_phase_candidates()`.

1. Redesign `stage_transaction` so a stage owns one mutable draft, freezes it exactly once into the immutable candidate, verifies that same allocation, and publishes that same allocation.
2. Remove every `new Artifact(*candidate)`, equivalent full-vector copy, and redundant candidate/draft dual ownership from Components 2-12.
3. Ensure a failed verifier destroys the unpublished candidate and rolls back all scoped reservations.
4. Ensure a published artifact is immutable and that a successor generation cannot mutate a prior generation.
5. Add transaction tests that count copy/move construction with a synthetic large artifact, force verification failure, force stale binding, and verify reservation rollback.

### P1.2 Remove canonical encoding from hot comparators

The canonical byte stream is an output contract, not a sorting primitive to regenerate on every comparison.

1. In `src/YgorMeshesBooleanIntersectionEvents.cc`, replace repeated `canonical_encoder` construction in `incidence_less`, incidence equality, ownership lookup, and ownership sorting with typed structural comparison functions or one precomputed immutable sort key per record.
2. Replace the linear `std::find` over encoded construction keys in `freeze_constructions()` with a deterministic index keyed by a precomputed digest and confirmed by exact structural equality. A hash collision must remain harmless.
3. Apply the same rule to symbolic registry, local/global arrangement, realization, and output comparators: encode a key at most once, or compare typed fields directly.
4. Generate full canonical artifact bytes only once after canonical IDs and record order are final. Reuse those bytes for the artifact digest and verifier byte comparison rather than re-encoding multiple times in the producer.
5. Add property tests that compare the new structural ordering with the old canonical-byte ordering over generated records, including equal keys, different provenance order, and deliberate digest collisions in a test index.

### P1.3 Replace dense-ID searches with tables

1. Build immutable vectors indexed by canonical IDs for raw-event, facet, edge-use, construction, symbolic, local, global, and selected records.
2. Build raw-ordinal-to-canonical-ID tables during input validation instead of repeatedly scanning `provenance` and `facets` in producers and verifiers.
3. Replace `std::find_if` over complete artifact vectors where the lookup key is a strong ID or raw ordinal.
4. Use sorted flat vectors for small canonical sets and deterministic hash indices only for lookup; never iterate a hash table to assign IDs or publish records.
5. Add malformed-ID and stale-index mutation tests so the optimization cannot mask corrupted handles.

### P1.4 Batch reservations without weakening limits

1. Precompute checked counts for a loop or task batch, reserve once, and then perform the batch.
2. Keep per-kind accounting exact; do not replace several resource kinds with one aggregate reservation.
3. For parallel work, reserve the maximum authorized batch before launching tasks or use deterministic task-local reservations committed in task-key order.
4. Add tests at `limit-1`, `limit`, and `limit+1` for each changed reservation path.

### Exit gate

- All baseline typed outcomes, canonical bytes, and semantic digests are unchanged.
- Transaction copy tests show no full artifact copy on the success path.
- Canonical-key allocations and linear strong-ID lookups are materially reduced on B2-B4.
- The full component and mutation suites pass.

## 5. P2 — Replace the exact-arithmetic bottlenecks

The exact kernel remains the sole authority. Optimize representation and algorithms, not the required exact result.

### P2.1 Add arithmetic microbenchmarks and reference checks

Before changing `src/YgorMeshesExactArithmetic.{h,cc}`, add deterministic microbenchmarks and property vectors for:

- one-, two-, medium-, and large-limb addition, subtraction, multiplication, division, and GCD;
- exact rational construction, normalization, comparison, addition, multiplication, and division;
- powers-of-two denominators typical of decoded binary coordinates;
- cancellation-heavy numerators and denominators;
- quotient/remainder identities and adversarial quotient-digit estimates.

For bounded operands, compare with a deliberately simple reference implementation retained in test code. For every division require `a == q*b + r` and `0 <= r < b`; for every GCD require exact divisibility and the expected sign/canonical form.

### P2.2 Add a no-allocation small-integer representation

Most decoded binary coordinates and many predicate intermediates fit in one or two limbs.

1. Give `big_uint` inline storage for the zero-, one-, and preferably two-limb cases, with heap storage only beyond that capacity.
2. Preserve the existing canonical little-endian limb semantics and canonical bytes.
3. Ensure moves, normalization, hashing, and exception safety work for every transition between inline and heap storage.
4. Add allocator-count tests proving that decoding ordinary `float`/`double` coordinates and evaluating small predicates does not allocate solely to hold one limb.

### P2.3 Replace bit-at-a-time division

`divide(const big_uint&, const big_uint&)` currently shifts and subtracts once per dividend bit and repeatedly grows the quotient.

Implement in this order:

1. zero, one, equal, and less-than fast paths;
2. power-of-two divisor fast path using shifts and a masked remainder;
3. single-limb divisor long division from the most significant limb;
4. normalized multi-limb long division with quotient-digit estimation and correction, such as Knuth Algorithm D;
5. retain the old bitwise division only as a bounded test oracle, not a production fallback.

The implementation must use checked sizes and must not depend on native integer overflow. Add worst-case correction vectors and randomized differential tests.

### P2.4 Replace expensive GCD and rational reduction patterns

1. Add fast paths for zero, equality, powers of two, and single-limb values.
2. Implement binary GCD or another deterministic limb-level GCD that does not repeatedly invoke full multi-limb division for common cases.
3. In rational multiplication and division, cross-cancel numerator/denominator factors before multiplying.
4. In rational addition/subtraction, use the denominator GCD to construct the least common denominator and remove only the remaining common factor from the result.
5. Add an internal constructor/factory for values proven reduced by the operation so the public constructor does not repeat the same GCD work.
6. Preserve the invariant that every externally visible `exact_rational` is canonical, has a positive denominator, and has denominator one for zero.

### P2.5 Make exact comparison cheaper without changing equality

1. Compare signs first.
2. Use numerator/denominator bit-length bounds to decide obviously separated positive magnitudes.
3. Use power-of-two/dyadic alignment when both denominators are powers of two.
4. Only then perform exact cross multiplication, with common-factor cancellation where profitable.
5. Cache a canonical hash only if it is immutable, thread-safe, excluded from semantics, and demonstrably beneficial. Hash equality never proves value equality.

### Required tests

Run the complete Component 3 suite after every substep, then all Component 2-14 tests because exact-number canonicalization affects every artifact. Add serialization round trips proving canonical bytes and hashes are unchanged for all pre-existing known-answer values.

### Exit gate

- All arithmetic reference/property tests pass.
- Canonical exact-number bytes remain unchanged unless explicitly schema-versioned and migrated.
- B8 shows a substantial reduction in division/GCD limb work and allocation count.
- B1-B7 preserve exact artifacts and show no exact-number limb-growth regression.

## 6. P3 — Add certified predicate filters and remove avoidable exact divisions

`src/YgorMeshesExactKernelFilters.cc` currently accepts no floating-point signs and routes every query through exact arithmetic. Implement filters incrementally; never replace exact fallback.

### P3.1 Version and instrument each filter independently

1. Add test-only policy controls to force each predicate through filter-disabled, filter-attempted, filter-accepted, and exact-fallback paths.
2. Implement and version one proof at a time. Begin with `orient2d`, then `orient3d`, then plane-side/dot-sign queries used by ray casting and support-plane tests.
3. Use rigorously derived error bounds under the strict compilation/runtime assumptions already enforced by the Boolean target.
4. A filter may return only a proven positive or negative sign. Exact zero and uncertainty must go to exact fallback.
5. Include the filter proof version in the arithmetic-policy bytes. A version change must invalidate stale evidence as designed.

### P3.2 Avoid division in incidence and boundary tests

1. Rewrite 2D point-on-segment classification to test exact collinearity plus coordinate-range inclusion; do not compute an affine parameter unless the caller requests it.
2. In `classify_point_polygon`, perform the boundary and winding tests with orientation and ordered coordinate comparisons only.
3. Add semantic APIs separating `is_on_closed_segment`, `classify_on_segment`, and `segment_parameter` so callers cannot accidentally pay for a rational division when only incidence is needed.
4. Apply the same principle to 3D segment and carrier checks: compare signed numerators when denominator sign is known, and construct a rational parameter only when it is part of a published event/certificate.

### P3.3 Construct planes from dyadics without denominator explosion

1. Detect input dyadic coordinates and align exponents directly to integer vectors.
2. Form the integer cross product and plane offset with one primitive normalization rather than multiplying four rational denominators and repeatedly dividing them.
3. Keep the general rational path for constructed points.
4. Differentially compare the fast dyadic plane with the general exact plane for random and adversarial inputs, including orientation parity and canonical coefficient sign.

### Required tests

- Extend `MeshExactKernel.Unit` and `.Properties` with forced filter/fallback equivalence.
- Test every permutation identity and exact-zero degeneracy.
- Run signed-zero, subnormal, FTZ/DAZ rejection, ambient rounding-mode, and strict-link-policy tests.
- Run all component tests with filters forced off and with filters enabled; canonical artifacts and typed outcomes must be identical.

### Exit gate

Each enabled filter has an executable proof version, forced-path tests, zero false accepts against exact evaluation, and measurable acceptance on non-degenerate B0-B7 cases. Exact divisions in point-in-polygon and segment-incidence counters are eliminated where no parameter is requested.

## 7. P4 — Make input validation and immutable geometry reuse scale

Component 2 is authoritative and must still reject every invalid input exactly. The changes below reduce repeated work while retaining exhaustive bounded oracles.

### P4.1 Canonicalize rings once

1. Precompute each face's canonical directed rotation once before sorting faces.
2. Use a linear-time minimal-rotation algorithm over canonical vertex IDs rather than generating every rotation inside sort comparators.
3. For orientation-insensitive duplicate-facet detection, compute the minimal rotation in both directions once and take the lesser key.
4. Store raw-face ordinal to validated-facet ID and raw-vertex ordinal to canonical-vertex ID tables.
5. Add ring rotation/reversal/permutation tests and compare the new keys with the old exhaustive rotation oracle on small rings.

### P4.2 Find a support triple in linear time

Choose the first canonical ring vertex, the first distinct later vertex, and the first later vertex not collinear with that line. This deterministically finds a valid support triple or proves the ring collinear without an all-triples search. Verify every remaining ring point against the resulting exact plane as before.

### P4.3 Accelerate simple-ring and embeddedness checks conservatively

1. Implement a deterministic exact-box sweep or hierarchy for non-adjacent ring edges and for same-operand facet pairs.
2. Closed/touching bounds overlap; uncertain bounds remain candidates.
3. Run exact segment/facet relation only on candidates.
4. Keep exhaustive all-pairs comparison in Component 14 for bounded rings/meshes and assert zero false negatives.
5. Do not use triangulation diagonals as source-boundary intersections.

The generic `enumerate_bounded_feature_self()` path must itself be accelerated; replacing one explicit nested loop with another nested-loop helper is not a speedup.

### P4.4 Make ear clipping incremental

1. Maintain a linked ring, exact convex/reflex status, and a deterministic queue ordered by canonical vertex key.
2. Query only reflex vertices whose conservative 2D bounds overlap an ear triangle.
3. Retain exact orientation, diagonal visibility, nonzero-area, and final area-partition checks.
4. Recompute only the two neighboring ear statuses after clipping.
5. Keep the existing exhaustive ear search as a bounded oracle and compare emitted triangulations or certified partition equivalence under ring permutations.

### P4.5 Reuse exact facet data safely

Create an immutable per-validated-facet geometry view containing references or cached exact projected ring points, edge bounds, triangle bounds, oriented normal, and source attribution. Either:

- include authoritative cached fields in the Component 2 artifact schema and verifier invariants; or
- keep a strictly derived, immutable, non-semantic cache tied to the verified artifact lifetime and independently reconstruct it in each verifier family.

Downstream stages must stop rebuilding projected rings, source vertex fans, and triangle geometry for every query.

### P4.6 Replace exhaustive canonical graph search where it dominates

If P0 counters show `canonicalize_graph_exhaustive()` dominates B1/B6:

1. implement deterministic color refinement over exact geometry/topology labels;
2. split all uniquely refined cells without search;
3. individualize only unresolved symmetric cells in canonical least-cell/least-node order;
4. add memoization by a full structural state key with exact equality;
5. retain exhaustive permutation canonicalization as a small-graph oracle;
6. prove canonical bytes invariant under raw index, face, shell, and component permutation.

Do not substitute raw input order for canonicalization.

### Exit gate

Component 2 unit/property tests, re-ingestion tests, invalid-input fuzzing, and permutation metamorphics pass. B1 validation exact-pair counters scale with conservative candidates rather than all facet pairs, and the output validated artifact remains equivalent.

## 8. P5 — Improve Component 4 producer and verifier broad phases

### P5.1 Build the producer hierarchy without sorting every subtree

1. Precompute each feature's three exact bound endpoints and doubled center keys once.
2. Establish deterministic total-order index arrays for each axis using center then facet ID.
3. Build median nodes from index ranges without recomputing rational centers or fully sorting each recursive range.
4. Retain a small-input exhaustive threshold selected from B0/B1 measurements; the threshold affects implementation only, never candidate semantics.
5. Pre-size node and traversal storage and use a flat stack.

### P5.2 Improve traversal and publication

1. Choose the child expansion deterministically from node counts and canonical child IDs.
2. Emit worker-local candidate vectors, sort each once, then perform a deterministic k-way merge and unique.
3. Do not insert into a shared ordered set.
4. Preserve touching-bound overlap and final canonical `(operand_a_facet, operand_b_facet)` order.

### P5.3 Replace the mandatory sweep verifier's linear active set

The verifier must remain independent of the producer BVH.

1. Keep the exact x-event sweep implementation family.
2. Replace vector linear erase and full active scans with an independently implemented deterministic interval index over y, followed by exact z overlap checks.
3. Reconstruct expected candidates and compare canonical keys exactly.
4. Keep exhaustive Cartesian overlap comparison for bounded exhaustive verification and Component 14 tests.

### Required tests

Extend Component 4 tests with increasing identical bounds, zero extent, one-ULP gaps, signed zero, subnormals, extreme exponents, insertion permutations, and thread counts. For every bounded random case compare producer and verifier candidates with exhaustive exact-box enumeration.

### Exit gate

Zero false negatives, identical candidate bytes, and improved B1/B2 node/build/verifier counters. A hierarchy-shape change may change diagnostic implementation statistics only; it may not change semantic bytes.

## 9. P6 — Index intersection events and symbolic registry work

### P6.1 Cache facet-pair inputs once

1. Consume the immutable geometry view from P4.
2. Reuse support planes, projected rings, exact edge carriers, triangle bounds, and source attribution.
3. Avoid constructing `ring3`, `ring2`, vertex fans, and canonical source encodings for every candidate pair.
4. Keep all event discovery operation-independent.

### P6.2 Use candidate-local event accumulators

1. Each facet-pair task writes to a private accumulator keyed by the canonical candidate ID.
2. Normalize incidences and ownership once per emitted event using structural keys from P1.
3. Merge accumulators in candidate order; no shared mutation or discovery-order IDs.
4. Precompute line/polygon cut parameters once per `(carrier, facet)` query and reuse them for interval construction and source attribution.

### P6.3 Replace registry quadratic identity discovery

1. Bucket point candidates by a cheap immutable fingerprint derived from canonical exact hashes and exact coordinate size.
2. Within a bucket, prove exact coordinate equality and compatible incidence before merging.
3. Bucket carriers by canonical direction/anchor fingerprints, then prove same-line equality exactly.
4. Build raw-event-ID and construction-ID maps once.
5. Assign final IDs only after sorting proven equivalence classes by exact canonical structural keys.
6. Never merge on hash or nearest-`T` equality.

### P6.4 Make the independent registry verifier asymptotically bounded

The current independent verifier compares every reconstructed point pair and repeatedly searches `a.vertices`, which can become cubic.

1. Independently sort reconstructed exact points lexicographically and derive run-length equivalence classes.
2. Independently build a point-to-published-ID lookup with exact equality confirmation.
3. Compare class membership and canonical order in `O(n log n)` plus exact comparison cost.
4. Use the same pattern for carriers and interval endpoints with a verifier-specific canonical line implementation.
5. Retain the old pairwise proof as an exhaustive oracle for small generated cases and assert agreement.

### Required tests

Run Components 5 and 6 unit/property suites, retriangulation/subdivision invariance, operand swap, generated equal-construction cases, same-rounded-distinct-point cases, hash-collision injection, raw-event permutation, and verifier mutation tests.

### Exit gate

B2-B4 show reduced exact equality checks, encoding allocations, and verifier complexity while raw events, symbolic identities, provenance, edge orders, radial orders, and canonical bytes remain equivalent.

## 10. P7 — Prune local-refinement work without changing the exact arrangement

### P7.1 Build facet-to-constraint indices once

`audit_crossings()` currently scans all symbolic curves for every facet and repeatedly searches raw intervals.

1. Build raw-interval-ID to incident-facets tables once.
2. Build facet-ID to atomic-curve references once.
3. Build an exact-point membership index for symbolic vertices with exact equality confirmation.
4. Reuse these tables in reconciliation audits and local arrangement construction.

### P7.2 Add a conservative 2D constraint broad phase per facet

1. Compute exact closed bounds for each projected atomic segment.
2. Use a deterministic sweep or hierarchy to enumerate possibly crossing non-adjacent segment pairs.
3. Include touching and collinear-overlap candidates.
4. Run `relate_segments` only on candidates.
5. Compare every small generated facet with the current all-pairs audit and assert identical reconciliation requests.

### P7.3 Use dense DCEL construction tables

1. Intern local vertices and atomic undirected edges through deterministic indices, not repeated map/set scans over complete vectors.
2. Sort outgoing halfedges once by exact angular comparator and stable tie key.
3. Maintain next/previous/twin links through dense IDs.
4. Compute face walks and coverage certificates in linear passes after the graph is frozen.
5. Reuse exact area terms where the same directed halfedge participates in walk and certificate calculations.

### P7.4 Keep reconciliation bounded and monotonic

1. Deduplicate requests by full canonical request key plus exact point equality.
2. Prove each successor generation adds at least one previously absent exact symbolic point or relation.
3. Record per-generation counts and reject a no-progress successor as an invariant error.
4. Add a fixture requiring reconciliation and assert deterministic generation count under input and task permutations.

### Exit gate

Component 7 synthetic arrangements, exact area coverage, adjacent-facet boundary agreement, Plan Gap tests, and random exhaustive planar-oracle comparisons pass. B2/B3 exact segment tests follow candidate counts rather than all constraint pairs.

## 11. P8 — Localize global stitching, links, and exact patch witnesses

### P8.1 Build all global incidence indices in single passes

1. Index local patch/edge occurrences by canonical symbolic endpoints, source edge, curve, operand, shell, and support plane.
2. Build source-sheet mates, seam uses, coincident groups, and occurrence germs from sorted flat groups.
3. Assign IDs after group keys are sorted; never from map/hash iteration order.
4. Replace repeated full-vector searches for patch sides, sheet uses, seams, and occurrences with dense lookup tables.

### P8.2 Restrict spherical-link work to incident germs

1. For each geometric vertex, gather only locally incident rays, planes, arcs, and sheet germs.
2. Deduplicate antipodal rays by exact direction equivalence.
3. Use a conservative local candidate index for arc crossings; retain uncertain candidates.
4. Construct and verify complete regions/continuations only within that vertex's local link.
5. Compare small links with the independent exhaustive spherical-link oracle and keep G3-G6/G9 tests permanent.

### P8.3 Optimize, but do not weaken, `patch_interior()`

The exact vertical decomposition in `src/YgorMeshesBooleanGlobalArrangement.cc` is required. Do not restore centroid sampling or any finite heuristic candidate set.

1. Cache the dominant projection, exact projected outer/hole rings, sorted unique slab x-coordinates, and edge x-ranges per global patch.
2. Maintain slab crossing events incrementally so an edge is inserted/removed only when the sweep enters/leaves its x-range.
3. Sort exact y crossings deterministically and test interval midpoints against the outer ring and every hole exactly.
4. Lift the first valid point by exact support-plane substitution as currently specified.
5. A local-refinement witness may be reused only if the global producer independently proves it lies in the global patch open interior and outside all holes; the mandatory Component 8 verifier must reconstruct that proof without calling the producer helper.
6. Keep the through-column annulus and add multi-hole, thin-corridor, vertical-edge, repeated-x, and ring-order permutation regressions.

### Exit gate

Components 8 and 13 global-arrangement verifiers, standalone verifier-link target, annular witness regression, all G3-G6/G9 arrangement tests, mutation tests, and scheduling/subdivision metamorphics pass. B5 witness work is reduced without changing the selected exact witness or canonical probe bytes unless a versioned formula change is intentionally approved.

## 12. P9 — Add conservative point-location acceleration to Component 9

Classification is expected to be one of the largest end-to-end costs because every patch side owns a direct classification certificate for both operands, primary and alternate rays, and the mandatory verifier independently reconstructs the evidence.

### P9.1 Stop duplicating facet data per triangle

`operand_triangles()` currently copies the source ring, ring vertex IDs, and vertex-fan vectors into every sourced triangle.

1. Store one immutable source-facet classification record containing the ring, projection, oriented plane/normal, vertex fans, and triangle list.
2. Let triangle records contain only exact triangle vertices or vertex IDs, source facet ID, primitive ID, and a reference/index to the source-facet record.
3. Build the records once per operand per classification stage and once independently in the verifier family.
4. Add memory-accounting tests that scale a highly triangulated polygon and prove ring/fan storage is per facet, not per triangle.

### P9.2 Build a conservative producer ray index

1. Build a deterministic hierarchy over closed exact triangle bounds or facet bounds.
2. For a formal ray, enumerate every bound the constant ray intersects or touches. If infinitesimal perturbation could affect an equality case, retain the bound.
3. Run the existing exact formal ray/triangle ownership logic only on candidates.
4. Preserve the fixed stable-key direction selection, parameter grouping, vertex/edge ownership, signed degree, and hit order.
5. Add a test-only exhaustive mode and compare complete `formal_operand_location`, including every hit field, not only inside/outside.

### P9.3 Build a different verifier acceleration

1. The mandatory verifier shall use a facet-level exact-box sweep or independently implemented hierarchy, not the producer triangle hierarchy/helper.
2. Intersect candidate facet planes with the formal ray and run the verifier's original-ring formal point-in-polygon test.
3. Reconstruct shell polarity from candidates and compare all source attribution and degree evidence.
4. Retain full facet scans as bounded exhaustive oracles.

### P9.4 Accelerate exterior attachment

1. Build closed exact bounds for global patches.
2. Enumerate only patches whose bounds can intersect the finite segment from exterior witness to target witness.
3. Preserve exact plane parameter, patch-with-holes relation, first-hit grouping, and component ambiguity checks.
4. Compare with the current full patch scan on all Component 9 fixtures.

### P9.5 Cache only identical exact queries

An exact location result may be memoized only by a full immutable query key containing operand semantic digest, exact base, infinitesimal direction, perturbation key, and requested starting direction. Each patch side must still publish its own direct certificate. Similar or nearby probes are not interchangeable.

### Required tests

Run Component 9 unit/property/adversarial suites, Components 10 and 14 selection tests, all analytic operation/type matrices, nested-shell and contact cases, primary/alternate-ray agreement, filter-on/off equality, exhaustive-vs-accelerated differential tests, and self-consistent evidence mutations.

### Exit gate

All direct certificates and canonical labeled-arrangement bytes are unchanged. B6 ray exact-test counts scale with conservative candidates rather than `probes * all facets/triangles` in both producer and verifier.

## 13. P10 — Shorten realization and output work while preserving every obligation

### P10.1 Add an `exact_in_T` singleton fast path

Under current schema-v1 semantics, every accepted axis value must equal its exact target.

1. Round and decode each selected exact coordinate once.
2. If any decoded coordinate differs from the exact target, return the existing `output_not_representable` evidence immediately for that canonical variable/axis.
3. When every variable has exactly one candidate, bypass generic candidate Cartesian-product enumeration and DFS branching.
4. Still generate the complete obligation universe, variable-obligation graph, constraint components, pair candidates, and independent certificate replay required by Component 11.
5. Publish a versioned canonical singleton transcript or the existing transcript with the same semantic interpretation. Do not silently omit solver evidence.

### P10.2 Precompute candidate keys outside sort comparators

In `src/YgorMeshesBooleanRealization.cc`, decode candidate bits and compute distance/error/order keys once. Do not call `decode_coordinate()` from a sort comparator. Precompute owner-free vertex/triangle keys once per entity.

### P10.3 Keep obligation generation sparse

1. Build dense variable-to-obligation and obligation-to-variable adjacency while obligations are emitted.
2. Deduplicate defining relations and selected incidence obligations by canonical structural keys with exact equality confirmation.
3. Use precomputed candidate-domain AABBs and a flat deterministic hierarchy for triangle-pair candidates.
4. Keep exhaustive all-pairs triangle testing as the bounded oracle required by Component 11.
5. Solve connected components in canonical least-variable order and merge transcripts in component-ID order.

### P10.4 Avoid repeated output reconstruction

1. Move verified realized arrays into output assembly without copying.
2. Pre-size output vertices, faces, and face-index storage from checked certificate counts.
3. Build canonical output order using precomputed structural keys and dense mappings.
4. Re-ingest and independently verify the final mesh exactly as before.

### Required tests

Run Component 11/12 unit and property tests, G2/G7/G8/G9c plan-gap tests, impossible one-third-coordinate fixtures, distinct-points-same-rounding fixtures, candidate-neighborhood tests, component transcript replay, triangle-pair oracle comparison, mutation tests, output re-ingestion, and all end-to-end operations.

### Exit gate

Exact targets, obligations, pair coverage, component decomposition, output bytes, and typed failures are unchanged. B7 shows no global DFS for singleton domains and pair exact checks follow conservative candidates.

## 14. P11 — Bound mandatory verification cost without reducing coverage

This package is a cross-component audit after P1-P10. It does not authorize disabling mandatory invariants.

### Required implementation

1. Use the producer/verifier split from P0 to identify mandatory verifier paths still asymptotically worse than the artifact they verify.
2. For each such path, implement an independent sorted/indexed/sweep/hierarchy reconstruction as described in Components 4, 6, 8, 9, 11, and 13.
3. Keep the exhaustive method in a bounded test-only oracle and run it against the new verifier on random small artifacts.
4. Ensure the mandatory verifier still reconstructs semantic facts from lower-level artifacts and cannot be satisfied by rebuilding counts, canonical bytes, or producer-shaped certificates alone.
5. Keep all standalone verifier-link restrictions. If a new shared utility is fundamental exact arithmetic or a read-only container primitive, document why it is not a forbidden producer helper.
6. Batch verifier scratch reservations and reuse verifier-local immutable indices across invariants in one report.
7. Stop verification after the first causal failed invariant as already specified, but preserve deterministic invariant order and diagnostic entities.

### Required tests

- Every existing mutation test;
- new mutations rebuilt with internally consistent counts, IDs, bytes, digests, and certificates;
- exhaustive-vs-mandatory verifier differential tests on bounded artifacts;
- stale evidence/schema tests;
- verifier resource-limit tests;
- standalone verifier-link targets.

### Exit gate

No invariant is removed from a mandatory specification, every mutation remains detectable, and mandatory verifier work on B1-B7 is bounded by documented candidate/entity counts rather than accidental all-pairs or repeated full scans.

## 15. P12 — Add deterministic parallel execution only after serial work is efficient

`src/YgorMeshesBooleanExecutor.h` currently creates one `std::async` task per launched item and erases the front of a future vector. Replace it only after P1-P11 profiles are stable.

### Required implementation

1. Implement a fixed-size executor with persistent workers, bounded queue capacity, and task records sorted by canonical task key before launch.
2. Assign each task a private result slot. Workers must not append to shared semantic vectors or assign IDs.
3. Merge successful results in task-key order after all required predecessor tasks complete.
4. Select the first failure by the lowest canonical task key, not by completion time. Preserve that error's exact provenance.
5. On cancellation or failure, stop launching new work, allow active tasks to observe cancellation, join all workers, and roll back unpublished reservations.
6. Parallelize only naturally independent units:
   - facet-pair event discovery;
   - per-facet local refinement after the symbolic registry is frozen;
   - per-vertex link preparation before canonical merge;
   - independent patch-side producer classifications;
   - realization constraint components.
7. Keep exact-number objects task-local or immutable. Any shared cache must be read-only after publication or sharded with no iteration-order influence.
8. Enforce `max_queued_tasks` and add saturation/resource-limit tests.

### Required tests

For every applicable fixture and operation, run thread counts 1, 2, and the configured maximum and require identical:

- typed outcome;
- canonical artifact and output bytes;
- stage semantic digests;
- verification reports/evidence apart from explicitly noncanonical timings;
- first error and replay seed;
- deterministic structural counters after canonical aggregation.

Add cancellation at launch, mid-stage, and just before publication, plus worker exception/allocation failure tests.

### Exit gate

Parallel execution improves B2/B3/B6/B7 wall time without changing any semantic result, error, report, or replay behavior and without increasing exact work materially through duplicate computation.

## 16. P13 — Qualification, performance gates, and rollout

### 16.1 Required correctness matrix

Before declaring the workstream complete, run:

- Debug and strict-FP Release builds;
- supported GCC and Clang versions;
- `float`/`double` with `uint32_t`/`uint64_t` where the test target supports them;
- mandatory and bounded exhaustive verification;
- thread counts 1, 2, and configured maximum;
- filters forced off and enabled;
- all input/facet/component permutations and replay cases in Component 14;
- sanitizers and debug standard-library mode where supported.

All existing Component 1-14 tests, Plan Gap G1-G9 tests, example, mutation, fuzz, replay, and qualification targets must pass.

### 16.2 Performance acceptance

Freeze numeric targets in the P0 baseline commit before any optimization implementation. Unless measurements justify a different reviewed target, use these initial release goals:

1. at least a 4x geometric-mean reduction in strict-FP Release mandatory-verification wall time over nontrivial B1-B7 workloads;
2. at least a 5x reduction on the classification-heavy B6 family;
3. no individual nontrivial workload more than 15% slower at the median without a documented exact-work or memory tradeoff approved in review;
4. no increase in authoritative bytes for an equivalent artifact and no unexplained increase in peak stage-private/verifier scratch bytes;
5. elimination of production bit-at-a-time multi-limb division;
6. zero canonical-key encoding allocations inside hot sort comparators;
7. producer and mandatory verifier exact pair/ray tests bounded by conservative candidates on the corresponding workload families;
8. byte-identical canonical outputs and equivalent typed failures to the P0 baseline.

Wall-clock gates belong in scheduled qualification on controlled runners. Continuous CI should gate exact outcomes, semantic digests, deterministic operation counters, candidate coverage, and bounded small-workload performance invariants, not noisy elapsed time.

### 16.3 Required documentation per completed package

For every P1-P12 package, add to the implementing pull request or commit notes:

- baseline fixture and size;
- before/after producer and verifier timing medians;
- before/after deterministic counters and peak bytes;
- exact tests added;
- full tests run;
- artifact/schema versions changed, if any;
- why the optimization cannot remove a true candidate, accept an unproved sign, merge distinct exact entities, skip a mandatory obligation, or alter deterministic publication.

## 17. Definition of done

The engine is considered performance-qualified only when:

- P0-P13 are complete in order;
- all exact outcomes, canonical artifacts, verifier reports, and typed failures satisfy the existing component specifications;
- every new acceleration has a bounded exhaustive differential oracle;
- mandatory verification remains independent and mutation-complete;
- the scheduled performance goals are met on controlled runners;
- benchmark and counter output is sufficient to diagnose future regressions by stage;
- no optimization relies on epsilon, tolerance, random sampling, approximate identity, heuristic cleanup, omitted proof obligations, or disabled verification.
