# Component 11 implementation plan: certified finite-coordinate geometry realization

## 1. Scope and outcome

Implement the checked boundary between exact Boolean topology and public finite coordinates. The stage consumes Component 10's verified immutable `selected_exact_boundary<T, I>`, its retained symbolic/exact dependencies, and Component 1's frozen realization policy. It evaluates only selected symbolic vertices, chooses one finite binary32/binary64 coordinate triple per selected symbol, and proves that the resulting triangulated embedding realizes the selected exact boundary without collapse, inversion, lost incidence, or new intersection.

The completed component must provide:

- one globally shared exact and `T` coordinate for every selected symbolic vertex;
- deterministic software nearest-even conversion independent of casts and the ambient rounding mode;
- a deterministic exact triangulation of selected polygonal domains as the baseline output policy;
- an explicit, finite, canonical realization-obligation set derived from exact selected topology;
- exact evaluation of candidate `T` bit patterns against every mandatory obligation;
- a complete deterministic search of the configured finite candidate domain when nearest rounding fails;
- a replayable certificate proving the accepted coordinate assignment and triangulated embedding;
- precise distinction among success, policy-relative `output_not_representable`, and `resource_limit`;
- canonical encoding, diagnostics, accounting, cancellation, rollback, and independent verification.

Component 11 does not change Boolean selection, merge or split symbols, snap coordinates, simplify topology, assign public mesh indices, or construct `fv_surface_mesh<T, I>`. Component 12 consumes `realized_boundary<T, I>`, optionally performs separately proved simplification, assigns `I` indices, canonicalizes public storage, and runs final publication checks.

All implementation must be self-contained in Ygor, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Use `vec3<T>` from `src/YgorMath.h` only as the final value container after bit patterns have been assembled and checked. Its public `x/y/z` storage and `isfinite()` are suitable for transport; its arithmetic, ordering, normalization, distance, and equality do not certify realization.
- Consume Component 3's exact values, selected construction handles, exact source-bit decoding, `round_binary_nearest_even<T>`, directed round-up/down, exact candidate-bit comparison, predecessor/successor enumeration, predicates, segment/triangle and polygon relations, conservative exact bounds, exact-only mode, and construction consistency checks. Improve those planned interfaces rather than adding a second rational or binary-conversion implementation.
- Consume Component 10's selected vertices, oriented cycles and holes, edges, patches, components, exact result-interior side, carriers, incidence, and provenance. Preserve its global sharing and orientation; do not reconstruct selected topology from coordinates.
- Adapt the undirected-edge grouping and opposite-use checks from `src/YgorMeshesVerification.{h,cc}` as independent structural patterns only. Replace raw indices with owner-safe realization IDs and evaluate degeneracy/intersection through Component 3 exact interpretation of candidate bits.
- Adapt deterministic planar ear-clipping concepts already specified for Component 2 and exact cycle/domain predicates from Components 3 and 7. Triangulation must operate on exact selected domains, not rounded `vec3<T>` values.
- Use Component 1's context, strong IDs, canonical factories/encoders, checked arithmetic, accounting containers, deterministic work keys, cancellation, diagnostics, traces, replay, verifier registry, transactions, and typed failures.
- Use `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only for optional summaries after deterministic records freeze. Never use `YLOGERR` for expected representability failure.

### 2.2 Reject as production behavior

- Do not use legacy snap grids, epsilons, `std::round`, `std::nextafter`, native casts from exact values, `long double`, ambient `fesetround`, random jitter, average points, or per-facet welding.
- Do not use `TriangleIsDegenerate`, `orient_sign`, `ValidateClosedTriangularMesh`, or other rounded predicates as certification. Their structural loops may inspire an independent check, but their arithmetic is not exact with respect to candidate bit patterns.
- Do not preserve a constructed point merely because its nearest conversion is finite. Finiteness is necessary but not sufficient.
- Do not round the same symbolic vertex independently for different facets or provenance paths. Do not merge distinct symbolic IDs whose candidates have equal bits, and do not split one symbol into several candidates.
- Do not drop a tiny triangle, edge, patch, hole, seam, or connected component to make a candidate pass.
- Do not infer that a polygon remains planar after independently rounding its vertices. Baseline realization emits an exact-domain triangulation; polygon-preserving realization is a future separately versioned policy.
- Do not classify search exhaustion as mathematical impossibility. `output_not_representable` states that no assignment exists in the complete configured candidate domain and must identify that policy/domain.
- Do not let hash order, pointer order, worker completion, native floating environment, or a predicate filter path affect candidate order, first conflict, accepted assignment, certificate, or bytes.

No existing Ygor component implements globally coupled exact-bit assignment and certification. The reusable pieces are containers, structural traversal patterns, and the planned Components 1, 3, 7, and 10 contracts.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanRealization.h`: realization policy-facing schemas, obligation and witness records, exact triangulation references, coordinate records, certificate, immutable `realized_boundary<T, I>`, artifact constants, read-only accessors, and stage entry point.
- `src/YgorMeshesBooleanRealization.cc`: dependency audit, exact coordinate materialization, construction-consistency audit, triangulation, obligation generation, candidate domains, deterministic solver, exact certification, encoding, coordinator, verifier adapter, and four `<T, I>` explicit instantiations.
- `tests/Test_MeshesBooleanRealization.cc`: focused rounding, triangulation, obligation, search, certificate, failure, mutation, encoding, and rollback tests.
- `tests/Test_MeshesBooleanRealizationProperties.cc`: generated embedding, provenance, transform, permutation, schedule, filter-path, and compiler determinism tests.
- `tests/MeshBooleanRealizationFixtures.h`: synthetic selected boundaries, exact rational/bit constructors, candidate-domain builders, brute-force assignment oracle, independent triangle-relation oracle, mutation helpers, deterministic PRNG, and replay records.

Modify only as required to reconcile prerequisites:

- `src/YgorMeshesBooleanContract.{h,cc}`: add realization IDs, policy/version fields, limits, accounting categories, invariant/diagnostic codes, artifact slot metadata, and encoding support listed below.
- `src/YgorMeshesExactArithmetic.{h,cc}` and `src/YgorMeshesExactKernel.{h,cc}`: complete the mandatory bit-rounding, neighbor enumeration, candidate-to-exact decode, exact relation, conservative bound, and substitution APIs already required by Component 3. Do not create realization-private arithmetic.
- `src/YgorMeshesBooleanSelection.h`: only reconcile complete read-only selected topology, exact handles, carrier/domain, side-orientation, and provenance accessors. Do not store candidate coordinates in Component 10.

Use namespace `ygor::mesh_boolean`. Keep solver domains, trail entries, propagation state, broad-phase scratch, and verifier implementations private to the `.cc` or tests. Public templates remain thin and use explicit instantiation for `float`/`double` with `std::uint32_t`/`std::uint64_t`.

Add the realization `.cc` to the strict arithmetic source list in `src/CMakeLists.txt`; apply Component 3's no-fast-math/contraction and floating-environment compile checks even though authoritative candidate evaluation is exact. Add both tests to CTest and `tests/compile.sh`, register `MeshBooleanRealization.Unit` and `.Properties`, and label them `mesh_boolean;component11`. Add authoritative GCC/Clang Debug/Release, ASan/UBSan malformed-artifact/search rollback, and TSan coordinate-materialization/certification runs without `|| true`.

Before coding, reconcile these prerequisite interfaces:

1. Add non-convertible `realization_vertex_id`, `realization_triangle_id`, `realization_halfedge_id`, `realization_obligation_id`, `candidate_value_id`, `candidate_assignment_id`, and `realization_certificate_id`, or artifact-local references with exactly those domains.
2. Freeze `realization_policy_v1`: target type, original-coordinate policy, baseline triangulated output, nearest-only versus bounded-neighborhood search, per-axis ULP radius, candidate and assignment limits, solver version, exhaustive pair-certification policy, resource limits, and certificate level. Unsupported enum values fail at context setup/decode.
3. Component 3 must assemble/decode binary32/binary64 bit patterns without native conversion decisions; enumerate each finite coordinate's predecessor/successor neighborhood in numeric order with explicit signed-zero handling; and compare/evaluate all candidates as exact dyadic rationals.
4. Component 10 must expose a bijection from selected vertex IDs to canonical symbolic handles and all exact derivations, and complete oriented polygon-with-holes domains whose boundaries use selected edges.
5. The verification environment must retain immutable Components 3/10, the exact construction store, and the frozen policy without globals or unbudgeted copies.
6. Resource policy must bound selected exact evaluations, construction derivations, triangulation triangles/diagonals, obligations and incidences, candidate values/domains, solver nodes/trail, pair checks, witnesses, certificate facts, verifier scratch, and canonical bytes.

## 4. Stage and ownership contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<realized_boundary<T, I>>>>
realize_selected_boundary(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the exact published `selected_exact_boundary<T, I>` from `artifact_slot::selected_exact_boundary`, retained registry/construction storage, matching owner/setup/kernel/policy identities, and the registered Component 11 verifier specification.
2. Reject stale, copied, replacement, wrong-generation, wrong-coordinate-type, mismatched construction-owner, or policy-incompatible dependencies before evaluating coordinates.
3. Validate dense selected IDs, closed selected incidence, complete symbolic mappings, exact carriers/domains, orientation, and provenance ranges. A malformed selected boundary is `internal_invariant_error`, not representability failure.
4. Open one `geometry_realization` transaction targeting `artifact_slot::realized_boundary`. No exact coordinate, triangulation ID, candidate, obligation, diagnostic, charge, or certificate becomes visible before complete verification.
5. Treat empty selected topology as immediate successful empty realization after dependency/policy verification, with empty coordinate/triangle/obligation stores and a passed certificate.
6. Materialize exact selected coordinates, triangulate exact domains, generate obligations, construct candidate domains, solve, independently verify, check cancellation, and atomically publish.

The artifact retains strong typed dependencies on the selected boundary, registry, exact construction storage, frozen policy, and accepted verification report. It contains no borrowed input mesh pointer, mutable solver state, approximate predicate result, output index `I`, or public mesh.

## 5. Artifact schema

### 5.1 Exact and realized vertices

Publish one `realization_vertex` in selected-vertex order containing:

- realization ID, owner-checked selected vertex reference, and canonical symbolic vertex reference;
- exact point handle/value reference and exact-value digest;
- sorted complete construction derivations and substitution-consistency evidence;
- source coordinate bits when the symbol is an original vertex;
- three accepted finite `coordinate_bits<T>` and transport `vec3<T>`;
- per-axis candidate-domain reference and accepted candidate rank;
- sorted incident selected edges, patches, realization triangles, and obligations.

Evaluate an exact coordinate once per canonical selected symbolic ID. If two selected records claim one symbol, merge only through Component 10's declared bijection; if one selected ID resolves to inconsistent exact values, fail. For every alternate provenance derivation, independently evaluate or retrieve its exact construction, prove all three coordinates equal, and replay every defining equation. A hash or identical expression is not equality proof.

`original_coordinate_policy::preserve_bits` fixes every original vertex to its exact source bit triple, including the sign of zero. If one canonical symbol has several original sources with mathematically equal coordinates but different zero signs, Component 6/10 must have retained the canonical source-bit policy; Component 11 follows that frozen choice and records all alternatives. Constructed exact zero starts at `+0` under Component 3's rule.

### 5.2 Exact triangulation

Baseline schema v1 triangulates every selected polygonal patch before candidate search:

1. Work in the patch's Component 3 exact support-plane chart and preserve Component 10 orientation, outer cycle, holes, seam vertices, and selected edge identities.
2. Bridge holes to the outer cycle with the lexicographically least visibility-valid pair under pre-ranked exact vertex/edge keys. A bridge must lie in the domain closure, have open interior in the domain, and intersect existing boundaries only at its endpoints. Record bridge proof; it is artificial topology, not a selected edge.
3. Apply deterministic exact ear clipping to the resulting weakly simple ring. An ear must have strictly orientation-correct exact area, a diagonal contained in the domain, and no other active vertex in or on its open triangle except documented duplicate bridge endpoints. Choose the least complete immutable ear key, not traversal order.
4. Emit oriented non-zero-area exact triangles, each referencing existing selected symbolic vertices only. Do not create Steiner vertices. Mark boundary halfedges with selected-edge references and diagonals/bridges as artificial paired halfedges.
5. Prove triangle interiors are disjoint, their union equals the exact patch domain, boundary uses equal the original cycles, all artificial edges have exactly two opposite uses within the patch, and orientation places result interior on the negative side.

If this no-Steiner baseline cannot triangulate a valid Component 10 domain, report `internal_invariant_error` because every valid polygon-with-holes admits such a triangulation and the implementation or upstream domain is defective. Resource exhaustion remains `resource_limit`. Triangulation is fixed from exact geometry before rounding and cannot change during search.

Publish `realization_triangle`, oriented halfedge, exact diagonal/bridge evidence, source patch mapping, and exact triangulation digest. Component 12 emits these triangles by default; it may reconstruct polygons only under a future proof-backed policy.

### 5.3 Obligations and witnesses

Freeze a closed `realization_obligation_kind` with at least:

- finite coordinate and fixed original bits;
- distinct symbolic vertices;
- selected-edge endpoint order/noncollapse;
- triangle orientation/nonzero area;
- triangle shared-vertex/shared-edge incidence;
- adjacent-triangle allowed-intersection relation;
- non-adjacent triangle disjointness;
- patch-local triangulation embedding and radial/fan order;
- selected patch side-orientation preservation;
- component/global embedding equivalence.

Every obligation stores a dense ID, kind/version, sorted participating realization IDs with oriented positions where needed, exact-boundary expected relation/sign, candidate relation, exact predicate formula/evidence, source selected entities/provenance, and a canonical pass witness. Unknown kinds or relations are rejected.

Generate a finite sufficient set as follows:

1. Require pairwise inequality for every topologically distinct selected vertex pair that shares a triangle, edge carrier, patch domain, vertex fan, or appears in a triangle pair whose relation is checked. Additionally, accepted coordinate triples must be globally unique across all distinct selected symbolic vertices; this simple schema-v1 rule prevents an unenumerated symbol collision and is checked with exact bit triples.
2. For every selected edge and artificial diagonal, require distinct endpoints and preserve the exact directed order used by incident triangles. For three or more selected vertices on one exact carrier, preserve their complete exact parameter order even when only consecutive subedges are emitted.
3. For every realization triangle `(a,b,c)`, require the candidate cross product to be nonzero and have the same sign in at least one exact canonical projection chosen from the exact triangle's dominant nonzero component. Record all three candidate cross components; the selected sign must agree with the exact orientation and patch-side convention.
4. For triangles sharing an edge, require bit-identical shared endpoints, opposite directed use, and exact candidate intersection equal to that complete shared segment only. For triangles sharing only one selected vertex, require intersection equal to that point only unless exact triangulation explicitly records another adjacency.
5. For all other triangle pairs, require exact disjointness. Use conservative candidate-bit AABBs only to prove strict separation; every overlapping/uncertain box pair receives Component 3's exact triangle/triangle relation. Record either an axis separation witness or the full exact relation. Never omit a possible pair because of rounded arithmetic.
6. Around every selected vertex and edge, compare the candidate triangle link/radial cyclic order with Component 10's exact link and the exact triangulation. Require one cycle per surface sheet, no fold-through, and consistent result-interior side. Derive signs from exact candidate dyadics, not normals computed in `T`.
7. Reconstruct each patch's candidate triangulated complex and prove all triangles satisfy the recorded adjacency/nonintersection relations. Together with fixed combinatorial incidence, global triangle-pair checks, and local link/radial checks, this certifies an embedded subdivision equivalent to the selected exact boundary.

Triangles are inherently planar, so schema v1 avoids an unsound rounded-polygon coplanarity requirement. The certificate must state `triangulated_v1`; it must not imply that original multi-vertex patch rings remain coplanar in `T`.

## 6. Candidate conversion and search domain

### 6.1 Initial candidates

For each exact coordinate, call Component 3's software `round_binary_nearest_even<T>`. Reject infinity/NaN results as absent finite candidates. Verify the returned bit pattern by exact decode, bracketing, and tie-to-even evidence. Never use a C++ cast as the normative result.

For fixed original vertices, the sole candidate is the preserved source bit pattern. For constructed coordinates under radius `R`, define the complete axis domain as nearest-even plus up to `R` finite predecessor values and `R` finite successor values in representable numeric order. Deduplicate the two zeros according to the frozen constructed-zero policy, exclude infinities/NaNs, and order candidates by:

1. ULP step distance from nearest-even;
2. exact absolute error from the target rational;
3. lower numeric value;
4. canonical raw bit pattern.

Perform fallible exact-error comparisons in a pre-ranking pass; solver comparators inspect only immutable ranks. The Cartesian product of the three axis domains is the complete candidate point domain for that symbolic vertex. Check product/count limits before materializing it. A radius of zero is nearest-only.

### 6.2 Global assignment solver

Constraints are coupled: choosing a point for one vertex can affect several triangles. Implement a deterministic finite constraint solver that is complete over the frozen Cartesian candidate domains:

1. Build obligation incidence and statically validate unary/fixed constraints. Choose the next unassigned vertex by smallest remaining point-domain size, then highest unassigned-obligation degree, then realization vertex ID.
2. Enumerate its candidate triples by `(max axis step, sum axis step, pre-ranked exact squared error, x rank, y rank, z rank, raw bits)`. Do not use native squared distance.
3. After assignment, evaluate every obligation whose participants are fully assigned and conservative necessary conditions for partially assigned obligations. Pruning may reject only on an exact contradiction; an uncertain bound cannot prune.
4. Maintain an accounting-backed reversible trail. Memoization may cache only canonical partial-assignment failures and cannot alter traversal, resource precedence, or the first canonical conflict.
5. At a full assignment, run the complete certificate pass from scratch. Accept the first passing assignment in this specified depth-first order.
6. If every assignment in all complete candidate domains is rejected, return `output_not_representable` with policy/domain digest, explored assignment count, a deterministic conflict summary, and the least canonical witnessed conflict for each exhausted top-level candidate branch.

Do not add an incomplete greedy fallback. A policy may explicitly choose `nearest_only` or a bounded radius; within that declared finite domain the solver is complete. If the declared assignment-node/work/memory/time/cancellation limit prevents exhaustive traversal, return `resource_limit`, never `output_not_representable`. Allocation failure is also `resource_limit`. The API documentation must call the failure policy-relative, not proof that no binary32/binary64 embedding exists anywhere.

Parallel search is not part of schema v1. Exact coordinate materialization and fully independent certification queries may use deterministic shards, but accepted assignment order and failure precedence remain serial and canonical.

## 7. Realization algorithm

### 7.1 Audit and exact preparation

1. Validate dependency identities, policy, selected topology, symbolic bijection, exact handles, and retained construction ownership.
2. Reserve conservative envelopes for exact values, triangulation, obligations, candidate domains, worst permitted solver trail/nodes, certificate, encoding, and independent verification. Refuse an unbounded implicit allocation.
3. Enumerate selected symbols in selected-vertex order. Lazily materialize each exact Cartesian point and verify every alternate derivation by exact equality and substitution.
4. Triangulate selected patch domains in canonical patch order and verify exact coverage/orientation before any candidate conversion.
5. Generate obligation records from exact topology/triangulation, canonicalize participating IDs, deduplicate only byte-identical semantic obligations, and assign dense IDs.

### 7.2 Candidate solving and final certification

1. Build and pre-rank all axis/point candidate domains. Reject a fixed-original non-finite bit pattern as an upstream contract defect; classify a constructed coordinate with no finite candidate in policy radius as `output_not_representable`.
2. Attempt the all-nearest/fixed assignment first; this is also the first assignment implied by candidate ordering.
3. If it fails and search is enabled, run the complete solver. Record deterministic conflict witnesses without retaining every explored assignment in the artifact.
4. Decode the accepted bits to exact dyadic points and rerun all unary, order, triangle, adjacency, pair-intersection, link/radial, orientation, and embedding checks without solver incremental caches.
5. Construct `vec3<T>` transport values by bit-preserving assembly, then copy their bits back and require exact equality with accepted patterns and finiteness.
6. Freeze vertices, triangles/halfedges, obligations, witnesses, search summary, and certificate; encode; invoke the independent verifier; compare report bindings; check cancellation; publish atomically.

The accepted assignment must not depend on whether an obligation was initially proved by an interval/AABB separator or exact relation fallback. Filter/exact-only modes produce identical semantic records; encode normalized proof relations rather than performance path counters.

## 8. Invariants and failures

Before a draft can succeed, prove:

1. Every selected symbolic vertex has exactly one exact point, one accepted finite bit triple, and one transport `vec3<T>` with those bits.
2. Every retained derivation evaluates to the same exact point and satisfies its defining equations.
3. Original source bits obey the frozen preservation policy exactly.
4. Distinct selected symbols have distinct accepted coordinate triples; no selected edge, triangle, ring, or link collapses.
5. Exact triangulation covers every selected patch once, introduces no Steiner symbol, and preserves selected boundaries/orientation.
6. Every candidate triangle is nondegenerate and orientation-compatible with its exact triangle and result-interior side.
7. Every candidate triangle pair has exactly its permitted shared-feature relation or is disjoint; no incidence is lost and no intersection is introduced.
8. Every vertex link and edge radial order agrees with the exact selected subdivision, and the candidate boundary is embedded and subdivision-equivalent to Component 10.
9. Every mandatory obligation has one independently replayable passing witness and every certificate count exhausts the artifact.
10. Empty selection realizes successfully; schedule, allocation, hash, thread, filter, and ambient-rounding variations do not alter bytes or equivalent failures.

Failure classification:

- Return `output_not_representable` only after proving no finite coordinate exists in a complete axis domain or exhaustively rejecting every assignment in the configured finite domain. Include exact symbols, conflicting obligation IDs, candidate-domain/search policy digest, relevant exact values/bit patterns, and replay token.
- Return `resource_limit` for declared exact-number/candidate/assignment/pair/work/byte limits, verifier exhaustion, cancellation, allocation failure, or inability to complete the configured search. State whether any representability conclusion was reached; normally none was.
- Return `internal_invariant_error` for malformed/stale dependencies, inconsistent constructions, invalid selected topology/domain, failed exact triangulation, unknown schema values, producer/verifier disagreement, transport-bit mismatch, or encoding corruption.
- Component 11 does not return `input_contract_error` or `index_overflow`; those belong to Components 2 and 12 respectively.

Check cancellation before dependency audit, each exact vertex/derivation, each patch triangulation, obligation generation, candidate-domain construction, every configured solver frontier, pair-certification batches, encoding, verification, and publication. Catch exceptions only at stage/task boundaries, join any workers, choose Component 1's canonical failure, and roll back.

Diagnostics must include policy and dependency digests; selected/symbolic/realization vertex, patch, edge, triangle, and obligation IDs; exact rational values or canonical digests; candidate raw bits; expected/actual relation; search radius/nodes; resource facts; provenance; and replay token. Decimal floating text is supplementary only.

## 9. Canonical encoding and deterministic execution

Define `YGBCAN11` as coordinate-type- and policy-specific realization semantics over one selected exact boundary:

- schema/type/policy/solver/triangulation/obligation versions;
- Component 10 semantic digest and exact-kernel policy digest;
- exact selected-vertex mappings and exact-value digests;
- accepted coordinate bits in X/Y/Z order and accepted candidate ranks;
- exact triangulation topology, selected/artificial edge roles, and patch mappings;
- normalized obligation expected/actual relations and witnesses;
- candidate-domain descriptors and deterministic search summary;
- realization certificate facts and combined semantic digest.

Exclude pointers, owner tokens, setup ordinal, native `vec3` object bytes/padding, hash layout, solver trail, rejected assignments except normalized summary/conflicts, timings, worker IDs, filter attempts, caches, and diagnostics.

Define `YGBREA11` as invocation-bound framing with schema/type versions, setup/realization-policy digest, exact dependency identities/generations/digests, construction-storage binding, deterministic statistics, length-prefixed `YGBCAN11`, and verification report/certificate binding. Wrap it with Component 1's `YGBART01` framing for `realized_boundary`. Decode rejects unknown versions/enums, noncanonical or nonfinite bit patterns, wrong coordinate type, malformed IDs/ranges, duplicate symbol mappings, invalid triangles/halfedges, absent obligations, unsorted witnesses, inconsistent counts/digests, bad lengths, and trailing bytes.

All standard comparators inspect IDs, precomputed exact ranks, integer step counts, and raw bits and are `noexcept`. Exact comparisons occur only in fallible pre-ranking. Hash tables are lookup accelerators. For fixed dependencies/policy, ambient rounding mode, signed-zero host behavior, thread count, worker delays, allocation addresses, hash collision mode, and predicate execution path produce byte-identical successful artifacts and equivalent canonical failures.

## 10. Mandatory independent verifier

Register a stable Component 11 artifact tag, schema/checker versions, and invariant set with Components 1/13. The verifier receives the candidate artifact, immutable selected boundary/registry/construction storage, exact-only kernel services, frozen policy, accounting, and cancellation. It must not call producer triangulation, obligation generation, candidate solver, incremental evaluator, certificate builder, or encoding helpers.

The verifier must:

1. Validate owner, slot/tag/schema/type/policy, strong dependencies, exact handles, dense IDs, enum domains, ranges, provenance, and construction-store bindings.
2. Independently enumerate selected symbols, evaluate all exact coordinates and alternate derivations, replay defining equations, and compare exact digests.
3. Independently triangulate each exact patch with a structurally separate deterministic implementation or verify coverage through an exact planar arrangement/domain-area proof that does not trust producer ear records. Compare oriented triangle multisets and artificial-edge incidence.
4. Decode every accepted coordinate bit pattern directly to exact dyadics, verify finiteness/original-bit policy, and compare transport `vec3<T>` bits.
5. Independently derive the required obligation universe from selected topology and verified triangulation. Require a bijection with stored obligations; do not trust stored expected relations.
6. Recompute all candidate orientation, edge-order, adjacency, triangle-pair intersection, vertex-link, radial-order, patch-side, and embedding relations in exact-only mode. Build conservative pair candidates independently or exhaustively compare every pair when within verifier policy; conservative rejection must have a replayed exact dyadic axis witness.
7. Reconstruct candidate domains from exact values and policy, verify each accepted candidate belongs to its domain, and confirm accepted candidate ranks/search summary. Success does not require replaying rejected search branches; `output_not_representable` diagnostics use a separate failure verifier that exhaustively replays bounded fixtures or checks a canonical solver transcript under the configured diagnostic level.
8. Recompute certificate counts/digests and independently encode `YGBCAN11`, `YGBREA11`, and `YGBART01`.

Verifier exhaustion prevents publication with `resource_limit`; disagreement is `internal_invariant_error`. Exhaustive verification mode compares bounded cases against the test-only brute-force Cartesian assignment oracle and all triangle pairs without broad phase.

Mutation tests alter every owner/ID/range, symbolic/exact handle, derivation, source bit, candidate bit/rank/domain, triangle vertex/orientation/edge role, patch mapping, obligation kind/participants/expected/actual relation/witness, search summary, policy/dependency binding, certificate fact, and serialization field/order. Every mutation must fail in Release/NDEBUG.

## 11. Test plan

### 11.1 Binary conversion and exact coordinate identity

- Exhaust signed zeros, minimum/maximum subnormals, minimum normals, powers of two, halfway ties with even/odd retained significands, largest finite values, overflow-adjacent rationals, and positive/negative values for float/double.
- Differentially verify nearest/down/up/predecessor/successor and radius neighborhoods against integer/rational bit oracles; change ambient rounding modes and require identical results and restored environment.
- Original vertices preserve bit patterns, including `-0`; constructed zero follows policy. Nonfinite candidates never enter a domain.
- Multiple affine/intersection/projection provenance paths for one symbol evaluate equal and emit one bit triple; inject a contradictory derivation and require failure before search.

### 11.2 Exact triangulation and obligations

- Triangles, convex/concave polygons, holes, multiple holes, narrow channels, collinear boundary runs retained as selected subdivisions, high-valence seams, and disconnected/cavity components.
- Permute cycle rotations, hole ordering, patch storage, and equivalent valid provenance; exact triangulation and IDs remain canonical.
- Verify exact coverage, boundary equality, artificial-edge twins, orientation, and no Steiner vertices. Inject invalid holes, crossing rings, impossible bridge evidence, omitted/duplicated triangles, or reversed ears.
- Generate and count every obligation class on tetrahedra, cubes, concave shells, genus-bearing shells, cavities, intersection seams, and coincident-source boundaries.

### 11.3 Realizable and unrepresentable embeddings

- Exactly representable disjoint/nested/intersecting solids retain original and constructed coordinates and pass nearest-only certification.
- Rational intersections not exactly representable in `T` but safely nearest-rounded pass with exact dyadic certificate.
- Two distinct exact vertices that nearest-round together: a neighboring assignment succeeds when available; nearest-only fails; a radius whose complete domains cannot separate them returns `output_not_representable`.
- Rounding-induced zero-area triangles, edge-order inversions, triangle fold-through, new non-adjacent intersection, lost shared-feature relation, and vertex-fan/radial inversion.
- Tiny features near zero, huge dynamic range, subnormal constructions, one-ULP separations, overflow-adjacent coordinates, and mixed fixed-original/constructed constraints.
- Polygon vertices that cease to be coplanar after rounding still succeed through certified exact-domain triangles; no artifact claims polygon planarity.

### 11.4 Solver and failure semantics

- Compare every bounded small candidate problem with exhaustive Cartesian enumeration and require the first passing assignment under the specified order.
- Cases requiring coordinated movement of two or more shared vertices, failed greedy choices/backtracking, repeated conflicts, and no solution in the domain.
- Radius zero/one/multiple, fixed domains, signed-zero deduplication, boundary-of-finite-range domains, exact candidate/assignment limits, and one-over cases.
- Interrupt before exhaustive completion and require `resource_limit`, never `output_not_representable`; exhaustive failure records the complete policy digest and canonical conflicts.
- Cancellation/allocation fault at every phase leaves no artifact or committed charge. Producer/verifier disagreement and malformed dependencies fail closed.

### 11.5 Embedding, metamorphic, and qualification tests

- Independently classify every candidate triangle pair for small artifacts and compare allowed incidence/disjointness with exact selected topology.
- Re-ingest a test-only indexed view through Component 2 and require valid closed embedded topology; compare oriented selected-domain provenance rather than rounded geometric equality.
- Apply exactly representable translations, axis permutations, orientation-corrected sign flips, and positive power-of-two scaling where target range permits. Candidate bits/obligations transform equivariantly.
- Swap operands and remap difference operations through Component 10; equivalent selected boundaries receive equivalent realization after provenance mapping.
- Permute source vertices/facets/shells, construction insertion, selected storage, triangulation work, and valid subdivision. Compare canonical IDs/bytes when exact selected artifacts are semantically identical.
- Vary threads, delays, hash collisions, allocation addresses, exact filter/cache mode, ambient rounding mode, and compiler optimization; require identical artifacts and failures.
- GCC/Clang Debug/Release canonical outputs match. ASan/UBSan cover malformed records and solver rollback; TSan covers exact materialization, independent pair checks, cancellation, diagnostics, and publication.
- Benchmark nearest-success, backtracking-heavy, triangle-pair-heavy, seam-heavy, and large-coordinate artifacts. Record exact evaluations, solver nodes, pair candidates/fallbacks, certificate bytes, and peak private memory without weakening checks.

Serialize all failures with source coordinate bits, exact rational/value digests, accepted/rejected candidate bits needed for the canonical conflict, symbolic/selected/realization IDs, triangle/obligation relations, policy/dependency digests, PRNG state, and replay token.

## 12. Component 12 handoff contract

Before Component 12 integration, prove it can consume only `realized_boundary<T, I>` and retained selected provenance to:

- enumerate one finite `vec3<T>` and raw bit triple per used symbolic vertex;
- enumerate complete oriented baseline triangles and distinguish selected boundary edges from artificial triangulation edges;
- map every realization vertex/triangle back to selected exact entities and certificates;
- establish that all coordinates, incidences, orientation, links, and nonintersections are already certified;
- recognize empty realization as successful empty output;
- check `I` capacity and assign canonical public indices without changing coordinate bits or connectivity.

Component 12 must not retry coordinate search, merge colliding symbols, retriangulate to hide a failed constraint, or downgrade a realization failure. Optional polygon reconstruction or simplification requires new exact and realized proofs and must preserve or replace all affected Component 11 obligations.

## 13. Implementation sequence and completion criteria

Implement in this order:

1. Reconcile Components 1/3/10 policy, IDs, exact conversion, selected-domain, resource, verifier-environment, and dependency interfaces; freeze schemas and encodings.
2. Implement exact selected-coordinate materialization, derivation equality/substitution checks, and original-bit preservation.
3. Implement deterministic exact polygon-with-holes triangulation and independent exact coverage/orientation verification.
4. Implement the complete obligation schema/generator and exact candidate evaluator, initially for fixed/nearest assignments with exhaustive triangle-pair checks.
5. Implement finite axis/point candidate domains, exact pre-ranking, deterministic complete backtracking, conflict reporting, resource/cancellation handling, and rollback.
6. Implement `realized_boundary`, certificate, canonical encodings, independent verifier, mutation suite, and transactional publication.
7. Add conservative candidate-bit pair acceleration only after exhaustive differential tests prove no missed pair; retain exhaustive mode as oracle.
8. Run conversion, provenance, triangulation, embedding, impossible-domain, solver-oracle, transform/permutation, compiler, sanitizer, schedule, replay, resource, and benchmark qualification before Component 12 integration.

Component 11 is complete only when:

- every selected symbol is evaluated once, all derivations agree exactly, and one globally shared finite bit triple is emitted;
- software conversion and neighbor enumeration are bit-exact and independent of native casts/rounding mode;
- exact deterministic triangulation covers every selected patch and baseline triangles avoid rounded polygon planarity assumptions;
- the obligation set is finite, complete for the emitted embedding, explicit, canonical, and independently replayable;
- accepted coordinates preserve vertex distinction, edge order, triangle orientation, permitted incidences, local links/radial order, nonintersection, and subdivision equivalence;
- bounded search is deterministic and complete over its declared domain, with `resource_limit` distinct from policy-relative `output_not_representable`;
- no snapping, tolerance, feature deletion, symbol merge/split, topology repair, or heuristic fallback is possible;
- empty selection succeeds, all failures are typed and transactional, and canonical bytes are schedule/filter/compiler stable;
- independent exact-only verification and mutation tests pass in Release as well as Debug;
- Component 12 can serialize the certified triangles and coordinates without making a geometric decision.
