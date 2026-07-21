# Component 16 Review Amendment: Effective-Contract Qualification and Release Evidence

## Status, precedence, and review conclusion

This file is a normative amendment to `component_16_test_infrastructure_qualification.md` produced by the independent Component 16 review required by `tracker.md`.

The original specification remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, corpus maintainers, release engineers, Component 15, and Component 17 must read both files together with `broad_plan.md` and every applicable reviewed component amendment.

The review found that the original Component 16 architecture is fundamentally aligned with the broad plan and should be retained. Qualification remains a permanent dependency-free C++17 subsystem with exact low-complexity test oracles, valid-manifold generation, invalid and producer-shaped mutation, deterministic campaigns and shrinking, permanent regression retention, replay, closed matrices, structural performance gates, transactional evidence, independent audit, and fail-closed release qualification.

The review identified eleven mandatory corrections:

1. Qualification must operate on an explicit **effective normative contract graph** that models base specifications, review amendments, precedence, and superseded clauses. Scanning all Markdown as an undifferentiated set would require tests for obsolete conflicting text.
2. Component 16 has a staged lifecycle: its harness and oracles are built alongside earlier components, but final release qualification occurs only after Component 17 is implemented and qualified, as required by the broad plan's implementation order.
3. Cross-platform semantic evidence must be separated from platform/build-bound evidence. Full evidence bytes cannot both contain platform/build identity and be byte-identical across platforms.
4. A release covering several supported platform profiles requires a canonical in-tree aggregation contract over independently produced profile evidence fragments. One process may not claim cells it did not execute.
5. Evidence freshness must be bound to an explicit implementation/build provenance manifest, not merely a caller-supplied build label or provider version.
6. The exact floating-point import and geometric oracle paths must remain independent of Component 03 production decoding and branch selection; raw bit copying may be shared, semantic decoding may not.
7. An ordinary public success already implies a Component 15 pass. Component 16 must not count rerunning the same verifier as independent proof of Component 15; it must use independently organized topology, exact-oracle, exhaustive-pair, mutation, and replay checks.
8. Corpus storage must support deliberate digest collisions. A single path named only by a digest cannot store two different records in one collision bucket.
9. Harness-level case concurrency and in-case Component 17 concurrency are separate axes and require one bounded ownership/resource policy. Nested unbounded pools or cross-case resource interference are prohibited.
10. Component 16 must consume reviewed Component 12-15 contracts, including numerical truth layers, physical dimensions, cleanup-cost roles, semantic/presentation digest domains, automorphism-aware correspondence, and pending/final publication authority.
11. The existing root CMake configuration does not currently establish CTest or `BUILD_TESTING`; Component 16 may reuse the planned strict target and local harness, but the bounded test tree requires an explicit dependency-free CTest integration hook.

No correction authorizes weaker qualification, source-order dependence, digest-only equality, hidden network use, external dependencies, runtime production dependence on test oracles, arbitrary platform claims, or circular self-certification.

## A. Effective normative contract graph

### A.1 Explicit source set and precedence

The traceability input must be an immutable `effective_contract_manifest` containing an explicit ordered list of:

- `broad_plan.md`;
- each base `component_01_*.md` through `component_17_*.md` specification;
- every normative component review amendment;
- amendment-to-base and amendment-to-amendment precedence edges;
- specification, version, and content digests;
- stable clause identifiers and clause kinds;
- explicit supersession or narrowing relationships; and
- reviewed non-executable rationale records.

The implementation must not recursively glob specification directories or infer precedence from filenames, timestamps, Git history, line order, or whichever file is scanned last.

A review amendment that states it controls on conflict must be represented as such in the manifest. A base clause may remain historically addressable for replay and migration, but an explicitly superseded clause must not remain an independently required current release obligation.

### A.2 Effective clause states

Every clause record must have one closed state:

- `effective_required`;
- `effective_non_executable` with a stable reviewed rationale;
- `historical_superseded` with one or more controlling clause identifiers;
- `compatibility_only` for old replay/provider support; or
- `invalid`.

Release qualification executes all `effective_required` clauses and rejects:

- unresolved contradictory effective clauses;
- a superseded clause treated as current without an explicit compatibility reason;
- an amendment whose base or predecessor digest is missing or wrong;
- an override cycle;
- an unknown precedence rule;
- a clause identifier reused for different normalized content without an explicit compatible revision; or
- an implementation/test manifest bound to another effective-contract digest.

### A.3 Plans versus specifications

Implementation plans and plan review amendments are implementation handoffs and planning gates. They may have a separate stable implementation-obligation inventory, but they must not silently become public contract clauses merely because they contain prescriptive wording.

The inventory must distinguish broad-plan requirements, effective component-specification requirements, implementation-plan acceptance obligations, test-infrastructure self-contracts, and historical/compatibility obligations.

### A.4 Annotation migration

Adding stable annotations to existing documents must be semantics-preserving. The migration must assign IDs to logical obligations rather than rendered lines, preserve amendment precedence, record old/new document digests, reject duplicate or orphan annotations, avoid renderer/locale dependence, and regenerate the complete effective inventory transactionally.

## B. Lifecycle and implementation order

Component 16 has three explicit phases:

1. **bootstrap qualification infrastructure** — serial harness, exact arithmetic, exact float import, codec tests, fixture infrastructure, and component-local registration needed while Components 01-15 are implemented;
2. **integration qualification infrastructure** — cross-component adapters, mutation, deterministic campaigns, shrinking, corpus, replay, and evidence construction; and
3. **release qualification closure** — final Component 17 concurrency/performance cells, all supported profile fragments, complete effective-contract coverage, and the aggregate release decision.

Component and integration scopes may run before Component 17 exists only when their manifest explicitly selects serial execution and makes no claim of Component 17 coverage.

Release scope is unavailable until the reviewed Component 17 contract is present; its serial references, adapters, forced schedules, worker qualification, counters, and structural gates are implemented; the effective-contract manifest contains its final clauses; and all required Component 17 cells have current evidence.

Marking Component 16 complete in `tracker.md` records completion of this planning review only. It does not mean implementation or release qualification is complete.

## C. Existing Ygor and build integration

The original existing-code assessment remains correct:

- `tests/compile.sh` is an ad hoc direct-compilation script and is not a normative manifest-driven harness;
- `tests2/compile_and_run.sh` downloads doctest from the network and cannot be used by this subsystem;
- existing sample meshes are fixture seeds only and are not expected-result oracles; and
- unrelated existing tests must remain intact.

The current root build does not establish CTest or `BUILD_TESTING`. The bounded subsystem must add a narrow top-level integration equivalent to `include(CTest)`, conditionally add `tests/mesh_boolean_bounded/`, retain `ygor_apply_mesh_boolean_strict_fp`, perform no configure/build/test network access, and preserve unrelated default behavior when bounded qualification is disabled.

CTest is only an invocation and discovery layer. Component 16 remains authoritative for stable test identity, ordering, required/optional state, completeness, canonical result encoding, evidence, and release status.

## D. Implementation provenance and freshness

A release profile must consume an immutable `implementation_build_manifest` containing at least:

- source revision or an explicit canonical source-tree digest over an ordered source list;
- effective-contract digest;
- build-system input digests relevant to bounded production and qualification targets;
- compiler identity/version and qualified platform profile;
- normalized authoritative compile/link options, including strict-floating and IPO/LTO disposition;
- all provider, policy, schema, codec, digest, replay, counter, and adapter versions;
- generated-table and committed-data digests;
- enabled bounded feature/test options; and
- a developer-supplied display build label as non-authoritative metadata.

A free-form build label must never substitute for source/configuration provenance. Evidence is stale when any transitive authoritative input changes. Provider replacement must rerun the exact affected qualification slice derived from explicit dependencies.

Release qualification must mutate and reject at least source digest, effective-contract digest, strict floating flags, provider version, generated data, corpus generation, matrix, profile, and build-option provenance.

## E. Evidence domains and multi-profile aggregation

### E.1 Three distinct artifacts

Component 16 must separate:

1. `semantic_case_result` — platform-independent logical input identity, expected relation, authoritative success/failure semantics, public semantic content, exact-oracle result where applicable, and semantic structural counters;
2. `qualification_profile_fragment` — one actually executed build/platform profile, complete primitive results, profile-specific environment evidence, sanitizer/race results, build manifest, resource observations, and profile digest; and
3. `release_qualification_evidence_set` — canonical aggregation of a closed required set of profile fragments and the release decision.

These use distinct schemas, canonical domains, digests, and equality rules.

### E.2 Cross-platform comparison

Cross-platform invariance is stated over `semantic_case_result` or another explicitly declared semantic projection. Profile fragments are expected to differ where compiler/platform/build/environment evidence differs. Tests must not require impossible full-fragment byte equality across unlike profiles.

The release manifest must classify each field/counter as semantic, profile-bound, advisory non-authoritative, or prohibited from release bytes.

### E.3 Profile fragments

A profile fragment may claim only cells it actually executed under its recorded build/profile. It must not claim another compiler, operating system, architecture, sanitizer, floating environment, or worker profile by inference.

Each fragment must independently pass registry/manifests, strict-build/platform qualification, complete assigned test cells, primitive-result self-audit, resources/cancellation, replay/corpus checks, and its assigned performance/race cells.

### E.4 Aggregate release set

The in-tree aggregator accepts validated fragments and requires:

- one common implementation-source identity;
- one effective-contract digest;
- one corpus generation and matrix set;
- compatible provider/policy/schema versions;
- exact ownership of every required profile cell;
- no missing, duplicate, overlapping, stale, foreign, or contradictory cell;
- required semantic equality across profiles; and
- deterministic canonical ordering and aggregate digest.

Aggregation is transactional and network-free. One process/profile cannot synthesize evidence for another profile. The aggregate fails closed when a required profile fragment is unavailable.

## F. Oracle independence

### F.1 Exact binary-float import

The Component 16 exact importer must decode `float` and `double` bit fields directly from fixed-width unsigned values obtained by a reviewed `memcpy` bit copy. It must independently implement sign, exponent, significand, normal/subnormal, signed-zero, maximum finite, and rejection semantics.

It must not call Component 03's production float decoder, exact-expansion adapter, predicate branch selector, projection selector, or bounded relation producer to construct expected answers.

Required tests include exhaustive reduced-format model floats, boundary bit patterns, independent reconstruct-to-bits checks, and mutations in both production and oracle decoders that the other side detects.

### F.2 Geometric oracle independence

The exact geometry oracle must choose projections, relation categories, ray ownership, and degeneracy branches through its own reviewed rational logic. Shared low-level canonical bytes, resource accounting, strong IDs, and immutable source bits are permitted; shared high-level decision control flow is not.

Each differential test must record an `oracle_independence_manifest` naming production helpers, oracle helpers, permitted shared primitives, and the independent branch/witness used.

## G. Reviewed Component 12-15 integration

Component 16 adapters, fixtures, corpora, mutations, equivalence policies, and evidence must consume the effective reviewed contracts for Components 12-15. At minimum they must understand:

- `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` as separate truth layers;
- closed metrics, physical dimensions, derivations, denominator certificates, and quantity/cost roles;
- advisory/proposed/reserved/committed/rejected/rolled-back cleanup distinctions;
- Component 14 semantic public canonicalization separate from presentation correspondence;
- correspondence equivalence classes and exact automorphism/orbit certificates;
- semantic public-content digests separate from source-bound presentation/replay digests; and
- Component 15's sole authority for `geometry.status == tolerance_checked` and ordinary publication.

A corpus expectation or metamorphic comparison must state whether it compares semantic public content, a permitted correspondence class, presentation-bound replay, or a typed failure. It must not require arbitrary representative equality inside a valid automorphism class.

An end-to-end ordinary success proves only that the production publication path reports a Component 15 pass. Independent Component 16 evidence for Component 15 additionally requires, as applicable:

- exact-index public topology reconstruction outside Component 15's high-level control flow;
- exact or independently bounded low-complexity occupancy and side checks;
- exhaustive bounded output triangle-pair checking against Component 15's spatial search;
- primitive reconstruction of cleanup, precision, and report aggregates;
- reviewed Component 02 re-ingestion comparison;
- producer-shaped mutations with repaired superficial counts/digests;
- semantic/presentation digest-domain mutations;
- automorphism/correspondence mutations; and
- focused replay compatibility and deterministic finding tests.

Rerunning Component 15 may test idempotence, replay compatibility, and deterministic diagnostics, but it is not a second independent verifier of the same behavior.

## H. Collision-safe corpus and transactions

A permanent corpus record has a stable semantic record identifier, canonical full logical content, failure/equivalence semantics, lookup/integrity digests, and full-content equality as final authority.

Storage must permit several different records with the same digest. A path consisting only of `<full-digest>.qcr` is insufficient. A conforming layout may use:

```text
corpus/generations/<generation-id>/records/<digest-prefix>/<full-digest>/<record-id>.qcr
```

or an equivalently canonical bucket container. On every digest match, compare complete canonical content and predicate/equivalence metadata. Collision-injection tests must store, index, replay, migrate, and retrieve at least two unequal records in one forced digest bucket.

Corpus publication must use immutable complete generations:

1. construct a proposed generation privately;
2. validate every record and collision bucket;
3. replay required records;
4. build the sorted index, traceability map, and generation manifest;
5. compute full digests;
6. publish the complete generation by same-filesystem transactional rename or equivalent reviewed mechanism; and
7. update one authoritative current-generation pointer last.

Readers use only the validated current-generation pointer. A failure leaves the previous generation authoritative. A crash may leave an unreferenced complete generation, but never a partially referenced generation or an index referring to absent records. Recovery and cleanup of unreferenced generations must be deterministic and tested.

Deleting or migrating a confirmed regression requires a reviewed tombstone/migration record. Digest collision, schema migration, or provider retirement never silently drops history.

## I. Outer and inner concurrency

The serial case runner remains the semantic reference. The qualification system must distinguish:

- **outer case concurrency** — independent test cases executed concurrently for throughput; and
- **inner engine concurrency** — Component 17 workers exercised inside one case as the subject under test.

Outer concurrency is optional and may use only the Component 17 test scheduler adapter or another explicitly reviewed bounded test-only adapter with equivalent ownership, join, resource, cancellation, and canonical-merge guarantees. Component 16 must not create an unrelated unbounded pool.

The release manifest must define one bounded global concurrency budget and nested policy. V1 must use one deterministic mode per case group:

- outer cases may run concurrently only when each case forces the engine serial;
- concurrency-qualification cases run one at a time while their inner Component 17 worker matrix is exercised; or
- a separately versioned nested profile pre-reserves all outer and inner workers, queues, private outputs, and memory and proves progress and deterministic results.

Cross-case memory pressure, worker availability, or completion order must not change an authoritative case from success to `resource_limit`, alter its primary failure, or change semantic counters. Per-case hard resource ledgers are private; aggregate runner limits may stop admission before a case begins but must report infrastructure incompleteness rather than reinterpret the case.

Every exit path joins outer and inner work before releasing artifacts or publishing evidence.

## J. Finite self-audit trust boundary

Component 16 must define a finite bootstrap trust boundary containing the explicit registry/manifest parser, Component 01 checked arithmetic/canonical framing/SHA-256 imported services, hand-authored codec/hash/arithmetic golden vectors, primitive case result records, and an independently organized evidence auditor.

The evidence producer and auditor must not share summary-construction control flow. The auditor reconstructs clause coverage, matrix completeness, mutation survival, corpus/replay status, profile completeness, structural gates, failure order, aggregate digests, and the release decision from primitive records.

The release decision is not recursively qualified by invoking the complete release decision on itself. Self-tests and dry-run mutations establish the bootstrap services and auditor, and their primitive results become mandatory cells in the enclosing evidence set.

A mutated producer summary with unchanged primitive records must be rejected. A matched mutation of producer summary and one digest must also be rejected through independent reconstruction and full-content checks.

## K. Additional required tests

In addition to the original test specification, Component 16 must include:

- base/amendment precedence, supersession, conflict, override-cycle, and stale-effective-contract tests;
- tests proving implementation plans do not silently become public contract clauses;
- CTest activation and no-network/prohibited-package build-audit tests;
- implementation-provenance source/configuration/strict-flag freshness mutations;
- semantic-record versus profile-fragment versus release-set digest-domain tests;
- cross-profile tests requiring equal semantic records but different profile evidence bytes;
- missing, duplicate, stale, foreign, overlapping, and contradictory fragment aggregation tests;
- independent exact-float decoder mutations and exhaustive reduced-format model tests;
- Component 15 circularity tests rejecting the production pass counted twice;
- reviewed truth/dimension/cleanup-role/correspondence/automorphism/digest-domain corpus and mutation tests;
- forced digest-bucket collisions containing unequal records;
- immutable corpus-generation interruption and recovery tests;
- outer/inner concurrency budget, oversubscription, nested fallback, cancellation, and cross-case interference tests; and
- evidence-producer/evidence-auditor matched-mutation tests.

## L. Corrected definition of done

Component 16 implementation is complete only when the original definition of done and all of the following are satisfied:

- the effective normative contract graph includes every controlling review amendment and excludes superseded conflicts from current release obligations;
- bootstrap, integration, and release-closure phases are explicit, and release closure waits for Component 17;
- root bounded-test integration establishes CTest/`BUILD_TESTING` without migrating unrelated tests or introducing network/dependency behavior;
- implementation provenance, effective contract, corpus, matrices, providers, build profile, and strict floating configuration all participate in freshness;
- semantic case records, profile fragments, and aggregate release evidence use distinct canonical domains;
- no profile fragment claims unexecuted platform cells and the aggregate release set is complete and internally consistent;
- exact float import and geometric expected answers are independent from Component 03 production decoding/control flow;
- Component 15 has independent qualification evidence beyond the ordinary publication pass;
- corpora store and replay unequal records in deliberately colliding digest buckets;
- corpus publication uses complete immutable generations with deterministic recovery;
- outer and inner concurrency are jointly bounded and cannot cause cross-case semantic interference;
- evidence self-audit reconstructs from primitive records without recursive self-certification; and
- all additional mutation, malformed-input, replay, resource, cancellation, and deterministic tests in this amendment pass with zero required survivors.
