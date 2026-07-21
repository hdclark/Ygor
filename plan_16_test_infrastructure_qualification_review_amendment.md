# Plan 16 Review Amendment: Effective-Contract Qualification and Release Evidence

## Status, precedence, and implementation intent

This file is a normative implementation-plan amendment to `plan_16_test_infrastructure_qualification.md`. It records the independent Component 16 review required by `tracker.md` and integrates the broad-plan implementation order, reviewed Component 12-15 contracts, Component 17's execution boundary, the actual Ygor test/build layout, and the corrected evidence/corpus contracts in the corresponding Component 16 review amendment.

The original plan remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, corpus maintainers, build engineers, and release engineers must read both files and the Component 16 specification amendment.

Retain the original architecture: explicit dependency-free registry, serial semantic runner, private case state, exact test arithmetic, exact low-complexity geometry, constructive fixtures/generators, classified mutators, deterministic campaigns and shrinking, permanent corpus, focused/full replay, closed matrices, structural counters, canonical evidence, independent self-audit, fail-closed release decision, strict C++17, and no external dependencies.

Apply these corrections:

1. compile an effective normative contract from base specifications plus controlling review amendments instead of treating every scanned sentence as simultaneously current;
2. separate bootstrap/integration infrastructure from release closure after Component 17;
3. add explicit top-level CTest activation because the current root does not define `BUILD_TESTING`;
4. separate semantic case records, profile evidence fragments, and aggregate multi-profile release evidence;
5. bind freshness to an explicit implementation/build manifest rather than only a build label;
6. make exact-float decoding and oracle branch selection independent of Component 03 production helpers;
7. qualify Component 15 independently rather than counting the publication pass twice;
8. replace the one-file-per-digest corpus layout with collision-safe immutable generations;
9. bound and separate outer test-case concurrency from inner Component 17 concurrency;
10. consume reviewed numerical/dimensional/correspondence/digest contracts; and
11. establish a finite non-recursive evidence-audit trust boundary.

## 1. Reviewed provider and schema versions

Retain original V1 providers where behavior remains valid, but add new nonzero versions conceptually equivalent to:

```text
effective_contract_manifest:          explicit_precedence_graph_v2
effective_clause_state:               required_superseded_compatibility_v2
implementation_obligation_inventory:  plan_gate_separate_domain_v1
clause_annotation_migration:           semantic_id_preserving_v2
qualification_lifecycle:              bootstrap_integration_release_closure_v2
implementation_manifest:              explicit_source_build_provenance_v1
semantic_case_result:                 cross_profile_semantic_projection_v1
profile_evidence_fragment:            executed_profile_primitive_records_v1
release_evidence_set:                 closed_profile_fragment_aggregate_v1
profile_cell_partition:               exact_single_owner_cells_v1
profile_aggregation:                  validate_then_canonical_aggregate_v1
exact_float_import:                   independent_iec60559_dyadic_v2
oracle_independence:                  explicit_helper_boundary_v1
component15_qualification:            independent_topology_geometry_mutation_v2
corpus_layout:                        collision_bucket_immutable_generation_v2
corpus_current_pointer:               manifest_last_generation_pointer_v1
qualification_scheduler:              component17_test_adapter_v1
nested_execution:                     outer_or_inner_prebounded_v1
self_audit:                           primitive_record_reconstruction_v2
```

Extend `ContractVersions.h` and Component 16's closed registries with stable IDs for lifecycle phase, effective clause state, profile identifier, evidence domain, field classification, implementation-manifest field, corpus generation, collision bucket, aggregate disposition, oracle-independence record, and self-audit mutation.

Reserve zero for invalid. Unknown, mixed, contradictory, or obsolete required versions fail before large allocation or test execution.

## 2. Corrected existing-Ygor assessment

### 2.1 Test paths

Preserve `tests/compile.sh` and `tests2/` for unrelated Ygor work, but do not use them as normative bounded-Boolean qualification paths:

- `tests/compile.sh` directly invokes `g++` and has no stable manifest, traceability, canonical evidence, resource model, replay, or deterministic arbitration;
- `tests2/compile_and_run.sh` downloads doctest through `wget`, violating the no-network/no-external-framework contract; and
- existing sample-mesh functions may provide reviewed coordinate/connectivity seeds only after exact-bit import, explicit expected semantics, independent validation, stable IDs, and all required type instantiations.

Do not use existing Boolean implementations, repair, orientation, remeshing, convex-hull, BSP, duplicate merge, or producer verification flags as expected-result oracles.

### 2.2 Explicit CTest activation

The current root `CMakeLists.txt` adds `src` and `binaries` but does not call `include(CTest)`, `enable_testing()`, or otherwise define `BUILD_TESTING`. Therefore the original plan may not simply reuse `BUILD_TESTING` without adding the top-level hook.

Add near the root subdirectory section:

```cmake
include(CTest)

add_subdirectory(src)
add_subdirectory(binaries)
if(BUILD_TESTING)
    add_subdirectory(tests/mesh_boolean_bounded)
endif()
```

An equivalent reviewed organization is acceptable, but it must:

- use standard CTest activation;
- preserve unrelated default behavior when `BUILD_TESTING=OFF`;
- require no downloaded framework or configure-time network access;
- keep all bounded targets strict C++17 with extensions disabled;
- apply `ygor_apply_mesh_boolean_strict_fp` after inherited global flags;
- keep bounded targets isolated from optional Eigen/GSL and the ordinary fast-math target; and
- register stable focused CTest entries whose names are generated from explicit manifests, not static initialization or directory enumeration.

Add a configure/build audit test over an explicit bounded CMake/source list. It must reject `FetchContent`, `ExternalProject`, `wget`, `curl`, package-download logic, external test framework discovery, or transitive linkage to prohibited optional/general targets. Do not recursively inspect arbitrary user directories.

## 3. Exact files, targets, and data layout amendments

### 3.1 Additional qualification modules

Add under `tests/mesh_boolean_bounded/qualification/`:

```text
EffectiveContractTypes.h
EffectiveContractManifest.h/.cc
EffectiveContractResolver.h/.cc
ImplementationObligationInventory.h/.cc
ClauseAnnotationMigration.h/.cc
ImplementationBuildManifest.h/.cc
ImplementationFreshness.h/.cc
EvidenceDomains.h
SemanticCaseResult.h/.cc
ProfileEvidenceFragment.h/.cc
ProfileCellPartition.h/.cc
ReleaseEvidenceSet.h/.cc
ReleaseEvidenceAggregator.h/.cc
ExactFloatImportIndependent.h/.cc
OracleIndependence.h/.cc
Component15IndependentOracle.h/.cc
CorpusGeneration.h/.cc
CorpusCurrentPointer.h/.cc
CorpusRecovery.h/.cc
QualificationSchedulerAdapter.h/.cc
QualificationSelfAuditor.h/.cc
```

Keep mutable builders, corpus staging state, fault injection, malformed readers, and private scheduler state out of installed and production headers.

### 3.2 Build-manifest support

Add a CMake helper such as:

```text
cmake/YgorMeshBooleanQualificationBuildManifest.cmake
```

It generates canonical inputs for `ImplementationBuildManifest` from an explicit ordered source/configuration list. Do not derive authoritative identity from timestamps, absolute build paths, usernames, PIDs, pointer values, or the repository's timestamp-derived project version.

A developer-provided `YGOR_MESH_BOOLEAN_BUILD_ID` remains useful display metadata, but release freshness must also include source/configuration/provider digests.

### 3.3 Executables and targets

Retain the original qualification and corpus targets and add:

- `ygor_mesh_boolean_release_aggregate` — validates profile fragments and constructs the aggregate release evidence set;
- `ygor_mesh_boolean_contract_inventory` — optionally emits/reviews effective-contract and implementation-obligation inventories; and
- isolated negative build probes only where compile/link failure must be tested in a separate process.

All tools are in-tree C++17 and use only standard-library/toolchain facilities. They must not invoke network services, external databases, shell-based JSON tools, downloaded test frameworks, or external hashing/serialization packages.

### 3.4 Corrected committed data tree

Amend the original data layout to:

```text
tests/mesh_boolean_bounded/qualification_data/
  contracts/
    effective_contract_v2.qec
    implementation_obligations_v1.qio
  manifests/
    bootstrap/
    integration/
    continuous/
    release/
  profiles/
    required_profiles_v1.qpp
  matrices/
  fixtures/
  corpus/
    CURRENT.qcp
    generations/
      <generation-id>/
        generation.qgm
        index.qci
        traceability.qct
        records/
          <digest-prefix>/<full-digest>/<record-id>.qcr
  performance/
  golden/
```

`CURRENT.qcp` names and authenticates one complete generation. Readers never infer the current corpus by directory ordering or newest timestamp.

## 4. Effective contract and traceability implementation

### 4.1 Explicit source manifest

`EffectiveContractManifest` stores an explicit ordered source list, document role, content digest, base/amendment relationship, precedence edge, supported compatibility range, and expected annotation inventory.

Do not recursively glob all `*.md`. Review amendments are first-class inputs. Validate that each amendment references the expected base digest/range and that precedence is acyclic and complete.

### 4.2 Resolution algorithm

`EffectiveContractResolver` must:

1. validate all document and annotation digests;
2. build a directed precedence graph;
3. reject cycles, missing bases, ambiguous controls, and duplicate semantic IDs;
4. resolve each clause into `effective_required`, `effective_non_executable`, `historical_superseded`, or `compatibility_only`;
5. retain controlling-clause reverse maps;
6. detect unresolved contradictory effective obligations;
7. emit canonical effective-clause bytes/digest; and
8. transactionally compare/regenerate the committed inventory.

Tests must include amendment-over-base, amendment-over-amendment, narrowing, compatible clarification, complete supersession, stale base digest, missing controlling clause, precedence cycle, duplicate ID, semantic-content change under one ID, and two unresolved effective conflicts.

### 4.3 Plan obligations

`ImplementationObligationInventory` separately records plan and plan-amendment implementation instructions. It may gate implementation completion and review, but it must not be merged silently into public contract coverage.

Tests must prove that adding a file-layout suggestion to a plan does not create a public behavior clause and that changing a reviewed provider/file layout updates implementation obligations/freshness without falsely changing public contract semantics.

## 5. Lifecycle gates

Add closed phases:

- `bootstrap`;
- `integration`;
- `continuous`;
- `release_fragment`; and
- `release_aggregate`.

A manifest declares exactly one phase and its allowed dependencies.

Bootstrap/integration may run before Component 17 only with the serial qualification runner and explicit future-unavailable Component 17 cells. Such cells are neither passed nor release-not-applicable.

`release_fragment` and `release_aggregate` must reject execution when:

- Component 17 is absent, obsolete, or partially registered;
- required serial references, forced schedules, worker-environment tests, counters, or structural gates are missing;
- the effective contract excludes current Component 17 content;
- stale pre-Component-17 evidence is supplied; or
- the profile partition is incomplete.

## 6. Implementation provenance and freshness

### 6.1 Manifest schema

`implementation_build_manifest` contains:

- canonical source revision and/or canonical source-tree digest over an explicit list;
- effective-contract and implementation-obligation digests;
- relevant CMake and generated configuration input digests;
- compiler identity/version, target architecture, operating-system profile, and standard-library profile;
- exact authoritative compile/link option records for every bounded production/qualification target;
- strict floating build profile, contraction, reassociation, signed-zero, subnormal, rounding, and IPO/LTO disposition;
- provider/policy/schema/codec/replay/counter/adapter versions;
- generated tables and qualification-data digests;
- enabled bounded build/test features; and
- non-authoritative display metadata.

Normalize only fields whose semantics are explicitly profile-independent. Absolute paths and timestamps are prohibited from authoritative bytes.

### 6.2 Source/configuration capture

Prefer source revision plus a clean-tree proof when available, but do not require Git at test runtime. Release packaging may provide a generated canonical source manifest. A dirty or source-archive build must use exact ordered file digests.

The manifest generator must fail closed when an authoritative source/configuration file is absent, duplicated, unreadable, or outside the explicit allowed root.

### 6.3 Freshness graph

Build a canonical dependency graph from implementation inputs to tests, matrices, corpus records, fragments, and aggregate evidence. Any changed transitive input makes dependent evidence stale.

Mutation tests must alter source digest, strict flags, provider version, compiler profile, generated table, effective contract, corpus generation, matrix, adapter, and counter schema and require the expected stale slice.

## 7. Evidence domain split

### 7.1 Semantic case result

`semantic_case_result` contains only fields intended to compare across qualified profiles:

- stable test/case identity and complete logical dimensions;
- exact source/artifact/corpus identity;
- expected equivalence or typed failure;
- actual authoritative semantic disposition;
- public semantic content/topology digest where applicable;
- stable finding class/subcode and canonical semantic witness;
- exact-oracle outcome/witness where applicable;
- semantic structural counters explicitly classified as invariant; and
- semantic result digest.

It excludes compiler strings, sanitizer text, paths, timing, actual worker scheduling, memory addresses, and profile-specific build records.

### 7.2 Profile evidence fragment

`qualification_profile_fragment` contains:

- profile ID and `implementation_build_manifest`;
- effective-contract, manifest, matrix, corpus-generation, provider, and policy digests;
- exact assigned profile cells;
- complete primitive case results and semantic projections;
- platform/floating-environment qualification;
- sanitizer/race cells where assigned;
- profile-bound resources, counters, and structural gates;
- corpus/replay/mutation/self-test results;
- all retained failures/skips/infrastructure findings;
- independent self-audit result; and
- profile-fragment digest.

The fragment claims only cells executed by that process under that profile.

### 7.3 Aggregate release evidence

`release_qualification_evidence_set` contains the closed required profile partition, canonical validated fragments, semantic cross-profile comparisons, exact cell ownership, aggregate failures, and release decision.

It must not copy a fragment pass bit as authority. The aggregator reconstructs completeness and conflicts from primitive fragment records.

### 7.4 Field classification

Every evidence field and counter is registered as one of:

- `semantic_invariant`;
- `profile_bound_authoritative`;
- `advisory_non_authoritative`; or
- `prohibited`.

Unknown classifications are invalid. Wall-clock time, raw sanitizer text, CPU marketing names, and local paths remain advisory appendix data and are excluded from semantic/profile/aggregate digests unless a separately reviewed stable encoding is required.

## 8. Profile partition and aggregation

### 8.1 Closed partition

`profile_cell_partition` explicitly assigns every required platform/compiler/sanitizer/race/type/policy/counter cell to exactly one profile or to a stable justified not-applicable record.

A fragment cannot dynamically claim cells based on availability, speed, prior results, or environment discovery beyond validation of its already assigned profile.

### 8.2 Aggregator validation

For every input fragment, validate framing, full content, digest, self-audit, profile identity, build provenance, freshness, exact cell list, and no unexecuted claims.

Across fragments require equal:

- implementation source/configuration identity except explicitly profile-varying compiler/build fields;
- effective contract;
- corpus generation;
- manifests and matrices;
- semantic provider/policy versions; and
- semantic case input identities.

Require semantic equality where the matrix declares it. A different ordinary success/failure, topology digest, finding, replay semantic result, or semantic counter is an aggregate failure.

Reject missing, duplicate, overlapping, stale, foreign, contradictory, differently partitioned, or differently scoped fragments.

### 8.3 Transactional publication

Construct aggregate evidence privately, run the independent aggregate self-auditor, write canonical bytes to a private path, verify decode/re-encode/full content/digest, and publish by atomic rename. A failed aggregation leaves no artifact labeled release success.

## 9. Independent exact float and geometric oracle

### 9.1 Float importer

Implement `ExactFloatImportIndependent` directly from `std::uint32_t`/`std::uint64_t` bit fields obtained by `memcpy`. It independently handles signed zero, subnormal, normal, maximum finite, NaN/infinity rejection, exact exponent, exact significand, and dyadic rational construction.

Do not call Component 03 `FloatingBits`, production exact-expansion adapters, bounded operations, projection selection, or predicate result constructors to determine expected values.

Use hand-authored bit vectors, exact reconstruct-to-bits checks, exhaustive toy binary formats, and randomized full-width round trips. Inject mutations into each decoder independently and require differential detection.

### 9.2 Oracle independence records

For each differential oracle family, register:

- production entry points/helpers;
- oracle entry points/helpers;
- permitted shared primitives;
- prohibited shared branch/control-flow helpers;
- exact domain/resource limits; and
- independent witness category.

The build/link audit must ensure test oracle object files are not linked into production targets.

### 9.3 Geometric control flow

The rational geometry oracle independently chooses projection, relation category, half-open ray ownership, carrier order, polygon winding, and exact triangle-pair relation. It must not ask the production component which branch/category to evaluate and then merely recompute that branch exactly.

## 10. Reviewed Component 12-15 adapters

### 10.1 Schema updates

Update Component 12-15 adapters, fixture codecs, corpus records, mutations, and equivalence comparisons to carry:

- three distinct numerical truth layers;
- physical dimensions, closed metrics, dimensional derivations, and denominator certificates;
- cleanup role transitions and exact committed costs;
- semantic canonical content versus presentation correspondence;
- correspondence equivalence classes and automorphism/orbit certificates;
- public semantic digest versus presentation/replay digest;
- pending-only Component 14 candidate status; and
- Component 15 final-publication authority.

Reject mixed reviewed/obsolete schemas before constructing large mutation copies or oracle work.

### 10.2 Independent Component 15 oracle

Implement `Component15IndependentOracle` as bounded test-only code outside Component 15 high-level control flow. It must provide, in domain:

- exact-index public directed-use grouping, edge pairing, vertex-link cycles, components, orientation, and duplicate-coordinate separation;
- exact/bounded shell and occupancy checks on analytic fixtures;
- exhaustive non-adjacent triangle pairs and exact rational relation categories;
- primitive cleanup action/ledger reconstruction;
- independent precision/report maximum reconstruction;
- reviewed Component 02 re-ingestion semantic comparison;
- semantic canonical-content and correspondence-class checks; and
- focused expected findings for producer-shaped mutations.

Rerunning production Component 15 remains useful for deterministic replay/idempotence but is not counted as independent verification.

### 10.3 Required mutations

Include wrong truth-layer substitution, dimension mismatch, area-versus-length comparison, cleanup role confusion, understated committed displacement, semantic/presentation digest swap, arbitrary automorphism representative requirement, corrupted orbit membership, public/presentation map mismatch, false pending/final status, forbidden-pair omission, occupancy error, re-ingestion circularity, and matched producer report/digest corruption.

## 11. Collision-safe corpus implementation

### 11.1 Record path and identity

Each record has stable `record_id`, semantic ID, canonical content, predicate/equivalence metadata, and production digest. The path is a lookup bucket, not identity:

```text
records/<digest-prefix>/<full-digest>/<record-id>.qcr
```

Validate record IDs, normalized paths, duplicate content, duplicate semantic IDs, and full canonical content within each digest bucket.

### 11.2 Immutable generation update

`corpus_tool add/migrate` must:

1. read and validate the current pointer and generation;
2. build a private complete proposed generation;
3. decode/re-encode every changed/new record;
4. replay required records and validate expected results;
5. perform digest-bucket full-content dedupe;
6. build sorted index, traceability, tombstones/migrations, and generation manifest;
7. validate every referenced file and digest;
8. rename/publish the complete generation; and
9. update `CURRENT.qcp` last.

Never mutate the authoritative generation in place. A failed operation leaves the previous current pointer unchanged.

### 11.3 Recovery

On startup, validate `CURRENT.qcp`. Unreferenced staging/incomplete generations are ignored and may be removed deterministically only after confirming they are not current/referenced. A complete unreferenced generation may be reported for developer recovery but must not become current implicitly from timestamp or directory order.

### 11.4 Collision tests

Use an injected test digest provider to create at least two unequal records with one digest. Require both stored, exact duplicate deduped by full content, distinct operation/tolerance/predicate records retained, deterministic lookup/replay/migration, generation digest stability, and no loss after interruption/recovery.

Production qualification continues to use Component 01 SHA-256; collision injection is test-only.

## 12. Outer/inner concurrency and resource isolation

### 12.1 Scheduler adapter

Add `QualificationSchedulerAdapter` as the test-only adapter anticipated by Plan 17. It defines immutable case descriptors, private case outputs, canonical case rank, exact worker/resource reservation, cancellation checkpoints, merge semantics, and executable serial equivalence.

Do not use `YgorThreadPool`, `taskqueue`, ad hoc `std::async`, detached threads, or a second unbounded pool.

### 12.2 Nested policy

V1 supports:

- `outer_parallel_inner_serial`; and
- `outer_serial_inner_parallel`.

A future `explicit_nested_budget` requires a separately versioned proof and qualification.

For release manifests:

- ordinary independent case batches may use outer parallelism only with inner engine worker count fixed to one;
- Component 17 concurrency/schedule/cancellation cases execute outer-serial and vary inner workers; and
- other nested modes are rejected before admission.

### 12.3 Resource semantics

Each case receives a private Component 01 resource ledger and hard limits unaffected by other admitted cases. The outer scheduler has separate global admission resources.

If global admission cannot reserve a complete case, delay admission or return qualification infrastructure incompleteness before the case begins. Do not silently reduce case limits and reinterpret resulting `resource_limit` as engine behavior.

Profile-bound outer memory/worker counters are distinct from per-case semantic resource outcomes. Every exit joins both outer and inner work before rollback or evidence publication.

### 12.4 Tests

Cover one/maximum outer workers with inner serial, every inner worker count with outer serial, prohibited nested mode, worker/memory boundary cases, cancellation before admission/during inner work/during outer merge, simultaneous failures, cross-case pressure, completion-order invariance, and join/resource reconciliation.

## 13. Non-recursive bootstrap and self-audit

### 13.1 Bootstrap goldens

Commit hand-reviewed golden vectors for canonical framing/malformed lengths, SHA-256/domain separation, checked arithmetic, registry ordering, primitive result encoding, effective-contract precedence, evidence domains, and failure ordering.

Bootstrap tests do not depend on a previously generated release evidence set.

### 13.2 Independent self-auditor

Place `QualificationSelfAuditor` in separate translation units with narrow read-only primitive-record inputs. It reconstructs:

- exact case inventory and missing/duplicate results;
- clause and matrix coverage;
- mutation dispositions/survival;
- corpus/replay results;
- profile-cell ownership and semantic cross-profile equality;
- structural gates;
- ordered failures;
- semantic/profile/release digests; and
- final decision.

Do not call producer summary builders. Share only canonical readers, closed enums, total keys, primitive record types, and Component 01 low-level services.

### 13.3 Self-audit mutations

Add mutations for summary count only, summary plus matching summary digest, omitted primitive result with repaired count, duplicated result under another index, stale clause map, changed profile-cell ownership, hidden mutation survivor, hidden corpus mismatch, semantic/profile domain swap, and forced aggregate pass.

Every mutation must be rejected by primitive reconstruction or full-content comparison.

## 14. Manifests, matrices, and focused tests

The release manifest explicitly partitions cells by profile and states:

- which type/policy/platform cells each profile executes;
- which semantic cases must agree across profiles;
- which sanitizer/race cells are profile-specific;
- which structural counters are semantic or profile-bound; and
- stable not-applicable rationales.

Authoritative campaigns use exact iteration/work/case counts. Time-based stopping remains non-authoritative. Aggregation rejects fragments with different case counts, matrices, or hard resource ceilings from the assigned release partition.

Add focused CTest entries:

```text
MeshBooleanQualification.EffectiveContract
MeshBooleanQualification.BuildProvenance
MeshBooleanQualification.EvidenceDomains
MeshBooleanQualification.ProfileFragment
MeshBooleanQualification.ReleaseAggregation
MeshBooleanQualification.IndependentFloatImport
MeshBooleanQualification.OracleIndependence
MeshBooleanQualification.Component15Independent
MeshBooleanQualification.CorpusCollision
MeshBooleanQualification.CorpusGenerationRecovery
MeshBooleanQualification.OuterInnerConcurrency
MeshBooleanQualification.SelfAuditMutations
MeshBooleanQualification.BuildNoNetwork
```

Apply the strict target helper to every authoritative binary, including corpus recovery and aggregation tools, so canonical encodings use one qualified build discipline.

## 15. Corrected implementation sequence

Amend original Section 26 as follows:

1. Add top-level CTest/`BUILD_TESTING` integration and bounded no-network build audit.
2. Add reviewed versions, closed enums, digest domains, lifecycle phase, profile IDs, and new subcodes/resource kinds.
3. Implement explicit registry/private context/serial harness and primitive result records.
4. Implement effective-contract manifest, precedence resolver, effective clause inventory, and separate implementation-obligation inventory.
5. Add implementation/build manifest generation, strict-profile capture, freshness graph, and mutation tests.
6. Split semantic case results, profile fragments, release evidence sets, codecs, and digests.
7. Implement profile fragment production and independent self-audit from primitive records.
8. Implement release fragment aggregation and closed profile-cell partition validation.
9. Promote and complete exact integer/rational services while preserving Component 03 behavior.
10. Implement independent float import and oracle-independence manifests/tests.
11. Add analytic fixtures, independent validation, generators, presentation variants, invalid mutators, and artifact mutators.
12. Update Component 12-15 adapters for reviewed truth/dimension/role/correspondence/digest contracts.
13. Implement the independent Component 15 qualification oracle and circularity detection.
14. Add deterministic campaigns, pre-run serialization, failure retention, predicates, and shrinking.
15. Implement collision-safe corpus records, immutable generations, current pointer, migration, recovery, and collision tests.
16. Add semantic/presentation-aware replay for every permanent record.
17. Add resource/allocation/exception/cancellation matrices.
18. Integrate Component 17 scheduler controls, outer/inner policy, and global admission resources while retaining the serial reference.
19. Add structural counters, performance fixtures, envelopes, and gates with field classification.
20. Complete evidence aggregation, dry-run mutations, and self-audit matched mutations.
21. Register every effective Component 01-17 clause and implementation obligation; resolve all missing/stale mappings.
22. Execute bootstrap and integration manifests for all four type pairs.
23. After Component 17 implementation, regenerate effective contracts/manifests and execute every concurrency/performance cell.
24. Produce one current fragment for every required platform profile.
25. Aggregate fragments and execute all release-set dry-run defects.
26. Publish release success only from a complete validated aggregate release evidence set.
27. Update `tracker.md` for this review after the amendment is committed; implementation completion remains governed by the corrected definition of done.

No earlier evidence may be relabeled or copied into a later lifecycle/profile without satisfying freshness and exact cell ownership.

## 16. Corrected implementation definition of done

Component 16 implementation is complete only when the original Section 27 and all of the following are true:

- `BUILD_TESTING`/CTest integration is explicit, bounded, dependency-free, and does not alter unrelated tests;
- the effective contract resolver models every base specification and controlling review amendment, with no unresolved conflict or superseded clause incorrectly required;
- implementation-plan obligations are tracked separately from public normative clauses;
- release closure cannot run or pass before reviewed Component 17 is present and fully covered;
- source/configuration/strict-build provenance participates in evidence freshness and no arbitrary build label substitutes for it;
- semantic case results, profile fragments, and release evidence sets have separate schemas, codecs, equality rules, and digest domains;
- every required platform/profile cell is owned by exactly one current fragment and aggregation validates a common implementation/specification/corpus/matrix/provider set;
- cross-profile comparisons use semantic records rather than impossible full-fragment byte equality;
- exact-float decoding and geometric oracle branching are independently implemented and mutation-tested against Component 03;
- Component 15 qualification includes independent topology, occupancy, exhaustive-pair, cleanup/precision/report, re-ingestion, mutation, automorphism, and digest-domain evidence;
- corpus storage retains unequal forced digest collisions and publishes only complete immutable generations;
- interrupted corpus updates recover deterministically without a partial referenced generation;
- outer and inner concurrency are jointly pre-bounded, use the Component 17 test adapter, and cannot change per-case semantic outcomes through cross-case contention;
- the evidence auditor reconstructs every summary/decision from primitive records and rejects matched producer-summary mutations;
- all new focused suites, malformed inputs, boundary matrices, replay cases, provider-replacement slices, and release dry runs pass; and
- the final release decision is emitted only by a complete current aggregate evidence set with zero required mutation survivors, no missing/stale/skipped-required cells, and no external dependency or network requirement.
