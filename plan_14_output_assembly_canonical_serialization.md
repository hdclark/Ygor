# Plan 14: Output Assembly and Canonical Serialization

## 0. Scope and fixed V1 design

Implement **only Component 14** from `component_14_output_assembly_canonical_serialization.md`. Consume the immutable, independently verified `cleaned_triangle_manifold<T>` from Component 13 and the immutable predecessor/context services required by the specification. Publish exactly one immutable `assembled_output_candidate<T,I>` for Component 15. Do not repeat Boolean classification, alter retained occurrence multiplicity, repair topology, move or merge coordinates, perform cleanup, retriangulate, merge adjacent triangles into polygons, run Component 15's final geometry verification, set `geometry.status == tolerance_checked`, or expose an ordinary `bounded_boolean_success<T,I>`.

Freeze V1 as the following provider set:

```text
public_mesh_adapter:             direct_private_fv_surface_mesh_triangles_v1
public_mesh_readback:            direct_const_fv_surface_mesh_view_v1
component_reconstruction:        exact_paired_edge_component_scan_v1
canonical_graph_model:           typed_oriented_incidence_graph_v1
initial_partition:               semantic_bits_topology_roles_v1
partition_refinement:            full_signature_equitable_refinement_v1
automorphism_resolution:         individualize_refine_lexicographic_minimum_v1
public_content_labeling:          minimum_oriented_mesh_block_encoding_v1
artifact_tie_resolution:          normalized_cleaned_lineage_minimum_v1
component_ordering:              full_component_bytes_then_artifact_bytes_v1
vertex_ordering:                 canonical_graph_label_v1
triangle_rotation:               minimum_forward_cyclic_index_triple_v1
triangle_ordering:               component_triple_occurrence_key_v1
index_conversion:                checked_zero_based_unsigned_v1
public_topology_rebuild:          sorted_directed_edge_and_corner_link_v1
precision_aggregation:           component03_output_lineage_aggregate_v1
report_assembly:                 independently_recounted_monotonic_reports_v1
provenance_layout:               normalized_interned_lineage_tables_v1
logical_serialization:           component01_canonical_bytes_le_v1
digest_provider:                 component01_sha256_domain_separated_v1
round_trip_provider:             component02_compatible_public_readback_v1
producer_verifier:               independent_rebuild_reencode_compare_v1
execution_reference:             serial_complete_canonicalization_v1
```

The executable serial implementation is the semantic reference. Parallel work may create private component descriptors, refinement signatures, provenance blocks, report fragments, serialization fragments, and verification proposals from immutable inputs. Final partition colors, branch decisions, canonical minima, component ordering, public indices, facet ordering, maxima and witnesses, logical bytes, digests, selected failure, diagnostics, and replay must equal the serial reference byte-for-byte. A resource limit encountered during canonical labeling is a typed `resource_limit`; V1 must never fall back to cleaned IDs, source order, traversal order, pointer order, hash order, or worker completion order.

V1 has these non-negotiable output rules:

- `fv_surface_mesh<T,I>` remains the exact public carrier required by the broad plan;
- every cleaned vertex occurrence produces exactly one public vertex entry, even when several entries have bit-identical coordinates;
- every cleaned oriented triangle produces exactly one three-index public face;
- public topology is determined only by the index mapping derived from cleaned topology;
- output coordinates are copied from authoritative Component 13 nominal `T` values under the frozen Component 03 signed-zero rule; no arithmetic construction occurs in this stage except checked counts and report aggregation;
- `vertex_normals`, `vertex_colours`, `involved_faces`, and arbitrary `metadata` are empty in the V1 canonical public mesh; precision, topology, provenance, diagnostics, and replay remain in the surrounding candidate/result artifacts;
- public mesh content bytes exclude diagnostic/provenance presentation data, while separate domain-separated artifact and provenance encodings commit to those records;
- canonical equality and ordering compare full canonical bytes; SHA-256 is an accelerator and integrity digest, never the sole equality proof;
- the candidate remains `assembled_pending_independent_verification`; only Component 15 can promote it to ordinary success; and
- production and normative-test code is strict portable C++17, standard-library-only, and compiled in the bounded Boolean strict-floating target.

Do not call, adapt, copy, or derive implementation from `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`. Mark Component 14 complete in `tracker.md` only after every instruction and qualification gate in Section 26 is represented and this complete plan is committed.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Reuse `fv_surface_mesh<T,I>` as the final public carrier

`fv_surface_mesh<T,I>` in `YgorMath.h` already supplies the required lowest-common-denominator storage:

- `std::vector<vec3<T>> vertices`;
- `std::vector<std::vector<I>> faces` with zero-based indices;
- a valid default-constructed empty representation;
- direct const readback of coordinates and face rings;
- copy/move/swap behavior suitable for transaction-private construction; and
- preservation of distinct vector entries whose coordinate bits are equal.

Retain this public API and its existing users. Component 14 must not replace the public result type with a halfedge mesh or a new container. Build one private `fv_surface_mesh<T,I>` inside the Component 14 transaction, validate it through a narrow adapter, place it in the proposed immutable candidate, and expose it only after the Component 14 producer verifier accepts the complete candidate.

The public class imposes few invariants and has mutable public members. Therefore it is a carrier, not a correctness provider. The adapter must enforce the stronger Boolean contract and must not expose mutable storage outside the stage transaction.

### 1.2 Do not use public mesh mutation or convenience topology methods

Do not call:

- `merge_duplicate_vertices`;
- `convert_to_triangles`;
- `remove_degenerate_faces`;
- `remove_disconnected_vertices`;
- `simplify_inner_triangles`;
- `compute_vertex_normals`;
- `recreate_involved_face_index` or `apply_involved_face_index_diff`; or
- any orientation, hole-filling, remeshing, refinement, BSP-cleanup, or zippering utility.

These methods operate after loss of cleaned occurrence identity, may infer equivalence from coordinate distance, may delete or rewrite topology, may add derived arrays whose order is not part of the public Boolean contract, and do not carry Component 01 ownership, Component 03 precision, Component 13 lineage, deterministic failure, transaction, replay, or verifier evidence. The cleaned artifact is already final topology; Component 14 is not a repair stage.

Leave `involved_faces` empty in the canonical V1 mesh. Reconstruct incidence in transaction-owned exact tables. This avoids treating a mutable convenience cache as authority and avoids adding non-semantic derived state to public content.

### 1.3 Existing Ygor mesh verification helpers are references, not providers

`YgorMeshesVerification.h/.cc` and related tests contain useful examples of index checks, edge-use counts, and orientation checks. They are insufficient for Component 14 because they do not establish:

- owner/version compatibility;
- a bijection to `cleaned_triangle_manifold<T>`;
- one closed link per topology-distinct occurrence;
- duplicate-coordinate preservation;
- canonical labeling and exact automorphism handling;
- Component 03 precision/report aggregation;
- deterministic logical bytes and digest domains;
- transaction/resource/cancellation behavior; or
- independently reconstructible evidence for Component 15.

Do not call these helpers as a producer or publication gate. Reimplement the exact integer-incidence checks inside the bounded subsystem. Pure, semantics-free sorted-edge or link-graph primitives may be shared with Components 02/05 only after they accept strong owner-bound IDs, expose deterministic typed outcomes, and preserve independent higher-level traversals.

### 1.4 Reuse Component 02 public-mesh intake concepts through a narrow readback adapter

Component 02 already defines the authoritative way to read a public `fv_surface_mesh<T,I>` without trusting `involved_faces`, normals, metadata, or mutators. Component 14 must add a small shared `PublicMeshReadView<T,I>` or extend the Component 02 immutable-source adapter so the same rules can be used for:

- Component 14 structural readback;
- Component 15 independent lexical audit and re-ingestion; and
- later repeated-Boolean input import.

The shared adapter may expose only exact vertex/facet counts, exact coordinate bits, ring lengths, and checked indices. It must not normalize rings, deduplicate vertices, triangulate, reorder, infer topology, or silently narrow values. Component 14's round-trip check invokes the adapter's read path, not the private builder's arrays through a privileged shortcut.

Do not invoke the full Component 02 shell-semantics producer from Component 14. Component 14 performs only the structural subset required by its specification. Component 15 owns final re-ingestion and shell/geometry acceptance.

### 1.5 Reuse or extract the bounded subsystem's canonical-labeling kernel

Component 02's plan already requires a typed colored incidence graph, full-signature partition refinement, and individualization/refinement search with lexicographic minimum encoding. Component 14 requires the same foundational algorithm over a different graph schema.

Do not implement two unrelated graph-canonicalization engines. Extract or extend an internal, dependency-free `CanonicalIncidenceLabeling` kernel with these boundaries:

- caller supplies immutable typed nodes, immutable typed directed relations, initial semantic color bytes, candidate-leaf encoder, artifact tie encoder, and resource/cancellation capabilities;
- the kernel owns deterministic equitable refinement, unresolved-cell selection, individualization, search, safe pruning, canonical-minimum comparison, branch statistics, and collision-safe memoization;
- it compares full canonical byte strings, using Component 01 SHA-256 only to accelerate equality/order checks with mandatory full-byte fallback;
- it exposes no source IDs or output-specific semantics;
- it is compiled in the strict bounded target and tested independently with collision injection and exhaustive small-graph oracles; and
- Component 02 and Component 14 use separate graph builders and separate producer verifiers.

If the Component 02 implementation has not yet extracted a conforming kernel, Component 14 must perform the extraction rather than copy its algorithm. Preserve Component 02 behavior and golden bytes. A kernel version change that affects either component requires explicit provider-version changes and golden regeneration review.

### 1.6 Reuse Component 01 infrastructure exactly

Use Component 01 for:

- owner tokens, strong IDs, checked count/index/byte arithmetic, and type descriptors;
- typed `stage_outcome`/errors and deterministic error arbitration;
- resource reservations, leases, snapshots, and reconciliation;
- cancellation checkpoints and deterministic work polling;
- transaction state, worker joining, immutable artifact publication, and rollback;
- `CanonicalBytes` fixed-little-endian framed encoding;
- exact `float`/`double` bit encoding;
- clean-room in-tree SHA-256 and collision-test providers;
- diagnostics, replay accumulation, context/input digests, and stable summaries; and
- serial/deterministic-execution capability interfaces.

Do not create another byte writer, hash, owner registry, transaction type, resource ledger, replay container, error hierarchy, or task scheduler.

### 1.7 Reuse Component 03 scalar-bit and precision services

Use Component 03 for:

- qualified `T` descriptors and `FloatingBits` conversion;
- the frozen coordinate signed-zero policy;
- immutable bounded-point and precision-ledger references;
- output-lineage precision aggregation;
- committed cleanup displacement and feature/component-removal records;
- deterministic outward-rounded maxima and sums;
- tolerance eligibility and remaining-margin calculations; and
- verifier-facing immutable ledger/budget views.

Do not recompute construction uncertainty from coordinates, use raw `vec3` arithmetic, convert through text, or invent a Component 14 epsilon. Copy nominal values by exact bits. When the frozen output signed-zero rule changes only a zero sign bit, create the prescribed Component 03 representation-effect/ledger record and include it in reports; the default V1 coordinate policy should preserve source nominal signed zero unless the already frozen context says otherwise.

### 1.8 Reuse Component 13 as the sole cleaned-topology authority

Consume `CleanedTriangleManifold.h` and `CleanupQueries.h` directly. Reuse its final:

- `cleaned_vertex_occurrence_id`, complete key, coordinate bits, bounded point, precision entry, occurrence-separation/multiplicity class, incident range, closed link, component membership, and complete lineage;
- paired-edge and reciprocal halfedge records;
- outward oriented triangle records and conservative geometry evidence;
- connected-component records;
- cleanup actions, correspondence, budgets, topology effects, no-new-intersection evidence, diagnostics, replay, and digests; and
- predecessor-to-final/final-to-predecessor maps.

Validate these records defensively, but do not reinterpret cleanup obligations, rerun actions, change component membership, regenerate geometry, or use cleaned numeric IDs as canonical-label inputs. Use the complete normalized cleaned entity keys only for artifact-level tie resolution after the public-content minimum has been determined.

### 1.9 Existing serialization is unsuitable for canonical logical bytes

`YgorMathIOSerialization.h` and `YgorIOXMLSerialization.h` serialize mesh members through named text fields. They may include optional/derived arrays and metadata, use decimal floating text, and do not define Component 14's domain separation, exact binary scalar bits, canonical component order, fixed integer widths, or collision-safe framing.

Keep those APIs unchanged for ordinary Ygor I/O. Do not use them for Component 14 content, artifact, provenance, replay, or candidate bytes. Use Component 01 `CanonicalBytes` and the explicit layouts in Section 18. No native struct, padding, pointer, `size_t`, vector capacity, locale, RTTI name, or implementation-defined enum representation is serialized.

### 1.10 No suitable existing graph-canonicalization dependency exists

Ygor has no existing provider that satisfies complete automorphism resolution, resource-bounded fail-closed behavior, semantic typed incidence, deterministic full-byte minima, and independent verification. Implement the canonical-labeling kernel in-tree as described above. Do not call an external graph library, subprocess, package, or optional dependency.

## 2. Exact files, target integration, API, versions, and checkpoints

### 2.1 Production files

Add under `src/YgorMeshesBooleanBounded/`:

- `OutputAssemblyTypes.h` — Component 14 strong IDs, closed enums, complete keys, counters, status values, error payloads, and fixed V1 policy descriptors.
- `AssembledOutputCandidate.h` — immutable candidate schema, checked section views, candidate status, and narrow Component 15 query surface.
- `OutputAssembly.h/.cc` — typed stage entrypoint, fixed checkpoint orchestration, serial semantic reference, canonical private merge, transaction, and publication.
- `OutputAssemblyPreflight.h/.cc` — cross-artifact validation, exact count/index/byte/work bounds, capability compatibility, and resource plan.
- `PublicMeshAdapter.h/.cc` — private `fv_surface_mesh<T,I>` builder, exact const readback view, checked public type/index traits, and failure-injection test adapter interface.
- `OutputComponentReconstruction.h/.cc` — exact component reconstruction from Component 13 paired topology, member-set evidence, and component-local views.
- `CanonicalIncidenceGraph.h/.cc` — Component 14 typed oriented graph construction and initial semantic color records.
- `CanonicalIncidenceLabeling.h/.cc` — shared generic full-signature refinement and individualize/refine search kernel if not already extracted by Component 02.
- `OutputCanonicalLabeling.h/.cc` — component leaf encoding, public-content minimum selection, normalized artifact tie selection, component ordering, and labeling evidence.
- `OutputPermutation.h/.cc` — checked component/vertex/facet permutations, cyclic face rotation, dense public positions, and forward/reverse map construction.
- `PublicMeshBuilder.h/.cc` — exact coordinate-bit copy, checked `I` conversion, private public arrays, and builder readback audit.
- `PublicTopologyReconstruction.h/.cc` — independent directed-edge grouping, reciprocal pairing, vertex-link graph, component/Euler/genus reconstruction, and public/internal equivalence checks.
- `OutputPrecisionReports.h/.cc` — Component 03 aggregation queries, deterministic maxima/witnesses, topology/geometry/cleanup/resource/determinism report assembly, and known-budget gates.
- `OutputProvenance.h/.cc` — normalized interned lineage tables, per-public-entity coverage, compression, reverse lookup, and provenance digest input.
- `OutputLogicalCodec.h/.cc` — V1 public-content, artifact, provenance, replay-reference, evidence, and candidate aggregate encoders/decoders.
- `OutputRoundTrip.h/.cc` — readback through the shared public adapter and structural Component 02 compatibility checks.
- `OutputAssemblyVerifier.h/.cc` — independently implemented candidate intake, public reconstruction, map/report/codec/digest/round-trip verification, and mutation rejection.
- `OutputAssemblyQueries.h` — owner-checked immutable queries needed by Component 15, diagnostics, replay, and tests.

Extend existing bounded-subsystem infrastructure rather than creating parallel registries:

- `ContractVersions.h` for Component 14 provider, adapter, graph, refinement, automorphism, ordering, permutation, public-topology, map, report, provenance, codec, digest-layout, round-trip, candidate, replay-reference, and verifier versions;
- Component 01 stage/checkpoint/strong-ID/resource/error/diagnostic/replay registries;
- Component 03 output-copy/representation-effect formula registry only if the frozen signed-zero policy requires a new explicit ledger operation;
- the strict bounded Boolean object target and explicit instantiation list; and
- the bounded test target and CTest registration.

Do not add Component 14 translation units to an ordinary target that permits fast-math, reassociation, finite-only assumptions, unauthorized contraction, or an unqualified floating environment.

### 2.2 Test files

Add under `tests/mesh_boolean_bounded/`:

- `TestOutputAssemblyContracts.cc`;
- `TestPublicMeshAdapter.cc`;
- `TestOutputAssemblyPreflight.cc`;
- `TestOutputComponentReconstruction.cc`;
- `TestOutputCanonicalGraph.cc`;
- `TestOutputCanonicalRefinement.cc`;
- `TestOutputAutomorphisms.cc`;
- `TestOutputCanonicalOrdering.cc`;
- `TestOutputIndexCapacity.cc`;
- `TestOutputDuplicateCoordinates.cc`;
- `TestOutputCoordinateBits.cc`;
- `TestPublicTopologyReconstruction.cc`;
- `TestOutputEquivalenceMaps.cc`;
- `TestOutputPrecisionReports.cc`;
- `TestOutputProvenance.cc`;
- `TestOutputLogicalSerialization.cc`;
- `TestOutputDigestDomains.cc`;
- `TestOutputRoundTrip.cc`;
- `TestOutputAssemblyMutation.cc`;
- `TestOutputAssemblyProperties.cc`;
- `TestOutputAssemblyAdversarial.cc`;
- `TestOutputAssemblyFuzzReplay.cc`;
- `TestOutputAssemblyResourcesCancellation.cc`;
- `TestOutputAssemblyDeterminismConcurrency.cc`;
- `TestOutputAssemblyStructuralPerformance.cc`;
- `OutputAssemblyFixtures.h/.cc`;
- `OutputCanonicalExhaustiveOracle.h/.cc`;
- `OutputTopologyOracle.h/.cc`;
- `OutputMutationBuilders.h/.cc`; and
- `GoldenOutputAssemblyV1.h`.

Keep exhaustive graph permutation, arbitrary-precision support used by shared test infrastructure, corrupt-candidate builders, constrained fake index types/adapters, fuzz generators, shrinkers, and golden regeneration tools test-only.

### 2.3 Typed entrypoint

Provide an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const assembled_output_candidate<T,I>>>
assemble_output_candidate(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const validated_operands_view<T,I>& validated,
    const source_triangle_complexes_view<T,I>& source_triangles,
    const canonical_source_manifolds_view<T,I>& source_manifolds,
    const canonical_intersection_complex_view<T,I>& intersections,
    const retained_surface_complex_view<T,I>& retained,
    const polygonal_output_complex_view<T,I>& polygonal,
    const triangulated_output_complex_view<T,I>& triangulated,
    const cleaned_triangle_manifold_view<T>& cleaned,
    const output_assembly_capabilities<T,I>& capabilities);
```

The exact predecessor bundle may use a single owner-checked `output_verification_dependencies_view<T,I>` to avoid a long parameter list, but the bundle must expose immutable typed views and exact dependency digests. It must not expose mutable implementation storage, caller meshes, user callbacks, arbitrary graph providers, arbitrary serializers, external allocators with semantic behavior, or unversioned heuristics.

`output_assembly_capabilities` freezes every V1 provider/schema version, recognized predecessor versions, public adapter layout, scalar/index combinations, signed-zero policy compatibility, canonical-label resource limits, codec/digest layouts, report detail, Component 15 handoff version, and deterministic execution capability. Validate the complete capability set before authoritative allocation.

The function returns one independently verified immutable candidate or one typed failure. The public mesh inside a failed/rejected candidate remains transaction-private unless diagnostic policy explicitly retains an internal artifact; it is never returned through ordinary success.

### 2.4 Stable checkpoints

Use these fixed Component 14 checkpoints in order:

1. context, public output policy, type, and capability validation;
2. predecessor owner/version/digest/dependency validation;
3. cleaned-manifold range, pair, triangle, link, component, report, and precision audit;
4. exact count, public-index, byte, canonical-work, and replay preflight;
5. persistent and peak resource reservation;
6. exact connected-component reconstruction from cleaned paired topology;
7. component membership and Component 13 report comparison;
8. typed canonical incidence graph construction;
9. initial semantic partition construction;
10. stable full-signature partition refinement;
11. unresolved-cell selection and automorphism search;
12. public-content minimum and artifact-tie minimum selection per component;
13. canonical component ordering;
14. checked public vertex permutation and `I` conversion;
15. oriented cyclic triangle rotation and canonical facet ordering;
16. complete internal/public vertex, face, edge-use, and component map construction;
17. private public-mesh allocation and coordinate-bit copy;
18. private public-facet construction;
19. public mesh lexical readback audit;
20. independent public directed-edge and undirected-edge reconstruction;
21. independent public vertex-link and connected-component reconstruction;
22. public/internal structural bijection and duplicate-coordinate-separation verification;
23. precision, tolerance-use, cleanup, topology-change, resource, and determinism aggregation;
24. normalized provenance table construction and coverage audit;
25. logical public-content/provenance/artifact/evidence/replay-reference encoding;
26. domain-separated digest construction and collision-safe structural comparison;
27. structural public-mesh round-trip and Component 02 compatibility audit;
28. proposed immutable candidate construction with pending statuses;
29. independent Component 14 candidate verification and re-encoding;
30. resource reconciliation and final cancellation poll; and
31. transaction commit.

Poll cancellation at every checkpoint and at deterministic work-count intervals inside large component walks, signature construction, refinement rounds, branch expansion, canonical byte comparisons, permutation/map construction, coordinate/facet batches, topology grouping, report/provenance batches, serialization, round-trip, and verifier passes. Never poll based on wall-clock time.

## 3. Strong IDs, closed enums, complete keys, and candidate layout

### 3.1 Strong ID domains

Define non-interchangeable strong IDs for at least:

- `assembly_component_id`;
- `assembly_graph_node_id`;
- `assembly_partition_color_id`;
- `assembly_partition_cell_id`;
- `assembly_refinement_round_id`;
- `assembly_search_state_id`;
- `assembly_branch_id`;
- `assembly_canonical_label_id`;
- `public_vertex_position_id`;
- `public_facet_position_id`;
- `public_corner_position_id`;
- `public_directed_edge_use_id`;
- `public_undirected_edge_id`;
- `public_link_arc_id`;
- `public_component_id`;
- `output_provenance_record_id`;
- `output_report_record_id`;
- `output_serialization_section_id`;
- `output_digest_domain_id`;
- `output_round_trip_evidence_id`; and
- `output_assembly_verification_finding_id`.

Public positions are internal checked ordinals, not aliases for cleaned IDs, `I`, `size_t`, pointers, hash values, or temporary graph IDs. Convert a `public_vertex_position_id` to `I` only through the checked adapter after global capacity proof. Search-state and branch IDs are transaction-private and never appear in semantic candidate bytes.

### 3.2 Closed enums

Use explicit fixed-width nonzero enumerators and reject unknown values for:

- public mesh adapter disposition: supported direct layout, unsupported scalar, unsupported index, unsupported face layout, invalid;
- graph node kind: vertex occurrence, triangle, oriented corner, paired edge, component anchor, invalid;
- graph relation kind: triangle-to-corner, corner-to-triangle, corner-to-vertex, vertex-to-corner, next-corner, previous-corner, corner-to-paired-edge, paired-edge-to-corner, paired-corner, component-member, invalid;
- partition state: initial, refining, stable-discrete, stable-nondiscrete, invalid;
- branch disposition: expanded, pruned-safe-prefix, memoized-full-state, complete-leaf, work-rejected, invalid;
- labeling disposition: unique-after-refinement, automorphism-resolved, resource-exhausted, inconsistent, invalid;
- component comparison result: less, equal-public-content, greater, collision-full-byte-fallback, invalid;
- coordinate-copy disposition: exact-bits-preserved, authorized-zero-sign-canonicalized, invalid;
- public topology disposition: lexical-valid, paired-manifold, closed-links, components-valid, bijective, invalid;
- candidate topology status: assembled-pending-independent-verification, invalid;
- candidate geometry status: finite-and-bounded-pending-independent-verification, invalid;
- serialization domain: public-mesh-content, artifact, provenance, assembly-evidence, replay-reference, aggregate-candidate, invalid;
- round-trip disposition: exact-structural-match, adapter-reordered, adapter-deduplicated, adapter-narrowed, adapter-malformed, invalid; and
- producer-verifier disposition: accepted, rejected-contract, rejected-labeling, rejected-public-mesh, rejected-maps, rejected-reports, rejected-codec, rejected-round-trip, invalid.

Compiler enum names, RTTI, implicit integer values, free-form status strings, and exception text are not serialization authorities.

### 3.3 Complete canonical keys

Define lexicographic complete keys, encoded using Component 01 canonical primitives. At minimum:

```text
component_member_key =
    (member kind,
     cleaned complete entity key bytes,
     exact oriented incidence key,
     normalized occurrence separation/multiplicity key)

initial_graph_color_key =
    (graph node kind,
     semantic payload schema version,
     exact coordinate bits when node is a vertex,
     local arity/valence,
     occurrence separation/multiplicity class where required,
     policy-recognized non-presentation semantic role bytes)

refinement_signature =
    (prior full color bytes,
     sorted runs of (relation kind, direction, neighbor full color bytes, multiplicity),
     graph schema/provider versions)

partition_cell_key =
    (full stable color bytes,
     cell size,
     graph node kind,
     sorted member structural signatures)

search_state_key =
    (ordered partition cell full bytes,
     individualization path full bytes,
     safely determined candidate-encoding prefix,
     graph/provider versions)

public_component_content_key =
    (component public-content bytes length,
     component public-content SHA-256 accelerator,
     complete component public-content bytes)

artifact_tie_key =
    (winning public-content bytes,
     normalized sorted cleaned vertex/edge/triangle complete-key bytes under the labeling,
     normalized provenance summary bytes,
     occurrence multiplicity ranks,
     provider versions)

public_vertex_key =
    (canonical component position,
     canonical graph vertex label)

public_facet_key =
    (canonical component position,
     minimum forward cyclic public index triple,
     canonical face occurrence/tie bytes when required)

public_directed_use_key =
    (from public position,
     to public position,
     public facet position,
     local corner)

maximum_witness_key =
    (quantity class,
     conservative exact bound bits,
     canonical public/predecessor entity key,
     ledger/action record key)
```

A hash is never a complete key. If two records compare equal by digest, compare lengths and full bytes. Equal complete keys for distinct records are an invariant failure unless the schema explicitly represents an equal-content multiset and assigns a canonical occurrence rank from the winning whole-component/whole-artifact labeling.

### 3.4 Candidate section layout

`assembled_output_candidate<T,I>` contains canonical immutable sections in this order:

1. header, stable context digest, operation, scalar/index descriptors, output policy, candidate status, and all provider/schema versions;
2. fully populated canonical `fv_surface_mesh<T,I>` with only V1 semantic members populated;
3. exact public coordinate bit records and coordinate-copy dispositions;
4. canonical component records and contiguous vertex/facet ranges;
5. cleaned-vertex to public-position and public-position to cleaned-vertex maps;
6. cleaned-triangle to public-facet and public-facet to cleaned-triangle maps;
7. cleaned-paired-edge to two public directed-use mappings and reverse public-use mappings;
8. canonical labeling/refinement/search evidence, including winning public and artifact encodings;
9. independently reconstructed public directed edges, undirected edges, vertex links, components, Euler/genus summaries, and structural evidence;
10. candidate output precision, tolerance-use maxima, deterministic witnesses, and remaining margin;
11. topology, geometry-pending, cleanup, topology-change, resource, determinism, and structural round-trip reports;
12. normalized provenance dictionaries and per-public-entity ranges;
13. public-content, provenance, artifact, assembly-evidence, replay-reference, and aggregate candidate logical encodings or reproducible retained encoder records as permitted by policy;
14. corresponding domain-separated SHA-256 digests and full-byte collision-fallback references;
15. immutable predecessor dependency handles/digests required by Component 15;
16. deterministic diagnostics and replay metadata;
17. producer resource/work counters; and
18. independent Component 14 verifier report and verifier digest.

Runtime owner tokens and immutable handles remain in typed object fields for validation, but deterministic semantic encodings use stable context/artifact digests rather than process-specific token bytes unless Component 01 explicitly defines a stable owner representation. No section may reference mutable builder arrays, temporary graph nodes, partition workspaces, branch stacks, allocator storage, task-local IDs, or stack memory.

### 3.5 Required component, vertex, face, and map records

Each canonical component record contains at least:

- canonical public component ID and contiguous global vertex/facet ranges;
- exact sorted cleaned member IDs for typed lookup plus normalized member complete-key bytes for deterministic evidence;
- canonical public-content bytes/digest and artifact-tie bytes/digest;
- vertex, edge, halfedge, triangle counts;
- exact coordinate-bit bounds encoded without raw floating ordering;
- reconstructed Euler characteristic and orientable genus;
- cleanup/topology-change semantic summary references;
- refinement rounds, final cell counts, automorphism branch/prune statistics, and resource use;
- canonical labeling permutation and inverse permutation digests; and
- record digest.

Each public vertex record contains at least:

- public position and checked `I` value;
- one cleaned vertex occurrence ID and complete normalized key;
- exact output x/y/z bits and copy disposition;
- Component 03 bounded-point/precision-ledger reference;
- canonical component membership;
- incident public corner/use ranges reconstructed from facets;
- occurrence-separation, multiplicity, and coincident-sheet class;
- normalized provenance range;
- cleanup/precision report references; and
- record digest.

Each public facet record contains at least:

- public facet position;
- one cleaned triangle ID and complete normalized key;
- three distinct public positions and checked `I` values in preserved outward order;
- selected forward cyclic rotation index in `{0,1,2}`;
- three public directed-use IDs;
- canonical component membership;
- normalized provenance range;
- inherited orientation/area evidence references; and
- record digest.

Every forward map has a reverse map. Every map publishes exact range/count evidence and a deterministic digest. The verifier must be able to reconstruct the bijection without trusting stored totals.

## 4. Cross-artifact validation and preflight

### 4.1 Context and capability validation

Before reading predecessor records, validate:

- Component 01 context owner, stable context digest, operation, output policy, determinism policy, verification floor, diagnostics, replay, and transaction stage;
- supported `T` is the qualified Component 03 binary32/binary64 type and `I` is a supported unsigned non-bool public index type;
- output policy is exactly V1 triangulated oriented manifold;
- canonical-labeling, graph, codec, digest, report, adapter, and Component 15 handoff versions are known and mutually compatible;
- public coordinate signed-zero policy equals the frozen Component 03/context policy;
- the context remains ordinary-publication eligible before Component 15;
- no caller callback or unversioned provider is present; and
- resource ceilings cover every mandatory persistent and temporary class.

Reject unknown required versions, nonzero reserved fields, stale/cross-context capabilities, topology-only diagnostic contexts attempting ordinary assembly, or an unsupported public layout before authoritative allocation.

### 4.2 Predecessor dependency validation

Validate all supplied artifacts have:

- the same live context owner and stable context digest;
- compatible operation, operand-role mapping, scalar/index descriptors, contact/output/cleanup policies, and deterministic policy;
- successful local verifier dispositions;
- recognized schema/provider versions;
- correct dependency graph and predecessor digest references;
- immutable lifetime through Component 15; and
- no mutable caller mesh or private producer scratch reference.

Recompute or verify required section digests according to Component 01 immutable-handle policy. Validate every provenance reference needed by Component 14 resolves through Components 02, 04, 05, 08, 10, 11, 12, and 13. Missing lineage is not inferred from coordinate equality or matching counts.

### 4.3 Cleaned manifold structural intake audit

Independently scan Component 13 records and verify at minimum:

- vertex, paired-edge, halfedge, triangle, component, action, ledger, and provenance ranges;
- every authoritative coordinate is finite and its exact bits match its bounded-point nominal value;
- every triangle references three distinct cleaned vertex occurrences and three distinct valid halfedges;
- each triangle halfedge cycle closes in outward order;
- every paired edge owns exactly two reciprocal halfedges with exact reversed endpoint occurrence IDs;
- each halfedge belongs to exactly one triangle;
- every undirected edge has exactly two triangle uses;
- each vertex's incident-corner set reconstructs one closed link;
- every triangle and vertex belongs to exactly one reconstructed connected component;
- Component 13 component labels, counts, topology changes, cleanup action coverage, outstanding-obligation count, and digests agree with reconstruction;
- every surviving coordinate has a complete Component 03 ledger lineage;
- every changed/removed feature has the required cleanup and budget evidence;
- no unresolved residual cell or cleanup obligation remains; and
- Component 13 advertises compatibility with the selected Component 14 versions.

A contradiction is `internal_invariant_error`. Do not skip, duplicate, reorient, weld, or repair a contradictory record.

### 4.4 Checked count and index bounds

Using Component 01 checked arithmetic, derive exact or conservative upper bounds for:

- cleaned and public vertex, paired-edge, halfedge, triangle/facet, corner, component, and provenance counts;
- graph nodes: `V + F + 3F + E + C` under the V1 node model;
- graph relations in both directions and refinement signature entries;
- partition cells, colors, search states, individualization depth, branches, memo records, prefix encodings, candidate leaf bytes, and comparisons;
- component/local/global vertex and facet permutations and reverse maps;
- `3 * facet_count`, all component prefix sums, and every byte count;
- public `std::vector` sizes and each three-index nested face vector;
- public positions representable in `uint64_t`, `size_t`, strong internal ordinal types, and `I`;
- public sentinel exclusions declared by the adapter;
- topology reconstruction edge uses, undirected groups, corner links, components, and Euler work;
- report/provenance table entries and references;
- every logical encoding and retained byte/stream state;
- digests, diagnostics, replay, round-trip, producer verifier, and persistent candidate storage; and
- abstract work units for all of the above.

For zero-based V1 indices, `vertex_count == 0` is valid; otherwise require `vertex_count - 1 <= max_usable_I`, where `max_usable_I` accounts for any adapter-declared sentinel. Do not require `facet_count` itself to fit `I` unless the public representation or a map explicitly uses `I` for facet positions. Every count must fit its actual storage type.

Fail `index_overflow` for public index representability and `resource_limit` for configured count/byte/work ceilings. Perform these checks before public mesh construction. Tests must use constrained fake adapters and ledger-only reservations rather than exhausting host memory.

### 4.5 Resource plan

Reserve separately through Component 01 for:

- persistent candidate public mesh arrays;
- persistent maps, component records, reports, provenance, bytes/digests, diagnostics, replay, and verifier evidence;
- temporary component reconstruction and intake audit;
- canonical graph nodes/relations/initial colors;
- refinement signatures, sort buffers, partitions, branch stack, memoization, candidate bytes, and full-byte collision fallback;
- permutations, checked index arrays, and map builders;
- private public mesh builder and nested face allocations;
- public topology reconstruction and structural comparisons;
- precision/report aggregation and witness tables;
- provenance normalization/compression;
- serialization materialization or streaming work;
- round-trip readback and Component 02 structural adapter workspace;
- independent producer verifier; and
- abstract work units for each phase.

Persistent or peak work must be pre-reserved or use checked bounded growth leases. Resource exhaustion cannot truncate a graph, stop before proving a canonical minimum, omit a provenance record, skip a topology check, discard a report contributor, abbreviate mandatory bytes, or publish a noncanonical fallback.

## 5. Exact connected-component reconstruction

### 5.1 Reconstruction source and traversal

Reconstruct components from Component 13's exact triangle/paired-edge topology before canonical labeling. Do not trust only stored component IDs.

The serial reference must:

1. create one unvisited mark per cleaned triangle;
2. select the next seed by the least complete cleaned triangle key, used only to make diagnostics and temporary traversal deterministic;
3. traverse across each triangle's three reciprocal paired halfedges;
4. collect every reached triangle, edge, halfedge, and vertex occurrence exactly once in typed member sets;
5. reject a reference crossing to an already reconstructed different component;
6. derive sorted member lists by complete cleaned keys;
7. compute independent counts and a full member-set encoding/digest; and
8. compare the partition with Component 13 component records and topology-change reports.

Traversal seed/order must not become public ordering. The later graph canonicalization determines the canonical component representation. Coordinate contact or equality never connects components.

An empty cleaned manifold produces zero components. A component containing no triangle, an isolated cleaned vertex, or an edge outside a triangle is an invariant failure and is not serialized.

### 5.2 Component-local dense workspace

For each reconstructed component, create transaction-private dense local ordinals for its vertices, edges, triangles, and corners solely to index arrays efficiently. Populate local-to-cleaned and cleaned-to-local tables by sorted complete cleaned keys. These ordinals:

- are not public labels;
- are not graph initial colors;
- are not semantic serialization fields;
- may not break refinement or automorphism ties; and
- are discarded after maps/evidence are finalized.

Validate the maps are total and bijective. Forced permutations of local ordinals must leave the selected canonical bytes and all public results unchanged.

### 5.3 Independent topology summaries

Before canonical labeling, derive for each component:

- `V`, `E`, `F`, and `H = 3F = 2E` checked identities;
- exact valence and incident triangle count per occurrence;
- exact oriented cyclic corner/link structure;
- coordinate-bit multiset under the frozen signed-zero output rule;
- occurrence-separation and multiplicity class multiset;
- Euler characteristic `chi = V - E + F` using checked signed arithmetic;
- orientability evidence inherited from reciprocal outward-oriented triangles; and
- genus candidate `g = (2 - chi) / 2` only after proving the component is connected, closed, and orientable, with a nonnegative even numerator.

A failed identity is an invariant error. Genus is a report fact, not a canonical-label shortcut unless explicitly included by the fixed graph schema; including it as a redundant initial color must not replace exact incidence.

## 6. V1 canonical content graph

### 6.1 Typed graph nodes

Build one graph per reconstructed connected component. Use these node domains:

- one **vertex node** for every cleaned vertex occurrence;
- one **triangle node** for every cleaned triangle;
- three **corner nodes** for every triangle, preserving the triangle's outward cyclic order;
- one **paired-edge node** for every cleaned paired edge; and
- one **component-anchor node** connected to every typed member only when needed to prevent accidental cross-domain omission in the generic kernel.

The graph must represent the entire oriented indexed triangle manifold. It must not depend on coordinate-derived adjacency, arbitrary face starting corners, cleaned numeric IDs, or stored array positions.

### 6.2 Typed directed relations

Emit explicit directed relation records with fixed enum values:

- triangle `contains-corner` and corner `belongs-to-triangle`;
- corner `uses-vertex` and vertex `has-corner`;
- corner `next` and `previous` preserving outward cyclic orientation;
- corner `uses-paired-edge` and edge `has-corner`;
- paired edge `opposite-corner` relations between its two directed uses;
- anchor/member relations by member domain if the anchor is enabled.

Each relation stores source node, target node, relation kind, and multiplicity one. Validate inverse-role completeness where the schema requires it. Sort relation runs by `(source temporary ordinal, relation kind, target temporary ordinal)` only for storage; refinement consumes relation kind plus neighbor color and is invariant under temporary ordering.

The directed `next` relation is essential: an orientation-reversing graph mapping is not equivalent to an orientation-preserving public mesh labeling merely because undirected adjacency matches. Reversed face triples are never candidates for canonical cyclic rotation.

### 6.3 Public-content initial colors

Initial public-content color bytes contain only semantic information that is allowed to affect the canonical public mesh:

For a vertex node:

- node kind/version;
- exact output coordinate x/y/z bits after the frozen signed-zero rule;
- exact valence and incident triangle count;
- versioned occurrence-separation/multiplicity class only where it is necessary to preserve topology-distinct sheets and is itself presentation-independent; and
- no source/caller/cleaned ordinal or detailed provenance.

For a triangle node:

- node kind/version;
- fixed arity three;
- orientation policy version; and
- any versioned multiplicity role required to distinguish legally repeated topology, never source facet order.

For a corner node:

- node kind/version;
- fixed role indicating it is an oriented triangle corner; and
- no local corner ordinal, because `next`/`previous` relations encode the cycle without privileging a starting corner.

For a paired-edge node:

- node kind/version;
- fixed two-use role; and
- any topology-separation role that is semantic in the final manifold.

For the component anchor:

- node kind/version only.

Do not include precision values, cleanup action order, source operand priority, caller indices, cleaned IDs, pointer values, container positions, hash values, thread IDs, or detailed provenance in public-content colors. Public mesh bytes must remain invariant under equivalent source presentation and cleanup history where the same cleaned oriented coordinate/topology artifact results.

### 6.4 Artifact tie colors

Construct a separate normalized artifact tie payload for each graph node from Component 13 complete entity keys and normalized lineage. This payload may include:

- surviving/creating cleanup action lineage expressed by canonical action keys, not commit-slot IDs;
- complete predecessor lineage multisets after canonical remapping;
- occurrence multiplicity/separation keys;
- normalized source/event/carrier/retained-use/face-region/Component 12 lineage; and
- record schema/provider versions.

Artifact tie payloads do not participate in selecting the least public-content encoding. They select a deterministic internal/public map among graph labelings that yield exactly the same public-content bytes. They must be independently invariant under caller vertex/facet/component permutations and documented operand remapping.

If two distinct nodes still have equal normalized artifact tie payloads, resolve them only through the winning whole-graph labeling and explicit equal-content occurrence ranks. Never use temporary ordinals. If the predecessor schema claims complete unique keys but supplies duplicates, fail as an invariant contradiction.

### 6.5 Graph construction audit

Before refinement, verify:

- node counts match `V + F + 3F + E (+1)`;
- every triangle has exactly three corner nodes in one `next`/`previous` cycle;
- every corner has exactly one vertex, triangle, and paired edge;
- every vertex's incident corner count equals cleaned valence/fan evidence;
- every paired edge has exactly two corner uses and they are opposite directed uses;
- every typed relation has the required inverse relation;
- no relation crosses components;
- no graph node is unreachable from the anchor or incidence structure; and
- graph bytes reconstructed from typed records are independent of temporary insertion order.

## 7. Deterministic equitable partition refinement

### 7.1 Initial partition

Sort distinct initial public-content color byte strings lexicographically using full bytes. Assign dense color IDs in that order. Group nodes into cells by equal full color bytes. SHA-256 may cache comparisons, but equal digests require full-byte comparison.

The initial partition record stores color bytes, member count, typed domain count, and a digest. Member arrays may use temporary ordinals internally but are never compared to resolve a semantic tie.

### 7.2 Refinement signature

For every node in each round, construct:

```text
signature =
    frame(prior full color bytes) ||
    for each sorted relation run:
        frame(relation kind,
              direction,
              neighbor full color bytes,
              multiplicity) ||
    frame(graph/refinement versions)
```

Sort relation runs by full `(relation kind, direction, neighbor color bytes)` and compress exact duplicates with checked multiplicity. Do not depend on adjacency vector order.

Sort all node signatures by full bytes. Assign new dense color IDs in lexicographic signature order. A round is stable only when the induced partition is identical, not merely when the number of colors is unchanged. Store a collision-safe round digest and structural counters.

### 7.3 Termination and consistency

Refinement terminates after at most the node count of strict partition splits. Charge every signature entry, byte, sort comparison, and round before work. Reject:

- a round that merges prior cells;
- a signature with missing/extra relation multiplicity;
- unknown relation kinds;
- count/byte overflow;
- unstable repeated partition state without equality;
- resource exhaustion; or
- cancellation.

A discrete stable partition yields one candidate labeling without branching. A nondiscrete stable partition enters the individualization/refinement search.

### 7.4 Deterministic unresolved-cell selection

Choose the branch cell by this total key:

1. smallest full stable color bytes;
2. smallest cell size;
3. fixed graph node-domain rank;
4. full canonical cell-neighborhood signature; and
5. provider version.

Do not use the least temporary member to choose the cell. The cell's members are a mathematical set. Branch enumeration order may use full current structural/artifact proposal keys for efficiency, but correctness must inspect every non-pruned member and the final minimum must not depend on enumeration order.

## 8. Exact automorphism handling and canonical minimum search

### 8.1 Individualization operation

For a selected nondiscrete cell and one candidate member, create a child partition by assigning that member a new individualized color framed from:

- the selected cell's full color bytes;
- the fixed individualization marker/version;
- the current search depth; and
- the member's current full structural signature, not its temporary ordinal.

All other cell members retain a common companion color distinct from the individualized color. Immediately run full refinement to stability.

The individualization path is private search evidence. It must not leak temporary IDs into candidate public bytes.

### 8.2 Complete leaf encoding

At a discrete leaf:

1. order graph nodes by final full color bytes;
2. derive the induced order of vertex nodes;
3. map each component-local vertex to a dense local public label `[0,V)`;
4. map each triangle's three outward corners to local labels;
5. rotate the triple to the lexicographically smallest of its three forward cyclic rotations;
6. sort triangles by the rotated triple plus the fixed legal occurrence tie key;
7. encode the complete component public-content block from exact coordinate bits and sorted local triples; and
8. encode the artifact tie block under the same labeling from normalized complete cleaned keys/provenance.

Reject a leaf that does not produce a permutation, repeats/omits a vertex or triangle, changes triangle orientation, maps distinct topology occurrences to one label, or produces invalid edge/link topology.

### 8.3 Winning comparison

Compare leaf candidates in this order:

1. complete component public-content bytes lexicographically;
2. if equal, complete normalized artifact tie bytes lexicographically;
3. if both equal, exact equal-content occurrence rank vectors produced by the whole labeling; and
4. if still indistinguishable, treat the labelings as equivalent and retain one canonical equivalence certificate rather than consulting traversal order.

Keep the lexicographically least candidate. Digests and lengths may reject inequality quickly, but full bytes decide equality/order. Store why each losing leaf was greater at the first differing framed field for diagnostics/performance counters, subject to diagnostic limits.

### 8.4 Safe pruning

V1 may prune only when a proof-producing lower-bound prefix shows every completion of the child partition is lexicographically greater than the current best complete public-content bytes, or when an identical full partition state plus identical relevant path constraints was already solved.

Do not prune from:

- digest order;
- temporary member order;
- an incomplete heuristic color histogram;
- a nominal coordinate bound;
- source/provenance preference before public-content equality;
- estimated automorphism group size; or
- wall-clock budget.

The safe-prefix encoder must mark unknown suffixes with a value proven lexicographically minimal for the framed schema. Test it exhaustively on small graphs against unpruned enumeration. Disabling pruning in a test provider must produce the same minimum.

### 8.5 Memoization

Memoization is optional. A memo key must contain full canonical partition-state bytes and every semantic condition that affects valid completions. A hash table may locate candidates, but equality compares full state bytes. Cache insertion/eviction must not affect correctness or observable ordering. If the configured cache fills, continue without memoization if remaining work is already reserved; do not evict in a schedule-dependent manner that changes resource outcomes. Alternatively, use a fixed deterministic cache policy included in the provider version.

### 8.6 Resource and failure behavior

Charge and cap:

- refinement rounds/signature entries/bytes;
- individualization depth;
- branch states;
- safe-prefix encodings;
- complete leaf encodings;
- full-byte comparisons;
- memo records;
- collision fallbacks; and
- abstract work.

If the complete minimum cannot be established within limits, return `resource_limit` with component signature, unresolved cell key, depth, branches, current best digest/length, required/available counters, and replay identity. Do not publish a best-so-far labeling.

### 8.7 Exhaustive test oracle

For bounded components with a small configured vertex/node count, the test-only oracle must enumerate all vertex permutations or all color-compatible graph permutations, filter orientation/incidence-preserving labelings, encode complete public component bytes, and select the exact lexicographic minimum. Compare the production provider's bytes and equivalence class. The oracle must not call production refinement or branch selection.

## 9. Canonical component, vertex, and facet ordering

### 9.1 Component block construction

For every component's winning labeling, construct a self-contained component public block containing:

- local vertex count;
- exact local vertex coordinate bits in canonical vertex order;
- local facet count;
- each canonical forward-oriented local index triple in canonical facet order; and
- public content/provider version fields required for unambiguous comparison.

Do not encode global offsets inside the component comparison block. This ensures equal components compare equal independent of their eventual global positions.

### 9.2 Component ordering

Sort components by:

1. full component public-content bytes;
2. full artifact tie bytes for deterministic internal maps among equal public blocks;
3. complete normalized component member-set bytes; and
4. equal-content occurrence rank from the winning whole-artifact multiset labeling if required.

Digest equality is never sufficient. For disconnected components with identical public bytes, exchanging their output blocks would leave public content unchanged; nevertheless maps/reports/provenance must use the normalized artifact tie/rank and never discovery order.

Do not order components by source operand, Component 13 numeric component ID, nominal bounding-box center, approximate coordinate comparison, signed volume, cleanup action order, or worker completion.

### 9.3 Global vertex order and offsets

Compute checked component vertex prefix sums in canonical component order. For each component, assign global `public_vertex_position_id = component_vertex_offset + local_canonical_label`. Validate:

- no overflow;
- positions cover exactly `[0, public_vertex_count)`;
- every cleaned vertex occurs once;
- every position receives one cleaned vertex;
- component ranges are contiguous and nonoverlapping; and
- conversion of every position to `I` succeeds.

Store forward/reverse maps in dense arrays indexed by final cleaned IDs and public positions only after validating ID ranges. The semantic order comes from canonical labels, not array positions.

### 9.4 Forward cyclic triangle rotation

For a cleaned triangle with mapped global positions `(i0,i1,i2)` in outward order, compare exactly:

```text
(i0,i1,i2)
(i1,i2,i0)
(i2,i0,i1)
```

Select the lexicographically least triple. Never consider `(i0,i2,i1)` or another reversal. Record the selected rotation and retain the cleaned triangle/provenance association.

Because positions are exact integers, no geometry/tolerance comparison is involved. Equal values inside a triple are invalid because cleaned triangle vertices must be distinct and the vertex map is injective.

### 9.5 Facet ordering

Within each canonical component, sort rotated triples lexicographically. If the representation legally permits two topologically distinct facets with equal triples, add a versioned canonical face occurrence key derived from the winning graph labeling and normalized cleaned complete key. In the ordinary V1 closed indexed two-manifold, equal oriented triples within one component are an invariant failure unless an explicit multiplicity schema says otherwise.

Compute checked component facet prefix sums. Assign `public_facet_position_id` in canonical component/facet order. Build cleaned-triangle/public-facet forward/reverse maps and verify total bijection.

### 9.6 Edge-use mapping

For every cleaned paired edge:

- locate its two cleaned halfedge/triangle uses;
- use the triangle map and recorded cyclic rotations to locate the exact public local corner for each directed edge;
- emit two public directed-use IDs with reversed public endpoints;
- require the two uses reconstruct one public undirected edge; and
- publish the cleaned-edge to two-public-uses map and reverse public-use to cleaned-edge map.

Do not rediscover this mapping from coordinate equality. The later public topology reconstruction independently checks it from public facets.

## 10. Public mesh adapter and private construction

### 10.1 Adapter traits

`PublicMeshAdapter<T,I>` must declare and verify:

- zero-based indexing;
- accepted unsigned `I` widths;
- maximum usable index and any reserved sentinel;
- coordinate scalar exactly `T`;
- face storage capable of exactly three indices;
- empty representation behavior;
- direct exact-bit coordinate write/readback;
- duplicate-coordinate preservation;
- whether move/swap commit operations are `noexcept` or require transaction-owned emplacement; and
- absence of implicit normalization, deduplication, reordering, or metadata insertion.

Production V1 specializes the adapter for `fv_surface_mesh<T,I>`. Tests provide adapters that intentionally reorder, deduplicate, narrow, reserve a sentinel, throw after N writes, or alter signed zero. The stage must detect or safely roll back every violation.

### 10.2 Private allocation

After all count/index/resource checks:

1. default-construct one transaction-private mesh;
2. reserve/resize `vertices` to exact public vertex count;
3. reserve/resize `faces` to exact facet count;
4. allocate each face with exactly three `I` entries through the adapter;
5. leave optional public arrays and metadata empty;
6. catch `std::bad_alloc` at the stage boundary and translate it to deterministic `resource_limit` after joining/rollback;
7. translate other unexpected exceptions at the transaction boundary to stable `internal_invariant_error`; and
8. expose no reference before candidate commit.

Host allocation failure remains possible despite logical resource preflight. It cannot publish partial arrays or a partially populated candidate.

### 10.3 Coordinate copying

For each public vertex position in canonical order:

- read the authoritative Component 13 nominal x/y/z exact bits;
- verify finiteness and consistency with the bounded-point nominal;
- apply the frozen signed-zero output rule exactly once using Component 03 `FloatingBits`;
- reconstruct `T` from the resulting bits with `memcpy`-based services;
- assign one `vec3<T>` payload without arithmetic;
- read back exact bits from the private mesh slot;
- require equality with expected output bits; and
- record copy disposition, source bits, output bits, and precision/lineage references.

No decimal conversion, cast through a different scalar, average, normalization, snapping, coordinate sort, or coordinate-based deduplication is permitted.

### 10.4 Index conversion and facet writes

For each canonical facet:

- convert each global public vertex position to `I` through the checked adapter;
- require each value is non-sentinel and `< public_vertex_count`;
- require all three values are distinct;
- write exactly three values in the selected forward cyclic order;
- read the face back and require length and values match; and
- record the public facet/corner mapping.

Do not call `convert_to_triangles`; the cleaned artifact is already triangular. Do not merge coplanar neighbors or remove faces that appear redundant.

### 10.5 Public mesh lexical audit

After all writes, read the mesh only through `PublicMeshReadView<T,I>` and verify:

- exact vertex/facet counts;
- optional arrays and metadata are empty under V1;
- every coordinate is finite and bit-identical to the output bit record;
- every face has length three;
- every index is non-sentinel, representable, in range, and matches the canonical facet record;
- every face has three distinct values;
- empty output has zero vertices and faces; and
- no adapter-side reorder, deduplication, normalization, narrowing, or insertion occurred.

## 11. Independent public topology reconstruction

### 11.1 Directed edge-use emission

From the readback face array, independently emit three records per facet:

```text
(facet position, local corner 0, i0 -> i1)
(facet position, local corner 1, i1 -> i2)
(facet position, local corner 2, i2 -> i0)
```

Validate checked `3 * facet_count`, unique `(facet,corner)`, and complete coverage. Sort records by canonical undirected endpoint pair `(min(i,j), max(i,j))`, direction, facet, and corner.

### 11.2 Undirected edge grouping and reciprocal pairing

For every exact endpoint group:

- require cardinality exactly two;
- require one use `u->v` and one use `v->u`;
- reject duplicate same-direction uses, self-edges, one-use boundaries, and three-or-more-use edges;
- assign a dense public undirected-edge ID by sorted endpoint pair;
- store two public directed-use IDs; and
- compare the endpoint pair and mapped cleaned entities with the claimed cleaned-edge mapping.

Coordinates do not participate. Point/edge touching separate components with equal coordinate bits remain separate because their public indices differ.

### 11.3 Vertex link reconstruction

Build an independent oriented corner-link representation from public facets and paired directed uses. For each corner `(u,v,w)` at vertex `v`, create one link arc from neighbor occurrence `u` to `w`, identified by the two incident public edge uses and facet corner.

For each public vertex:

1. collect all incident corners by exact index;
2. require each link-neighbor incidence has exactly one predecessor and successor under edge pairing;
3. independently construct the undirected link graph whose nodes are incident corners and whose edges cross paired face edges at the vertex;
4. require every link node degree is two;
5. require the graph is connected;
6. require one cycle visits every incident corner exactly once; and
7. reject isolated vertices, open chains, early repeats, multiple cycles, repeated incidence, or cross-index links.

The producer implementation must not merely compare against Component 13 stored link cycles. It reconstructs from public faces and compares the resulting cycle up to orientation-preserving cyclic rotation after mapping public positions to cleaned occurrences.

### 11.4 Public connected components

Traverse facets across reconstructed reciprocal edges. Collect exact public vertex/edge/facet member sets. Require:

- every facet belongs to one component;
- every referenced vertex belongs to one component;
- no isolated public vertex exists;
- component count and member ranges match canonical component records;
- mapped cleaned members equal the independently reconstructed Component 13 components; and
- coordinate contact never joins components.

Recompute `V/E/F`, Euler characteristic, and genus as in Section 5.3. Compare reports and component signatures.

### 11.5 Structural bijection

Verify independently:

- public vertex count equals cleaned vertex count;
- public facet count equals cleaned triangle count;
- forward and reverse vertex maps are inverses;
- forward and reverse facet maps are inverses;
- every cleaned edge maps to exactly two reciprocal public directed uses;
- every public directed use maps to one cleaned halfedge/edge use;
- each public triangle's mapped cleaned vertices match the cleaned outward triangle up to the recorded forward cyclic rotation;
- public edge pairings and vertex links map exactly to cleaned topology; and
- component member sets match.

Counts/digests are evidence but never replace record-by-record checks.

### 11.6 Duplicate-coordinate separation audit

Group public vertices by exact output coordinate bit triple solely for diagnostics. For every group with multiplicity greater than one:

- require each public index remains distinct;
- compare its cleaned occurrence and separation/multiplicity class;
- verify no face index substitution welded two occurrences;
- verify each occurrence's link remains a separate one-cycle fan;
- verify component membership remains distinct where required; and
- publish a deterministic duplicate-coordinate preservation summary.

This grouping never creates adjacency or identity.

## 12. Internal/public maps and deterministic evidence

### 12.1 Vertex maps

Store dense maps:

- `cleaned_vertex_occurrence_id -> public_vertex_position_id`;
- `public_vertex_position_id -> cleaned_vertex_occurrence_id`; and
- `public_vertex_position_id -> checked I value`.

Build maps from the winning canonical labeling. Validate owner/domain/range, one assignment each, and inverse equality. Encode map entries in public position order plus a separately framed cleaned-key-sorted view for verifier diagnostics.

### 12.2 Facet maps

Store:

- `cleaned_triangle_id -> public_facet_position_id` plus cyclic rotation;
- `public_facet_position_id -> cleaned_triangle_id`; and
- public corner to cleaned halfedge/corner mapping.

Verify orientation and exact index triple at construction and readback.

### 12.3 Edge/use maps

Store:

- cleaned paired edge to two public directed-use IDs;
- cleaned halfedge to public directed-use ID;
- public directed-use to cleaned halfedge and cleaned paired edge;
- public undirected edge to cleaned paired edge; and
- cleaned paired edge to public undirected edge.

The maps must be bijective for V1. A mismatch is an invariant error even if edge counts agree.

### 12.4 Component maps

Store:

- cleaned reconstructed component complete key to canonical public component ID;
- public component ID to sorted cleaned member sets;
- contiguous public vertex/facet ranges; and
- public component canonical content/artifact signatures.

Do not assume Component 13 numeric component IDs are dense or canonical for public ordering.

### 12.5 Map digests and structural fallback

Encode every map domain separately with explicit record counts and domain tags. Aggregate a map digest for quick verification, but Component 14 and 15 must compare actual records for correctness. Collision-injection tests must prove a forged/equal map digest does not hide a changed mapping.

## 13. Precision and tolerance-use aggregation

### 13.1 Output lineage set

Construct the exact output lineage set from every public vertex's current Component 03 ledger entry and every Component 13 cleanup record contributing to a surviving coordinate/topology report. Validate:

- each public vertex has one current finite ledger entry;
- all required parent entries are reachable;
- no surviving lineage contributor is omitted;
- removed feature/component records required by policy remain referenced even though they have no public vertex; and
- signed-zero/output-copy representation-effect entries are included where applicable.

### 13.2 Candidate output precision

Invoke the Component 03 versioned aggregation service over the exact output lineage set. For V1:

- sequential inherited/construction/cleanup contributions on one lineage follow Component 03's outward-rounded rule;
- global candidate output precision is the conservative maximum over surviving output lineages plus any policy-required global removal/representation contribution;
- no Component 14 calculation may shrink a predecessor value; and
- the aggregation record identifies every parent and formula version.

Independently scan ledger views to verify the returned maximum and choose equal maxima by `maximum_witness_key`.

### 13.3 Required reported quantities

Record separately:

- `output_precision`;
- maximum authorized tolerance;
- maximum realized one-action displacement;
- maximum cumulative original-lineage displacement;
- maximum removed local feature size;
- maximum removed component size;
- maximum output-copy/representation contribution;
- remaining tolerance margin under the frozen policy; and
- deterministic witnesses attaining each maximum.

Do not merge uncertainty, movement, and feature removal into one epsilon. A quantity with no applicable records uses the versioned exact zero/empty-witness rule.

### 13.4 Known failure gates

Before candidate publication, fail with the most specific typed error when:

- an applicable output precision exceeds tolerance;
- cumulative displacement exceeds its policy budget;
- a feature/component removal exceeds authorization;
- a ledger contributor is missing or inconsistent;
- an aggregate is non-finite or unrepresentable; or
- report values understate reconstructed records.

Do not defer a known budget violation to Component 15. Component 15 independently re-aggregates and may still reject a candidate that passed Component 14.

### 13.5 Deterministic witness selection

For equal conservative values, compare full `maximum_witness_key` bytes. Do not choose the first encountered record or use raw nominal floating order. Store all equal-max witness count and the canonical primary witness; optional secondary witnesses are retained only within diagnostics limits without changing the primary.

## 14. Report assembly

### 14.1 Topology report

Derive public topology facts from Section 11 reconstruction, not copied Component 13 summaries. Include:

- public vertex, directed-use, undirected-edge, facet, component, and shell/component counts;
- two uses per undirected edge;
- reciprocal directions;
- one closed link per public vertex;
- connected component ranges and signatures;
- Euler/genus summaries;
- consistent inherited outward orientation evidence;
- duplicate-coordinate group and preserved-separation counts;
- cleanup-induced component/genus/cavity changes from Component 13 with reconstructed consistency checks;
- empty-result status;
- internal/public bijection result; and
- structural round-trip result.

Set status exactly `assembled_pending_independent_verification`. No Component 14 path may set final success.

### 14.2 Geometry-pending report

Include:

- finite coordinate and exact-bit audit;
- inherited accepted orientation/area/altitude evidence per public facet;
- output precision and all tolerance-use quantities;
- maximum witnesses;
- inherited no-new-intersection and local side-plausibility evidence references from cleanup;
- signed-zero/output-copy audit;
- known Component 15 obligations: independent triangle nondegeneracy, forbidden intersections, side probes, shell semantics, re-ingestion, and final tolerance acceptance; and
- status `finite_and_bounded_pending_independent_verification`.

Do not describe geometry as `tolerance_checked`, embedded, intersection-free, or Boolean-correct solely from Component 14 checks.

### 14.3 Cleanup and topology-change report

Preserve the complete Component 13 action log or a versioned compressed representation with resolvable per-action records. Summarize:

- action counts by class;
- vertices moved, merged, split, duplicated, or removed;
- edges/triangles removed, replaced, or retriangulated;
- components removed;
- component/genus/cavity changes;
- displacement/feature/component-removal maxima;
- policy authorizations used;
- output entities affected by each action; and
- replay/certificate references.

Compression may intern repeated keys/ranges but must not discard evidence needed by Component 15.

### 14.4 Resource report

Record actual and peak values for every reserved resource class, including:

- component reconstruction;
- graph nodes/relations/colors/signatures;
- refinement rounds/cells;
- automorphism branches/prunes/memo states/leaves;
- canonical byte materialization/comparison;
- permutations/maps;
- public mesh arrays/nested face allocations;
- topology reconstruction;
- report/provenance records;
- logical bytes/digest work;
- round-trip/verifier work;
- persistent/temporary bytes; and
- abstract work units.

Reconcile leases before commit. Advisory/hard-limit crossings and deterministic failures use Component 01 records.

### 14.5 Determinism report

Include:

- graph/refinement/automorphism provider versions;
- initial/stable partition color and cell counts;
- refinement rounds per component;
- unresolved cells selected with full canonical keys;
- branches expanded, safely pruned, memoized, and completed;
- automorphism-equivalent leaf count where available;
- canonical component public/artifact signatures;
- global component/vertex/facet permutation digests;
- logical serialization and digest-layout versions/byte counts;
- full-byte collision fallback counts;
- execution policy and worker count as non-semantic metadata;
- proof that final merge used canonical key order; and
- deterministic resource-limit/failure key when applicable.

Do not record pointers, task addresses, unordered bucket positions, or schedule-dependent timestamps as authoritative data.

## 15. Provenance normalization and coverage

### 15.1 Normalized dictionary model

Build immutable interned dictionaries for repeated provenance records. Dictionary keys are full canonical normalized bytes. Sort by full key, assign dense dictionary IDs, and verify full-byte equality on digest collisions.

Suggested dictionaries:

- source operand/shell/facet/edge/vertex references;
- Component 04 source triangle/internal diagonal lineage;
- Component 05 canonical topology lineage;
- Component 08 event/carrier/relation references;
- Component 10 retained use, orientation, ownership, and occurrence classes;
- Component 11 face-region/cycle/boundary occurrence references;
- Component 12 triangle/patch/diagonal/obligation lineage;
- Component 13 action/correspondence/budget/topology-change lineage; and
- Component 03 precision-ledger/budget references.

Dictionary IDs are artifact-local and assigned from sorted full bytes. They are not public mesh indices.

### 15.2 Per-vertex provenance

Every public vertex must resolve to:

- exactly one cleaned vertex occurrence;
- one or more Component 11 output occurrence requirements;
- source vertex and/or canonical intersection event lineage;
- contributing operands, shells, facets, edges, retained uses, and carriers;
- cleanup actions that moved, merged, split, duplicated, or preserved it;
- occurrence separation/multiplicity/coincident-sheet class;
- current bounded point and precision entry;
- cumulative displacement records for all original lineages; and
- normalized dictionary ranges and digest.

A public vertex with missing lineage is a failure. Equal coordinates never substitute for missing provenance.

### 15.3 Per-facet provenance

Every public facet must resolve to:

- exactly one cleaned triangle;
- one Component 12 triangle/patch lineage;
- one Component 11 face region and contour/cycle lineage;
- Component 10 retained uses, orientation, multiplicity, and coincident ownership;
- source operand/shell/facet/triangle and caller feature identities;
- event/carrier boundary lineage where applicable;
- cleanup retriangulation/replacement actions; and
- normalized dictionary ranges and digest.

### 15.4 Coverage and reverse lookup

Verify every public entity has provenance and every surviving cleaned vertex/triangle appears in exactly one public entity. Removed Component 12/13 entities appear only in cleanup/action dictionaries and are not assigned public positions. Publish reverse ranges from selected predecessor entities to public vertices/facets where Component 15 requires them.

### 15.5 Public-content separation

Detailed provenance must not alter public mesh content ordering except through the artifact tie phase after equal public-content bytes. Encode it in the provenance domain and commit its digest into artifact/candidate domains. This keeps public mesh bytes presentation-independent while preserving deterministic maps and full traceability.

## 16. Empty-result path

An empty cleaned manifold is a first-class path, not a special failure.

Produce:

- zero components, vertices, edges, facets, graph nodes, permutations, and public maps;
- default-valid empty `fv_surface_mesh<T,I>` with all optional members empty;
- canonical empty public-content bytes;
- output precision determined by Component 03's frozen empty-result rule, never assumed zero without that rule;
- cleanup/topology-change reports distinguishing originally empty, Boolean-selected empty, and cleanup-authorized removed-to-empty;
- empty but valid provenance/entity tables plus retained removal/action evidence;
- pending topology/geometry statuses;
- canonical domain-separated empty digests;
- successful structural round-trip through the adapter; and
- complete Component 15 obligations/dependencies.

Do not create a dummy vertex, sentinel facet, zero-area triangle, or empty component record. The canonical-label search is skipped only because there are no components; all version, report, codec, round-trip, and verifier gates still run.

## 17. Logical serialization contract

### 17.1 General encoding rules

Use Component 01 `CanonicalBytes` exclusively. Every logical domain must have:

- a fixed nonzero schema version;
- a fixed domain-separation byte string;
- a fixed field order;
- explicit fixed-width integers or one prescribed canonical unsigned-varint format;
- exact `T` bits encoded at the qualified width;
- checked length-prefixed sequences;
- explicit required/optional framed sections;
- zeroed reserved fields;
- a decoder that rejects truncation, duplicate singleton fields, unknown required tags, invalid enums, impossible lengths/counts, trailing bytes, and noncanonical varints; and
- exact preflight or a reproducible streaming encoder whose byte count is checked.

Never serialize native object memory, nested vector capacity, `size_t`, pointers, owner addresses, allocator state, host endianness, locale text, compiler names as semantics, implicit enums, or unordered iteration.

### 17.2 Public mesh content domain

Freeze the V1 public mesh content layout in this order:

1. domain magic `YGOR_BOUNDED_OUTPUT_MESH_V1` as fixed bytes;
2. public-content schema and canonical-ordering versions;
3. scalar descriptor and scalar bit width;
4. public index descriptor, while actual serialized indices are widened to canonical `u64`;
5. coordinate signed-zero policy;
6. output policy `triangulated_oriented_manifold_v1`;
7. component count;
8. public vertex count;
9. for each public vertex in canonical order: x bits, y bits, z bits;
10. public facet count;
11. for each public facet in canonical order: ring length exactly `3`, then three canonical `u64` indices; and
12. reserved terminator/version fields required by Component 01 framing.

Do not include normals, colors, `involved_faces`, arbitrary metadata, public vector capacities, component offsets, maps, provenance, reports, runtime owner tokens, or digests inside the mesh content domain. The ordering already commits to components implicitly through the vertex/facet sequences.

The empty content domain encodes all headers with zero counts and no entity records; it is not an empty byte string.

### 17.3 Artifact domain

Encode in order:

1. artifact magic/schema/provider versions;
2. stable context/input digest and operation/output policy;
3. scalar/index descriptors;
4. predecessor artifact digest table with component/version/domain tags;
5. public mesh content byte length and digest, plus retained full bytes or stable encoder reference according to policy;
6. canonical component records/ranges/signatures;
7. coordinate-copy records;
8. internal/public maps and map section digests;
9. public topology reconstruction records and summaries;
10. precision/tolerance-use records and deterministic witnesses;
11. topology/geometry-pending/cleanup/topology-change/resource/determinism/round-trip report records;
12. provenance digest and evidence-domain digest references;
13. replay-reference digest;
14. candidate pending status values; and
15. required reserved/version fields.

Runtime immutable handles are not serialized. The stable dependency table is sufficient to bind the candidate to predecessor content.

### 17.4 Provenance domain

Encode:

1. provenance magic/schema;
2. normalized dictionary schema versions;
3. each dictionary in canonical full-key order with explicit counts/ranges;
4. per-public-vertex provenance ranges in public position order;
5. per-public-facet provenance ranges in public facet order;
6. reverse lookup ranges required by Component 15;
7. cleanup/removal-only records not attached to surviving entities;
8. coverage counts and canonical coverage evidence; and
9. reserved fields.

No dictionary ID is meaningful outside this encoding without the schema/version.

### 17.5 Assembly evidence domain

Encode:

- graph schema and complete graph count summaries;
- initial/stable partition color/cell bytes or reproducible committed encodings;
- canonical labeling permutations and inverses;
- branch/refinement/automorphism statistics;
- winning component public/artifact bytes/digests;
- public topology reconstruction records;
- structural bijection evidence;
- duplicate-coordinate separation evidence;
- round-trip evidence; and
- producer verifier findings/report digest.

Diagnostic-policy limits may omit non-mandatory losing-branch details, but never winning-label, map, topology, or round-trip evidence required by Component 15.

### 17.6 Replay-reference domain

Do not duplicate the entire Component 01 replay format unnecessarily. Encode a stable reference containing:

- replay schema/provider version;
- stable context/input replay digest;
- exact candidate domain digests and byte counts;
- predecessor digest table;
- canonical primary diagnostic/failure reference if present;
- execution/resource policy identifiers; and
- whether full candidate/predecessor bytes are embedded, content-addressed, or reproducible from the source replay.

When diagnostic policy promises full-on-failure or full-always retention, reserve and retain the required bytes before stage work. Failure to satisfy promised replay retention is a preflight/resource failure, not silent degradation.

### 17.7 Canonical decoder and read limits

The decoder is required for golden tests, replay, mutation tests, and Component 15 re-encoding comparison. It must:

- validate every count before allocation;
- use Component 01 checked arithmetic for count-to-byte calculations;
- enforce caller/context hard limits;
- reject duplicate or out-of-order records where canonical order is required;
- reject noncanonical representations that decode to the same logical value;
- preserve exact scalar bits;
- return typed byte offset/field/domain diagnostics; and
- never automatically publish or execute a Boolean operation.

## 18. Digest domains and structural equality

### 18.1 Required digests

Compute with Component 01 clean-room SHA-256 and fixed domain tags:

- `public_mesh_content_digest` over Section 17.2 bytes;
- `output_provenance_digest` over Section 17.4 bytes;
- `output_assembly_evidence_digest` over Section 17.5 bytes;
- `output_artifact_digest` over Section 17.3 bytes;
- `output_replay_reference_digest` over Section 17.6 bytes; and
- `assembled_candidate_digest` over a fixed aggregate frame containing domain versions, byte lengths, and the preceding digests in fixed order.

If cleanup/action certificates or reports have independent predecessor digests, preserve them as referenced subdomains; do not recompute their semantics under a Component 14-specific hash layout.

### 18.2 Domain separation

Each SHA stream begins with a fixed domain-separation frame containing:

- engine family/version;
- Component 14 domain enum;
- domain schema/provider version;
- scalar/index descriptors where applicable; and
- a fixed separator length/value.

The same logical bytes in two domains must not yield the same hash input stream. Tests must verify the domain prefixes differ and swapping domain digests changes the aggregate digest.

### 18.3 Digest use limitations

Digests may:

- accelerate full-byte equality/order checks;
- identify replay/artifact records;
- detect corruption; and
- summarize canonical sections.

Digests may not:

- establish graph-node identity;
- resolve automorphisms;
- prove component equality without full bytes;
- replace internal/public map comparison;
- replace topology reconstruction;
- replace report coverage; or
- make a forged candidate acceptable.

Every equality path used for semantics must have full structural/byte fallback. A test-only digest provider that truncates or forces collisions must leave selected public bytes, maps, failures, and verifier outcomes unchanged.

### 18.4 Streaming and retained bytes

For potentially large domains, support a reproducible encoder that writes identical chunks to:

- SHA-256;
- an optional retained-byte sink required by diagnostics/replay; and
- a checked byte counter.

Canonical comparisons used for component ordering/automorphism minima require random or repeatable full-byte access. Retain bounded component candidate bytes or use a deterministic replayable block store with reserved capacity. Do not compare only streaming digests.

## 19. Structural round-trip and Component 02 compatibility

### 19.1 Round-trip procedure

After candidate construction:

1. obtain a const `PublicMeshReadView<T,I>` through the shared adapter;
2. copy/read the public mesh into a transaction-private structural intake record using the same lexical rules available to Component 02;
3. compare exact coordinate bits and exact face index sequences with Section 17.2 content;
4. independently reconstruct directed uses, undirected pairs, vertex links, and components from the intake record;
5. verify duplicate-coordinate public entries remain distinct;
6. compare reconstructed topology to the Component 14 public reconstruction and mapped Component 13 topology;
7. attach the candidate precision metadata through the versioned repeated-input import record without resetting uncertainty;
8. run the structural portion of Component 02 input validation, excluding final shell/geometry checks owned by Component 15; and
9. store complete round-trip evidence and digest.

The round-trip must use public fields only. It must not access candidate maps to reconstruct topology, except afterward for comparison.

### 19.2 Structural acceptance floor

Require at least:

- supported public scalar/index layout;
- finite coordinates;
- three-index facets with valid distinct indices;
- exactly two opposite uses per undirected edge;
- one closed link per public vertex;
- consistent local orientation;
- no isolated vertices;
- exact component reconstruction; and
- successful precision metadata import without loss.

Component 14 does not establish shell occupied-side semantics or final epsilon-validity through this structural check. Component 15 performs the full public re-ingestion and Boolean side-consistency gates.

### 19.3 Adapter mutation detection

Test and reject adapters that:

- reorder vertices or facets;
- rotate/reverse facets unexpectedly;
- deduplicate equal coordinates;
- merge or split public vertices;
- narrow `T` or `I`;
- normalize signed zero contrary to policy;
- insert/remove a face;
- populate or reinterpret metadata as topology;
- reserve/emit sentinel indices; or
- return a stale/mutable view.

Failure publishes no candidate.

## 20. Component 15 handoff and status boundary

### 20.1 Immutable handoff surface

`OutputAssemblyQueries.h` must give Component 15 immutable owner-checked access to:

- the exact public `fv_surface_mesh<T,I>` candidate;
- public coordinate bit records;
- canonical component/vertex/facet order and ranges;
- all internal/public forward and reverse maps;
- cleaned edge/public directed-use maps;
- canonical graph/labeling evidence and provider versions;
- independently reconstructed public topology evidence;
- precision and tolerance-use input records plus candidate summaries;
- topology, geometry-pending, cleanup, topology-change, provenance, resource, determinism, and round-trip reports;
- logical encoders/retained bytes and every digest domain;
- complete predecessor dependency handles/digests required by the verification level;
- diagnostics and replay metadata; and
- Component 14 producer verifier report.

Queries validate context owner, candidate version, section range, and immutable lifetime. They return checked views, not raw mutable vectors or implementation pointers.

### 20.2 Pending-only statuses

The candidate must state:

- topology: `assembled_pending_independent_verification`;
- geometry: `finite_and_bounded_pending_independent_verification`;
- ordinary publication eligibility: `false`; and
- final success digest: absent/unset.

The candidate may record that Component 14's producer checks passed. It must not state or imply that Component 15's independent forbidden-intersection, Boolean side, shell, re-ingestion, or final tolerance gates passed.

Any attempt to construct a Component 14 candidate with `tolerance_checked`, a final success status, or an ordinary-success wrapper is a producer/verifier invariant failure.

### 20.3 Rejected candidate lifetime

If Component 15 rejects the candidate, the public mesh remains an internal immutable diagnostic artifact only according to the frozen diagnostic/replay retention policy. It is not moved or copied into ordinary success. Component 14's API must not provide a public escape hatch that bypasses Component 15.

### 20.4 Final publication ownership

Component 15 alone:

- performs independent final verification;
- promotes topology/geometry reports to final accepted statuses;
- finalizes the ordinary success digest;
- constructs/publishes `bounded_boolean_success<T,I>`; and
- decides diagnostic retention for a rejected candidate.

Component 14 must supply enough evidence but must not prescribe or implement Component 15 algorithms beyond the documented handoff contract.

## 21. Errors, diagnostics, replay, and deterministic arbitration

### 21.1 Required failure subcodes

Allocate a disjoint Component 14 subcode range with explicit stable values for at least:

- unsupported output policy/provider/schema/version;
- unsupported public mesh/scalar/index layout;
- wrong/stale/cross-context predecessor or capability;
- predecessor dependency/digest/verifier mismatch;
- malformed cleaned vertex/edge/halfedge/triangle/component/link record;
- missing or contradictory cleanup/provenance/precision evidence;
- cleaned structural identity failure;
- count/range/byte/work overflow;
- public index capacity exceeded;
- public sentinel collision;
- resource reservation/allocation failure;
- graph node/relation count or inverse mismatch;
- invalid initial semantic color;
- refinement signature/partition inconsistency;
- canonical labeling work exhausted;
- unsafe pruning contradiction;
- canonical minimum/automorphism inconsistency;
- duplicate complete canonical key;
- component ordering contradiction;
- vertex/facet permutation non-bijective;
- triangle reversal or cyclic-rotation mismatch;
- coordinate non-finite/bit-copy/signed-zero mismatch;
- public face allocation/length/index/write/readback mismatch;
- public directed edge use count/direction mismatch;
- public vertex-link open/multiple/repeated mismatch;
- public component/Euler/genus mismatch;
- duplicate-coordinate occurrence welded or lost;
- internal/public vertex/facet/edge/component map mismatch;
- precision/tolerance/report aggregation mismatch;
- known precision/budget violation;
- provenance missing/non-total/noncanonical;
- codec length/tag/order/reserved-field error;
- logical byte or digest mismatch;
- digest collision fallback contradiction;
- round-trip adapter/structure/precision-import mismatch;
- pending-status violation;
- producer verifier rejection;
- resource reconciliation failure;
- cancellation; and
- unexpected construction invariant failure.

Map index representability to `index_overflow`; configured/host allocation and canonical-search exhaustion to `resource_limit`; known tolerance/precision failures to the specific public numerical category; cancellation to `cancelled`; committed predecessor or producer/verifier contradictions to `internal_invariant_error`; and final-geometry-only uncertainty is not evaluated here except for known inherited budget gates.

### 21.2 Error payload

Every error contains, within diagnostic policy limits:

- component/stage/checkpoint/subcode/category;
- stable context/input digest and relevant predecessor digests;
- scalar/index/provider/schema descriptors;
- least canonical offending component/node/cell/cleaned/public entity keys;
- expected/observed counts, indices, bits, map endpoints, relation roles, or status values;
- canonical-label search depth/cell/branch/resource counters where relevant;
- precision/tolerance contributors and witnesses where relevant;
- byte domain, field tag, and offset for codec failures;
- configured/used resource limits;
- deterministic primary-failure arbitration key; and
- replay identity/payload reference.

Do not use `exception::what()`, pointer values, task IDs, wall-clock time, or hash bucket order as authoritative fields. Optional debug text may retain exception text only after the stable error is fixed.

### 21.3 Deterministic primary failure

Collect bounded candidate failures at deterministic phase boundaries. Reduce using Component 01's complete failure key, including canonical component/entity keys rather than discovery order. Parallel workers return private failures; merge all admitted work in canonical key order. Resource/cancellation policies define whether later checks are admitted, but the same frozen policy and input must choose the same primary failure for every worker schedule.

### 21.4 Replay sufficiency

Replay must reproduce:

- exact source input/options/context;
- Component 13 and required predecessor artifact digests/content references;
- graph/provider versions and resource ceilings;
- selected canonical component bytes and permutations;
- public mesh content bytes;
- reports/provenance/digests;
- primary failure or accepted candidate status; and
- deterministic counters needed to locate a canonical-search/resource boundary.

A focused Component 14 replay may begin from embedded verified predecessor artifacts only when their canonical bytes and dependency graph are present. Otherwise replay begins from source inputs and reconstructs the pipeline. Never accept a digest-only predecessor if its content cannot be reproduced or located under the frozen replay policy.

## 22. Resource, cancellation, transaction, and concurrency rules

### 22.1 Transactionality

The stage owns one Component 01 transaction. All graph work, public mesh arrays, maps, reports, provenance, bytes, round-trip records, and verifier state are transaction-private until commit.

Commit protocol:

1. stop admitting work;
2. poll cancellation;
3. join all execution scopes;
4. finalize canonical minima and permutations;
5. finish private public mesh and reports;
6. construct proposed immutable candidate;
7. run independent Component 14 verifier;
8. finalize logical bytes/digests/replay references;
9. reconcile persistent resources and promote leases;
10. poll cancellation immediately before publication; and
11. atomically publish one immutable artifact handle.

Any failure/cancellation joins work, discards the mesh/candidate, releases reservations, preserves predecessors, and publishes one typed error. Rollback is idempotent/noexcept after owned exception capture.

### 22.2 Cancellation safe points

Poll at stable deterministic intervals during:

- predecessor scans;
- component traversals;
- graph node/relation construction;
- signature batches and refinement rounds;
- branch-state expansion and leaf encoding;
- full-byte comparison batches;
- component/permutation/map batches;
- public coordinate/facet writes;
- public topology sort/group/link/component passes;
- precision/report/provenance batches;
- logical serialization/digest chunks;
- round-trip intake; and
- verifier passes.

Do not abandon a scalar bit copy, partially mutate shared state, or detach work. Local pure work may finish and be discarded at the next checkpoint.

### 22.3 Deterministic parallel boundaries

Permitted parallel work:

- one private graph/component workspace per reconstructed component;
- private signature construction for a fixed partition round;
- private search subtrees only when the scheduler admits deterministic canonical subranges and all results are merged before selection;
- private provenance/report blocks;
- private serialization chunks with fixed logical offsets; and
- independent verification batches.

Not permitted:

- assigning public indices from task completion;
- mutating one shared partition or branch best without a deterministic reducer;
- schedule-dependent memo cache semantics;
- assigning dictionary/map IDs in workers;
- writing public mesh arrays before final canonical offsets are frozen;
- committing precision/report maxima from worker order; or
- publishing before all workers join.

The serial provider remains required and normative even after Component 17 integration.

### 22.4 Structural performance expectations

Ordinary asymmetric components should stabilize after few refinement rounds and require no or small automorphism search. Target work is approximately:

- `O(V + E + F)` component/topology construction;
- `O((N + R) log N)` per refinement round for `N` graph nodes and `R` relations using sorted signatures;
- output-sensitive automorphism search bounded by configured work;
- `O(F log F + E log E)` public facet/edge grouping;
- linear report/provenance/serialization work plus deterministic sorting of dictionaries; and
- linear retained candidate size.

Highly symmetric components may be exponential under individualization/refinement. They must either complete under limits or fail `resource_limit`; no noncanonical heuristic fallback is allowed.

## 23. Independent Component 14 producer verifier

### 23.1 Independence requirements

`OutputAssemblyVerifier` consumes the proposed immutable candidate and predecessor query views. It must not call the producer's:

- graph builder as its sole graph evidence;
- partition refinement/search to decide canonicality;
- component traversal;
- public edge grouping;
- vertex-link traversal;
- map builder;
- report maximum aggregator;
- provenance coverage flags;
- serialization buffer; or
- round-trip success boolean.

It may share Component 01 checked arithmetic/canonical byte/SHA primitives, Component 03 exact bit and ledger primitives, and immutable schema definitions. Higher-level reconstruction must be separately organized.

### 23.2 Verifier checks

The verifier must:

1. validate candidate/predecessor owners, versions, dependency graph, pending statuses, and ranges;
2. lexically read the public mesh through `PublicMeshReadView`;
3. recompute coordinate bit equality and face triples;
4. independently reconstruct directed edge uses, reciprocal pairs, links, components, Euler/genus, and duplicate-coordinate groups;
5. reconstruct the internal/public vertex/facet/edge/component bijection from public facets plus cleaned records and compare maps;
6. validate every canonical component block by re-encoding the supplied winning labeling;
7. for components under the exhaustive threshold, independently enumerate legal labelings and prove the public block is minimal;
8. for larger components, verify all committed refinement/search certificates, safe-prune proofs, and winning comparisons without simply trusting the producer result;
9. recompute component/global public ordering and permutations;
10. independently rescan Component 03 ledgers and cleanup records for report maxima/coverage;
11. independently verify provenance dictionaries, entity coverage, and reverse ranges;
12. regenerate every logical domain through a separate traversal and compare full bytes/lengths/digests;
13. rerun structural round-trip through the adapter;
14. verify resource reconciliation and deterministic counters are internally consistent; and
15. emit an accepted verifier report/digest or reject publication.

For large canonical-label searches, the verifier need not duplicate the entire exponential search if the frozen provider includes independently checkable branch certificates. Those certificates must prove complete branch coverage and every safe prune. If such a certificate cannot be verified within the required limits, candidate publication fails; the verifier may not accept the producer's claimed minimum on trust.

### 23.3 Canonical-search certificate

Record a compact, replayable search certificate containing:

- initial graph digest plus full graph byte reference;
- stable refinement partition per state or deterministic reconstruction inputs;
- selected unresolved cell key;
- complete set of admitted member branches expressed by normalized structural selectors;
- child state digests/full-byte references;
- complete leaves and candidate public/artifact encodings;
- every safe-prune prefix proof;
- memo equivalence proof by full partition bytes; and
- selected minimum comparison path.

The verifier reconstructs state transitions and proves no branch was omitted. Resource limits bound certificate size/work; inability to produce a complete certificate is `resource_limit`, not permission to omit it.

### 23.4 Mutation rejection inventory

The verifier suite must mutate valid candidates by:

- welding two equal-coordinate public vertices;
- splitting one public vertex without consistent maps/faces;
- changing one coordinate bit or zero sign;
- narrowing one coordinate through text;
- changing one index;
- reversing/rotating/scrambling one facet illegally;
- deleting/duplicating one facet;
- breaking one edge pair or link;
- changing component order;
- replacing a canonical vertex labeling with a larger automorphic labeling;
- omitting one branch/prune certificate;
- changing one map while preserving counts;
- under-reporting precision/displacement/removal;
- omitting one cleanup/provenance contributor;
- changing logical framing/order/endian/varint form;
- forging/truncating one digest;
- changing pending status to final success;
- altering round-trip evidence; and
- modifying resource/counter records.

Every required mutation must be rejected deterministically. Mutation builders may repair unrelated counts and recompute producer-shaped digests to ensure the verifier checks semantics rather than superficial corruption.

## 24. Required tests and validation matrix

### 24.1 Basic assembly known answers

Assemble and commit exact expected public arrays/maps/reports/bytes for:

- empty output;
- one tetrahedron;
- one triangulated box;
- disconnected unequal components;
- nested outer/cavity/island shells represented as disconnected boundary components;
- point-touching components with duplicate coordinate entries;
- edge-touching components with duplicate endpoint entries;
- coincident but topology-distinct shell occurrences permitted by predecessor policy;
- cleanup-generated vertex splits/occurrence duplications;
- outputs where every coordinate bit triple occurs more than once; and
- outputs removed to empty by authorized whole-component cleanup.

Verify exact counts, coordinate bits, facets, orientation, component ranges, links, maps, provenance, reports, domain bytes, digests, and round-trip evidence.

### 24.2 Type matrix

Run at least:

- `float`/`std::uint32_t`;
- `float`/`std::uint64_t`;
- `double`/`std::uint32_t`; and
- `double`/`std::uint64_t`.

Include signed zero, subnormals, smallest/largest normals, adjacent values, extreme finite exponents, large translations, mixed magnitudes, and repeated exact bits. Verify exact copy and logical bytes across qualified compilers/platforms.

### 24.3 Public index capacity and checked arithmetic

Using constrained adapter/index policies, test:

- zero vertices/facets;
- one vertex below/at/above maximum usable public index count;
- a reserved sentinel at `max(I)`;
- public vertex count representable while a prefix sum overflows an internal type;
- facet/corner counts near checked `size_t`/`uint64_t` limits;
- `3 * facet_count` overflow;
- nested face allocation byte overflow;
- map/provenance/logical-byte count overflow;
- allocation succeeds then later checked conversion fails; and
- rollback after each boundary.

Use fake counts/reservations; do not allocate enormous memory. No partial public mesh may escape.

### 24.4 Duplicate-coordinate preservation

Cover:

- separate local fans at equal coordinates within one component where predecessor policy permits topology-distinct occurrences;
- point-touching components sharing one coordinate;
- edge-touching components sharing two endpoint coordinates;
- repeated identical disconnected components;
- source and event occurrences with equal nominal bits;
- cleanup-created duplicate occurrences;
- all vertices equal by coordinate bits but topologically distinct; and
- `+0/-0` variants under preserve and authorized canonicalize-zero policies.

Require distinct public indices, exact links/components, total provenance, stable bytes, and mutation rejection for every accidental weld.

### 24.5 Canonical graph construction

For hand-auditable components, verify exact typed node/relation counts and initial color bytes. Permute temporary node/edge/corner insertion, cleaned arrays, local ordinals, and relation storage. The graph's normalized bytes, stable partition, winning public block, and evidence must remain identical.

Inject missing/inverted `next`, wrong opposite corner, duplicate relation, omitted edge node, incorrect initial semantic field, and transient ID in a color. Producer/verifier must reject each.

### 24.6 Refinement known answers

Commit stable partition sequences for:

- asymmetric tetrahedral variants;
- path-like asymmetric triangulated shells;
- cyclic/dihedral rings of triangles;
- tetrahedral and box symmetries;
- repeated-coordinate topologies;
- topology-distinguished equal-coordinate vertices;
- components differing only by one coordinate bit;
- several refinement rounds before discreteness; and
- deliberately colliding signature digests.

Verify full signature bytes, cell splits, stable-state detection, and independence from signature-generation order.

### 24.7 Automorphism and exhaustive oracle

Compare production canonical blocks against exhaustive permutation minima for bounded fixtures including:

- regular tetrahedron;
- symmetrically triangulated cube/box;
- bipyramid and cyclic fan symmetries;
- repeated identical disconnected components;
- components with all equal coordinate colors but asymmetric topology;
- components with nontrivial orientation-preserving and orientation-reversing graph automorphisms;
- automorphism cells surviving many refinements;
- equal public-content leaves with different normalized artifact ties; and
- forced hash collisions.

Disable pruning/memoization and permute branch enumeration. The minimum must not change. Inject one unsafe prune certificate and require verifier rejection.

### 24.8 Component/vertex/facet permutation metamorphics

Permute:

- caller source vertices/facets/rings/shells/components with valid remapping;
- alternative legal source triangulations where the cleaned final artifact is equivalent;
- Component 11/12/13 internal IDs and storage order;
- cleanup allocation/free-list/action discovery histories that produce equivalent canonical cleaned records;
- component traversal seeds;
- corner and link traversal starts;
- triangle cyclic starts;
- provenance dictionary insertion; and
- worker/task partition.

After documented predecessor remapping, require identical public mesh bytes, component blocks, logical domains, digests, reports, and deterministic primary failures. Maps/provenance must be semantically identical under the remapping.

### 24.9 Triangle orientation and rotation

For all six permutations of one triangle:

- three forward cyclic rotations yield one canonical triple;
- three reversed rotations are rejected or yield a different invalid orientation path;
- public edge directions reconstruct correctly;
- cleaned triangle/provenance remains attached; and
- public bytes are stable for legal cyclic input presentations.

Inject one reversed Component 13 triangle, one reversed public facet, and one face sorted without preserving orientation. Intake/verifier must fail.

### 24.10 Public topology reconstruction

Independently test:

- directed-use emission;
- exact undirected grouping;
- reciprocal directions;
- vertex links;
- connected components;
- Euler/genus summaries;
- internal/public bijection; and
- duplicate-coordinate grouping.

Mutate with corrected counts: one boundary edge, one three-use edge, same-direction pair, bow-tie vertex, disconnected link cycles, missing facet, duplicate facet, isolated vertex, cross-component edge, wrong edge map, and wrong component range. Every mutation fails.

### 24.11 Coordinate-bit copy and signed zero

Test exact bits for:

- normals/subnormals;
- positive/negative zero in each axis;
- adjacent values;
- maximum finite values;
- large/small mixed coordinates; and
- repeated bit patterns.

Use adapters that decimal-round-trip, cast through `float`, canonicalize all zeros, flush subnormals, or normalize coordinates. Detect every unauthorized change. Verify authorized zero-sign policy creates the exact prescribed record without displacement or topology change.

### 24.12 Precision and report aggregation

Construct ledgers where each maximum comes from:

- inherited source precision;
- event construction uncertainty;
- cleanup-created coordinate uncertainty;
- one-step displacement;
- cumulative multi-step displacement;
- removed local feature size;
- removed component size;
- output representation/signed-zero effect; and
- empty-output policy.

Test equal maxima and deterministic witness ties, values just below/at/above tolerance, omitted contributors, wrong parent formula, under-reported summaries, and mixed independent/sequential lineages. Independent scans must detect every undercount.

### 24.13 Provenance coverage and compression

Test public entities with:

- source-only lineage;
- event-only constructed coordinate lineage;
- mixed source/event lineage;
- coincident ownership/multiplicity;
- split/duplicate cleanup lineage;
- retriangulation lineage;
- component removal records; and
- repeated shared dictionary entries.

Permute insertion and force dictionary digest collisions. Require canonical dictionary IDs/bytes. Remove or redirect one lineage, create an unreferenced surviving cleaned entity, or attach one public entity twice; verifier rejects.

### 24.14 Logical serialization golden tests

Commit exact V1 bytes for representative empty/non-empty/type/symmetry/duplicate-coordinate candidates. Test:

- little-endian fixed fields;
- canonical varint boundaries if used;
- exact scalar bits;
- signed-zero rule;
- sequence framing and counts;
- required/optional tags;
- unknown required field rejection;
- optional field skipping only where allowed;
- duplicate fields;
- truncation at every byte boundary for small records;
- trailing bytes;
- simulated big/little endian hosts;
- native struct padding differences;
- locale changes;
- different vector capacities/allocators; and
- decoder allocation limits.

Qualified configurations produce identical logical bytes.

### 24.15 Digest collision/domain tests

Use truncated/forced digest providers to prove:

- component ordering compares full bytes;
- graph signatures compare full bytes;
- memoization validates full partition state;
- public/artifact/provenance/evidence/replay domains remain separated;
- changed content with a forged digest is rejected;
- aggregate digest framing prevents domain swap/concatenation ambiguity; and
- digest equality never establishes map/entity identity.

### 24.16 Round-trip tests

For every known-answer candidate:

- read the exact public mesh through the shared adapter;
- compare coordinate bits and face index sequences;
- reconstruct topology;
- verify duplicate occurrences remain separate;
- import published precision metadata without reset;
- pass Component 02 structural intake; and
- compare component/link/Euler/genus facts.

Run malicious adapters from Section 19.3 and require transactional failure.

### 24.17 Pending-status and Component 15 handoff tests

Verify:

- candidate statuses are pending only;
- no final success digest exists;
- ordinary success construction APIs reject a Component 14 candidate;
- every mandatory Component 15 dependency/query is available and owner/version checked;
- a rejected candidate cannot escape through ordinary result paths; and
- mutation to `tolerance_checked` is rejected by Component 14 verifier and Component 15 intake fixtures.

### 24.18 Determinism and concurrency

Run with:

- thread counts 1, 2, and maximum;
- forced task delays;
- reversed component/refinement/signature/provenance block generation;
- branch subtree scheduling permutations;
- different allocator/free-list layouts;
- hash/signature/digest collision injection;
- memoization enabled/disabled where provider permits;
- pruning enabled/disabled in tests;
- different public vector capacities; and
- repeated execution.

Require byte-identical public mesh, logical domains, digests, reports, candidate evidence, primary failure, and replay for fixed versions/platform profile.

### 24.19 Fuzzing and shrinking

Generate valid cleaned manifolds with controlled:

- component count and equality classes;
- topology/genus;
- graph symmetry/automorphism size;
- coordinate duplication and signed-zero patterns;
- vertex valence and triangle count;
- cleanup/provenance complexity;
- index capacity;
- scalar exponent patterns;
- report/ledger maxima ties;
- resource limits; and
- adapter behaviors.

For small components compare against exhaustive canonical/topology oracles. Serialize and deterministically shrink every crash, hang, noncanonical result, accidental deduplication, topology/map/report mismatch, unsafe prune, digest inconsistency, round-trip failure, verifier disagreement, or unexpected typed failure while preserving predecessor dependencies and failure.

### 24.20 Resource and cancellation

For every resource class test limit-minus-one, limit, and limit-plus-one. Cancel during every stable checkpoint and inside:

- predecessor scans;
- component reconstruction;
- graph construction;
- refinement/signature sorting;
- automorphism branching/pruning/leaf encoding;
- permutation/map construction;
- public allocations/writes;
- topology reconstruction;
- report/provenance assembly;
- serialization/digesting;
- round-trip; and
- verifier work.

Require all workers join, reservations return, private mesh/candidate is destroyed, predecessors remain intact, and no partial public artifact is visible.

### 24.21 Structural performance gates

Record and assert:

- cleaned/public entity counts;
- graph nodes/relations;
- initial/stable colors/cells;
- refinement rounds/signature bytes/sort comparisons;
- unresolved cells;
- branches expanded/pruned/memoized/leaves;
- full-byte comparisons/collision fallbacks;
- component/vertex/facet permutation work;
- coordinate/facet writes;
- public topology records;
- report/provenance bytes;
- logical serialization bytes;
- round-trip/verifier records;
- peak temporary/persistent bytes; and
- abstract work units.

Ordinary asymmetric cases must remain near the documented sorting/linear targets. Symmetric adversarial cases may fail at configured limits but may not use transient ordering or skip verification.

### 24.22 Platform and sanitizer matrix

Run supported GCC/Clang strict C++17 debug/optimized builds and the repository's qualified architecture profiles with:

- ASan/UBSan;
- TSan for permitted deterministic parallel paths;
- float/double and u32/u64 public indices;
- simulated constrained indices/adapters;
- changed locale;
- parent global fast-math flags overridden by strict target; and
- every Component 01/03 floating-environment qualification.

Component 14 performs minimal floating arithmetic, but exact scalar-bit behavior and strict target qualification remain mandatory.

## 25. Implementation sequence and gates

Implement in this order. Do not integrate the next step until the stated gate passes.

1. **Schemas, versions, files, strict target, and empty skeleton.** Add enums/IDs/candidate/query skeletons, adapter traits, pending statuses, empty logical domains, and contract tests. Gate: valid empty candidate skeleton encodes/decodes, cannot publish ordinary success, and all invalid versions/statuses fail.
2. **Predecessor intake and checked preflight.** Implement dependency/owner/digest audits, cleaned structural scan, exact count/index/byte/work/resource bounds. Gate: every predecessor/count/index mutation fails before public allocation.
3. **Public mesh adapter.** Implement direct private builder/const readback, exact bit/index traits, malicious test adapters, and transactional allocation behavior. Gate: exact empty/simple writes read back; every normalization/dedup/narrow/sentinel mutation is detected.
4. **Component reconstruction and local workspaces.** Implement exact paired-topology traversal, member sets, component comparison to Component 13, and topology summaries. Gate: component/touch/coincident/permutation fixtures reconstruct exactly.
5. **Shared canonical-labeling kernel extraction.** Implement/refactor typed graph interface, full-byte initial partition/refinement, collision-safe comparisons, and preserve Component 02 goldens. Gate: kernel unit/exhaustive graph tests pass with insertion/collision permutations.
6. **Component 14 graph model.** Implement typed oriented nodes/relations, public/artifact color payloads, graph audits. Gate: known graph bytes and all relation mutations pass.
7. **Automorphism search and certificates.** Implement cell selection, individualization/refinement, leaf encoders, public-first/artifact-second minimum, safe pruning, memoization policy, resource limits, and search certificate. Gate: exhaustive oracle, pruning-disabled, branch-permutation, symmetry, and resource-boundary suites pass.
8. **Canonical component/vertex/facet ordering.** Implement component blocks/order, prefix sums, vertex/facet permutations, forward cyclic rotations, and maps. Gate: known-answer/permutation/orientation/type/index tests pass.
9. **Private public mesh construction.** Implement exact coordinate-bit copy, signed-zero rule, checked `I` writes, readback lexical audit, optional fields empty. Gate: coordinate/adversarial adapter and rollback suites pass.
10. **Independent public topology reconstruction.** Implement directed/undirected edge grouping, link graph, components, Euler/genus, duplicate groups, and structural bijection. Gate: all topology/map mutations are rejected.
11. **Precision/report aggregation.** Integrate Component 03 output-lineage aggregation, deterministic maxima/witnesses, topology/geometry-pending/cleanup/resource/determinism reports, and known budget gates. Gate: exact ledger/tolerance boundary and under-reporting mutations pass.
12. **Provenance normalization.** Implement dictionaries, per-entity/reverse ranges, cleanup-only records, coverage, and collision-safe canonical IDs. Gate: full lineage matrix and mutation suite pass.
13. **Logical codecs and digest domains.** Implement all domain encoders/decoders, streaming/retention policy, SHA framing, full-byte fallback, goldens, truncation/endian/locale/collision tests. Gate: exact bytes stable across qualified configurations.
14. **Round-trip and Component 02 compatibility.** Implement shared readback structural intake, precision import, public topology comparison, and malicious adapter checks. Gate: every known candidate round-trips and all adapter mutations fail.
15. **Candidate handoff and independent producer verifier.** Complete immutable queries, search certificates, separate reconstruction/re-encoding/report/provenance verification, pending-status enforcement, and Component 15 intake fixtures. Gate: zero required mutation survivors and no ordinary-success escape.
16. **Deterministic parallel integration.** Add only permitted private tasks/canonical merge through Component 17 capability while retaining serial reference. Gate: worker/delay/partition permutations are byte-identical and TSan clean.
17. **Qualification.** Complete type/index/platform, known-answer, exhaustive oracle, property/metamorphic, adversarial, fuzz/shrink, mutation, resource, cancellation, concurrency, sanitizer, round-trip, replay, and structural-performance suites. Gate: every Section 26 item passes before Component 15 implementation begins.

Keep implementation commits reviewable and bisectable. Never expose a partially canonicalized, verifier-skipped, or final-status candidate as a supported downstream artifact.

## 26. Definition of done

The Component 14 implementation plan is fulfilled only when all of the following are true:

- the exact next-stage contract consumes one verified `cleaned_triangle_manifold<T>` and publishes one immutable `assembled_output_candidate<T,I>` or one typed failure;
- every predecessor owner/version/digest/range/lineage/precision dependency is defensively validated;
- every cleaned vertex occurrence maps bijectively to one public vertex position and checked `I` value;
- every cleaned triangle maps bijectively to one public three-index facet;
- every cleaned paired edge/halfedge maps bijectively to reconstructed public directed/undirected uses;
- topology-distinct coordinate-equal occurrences remain distinct public entries and separate links/components where required;
- all counts, prefix sums, allocations, byte sizes, `3 * facet_count`, `size_t` conversions, public indices, and sentinels are checked before commit;
- exact coordinate bits are copied under the frozen signed-zero policy with no text conversion, narrowing, snapping, averaging, or unrecorded normalization;
- triangle outward orientation is preserved and only forward cyclic rotation is permitted;
- public directed edges, reciprocal pairs, two-use groups, vertex links, components, Euler/genus summaries, and internal/public structural equivalence are independently reconstructed from public facets;
- `fv_surface_mesh<T,I>` is used only as a private-built public carrier, with optional arrays/metadata empty in V1 and no mutable convenience method invoked;
- canonical component, vertex, and facet ordering is independent of source/caller/internal IDs, input permutations, cleanup allocation/history, hash iteration, worker count, and schedule;
- the typed oriented incidence graph represents every vertex, triangle, corner, paired edge, cyclic orientation, and reciprocal use required by the public manifold;
- full-signature equitable refinement is deterministic and collision-safe;
- every unresolved automorphism is handled by complete individualization/refinement search or causes typed resource failure;
- no source/transient ordering fallback is used for symmetric meshes;
- public-content minimum selection precedes normalized artifact tie resolution, so provenance cannot perturb canonical mesh bytes;
- safe pruning and memoization have complete independently checkable certificates and exhaustive bounded-oracle coverage;
- equal digests always fall back to full bytes for semantic equality/order;
- component/vertex/facet permutations and every forward/reverse map are total, bijective, range-valid, and independently verified;
- output precision is no smaller than every required inherited, construction, cleanup, removal, and representation contributor;
- uncertainty, one-step displacement, cumulative displacement, feature removal, component removal, and user tolerance remain separate and have deterministic maximum witnesses;
- known precision/budget violations fail before candidate publication;
- topology, geometry-pending, cleanup, topology-change, resource, determinism, provenance, and round-trip reports are complete, monotonic, and independently recountable;
- every public vertex and facet has complete normalized lineage through all required predecessor components, and every surviving cleaned entity is covered exactly once;
- public mesh content, artifact, provenance, assembly-evidence, replay-reference, and aggregate candidate logical encodings have fixed versioned platform-independent layouts;
- every logical domain uses Component 01 canonical bytes and domain-separated SHA-256, never native serialization or an external dependency;
- canonical bytes and digests are stable across every qualified platform/configuration and under forced hash collisions;
- the public mesh structurally round-trips through the shared in-tree adapter, preserves exact bits/indices/duplicate occurrences, imports precision without reset, and passes the Component 02 structural intake subset;
- the candidate remains `assembled_pending_independent_verification` / `finite_and_bounded_pending_independent_verification`, has no final success digest, and cannot escape as ordinary success;
- Component 15 receives immutable access to the public mesh, cleaned manifold, maps, predecessors, ledgers, reports, logical encoders/bytes, digests, round-trip evidence, resources, diagnostics, and replay required by its specification;
- the independently implemented Component 14 verifier reconstructs topology/maps/reports/provenance/bytes, validates canonical-search completeness, and rejects every required mutation;
- resource exhaustion and cancellation join all workers, release all reservations, roll back private arrays/candidate state, preserve predecessors, and publish nothing;
- the serial semantic reference and every permitted deterministic parallel execution produce byte-identical public content, reports, evidence, errors, and replay;
- exact-oracle, known-answer, type/index, property, metamorphic, adversarial, fuzz/shrink, mutation, resource, cancellation, concurrency, sanitizer, platform, serialization, digest-collision, round-trip, replay, and structural-performance suites pass; and
- all production and normative-test code is strict portable C++17, self-contained within Ygor, and uses no external dependency.
