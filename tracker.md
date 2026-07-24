# Implementation tracker

## Tracker rules and authoritative order

Work through this file from top to bottom. The implementation order in `plan_15_assessment_amendment.md` is authoritative for productization, and `plan_16_qualification_release.md` completes and replaces the former informal P13 qualification task.

- `[x]` means the implementation, focused tests, independent verification obligations, schema/documentation updates, and applicable regression gates for that stated scope have landed.
- `[ ]` means outstanding work. Do not check a parent item until every nested item and its exit gate pass.
- The completed original engine remains the `experimental_exact_v1` backend until the qualification manifest explicitly promotes a backend/result-mode/workload profile.
- Components 3 through 9 retain their exact semantics. Productization work must wrap or isolate them; it must not weaken predicates, incidence, topology, determinism, transactional publication, or fail-closed behavior.
- No implementer may infer production readiness from the completed baseline section. Production claims are controlled only by the unchecked productization and qualification sections below.

## Completed exact-engine baseline

- [x] Component 1 baseline contracts, immutable IDs, diagnostics, deterministic utilities, transactions, replay, accounting, and cancellation.
- [x] Component 3 exact arithmetic and predicates, alongside the original Component 14 arithmetic and predicate oracles.
- [x] Component 13 original artifact verifiers and Component 2 strict topology validation/canonicalization.
- [x] Component 4 broad phase, tested against exhaustive pair enumeration.
- [x] Components 5 and 6 symbolic events and registry.
- [x] Component 7 local arrangements on synthetic exact constraints and discovered events.
- [x] Component 8 global stitching, topological occurrences, spherical links, and arrangement checks.
- [x] Component 9 independent patch-side classification and Component 10 exact boundary selection/topology classification.
- [x] Component 11 strict `exact_in_T` realization, checked impossibility reporting, component-decomposed solving, and realization certificates for the original result contract.
- [x] Component 12 manifold mesh assembly, canonicalization, and re-ingestion for the original result contract.
- [x] Original Component 14 component-level unit, property, adversarial, metamorphic, mutation, replay, bounded fuzz, benchmark, and plan-gap infrastructure. This check does **not** mean the production corpus or Plan 16 qualification campaign is complete.
- [x] Convert every case in `tests/MeshBooleanPlanGapCases.md` into an executable CI test with an explicit success or typed-failure expectation.
- [x] Resolve the G1-G9 contract and algorithm gaps: topological occurrences, complete spherical links, certified open-side probes, frozen `independent_patch_side_v1`, stratified topology classification, manifold preflight, strict `exact_in_T`, defining-relation replay, realization decomposition, and conservative realization pair generation.
- [x] Retain the touching-cube and one-third fixtures as permanent regressions after the original plan-gap matrix passed.
- [x] Add the original advanced example and end-to-end test demonstrating explicit kernel/verifier setup, strict input validation, error handling, and extraction of a manifold `fv_surface_mesh<double, std::uint64_t>` when available. The product-facing migration example remains outstanding under P5.

## Completed performance acceleration baseline

The P0-P12 items in `plan_speed.md` are complete for the original exact-engine contract. Their canonical artifacts, safety checks, and benchmark baselines remain regression constraints for all later work.

- [x] **Speed P0 — Freeze exact and performance baselines.** Versioned producer/verifier timing, deterministic counters, B0-B8 benchmarks, baseline tests, semantic digests, canonical bytes, structural counters, and controlled Release results.
- [x] **Speed P1.1 — Remove full artifact copies.** Freeze, verify, and publish one allocation with rollback, stale-binding, immutability, and resource tests.
- [x] **Speed P1.2 — Remove encoding from hot comparisons.** Typed structural comparisons/immutable keys with canonical encoding retained at publication and verification boundaries.
- [x] **Speed P1.3 — Add dense lookup indices and batched exact resource accounting.** Checked cardinalities, stale/malformed-handle tests, collision tests, and boundary-limit tests.
- [x] **Speed P2.1-P2.3 — Optimize exact arithmetic.** Differential micro-oracles, inline limb storage, deterministic division/GCD, rational normalization/comparison, and unchanged canonical exact-number serialization.
- [x] **Speed P3.1-P3.2 — Add certified predicate filters and division-free incidence paths.** Strict uncertain-to-exact fallback and filter-on/off artifact equivalence.
- [x] **Speed P4.1-P4.3 — Accelerate validation/canonicalization and reuse immutable facet geometry.** Linear ring/plane discovery, conservative self broad phases, incremental triangulation, and scalable graph canonicalization.
- [x] **Speed P5 — Optimize Component 4 producer and independent verifier broad phases** without false negatives or verifier-family coupling.
- [x] **Speed P6.1-P6.2 — Index event discovery and symbolic interning** with exhaustive bounded identity oracles.
- [x] **Speed P7 — Add per-facet constraint indices and conservative local 2D broad phase** while retaining exact reconciliation and exhaustive bounded oracles.
- [x] **Speed P8.1-P8.2 — Localize global stitching/spherical links and optimize exact patch witnesses** without reintroducing centroid or tolerance heuristics.
- [x] **Speed P9.1-P9.2 — Remove source-geometry duplication and accelerate independent point location** with complete hit-record differential checks.
- [x] **Speed P10 — Add strict `exact_in_T` singleton realization and sparse obligation/output handling** while preserving the complete obligation universe and independent replay.
- [x] **Speed P11 — Audit and bound remaining mandatory verifier costs** with independent reconstruction families and exhaustive bounded oracles.
- [x] **Speed P12 — Replace per-task `std::async` with deterministic bounded workers** and canonical merge/error selection.

---

# Outstanding assessment-driven productization

## P0 — Freeze product contracts before new behavior

Do not begin normalization repair, approximate realization, or backend fallback implementation until this phase is reviewed and complete.

- [x] **P0 complete — freeze the amended product boundary.**
  - [x] Approve and implement the immutable tagged `boolean_product_result`/result-envelope model with `exact_stratified`, `exact_in_T_mesh`, and `certified_approximate_mesh` representations.
  - [x] Add the explicit maturity model (`experimental`, `candidate`, `qualified`, `deprecated`) and ensure the current in-tree engine defaults to `experimental_exact_v1` with explicit opt-in.
  - [x] Add versioned backend-selection, backend-capability, preparation, normalization, result-representation, realization-semantic, attribute-transfer, and qualification-manifest policies to Component 1.
  - [x] Separate realization semantic meaning from candidate/search strategy; prohibit search settings from changing an exact result into approximate success.
  - [x] Define stable backend identity/version/build/capability provenance and bind it into every result, diagnostic, certificate, and replay.
  - [x] Add the amended error categories/subcodes: `normalization_required`, `normalization_failed`, `backend_unavailable`, `backend_capability_mismatch`, `backend_disagreement`, `backend_unqualified`, `exact_result_serialization_error`, `attribute_transfer_conflict`, `approximation_policy_rejected`, and `qualification_policy_violation`.
  - [x] Bump option, artifact, error, certificate, and replay schemas together; add strict decode rejection for stale bindings, unknown capabilities/enums, incompatible schemas, and semantic reinterpretation.
  - [x] Define the lifetime/ownership contract for a durable exact result independently of `boolean_context`.
  - [x] Update public API documentation to state that strict already-valid operands are currently required, `exact_in_T` is a strict special-purpose mode, and no backend/result profile is production-qualified merely because the original engine tests pass.
  - [x] Add contract, serialization, compatibility, invalid-policy, maturity-selection, and fail-closed fallback tests for every P0 policy combination.

**P0 exit gate:** all amended contracts are frozen and reviewed; schema/replay golden vectors pass; no product-facing API can silently normalize, fall back, discard an exact success, or label approximate geometry as exact.

## P1 — Publish a durable authoritative exact result

Start only after P0.

- [x] **P1 complete — detach and publish the verified exact stratified boundary.**
  - [x] Detach Component 10's verified `selected_exact_boundary` and all required exact-coordinate/construction, occurrence, spherical-link, topology-obstruction, side-label, and provenance data from invocation-private storage.
  - [x] Define immutable owning and shared-lifetime handles that cannot outlive referenced storage incorrectly and cannot retain stale context-owner tokens.
  - [x] Add versioned canonical serialization/deserialization, digest binding, replay, resource accounting, and corruption detection for the exact result.
  - [x] Expose exact coordinates as canonical rationals or versioned construction records without forcing conversion to `T`.
  - [x] Expose explicit topology class, surface occurrences, local link components, obstruction records, source contributors, backend provenance, and operation/preparation provenance.
  - [x] Provide a later-realization entry point so one exact result can be requested as exact-coordinate output or realized into multiple coordinate/index types and policies.
  - [x] Change Component 12/result-envelope handling so manifold rejection or finite-`T` realization failure does not erase a valid exact success when exact retention was requested.
  - [x] Extend Component 13 with an independent exact-result reader/verifier that reconstructs bindings from serialized bytes rather than producer-owned derived state.
  - [x] Add round-trip, cross-process replay, stale-certificate, stale-backend/preparation binding, byte corruption, truncation, resource-limit, non-manifold retention, and lifetime tests.

**P1 exit gate:** a verified exact manifold or stratified non-manifold result survives context destruction, round-trips canonically, and remains available when every mesh realization is declined or fails.

## P2 — Establish the explicit preparation and normalization boundary

Start only after P1. Strict validation remains exact and non-mutating; normalization is a separate, explicitly approximate/modeling operation.

- [ ] **P2.1 — Extract strict validation as a reusable product service.**
  - [ ] Implement `validate_operand_strict(...)` returning an immutable `prepared_operand` plus validation/provenance certificate without geometry-changing edits.
  - [ ] Accept prepared operands in backend requests only after verifying input digest, policy, report, and certificate bindings.
  - [ ] Keep Component 2 as the authoritative strict validator and prohibit normalization heuristics inside it.
  - [ ] Add stale prepared-operand, foreign-context, mutation-after-validation, reserialization, and direct-raw-versus-prepared equivalence tests.

- [ ] **P2.2 — Define normalization policy/report infrastructure.**
  - [ ] Define units, model tolerance, diagnosis-only versus structural-only versus geometry-changing modes, per-operation enablement, deterministic edit ordering, and resource/cancellation policy.
  - [ ] Define `normalization_report` with input/output digests, every edit, exact/bounded displacement, topology changes, unresolved defects, source-to-prepared vertex/edge/facet/shell/attribute maps, reversibility, and post-normalization strict-validation certificate.
  - [ ] Implement diagnosis-only mode first; it must never alter the operand.
  - [ ] Add independent report verification and prove every successful normalized operand passes full strict Component 2 validation.

- [ ] **P2.3 — Implement normalization operations one at a time.** Do not combine multiple new repair classes in one unreviewable change; each item needs focused positive/negative tests, report replay, strict revalidation, regression corpus additions, and measured displacement/topology evidence.
  - [ ] Deterministic safe structural canonicalization and irrelevant-storage removal, preserving source mappings.
  - [ ] Exact duplicate vertex/facet diagnosis and only policy-authorized exact consolidation/removal.
  - [ ] Orientation and nested-shell polarity diagnosis/repair without geometric movement.
  - [ ] Attribute-seam-aware duplicate/near-duplicate vertex consolidation.
  - [ ] Crack and small-gap diagnosis/closure under explicit model tolerance.
  - [ ] Non-planar polygon diagnosis followed by explicitly selected triangulation or refitting policy.
  - [ ] Duplicate/overlapping facet resolution with deterministic ownership and attribute conflict reporting.
  - [ ] Sliver feature diagnosis and any policy-approved handling with complete topology/displacement evidence.
  - [ ] Self-intersection diagnosis and, only if separately approved, repair as the final/highest-risk normalization class.

- [ ] **P2.4 — Correct the product claim boundary and examples.**
  - [ ] Until P2 repair modes are implemented and qualified, make unknown-provenance STL/OBJ/scan/CAD input produce a clear preparation requirement rather than an implied supported workflow.
  - [ ] Document how callers choose strict validation, diagnosis-only normalization, or a specific repair policy and how they inspect/reject the report before Boolean execution.

**P2 exit gate:** strict and normalized operands are distinct types/provenance paths; every edit is auditable and independently verified; no hidden healing occurs in Boolean evaluation.

## P3 — Add complete output representation modes

Start only after P2. Preserve the existing strict `exact_in_T` algorithm as a regression baseline while integrating it into the amended result model.

- [ ] **P3.1 — Integrate strict `exact_in_T` into the tagged product result.**
  - [ ] Preserve exact-coordinate equality, complete defining relations, topology/embedding obligations, and existing impossible-coordinate behavior.
  - [ ] Bind every strict mesh realization to the durable exact-result digest and retain the exact result when strict realization fails if requested.
  - [ ] Remove any public wording that treats strict `exact_in_T` success rate as the ordinary CAD-output target.
  - [ ] Re-run all G2/G7/G8/G9c, one-third, rounding-collision, mutation, replay, re-ingestion, and speed-baseline gates through the new envelope.

- [ ] **P3.2 — Add exact-coordinate export.**
  - [ ] Define a stable exact-coordinate mesh/stratified representation using canonical rationals or versioned constructions.
  - [ ] Preserve surface occurrences and non-manifold topology where the exact result requires them.
  - [ ] Add canonical serialization, public lifetime, indexing/capacity policy, provenance, and independent verification.

- [ ] **P3.3 — Implement `certified_approximate_embedding_v1`.**
  - [ ] Define caller-supplied global/per-axis displacement bounds, original-vertex movement policy, support-plane deviation, model-tolerance metadata, deterministic candidate generation, and search/resource limits.
  - [ ] Generate the complete variable/obligation graph and require occurrence-isomorphic topology, preserved orientation/order/incidence, non-collapse, and no introduced prohibited intersections.
  - [ ] Emit exact target/output bits, displacement vectors/maxima, relaxed-relation inventory/deviations, accepted assignment transcript, and exact-result digest binding.
  - [ ] Return policy-relative `output_not_representable` on bounded search exhaustion; never claim mathematical impossibility unless proved.
  - [ ] Extend Component 13 with an independently implemented certificate generator/replayer family and standalone link-separation tests.
  - [ ] Add adversarial non-dyadic, rounding collision, thin/sliver, high-valence, coplanar, large-range, subnormal, candidate-boundary, solver-limit, and mutation tests.

**P3 exit gate:** exact, strict finite-`T`, and certified approximate outputs are semantically distinct; each is bound to one durable exact result and independently verified under its declared contract.

## P4 — Introduce backend adapters and controlled selection

Start only after P3. Backend integration must not become an unverified semantic shortcut.

- [ ] **P4 complete — implement the capability-described backend boundary.**
  - [ ] Stabilize the common backend request/result envelope, capability schema, ownership model, cancellation/resource behavior, deterministic diagnostics, and adapter verification hooks.
  - [ ] Wrap Components 3-10 as `experimental_exact_v1` without changing their exact semantics; keep backend-private artifacts out of ordinary public headers where practical.
  - [ ] Implement `explicit_backend` selection with no fallback and deterministic capability mismatch reporting.
  - [ ] Implement qualification-manifest-driven `qualified_default`; it must reject selection when no backend/profile is qualified.
  - [ ] Add at least one independently implemented mature backend adapter in diagnostic-only mode for the declared qualification workload.
  - [ ] Verify adapter input conversion, regularization/orientation/tolerance semantics, output topology, exact/guarded occupancy evidence, ownership, provenance, and failure-category mapping.
  - [ ] Implement `diagnostic_compare` with one declared producer; preserve all disagreements and prohibit majority-vote publication.
  - [ ] Implement caller-declared `explicit_fallback_chain` only for listed failure categories; retain the primary failure and producing backend in the result.
  - [ ] Prohibit fallback for internal invariant errors, stale/malformed evidence, replay mismatch, verifier disagreement, capability mismatch that changes promised semantics, or unapproved normalization/approximation.
  - [ ] Add backend unavailable/version drift/capability drift, semantic mismatch, disagreement, deterministic selection, cancellation, fallback, and adapter-corruption tests.

**P4 exit gate:** every result names its producer and capabilities; fallback is explicit and auditable; external agreement is evidence only; no adapter can bypass mandatory verification.

## P5 — Complete provenance, attributes, and the application-facing service

Start only after P4.

- [ ] **P5.1 — Define and implement stable provenance/attribute transfer.**
  - [ ] Define versioned policies for source body/shell/facet IDs, materials, per-face metadata, vertex normals, sharp-edge tags, texture seams, opaque channels, and compact construction provenance.
  - [ ] Define deterministic split, merge, conflict, interpolation/copy, and omission behavior for multi-source output entities.
  - [ ] Ensure attributes never influence exact geometric topology decisions.
  - [ ] Bind output vertex/facet mappings to exact entities and source contributors; report every omission or conflict.
  - [ ] Add independent mapping verification, replay, conflict, seam, removed-internal-face, coincident-source, and attribute-value-invariance tests.

- [ ] **P5.2 — Add the one-call conservative service.**
  - [ ] Implement the simple `boolean_operation(a, b, op, boolean_service_options)` product API.
  - [ ] Default to no geometry-changing normalization, no silent fallback, mandatory verification, explicit result representation, retained exact result when requested, and qualification-manifest enforcement.
  - [ ] Require explicit opt-in for experimental backends and reject an unavailable qualified default with a typed error.
  - [ ] Return backend, preparation, exact-result, realization, attribute, verification, and qualification provenance in one result envelope.
  - [ ] Keep kernel/verifier/executor/backend/store dependency injection behind a separately documented expert API.
  - [ ] Add service-level cancellation, resource-limit, thread-count, replay, exception-safety, and partial-publication tests.

- [ ] **P5.3 — Replace the product-facing example and migration guidance.**
  - [ ] Add a minimal ordinary-caller example using the one-call service, strict versus normalized preparation choices, explicit backend/maturity selection, exact-result retention, exact/approximate mesh handling, attribute reports, and typed failures.
  - [ ] Retain an advanced example for dependency injection, clearly labeled as expert/internal usage.
  - [ ] Add end-to-end tests for both examples and remove any implication that unknown-provenance meshes are accepted without a preparation policy.

**P5 exit gate:** ordinary callers do not register internal verifiers manually; provenance and attribute behavior is explicit; the simple service cannot select an experimental/unqualified or semantically incompatible path silently.

## P6 / Speed P13 — Execute production qualification and promotion

Start only after P5. This section is the sole successor to the former one-line P13 item. Follow `plan_16_qualification_release.md` in this exact order.

- [ ] **P6.1 — Define and enforce qualification schemas.**
  - [ ] Add versioned canonical qualification manifest, normalized outcome taxonomy, machine-readable result summary, and human-readable report schemas.
  - [ ] Bind backend/adapter/build/capabilities, result modes, preparation policies, types, compilers/platforms, verifiers, corpora, generators/seeds, fuzz campaigns, chains, limits, performance protocol, thresholds, exclusions, and known limitations.
  - [ ] Make any material capability, policy, verifier, compiler mode, corpus, or schema change invalidate or explicitly compatibility-review the prior qualification claim.

- [ ] **P6.2 — Expand and inventory the permanent corpus.**
  - [ ] Reach at least 10,000 generated construction-known operand pairs.
  - [ ] Reach at least 1,000 representative licensed or internally generated CAD-like tessellated operand pairs.
  - [ ] Reach at least 1,000 deterministic operation chains of at least five successful or intentionally failing steps.
  - [ ] Preserve every minimized regression discovered by any test, fuzz, sanitizer, backend comparison, or customer workload.
  - [ ] Encode coverage by geometry category, operation, both difference directions, type/index specialization, result mode, preparation policy, and expected outcome.
  - [ ] Include non-box intersections, skew/rotated convexity, concavity, cavities, coplanar overlays, high valence, thin features, alternate subdivisions, scale extremes, non-dyadic intersections, stratified results, attribute conflicts, and normalization defects.

- [ ] **P6.3 — Implement construction-aware generators and operation-chain harnesses.**
  - [ ] Add exact halfspace/polytope, profile/extrusion-like, coplanar-overlay, nested-shell/cavity, subdivision, feature-alignment, and representable-transform generators with retained recipes.
  - [ ] Add valid intersecting-geometry fuzzing; retain disjoint boxes only as a smoke family.
  - [ ] Add invalid/preparation fuzzing with exact defect labels and normalization-report checks.
  - [ ] Add chain execution with re-ingestion, exact-result retention, varied association/order, explicit normalization boundaries, and transactional failure propagation.
  - [ ] Add deterministic provenance-guided minimization and automatic permanent-regression promotion.

- [ ] **P6.4 — Run independent backend comparison in diagnostic-only mode.**
  - [ ] Execute at least one mature independent backend over the declared workload profile.
  - [ ] Preserve inputs, options, versions, all outputs/failures, semantic differences, and guarded/exact probe evidence for every disagreement.
  - [ ] Minimize and classify every disagreement as correct, incorrect, unsupported, policy-different, or unresolved.
  - [ ] Block qualification while any material disagreement remains unexplained.

- [ ] **P6.5 — Add false-success detection and accounting.**
  - [ ] Normalize all outcomes into verified exact success, verified approximate success, expected/unexpected typed failure, backend/verifier disagreement, false success, nondeterminism, resource/timeout, or infrastructure failure.
  - [ ] Re-ingest every mesh success, independently reconstruct topology, replay certificates from output bits, classify guarded probes, check embedding/orientation/nesting, bind to exact-result digest, and feed it into later chain operations.
  - [ ] Count false success separately from safe failure by backend, mode, preparation policy, category, operation, specialization, and platform.
  - [ ] Treat any semantic mislabeling, incorrect occupancy/topology/orientation/embedding, stale binding, or incorrect attribute map as a blocking false success.

- [ ] **P6.6 — Add controlled CAD-like corpus ingestion.**
  - [ ] Record source/generator class, intended model tolerance, provenance/license, preparation report, expected product outcome, and redistribution constraints.
  - [ ] Support content-addressed external/private artifacts with repository manifests, digests, retrieval procedures where permitted, anonymized category/outcome summaries, and compact in-repository CI representatives.

- [ ] **P6.7 — Add preparation, result-mode, attribute, and provenance qualification suites.**
  - [ ] Qualify strict validation independently from every normalization mode/repair class.
  - [ ] Qualify exact stratified, strict `exact_in_T`, and certified approximate representations independently.
  - [ ] Verify all advertised attribute/provenance policies, seams, conflicts, omissions, multi-source mappings, and downstream-style source queries.
  - [ ] Demonstrate that approximate and normalization tolerances/displacements are explicit, recorded, independently checked, and never hidden test epsilons.

- [ ] **P6.8 — Complete compiler, sanitizer, determinism, resource, and fuzz-duration matrices.**
  - [ ] Run current and oldest-supported GCC/Clang, Debug/optimized, supported standard libraries, x86-64 and an independent 64-bit architecture, strict floating-point modes, and 32/64-bit index specializations as applicable.
  - [ ] Pass ASan/UBSan and concurrency suites under TSan; run debug iterator/library modes where available.
  - [ ] Run one/multiple workers, queue/task partitions, broad-phase variants, filter accept/fallback/mixed paths, allocation perturbations, configurable hash seeds, ambient rounding modes, and separate-process replay.
  - [ ] Run at least the Plan 16 minimum valid, invalid/preparation, and chain fuzz CPU-hour campaigns per required sanitizer/configuration; preserve and minimize every unique outcome.

- [ ] **P6.9 — Freeze performance/memory/cancellation methodology and enforce gates.**
  - [ ] Benchmark strict validation, each normalization class, backend evaluation, topology preflight, exact-result serialization, all realization modes, mandatory/exhaustive verification, and operation chains on representative/adversarial workloads.
  - [ ] Record wall/CPU time, peak RSS and authoritative bytes separately, exact-number growth, candidate/event/patch growth, verifier overhead, realization search, serialization size, cancellation latency, and outcome category.
  - [ ] Preserve the approved `plan_speed.md` P0 baseline comparisons and review the original targets (including B1-B7 geometric mean, B6, per-case regression, and authoritative-byte constraints) against the amended product path; record any explicitly approved revised threshold rather than silently dropping it.
  - [ ] Prove resource-limit and cancellation failures remain transactional with no partial publication.

- [ ] **P6.10 — Execute the frozen candidate campaign and resolve outcomes.**
  - [ ] Run the complete frozen manifest on controlled infrastructure.
  - [ ] Resolve, minimize, or mark blocking every unexpected failure, disagreement, nondeterministic result, timeout, infrastructure issue, and performance/resource regression.
  - [ ] Add every resolved defect case to the permanent corpus and rerun affected configurations.

- [ ] **P6.11 — Commit the reproducible report and promote only reviewed profiles.**
  - [ ] Commit the human-readable qualification report plus machine-readable manifest/results, exact repository state, commands, dependencies, hardware/platform matrix, corpus coverage, seeds/durations, outcomes, disagreement resolutions, sanitizer/determinism results, performance/resource tables, rates, limitations, and replay artifact digests.
  - [ ] Update `qualified_default` only for explicitly named backend/result-mode/preparation/workload profiles that pass every applicable gate.
  - [ ] Keep all other profiles `experimental` or `candidate`; never generalize a narrow qualification claim.
  - [ ] Implement qualification revocation/demotion when a false success, unexplained disagreement, schema incompatibility, or material platform defect is discovered.

### Final production release gates

Do not check P6 complete until every applicable item is checked for the exact profile being promoted.

- [ ] Zero known false successes or semantic mislabeling.
- [ ] Zero unexplained producer/verifier disagreements.
- [ ] Zero material unexplained independent-backend disagreements.
- [ ] Zero nondeterministic canonical outcomes across the required schedule/platform subset.
- [ ] All permanent regressions pass in every applicable supported configuration.
- [ ] All required sanitizer and undefined-behavior runs pass, with every suppression documented and reviewed.
- [ ] Corpus floors and category/operation coverage are met and enforced by the manifest.
- [ ] Operation chains pass with re-ingestion, exact-result retention, and transactional failure behavior.
- [ ] Product-approved success and typed-failure thresholds pass for the declared workload; strict `exact_in_T` is not used as the practical-output success target.
- [ ] Performance, peak memory, exact-number growth, verifier overhead, and cancellation latency are measured and approved.
- [ ] Every normalization and approximate mode has explicit independently verified tolerance/displacement evidence.
- [ ] Every advertised attribute/provenance policy passes its qualification contract.
- [ ] A complete independently reproducible report is committed and bound to the qualification manifest.
- [ ] Public API documentation names all supported and unsupported profiles, semantics, limitations, and maturity states.

- [ ] **P6 / Speed P13 complete — production qualification finished.** Check only after the final report names exactly which backend/result-mode/preparation/workload profiles, if any, are qualified and every gate above passes for those profiles.
