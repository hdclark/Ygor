# Plan 10: Boolean Selection and Occurrence Accounting

## 0. Scope and fixed V1 design

Implement **only Component 10** from `component_10_boolean_selection_occurrence_accounting.md`. Consume the verified immutable artifacts from Components 01-09 and publish one immutable `retained_surface_complex<T,I>` for Component 11. Do not allocate output vertices, halfedges, face cycles, triangles, cleanup actions, or a public mesh.

Freeze V1 as the deterministic provider:

```text
selection_provider:                    side_truth_and_lineage_selection_v1
coincidence_provider:                  symbolic_sheet_cell_ownership_v1
retained_incidence_provider:           semantic_boundary_incidence_v1
edge_occurrence_provider:              exact_mate_signature_pairs_v1
vertex_occurrence_provider:            degree_two_local_link_cycles_v1
feasibility_provider:                  complete_retained_complex_audit_v1
```

The executable serial path is the semantic reference. Parallel work may produce only private complete-keyed proposals; canonical IDs, owners, dispositions, occurrences, failures, diagnostics, bytes, and digests must match the serial path exactly.

Non-negotiable rules:

- Audit every positive-area Component 09 atom exactly once.
- Construct A/B occupancy tuples on both conceptual sides from Component 02 source-side semantics and Component 09 opposite-side labels.
- Call the frozen Component 01 truth-table service exactly once per atom; never duplicate Boolean logic locally.
- Determine retain/discard and preserve/reverse solely from result occupancy on the two sides.
- Resolve coincident/coplanar sheets jointly from Component 07-09 lineage and Component 01 symbolic owner ranking.
- Suppress internal sheets and non-owner duplicates without losing provenance.
- Represent multiplicity with separate retained-use and occurrence identities, never multi-use edges.
- Pair boundary incidences only when one complete lineage-defined edge-occurrence slot identifies exactly one mutually compatible forward/reverse pair. The two member surface-owner lineages may differ, as they normally do on a proper transverse A/B seam.
- Partition vertex/event occurrences by closed cycles in an exact local link graph; coordinate coincidence never joins cycles.
- Fail closed on ambiguity, contradiction, resource exhaustion, cancellation, or verifier disagreement.
- Use strict portable C++17 and the standard library only. No external, vendored, downloaded, optional, or runtime-invoked dependency.
- Do not call, adapt, or copy `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`.

Prohibited shortcuts include coordinate/tolerance welding, normal comparison, geometric nearest-neighbour matching, random or hash-dependent decisions, source-triangle-order ownership, recomputing classifications or relations, requiring paired faces to have the same source operand or sheet owner, and allowing Component 11 to repair or choose among multiple mates/fans.

## 1. Existing Ygor assessment and reuse decisions

### 1.1 Public mesh and vector types

`fv_surface_mesh<T,I>` in `src/YgorMath.h` is a permissive mutable transport type. Its public indices, optional `involved_faces`, normals, colours, metadata, and mutators do not encode classification atoms, symbolic ownership, exact carrier intervals, multiplicity, or separate equal-coordinate occurrences.

Therefore:

- do not use a temporary `fv_surface_mesh` for this stage;
- do not use public vertex/face indices as Component 10 identities;
- do not consult `involved_faces`, normals, colours, or metadata for selection;
- do not call `merge_duplicate_vertices`, `convert_to_triangles`, `remove_degenerate_faces`, `remove_disconnected_vertices`, simplification, slicing, or remeshing helpers; and
- reserve public-mesh assembly for Component 14.

`vec2<T>` and `vec3<T>` may appear only as nominal payloads owned by Component 03 records. Component 10 performs no authoritative dot/cross, normalization, distance, angle, projection, equality, or ordering computation.

### 1.2 Verification and graph utilities

`YgorMeshesVerification.h/.cc` can support later public-mesh smoke tests, but its edge-count/public-index checks do not reconstruct one cyclic link per topological vertex, preserve equal-coordinate occurrences, validate lineage, or provide typed transactional evidence. It is not the production verifier for this artifact.

No existing Ygor graph or concurrency utility satisfies the strong-ID, exact-occurrence, deterministic merge, resource, cancellation, replay, and independent-verification contracts. Implement the required small graph machinery in the bounded subsystem with contiguous arrays, sorted complete keys, checked prefix sums, CSR ranges, and iterative traversal. Use Component 01/17 execution services, not `YgorThreadPool::work_queue`.

### 1.3 Existing Boolean implementations and reusable test material

The existing Boolean implementations were reviewed before choosing a greenfield Component 10 provider:

- `YgorMeshesBoolean` is a volumetric grid Boolean. Its classification and grid-aligned output do not preserve source-surface atoms, event lineage, coincident ownership, or topological occurrence identity.
- `YgorMeshesBoolean2` and `YgorMeshesBoolean3` split and classify triangles but use snapping or welding, per-fragment ray classification, and presentation-level coplanar relations. Their selection records cannot satisfy the compute-once side-label, symbolic ownership, or no-coordinate-topology contracts.
- `YgorMeshesBoolean4` delegates to a BSP volume and converts back to `fv_surface_mesh`; the conversion discards the source/event/occurrence evidence required by Components 10, 11, and 15.
- `YgorMeshesBoolean5` is the closest structural precedent, but it keys constructed topology on a fixed snap grid, classifies each arrangement facet through normal/ray queries, and applies operation- and operand-specific selection conditionals including a hard-coded representative operand for coincident sheets. Those decisions conflict with the Component 01 truth-table and symbolic-owner authorities and cannot preserve point-/edge-contact occurrence separation.

Do not link, include, call, adapt, or copy these implementations into the bounded subsystem, and do not treat their output bytes or pass/fail behavior as a normative oracle. It is acceptable and encouraged to port geometry-only fixture builders, operation-case taxonomies, empty/identity cases, exact analytic volume expectations, deterministic input permutations, and adversarial contact examples into `tests/mesh_boolean_bounded/`. Ported fixtures must be re-expressed through the Component 01-09 artifact builders, must use the bounded subsystem's expected retained-topology oracle, and must remain independent of legacy implementation output.

### 1.4 Mandatory predecessor reuse

Use only narrow immutable views:

- Component 01: operation/remapping, truth table, symbolic matrix, owner ranking, strong IDs, checked arithmetic, resources, cancellation, deterministic errors, transactions, codec, SHA-256, diagnostics, replay.
- Component 02: source shell/facet orientation, occupied-side convention, shell semantics, provenance.
- Component 03: inherited bounded-point validation and canonical scalar bits only.
- Component 04: semantic source-facet boundaries, triangle groups, internal-diagonal transparency.
- Component 05: source halfedges, source-edge direction, source-facet incidence, source-vertex fan order, shell membership.
- Component 07: relation/coincidence/contact class, symbolic rule and owner eligibility.
- Component 08: events and distinct occurrences, source-edge intervals, carriers, overlap regions, ordering, aggregates, separation descriptors.
- Component 09: positive-area atoms, ordered semantic boundaries, sectors, side labels, contact/coincidence lineage, occurrence-separation constraints.

A need to run a new geometric predicate is a predecessor-contract defect, not permission to add one here.

## 2. Files, API, versions, and checkpoints

### 2.1 Files

Add under `src/YgorMeshesBooleanBounded/`:

- `SelectionTypes.h` — closed enums, IDs, temporary records, complete keys, versions.
- `RetainedSurfaceComplex.h` — immutable artifact and checked read-only view.
- `Selection.h/.cc` — typed entrypoint and phase orchestration.
- `SelectionTruth.h/.cc` — side tuple construction and Component 01 truth-cell dispatch.
- `SelectionCoincidence.h/.cc` — sheet-cell reconstruction, owner/cancellation/multiplicity resolution.
- `SelectionRetainedUses.h/.cc` — retained-use and provenance construction.
- `SelectionIncidence.h/.cc` — semantic boundary incidences and continuation records.
- `SelectionEdgeOccurrences.h/.cc` — exact mate signatures and two-use edge requirements.
- `SelectionVertexOccurrences.h/.cc` — endpoint ports, local link graph, closed-cycle occurrences.
- `SelectionFeasibility.h/.cc` — complete balance/manifold-feasibility audit.
- `SelectionCodec.h/.cc` — canonical bytes and digests.
- `SelectionVerifier.h/.cc` — independent reconstruction and mutation rejection.

Extend `tests/mesh_boolean_bounded/` with focused tests for truth/dispositions, coincidence/multiplicity, retained uses, continuation, edge occurrences, vertex occurrences, feasibility, known artifacts, algebraic properties, retriangulation, canonicalization/codec, mutation, fuzz/replay, resources/cancellation, and structural performance. Keep exact oracles and corrupt constructors test-only.

### 2.2 Typed entrypoint

Provide an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const retained_surface_complex<T,I>>>
build_retained_surface_complex(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const validated_operands_view<T,I>& validated,
    const source_triangle_complexes_view<T,I>& source_triangles,
    const canonical_source_manifolds_view<T,I>& manifolds,
    const signed_feature_relations_view<T,I>& relations,
    const canonical_intersection_complex_view<T,I>& intersections,
    const classification_complex_view<T,I>& classification,
    const selection_capabilities<T,I>& capabilities);
```

Validate all owner tokens, versions, operations, policy digests, predecessor digests, verification dispositions, IDs, ranges, and counts before authoritative allocation. Support empty results as a valid fully audited artifact. Execute in one stage transaction and publish only after independent verification.

### 2.3 Versions

Add nonzero explicit versions for every provider above and for side tuples, truth evidence, dispositions, sheet cells, ownership/suppression, multiplicity, retained uses, incidences, continuation, edge mate signatures, edge occurrences, local ports/arcs, vertex occurrences, feasibility evidence, artifact schema, codec, and verifier. Zero and unknown required versions fail. Include versions in artifacts, errors, replay, and canonical bytes.

### 2.4 Stable checkpoints

Use the Component 10 stage and fixed checkpoints:

1. context/capability validation;
2. predecessor validation;
3. count/index/byte/work preflight;
4. resource reservation;
5. atom/label coverage validation;
6. side tuple construction;
7. truth evaluation;
8. sheet-cell reconstruction;
9. owner/cancellation/multiplicity resolution;
10. final disposition audit;
11. retained-use construction;
12. incidence normalization;
13. continuation construction;
14. mate-key grouping and edge pairing;
15. local port/corner construction;
16. edge-mate arc construction;
17. local-link cycle extraction;
18. endpoint remap and complete feasibility audit;
19. canonicalization/reverse maps/statistics;
20. producer checks;
21. codec/digests/replay;
22. independent verification;
23. resource reconciliation/final cancellation poll;
24. commit.

Poll cancellation at every checkpoint and bounded intervals in long loops. All workers join before rollback.

## 3. IDs, enums, complete keys, and artifact layout

### 3.1 Strong ID domains

Define separate domains for at least:

- `selection_disposition_id`, `side_tuple_id`, `truth_evidence_id`;
- `coincidence_sheet_cell_id`, `coincidence_member_id`, `owner_decision_id`, `suppression_record_id`, `multiplicity_id`;
- `retained_surface_use_id`, `retained_use_provenance_id`;
- `retained_incidence_id`, `continuation_id`;
- `edge_mate_group_id`, `planned_edge_occurrence_id`, `carrier_balance_id`;
- `endpoint_domain_id`, `local_port_id`, `face_corner_arc_id`, `edge_mate_arc_id`, `local_link_component_id`, `vertex_occurrence_requirement_id`;
- verifier/replay evidence IDs where generic evidence is unsafe.

Do not alias these to predecessor IDs, `I`, `size_t`, or raw offsets.

### 3.2 Closed enums

Use explicit nonzero values for:

- final disposition: retain-preserve, retain-reverse, discard-equal-sides, suppress-internal, suppress-non-owner, cancel-coincident, represented-by-multiplicity, invalid;
- side-state origin: source-shell, numeric classification, boundary-derived, symbolic, coincident-owned;
- sheet relation: ordinary, same-orientation coincidence, opposite-orientation coincidence, partial overlap, occurrence-distinct coincidence;
- incidence disposition: planned edge, transparent continuation, topology-separation delimiter, zero-measure support, consumed owner seam, suppressed audit only;
- carrier kind: whole source edge, source-edge interval, transverse carrier, coplanar overlap boundary, contact delimiter;
- endpoint role, direction role, arc kind, occurrence-separation class, verifier disposition.

Unknown values fail decoding; no default fallthrough.

### 3.3 Complete keys

A disposition key includes context owner namespace, atom semantic key, operation/remapped roles, side-label IDs, truth-table version, symbolic-policy version, and schema. It excludes coordinates, triangle presentation, worker/task IDs, pointers, hashes, and traversal ordinals.

A sheet-cell key includes canonical support/coincidence lineage, overlap-region identity, conceptual side order, occurrence-separation class, sorted semantic member atom keys, and versions. Different source triangulations of the same semantic sheet must produce the same remapped sheet-cell structure.

A retained-use key includes the atom semantic key, final orientation, sheet owner/cancellation class, multiplicity occurrence, semantic boundary signature, and versions.

Define a `surface_occurrence_descriptor` for one directed retained incidence. It contains the retained-use semantic lineage, source operand role, sheet-owner lineage, multiplicity occurrence, oriented result-side transition, source/event sector at both endpoints, and occurrence-separation lineage. This descriptor identifies one face use; it is not an undirected-edge identity.

An undirected edge-occurrence slot key includes:

```text
(carrier semantic identity,
 canonical open interval/span identity,
 unordered endpoint-domain lineage,
 occurrence-separation class,
 canonical output-edge occurrence slot,
 result-transition compatibility class,
 canonical unordered pair of expected surface-occurrence descriptors,
 exact endpoint fan-pair compatibility descriptors,
 schema versions)
```

Each directed incidence stores its own `surface_occurrence_descriptor`, the exact expected opposite descriptor, and a forward/reverse role under the canonical carrier orientation. Grouping requires reciprocal actual/expected descriptors and one shared slot key. The two descriptors may have different source operands, source facets, retained-sheet owners, and per-face multiplicity records. In particular, a proper transverse seam normally pairs one A-owned descriptor with one B-owned descriptor. Same-sheet source-edge pairs and occurrence-distinct coincident pairs are represented by their own canonical descriptor pairs and slot discriminators.

The slot key and descriptors must not include coordinates, nominal parameters, normals, tolerance, hash values, discovery order, or future output IDs. A descriptor pair is canonicalized by the complete descriptor order, not by operand preference or hash order.

A local-port key includes endpoint domain, retained incidence, retained use, endpoint role, source/event sector, sheet/multiplicity occurrence, and separation lineage. Face-corner and edge-mate arcs are keyed by their exact two ports plus semantic reason. A vertex occurrence key is the complete sorted cycle of ports/arcs canonicalized over both cycle directions and all rotations, with the endpoint domain and versions.

Full keys define equality and order. Hashes may accelerate lookup only after full-key comparison and never determine IDs, errors, or bytes.

### 3.4 Immutable artifact

`retained_surface_complex<T,I>` stores a header with owner, operation/roles, all provider/schema versions, predecessor IDs/digests, strict floating profile, counts/ranges, section digests, complete digest, verification disposition, and persistent resource leases.

Use canonical contiguous tables and checked CSR ranges for:

1. one disposition and side/truth evidence record per positive-area atom;
2. sheet cells, members, symbolic decisions, owner ranks, suppression/cancellation, multiplicity;
3. retained uses and full provenance;
4. directed retained incidences and incidence audit dispositions;
5. reciprocal continuation records;
6. edge-occurrence slot groups, expected surface-occurrence descriptor pairs, and planned two-use edge occurrences;
7. endpoint domains, local ports, face-corner arcs, edge-mate arcs, link components, vertex occurrence requirements;
8. carrier/source-edge balance and complete feasibility evidence;
9. forward/reverse maps to atoms, labels, source features, relations, events, intervals, carriers, and occurrences;
10. statistics, resources, diagnostics, replay, verifier evidence, and canonical maps.

Published references point only to immutable predecessor storage and stage-owned immutable buffers whose lifetime covers Components 11-15.

## 4. Side tuples, truth evaluation, and dispositions

For each positive-area atom:

1. Read source operand, shell, facet orientation, and occupied-side convention from Components 02/09.
2. Derive the source operand occupancy on the atom's negative and positive conceptual sides. Under V1 regular-solid semantics the values must be complementary for a valid oriented boundary atom.
3. Read the opposite operand's negative/positive side states from the Component 09 label. Accept only states explicitly eligible for Component 10.
4. Remap source/opposite roles to canonical A/B positions.
5. Build the four-bit `side_occupancy` tuple and preserve the origin/evidence of each bit.
6. Call Component 01's frozen operation truth-table service once.
7. Store rule ID/version, input bits, output occupancy on each side, retain/discard, preserve/reverse, and multiplicity response.

Required rule:

```text
result_negative == result_positive  => not a regularized boundary
result_negative == occupied and result_positive == empty => preserve
result_negative == empty and result_positive == occupied => reverse
```

Do not infer orientation from normals or operand name. Missing, unresolved, contradictory, or unsupported side states are typed failures, never default discard.

Every atom receives exactly one preliminary and one final disposition record. Discard/suppression records are permanent verification evidence. Independently verify the complete atom ID set, not merely counts.

## 5. Coincident/coplanar sheet resolution and multiplicity

### 5.1 Sheet-cell reconstruction

After individual truth evaluation, group only atoms that Components 07-09 prove belong to the same semantic coincident/overlap sheet cell. Reconstruct partial coplanar overlap region-locally from overlap IDs, boundary incidence, symbolic side order, source-facet semantic lineage, and occurrence descriptors. Never group by plane or coordinate equality.

For each sheet cell, require:

- complete member coverage with no atom in two cells;
- compatible support/overlap lineage;
- explicit same/opposite orientation relation;
- complete conceptual side occupancy and symbolic rule references;
- separation of topologically distinct coincident occurrences;
- independence from Component 04 internal triangulation diagonals.

### 5.2 Joint decision

Classify each cell as one of:

- no result transition: suppress all as internal/cancelled;
- one result boundary sheet: select exactly one canonical owner;
- several topology-distinct required sheets: create explicit multiplicity occurrences, each with exactly one owner;
- invalid contradiction.

Owner eligibility is established by Component 07 symbolic policy. Rank eligible candidates with Component 01's frozen order: ability to realize required final orientation, operation-specific operand priority, symbolic feature-dimension priority, full canonical source-feature key, and directed occurrence discriminator. Do not invent another rank. Require a unique minimum; retain all rank evidence.

Handle the complete matrix for union, intersection, both differences, and xor across equal operands, same/opposite orientation coincidence, partial overlap, shared internal faces, cavity/outer-shell coincidence, zero-volume face contact, and occurrence-distinct coincident shells. Equal operands must follow Component 01 policy exactly.

Non-owner atoms are suppressed but their source/caller provenance is attached to the owner/suppression ledger. Internal two-sided sheets never produce retained uses. Multiplicity is an integer checked domain; V1 ordinary regular-solid output normally requires one occurrence per selected geometric sheet, but representation and tests must support several topology-distinct occurrences.

## 6. Retained uses, incidences, and continuation

### 6.1 Retained uses

Create one retained use for each selected atom occurrence after ownership/multiplicity resolution. Record source operand/shell/facet/group/atom/caller provenance, prescribed output orientation, result occupancy on both sides, sheet owner, multiplicity occurrence, semantic boundary-use range, permitted continuation range, and stable key.

A selected atom cannot be absent or duplicated. A discarded/suppressed atom cannot own a retained use. Alternative source triangulation may change bookkeeping evidence but not semantic retained topology.

### 6.2 Directed boundary incidences

For every oriented semantic boundary use of every retained atom, create exactly one directed incidence. Map retained orientation onto the predecessor boundary direction, including reversed retained uses. Each incidence records:

- retained use and source atom;
- canonical start/end endpoint domains;
- source edge/interval, carrier/span, overlap boundary, or contact lineage;
- canonical carrier direction and local forward/reverse role;
- source/event sectors at both endpoints;
- owner/multiplicity/separation class;
- its `surface_occurrence_descriptor` and exact expected opposite descriptor when it requires a planned output edge;
- adjacent retained-use candidates and predecessor descriptors;
- final incidence disposition.

No semantic boundary may be omitted. Component 04 internal diagonals are absent unless Component 08/09 proves a semantic delimiter occupies them; bookkeeping diagonals otherwise cancel before this stage.

### 6.3 Continuation

Emit reciprocal continuation only when Components 09/10 prove two retained atom boundaries are the same transparent seam and all of these agree:

- output orientation and result-side transition;
- sheet owner and multiplicity occurrence;
- exact seam lineage and opposite directions;
- endpoint occurrence lineage;
- local sector continuation permission;
- no selected carrier, point/edge-contact separation, symbolic delimiter, or incompatible coincidence boundary.

Sort continuation proposals by complete undirected seam/member key, require exactly the documented reciprocal members, and publish both directions. Continuation is prohibited across selected intersection carriers, topology-separated contacts, different owners/multiplicities, or different local result fans. Coordinate coplanarity/proximity is irrelevant.

Every retained incidence ends in exactly one of: transparent continuation, planned edge, explicit zero-measure downstream support, owner-seam consumption, or audit-only suppression. No unresolved disposition may publish.

## 7. Exact planned edge occurrences

### 7.1 Slot construction and reciprocal mate grouping

For every incidence requiring an output edge, construct its complete `surface_occurrence_descriptor`, expected opposite descriptor, shared undirected edge-occurrence slot key, and directed role. Derive expected pairs from authoritative Component 05/08/09 carrier, adjacency, sector, coincidence, multiplicity, and occurrence-separation lineage before grouping. Canonically sort by slot key and descriptor. A valid V1 slot contains exactly two members:

- one forward and one reverse under the carrier orientation;
- matching unordered endpoint-domain lineage;
- matching carrier interval/span, occurrence-separation class, and output-edge slot discriminator;
- mutually reciprocal actual and expected `surface_occurrence_descriptor` values;
- a canonical unordered descriptor pair equal to the slot's expected pair;
- compatible result-side transitions; and
- compatible local sectors and endpoint fan-pair descriptors at both endpoints.

Do **not** require the two members to have the same source operand, source facet, retained-sheet owner, or per-face multiplicity record. Proper transverse intersection edges normally pair one retained use from A with one retained use from B. A source-edge boundary may pair two uses from one source sheet, while coincident occurrence multiplicity may create several separate descriptor-pair slots on the same geometric carrier. These cases differ by complete lineage and slot identity, never by coordinates.

Create one `planned_edge_occurrence` only after all checks pass. Store the slot key, the two directed members in canonical order, both actual/expected descriptors, expected opposite endpoints, carrier/source provenance, endpoint sectors, multiplicity/separation evidence, and pair-feasibility evidence.

Cardinality 0/1/>2, two same-direction members, incompatible endpoints, a non-reciprocal descriptor pair, an incorrect slot discriminator, or several possible pairings is failure. Do not invoke bipartite/general/minimum-cost matching. When four uses occupy one geometric carrier, their authoritative lineage and expected descriptor pairs must partition them into two distinct two-member slots; otherwise fail rather than guess.

Zero nominal length and equal endpoint coordinates do not erase an edge occurrence. Topological endpoint identity remains authoritative.

### 7.2 Balance

For each source-edge sequence and carrier chain, publish sorted membership and checked start/end accounting. Verify every planned member is consumed once, every required interval is represented, directions balance on closed chains, open local chains close through documented source-edge transitions, and equal-parameter clusters preserve all occurrence identities. Counts supplement but never replace complete member-set and expected-pair comparison.

## 8. Vertex/event occurrences by local link cycles

This phase resolves which retained incidence endpoints may share one future Component 11 output vertex without using coordinates and without circular dependence on pre-existing output vertices.

### 8.1 Endpoint domains and ports

Create endpoint domains from authoritative source vertices, Component 08 event occurrences/clusters, source-edge interval endpoints, carrier endpoints, and Component 09 separation descriptors. One geometric event may have several endpoint domains when topology requires distinct occurrences.

For every endpoint of every planned-edge incidence, create one `local_port` identified by endpoint domain, incidence, retained use, endpoint role, source/event sector, sheet/multiplicity occurrence, and separation lineage. Each port must participate in exactly one face-corner arc and one edge-mate arc.

### 8.2 Face-corner arcs

Use each retained atom's authoritative oriented boundary order. At each boundary vertex, connect the incoming incidence port to the outgoing incidence port of the same retained surface sector. Transparent seams must already have been removed/consumed consistently; selected delimiters remain corners. Preserve zero-length and repeated-coordinate corners as distinct ports when lineage differs.

Reject open atom boundary order, duplicate corner consumption, incompatible endpoint domains, or a corner crossing a prohibited sector boundary.

### 8.3 Edge-mate arcs

For each planned edge and each of its two endpoint domains, connect the endpoint port of one directed member to the corresponding endpoint port of its exact reciprocal descriptor member. Validate reversed topological endpoints and compatible local sector evidence. Each planned edge creates exactly two endpoint mate arcs.

### 8.4 Degree-two graph and cycle extraction

Within each endpoint domain, build an undirected bipartite-by-arc-kind link graph over ports:

- every port has degree exactly two;
- exactly one incident arc is a face-corner arc;
- exactly one incident arc is an edge-mate arc;
- no arc crosses a Component 08/09 separation, sheet occurrence, or multiplicity boundary.

Canonicalize adjacency, then traverse every unvisited least-key port until it returns to the start. A valid connected component is one simple closed alternating cycle consuming each member port once. An open path, branch, repeated port before closure, disconnected member set, or multiple cycles proposed as one occurrence is failure.

Publish one `vertex_occurrence_requirement` per closed cycle. Canonicalize the cycle over all rotations and both directions, choosing the least complete encoding. Distinct cycles remain distinct future vertices even when their bounded coordinate references are bit-identical. This is the required behavior for point-touching solids, edge-touching solids, bow-ties, coincident sheets, and high-valence events.

After cycles are assigned, remap each planned edge endpoint to exactly one vertex occurrence. The two edge members must map to reversed occurrence endpoints. Component 11 must receive no unresolved endpoint choice.

## 9. Complete feasibility audit and output contract

Before publication, prove:

- the atom disposition IDs equal the complete positive-area atom IDs;
- every retained use corresponds to exactly one selected atom occurrence;
- every retained atom boundary has one incidence audit record;
- every incidence is consumed exactly once by continuation, a planned edge, or an explicitly permitted audit/zero-measure disposition;
- every continuation is reciprocal and exact-lineage compatible;
- every planned edge has exactly two opposite directed uses whose actual/expected surface-occurrence descriptors are reciprocal and whose canonical descriptor pair matches one authoritative edge-occurrence slot;
- valid cross-operand and cross-owner pairs are preserved rather than rejected by an owner-equality assumption;
- every planned edge endpoint maps to one vertex occurrence;
- every local port has one corner and one mate arc;
- every vertex occurrence is one closed link cycle;
- carrier/source-edge member sets, expected descriptor pairs, and start/end balances are complete;
- no point/edge-contact or coincident occurrence separation was crossed;
- no coordinate/tolerance/normal/hash field affected identity or grouping;
- all reverse maps, dense IDs, CSR ranges, resources, and digests reconstruct exactly.

On success, publish the immutable artifact with complete dispositions, retained uses, coincidence/ownership/multiplicity evidence, incidences, continuation, planned edge requirements and descriptor-pair evidence, vertex/event fan requirements, balance evidence, provenance, resources, diagnostics, replay, and verifier disposition.

Component 11 receives checked iteration/access for retained uses, incidences, continuation, exact two-member edge occurrences, reciprocal surface-descriptor pairs, vertex occurrence cycles, coordinate-source references, and reverse mappings. It receives no permission to repeat selection, infer sharing from coordinates, choose a mate, merge cycles, or repair balance.

A valid empty Boolean result publishes an empty retained-use/occurrence domain with complete discard/suppression evidence and deterministic digest.

## 10. Determinism, concurrency, resources, encoding, and verification

### 10.1 Deterministic execution

Permitted parallel phases are per-atom side/truth proposals, per-sheet candidate preparation, retained-use/incidence proposal generation, independent edge-slot/descriptor creation, and independent endpoint-domain link construction. Tasks read immutable inputs and write private deterministic slices. Canonical merge uses full-key sort, exact duplicate reconciliation, dense remapping, and Component 01 failure arbitration.

Worker count, task delay, union roots, hash collisions, allocation order, queue order, discovery order, endpoint traversal start, and input triangle order must not affect semantic records or bytes. Every worker establishes the strict floating environment and all work joins before commit/rollback.

### 10.2 Resource accounting

Add distinct resource kinds/subkinds for dispositions, side/truth evidence, sheet cells/members, owner candidates/decisions, suppression/cancellation, multiplicity, retained uses/provenance, incidences, continuation, edge slots/descriptor pairs/planned edges, endpoint domains, ports/arcs/link components/vertex occurrences, balance/feasibility/verifier records, canonical sort records, diagnostics/replay, temporary/persistent bytes, and work units.

Preflight with checked arithmetic; reserve before allocation; reconcile exact use deterministically; transfer only verified persistent leases. `resource_limit` must never trigger merged occurrences, lost evidence, skipped verification, or heuristic pairing. Representability failures are `index_overflow`.

### 10.3 Canonical encoding and replay

Encode fixed-width fields in this order:

1. magic/schema/owner/operation/roles;
2. provider/policy/version registry and predecessor IDs/digests;
3. dispositions and side/truth evidence;
4. sheet cells, ownership, suppression, cancellation, multiplicity;
5. retained uses/provenance;
6. incidences and continuation;
7. edge slots, surface-occurrence descriptor pairs, planned edges, and balance;
8. endpoint domains, ports/arcs/link cycles/vertex occurrences;
9. feasibility/statistics/resources/verifier disposition;
10. section and complete digests.

Use Component 03 canonical scalar bits for referenced numeric metadata. Never serialize raw memory, capacity, `size_t`, pointers, task IDs, locale text, or unordered iteration. Replay retains exact policy/predecessor identity and the least canonical witnesses needed to reproduce every decision/failure.

### 10.4 Independent verifier

Implement `SelectionVerifier` in a separate source module. It must not call producer orchestration, producer sheet grouping, owner chooser, edge-slot grouping, local-link traversal, or producer codec helpers as its sole proof.

The verifier independently:

- validates owners, versions, IDs, ranges, resources, and predecessor digests;
- enumerates positive-area atoms and rebuilds side tuples;
- recomputes all truth cells from the frozen table bytes;
- reconstructs sheet cells from Component 07-09 lineage with an alternate sort/scan order;
- independently checks owner eligibility/rank, cancellation, suppression, and multiplicity;
- reconstructs retained uses and complete incidence membership;
- rebuilds reciprocal continuation;
- derives expected surface-occurrence descriptors and output-edge slot discriminators independently from Component 05/08/09 lineage;
- groups edge members by independently encoded full slot keys, requires exact reciprocal descriptor pairs, and explicitly permits valid pairs with different operands or sheet owners;
- rebuilds endpoint ports, corners, mate arcs, and closed cycles using an alternate traversal order;
- verifies endpoint remaps and all balance/member-set constraints;
- scans keys/evidence for forbidden coordinate-derived identity;
- independently encodes semantic sections and recomputes digests/resources.

Producer/verifier disagreement is `internal_invariant_error` and prevents commit. Provide test-only corrupt constructors unavailable in production.

## 11. Failures and diagnostics

Allocate stable Component 10 subcodes covering unsupported versions; wrong owner/operation/role/domain; predecessor digest/verification mismatch; overflow/limit; missing/duplicate atom or label; invalid side tuple/remap/truth cell; wrong retain/orientation; malformed sheet-cell membership; wrong/multiple/no owner; internal sheet retained; required sheet suppressed; multiplicity corruption; retained-use/provenance mismatch; incidence endpoint/direction/carrier error; internal diagonal leakage; invalid/nonreciprocal continuation; malformed edge-slot or surface-occurrence descriptor; mate cardinality/direction/endpoint incompatibility; non-reciprocal expected mate; descriptor-pair or slot-discriminator mismatch; erroneous same-owner requirement; incidence under/over-consumption; malformed endpoint domain/port/corner/mate arc; open/branched/multicycle link; separation crossing; endpoint remap mismatch; balance failure; canonical/range/reverse-map/codec/digest error; verifier rejection; resource reconciliation; cancellation; and internal construction contradiction.

Errors include the least canonical operation, atom, side tuple, truth rule, relation/symbolic decision, sheet cell, retained use, incidence, interval/carrier, edge slot, actual/expected surface-occurrence descriptors, endpoint domain, port/arc, planned edge, and vertex occurrence witnesses; expected/actual values; provider/policy versions; resource counters; checkpoint; and replay identity.

Use `geometric_condition_exceeds_tolerance` only when a valid predecessor explicitly marks evidence insufficient for ordinary selection. Use `index_overflow`, `resource_limit`, and `cancelled` for their respective cases. Contradictory committed predecessor evidence, impossible pair/fan topology, or producer/verifier disagreement is `internal_invariant_error`.

## 12. Test and qualification plan

### 12.1 Truth and orientation

Test all 80 Component 01 operation/side cells, both source operand roles, source occupied-side conventions, numeric/boundary/symbolic/coincident origins, preserve/reverse, and invalid wider states. Assert one lookup per atom and independent table recomputation.

### 12.2 Known-answer operation matrix

Commit canonical artifacts for empty, disjoint, containment, proper overlap, equal operands, vertex/edge/face touching, tangency, partial coplanar overlap, same/opposite coincident shells, cavities/islands, disconnected components, and topology-distinct coincident shells. Run union, intersection, both differences, and xor in both operand presentations where applicable.

### 12.3 Coincidence and multiplicity

Cover identical and radically different triangulations, subdivided facets, partial overlap through vertices/edges, same/opposite orientation, shared internal faces, zero-volume face contact, duplicate shells, and several coincident occurrences. Verify one owner when one sheet is required, none when cancelled, explicit separate occurrences when required, and no coordinate grouping.

### 12.4 Incidence, continuation, and edges

Test whole/split source edges, transverse loops, coplanar boundaries, equal-parameter clusters, zero-nominal-length intervals, transparent atom seams, internal diagonals, topology-separated contacts, and four geometric uses partitioning into two lineage pairs. Include at least:

- a proper transverse overlap whose output seam pairs an A-owned retained use with a B-owned retained use;
- a source-edge pair whose two uses belong to one retained source sheet;
- coincident multiplicity producing several separate descriptor-pair slots on the same geometric carrier;
- an operand-swapped/remapped fixture that preserves the canonical pair after documented remapping; and
- mutations that force owner equality, swap one expected opposite descriptor, reuse one descriptor in two slots, or erase the slot discriminator.

Exhaustively enumerate small labeled pairing fixtures and require either the unique reciprocal descriptor-pair partition or deterministic failure; never accept arbitrary matching.

### 12.5 Vertex occurrence/link tests

Cover one ordinary fan, high-valence transverse events, point-touching components sharing coordinates, edge-touching components, bow-ties requiring two cycles, coincident sheets, repeated-coordinate corners, and distinct events with identical nominal points. Inject open paths, branches, duplicate/missing arcs, crossed separation, merged cycles, and inconsistent paired-edge endpoint cycles; all must fail independently.

### 12.6 Algebraic and metamorphic properties

Verify commutativity for union/intersection/xor, directed difference remapping, idempotence, `A-A` emptiness, identity with empty, absorption, equal-operand policy, and repeated evaluation stability. Apply legal source-facet/edge subdivision, alternative triangulation, ring rotation, component/feature permutations, operand exchange/remapping, corrected global orientation reversal, axis permutation, sign flip, exact translation, power-of-two scaling, thread counts 1/2/max, forced delays, reversed grouping/traversal, and forced hash collisions. Require equivalent semantic topology and byte-identical canonical artifacts after documented remapping.

### 12.7 Mutation verification

Corrupt each major fact independently: remove/duplicate disposition; flip side bit/truth result/orientation; choose wrong owner; retain internal/non-owner sheet; suppress required owner; alter multiplicity; add/remove retained use; lose provenance; change incidence endpoint/direction/carrier; leak internal diagonal; add/remove continuation; alter edge-slot key/member/direction; alter actual or expected surface-occurrence descriptor; force a valid cross-owner pair to fail; pair one member with a non-reciprocal descriptor; assign three uses to one edge; omit/double-consume incidence; alter port/corner/mate arc; merge/split cycles incorrectly; cross separation; alter endpoint remap/balance; scramble canonical order; forge counts/resources/digests. Repair cached counts/digests where possible. The verifier must reject every mutation.

### 12.8 Fuzzing, resources, cancellation, and performance

Generate valid exact-template classification artifacts varying operation, shell nesting, contact dimension, coincidence count/orientation, source subdivision/triangulation, event valence, duplicate coordinates, multiplicity, symbolic rules, and limits. Structure-shrink every crash, nondeterminism, wrong owner, accidental weld, cross-owner pair rejection, ambiguous pair acceptance, invalid fan, or verifier disagreement while preserving exact replay.

For every resource kind test limit-minus-one, limit, and limit-plus-one. Cancel at every checkpoint and inside long sorts/traversals. Run GCC/Clang Debug/Release, `float`/`double`, `uint32_t`/`uint64_t`, ASan/UBSan, and TSan for parallel paths.

Track structural counters and require:

- exactly one truth lookup per positive-area atom;
- zero geometric predicate/shell-query calls from Component 10;
- grouping via sort plus linear scan;
- edge work `O(E log E)` with no quadratic/general matching;
- local link construction/cycle traversal `O(P log P + P)`;
- no unconditional all-pairs atom/incidence/sector work;
- scalable verifier with the same asymptotic bounds.

Maintain a manifest mapping every normative Component 10 clause to named tests and verifier checks. CI fails for an unmapped clause or surviving required mutation.

## 13. Implementation sequence and definition of done

Implement in this order, with a gate after each step:

1. schemas/versions/IDs/enums/subcodes/build targets;
2. predecessor views and preflight;
3. side tuples and truth dispositions;
4. sheet-cell reconstruction;
5. owner/suppression/cancellation/multiplicity;
6. retained uses/provenance;
7. incidence normalization;
8. reciprocal continuation;
9. exact edge-slot keys, reciprocal surface-occurrence descriptors, and two-use edge occurrences;
10. endpoint domains, ports, and corners;
11. mate arcs and closed local-link cycles;
12. endpoint remap, balance, and complete feasibility;
13. immutable artifact/codec/digests/verifier;
14. transactions/cancellation/concurrency/replay/performance;
15. complete qualification and traceability.

Component 10 is complete only when:

- every predecessor owner/version/digest/reference is checked;
- all count/index/range/byte/work/multiplicity arithmetic is checked and resource-accounted;
- every positive-area atom has one independently reconstructible final disposition;
- source and opposite side occupancy come only from Components 02 and 09;
- the frozen truth table is called exactly once per atom and determines retention/orientation exactly;
- all coincident/partial-overlap semantics are total for every operation;
- each required sheet occurrence has exactly one canonical eligible owner;
- internal/non-owner sheets are suppressed with full evidence;
- multiplicity is explicit and never represented by non-manifold incidence;
- every selected atom occurrence has one retained use with complete provenance;
- every retained semantic boundary has one directed incidence and one final consumption disposition;
- continuation is reciprocal, exact-lineage-defined, and separation-safe;
- internal triangulation diagonals never affect selection topology;
- every planned edge has exactly two opposite full-slot-compatible uses with mutually reciprocal actual/expected surface-occurrence descriptors;
- valid paired members may have different source operands or retained-sheet owners, while every ambiguity in the expected pair fails closed;
- no heuristic, coordinate, tolerance, normal, hash, source-owner equality, or schedule chooses a mate;
- every endpoint port has one corner arc and one mate arc;
- every vertex/event occurrence is exactly one closed local-link cycle;
- distinct cycles at one coordinate remain distinct and no bow-tie is planned;
- point/edge touching and coincident occurrence separation is preserved without geometric gaps;
- paired edge members map to reversed final vertex-occurrence endpoints;
- the immutable artifact is canonical, transactional, independently verifiable, and sufficient for Component 11 without reclassification or repair;
- exhaustive small oracles and mutation tests reject ambiguity/corruption;
- cancellation/resource failure publishes nothing and never simplifies topology;
- artifacts/errors/replay are stable across all required permutations and schedules;
- all unit, known-answer, algebraic, metamorphic, adversarial, oracle, mutation, fuzz/shrink, resource, cancellation, sanitizer, compiler/type-matrix, and structural-performance tests pass;
- every normative clause maps to executable evidence; and
- production and normative tests remain dependency-free strict C++17 and use no excluded legacy Boolean implementation.
