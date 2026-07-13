# Component 10 implementation plan: regularized Boolean boundary selection

## 1. Scope and outcome

Implement the only operation-dependent exact-topology stage. It consumes Component 9's verified immutable `labeled_arrangement<T, I>`, the retained Component 8 arrangement topology and provenance, and Component 1's frozen operation contract, then publishes one immutable `selected_exact_boundary<T, I>`.

For every atomic positive-area global patch domain, the stage evaluates the centralized Boolean truth function on the canonical negative- and positive-side occupancy pairs. It discards domains whose result occupancy does not change and retains exactly one oriented geometric representative for domains whose result occupancy changes. Every retained patch is oriented so occupied result volume is on its negative side. Selection must be a finite decision over exact labels and IDs; it performs no geometric predicate, point classification, coordinate realization, tolerance comparison, topology repair, or operation-specific geometric algorithm.

The completed component must provide:

- total, auditable decisions for every global patch, including discarded patches;
- deterministic representative selection for coincident source sheets without operand priority;
- oriented selected patch domains, cycles, edges, vertices, adjacency, and complete source/event provenance;
- exact removal of internal interfaces and regularized lower-dimensional-only contacts;
- proof that selected incidence is a closed orientable two-manifold before realization;
- an empty artifact as successful output when no positive-area result boundary exists;
- canonical encoding, diagnostics, accounting, cancellation, replay, independent verification, and transactional publication.

Component 10 does not classify cells, derive side labels, alter coincidence groups, evaluate exact Cartesian coordinates, choose `T` coordinates, triangulate for representability, simplify polygonal geometry, assign output mesh indices, or assemble an `fv_surface_mesh`. Component 11 consumes the selected exact topology and owns realization. Component 12 owns optional proof-backed simplification and final serialization.

All implementation must be self-contained in Ygor, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Consume Component 1's `boolean_context`, frozen `operation_contract::occupied(occupancy_pair)`, result-side convention, operation enum, strong IDs, owner tokens, checked arithmetic, accounting containers, deterministic executor, canonical encoder, diagnostics, traces, replay bindings, verifier registry, artifact transactions, cancellation, and typed failures. There must be no second switch on operation in Component 10.
- Consume Component 8's immutable global patch domains, canonical support-plane sides, normalized cycles, atomic edges/vertices, sheet patch uses, source-sheet members, coincidence groups, seams, radial/vertex sectors, exact domain equality, and complete provenance. Selection may project this topology but must not reconstruct it from coordinates.
- Consume Component 9's exactly one negative- and positive-side `occupancy_pair` for every `global_patch_id`, dependency bindings, transfer evidence, and classification certificate. Never repeat point location or infer labels from source orientation.
- Adapt the structural undirected-edge aggregation and directed-use checks from `src/YgorMeshesVerification.{h,cc}` only as an independent incidence-check pattern. Replace raw mesh indices and coordinate-derived edges with owner-safe global IDs and exact domain records.
- Adapt canonical adjacency traversal patterns from `src/YgorMeshesOrient.cc` only for deterministic connected-component and orientation consistency checks. Do not mutate orientation, guess an exterior, or repair cycles.
- Use `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only for optional summaries after deterministic records freeze. Never use `YLOGERR` for an expected or invariant failure.

No current Ygor implementation provides exact truth-table projection over two-sided arrangement labels, exact coincident-domain representative selection, or a certified symbolic selected boundary. Components 1, 8, and 9 are planned prerequisites and must be implemented and passing before production integration.

### 2.2 Reject as production behavior

- Do not use legacy BSP, volumetric-grid, triangle-centroid, ray-classification, facet-normal, `involved_faces`, or Boolean operation code. The broad plan explicitly excludes all existing `YgorMeshesBoolean*` implementations.
- Do not use `vec3<T>`, projected floating area, `long double`, epsilon, snapping, rounded equality, winding recomputation, randomization, hash order, pointer order, or source insertion order for selection, orientation, cancellation, or representative identity.
- Do not select source faces independently. One Component 8 `global_patch` is already the common exact positive-area domain; all coincident sheet uses over it describe the same candidate boundary.
- Do not toggle or cancel by source-sheet count, signed multiplicity, parity, operand role, operation-specific preference, or source normal. Side truth values alone determine whether the geometric domain is part of the result and which side is interior.
- Do not preserve an edge, curve, or point solely because an input event exists. Lower-dimensional contact is absent from a regularized boundary unless incident to retained positive-area patches as required topology.
- Do not merge adjacent selected patches in the baseline. Component 12 may perform optional exact-proof simplification. Keeping transparent subdivision edges is correct when their selected incidence is valid.
- Do not repair a non-manifold selected edge, duplicate domain, open cycle, contradictory label, or missing adjacency by dropping a patch or changing orientation. Fail closed as an upstream/internal invariant defect.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanSelection.h`: closed decision/orientation schemas, owner-bound selected topology references, representative/provenance records, selection certificate, immutable `selected_exact_boundary<T, I>`, artifact constants, read-only accessors, and stage entry point.
- `src/YgorMeshesBooleanSelection.cc`: dependency audit, truth-table projection, representative choice, selected topology projection, incidence/orientation validation, canonical encoding, coordinator, verifier adapter, and four `<T, I>` explicit instantiations.
- `tests/Test_MeshesBooleanSelection.cc`: focused truth-table, orientation, coincidence, topology, failure, mutation, encoding, and rollback tests.
- `tests/Test_MeshesBooleanSelectionProperties.cc`: Boolean-identity, analytic, generated, permutation, operand-swap, operation, sharding, and schedule tests.
- `tests/MeshBooleanSelectionFixtures.h`: synthetic arrangement/labeled artifacts, exact patch/coincidence builders, independent truth-table and selected-incidence oracle, analytic solids, mutation helpers, deterministic PRNG, and replay records.

Modify as required to reconcile prerequisites rather than creating parallel facilities:

- `src/YgorMeshesBooleanContract.{h,cc}`: add selection-specific strong IDs/references, limits, accounting categories, diagnostics, invariant codes, artifact slot metadata, and encoding support listed below.
- `src/YgorMeshesBooleanGlobalArrangement.h`: only reconcile read-only exact domain, oriented cycle, edge/vertex incidence, sheet-use, coincidence, and provenance accessors. Do not add operation or selected state to Component 8.
- `src/YgorMeshesBooleanCellClassification.h`: only reconcile total per-patch side-label access and operation-independent semantic dependency digest. Do not add cached operation results to Component 9.

Use namespace `ygor::mesh_boolean`. Keep provisional decisions, representative candidate ranges, selected-incidence maps, worker shards, and verifier implementations private to the `.cc` or tests. Public templates remain thin and use explicit instantiation for `float`/`double` with `std::uint32_t`/`std::uint64_t`.

Add the selection `.cc` to `src/CMakeLists.txt`. Although selection performs no floating arithmetic, retain Component 1's strict compilation policy consistently. Add both tests to CTest and `tests/compile.sh`, register `MeshBooleanSelection.Unit` and `.Properties`, and label them `mesh_boolean;component10`. Add authoritative GCC/Clang Debug/Release, ASan/UBSan malformed-artifact/rollback, and TSan decision-shard/cancellation/publication runs without `|| true`.

Before coding, reconcile these prerequisite interfaces:

1. Extend Component 1 with non-convertible `patch_selection_decision_id`, `selected_patch_id`, `selected_cycle_id`, `selected_halfedge_id`, `selected_edge_id`, `selected_vertex_id`, and `selection_certificate_id`, or artifact-local references with precisely those domains. Never reuse raw `global_*` IDs as selected-store indices.
2. Extend resource policy with explicit or documented aggregate limits for patch decisions, representative candidates/evidence, selected patches/cycles/halfedges/edges/vertices, adjacency entries, provenance references, certificate facts, verifier scratch, and canonical bytes. Bound rejected decisions too because they are authoritative certificate data.
3. Component 1's operation contract must expose one constexpr/noexcept audited truth function for all five operations and reject unknown operation values at setup/decode. Component 10 calls it twice per patch; it does not duplicate formulas.
4. Component 8 must expose one canonical geometric domain per `global_patch_id`, all separate covering sheet uses, coincidence-group membership, normalized cycles and holes, directed atomic edge uses, vertices, support-plane orientation, and exact provenance. Each positive-area domain must occur exactly once regardless of source multiplicity.
5. Component 9 must expose exactly one owner-checked negative and positive occupancy pair per global patch, with unambiguous mapping to Component 8's canonical plane sides and operation-independent dependency identity.
6. The verification environment must expose immutable Components 8/9 and the frozen operation contract to Component 10's typed verifier without globals or unbudgeted copies.

## 4. Stage and ownership contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<selected_exact_boundary<T, I>>>>
select_boolean_boundary(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the exact published `labeled_arrangement<T, I>` from `artifact_slot::labeled_arrangement`, its strongly retained `arrangement_complex<T, I>` and earlier exact dependencies, the frozen operation contract, matching owner/setup/kernel policies, and the registered Component 10 verifier specification.
2. Reject stale, copied, replacement, wrong-generation, wrong-role, operation-dependent Component 9 data, or mismatched Component 8/9 dependencies even when semantic bytes happen to match.
3. Validate dense IDs, total patch-to-side-label mappings, canonical side conventions, cycles, domain/coincidence membership, sheet-use provenance, and reverse ranges before evaluating any decision.
4. Open one `boolean_selection` transaction targeting `artifact_slot::selected_exact_boundary`. No decision, selected ID, report, diagnostic, or charge becomes visible before complete verification.
5. Evaluate all patches against exactly the context's frozen operation; never accept an operation parameter that can disagree with the context or artifact framing.
6. Normalize, assign IDs, encode, independently verify, check cancellation, and atomically publish only after every source patch has one decision and selected topology is closed and orientable.

The artifact retains strong typed dependencies on the labeled arrangement, global arrangement, symbolic/exact construction storage needed by selected handles, and accepted verification report. It contains no borrowed mesh pointer, coordinate realization, approximate value, mutable worklist, hash bucket order, provisional index, or output mesh index.

## 5. Selection semantics and schemas

### 5.1 Central truth-table projection

For each `global_patch_id p`, retrieve canonical labels `L-=(a-, b-)` and `L+=(a+, b+)`, where minus/plus refer only to Component 8's exact support-plane sides. Evaluate:

```cpp
const bool r_negative = operation_contract.occupied(L_minus);
const bool r_positive = operation_contract.occupied(L_plus);
```

The complete decision table is:

| `r_negative` | `r_positive` | decision | selected orientation |
|---|---|---|---|
| false | false | discard, exterior/non-result interface | none |
| true | true | discard, internal interface | none |
| true | false | select | preserve canonical patch orientation |
| false | true | select | reverse canonical patch orientation |

“Preserve” means emit each canonical cycle direction for which the canonical plane normal has result interior on the negative side. “Reverse” reverses the oriented surface: reverse every boundary cycle direction, swap next/previous, and preserve each cycle's outer/hole role under the documented oriented-domain convention. Do not reverse merely a source sheet use; orient the geometric patch domain itself.

This table is the only selection rule. Equal side result occupancy discards the domain even when operand labels differ, source sheets cross, or coincidence multiplicity is nonzero. Differing result occupancy selects exactly one domain even when several source sheets cover it.

### 5.2 Decision records and certificates

Freeze closed enums for decision kind (`discard_exterior`, `discard_internal`, `select_preserved`, `select_reversed`), selected orientation, representative reason, provenance role, selected edge incidence kind, and selection invariant. Reject unknown values during decode.

Publish one `patch_selection_decision` for every Component 8 global patch in `global_patch_id` order. It stores:

- decision ID and owner-checked source patch reference;
- negative/positive Component 9 side-label references and both occupancy pairs;
- the two truth-table result bits and operation-contract formula/version;
- selected/discarded status and orientation transform;
- coincidence group, covering source uses, selected representative/provenance reference, and normalized evidence digest;
- selected patch reference when retained, absent when discarded.

The decision store is total even for an empty result. This permits independent audit of internal-interface and regularization removals without recomputing producer state.

Publish a `boolean_selection_certificate` containing dependency and operation-contract digests, all four side-result case counts, decisions by kind, selected topology counts, coincidence representative facts, lower-dimensional incidence facts, closed-edge/orientation/component facts, provenance totals, resource facts, and one combined semantic digest.

### 5.3 Coincident representative and provenance policy

There is one selection decision per Component 8 geometric patch, not per source use. For a patch with multiple covering sheets:

1. First evaluate side result occupancy. If equal, discard the geometric domain and do not choose an output representative.
2. If unequal, retain the single `global_patch` domain and all sorted covering provenance. A representative record identifies the least canonical covering `sheet_patch_use` by a versioned complete semantic key only for trace attribution.
3. The key may include owner-free source-sheet member identity, operand role, shell/facet provenance, orientation parity, and coverage range after exact domain equality is already established. It must not affect selected geometry, result orientation, or whether the patch is selected.
4. If no source use covers a positive-area global patch, or a listed use does not exactly cover it, fail as an upstream invariant defect. A valid selected patch always has at least one complete source derivation.
5. Preserve every non-representative source use as provenance. Do not cancel, toggle, or delete it from evidence.

Opposite-facing and same-facing A/B coincidences therefore need no special Boolean geometry code. Their Component 9 labels drive the same four-row table. Equal operands, cavity-wall coincidences, and partially coincident domains are handled atom by atom after Component 8 common subdivision.

### 5.4 Selected topology projection

Build selected topology from retained geometric domains without geometric recomputation:

1. Create one `selected_patch` per selected decision, storing source `global_patch_id`, orientation transform, oriented cycle range, side result labels, representative record, all provenance, and certificate reference.
2. Create one `selected_vertex` for each Component 8 `global_vertex_id` referenced by a selected cycle. Store the original symbolic vertex handle and sorted selected/source incidence. Do not include isolated arrangement vertices.
3. Create one `selected_edge` for each Component 8 `global_atomic_edge_id` used by selected patch boundaries after orientation. Store its two directed `selected_halfedge` uses, endpoint IDs, source edge/seam provenance, and exact carrier handle. Do not create edges supported only by discarded patches or isolated contacts.
4. Materialize oriented cycles by remapping Component 8 halfedge/domain boundary uses. Reverse selected patches atomically: directed endpoints, next/previous, and cycle order must all agree. Canonically rotate each oriented cycle to its least complete immutable key after orientation.
5. Build reverse adjacency solely from the frozen selected forward uses. Every selected geometric edge must have exactly two oppositely directed uses belonging to two local boundary sides of the selected surface. The two uses may originate from distinct source operands or coincidence members, but adjacency is by global geometric edge identity.
6. Build selected connected components by canonical traversal of patch adjacency. Orientation consistency is local and must hold before component assignment; no flood-fill may flip patches.

Retain seam vertices and edges whenever they occur in selected patch cycles, including transparent subdivision boundaries needed by the exact patch representation. Exclude every arrangement seam/edge/vertex with no selected positive-area incidence. Thus isolated contact curves and points disappear under regularization without a separate cleanup pass.

The baseline does not merge coplanar patches or remove degree-two seam vertices. Any later simplification must be owned by Component 12 and prove exact domain equality, simple cycles, embedding, and realization obligations.

## 6. Selection algorithm

### 6.1 Dependency audit and decision generation

1. Validate all artifact bindings and prove a bijection between Component 8 global patches and Component 9 per-patch side-label records.
2. Validate each patch's canonical support-plane side convention, positive exact area certificate, normalized cycles, source-use coverage, and coincidence mapping without evaluating coordinates.
3. Partition global patches by a versioned canonical frontier. For each patch, retrieve immutable labels, call `operation_contract::occupied` for both sides, classify through the four-row decision table, and form one private provisional decision.
4. For selected coincident patches, choose only the provenance representative by the frozen semantic key and retain all source uses. For discarded patches, record why no representative is needed.
5. Merge worker shards by `global_patch_id`, not completion order. Require exactly one decision per patch and choose the canonical first failure under Component 1's policy.

Truth-table evaluation is constant work per patch and should initially be serial unless parallelism measurably helps large artifacts. A parallel implementation must not allocate final IDs or publish from workers.

### 6.2 Topology projection and orientation

1. Enumerate selected decisions in canonical source-domain order and derive all oriented cycle uses according to preserve/reverse status.
2. Collect referenced global vertices and edges, pre-rank complete immutable remapping keys, and assign dense selected IDs. Never sort with a fallible exact comparison; all needed geometric ranks already come from Component 8.
3. Rewrite cycles and halfedges to selected IDs, canonicalize cycle rotations, and verify endpoints, directions, next/previous involution, outer/hole role, and source-domain coverage.
4. Group directed uses by source `global_atomic_edge_id`. Require exactly two uses, opposite endpoint order, compatible exact carrier/domain incidence, and opposite induced boundary orientation.
5. Build patch adjacency and connected components. Require every patch reachable through its recorded cycles, every selected vertex incident to selected edges/patches, and no selected topology record lacking positive-area support.
6. Cross-check every selected edge against Component 8 seam/source-edge/transparent topology and every omitted lower-dimensional feature against zero selected patch incidence.

If an exact regularized result should be closed, any edge use count other than two, orientation mismatch, non-closing cycle, duplicate positive-area patch domain, or isolated selected record is `internal_invariant_error`. Do not classify it as an output representability problem; no rounding has occurred.

### 6.3 Certificate, verification, and publication

1. Recompute counts and decision partitions; prove total decisions equal global patches and selected plus discarded counts exhaust the input.
2. Prove all selected side pairs differ and all discarded side pairs agree under the frozen operation contract.
3. Prove selected domains are unique, topology is closed/orientable, and every selected/omitted lower-dimensional record has exactly the incidence implied by retained patches.
4. Freeze canonical IDs, provenance ranges, topology, decision records, and certificate facts.
5. Encode semantic and invocation-bound payloads, invoke the independent verifier, compare certificate/report bindings, perform a final cancellation check, and publish atomically.

An arrangement with no patches, or an operation selecting no patches, publishes a valid artifact with empty selected topology, a total empty or all-discarded decision store as applicable, and a passed certificate. It is not `output_not_representable` or an error.

## 7. Exact invariants and failure handling

Before a draft can succeed, prove:

1. Every Component 8 global patch has exactly one Component 9 negative/positive label pair and exactly one Component 10 decision.
2. Every decision's result bits are exactly Component 1's frozen truth function applied to its recorded occupancy pairs.
3. A patch is selected if and only if its two result bits differ; orientation places `true` on the selected patch's negative side.
4. Every positive-area geometric domain occurs at most once in selected topology regardless of source-use or coincidence multiplicity.
5. Representative choice occurs only after selection, uses a complete deterministic semantic key, has no geometric effect, and preserves all alternate provenance.
6. Every selected cycle is an orientation-correct projection of one Component 8 exact domain cycle; every selected edge has exactly two opposite directed uses and every selected vertex/edge has positive-area patch incidence.
7. No discarded-only seam, isolated contact curve, or point appears in selected topology, while every edge/vertex needed by a selected patch remains present.
8. Selected connected components are closed and orientable before coordinate realization; no operation-dependent repair or simplification occurred.
9. Empty selection is valid and complete.
10. IDs, decisions, representative records, selected topology, canonical bytes, selected first failure, diagnostics, and certificates are independent of hash behavior, allocation, worker completion, thread count, and exact filter/cache state for one frozen context.

Return `resource_limit` for declared bytes/work/entity/certificate/verifier limits, cancellation, allocation failure, or verifier exhaustion. Return `internal_invariant_error` for stale/malformed dependencies, missing or contradictory labels, unknown operation/schema values, incomplete source-use coverage, duplicate domains, invalid cycles, non-manifold selected incidence, orientation contradiction, producer/verifier disagreement, or encoding mismatch. Component 10 does not return `input_contract_error`, `index_overflow`, or `output_not_representable`.

Before work, use checked arithmetic to bound decisions, representative candidates, selected patch/cycle/halfedge/edge/vertex mappings, adjacency, provenance references, certificate entries, canonical bytes, and verifier duplication. Reserve conservative accounting envelopes before constructing shards. Never omit a decision, source provenance item, topology use, or verification check to fit a limit.

Check cancellation before dependency validation, each patch frontier, shard merge, topology projection, edge grouping, component traversal, encoding, verification, and publication. Catch exceptions only at task/stage boundaries, join siblings, select the canonical failure, and roll back. Diagnostics include operation, decision/patch/side-label/coincidence/sheet-use/edge/vertex IDs, both occupancy pairs and result bits, expected/actual incidence, dependency/policy digests, resource facts, and replay token.

## 8. Canonical encoding and deterministic execution

Define `YGBCAN10` as operation-specific selected-boundary semantics over one frozen labeled arrangement: schema/type versions; operation and truth-table formula version; operation-independent Component 9 semantic digest; all patch decisions in source patch order; recorded side occupancy/result bits; selected orientation transforms; representative and complete provenance records; selected vertices/edges/halfedges/cycles/patches/components; certificate facts; and deterministic semantic counts.

Exclude owner tokens, pointers, setup invocation ordinal, worker/provisional indices, diagnostics, traces, timings, caches, hash layout, exact filter-attempt state, and realized coordinates. Unlike Component 9 encoding, operation is authoritative semantic content: different operations may produce different `YGBCAN10`, while repeated selection for one frozen operation must be byte-identical.

Define `YGBSEL10` as invocation-bound framing with schema/type versions, setup/operation-contract digest, exact dependency identities/generations/digests, deterministic statistics, length-prefixed `YGBCAN10`, exact construction-storage binding, and verification report/certificate binding. Wrap it with Component 1's `YGBART01` framing for `selected_exact_boundary`. Decode rejects unknown enums/versions, wrong operation, malformed owners/IDs/ranges, missing/duplicate decisions, invalid booleans, inconsistent orientation transforms, broken cycles/edge incidence, unsorted provenance, bad lengths, and trailing bytes.

Partition decision work by canonical global-patch ranges independent of thread count. Workers write private accounting-backed decision shards and cannot assign selected IDs, build shared adjacency, or publish. The coordinator merges by source patch ID, chooses provenance representatives, projects topology, assigns IDs, encodes, verifies, and publishes.

All standard comparators inspect immutable IDs, precomputed ranks, and fixed semantic fields and are `noexcept`. Hash tables are lookup accelerators only. For one frozen operation and dependencies, thread count, shard boundaries, worker delays, allocation addresses, and hash collision mode produce byte-identical artifact bytes and equivalent diagnostics.

## 9. Mandatory independent verifier

Register a stable Component 10 artifact tag, schema/checker versions, and invariant set with Components 1/13. The read-only verifier receives the candidate, immutable Components 8/9, frozen operation contract, accounting, and cancellation. It must not call producer decision batching, representative selection, selected-topology projection, adjacency construction, ID assignment, certificate, or encoding helpers.

Mandatory checks:

1. Validate owner, slot/tag/schema, strong dependency identities/generations/digests, operation binding, policy versions, dense IDs, enum domains, ranges, role-qualified provenance, and construction owners.
2. Independently join Component 8 patches to Component 9 side labels and require total one-to-one coverage.
3. Independently evaluate Component 1's truth function for both sides of every patch and compare decision kind, selected status, and orientation. Do not trust stored result bits.
4. Independently verify every selected coincident domain appears once, derive the least provenance representative under a separately implemented comparison, and compare complete retained source-use coverage.
5. Rebuild every oriented selected cycle directly from Component 8 plus the verified orientation bit. Compare selected vertices, halfedges, edges, patches, and reverse incidence without trusting stored maps.
6. Group rebuilt edge uses independently and require exactly two opposite uses, closed cycles, orientation consistency, unique positive-area domains, no isolated selected records, and exact exclusion of lower-dimensional-only incidences.
7. Recompute selected connected components and all certificate facts/statistics.
8. Independently encode `YGBCAN10`, `YGBSEL10`, and `YGBART01`; verify report, replay, trace, and construction-storage bindings separately.

Verifier resource exhaustion prevents publication with `resource_limit`; any semantic disagreement is `internal_invariant_error`. Exhaustive verification may additionally compare bounded artifacts against a deliberately simple oracle that scans all patches, applies explicit test-only truth tables, and constructs an unordered exact-domain boundary multiset without sharing producer control flow.

Mutation tests alter every owner/ID/range, side-label reference/value, result bit, operation/formula version, decision kind, orientation, selected/source mapping, representative/provenance member, cycle role/order/next/previous, halfedge endpoint, edge incidence, component membership, dependency binding, certificate fact, and serialization field/order. Every mutation must fail in Release/NDEBUG.

## 10. Test plan

### 10.1 Truth tables and orientation

- Exhaust all 16 pairs of negative/positive `occupancy_pair` values for union, intersection, `A-B`, `B-A`, and symmetric difference. Check the centralized contract result, all four decision rows, and preserve/reverse orientation.
- For every operation, test patch canonical normal parity independently of source facet orientation and operand role. Result interior must always map to selected negative side.
- Test labels that differ by one or both operand components but map to equal result occupancy; these are discarded internal/exterior interfaces.
- Test malformed missing, duplicate, swapped-side, stale, and operation-dependent labels; all fail before publication.

### 10.2 Coincidence and regularization

- Equal same-facing solids: union/intersection retain one boundary domain; both differences and xor are empty.
- Equal opposite-facing/cavity-wall configurations, duplicate agreeing source records, and deliberate contradictory upstream labels/provenance.
- Partial coplanar overlap with common subdivision, containment, coincident cavity walls, and a transverse seam entering/leaving a coincident group. Decisions are per atomic domain and never per source count.
- Tangent point, tangent edge, face contact, disjoint solids, nested solids, and boundary-only contact. Isolated points/curves are absent unless incident to selected positive-area patches.
- Three or more coincident source uses in permuted orders. Representative attribution is canonical, all provenance survives, and selected geometry is unchanged.

### 10.3 Selected topology

- Empty arrangement, all-discarded result, one closed shell, multiple disconnected shells, nested cavity boundaries, genus-bearing shells, and selected patches with outer and hole cycles.
- Preserved and reversed patches with multi-edge cycles; verify endpoint reversal, next/previous, cycle rotation, outer/hole convention, and selected-side labels.
- Edges where selected adjacency changes source operand across an intersection seam and where both uses derive from a coincident group.
- Transparent source tessellation/artificial boundaries retained in baseline selected topology, proving closed incidence without optional merging.
- Inject one missing/extra patch, duplicate domain, unmatched edge, same-direction pair, broken cycle, isolated edge/vertex, or false selected contact and require stable invariant diagnostics.

### 10.4 Boolean identities and metamorphic tests

- At exact-boundary domain/orientation level test idempotence, `A-A=empty`, commutativity for union/intersection/xor, operand-order mapping for differences, absorption, distributive identities on bounded fixtures, and xor identities.
- Test disjoint, overlapping, nested, cavity, equal, touching, tangent, transverse, and coincident analytic solids for all operations and both operand orders.
- Compare decisions with an independent patch-wise truth-table oracle and selected topology with an independent exact-domain incidence multiset.
- Swap operands and map role-qualified provenance: union/intersection/xor selected geometry and orientation remain equivalent; `A-B` maps to `B-A` after operation remapping.
- Apply exactly representable translations, axis permutations, orientation-corrected sign flips, and positive power-of-two scaling. Selection combinatorics and truth evidence remain equivariant without using coordinates.
- Permute source vertices/facets/shells, arrangement insertion, coincidence members, side-label record order, and valid source subdivision. Compare through canonical upstream maps and quotient topology where subdivision legitimately differs.
- Vary thread count, shard boundaries, worker delays, hash seed/collision mode, allocation addresses, and exact filter/cache state; require identical IDs, decisions, failures, diagnostics, and bytes for one frozen operation.

### 10.5 Failure, resource, and qualification tests

- Wrong owner, stale/replacement dependency, mismatched arrangement/classification digest, unknown operation, invalid side convention, incomplete patch coverage, contradictory decision, invalid representative, malformed cycle, duplicate selected domain, open/non-manifold edge, and verifier/encoding disagreement all fail closed.
- Exact-at-limit and one-over decision/representative/provenance/patch/cycle/halfedge/edge/vertex/adjacency/certificate/private-byte/work/diagnostic/trace cases; allocation failure and cancellation at every phase expose no partial artifact.
- Fault injection before verification/publication leaves no selected-boundary slot or committed selection charge. Post-publication observer failure cannot mutate the artifact.
- Debug/Release and GCC/Clang canonical outputs match. ASan/UBSan cover malformed references/ranges and rollback; TSan covers decision shards, cancellation, verifier, diagnostics, and publication.
- Benchmark very large all-discarded arrangements, all-selected arrangements, seam-heavy topology, and coincidence-heavy provenance. Record decision throughput, topology-map work, verifier work, canonical bytes, and peak private memory without weakening checks.
- Serialize failures with operation, source coordinate bits through dependency replay, patch/label/coincidence/sheet/edge IDs, occupancy/result bits, dependency/policy digests, expected/actual incidence, PRNG state, and replay token.

## 11. Component 11 handoff contract

Before Component 11 integration, prove it can consume only `selected_exact_boundary<T, I>` and retained exact dependencies to:

- enumerate every selected symbolic vertex once and every oriented exact patch/cycle/edge incidence completely;
- know the exact result-interior side of every patch without re-evaluating the Boolean operation;
- recover canonical symbolic vertex and exact carrier/domain handles needed to build realization constraints;
- preserve shared selected vertex/edge identity globally across source operands and coincident derivations;
- access complete provenance and selection certificates for representability diagnostics;
- recognize empty selection as a successful empty realization request.

Component 11 must not reconsider discarded patches, choose a different coincident source representative, reverse selected orientation, or repair non-manifold topology. Component 10 supplies exact combinatorics and orientation, not `T` coordinates or a preferred triangulation.

## 12. Implementation sequence and completion criteria

Implement in this order:

1. Reconcile Components 1/8/9 operation, IDs, side-label, topology/provenance, resource, verifier-environment, and dependency interfaces; freeze Component 10 enums, policy versions, invariant codes, and encodings.
2. Implement immutable decision, selected topology, representative/provenance, certificate, and owner-checked accessor schemas with canonical encode/decode tests.
3. Implement serial total truth-table projection for all patches and exhaustive 16-side-pair tests for every operation and orientation.
4. Implement deterministic coincident provenance representative attribution while preserving all source uses; add equal/opposite/partial coincidence tests.
5. Implement selected vertex/edge/halfedge/cycle/patch projection, preserve/reverse mapping, closed incidence, connected components, and lower-dimensional exclusion checks.
6. Add checked resource envelopes, optional deterministic decision workers/merge, cancellation, diagnostics, replay, and transaction rollback.
7. Implement the independent verifier, canonical encodings/digests, mutation suite, certificate gate, and atomic publication.
8. Run Boolean identity, analytic/oracle, operand-swap, transform/subdivision, operation, schedule/hash, resource/cancellation, compiler, sanitizer, replay, and benchmark qualification before Component 11 integration.

Component 10 is complete only when:

- every global patch has one auditable decision produced solely by Component 1's frozen truth function over Component 9's canonical side labels;
- selected status is exactly side-result inequality and every retained patch has result interior on its negative side;
- coincident source multiplicity never changes geometry or orientation, one deterministic attribution representative is recorded, and all provenance is retained;
- equal-result interfaces and lower-dimensional-only contacts are absent while every seam edge/vertex needed by selected positive-area patches remains;
- selected domains are unique and selected cycles/edges/vertices form a certified closed orientable exact boundary before realization;
- empty results publish successfully with complete decision/certificate evidence;
- no floating coordinate, tolerance, geometric reclassification, operation-specific geometry, heuristic cancellation, repair, or optional simplification enters selection;
- independent truth, representative, topology, incidence, certificate, and encoding verification passes in Release as well as Debug;
- exhaustive truth-table, coincidence, Boolean-identity, mutation, resource/cancellation rollback, replay, compiler, sanitizer, and schedule-determinism suites pass;
- Component 11 can construct all realization obligations from the immutable selected exact boundary without revisiting selection semantics.
