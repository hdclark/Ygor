# Component 9 implementation plan: exact cell and side classification

## 1. Scope and outcome

Implement the operation-independent stage that consumes Component 8's verified `arrangement_complex<T, I>`, its retained exact and validated dependencies, and the frozen classification policy, and publishes one immutable `labeled_arrangement<T, I>`. The stage must assign exact `(inside_A, inside_B)` occupancy to every conservative open-region fragment represented by the arrangement and to both canonical sides of every atomic patch.

The completed component must provide:

- an explicit, owner-safe region model over Component 8's conservative side-graph components, without claiming that disconnected fragments are one maximal 3D cell;
- exact classification of every supplied formal open probe against both complete validated operands, with no finite normal offset or tolerance;
- exact signed ray/degree evidence, stable-ID symbolic handling of facet/edge/vertex degeneracy, and alternate-query cross-checks;
- checked transfer semantics for individual oriented sheets, coincident sheet groups, tangencies, and transparent adjacency;
- deterministic graph propagation with predecessor evidence, complete cycle checks, and two-path contradiction diagnostics;
- canonical negative- and positive-side occupancy for every `global_patch_id` and `patch_side_id`;
- an exterior-at-infinity certificate, optional proof-backed equivalence of over-segmented fragments, and exact arbitrary-probe validation hooks;
- independent verification, canonical encoding, diagnostics, accounting, cancellation, replay, and transactional publication.

Component 9 does not evaluate a Boolean truth table, select or orient result patches, cancel coincident representatives, realize coordinates, simplify geometry, or assemble an `fv_surface_mesh`. Component 10 consumes labels and owns all operation-dependent decisions.

All implementation must be self-contained in Ygor, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Consume Component 1's `boolean_context`, `occupancy_pair`, strong IDs, owner tokens, checked arithmetic, accounting containers, deterministic executor, canonical encoder, diagnostics, traces, replay bindings, verifier registry, transactions, cancellation, and typed failures.
- Consume Component 2's immutable exact operands, shell forest, oriented facet polygons/triangulations, twins, vertex links, exact bounds, and proof that the occupied material side of every accepted oriented source facet is its negative side. Do not infer nesting or orientation again from rounded geometry.
- Consume Component 3's exact numbers, points, planes, rays, affine constructions, rich ray/polygon relations, neutral `oriented_shell_view`, `generic_ray_v1`, `ray_boundary_owner_v1`, formal polynomial comparisons, stable-ID perturbation, and exact-only verifier path.
- Consume Component 8's global patches, separate source-sheet uses, canonical plane sides, seams/radial sectors/vertex sectors, coincidence groups, region-preserving transitions, crossing transitions, conservative open-region components, and certified `open_probe_descriptor`s. Classification must not reconstruct missing seams, radial order, spherical links, or planar coincidence overlays.
- Adapt only the deterministic graph-control pattern from `src/YgorMeshesOrient.cc`: canonical adjacency construction, worklist propagation, predecessor recording, and contradiction detection. Replace raw indices, mutable repair, approximate geometric seeding, and orientation mutation with immutable typed IDs and exact transfer checks.
- Use `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only to forward optional summaries after deterministic records freeze. Never use `YLOGERR` for an expected or invariant failure.

No current Ygor implementation provides exact classification of formal open probes, signed source-boundary ownership, classification of a complete nested-shell operand, coincident-group transfers, or path-certified arrangement labeling. Components 1-8 are planned production prerequisites and must be implemented and passing before Component 9 production integration.

### 2.2 Reject as production behavior

- Do not use BSP classification, tetrahedralization ray casting, `fv_surface_mesh::involved_faces`, legacy Boolean facet classification, or outward-orientation heuristics. They use floating offsets, epsilons, native division, arbitrary rays, randomization, triangulation-dependent counts, or mutable repair.
- Do not use `vec3<T>`, `long double`, normalized floating normals, a facet barycenter rounded to `T`, `p + epsilon*n`, jitter, random rays, nearest-point tests, distance tolerances, or AABB-center guesses for occupancy.
- Do not classify a boundary point as inside through an inclusive point-in-mesh predicate. Every Component 8 descriptor denotes an open formal point, and its unperturbed base-stratum incidence remains explicit evidence.
- Do not toggle occupancy once per incident facet. Source-edge/vertex hit groups are resolved by exact boundary ownership, and a coincident group is transferred once from its complete oriented member vector.
- Do not infer that every Component 8 conservative graph component is a maximal global cell, nor merge fragments because their labels happen to agree. Equal occupancy does not prove complement connectivity.
- Do not let requested Boolean operation, patch preference, source multiplicity, hash order, queue order, thread schedule, filter path, or selected ray determine classification IDs or labels.
- Do not repair incomplete Component 8 adjacency or contradictory Component 2 orientation. Fail closed against stable upstream IDs.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanCellClassification.h`: closed classification schemas, owner-bound region references, occupancy and side-label records, seed/propagation/exterior certificates, immutable `labeled_arrangement<T, I>`, artifact constants, read-only accessors, and the stage entry point.
- `src/YgorMeshesBooleanCellClassification.cc`: dependency validation, formal-probe classification, signed degree evaluation, crossing transfer construction, deterministic propagation, conflict-path reconstruction, side-label assembly, canonical encoding, coordinator, verifier adapter, and four `<T, I>` explicit instantiations.
- `tests/Test_MeshesBooleanCellClassification.cc`: focused seed, transfer, propagation, side-label, failure, mutation, encoding, and rollback tests.
- `tests/Test_MeshesBooleanCellClassificationProperties.cc`: generated oracle, ray-choice, operand-swap, transformation, permutation, sharding, and schedule tests.
- `tests/MeshBooleanCellClassificationFixtures.h`: synthetic validated/global artifacts, formal-probe and shell builders, independent low-complexity exact degree oracle, analytic solids, deterministic PRNG, mutation helpers, and replay records.

Modify as required to reconcile prerequisites rather than creating parallel facilities:

- `src/YgorMeshesBooleanContract.{h,cc}`: add the strong ID/reference domains, feature references, limits, accounting categories, diagnostics, invariant codes, and encoding support listed below.
- `src/YgorMeshesExactKernel.{h,cc}`: add the formal-origin operand point-location and exact arbitrary-probe interfaces in Section 5 if Component 3 has not already exposed them; extend Component 3 tests simultaneously.
- `src/YgorMeshesBooleanGlobalArrangement.h`: only reconcile concrete read-only region-component, transition, crossing, and probe descriptor types. Do not move occupancy into Component 8.

Use namespace `ygor::mesh_boolean`. Keep ray-hit groups, signed-degree accumulators, propagation worklists, provisional paths, worker shards, and verifier implementations private to the `.cc` or tests. Public templates remain thin and use explicit instantiation for `float`/`double` with `std::uint32_t`/`std::uint64_t`.

Add the classification `.cc` and any extended kernel `.cc` to Component 1's named strict-arithmetic source list in `src/CMakeLists.txt`; effective fast-math and contraction must be disabled and compile-time guarded. Add both tests to CTest and `tests/compile.sh`, register `MeshBooleanCellClassification.Unit` and `.Properties`, and label them `mesh_boolean;component9`. Add authoritative GCC/Clang Debug/Release, ASan/UBSan malformed-artifact/rollback, and TSan seed-shard/propagation/cancellation/publication runs without `|| true`.

Before coding, reconcile these prerequisite interfaces:

1. Extend Component 1 with non-convertible `classification_region_id`, `classification_transition_id`, `seed_certificate_id`, and `propagation_path_id`, or owner-safe artifact-local references with those domains. Retain `cell_id` as the public conceptual alias only if its documentation states that Component 9 uses it for a conservative open-region fragment, not necessarily a maximal 3D cell.
2. Extend resource policy with explicit or documented aggregate limits for classification regions, directed crossing arcs, seed queries, ray-hit groups/evidence, propagation queue entries, predecessor/path records, side labels, equivalence/exterior certificates, verifier probes, certificate facts, and classification scratch. The existing `cells` limit alone must have an exact documented relationship to all authoritative records.
3. Component 3 must accept a Component-8-neutral formal open-point view consisting of exact base point, finite rational direction polynomial, perturbation key/formula, and complete signed constraints. It must classify that formal point against a complete `oriented_shell_view`/operand without choosing a finite epsilon.
4. Component 3 point location must return a signed grouped boundary degree, not only a parity bit: complete source facet/edge/vertex hit ownership, formal ray parameters, unperturbed incidences, selected symbolic decisions, and evidence sufficient to prove the final total is exactly `0` or `1` under Component 2's accepted nested-shell policy.
5. Component 8 must expose every conservative component and every transition exactly once in canonical order, directed crossing endpoints, patch-side membership, member-use occupied-side mapping, and a valid formal probe for each component. It must distinguish individual and coincident crossings from tangent/incidence/transparent transitions.
6. The verification environment must expose exact-only kernel services and immutable Component 2/8 dependencies to the Component 9 typed verifier without globals or unbudgeted copies.

## 4. Stage, ownership, and region model

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<labeled_arrangement<T, I>>>>
classify_arrangement_cells(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the exact published `arrangement_complex<T, I>` from `artifact_slot::arrangement_complex`, its strongly retained `validated_operands<T, I>` and exact construction storage, matching owner/setup/kernel policies, and the registered Component 9 verifier specification.
2. Reject stale, copied, replacement, wrong-generation, wrong-role, or operation-dependent arrangement dependencies even if a digest happens to match.
3. Validate dense IDs, all patch-side/component memberships, transition endpoint/kind symmetry, crossing provenance, source-use occupied sides, probe constraints, and reverse ranges before point location.
4. Open one `cell_classification` transaction targeting `artifact_slot::labeled_arrangement`; no region, label, seed, path, report, or diagnostic becomes visible before complete verification.
5. Classify and propagate independently of the requested Boolean operation. All five operations over the same frozen arrangement produce identical `YGBCAN09` bytes.
6. Normalize IDs and evidence, independently verify, check cancellation, and atomically publish only after every component and patch side has one label and all crossing/cycle checks agree.

Treat each Component 8 conservative region-preserving connected component as one `classification_region`. Assign it a dense `classification_region_id`/`cell_id` without asserting maximal global connectivity. Every Component 8 side, seam sector, source-edge sector, and vertex sector maps to exactly one such region. Every region receives exactly one occupancy pair.

Optional global same-cell proofs may create a separate canonical `region_equivalence` relation. Merge only after an exact path in the open complement is certified against all arrangement sheets; labels must already agree. Keep the original region IDs and mappings even when an equivalence class is published. Label equality alone, point contact, or a path on a boundary never proves equivalence. The baseline implementation may publish only singleton equivalence classes.

The artifact retains strong typed dependencies on the arrangement, validated operands, exact construction storage, and accepted verification report. It contains no borrowed mesh pointer, finite offset point, approximate ray, worker index, mutable queue, requested operation result, selected patch, or realized coordinate.

## 5. Exact formal-probe point location

### 5.1 Formal open-point API

Define a kernel-neutral `formal_open_point_view` that can represent all Component 8 descriptors:

- exact point `p` known to lie in an open 3D region;
- `p + epsilon*d` for a seam/source-edge/vertex-sector base, where `d` is an exact rational direction and `epsilon` is a positive formal infinitesimal;
- `p +/- epsilon*n_P` for a patch-side base through `open_side_v1`;
- the empty-arrangement universe descriptor.

The view carries base-stratum kind/ID, exact base construction, direction polynomial or plane-side formula, perturbation key and version, incident signed constraints, and nonincident separation evidence. Validate every constraint against Component 8 before use. The kernel evaluates signs lexicographically by the first nonzero exact coefficient; it never instantiates `epsilon`.

Add an operation equivalent to:

```cpp
status_or<formal_operand_location>
locate_formal_open_point(kernel_request& request,
                         const formal_open_point_view& probe,
                         const oriented_shell_view& operand,
                         const point_location_policy& policy);
```

`formal_operand_location` stores `outside` or `inside`, signed boundary degree, primary ray/formal-direction identity, sorted hit groups, exact ray parameters, grouped source ownership, symbolic decisions, unperturbed base incidences, and a canonical evidence digest. Returning `boundary` for a descriptor already certified open is an upstream or kernel invariant failure, not a third occupancy state.

### 5.2 Signed degree algorithm

Implement the exact-only reference path first:

1. Validate the formal point and operand neutral view, including all facet rings/triangles, twins, vertex fans, shell roles, and owner bindings.
2. Try Component 3's fixed primitive directions in `generic_ray_v1` order. A finite direction is accepted only when all formal origin/facet signs and hit parameters can be grouped and owned without an unresolved degeneracy. Otherwise use the documented stable-ID formal direction; do not invent a local retry list.
3. Intersect the formal ray with exact source polygons. Triangles may accelerate, but reconcile all internal diagonals to the source facet and retain the unperturbed source-feature incidence.
4. Pre-rank exact/formal forward parameters, group equal hits, and apply `ray_boundary_owner_v1` over complete edge twins and vertex fans. One source boundary crossing contributes its oriented sign once; tangent, coplanar, and paired non-crossing groups contribute zero.
5. Sum signed contributions over all oriented shells/facets. Under `outward_oriented_nested_shells`, require the total degree to be exactly `0` for outside or `1` for occupied material. A negative value, value greater than one, unresolved group, or triangulation-dependent total contradicts accepted input or the kernel.
6. Record the complete normalized query certificate, including zero-contribution tangencies and all symbolic coefficients needed to replay ownership.

The algorithm classifies the complete operand, not shells independently followed by an informal nesting rule. As an additional invariant, independently derive occupancy from Component 2's shell containment/polarity records and exact shell locations and require agreement with the signed degree.

### 5.3 Alternate queries and ambiguity

For every seed, retain a primary certificate. If the primary finite direction meets any unperturbed edge, vertex, tangent, or coplanar relation, immediately execute the next independent admissible finite/formal path and require equal occupancy before accepting the seed. Symbolic ownership resolves a deterministic answer but does not suppress this ambiguity cross-check.

In exhaustive verification, and in the mandatory independent verifier for the bounded policy specified there, classify every seed through at least two distinct deterministic directions or one finite and one formal direction. Their occupancy and signed degree must agree even when hit decompositions differ. A disagreement is `internal_invariant_error` with both complete certificates.

### 5.4 Exterior-at-infinity

Construct an exact canonical exterior witness from the union of Component 2's exact operand bounds. For nonempty finite operands choose a checked rational point strictly greater than every maximum coordinate, for example `(M, M, M)` with `M = 1 + max(abs(all coordinate extrema))`, and certify that the positive coordinate rays remain outside all bounds. Empty operands use the exact origin.

Bind this witness to the represented side graph through an exact arrangement point-location/ray-walk certificate. For a nonempty arrangement, target the least canonical global patch's exact open-interior witness, trace the segment/ray from the bound-disjoint exterior to that target, group all hits with the same seam/vertex ownership rules, and map the initial open interval before the first hit to the corresponding Component 8 patch side or sector region. The initial region must have label `(false, false)`. If the arrangement is empty, use Component 8's universe component and publish it with that label. Failure to identify exactly one represented exterior fragment, or a mapped exterior fragment with another label, is an incomplete-adjacency invariant failure.

Additional disconnected fragments of the mathematical exterior are allowed; their own exact seeds must also classify outside both operands. Do not merge them merely because they are exterior.

## 6. Public artifact schema

Freeze closed enums for operand-location kind, seed-query path, crossing kind, transfer kind, propagation origin, path-step kind, equivalence proof kind, and classification invariant. Reject unknown values during decode.

### 6.1 Region and seed records

Each `classification_region` stores:

- dense ID and the exact Component 8 conservative-component ID;
- sorted ranges of patch sides, seam/source-edge sectors, vertex sectors, and region-preserving transitions;
- canonical `occupancy_pair label`;
- one `seed_certificate_id` for the Component 8 formal probe;
- canonical propagation predecessor/path references for each connected crossing-graph traversal;
- optional exterior and same-cell-equivalence references.

Each `seed_classification_certificate` stores the normalized formal descriptor, one `formal_operand_location` for A and B, any mandatory alternate locations, exact degree totals, source-feature evidence, policy/formula versions, and evidence digest. Classify every Component 8 conservative component's descriptor against both operands, including components later reachable by propagation. This direct classification is an independent check on graph transfer, not redundant data to omit.

### 6.2 Directed crossing transfers

Normalize every Component 8 crossing into paired directed `classification_transition` records between two distinct region IDs. A forward record stores reverse ID, source crossing ID/kind, affected operand mask, exact endpoint side identities, expected before/after constraints, and complete source-use/coincidence provenance. Reverse swaps endpoints and constraints and must be involutive.

For an individual noncoincident source sheet:

- derive which endpoint is the source use's occupied negative side from canonical support-plane parity and Component 8 side incidence;
- constrain that endpoint's component for the owning operand to `true` and the opposite endpoint to `false`;
- require the other operand's component to remain unchanged across the transition;
- reject a transition that does not join the two open sides, has an inconsistent source role, or behaves as a tangent.

For a coincident-group crossing:

1. Examine every separate source-sheet use in the group; do not reduce the record to facet count or signed multiplicity.
2. For each operand represented, translate every member's source orientation/occupied side to canonical negative/positive geometric sides, using Component 8's exact sector order to identify the two actual open endpoint regions.
3. Require all member uses of one accepted operand to prescribe one consistent pair of side occupancies. Component 2's embedded disjoint-shell contract normally permits at most one material boundary layer of an operand over an open domain; repeated covering records must agree and remain provenance, not toggle repeatedly.
4. Constrain both endpoint occupancy components simultaneously from the resulting per-operand side vector. For an operand absent from the group, require its value to pass through unchanged.
5. Require at least one operand component to change; otherwise the crossing is not a separating source boundary and contradicts Component 8's transition kind.

An opposite-facing A/B coincidence can therefore change both components in opposite directions; a same-facing coincidence can change both in the same direction. No case-specific Boolean semantics or representative preference appears here.

Tangent/contact/incidence-only and transparent transitions are region-preserving and must already be inside one Component 8 conservative component. Audit them for equal labels and never create a crossing/toggle. A Component 8 region-preserving transition whose endpoints map to different classification regions is malformed input to this stage.

### 6.3 Patch-side labels

Publish exactly one `patch_side_label` for every `patch_side_id`, storing its region, canonical plane side (`negative` or `positive`), and occupancy pair. Publish exactly one `global_patch_side_labels` record for every `global_patch_id`, containing its negative and positive side-label IDs and values.

Derive labels only through the patch-side-to-region map. Do not query a point on the patch or classify the boundary itself. Require every separate `sheet_patch_use` and coincident group covering the patch to agree with the corresponding transfer constraints. Equal side labels are valid for tangential or locally non-result-separating geometry; Component 10 decides whether the requested truth function changes.

### 6.4 Provenance and certificate

Store one canonical propagation forest per undirected connected component of the crossing graph. Root it at the least complete region key, order outgoing arcs by immutable transition key, and record for every reached region the predecessor, transition, expected transfer, and resulting label. Also store all non-tree arc checks and a cycle-basis summary.

When a propagated label or crossing constraint disagrees with the directly classified seed, reconstruct two complete canonical paths: the established root-to-region path and the competing path/edge, including seed and transfer evidence. Never report only the final booleans.

Publish a `cell_classification_certificate` containing dependency/policy digests, region/transition/seed/side counts, degree and alternate-query facts, propagation components/paths/cycles, transfer checks by kind, exterior facts, optional equivalence facts, and a combined semantic digest.

## 7. Classification and propagation algorithm

### 7.1 Dependency audit and graph construction

1. Validate arrangement and retained dependency bindings before opening geometric worker phases.
2. Enumerate all Component 8 conservative components, require exactly one valid probe descriptor each, and create one provisional classification region per component.
3. Recompute region-preserving connected-component membership from transparent/seam/source-edge/vertex/tangent transitions and compare it exactly to Component 8. This is an audit, not a repair or alternate arrangement.
4. Convert every individual/coincident crossing to normalized paired directed arcs and prove total coverage: every crossing appears once undirected/twice directed, every endpoint exists, no self-crossing arc exists, and no source sheet can be bypassed by a preserving transition.
5. Build reverse patch-side/sector mappings only from frozen forward records and compare all supplied reverse ranges.

### 7.2 Parallel exact seed classification

1. Partition regions by canonical region key under Component 1's fixed deterministic execution policy.
2. Validate and classify each region's formal descriptor against A and B through Section 5. Each worker writes only an accounting-backed private result shard and cannot assign IDs or publish.
3. Run required alternate queries, normalize hit groups/evidence, and return one complete provisional seed record and occupancy pair.
4. Merge shards by region key, not completion order. Select the canonical first failure by Component 1 precedence.
5. Require every region exactly once, including regions in disconnected crossing-graph components and operands with no nearby source sheet.

### 7.3 Transfer validation and propagation

1. Derive every crossing's endpoint occupancy constraints from immutable source orientation and coincidence membership before traversing the graph.
2. For each undirected crossing-graph component, choose the least canonical region as root and initialize it from its exact seed label.
3. Traverse arcs in canonical breadth-first order. Apply the typed transfer, check all constrained components, and assign the destination's propagated label if unseen.
4. If the destination is already seen, require exact equality and record the non-tree edge/cycle check. If it differs, preserve both predecessor chains and fail.
5. At every visited destination require the propagated value to equal its independently exact-classified seed. This catches a missing crossing, wrong radial endpoint, source orientation reversal, bad coincidence transfer, or point-location defect at the first canonical edge/region.
6. After traversal, require every region visited in exactly one crossing-graph component and every directed arc checked in both directions. Reverse traversal must recover the source label.
7. Walk a canonical fundamental cycle basis and require composition of transfers to return the complete starting occupancy. Also scan all arcs directly; the basis certificate is compact evidence, not permission to omit an edge check.

This design intentionally classifies every conservative fragment even though one seed per crossing-connected graph would suffice for propagation. Direct seeds provide operation-independent ground truth and allow disconnected or over-segmented cells to be validated without assuming unproven global connectivity.

### 7.4 Final labels and optional equivalence

1. Freeze each region's agreed direct/propagated label.
2. Materialize all patch-side and per-patch negative/positive labels, then validate every source-use occupied-side relation and every transition endpoint against them.
3. Build and validate the exterior certificate.
4. If optional open-complement path proofs are enabled, group only regions connected by such proofs, require equal labels, and publish the equivalence relation without renumbering or deleting regions. Otherwise publish singleton classes.
5. Fallibly pre-rank exact evidence/keys, assign final IDs, build reverse indices from final stores, encode, independently verify, check cancellation, and publish atomically.

## 8. Exact invariants and failure handling

Before a draft can succeed, prove:

1. Every Component 8 conservative component maps to exactly one classification region and every region has exactly one valid seed certificate and occupancy pair.
2. Every patch side, seam/source-edge sector, vertex sector, preserving transition, and crossing transition has total owner-safe mapping; no fragment is silently omitted.
3. Every formal probe satisfies its signed open constraints and classifies non-boundary against both complete operands with degree exactly zero or one.
4. Required alternate rays/formal paths agree; source facet triangulation and hit grouping do not change occupancy.
5. Every individual crossing changes exactly its owning operand from exterior side to occupied side and preserves the other operand.
6. Every coincident crossing applies one consistent side vector per represented operand, retains all source members, and never toggles by arbitrary multiplicity.
7. Tangent, contact, incidence-only, and transparent connectivity does not change either occupancy component.
8. Every propagation path and cycle agrees with every direct seed; reverse transitions are involutive and all graph arcs are checked.
9. Every patch has exactly one negative and positive side label derived from distinct open side nodes, never from boundary-inclusive classification.
10. At least one certified represented exterior fragment is outside both operands; empty arrangement/operand cases use the documented universe behavior.
11. Optional region equivalence has an exact open-path proof and never relies only on equal occupancy.
12. IDs, labels, evidence, selected first failure, diagnostics, and canonical bytes are independent of operation, input traversal, hash behavior, pointer/allocation values, worker completion, thread count, and exact filter path.

Return `resource_limit` for declared bytes/work/entity/exact-number/ray-attempt limits, cancellation, allocation failure, or verifier exhaustion. Return `internal_invariant_error` for malformed/stale dependencies, invalid probes, unresolved formal point location, degree outside `{0,1}`, ray disagreement, incomplete adjacency, contradictory source orientation, inconsistent coincidence members, propagation conflict, missing side label, exterior failure, producer/verifier disagreement, or encoding mismatch. Component 9 does not return `input_contract_error`, `index_overflow`, or `output_not_representable`; accepted Component 2 input defects discovered here indicate an upstream invariant failure.

Before work, use checked arithmetic to bound regions, crossing arcs, operand seed queries, candidate ray/facet work, hit groups, symbolic coefficients, propagation entries, predecessor/cycle records, side labels, certificate entries, canonical bytes, and verifier duplication. Use deterministic sparse bounds and canonical chunk grants. Never drop a ray hit, source member, transition, path record, or verification check to fit a limit.

Check cancellation before dependency validation, each worker/grant round, bounded facet/ray interval, exact fallback, hit grouping, graph frontier, path reconstruction, side-label assembly, encoding, verification, and publication. Catch exceptions only at task/stage boundaries, join siblings, select the canonical failure, and roll back. Diagnostics include region/patch-side/transition/source-use/coincidence/shell/facet IDs, exact/formal ray evidence, both conflicting paths, policy/dependency digests, resource facts, and replay token.

## 9. Canonical encoding and deterministic execution

Define `YGBCAN09` as operation-independent semantic classification of one frozen arrangement: schema/type versions; dependency semantic digest; point-location/degree/perturbation/transfer/propagation/equivalence policy versions; regions and Component 8 mappings; normalized seed classifications and alternate-query evidence; directed crossing transfers; propagation forests/non-tree checks/cycle facts; patch-side and per-patch labels; exterior/equivalence records; certificate facts; and deterministic semantic counts.

Exclude owner tokens, pointers, raw worker/provisional indices, requested operation, setup invocation ordinal, diagnostics, traces, timings, caches, filter-attempt state, and approximate values. The exact input/arrangement dependency is bound by the invocation wrapper; the semantic payload still retains stable source-feature IDs and exact evidence needed to interpret labels.

Define `YGBLAB09` as invocation-bound framing with schema/type versions, setup digest, exact dependency identities/generations/digests, kernel and classification policies, deterministic statistics, length-prefixed `YGBCAN09`, construction-storage binding, and verification report/certificate binding. Wrap it with Component 1's `YGBART01` framing for `labeled_arrangement`. Decode rejects unknown enums/versions, malformed owner-free IDs/ranges, duplicate/missing region mappings, invalid booleans, unsorted hit groups, broken reverse arcs/paths, incomplete side labels, bad lengths, noncanonical exact values, and trailing bytes.

Partition seed work by a versioned canonical region frontier independent of thread count. Workers use private shards and cannot mutate the graph, allocate final IDs, or publish. Reserve kernel/storage envelopes in sorted `canonical_work_key(cell_classification, region, operand, query_path)` rounds. The coordinator alone merges seeds, derives transfers, propagates, assigns IDs, encodes, invokes verification, and publishes.

All fallible exact/formal comparisons occur before standard sorting; frozen comparators inspect immutable ranks and fixed fields and are `noexcept`. Hash tables are lookup accelerators only. For one frozen invocation, thread count, shard boundaries, worker delays, cache/filter state, allocation addresses, and hash collision mode produce byte-identical labels, IDs, errors, reports, and artifact bytes.

## 10. Mandatory independent verifier

Register a stable Component 9 artifact tag, schema/checker versions, and invariant set with Components 1/13. The read-only verifier receives the candidate, exact arrangement/validated dependencies, exact-only kernel services, accounting, and cancellation. It must not call producer seed batching, hit grouping, transfer derivation, graph traversal, path construction, ID assignment, certificate, or encoding helpers.

Mandatory checks:

1. Validate owner, slot/tag/schema, dependency identities/generations/digests, policy versions, dense IDs, enum domains, role-qualified references, ranges, reverse maps, and construction owners.
2. Reconstruct the Component 8 conservative region components independently and compare total fragment/transition mappings without trusting stored region membership.
3. Substitute every formal probe into all incident/nonincident signed constraints and prove it denotes an open point in the stated region.
4. Independently classify every seed against both operands in exact-only mode using a structurally separate source-polygon hit grouping and signed-degree accumulation. Use an alternate deterministic ray/formal path for every seed within the mandatory verifier's documented resource policy, and require degree/occupancy agreement.
5. Independently derive every individual and coincident crossing's endpoint constraints from source-sheet uses, canonical plane sides, and Component 2 occupied-side policy. Compare all directed transfer records and reverse involutions.
6. Re-propagate labels from canonical roots with a separately implemented traversal, check every edge and fundamental cycle, compare direct seeds, and reconstruct conflicts if any.
7. Rebuild all patch-side/per-patch labels from Component 8 maps and verify source-use/coincidence orientation facts; ensure no boundary point classification supplied a side value.
8. Reconstruct the canonical exterior witness and exact arrangement attachment, require `(false,false)`, and independently validate every optional region-equivalence path.
9. Recompute all certificate facts/statistics and independently encode `YGBCAN09`, `YGBLAB09`, and `YGBART01`; verify report, replay, trace, and construction-storage bindings separately.

Verifier resource exhaustion prevents publication with `resource_limit`; any semantic disagreement is `internal_invariant_error`. Exhaustive mode compares bounded cases with a deliberately slow exact solid-angle/degree or multi-ray oracle that shares exact scalar values but not production hit ownership or propagation control flow.

Mutation tests alter every owner/ID/range, region membership, probe base/direction/constraint/key, ray path/parameter/hit owner/contribution, degree/occupancy, source use/occupied side, coincidence member/orientation, transition endpoint/kind/reverse/constraint, predecessor/path/cycle, patch-side label, exterior/equivalence fact, dependency binding, policy version, certificate fact, and serialization field/order. Every mutation must fail in Release/NDEBUG.

## 11. Test plan

### 11.1 Focused exact seed classification

- Empty operands and empty arrangement; one untouched convex shell; disjoint shells; nested shells; inward cavity; island inside a cavity; and multiple disconnected material components.
- Formal probes based in patch interiors, seam interiors, ordinary source edges, original/intersection vertices, tangent sectors, and the empty universe.
- Rays through a facet interior, source edge, source vertex, high-valence vertex fan, coplanar facet, tangent contact, and several equal-parameter features; compare finite and formal query paths.
- Concave facets and radically different certified triangulations, proving internal diagonals do not affect degree or ownership.
- Signed zero, subnormal/extreme exponents, one-ULP gaps, cancellation-heavy plane/ray parameters, and large exact rational constructions for all four `<T, I>` specializations.

Each case checks unperturbed incidences, grouped ownership, signed contributions, degree in `{0,1}`, shell-polarity cross-check, alternate-query agreement, canonical evidence, and independent verifier output.

### 11.2 Transfers, contacts, and coincidences

- One noncoincident A sheet and one B sheet in both canonical plane parities; require only the owning occupancy component to change.
- Transverse sheet intersections with four radial sectors; closed seam loops; seams ending at source edges/vertices; and several sheets meeting at one carrier or vertex.
- Tangent point/edge/face contact and coplanar boundary-only contact, proving no false crossing or label change.
- Equal solids, same-facing coincidence, opposite-facing coincidence, partial coincident domains, containment boundaries, cavity-wall coincidence, and a transverse seam entering/leaving a coincidence group.
- Several provenance records covering one source layer and deliberately contradictory same-operand member orientations; preserve agreeing records and reject contradiction rather than applying odd/even facet count.
- Region-preserving artificial cuts and source tessellation boundaries, proving equal labels without creating source-sheet crossings.

### 11.3 Propagation and side labels

- Trees, single cycles, multiple independent cycles, disconnected crossing graphs, over-segmented fragments of one cell, and isolated exterior fragments.
- Inject one wrong crossing endpoint, operand role, source occupied side, coincident contribution, omitted edge, false tangent, or seed label and require deterministic two-path diagnostics.
- Require forward/reverse transfer involution, all cycle compositions returning to the start, and all direct seeds matching propagated labels.
- Verify exactly two side labels per global patch, correct canonical negative/positive mapping, complete source-use agreement, and valid equal-side labels at nonseparating contact.
- Exterior witness for empty, disjoint, nested, and widely separated/extreme-coordinate operands; failure fixtures with no unique side-graph attachment.

### 11.4 Analytic, differential, and metamorphic tests

- Analytic disjoint, nested, cavity, equal, touching, tangent, transverse, same-facing coincidence, and opposite-facing coincidence cases with hand-derived labels for every open region.
- Compare production seeds, propagation, mandatory verifier, and independent exact multi-ray/degree oracle over deterministic small generated solids and synthetic side graphs.
- Classify additional exact rational probes in open regions through a verification-only exact point-to-region locator and require agreement with the containing region label. Generate probes from known rational cells first; never accept approximate containment as the oracle.
- Swap operands and require only the two occupancy components, role-qualified provenance, and corresponding IDs to map; unoriented region geometry and graph structure remain equivalent.
- Apply exactly representable translations, axis permutations, orientation-corrected sign flips, and positive power-of-two scaling; labels and combinatorial canonical form remain invariant/equivariant.
- Permute source vertices/facets/shells, local/global record order, transition insertion, rays among independently valid choices, and valid source subdivision/tessellation. Compare through canonical dependency/refinement maps.
- Vary operations and require identical Component 9 artifact bytes. Vary threads, shards, worker delays, hash seed/collision mode, allocation addresses, exact filter path, and cache state; require identical IDs, labels, selected failures, diagnostics, and bytes.
- Golden round trips cover empty universe, nested cavity, transverse seam, tangent vertex, and same/opposite coincident transfer grammars.

### 11.5 Failure, resource, and qualification tests

- Wrong owner, stale/replacement dependency, malformed region membership, missing/duplicate transition, invalid formal constraint, boundary-valued probe, unresolved ray group, alternate-ray disagreement, degree outside `{0,1}`, contradictory source side, incomplete coincidence vector, propagation conflict, missing patch side, and exterior attachment failure all fail closed.
- Exact-at-limit and one-over region/arc/seed/ray-attempt/hit/evidence/queue/path/cycle/side/equivalence/certificate/private-byte/work/exact-number/diagnostic/trace cases; allocation failure and cancellation at every phase expose no partial artifact.
- Fault injection before verification/publication leaves no `labeled_arrangement` slot or committed classification charge. Post-publication observer failure cannot change labels.
- Debug/Release and GCC/Clang canonical outputs match. ASan/UBSan cover malformed references, formal-polynomial/hit/path guards, and rollback; TSan covers seed shards, cancellation, verifier, diagnostics, and publication.
- Benchmark facet-heavy seed rays, high-valence vertex ownership, deeply nested shells, many disconnected regions, seam-heavy crossing graphs, and coincidence-heavy transfers. Record exact query/hit/fallback counts, graph work, verifier work, peak private bytes, and limb growth without weakening checks.
- Serialize failures with source coordinate bits, formal coefficients, exact parameters, region/transition/patch/source IDs, both paths/rays, dependency/policy digests, PRNG state, and expected/actual labels. Decimal coordinates alone are insufficient.

## 12. Component 10 handoff contract

Before Component 10 integration, prove it can consume only `labeled_arrangement<T, I>` and retained Component 8 topology to:

- enumerate every atomic geometric patch and its separate source-sheet/coincidence provenance;
- obtain exactly one canonical negative and positive occupancy pair for every patch;
- evaluate only Component 1's frozen truth table on those two open-side labels;
- distinguish equal-result-occupancy patches from boundary-separating patches without reclassifying points;
- orient a selected patch so result interior is on its negative side, independent of source operand priority;
- choose among geometrically coincident source uses only after side truth values establish that one boundary is required;
- diagnose a missing, stale, contradictory, or operation-dependent label against stable Component 9 IDs.

Component 10 must not repeat ray casting, infer labels from source normals, toggle coincidence multiplicity, or mutate the labeled artifact. Component 9 provides occupancy only, not a preferred patch representative or Boolean selection.

## 13. Implementation sequence and completion criteria

Implement in this order:

1. Reconcile Components 1/2/3/8 IDs, formal-probe point location, signed degree, source-side, resource, verifier-environment, and dependency interfaces; freeze Component 9 enums, policies, invariant codes, and encodings.
2. Implement immutable region/seed/transition/path/side/exterior/certificate schemas with owner-checked accessors and canonical encode/decode tests.
3. Implement Component 3's exact-only formal-origin point location, grouped signed boundary degree, alternate paths, and analytic/adversarial kernel tests.
4. Implement single-threaded dependency audit, direct classification of every Component 8 probe against both operands, and exterior attachment.
5. Implement individual/coincident transfer derivation, deterministic propagation, reverse/cycle checks, conflict path reconstruction, and patch-side label assembly.
6. Add canonical IDs, accounting envelopes, deterministic seed workers/merge, cancellation, diagnostics, replay, and transaction rollback.
7. Implement the independent verifier, exact arbitrary-probe test locator, canonical encodings/digests, mutation suite, certificate gate, and atomic publication.
8. Run analytic, oracle, alternate-ray, operand-swap, transform/scale, source-subdivision, operation-independence, thread/schedule/filter, adversarial-bit, resource, sanitizer, replay, and benchmark qualification before Component 10 integration.

Component 9 is complete only when:

- every Component 8 conservative open-region fragment has one exact operation-independent `(inside_A, inside_B)` label and one replayable seed certificate;
- no seed uses a finite offset, tolerance, rounded coordinate, or boundary-inclusive point test;
- primary and independent exact/symbolic rays agree, including all edge, vertex, coplanar, tangent, and nested-shell cases;
- every individual and coincident crossing has one exact checked transfer derived from complete source orientation/occupied-side provenance, while tangencies preserve labels;
- all propagation paths, reverse arcs, and closed cycles agree with every direct seed, with deterministic complete conflict paths on failure;
- every atomic patch has exact canonical negative/positive side labels and no operation-dependent selection data;
- the represented exterior-at-infinity is certified outside both operands and optional fragment equivalences have exact open-path proofs;
- independent point-location, transfer, propagation, side-label, exterior, certificate, and encoding verification passes in Release as well as Debug;
- analytic, random exact-probe, mutation, resource/cancellation rollback, replay, compiler, sanitizer, and schedule-determinism suites pass;
- Component 10 can select and orient exact boundary patches solely from immutable side labels and Component 1's truth table, without geometric reclassification or topology repair.
