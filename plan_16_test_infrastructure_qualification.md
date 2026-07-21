# Plan 16: Test Infrastructure and Release Qualification

## 0. Scope and fixed V1 design

Implement **only Component 16** from `component_16_test_infrastructure_qualification.md`. Component 16 is the permanent, dependency-free verification and qualification system for Components 01 through 17. It owns the normative test harness, specification traceability, test-only exact arithmetic and low-complexity geometric oracles, analytic fixtures, valid generators, invalid-input and intermediate-artifact mutators, deterministic campaigns, shrinking, permanent regression corpus, replay qualification, closed qualification matrices, canonical evidence bundles, and the fail-closed release qualification decision.

Component 16 must not:

- make a production geometric, topological, classification, selection, cleanup, or publication decision;
- become a runtime dependency of the ordinary `bounded_boolean` path;
- duplicate the production verifiers, canonical codec, SHA-256 provider, resource ledger, transaction system, replay schema, or deterministic execution service owned by Components 01-17;
- treat a producer and its verifier as independent when they share the behavior under test;
- use `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}` as implementation material or as an oracle;
- use or download an external test framework, arbitrary-precision library, geometry library, fuzzing library, graph library, serializer, database, benchmark framework, corpus service, or random-data service;
- use network access during configure, build, test, replay, corpus validation, shrinking, or qualification;
- use process-global mutable test state, static-registration order, filesystem enumeration order, locale, pointer values, wall-clock time, thread completion order, or `std::random_device` as authoritative input;
- infer topology or identity from coordinate equality, approximate equality, tolerance, digest equality, or hash-table placement;
- label a filtered, incomplete, stale, skipped-required, or non-qualified-platform run as release qualification; or
- silently delete, weaken, or stop replaying a confirmed regression.

Freeze V1 as follows:

```text
harness:                         explicit_registry_private_result_merge_v1
assertion_records:               stable_assertion_id_exact_witness_v1
manifest:                        closed_scope_expansion_v1
traceability:                    annotated_clause_inventory_v1
canonical_test_codec:            component01_canonical_bytes_v1
qualification_digest:            component01_sha256_domain_separated_v1
seed_stream:                     splitmix64_counter_domains_v1
exact_unsigned_integer:          base_2pow32_limbs_v1
exact_signed_integer:            sign_magnitude_canonical_zero_v1
exact_rational:                  reduced_positive_denominator_v1
exact_float_import:              iec60559_bits_to_dyadic_v1
exact_geometry:                  rational_determinant_projection_v1
analytic_fixtures:               integer_dyadic_constructive_solids_v1
valid_generators:                proof_by_construction_then_validate_v1
presentation_variants:           topology_preserving_transform_registry_v1
invalid_input_mutation:          classified_single_fault_first_v1
artifact_mutation:               producer_shaped_test_builder_v1
campaign_scheduler:              case_index_domain_stream_v1
failure_retention:               first_canonical_plus_bounded_secondary_v1
shrinker:                        deterministic_best_first_metric_v1
corpus_layout:                   content_addressed_manifest_last_v1
corpus_deduplication:            digest_bucket_full_content_compare_v1
replay_execution:                validated_component_or_full_v1
matrix_expansion:                closed_cells_with_explicit_na_v1
performance_gates:               structural_counter_envelopes_v1
evidence_bundle:                 canonical_authoritative_plus_appendix_v1
release_decision:                complete_evidence_fail_closed_v1
execution_reference:             serial_qualification_semantics_v1
```

The serial qualification runner is normative. Component 17 may execute independent cases concurrently only through immutable task descriptors and private outputs. Canonical ordered results, primary failure, retained failures, evidence bytes, corpus decisions, shrink result, counters, and the release decision must equal the serial reference.

Mark Component 16 complete in `tracker.md` only after every requirement below is represented and this plan is committed. The tracker mark records planning completion, not implementation completion.

## 1. Existing Ygor assessment and mandatory reuse

### 1.1 Existing test paths

Ygor currently has two unsuitable normative-test paths:

- `tests/compile.sh` directly compiles many standalone programs. It has no immutable manifest, stable discovery, machine-readable result schema, deterministic failure arbitration, common resource accounting, replay, traceability, or release evidence.
- `tests2/Main.cc` uses doctest, and `tests2/compile_and_run.sh` downloads doctest over the network. That violates the no-network/no-external-framework contract and makes the framework version non-replayable.

Preserve both for unrelated Ygor work. Do not migrate all existing tests in this component. Normative bounded-Boolean tests must use the dependency-free CTest-compatible harness introduced by Plan 01 under `tests/mesh_boolean_bounded/`. CTest is only an invocation layer; Component 16 owns test identity, order, required status, result encoding, completeness, and release qualification.

### 1.2 Build isolation

Reuse `ygor_apply_mesh_boolean_strict_fp`, `BUILD_TESTING`, and the bounded-Boolean test target. Every authoritative test target must:

- compile as strict portable C++17 with extensions disabled;
- receive strict floating-point options after Ygor's inherited GNU `-ffast-math`;
- reject `__FAST_MATH__` in common authoritative code;
- use no fetched content or test-package discovery; and
- state its build/platform qualification profile explicitly.

Existing sanitizer options may define additional qualification cells. Sanitizer output is platform instrumentation, not canonical evidence and not a replacement for in-tree race, mutation, resource, or malformed-input tests.

### 1.3 Existing fixtures

`YgorMath_Samples.h/.cc` contains tetrahedron, octahedron, icosahedron, single-triangle, and single-quad samples. Reuse only reviewed coordinate/connectivity data as fixture seeds after exact-bit import, explicit orientation/occupancy metadata, independent validation, stable fixture IDs, and instantiation for every required `T`/`I` pair. The open samples are useful as invalid inputs. Existing sample functions and mesh utilities are not expected-result oracles.

Do not use duplicate merging, orientation repair, hole filling, remeshing, refinement, Delaunay, convex-hull, BSP, or old Boolean routines to certify generator validity or Boolean results.

### 1.4 Promote Component 03's exact oracle

Plan 03 already requires a test-only base-`2^32` exact integer/rational implementation and explicitly permits Component 16 to adopt it. Refactor it into the shared modules below rather than creating a second exact-number implementation. Preserve Component 03 tests and golden behavior.

The promotion must retain test-only linkage, generalize the API, add quotient/remainder, divisibility, GCD and normalized rational support, enforce explicit resource domains, and keep Component 03 oracle-containment tests as direct clients. Component 03's production exact-float expansion provider is not the arbitrary-precision oracle and must not generate expected answers.

### 1.5 Mandatory predecessor reuse

Reuse without duplication:

- Component 01 IDs, owner validation, checked arithmetic, typed outcomes/errors, context/policies, transactions, resources, cancellation, canonical bytes, SHA-256, replay framing, and deterministic finding arbitration;
- Component 02 public-input adapter, validated topology and shell semantics;
- Component 03 strict bit operations, bounded evidence, precision/tolerance records, and promoted test oracle;
- Components 04-14 immutable artifacts, test views, codecs, structural counters, and independent verifiers;
- Component 15 final verifier, diagnostics, re-ingestion, focused replay, and publication evidence; and
- Component 17 deterministic execution controls, serial references, forced schedules/partitions/delays, counters and worker qualification when available.

Component 16 supplies test adapters and independent orchestration, not replacement producers.

### 1.6 Existing services that are not authoritative

Do not use `YgorSerialize`, native object bytes, mesh text I/O, `std::hash`, SpookyHash, MD5, an external DB, or filenames as canonical evidence. Do not use `std::mt19937` as a cross-implementation replay authority. Do not use existing thread pools as the qualification scheduler unless Component 17 qualifies them under its contract. Do not use `YgorMeshesVerification` or producer pass flags as independent proof.

## 2. Exact files, targets, and data layout

### 2.1 Test-support modules

Add under `tests/mesh_boolean_bounded/qualification/`:

```text
QualificationVersions.h
QualificationTypes.h
QualificationRegistry.h/.cc
QualificationManifest.h/.cc
QualificationHarness.h/.cc
QualificationAssertions.h
QualificationResult.h/.cc
QualificationCodec.h/.cc
QualificationEvidence.h/.cc
QualificationFailures.h/.cc
QualificationResources.h/.cc
ContractClauseInventory.h/.cc
ContractTraceability.h/.cc
DeterministicSeed.h/.cc
ExactUnsignedInteger.h/.cc
ExactInteger.h/.cc
ExactRational.h/.cc
ExactFloatImport.h/.cc
ExactGeometryOracle.h/.cc
AnalyticFixtures.h/.cc
ValidMeshGenerators.h/.cc
PresentationVariants.h/.cc
InvalidInputMutators.h/.cc
ArtifactMutationRegistry.h/.cc
QualificationMatrices.h/.cc
CampaignRunner.h/.cc
FailurePredicate.h/.cc
CaseShrinker.h/.cc
CorpusCodec.h/.cc
CorpusStore.h/.cc
ReplayQualification.h/.cc
PerformanceQualification.h/.cc
QualificationSelfTests.h/.cc
ComponentAdapters.h/.cc
QualificationMain.cc
CorpusToolMain.cc
```

Keep implementation-only builders, mutable campaign/shrink state, arbitrary-precision limbs, malformed decoders and fault-injection internals out of installed headers.

### 2.2 Per-component registration

Add explicit registration translation units:

```text
Component01Qualification.cc
...
Component17Qualification.cc
```

Each calls a named `register_component_NN_tests(registry&)` function. Never rely on static constructor registration or linker order. Existing component test functions may be wrapped only when they expose stable descriptors, deterministic inputs/results, and the required strict target.

### 2.3 Test adapters

For every component add a test-only adapter in its existing test support area, e.g. `ComponentNNTestAdapter.h/.cc`. The adapter must:

- construct valid immutable predecessor artifacts through production validation;
- invoke the component under a frozen context;
- expose read-only logical records and structural counters;
- invoke the independent verifier;
- build intentional corrupt artifacts without undefined behavior;
- inject resource limits, cancellation and defined faults;
- select supported providers/policies; and
- serialize a complete focused replay.

Friend/test hooks are permitted only under `YGOR_MESH_BOOLEAN_QUALIFICATION_BUILD=1`. They must not return mutable production storage, bypass ordinary successful-path validation, or be linkable from the production library.

### 2.4 CMake targets

Extend `tests/mesh_boolean_bounded/CMakeLists.txt` with:

- `ygor_mesh_boolean_qualification_support`, a static test-only library;
- `ygor_mesh_boolean_qualification`, the manifest-driven runner;
- `ygor_mesh_boolean_corpus_tool`, the validator/updater/migrator; and
- small isolated negative probe executables only where process isolation is necessary.

Apply the strict-FP helper and C++17 requirements to all authoritative targets. Add options:

```text
YGOR_BUILD_MESH_BOOLEAN_QUALIFICATION
YGOR_MESH_BOOLEAN_QUALIFICATION_DATA_ROOT
YGOR_MESH_BOOLEAN_BUILD_ID
YGOR_MESH_BOOLEAN_PLATFORM_PROFILE
```

A missing build ID is acceptable for component/integration development but is a release failure. Do not use the timestamp-derived Ygor project version as the implementation identity.

### 2.5 Committed data tree

Add:

```text
tests/mesh_boolean_bounded/qualification_data/
  contracts/        # clause inventories and clause-test map
  manifests/        # component, integration, continuous, release
  matrices/         # operation, degeneracy, type, policy, resource, etc.
  fixtures/         # analytic, adversarial, malformed
  corpus/
    index_v1.qci
    records/<digest-prefix>/<full-digest>.qcr
  performance/      # fixtures and structural envelopes
  golden/           # codecs, evidence, seed streams, shrink order
```

Every authoritative file uses Component 01 canonical framing, explicit versions, checked lengths/counts and a digest where applicable. Human-readable explanations may accompany records but are non-authoritative. Logical paths use `/` and contain no absolute paths, usernames, hostnames, PIDs, pointer values or locale-rendered numbers.

## 3. Stable identifiers, versions, enums and failures

### 3.1 Registry

Extend `ContractVersions.h` with nonzero V1 values for all Component 16 providers/schemas. Keep numeric assignments in the central registry, while test-only declarations remain behind test include boundaries. Reserve zero for invalid. Add compile-time serialized-enum uniqueness checks and runtime string-ID uniqueness checks.

### 3.2 IDs

Use restricted canonical ASCII identifiers with checked maximum lengths:

- clause `MB.C<NN>.<section>.<ordinal>`;
- test `MBQ.C<NN>.<level>.<name>.V<version>`;
- assertion `<test-id>.A<name-or-ordinal>`;
- fixture `MBF.<family>.<name>.V<version>`;
- generator `MBG.<family>.V<version>`;
- mutation `MBM.C<NN>.<field>.<name>.V<version>`;
- shrink operation `MBS.<domain>.<name>.V<version>`;
- performance gate `MBP.C<NN>.<rule>.V<version>`; and
- corpus record semantic ID plus content SHA-256.

IDs are not derived from line numbers, compiler type names, function addresses, registration order or filenames alone.

### 3.3 Closed enums

Define explicit fixed-width enums for qualification scope, test level, pass/fail/skip/infrastructure-error, expected success/failure/policy-dependent/not-applicable, equivalence kind, failure predicate, corpus compatibility, mutation disposition, freshness state and release-rejection reason. Reject zero/unknown values, duplicate singleton fields, incompatible combinations and nonzero reserved fields.

### 3.4 Subcodes

Reserve a Component 16 range and distinguish at least: duplicate ID, malformed descriptor, manifest digest mismatch, incomplete closed matrix, illegal release filter, missing build/platform profile, missing/stale clause mapping, missing test, required skip, exact-number limit, exact decode failure, oracle-domain error, generator-invalid, mutation-malformed, mutation-survived, seed mismatch, campaign mismatch, shrink non-reduction/predicate-loss, corpus collision/content mismatch, corpus transaction failure, replay incompatibility/mismatch, missing counter, performance gate failure, fault injection mismatch, resource leak, worker mismatch, evidence self-audit failure and qualification digest mismatch.

## 4. Immutable schemas

### 4.1 Test descriptor

`qualification_test_descriptor` contains stable ID/version, responsible component, level, required scopes, mapped clauses, supported type/policy/provider cells, fixture/generator/mutation/replay/performance dependencies, resource/work envelope, concurrency permission, canonical callback ID and expected outcome. The process-local callback pointer is not serialized. Resolve all callback IDs before execution and reject duplicates.

### 4.2 Concrete case descriptor

A case contains parent test ID, canonical ordinal, full dimension tuple, type/operation/operand order/policy/thread/resource/mutation/fixture selections, seed or corpus reference, expected equivalence, work/retention ceilings and case digest. Expand the complete ordered case list before execution. Release scope cannot create/omit cases based on timing, speed, hash order, prior pass rate or random early stopping.

### 4.3 Assertion and finding

Every assertion has a stable ID. A failure contains test/case/assertion IDs, class/subcode, exact expected/actual scalar bits or canonical values, owner/entity IDs, bounded evidence, clause IDs, deterministic witnesses, resource/cancellation progress and replay reference. Source location is normalized non-authoritative metadata. Formatting is bounded and never controls status.

### 4.4 Manifest

`qualification_suite_manifest` contains scope/version, specification and clause digests, required build/platform/type/policy/provider versions, exact ordered tests/matrix cells, campaign seeds and counts, corpus/mutation/performance digests, optional cells with exact reasons, resource ceilings, retention policy and manifest digest.

A smaller scope is a separate immutable manifest, not a filtered release manifest. Release mode rejects filters, iteration reductions, ignored failures, uncommitted corpus overrides and unspecified required platform cells.

### 4.5 Evidence bundle

Separate authoritative data from a non-authoritative appendix. Authoritative data includes exact ordered results, clause coverage, matrix coverage, oracle comparisons, mutation outcomes, corpus/replay results, structural counters/gates, self-tests, failures/skips and aggregate digest. The appendix may contain times, CPU descriptions, notes and local paths and is excluded from the digest.

### 4.6 Equality

Digests may select a bucket, never prove semantic equality. For every semantic comparison verify schema/length and then compare full canonical bytes or decoded fields. Include collision-injection tests for fixture, corpus, evidence and result comparisons.

## 5. Deterministic harness

### 5.1 Registry and discovery

`QualificationMain` constructs an empty registry, invokes component registration functions in numeric order, freezes and validates it, loads a manifest, expands cases and sorts by the complete canonical case key. Filesystem enumeration may locate data only after sorting normalized logical paths and validating them against the manifest.

Reject duplicate IDs, missing callbacks, empty mapped-clause sets where required, invalid scope masks, unknown versions, unavailable required adapters and descriptors whose declared capabilities disagree with the build.

### 5.2 Case isolation

Each case receives a private `qualification_case_context` containing immutable descriptor, frozen Boolean context, private resource ledger, cancellation source, deterministic seed substream, private findings, temporary data root and output builder. A case cannot read another case's mutable state. Reset or verify floating environment and thread-local services at each case boundary.

### 5.3 States

- `pass`: all required assertions and cleanup checks succeed;
- `fail`: tested behavior disagrees with an expectation;
- `skip`: only for a manifest-authorized optional cell with a stable reason;
- `infrastructure_error`: harness, descriptor, data, adapter or evidence system is invalid.

A required skip is a qualification failure. Unexpected exceptions are caught at the case boundary, converted to deterministic infrastructure records, all work is joined and resources are reconciled.

### 5.4 Deterministic execution and merge

The serial reference executes cases in canonical order. Component 17 may distribute immutable descriptors; each worker returns a private result. Merge by case key, reject missing/duplicate/conflicting outputs, recompute failure keys, and choose the primary displayed failure by canonical case/assertion/finding order. Completion order cannot affect results, retained failures, evidence or exit status.

### 5.5 Exit codes and output

Use stable nonzero process statuses for test failure, infrastructure failure, incomplete qualification and malformed invocation; keep exact numeric values versioned. Emit canonical evidence to an explicitly requested path through transactional write/rename. Human summaries are secondary. Never parse logs to determine qualification.

## 6. Contract annotation and traceability

### 6.1 Stable clause annotations

Add stable machine-readable annotations adjacent to every normative requirement in `broad_plan.md` and `component_01_*.md` through `component_17_*.md`. Use a non-rendering HTML comment such as:

```text
<!-- MB.C16.2.18.001 -->
```

Do not derive identity from heading text or line number. When one bullet has several independently testable obligations, assign separate IDs. Definitions/rationales may be marked non-executable only with a stable rationale code.

### 6.2 Inventory generator

`ContractClauseInventory` deterministically scans an explicit ordered source-file list, validates annotation grammar/uniqueness/component ownership, records normalized clause text and kind, and emits canonical inventory bytes/digest. It never recursively scans arbitrary directories.

Commit generated inventories for review. Qualification regenerates them and fails when committed content is stale. Specification edits therefore require an explicit traceability update.

### 6.3 Clause-test map

For every clause record responsible component, one or more test IDs, levels, scope requirements, type/policy coverage and direct-executable or justified-non-executable status. Release fails for an unmapped required clause, absent test, wrong component, required test marked optional, duplicate clause, stale version or inconsistent digest.

### 6.4 Provider dependency graph and freshness

Maintain a canonical graph from source/contract/provider/artifact/codec versions to tests, corpus records, matrices and evidence. A provider replacement computes the affected slice and must rerun direct tests, relevant verifier mutations, regressions, metamorphic/oracle comparisons, codec/replay checks, concurrency cells and structural gates. Evidence is stale if any transitive authoritative dependency digest differs.

## 7. Deterministic seed stream

### 7.1 Seed record

Store campaign kind/version, 64-bit root seed, PRNG provider, generator families/ranges, operation/policy/type distributions, thread/schedule/resource/mutation distributions, shrink version and exact iteration count/stopping condition. No unbounded time-based stopping is permitted in authoritative manifests.

### 7.2 SplitMix64 counter provider

Implement the specified stateless mapping using unsigned `uint64_t` modulo arithmetic:

```text
z = root + 0x9E3779B97F4A7C15 * (counter + 1)
z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9
z = (z ^ (z >> 27)) * 0x94D049BB133111EB
result = z ^ (z >> 31)
```

Derive independent domain roots with Component 01 domain-separated SHA-256 over campaign ID, generator/mutation/shrink ID, case ordinal and field ID, then take a specified little-endian 64-bit word. A consumer requests values by explicit counter; branching in one field does not shift another field's stream.

Use rejection sampling for bounded integers and a fixed bit construction for floating categories. Never use modulo bias, implementation-defined distributions or STL engine serialization.

### 7.3 Golden tests

Commit exact outputs for root/counter/domain combinations, bounded sampling, permutations and weighted selections. Verify identical case sequences, mutation order, schedule controls, shrink order, primary failure and evidence bytes across qualified compilers/platforms.

## 8. Test-only exact arithmetic

### 8.1 Unsigned integer

Represent magnitude as little-endian `std::vector<uint32_t>` base `2^32`, with no high zero limbs and empty vector for zero. Provide comparison, add, subtract with precondition, multiply, left/right shifts, bit length, fixed-width import, canonical encoding and checked resource accounting.

Implement quotient/remainder using a simple single-limb path and normalized Knuth Algorithm D for multi-limb divisors. Avoid signed overflow and undefined shifts. Return typed limit failure when configured limb/work ceilings are exceeded.

### 8.2 Signed integer

Use sign-magnitude with signs negative/zero/positive. Zero has empty magnitude and zero sign only. Implement comparison, negate, add/subtract/multiply, quotient/remainder with documented truncation semantics, exact divisibility, absolute value, GCD input extraction and canonical serialization. No negative zero is representable.

### 8.3 GCD and rational

Use deterministic Euclidean GCD. `ExactRational` stores numerator and strictly positive denominator, reduces on construction, uses denominator one for zero and rejects division by zero. Provide exact comparison, sign, `+ - * /`, integer and binary-float conversion, determinant helpers and canonical bytes.

Use cancellation-before-multiplication where it reduces intermediate size, but exact results and resource accounting remain deterministic. Every operation validates resource bounds before unbounded growth.

### 8.4 Oracle domain

Every oracle call declares limits for limbs, numerator/denominator bits, entities, candidate pairs, permutations, ray hits, arrangement cells and work units. Out-of-domain is a manifest/configuration result, never an approximate answer. A required release cell outside its declared domain fails as bad qualification configuration.

### 8.5 Independence/self-tests

Test arithmetic against fixed-width operations where safe, exhaustive small signed values, algebraic identities, quotient identity `a=q*b+r`, remainder bounds/sign, divisibility, GCD identities, rational normalization and malformed encodings. Use hand-computed decimal/hex vectors, not production expansion arithmetic.

## 9. Exact float import and geometry oracle

### 9.1 Binary float import

For `float` and `double`, decode sign/exponent/fraction from exact bits using `memcpy`-based Component 03 helpers. Convert finite values to exact dyadic rationals, preserving source signed-zero metadata while rational zero remains unsigned. Reject NaN/infinity where forbidden. Never format/parse decimal text.

Test zeros, all exponent classes, smallest/largest subnormal, normal boundaries, adjacent values, maximum finite and randomized bit patterns against reconstruct-and-compare-bit logic.

### 9.2 Predicates

Implement independent exact rational versions of:

- 2D orientation;
- 3D orientation/signed tetrahedral volume;
- point-plane side;
- exact projection selection with explicit tie rule;
- point-in-triangle;
- segment-plane relation and parameter;
- edge-face crossing category;
- coplanar edge-edge relation;
- triangle-triangle proper/coplanar relation;
- event parameter order;
- polygon signed area and winding;
- bounded exact shell ray crossing/winding; and
- containment of exact values in Component 03 enclosures.

Return exact categories and rational witnesses. Do not mirror the production predicate dependency graph or call production relations to choose an oracle branch.

### 9.3 Exhaustive bounded oracles

Provide exhaustive all-pairs candidate enumeration, bounded permutation minimization for canonicalization, exact small carrier ordering, exact small winding/cell templates and independent output triangle-pair checking. Enforce explicit combinatorial limits before enumeration.

## 10. Fixtures and known-answer solids

### 10.1 Canonical fixture codec

Fixtures record exact source bits, indices/ring boundaries, operand roles, options, precision/tolerance, expected result/equivalence, versions and derivation metadata. Decoder validates all counts/indices/lengths/enums before allocation. Text renderings are optional; canonical binary records are authoritative.

### 10.2 Required families

Create hand-auditable integer/dyadic fixtures for empty meshes, tetrahedra, boxes, transformed boxes, convex polytopes, polygonal prisms/extrusions, cavities/islands, disconnected solids, source subdivisions and controlled contacts/overlaps. Store analytic occupancy/component/shell/selected-surface expectations or a bounded exact derivation recipe.

Include equal operands, all contact dimensions, tangency, coplanar partial/complete overlap, original-vertex/edge crossings, same-coordinate distinct events, concavity, holes, thin/sliver features, signed zero, subnormal, adjacent floats, extreme exponents, large translation, near parallelism, tolerance boundaries, cleanup cases and repeated Boolean chains.

### 10.3 Fixture validation

Every valid fixture first passes an independent fixture topology/geometry validator and Component 02. Invalid fixtures prove the intended violation and expected typed failure. Known-answer tests assert full normative artifact content, not only success.

## 11. Valid manifold generators and presentation variants

### 11.1 Common generator contract

A generator returns exact template metadata, public mesh bits, shell/occupancy semantics, provenance, deterministic parameters and a generator-specific shrink description. Before use as valid input, run an independent topology validator and Component 02. A generator validation failure is infrastructure failure, not an engine defect.

### 11.2 Constructive families

Implement at minimum:

- integer/dyadic convex polytopes from reviewed templates;
- boxes and orthogonal extrusions;
- star-shaped radial solids with a simple exact angular template and validated triangulation;
- polygonal extrusions with holes;
- nested shell trees with alternating orientation;
- disconnected multi-solids;
- topology-preserving edge/face subdivisions, triangulation refinements and legal flips;
- controlled handles/tunnels from boundary-compatible block assemblies;
- controlled thin features;
- selected-ULP vertex perturbations; and
- paired near-contact/near-parallel constructions.

Prefer proof by construction. Still validate independently; do not trust a generator's own flags.

### 11.3 Equivalent presentation registry

Implement vertex/facet/component permutations, ring rotation, globally corrected reversal, legal retriangulation/subdivision, duplicate-coordinate distinct topology, axis permutations, sign flips with orientation correction, exactly representable translations, power-of-two scale, operand exchange/remapping, allocation/traversal perturbation, thread count and partition changes.

Each transform declares an exact expected equivalence kind. Do not require identical non-authoritative diagonals unless the frozen output policy does.

## 12. Invalid input and intermediate-artifact mutation

### 12.1 Public input mutators

Provide stable mutators for all invalid categories in Component 16: out-of-range indices, undersized rings, duplicate consecutive indices, repeated directed edge, open/three-use edge, bow-tie/disconnected link, inconsistent orientation, malformed nesting, ambiguous semantics, excessive non-planarity, self-crossing ring, zero-area face, non-finite coordinates, unsupported metadata, severe self-intersection and count/index overflow.

Each records intended primary fault and allowed secondary faults. Single-fault tests preserve unrelated contracts where practical and independently prove the fault before engine invocation.

### 12.2 Artifact mutators

For every component artifact, create producer-shaped corruptions including wrong/stale owners, duplicate/missing entities, broken reciprocal pairing, coordinate-based welding, lineage corruption, omitted candidates/relations, changed crossing/multiplicity, duplicate/merged event, false classification union/winding, selection/orientation/multiplicity errors, output pairing/cycle faults, triangulation boundary/diagonal faults, unsafe cleanup/understated displacement, assembly map corruption, verifier report corruption and concurrency merge corruption.

The builder may recompute superficial counts/digests when the mutation intends to test deeper verification. Record the intended rejecting verifier/clause. A malformed artifact rejected before reaching the intended seam does not count, unless the manifest explicitly accepts an earlier independent gate.

### 12.3 Independence audit

For each mutation identify producer helpers and verifier helpers involved. Build/link boundaries and tests must prove the intended verifier does not call the producer's grouping, relation, pairing, classification, cleanup or pass routine for the behavior under test.

## 13. Closed qualification matrices

Store explicit canonical matrices rather than implicit nested loops. Every required cell is success, typed failure, policy-dependent, justified not-applicable or missing; missing is qualification failure.

Required dimensions include:

- all Boolean operations and directed operand orders;
- every degeneracy/contact category in Component 16 Section 2.13;
- algebraic laws and operation remapping;
- `float/double` × `uint32_t/uint64_t`;
- supported compiler/platform/floating-environment profiles;
- all supported policy/provider/codec/replay versions;
- minimum/maximum/sentinel/count boundaries;
- resource limit minus one/equal/plus one/overflow/rollback/contention;
- every cancellation checkpoint;
- thread counts, schedule/partition/delay variants and simultaneous failures;
- exact-oracle categories;
- every generator and mutation;
- permanent corpus compatibility; and
- structural performance gates.

Matrix expansion sorts by stable dimension identifiers, validates declared domains and emits a digest. Release evidence contains both expected and actual disposition for every cell.

## 14. Deterministic campaigns and fuzzing

### 14.1 Execution

For each ordinal, derive a complete case before running it, independently validate generator assumptions, serialize the full pre-execution case, execute selected component/full paths, run intended verifiers and bounded oracles, retain the first canonical failure plus policy-bounded secondary failures, and obey exact per-case/campaign work ceilings.

Do not mutate arbitrary bytes directly into production structs. Fuzz decoders with bytes; fuzz artifacts through validated test builders.

### 14.2 Failure retention

A failure record contains source/artifact bytes, seed/ordinal, full options, providers/policies, thread/resource controls, stage, assertion/finding, oracle result and replay. Retain it before shrinking. Storage exhaustion becomes deterministic infrastructure failure; never discard the only complete reproducer.

### 14.3 Campaign kinds

Provide deterministic valid-property, invalid-input, artifact-mutation, malformed-codec, metamorphic, exact-differential, resource/cancellation, concurrency and structural-performance campaigns. Component/integration/continuous/release manifests select exact seeds and iteration counts.

## 15. Deterministic shrinking

### 15.1 Failure predicate

Freeze whether the shrink preserves exact typed error/subcode, specified verifier finding, crash/UB, nondeterminism, oracle disagreement, replay mismatch, structural regression or broader class. Never silently weaken it. A witness change is allowed only when policy says so.

### 15.2 Metric and search

Use a lexicographic metric over canonical logical fields: total entities, participating closure, serialized bytes, coordinate bit complexity/magnitude, options/policy complexity, thread/schedule complexity and canonical bytes. Each accepted step must strictly reduce the metric.

Run deterministic best-first search. Generate candidates in stable shrink-operation order, validate, deduplicate by digest plus full bytes, execute predicate under exact controls, and break equal metrics by canonical bytes. Enforce visited/candidate/work/byte limits and return best proven candidate with completeness status.

### 15.3 Operations

Implement topology-aware removal of unrelated components/shells/faces and inverse subdivision; ring/vertex reduction preserving validity; artifact evidence-closure cropping; exact bit movement toward zero, adjacent values, translation normalization, power-of-two scale and tolerance/precision boundary reduction; operation/policy/thread/schedule simplification; mutation removal; and replay payload reduction.

A valid-input failure may never be reported as minimized if shrinking made it invalid. Re-run independent validity after every such candidate. Record old/new bits and operation ID for numeric steps.

### 15.4 Self-tests

Use synthetic predicates and fixed real reproductions to verify order, strict reduction, validity, predicate preservation, termination, tie resolution, resource behavior and replay of the final result. Commit golden candidate/acceptance sequences.

## 16. Permanent corpus

### 16.1 Record schema

Each `qcr` stores exact input/artifact bits, types, operation/options, expected success/failure and stable finding, expected content/topology digest or equivalence, geometry/precision/cleanup/verification status, provider/policy constraints, originating campaign/seed, minimized predicate, clause IDs, original/minimized digests, fixed build metadata when relevant, expected post-fix result and explanation.

### 16.2 Content addressing and deduplication

Compute a semantic record digest over canonical fields. Use a digest-prefix directory only as lookup. On any digest match compare full content and predicate semantics. Same mesh with different tolerance, operation, policy, failure or witness remains distinct.

### 16.3 Transactional update

`corpus_tool add` writes a private candidate, validates/decode-re-encode, executes replay/expected result, computes digest, checks full-content dedupe, writes the record, builds a proposed sorted index/traceability update, validates the complete corpus and publishes the index last by atomic rename. A failed update leaves the old index authoritative and no referenced partial record.

The tool refuses to modify the source tree during ordinary test execution. Updates require an explicit developer command and clean destination policy. Canonical output is independent of temporary filenames.

### 16.4 Migration/deletion

Schema migration is a deterministic old-to-new conversion with old/new digests, compatibility evidence and transactional index replacement. Never silently delete a confirmed regression. Retirement requires an explicit tombstone/migration record, reviewed rationale and preserved historical replay expectation where the specification requires it.

## 17. Replay qualification

Validate record framing, lengths, counts, indices, versions and digest before allocation/execution. Reconstruct exact bits/indices, frozen context, providers/policies, resources, execution controls and requested component/full path. Compare result with expected full content/equivalence or primary failure and emit a canonical compatibility result.

Unsupported obsolete required versions are explicit incompatibility, not reinterpretation. Replay every permanent record across required thread/type/policy cells. Add focused replay tests for each component and full end-to-end replay through Component 15.

## 18. Resource, cancellation, allocation and exception qualification

### 18.1 Test-system resources

Add Component 16 resource kinds for registry/manifest/case records, exact limbs/rationals, generated entities, mutation copies, campaign failures, shrink frontier/visited set, corpus bytes/index, replay bytes, traceability, evidence, reports, tasks and abstract work. Use Component 01 checked arithmetic/reservations and separate temporary/persistent usage.

Successful cases need not retain complete transient data; every failure must retain the complete reproducer subject to a manifest ceiling that is pre-reserved. No unbounded success-history vector is permitted.

### 18.2 Boundary matrix

For every production and test resource class test limit−1, limit, limit+1, overflow before comparison, rollback after later failure, concurrent contention, temporary/persistent accounting and deterministic primary failure when several limits cross. Use constrained adapters for impossible physical counts.

### 18.3 Fault injection

Implement test-only deterministic allocators/resource adapters keyed by logical allocation/reservation ordinal and kind, not native allocator behavior. Cover containers, task outputs, maps/sorts, exact limbs, public storage, reports/replay/serialization and worker/task storage. Map failures to documented typed outcomes and prove strong transaction safety.

### 18.4 Cancellation/exceptions

Exercise every registered checkpoint before stage start, after reservation, during serial loops, parallel tasks, canonical merge, serialization/digest, final verification and immediately before commit. Confirm all workers join, reservations return, no partial artifact/evidence is visible, predecessors remain valid and deterministic progress is reported. Catch injected exceptions at defined boundaries and map them without leaks or partial publication.

## 19. Concurrency and race qualification

Component 16 does not implement a second worker pool. Through Component 17 controls run one, two and maximum workers, reversed submission, deterministic delays, alternative partitions, small/large grains, simultaneous failures, cancellation with private outputs and repeated execution. Compare artifacts, errors, reports, replay, counters and evidence under the frozen invariance contract.

When compiler race instrumentation is available, record the configured matrix result outside canonical semantic bytes. Independently run in-tree stress for shared mutable access, publication-before-join, reservation leaks, double commit/rollback, stale task-local IDs, cancellation misuse, condition-variable predicates, task lifetime and nondeterministic merge. Any race finding blocks release.

## 20. Structural performance qualification

### 20.1 Counter registry

Version definitions for source/output visits, broad-phase nodes/candidates, bounded relation checks, event merges, classification edges, retained uses, triangulation candidates/ears, cleanup candidates/actions, canonical-label states, verifier pairs, task/merge records, bytes and abstract work. A test fails if required counters are absent or provider versions disagree.

### 20.2 Fixtures and gates

Commit scalable families for large disjoint meshes, clustered intersections, many events per edge, many independent faces, high valence, complex polygons, cleanup-heavy meshes, symmetric canonicalization and verifier loads. Store structural relations/envelopes rather than only one absolute count.

Required gates detect all-pairs narrow phase on disjoint meshes, relation recomputation, duplicate event construction, per-vertex ray classification, prohibited carrier ordering complexity, unbounded triangulation retries, excessive cleanup rescans, source-order canonical fallback and verifier candidate omission.

A provider may return its documented resource limit but cannot hide regression by disabling verification, reducing committed fixture size or changing order. Envelope changes require explicit provider/counter/fixture version, rationale and review-visible old/new evidence.

### 20.3 Wall-clock data

Optional timing includes fixture/build/platform/provider/workers/warm-up/samples/counters/memory. It is non-authoritative unless a separately versioned stable-environment release policy says otherwise. Timing never overrides structural correctness evidence.

## 21. Evidence and release decision

### 21.1 Canonical construction

After all cases finish, sort results by canonical key; validate completeness; recompute clause/matrix/mutation/corpus/replay/performance summaries from primitive results; reconcile resources; encode authoritative fields; compute domain-separated SHA-256; decode into an independent self-auditor; then transactionally publish evidence. Never trust producer-supplied summary counts as sole evidence.

### 21.2 Release pass conditions

Release passes only when every required test ran/passed, every required clause has fresh evidence, no valid test expects `internal_invariant_error`, exact oracles agree, no broad-phase false negative exists, no required mutation survives, every ordinary success re-ingests and passes Component 15, all required type/platform/policy/resource/cancellation/concurrency/replay cells pass, structural gates pass and the bundle is complete/self-consistent.

Missing evidence, unexpected skip, stale dependency, unsupported required platform, race finding, resource leak, malformed mutation, replay mismatch, corpus failure, oracle disagreement, structural regression or digest/self-audit failure rejects release.

### 21.3 Failure arbitration

Select the primary displayed failure by stable case then assertion/finding key while retaining all failures within predeclared limits. Parallel completion and timing cannot affect ordering, evidence bytes or decision. If retention cannot satisfy the manifest, qualification is infrastructure failure rather than a truncated pass.

### 21.4 Release dry-run mutations

Build intentionally altered evidence for one failed test, missing test, required skip, stale clause, mutation survivor, replay mismatch, oracle disagreement, performance regression, unsupported cell, missing counter and corrupted digest. Require deterministic rejection and exact reason.

## 22. Security and malformed-input robustness

Treat fixtures, corpus, replay, manifests, matrices and evidence as untrusted. Before allocation check framing, nesting depth, lengths, counts, indices, references, enum/version values, integer arithmetic, exact-number encoding, path normalization and diagnostics limits. Reject unknown required fields/versions, malformed limbs/rationals, duplicate singleton fields, path traversal and partially decoded execution.

Fuzz each decoder with deterministic arbitrary bytes and structured corruption. Require no UB, unchecked allocation, accidental execution, unbounded diagnostics or machine-specific path use. Add round-trip, truncation, endian-simulation, locale and digest-corruption tests.

## 23. Component test-adapter and coverage requirements

For each Component 01-17 adapter provide tests for minimum valid input, empty valid artifact where supported, all enum/policy branches, all stable errors, owner/ID domains, count boundaries, invariants/prohibited seams, codec/digest versions, commit/rollback/cancellation/resource behavior and every registered mutation.

For generated valid cases require published artifacts pass independent verifiers, IDs are unique/owner-correct, precision is monotonic, coordinate equality does not create identity, event sharing is consistent, paired halfedges reciprocal, cycles close, cleanup preserves manifoldness after every action, public assembly is bijective, final verification accepts ordinary successes and replay reproduces result/failure.

For provider replacement, use the dependency graph to execute the exact affected slice; do not allow an implementation change to weaken public contracts or silently invalidate old replay.

## 24. Required Component 16 self-tests

Register self-tests for:

1. registry/discovery/order and duplicate/malformed registration;
2. pass/fail/skip/infrastructure states, assertion encoding and exit codes;
3. manifest expansion, illegal filters and scope labeling;
4. clause scanning, missing/stale/wrong mappings and duplicate clauses;
5. seed/domain stream reproduction and unbiased sampling;
6. exact integer/rational arithmetic, limits and malformed encodings;
7. exact float conversion and geometry known answers;
8. analytic fixture validation and all generator validity proofs;
9. public/artifact mutator intended-fault checks;
10. mutation disposition and zero-survivor accounting;
11. deterministic campaign materialization/failure retention;
12. shrink order, strict metric, predicate/validity preservation and termination;
13. corpus codec, full-content collision fallback, transaction and migration;
14. replay compatibility/result comparison;
15. resource rollback, allocation fault, cancellation and exception mapping;
16. parallel private-result merge and simultaneous failure arbitration;
17. structural counter/gate interpretation;
18. evidence canonicalization, appendix exclusion, completeness, self-audit and digest;
19. malformed corpus/replay/manifest/matrix/evidence fuzzing; and
20. every release dry-run rejection.

A broken self-test or missing self-test result prevents qualification.

## 25. Required qualification suites

Create separate stable CTest entries/manifests for:

- harness/infrastructure self-tests;
- exact arithmetic and geometry oracle;
- fixture/generator/mutator/shrinker/corpus infrastructure;
- each Component 01-17 unit/known-answer/property/metamorphic/adversarial/mutation/replay/resource suite;
- compact end-to-end smoke coverage;
- full degeneracy/operation/algebraic matrices;
- permanent corpus replay;
- exact-oracle differential suite;
- resource/cancellation/concurrency matrices;
- structural performance gates; and
- release evidence dry-run/full release qualification.

The smoke set must cover empty identities, disjoint operations/differences, containment, proper overlap, equal operands, all contact dimensions, cavity/island, concave polygon, near-parallel, cleanup, duplicate-coordinate separation, repeated chain, all types and one thread/resource/cancellation matrix. It is never mislabeled release.

## 26. Implementation sequence and gates

Implement in this order; every gate leaves previous tests buildable and passing:

1. Add versions, enums, IDs, subcodes, resource kinds and registry checks.
2. Add explicit registry, private case context, result/assertion records, serial harness, CLI and self-tests.
3. Add immutable manifests and canonical case expansion.
4. Add clause annotations, scanner, inventory, mapping, freshness and traceability tests.
5. Promote Plan 03 exact arithmetic; preserve Component 03 behavior.
6. Complete division/GCD/rational normalization, exact float import, geometry oracle, domains and self-tests.
7. Add fixture codec/analytic solids and independent validation.
8. Add valid generators and required families.
9. Add presentation variants and equivalence registry.
10. Add invalid-input mutators and typed-error tests.
11. Add all component adapters/artifact mutations and intended-verifier tests.
12. Add closed matrices for all required dimensions.
13. Add PRNG/seed records, campaign scheduler, pre-run serialization and fixed-seed tests.
14. Add predicates, metric, shrink operations, best-first search and self-tests.
15. Add corpus codec/store/index/dedupe/transaction/migration and malformed tests.
16. Add component/full replay and permanent corpus execution.
17. Add resource/fault/cancellation matrices and prove zero partial publication/leaks.
18. Integrate Component 17 controls while retaining serial evidence semantics.
19. Add structural counters, fixtures, envelopes and gates.
20. Add evidence reconstruction, self-audit, digest, transaction and release decision.
21. Register every Component 01-17 test/clause mapping and resolve missing/stale coverage.
22. Execute component/integration manifests for all four type pairs.
23. Execute continuous manifest, complete mutation set, corpus, campaigns and smoke set.
24. Execute every release dry-run defect and require fail-closed rejection.
25. Execute the full release manifest for each supported configured platform profile, including sanitizer/race cells required by policy.
26. Update `tracker.md` only after this implementation plan is complete and committed; implementation completion is governed by Section 27.

Do not weaken a prior provider, oracle domain, mutation, matrix, corpus expectation or performance fixture to make a later gate pass. Version intentional changes and migrate evidence explicitly.

## 27. Implementation definition of done

Component 16 implementation is complete only when:

- the dependency-free strict C++17 harness deterministically discovers, orders, executes, merges, reports and exits for every state;
- no normative test downloads or requires an external framework/library/service;
- every normative clause in `broad_plan.md` and Components 01-17 has a stable annotation and fresh executable mapping or reviewed non-executable rationale;
- all manifest, test, assertion, fixture, generator, mutation, shrink, matrix, corpus, replay, performance and evidence IDs/versions are explicit and validated;
- the shared base-`2^32` exact integer/rational service is canonical, resource-bounded, independently tested and used by Component 03/16 without production linkage;
- exact binary-float import and required low-complexity/exhaustive oracles pass known answers and algebraic tests;
- analytic fixtures and every valid generator are independently validated before valid-input conclusions rely on them;
- every invalid-input and artifact mutator produces its intended fault and is rejected by the intended verifier or an explicitly accepted earlier independent gate;
- required mutation survival is zero;
- campaigns are byte-replayable from seed records and retain complete pre-shrink failures;
- shrinking is deterministic, strictly reducing, predicate-preserving, validity-preserving where required, bounded and replayable;
- every confirmed defect has a minimized permanent content-addressed corpus record and no record is silently deleted;
- corpus updates, migrations and evidence publication are transactional;
- every permanent record replays under all required compatibility/type/policy/thread cells;
- all operation, operand-order, algebraic, degeneracy, type, platform, policy, resource, cancellation, concurrency, replay, mutation and performance cells are explicit and covered;
- every ordinary end-to-end success re-ingests and passes Component 15;
- exact differential tests find no disagreement and exhaustive broad-phase tests find no false negative;
- resource, allocation, exception, cancellation and concurrency tests prove worker join, rollback, predecessor validity and zero partial publication;
- structural gates detect prohibited regressions without disabling verification or reducing committed fixture sizes;
- authoritative evidence is independent of scheduling, filesystem order, locale, wall-clock time and non-authoritative metadata;
- freshness, full-content collision fallback, self-audit, aggregate digest and release decision pass mutation tests;
- release cannot report success with failed/missing/skipped/stale evidence, missing clause, mutation survivor, replay mismatch, oracle disagreement, unsupported required cell, race finding, resource leak, structural regression or corrupted evidence; and
- all prior Component 01-15 tests and eventual Component 17 tests remain passing under required strict targets and supported build matrix.
