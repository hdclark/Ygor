# Plan 15: Independent Verification, Diagnostics, and Replay

## 0. Scope and fixed V1 design

Implement **only Component 15** from `component_15_verification_diagnostics_replay.md`. Consume the immutable `assembled_output_candidate<T,I>` from Component 14 together with the immutable predecessor artifacts and the owner-checked Component 01/03 services required by the frozen verification policy. Publish one immutable `verified_boolean_result<T,I>` and permit construction of one ordinary `bounded_boolean_success<T,I>` **only after every mandatory publication gate succeeds**. On rejection, resource exhaustion, cancellation, malformed replay input, incompatible version, or verifier contradiction, publish no ordinary success and return one deterministic typed failure with canonical witnesses and replay information.

Component 15 verifies and publishes. It must not:

- change any public coordinate bit, public index, facet order, orientation, topology relation, occurrence, multiplicity, classification, cleanup action, precision record, or predecessor artifact;
- weld or split vertices, retriangulate, remove features, move coordinates, retry cleanup, choose another symbolic rule, or reinterpret tolerance;
- accept a producer map, count, report, digest, search result, classification, or pass boolean as the sole evidence for the corresponding fact;
- infer topology, identity, lineage, adjacency, or allowed contact from coordinate equality/proximity, tolerance, a hash, an AABB overlap, traversal order, allocation order, pointer values, or worker completion order;
- call, adapt, copy from, or derive implementation from `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- introduce an external geometry, graph, exact-arithmetic, interval, spatial-index, serialization, hashing, testing, fuzzing, replay, or concurrency dependency; or
- compile production or normative-test code outside the strict portable C++17 target established by Component 01.

Freeze V1 as this provider set:

```text
candidate_intake:                 complete_dependency_graph_audit_v1
public_mesh_readback:             component14_direct_const_read_view_v1
public_lexical_audit:             exact_bits_indices_triangles_v1
public_topology_reconstruction:   sorted_use_pair_link_component_v1
internal_public_bijection:        full_incidence_lineage_map_proof_v1
triangle_geometry:                bounded_dominant_projection_area_v1
shell_orientation:                paired_edge_propagation_and_probe_side_v1
event_lineage_audit:              canonical_relation_event_occurrence_trace_v1
classification_reconstruction:    zero_delta_bfs_quotient_potential_v1
selection_reconstruction:         frozen_truth_table_surface_transition_v1
probe_coverage:                   canonical_required_region_cover_v1
probe_anchor_policy:              fixed_positive_barycentric_candidates_v1
probe_direction_policy:           oriented_small_integer_vectors_v1
probe_offset_policy:              bounded_dyadic_clearance_interval_v1
operand_occupancy:                independent_bounded_ray_winding_v1
verification_spatial_index:       top_down_rank_median_aabb_tree_v1
forbidden_pair_relation:          bounded_triangle_support_overlay_v1
construction_audit:               residual_enclosure_lineage_recheck_v1
cleanup_audit:                    independent_certificate_replay_complex_v1
precision_aggregation:            primitive_ledger_path_reduction_v1
report_regeneration:              reconstructed_fact_reports_v1
reingestion:                      fresh_component02_operand_contract_v1
finding_arbitration:              component01_total_key_minimum_v1
logical_serialization:            component01_canonical_bytes_le_v1
digest_provider:                  component01_sha256_domain_separated_v1
replay_provider:                  component01_focused_verification_replay_v1
final_self_audit:                 independent_evidence_codec_resource_audit_v1
execution_reference:              serial_complete_final_verification_v1
```

The executable serial implementation is the semantic reference. Parallel execution may create only private check results, pair blocks, probe proposals, ledger reductions, finding buffers, and report fragments from immutable inputs. Canonical merge, checked entity sets, primary failure, retained findings, semantic counters, reports, logical bytes, replay bytes, digests, and publication disposition must equal the serial reference byte-for-byte for the same versions, qualified platform, policy, and resource ceilings.

V1 publication rules are non-negotiable:

- publish the exact `fv_surface_mesh<T,I>` stored in Component 14; never rebuild a substitute mesh;
- reconstruct topology from public indices and oriented facets, never coordinates;
- support every geometric acceptance with Component 03 bounded evidence;
- give every mandatory check a completed/pass evidence record; no verification level may disable the mandatory floor;
- assign `geometry.status == tolerance_checked` only in the private final success proposal after all gates, report regeneration, canonical encoding, self-audit, resource reconciliation, worker join, and final cancellation poll succeed;
- return typed failure rather than topology-only ordinary success;
- compare full structure/bytes whenever equality is semantic; digests are accelerators only; and
- keep a rejected candidate internal and immutable.

Mark Component 15 complete in `tracker.md` only after every instruction and qualification gate in Section 25 is represented and this plan is committed.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Public mesh carrier

Use the candidate's `fv_surface_mesh<T,I>` only through the const `PublicMeshReadView<T,I>` established by Plan 14. The read view exposes checked counts, exact scalar bits, ring lengths, indices, and recognized empty optional arrays. It must not normalize rings, reorder, deduplicate, repair, narrow, recompute normals, populate `involved_faces`, or borrow mutable storage.

Do not call public mesh mutators or convenience routines, including duplicate merging, conversion, degeneracy removal, disconnected-vertex removal, simplification, normal computation, incidence-cache recreation, orientation repair, hole filling, remeshing, or zippering. Component 15 is not another cleanup stage.

### 1.2 `YgorMeshesVerification` is not a provider

`YgorMeshesVerification.h/.cc` is useful only as a source of simple fixtures and examples. Its boolean/exception interfaces, nominal geometry, edge counts, and orientation checks do not reconstruct reciprocal uses, closed vertex links, components, shell semantics, internal/public bijection, duplicate-coordinate separation, lineage, classification, cleanup, precision, reports, replay, or deterministic typed evidence. Preserve its existing API but do not call it as a publication gate.

Implement Component 15 integer-incidence checks inside the bounded subsystem. Share a semantics-free sorted-edge or cycle primitive only when it accepts owner-bound strong IDs, complete deterministic keys, no producer-owned grouping result, and is independently tested by each caller.

### 1.3 Arithmetic reuse

Consume only Component 03 strict-target capabilities for exact scalar bits, outward arithmetic, finite intervals, bounded points/vectors/planes/parameters, residuals, determinant/orientation results, exact-nominal expansion signs, conditioning, conservative AABBs, precision ledger, and tolerance budget. Do not call legacy `orient_sign`, raw adaptive predicates, `vec3` arithmetic, normalized directions, square roots, angle/distance helpers, or epsilon comparisons for authoritative decisions.

A shared exact-nominal sign is permitted; Component 15 must still independently organize the higher-level topology, triangle-pair, probe, occupancy, cleanup-replay, and aggregation paths and must account for inherited uncertainty.

### 1.4 Existing BSP and spatial indexes are unsuitable

Do not use `YgorMeshesBSPTree`, public R-tree/octree/cell indexes, `index_bbox<T>`, tetrahedralization spatial helpers, or Component 06's hierarchy as the final verifier. These providers use incompatible mutable layouts, raw floating comparisons, insertion/traversal order, uninflated bounds, or producer control flow and do not carry the required owner/version/precision/lineage/replay evidence.

Build the independent top-down rank-median hierarchy in Section 13 from Component 03 finite closed AABBs. It is deliberately different from Component 06's rank-Morton LBVH and Component 13's cleanup forest.

### 1.5 Existing serialization and hashing are unsuitable

Do not use Ygor text/XML serializers, native struct serialization, `std::hash`, SpookyHash, MD5, or public mesh I/O as canonical evidence. Use Component 01 `CanonicalBytes` and domain-separated SHA-256, with full-byte fallback on all semantic comparisons.

### 1.6 Mandatory predecessor/infrastructure reuse

Reuse, without duplicating:

- Component 01 context, strong IDs, checked arithmetic, typed outcomes/errors, truth/symbolic tables, transactions, resources, cancellation, diagnostics, replay, canonical bytes, SHA-256, and deterministic execution capabilities;
- Component 02 validated source topology, shell semantics, immutable source descriptions, and fresh public-input adapter;
- Component 03 bounded arithmetic, values, predicates, bounds, ledgers, budgets, and precision aggregation primitives;
- Components 04-08 source-triangle/manifold/relation/event/carrier evidence;
- Components 09-10 classification, side, selection, orientation, multiplicity, and coincident-ownership artifacts as claims to compare;
- Components 11-12 output occurrences, paired cycles, triangulation, residual cells, obligations, and provenance;
- Component 13 cleaned manifold, action certificates, before/after correspondence, topology effects, budgets, and final query views;
- Component 14 public mesh, exact bit records, maps, reports, canonical bytes/digests, pending statuses, dependency handles, and handoff evidence; and
- Component 17 deterministic execution services when available, while retaining the serial verifier as normative.

Do not create a second truth table, symbolic matrix, source/event registry, precision ledger, tolerance ledger, cleaned manifold, public mesh adapter, canonical byte writer, hash provider, transaction framework, or scheduler.

### 1.7 Greenfield independent control flow

New Component 15 modules are required for public topology reconstruction, full structural/lineage bijection, bounded public triangle acceptance, independent classification and quotient potentials, deterministic probes and operand occupancy, independent output pair search/relation, cleanup-certificate replay, primitive-ledger/path aggregation, final reports, findings, focused replay, re-ingestion, and success publication. These modules must not route through corresponding producer builders, grouping/traversal implementations, reducers, or pass booleans.

## 2. Exact files, target integration, versions, and checkpoints

### 2.1 Production files

Add under `src/YgorMeshesBooleanBounded/`:

- `FinalVerificationTypes.h` — closed enums, strong IDs, complete keys, V1 constants, counters, evidence categories, and failure payloads.
- `VerifiedBooleanResult.h` — immutable verified artifact, checked section views, final invariants, and controlled ordinary-success conversion.
- `FinalVerification.h/.cc` — entrypoint, fixed checkpoints, serial reference, deterministic merge, transaction, and publication.
- `VerificationPreflight.h/.cc` — dependency/owner/version/status/range/digest/evidence audit; checked count/byte/work bounds; resource plan.
- `VerificationPublicMesh.h/.cc` — lexical readback and malicious adapter seams.
- `VerificationTopology.h/.cc` — directed uses, edge grouping/pairs, links, components, shells, orientation propagation, Euler/genus, and topology evidence.
- `VerificationBijection.h/.cc` — public/cleaned vertex, facet, edge-use, component, bit, incidence, occurrence, and lineage matching.
- `VerificationTriangleGeometry.h/.cc` — bounded area/orientation, projection selection, shell-local geometry, and authorized boundary cases.
- `VerificationLineage.h/.cc` — relation/event/occurrence/public identity separation, carrier order, and complete trace coverage.
- `VerificationClassification.h/.cc` — zero-delta graph, canonical BFS groups, quotient graph, delta cycles, seeds, and integer potentials.
- `VerificationSelection.h/.cc` — truth-table retention, orientation, multiplicity, contact regularization, and coincident ownership.
- `VerificationProbePolicy.h/.cc` — coverage planner, anchors, directions, offsets, escalation, and evidence.
- `VerificationOperandOccupancy.h/.cc` — independent bounded ray winding over each source operand.
- `VerificationAabbTree.h/.cc` — top-down rank-median hierarchy, pair/ray traversals, structural checks, and counters.
- `VerificationTriangleRelations.h/.cc` — independent output triangle/triangle and probe-segment/triangle relation paths.
- `VerificationConstructionAudit.h/.cc` — construction parameters, residuals, carriers, envelopes, rounding, conditioning, and precision dominance.
- `VerificationCleanupReplay.h/.cc` — minimal verifier-owned paired complex, action replay, link/interface/topology/budget/intersection checks, and final comparison.
- `VerificationPrecisionAudit.h/.cc` — primitive ledger DAG validation, path aggregation, displacement/removal maxima, witnesses, and tolerance eligibility.
- `VerificationReports.h/.cc` — final report regeneration and cross-report consistency.
- `VerificationReingestion.h/.cc` — fresh Component 02 ordinary input validation and structural/semantic comparison.
- `VerificationFindings.h/.cc` — canonical finding keys, summaries, coalescing, bounded retention, and Component 01 arbitration adapter.
- `VerificationCodec.h/.cc` — verification/report/result encoders/decoders, domain framing, full-byte comparison, and digests.
- `VerificationReplay.h/.cc` — focused selector schema, finalization, safe decoder, artifact embedding/reference, and replay verification.
- `VerificationSelfAudit.h/.cc` — final evidence/range/report/codec/resource/status audit without recursively rerunning all geometry.
- `VerificationQueries.h` — narrow immutable views for diagnostics, Component 16, replay, and public result conversion.

Extend `YgorMeshesBooleanBounded.h`, `ContractVersions.h`, Component 01 stage/checkpoint/ID/subcode/resource/replay registries, the Component 02 re-ingestion capability, strict target, and explicit instantiations. Keep scratch/build/mutation state out of installed headers.

### 2.2 Tests

Add dependency-free strict-target tests for contracts, public mesh, topology, bijection, triangle geometry, lineage, classification, selection, probes, operand occupancy, AABB tree, triangle relations, constructions, cleanup replay, precision, reports, re-ingestion, serialization, replay, independence mutations, exhaustive oracle, properties, adversarial floats, resources/cancellation, concurrency, structural performance, fixtures, mutation builders, exact oracle, exhaustive pairs, deterministic fuzz/shrink, and golden bytes.

### 2.3 Version registry

Add explicit nonzero V1 versions for every provider/artifact/evidence/codec/replay/result named above. Reserve zero for invalid. Reject unknown required versions, invalid enums, duplicate singleton fields, incompatible predecessor versions, and nonzero reserved V1 fields. Include all versions in findings, reports, logical bytes, replay, and final digest.

### 2.4 Stable checkpoints

Use this fixed order:

1. capability/owner validation;
2. candidate status/version/type audit;
3. dependency/evidence graph audit;
4. checked count/index/byte/work preflight;
5. aggregate resource reservation;
6. public lexical readback;
7. directed-use extraction;
8. undirected grouping/pairing;
9. vertex links;
10. components/shells;
11. internal/public bijection;
12. triangle geometry;
13. shell orientation;
14. lineage audit;
15. classification groups;
16. quotient potentials/winding;
17. selection/orientation/multiplicity;
18. verification hierarchy build/audit;
19. forbidden pair enumeration;
20. pair classification;
21. probe coverage planning;
22. anchor/direction/offset construction;
23. probe clearance;
24. independent operand occupancy;
25. probe acceptance;
26. construction audit;
27. cleanup replay/final comparison;
28. precision/tolerance aggregation;
29. report regeneration;
30. canonical byte/candidate digest audit;
31. fresh public re-ingestion;
32. final evidence;
33. replay finalization;
34. proposed verified result;
35. final self-audit;
36. resource reconciliation/worker join;
37. final cancellation poll; and
38. atomic commit or deterministic failure.

Poll cancellation at every checkpoint and deterministic entity/work intervals, never by time.

### 2.5 Failure subcodes

Allocate a disjoint Component 15 range covering at least: illegal final candidate status; incompatible version/type/owner/dependency; missing evidence; range/count/digest contradiction; lexical coordinate/index/facet failures; edge/link/component/shell failures; map non-bijection; coordinate/orientation/occurrence weld mismatch; triangle geometry or shell side unresolved; event/lineage/carrier/occurrence failures; classification partition/delta/cycle/winding failures; selection/orientation/multiplicity/owner failures; missing/no-safe/third-surface/occupancy probe failures; AABB build/traversal contradiction; forbidden/unresolved pair relation; construction residual/envelope/rounding/conditioning failure; malformed/unreplayable cleanup certificate; cleanup topology/budget/intersection mismatch; missing/cyclic/non-monotonic/understated ledger; understated displacement/removal/tolerance excess; report/status/evidence contradiction; codec/digest/domain failure; re-ingestion mismatch; malformed replay; final self-audit rejection; unverified success conversion; and transaction/resource/cancellation contradiction.

Map coherent bounded geometry difficulty to `result_geometry_not_validated`, `geometric_condition_exceeds_tolerance`, or `cleanup_budget_exceeded`; committed-artifact contradictions to `internal_invariant_error`; and exact resource/index/cancellation conditions to their typed categories.

## 3. Strong IDs, enums, complete keys, and artifact layout

Define distinct IDs for checks, findings, evidence, public uses/edges/link arcs/components/shells, bijection records, lineage traces, classification groups/quotient edges, probe requirements/attempts/rays, AABB nodes, triangle pairs, cleanup replay steps, precision paths, report/serialization/digest sections, and replay selectors. They are not aliases for public indices, predecessor IDs, `I`, `size_t`, pointers, or task IDs.

Define fixed-width closed enums for check class/disposition, finding severity/publication effect, edge/link/triangle/lineage/classification/selection/probe/ray/triangle-relation/cleanup/report/replay/final dispositions. Reject unknown values; enum names and free-form strings are not serialization authorities.

Use full lexicographic keys. Required examples:

```text
public_use = (from, to, facet, corner)
public_edge = (min endpoint, max endpoint)
link_corner = (vertex, facet, corner)
component = (sorted facet members, sorted vertex members, incidence bytes)
bijection = (public domain/position, predecessor complete key, exact bits,
             mapped incidence bytes, occurrence/lineage bytes)
lineage = (public entity, cleaned entity, C12, C11, C10, C09, C08/C07,
           source lineage, cleanup action path)
classification_group = (minimum primitive key, sorted members, boundary delta keys)
probe_requirement = (coverage class, referenced complete key, policy version)
probe_attempt = (requirement, facet, anchor ordinal, direction ordinal,
                 dyadic exponent, exact point/enclosure bytes)
triangle_pair = (min facet, max facet)
maximum_witness = (quantity, conservative value bytes, entity/lineage key,
                   ledger/action key)
finding = (publication effect/severity, arbitration rank, check class, subcode,
           entity keys, expected/actual bytes, numerical bytes, versions)
```

Hashes never complete a key. Equal hashes require length/full-byte comparison.

`verified_boolean_result<T,I>` stores: header/context/policies/versions; immutable exact Component 14 mesh/bit records; reconstructed topology; bijection; geometry/classification/selection/probe/pair evidence; construction/cleanup/precision evidence; regenerated reports; mandatory-check evidence; accepted exceptions; final logical domains/digests; replay; findings; resource/work/transaction evidence; and immutable interpretation references. A failure stores no verified artifact or public mesh view.

## 4. Intake, evidence completeness, and preflight

Before reading public arrays, validate owner/context/operation/type/policies/platform; exact pending Component 14 statuses; absence of final status/digest/conversion token; verification floor; all candidate counts/ranges/maps/reports/domains/digests/dependencies; and provider compatibility.

Build a verifier-owned dependency graph and require the exact Component 01-14 chain, common owner/context/operation/types/policies, accepted local verifier dispositions, resolving digest references, no stale/cross-context/cross-operand/future-stage/mutable/scratch handles, matching ID domains/ranges, exact precision context/ledger snapshots, sufficient retained content/references, and lifetime leases through join/commit/rollback. A digest match alone is insufficient.

Build an evidence-completeness table. At minimum:

- each public vertex maps to one cleaned occurrence and complete source/event/cleanup/precision lineage;
- each public facet maps to one cleaned triangle, Component 12 triangle, Component 11 region, Component 10 selection, and Component 09 side evidence;
- each public edge/use maps through cleaned paired topology;
- each event-derived coordinate maps to one Component 08 event/Component 07 relation/construction;
- each cleanup-modified/removed entity maps to an action certificate/budget authorization;
- each report maximum maps to primitive records; and
- every required finding/replay reference resolves.

Do not infer missing evidence from equal coordinates, matching counts, proximity, or hashes.

Using checked arithmetic, preflight all public/topology/bijection/tree/pair/probe/ray/lineage/classification/cleanup/ledger/finding/report/serialization/re-ingestion/replay counts, products, prefix sums, byte sizes, `size_t`/`I` conversions, sentinels, task descriptors, temporary/persistent bytes, and work. Reserve before allocation. Reject hard-limit overflow rather than truncating/skipping a gate.

## 5. Transaction and orchestration

Run Component 15 in one final private transaction containing all scratch, finding, report, codec, replay, and proposed-success state. Predecessor handles remain immutable. Each phase returns a typed outcome and immutable evidence proposal. Canonical phase barriers validate/sort/merge proposals before the next dependent phase.

No final status, report, success digest, verified artifact, or ordinary mesh view becomes visible before commit. On failure/cancellation/exception, stop new work, join workers, finalize canonical primary failure/progress/replay, destroy proposed success, release reservations, and preserve predecessor artifacts.

## 6. Public lexical audit

Read through `PublicMeshReadView` and verify exact declared counts, finite coordinates, coordinate bits equal candidate bit records, exactly three indices per facet, representable non-sentinel in-range indices, three distinct indices, no adapter normalization/reordering/deduplication/narrowing, V1 optional arrays empty, and empty output has no vertices/facets.

Emit transaction-owned exact lexical records. Do not merge equal coordinates or classify topology from bits. Compare candidate public arrays through the adapter rather than a privileged builder path.

## 7. Independent public topology reconstruction

### 7.1 Directed uses and pairs

Emit three records per public facet `(from,to,facet,corner)`, check `3F`, sort by undirected endpoint pair then full directed-use key, and group exact index endpoints. Require exactly two uses and opposite directions. Derive reciprocal pairs independently, retain witnesses, and detect repeated directed uses, missing reverse uses, boundaries, same-direction pairs, and overuse. Equal-coordinate distinct indices remain distinct.

### 7.2 Vertex links

For each `(facet,corner)` at vertex `v`, derive its two incident public edge groups. Build a corner-link graph by crossing each paired edge while remaining at `v`. Require every incident corner exactly once, degree two, reciprocal transitions, and one closed cycle for every used vertex. Reject open chains, repeated corners, several cycles/bow ties, and degree mismatch. Canonicalize cycle start by minimum corner key and compare cyclic incidence with cleaned evidence only after reconstruction.

### 7.3 Components, shells, and topology summaries

Traverse facet adjacency through reciprocal edges from the smallest unassigned facet. Assign every facet and used vertex/edge exactly once, keep point/edge-touching occurrences separate when indices separate them, reject isolated entities, compute canonical member-set signatures, and compare with Component 13/14 claims.

Propagate orientation through paired edges. Compute `V`, `E`, `F`, Euler characteristic, orientability, and for each closed connected orientable shell require `2-chi` nonnegative/even and genus `(2-chi)/2` representable. Empty output has zero components. Compare topology-change claims only after independent reconstruction.

## 8. Internal/public bijection proof

Validate candidate maps as accelerators only. Build public structural descriptors from reconstructed facets/links/components and cleaned descriptors from Component 13 query views. Match by exact coordinate bits, valence, oriented local incidence, component/shell role, occurrence separation/multiplicity class, and normalized lineage—not coordinates alone.

Require equal vertex/facet counts; one-to-one public/cleaned vertices; one-to-one oriented facets up to forward cyclic rotation; one-to-one cleaned paired edges/public use pairs; equal links/components; exact nominal bits under signed-zero policy; preserved duplicate-coordinate occurrences; and total forward/reverse maps.

For equal structural candidates, refine with expanding incidence/lineage signatures. If ambiguity remains, compare the complete induced component mapping against Component 14 canonical labeling evidence. Resource exhaustion fails; never choose lowest producer ID. Compare reconstructed maps with stored maps and require equality.

## 9. Bounded triangle geometry and shell orientation

For every public triangle, import its three bounded points and independently evaluate projected signed double areas on YZ, ZX, and XY using Component 03. Select the definitely largest absolute lower-bound projection with X/Y/Z tie order. Require a definitely nonzero accepted area/orientation, agreement with cleaned orientation and shell propagation, and no inversion from public assembly.

A nominally nonzero but uncertainty-crossing area is not accepted unless an exact documented narrow boundary authorization applies and still proves the final contract. No unresolved zero area remains in ordinary success.

For each reconstructed shell, verify paired-edge orientation, derive bounded orientation/side evidence, compare with cavity/island nesting and output solid policy, and require occupied side to be established within precision. Empty output uses explicit empty semantics.

## 10. Event sharing, occurrence separation, and lineage

For every public vertex/facet/use, walk exact references backward through Component 14, 13, 12, 11, 10, 9, 8, 7, and source artifacts as applicable. Require all IDs owner/version/range-valid, acyclic, canonical, and complete.

For event-derived data require one event identity, one nominal coordinate, one bounded point/precision entry, identical consumers, one canonical relation producer, valid source supports, and carrier/event order consistent with published bounded parameter/tie records. Distinct event identities may have equal bits and must remain distinct. Event identity, output occurrence identity, cleaned occurrence, and public index are separate domains.

Require explicit occurrence duplication for topology separation/multiplicity/coincident sheets and prove cleanup did not weld unrelated occurrences. Missing lineage fails; no coordinate-based reconstruction is permitted.

## 11. Independent classification and Boolean selection

### 11.1 Primitive classification graph

Reconstruct classification primitives and adjacency from Component 05 exact topology plus Component 07 cut/contact/signed-delta records and Component 08 events. For each possible adjacency classify exact uncut zero-delta, cut/contact, nonzero directed delta, or invalid. Never consume Component 09 group labels during union.

Sort zero-delta edges by complete keys and run canonical BFS/union from the smallest unassigned primitive. Require each primitive exactly once, no group crossing a cut/nonzero delta, and complete member signatures. Compare the final partition with Component 09 only after reconstruction.

### 11.2 Quotient graph and winding potentials

Collapse independent groups, aggregate directed crossing deltas with checked integers, require reverse antisymmetry and duplicate compatibility, and solve integer potentials from canonical validated source shell seeds. When revisiting a group, require equal potential; otherwise emit the smallest canonical inconsistent cycle. Require supported winding domain and agreement with source shell semantics.

Derive both sides of every retained atom from group potentials and signed transition. Compare Component 09 labels as claims.

### 11.3 Selection

Apply the frozen Component 01 truth table independently to reconstructed A/B occupancy on each side. Require retention exactly when result occupancy changes, orientation from occupied to unoccupied according to convention, correct operand-swap/difference remapping, exact coincident ownership, absent internal two-sided surfaces, required multiplicity as separate manifold occurrences, and no false positive-volume connection across point/edge contact. Trace every public facet to exactly one retained region and compare with Component 10/11/12 ancestry.

## 12. Deterministic side probes and independent operand occupancy

### 12.1 Coverage requirements

Create mandatory requirements covering every output connected component, shell role, provenance/selection class, topology-change region, coincident-owner class, precision/conditioning extremum, and region implicated by nearby non-adjacent geometry. Assign each requirement to the minimum eligible public facet under a full key; one probe may satisfy several requirements only with explicit coverage records. No uncovered requirement is allowed.

### 12.2 Anchors and directions

Use versioned positive barycentric anchors, in order:

```text
(1,1,1)/3
cyclic (2,1,1)/4
cyclic (3,3,2)/8
permutations of (4,3,1)/8
```

Construct through Component 03 fixed formulas. Reject an anchor not definitely in the intended triangle interior under the required margin.

Use a fixed table of primitive integer directions (axis, face diagonal, body diagonal, and reviewed additional coprime vectors). Orient a candidate consistently with the bounded triangle normal; reject unresolved dot sign and try the next direction. Do not normalize or use random directions.

### 12.3 Offset interval and clearance

For each anchor/direction derive a safe offset interval from local precision, area/orientation margin, nearest unrelated conservative geometry, feature separation, and remaining tolerance. Enumerate positive power-of-two/dyadic candidates in a fixed order. Require representable distinct plus/minus points, both inside the allowable local prism, no crossing of another incident support, no unrelated output AABB overlap without definite clearance, and a probe segment intersecting only the intended boundary support. Never use one universal epsilon.

If an attempt is uncertain, continue by canonical attempt order. Collect all accepted attempts and select the smallest complete attempt key; do not let first worker completion win.

### 12.4 Independent operand occupancy

Evaluate A and B occupancy at each accepted plus/minus point independently from Components 09/10. Build verification operand trees from validated source triangles. For each fixed ray direction, use Component 03 support/intersection/projection predicates and Component 02 orientation plus the Component 01 half-open feature-ownership rule for exact ties. Sum signed crossings with checked integers and require definite winding in V1 0/1 domain. Ordinary uncertainty tries the next ray; it is not symbolically converted into an exact tie. If all rays are unresolved, fail.

Require one result side occupied and one unoccupied, transition direction consistent with facet orientation, result occupancy equal the frozen Boolean truth table, source side values consistent with independent classification, and the segment clear of a third surface. If no safe attempt exists for a required region, fail rather than skip.

## 13. Independent forbidden-intersection search

### 13.1 Primitive table and independent tree

Create one primitive per public facet containing exact public positions/bits, bounded points, conservative inflated AABB, reconstructed incidence/component, lineage/authorization class, and complete key. Rebuild bounds from points; never copy Component 06/13/14 nodes.

Build a flat top-down tree:

1. union range bounds with Component 03;
2. leaf at at most four primitives, sorted by complete triangle key;
3. otherwise choose axis with lexicographically greatest `(lower-rank span, upper-rank span)`, X/Y/Z ties;
4. sort by `(lower endpoint total-order key, upper endpoint key, triangle key)`;
5. split at `floor(n/2)`, nonempty children;
6. recurse left/right, store preorder nodes.

Independently verify membership, bounds, split decisions, child ranges, preorder, leaf capacity, and root coverage.

### 13.2 Canonical self-pair traversal

Start `(root,root)` and process node pairs from a sorted complete-key frontier. Prune only on definite closed-bound separation. For same internal node enqueue `(left,left),(left,right),(right,right)`; otherwise split the larger primitive-count node, with bound-span/key ties. Leaf pairs emit unordered facet pairs, then full-key sort/group assigns one semantic pair record. Never exclude by coordinate, component, source, or proximity.

### 13.3 Exact authorization

Classify pairs by exact public topology: shared edge, shared vertex only, topologically disjoint, explicitly authorized coincident occurrence, accepted same-patch relation, or unknown. Shared edge permits only the common edge/endpoints; shared vertex only that public point; coincident/same-patch permissions require exact owner/version/entity/support/multiplicity/envelope records. Equal coordinate bits with different indices are topologically disjoint absent explicit lineage.

### 13.4 Independent pair decision graph

Use a separately organized bounded graph:

1. validate primitives/authorization;
2. AABB separation;
3. independently build support planes;
4. evaluate six vertex/opposite-plane signs with exact-nominal evidence when needed;
5. prove one-sided separation or classify noncoplanar/coplanar/unresolved support;
6. for noncoplanar support, test all edges against opposite planes, construct bounded crossing parameters, and test closed-triangle containment in deterministic dominant projection;
7. for coplanar support, test projected edge-edge relations and point-in-triangle relations;
8. form the complete possible/definite support set;
9. compare with exact authorization; and
10. return definitely clear, permitted contact, forbidden interaction, or unresolved.

Do not call Component 07 high-level composite control flow. Detect transverse intersection, vertex penetration, edge-face/edge-edge crossing, coplanar partial overlap, unauthorized coincidence, unrelated sheets overlapping through uncertainty, and cleanup-introduced interaction. Tolerance is not adjacency expansion. If possible support exceeds permitted support and cannot be separated, fail closed.

For bounded fixtures compare accelerated results against exhaustive unordered all-pairs. Record tree/frontier/pair/relation counters. Large disjoint outputs must not exhibit accidental all-pairs narrow phase; hard limits fail rather than omit work.

## 14. Construction residual and envelope audit

Build the canonical set of all non-source/public coordinates and validate the construction/precision DAG: recognized owner/version/formula/type, valid sorted parent ranges, acyclic dependencies, one producer per canonical identity, every used construction reachable, every public reference resolving, and full-byte integrity.

As applicable independently verify source-edge parameter containment, nominal point in enclosure, edge residual, source-face plane residual, projected/barycentric containment, carrier consistency, all event consumers equal, inherited uncertainty/roundoff/conditioning/representation parents present, finite nonnegative outward bounds, stored envelope no smaller than recomputed primitive bounds, correct conditioning/tolerance disposition, and exact frozen rounding/signed-zero bits.

Do not choose an alternative nominal point. Reject definite residual/support failure, understated envelope, uncertainty exceeding tolerance, unresolved feature classification, inconsistent consumers, or incomplete lineage. Use specific geometry/tolerance categories unless committed records contradict.

## 15. Cleanup certificate replay

Reconstruct a verifier-private paired triangle complex from Component 12, not Component 13's final topology or cumulative counters. Store generation-checked occurrences, bounded coordinates/lineage, paired halfedges, triangles, links/components, alive state, correspondence, and patch interfaces. Do not rediscover or choose cleanup actions.

Before replay independently rebuild Component 12 pairs, cycles, links, components, residual cells, and obligations and compare the first before-state.

For every Component 13 action in canonical order:

1. validate version/kind/generations;
2. require exact before entities alive and matching;
3. reconstruct closed affected patch/exterior interface;
4. independently check action-specific link/fan/interface/obligation/topology eligibility;
5. validate created/removed/reused occurrences and halfedges without coordinate identity;
6. evaluate replacement support/residual/swept bounds/displacement through Component 03;
7. verify tolerance proposal/reservation/commit and no reused/omitted budget;
8. check affected/swept patch against external non-adjacent geometry with Component 15 pair provider;
9. apply the exact delta;
10. rebuild affected pairs/links/components;
11. compare complete after state/counters; and
12. update obligations exactly.

Implement explicit branches for every V1 action: zero-motion fan/occurrence split, prohibited zero-length handling, zero-area/fold replacement, valid short-edge collapse, collapse plus splitting, collinear simplification, bounded local retriangulation, and authorized removal of one whole closed component. Unknown/generic serialized mutations fail. Reject unsupported component merge/split, handle/cavity/genus change except authorized whole-component removal.

Recompute each displacement step/path, maximum realized displacement, local feature removal, component removal, reserved/committed/rolled-back/remaining budget, and cleanup precision. Reject missing/duplicate/reused records, cycles, wrong origin, understated cost, or unproved local no-new-intersection.

After all actions require zero obligations, rebuild final topology, canonicalize by complete final keys, and compare every alive vertex/edge/halfedge/triangle/bit/lineage/component with Component 13 and Section 8 public mapping. An action-log/final-artifact contradiction is internal error; coherent excess is cleanup budget failure.

## 16. Precision-ledger and tolerance aggregation

Scan every reachable primitive ledger/budget record rather than summaries. Validate owner/schema/units/quantity/formula/stage, finite nonnegative value, nominal containment, parent ranges/acyclicity/type compatibility, monotonicity, public/construction/cleanup coverage, committed/rolled-back state, duplicate consistency, and full-byte integrity.

Keep separate: machine floor; A/B inherited precision; construction uncertainty; cleanup coordinate uncertainty; representation effects; final output precision; one-step displacement; cumulative displacement; local feature removal; component removal; tolerance and remaining margin.

Using Component 03 outward formulas establish:

```text
final_output_precision >= machine floor, both input precisions,
                          every used construction/cleanup uncertainty,
                          every representation effect
maximum_realized_displacement >= every surviving cumulative path
reported feature/component removal >= every authorized witness
ordinary success requires every policy-bounded quantity <= its tolerance class
final_output_precision <= caller tolerance
```

For equal maxima choose the smallest full witness key after comparing conservative value total-order keys. Reject NaN/infinity/negative/inverted/unversioned entries, missing contributors, formula cycles, underreported maxima, overflow, or inconsistent remaining margin.

## 17. Report regeneration and completion evidence

Rebuild final topology, geometry, precision, cleanup, topology-change, provenance, resource, deterministic-execution, verification, re-ingestion, and replay/codec reports from Component 15 facts. Candidate reports are comparison inputs only; never promote their status.

Topology reports include exact entity counts, valence/link coverage, components/shells, Euler/genus/orientation, duplicate-coordinate groups without identity inference, and bijection coverage. Geometry reports include triangle margins, shell side evidence, probe coverage/attempts, independent occupancy, pair-search counters/dispositions, construction residuals, cleanup replay, precision/tolerance, and accepted exceptions.

Cross-check all count equations, component/genus/cavity changes, provenance ranges, maxima/witnesses, cleanup action/cost totals, resource reservations/actuals, provider versions, serialization lengths, replay identity, and finding references.

Define a fixed mandatory-gate bitmap with unique bits for dependency intake, lexical, edge pairs, links, components/shells, bijection, triangle geometry, shell orientation, lineage, classification, selection, probes, independent occupancy, forbidden pair search, constructions, cleanup replay, precision, reports, canonical encoding, re-ingestion, resources/determinism, and self-audit. A phase sets its bit only after final evidence. Success requires the exact mask.

Retain compact pass evidence: provider versions; candidate/predecessor commitments; checked counts; topology/bijection/pair/probe/cleanup/precision/re-ingestion/report/codec digests and extreme witnesses; completion bitmap; and accepted exceptions. Evidence records that checks ran; it does not substitute for them.

## 18. Canonical serialization, digests, and replay

Use Component 01 canonical bytes exclusively and traverse reconstructed/final data independently from Component 14 encoders. Fixed V1 domains:

1. verified public mesh content;
2. topology/bijection evidence;
3. geometry/probe/pair/construction/cleanup/precision evidence;
4. final reports;
5. provenance;
6. findings/truncation;
7. re-ingestion;
8. replay reference;
9. pass evidence; and
10. aggregate verified result.

Use explicit versions/tags, little-endian fixed-width fields, exact scalar bits, checked lengths/sequences, and sorted canonical records. Exclude native layout, pointers, `size_t`, capacities, allocator state, locale, thread/time data, and free-form exception text.

Preflight lengths, stream SHA-256, retain full bytes or reproducible immutable encoder/content references per policy, compare full bytes on equality/collision, and independently regenerate required Component 14 candidate domains. Candidate-byte mismatch is a committed assembly contradiction; do not silently replace it.

Final replay records exact source inputs/content references, normalized options, types/platform, all versions, predecessor/candidate commitments, resource/execution policy, selected verification phase, primary status/finding, sorted retained findings/truncation, canonical progress, optional focused selector, success content digest or failure-witness digest, report/evidence/re-ingestion digests, and replay digest.

Focused selectors are a closed union naming one public vertex/edge/facet/pair, event, classification group, retained use, cleanup action, precision path, report field, codec field, or re-ingestion check by complete keys. Focused replay cannot change the full-run disposition.

Decode safely: validate magic/versions/tags/types/enums/reserved fields/lengths/counts/digests before dependent allocation; reject duplicates, invalid order/ranges, missing fields, trailing bytes, unknown required tags; skip only permitted optional framed fields; preserve exact bits; never invoke callbacks, paths, environment, network, or external tools. Replay creates fresh services and reruns checks rather than trusting embedded pass flags.

Under frozen versions/platform/resources, replay must reproduce byte-identical success domains or the same failure category/subcode/primary key/numerical witness/progress/failure digest across thread counts.

## 19. Fresh public re-ingestion

Create a fresh Component 02 transaction over the exact public mesh through `PublicMeshReadView`, with exact bits/rings, independently verified output precision, and the same solid/contact policy. Do not reuse Component 14 round-trip state or Component 15 topology arrays; no normalization, rotation, deduplication, repair, or signed-zero alteration.

Run the full ordinary structural/solid portions required for repeated Boolean use: lexical/index/finiteness, reciprocal edges, closed links, shells, orientation, nesting/cavity/island semantics, triangular geometry/planarity, required input interaction checks, and canonical validated artifact verification.

Compare complete semantic descriptors: exact bits/indices, oriented cyclic facets, edge groups, link corner sets/cycles, component/shell member sets, orientation/nesting, separate equal-coordinate occurrences, absence of normalization, and imported precision at least final output precision. Commit a re-ingestion digest.

Compare re-ingested occupied-side semantics with probe classes. Empty output must re-ingest as empty. The result is ordinary-success eligible only if it can be used as a later operand under published precision. Representative tests must perform follow-on Booleans and prove monotonic precision.

Expected final validation failure maps to result geometry failure; a disagreement between conforming independent reconstructions is an internal regression.

## 20. Findings, arbitration, and localization

`verification_finding` contains versions, stable post-sort ID, publication effect/severity, category/subcode/checkpoint, policy versions, public/predecessor keys, exact bits/bounds/tolerance/margin, structured expected/actual facts, authorization, compact excerpt, deterministic summary template/parameters, focused selector, and complete evidence key/full-content reference.

Sort by publication effect/severity, Component 01 arbitration rank, check class, public/predecessor keys, subcode, exact numerical bytes, expected/actual bytes, and versions. Never use discovery order, hash, pointer, task, or time.

At barriers validate, sort, full-byte compare, coalesce only identical evidence, detect contradictory dispositions, preserve a mandatory primary slot, retain optional findings within canonical count/byte limits, and append explicit per-kind truncation counts. Truncation never changes/drops primary evidence.

Submit all rejecting candidates to Component 01's frozen reducer. Minimal structured witnesses are required: one malformed field, edge-use group, link graph, member-set/orientation cycle, mapping ambiguity, triangle projection, lineage path, quotient cycle, truth-table transition, probe attempts, facet pair/support authorization, construction DAG node, cleanup before/after patch, precision path, report/codec field, or re-ingestion mismatch.

Map geometry difficulty, tolerance, cleanup budget, resource, index, cancellation, malformed replay, and committed contradictions to their precise public categories. Do not label ordinary uncertainty as an internal bug.

## 21. Verified result and publication

`verified_boolean_result<T,I>` is non-default-constructible and created only by a private success factory requiring an open final transaction, exact completion mask, no rejecting findings, reconciled resources, finalized reports/domains/replay, and accepted self-audit. It stores the unchanged Component 14 mesh and all final evidence/reports/digests/replay references.

Construct `bounded_boolean_success<T,I>` only from a verified artifact. Expose/move the exact mesh, output precision, maximum tolerance, maximum displacement, public reports/digest, and permitted read-only replay/diagnostic handles. No conversion exists from pending candidate, cleaned topology, producer report, or topology-only artifact.

Success sequence: join workers; finalize findings; require none rejecting; require completion mask except self-audit; regenerate/compare bytes/digests; complete re-ingestion; reconcile resources; build private artifact; run self-audit; set self-audit evidence using a nonrecursive core/envelope digest design; final cancellation poll; atomically commit; expose ordinary success.

Failure sequence: stop new work; join; canonicalize progress/primary error; destroy proposed success; release temporary reservations; keep candidate inaccessible through success; do not expose final statuses/digest; finalize promised diagnostics/replay; return one typed failure.

Empty success requires both public arrays empty, all maps/tables consistent empty, selection/occupancy proof of emptiness, complete removal evidence if cleanup emptied the result, conservative precision, explicit empty probe/pair policy, successful empty re-ingestion, and provenance domains distinguishing origins while public content remains identical.

## 22. Resources, cancellation, deterministic parallelism, and performance

Account separately for lexical records, uses/edges/links/components, bijection candidates, triangle primitives/tree nodes/frontier/pairs/relation work, lineage/classification/selection, probes/rays, construction DAG, cleanup replay, precision paths, findings/bytes, reports/evidence/serialization/digests, re-ingestion, replay, tasks, temporary/persistent bytes, and abstract work. Preflight/reserve aggregate capacity or deterministic slices. Hard crossings fail; no mandatory check is reduced or skipped.

Charge deterministic work units to scans, comparisons, transitions, tree visits, pair predicates, ray attempts, cleanup actions, ledger parents, byte blocks, and re-ingestion. Parallel overhead is nonsemantic.

Poll cancellation at fixed checkpoints/work intervals. After observation, schedule no new result work, stop active tasks at safe points, join all, roll back, and report canonical completed checkpoint/range rather than request time/worker.

Provide strong transaction-level exception safety. Catch unexpected exceptions only at controlled boundaries, join, optionally record nonauthoritative debug text, and return deterministic internal error. Expected outcomes never use exceptions.

Implement/retain the complete serial V1 verifier. Permitted parallel work has immutable inputs/private outputs: facet-range extraction, frozen-incidence link checks, component-local triangle checks, lineage ranges, local classification evidence, preassigned probe attempts, disjoint tree/pair batches, independent construction nodes, noncrossing cleanup certificate primitive checks, local precision candidates, and fixed report/serialization chunks.

Canonical global grouping/solves, pair dedup/classification merge, sequential cleanup replay, probe winner selection, maxima/witness reduction, finding arbitration, report/byte/digest finalization, re-ingestion comparison, and publication use deterministic reducers. Prohibit shared topology mutation, task-completion IDs, first-success probe/ray, race-dependent pruning, atomic floating sums, arrival-order failures, partial publication, detached work, or return before join.

Target sorting/linear topology, lineage, report, and serialization work; top-down tree approximately `O(F log^2 F)` for the simple reference; output-sensitive pair work; cleanup replay proportional to initial state/action patches/pair work. Adversarial dense overlap may be quadratic and must complete or hit an exact hard limit. Large disjoint fixtures must not perform all-pairs narrow phase.

## 23. Self-audit and maintained independence

`VerificationSelfAudit` validates proposed artifact owner/version/type/status, ranges/coverage, completion bit/evidence ownership, retained counts, independently re-encodes audited core, verifies domain bytes/digests, report references, resource reconciliation/transaction eligibility, no pending/topology-only leak, unchanged Component 14 mesh content, and success-factory exclusivity. It does not repair or run missing gates.

Maintain a documented dependency matrix:

| Fact | Producer path | Component 15 path |
|---|---|---|
| public topology | C13/C14 pairs/maps | public facet sort/group/link traversal |
| internal/public map | C14 builder | structural/lineage matching and incidence proof |
| components/shells | C13/C14 labels | public adjacency/orientation reconstruction |
| event coordinates | C07/C08 | lineage DAG/consumer equality/residual audit |
| classification | C09 | primitive cut graph, zero-delta BFS, quotient potentials |
| selection | C10 | reconstructed side states plus frozen truth/symbolic tables |
| side occupancy | C09/C10 | deterministic probes plus independent source rays |
| output interactions | C06/C07/C13 | top-down tree plus separate pair graph |
| cleanup | C13 | sequential replay from C12 |
| precision maxima | C03/C13/C14 summaries | primitive ledger/path aggregation |
| reports/bytes | C14 builders | regenerated reports/separate traversal encoding |
| input compatibility | C14 round trip | fresh C02 ordinary re-ingestion |

Independent provider translation units must not include producer-private traversal/grouping/selector/scheduler/report-builder headers. Shared higher-level refactors require independence review, versioning where observable, matched-mutation tests, and another organization for the checked fact.

Matched mutations must include identical wrong edge/link/component labels; wrong map permutation; omitted broad-phase pair; wrong classification/selection; wrong event coordinate propagated to summaries; forged cleanup cumulative counter/final summary; understated precision in all producer reports; wrong canonical candidate bytes/digest; and false Component 14 round-trip. Each must be rejected. Zero survivors is mandatory.

## 24. Required tests, validation matrix, and implementation gates

All normative tests are in-tree, strict C++17, dependency-free, deterministic, and replay-producing. Component 16 later provides permanent exact-oracle/generator/shrinker/corpus/release infrastructure; Component 15 exposes the seams without a production exact dependency.

### 24.1 Known-valid/type matrix

Test empty results, tetrahedron, box, disconnected solids, nested cavity/island shells, point/edge-touching separate occurrences, equal/coincident operands for every operation/orientation, authorized coincident occurrences, cleanup splits/simplification/component removal, repeated Boolean precision chains, and all `float`/`double` with `uint32_t`/`uint64_t`. Assert exact mesh, topology, probes, pair search, construction/cleanup/precision, re-ingestion, reports/domains/digests/replay.

### 24.2 Intake/dependency mutations

Wrong/stale owner, operation/type/policy, versions/reserved fields, digest link, missing evidence, cross-context IDs, invalid/overlapping ranges, mutable/scratch lifetime, illegal final candidate status, replay/codec mismatch, dependency cycle/duplicate. Reject before large allocation.

### 24.3 Lexical/topology mutations

Nonfinite/changed bits/zero sign, optional arrays, nontriangle/out-of-range/sentinel/repeated index, isolated vertex, missing/duplicate/reversed facet, missing reverse/repeated/same-direction/three-use edge, bow tie, open/repeated link, wrong component, cross-component edge, reversed shell, weld/split equal-coordinate occurrence. Repair producer counts/maps/digests where possible; independent path must still reject.

### 24.4 Bijection mutations

Many-to-one/one-to-many vertices, wrong equal-coordinate occurrence, facet map permutation, wrong rotation/reversed match, redirected edge/component maps, omitted/double-covered entity, changed occurrence class or lineage. Full structural matching rejects all.

### 24.5 Triangle/shell geometry

Well-conditioned projections, slivers above/at/below boundary, unresolved collinearity, repeated-coordinate zero area, signed zero/subnormal/extreme exponents/translation/one-ULP mutation, outward/inward shells, cavities/islands, unresolved side. Compare bounded conclusions with Component 16 exact fixtures.

### 24.6 Lineage mutations

Duplicate event with differing coordinate, recomputed consumer, equal-bit event merge, wrong source/carrier, changed tie order, identity-domain conflation, omitted multiplicity, cleanup weld, missing precision parent, stale IDs.

### 24.7 Classification/selection

Known zero-delta groups, cuts, quotient cycles, nested shells, tie/coincident rules, source subdivision. Mutate zero-delta/cut, delta sign, cycle, seed, group, side label. Test all operations over disjoint/containment/overlap/equal/contact/coincident cases and algebraic/operand-swap metamorphics. Mutate internal surface, omitted boundary, orientation, truth entry, owner, multiplicity, false contact connection.

### 24.8 Probes/occupancy

Generous clearance, narrow gaps, thin shells, nearby sheets, high curvature, coincident ownership, point/edge contact, cleanup regions, precision extrema, cavities/islands, fallback anchors/directions, no-safe-probe. Assert exact coverage/attempt order/offset/clearance/independent occupancy/winner. Mutate unsafe offset, missing requirement, first-worker winner, unversioned direction, uncertain ray acceptance, third-surface crossing. Compare source winding with exact analytic fixtures.

### 24.9 Tree/all-pairs/pair relations

Golden tree layouts for empty/small/equal/skewed bounds; axis ties, median, preorder, union, signed-zero order, collisions. Compare accelerated candidate/disposition set with exhaustive all pairs. Mutate bound inflation, node bound, split, primitive, child/range, separation, adjacency exclusion, frontier omission/duplicate, overflow. Test separated, exact adjacency, point/edge-touching separate components, transverse/penetrating/edge crossings, coplanar contact/overlap, authorized/unauthorized coincidence, uncertainty-overlap sheets, cleanup crossing, one-ULP gaps, support exceeding authorization.

### 24.10 Construction/cleanup/precision

Construction endpoints/interiors/rounding/residuals/inherited precision/conditioning/events/signed zero/tolerance boundaries; mutate parent/formula/parameter/residual/bits/envelope/conditioning/rounding and understate by one representable step.

Replay every cleanup action and combination; mutate action order/duplication/generation/before/interface/link/after topology/unauthorized genus/weld/budget/origin/cost/intersection/final artifact/obligation. Require exact final replay.

Build ledgers where every quantity uniquely/tied supplies maximum; test path composition, missing parent, cycles, nonmonotonic/invalid units/NaN/negative/underreported maximum/wrong margin/tolerance boundary/rolled-back commit. Preserve quantity separation.

### 24.11 Reports/codec/re-ingestion/replay

Mutate every report count/status/witness/coverage/provider/bitmap/evidence relation with corrected digests where practical. Test golden bytes, framing boundaries, exact bits, endianness, sequence errors, unknown fields, truncation, trailing bytes, padding/capacity/locale, reordered records, forged/domain-substituted digests, forced collisions.

Fresh re-ingestion for every valid result and follow-on Boolean use. Malicious adapters reorder/rotate/reverse/narrow/text-roundtrip/normalize zero/flush subnormal/deduplicate/repair/omit precision/import producer topology.

Full/focused replay for success and every failure; supported old/unknown versions, corrupt lengths/counts/digests, exact bits, missing content records, changed resources, thread counts. Reproduce same result/witness/progress safely.

### 24.12 Independence, metamorphic, concurrency, resources, fuzz, performance

Run every matched mutation and prove it survives producer-only fixture but fails Component 15. Maintain clause-to-mutation mapping and zero survivors.

Metamorphics: source permutations, legal subdivision/triangulation, cyclic rotation, operand swap/remap, axis/sign/translation/power-of-two transforms, predecessor ID/storage/history permutations, task partitions, thread counts, repeated runs. Require exact canonical relation.

Run serial/1/2/max workers, reversed tasks, delays/yields, partitions, simultaneous failures, allocator changes, collisions. Require identical result/findings/probes/pairs/maxima/reports/counters/replay/digests; TSan clean.

For every resource test limit-1/limit/limit+1 and allocation failures. Cancel at every checkpoint/inner poll. Verify join, rollback, reservation return, predecessor validity, no status/success leak, canonical progress, valid promised replay.

Fuzz/mutate all artifact domains, deterministically shrink crashes/hangs/UB/false acceptance/rejection/nondeterminism/leaks/replay disagreement, serialize exact bits/options/artifacts, and add permanent regressions through Component 16. Run ASan/UBSan/TSan, strict GCC/Clang, architectures/platform probes, locales, and parent fast-math override. Structural counters must reject accidental all-pairs on large disjoint output.

### 24.13 Ordered implementation sequence

1. Schemas/versions/errors/result-factory skeleton — no unverified success path.
2. Dependency/evidence preflight/resources — every owner/version/range/count mutation fails early.
3. Lexical reader — exact and malicious adapter tests.
4. Edge/link/component/shell reconstruction — topology mutations pass.
5. Structural/lineage bijection — map/equal-coordinate mutations pass.
6. Triangle/shell geometry — exact boundary/adversarial tests.
7. Lineage audit — event/carrier/occurrence mutations.
8. Classification quotient potentials — graph/oracle tests.
9. Selection/orientation/multiplicity — operation/contact matrix.
10. Verification tree/exhaustive pairs — false-negative tests.
11. Independent pair graph/authorization — interaction matrix.
12. Probes/independent occupancy — fallback/unsafe/ray oracle.
13. Construction audit — containment/conditioning mutations.
14. Cleanup replay — all actions/topology/budget/intersection.
15. Ledger/path aggregation — maxima/tolerance mutations.
16. Reports/completion evidence — report/status/bitmap mutations.
17. Final codecs/findings/replay — goldens/malformed/collisions/arbitration.
18. Fresh re-ingestion — repeated Boolean/malicious adapters.
19. Verified factory/self-audit/public paths — atomicity/matched mutations.
20. Deterministic parallel integration — byte-identical/TSan.
21. Full qualification — all tests, zero mutation survivors, reviewed performance.

Do not mark complete or expose ordinary `bounded_boolean` success before Step 21.

## 25. Definition of done

Component 15 is complete only when:

- it consumes one immutable pending candidate plus owner-checked predecessor/capability views and returns one immutable verified result or typed failure;
- all owners, versions, digests, ranges, types, statuses, evidence, counts, resources, bytes, and lifetimes are validated;
- public mesh lexical content and index topology are independently audited without normalization or coordinate identity inference;
- directed uses, reciprocal edges, links, components, shells, Euler/genus, and orientation are independently reconstructed;
- public/internal vertices, facets, edge uses, components, coordinate bits, occurrences, and lineage are proven bijective;
- every triangle and shell is accepted under conservative bounded geometry;
- event sharing, carrier order, occurrence separation, multiplicity, and complete lineage are verified;
- classification groups and quotient potentials are rebuilt and consistent;
- Boolean selection/orientation/multiplicity/contact/coincident ownership equal frozen policies;
- every required region has a deterministic safe probe and independent operand occupancy evidence or success fails;
- a separately organized conservative hierarchy/pair graph checks all potentially forbidden interactions and agrees with exhaustive bounded tests;
- all construction residuals/support/conditioning/rounding/envelopes are conservative and within tolerance;
- every cleanup action is independently replayed from Component 12 and final state/budget/topology/intersection evidence equals Components 13/14;
- all primitive precision/budget records and paths are valid, separate quantity classes remain separate, and maxima dominate all contributors with deterministic witnesses;
- final reports and completion evidence are regenerated and no status is stronger than evidence;
- candidate/final/report/finding/re-ingestion/replay bytes and domain-separated digests are independently regenerated, portable, and collision-safe by full-content fallback;
- the exact mesh/final precision pass fresh ordinary Component 02 re-ingestion and follow-on Boolean use;
- findings are structured, deterministic, localized, truncation-safe, and replayable;
- success preserves the exact Component 14 mesh, atomically assigns only Component 15 final status, and is constructible only through the verified factory;
- every failure/cancellation/resource/exception path joins, rolls back, releases, preserves predecessors, exposes no ordinary success, and returns deterministic diagnostics/replay;
- serial and every permitted parallel schedule produce identical semantic output/reports/findings/replay/digests;
- include boundaries and matched-mutation tests prove independence from producer topology/grouping/classification/search/cleanup/aggregation/codec mistakes;
- all known-answer, exact-oracle, mutation, property, metamorphic, adversarial, all-pairs, probe, cleanup, precision, codec, re-ingestion, replay, fuzz/shrink, resource, cancellation, concurrency, sanitizer, platform, and structural-performance tests pass with zero required mutation survivors; and
- all production and normative-test code is self-contained strict portable C++17, standard-library-only, and uses neither legacy Boolean files nor external dependencies.
