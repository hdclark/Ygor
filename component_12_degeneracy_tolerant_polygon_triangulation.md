# Component 12: Degeneracy-Tolerant Polygon Triangulation

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete polygon decomposition, hole-bridging provider, monotone partition, ear-selection strategy, active-ring storage, projection representation, local predicate escalation, and deterministic work queue may change. Complete boundary preservation, pair-at-creation internal diagonals, orientation preservation, bounded noncrossing decisions, deterministic termination, explicit degeneracy handoff, coverage verification, and failure contracts in this document are normative.

## 0. Purpose

This component converts the verified polygonal manifold produced by Component 11 into an explicitly triangulated internal surface while preserving the exact cross-face topology already established.

Its purposes are to:

- triangulate every positive-area polygonal face region, including concave regions and regions with holes or nested contours;
- preserve every Component 11 boundary halfedge, paired edge, output vertex occurrence, occurrence separation, and face-region orientation;
- add internal diagonals only as reciprocal halfedge pairs owned by one face-region triangulation;
- use deterministic bounded planar predicates that distinguish definite orientation, exact tie, uncertainty, and invalid geometry;
- tolerate repeated nominal coordinates, topology-distinct zero-length boundary edges, near-collinear chains, narrow corridors, and other bounded degeneracies without coordinate-based welding;
- escalate uncertain local decisions through deterministic aggregate evidence rather than making inconsistent per-ear guesses;
- emit explicit cleanup-required triangles, loops, chains, and local obligations when a topologically valid boundary cannot be represented entirely by definite positive-area triangles without authorized simplification;
- prove boundary conservation, triangle orientation, noncrossing internal adjacency, and complete region coverage under the bounded model; and
- provide Component 13 with a deterministic paired triangulated complex and complete certificates for every remaining degeneracy.

The component triangulates without changing the selected Boolean topology. It does not repeat classification or selection, change occurrence partitions, move vertices, collapse edges, weld coordinates, remove components, spend cleanup displacement budget, merge coplanar result faces, or serialize the public mesh.

The principal output is an immutable `triangulated_output_complex<T>` containing:

- the unchanged Component 11 output vertex occurrences and bounded coordinates;
- the unchanged paired region-boundary edges and halfedges;
- canonical internal diagonal pairs;
- canonical oriented triangle records covering every positive-area face region;
- explicit cleanup-required degenerate triangles or residual local patches where permitted;
- exact maps from polygonal regions and cycles to triangles and diagonals;
- projection, predicate, coverage, orientation, and noncrossing evidence;
- complete provenance and boundary-conservation ledgers; and
- resource, verification, digest, and replay metadata.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `polygonal_output_complex<T>` from Component 11;
- the immutable `retained_surface_complex` from Component 10 for orientation, occurrence, and selected-boundary audit references;
- source-facet support frames, source polygon provenance, and internal-diagonal/source-edge distinctions from Components 04 and 05;
- the immutable `canonical_intersection_complex<T>` from Component 08 when event/carrier lineage is needed for bounded projection or repeated-coordinate audit;
- the immutable `precision_context<T>`, bounded orientation/intersection/residual services, precision ledger, and tolerance-budget read services from Component 03;
- the immutable Boolean context, output and degeneracy policies, identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, and transaction services from Component 01;
- the selected projection, contour, triangulation, diagonal, degeneracy, artifact, and serialization versions; and
- verification settings controlling scalable coverage checks and exhaustive bounded triangulation oracles.

The component must not read mutable caller meshes, recompute intersection coordinates, change Component 11 vertex or edge identities, infer polygon connectivity from coordinate proximity, or call an external polygon triangulator.

### 1.2 Required predecessor guarantees

The component may rely on Component 11 having established:

- one canonical output vertex occurrence per Component 10 occurrence requirement;
- one authoritative bounded coordinate and precision envelope per output vertex occurrence;
- reciprocal paired boundary halfedges with exact reversed topological endpoints;
- complete retained boundary-incidence consumption;
- canonical positive-area face regions with prescribed orientation;
- one or more closed boundary cycles per region;
- explicit contour roles and nesting where determined;
- zero-nominal-length and repeated-coordinate descriptors;
- no definitely self-crossing positive-area boundary omitted from the arrangement;
- one closed local fan per output vertex occurrence, including geometrically degenerate occurrences; and
- deterministic canonical ordering and digests.

The component must defensively verify owner tokens, versions, index ranges, pair reciprocity, cycle closure, cycle membership, contour hierarchy, source-facet support, bounded coordinate validity, orientation convention, and predecessor digests.

A contradiction in a committed Component 11 artifact is an `internal_invariant_error`. The triangulator must not repair an open cycle, create a missing boundary edge, reverse a region, or merge contour vertices to obtain a triangulable polygon.

### 1.3 Accepted polygonal region domain

A face region is accepted for triangulation when:

- its positive-area support is planar within the inherited precision contract of its source-facet semantic region;
- its boundary consists of one or more closed oriented cycles supplied by Component 11;
- outer, hole, island, and deferred zero-measure roles are explicit enough to identify the intended positive-area domain;
- nonincident positive-area boundary segments have no definite unrepresented crossing;
- all boundary vertices have finite bounded coordinates;
- all boundary halfedges remain topologically paired with adjacent face-region halfedges; and
- any geometric degeneracy is either locally bounded within the accepted degeneracy policy or must produce a typed failure.

The accepted domain may include:

- convex and concave polygons;
- multiple holes;
- deep contour nesting represented as separate regions or an explicit contour tree;
- repeated nominal coordinates with distinct topological occurrence IDs;
- zero-nominal-length boundary edges with distinct endpoint occurrences;
- collinear or nearly collinear boundary chains;
- narrow corridors and needle-like regions;
- equal projected parameters with deterministic lineage order; and
- weak self-touch caused solely by topology-distinct coordinate coincidence.

The component must reject or fail on a definitely positive-area self-crossing boundary, unresolved contour topology, nonplanarity beyond the precision/tolerance contract, or a bounded ambiguity for which different triangulations would represent different positive-area regions beyond authorized tolerance.

### 1.4 Boundary preservation precondition

Every Component 11 boundary halfedge entering one face region is immutable as a topological boundary use.

Triangulation may attach that halfedge to a triangle or an explicit cleanup-required local patch, but it must not:

- delete it;
- reverse it;
- change its endpoint occurrences;
- replace it with a coordinate-equal edge;
- move it to another face region;
- alter its reciprocal cross-face pair; or
- consume it more than once.

An edge that later disappears must do so only through Component 13's certified topological cleanup.

### 1.5 Projection and support preconditions

Each positive-area face region must provide or permit deterministic recovery of an oriented planar support frame consistent with its retained output orientation.

The frame must contain:

- a bounded origin;
- two in-plane basis directions or an equivalent projection mapping;
- a documented orientation relation between projected 2D orientation and output 3D orientation;
- inherited precision and conditioning metadata;
- source-facet or canonical support provenance; and
- a stable frame key and version.

Projection is a numerical representation for triangulation, not a new topology source. Distinct output occurrences remain distinct when their projected coordinates match.

### 1.6 Capacity and lifetime preconditions

Before triangulation begins, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all region-local corner occurrences;
- all active contour/ring nodes and reverse maps;
- all bridge, partition, ear, and candidate records;
- all internal diagonal pairs and two halfedges per diagonal;
- all output triangles and corner references;
- all cleanup-required degeneracy records;
- all projection and bounded-predicate evidence;
- all boundary, area, coverage, and noncrossing verification data;
- all temporary sorting, spatial indexing, queue, graph, diagnostics, and replay storage; and
- worst-case work up to configured region, contour, corner, candidate, diagonal, triangle, and predicate limits.

Published records may reference only immutable predecessor storage and immutable stage-owned storage whose lifetime covers Components 13-15.

## 2. Required behavior

### 2.1 Per-region transactional triangulation

Each face region may be triangulated in private task-local storage, but no region result may enter the proposed artifact until:

- all of its boundary halfedges are accounted for;
- all added internal diagonals are paired;
- all triangle orientations and local incidences are checked;
- all cleanup-required records are complete;
- all resource reservations are committed; and
- the region triangulation passes verification.

The full stage publishes only after every region succeeds and the global shared-boundary audit passes. Failure of one region rolls back the entire Component 12 stage.

### 2.2 Deterministic support frame and projection

The component must use the authoritative source-facet or canonical support frame when available. If a frame must be constructed, the construction must be deterministic, bounded, and based on stable lineage rather than arbitrary coordinate extrema.

Frame selection must:

- preserve the region's output orientation;
- avoid a projection direction whose uncertainty permits collapse of a definitely positive-area source region;
- include projection roundoff in every 2D enclosure;
- prescribe rounding points to `T` or a versioned internal bounded scalar representation;
- handle large translations, extreme exponents, subnormals, and signed zero; and
- produce the same frame under source triangle permutations and worker schedules.

A frame whose conditioning is insufficient to distinguish required positive-area topology within tolerance causes `geometric_condition_exceeds_tolerance`.

### 2.3 Region-local topological model

The triangulator must create a region-local topological model that preserves:

- each boundary halfedge as a distinct immutable directed item;
- each output vertex occurrence as a distinct topological corner even when projected coordinates match;
- contour order and role;
- hole and island relationships;
- repeated visits to the same coordinate through different occurrence IDs;
- zero-nominal-length edges;
- source/carrier edge roles; and
- provenance back to the Component 11 face cycle.

The local model may introduce temporary bridge duplicates or traversal nodes, but temporary identities must not escape as published output vertices. Every published triangle corner must reference an existing Component 11 output vertex occurrence unless a future version explicitly authorizes a new bounded construction through a separate component contract.

### 2.4 Bounded planar predicate model

Every topology-affecting planar test must return a structured bounded result rather than a bare Boolean.

Required queries include:

- orientation of three projected occurrences;
- side of a point relative to a directed segment;
- proper, endpoint, overlap, tied, uncertain, or absent segment intersection;
- visibility of a candidate diagonal or bridge;
- point/sector membership for a candidate ear;
- projected signed-area bounds for cycles and triangle sets;
- ordering along a projected carrier when independently audited; and
- definite separation of nonadjacent edges.

Each result must retain nominal value, conservative enclosure, provenance, conditioning reason, and deterministic disposition.

Exact floating ties may use only the frozen symbolic or lineage ordering explicitly authorized for triangulation bookkeeping. Numerical uncertainty that could change positive-area coverage must not be treated as an exact tie.

### 2.5 Deterministic orientation escalation

A local three-corner orientation may be uncertain because corners are repeated, nearly collinear, or poorly conditioned. The component must use a frozen escalation sequence before failing or classifying the corner as cleanup-required.

A permitted provider-independent escalation sequence may include:

- the direct bounded orientation of the immediate predecessor, corner, and successor;
- bounded orientation using farther distinct topological neighbors on one or both sides;
- a bounded aggregate turn over a maximal collinear/near-collinear chain;
- bounded signed area of the containing contour segment or remaining region;
- source-facet orientation and arrangement incidence evidence; and
- an explicitly authorized symbolic tie rule for exact representational equality.

The exact sequence must be versioned and deterministic. Different consumers must not choose different algebraic formulations for the same canonical decision.

If aggregate evidence proves the corner contributes no resolvable positive area and its removal would require topology or geometry change, the corner remains represented through a cleanup-required record. If evidence remains uncertain and alternative decisions can change positive-area coverage beyond tolerance, triangulation fails.

### 2.6 Hole and nested-contour integration

A provider may bridge holes, form a constrained planar subdivision, use monotone decomposition, or use another in-tree method. Regardless of provider, it must preserve the Component 11 contour domain exactly.

Any temporary or published bridge/diagonal must:

- connect existing output vertex occurrences;
- lie in the accepted face region under bounded visibility tests;
- avoid every nonincident boundary segment except explicitly permitted endpoint or topology-distinct equal-coordinate contact;
- preserve contour orientation and hole exclusion;
- be selected by a deterministic complete key;
- be introduced as a reciprocal internal halfedge pair if published; and
- carry the contour and visibility evidence that justified it.

A hole must not be filled, dropped, or merged with an outer boundary because its area is small. Feature removal belongs to Component 13 and requires a budget certificate.

If no admissible bridge or decomposition exists without making an uncertain positive-area crossing, the component must fail or emit only the specifically supported cleanup-required local structure; it must not choose the nominally shortest bridge.

### 2.7 Candidate diagonal admissibility

A candidate internal diagonal is admissible only when all of the following hold:

- its endpoint occurrence IDs are permitted in the same face region;
- it is not an existing boundary edge unless the provider is accounting for that boundary use;
- its open interior is definitely inside the intended polygonal region or is an exact topology-preserving tie handled by policy;
- it has no forbidden proper crossing or positive-length overlap with a nonincident boundary or accepted diagonal;
- its insertion preserves the region's combinatorial disk decomposition;
- its two directed uses can be paired at creation time;
- its orientation contribution is compatible with output face orientation; and
- its geometric uncertainty is within the accepted policy.

Diagonal identity must be based on region, endpoint occurrence IDs, decomposition role, and complete lineage. Equal endpoint coordinates do not merge diagonals from different regions or occurrences.

### 2.8 Ear, partition, or cell acceptance

Regardless of the chosen triangulation provider, every accepted triangle must be justified by a local record equivalent to:

- three distinct output vertex occurrence IDs;
- three directed boundary or internal-diagonal halfedge uses forming a closed cycle;
- output orientation;
- bounded projected orientation result;
- candidate-domain membership;
- absence of excluded contour interior;
- absence of forbidden crossings;
- provenance to the consumed polygonal region; and
- deterministic candidate key and acceptance order.

A provider using monotone cells or constrained subdivisions may not literally select ears, but it must publish equivalent per-triangle evidence.

Candidate acceptance must not depend on first-found hash order, floating sort instability, or worker timing.

### 2.9 Internal diagonal pair creation

Every published internal diagonal must be created atomically as one paired edge with two reciprocal halfedges.

The pair must:

- have exact reversed endpoint occurrence IDs;
- be incident to exactly two triangles in the same polygonal face region;
- be marked as an internal triangulation diagonal, never a source or Boolean boundary edge;
- carry bounded visibility/noncrossing evidence;
- preserve region orientation on both incident triangles;
- have a canonical key independent of insertion order; and
- be removable from source-facet semantic reasoning by downstream verifiers.

A temporary bridge that becomes a published diagonal follows the same rule. A temporary traversal duplication that is not a real internal edge must not appear in the final triangulated artifact.

### 2.10 Boundary halfedge assignment

Every Component 11 boundary halfedge must be assigned to exactly one triangle or one explicit cleanup-required local patch on its incident face-region side.

The assignment must preserve:

- the original halfedge ID and reciprocal cross-face pair;
- direction;
- endpoint occurrences;
- edge role and provenance;
- incident polygonal face region; and
- membership in the original face cycle.

After triangulation, the halfedge's incident face reference may point to a triangle rather than the polygonal region, but the region mapping must remain recoverable.

No boundary halfedge may be consumed by two triangles, left unassigned without a degeneracy record, or replaced by an internal diagonal.

### 2.11 Repeated coordinates and zero-length boundary edges

A zero-nominal-length boundary edge with distinct topological endpoints remains a boundary constraint.

The triangulator must:

- retain both endpoint occurrence IDs;
- preserve the edge and its cross-face pair;
- include it in the region boundary-conservation ledger;
- avoid using coordinate equality to remove either endpoint;
- prevent infinite candidate cycling around repeated coordinates;
- classify adjacent orientation using the deterministic escalation sequence; and
- either attach the edge to a valid positive-area triangle or emit a cleanup-required local triangle/patch that keeps the complete paired topology.

A candidate triangle may be cleanup-required when its topological corners are distinct but its bounded area is zero or below the policy threshold. A face with repeated topological vertex ID in one triangle is prohibited unless a future explicitly versioned residual-patch representation supports it; ordinary triangle records must use three distinct occurrence IDs.

### 2.12 Triangle geometric categories

Every produced triangle must receive exactly one geometric category:

- `definite_positive_area`: bounded orientation has the required sign with a margin above the accepted degeneracy floor;
- `positive_area_within_cleanup_margin`: orientation is consistent, but area/altitude/edge measures require Component 13 review under tolerance policy;
- `zero_area_cleanup_required`: topology is complete but the bounded area contains only zero or a documented sub-tolerance range;
- `symbolic_exact_tie_cleanup_required`: exact representational degeneracy is retained under frozen topology and must be resolved by cleanup;
- `invalid_inverted`;
- `invalid_uncertain_positive_area_topology`; or
- `invalid_non_finite`.

Only the first four categories may enter the proposed Component 12 artifact. The latter categories cause failure.

The category does not authorize movement or deletion. It defines Component 13 obligations and verification thresholds.

### 2.13 Positive-area overlap and crossing prevention

The component must ensure that triangles within one face region do not have positive-area overlap except at shared edges or vertices, and that internal diagonals do not cross.

A scalable production check may use an in-tree region-local spatial index, sweep, hierarchy, or bounded candidate enumeration. It must be conservative and deterministic.

The check must distinguish:

- shared paired diagonal;
- shared boundary endpoint;
- topology-distinct equal-coordinate contact;
- collinear/overlapping zero-measure degeneracy pending cleanup;
- forbidden proper crossing;
- forbidden positive-area overlap; and
- unresolved uncertainty.

A forbidden crossing or overlap is a failure. An unresolved relation that could change positive-area coverage beyond tolerance is also a failure.

### 2.14 Coverage and conservation verification

For every polygonal face region, the component must verify coverage through independent complementary evidence.

Required evidence includes:

- exact boundary-halfedge conservation;
- internal-diagonal balance, with every diagonal used exactly twice in opposite directions;
- triangle-cycle closure;
- connectivity of the triangulated positive-area domain corresponding to each region component;
- Euler-characteristic consistency for the region's contour topology, accounting for explicitly tagged degeneracies;
- bounded oriented-area agreement between accepted region contours and produced triangles within conservative enclosure combination;
- no forbidden triangle overlap or missing positive-area pocket detected by the selected verification level; and
- complete assignment of cleanup-required zero-measure structures.

Area agreement alone is not sufficient, because overlapping and missing triangles can cancel algebraically. Boundary and noncrossing evidence are mandatory.

### 2.15 Orientation preservation

Every definite positive-area triangle must be oriented consistently with its Component 11 face region and the selected result's outward convention.

Orientation must be verified in the authoritative support frame and, where required, through a bounded 3D normal/plane relation. The component must not reverse an individual triangle merely because a provider generated it clockwise.

If the provider's candidate orientation is reversed, the candidate must be rejected or reconstructed before publication. Internal diagonal pair directions must then remain reciprocal.

Cleanup-required zero-area triangles must retain a prescribed conceptual orientation and incident halfedge order so Component 13 can remove them without guessing the intended side.

### 2.16 Degenerate loop and residual-patch handoff

When a Component 11 cycle or local chain is topologically required but cannot participate in a definite positive-area triangulation without cleanup, the component must emit a complete residual record.

A residual record must identify:

- owning polygonal face region and cycle;
- all boundary halfedges and output vertex occurrences involved;
- local paired-edge and vertex-fan context;
- bounded area, edge-length, and separation evidence;
- whether the structure is a zero-area triangle, zero-length edge fan, collinear chain, pinched contour, or other versioned degeneracy class;
- candidate cleanup operations known to be topologically eligible, if any, without committing to one;
- forbidden merges or occurrence separations that cleanup must preserve;
- provenance and precision-ledger references; and
- a deterministic obligation key.

The combined triangles and residual records must account for the complete Component 11 region boundary. The triangulator must not hide an untriangulated corner in diagnostics only.

### 2.17 No cleanup budget expenditure

Component 12 may read tolerance and degeneracy thresholds to determine whether a residual is potentially eligible for Component 13, but it must not reserve or commit displacement/removal budget.

No output vertex coordinate may change. No edge or component may be removed. The Component 12 precision ledger may add only bounded projection/predicate verification contributions that do not represent geometric displacement.

A polygon that requires more geometric change than the caller could authorize may fail early with a precise condition status, but the stage must not approximate the geometry itself.

### 2.18 Deterministic triangulation order

Publication must be independent of region processing order and worker schedule.

The component must:

- process or merge face regions in canonical region-key order;
- assign projected corner records deterministically;
- order bridge/partition/ear candidates by a total canonical key;
- prescribe tie handling for equal nominal scores;
- assign internal diagonal and triangle IDs after canonical sorting;
- canonicalize triangle corner rotation while preserving orientation;
- merge residual obligations by full lineage key;
- select the same primary failure under every schedule; and
- commit only after global boundary verification.

Geometric optimization metrics such as shortest diagonal, largest ear, or best aspect ratio may be part of a deterministic key only after bounded comparison yields a definite ordering or a stable lineage tie rule resolves an exact tie. They must not create platform-dependent topology.

### 2.19 Termination requirements

The triangulation provider must have a documented monotonic progress measure.

For example, each accepted operation may reduce:

- the number of untriangulated positive-area corners;
- the number of unresolved contour components;
- the number of active cells; or
- a lexicographically ordered finite state tuple.

Walking past uncertain corners without changing state is not progress. Retry queues must have bounded deterministic attempt counts and must record why a candidate was deferred.

If no admissible positive-area triangle, decomposition, or supported residual handoff can reduce the remaining state, the component must fail with a typed geometry status. It must not loop, randomly perturb coordinates, or relax predicates silently.

### 2.20 Empty and triangle-only regions

An empty Component 11 artifact produces an empty triangulated artifact.

A face region already bounded by exactly three distinct occurrence vertices must still be audited. It may become:

- one definite positive-area triangle;
- one cleanup-required triangle; or
- a typed failure.

The component must not bypass orientation, boundary, or precision checks merely because no diagonal is required.

### 2.21 Resource limits and pathological polygons

The component must account separately for:

- projected corner records;
- active contour/ring nodes;
- visibility and orientation candidates;
- bridge and partition records;
- internal diagonal pairs and halfedges;
- triangle records;
- residual/degeneracy obligations;
- local spatial-index nodes and candidate pairs;
- predicate and verification work;
- diagnostics and replay storage; and
- persistent artifact bytes.

A polygon with many holes, many repeated coordinates, or adversarially few valid ears may require superlinear work. The provider must charge abstract work units and fail with `resource_limit` rather than omit tests, accept an unverified diagonal, cap hole count by dropping contours, or publish incomplete triangulation.

### 2.22 Cancellation and transactionality

Projection, contour integration, candidate generation, bounded predicate escalation, diagonal creation, triangle acceptance, residual handoff, coverage checks, and publication verification must occur transactionally.

Cancellation must be polled at deterministic safe points during each large region, contour bridge search, candidate batch, predicate escalation, triangle insertion, local crossing query, and verification pass.

On cancellation, all workers must join, reservations must return, and no partial diagonal or triangle set may be visible. The result is `cancelled`.

### 2.23 Independent verification evidence

The component must publish enough evidence for an independent verifier to reconstruct and check:

- the support frame and projected bounded coordinates;
- boundary halfedge preservation and assignment;
- contour and hole domain;
- each internal diagonal's endpoint, visibility, and pair;
- every triangle's three halfedges, corners, orientation, and region owner;
- internal diagonal two-use balance;
- region boundary conservation;
- Euler and connectivity relations;
- bounded area agreement;
- absence of forbidden crossings and positive-area overlap;
- every cleanup-required residual obligation;
- deterministic candidate and ID ordering; and
- absence of coordinate welding or hidden cleanup.

For bounded fixtures, the verifier must use a separately implemented triangulation/coverage oracle or exhaustive legal-diagonal enumeration where feasible. It must not call the producer's ear test, bridge chooser, active-ring updater, or coverage accumulator as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `triangulated_output_complex<T>` artifact containing or referencing:

- artifact, projection, triangulation, internal-diagonal, triangle-category, degeneracy, and serialization versions;
- all Component 11 output vertex occurrences and authoritative bounded coordinates unchanged;
- all Component 11 paired boundary edges and reciprocal halfedges unchanged in identity and endpoint topology;
- canonical internal diagonal paired-edge records and reciprocal halfedges;
- canonical oriented triangle records and IDs;
- exact triangle-to-region and region-to-triangle mappings;
- boundary-halfedge-to-triangle or residual-obligation assignments;
- internal-diagonal-to-two-triangle assignments;
- support-frame and projected bounded-coordinate records;
- per-candidate or compact shared predicate evidence sufficient for audit;
- triangle geometric categories;
- cleanup-required zero-area triangle, edge, chain, contour, and residual-patch records;
- boundary conservation, connectivity, Euler, area, noncrossing, and overlap verification evidence;
- source, retained-use, face-region, cycle, event, carrier, occurrence, and caller provenance;
- precision-ledger references and proof that no cleanup displacement was spent;
- resource and structural statistics;
- canonical input and output digests; and
- replay metadata sufficient to reproduce every frame, candidate, diagonal, triangle, and residual decision.

The artifact must guarantee:

- every positive-area Component 11 face region is completely triangulated or has only explicitly bounded cleanup-required residual structure;
- every Component 11 boundary halfedge is preserved and assigned exactly once on its incident region side;
- every published internal diagonal is born paired and has exactly two opposite directed triangle uses;
- every triangle uses three distinct output vertex occurrence IDs;
- every definite positive-area triangle has the prescribed output orientation;
- no forbidden internal diagonal crossing or positive-area triangle overlap is present;
- region contour topology and holes are preserved;
- repeated coordinates and zero-length topological edges have not been welded or deleted;
- no output vertex coordinate has moved;
- no Boolean classification, occurrence partition, or cross-face pairing has changed;
- all residual degeneracies are explicit, bounded, and consumable by Component 13;
- IDs, diagnostics, and digest are independent of traversal and schedule; and
- Component 13 can perform cleanup without reconstructing polygon contours or guessing which boundary topology was intended.

On failure, no triangulated output artifact is published. The typed error must identify the face region, cycles, corners, candidate diagonal or triangle, bounded predicate enclosure, contour role, residual state, resource counters, policy versions, and deterministic replay payload relevant to the failure.

## 4. Required invariants and prohibited behavior

Required invariants:

- Component 11 output vertex occurrences and boundary pairs remain unchanged;
- every boundary halfedge is consumed exactly once by a triangle or explicit residual obligation;
- every internal diagonal is a reciprocal pair with exactly two triangle uses;
- every ordinary triangle has three distinct topological corners;
- definite positive-area triangles preserve face-region orientation;
- no positive-area hole is filled or omitted;
- no forbidden diagonal crossing or positive-area triangle overlap is published;
- repeated coordinates remain topology-distinct where their occurrence IDs differ;
- uncertain local orientation follows one frozen escalation sequence;
- cleanup-required geometry is explicit and budget-free at this stage;
- triangulation terminates with success or a typed failure;
- all artifacts are immutable, context-owned, transactional, deterministic, and independently verifiable; and
- ambiguity that can change positive-area coverage causes failure rather than a nominal heuristic choice.

Prohibited behavior:

- using an external polygon, constrained-Delaunay, mesh, exact-arithmetic, or geometry library;
- welding corners because projected coordinates match or are within tolerance;
- deleting zero-length boundary edges during ring preprocessing;
- dropping small holes or contours;
- changing a boundary halfedge's endpoint, direction, pair, or region;
- creating an unpaired internal diagonal;
- accepting a diagonal because it is nominally shortest without bounded visibility;
- classifying an uncertain orientation as zero solely from a scalar epsilon;
- evaluating equivalent predicates independently in different consumers;
- moving vertices or spending cleanup budget;
- reversing triangles after publication without updating paired incidence;
- hiding an untriangulated region in a warning;
- relying on area equality as the only coverage proof;
- allowing retry queues with no monotonic progress;
- assigning IDs from hash order, pointer address, or worker timing; or
- publishing partial triangulation after cancellation or resource exhaustion.

## 5. Test and validation specification

### 5.1 Basic polygon unit tests

Triangulate known-answer regions for:

- one triangle;
- convex polygons of sizes 4 through at least 32;
- concave polygons with one and many reflex vertices;
- orthogonal polygons;
- star-shaped polygons;
- polygons with long collinear chains;
- one-hole annuli;
- several sibling holes; and
- deeply nested contour fixtures represented according to Component 11 policy.

Verify exact boundary preservation, triangle orientation, diagonal pairing, triangle count where topology is nondegenerate, and independent coverage.

### 5.2 Boolean-shaped polygon corpus

The permanent corpus must include polygons emitted by Components 08-11 from:

- box/box transverse intersections;
- oblique convex-polytope intersections;
- concave extrusions;
- partial coplanar overlaps;
- equal operands with different triangulations;
- cavities and islands;
- several carrier loops on one source facet;
- intersection curves passing through original vertices and edges;
- thin corridors; and
- repeated Boolean chains with inherited precision.

Hand-authored simple polygons alone are insufficient qualification.

### 5.3 Repeated-coordinate and zero-length tests

Include:

- two distinct corner occurrences with bit-identical projected coordinates;
- several equal-coordinate occurrences separated around one contour;
- zero-nominal-length boundary edges;
- a repeated-coordinate point touch between outer and hole contours;
- a topology-distinct overlap of two contour vertices;
- equal-parameter event clusters; and
- a collinear chain whose middle vertices have distinct provenance obligations.

Verify termination, no welding, boundary assignment, explicit cleanup obligations, and stable topology.

### 5.4 Narrow and near-degenerate tests

Include features with width, altitude, area, or edge length:

- clearly above tolerance;
- just above the machine/precision floor;
- just above, at, and just below the cleanup policy threshold;
- just below, at, and just above caller tolerance; and
- affected by large coordinate translation or mixed magnitudes.

Verify definite triangles, cleanup-required categories, and failures are separated correctly. Tolerance must not act as a general orientation epsilon.

### 5.5 Hole bridge and decomposition tests

For each supported provider, test:

- several admissible bridges with deterministic tie ordering;
- one narrow visible bridge;
- endpoint-only contact;
- topology-distinct equal-coordinate endpoint contact;
- a nominally short but crossing bridge;
- a bridge whose visibility is uncertain beyond tolerance;
- many holes competing for bridge endpoints; and
- contour permutations.

Verify holes remain excluded, bridges are paired if published, and uncertain positive-area crossings cause failure.

### 5.6 Orientation escalation tests

Construct local corners where:

- immediate orientation is definite;
- immediate orientation is uncertain but farther neighbors prove the turn;
- a collinear chain aggregate proves zero contribution;
- contour area proves orientation;
- source-facet incidence resolves an exact tie;
- symbolic exact equality is eligible for a frozen tie rule; and
- no bounded escalation can distinguish alternative positive-area topology.

Verify one deterministic escalation path and the correct final category or failure.

### 5.7 Boundary and coverage tests

Independently verify for every known-answer region:

- exact boundary-halfedge multiset equality;
- internal diagonal two-use balance;
- triangle adjacency connectivity;
- Euler characteristic accounting for holes;
- bounded oriented-area agreement;
- no forbidden proper crossings;
- no positive-area overlaps; and
- no omitted positive-area cells.

Use exact rational or integer-coordinate test oracles for low-complexity fixtures.

### 5.8 Alternative legal triangulation metamorphic tests

Apply different valid source triangulations, facet subdivisions, ring rotations, contour orderings, projection-axis candidates, and candidate queue permutations.

Where the deterministic output policy freezes one triangulation, the final canonical triangle and diagonal artifacts must be byte-identical. Where a provider version explicitly permits different internal triangulations, compare exact boundary topology, occupied region, coverage evidence, precision, and cleanup obligations, and keep the provider version in the digest.

### 5.9 Mutation tests

Corrupt valid artifacts by:

- deleting one boundary halfedge assignment;
- assigning one boundary halfedge twice;
- changing one boundary endpoint;
- losing a hole contour;
- inserting a crossed diagonal;
- leaving an internal diagonal unpaired;
- pairing a diagonal to the wrong reverse endpoints;
- reversing one triangle;
- duplicating one triangle;
- deleting one triangle while preserving counts;
- creating positive-area overlap;
- changing one triangle category;
- omitting a residual obligation;
- welding repeated-coordinate corners;
- scrambling canonical triangle IDs; and
- forging area or digest evidence.

Independent verification must reject every mutation.

### 5.10 Determinism and numerical metamorphic tests

Apply:

- vertex, cycle, region, facet, shell, and component permutations;
- operand exchange with operation remapping;
- axis permutation;
- sign flip with corrected orientation;
- exactly representable translation;
- power-of-two scaling with precision scaling;
- signed-zero variants where policy permits;
- adjacent-float and subnormal perturbations;
- thread counts 1, 2, and maximum;
- forced task delays;
- reversed candidate insertion;
- reversed active-ring starts; and
- hash-collision injection.

For a fixed provider and policy version, support frames, candidate decisions, diagonal keys, triangle IDs, categories, residual obligations, diagnostics, and digest must be byte-identical after documented remapping.

### 5.11 Fuzzing and shrinking

Generate valid polygonal output complexes with controlled:

- corner count;
- reflex-vertex count;
- hole count and nesting depth;
- repeated-coordinate count;
- zero-length boundary edges;
- collinear-chain length;
- corridor width;
- event/carrier provenance;
- coordinate scale and ULP perturbation; and
- resource limits.

For bounded small cases, compare against exhaustive legal-diagonal triangulation or exact constrained-cell oracles implemented in-tree for tests. Every crash, hang, boundary loss, crossed diagonal, coverage mismatch, nondeterminism, or verifier disagreement must serialize and shrink while preserving the failure.

### 5.12 Performance and structural gates

Measure and assert structural counters for:

- predicate evaluations;
- candidate diagonals;
- bridge visibility tests;
- local crossing candidates;
- accepted and deferred candidates;
- orientation escalation depth;
- spatial-index nodes;
- triangles and residual obligations; and
- abstract work units.

Large ordinary simple polygons must not accidentally trigger exhaustive all-diagonal behavior. Adversarial cases may consume configured work and fail with `resource_limit`, but never skip verification.

### 5.13 Resource, cancellation, and concurrency tests

For projected corners, active nodes, candidates, bridges, diagonals, triangles, residual records, predicate work, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during projection, hole integration, candidate generation, orientation escalation, diagonal insertion, triangle acceptance, crossing checks, residual construction, and coverage verification. Confirm all workers join, reservations return, and no partial triangulation is visible.

### 5.14 Definition of done

Component 12 is complete only when:

- every positive-area Component 11 region is completely and deterministically triangulated;
- every boundary halfedge is preserved and assigned exactly once;
- every internal diagonal is born paired and used exactly twice;
- holes and nested contours are preserved;
- repeated coordinates and zero-length topological edges are handled without welding or deletion;
- uncertain orientation follows a bounded deterministic escalation sequence;
- every remaining degeneracy is explicit and consumable by Component 13;
- no cleanup displacement is spent;
- independent coverage and mutation verification are effective;
- all providers terminate or fail with a typed status;
- deterministic replay is byte-stable across schedules; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
