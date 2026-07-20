# Plan 09: Connectivity and Winding Classification

## 0. Scope and non-negotiable constraints

Implement **only Component 09** from `component_09_connectivity_winding_classification.md`. This stage consumes the verified immutable artifacts from Components 01-08 and publishes exactly one immutable `classification_complex<T,I>` for Component 10 and the later independent verifiers.

The V1 implementation is fixed by this plan as a deterministic **lineage-driven source-surface arrangement, descriptor-gated zero-delta grouping, signed quotient-graph, and bounded seeded winding propagation** provider. The executable serial path is the semantic reference. Parallel work may emit only private, complete-keyed proposals; canonical identities, grouping, seed selection, propagation values, failures, diagnostics, bytes, and digests must be identical to the serial reference.

The implementation must:

- decompose each source facet into classification atoms using only source topology and the authoritative event, interval, carrier, overlap, contact, and occurrence records from Component 08;
- make Component 04 internal triangulation diagonals transparent unless an authoritative Component 08 descriptor crosses or terminates on them;
- preserve positive-area atoms, boundary sectors, and zero-measure occurrence descriptors as separate explicit domains;
- derive every winding-changing adjacency from Component 07 crossing contributions aggregated by Component 08, never from a new geometric crossing test;
- group only atoms joined by exact topology through a descriptor-authorized zero-delta continuation;
- construct a canonical quotient graph with checked signed total and per-shell deltas;
- obtain at most one successful independent bounded seed query for each otherwise unanchored numeric propagation component, with deterministic retries only for tied or uncertain directions;
- propagate checked integer winding and per-shell contributions, then verify every graph equation, every anchor, every reverse edge, every self-loop, every parallel constraint, and local event/vertex conservation;
- preserve tangent, boundary, coplanar, coincident, symbolic-side, and topology-separated occurrence states without coercing them to ordinary inside/outside;
- publish the opposite-operand occupancy on both conceptual sides of every positive-area atom in the exact form required by Component 10;
- produce complete reconstruction, diagnostic, resource, replay, and independent-verification evidence; and
- remain strict portable C++17 with no external, vendored, downloaded, optional, or runtime-invoked dependency.

The component must not:

- call, adapt, copy from, or depend on `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- reread or mutate caller meshes;
- infer identity, adjacency, connectivity, atom equivalence, event equivalence, or occurrence equivalence from coordinate equality, coordinate proximity, overlapping uncertainty bounds, hashes, or tolerance;
- use the user tolerance as an inclusion epsilon, weld radius, arrangement simplifier, or tie breaker;
- recompute a Component 07 relation, crossing sign, crossing multiplicity, symbolic decision, point construction, or carrier support;
- reconnect Component 08 events because their nominal coordinates or carrier parameters compare equal;
- let a source-triangle internal diagonal create a semantic atom boundary or extra seed query;
- cast one ray per vertex, edge, triangle, or atom when connectivity permits one query for a whole propagation component;
- choose a random ray, wall-clock seed, hash-iteration order, first-writer result, union-root address, pointer address, allocation order, or worker completion order;
- clamp winding to zero or one, choose the first inconsistent propagation path, suppress a nonzero cycle, or default an unvisited group to outside;
- allocate retained Boolean uses, output vertex occurrences, output halfedges, output cycles, triangulated output faces, cleanup operations, or a public mesh;
- publish a partial atom set, group set, quotient graph, seed table, winding table, or side-label table after failure, cancellation, resource exhaustion, or verifier rejection;
- use exceptions for expected contract, geometry, resource, cancellation, codec, or verification failures; or
- serialize raw structs, padding, pointers, `size_t`, implementation-defined enum values, unordered iteration, or `std::hash` values.

Use Component 01 for ownership, strong IDs, checked count/index/byte arithmetic, stages, transactions, resources, cancellation, deterministic failure arbitration, diagnostics, replay, canonical encoding, SHA-256, and immutable publication. Use Component 02 as the only authority for source shell orientation, nesting, occupied-side semantics, and accepted regular-solid winding. Use Component 03 as the only authority for bounded points/scalars, projection, orientation, containment, interval comparison, construction uncertainty, strict floating behavior, and canonical scalar bits. Use Component 04 as the only authority for source-facet semantic rings, projection/support records, source-triangle groups, and internal-diagonal provenance. Use Component 05 as the only authority for source halfedge topology, source-edge direction, source-facet incidence, vertex fans, and shell membership. Use Component 07 as the only authority for crossing contributions and symbolic/contact meaning. Use Component 08 as the only authority for conceptual events, topology occurrences, ordered source-edge intervals, carrier spans, overlap regions, and cut/contact descriptors.

No failed, cancelled, partially encoded, or verifier-rejected artifact may publish. Mark Component 09 complete in `tracker.md` only after every requirement in Section 18 is represented by an implementable instruction and qualification gate.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Public mesh and vector types

`fv_surface_mesh<T,I>` in `src/YgorMath.h` remains the public boundary container only. Its `involved_faces` index is optional convenience state, and its mutators include tolerance-based duplicate merging and topology-changing cleanup. It is not an atom graph, incidence registry, winding provider, or immutable artifact.

Therefore:

- do not store atoms, groups, quotient nodes, or seed results in a temporary `fv_surface_mesh`;
- do not use public vertex or face indices as Component 09 IDs;
- do not consult `involved_faces`, vertex normals, metadata, or mutable helper caches;
- do not call `merge_duplicate_vertices`, `convert_to_triangles`, `remove_degenerate_faces`, `remove_disconnected_vertices`, `simplify_inner_triangles`, or `slice_with_planes`; and
- reserve public-mesh assembly for Component 14.

`vec2<T>` and `vec3<T>` may appear only as nominal payloads inside Component 03-owned bounded records. Their ordinary arithmetic, exact equality/order, normalization, distance, angle, and bare boolean predicates do not carry inherited uncertainty, lineage, conditioning, owner identity, typed failure, or replay evidence. Do not call these operations for authoritative arrangement, angular order, witness containment, ray intersection, or winding decisions.

### 1.2 Adaptive predicates and planar helpers

`YgorMeshesAdaptivePredicates` contains useful exact-floating sign machinery, but it does not propagate input uncertainty or construction envelopes. Production Component 09 must access it only if Component 03 has adopted and qualified it behind a versioned bounded capability. Tests may use it as an independent oracle only on fixtures satisfying its documented assumptions.

Existing point-in-polygon, contour, plane, line, line-segment, Delaunay, monotone decomposition, and triangulation utilities use interfaces or epsilon policies that are insufficient for this stage. They may be used for fixture preparation or non-authoritative visualization, never for atom topology, grouping, seed acceptance, or publication.

### 1.3 Existing orientation, BSP, verification, and graph code

`YgorMeshesOrient` mutates meshes, forms epsilon-based adjacency, uses bounding-box/centroid heuristics, and performs ordinary ray casting. `YgorMeshesBSPTree` uses raw planes and a conversion model unrelated to Component 07/08 lineage. Existing mesh verification routines are boolean/exception-oriented and do not reconstruct this artifact. None is a production provider for Component 09.

There is no existing deterministic strong-ID disjoint-set or signed-constraint-graph implementation satisfying Component 01 ownership, canonical-ID, resource, cancellation, diagnostic, and verifier contracts. Implement the small required graph machinery within the bounded subsystem using contiguous arrays, full-key sorting, checked arithmetic, and iterative traversal.

### 1.4 Improve and share the Component 02 bounded point classifier

Component 02 already requires a deterministic bounded point-against-shell query for shell nesting. Do not create a second incompatible ray/winding subsystem. Extract or complete the common mechanics as a versioned internal service:

```text
bounded_shell_query_provider_v1
```

The common provider owns direction generation, conservative triangle indexing, bounded ray/triangle classification, the half-open hit rule, per-shell signed contribution accumulation, deterministic retry, and query evidence. Component 02 supplies a validated interior probe and asks for containment during nesting. Component 09 supplies an atom witness and asks for the opposite operand's complete shell-contribution vector. Each adapter retains its own policy checks and error mapping.

If Component 02 was implemented before extraction, refactor it without changing its artifact bytes or observable semantics unless the shared-provider version is deliberately advanced. Add cross-adapter equivalence tests. A missing bounded primitive must be added to Component 03, not reimplemented here with raw arithmetic.

### 1.5 Spatial indexes and concurrency

Reuse a conforming immutable conservative triangle-query capability from Component 02 or Component 06 only when its owner, bound-inflation policy, canonical ordering, no-false-negative guarantee, and version are explicit. Otherwise add the narrow shared `BoundedShellQueryIndex` implementation described below. Bounding boxes prune candidates; they never establish inside/outside.

`YgorThreadPool::work_queue` is not a semantic executor because it lacks stage transactions, deterministic failure arbitration, strict worker floating-environment qualification, private-output merge, and join-before-rollback. Use Component 01/17 execution capabilities. The serial path remains mandatory.

### 1.6 Permitted implementation machinery

Use fixed-width integers, `std::array`, `std::vector`, `std::optional`, `std::variant`, `std::sort`, `std::lower_bound`, checked prefix sums, sorted-vector grouping, deterministic union-find, CSR ranges, and explicit BFS/DFS. `std::unordered_*` may be used only as a private acceleration cache whose hits verify full keys; it must never define equality, order, IDs, failures, bytes, or digests.

## 2. Fixed V1 providers and serial workflow

Freeze these identities:

```text
classification_provider:                 lineage_surface_arrangement_v1
triangle_arrangement_provider:           authoritative_segment_rotation_system_v1
facet_atom_provider:                     transparent_internal_diagonal_cells_v1
adjacency_provider:                      descriptor_gated_surface_adjacency_v1
grouping_provider:                       canonical_zero_delta_components_v1
quotient_provider:                       canonical_signed_constraint_graph_v1
atom_witness_provider:                   certified_local_cell_witness_v1
shell_query_provider:                    bounded_shell_query_provider_v1
shell_query_direction_policy:            canonical_primitive_integer_directions_v1
propagation_provider:                    canonical_spanning_forest_potential_v1
symbolic_side_provider:                  lineage_symbolic_side_projection_v1
```

Any change to atom domain, local embedding, internal-diagonal transparency, adjacency/union matrix, delta sign, seed admissibility, direction sequence, half-open rule, shell accumulation, propagation constraint, symbolic-side derivation, key fields, canonical order, or observable record layout requires an explicit version change.

The serial semantic reference executes these phases in order:

1. validate owner tokens, context operation, strict floating profile, provider/schema versions, predecessor identities/digests, and predecessor verification dispositions;
2. preflight all counts, products, ranges, IDs, work, temporary bytes, persistent bytes, diagnostics, and replay limits;
3. enumerate Component 08 incidence for each source triangle into private complete-keyed arrangement proposals;
4. normalize local vertices and loci without coordinate-based merging;
5. construct the bounded planar rotation system and enumerate triangle-local cells;
6. certify local cell coverage, orientation, boundaries, and interior witnesses;
7. merge cells across transparent internal diagonals to form source-facet atoms;
8. construct boundary sectors, zero-measure occurrence records, and complete raw atom adjacency;
9. verify reverse adjacency and authoritative crossing/contact provenance;
10. group exactly the descriptor-authorized zero-delta continuations;
11. assign canonical group IDs from complete member sets;
12. construct and verify the signed quotient graph, including total and per-shell deltas;
13. discover numeric propagation components and deterministic anchors;
14. select one canonical witness and perform at most one successful independent shell query for each otherwise unanchored component;
15. propagate winding and shell contributions over a canonical spanning forest;
16. verify every graph equation, anchor, local conservation condition, and accepted solid-domain value;
17. derive boundary, symbolic-side, coincident, and occurrence-separation labels for every atom;
18. build immutable reverse maps, statistics, reconstruction evidence, and canonical records;
19. run the independent verifier;
20. encode canonical bytes, recompute digests/resources, poll final cancellation, and atomically commit.

No later phase may repair an earlier contradiction. A verifier failure rolls back the stage.

## 3. Exact file and target layout

Add under `src/YgorMeshesBooleanBounded/`:

- `ClassificationComplex.h` — immutable artifact and checked read-only view;
- `ClassificationTypes.h` — closed enums, temporary records, complete keys, witnesses, and version constants;
- `Classification.h/.cc` — typed entrypoint and phase orchestration;
- `SurfaceArrangement.h/.cc` — triangle-local lineage arrangement and face-walk construction;
- `FacetClassificationAtoms.h/.cc` — internal-diagonal transparency, facet atoms, boundaries, and witnesses;
- `ClassificationAdjacency.h/.cc` — source-edge, cut, event-sector, vertex-fan, contact, and reverse adjacency;
- `ClassificationGrouping.h/.cc` — deterministic union proposals, groups, and reverse membership;
- `ClassificationQuotientGraph.h/.cc` — signed total/per-shell constraints and canonical edge aggregation;
- `BoundedShellQuery.h/.cc` — shared Component 02/09 bounded shell-query primitives and immutable query index;
- `ClassificationPropagation.h/.cc` — anchors, spanning-forest potentials, residual checks, and shell semantics;
- `ClassificationLabels.h/.cc` — atom side occupancy and boundary/symbolic projection;
- `ClassificationCodec.h/.cc` — canonical bytes and section/artifact digests;
- `ClassificationVerifier.h/.cc` — independent reconstruction and mutation rejection.

Extend `tests/mesh_boolean_bounded/` with:

- `TestClassificationArrangement.cc`;
- `TestClassificationAtoms.cc`;
- `TestClassificationAdjacency.cc`;
- `TestClassificationGrouping.cc`;
- `TestClassificationShellQuery.cc`;
- `TestClassificationPropagation.cc`;
- `TestClassificationContacts.cc`;
- `TestClassificationCanonicalization.cc`;
- `TestClassificationMutation.cc`;
- `TestClassificationFuzz.cc`;
- `ClassificationFixtures.h/.cc`;
- `ClassificationExactOracle.h/.cc`; and
- predecessor-shaped fixture builders with test-only corrupt constructors.

Register separate CTest targets for arrangement, atoms/adjacency, grouping/quotient, shell query, propagation, contacts, canonicalization/properties, mutation, and fuzz/replay. Apply the Component 01 strict floating-point target to all production and normative-test translation units. Keep production code standard-library-only.

## 4. Versions, checkpoints, and typed failures

### 4.1 Version registry

Add nonzero explicit constants in `ContractVersions.h` for every provider named in Section 2 and for:

- atom-domain schema;
- local-arrangement schema;
- adjacency schema;
- group schema;
- quotient schema;
- seed-query schema;
- side-label schema;
- `classification_complex` artifact schema;
- canonical codec; and
- independent verifier.

Unknown required versions fail. Never interpret zero or unknown as latest. Include versions in artifacts, diagnostics, replay, and canonical bytes.

### 4.2 Stable checkpoints

Reserve stable Component 09 checkpoints in this order:

1. capability/owner/version validation;
2. predecessor digest/reference validation;
3. count/work/byte/ID preflight;
4. triangle-incidence proposal generation;
5. local vertex/locus normalization;
6. angular ordering and rotation-system construction;
7. local face-walk/cell enumeration;
8. local coverage and witness certification;
9. internal-diagonal cell matching;
10. source-facet atom publication proposal;
11. sector/zero-measure construction;
12. raw adjacency construction;
13. reverse/delta/provenance verification;
14. zero-delta grouping;
15. group canonicalization;
16. quotient construction;
17. propagation-component and anchor discovery;
18. witness selection and shell-query attempts;
19. winding/shell-vector propagation;
20. complete residual/local-conservation verification;
21. side-label derivation;
22. immutable artifact assembly;
23. independent verification;
24. canonical encoding/digest/resource reconciliation;
25. pre-publication cancellation and commit.

Poll cancellation and reconcile deterministic resource slices at each checkpoint and within bounded long loops.

### 4.3 Failure subcodes

Allocate a disjoint Component 09 subcode range with explicit values for at least:

- wrong owner, version, operation, predecessor identity, predecessor digest, or verification disposition;
- count, byte, work, ID, integer-winding, or shell-contribution overflow;
- malformed event/occurrence/interval/carrier/descriptor reference;
- triangle incidence outside its source facet or triangle;
- missing authoritative endpoint or reverse locus;
- duplicate lineage record with contradictory payload;
- local loci proper-cross without an authoritative event;
- unresolved angular order or invalid rotation system;
- open/repeated/nonterminating face walk;
- local cell coverage gap or positive-area overlap;
- cell orientation or interior witness unavailable;
- internal-diagonal interval mismatch;
- semantic atom depends on triangulation diagonal;
- atom coverage gap, overlap, duplicate, or missing membership;
- invalid source-edge, event-sector, or vertex-fan transition;
- adjacency missing reverse or reverse delta not additive inverse;
- crossing delta/provenance differs from Component 07/08;
- prohibited union across cut/contact/occurrence separation;
- required continuation missing;
- group membership incomplete, duplicate, or noncanonical;
- quotient edge/member multiplicity inconsistency;
- nonzero quotient self-loop;
- conflicting parallel constraints;
- no positive-area seed candidate;
- seed witness not certified inside its atom or clear of delimiters;
- shell-query origin overlaps boundary;
- shell-query attempt uncertain/tangent/coplanar;
- shell-query directions exhausted;
- half-open hit ownership contradiction;
- shell contribution inconsistent with Component 02;
- unanchored propagation component;
- conflicting anchors;
- propagated path disagreement;
- nonzero cycle residual;
- local event/vertex conservation failure;
- accepted nonboundary winding outside zero/one;
- missing/contradictory boundary or symbolic side label;
- occurrence separation lost;
- codec/version/range/tag/trailing-data/digest error;
- independent verifier rejection;
- resource reservation/reconciliation failure;
- cancellation; and
- internal construction invariant failure.

Map unresolved bounded geometry that prevents safe arrangement, witness, or shell query to `geometric_condition_exceeds_tolerance`; representability to `index_overflow`; configured exhaustion to `resource_limit`; cancellation to `cancelled`; and committed predecessor contradictions or producer/verifier disagreement to `internal_invariant_error`. Do not use `internal_invariant_error` for an expected difficult but self-consistent geometric input.

## 5. Typed API and capability boundaries

Provide an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const classification_complex<T,I>>>
build_classification_complex(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const validated_operands_view<T,I>& validated,
    const source_triangle_complexes_view<T,I>& source_triangles,
    const canonical_source_manifolds_view<T,I>& manifolds,
    const signed_feature_relations_view<T,I>& relations,
    const canonical_intersection_complex_view<T,I>& intersections,
    const classification_capabilities<T,I>& capabilities);
```

Validate all inputs before authoritative allocation. The entrypoint supports either or both empty operands, executes in one stage transaction, joins private work before rollback, returns one verified artifact or one typed error, never logs as control flow, and never exposes mutable arrays or callbacks that can recompute predecessor facts.

Required narrow views are:

- Component 01: owner/operation/policies, strong-ID factories, checked arithmetic, resources, cancellation, deterministic execution/failure scopes, codec, digest, diagnostics, replay, transaction;
- Component 02: shell membership, parent/depth, orientation, occupied side, accepted winding convention, shell bounds, and source-facet orientation;
- Component 03: bounded points/vectors/scalars, facet projection, five-way comparisons, orientation and containment predicates, ray constructions, interval arithmetic, finite/conditioning/tolerance dispositions, canonical scalar bits;
- Component 04: source-facet rings, deterministic projection/support, triangle membership, edge roles, internal-diagonal pairing, coverage and provenance digests;
- Component 05: canonical vertices/edges/halfedges/triangles/facets/shells, reciprocal incidence, source-edge direction, vertex-fan order, and checked reverse maps;
- Component 07: relation identities, crossing contributions, shell owner, symbolic rule and side order, contact/coincidence class, and authoritative construction references;
- Component 08: events/occurrences, source-edge sequences/intervals, carrier sequences/active spans, overlap records, aggregates, exact cut/contact descriptors, occurrence-separation records, and complete forward/reverse incidence.

Component 10 receives a `classification_complex_view` with checked canonical iteration/access for atoms, sectors, zero-measure occurrences, adjacency, groups, quotient edges, seeds, winding, shell contributions, boundary states, and per-atom side labels. It receives no permission to run a new inclusion query or reinterpret relation geometry.

## 6. Strong IDs, enums, and complete keys

Add distinct strong-ID domains for at least:

- `classification_atom_id`;
- `local_arrangement_vertex_id`, `local_arrangement_locus_id`, `local_arrangement_dart_id`, and `local_arrangement_cell_id` in verifier/replay evidence;
- `atom_boundary_use_id`;
- `classification_sector_id`;
- `classification_occurrence_id`;
- `atom_adjacency_id`;
- `classification_group_id` and `group_membership_id`;
- `quotient_edge_id` and `quotient_edge_member_id`;
- `propagation_component_id`;
- `seed_candidate_id`, `seed_query_id`, `seed_attempt_id`, and `seed_hit_id`;
- `shell_contribution_id`;
- `propagation_assignment_id` and `constraint_residual_id`;
- `atom_side_label_id`; and
- reconstruction/verifier evidence IDs where a generic evidence ID is unsafe.

Do not alias these domains to predecessor IDs, `I`, `size_t`, or raw offsets.

Define explicit nonzero closed enums for:

- atom dimension and role: positive-area, boundary sector, zero-measure occurrence;
- local locus role: triangle boundary, original source-edge interval, transparent internal diagonal, transverse carrier span, coplanar overlap boundary, coincident boundary, tangent/contact delimiter;
- adjacency class: uncut continuation, transparent diagonal, numeric crossing, tangent continuation, contact delimiter, symbolic delimiter, coplanar boundary, coincident-sheet relation, topology-separated contact, intentionally absent, invalid;
- union eligibility: required, permitted, prohibited;
- boundary/contact state;
- seed source: empty opposite, certified exterior bound, boundary-derived anchor, bounded shell query;
- query attempt and hit dispositions;
- occupancy state: strict outside, strict inside, boundary, symbolic negative, symbolic positive, coincident-owned, unresolved, invalid;
- propagation edge role and verifier disposition.

### 6.1 Atom key

A final positive-area atom key contains:

```text
(
  context owner namespace,
  source operand/shell/facet semantic key,
  sorted canonical semantic boundary-cycle keys,
  sorted incident event/occurrence/carrier/descriptor lineage,
  source orientation role,
  atom-domain/provider/schema versions
)
```

It must not contain nominal coordinates, local source-triangle ID when bookkeeping-only, internal-diagonal identity, traversal ordinal, DSU root, worker/task identity, or hash. Triangle-local cells use temporary complete keys, but final atom identity is assigned only after transparent-diagonal merging and semantic boundary cancellation.

### 6.2 Adjacency and group keys

An adjacency key contains directed source/destination atom keys, exact source-topology locus or Component 08 descriptor, adjacency class, union eligibility, total delta, complete sparse per-shell delta vector, symbolic/contact payload, member lineage, and schema version. The reverse has the same undirected semantic locus and exactly negated numeric deltas.

A group key contains the complete sorted atom member keys plus preserved zero-delta boundary-relation keys. IDs are assigned by sorting group keys, never from union roots.

### 6.3 Quotient, seed, and label keys

A quotient-edge key contains source/destination group keys, directed semantic locus, total and sparse shell deltas, symbolic role, and complete sorted adjacency-member keys. Independent crossings between the same group pair remain separate unless their entire semantic key is equal; duplicate proposals merge while retaining every member reference.

Seed keys contain propagation component, target group, atom/cell witness lineage, witness-construction policy, opposite operand, query provider/direction policy, and schema version. Side-label keys contain atom, group, source orientation/occupied-side convention, opposite numeric/symbolic side states, and complete relation/descriptor lineage.

Full keys define equality and order. Hashes are accelerators only. Force-collision tests must not change behavior.

## 7. Immutable artifact schema

`classification_complex<T,I>` stores a header with owner, operand roles, context operation, ordinary-publication eligibility, all provider/schema versions, predecessor artifact IDs/digests, strict floating profile, precision capability, counts/ranges, section digests, complete digest, verification disposition, and immutable resource leases.

Use canonical contiguous tables with checked CSR-style ranges for:

1. **Atom records** — source operand/shell/facet, source triangles contributing geometry, dimension/role, orientation, canonical boundary-use ranges, event/carrier/descriptor lineage, local-cell reconstruction range, certified witness, group, and label.
2. **Local reconstruction records** — triangle-local vertices, loci, darts, rotation ranges, cells, boundary walks, transparent-diagonal match records, and coverage certificates. These are evidence, not downstream semantic identity.
3. **Boundary uses** — directed atom boundary locus, neighboring atom/sector where applicable, predecessor interval/span/descriptor, and orientation.
4. **Sector and occurrence records** — source-edge/event/source-vertex local sector, incident positive-area atoms, continuation prohibitions, contact class, and exact Component 08 occurrence separation.
5. **Adjacency records** — directed atoms, class, union eligibility, total and sparse shell deltas, symbolic/contact payload, reverse ID, and complete provenance.
6. **Group records and memberships** — sorted atoms, source components/shells represented, boundary categories, quotient range, seed candidates, canonical key/digest, and atom-to-group reverse map.
7. **Quotient nodes/edges/members** — directed constraints, reverse, parallel lineage, symbolic relation, total/shell deltas, and member adjacency evidence.
8. **Propagation components** — sorted nodes/edges, canonical root, anchors, selected seed, and verification summary.
9. **Seed candidates/queries/attempts/hits** — witness construction and enclosure, clearance certificate, direction ordinal, BVH candidates, every bounded hit disposition, half-open ownership, per-shell sums, retries, and final result.
10. **Winding and shell-contribution records** — checked integer values, assignment predecessor, anchor, and residual evidence.
11. **Boundary and side labels** — base opposite winding, negative/positive conceptual-side occupancy, numeric/symbolic/coincident origin, source occupied-side convention, contact/overlap/coincidence class, and Component 10 eligibility.
12. **Statistics, resource, diagnostics, replay, verifier, and canonical mapping records**.

Every reference is owner/domain/range checked. Published storage references only immutable predecessors and stage-owned immutable buffers whose lifetime extends through Components 10-15.

## 8. Arrangement, atoms, and adjacency algorithms

### 8.1 Triangle-local input normalization

For each source triangle, gather incidence by exact Component 08 reverse maps. Emit private proposals for:

- the three source-triangle corners and boundary intervals;
- every event occurrence on the triangle boundary or interior;
- every transverse carrier active span crossing the triangle;
- every coplanar/overlap/coincident boundary segment;
- every tangent/contact delimiter and required zero-measure occurrence; and
- the triangle-local portion of each source-facet semantic boundary.

Normalize by complete lineage key. Equal-coordinate records remain distinct unless Component 08 says they are the same occurrence. Duplicate proposals with equal full keys must have identical payloads; otherwise fail. Do not create a new intersection between two loci. If their bounded embedding proves a proper crossing without a Component 08 event, report a committed-predecessor contradiction. If their relation remains unresolved and topology could differ, return `geometric_condition_exceeds_tolerance`.

### 8.2 Rotation system without `atan2`

Project using the Component 04 facet frame and Component 03 bounded operations. For every local arrangement vertex, build outgoing directed darts. Order them by a fixed half-plane classification followed by bounded orientation of direction pairs; exact collinearity/ties use Component 08 lineage, overlap orientation, and symbolic order. Never normalize directions or use transcendental angles.

Every locus has two reverse darts except an explicitly represented triangle-boundary clipping record whose reverse belongs to the outside sentinel. Validate endpoint incidence, reverse reciprocity, no duplicate dart, and cyclic order. An unresolved angular comparison that can change face walks is a typed geometric failure.

### 8.3 Cell enumeration and coverage

Enumerate local faces with the standard rotation-system rule fixed for the source-facet orientation: at a dart destination, choose the immediately preceding outgoing reverse dart so the walked cell stays on the prescribed side. Mark each directed dart once. Detect open walks, repeated darts before closure, nontermination, and invalid outside transitions.

Classify the triangle-exterior walk from exact triangle-boundary incidence, not by signed nominal area alone. For each interior cell:

- record all oriented boundary cycles and containment nesting;
- certify nonzero/positive area under Component 03;
- choose a deterministic witness candidate sequence from exact rational barycentric combinations of source and event points;
- prove the witness enclosure is inside the cell and separated from every nonincident delimiter by a positive certified clearance; and
- retain all rejected witness attempts and bounds.

A nominal centroid is never accepted without containment. If a positive-area cell lacks a stable witness, fail rather than sampling across a cut. Verify that cell interiors are disjoint and their union covers the triangle away from published delimiters. Euler counts are diagnostic support, not the sole proof.

### 8.4 Transparent internal diagonals and source-facet atoms

For every Component 04 internal diagonal, use paired triangle topology and Component 08 interval/event incidence to partition both sides into identical canonical open intervals and endpoint clusters. Match local cells incident to each interval.

- If no authoritative semantic descriptor occupies the interval, continuation is mandatory and the cells are merged.
- If a cut/contact/overlap descriptor crosses or occupies the interval, follow that descriptor's exact continuation or separation rule.
- A bookkeeping diagonal contributes reconstruction evidence but never remains in a final semantic atom boundary.
- Mismatched partitions, missing partners, or a final atom boundary containing an unoccupied internal diagonal are failures.

Within each source facet, form connected components of local cells through all mandatory transparent matches. Rebuild each final atom's semantic boundary by cancelling paired internal uses and sorting the remaining original-source-edge/carrier/contact uses into oriented cycles. Assign atom IDs only after this merge.

Independently verify source-facet coverage: every positive-area source location away from delimiters belongs to one atom, atoms do not overlap in positive area, every semantic delimiter has the required incident sides, and legal alternative source triangulations produce equivalent atom boundaries after remapping.

### 8.5 Boundary sectors and zero-measure occurrences

Build explicit records at source-edge interval endpoints, event clusters, carrier endpoints, and source-vertex fans. Traverse Component 05 fan order and Component 08 descriptor incidence to identify local sectors and their incident positive-area atoms. Preserve topology-separated occurrences even when coordinates and conceptual events are shared.

Zero-measure point/edge contact does not become a positive-area atom and does not create a union edge. It records which sectors meet, whether positive-area continuation exists, whether volume connectivity is permitted, and which output occurrences must remain separate. Zero-measure records inherit classification from incident groups and symbolic relations; they never force an unstable seed query.

### 8.6 Complete adjacency matrix

Generate adjacency only through exact source topology or a Component 08 descriptor. Every potential transition receives exactly one class and explicit `union_eligibility`:

- uncut original source-edge continuation: delta zero, union required;
- transparent internal diagonal: delta zero, union required;
- ordinary same-region event/vertex-sector continuation: delta zero, union required only when descriptor and fan topology authorize it;
- numeric crossing interface: delta from Component 07/08, union prohibited;
- tangent continuation: numeric delta zero; union permitted/required only for the exact sectors designated as same positive-area region;
- contact delimiter: numeric delta zero, union prohibited unless an explicit regularized continuation record says otherwise;
- symbolic delimiter: preserve symbolic side relation, union prohibited unless the symbolic policy explicitly identifies one region;
- coplanar overlap boundary: preserve overlap-side/ownership data, generally union prohibited across the delimiter;
- coincident sheet relation: preserve same/opposite orientation and sheet occurrence, never union different sheets by coordinate equality;
- topology-separated point/edge contact: delta zero, union prohibited;
- intentionally absent: record why no adjacency exists.

For each directed adjacency publish its reverse. Numeric total and every sparse shell delta are exact additive inverses. Crossing deltas are copied by reference from authoritative aggregates; Component 09 performs no independent edge-plane sign test. Retain all member contributions even when their total is zero.

## 9. Grouping and quotient graph

### 9.1 Deterministic zero-delta grouping

Create union proposals only for adjacency marked `required` or policy-authorized `permitted`, with total and all shell deltas zero. Sort proposals by complete undirected atom-pair and adjacency key. Use a checked contiguous union-find for construction efficiency, but do not expose its roots.

After processing proposals, enumerate each component's complete sorted atom keys, sort components by those member sets, and assign dense group IDs. Reconstruct the unionable graph with a separate sorted BFS in the verifier and compare complete member sets.

A group may preserve boundary/contact relations among members as evidence, but every positive-area atom belongs to exactly one group and no group crosses an authoritative winding-changing cut or topology-separated contact.

### 9.2 Quotient graph

Map every nonabsorbed adjacency to its source/destination groups. Emit canonical directed quotient proposals. Exact duplicate proposals merge only by full semantic key and retain a sorted member list. Independent crossings between the same groups remain parallel edges.

For each numeric edge store:

```text
winding(destination) = winding(source) + total_delta
shell_vector(destination) = shell_vector(source) + sparse_shell_delta
```

Require the sum of sparse shell deltas to equal the total delta under the frozen convention. Reverse edges negate both. A numeric self-loop must have zero total and zero shell vector; otherwise fail. Symbolic self-relations may remain with explicit nonnumeric role. Zero-delta boundary relations needed by Component 10 may remain as symbolic/contact quotient records without becoming propagation constraints.

Canonical node and edge order is independent of traversal and schedule.

## 10. Anchors, bounded shell query, and propagation

### 10.1 Propagation components and anchor priority

Build connected components over numeric quotient constraints. Each receives anchors in this deterministic priority:

1. opposite operand empty: zero total and empty shell vector;
2. a certified atom witness enclosure definitely outside every opposite shell's conservative bound: zero;
3. an authoritative boundary-derived numeric anchor whose complete Component 07/08 relation and Component 02 shell side semantics determine the value without displacement;
4. one bounded shell query on the canonical admissible atom witness.

Bounds may prove exterior only when the complete witness enclosure is definitely separated from every shell bound. Bounds never prove interior. If multiple non-query anchors exist, retain and verify all. If they conflict, fail. Boundary-derived anchors must be explicitly enumerated by policy; do not invent one from an arbitrary contact.

### 10.2 Witness selection

A seed candidate must be a certified positive-area cell witness belonging to the group. Rank candidates by:

1. greatest certified lower bound on clearance to atom delimiters;
2. greatest certified cell-area lower bound;
3. least complete atom/cell key.

The selected witness enclosure must be contained in the atom, avoid unresolved opposite-boundary envelopes, and be valid for the entire query. If the first candidate is inadmissible against the opposite query index, try remaining candidates in canonical order and record reasons. If no candidate is admissible and propagation cannot anchor the component, return a typed geometric failure.

### 10.3 Shared bounded shell-query provider

The provider uses a fixed versioned sequence of primitive integer direction triples, normalized only through Component 03's bounded construction where needed. Order by squared integer length, then lexicographic signed components, excluding zero and duplicate/opposite directions according to policy. No random direction or transcendental generation is permitted.

For each attempt:

- construct the bounded ray from the witness enclosure;
- query a deterministic conservative BVH over opposite source triangles, inflated by all inherited precision;
- evaluate each ray/triangle relation once through Component 03;
- classify definite proper hits, exact edge/vertex hits, tangent, coplanar, behind-origin, boundary-overlap, and unresolved cases;
- apply one frozen half-open ownership rule compatible with Component 07 so a shared source edge or vertex contributes once;
- accumulate checked signed contributions by `source_shell_id`, then total them using Component 02 orientation/nesting semantics;
- reject and retry the whole direction if any required relation remains unresolved, coplanar, or origin-overlapping; and
- publish complete candidate, hit, ownership, retry, and sum evidence.

A successful query returns a sparse shell vector and total integer winding. For an accepted regular solid and a witness away from boundary, total must be zero or one. Values outside that domain are not clamped.

Only one successful query is permitted per otherwise unanchored propagation component. Deterministic failed attempts do not violate that limit. Additional successful queries may run only under explicit diagnostic/exhaustive verification modes and must agree.

### 10.4 Checked propagation and complete consistency proof

Choose the least canonical anchored group as root. Sort outgoing numeric edges by complete key. Build a canonical spanning forest and assign each unvisited group exactly once with checked integer addition; repeated discovery verifies equality.

After assignment, evaluate **every** numeric quotient edge, not merely a sampled cycle basis:

```text
residual_total = value(dst) - value(src) - edge.total_delta
residual_shell = shell(dst) - shell(src) - edge.shell_delta
```

Every residual must be zero. This checks tree edges, non-tree edges, all fundamental cycles, parallel edges, reverse edges, and self-loops. Re-evaluate every anchor against the assigned value. Verify each sparse shell contribution against Component 02 shell parity and that its sum equals total winding.

Also reconstruct local closed walks around every event cluster and source-vertex sector and require numeric delta conservation plus compatible symbolic-side order. A contradiction is never resolved by choosing one path.

Every numeric component must be fully visited and anchored. Accepted nonboundary values are exactly zero or one under the Component 02 policy; unsupported values produce a precise failure.

## 11. Boundary, coincident, and per-atom side labels

For each group publish base opposite-operand numeric winding/inclusion and shell contributions. For each positive-area oriented atom derive:

- opposite occupancy on the atom's conceptual negative side;
- opposite occupancy on its conceptual positive side;
- whether each value is numeric, boundary-derived, symbolically assigned, or coincident-owned;
- boundary/contact dimension and tangent/transverse/coplanar/coincident class;
- same/opposite sheet orientation and symbolic owner/cancellation lineage;
- whether crossing a boundary changes numeric winding;
- all incident crossing/contact quotient records;
- source shell occupied-side orientation from Component 02; and
- group, seed/anchor, propagation, and relation evidence.

For an ordinary noncoincident atom, opposite occupancy normally agrees on both infinitesimal sides. For coplanar/coincident atoms, derive the two conceptual side states solely from Component 07 symbolic policy, sheet orientation/order, Component 08 overlap incidence, and neighboring numeric groups. Do not construct a coordinate offset.

Every boundary sector and zero-measure occurrence receives compatible inherited labels and explicit occurrence-separation flags. Invalid, missing, or contradictory side states make the artifact ineligible for Component 10; never default to outside or discard.

## 12. Determinism, concurrency, resources, and transactionality

### 12.1 Parallel execution

Permitted parallel phases are per-triangle proposal generation, independent local arrangement construction, independent facet merge proposals, raw adjacency proposals over canonical ranges, independent seed-attempt candidate evaluation, and verifier partitions. Each task reads immutable inputs and writes a private buffer with a deterministic reservation slice.

Canonical merge performs full-key sort, exact duplicate reconciliation, dense ID remapping, and deterministic failure selection. DSU union order, path compression, queue order, BVH visitation, task delay, worker count, and hash collisions must not affect observable results. Canonical grouping, final seed target choice, propagation root/order, bytes, and primary failure must reproduce the serial reference.

Every worker establishes and verifies the strict Component 01 floating environment. All workers join before commit or rollback.

### 12.2 Resource accounting

Add distinct Component 01 resource kinds or documented subkinds for:

- local arrangement vertices/loci/darts/cells and boundary uses;
- facet atoms and memberships;
- sectors and zero-measure occurrences;
- raw/canonical adjacency and reverse records;
- union proposals and DSU storage;
- groups and memberships;
- quotient nodes/edges/member evidence;
- propagation components, anchors, and assignments;
- seed candidates/queries/attempts/BVH candidates/hits;
- shell contributions and residual evidence;
- side labels;
- verification records;
- canonical/replay/diagnostic bytes;
- temporary and persistent bytes; and
- abstract work units.

Preflight conservative maxima with checked arithmetic before allocation. Reserve before allocation, reconcile actual usage deterministically, and transfer only verified persistent leases at commit. A limit failure may not merge atoms, skip evidence, omit a seed attempt, truncate required incidences, or weaken verification.

### 12.3 Cancellation

Poll at every stable checkpoint and bounded intervals during incidence enumeration, dart sorting, face walks, diagonal matching, adjacency generation, union processing, quotient construction, BVH query, hit accumulation, propagation, residual checking, verification, and encoding. Cancellation returns `cancelled`, joins workers, destroys private buffers, releases all reservations, and exposes no partial artifact.

## 13. Canonical encoding, diagnostics, and replay

Encode explicit fixed-width fields in this order:

1. magic, artifact schema, owner, operation, operand roles;
2. provider/policy/schema/version registry and strict floating profile;
3. predecessor IDs/digests and source semantic/triangulation digests;
4. atom and local reconstruction sections;
5. boundaries, sectors, and occurrence sections;
6. adjacency and reverse sections;
7. groups and memberships;
8. quotient nodes/edges/members;
9. propagation components and anchors;
10. seed queries/attempts/hits;
11. winding, shell contributions, assignments, and residuals;
12. boundary and side labels;
13. statistics/resources/verifier disposition; and
14. section and complete digests.

Encode floating values through Component 03 canonical bit services. Encode sparse vectors in sorted shell order. Never encode capacity, raw object memory, locale text, pointer, task ID, or unordered iteration. Semantic bytes exclude presentation and legal internal-triangulation ordinals where the atom contract declares them irrelevant; replay sections retain exact predecessor presentation needed to reproduce execution.

Every failure includes the least canonical classification direction, source features, event/occurrence/carrier/descriptor/relation lineage, local arrangement witnesses, atoms/groups/edges, expected/actual deltas or labels, bounded numeric intervals and exact nominal bits, seed direction/hits, policy versions, counters, cancellation checkpoint, and replay identity. Diagnostic capacity does not alter the primary failure.

## 14. Independent verifier

Implement `ClassificationVerifier` as a separate target/source module. It receives immutable predecessor views, the proposed artifact, and narrow Component 03 verification capabilities. It must not call producer orchestration, DSU grouping, seed selection, spanning-forest propagation, or producer codec helpers as its sole proof.

The verifier independently:

- validates all owners, versions, dense IDs, sorted ranges, references, reverse maps, and resources;
- rebuilds triangle incidence from Component 08 reverse mappings;
- reconstructs local planar embeddings using an alternate enumeration order and verifies dart rotation, face walks, coverage, and witness containment;
- rematches internal-diagonal intervals and proves no semantic diagonal remains;
- reconstructs final facet atom boundaries and complete member sets;
- rebuilds source-edge/event/vertex-fan adjacency and rejects coordinate-only edges;
- finds zero-delta components with sorted BFS rather than producer DSU;
- rebuilds quotient constraints and member multiplicity;
- verifies every total/shell delta directly against Component 07/08;
- revalidates seed witness containment and reruns every successful seed query with a verifier-owned BVH or exhaustive bounded mode;
- solves graph constraints in a different deterministic node/edge order and checks every residual and anchor;
- reconstructs local conservation and Component 02 shell semantics;
- derives boundary/symbolic side labels independently; and
- re-encodes all semantic sections, recomputes digests, and reconciles persistent resources.

For bounded tests, switch to exhaustive source-surface decomposition and an in-tree arbitrary-precision integer/rational oracle for projection orientation, ray/triangle relation, half-open ownership, shell winding, atom coverage, and graph values. Production never depends on the exact oracle.

Any producer/verifier disagreement returns `internal_invariant_error::classification verifier rejection` and prevents commit. Provide test-only corrupt constructors unavailable in production builds.

## 15. Test and qualification plan

### 15.1 Arrangement and atom tests

Cover empty operands; untouched tetrahedra; one and several transverse delimiters; several events on one source edge; carrier continuation across internal diagonals; closed loops spanning triangles; tangent point/edge contacts; coplanar overlap boundaries; same/opposite coincident sheets; high-valence vertices; multiple loci at one event; equal-coordinate distinct occurrences; and coordinate-coincident disconnected shells.

Independently assert rotation order, dart reverses, closed walks, cell coverage, witness containment/clearance, semantic boundary cancellation, and source-facet coverage.

### 15.2 Known-answer grouping and quotient tests

Commit exact expected atoms, adjacency classes, union eligibility, groups, quotient edges, shell deltas, anchors, winding, labels, and occurrence flags for disjoint solids, strict containment, one overlap loop, several loops, a large untouched region, cavities/islands, disconnected solids, point/edge/face touching, equal operands, and partial coincidence.

### 15.3 Shell-query tests

Test clear interior/exterior, nested cavities/islands, multiple disconnected shells, first direction hitting a vertex/edge, deterministic retry, tangent/coplanar attempts, boundary-overlapping origin, all directions ambiguous, large translation with small features, signed zero/subnormals, adjacent floats, precision/tolerance boundaries, internal-diagonal hits, source retriangulation, and Component 02-versus-09 adapter equivalence. Assert stable attempt order, half-open ownership, shell vectors, and precise typed failures.

### 15.4 Propagation and cycle tests

Use synthetic quotient graphs with `+1`/`-1`, multiple paths, consistent/inconsistent parallel edges, nested sparse shell vectors, valid zero-residual cycles, injected nonzero cycles, self-loops, conflicting anchors, unanchored components, integer overflow, and local event-sector conservation mutations. Producer and verifier must agree on all valid values and reject all contradictions.

### 15.5 Contact/coincidence matrix

Cover vertex, edge, and face touch; tangency; same/opposite coincident facets; partial coplanar overlap; equal operands; separate coincident shells; and multiple sheets at one event cluster. Verify numeric zero where required, nonunion delimiters, symbolic side occupancy, ownership/cancellation lineage, and occurrence separation.

### 15.6 Exact-oracle and metamorphic tests

On small integer-coordinate meshes, compare local embedding/cells, facet coverage, ray hits, shell contributions, total winding, quotient values, and side labels with the rational oracle.

Apply alternative legal facet triangulations, topology-preserving source subdivision, operand exchange/remapping, source vertex/edge/facet/shell/component permutations, ring rotation, corrected global orientation reversal, axis permutation, sign flip, exact translation, power-of-two scaling with precision scaling, thread counts 1/2/max, forced task delays, altered union order, queue order, BVH child order, and reversed merge partitions. Require equivalent semantic atoms and byte-identical canonical artifact/error after documented remapping; retriangulation alone must not increase seed count.

### 15.7 Mutation tests

Inject, one at a time: missing/extra local vertex/locus/dart/cell; changed angular order; open face walk; proper crossing without event; internal diagonal retained as semantic boundary; cell merged across cut; atom split at transparent diagonal; missing/duplicate atom; coordinate-only adjacency; missing continuation; changed union eligibility; atom in two groups; noncanonical group; flipped total/shell delta; reverse not negated; lost parallel member evidence; changed seed/witness/direction/result; omitted shell contribution; changed propagated winding; hidden residual; changed boundary/symbolic side; lost occurrence separation; forged count/resource/digest. Repair cached counts/digests where possible. The independent verifier must reject every mutation.

### 15.8 Fuzzing, shrinking, resources, and build matrix

Generate valid exact-template manifolds varying shell count/nesting/genus, cut loops, event valence, triangulation/subdivision, tangent/coplanar/coincident contact, duplicate coordinates, ULP perturbation/scale, precision/tolerance, direction ties, limits, and execution partitions. Serialize and structure-shrink every crash, hang, nondeterminism, false union, missed cell, invalid seed, path inconsistency, lost occurrence, oracle disagreement, or verifier disagreement.

For every resource kind test limit-minus-one, limit, and limit-plus-one. Cancel during every checkpoint. Run GCC/Clang Debug/Release, `float`/`double`, `uint32_t`/`uint64_t`, ASan/UBSan, and TSan for parallel paths. Prove strict translation units are not compiled with fast-math/reassociation.

Track structural counters for incidence, loci/darts/cells, adjacency, union proposals, quotient edges, seed attempts, BVH candidates, bounded predicates, residual checks, and bytes. Large disjoint/clustered fixtures must detect one-query-per-atom, repeated relation evaluation, or unconditional all-pairs behavior. Wall-clock time alone is not a correctness gate.

Maintain a manifest mapping every normative clause in `component_09_connectivity_winding_classification.md` and every Section 18 item to named tests and verifier checks. CI fails for any unmapped clause or surviving required mutation.

## 16. Complexity requirements

The production implementation must be output-sensitive:

- local arrangement work scales with authoritative triangle incidence/loci plus emitted cells and deterministic sorts;
- facet merging scales with local cells and matched internal-diagonal intervals;
- adjacency scales with published intervals, sectors, and descriptors;
- grouping is near-linear after full-key sorting;
- quotient construction scales with adjacency/member evidence;
- ordinary shell queries are at most one successful query per otherwise unanchored propagation component and use a conservative index;
- propagation and complete residual verification are linear in quotient nodes plus edges; and
- scalable verification does not become unconditional global all-pairs.

Document counter-based regression thresholds. Optimization may not weaken conservatism, evidence, or deterministic behavior.

## 17. Implementation sequence and gates

1. **Schemas, versions, IDs, enums, subcodes, files, and build targets.** Gate: strict C++17 headers compile; all keys/enums round-trip; no pointer or `size_t` is serialized.
2. **Shared bounded shell-query primitives and Component 02/09 adapters.** Gate: direction, half-open, sign, exact-oracle, and adapter-equivalence tests pass.
3. **Triangle incidence and local vertex/locus normalization.** Gate: lineage duplicates and equal-coordinate-distinct cases pass; malformed incidence has precise failures.
4. **Rotation system and local cell enumeration.** Gate: exact small arrangements, coverage, face-walk mutations, and witness certificates pass.
5. **Transparent internal-diagonal merge and final facet atoms.** Gate: alternative triangulation/subdivision corpus produces equivalent semantic atoms.
6. **Sectors, zero-measure records, and raw adjacency.** Gate: complete source-edge/event/vertex/contact matrix reconstructs independently.
7. **Zero-delta grouping and canonical IDs.** Gate: known-answer groups, forced union order, hash collisions, and false/missing union mutations pass.
8. **Signed quotient graph.** Gate: reverse, multiplicity, self-loop, total/shell-vector, and complete-key dedup tests pass.
9. **Witnesses, anchors, and bounded seed queries.** Gate: clearance, retry, nested shell, one-successful-query, and exact winding tests pass.
10. **Propagation and complete residual/local conservation verification.** Gate: all path/cycle/parallel/overflow/conflicting-anchor cases pass.
11. **Boundary/symbolic side labels.** Gate: complete tangent/coplanar/coincident/occurrence matrix validates as Component 10 input.
12. **Immutable artifact, codec, digests, and independent verifier.** Gate: round trips, all mutations, resource reconstruction, and collision tests pass.
13. **Transactions, cancellation, replay, deterministic concurrency, and structural performance.** Gate: every rollback/thread/partition/benchmark test passes.
14. **Full qualification and traceability.** Gate: Section 18 is fully satisfied before `tracker.md` changes.

Do not integrate Component 10 against an unverified or partially labeled artifact.

## 18. Definition of done

Component 09 is complete only when all of the following are true:

- every predecessor owner, version, digest, reference, and verification disposition is checked before use;
- all count, index, byte, work, integer-winding, and shell-vector arithmetic is checked and resource-accounted;
- triangle-local arrangements use only authoritative Component 08 lineage and never create an unregistered crossing;
- every locus has valid endpoints/reverse darts/canonical angular order and every local cell walk is independently reconstructible;
- every positive-area local cell has a certified interior/clearance witness;
- final atoms cover each source facet exactly once away from delimiters and do not depend on internal triangulation diagonals;
- legal source retriangulation/subdivision preserves semantic atom boundaries, groups, values, labels, and seed count except where new semantic source features are deliberately introduced;
- boundary sectors and zero-measure occurrences preserve every required contact and topology-separated occurrence;
- adjacency derives only from exact source topology and Component 08 descriptors;
- every adjacency has a valid reverse and every numeric delta is reused from Component 07/08 with exact shell provenance;
- zero-delta grouping crosses only descriptor-authorized continuation and never coordinate-only contact;
- every positive-area atom belongs to exactly one canonical group;
- group IDs derive from complete member sets and are invariant under union order, traversal, allocation, hash collision, and schedule;
- quotient edges preserve independent crossing multiplicity and complete member evidence;
- every numeric propagation component has deterministic valid anchors;
- ordinary production performs at most one successful independent seed query per otherwise unanchored propagation component;
- seed witnesses are certified inside atoms and clear of unresolved boundaries;
- the shared shell-query provider uses fixed deterministic directions, Component 03 bounded arithmetic, and the frozen half-open/sign convention;
- seed ambiguity causes retry or typed failure, never a guessed boolean;
- shell contribution vectors and total winding propagate with checked integers;
- every tree, non-tree, reverse, self-loop, parallel, and anchor constraint is verified;
- every local event/source-vertex sector cycle satisfies numeric and symbolic conservation;
- accepted nonboundary winding is exactly zero or one under Component 02's regular-solid policy without clamping;
- boundary, tangent, coplanar, coincident, symbolic, and occurrence-separation states remain explicit and complete;
- every positive-area atom has valid opposite-operand negative-side and positive-side labels sufficient for Component 10 without new geometric queries;
- all records are immutable, context-owned, canonical, transactionally published, and independently verifiable;
- the independent verifier reconstructs arrangements, groups, quotient constraints, seed winding, propagation, and labels through materially different control flow;
- every required corruption is rejected even when cached counts and digests are repaired;
- cancellation joins all work, releases reservations, and publishes nothing;
- resource exhaustion fails deterministically without simplifying or omitting required evidence;
- artifact, error, diagnostic, and replay bytes are stable across input presentation, legal triangulation, thread count, task partition, hash collision, and traversal permutations after documented remapping;
- unit, known-answer, property, metamorphic, adversarial, exact-oracle, mutation, fuzz/shrink/replay, resource, cancellation, sanitizer, compiler, type-matrix, and structural performance tests pass;
- every normative Component 09 clause maps to executable evidence;
- no excluded legacy Boolean implementation, unsuitable Ygor helper, external dependency, downloaded framework, random ray, tolerance-based weld, or coordinate-based connectivity is used; and
- all production and normative-test code remains strict portable C++17.
