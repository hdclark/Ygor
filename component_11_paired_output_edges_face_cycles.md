# Component 11: Paired Output Edges and Face-Cycle Construction

## Status and normative language

This document specifies a required component of the dependency-free bounded floating-point surface-mesh Boolean engine described by `broad_plan.md`.

Production and normative-test code must:

- be strict portable C++17;
- use only Ygor and the C++17 standard library;
- execute topology-affecting arithmetic under the frozen floating-point environment from Components 01 and 03;
- preserve exact indexed topology independently of coordinate equality; and
- publish no artifact until producer checks and an independent verifier both succeed.

The concrete storage layout, graph representation, cycle-walk implementation, contour representation, and serial/parallel work decomposition may change. The contracts in this document are normative.

## 0. Purpose and scope

Component 11 realizes the selected and occurrence-partitioned result surface from Component 10 as an immutable polygonal two-manifold skeleton.

It must:

1. materialize exactly one output vertex occurrence for each authoritative, nonempty Component 10 vertex occurrence requirement;
2. attach each output occurrence to one already-authoritative bounded coordinate record without recomputation;
3. realize every planned source-edge or intersection-carrier occurrence as one atomic pair of opposite directed halfedges;
4. preserve whole source edges, split source-edge intervals, transverse carriers, coplanar-overlap boundaries, contact-separation boundaries, and topology-distinct zero-nominal-length intervals;
5. merge retained atoms only across exact transparent-continuation records supplied by Component 10;
6. construct deterministic oriented boundary cycles for every retained polygonal face region;
7. represent holes, repeated coordinates, separate coincident occurrences, and deferred zero-measure topology without coordinate-based welding;
8. prove complete incidence, edge, cycle, region, and vertex-link balance; and
9. provide Component 12 with a verified immutable `polygonal_output_complex<T,I>` that requires no Boolean reclassification, edge repair, or cycle reconstruction.

Component 11 realizes topology. It must not:

- repeat operation truth-table evaluation or winding classification;
- recompute relation predicates or intersection coordinates;
- choose a different event or carrier order;
- alter a Component 10 occurrence partition or planned mate;
- move, snap, average, project back, weld, collapse, or delete output coordinates;
- triangulate polygonal regions;
- spend cleanup budget;
- merge coplanar result faces merely to reduce face count;
- assemble `fv_surface_mesh<T,I>`; or
- publish an ordinary Boolean success.

The principal output is an immutable `polygonal_output_complex<T,I>`.

## 1. Required inputs

The component must accept immutable checked views of:

- the `retained_surface_complex<T,I>` from Component 10;
- the `canonical_intersection_complex<T,I>` from Component 08;
- the `signed_feature_relations<T,I>` from Component 07 when relation provenance is required for an audit;
- the canonical source manifolds from Component 05;
- source-facet semantic groups, source-boundary identity, support-frame lineage, and internal-diagonal provenance from Components 04 and 05;
- validated shell orientation and occupied-side semantics from Component 02;
- the precision context, bounded point/parameter/residual/area/segment services, precision ledger, and read-only tolerance policy from Component 03;
- the Boolean context, operation and operand-role mapping, strong-ID domains, canonical comparators, checked arithmetic, resource policy, cancellation, deterministic failure arbitration, diagnostics, replay, and stage transactions from Component 01;
- Component 17 deterministic execution services when available, while retaining a serial semantic reference; and
- explicit nonzero provider, schema, key, artifact, codec, replay, and verifier versions.

The component must not read mutable caller meshes or mutable predecessor builders.

## 2. Required predecessor guarantees

Component 11 may rely on Component 10 having established all of the following, but it must defensively verify them before construction:

- one audited disposition for every positive-area classification atom;
- the complete set of retained oriented surface uses;
- exact boundary incidence records in authoritative retained-use order;
- reciprocal transparent-continuation records;
- explicit prohibited-continuation and topology-separation records;
- one canonical planned edge occurrence for every boundary edge that must exist in the polygonal result;
- exactly two compatible opposite-directed retained boundary incidences for each planned edge occurrence;
- one canonical vertex occurrence requirement for every future output vertex, with one closed local link cycle and complete endpoint-port membership;
- every planned edge endpoint already mapped to one vertex occurrence requirement;
- authoritative source-edge and carrier interval identities and order, including equal-parameter tie keys;
- explicit multiplicity and occurrence separation for point-, edge-, and coincident-sheet contacts;
- one authoritative bounded coordinate source for every vertex occurrence requirement;
- complete source, event, carrier, shell, facet, atom, retained-use, and caller provenance;
- complete pair-feasibility and start/end balance evidence; and
- deterministic canonical keys and digests.

A contradiction in a committed predecessor artifact is an `internal_invariant_error`. Component 11 must not repair it by searching for another mate, merging fan cycles, deleting incidences, duplicating uses, or using geometry to invent adjacency.

## 3. V1 incidence and zero-measure contract

### 3.1 Exhaustive incidence dispositions

Every Component 10 retained incidence must appear exactly once in the Component 11 audit and have exactly one of these dispositions:

1. **paired boundary use** — one directed member of a planned output edge;
2. **paired zero-measure boundary use** — one directed member of a planned output edge whose nominal or bounded geometry is zero-measure, but whose topology must remain in a face cycle;
3. **transparent internal continuation** — one direction of a reciprocal seam consumed while forming a face region;
4. **consumed owner seam** — an exact coincidence-partition seam consumed by the canonical owner;
5. **support-only zero-measure evidence** — a non-boundary audit/obligation record that produces no output edge, halfedge, cycle member, local endpoint port, or output vertex;
6. **suppressed audit-only evidence** — a non-owner or otherwise suppressed record retained only for verification; or
7. invalid/unresolved.

No incidence may be silently absent, multiply classified, or both realized and suppressed.

### 3.2 Boundary versus support-only zero measure

The following distinction is mandatory:

- A zero-measure item that appears in a polygonal face boundary or face cycle is a **paired zero-measure boundary use**. It must belong to a Component 10 planned edge occurrence with exactly two opposite directed uses. It creates one paired output edge and two halfedges, participates in cycle and vertex-link accounting, and carries a downstream Component 12/13 obligation.
- A **support-only zero-measure evidence** record is not part of the polygonal boundary. It creates no output topology. It must identify its retained use or region attachment, exact lineage, reason for preservation, bounded evidence, and downstream diagnostic or cleanup obligation.

A support-only record must not:

- create a dangling output vertex;
- connect two positive-area regions;
- alter a continuation component;
- appear in a region boundary multiset;
- appear in a successor/predecessor permutation;
- appear in a face cycle; or
- satisfy a missing planned edge member.

If a future version needs an isolated topological support occurrence, it requires a separate nonzero schema and downstream contract. V1 does not encode isolated support as an ordinary output vertex or halfedge.

### 3.3 Realized-boundary equations

Let `E` be the number of planned edge occurrences and `H = 2 * E` with checked arithmetic. Then:

- the set of paired boundary uses, including paired zero-measure boundary uses, contains exactly `H` canonical incidence identities;
- each such incidence maps bijectively to one output halfedge;
- support-only and audit-only records map to no halfedge;
- total cycle-halfedge references equal `H`; and
- every output halfedge belongs to exactly one face cycle.

These are member-set equalities, not count-only checks.

## 4. Input validation and capacity preflight

Before allocation, validate:

- context owner, operation, operand roles, policy versions, provider versions, and artifact versions;
- predecessor verification dispositions and complete digests;
- strong-ID domains, checked ranges, CSR offsets, sortedness promises, and duplicate constraints;
- every source/event/carrier/interval/retained-use/incidence/continuation/planned-edge/local-port/occurrence reference;
- retained orientation against Component 02 and Component 10 evidence;
- coordinate-source identity and finite bounded-point validity;
- exact planned-edge cardinality, direction, endpoint, fan, interval, multiplicity, and separation compatibility;
- every vertex occurrence requirement is one nonempty closed port cycle and is referenced by at least one planned-edge endpoint in V1;
- every transparent continuation is reciprocal and permitted;
- every support-only record has no endpoint port or planned-edge membership and has a complete attachment/obligation certificate; and
- all required downstream internal ID domains are representable.

Preflight with overflow-safe arithmetic and explicit resource reservations for:

- incidence audit rows and reverse maps;
- output vertex occurrences and coordinate references;
- continuation graph storage and region membership;
- boundary darts and transparent-walk evidence;
- paired edges and exactly two halfedges per edge;
- successor/predecessor tables;
- cycles and cycle references;
- contour and nesting records;
- zero-measure descriptors and support-only evidence;
- carrier/source-edge balance evidence;
- bounded admissibility evidence;
- vertex-link evidence;
- canonical sort/remap buffers;
- producer and verifier work;
- diagnostics and replay; and
- persistent artifact bytes.

Representability failure is `index_overflow`. A configured capacity failure is `resource_limit`. Neither may cause reduced evidence or partial topology.

## 5. Required behavior

### 5.1 Canonical retained-incidence audit

Build one canonical audit row for every Component 10 retained incidence. Each row records:

- complete incidence key and disposition;
- retained-use and source atom/facet lineage;
- oriented start/end occurrence requirements when the disposition is realized;
- local ports and fan sectors when realized;
- source-edge, carrier, transparent-seam, coincidence-seam, or support lineage;
- multiplicity and occurrence-separation class;
- expected planned edge and mate, reciprocal continuation, or support attachment;
- consuming Component 11 entity, or an explicit no-entity reason; and
- deterministic verification and replay evidence.

Require exact coverage of all predecessor incidence identities. Every realized boundary incidence belongs to exactly one planned edge. Every transparent incidence belongs to exactly one reciprocal continuation pair. Support-only, consumed, and suppressed rows belong to neither.

### 5.2 Canonical output vertex occurrence materialization

Sort Component 10 vertex occurrence requirements by complete key and assign dense Component 11 IDs only after validating the full set.

For each requirement:

- require a nonempty closed alternating local-port cycle in V1;
- require every member port to be an endpoint of a planned edge incidence;
- resolve exactly one authoritative source-vertex, canonical-event, or explicitly versioned predecessor construction identity;
- reference the existing bounded point and precision-ledger entry without recomputation;
- preserve nominal bit patterns, axis/radial bounds, provenance, multiplicity, coincidence, and separation class;
- store the expected cyclic fan membership and endpoint reverse-map range; and
- map every realized incidence endpoint/local port to exactly this occurrence.

Equal or overlapping coordinates do not merge occurrences. One occurrence requirement is not split because several faces use it.

### 5.3 Face-region formation

Construct a graph whose vertices are retained surface uses and whose edges are exact reciprocal transparent-continuation pairs.

A continuation edge is valid only when:

- both directed continuation records are present and reciprocal;
- seam lineage and endpoint occurrence requirements match exactly;
- retained orientations and result-side transitions are compatible;
- support semantic identity and frame lineage are compatible;
- sheet owner, multiplicity occurrence, and separation class agree;
- neither side is a selected carrier, paired boundary use, point/edge-contact separator, incompatible coincident boundary, support-only record, or suppressed record; and
- all owner and range references are valid.

Connected classes form provisional regions. Positive-area final region splitting must then use authoritative retained-sector continuation evidence so that zero-measure-only connectors cannot join unrelated positive-area interiors. This split completes before paired-edge, halfedge, and cycle IDs are frozen.

A provider may use checked CSR traversal or union-find internally, but final region identity is based on sorted complete member sets and split evidence, never roots or discovery order.

### 5.4 Exact boundary extraction

For each final face region, derive the directed boundary multiset from retained incidence dispositions:

- paired boundary and paired zero-measure boundary uses remain on the boundary;
- transparent internal incidences are consumed through exact reciprocal seam traversal;
- consumed owner seams, support-only records, and suppressed records produce no boundary dart;
- multiplicity-distinct or occurrence-distinct incidences never cancel; and
- cancellation occurs only through an exact transparent-continuation identity.

Record a complete cancellation and transparent-walk ledger. Projected overlap, coordinate equality, approximate collinearity, and nominal opposite direction are never cancellation criteria.

The final region boundary multiset must equal the exact set of planned-edge directed members assigned to the region.

### 5.5 Atomic pair-at-creation edge realization

For each Component 10 planned edge occurrence, validate both directed members before materialization:

- exactly two distinct retained incidence identities;
- one forward and one reverse role under authoritative carrier orientation;
- exact reversed output occurrence endpoints;
- compatible endpoint ports and fan sectors;
- one compatible source-edge/carrier interval identity;
- reciprocal actual/expected surface-occurrence descriptors;
- compatible result-side transitions;
- compatible owner, multiplicity, and separation evidence as defined by the planned slot; and
- explicit zero-measure or loop permission when applicable.

Then allocate one private pair record and initialize from already validated endpoints `v0` and `v1`:

```text
h0.origin      = v0
h0.destination = v1
h1.origin      = v1
h1.destination = v0
h0.pair        = h1.id
h1.pair        = h0.id
```

Only after all fields are populated must the builder assert reciprocal pair IDs and reversed endpoints. No field may be initialized from an unresolved field in the opposite halfedge. No single halfedge may enter a proposed immutable artifact.

The pair retains incident region, retained-use/incidence identity, endpoint port/fan references, source/carrier role, interval/event lineage, multiplicity, separation, length bounds, and replay evidence.

### 5.6 Whole, split, carrier, coplanar, and zero-length roles

Materialize exactly the edge occurrence planned by Component 10:

- one whole source edge when planned as one occurrence;
- one pair per selected source-edge interval;
- one pair per selected transverse carrier interval and multiplicity occurrence;
- one pair per selected coplanar-overlap boundary occurrence;
- one pair per topology-separation contact boundary when planned; and
- separate pairs for equal-parameter or equal-coordinate topology-distinct intervals.

Component 08 event and interval order is authoritative. Component 11 may audit it but must not reconstruct or re-sort it from nominal coordinates.

For every zero-nominal-length or uncertainty-overlapping edge, publish a descriptor containing length bounds, endpoint identities, preservation reason, incident uses/regions/cycles, cleanup eligibility constraints, and Component 12/13 obligation. Do not collapse or delete it.

### 5.7 Carrier and source-edge balance

For every referenced source-edge sequence or carrier domain:

- consume the authoritative ordered interval/event sequence;
- compare exact selected member identities and directions;
- verify each planned edge appears exactly once in one balance domain;
- verify start/end balance on closed chains;
- verify documented closure through source edges for open carrier chains;
- preserve equal-parameter tie order by frozen lineage keys; and
- reject missing, duplicate, crossed, or definitely reversed realization.

Counts supplement but never replace ordered member-set comparison.

### 5.8 Successor and predecessor permutation

Represent each realized region-boundary incidence as an oriented dart. Use predecessor authoritative retained-use cyclic boundary order to define `face_next` and `face_prev`. Use `seam_twin` only for validated transparent continuations.

For a boundary dart `b`, the V1-equivalent topological rule is:

```text
j = face_next(b)
while j is transparent-internal:
    j = face_next(seam_twin(j))
return j
```

The walk must remain in the same final region, support class, sheet, multiplicity occurrence, and permitted fan. It may not encounter support-only, consumed, suppressed, or unresolved records. Bound each walk by the exact region dart count.

Map boundary darts bijectively to output halfedges. The resulting successor must satisfy:

- destination of the current halfedge equals origin of the next by occurrence identity;
- both halfedges belong to the same final region;
- the transition corresponds to the exact retained-sector/continuation walk; and
- no separation boundary is crossed.

Construct predecessor as the inverse permutation. Require totality, one predecessor and one successor per halfedge, reciprocal inverse relations, and exact member coverage.

Turning angle, nearest endpoint, nominal polar order, source triangle order, and arbitrary outgoing choice are prohibited.

### 5.9 Face-cycle extraction and canonical identity

Extract maximal cycles from the validated successor permutation. Each cycle must:

- close at its starting halfedge and output occurrence;
- contain no directed halfedge twice;
- share no directed halfedge with another cycle;
- preserve final region orientation;
- remain within one region; and
- consume only realized boundary halfedges.

Canonicalize each oriented cycle by its lexicographically least orientation-preserving rotation of complete directed-halfedge keys. Never reverse a cycle to obtain a smaller key.

The cycle identity key is:

```text
(final face-region key,
 orientation-preserving canonical halfedge sequence,
 cycle-key schema version)
```

It must not contain a contour role that is derived later. Outer, hole, coincident, and deferred roles belong to contour records and may not renumber cycle IDs or create a cycle-identity dependency on later classification.

### 5.10 Contour roles, holes, islands, and nesting

Each positive-area V1 final region contains:

- exactly one positive-area outer cycle;
- zero or more direct positive-area hole cycles; and
- zero or more explicit deferred zero-measure or occurrence-distinct contour records.

Positive-area islands are separate final regions. Multiple unrelated positive-area outer cycles require the earlier exact region split; contour analysis must not perform a late topology rewrite.

Determine roles primarily from:

- retained arrangement and positive-area member components;
- prescribed region orientation;
- source-facet semantic support and side lineage;
- oriented boundary incidence and local sectors; and
- Component 10 ownership, multiplicity, and separation records.

Bounded projected orientation and containment are independent admissibility checks or explicitly authorized disambiguators, not new topology sources.

### 5.11 Certified contour witnesses

A containment test must not use an arbitrary boundary vertex, an epsilon offset, or merely any adjacent retained atom.

For each required containment relation, use one of:

1. a predecessor-certified arrangement-cell witness with an explicit intended-side relation to the contour; or
2. a deterministically constructed Component 03 bounded witness derived from a certified positive-area atom/sector, with proof that its enclosure is strictly on the required contour side and definitely separated from every relevant boundary segment.

The witness record must identify the contour side it certifies, source atom/sector lineage, bounded construction/evidence, separation margins, and deterministic key.

If no strict witness can be certified, or if its enclosure can cross a nonincident contour so that positive-area nesting could change, return `geometric_condition_exceeds_tolerance`. Do not nudge a nominal point.

### 5.12 Bounded embedding and noncrossing admissibility

Use Component 03 structured bounded services to verify:

- every edge endpoint is admissible for its claimed source-edge/carrier interval;
- authoritative order admits no definite reversal;
- equal-parameter ties follow frozen lineage order;
- positive-area cycle orientation agrees with support orientation;
- no nonincident boundary segments have a definite unrepresented proper crossing;
- no positive-length overlap exists without exact coincidence/owner lineage;
- topology-distinct exact touches remain separate; and
- uncertainty that can change positive-area connectivity or nesting is either an explicitly supported downstream degeneracy or a typed failure.

Use a deterministic in-tree conservative broad phase for scalable candidate enumeration and exhaustive checks below a frozen threshold. A broad phase may only prune definitely separated pairs.

### 5.13 Vertex-link reconstruction

After cycle finalization, reconstruct the outgoing halfedge link around every output occurrence. For outgoing halfedge `h` at occurrence `v`, the polygonal halfedge relation is conceptually:

```text
around_origin(h) = pair(prev(h))
```

Require the result to originate at `v`. Traverse from a canonical start and require one closed cycle consuming every outgoing halfedge at `v` exactly once.

Compare the reconstructed member and transition sets with the Component 10 vertex occurrence requirement, allowing cyclic rotation but no member loss, duplication, cross-separation transition, or joining of distinct cycles.

### 5.14 Complete balance and lineage audit

Before publication, prove exact identity equalities:

- output vertex occurrence keys equal Component 10 nonempty vertex occurrence requirement keys one-to-one;
- paired edge keys equal planned edge occurrence keys one-to-one;
- `halfedge_count == 2 * paired_edge_count` with checked arithmetic;
- realized boundary incidence identities equal halfedge incidence identities one-to-one;
- support-only, consumed, and suppressed incidences map to no output halfedge;
- transparent continuations are consumed internally and map to no edge;
- every halfedge has one pair, region, successor, predecessor, and cycle;
- total cycle references equal the exact halfedge set;
- each region boundary multiset equals the union of its cycles;
- each source-edge/carrier domain balances;
- each output occurrence has one closed link matching Component 10; and
- no entity lacks predecessor lineage.

## 6. Empty result

If Component 10 contains no retained uses, planned edges, or vertex occurrence requirements, Component 11 must publish a valid empty artifact containing:

- empty topology and reverse-map sections;
- complete predecessor, policy, provider, and version references;
- the complete discard/suppression audit reference;
- zero topology/resource counts;
- deterministic section and full digests; and
- successful producer and independent-verifier dispositions.

Do not allocate dummy vertices, edges, regions, or cycles. Support-only evidence cannot make an otherwise empty regularized surface nonempty.

## 7. Output contract

On success, publish one immutable `polygonal_output_complex<T,I>` containing or referencing:

- owner, operation, operand roles, strict floating profile, and all versions;
- predecessor artifact identities and complete digests;
- the complete retained-incidence audit;
- canonical output vertex occurrences and authoritative bounded coordinate references;
- canonical final face regions, sorted retained-use members, continuation members, and split evidence;
- boundary darts and transparent-consumption ledgers;
- paired output edges and exactly two reciprocal halfedges per edge;
- successor/predecessor maps;
- oriented face cycles with role-independent canonical identities;
- contour nodes and outer/hole/deferred roles;
- certified containment witnesses and contour relationships;
- paired zero-measure boundary descriptors and support-only evidence records as distinct sections;
- source-edge/carrier order and balance evidence;
- bounded embedding and noncrossing evidence;
- output vertex-link evidence;
- complete forward/reverse maps and provenance;
- resource and structural statistics;
- diagnostics and replay metadata;
- producer and independent-verifier evidence;
- canonical section digests and complete digest.

The artifact guarantees that Component 12 can triangulate positive-area regions and carry deferred obligations without repeating Component 10 selection, occurrence partitioning, edge pairing, region grouping, cycle extraction, or contour topology decisions.

On failure, publish no polygonal artifact.

## 8. Independent verification

The independent verifier must be implemented separately from producer orchestration and must not use producer grouping, pair allocation, successor construction, cycle walking, contour classification, or codec helpers as its sole proof.

It must independently:

- reconstruct the incidence audit from Component 10;
- verify the boundary/support-only zero-measure partition;
- rebuild occurrence-to-output-vertex bijection from complete keys;
- rebuild continuation components and positive-area final region splits using an alternate traversal;
- derive exact region boundary multisets;
- reconstruct every planned pair directly from Component 10 members and endpoints;
- derive successor/predecessor through separately implemented transparent-dart walking;
- extract cycles from a different start order, canonicalize, and compare complete sequences;
- verify contour roles from arrangement lineage and independently obtained bounded evidence;
- validate certified containment witnesses;
- run exhaustive noncrossing/containment checks below a bounded threshold and scalable checks otherwise;
- reconstruct vertex links from artifact topology and compare Component 10 fan cycles;
- verify all member-set equations, resource totals, canonical IDs, and digests; and
- verify that no coordinate-based identity or adjacency field exists in a semantic key.

Producer/verifier disagreement is `internal_invariant_error` and prevents commit.

## 9. Determinism, concurrency, cancellation, and transactionality

The executable serial implementation is the semantic reference.

Parallel work may prepare private complete-keyed proposals for occurrences, continuation classes, edge pairs, carrier domains, regions, cycles, contour evidence, and verifier partitions. Publication must use canonical full-key sort, exact duplicate reconciliation, checked prefix sums, deterministic failure arbitration, and dense remapping.

Worker count, task partition, delays, pointer values, allocation order, hash collision, union root, graph start, edge discovery, cycle start, and contour input order must not change:

- semantic records;
- IDs;
- selected primary failure;
- diagnostics;
- replay;
- canonical bytes; or
- digest.

Every worker establishes the frozen floating environment. Cancellation is polled at deterministic work-count checkpoints. On failure or cancellation, all workers join, all reservations roll back, and no partial artifact is visible.

## 10. Strict build and dependency contract

Component 11 production and normative-test translation units must be part of the strict bounded Boolean target established by Components 01 and 03.

They must not be compiled in an ordinary target that permits:

- `-ffast-math` or equivalent;
- reassociation;
- finite-only assumptions;
- an unfrozen contraction policy;
- an unsupported rounding mode; or
- topology-affecting arithmetic outside Component 03 services.

The implementation must extend existing bounded-subsystem registries and explicit instantiation matrices rather than create parallel contract, hash, resource, or error systems. Required V1 instantiations cover `float`/`double` with `uint32_t`/`uint64_t`, subject to the public supported-type contract.

No external graph, polygon, arrangement, mesh, exact-arithmetic, geometry, serialization, testing, hashing, or concurrency dependency may be added, vendored, downloaded, optionally loaded, or invoked.

## 11. Existing Ygor functionality assessment

Existing Ygor code may provide fixture shapes, general algorithmic ideas, and non-normative smoke checks, but it does not satisfy this component's production contract:

- `fv_surface_mesh<T,I>` is a mutable public transport type and is not an intermediate occurrence/halfedge/lineage representation.
- `FindBoundaryChains` and related `YgorMeshesHoles` code build epsilon-distance vertex representatives, skip representative-equal edges, and can choose an arbitrary remaining continuation when several exist. They must not build Component 11 topology.
- `FillBoundaryChainsByZippering` mutates a public mesh, skips short/duplicate-index edges, and triangulates; it belongs to neither this stage nor its verifier.
- `EnsureConsistentFaceOrientation` infers adjacency through epsilon representatives and reverses mutable face rings. Component 11 receives authoritative orientation and must fail on contradiction.
- `YgorMathMonotoneDecomposition` normalizes duplicate/collinear vertices, rejects zero-length edges and reused coordinates, uses ordinary coordinate comparisons and `long double` area, and lacks bounded evidence and lineage. It is not a Component 11 provider.
- `YgorMeshesVerification` checks public indices and edge counts but does not reconstruct occurrence identity, transparent consumption, face cycles, or one closed vertex link. It may be a later smoke check only.
- legacy `YgorMeshesBoolean{,2,3,4,5}` implementations do not satisfy the bounded compute-once lineage and no-welding architecture and must not be called, adapted, or copied into this component.

Small purely combinatorial helpers may be shared only when they accept strong IDs/checked ranges, have no coordinate-based normalization, are deterministic by complete keys, and are independently tested.

## 12. Typed failures and diagnostics

Stable Component 11 subcodes must cover at least:

- unsupported provider/schema/query version;
- wrong owner, operation, role mapping, predecessor, or digest;
- invalid range, strong-ID domain, index capacity, or resource reservation;
- missing, duplicate, unresolved, or multiply classified incidence;
- invalid boundary/support-only zero-measure classification;
- support-only evidence leaking into ports, regions, boundaries, edges, or cycles;
- invalid/nonreciprocal transparent continuation or prohibited continuation crossing;
- duplicate/malformed/empty vertex occurrence requirement or coordinate-source mismatch;
- provisional/final region support, orientation, sheet, or split mismatch;
- boundary-dart walk escape, loop, or under/over-consumption;
- planned-edge cardinality, direction, endpoint, port, descriptor, interval, multiplicity, or owner mismatch;
- pair initialization or reciprocity failure;
- source-edge/carrier missing member, duplicate member, imbalance, order reversal, or crossed realization;
- successor/predecessor missing, duplicate, noninverse, cross-region, or fan-incompatible transition;
- cycle nonclosure, premature repeat, duplicate use, wrong orientation, or canonicalization failure;
- role-dependent cycle identity mutation;
- missing/multiple outer contour, wrong hole relation, unresolved island split, invalid witness, crossing, or topology-changing containment uncertainty;
- unsupported zero-measure topology;
- vertex-link open, branched, multi-cycle, duplicate, missing sector, or fan mismatch;
- complete member-set/reverse-map/resource/codec/digest/verifier mismatch;
- cancellation; and
- internal contradiction.

Use categories consistently:

- `geometric_condition_exceeds_tolerance` for valid inputs whose bounded evidence cannot choose one topology safely;
- `index_overflow` for representability;
- `resource_limit` for configured capacity exhaustion;
- `cancelled` after joined rollback; and
- `internal_invariant_error` for contradictory committed artifacts, impossible exact topology, corruption, or producer/verifier disagreement.

Every error records the checkpoint, least canonical witnesses, relevant expected/actual member sets, numerical evidence, versions, resource counters, and deterministic replay identity.

## 13. Required tests

### 13.1 Contract and incidence tests

Test every incidence disposition, including paired zero-measure boundary and support-only zero-measure evidence. Inject:

- missing and duplicate rows;
- one incidence assigned to both edge and continuation;
- support-only evidence given a local port;
- support-only evidence placed in a region boundary or cycle;
- a zero-measure boundary use without a planned mate;
- a planned edge member mislabeled support-only; and
- forged counts with wrong member sets.

### 13.2 Vertex occurrence tests

Cover ordinary source fans, shared events, high-valence events, point- and edge-touching separate occurrences, coincident multiplicity sheets, equal-coordinate source/event identities, zero-length boundary edges, hash collisions, and input-order permutations.

Reject empty V1 occurrence requirements and disconnected cycles merged into one occurrence.

### 13.3 Region and boundary tests

Cover continuation across source-facet diagonals, classification seams, uncut source boundaries, and owner seams. Prohibit continuation across selected carriers, point/edge-contact separators, incompatible multiplicity, opposite orientation, non-owner seams, and support-only zero-measure records.

Include a provisional class connected only by zero-measure support and require separate positive-area final regions.

### 13.4 Pair-at-creation tests

Cover whole edges, split intervals, transverse carriers, closed carrier loops, chains closing through source edges, coplanar boundaries, equal-parameter clusters, zero-nominal-length intervals, multiplicity copies, and cross-operand A/B seams.

Use poisoned/uninitialized builder storage in a test harness to prove pair endpoint initialization depends only on validated `v0`/`v1`, not opposite unresolved fields.

### 13.5 Cycle and contour tests

Commit known answers for triangle, convex and concave polygons, annuli, multiple holes, separate island regions, thin corridors, repeated projected coordinates, coordinate-coincident distinct occurrences, zero-length edges, deferred zero-measure contours, and high-valence fan-controlled continuation.

Verify cycle IDs remain unchanged when contour roles are assigned, independently recomputed, or input contour order is permuted.

### 13.6 Certified witness and admissibility tests

Test strict arrangement-cell witnesses, certified atom/sector witnesses, repeated-coordinate boundaries, topology-distinct touches, narrow holes, deep nesting, uncertain witness separation, proper crossings, overlaps, and equal-parameter ties.

Reject an arbitrary adjacent-atom point that lies on the wrong contour side and any epsilon-offset witness.

### 13.7 Independent verifier mutation tests

Corrupt one fact at a time, repairing cached counts/digests where possible:

- merge/split occurrence identities;
- alter coordinate source;
- change continuation or region membership;
- leak support-only evidence into topology;
- remove/alter a pair or endpoint;
- change successor/predecessor;
- break/reuse/reverse a cycle;
- insert role into cycle identity;
- change outer/hole/island relation;
- alter a witness or separation bound;
- change carrier order/balance;
- merge vertex links;
- scramble IDs/ranges/resources; or
- forge digests.

Every required mutation must be rejected.

### 13.8 Exhaustive, fuzz, metamorphic, and replay tests

For bounded fixtures, exhaustively enumerate valid continuation classes, two-use planned pairs, successor permutations implied by lineage, cycle covers, V1 one-outer/direct-hole region partitions, and vertex-link cycles.

Fuzz controlled concavity, holes, carrier loops, event valence, equal-parameter clusters, point/edge contacts, duplicate coordinates, zero-length edges, support-only zero-measure records, and resource limits. Shrink every crash, arbitrary pairing, accidental weld, nonclosure, contour disagreement, or verifier mismatch.

Apply source vertex/facet/triangle/shell permutations, ring rotation, legal subdivision/retriangulation, operand remapping, axis permutation, corrected sign flip, exactly representable translation, power-of-two scaling, thread counts, task delays, reversed graph starts, reversed edge discovery, reversed cycle starts, contour permutations, and forced hash collisions. Require canonical identity and bytes after documented predecessor-ID remapping.

### 13.9 Resource, cancellation, sanitizer, portability, and structural performance tests

Test limit-minus-one, limit, and limit-plus-one for every resource subkind. Cancel at every stable checkpoint and inside long sorts, walks, broad phases, codec, and verifier work. Confirm joined rollback and no visible partial artifact.

Run supported GCC/Clang Debug/Release matrices, ASan/UBSan, TSan for parallel paths, strict floating-environment conformance, and build checks proving no forbidden external or legacy Boolean dependency is linked.

Structural gates include:

- output vertex count exactly equals nonempty occurrence requirement count;
- paired edge count exactly equals planned edge count;
- halfedge and cycle-reference counts exactly equal `2 * E`;
- support-only evidence maps to zero topology entities;
- zero coordinate-neighbour and tolerance-weld queries;
- linear boundary/cycle/link traversal in visited members;
- documented `O(n log n)` canonical sorts;
- no unconditional scalable all-pairs noncrossing search;
- no recursion proportional to input size; and
- no hidden per-edge hot-loop allocation after reservation.

## 14. Definition of done

Component 11 is complete only when:

- every Component 10 nonempty vertex occurrence requirement is realized exactly once with authoritative coordinate lineage;
- every planned edge is born as one validated reciprocal pair with explicit endpoint initialization;
- paired zero-measure boundaries and support-only zero-measure evidence are unambiguously separated;
- every realized boundary incidence and transparent continuation is consumed exactly once in its proper domain;
- support-only evidence creates no output topology;
- every halfedge belongs to exactly one closed oriented cycle;
- cycle identity is independent of later contour role;
- every output occurrence has one closed link matching Component 10;
- V1 regions have one outer, direct holes, separate island regions, and explicit deferred obligations;
- containment uses certified strict witnesses and fails closed on topology-changing uncertainty;
- equal coordinates and zero-nominal-length edges remain distinct where required;
- carrier ordering and geometric admissibility never invoke heuristic pairing or continuation;
- independent reconstruction rejects all required mutations;
- replay and canonical bytes are stable across traversal, allocation, hashing, and schedules;
- resources, cancellation, transaction rollback, diagnostics, and strict-build integration are complete;
- Component 12 can consume the artifact without repeating selection, occurrence partitioning, edge pairing, region grouping, or cycle construction; and
- all production and normative-test code is strict self-contained C++17 with no external dependency.
