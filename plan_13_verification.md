# Component 13 implementation plan: invariant verification, evidence, and publication gates

## 1. Scope and outcome

Implement the cross-cutting verification infrastructure that makes every Components 1-12 publication invariant executable in Release as well as Debug builds. The component accepts an immutable candidate artifact, its declared slot/type/schema, immutable prerequisite artifacts, the frozen context and exact-kernel services, and a versioned verification specification. It produces a deterministic `verification_report` that states exactly what ran, carries independently replayable evidence, and either authorizes Component 1 to mint a private publication token or prevents publication with a typed failure.

The completed component must provide:

- a frozen verifier registry and type-safe adapter boundary for every artifact slot;
- stable invariant identifiers, mandatory/exhaustive specifications, and documented check precedence;
- read-only checker execution with deterministic first-causal-failure selection;
- versioned, canonical, serializable reports and evidence records bound to artifact and dependency digests;
- a separate nonserializable publication-authority token that cannot be forged or reused;
- exact-value diagnostics, corrupted-handle diagnostics, dependency slicing, replay, and minimization hooks;
- explicit verifier accounting, cancellation, exception containment, and transactional rollback;
- independent mandatory checkers for Components 1-12 and separately implemented bounded-test oracles where required;
- a special nonrecursive final-verification transaction that alone authorizes returning Component 12's public mesh;
- mutation, stale-evidence, replay, determinism, resource-boundary, Release/NDEBUG, and sanitizer qualification.

Verification is infrastructure implemented alongside Components 1 and 3 and then extended with each producer. Do not postpone it until after Component 12. A stage verifier may be less optimized than its producer, but no mandatory check may be skipped because it is expensive; it must complete within charged resources or fail with `resource_limit`.

All implementation must be self-contained, portable C++17, and follow Ygor conventions. Add no external dependencies. Explicitly do not include, call, adapt, inspect during implementation, or test against `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Current facilities

No current source implements the planned Boolean context, exact kernel, immutable artifacts, verifier registry, reports, evidence, replay grammar, or publication gates. Those are planned interfaces and must be reconciled before Component 13 code is frozen.

Reuse or adapt only these existing facilities:

- `fv_surface_mesh<T, I>` in `src/YgorMath.{h,cc}` remains the Component 12 public carrier. A final checker must parse its six public fields directly and never trust a producer-side topology cache.
- Adapt ordered undirected-edge aggregation and opposite-use checking from `src/YgorMeshesVerification.{h,cc}` as simple independent structural patterns. Keep the existing API available for unrelated callers.
- Adapt `fv_surface_mesh<T, I>::recreate_involved_face_index` only as a reference for rebuilding reverse incidence. The verifier must implement a checked, read-only reconstruction and compare it with the stored field.
- Use `YLOGDEBUG`, `YLOGINFO`, and `YLOGWARN` only after canonical diagnostics are frozen, as optional observers. Logging never determines pass/fail or report bytes.
- Use the repaired in-tree MD5 implementation required by Component 1 through its domain-separated canonical digest wrapper.
- Follow the repository's standalone C++ test executable style while adding authoritative CTest registration.

### 2.2 Deficiencies that prohibit direct reuse

`src/YgorMeshesVerification.{h,cc}` is not a Boolean artifact verifier because it:

- returns booleans or prose exceptions rather than typed invariant results and exact evidence;
- uses native floating arithmetic for degeneracy;
- checks triangle edge counts but not complete vertex links, embeddedness, shell semantics, realization obligations, canonical order, or provenance;
- does not bind results to artifact identity, owner, dependencies, schema, or digests;
- has no deterministic first-failure policy, accounting, cancellation, replay, mutation hooks, or publication authority;
- permits incomplete orientation checking when an edge does not have exactly two uses.

Do not use Ygor's generic binary serializer for authoritative records. Define canonical encoding through Component 1 because native layout, endian behavior, implicit data, and fatal error paths are unsuitable. Do not use `YLOGERR` for expected or internal verifier failures because it exits the process. Never invoke mesh cleanup, welding, remeshing, triangulation, orientation repair, tolerance comparison, or mutation from a verifier.

The existing project-wide GCC `-ffast-math` conflicts with exact conversion and verification. Boolean sources must receive effective strict floating-point flags after global flags and compile-time guards must reject `__FAST_MATH__` or finite-math-only compilation. Verify effective commands in CI.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanVerification.h`: invariant taxonomy, verification levels/specifications, immutable environment and dependency views, report/evidence/diagnostic schemas, verifier registration/service declarations, dependency-slice interfaces, replay codecs, and final-verification entry point.
- `src/YgorMeshesBooleanVerification.cc`: registry validation/freezing, deterministic dispatch, common binding and report checks, report/evidence canonical codecs, digest validation, first-failure arbitration, dependency slicing, final verification, exception containment, and four supported `<T, I>` adapters where templates are required.
- `tests/Test_MeshesBooleanVerification.cc`: registry, schema, report, evidence, authority, stale-binding, resources, cancellation, mutation, finalization, and Release-check tests.
- `tests/Test_MeshesBooleanVerificationProperties.cc`: repeated-process/thread determinism, permutation, replay, dependency minimization, checker/oracle differential, and cross-component mutation properties.
- `tests/MeshBooleanVerificationFixtures.h`: synthetic artifact registry, malformed views, canonical report builders, evidence/replay decoders, mutation operators, accounting/cancellation/fault seams, deterministic PRNG, and independent small-artifact oracles.

Modify:

- `src/YgorMeshesBooleanContract.{h,cc}` to reconcile the three-argument verifier callback, strong invariant/evidence IDs, resource kinds, report bindings, artifact views, private publication tokens, transaction gate, finalization marker, canonical digest domains, and replay metadata.
- `src/YgorMeshesBooleanExactKernel.{h,cc}` to expose exact-only read services and canonical exact-value encoding needed by independent checkers without exposing mutable caches.
- each Components 2 and 4-12 source to register and implement its typed checker adapter beside the artifact definition; Component 3 registers exact-kernel checks, and Component 1 registers context/store/transaction checks.
- `src/CMakeLists.txt`, root `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/compile.sh`, and `.gitlab-ci.yml` for strict Boolean compilation and authoritative tests.

Use namespace `ygor::mesh_boolean`. Keep typed adapter casts, checker scratch graphs, independent reconstructions, minimizer work queues, report builders, and decoder implementation private to `.cc` files or tests.

Register CTest targets `MeshBooleanVerification.Unit` and `MeshBooleanVerification.Properties`, labeled `mesh_boolean;component13`. Run GCC and Clang Debug/Release, ASan/UBSan malformed-artifact and rollback tests, and TSan concurrent verification/finalization tests without ignored failures or network access.

## 4. Required interface reconciliation

Freeze these decisions before implementing artifact checkers:

1. Replace Component 1's provisional two-argument callback with:

```cpp
using verifier_callback = status_or<verification_report> (*)(
    const artifact_view&,
    const verification_spec&,
    const verification_environment_view&) noexcept;
```

The environment is necessary for immutable prerequisites, raw inputs where contract verification requires them, exact-only kernel services, frozen operation/policies, accounting, cancellation, and replay metadata. It contains no mutable artifact pointer or global lookup.

2. Distinguish `evidence_record`, which is versioned and serializable, from `publication_certificate`, which is a private process-local authority token. Component 13's requirement for serializable certificates is fulfilled by evidence records in reports. The authority token has no public constructor and is never encoded.

3. Reports and authority tokens are not part of the artifact semantic digest. `published_artifact` owns them beside the immutable payload. Invocation-bound artifact encodings may bind prerequisite reports, but a candidate must not include its own report and create a digest cycle.

4. A completed invariant failure returns a trustworthy failed report. `status_or` carries inability to produce a trustworthy report: resource exhaustion/cancellation, malformed invocation boundary, unsupported schema/platform, or verifier defect. Publication accepts only a complete `pass` report.

5. Freeze the Component 12 public success wrapper consistently with `plan_12_output_assembly.md`; final verification authorizes that immutable result. Do not retain Component 1's earlier mesh-only alias if it would discard required audit bindings.

6. Model `artifact_slot::final_verification` as a finalization record, not an artifact recursively dispatched to another verifier.

7. Component 13 owns common schemas and orchestration. Typed artifact internals and independent checkers stay with their producing components to avoid circular headers and one monolithic verifier.

## 5. Core schemas and contracts

### 5.1 Verification level and specification

Schema v1 supports exactly:

```cpp
enum class verification_level : std::uint8_t {
    mandatory = 0,
    exhaustive = 1,
};
```

`mandatory` is always enabled and is the minimum publication gate in every build. `exhaustive` includes all mandatory invariants plus versioned independent reconstruction/oracle checks. Build type may control whether exhaustive mode is requested by default, never whether mandatory checks exist.

Define `verification_spec` with fixed-width fields for spec schema, checker version, level, artifact slot, artifact type tag, artifact schema, ordered required invariant codes, invariant-set digest, resource policy, and deterministic failure-order policy version. Construction must normalize and validate the invariant list before dispatch. Reject unknown, duplicate, out-of-order after decode, missing mandatory, wrong-slot, or unsupported codes rather than ignoring them.

Each registered artifact has one immutable mandatory invariant set per supported spec version. Exhaustive sets are explicit supersets. A caller may request stronger registered coverage but cannot subtract mandatory codes.

### 5.2 Stable invariant taxonomy

Define `enum class invariant_code : std::uint32_t` with reserved numeric ranges:

- `0x010000-0x01ffff`: context, IDs, accounting, deterministic stores, transactions;
- `0x020000-0x02ffff`: validated operands;
- `0x030000-0x03ffff`: exact numbers, predicates, constructions;
- `0x040000-0x04ffff`: candidate stream;
- `0x050000-0x05ffff`: raw events;
- `0x060000-0x06ffff`: symbolic registry;
- `0x070000-0x07ffff`: local refinement;
- `0x080000-0x08ffff`: global arrangement;
- `0x090000-0x09ffff`: cell classification;
- `0x0a0000-0x0affff`: Boolean selection;
- `0x0b0000-0x0bffff`: realization;
- `0x0c0000-0x0cffff`: output assembly;
- `0x0d0000-0x0dffff`: report, evidence, replay, and finalization infrastructure.

Assign explicit values; never depend on declaration ordinal. Every code has stable metadata: owning slot/stage, mandatory/exhaustive class, description key, expected failure category, evidence kind, and documented complexity/accounting units.

Global first-cause precedence is: invocation/owner binding, schema/type, dependency identity, handle/range structure, local incidence, exact geometry, coverage/global semantics, canonical ordering, encoding/digest. Within one invariant, choose by canonical work key, stable subcode, and canonical entity tuple. Checkers may stop after the first selected failure but must mark every later requested code `not_run_due_to_prior_failure`; they must never imply those checks passed.

### 5.3 Artifact and environment views

Retain Component 1's lifetime-owning `artifact_view` and add schema/generation as explicit checked fields if they are not recoverable safely from the typed payload. Digest equality never substitutes for owner and strong object identity within a process.

`verification_environment_view` contains immutable, non-owning views backed by lifetime owners for:

- context owner token, setup digest, operation and all frozen policies;
- exact-kernel service identity, arithmetic-policy digest, and exact-only service interface;
- raw operands only for checks whose contract requires source reconstruction;
- a slot-indexed prerequisite table with owner, slot, type, schema, generation, strong lifetime, payload, artifact digest, semantic digest, and accepted report digest;
- resource-accounting facade restricted to verifier reservations/charges;
- cancellation token, replay descriptor, and deterministic execution policy.

Typed adapters validate slot, type tag, schema, owner, generation, setup/kernel policy, dependency table, and payload non-nullness before casting. Invalid adapter boundaries produce a typed verifier defect or stale/malformed invocation failure without dereferencing payload data.

### 5.4 Report and invariant results

Define equivalent immutable records:

```cpp
enum class verification_outcome : std::uint8_t {
    pass,
    invariant_failure,
    malformed_artifact,
    stale_evidence,
    resource_limit,
    verifier_defect,
};

enum class check_status : std::uint8_t {
    passed,
    failed,
    not_run_due_to_prior_failure,
};

struct invariant_result {
    invariant_code code;
    check_status status;
    std::vector<feature_ref> entities;
    std::vector<evidence_id> evidence;
    std::optional<diagnostic_id> causal_diagnostic;
};
```

`verification_report` stores owner token only in its process-local wrapper and canonically stores stage, slot, artifact type/schema, setup/artifact/dependency/invariant-set digests, report/spec/checker/evidence schema versions, level, outcome, requested invariant codes, one ordered result per code, canonical evidence records, diagnostics, optional dependency slice/minimized seed, accounting summary, and report digest.

A pass requires every requested code exactly once with `passed`, no failed/not-run result, and no authority-affecting truncation. A fail identifies exactly one first causal result; supplementary diagnostics cannot change failure selection. Every entity vector is sorted/deduplicated. Invalid handles are represented by a `corrupted_handle_locator` containing slot/type, canonical field path, raw encoded value, and expected domain; never forge a valid `feature_ref` from corrupt data.

### 5.5 Evidence records

Define a tagged `evidence_record` with stable evidence kind/schema, invariant code, canonical entity references, exact operands, exact relations/signs, provenance edges, dependency digests, and evidence digest. Exact values use Component 3 canonical signs and big-integer/rational limbs; source floats use raw binary32/binary64 bits. Decimal strings may be added for humans but are never sole or canonical evidence.

Evidence must be sufficient for an independent replay routine to reconstruct the claimed relation from immutable dependencies. Do not serialize pointers, owner tokens, cache indices without semantic bindings, native object bytes, locale text, timestamps, thread IDs, file paths, or producer helper state.

### 5.6 Publication authority

Define a move-only `publication_certificate` with a private constructor accessible only to the context's accepted-report path. Bind it to owner, stage, slot, artifact type/schema/generation, artifact digest, report digest, invariant-set digest, checker/spec versions, and prerequisite-generation digest.

Mint only after validating a complete passing report. `stage_transaction::publish` rechecks every binding, transaction state, cancellation, prerequisite publication, and mandatory invariant equality. Any reuse across owner, transaction, artifact mutation, report, slot, stage, schema, generation, or dependency replacement fails closed. Certificates are not serializable and report replay never recreates authority.

## 6. Registry, execution, and report construction

### 6.1 Registry lifecycle

Key registration by `(artifact_slot, artifact_type_tag, verification_spec_version)`. A registration includes callback, artifact/checker/evidence schemas, mandatory and exhaustive invariant lists, invariant metadata, and declared dependency slots.

Reject duplicate keys, duplicate/unknown invariant codes, wrong code ownership, missing callback, unknown slot/type/schema, dependency cycles, and mandatory sets not contained in exhaustive sets. `freeze(required_artifact_manifest)` verifies every required Components 1-12 adapter is present, creates canonical lookup order, computes a registry digest, and permanently prevents mutation. Context creation requires a shared immutable frozen service.

### 6.2 Deterministic execution

For one verification:

1. Validate the view, environment, registration, specification, and dependency bindings without touching untrusted indexed fields.
2. Reserve the declared report, evidence, diagnostic, exact-kernel, and checker-scratch envelope in canonical order.
3. Snapshot immutable identity/digest bindings needed to prove read-only execution.
4. Run checks in numeric invariant order. Parallel subchecks use canonical work keys and private shards.
5. Normalize shard results before choosing a failure. Worker completion order never chooses it.
6. Select the least failure by invariant code, work key, subcode, and entity key; mark later checks not run.
7. Canonically sort/deduplicate evidence and diagnostics, validate every reference, and compute evidence digests.
8. Recheck cancellation and dependency identities, encode the report, compute its digest, and return it.
9. Compare artifact semantic bytes/digest and all immutable dependency identities before and after verification in verification tests; production relies on const-only APIs and ownership rather than an unbudgeted duplicate.

No checker mutates, repairs, canonicalizes in place, drops malformed records, or publishes. Hash tables may accelerate lookup, but sorted semantic keys determine output and first failure. Comparators are `noexcept` and inspect precomputed canonical keys only.

### 6.3 Failure containment

- A completed artifact invariant violation yields a failed report and ultimately `internal_invariant_error`, except checks explicitly validating caller input during Component 2 retain `input_contract_error` before that artifact is published.
- Verifier resource, allocation, exact-number, work, evidence, encoding, or cancellation exhaustion returns `resource_limit`; it cannot produce pass.
- Unsupported report/spec/platform versions return the matching setup/platform failure before artifact interpretation.
- Malformed/stale evidence, digest mismatch, owner/generation mismatch, producer/checker disagreement, or a checker exception is an internal invariant failure and prevents publication.
- Catch `std::bad_alloc`, other `std::exception`, and unknown exceptions at adapter and service boundaries according to Component 1's taxonomy. Never continue with partial evidence.
- Assertions may aid development but no mandatory Release check may depend on `assert`.

## 7. Canonical encoding, replay, and diagnostics

Define `YGBVER13` framing for reports and `YGBEVD13` for evidence. Use Component 1 fixed-width big-endian integers, explicit enum/bool validation, `u64` length prefixes, canonical exact limbs/rationals, and domain-separated digests. Preserve Component 1's report formula:

```text
report_digest =
  MD5("YGBVER01" || setup_digest || artifact_digest ||
      verifier/spec versions || invariant_set_digest ||
      canonical check/evidence payload)
```

The report payload includes dependency digests so stale upstream evidence is detectable. Decode rejects unknown versions or invariant codes, invalid enums/booleans, malformed exact numbers, unsorted/duplicate records, inconsistent outcome/status combinations, invalid references, bad lengths, trailing bytes, digest disagreement, and evidence not bound to the report's artifact/dependencies.

Diagnostics contain stable code/subcode, stage, invariant, canonical work key, valid feature refs or corrupted-handle locator, exact operands/relations, raw source float bits, local subgraph records, stage options/policy digests, input/setup/artifact/dependency digests, resource facts, and replay locator. Canonical payloads exclude approximate-only decimals, timestamps, source locations, addresses, thread IDs, native hash order, and localized prose. Render human text separately; logging is observational after the report is frozen.

Replay files contain versioned operands or references to exact canonical operand archives, options/platform/kernel policy, artifact/dependency payloads or content-addressed references, report, and expected first failure. Unknown engine/report/evidence/artifact versions are rejected. A digest alone is not treated as an input archive.

## 8. Dependency slicing and minimization hooks

Each typed checker exposes a read-only `verification_dependency_graph` whose nodes are stable artifact/source entities and evidence records and whose directed edges identify exact dependencies/provenance. The common API must:

- enumerate roots for the first failed invariant;
- return a deterministic transitive dependency slice in stable node/edge order;
- distinguish required semantic dependencies from diagnostic-only attribution;
- encode original IDs plus reconstruction keys needed after a reduced input receives new canonical IDs;
- validate that every graph reference belongs to the bound artifact/dependencies;
- account all graph traversal and encoded seed bytes.

Component 13 supplies slicing and a replayable `minimized_replay_seed` schema. Component 14 owns iterative delta debugging and geometric corpus minimization. Online verification does not run unbounded minimization; it may attach a dependency slice or a separately budgeted reduced seed. A minimized case is accepted only if fresh reconstruction produces the same first invariant/subcode under the same schema, not merely the same text message.

## 9. Required checker coverage

Artifact-specific plans remain normative for field-level checks. Component 13 must consolidate their invariant codes and ensure each adapter implements at least the following independent coverage.

### 9.1 Context and generic publication infrastructure

Check frozen operation/options/platform/kernel/verifier identities; strong-ID type/domain validity; dense immutable store ordering; checked resource counters and reservation/commit state; dependency graph legality; deterministic task/store ordering; canonical digest recomputation; transaction state; and report/certificate publication bindings. Test-only synthetic artifacts exercise generic infrastructure without requiring geometry stages.

### 9.2 Validated operands

Rescan raw coordinates and rings; check exact conversion, finite bits, indices, consecutive/distinct vertices, planes, projection, simple nonzero facet boundaries, triangulation coverage, edge twins/opposite uses, vertex-link cycles, shell decomposition, nonadjacent embeddedness, strict nesting, orientation/volume polarity, occupied-side semantics, bounds, provenance, owner, canonical order, and semantic encoding. The checker must not call producer topology builders or trust stored reverse incidence.

### 9.3 Exact kernel

Check integer/rational canonical forms, float-bit conversion, normalized planes/carriers/constructions, construction DAG ownership and acyclicity, predicate certificate signs under symmetry/permutation rules, substitution of constructed values into claimed carriers, symbolic perturbation ordering, and filter/exact-only equivalence. Independent replay uses exact-only operations and never the producer's cached result.

### 9.4 Candidate stream

Recompute source bounds and containment; validate inclusive touching and fallback records; independently reconstruct canonical sweep candidates; and in exhaustive mode compare with full Cartesian facet pairs to prove no false negatives. Mandatory coverage validates every emitted candidate and every pruning witness required by the registered broad-phase policy.

### 9.5 Raw events and symbolic registry

For events, require one complete classification per candidate, exact carrier/plane relation, incidence and substitution for points/intervals/regions, complete feature attribution, coplanar overlay consistency, completeness witnesses, construction normalization, operand reversal semantics, and canonical aggregation. For registry, independently form exact equivalence classes, prove equal entities merged and unequal collisions separated, verify carrier atomization, edge parameter order/twin reversal, raw-to-canonical mappings, incidence closure, angular/radial order, retained construction provenance, generations, and reconciliation requests.

### 9.6 Local refinement

Reconstruct source boundaries from registry splits; verify all point/curve constraints, exact planar segment relations, atomization, DCEL twins/stars/next/previous, weak walks and zero-area structures, face grouping/unbounded face, source-domain coverage, patch decomposition, artificial cuts, area/boundary cancellation, Euler relations, and reverse agreement across source edges. Exhaustive mode uses a separate bounded rational planar-arrangement oracle.

### 9.7 Global arrangement

Check total local/global mappings and reverse coverage, global cycles and sheet mates, exact source facet/shell reconstruction, seam atomization, complete incidence, radial layers/sectors, spherical vertex links, coincidence domains and area, transparent artificial cuts, side-transition completeness, formal probe constraints, and canonical encoding. Independent bounded oracles reconstruct planar overlays, radial orders, links, and source sheets without producer grouping helpers.

### 9.8 Cell classification

Reconstruct classification regions and validate formal probes; independently evaluate exact signed degree against both operands; use alternate deterministic directions/formal paths under the mandatory registered policy; check shell-polarity agreement, sheet and coincidence transfers, tangent transparency, seed/propagation agreement, reverse transitions, every fundamental cycle, patch-side labels, and the exterior-at-infinity label.

### 9.9 Boolean selection

Re-evaluate the frozen truth table for every patch-side occupancy pair; require selection exactly when result occupancy changes; check preserve/reverse orientation, one decision per patch, coincidence representative independence, provenance completeness, lower-dimensional exclusion, selected cycles, exactly two opposite edge uses, vertex links, closure, orientability, manifoldness, and successful empty selection. Do not duplicate operation logic outside Component 1's frozen truth function.

### 9.10 Geometry realization

Replay exact coordinates/constructions; decode candidate raw bits to exact dyadics; verify candidate-domain membership and rank, vertex distinctness, edge order, triangle signs/orientation, incidences/non-incidences, link/radial order, and global embedding obligations. Reconstruct the complete obligation universe independently and verify every witness. Bounded fixtures compare solver conclusions with brute-force assignment; resource exhaustion must never be mislabeled `output_not_representable`.

### 9.11 Output assembly and final result

Parse all `fv_surface_mesh` fields from scratch; validate field policy, index ranges before dereference, exact raw coordinate bits, face/realization bijections, cyclic orientation only, edge uses, links, components, reverse incidence, first-use indices, owner-free component/face order, dependency bindings, realization-obligation applicability, canonical bytes, and index capacity. Re-ingest nonempty output through Component 2's fresh validation path and use its explicit empty-output contract for empty success.

## 10. Final verification and public-return gate

`finalize_boolean_result<T, I>(boolean_context<T, I>&)` must:

1. Require the exact immutable `assembled_output` and all accepted prior reports from the same owner/setup and expected generations.
2. Verify registry/spec versions and that every prerequisite artifact has a complete accepted mandatory report bound to its current digest and invariant set.
3. Invoke the Component 12 checker using the mandatory final specification; do not trust the report used to publish the internal assembled artifact as the sole final check.
4. Require final topology, geometry, orientation, canonical order, realization-obligation applicability, and Component 2 re-ingestion invariants.
5. Validate report/evidence/dependency digests and perform a final cancellation/resource check.
6. Ask the context to mint the final private authority token and atomically install the report plus finalization marker at the nonthrowing publication linearization point.
7. Only after that marker exists, copy/move the immutable mesh and audit wrapper into public success.

Final verification is not recursively verified. Failure installs no marker and exposes no mesh. Repeating finalization, replacing a prerequisite, using a stale report, or presenting the wrong owner/generation fails closed. Observer logging occurs after successful installation and cannot revoke or alter the result.

## 11. Resource and complexity contract

Extend `resource_kind` and options with verifier work, scratch bytes, exact limbs, evidence records/bytes, report bytes, dependency graph nodes/edges, replay bytes, and minimization work. Every invariant metadata record documents asymptotic cost and charged units. Use checked arithmetic for all count/byte bounds.

Mandatory checkers may perform complete pair scans where required for proof. If a stage's mandatory policy uses a scalable certificate instead, that certificate format and its independent validation must be versioned and differential-tested against exhaustive bounded cases. Never silently weaken a registered invariant set for large inputs.

Reserve conservative envelopes before checker allocation. Parallel checkers use canonical chunk grants. Check cancellation before dispatch, at documented bounded loop intervals, before exact fallback/allocation/encoding, after workers join, and immediately before report return/publication. At-limit succeeds; one-over returns `resource_limit` with requested/current/limit facts. Partial reports cannot authorize publication.

## 12. Test plan

### 12.1 Registry, specifications, and authority

- Register/freeze a complete synthetic manifest and every production adapter; reject duplicate keys, missing slots, wrong code ranges, dependency cycles, unknown schemas, late registration, and non-superset exhaustive sets.
- Reject missing, extra, duplicate, unknown, and reordered decoded invariant codes; prove mandatory coverage cannot be disabled.
- Exercise pass, invariant failure, malformed artifact, stale evidence, resource failure, and verifier-defect outcomes.
- Test absent, failed, incomplete, wrong-owner/stage/slot/type/schema/generation/artifact/report/invariant/dependency certificates and duplicate publication.
- Mutate an artifact or dependency after report construction through test-only drafts and prove stale report/token rejection.
- Prove authority tokens cannot be publicly constructed, serialized, copied across contexts, or reconstructed from replay.

### 12.2 Report, evidence, diagnostics, and replay

- Golden-test `YGBVER13`, `YGBEVD13`, invariant-set and report digests on little-endian hosts plus byte-swapped simulations.
- Round-trip empty/nonempty pass and failed reports, every evidence kind, exact integers/rationals, float raw bits including signed zero, corrupted-handle locators, and dependency slices.
- Reject unknown versions/codes/enums, malformed lengths/limbs/rationals, duplicate/unsorted records, inconsistent outcome/status, invalid refs, bad digest, truncation, and trailing bytes.
- Require exact first-cause selection and `not_run_due_to_prior_failure` marking when several fields are corrupt.
- Ensure approximate decimal text, logger output, allocation address, thread ID, timestamp, locale, and hash order cannot affect canonical bytes.
- Replay identical reports across repeated processes and thread counts `1`, `2`, and a larger available count.

### 12.3 Read-only, resources, cancellation, and faults

- Compare canonical artifact/dependency bytes before and after every checker and expose only const typed views.
- Force exact-limit and one-over behavior for work, scratch, exact limbs, evidence/report/replay bytes, graph records, diagnostics, and allocation.
- Cancel before verification, during each long checker phase, before encoding, and before publication. Require no pass, authority, artifact, final marker, or committed private charge.
- Inject exceptions/allocation failures at adapter cast, reconstruction, exact replay, evidence merge, encode, digest, and prepublication points.
- Run Release/NDEBUG mutation tests to prove no gate relies on assertions.
- Run TSan with concurrent read-only verification and attempted duplicate finalization.

### 12.4 Cross-component mutation matrix

For every major Components 1-12 record field, provide at least one mutation that preserves memory safety but violates semantics and require the intended invariant/subcode. Include owner, schema, ID/range, dependency, generation, exact value, incidence, order, coverage, provenance, certificate, canonical bytes, and digest fields.

Specifically cover raw operand coordinates/rings; exact limbs/denominators/constructions; bounds/candidates; event kind/carrier/incidence; registry equivalence/order; local DCEL/coverage; global mates/radial/link/coincidence; labels/probes/paths; selection truth/orientation/topology; realization bits/domains/obligations/embedding; and every public mesh vector/mapping/order field. Ensure a checker never crashes or dereferences an invalid handle before range validation.

### 12.5 Independent oracle and metamorphic qualification

- Compare producer and checker with separately implemented bounded exact oracles for arithmetic, broad-phase exhaustive pairs, planar arrangements, source reconstruction, radial/link order, signed-degree classification, truth-table selection, realization assignment, and final mesh topology.
- Permute source vertices/facets/shells/rings, artifact stores, work shards, provenance representatives, and hash collision modes as permitted by each artifact contract.
- Exercise operand swap with operation remapping, exact translations, axis permutations, orientation-corrected reflections, positive powers-of-two scaling, and source subdivision quotient mappings.
- Vary exact filters, ambient rounding modes, worker delays, allocation addresses, and thread counts. Require artifact-equivalent reports and byte-identical canonical reports when the artifact semantic bytes are identical.
- Minimize seeded failures through the dependency graph and require fresh replay to preserve the same first invariant/subcode.

### 12.6 Final and build qualification

- End-to-end final verification covers all five operations, disjoint/overlap/nesting/equality/touching/tangency/coplanar overlap/cavities/multiple components/genus/high valence and empty results.
- Re-ingest every successful nonempty output through Component 2 and compare exact topology/orientation mappings; test the explicit empty path separately.
- Compare GCC/Clang Debug/Release report and output bytes.
- Run ASan/UBSan malformed-view, decoder, range, rollback, and mutation suites; run TSan registry-read, checker-shard, cancellation, report merge, and finalization suites.
- Register named CTests with component-specific timeouts. CI must not use `|| true` for authoritative tests.
- Inspect compile commands and use compile-time guards to prove every Boolean source is effective C++17 without fast-math.

## 13. Implementation sequence

Implement in this order:

1. Reconcile Component 1 callback, environment, report/evidence, digest, transaction, final-result, and authority interfaces; freeze terminology and eliminate digest cycles.
2. Freeze invariant numeric ranges, metadata, precedence, verification levels/specifications, registry manifest, and `YGBVER13`/`YGBEVD13` schemas with golden vectors.
3. Implement registry registration/freeze, typed synthetic adapter, immutable environment/dependency views, accounting, cancellation, exception boundary, and deterministic result merge.
4. Implement report/evidence builders, canonical codecs/digests, validators, corrupted-handle diagnostics, and private authority mint/validation.
5. Implement context/store/transaction checks and generic publication tests so Component 1 cannot publish an unchecked artifact.
6. Implement exact-kernel canonical-form, predicate, construction, and exact-only replay checks alongside Component 3.
7. Implement Component 2's typed checker and mutation matrix before integrating its production publication.
8. Add each Components 4-12 typed checker with its producer, required independent reconstruction, invariant metadata, resource model, and mutation suite before that producer becomes a prerequisite.
9. Implement dependency graph/slice APIs and replay seeds; integrate Component 14's external minimizer without putting unbounded minimization in publication.
10. Implement final nonrecursive verification, Component 2 output re-ingestion, authority marker, and public-return gate.
11. Run full cross-component oracle, replay, permutation, resource, fault, Release/NDEBUG, compiler, and sanitizer qualification.

## 14. Completion criteria

Component 13 is complete only when:

- every Components 1-12 artifact type has a frozen mandatory specification and registered typed checker;
- a pass states exactly which checks ran and every later unrun check is explicit after failure;
- producer and checker do not share unchecked derived state or producer control-flow helpers for authoritative reconstruction;
- every major artifact field has a Release-mode mutation test detected by the intended invariant;
- reports/evidence are canonical, versioned, independently replayable, and stable across processes and thread counts;
- stale artifact, dependency, evidence, report, and authority bindings are always rejected;
- diagnostics contain exact evidence/raw bits and deterministic local context rather than approximate-only prose;
- mandatory verifier costs are documented and fully accounted, and any exhaustion prevents publication;
- verification is read-only and no repair, cleanup, tolerance, or heuristic fallback exists;
- every prepublication fault rolls back with no artifact/final marker/public mesh or leaked committed charge;
- final verification alone authorizes public result return and is not recursively verified;
- all four `<T, I>` combinations pass GCC/Clang Debug/Release, deterministic replay, CTest, and applicable sanitizer suites under strict C++17 compilation.
