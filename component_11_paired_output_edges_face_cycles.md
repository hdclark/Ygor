# Component 11: Paired Output Edges and Face-Cycle Construction

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete halfedge storage, retained-region grouping provider, carrier merge representation, cycle-walk algorithm, contour-hierarchy representation, and deterministic graph implementation may change. The allocation of topology from Component 10 occurrence requirements, pair-at-creation rule, exact lineage-based endpoint sharing, complete incidence consumption, deterministic carrier realization, closed face-cycle construction, hole preservation, ambiguity failure, verification, and publication contracts in this document are normative.

## 0. Purpose

This component realizes the selected and occurrence-partitioned result surface from Component 10 as an explicit indexed polygonal two-manifold skeleton.

Its purposes are to:

- allocate one output vertex occurrence for every authoritative vertex/event fan requirement selected by Component 10;
- attach each output occurrence to the one bounded source or constructed coordinate already owned by Components 03, 05, and 08;
- realize every planned source-edge or intersection-carrier occurrence as exactly one pair of oppositely directed output halfedges;
- preserve whole source edges, split source-edge intervals, transverse intersection edges, coplanar overlap boundaries, and topology-distinct zero-length intervals;
- merge retained atom boundaries only where Component 10 explicitly permits transparent continuation;
- construct deterministic oriented boundary cycles for every retained polygonal face region;
- represent holes, nested contours, repeated coordinates, and separate coincident occurrences without coordinate-based welding;
- prove that every retained directed boundary incidence is consumed exactly once by a paired edge and exactly once by a face cycle; and
- provide Component 12 with a verified immutable polygonal manifold whose topology requires no Boolean reclassification or edge-repair heuristic.

The component realizes topology. It does not repeat Boolean truth-table evaluation, recompute intersection events, alter carrier ordering, move coordinates, collapse edges, triangulate face regions, remove degeneracies, merge coplanar output faces beyond the explicit retained-region contract, or serialize the public mesh.

The principal output is an immutable `polygonal_output_complex<T>` containing:

- canonical output vertex occurrences and their bounded coordinate references;
- canonical paired output edges and reciprocal halfedges;
- retained polygonal face regions;
- one or more oriented face cycles per region;
- explicit contour roles and nesting relationships where they are determinable without cleanup;
- tagged degenerate or zero-measure cycles that remain topologically required for Components 12 and 13;
- exact reverse mappings to Component 10 retained uses, occurrence requirements, and boundary incidences; and
- complete topology, ordering, precision, resource, verification, digest, and replay evidence.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `retained_surface_complex` from Component 10;
- the immutable `canonical_intersection_complex<T>` from Component 08;
- the immutable `signed_feature_relations<T>` from Component 07 when relation provenance is needed to audit carrier direction or exact-tie lineage;
- the immutable `canonical_source_manifolds` from Component 05;
- source-facet groups, source-boundary identities, and internal-diagonal provenance from Components 04 and 05;
- validated source shell orientation and source occupied-side conventions from Component 02;
- the immutable `precision_context<T>`, bounded point/parameter/residual services, precision ledger, and tolerance-budget read services from Component 03;
- the immutable Boolean context, identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, and transaction services from Component 01;
- the selected output-occurrence, paired-edge, face-region, face-cycle, contour, artifact, and serialization versions; and
- verification settings controlling scalable and exhaustive topology-realization checks.

The component must not read mutable caller meshes, infer sharing from equal coordinates, repeat relation predicates, recompute a constructed point, repeat Boolean selection, invent an occurrence partition, or modify predecessor artifacts to make cycle construction succeed.

### 1.2 Required predecessor guarantees

The component may rely on predecessor artifacts having established:

- one audited disposition for every positive-area classification atom;
- the complete set of retained oriented surface uses;
- exact continuation permission or prohibition across every retained atom boundary;
- one canonical vertex/event fan occurrence requirement for each future output vertex occurrence;
- one canonical edge/carrier occurrence requirement for each future output edge occurrence;
- exactly two compatible opposite-directed retained boundary uses for every planned output edge occurrence;
- deterministic start/end balance on every source-edge and carrier domain;
- canonical source-edge interval and carrier ordering, including equal-parameter tie keys;
- explicit multiplicity and topological separation for point-, edge-, and coincident-sheet contacts;
- complete source, event, carrier, shell, facet, retained-use, and caller provenance;
- one bounded coordinate and precision envelope for every source vertex and canonical event used by the result; and
- deterministic canonical keys and digests.

The component must defensively verify owner tokens, policy and artifact versions, retained-use coverage, occurrence membership, endpoint compatibility, orientation, interval references, event references, carrier references, pair-feasibility evidence, and predecessor digests.

A contradiction in a committed predecessor artifact is an `internal_invariant_error`. The component must not repair a missing mate by nearest-neighbor search, duplicate a face use to balance a carrier, drop an incidence, or merge occurrence requirements.

### 1.3 Topological realization domain

The realization domain consists of all retained surface uses and all boundary or transparent-continuation incidences that Component 10 associated with them.

Every retained incidence must be classified before construction as exactly one of:

- a boundary incidence that becomes one directed use of an output edge occurrence;
- a transparent internal incidence across which compatible retained atoms form one polygonal face region;
- a topology-separation delimiter that must not be crossed;
- a zero-measure incidence that remains represented for later degeneracy handling;
- a consumed coincidence-partition seam whose canonical owner continues across it;
- an explicitly suppressed non-owner incidence recorded only as audit evidence; or
- invalid/unresolved.

No retained incidence may be silently absent from the realization audit.

### 1.4 Vertex occurrence input requirements

Every Component 10 vertex/event fan requirement supplied to this component must identify:

- its canonical occurrence key and owner context;
- whether its coordinate source is an accepted source vertex, a canonical intersection event, or another explicitly supported bounded construction lineage;
- all incident retained sectors and directed boundary-incidence endpoints;
- one complete closed cyclic fan requirement, including for geometrically zero-measure configurations;
- multiplicity and topology-separation class;
- exact source/event/carrier provenance;
- the bounded coordinate record and precision-ledger reference to be used without recomputation; and
- deterministic cyclic-order evidence sufficient to connect incoming and outgoing halfedges.

Two occurrence requirements may reference bit-identical bounded coordinates. They remain distinct output vertices unless Component 10 explicitly gave them one occurrence identity. Conversely, one occurrence requirement must produce exactly one output vertex occurrence even when several source triangles, retained uses, or event consumers refer to it.

### 1.5 Edge occurrence input requirements

Every planned edge occurrence must identify:

- its canonical occurrence key;
- exactly two retained directed boundary incidences;
- opposite local directions after retained-use orientation is applied;
- compatible start and end vertex occurrence requirements;
- source-edge interval, transverse carrier interval, coplanar overlap boundary, or other supported carrier lineage;
- endpoint event/source-vertex keys;
- multiplicity and topology-separation class;
- local fan membership at both endpoints;
- whether the nominal endpoints are distinct, equal, or uncertainty-overlapping; and
- deterministic pairing and balance evidence.

The two uses must agree on the unordered endpoint occurrence pair. Their directions must be exact reversals in topology. Nominal coordinate equality or disagreement within uncertainty is not a substitute for endpoint identity compatibility.

### 1.6 Face-region input requirements

A retained polygonal face region may be assembled from one or more retained atoms only when Component 10 permits continuation across every internal boundary between them.

The input must make it possible to recover:

- the complete member retained-use set;
- one prescribed output orientation;
- one supporting source-facet semantic region or another explicitly versioned compatible support class;
- all external boundary incidences;
- all transparent internal incidences;
- all prohibited continuation boundaries;
- all holes, islands, coincidence seams, and zero-measure delimiters represented by the retained arrangement; and
- complete provenance for every member and boundary.

This component must not merge regions merely because they are coplanar, have parallel nominal normals, share coordinates, or could reduce face count.

### 1.7 Capacity and lifetime preconditions

Before realization begins, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all output vertex occurrences and reverse maps;
- all output edge occurrences and two halfedges per edge;
- all retained-region membership and continuation edges;
- all face cycles and cycle halfedge references;
- all contour-nesting and degeneracy descriptors;
- all source, event, carrier, and retained-use provenance links;
- all temporary sorting, graph traversal, union, cycle, verification, diagnostics, and replay storage;
- all work up to configured occurrence, edge, halfedge, face-region, cycle, and incidence limits; and
- index domains needed by downstream internal artifacts.

Published records may reference only immutable predecessor storage and immutable stage-owned storage whose lifetime covers Components 12-15.

## 2. Required behavior

### 2.1 Complete retained-incidence audit

Before allocating final IDs, the component must create a canonical audit table over every Component 10 retained incidence.

For each incidence, the audit must record:

- retained-use owner;
- source atom and source-facet lineage;
- oriented start and end occurrence requirements;
- carrier or transparent-seam lineage;
- occurrence partition;
- expected mate when it is a boundary incidence;
- continuation neighbor when it is transparent;
- suppression or topology-separation reason when applicable; and
- final realization disposition.

The component must verify that each retained boundary incidence belongs to exactly one edge occurrence requirement and each transparent internal incidence belongs to exactly one compatible continuation relation in each direction.

Missing, duplicate, or multiply owned incidence is a failure. Counts alone are insufficient; full canonical member keys must match.

### 2.2 Canonical output vertex occurrence allocation

The component must allocate one immutable output vertex occurrence for each canonical vertex/event fan requirement.

Allocation must:

- sort complete occurrence keys canonically before assigning IDs;
- copy or reference the authoritative bounded coordinate without recomputing it;
- preserve the nominal `T` bit pattern and all precision-envelope metadata;
- retain source-vertex or event lineage and every contributing retained sector;
- retain multiplicity, coincidence, and topology-separation classes;
- retain the expected cyclic fan membership; and
- publish a reverse map from every retained incidence endpoint to its output vertex occurrence.

An occurrence with no consumed retained incidence is invalid unless it is an explicitly preserved zero-measure support required by a documented downstream degeneracy record. An incidence endpoint that maps to more than one output occurrence is invalid unless Component 10 supplied separate endpoint occurrence keys for the separate uses.

No coordinate lookup table may be used as an identity map.

### 2.3 Bounded geometry attachment

Each output vertex occurrence must reference exactly one authoritative bounded point record.

For source-vertex occurrences, the record must derive from the accepted source coordinate and inherited precision. For event occurrences, it must be the one canonical Component 08 construction record. If a future supported lineage combines values, that construction must have been created and bounded by its authoritative producer before this component begins.

This component may evaluate bounded residuals to audit admissibility, but it must not create a numerically different replacement point. A failed residual or non-finite coordinate causes a typed failure rather than a local projection.

The stage must add no cleanup displacement. Its published precision contribution is therefore limited to inherited storage/serialization bounds explicitly defined by Component 03.

### 2.4 Retained face-region formation

The component must group retained atoms into polygonal face regions through the exact transparent-continuation relation supplied by Component 10.

Grouping must satisfy:

- continuation is reciprocal;
- member orientations are compatible;
- members belong to one occurrence/multiplicity sheet;
- no selected carrier boundary is crossed;
- no point- or edge-contact separation is crossed;
- no incompatible coincident owner boundary is crossed;
- internal source-triangulation diagonals remain transparent when permitted;
- each connected continuation class receives one canonical region key; and
- region IDs are assigned after canonical member sorting, not from union roots or traversal starts.

A provider may use union-find, graph traversal, region growing, or another in-tree algorithm. The resulting equivalence classes and boundary incidence sets are normative.

A connected continuation class that has several disjoint positive-area interiors must be split into separate face regions unless its explicit contour topology represents one face with holes. The split decision must come from the retained arrangement and boundary connectivity, not a coordinate-distance heuristic.

### 2.5 Boundary extraction from region membership

For each face region, the component must determine its directed boundary multiset by exact incidence cancellation:

- a transparent internal incidence shared by two compatible member atoms is consumed internally and does not become an output edge;
- a selected or topology-separating incidence remains on the region boundary;
- an incidence with a suppressed non-owner on the opposite geometric sheet remains governed by its Component 10 edge occurrence requirement;
- multiplicity-distinct incidences never cancel each other; and
- two incidences cancel only when their exact lineage, occurrence partition, and continuation contract identify them as the same transparent seam in opposite directions.

Boundary extraction must preserve a complete cancellation ledger. The component must not cancel two nominally coincident segments by projected overlap.

### 2.6 Atomic pair-at-creation edge realization

Every output edge occurrence must be realized atomically as:

- one immutable undirected paired-edge record;
- exactly two directed halfedges;
- reciprocal pair references;
- reversed topological endpoints;
- one incident retained face region per directed halfedge;
- carrier/source provenance;
- endpoint occurrence and local-fan references; and
- deterministic edge and halfedge keys.

The pair must be created only after both directed uses have been validated. A single halfedge must never become visible in the proposed artifact.

Conceptually:

```text
halfedge_0.origin == halfedge_1.destination
halfedge_0.destination == halfedge_1.origin
halfedge_0.pair == halfedge_1
halfedge_1.pair == halfedge_0
```

The two halfedges may belong to different face regions or to two boundary cycles of the same region only when that topology is explicitly permitted and still yields a two-manifold edge.

### 2.7 Whole and split source-edge realization

For retained source edges, the component must realize:

- an uncut whole source edge as one edge occurrence when Component 10 planned one occurrence;
- each open or closed interval between ordered source-edge events as its own occurrence when selected;
- endpoint intervals and exact event clusters using the Component 08 canonical order;
- separate copies for multiplicity or edge-touching topology; and
- transparent source-edge intervals as internal region continuation rather than output edges when Component 10 says they are not a result boundary.

The component must not sort source-edge endpoints again from nominal coordinates. It may verify Component 08 order using bounded parameters and tie keys, but the canonical event sequence is authoritative.

If predecessor ordering evidence is contradictory or an uncertainty-overlap permits geometrically crossed realization inconsistent with the planned topology, the component must return `geometric_condition_exceeds_tolerance` or `internal_invariant_error` according to whether the issue is geometric conditioning or a committed artifact contradiction.

### 2.8 Intersection-carrier realization

For transverse face-face carriers, coplanar overlap boundaries, and other canonical carrier domains, the component must:

- consume the Component 08 carrier and interval identities;
- respect the Component 10 occurrence partition and direction for each retained use;
- realize each selected carrier interval as exactly one paired output edge occurrence per required multiplicity;
- preserve equal-parameter but topology-distinct event endpoints;
- preserve closed carrier loops and carrier chains that close through source edges;
- retain relation and construction provenance; and
- verify start/end balance after all carrier edge pairs are created.

Carrier realization must not reconstruct intersections from planes, intersect projected lines, or choose mates by nearest parameter.

### 2.9 Equal coordinates and zero-nominal-length edges

Topology may require an output edge whose endpoint occurrences have equal nominal coordinates or overlapping precision envelopes.

Such an edge must remain represented when:

- Component 10 planned a distinct edge occurrence;
- the endpoint occurrence identities are distinct or the local topology requires a loop-like degenerate support;
- deleting it would alter fan connectivity, contour membership, multiplicity, or contact separation; and
- later triangulation/cleanup requires the edge for a complete certificate.

The edge must be tagged with:

- nominal length bounds;
- endpoint lineage;
- reason it is topologically required;
- incident face uses;
- cleanup eligibility constraints; and
- whether Component 12 may triangulate around it directly or must emit a degenerate-support record.

This component must not collapse, delete, or weld the edge.

### 2.10 Local successor and predecessor relation

For every output halfedge incident to a face region, the component must determine exactly one next halfedge and one previous halfedge in the oriented boundary of that region.

The successor relation must be derived from:

- the output vertex occurrence's Component 10 fan requirement;
- the retained face-region membership;
- incoming and outgoing incidence orientation;
- source-facet and carrier continuation lineage;
- topology-separation constraints; and
- deterministic cyclic-order evidence.

It must not be chosen by the smallest turning angle among nominal vectors unless that ordering is merely an independently verified realization of the authoritative fan order and remains definite within the bounded model.

At a high-valence or equal-coordinate event, separate occurrence requirements and fan sectors must control continuation. A halfedge may not jump to another coincident sheet, point-touching component, or multiplicity occurrence.

### 2.11 Boundary-cycle extraction

After all halfedges have reciprocal pairs and local successors, the component must extract maximal directed cycles.

Cycle extraction must guarantee:

- every region-boundary halfedge belongs to exactly one cycle;
- every cycle closes at its starting halfedge and output vertex occurrence;
- no directed halfedge appears twice in one cycle;
- no directed halfedge appears in two cycles;
- successor and predecessor links are reciprocal;
- the cycle orientation is consistent with the retained face-region orientation;
- a cycle has a canonical start chosen after construction; and
- cycle IDs are assigned by canonical cycle keys, not discovery order.

A walk that enters a previously visited halfedge other than its start, fails to close, or exceeds the exact halfedge count is an invariant failure. The component must not cut the walk at an arbitrary repeated coordinate.

### 2.12 Canonical cycle representation

Each face cycle must publish:

- ordered halfedge IDs;
- ordered output vertex occurrence IDs;
- prescribed orientation;
- supporting face-region and source-facet lineage;
- carrier/source-edge role for each boundary segment;
- a canonical rotation key;
- bounded projected-area evidence when meaningful;
- degeneracy and repeated-coordinate descriptors; and
- a deterministic digest.

Canonical rotation must preserve orientation. Reversing a cycle to obtain a smaller lexical key is prohibited unless the entire face-region orientation is also explicitly reversed by an authoritative predecessor decision, which this component does not perform.

### 2.13 Outer contours, holes, islands, and nesting

For each positive-area face region, the component must classify its cycles sufficiently for Component 12 to triangulate the intended polygonal region.

The published contour structure must distinguish:

- outer boundary cycles;
- hole boundary cycles;
- nested islands that form separate positive-area face regions or explicitly nested contour nodes;
- zero-measure cycles pending cleanup;
- coincident but occurrence-distinct cycles; and
- invalid crossing or ambiguous contour relations.

Contour role and nesting should be derived from the retained arrangement, oriented boundary incidence, and exact source-facet semantics. Bounded projected orientation and containment tests may be used for verification or where the arrangement contract leaves a choice, but they must return a definite or explicitly tied result.

A nominal point-in-polygon test on a possibly repeated coordinate is not sufficient by itself. When geometric uncertainty makes two nonincident contours' relative order or containment indeterminate enough that different choices would change positive-area topology beyond tolerance, the stage must fail rather than guess.

A face region with multiple unrelated outer contours must be split into canonical region records unless the selected representation explicitly supports a multipolygon region and Component 12's contract is versioned to consume it.

### 2.14 Carrier-order and noncrossing admissibility

Although predecessor ordering is authoritative, this component must verify that the realized topology is geometrically admissible within published precision envelopes.

At minimum, it must check:

- each edge endpoint lies within the bounded source edge or carrier interval claimed by its lineage;
- ordered events do not admit a definite reversal;
- two nonadjacent boundary intervals in one planar face region do not have a definite positive-length crossing that is absent from the arrangement;
- equal-parameter tie order uses the frozen lineage key and does not imply an undocumented geometric weave;
- cycle orientation evidence is compatible with the source-facet orientation; and
- any uncertainty capable of changing boundary connectivity is within a specifically authorized downstream degeneracy case or causes failure.

The component may preserve zero-measure overlaps and repeated-coordinate contacts for Components 12 and 13. It must not preserve a definitely self-crossing positive-area boundary as though it were a valid polygon.

### 2.15 Vertex-link feasibility verification

After cycle construction, the component must reconstruct the incident halfedge link around every output vertex occurrence.

For an ordinary occurrence, the link must form one closed cycle consistent with the Component 10 fan requirement. For an explicitly tagged degenerate occurrence, the link must still have a complete expected combinatorial structure and a documented cleanup obligation.

The verification must detect:

- two disconnected fans assigned to one vertex occurrence;
- a missing incoming or outgoing face sector;
- duplicate halfedge incidence;
- a fan transition crossing a topology-separation boundary;
- inconsistent multiplicity; and
- a mismatch between endpoint edge occurrences and face-cycle successor relations.

The component must not merge disconnected links because their coordinates coincide.

### 2.16 Complete edge and cycle balance

Before publication, the component must verify all of the following exact counts and member sets:

- each planned edge occurrence produced exactly one paired-edge record;
- each paired edge has exactly two reciprocal halfedges;
- each retained boundary incidence is consumed by exactly one halfedge;
- each halfedge belongs to exactly one face cycle;
- each transparent internal incidence is consumed exactly once in region grouping and produces no boundary edge;
- each cycle halfedge's incident region matches the cycle owner;
- starts and ends balance on every source-edge and carrier domain;
- each region boundary multiset equals the union of its cycles; and
- no extra output vertex, edge, halfedge, region, or cycle exists without predecessor lineage.

Verification must compare canonical identities, not only totals.

### 2.17 Empty-result behavior

A valid operation whose Component 10 retained-surface complex is empty must produce a valid empty `polygonal_output_complex<T>`.

The empty artifact must contain:

- no output vertices, edges, halfedges, regions, or cycles;
- complete predecessor and policy references;
- zeroed resource and topology counts;
- a deterministic digest; and
- verification evidence that no retained incidence was omitted.

An empty result is not a failure and must not allocate a dummy vertex or face.

### 2.18 Deterministic construction and parallel merge

Parallel work may prepare task-local occurrence records, region membership edges, edge-pair proposals, or per-region cycle data. Publication must:

- canonicalize vertex occurrence keys before ID assignment;
- canonicalize retained-region member sets before region ID assignment;
- canonicalize edge occurrence keys before paired-edge and halfedge ID assignment;
- merge carrier and source-edge realizations by full lineage keys;
- derive successor relations from canonical fan evidence;
- canonicalize cycle rotations and contour trees;
- choose the same primary failure under every schedule; and
- commit only after complete independent checks.

Hash iteration order, pointer address, allocator behavior, union root, task completion order, cycle discovery start, and worker count must not affect IDs, bytes, diagnostics, or digest.

### 2.19 Resource limits and pathological arrangements

The component must account separately for:

- output vertex occurrences;
- occurrence-to-incidence reverse mappings;
- retained-region graph edges and members;
- paired edges and halfedges;
- cycle halfedge references;
- contour and nesting records;
- zero-measure and degeneracy descriptors;
- sorting and verification work;
- diagnostics and replay storage; and
- persistent artifact bytes.

High-valence event clusters, many equal-parameter events, deep contour nesting, or large coincident multiplicity may be output-sensitive and large. The component must fail with `resource_limit` rather than drop cycles, merge occurrences, truncate provenance, or publish unpaired edges.

### 2.20 Cancellation and transactionality

Occurrence allocation, face-region grouping, boundary extraction, edge-pair creation, successor construction, cycle walking, contour classification, admissibility checks, and verification must occur in one stage transaction or private subtransactions that publish one final immutable artifact.

Cancellation must be polled at deterministic safe points during each potentially large canonical sort, grouping pass, carrier domain, region boundary build, cycle extraction, contour analysis, and verification pass.

On cancellation, all workers must join, reservations must return, and no partial vertex table, halfedge pair, or face cycle may be visible. The result is `cancelled`.

### 2.21 Independent verification evidence

The component must publish enough evidence for an independent verifier to reconstruct and check:

- the one-to-one mapping from Component 10 occurrence requirements to output vertex occurrences;
- the bounded coordinate lineage of every vertex occurrence;
- the one-to-one mapping from edge occurrence requirements to paired edges;
- reciprocal endpoints and halfedge pairs;
- exact retained-incidence consumption;
- transparent-continuation grouping;
- region boundary multisets;
- successor and predecessor relations;
- every face cycle and canonical rotation;
- contour roles and nesting;
- source-edge and carrier start/end balance;
- output vertex links;
- zero-measure and degeneracy tags;
- deterministic ID and digest inputs; and
- absence of coordinate-based identity or adjacency.

For bounded fixtures, the verifier must rebuild regions, edge pairs, cycles, and vertex links from predecessor member tables. It must not call the producer's region grouper, edge-pair allocator, successor chooser, or cycle walker as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `polygonal_output_complex<T>` artifact containing or referencing:

- artifact, occurrence, paired-edge, halfedge, face-region, face-cycle, contour, and serialization versions;
- canonical output vertex occurrence records and IDs;
- one authoritative bounded coordinate and precision-ledger reference per vertex occurrence;
- canonical paired-edge records;
- exactly two reciprocal output halfedges per paired edge;
- canonical retained polygonal face-region records;
- complete region-member retained-use mappings;
- oriented face cycles with canonical rotations;
- outer/hole/nesting classifications or explicitly tagged deferred degeneracy states;
- whole-source-edge, split-source-edge, transverse-carrier, coplanar-boundary, and other supported edge-role records;
- exact retained-incidence and transparent-continuation consumption tables;
- source, shell, facet, triangle-group, atom, event, carrier, retained-use, occurrence, and caller provenance;
- zero-nominal-length edge and repeated-coordinate descriptors;
- vertex-link, edge-balance, cycle-closure, and contour-admissibility evidence;
- resource and structural statistics;
- independent-verification evidence;
- canonical input and output digests; and
- replay metadata sufficient to reproduce every allocation, pairing, continuation, and cycle decision.

The artifact must guarantee:

- every Component 10 vertex occurrence requirement is realized exactly once or rejected with a typed failure;
- every Component 10 edge occurrence requirement is realized as exactly one paired edge with two opposite directed face uses;
- every retained boundary incidence is consumed exactly once;
- every output halfedge has a reciprocal pair from the moment it enters the proposed artifact;
- every output halfedge belongs to exactly one face cycle;
- every face cycle closes and does not reuse a directed edge;
- every output vertex occurrence has exactly the one complete closed local fan prescribed by Component 10;
- transparent atom boundaries do not appear as result edges;
- selected carrier and topology-separation boundaries do appear with the required multiplicity;
- coordinate-coincident but topology-distinct occurrences remain distinct;
- no topology was inferred from tolerance or coordinate proximity;
- no coordinate was moved or recomputed;
- IDs, diagnostics, and digest are independent of traversal and schedule; and
- Component 12 can triangulate every positive-area face region without repeating Boolean selection, event construction, edge pairing, or face-cycle reconstruction.

On failure, no polygonal output artifact is published. The typed error must identify the retained use, occurrence requirement, source edge or carrier, endpoint event/source vertices, face region, cycle walk, contour relation, numerical bounds, resource counters, policy versions, and deterministic replay payload relevant to the failure.

## 4. Required invariants and prohibited behavior

Required invariants:

- one output vertex occurrence per authoritative Component 10 occurrence requirement;
- one bounded coordinate lineage per output vertex occurrence;
- one paired edge per authoritative Component 10 edge occurrence requirement;
- exactly two reciprocal halfedges per paired edge;
- exact reversed topological endpoints across every pair;
- no halfedge published without its mate;
- every retained boundary incidence consumed exactly once;
- every transparent continuation consumed exactly once and omitted from the boundary;
- every halfedge belongs to exactly one closed oriented face cycle;
- every output vertex occurrence has one closed incident face fan, including geometrically degenerate occurrences;
- holes and nested contours are explicit rather than encoded by crossed cycles;
- equal coordinates never imply shared vertices, edges, cycles, or regions;
- carrier and source-edge order comes from canonical predecessor lineage;
- no stage-local geometric cleanup occurs;
- all artifacts are immutable, context-owned, transactional, deterministic, and independently verifiable; and
- ambiguity that can change positive-area topology causes typed failure rather than heuristic pairing.

Prohibited behavior:

- pairing edges by nearest endpoints, equal coordinates, or approximate collinearity;
- allocating one output vertex for several Component 10 fan requirements because coordinates match;
- allocating several output vertices for one fan requirement because several faces consume it;
- creating a halfedge before its opposite use has been validated and reserved;
- repairing unbalanced carrier counts by duplicating or deleting incidences;
- re-sorting events from nominal coordinates instead of using Component 08 order;
- crossing a topology-separation delimiter while forming a face region or cycle;
- merging coplanar retained regions solely to reduce face count;
- choosing cycle continuation solely by nominal turning angle at a repeated coordinate;
- discarding zero-length topological edges without a Component 13 cleanup certificate;
- reversing a retained face orientation to simplify contour classification;
- publishing a self-crossing positive-area face boundary as a valid polygon;
- assigning IDs from pointer addresses, hash order, worker timing, or cycle discovery order;
- publishing partial topology after cancellation or resource exhaustion; or
- calling an external graph, polygon, arrangement, mesh, exact-arithmetic, or geometry library.

## 5. Test and validation specification

### 5.1 Vertex occurrence allocation tests

Construct authoritative occurrence requirements for:

- one ordinary source vertex fan;
- one canonical event shared by many source triangles;
- two point-touching components at one bit-identical coordinate;
- two edge-touching shells with separate endpoint fans;
- several coincident sheets with identical coordinates and separate multiplicity;
- a source vertex and event that round to the same coordinate but remain distinct;
- high-valence event clusters; and
- occurrence-key hash collisions.

Verify exact one-to-one allocation, preserved bounded coordinate lineage, closed fan membership, stable IDs, and no coordinate-based welding.

### 5.2 Paired-edge unit tests

Include edge occurrence fixtures for:

- one whole retained source edge;
- one source edge split by one event;
- several ordered events on one source edge;
- one transverse carrier interval;
- a closed carrier loop;
- a carrier chain closing through source edges;
- a coplanar overlap boundary;
- equal-parameter event clusters;
- zero-nominal-length topology-distinct intervals;
- two multiplicity copies of one geometric interval; and
- four geometric face uses partitioned by Component 10 into two edge occurrences.

Verify reciprocal pair references, reversed endpoint occurrences, exact two-use incidence, provenance, and deterministic IDs.

### 5.3 Retained-region grouping tests

Test continuation across:

- uncut source edges;
- transparent source-facet triangulation diagonals;
- classification decomposition seams;
- coincident owner partition seams; and
- several atoms forming one concave retained region.

Test prohibited continuation across:

- selected transverse carriers;
- point-only and edge-only contacts;
- incompatible multiplicity occurrences;
- opposite retained orientation;
- coincident non-owner separation; and
- a boundary where result occupancy changes in another local fan.

Independently compare canonical member sets and region boundary multisets.

### 5.4 Face-cycle known-answer tests

Commit exact cycles for:

- a triangle;
- a convex polygon;
- a concave polygon;
- an annulus;
- a polygon with several holes;
- nested contours and islands;
- several disconnected retained regions in one source facet;
- a thin corridor;
- repeated projected coordinates;
- several topological vertices at one coordinate;
- a zero-length boundary edge; and
- a high-valence carrier event where fan partitions control successor selection.

Verify closure, unique halfedge consumption, orientation, canonical rotation, contour role, and reverse reconstruction from halfedge records.

### 5.5 Carrier balance and ordering tests

Include:

- multiple intersections along one source edge;
- several independent loops on one face pair;
- equal projected parameters with distinct event IDs;
- exact endpoint events;
- near-overlapping parameter envelopes with a valid lineage tie order;
- a definite order reversal mutation;
- one missing start;
- one duplicated end;
- a wrong occurrence partition; and
- a crossed carrier pairing.

The stage must accept only the cases whose predecessor order and bounded admissibility are consistent.

### 5.6 Hole and nesting tests

For deterministic source-facet frames, test:

- one outer contour and one hole;
- several sibling holes;
- a hole containing an island represented as a separate region;
- deeply nested alternating contours;
- equal-coordinate contour touch with topology separation;
- contours sharing a zero-length segment occurrence;
- contour order permutations; and
- an uncertainty-overlap that makes positive-area containment genuinely indeterminate.

Verify explicit contour trees or canonical region splitting. The indeterminate positive-area case must fail rather than choose a nominal containment.

### 5.7 Duplicate-coordinate and zero-measure tests

Include:

- point-touching union components;
- edge-touching union components;
- coincident shells retained as separate occurrences;
- distinct events with one rounded coordinate;
- repeated vertices in a cycle through distinct occurrence IDs;
- zero-nominal-length paired edges; and
- a zero-measure cycle attached to otherwise positive-area regions.

Verify complete topology is preserved for Component 12/13 and no automatic welding or deletion occurs.

### 5.8 Independent topology verification tests

For every valid fixture, independently reconstruct:

- occurrence-to-incidence maps;
- paired edges from Component 10 edge requirements;
- retained-region equivalence classes;
- boundary multisets;
- successor/predecessor maps;
- face cycles;
- vertex links;
- contour membership; and
- source-edge/carrier balance.

The verifier must use separately implemented traversal and set-comparison code.

### 5.9 Mutation tests

Corrupt valid artifacts by:

- merging two coordinate-equal vertex occurrences;
- splitting one occurrence without authorization;
- changing one bounded coordinate reference;
- deleting one halfedge;
- changing a pair reference;
- reversing only one endpoint pair;
- assigning three uses to one edge;
- consuming one retained incidence twice;
- leaving one incidence unused;
- crossing a topology-separation boundary in region grouping;
- selecting the wrong successor at a high-valence event;
- breaking one cycle;
- reusing one halfedge in two cycles;
- changing an outer contour to a hole;
- scrambling canonical IDs; and
- forging counts or digests.

Independent verification must reject every mutation.

### 5.10 Metamorphic and determinism tests

Apply:

- source vertex, facet, triangle, shell, and component permutations;
- facet ring rotation;
- legal source subdivision and re-triangulation;
- operand exchange with operation remapping;
- axis permutation;
- sign flip with corrected orientation;
- exactly representable translation;
- power-of-two scaling with precision scaling;
- thread counts 1, 2, and maximum;
- forced task delays;
- reversed union roots;
- reversed carrier discovery;
- reversed cycle walk starts; and
- contour input permutations.

For a fixed policy version, canonical occurrence IDs, edge pairs, halfedge IDs, region member sets, cycles, contour trees, diagnostics, and digest must be byte-identical after documented remapping.

### 5.11 Fuzzing and shrinking

Generate valid retained-surface complexes with controlled:

- face-region concavity;
- hole and nesting depth;
- carrier loop count;
- event valence;
- equal-parameter clusters;
- point- and edge-touch multiplicity;
- coordinate duplication;
- zero-nominal-length edges;
- transparent seam graphs;
- occurrence partition count; and
- resource limits.

Compare bounded cases against exhaustive incidence pairing and cycle-cover oracles. Every crash, nonclosure, duplicate use, accidental weld, nondeterministic cycle, contour disagreement, or verifier mismatch must serialize predecessor artifacts and shrink while preserving the failure.

### 5.12 Resource, cancellation, and concurrency tests

For occurrences, reverse maps, region members, continuation edges, paired edges, halfedges, cycle references, contour nodes, temporary bytes, work units, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during occurrence allocation, region grouping, boundary extraction, source-edge realization, carrier realization, successor construction, cycle extraction, contour classification, and verification. Confirm all workers join, reservations return, and no partial topology is visible.

### 5.13 Definition of done

Component 11 is complete only when:

- every Component 10 occurrence requirement is realized exactly once;
- every planned edge occurrence is born as one reciprocal halfedge pair;
- every retained boundary incidence is consumed exactly once;
- every transparent continuation is consumed exactly once and omitted from the boundary;
- every halfedge belongs to exactly one closed oriented face cycle;
- every output vertex occurrence has one closed fan, including geometrically degenerate occurrences;
- holes, nested contours, equal coordinates, and zero-length topological edges are represented explicitly;
- carrier ordering ambiguity never causes heuristic pairing;
- independent reconstruction and mutation tests are effective;
- deterministic replay is byte-stable across traversal and schedules;
- the artifact is a complete topology-only input to Component 12; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
