# Plan 16: Production Qualification and Release Gates

## 0. Purpose and status

This plan completes and replaces any informal interpretation of the unfinished P13 qualification workstream. It defines the evidence required before any mesh Boolean backend or result mode is advertised as production-ready for a downstream CAD application.

The qualification program is independent of implementation completion. Passing unit tests, component verifiers, sanitizers, or a small regression corpus is necessary but not sufficient. The principal release risk is a plausible-looking false success, not merely a crash or typed failure.

This plan applies to:

- `experimental_exact_v1`;
- every optional external backend adapter;
- strict validation and each normalization policy;
- exact stratified output;
- `exact_in_T` mesh output;
- certified approximate mesh output;
- attribute/provenance transfer; and
- the one-call product service.

## 1. Maturity states

Every backend/result-mode pair has one explicit maturity state:

- `experimental`: available only by explicit opt-in; incomplete evidence is allowed and documented.
- `candidate`: full functionality is present and the qualification campaign is running; not a production default.
- `qualified`: all mandatory gates for a declared workload profile pass and a frozen report is published.
- `deprecated`: retained only for replay or migration.

Qualification is scoped. A backend may be qualified for exact rational polyhedra but not unknown-provenance tessellations, or for strict validation but not normalization. The manifest must not generalize beyond the tested profile.

## 2. Qualification manifest

Add a versioned canonical manifest describing:

- backend and adapter identifiers, versions, commit/build IDs, and capability digests;
- result modes under test;
- preparation/normalization policies and tolerance units;
- coordinate/index specializations;
- compiler, standard library, architecture, operating system, and floating-point mode;
- verifier versions;
- corpus IDs and licenses/provenance classes;
- generator versions, seeds, and parameter ranges;
- fuzz engines, dictionaries, mutators, durations, and worker counts;
- operation-chain definitions;
- resource limits and cancellation policy;
- performance hardware and measurement protocol;
- expected failure taxonomy;
- release thresholds; and
- all exclusions or known limitations.

The manifest is hashed and bound into qualification reports and qualified-default backend selection. Changing a capability, policy, compiler mode, verifier, or corpus invalidates the corresponding qualification claim until rerun or explicitly reviewed under a compatibility rule.

## 3. Corpus architecture

### 3.1 Permanent deterministic corpus

Maintain a permanent corpus with compact canonical inputs or generator recipes. The initial production campaign must include at least:

- 10,000 generated exact or construction-known operand pairs;
- 1,000 representative licensed or internally generated CAD-like tessellated operand pairs;
- 1,000 deterministic operation chains, each containing at least five successful or intentionally failing operations; and
- every minimized regression ever discovered by unit, property, fuzz, differential, sanitizer, or customer testing.

Large cases may be stored through content-addressed external artifacts, but the repository must contain a manifest, digest, retrieval procedure, license/provenance classification, and enough compact cases to exercise CI without external availability.

### 3.2 Geometry categories

The corpus must cover, in controlled combinations:

- disjoint, containment, equality, and near-contact;
- rotated and skewed convex solids;
- concave solids and re-entrant features;
- multiple disconnected components;
- nested shells and multiple cavities;
- partial and full coplanar overlap with same and opposite orientation;
- vertex-, edge-, and face-contact at low and high valence;
- repeated cuts and many events on one carrier;
- long thin features, slivers, severe aspect ratios, and dense tessellation;
- alternate triangulations and polygon subdivisions of the same exact boundary;
- radically different operand scales and coordinate exponents;
- signed zero, subnormals, adjacent floats, and cancellation-heavy constructions;
- exact intersections with non-dyadic coordinates;
- valid stratified non-manifold results;
- index-capacity boundaries and resource-limit behavior;
- attributes, seams, and provenance conflicts; and
- normalization defects such as cracks, duplicate vertices/facets, inconsistent orientation, non-planarity, and self-intersection.

Every applicable Boolean operation and operand order must appear for every category. Difference tests must cover both `A-B` and `B-A` rather than infer one from the other.

### 3.3 Generated exact cases

Use topology-preserving generators whose source geometry and expected relationships are independently known. Preferred families include:

- intersections of rational halfspaces with exact polytope construction;
- constructive solid sequences with exact transformation matrices where representable;
- extrusion and revolution-like polyhedral approximations generated from exact planar profiles;
- controlled coplanar overlays;
- nested shell/cavity generators;
- exact subdivision/refinement variants; and
- adversarial feature-alignment generators.

Store the construction recipe, not only the emitted mesh. The recipe provides an independent source of expected occupancy, volume, or incidence facts.

### 3.4 Real-world and CAD-like cases

Real-world cases must represent the downstream workload rather than generic downloadable meshes. Record:

- source system or generator class;
- intended model tolerance;
- whether the input was exported from analytic CAD, hand-edited, scan-derived, or synthetic;
- preparation actions and reports;
- licensing and redistribution constraints; and
- expected product outcome.

Private customer cases may remain outside the repository, but their anonymized category counts, digests, outcomes, and failure summaries belong in the qualification report.

## 4. Oracle hierarchy

No single oracle is sufficient. Use a hierarchy of independent evidence.

### 4.1 Exact low-complexity oracles

For bounded cases, use deliberately slow independent implementations for:

- exhaustive feature pairing;
- exact point/solid classification;
- exact planar overlay;
- selected-boundary occupancy checks;
- topology occurrence and spherical-link reconstruction;
- exact oriented volume and Euler/shell invariants; and
- brute-force realization assignments.

These oracles may share primitive exact number types but must not call producer grouping, ordering, stitching, triangulation, obligation-generation, or canonicalization helpers.

### 4.2 Analytic/construction-known checks

Where construction recipes are known, check:

- expected containment and contact relations;
- exact or symbolic volumes;
- cross-section occupancy;
- known component and cavity counts;
- operation identities; and
- exact probe classifications away from the boundary.

### 4.3 Independent backend comparison

Run at least one independently implemented mature backend for the candidate workload profile. Comparison is diagnostic evidence, not a proof and not an automatic majority vote.

For each disagreement:

1. preserve all inputs, options, backend versions, and outputs;
2. independently classify exact or guarded probes;
3. compare topology and occupancy, not only triangle counts or approximate volume;
4. minimize the case while preserving disagreement;
5. classify each backend outcome as correct, incorrect, unsupported, policy-different, or unresolved; and
6. block production qualification while any material disagreement remains unexplained.

A backend that returns a result under different regularization, tolerance, or non-manifold semantics must be compared only after the semantic difference is made explicit.

### 4.4 Mesh realization checks

For every mesh success:

- re-ingest the output through strict Component 2 validation;
- reconstruct topology independently;
- bind it to the exact result digest;
- classify guarded probes on both sides of output facets;
- check for introduced non-adjacent intersections;
- verify orientation and shell nesting;
- replay the realization certificate from output bits alone; and
- use the output in subsequent chain operations.

For approximate mode, additionally verify all displacement and relaxed-relation bounds. Approximate success is never accepted by comparing within an unrecorded test epsilon.

## 5. Outcome taxonomy and false-success accounting

Every test produces one normalized outcome:

- verified exact success;
- verified certified-approximate success;
- expected typed failure;
- unexpected typed failure;
- backend disagreement;
- verifier disagreement;
- false success;
- nondeterministic outcome;
- timeout/resource-limit outcome; or
- infrastructure failure.

A **false success** is any published success whose set occupancy, topology, orientation, embedding, certificate binding, attribute mapping, or declared representation semantics is incorrect. False success is the highest-severity defect.

Qualification reports must include counts and rates by backend, mode, preparation policy, geometry category, operation, specialization, and platform. Do not collapse safe failures and false successes into one success-rate number.

## 6. Fuzzing and shrinking

### 6.1 Valid-geometry fuzzing

Fuzz valid geometry using construction-aware mutators:

- move features into exact contact;
- create and remove coplanarity;
- vary triangulation without changing the solid;
- split/merge source facets exactly;
- add cavities and disconnected shells;
- alter coordinates by one ULP;
- apply representable rigid transforms and power-of-two scales;
- increase event valence; and
- compose successful outputs into longer operation chains.

The current disjoint-box generator is retained only as a smoke test, not as qualification evidence.

### 6.2 Invalid/preparation fuzzing

Generate malformed meshes with controlled defects and require precise, deterministic validation or normalization behavior. Verify that normalization reports every edit and that the prepared result passes strict validation.

### 6.3 Duration and preservation

For a production qualification campaign, run at least:

- 24 aggregate CPU-hours per sanitizer/configuration for valid-geometry fuzzing;
- 24 aggregate CPU-hours per sanitizer/configuration for invalid/preparation fuzzing;
- 24 aggregate CPU-hours for operation-chain fuzzing; and
- a scheduled long-running unsanitized campaign sufficient to exercise large cases and exact-number growth.

These are minimum campaign floors, not claims of statistical completeness. Every unique failure/disagreement is serialized, minimized, and added to the permanent corpus before closure.

## 7. Determinism and schedule matrix

For a representative subset and every regression, rerun across:

- one and multiple worker counts;
- alternate task partitions and queue bounds;
- broad-phase construction choices;
- filter enabled, forced fallback, and mixed paths;
- randomized allocation perturbations;
- supported hash seeds where lookup hashes are configurable;
- ambient rounding modes where the platform permits controlled testing; and
- replay in separate processes.

Canonical exact artifacts, result bytes, typed failures, diagnostics, and certificate digests must agree. Noncanonical timing/counter fields are excluded by schema.

## 8. Compiler, sanitizer, and platform matrix

The production matrix includes, where supported by project policy:

- current and oldest-supported GCC;
- current and oldest-supported Clang;
- Debug and optimized builds;
- libstdc++ and libc++ where practical;
- x86-64 and at least one independent 64-bit architecture;
- AddressSanitizer and UndefinedBehaviorSanitizer;
- ThreadSanitizer on concurrency-enabled suites;
- debug iterator/library modes;
- strict floating-point flags with compile-time guards; and
- 32-bit and 64-bit index specializations.

The report records exact compiler and library versions, flags, environment variables, and skipped configurations with reasons. A CI job definition without a committed passing report is not qualification evidence.

## 9. Performance, memory, and cancellation

### 9.1 Workloads

Benchmark representative small, medium, large, and adversarial workloads for:

- strict validation;
- normalization diagnosis and each repair class;
- backend exact evaluation;
- topology preflight;
- exact-result serialization;
- exact-in-`T` realization;
- certified approximate realization;
- mandatory verification;
- exhaustive verification; and
- operation chains.

### 9.2 Measurements

Record:

- wall and CPU time;
- peak resident memory and authoritative-accounted bytes separately;
- exact-number peak limbs/bits;
- candidate/event/patch growth;
- verifier overhead;
- realization search nodes/components;
- serialization size;
- cancellation latency; and
- success/failure category.

Do not claim a hard RSS bound from allocator accounting. Resource limits must fail transactionally without partial publication.

### 9.3 Regression policy

Freeze baseline hardware and a statistical measurement protocol. Performance regressions beyond the approved threshold require review, but performance pressure may not weaken exactness, verification, or failure semantics.

## 10. Operation-chain qualification

CAD usage is compositional. Add deterministic chains that:

- alternate union, intersection, and both difference directions;
- re-ingest every successful mesh output;
- retain exact results when mesh realization fails;
- vary operand order and tree association;
- apply legal subdivisions between steps;
- mix disconnected solids and cavities;
- exercise normalization only at explicitly marked boundaries; and
- compare exact/approximate/provenance behavior across the chain.

Record error propagation and ensure that a failed step cannot leak a partially mutated operand into later steps.

## 11. Attribute and provenance qualification

For every supported attribute policy:

- verify deterministic source-to-output mappings;
- test seams, conflicts, multi-source facets, and removed internal faces;
- prove geometric topology is unchanged by attribute values;
- verify omitted attributes are reported;
- serialize and replay mappings; and
- use downstream-style queries to recover source contributors.

A geometry-only success cannot qualify the product API if advertised attribute behavior is untested.

## 12. Release gates

A backend/result-mode/workload profile may be marked `qualified` only when all applicable gates pass:

1. **Zero false successes.** No known incorrect published result or semantic mislabeling.
2. **Zero unexplained verifier disagreements.** Producer/verifier disagreement is blocking.
3. **Zero unexplained independent-backend disagreements** on material cases.
4. **Zero nondeterministic canonical outcomes** across the required schedule/platform subset.
5. **All permanent regressions pass** in every applicable supported configuration.
6. **All sanitizer and undefined-behavior runs pass.** Any suppression is documented and reviewed.
7. **Corpus floors are met** with category/operation coverage demonstrated by the manifest.
8. **Operation chains pass** with transactional failure and re-ingestion.
9. **Success and typed-failure rates meet product-approved thresholds** for the declared workload. `exact_in_T` is assessed as a strict mode and is not used as the practical-output success target.
10. **Performance, peak memory, exact-number growth, and cancellation latency are measured** and within approved limits.
11. **Normalization and approximate modes have explicit tolerance/displacement evidence.** No hidden epsilon is present.
12. **Attribute/provenance behavior passes its advertised contract.**
13. **A complete reproducible report is committed** and bound to the qualification manifest.
14. **Known limitations and unsupported categories are documented** in the public API.

Qualification is revoked or moved back to `candidate` when a new false success, unexplained disagreement, schema incompatibility, or material platform defect is discovered.

## 13. Qualification report contents

Commit a human-readable report plus machine-readable manifest/result summaries containing:

- executive result and exact scope of the claim;
- repository commit and dirty-state proof;
- complete build commands and dependency versions;
- hardware/OS/compiler/standard-library matrix;
- corpus inventory and category coverage;
- generator seeds and fuzz durations;
- outcome tables and failure taxonomy;
- every disagreement and its resolution;
- sanitizer and determinism results;
- performance/memory/cancellation tables;
- success/failure rates by workload category;
- backend and result-mode promotion decisions;
- known limitations and deferred work; and
- links/digests for replay artifacts.

The report must make it possible for another engineer to reproduce the campaign without relying on unstated local knowledge.

## 14. CI tiers

### Continuous

Run all minimized regressions, unit/component tests, a bounded exact generated corpus, strict determinism checks, and representative end-to-end chains. The continuous tier must contain non-box intersections and at least one case for every major degeneracy family.

### Scheduled extended

Run the broader generated and real-model corpus, sanitizer subsets, multiple compilers, valid/invalid fuzzing, backend comparison, and performance smoke tests.

### Qualification

Run the frozen full manifest on controlled hardware, generate the report, and archive all logs/artifacts. Qualification jobs fail on missing coverage, missing report fields, unexplained outcomes, or unapproved skips.

## 15. Implementation sequence

1. Define manifest, outcome, and report schemas.
2. Expand the permanent corpus beyond boxes and encode category coverage.
3. Implement exact construction-aware generators and operation-chain harnesses.
4. Add independent backend adapters in diagnostic-only mode.
5. Add false-success classification and minimization workflow.
6. Add real/CAD-like corpus ingestion with licensing/provenance controls.
7. Add preparation, approximate realization, attribute, and provenance qualification suites.
8. Freeze performance methodology and cancellation tests.
9. Execute the candidate campaign and resolve every unexplained outcome.
10. Commit the final report and update the qualified-default manifest only after review.

## 16. Definition of done

P13 and production qualification are complete only when:

- the manifest and report schemas are versioned and enforced;
- permanent coverage contains thousands of nontrivial exact and CAD-like cases rather than a box-dominated sample;
- fuzzing exercises valid intersecting geometry, invalid/preparation behavior, and operation chains;
- successful outputs are re-ingested and independently checked for false success;
- at least one independent backend participates in diagnostic comparison;
- all disagreements are preserved, minimized, and resolved or declared blocking;
- sanitizer, compiler, architecture, determinism, and resource matrices have committed outcomes;
- performance and memory data are published for representative and adversarial workloads;
- attribute/provenance and normalization behavior are tested where advertised;
- zero known false successes or unexplained verifier/backend disagreements remain; and
- the repository contains a reproducible qualification report naming exactly which backend/result-mode/workload profiles, if any, are production-qualified.
