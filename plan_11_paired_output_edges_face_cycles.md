# Plan 11: Paired Output Edges and Face-Cycle Construction

## 0. Scope and fixed V1 design

Implement only Component 11 from `component_11_paired_output_edges_face_cycles.md`.

Consume the verified immutable artifacts from Components 01-10 and publish one immutable `polygonal_output_complex<T,I>` for Component 12. Do not repeat Boolean classification or selection, construct or move coordinates, change occurrence partitions, choose alternative mates, collapse or weld topology, triangulate regions, spend cleanup budget, assemble `fv_surface_mesh<T,I>`, or publish a public Boolean result.

Freeze V1 as:

```text
incidence_audit_provider:        canonical_retained_incidence_audit_v2
vertex_provider:                 sorted_nonempty_occurrence_materialization_v2
region_provider:                 exact_positive_area_continuation_components_v2
boundary_provider:               oriented_dart_cancellation_v2
edge_provider:                   explicit_endpoint_atomic_pair_v2
successor_provider:              continuation_skipping_dart_permutation_v2
cycle_provider:                  role_independent_canonical_successor_cycles_v2
contour_provider:                lineage_first_single_outer_region_v2
witness_provider:                certified_contour_side_witness_v1
admissibility_provider:          bounded_planar_embedding_audit_v2
vertex_link_provider:            halfedge_rotation_link_audit_v1
verification_provider:           independent_polygonal_topology_rebuild_v2
```

The version increments are intentional. They distinguish this reviewed contract from any earlier draft that:

- conflated support-only zero-measure evidence with paired boundary topology;
- allowed an empty ordinary output occurrence;
- initialized pair endpoints through unresolved opposite-halfedge fields;
- included a later-derived contour role in cycle identity; or
- accepted an insufficiently certified containment witness.

The executable serial path is the semantic reference. Parallel execution may prepare private complete-keyed proposals, but IDs, failures, diagnostics, bytes, digests, and replay must match the serial reference exactly.

### 0.1 Non-negotiable V1 rules

- One nonempty Component 10 `vertex_occurrence_requirement` produces exactly one output vertex occurrence.
- One Component 10 `planned_edge_occurrence` produces exactly one paired-edge record and exactly two reciprocal halfedges.
- Every halfedge endpoint is initialized directly from validated occurrence IDs before reciprocal assertions.
- Every boundary item that appears in a region boundary or cycle is a planned edge member, including a zero-measure boundary item.
- A support-only zero-measure record creates no output vertex, port, edge, halfedge, boundary dart, cycle member, or region connectivity.
- Topological identity and adjacency use predecessor IDs and lineage only. Coordinates, tolerance, nominal angle, projected distance, and hash values do not create topology.
- Face regions use only reciprocal Component 10 transparent-continuation records and positive-area continuation evidence.
- Zero-measure support cannot bridge unrelated positive-area regions.
- Boundary cycles use the exact continuation-skipping dart permutation; no turning-angle choice exists.
- V1 final positive-area regions contain exactly one outer contour and zero or more direct holes. Positive-area islands are separate regions.
- Cycle identity is fixed before contour-role assignment and excludes the derived role.
- Containment verification uses a certified strict-side witness; no epsilon offset is permitted.
- No coordinate is recomputed, snapped, averaged, projected back, or moved.
- Ambiguity capable of changing positive-area topology fails closed.
- All code is strict portable C++17, self-contained in Ygor, and free of external dependencies.
- Do not call, adapt, or copy `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`.

A valid empty retained surface publishes a fully audited empty artifact and deterministic digest.

## 1. Existing Ygor assessment and reuse decisions

### 1.1 `fv_surface_mesh<T,I>` is not an intermediate representation

`fv_surface_mesh<T,I>` is a mutable public transport container. It does not encode strong owner-bound identity, occurrence partitions, paired halfedges, exact retained incidence, transparent seams, contour roles, precision references, transactional ownership, or independent verification evidence.

Therefore:

- do not instantiate a temporary public mesh in Component 11;
- do not use public vertex indices as output occurrence identity;
- do not derive adjacency from face arrays, `involved_faces`, normals, colours, metadata, or coordinates;
- do not call duplicate-vertex merge, degenerate-face removal, disconnected-vertex cleanup, triangulation, remeshing, slicing, simplification, or hole-filling methods; and
- leave public indexing and serialization to Component 14.

Use `vec2<T>` and `vec3<T>` only as nominal payloads inside Component 03 bounded records or for non-authoritative visualization. Bare vector equality, order, distance, angle, dot, cross, normalization, or length must not affect topology.

### 1.2 `YgorMeshesHoles` is unsuitable for production reuse

`FindBoundaryChains` and its helpers cannot realize Component 11 because they:

- build vertex representatives through epsilon-radius spatial search;
- intentionally weld nearby coordinates for adjacency;
- skip index-equal and representative-equal edges;
- operate after public mesh materialization;
- can select an arbitrary remaining outgoing boundary candidate while only flagging ambiguity; and
- expose no occurrence fan, paired-edge birth, retained lineage, precision, resource, transaction, or verifier evidence.

`FillBoundaryChainsByZippering` mutates a public mesh, skips short or duplicate-index edges, and triangulates. `EnsureConsistentFaceOrientation` builds epsilon-welded adjacency and reverses mutable face rings. None may be called by Component 11.

Their fixture shapes may be reused only after expected topology is restated through explicit Component 10 artifacts.

### 1.3 Existing polygon routines are references only

`YgorMathMonotoneDecomposition` contains useful algorithmic ideas and fixtures, but it:

- accepts raw projected coordinates rather than output occurrence and halfedge identities;
- deletes repeated endpoints and collinear vertices;
- rejects zero-length edges, reused coordinates, and touching loops;
- uses direct coordinate equality/order and ordinary bare predicates;
- uses `long double` area decisions;
- lacks precision envelopes, lineage, resources, transactions, canonical replay, and independent verification.

It must not classify Component 11 contours or clean cycles. Component 12 may implement a new bounded provider under its own contract.

`YgorMathConstrainedDelaunay` and other coordinate-deduplicating triangulators are also irrelevant to this stage.

### 1.4 Existing public verification is only a later smoke check

`YgorMeshesVerification` checks public face indices, edge counts, triangle degeneracy, and orientation. It does not reconstruct:

- one Component 10 occurrence per output vertex;
- exact pair-at-creation lineage;
- transparent seam consumption;
- face-region member sets;
- cycle successor permutations;
- contour witnesses; or
- one closed local vertex link.

It cannot be the Component 11 producer or verifier. Components 14 or 15 may use it as an additional public-mesh smoke test.

### 1.5 Legacy Boolean implementations are prohibited

The existing `YgorMeshesBoolean`, `YgorMeshesBoolean2`, `YgorMeshesBoolean3`, `YgorMeshesBoolean4`, and `YgorMeshesBoolean5` implementations use incompatible grid, BSP, snapping, per-fragment classification, public-mesh, or coordinate-derived topology models. Do not link, include, call, adapt, copy, or treat their output as a normative oracle.

Geometry-only fixture builders and operation taxonomies may be ported into bounded tests when expected artifacts are independently specified.

### 1.6 Mandatory narrow reuse

Reuse only through established contracts:

- Component 01 strong IDs, checked arithmetic, resources, cancellation, transactions, deterministic errors, canonical codec primitives, SHA-256, diagnostics, and replay;
- Component 03 bounded points, finite intervals, projection, orientation, area, segment relation, containment, residual, parameter, length, AABB, exact-tie, and precision-ledger services;
- Component 04 semantic source-facet groups, support frames, and internal-diagonal provenance;
- Component 05 source halfedges, source-edge direction, source-facet incidence, and source-vertex fan lineage;
- Component 07 relation and symbolic provenance;
- Component 08 events, intervals, carriers, order, and construction lineage;
- Component 10 retained uses, incidences, continuations, planned edge pairs, local ports, vertex occurrence cycles, and balance evidence;
- Component 17 deterministic execution services when available; and
- standard-library contiguous storage, stable sorting, binary search, prefix sums, and iterative traversal.

A need to run a new selection predicate, discover a new mate, or construct a new coordinate is a predecessor-contract defect, not permission to do so here.

## 2. Files, build integration, APIs, and versions

### 2.1 Production files

Add under `src/YgorMeshesBooleanBounded/`:

- `OutputTopologyTypes.h` — closed enums, strong IDs, complete keys, versions, checked range/count types, temporary records, and failure details.
- `PolygonalOutputComplex.h` — immutable `polygonal_output_complex<T,I>`, section views, owner-checked accessors, and downstream query views.
- `OutputTopology.h/.cc` — typed entrypoint, checkpoint orchestration, serial semantic reference, canonical merge, and stage transaction.
- `OutputTopologyPreflight.h/.cc` — predecessor validation, exact count/byte/work bounds, capability compatibility, and resource plan.
- `OutputIncidenceAudit.h/.cc` — complete retained-incidence audit and the paired-boundary/support-only partition.
- `OutputVertices.h/.cc` — nonempty output occurrence allocation and authoritative bounded-point attachment.
- `OutputRegions.h/.cc` — reciprocal continuation graph, positive-area split, region membership, and boundary-dart construction.
- `OutputEdges.h/.cc` — planned-edge validation, explicit-endpoint atomic pair materialization, zero-length descriptors, and carrier/source-edge balance.
- `OutputCycles.h/.cc` — continuation-skipping successor/predecessor permutation, cycle extraction, and role-independent canonical cycle identity.
- `OutputContours.h/.cc` — arrangement-lineage contour roles, one-outer validation, region nesting, and deferred contour records.
- `OutputContourWitnesses.h/.cc` — strict contour-side witness import/construction, separation evidence, and deterministic witness keys.
- `OutputAdmissibility.h/.cc` — bounded interval, order, orientation, noncrossing, overlap, and containment audits.
- `OutputVertexLinks.h/.cc` — producer-side output link reconstruction and Component 10 fan comparison.
- `OutputTopologyCodec.h/.cc` — canonical encoding/decoding, section digests, full digest, and replay payload.
- `OutputTopologyVerifier.h/.cc` — separately implemented reconstruction, alternate traversals, exhaustive bounded checks, and mutation rejection.
- `OutputTopologyQueries.h` — narrow immutable views for Components 12, 15, diagnostics, and tests.

### 2.2 Infrastructure extensions

Extend, rather than duplicate:

- `ContractVersions.h` with all Component 11 provider, schema, key, codec, replay, and verifier versions;
- Component 01 stage, checkpoint, error-subcode, resource-kind, strong-ID-domain, diagnostic, and replay registries;
- Component 03 capability declarations only where a required structured bounded query is not already exported;
- the strict bounded Boolean target and explicit template-instantiation lists; and
- bounded test targets and CTest registration.

### 2.3 Strict target requirements

Do not compile Component 11 production or normative-test translation units in an ordinary Ygor target that permits fast math, reassociation, finite-only assumptions, a non-frozen contraction policy, or an unsupported rounding mode.

Use the strict target established by Components 01 and 03. Add explicit instantiations for:

```text
float,  uint32_t
float,  uint64_t
double, uint32_t
double, uint64_t
```

subject to platform qualification and public supported-type policy.

Add build-time or link-time tests proving the Component 11 production target does not depend on an external library or a legacy Boolean implementation.

### 2.4 Test files

Add focused tests under `tests/mesh_boolean_bounded/`:

- `TestOutputTopologyContracts.cc`
- `TestOutputTopologyIncidenceAudit.cc`
- `TestOutputTopologyZeroMeasure.cc`
- `TestOutputTopologyVertices.cc`
- `TestOutputTopologyRegions.cc`
- `TestOutputTopologyBoundaryDarts.cc`
- `TestOutputTopologyPairs.cc`
- `TestOutputTopologyCarrierBalance.cc`
- `TestOutputTopologyCycles.cc`
- `TestOutputTopologyContours.cc`
- `TestOutputTopologyWitnesses.cc`
- `TestOutputTopologyAdmissibility.cc`
- `TestOutputTopologyVertexLinks.cc`
- `TestOutputTopologyMutation.cc`
- `TestOutputTopologyProperties.cc`
- `TestOutputTopologyFuzzReplay.cc`
- `TestOutputTopologyResourcesCancellation.cc`
- `TestOutputTopologyDeterminismConcurrency.cc`
- `TestOutputTopologyStructuralPerformance.cc`
- `OutputTopologyFixtures.h/.cc`
- `OutputTopologyExactOracle.h/.cc`
- `GoldenOutputTopologyV2.h`

Keep arbitrary-precision oracles, corrupt-artifact builders, randomized generators, shrinkers, and golden regeneration tools test-only.

### 2.5 Typed entrypoint

Provide an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const polygonal_output_complex<T,I>>>
build_polygonal_output_complex(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const validated_operands_view<T,I>& validated,
    const source_triangle_complexes_view<T,I>& source_triangles,
    const canonical_source_manifolds_view<T,I>& source_manifolds,
    const signed_feature_relations_view<T,I>& relations,
    const canonical_intersection_complex_view<T,I>& intersections,
    const retained_surface_complex_view<T,I>& retained,
    const output_topology_capabilities<T,I>& capabilities);
```

`capabilities` freezes provider/schema versions, bounded-query versions, verification thresholds, internal ID widths, strict floating profile, and downstream compatibility. It contains no caller-supplied provider function pointer.

Validate all owners, roles, versions, digests, verification dispositions, ID domains, ranges, coordinate references, and capabilities before authoritative allocation.

### 2.6 Version registry

Add explicit nonzero versions for at least:

- incidence audit and disposition schema;
- paired-boundary versus support-only zero-measure schema;
- output vertex occurrence and coordinate-reference schema;
- provisional/final region and split schema;
- continuation-consumption and boundary-dart schema;
- paired edge and halfedge schema;
- pair endpoint initialization schema;
- endpoint port/fan reference schema;
- successor/predecessor schema;
- role-independent cycle key and cycle schema;
- contour role, parent, and region-nesting schema;
- contour-side witness schema;
- bounded admissibility schema;
- carrier/source-edge balance schema;
- vertex-link schema;
- artifact, codec, replay, diagnostic, and verifier schema; and
- every fixed provider listed in Section 0.

Zero, unknown, or incompatible required versions fail before construction. Include all versions in artifacts, errors, canonical bytes, and replay.

## 3. Strong IDs, enums, complete keys, and artifact layout

### 3.1 Strong ID domains

Define non-interchangeable strong IDs for at least:

- `output_incidence_audit_id`;
- `zero_measure_support_evidence_id`;
- `output_vertex_occurrence_id`;
- `output_coordinate_reference_id`;
- `provisional_face_region_id` and `output_face_region_id`;
- `region_member_id`, `continuation_consumption_id`, and `region_split_evidence_id`;
- `boundary_dart_id` and `boundary_transition_id`;
- `paired_output_edge_id` and `output_halfedge_id`;
- `halfedge_endpoint_fan_ref_id`;
- `face_cycle_id` and `cycle_halfedge_ref_id`;
- `contour_node_id`, `contour_relation_id`, and `contour_witness_id`;
- `zero_measure_boundary_id` and `admissibility_evidence_id`;
- `carrier_balance_audit_id` and `vertex_link_evidence_id`; and
- verifier/replay evidence IDs where a generic ID would permit domain confusion.

Do not alias these IDs to predecessor IDs, public `I`, `size_t`, offsets, pointers, or hashes. Checked accessors validate owner and domain.

### 3.2 Closed enums

Use explicit nonzero enumerators and reject unknown values for:

- incidence disposition: paired-boundary, paired-zero-measure-boundary, transparent-internal, consumed-owner-seam, support-only-zero-measure, suppressed-audit-only, invalid;
- coordinate source: source-vertex, canonical-event, supported-predecessor-construction, invalid;
- edge role: whole-source-edge, split-source-edge-interval, transverse-carrier-interval, coplanar-overlap-boundary, topology-separation-contact, supported-other, invalid;
- endpoint nominal relation: definitely-distinct, bit-equal, uncertainty-overlapping, same-occurrence-loop, invalid;
- cycle geometric category: definite-positive-area, proven-zero-measure, bounded-deferred, occurrence-distinct-coincident, invalid;
- contour role: outer, hole, deferred-zero-measure, coincident-distinct, invalid;
- witness source: predecessor-arrangement-cell, certified-retained-sector-construction, invalid;
- bounded admissibility: definite-valid, exact-lineage-tie-valid, deferred-degeneracy-valid, uncertain-topology-changing, definite-invalid, invalid;
- verifier disposition and failure phase.

No `default:` branch silently accepts a future enum value.

### 3.3 Complete semantic keys

Full keys define identity, equality, grouping, order, and ID assignment. Hashes may accelerate lookup only after full-key equality and never determine order, failure, bytes, or identity.

#### Output vertex key

Use the complete Component 10 occurrence requirement key plus owner namespace and schema versions. Exclude coordinates, coordinate hashes, offsets, task IDs, and traversal order.

#### Provisional region key

```text
(context owner,
 support semantic identity,
 occurrence/multiplicity sheet,
 prescribed orientation,
 sorted complete retained-use member keys,
 sorted reciprocal continuation keys,
 provider/schema versions)
```

#### Final region key

Add the exact positive-area member component and region-split evidence. Exclude area magnitude, AABB order, traversal root, and union representative.

#### Paired-edge key

Use the complete Component 10 planned edge key, exact two directed incidence keys, mapped endpoint occurrence keys, role/lineage, zero-measure class, and versions.

#### Halfedge key

```text
(paired-edge key,
 directed incidence key,
 canonical direction role,
 schema version)
```

Canonical slot assignment uses complete halfedge key order.

#### Cycle key

Compute the orientation-preserving lexicographically least rotation of complete halfedge keys, then use:

```text
(final region key,
 canonical oriented halfedge sequence,
 role-independent cycle-key schema version)
```

Do not include outer/hole/deferred contour role. Do not reverse the sequence. Contour role assignment cannot renumber cycles.

#### Contour key

Use final region key, cycle key, derived role, authoritative arrangement/support lineage, parent relation, occurrence separation, witness reference, and versions.

#### Witness key

Use final region/support identity, subject contour, intended side, source arrangement cell or retained sector lineage, canonical construction/evidence identity, and version. Exclude nominal epsilon offsets and arbitrary sample ordinals.

### 3.4 Immutable artifact layout

`polygonal_output_complex<T,I>` has a header containing:

- owner, operation, operand roles;
- strict floating profile;
- all provider/schema/policy/query versions;
- predecessor IDs and complete digests;
- checked counts and section ranges;
- resource leases and structural statistics;
- producer/verifier dispositions;
- section digests and complete digest.

Use canonical contiguous tables and checked CSR ranges for:

1. all incidence audit rows;
2. support-only zero-measure evidence and attachments;
3. output occurrences, coordinate references, sectors, ports, and reverse maps;
4. provisional/final regions, retained-use members, continuation members, and split evidence;
5. boundary darts and transparent-walk/cancellation evidence;
6. paired edges and exactly two halfedges per pair;
7. endpoint ports/fan references, region, role, and provenance;
8. successor/predecessor relations;
9. face cycles and canonical ordered halfedge/vertex references;
10. contour nodes, relations, and certified witnesses;
11. paired zero-measure boundary and repeated-coordinate descriptors;
12. source-edge/carrier order and balance evidence;
13. bounded embedding/noncrossing/containment evidence;
14. output vertex links;
15. forward/reverse maps and predecessor provenance;
16. resources, diagnostics, replay, codec, and verifier evidence.

Published references point only to immutable predecessor storage or immutable stage-owned buffers whose lifetime covers Components 12-15.

## 4. Stable checkpoints

Use these fixed checkpoints:

1. context and capability validation;
2. predecessor owner/version/digest/verification validation;
3. exact count, ID, byte, and work preflight;
4. resource reservation;
5. complete retained-incidence audit;
6. zero-measure boundary/support-only partition verification;
7. occurrence requirement validation and output occurrence allocation;
8. reciprocal continuation graph validation;
9. provisional region construction;
10. positive-area final region split;
11. exact boundary extraction;
12. planned-edge validation and atomic pair materialization;
13. source-edge/carrier balance verification;
14. boundary successor/predecessor construction;
15. face-cycle extraction and role-independent canonicalization;
16. contour role and final-region validation;
17. strict contour-side witness validation/construction;
18. bounded embedding/noncrossing/containment checks;
19. output vertex-link reconstruction;
20. complete edge/cycle/incidence/member-set audit;
21. canonical dense remap, reverse maps, and statistics;
22. producer invariant checks;
23. canonical codec, digests, and replay;
24. independent verification;
25. resource reconciliation and final cancellation poll;
26. commit.

Poll cancellation at every checkpoint and deterministic work-count intervals inside large sorts, scans, graph traversals, transparent walks, carrier domains, cycle walks, contour comparisons, broad-phase candidate loops, codec, and verifier passes. Never poll by wall clock.

## 5. Preflight equations and resource reservations

Let:

```text
U  = retained-use count
N  = total retained-incidence audit count
K  = directed transparent-continuation count
E  = planned edge occurrence count
H  = checked(2 * E)
Q  = nonempty vertex occurrence requirement count
Zb = paired zero-measure boundary incidence count, included in H
Zs = support-only zero-measure evidence count, excluded from H
```

Before allocation prove:

- every one of the `N` incidences has one supported disposition;
- realized boundary incidence count, including `Zb`, is exactly `H`;
- every planned edge contributes exactly two realized members;
- every support-only record in `Zs` contributes zero ports, edges, halfedges, boundary darts, and cycle references;
- every ordinary output occurrence requirement is nonempty and referenced by planned-edge endpoint ports;
- if `E == 0`, then `Q == 0` in V1;
- output vertex count is exactly `Q`;
- paired edge count is exactly `E`;
- halfedge count is exactly `H`;
- total cycle-halfedge references are exactly `H`;
- region member references are exactly `U`;
- continuation references are exactly the verified `K` member set;
- cycles are at most `H`, with one-edge loop-like degeneracies only under explicit policy;
- contour nodes are at most cycle count;
- support-only evidence is separately bounded by `Zs`; and
- all temporary and persistent structures fit strong-ID, byte, work, and policy limits.

Reserve persistent and peak temporary resources before allocation. Reconcile deterministically and transfer only verified persistent leases on commit.

## 6. Incidence audit and zero-measure partition

### 6.1 Audit construction

Sort all Component 10 retained incidences by complete incidence key and create one row containing:

- retained-use and source lineage;
- oriented endpoints and local ports if realized;
- carrier/seam/support lineage;
- multiplicity/separation class;
- expected planned edge/mate, reciprocal continuation, or support attachment;
- final disposition;
- consuming output entity or explicit no-entity reason.

### 6.2 Disposition mapping

Map predecessor records exactly:

- planned ordinary edge member -> `paired-boundary`;
- planned zero-measure edge member -> `paired-zero-measure-boundary`;
- reciprocal transparent seam member -> `transparent-internal`;
- consumed canonical owner seam -> `consumed-owner-seam`;
- explicit non-boundary zero-measure obligation -> `support-only-zero-measure`;
- suppressed non-owner evidence -> `suppressed-audit-only`;
- unresolved or contradictory -> failure.

A topology-separation delimiter that remains a result boundary must already be a planned edge member; it is not a free-standing disposition.

### 6.3 Exact coverage rules

Require:

- each planned directed member appears in exactly one realized audit row;
- each transparent member appears in exactly one reciprocal continuation pair;
- support-only members have no planned edge, continuation, endpoint port, or boundary ownership;
- consumed/suppressed rows have no consuming topology entity;
- no incidence belongs to more than one domain;
- no planned member is missing; and
- complete member sets match predecessor declarations.

## 7. Output vertex occurrence allocation

Sort all Component 10 vertex occurrence requirements by complete key. Equal keys fail. Equal coordinates are irrelevant.

For each requirement:

1. Validate owner, endpoint domain, separation/multiplicity class, and all ranges.
2. Require a nonempty simple closed alternating local-port cycle.
3. Require every port to reference one endpoint of a realized planned-edge incidence.
4. Resolve one authoritative coordinate source:
   - accepted source vertex;
   - canonical Component 08 event; or
   - explicitly supported predecessor construction.
5. Obtain the existing Component 03 bounded point and precision-ledger reference.
6. Verify finiteness, enclosure, residual, and lineage.
7. Store complete contributing sectors, ports, expected fan members, and reverse maps in canonical order.
8. Assign dense IDs only after validating the full sorted set.

A support-only zero-measure record does not produce a vertex occurrence. An empty ordinary occurrence is a predecessor contradiction in V1.

Producer checks prove one-to-one forward/reverse maps, exact endpoint coverage, preserved coordinate-source identity, and no coordinate-based grouping.

## 8. Exact regions and boundary darts

### 8.1 Reciprocal continuation graph

Build checked CSR adjacency over retained uses from sorted reciprocal continuation pairs. Validate exact opposite seam direction, lineage, endpoint requirements, orientation, support, sheet, multiplicity, and owner compatibility.

Do not cross:

- planned ordinary or zero-measure boundaries;
- topology-separation contacts;
- support-only zero-measure evidence;
- incompatible coincidence ownership;
- different multiplicity occurrences;
- different result-side transitions; or
- suppressed/consumed records except the exact consumed-owner seam semantics already represented as continuation by Component 10.

### 8.2 Provisional and final regions

Each continuation component is provisional. Validate common support semantic region, frame lineage, prescribed orientation, occurrence/multiplicity sheet, and precision/support contract.

Build a second exact positive-area graph. Include an adjacency only when Component 09/10 sector evidence certifies positive-area continuation. Exclude support-only and zero-measure-only bridges unless they are merely attached evidence, never graph edges.

Connected positive-area components become final regions. Attach support-only evidence only through a unique exact owner/region reference. Nonunique attachment fails if topology-affecting.

Finalize region IDs before edge/halfedge/cycle IDs.

### 8.3 Oriented dart model

For each nonsuppressed retained incidence with topology participation, create an oriented dart owned by one retained use. Define:

```text
face_next(d) = next incidence in authoritative retained-use boundary order
face_prev(d) = previous incidence
seam_twin(d) = reciprocal dart only for transparent-internal incidence
```

Do not create darts for support-only, consumed, or suppressed evidence.

For each boundary dart `b`:

```text
j = face_next(b)
while disposition(j) == transparent-internal:
    j = face_next(seam_twin(j))
return j
```

Bound the walk by exact region dart count. Crossing a final-region split, support-only record, suppression, unresolved state, or repeated transparent state before boundary is failure.

### 8.4 Boundary multiset

For each final region:

- collect all realized planned-edge members owned by region retained uses;
- consume every transparent seam exactly once in each direction;
- retain multiplicity/occurrence-distinct coincident incidences separately;
- record every transparent crossing;
- compare the exact result against Component 10 planned directed members assigned to the region.

No projected segment cancellation is permitted.

## 9. Atomic paired edges and balance

### 9.1 Planned-edge validation

Sort planned edge occurrences by complete key. Before allocation require:

- exactly two distinct directed incidence members;
- one canonical forward and one reverse role;
- both audit rows are realized boundary dispositions;
- both map to final regions;
- exact reversed mapped occurrence endpoints;
- compatible local ports and fan sectors;
- reciprocal actual/expected surface occurrence descriptors;
- matching carrier/span/interval slot and endpoint lineage;
- compatible result-side transitions, ownership semantics, multiplicity, and separation;
- explicit permission for loop or zero-measure form.

Do not search for an alternative mate.

### 9.2 Explicit-endpoint pair builder

Determine canonical halfedge slots from complete directed-halfedge keys. With checked arithmetic:

```text
edge_id = dense edge index
h0.id   = 2 * dense edge index
h1.id   = 2 * dense edge index + 1
```

Let validated directed member 0 map to `(v0 -> v1)`. Initialize:

```text
h0.origin      = v0
h0.destination = v1
h1.origin      = v1
h1.destination = v0
h0.pair        = h1.id
h1.pair        = h0.id
```

Then fill region, incidence, local ports, fan sectors, role, lineage, direction, and descriptors. Assert:

```text
h0.origin      == h1.destination
h0.destination == h1.origin
h0.pair        == h1.id
h1.pair        == h0.id
```

No endpoint field may be initialized by reading an unresolved field in the opposite builder record. Validate the complete pair and append both records atomically to private storage. Successor, predecessor, and cycle remain explicit unresolved builder fields until Section 10 and cannot enter an immutable artifact.

### 9.3 Zero-measure boundary descriptor

For equal-bit or uncertainty-overlapping endpoints, call Component 03 bounded length service and store:

- lower/upper length bounds;
- endpoint occurrence and coordinate-source identities;
- topological preservation reason;
- planned edge and two incidences;
- eventual incident regions/cycles;
- cleanup eligibility constraints; and
- Component 12 handling requirement.

Support-only zero-measure evidence uses its separate schema and never shares this paired-edge descriptor.

### 9.4 Carrier/source-edge balance

For every referenced Component 08 source-edge sequence or carrier domain:

- consume authoritative ordered interval/event members;
- compare exact planned member identities and directions;
- verify every planned edge belongs to exactly one domain audit;
- verify closed-loop start/end balance;
- verify documented source-edge closure for open carrier chains;
- preserve equal-parameter tie ordering by frozen lineage key;
- reject definite order reversal, duplicate, missing member, undocumented gap, or crossed realization.

## 10. Successor permutation and cycles

### 10.1 Boundary-dart to halfedge bijection

Create a bijection between every realized audit row and one output halfedge. Transfer the final-region dart successor to the corresponding halfedge.

For `h -> next(h)` require:

- `h.destination == next(h).origin` by occurrence identity;
- same final region;
- exact retained-sector/transparent-walk continuity;
- same occurrence fan, sheet, multiplicity, and owner boundary; and
- no topology separation crossing.

Construct predecessor only as inverse. Require totality, range validity, indegree/outdegree one, and exact realized member coverage.

### 10.2 Directed cycle extraction

Walk successor cycles from a canonical unvisited halfedge order. For each cycle:

- close at start halfedge and start occurrence;
- reject a previously visited non-start halfedge;
- reject current-walk reuse;
- bound steps by region boundary count;
- preserve orientation;
- collect ordered halfedge and origin occurrence IDs.

After all walks, every halfedge belongs to exactly one cycle.

### 10.3 Role-independent canonical cycle IDs

Compute orientation-preserving least rotation of complete halfedge keys. Build the cycle key solely from final region key, rotated sequence, and cycle-key schema version. Sort cycle keys globally and assign IDs.

Do not include outer/hole/deferred role. Contour classification occurs later and must not alter cycle ID, successor/predecessor, or canonical rotation.

Publish ordered roles/provenance per boundary segment, support lineage, projected-area evidence reference, repeated-coordinate/degeneracy descriptors, and digest.

## 11. Contours, witnesses, and bounded admissibility

### 11.1 V1 contour representation

For every positive-area final region require:

- exactly one positive-area outer cycle;
- zero or more direct positive-area holes;
- zero or more deferred zero-measure or occurrence-distinct contour records.

A positive-area island inside a hole is a separate final region. Multiple unrelated outer candidates indicate a failed earlier region split; do not perform a late split after edge IDs are frozen.

### 11.2 Lineage-first role assignment

Use:

- positive-area retained member component;
- prescribed orientation;
- source-facet semantic support and arrangement side;
- boundary incidence and local sector evidence;
- Component 10 owner, multiplicity, and separation data.

Request Component 03 structured projected signed-area evidence in the authoritative frame. Use it as verification or only as an explicitly permitted bounded disambiguator. A nominal area sign is insufficient.

Require exactly one outer. Attach holes to that outer. Validate separate island regions through cross-region support nesting evidence.

### 11.3 Certified strict-side witnesses

Prefer a predecessor-certified arrangement-cell witness. Otherwise construct a witness only through a canonical Component 03 operation over a certified positive-area retained atom or sector.

Every witness must prove:

- the intended side of the subject contour;
- source arrangement/atom/sector lineage;
- bounded finite enclosure;
- definite separation from every relevant boundary segment;
- deterministic construction and key; and
- compatibility with region orientation and role.

An arbitrary adjacent atom point is insufficient unless it is certified on the required contour side. A boundary vertex, repeated coordinate, nominal centroid without enclosure, and epsilon offset are prohibited.

If the witness enclosure can cross a relevant contour or does not prove unique containment, fail with `geometric_condition_exceeds_tolerance`.

### 11.4 Bounded containment

Use a deterministic conservative ray/sweep direction derived from stable support-frame identity. Call Component 03 structured segment/containment services and retain inside/outside/on-boundary/uncertain evidence.

Exact topology-distinct touches follow occurrence/separation lineage. Uncertainty that can change hole ownership or positive-area nesting fails.

### 11.5 Noncrossing and order admissibility

Build a deterministic per-support conservative interval/AABB candidate index over projected segment bounds. The index only prunes definitely separated pairs.

For every nonincident candidate pair classify with Component 03:

- definite proper unrepresented crossing -> failure;
- definite positive overlap without exact owner/coincidence lineage -> failure;
- exact endpoint/contact tie with documented separation -> valid and recorded;
- explicit zero-measure overlap with downstream obligation -> valid and recorded;
- topology-changing uncertainty -> `geometric_condition_exceeds_tolerance`;
- definite disjoint -> valid.

Also verify interval endpoint residuals, authoritative event order, tie keys, support orientation, and absence of definitely self-crossing positive-area cycles.

Use exhaustive all-pairs checks only below a frozen bounded threshold in production; the verifier may use exhaustive checks for bounded fixtures.

### 11.6 Deferred records

For every deferred paired zero-measure edge/cycle store exact halfedges, occurrences, bounded evidence, preservation reason, incident regions, Component 12 representation rule, Component 13 cleanup constraints, and replay witness.

For every support-only record store exact predecessor identity, unique region/use attachment or explicit global diagnostic ownership, bounded evidence, no-topology proof, downstream obligation, and replay witness.

Unexplained zero-measure structure is rejected.

## 12. Vertex links and complete producer audit

### 12.1 Vertex-link reconstruction

For each outgoing halfedge `h` at output occurrence `v` define:

```text
around_origin(h) = pair(prev(h))
```

Require it also originates at `v`. Traverse from least complete outgoing key and require one closed cycle containing every outgoing halfedge exactly once.

Map endpoint fan references back to Component 10 ports and compare exact:

- port member set;
- retained face sectors;
- corner transitions;
- edge-mate transitions;
- sheet/multiplicity/separation boundaries.

Allow cyclic rotation only. Empty, open, branched, duplicate, or multi-cycle links fail.

### 12.2 Complete member-set equations

Prove:

- output occurrence keys equal Component 10 nonempty occurrence keys one-to-one;
- paired edge keys equal planned edge keys one-to-one;
- halfedges equal exact planned directed incidence members one-to-one;
- support-only, consumed, and suppressed rows map to no halfedge;
- transparent rows map to no output edge and are consumed exactly once;
- every pair is reciprocal with reversed endpoints;
- every halfedge has one successor, predecessor, region, and cycle;
- cycle reference member set equals halfedge member set;
- region boundary sets equal unions of their cycles;
- source-edge/carrier domains balance;
- every occurrence link matches Component 10; and
- all output records have complete lineage.

### 12.3 Empty artifact

When retained uses, planned edges, and occurrence requirements are empty, publish empty topology sections, complete predecessor/policy references, zero counts, discard/suppression audit references, deterministic digests, and successful verifier disposition.

Support-only diagnostic evidence may be preserved in a separate audit section only when the regularized retained surface is still empty; it must not allocate topology.

## 13. Determinism, resources, encoding, and independent verification

### 13.1 Deterministic execution

Parallel tasks read immutable inputs and write private complete-keyed records. Canonical merge uses stable full-key sort, exact duplicate reconciliation, checked prefix sums, dense remapping, and Component 01 deterministic failure arbitration.

Thread count, delay, allocator, pointer, hash collision, continuation root, edge discovery, cycle start, contour order, witness candidate iteration, and verifier partition do not affect results. Every worker establishes the strict floating environment and joins before commit or rollback.

### 13.2 Resource accounting

Use separate resource subkinds for:

- incidence audit;
- support-only zero-measure evidence;
- occurrences, coordinate references, sectors, ports, and fan evidence;
- continuation graph and CSR storage;
- provisional/final regions, split evidence, and boundary darts;
- pairs, halfedges, endpoint references, and paired zero-measure descriptors;
- successor/predecessor and visitation;
- cycles and canonical rotation work;
- contours, witnesses, nesting, and deferred records;
- projected broad phase and bounded query evidence;
- carrier/source-edge balance and vertex links;
- canonical sort/remap;
- producer/verifier temporary and persistent bytes;
- diagnostics, replay, and work units.

`resource_limit` never drops evidence, skips admissibility, merges occurrences, or truncates topology.

### 13.3 Canonical encoding

Encode fixed-width fields in this order:

1. magic, artifact schema, owner, operation, operand roles;
2. strict floating profile, providers, policies, query versions, predecessor IDs/digests;
3. incidence audit;
4. support-only zero-measure evidence;
5. output occurrences, coordinate references, sectors, ports, reverse maps;
6. provisional/final regions, members, continuations, split evidence;
7. boundary darts and transparent-walk evidence;
8. paired edges and halfedges;
9. successor/predecessor and balance evidence;
10. role-independent cycles and ordered references;
11. contours, witnesses, nesting, deferred paired-boundary topology;
12. admissibility and vertex-link evidence;
13. complete balance, statistics, resources, producer/verifier disposition;
14. section digests and full digest.

Use Component 03 canonical scalar-bit encoding. Never encode raw object memory, padding, `size_t`, pointers, capacities, locale text, unordered iteration, task IDs, or wall-clock data.

### 13.4 Independent verifier

Implement in separate source files. It must not call producer orchestration or producer helpers as its sole proof.

Independently:

- validate owners, versions, ranges, resources, and predecessor digests;
- rebuild incidence dispositions and the realized/support-only split;
- rebuild occurrence bijection;
- rebuild continuation and positive-area final region components with alternate traversal;
- derive exact boundary sets;
- reconstruct planned pairs directly from Component 10;
- derive successor/predecessor with separately coded dart walking;
- extract cycles from a different start order and compare role-independent canonical sequences;
- verify contour roles from arrangement lineage;
- independently validate strict-side witnesses and containment;
- run exhaustive noncrossing/containment below threshold and scalable checks otherwise;
- reconstruct vertex links;
- compare all member-set equations;
- independently encode semantic sections and recompute digests/resource totals.

Producer/verifier disagreement prevents commit.

## 14. Failures and diagnostics

Add stable subcodes for every failure family in the component specification, including specific codes for:

- support-only evidence assigned ports or topology;
- zero-measure boundary lacking a planned pair;
- empty ordinary occurrence requirement;
- pair endpoint read-before-initialization or reciprocity mismatch;
- contour role included in cycle identity;
- missing/invalid contour-side witness;
- witness not strictly separated from boundary;
- topology-changing containment uncertainty; and
- ordinary target or strict floating profile mismatch.

Each error records checkpoint, least canonical witnesses, expected/actual members, bounded evidence, versions, resource counters, and replay identity.

Do not convert topology contradiction into nominal geometric fallback.

## 15. Test and qualification plan

### 15.1 Incidence and zero-measure tests

Construct valid artifacts containing all dispositions. Verify exact coverage and no-entity mappings. Mutate:

- missing/duplicate rows;
- edge plus continuation double ownership;
- support-only with local port;
- support-only in continuation graph;
- support-only in region boundary/cycle;
- zero-measure boundary without mate;
- planned zero-measure member reclassified support-only;
- forged counts/digests.

### 15.2 Output occurrence tests

Cover source vertex fans, shared event fans, point/edge-touch separate cycles, coincident multiplicity, equal-coordinate source/event identities, high valence, zero-length paired edges, hash collision, and permuted order.

Reject empty occurrence, open link, branch, disconnected cycles merged into one, and support-only-created occurrence.

### 15.3 Region and dart tests

Test transparent diagonals, classification seams, owner seams, concavity, annuli, and multiple holes. Prohibit selected carrier, point/edge separation, incompatible multiplicity, opposite orientation, non-owner seam, occupancy-changing fan, and support-only bridge.

Commit exact dart permutations and transparent-walk ledgers. Inject wrong twins, loops, split crossing, missing next, cross-sheet transition, and nominal turning-angle ambiguity.

### 15.4 Pair tests

Cover all edge roles, cross-operand transverse seams, equal-parameter clusters, zero-length topology, multiplicity copies, and four geometric uses partitioned into two planned pairs.

Initialize builder storage with poison patterns and assert endpoint fields are written from validated `v0/v1` before being read. Inject one/three members, same direction, wrong endpoint, wrong port, wrong descriptor, wrong interval, wrong region, and altered reciprocal ID.

### 15.5 Carrier balance tests

Cover multiple events, loops, chains through source edges, endpoint events, overlapping parameter envelopes with valid tie lineage, definite reversal, missing start, duplicated end, wrong partition, and crossed topology.

### 15.6 Cycle identity tests

Commit triangle, convex, concave, annulus, holes, islands, repeated coordinates, zero-length boundary, deferred cycle, coincident-distinct cycle, and high-valence known answers.

Verify cycle IDs/bytes are identical before and after contour-role assignment and under role-evidence recomputation. Mutation inserting role into cycle key must fail verifier.

### 15.7 Contour witness tests

Cover predecessor arrangement-cell witnesses, certified sector-derived witnesses, strict separation, narrow holes, deep nesting, repeated-coordinate touches, sibling holes, separate islands, and uncertain enclosures.

Reject arbitrary adjacent atom points on the wrong side, boundary points, nominal centroids without proof, and epsilon offsets.

### 15.8 Bounded admissibility tests

Exercise disjoint segments, proper crossing, overlap, endpoint touch, topology-distinct equal-coordinate touch, uncertain crossing, near-zero area, signed zero, extreme exponents, subnormals, narrow corridors, and equal-parameter ties.

Every topology-affecting query must retain structured Component 03 evidence.

### 15.9 Vertex-link tests

Cover ordinary and high valence, point/edge-touch separate links, coincident sheets, zero-length support, and degenerate-but-complete links. Inject open, branch, duplicate, wrong predecessor, cross-separation transition, and Component 10 port mismatch.

### 15.10 Verifier mutations

Corrupt occurrence identity, coordinate source, region membership, continuation consumption, zero-measure class, pair/endpoints, successor/predecessor, cycles, cycle key, contour role, witness, carrier balance, vertex links, IDs, ranges, resources, and digests. Repair cached fields where possible. Require rejection.

### 15.11 Exhaustive oracle, fuzzing, and shrinking

Generate small valid retained complexes with controlled region concavity, holes, carrier loops, valence, equal parameters, contacts, duplicate coordinates, zero-length paired edges, support-only records, and occurrence partitions.

Exhaustively enumerate continuation components, valid planned pairs, lineage-implied successor permutations, cycle covers, one-outer/direct-hole partitions, strict witness validity, and vertex links. Production must match the unique valid result or fail if no unique contract-valid result exists.

Shrink every crash, nonclosure, accidental weld, arbitrary pairing, support leakage, contour disagreement, nondeterminism, or verifier mismatch while preserving replay.

### 15.12 Metamorphic and determinism tests

Apply source permutations, ring rotation, legal subdivision/retriangulation, operand remapping, axis permutation, corrected sign flip, exact translation, power-of-two scaling, thread counts, forced delays, reversed continuation traversal, edge discovery, cycle starts, contour order, witness candidate order, and forced hash collisions.

Require byte-identical Component 11 artifacts after documented predecessor-ID remapping.

### 15.13 Resource, cancellation, sanitizer, portability, and build tests

Test every resource subkind at limit-minus-one, limit, and limit-plus-one. Cancel at every checkpoint and inside long operations. Confirm joined rollback and no visible partial artifact.

Run supported compiler/type/configuration matrices, ASan/UBSan, TSan, strict floating conformance, and dependency/link checks.

### 15.14 Structural performance gates

Assert:

- one audit row per retained incidence;
- occurrences exactly equal nonempty Component 10 requirements;
- pairs exactly equal planned edges;
- halfedges and cycle references exactly equal `2 * E`;
- support-only records produce zero topology entities;
- zero coordinate-neighbour and tolerance-weld queries;
- linear dart/cycle/link traversal;
- documented `O(n log n)` canonical sorting;
- scalable noncrossing candidate enumeration avoids unconditional quadratic work;
- no input-size recursion; and
- no hidden hot-loop allocation after reservation.

Commit baseline counters for large concave regions, many holes, long carriers, high-valence events, duplicate-coordinate occurrences, and large support-only evidence sets.

## 16. Implementation sequence and definition of done

Implement in this order:

1. versions, IDs, enums, complete keys, immutable artifact/view skeleton;
2. strict target integration, typed entrypoint, preflight, resources, checkpoints;
3. complete incidence audit and zero-measure partition;
4. nonempty occurrence allocation and coordinate references;
5. continuation graph, provisional/final positive-area regions;
6. boundary darts, transparent permutation, and exact boundary sets;
7. explicit-endpoint atomic pairs and carrier balance;
8. successor/predecessor, cycle extraction, role-independent cycle IDs;
9. contour roles, one-outer validation, certified witnesses, containment;
10. bounded noncrossing/order admissibility and deferred records;
11. vertex-link and complete producer audits;
12. canonical codec, replay, diagnostics;
13. independent verifier and mutation hooks;
14. full tests, fuzzing, resources, cancellation, determinism, sanitizers, and structural performance gates.

Do not mark Component 11 complete until:

- every nonempty occurrence requirement is realized once;
- every planned edge is born as one explicit-endpoint reciprocal pair;
- paired and support-only zero-measure records are disjoint and complete;
- every realized boundary incidence and transparent continuation is consumed exactly once;
- support-only evidence creates no topology;
- every halfedge belongs to one closed oriented cycle;
- cycle IDs exclude later contour roles;
- every occurrence reconstructs one Component 10 link cycle;
- final regions have one outer, direct holes, separate islands, and explicit deferred records;
- containment uses strict certified witnesses;
- equal coordinates and zero-length edges preserve required separation;
- carrier and embedding ambiguity never invokes a heuristic;
- independent reconstruction rejects every required mutation;
- canonical replay is stable across traversal, allocation, hash, and schedule;
- resources, cancellation, rollback, diagnostics, strict build, and dependency checks pass;
- Component 12 consumes the artifact without repeating Component 10/11 decisions; and
- all production and normative-test code is strict self-contained C++17.
