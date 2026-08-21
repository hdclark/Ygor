# Component 14 implementation plan: exact testing, replay, and qualification

## 1. Scope and outcome

Implement the cross-component test system that supplies exact, reproducible evidence for every contract in Components 1-13 and for the complete Boolean pipeline. The system must test stage APIs and immutable artifacts directly, compare exact relations or typed failures, preserve every random failure as a versioned replay, and produce a machine-checkable release qualification report. It must never accept an image, triangle count, approximate volume, decimal tolerance, or successful process exit as proof of Boolean correctness.

The completed component must provide:

- dependency-free C++17 test infrastructure integrated with CMake and CTest;
- a versioned requirements inventory mapping every contract clause to positive, negative, property, oracle, mutation, or qualification tests;
- exact known-answer arithmetic and predicate suites, including forced filter and exact-fallback paths;
- deliberately slow, structurally independent bounded oracles for broad phase, planar arrangements, classification, analytic solids, realization, and final topology;
- a versioned degeneracy corpus covering every applicable operation and operand order;
- deterministic valid/invalid generators, metamorphic transforms, execution-policy variation, replay capture, and validity-preserving shrinking;
- exact end-to-end output re-ingestion and independent side-occupancy checks;
- mutation qualification proving detection of the defect classes named by the component contract;
- bounded per-change suites and broader scheduled compiler, sanitizer, architecture, replay, fuzz, property, and performance qualification;
- canonical qualification artifacts with no unexplained disagreement, nondeterminism, sanitizer finding, or flaky result.

All normative tests and tools must be self-contained in Ygor, compile as strict C++17, and use no external geometry, Boolean, property-test, unit-test, fuzzing, serialization, or benchmark dependency. Do not include, call, compare against, or derive expected answers from `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`. Legacy geometric scenarios may be manually migrated only after their expected exact relation is independently specified.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Facilities to retain or adapt

- Preserve the repository's small standalone executable style from `tests/*.cc`, but provide a shared in-tree harness and authoritative CTest registration. A test executable must return nonzero on any failed assertion, uncaught exception, timeout, replay mismatch, or incomplete case.
- Retain `tests/compile.sh` as a developer compatibility path. Make it delegate Boolean test construction to CMake or compile the same named targets; it is not the release test runner.
- Use `fv_surface_mesh<T, I>` and basic vector types from `src/YgorMath.{h,cc}` to construct public inputs and independently parse public outputs.
- Use patterns from `src/YgorMeshesVerification.{h,cc}` only as secondary smoke checks and examples of ordered edge aggregation. Component 13 checkers and Component 14 independent oracles remain authoritative and must reconstruct topology without producer caches.
- Use `YLOGDEBUG`, `YLOGINFO`, and `YLOGWARN` through scoped capture only for supplemental diagnostics after canonical failure data is frozen. Logger text never determines expected behavior.
- Reuse Component 1 canonical encoding/digest primitives, Component 3 exact value types and exact-only primitive operations, and Component 13 report/evidence/replay schemas. Sharing scalar arithmetic is allowed; sharing the production control flow under test is not.
- Port hand-auditable geometric inputs from `tests/Test_MeshesBoolean5.cc` and `tests2/YgorMeshesBoolean*.cc` only as source fixtures. Replace implementation-internal calls, direct `.cc` inclusion, tolerance/volume assertions, and legacy expected behavior with new stage APIs and exact expectations.

### 2.2 Deficiencies to correct

The current repository has no top-level CTest integration, shared deterministic test configuration, exact low-complexity oracle package, topology-preserving generator, invalid-input fuzzer, shrinker, versioned Boolean corpus, replay runner, mutation gate, benchmark harness, coverage inventory, or qualification report. Existing Boolean tests predominantly use excluded implementations, approximate metrics, process-local fixtures, and unrecorded behavior.

The current `.gitlab-ci.yml` runs standalone tests with `|| true`; this hides failures, crashes, sanitizer reports, and timeouts. Replace these loops for authoritative tests with CTest invocations whose exit status gates the job. Tests must not fetch doctest or any other dependency from the network.

Root `CMakeLists.txt` currently adds GCC `-ffast-math`. Components 1-14 already require a named strict Boolean source/test policy. Ensure effective Boolean compilation and final compiler-driver link commands remove fast-math/finite-math assumptions and contraction where required, do not link startup objects such as GCC `crtfastmath` that alter the process floating-point mode, add compile-time rejection of `__FAST_MATH__`, and preserve signed zero, subnormals, and floating-environment tests. Inspect `compile_commands.json`, representative final link commands, and runtime subnormal behavior in qualification; later flags or startup state must not silently re-enable unsafe behavior.

Generic Ygor serializers, OBJ/OFF/PLY/STL files, decimal dumps, and native object bytes are not authoritative replay formats. They may be emitted for human inspection only.

## 3. Files, targets, and ownership

### 3.1 Shared infrastructure

Add:

- `tests/CMakeLists.txt`: authoritative test target creation, labels, timeouts, fixtures, and test-data paths.
- `tests/MeshBooleanTestHarness.{h,cc}`: dependency-free registration, assertions, exact/typed-failure matchers, deterministic case names, skip policy, and nonzero process result.
- `tests/MeshBooleanTestConfig.{h,cc}`: canonical seed, tier, budgets, type matrix, operation subset, execution schedules, artifact directory, and command-line/environment parsing.
- `tests/MeshBooleanTestSupport.{h,cc}`: raw float-bit builders, exact assertions, artifact/result normalization, log capture, temporary artifact handling, and automatic replay-on-failure guard.
- `tests/MeshBooleanAnalyticFixtures.{h,cc}`: exact-first boxes, tetrahedra, convex polytopes, extrusions, cavities, nested/disconnected shells, high-valence cases, equivalent subdivisions, and invalid meshes.
- `tests/MeshBooleanDegeneracyCorpus.{h,cc}`: catalog metadata and exact-rational scenario builders.
- `tests/MeshBooleanOracles.{h,cc}`: common oracle result schemas and canonical exact-boundary comparison.
- `tests/MeshBooleanPlanarOracle.{h,cc}`: bounded direct exact segment/polygon arrangement oracle.
- `tests/MeshBooleanClassificationOracle.{h,cc}`: bounded independent symbolic-ray/signed-degree classifier.
- `tests/MeshBooleanPolytopeOracle.{h,cc}`: analytic exact halfspace/polytope occupancy and boundary oracle.
- `tests/MeshBooleanGenerators.{h,cc}`: constructive valid-solid, invalid-input, and targeted degeneracy generators/mutators.
- `tests/MeshBooleanShrinker.{h,cc}`: deterministic provenance-guided delta debugger and validity-preserving geometric reducers.
- `tests/MeshBooleanReplay.cc`: command-line replay, corpus traversal, expected-result verification, and optional human diagnostics.
- `tests/MeshBooleanQualification.cc`: inventory validation, platform facts, complete corpus/oracle/metamorphic execution, and report production.
- `tests/Test_MeshesBooleanEndToEnd.cc`: bounded analytic, degeneracy, output re-ingestion, side-probe, typed-failure, and empty-result tests.
- `tests/Test_MeshesBooleanMetamorphic.cc`: Boolean identities, representation changes, exact transforms, and operation/operand relations.
- `tests/Test_MeshesBooleanFuzz.cc`: deterministic valid and invalid generated cases, replay capture, and bounded shrinking.
- `tests/Test_MeshesBooleanMutation.cc`: mutation manifest execution and expected detector verification.
- `tests/mesh_boolean/requirements.tsv`: stable requirement-to-test traceability inventory.
- `tests/mesh_boolean/degeneracies.tsv`: operation/operand-order/type coverage matrix.
- `tests/mesh_boolean/corpus/README.md` and `manifest.tsv`: discriminated corpus schema policy and canonical ordered case manifest.
- `tests/mesh_boolean/corpus/known_answers/`, `degeneracies/`, and `regressions/`: versioned canonical records stored as reviewable files.
- `benchmarks/CMakeLists.txt` and `benchmarks/MeshBooleanBenchmark.cc`: separate correctness-preserving performance runner and fixed case manifest.

Do not consolidate the component-specific tests already specified in Plans 1-13. Their files remain the owning unit/property suites, fixture headers, and named CTests. Component 14 supplies shared infrastructure, enforces their registration and inventory coverage, and adds cross-component suites.

### 3.2 Production/test boundary reconciliation

Modify only where required for controllability and replay:

- `src/YgorMeshesBooleanContract.{h,cc}`: freeze test-visible execution policies, deterministic seed derivation, type support, resource controls, fault-seam IDs, and replay metadata without exposing mutable production internals.
- `src/YgorMeshesBooleanReplay.{h,cc}` from Components 1/13: finalize canonical invocation/result/replay codecs and compatibility validation used by the standalone runner.
- Components 2-13 headers: expose immutable artifact views, exact semantic encodings, verification entry points, and explicitly named test-only fault seams already required by their plans. Do not expose private containers merely to ease testing.
- root `CMakeLists.txt` and `src/CMakeLists.txt`: enable CTest, strict Boolean target flags, build options, and compile guards.
- `tests/compile.sh`: retain developer compatibility without duplicating source lists or ignoring failures.
- `.gitlab-ci.yml`: add authoritative compiler/configuration/sanitizer jobs and preserve failure artifacts.

Use namespace `ygor::mesh_boolean::testing` for reusable test code. Oracle implementation details, generators, shrink queues, and harness registration remain outside the installed public API. Production fault seams compile only when `YGOR_MESH_BOOLEAN_TEST_FAULTS` is defined for dedicated mutation targets and default to no fault.

### 3.3 Named targets and tiers

Register the Plans 1-13 CTests plus:

- `MeshBoolean.EndToEnd`, labels `mesh_boolean;component14;continuous`;
- `MeshBoolean.Metamorphic`, labels `mesh_boolean;component14;property`;
- `MeshBoolean.Replay`, labels `mesh_boolean;component14;replay;continuous`;
- `MeshBoolean.Fuzz`, labels `mesh_boolean;component14;fuzz;scheduled`;
- `MeshBoolean.Mutation`, labels `mesh_boolean;component14;mutation;scheduled`;
- `MeshBoolean.Qualification`, labels `mesh_boolean;component14;qualification;scheduled`.

Define `YGOR_BOOLEAN_TEST_TIER=continuous|extended|qualification`. The tier changes budgets and enabled case sets, never expected semantics or mandatory checks. Continuous tests use fixed bounded cases and committed regressions; extended tests add larger deterministic property/oracle matrices; qualification runs the complete supported matrix. CTest timeouts are explicit per target and timeout is failure.

## 4. Harness and deterministic configuration

### 4.1 Harness contract

The in-tree harness must support test registration, fatal/nonfatal assertions, exact equality/relation assertions, expected typed failure matching, parameterized case names, and deterministic result ordering. It catches and reports exceptions at the case boundary but does not turn them into expected failures unless the tested API contract specifies that exception.

Every failure record includes test ID, case ID, master seed, derived stream ID, generator/schema versions, operation, operand roles, `<T, I>`, policy and schedule IDs, expected relation/failure, actual first failure, and replay path/digest. Sort reports by stable test/case ID rather than execution completion. Parallel test execution must not alter report bytes.

Assertions comparing artifacts must select the correct equivalence:

- byte equality only where a canonical encoding contract requires it;
- semantic exact-artifact equality before realization where multiple legal representations exist;
- exact oriented boundary equivalence for Boolean set semantics;
- exact predicate or occupancy relation for local checks;
- full typed error category and required diagnostic bindings for expected failure.

Never silently skip a supported type, platform, policy, corpus case, or required invariant. An unsupported platform must be an explicit tested result; a missing optional sanitizer/compiler is reported as platform-matrix absence and cannot satisfy release qualification.

### 4.2 Seeds and budgets

Use an in-tree fixed algorithm PRNG with a versioned transition and seed-derivation function; do not use `std::random_device`, implementation-defined distributions, wall time, addresses, thread IDs, or hash iteration. A test invocation records a 128-bit master seed as canonical fixed-width integers. Derive independent streams from `(master seed, test ID, case ordinal, purpose tag)` so filtering or adding another test does not change existing cases.

Freeze configuration before execution. Include case count, maximum vertices/facets/shells, rational bit size, mutation count, schedules, shrink attempts/work, wall-time policy, exact/resource limits, and artifact byte limit. At-limit cases must run; one-over cases must return the declared typed failure. Wall time may stop a scheduled campaign only between cases and must preserve the next case ordinal; it cannot change the result of an active authoritative case.

Test all supported combinations `float`/`double` with `std::uint32_t`/`std::uint64_t`. Use small test-only index capacity models for practical exact boundary testing, but retain real near-capacity checked-arithmetic tests without allocating impossible vectors.

## 5. Canonical corpus and replay

### 5.1 Replay record

Use the Component 1/13 canonical framing and digest utilities. Freeze a Component 14 replay envelope, for example `YGBRPL14` schema v1, containing:

- engine, context, exact-kernel, artifact, verifier, generator, shrinker, and replay schema versions;
- canonical raw operand records with source `float`/`double` bit patterns, rings/facets, operand roles, and source IDs;
- operation, shell/domain/realization policy, all resource limits, filter/BVH/hash/work-partition/thread/schedule choices, and platform requirements;
- master seed, derived stream IDs, case ordinal, generator/mutator recipe, and transformation history;
- expected exact relation, canonical exact boundary digest/payload where available, or complete typed failure expectation;
- actual result/report/evidence digests for a regression and its stable first invariant/subcode;
- dependency slice and minimization lineage when present.

Canonical encoding uses fixed-width big-endian values, explicit enum validation, raw source bits, sorted records, checked lengths, domain-separated digests, and no pointers, timestamps, locale text, native layout, or filesystem-dependent path. Decode rejects unknown incompatible versions, malformed lengths/bits/enums, unsorted or duplicate records, missing payloads, digest mismatch, trailing bytes, unsupported options, and limits exceeded before allocation.

A digest is not a substitute for input content. Content-addressed references are accepted only when the bound corpus file is supplied and verifies. OBJ/OFF/PLY/STL and decimal summaries may accompany a failure but cannot replay it.

### 5.2 Corpus lifecycle

`manifest.tsv` is a discriminated union keyed by `record_kind`. Common fields are stable ID, record schema, category tags, supported scalar/index types, payload path, expectation kind, and digest. `arithmetic_vector` and `predicate_vector` records carry operation/relation and integer-vector fields but no mesh operands. `boolean_case` records additionally require Boolean operation, operand order, policies, and source mesh payload. `codec_golden`, `invalid_input`, and `regression` records define their own mandatory fields and explicit `not_applicable` values. The decoder validates exactly the mandatory/forbidden fields for each kind. Sort by stable ID and reject duplicates, stale digest, missing file, unlisted file, unknown kind/category, invalid `not_applicable`, or unsupported schema.

Populate the initial corpus with exact known-answer arithmetic/predicate vectors, every degeneracy in Section 8, all typed failures and resource boundaries, canonical encoding golden files, and independently re-specified useful legacy geometries. Every newly discovered failure is first replayed, deterministically minimized, reviewed for exact expectation, placed in `regressions/`, and added to both manifest and requirement inventory. Never overwrite a historical record to hide changed behavior; add a versioned successor with an explicit migration rationale.

Replay must reproduce the same semantic result or same stable first failure/invariant, not merely matching prose. A report-byte equality requirement applies only where all bound semantic bytes and policy versions are identical. Cross-platform legitimate diagnostic text or performance differences are ignored; exact results, canonical payloads, and typed failure bindings are not.

## 6. Independent exact oracles

### 6.1 Independence rule

An oracle may use Component 3 canonical exact integers/rationals and primitive exact arithmetic after those primitives pass independent known-answer tests. It must not call the production stage's candidate traversal, intersection classification, arrangement builder, event registry, ray ownership, propagation, Boolean selection helper, realization search, canonicalizer, or topology cache. Put oracle algorithms in test-only translation units and document the production functions they are forbidden to call.

For each oracle differential, compare production, Component 13's independent verifier, and the Component 14 bounded oracle where all three apply. A disagreement is a test failure even when two agree; preserve all three evidence records for diagnosis. Do not majority-vote.

### 6.2 Required oracles

Implement:

1. **Arithmetic known-answer oracle.** Check canonical limbs, signs, normalization, arithmetic, division/GCD, rational reduction/comparison, exact dyadic conversion, and algebraic identities against committed vectors generated independently and reviewed as integer literals. Cover zero, one, signs, carry/borrow, limb boundaries, huge cancellation, and allocation/resource boundaries.
2. **Predicate oracle.** Evaluate determinant/polynomial signs by direct exact expansion for bounded dimensions. Enumerate every relation category and permutation/sign law. Force filter accept-positive, accept-negative, accept-zero where legal, uncertain fallback, and exact-only paths; require equivalent sign/certificate.
3. **Exhaustive candidate oracle.** Enumerate the complete required cross/self feature Cartesian products and exact inclusive bounds. Production may emit legal false positives, but every truly interacting pair must be present and canonical; compare exact policy output only when that policy promises equality.
4. **Planar arrangement oracle.** Project to a canonical exact plane, enumerate all segment intersections directly, split at all exact parameters, build stars by direct pairwise angular comparison, trace cells, and calculate exact coverage. Support bounded convex and concave simple facets and multiple overlap intervals without producer triangulation.
5. **Registry/incidence oracle.** Use all-pairs exact equality and interval union, not production fingerprints or hash tables, to form equivalence classes, exact edge orders, and incidence closure.
6. **Global topology oracle.** Reconstruct source sheets, seam pairings, radial order, coincidence layers, vertex links, connected components, and oriented cycles from immutable exact incidence only.
7. **Classification oracle.** Evaluate bounded exact signed degree/solid angle or multiple independently selected symbolic rays. At least two valid ray strategies must agree. Do not share production hit ownership, perturbation priority, seed selection, or propagation.
8. **Analytic polytope oracle.** Represent boxes, tetrahedra, and small convex polytopes as exact halfspaces; derive exact occupancy and low-complexity Boolean boundary cells directly. Compose known nested/disconnected shells and cavities analytically without voxelization.
9. **Selection oracle.** Apply an explicit test-only truth table to exact side labels and construct an unordered oriented exact-domain boundary multiset. Compare against production decisions and incidence.
10. **Realization oracle.** For bounded candidate domains, enumerate Cartesian assignments, replay every exact obligation, and classify certified solution/no solution/resource exhaustion independently of production search order.
11. **Final mesh oracle.** Parse all public vectors from scratch with range checks before dereference; reconstruct edges, links, components, shell nesting, orientation, exact coordinate bits, source mappings, Euler characteristic, and exact oriented volume where meaningful.

Each oracle declares a hard bounded domain and returns `oracle_out_of_domain` to the harness, never a guessed answer. Inventory rows requiring an oracle must select fixtures inside that domain; out-of-domain cannot count as coverage.

## 7. Unit and contract test matrix

### 7.1 Components 1 and 3 infrastructure

- Exercise every operation, operand-role remap, shell/domain/realization policy, stable ID domain, canonical codec, resource kind, cancellation point, transaction state, publication gate, typed error, and replay binding.
- For every exact integer/rational operation, test canonical zero/sign/limbs, aliases, carry/borrow, multiplication/division, GCD/reduction, comparison, checked conversions, exact limit, allocation fault, and algebraic identities.
- Decode every supported finite binary32/binary64 class exactly: positive/negative zero, normals, subnormals, smallest/largest finite, adjacent values, and powers of two. Reject NaN/infinity and unsupported platform assumptions precisely.
- Cover every 2D/3D predicate relation, all argument permutations and sign laws, normalized constructions, substitution/incidence, symbolic perturbation order, ambient rounding modes, and filter/exact certificate equivalence.

### 7.2 Components 2 and 4-13

Use the component-owned unit/property files from Plans 2 and 4-13. The traceability inventory must cover every public constructor, artifact field, invariant code, relation enum, transaction, canonical codec, malformed decode, resource boundary, and failure category. In particular:

- input validation covers indices, rings, planarity, simple nonzero facets, edge uses, links, shells, embeddedness, nesting/orientation, duplicates/contacts, finite coordinates, and canonical permutations;
- broad phase covers exhaustive no-false-negative comparisons, touching bounds, ULP gaps, fallback bounds, self/cross modes, hierarchy choices, and canonical streams;
- events and registry cover every feature-pair relation, point/interval/region provenance, coplanar overlays, equal derivations, unequal fingerprint collisions, edge reversal, and exact order;
- local/global arrangements cover atomization, weak/zero-area structures, stars/twins/cycles, facet coverage, seam stitching, radial layers, links, coincidence, source reconstruction, and subdivision quotienting;
- classification and selection cover exact seeds/probes, alternate rays, cycle propagation, tangent/coincident transfers, all truth-table rows, operation changes, lower-dimensional exclusion, orientation, and selected manifold topology;
- realization covers each candidate policy, equal-rounding distinct exact points, order/inversion/collision/non-incidence obligations, brute-force bounded conclusions, `output_not_representable`, and distinction from `resource_limit`;
- output and verification cover every public vector/mapping, canonical order, index capacity, output re-ingestion, report/evidence schemas, stale bindings, authority, first-cause precedence, Release/NDEBUG checks, and final publication.

Every typed failure receives at least one exact positive trigger and one nearby success: `input_contract_error`, `unsupported_platform`, `resource_limit`, `index_overflow`, `output_not_representable`, and `internal_invariant_error`. At-limit succeeds where the operation is otherwise valid; one-over fails with exact requested/current/limit evidence. Inject faults before and after reservations, construction, verification, encoding, and publication and prove rollback leaves no artifact, authority, final marker, public mesh, or leaked committed charge.

## 8. Degeneracy corpus

### 8.1 Exact-first scenario generation

Describe each source scenario with small exact integers/dyadic rationals, derive expected carriers/intersections/occupancy analytically, then convert source vertices only when exactly representable in the requested `T`. Record the original exact specification and source bit patterns. Never create an expected relation by running the production engine.

For each applicable scenario, test all five operations (union, intersection, both directed differences, symmetric difference), both operand presentation orders with the correct operation remap, all supported `<T, I>` combinations, relevant same/opposite orientations, and exact-boundary comparison before realization. The matrix explicitly marks mathematically inapplicable cells with a reviewed reason; blank cells fail qualification.

### 8.2 Mandatory categories

The catalog must include:

- disjoint, strict containment in both directions, equal operands, complementary local boundaries, empty operands/results, and disconnected components;
- vertex-vertex, vertex-edge, vertex-face, edge-edge, edge-face, and face-face contact, each separated into crossing, touching/tangent, overlap, and incidence-through-original-feature forms where applicable;
- collinear and coplanar disjoint, partial overlap, full overlap, containment, equal and opposite orientation, shared boundary only, and positive-area overlap;
- proper transverse crossing, tangency, events through original vertices/edges, several derivations at one exact point, several intervals on one concave facet, and coincident constructions with different provenance;
- cavities, alternating nested shells, multiple disjoint shells, high-valence vertices, genus cases, long thin facets, concave polygons, and radically different legal source subdivisions/triangulations;
- signed zero in every coordinate position, smallest/largest subnormals, extreme finite exponents, adjacent floats, one-ULP separations/mutations, catastrophic cancellation patterns, and power-of-two scales near exponent limits;
- distinct exact constructed points that map to the same nearest `T`, cases requiring alternate representatives, realizable boundary-limit cases, and provably unrepresentable results.

Each record tags the feature relation, expected dimension, orientation relation, operation applicability, realization expectation, source representability proof, and requirement IDs. The qualification report shows counts and pass/fail by category rather than only total cases.

## 9. Property and metamorphic testing

### 9.1 Boolean/set properties

Compare canonical exact selected boundaries, occupancy, and typed outcomes under declared preconditions:

- commutativity of union, intersection, and symmetric difference;
- operand swap with `A-B` mapped to `B-A` and orientation/provenance remapped;
- idempotence, identity with empty, `A-A`, absorption, and operation consistency identities;
- De Morgan relations only under an explicitly represented finite bounding solid/domain so no unsupported universe boundary is implied;
- containment/disjoint analytic identities and agreement between direct operation and equivalent compositions.

If finite-`T` realization differs or fails, compare the verified exact selected boundary and require the realization result to match its declared policy. Do not weaken the property to volume or facet count.

### 9.2 Representation and transform properties

Generate equivalent inputs by facet-ring rotation, legal ring reversal plus orientation correction, vertex-index renumbering, facet/shell/component permutation, source-edge/facet subdivision, diagonal changes, and internal triangulation changes. Require equivalent canonical validated operands and exact Boolean boundaries; require byte-identical public output only where Component 12 canonicalization promises it.

Apply exactly representable translations, axis permutations, orientation-corrected sign flips/reflections, and positive power-of-two scaling. Transform interacting cases, not only identity-with-empty cases. Map the result back exactly and compare boundary, labels, provenance-independent geometry, and typed realization behavior.

### 9.3 Execution determinism

Run the same canonical invocation across supported thread counts, work partitions, worker delay patterns, BVH policies/shapes, filters forced on/off/exact-only, hash seeds/collision modes, input insertion orders, allocator-address perturbations, and ambient rounding modes. Require semantic artifacts and canonical outputs/reports to satisfy their byte contracts. Restore floating environment after every case and treat a leaked mode as failure.

Run selected replay cases in separate processes to detect global state, address, locale, and initialization-order dependence. Capture a child process's canonical result file, not logger text. Flaky scheduling or a non-reproducible result is a deterministic correctness defect and is promoted to regression.

## 10. Fuzzing, replay capture, and shrinking

### 10.1 Valid generators

Construct valid embedded solids rather than hoping arbitrary indices are valid:

- convex polytopes from bounded exact halfspaces with independently known incidence;
- exact extrusions of generated simple planar polygons;
- boxes/tetrahedra and transformed compositions;
- nested alternating shells, cavities, and disconnected disjoint components;
- topology-preserving source-edge/facet subdivision and triangulation variants;
- controlled genus/high-valence templates with exact embeddings.

Every generated operand passes Component 2 validation and an independent fixture invariant check before it is used as a valid case. A generator producing an invalid object is a generator defect, not an expected Boolean rejection.

Targeted valid mutators align planes/edges/vertices, introduce exact contacts, duplicate coordinates without topology merge where allowed, nest or separate shells, change one source bit by one ULP, vary subdivisions, and move between relation categories while preserving a recorded exact construction recipe.

### 10.2 Invalid generators

Generate malformed indices/ranges, short/duplicate ring uses, nonfinite coordinates, nonplanar or zero-area facets, self-crossing polygons, open/nonmanifold/oppositely misused edges, split vertex links, inconsistent orientation, intersecting or ambiguously touching shells, duplicate shells under rejecting policies, self-intersections, stale handles, malformed codecs, and policy/platform mismatches. Range-check and decode fuzzing must run under sanitizers and require precise rejection without undefined behavior, unbounded allocation, partial publication, or process termination.

### 10.3 Failure capture

Before invoking any potentially crashing code, a supervising process must atomically write the complete canonical invocation replay to a temporary failure slot, reread and validate its digest, and only then launch the case in a child process. The child writes its canonical result to a separate file. On normal success the supervisor may remove the prewritten invocation; on assertion, oracle disagreement, verifier failure, exception, signal, sanitizer abort, timeout, missing/truncated result, abnormal exit, or nondeterminism it preserves the invocation and attaches the observed exit/result record. In-process bounded unit cases may use the same prewrite guard, but random, malformed-input, sanitizer, fault-injection, and schedule cases must use supervision. If prewriting fails, do not run the case; fail with the reserved path and encoding error. Thus fatal code never needs to execute cleanup to leave a complete replay.

The first rerun is exact replay with no shrinking. Accept a random failure as reproducible only if fresh reconstruction yields the same exact disagreement or stable first invariant/subcode. Preserve unreproducible cases as deterministic-correctness defects with all observed outcomes; never discard them as flaky.

### 10.4 Deterministic shrinking

Use Component 13 dependency slices as hints, not proof. Apply reducers in canonical order:

1. remove irrelevant disconnected components and shells;
2. remove source facets/ring vertices while reconstructing and revalidating a legal solid;
3. remove redundant source subdivisions/triangulation changes;
4. reduce operation options, schedules, thread count, and resource settings;
5. remove events/constraints through provenance-guided source reduction, never by mutating a published artifact;
6. reduce exact integer/rational magnitudes and coordinate exponents while preserving source representability;
7. simplify relation recipes and one-ULP mutations.

Each candidate is rebuilt from source, validated, rerun under the same schema/policy, and accepted only if it preserves the same exact oracle disagreement or stable first invariant/subcode. Cache candidate replay digests only as a performance optimization; deterministic ordered traversal chooses the result. Charge and record attempts, work, exact limbs, replay bytes, and elapsed diagnostic time. Budget exhaustion returns the smallest case found so far and does not alter the original test failure.

## 11. End-to-end qualification

For every successful operation:

1. Require mandatory Component 13 reports for all published artifacts and finalization.
2. Compare the selected exact boundary with an analytic or bounded exact oracle when in domain.
3. Re-ingest every nonempty public mesh through a fresh Component 2 context with no reused caches or IDs. Exercise the explicit empty-output contract separately.
4. Independently parse the public mesh and reconstruct edge uses, links, components, shell nesting, orientation, embedding obligations, and canonical ordering.
5. Construct exact probes on both sides of every output facet from realization certificates/formal neighborhoods; classify them against both original operands with the independent classifier and require the Boolean truth value to change in the oriented direction.
6. Check connected components, shell nesting, Euler characteristic, and exact oriented volume where defined as supporting invariants. Never let these aggregate invariants replace side classification or exact boundary comparison.
7. Serialize and replay the invocation/result and require canonical agreement.

For `output_not_representable`, require a verified exact selected boundary, a complete policy-specific impossibility/search certificate, and Component 13 agreement. Resource/cancellation exhaustion cannot be accepted as representability failure. For every other failure, require no final mesh/publication marker and exact typed diagnostics.

## 12. Mutation qualification

Create a stable mutation manifest with one fault ID, owning component, injected defect, expected detecting test IDs, invariant code/subcode, and applicable build. CMake validates the manifest and generates one isolated production-library/test pair per selected fault, named `ygor_mesh_boolean_mutant_<fault_id>` and `MeshBoolean.Mutation.<fault_id>`, with only `YGOR_MESH_BOOLEAN_TEST_FAULTS=<fault_id>` enabled. No ordinary target links a mutant library. CTest records expected detector and first invariant for each mutant; qualification rejects an unregistered manifest row, a mutant without exactly one target/test, or a result attributed to another fault. At minimum include:

- invert positive/negative/zero predicate signs and bypass exact fallback;
- omit a required broad-phase candidate;
- omit or misclassify an event and drop an overlap endpoint/interval;
- merge unequal registry entities or fail to merge equal entities;
- corrupt exact order, incidence, local arrangement edge, coverage, or face orientation;
- mis-stitch a seam, radial layer, coincidence domain, or vertex link;
- reverse a classification transfer or assign a wrong seed/side label;
- reverse/drop/retain an incorrect selected patch or include lower-dimensional contact;
- accept unsafe rounding, collapse distinct vertices, skip an obligation, or mislabel search exhaustion;
- emit bad index/order/orientation, skip manifold validation, or publish before verification;
- accept stale owner/generation/report/evidence or bypass a transaction/resource gate.

Build one enabled mutation per dedicated target/process to avoid fault interactions. A mutant is killed only when the declared authoritative test fails for the intended invariant; crash, timeout, unrelated setup failure, or compile failure does not count unless that mutation specifically targets compile-time guards. Release qualification requires every manifest mutation killed in Release/NDEBUG where applicable.

## 13. Coverage inventory and reports

### 13.1 Requirements inventory

Assign stable IDs to every normative bullet/contract clause in `broad_plan.md`, `component_01` through `component_14`, and each plan's completion criteria. Each `requirements.tsv` row records owner, clause text digest, test or qualification-check IDs, test kind, positive/negative cases, oracle/verifier used, `<T, I>` coverage, tier, and required platform modes. Qualification rejects missing/duplicate IDs, stale text digests, absent registrations, checks that ran zero cases, unsupported unreviewed cells, and requirements covered only by aggregate metrics.

Add a documentation qualification check for the broad-plan release requirement. It verifies that installed public Boolean API documentation names the accepted domain, regularization and all operations, orientation/shell policy, determinism contract, typed errors and resource behavior, complexity/limits, exact-combinatorics guarantees, and finite-`T` representability limitation, and that documented enum/API names resolve to the shipped headers. Missing required sections or stale API names block release; prose quality remains reviewable rather than inferred from keyword counts.

`degeneracies.tsv` expands every Section 8 category over operation, operand order, orientation, and relevant type/policy. Explicit `not_applicable` entries require a reason code. Qualification fails on any unexplained blank, missing corpus digest, or case whose observed category differs from its exact specification.

### 13.2 Qualification report

Emit a canonical machine-readable report plus a rendered text summary. Bind it to source revision, dirty-state digest if permitted for local runs, compiler identity/version, target architecture/endian/IEC-60559 facts, effective compile-option digest, build type, sanitizer/debug mode, test/replay/corpus/inventory versions, supported type matrix, exact seeds/budgets, case counts, and every test result.

Include requirement and degeneracy coverage, oracle comparisons, mutation kills, replay pass count, minimized regressions, deterministic schedule/process comparisons, resource/fault outcomes, sanitizer/platform matrix, and benchmark references. Exclude timestamps from canonical identity; wall duration is a non-authoritative metric. Signatures are not required, but every linked artifact has a domain-separated digest.

Release qualification passes only with zero unexplained oracle disagreement, invariant failure, nondeterministic replay, flaky case, sanitizer finding, mutation survivor, missing required platform, or inventory gap. An explicit approved platform exclusion is versioned policy, not an ad hoc skip.

## 14. Build, CI, and platform matrix

### 14.1 CMake requirements

Use `include(CTest)` and `add_subdirectory(tests)` under `BUILD_TESTING`. Add options for Boolean tests, tier, mutation targets, benchmarks, and strict floating point. Link only in-tree Ygor and `Threads::Threads`. Test data paths come from CMake definitions and do not depend on current working directory.

Apply strict C++17 and strict floating flags target-locally to every Boolean production, oracle, replay, and test target, including final compiler-driver link invocations. Verify `__cplusplus`, reject `__FAST_MATH__`, prevent fast-math startup code from enabling flush-to-zero or denormals-are-zero, and add supported compiler flags that disable unsafe contraction/finite assumptions. Test the runtime mode with actual volatile subnormal input and result arithmetic before adversarial cases. Do not globally remove flags needed by unrelated Ygor targets unless separately approved.

Authoritative dependency-free jobs configure `WITH_EIGEN=OFF` and `WITH_GNU_GSL=OFF`. Ensure the Boolean library/test targets then have no transitive Eigen, GSL, Boost, downloaded test framework, or external geometry/Boolean link dependency; qualification inspects generated target/link metadata and representative final link commands. If the monolithic `ygor` target cannot provide that interface, introduce an in-tree `ygor_mesh_boolean` target containing only required Ygor sources rather than allowing an external dependency into normative tests.

Enable a reviewed compiler-specific warning set for GCC and Clang on every Boolean production/test/oracle tool and make warnings errors in authoritative Debug and Release jobs. Include at least `-Wall -Wextra -Wpedantic` plus supported conversion, shadow, format, switch-enum, non-virtual-dtor, and old-style-cast diagnostics, with narrowly documented target-local suppressions only where audited. Qualification records the effective warning flags and requires zero unapproved warnings.

CTest registrations have stable names, labels, timeouts, processors/resource locks where needed, and required fixture setup. Tests create artifacts only beneath a provided build/artifact directory and never `/tmp` hard-coded paths. Benchmarks are separate targets and are not ordinary correctness tests.

### 14.2 Continuous and scheduled CI

Per-change authoritative jobs:

- GCC Debug strict-FP continuous CTest;
- GCC Release/NDEBUG strict-FP continuous CTest;
- Clang Debug and Release strict-FP continuous CTest;
- ASan/UBSan malformed input, decoder, range, rollback, oracle, and end-to-end suites;
- replay of every committed regression and degeneracy continuous case.

At least one GCC and one Clang continuous job must also use `-DWITH_EIGEN=OFF -DWITH_GNU_GSL=OFF` and pass the dependency/link-interface audit.

Scheduled/release jobs:

- extended/qualification property, fuzz, oracle, shrink, and mutation matrices;
- TSan concurrent stores, sharding, cancellation, report merge, replay capture, and finalization;
- libstdc++ debug iterators/assertions;
- supported x86-64 and aarch64, and any additional declared architecture/endian configurations;
- all four `<T, I>` combinations, ambient rounding modes, filter policies, thread/schedule policies, and strict compiler configurations;
- optional MSan only with a fully instrumented runtime, clearly distinguished from required ASan/UBSan/TSan jobs.

Pin release compiler/container versions and retain compatibility jobs separately. CI must invoke CTest directly, must not use `|| true`, and must upload replay files, minimized cases, qualification reports, compile commands, and logs on failure. Network access is not required to run normative tests.

### 14.3 Sanitizer and fault behavior

Sanitizer findings always fail even if an API returned the expected error. Configure child-process and timeout handling so sanitizer exit status is preserved. Run malformed decoders/artifacts and invalid inputs under ASan/UBSan; concurrent deterministic paths under TSan; allocation/fault seams and Release/NDEBUG gates in ordinary and sanitized builds. Do not combine incompatible sanitizers in one job.

## 15. Performance qualification

Benchmark only after correctness suites pass. Use fixed versioned corpus cases and `std::chrono::steady_clock`, with warm-up, repeated samples, median/quantiles, compiler/platform metadata, and no test tolerance controlling geometry. Record candidates, exact fallbacks, maximum limb count, events, arrangement size, realization search work, verifier work, peak accounted bytes, and wall/CPU metrics where available.

Performance thresholds may block a release but cannot disable exact fallback, verification, replay, certificate obligations, degeneracy cases, or resource checks. Maintain separate verified-production and stage microbenchmarks. A benchmark failure is never converted into an approximate geometry mode.

## 16. Implementation sequence

Implement in this order:

1. Add authoritative CTest integration, dependency-free harness, deterministic configuration, strict Boolean compile targets/guards, and CI jobs that propagate failures.
2. Freeze broad-plan/component requirement and degeneracy IDs, discriminated manifests, corpus policy, replay envelope, seed derivation, budgets, and exact assertion semantics.
3. Implement replay encode/decode/runner, failure artifact guard, corpus validation, and golden malformed-codec tests before randomized testing.
4. Implement arithmetic/predicate known-answer vectors and direct exact determinant oracle alongside Components 1 and 3; qualify strict floating and filter/fallback paths.
5. Integrate every component-owned unit/property target from Plans 1-13 into CTest and populate requirement rows as each API/invariant freezes.
6. Implement exhaustive candidate, planar arrangement, registry/incidence, global topology, classification, analytic polytope, selection, realization, and final mesh oracles in bounded order corresponding to Components 4-12.
7. Build the exact-first degeneracy catalog and complete operation/operand/type matrix; migrate useful legacy geometry only after independent expectations exist.
8. Add Boolean algebra, representation, transform, filter, hash/BVH, thread/schedule, ambient-rounding, and separate-process determinism suites.
9. Add constructive valid generators, invalid generators, targeted degeneracy mutators, automatic replay capture, and deterministic provenance-guided shrinking.
10. Add end-to-end exact-boundary comparison, output re-ingestion, independent side probes, representability-failure checks, and aggregate supporting invariants.
11. Implement mutation targets/manifest and prove every required defect class is killed by the intended Release-capable test.
12. Implement qualification report/inventory validation, full compiler/sanitizer/architecture matrix, and separate benchmark runner; gate release on a complete clean report.

Component 14 infrastructure is developed alongside Components 1, 3, and 13 and extended before each later component becomes a production prerequisite. Do not defer replay, exact oracles, or inventory population until the complete engine exists.

## 17. Completion criteria

Component 14 is complete only when:

- every normative broad-plan clause and every contract clause/completion criterion in Components/Plans 1-14 maps to an executed positive, negative, property, oracle, mutation, documentation, or qualification check;
- every documented degeneracy has reviewed coverage for every applicable operation, operand order, orientation, policy, and supported `<T, I>` combination;
- arbitrary-precision arithmetic, exact conversion, predicates, constructions, filter paths, and fallback paths pass independent known-answer and algebraic tests;
- bounded production artifacts and Component 13 checkers agree with structurally independent exhaustive oracles for every required stage;
- all typed failures, at-limit/one-over resources, cancellation points, index capacities, transaction rollback, and publication gates are tested in Debug and Release/NDEBUG;
- random valid/invalid tests are deterministic, a supervisor validates and persists a complete canonical invocation before crash-prone execution, every failure retains it, and fresh replay reproduces exact semantics or the stable first invariant/subcode;
- shrinking preserves input validity where required and the same exact failure, and every accepted bug becomes a minimized committed regression;
- canonical exact boundaries validate Boolean identities and representation/transform invariance even when finite-`T` realization legitimately differs or fails;
- every successful output is freshly re-ingested and independently checked with exact side probes, topology, orientation, nesting, and realization obligations;
- mutation qualification detects wrong signs, omitted candidates/events, bad merges/orders/incidence, reversed patches, bad labels, unsafe realization, malformed assembly, stale evidence, and publication bypasses;
- CTest and CI fail on any failed test, crash, timeout, sanitizer report, replay mismatch, inventory/documentation gap, unapproved warning, or nondeterminism, with no ignored authoritative result or network-fetched dependency;
- GCC and Clang Debug/Release strict-C++17 builds, all four `<T, I>` combinations, required sanitizer/debug modes, supported architectures, schedule/filter/rounding policies, and committed replay corpus pass;
- dependency-free GCC/Clang jobs pass with Eigen/GSL disabled and prove normative Boolean targets have no prohibited transitive dependency;
- qualification emits a canonical versioned corpus, platform matrix, oracle/mutation/coverage/documentation results, minimized regressions, and report with zero unexplained disagreement or missing required cell;
- benchmarks are reproducible and separate from correctness, and no performance policy weakens exactness, verification, or failure behavior.

## 18. Plan-gap qualification amendment

`tests/MeshBooleanPlanGapCases.md` is a release gate. G1-G9 remain individually registered and cease being red tests only when their exact success or typed-failure assertions pass for all applicable operations, operand orders, policies, and `<T, I>` combinations.

Add bounded independent oracles for geometric-point occurrence partitioning, complete spherical links, open-probe substitution, exact-target/defining-relation replay, decomposed assignment, and exhaustive triangle pairs. Add self-consistent artifact rebuilders for occurrence weld, radial swap, missing continuation, one-ULP defining-incidence violation, hybrid classification claim, and forged realization obligation. Add standalone Component 8/11 verifier link tests that cannot resolve producer-only symbols.

The permanent corpus includes vertex-touching cubes, edge-touching cubes, and the one-third prism. Qualification distinguishes `result_topology_not_supported`, `output_not_representable`, `resource_limit`, and `internal_invariant_error`, verifies topology-before-realization precedence, and requires schema/version rejection for stale policy and artifact encodings.
