# Component 09: Connectivity and Winding Classification

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete classification-atom representation, graph storage, union provider, seed-inclusion provider, propagation algorithm, and parallel partitioning may change. The cut-aware connectivity, one-classification-per-group, signed-delta propagation, shell-semantics consistency, deterministic seeding, cycle verification, boundary-state preservation, and failure contracts in this document are normative.

## 0. Purpose

This component determines how every source-surface region of operand A lies relative to the solid represented by operand B, and how every source-surface region of B lies relative to A.

Its purposes are to:

- partition each source surface into classification atoms separated only by authoritative intersection or contact structure;
- connect atoms through exact uncut topology so large untouched regions share one classification;
- form canonical classification groups whose members must have the same opposite-operand winding or inclusion state;
- propagate integer winding changes across cuts using the signed crossing multiplicities from Component 07;
- compute only the deterministic seed classifications needed to anchor disconnected propagation components;
- preserve boundary, tangent, coplanar, coincident, and symbolic contact states rather than coercing them to ordinary inside/outside;
- verify path independence and local/global winding consistency; and
- produce the side-occupancy labels required by Component 10 without repeating a geometric ray test for every vertex, edge, triangle, or output piece.

The component classifies topology induced by the shared intersection complex. It does not construct final output face cycles, allocate final output vertex occurrences, choose retained Boolean surfaces, pair output halfedges, triangulate output polygons, or perform cleanup.

The principal output is an immutable `classification_complex` containing:

- canonical surface classification atoms;
- cut-aware adjacency and signed crossing-delta edges;
- canonical classification groups and membership maps;
- deterministic seed inclusion records;
- integer winding and regularized inclusion values;
- boundary and symbolic side states;
- per-atom opposite-operand side labels for Boolean selection; and
- complete provenance, consistency, verification, resource, digest, and replay evidence.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `canonical_intersection_complex<T>` from Component 08;
- the immutable `signed_feature_relations<T>` from Component 07;
- the immutable `canonical_source_manifolds` from Component 05;
- source-facet triangle groups and source-boundary provenance from Components 04 and 05;
- validated source shell orientation, nesting forest, occupied-side semantics, and source winding contract from Component 02;
- the immutable precision context and bounded predicate/construction services from Component 03;
- the immutable Boolean context, identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, and transaction services from Component 01;
- the selected classification-atom, adjacency, seed-provider, winding, artifact, and serialization versions; and
- verification settings controlling scalable and exhaustive classification checks.

The component must not read mutable caller meshes, infer cuts from coordinate proximity, merge source components because their coordinates touch, or alter Component 07 crossing multiplicities.

### 1.2 Required predecessor guarantees

The component may rely on predecessor artifacts having established:

- exact immutable source halfedge topology for both operands;
- complete source-facet and shell provenance;
- a complete conservative relation disposition for every broad-phase candidate;
- one canonical event identity per event-equivalence lineage;
- distinct occurrence identities for equal-coordinate but topologically separate events;
- complete ordered event sequences and interval partitions on affected source edges;
- canonical transverse and coplanar carrier records;
- authoritative numeric and symbolic crossing contributions;
- cut, tangent, contact, overlap, coincidence, and occurrence-separation descriptors;
- source-facet semantics independent of internal triangulation diagonals; and
- deterministic canonical ordering and digests.

The component must defensively verify owner tokens, versions, source-feature ranges, event and interval references, crossing-delta domains, adjacency preconditions, and artifact digests. Contradictory committed predecessor facts are an `internal_invariant_error`; they must not be repaired by a fallback point-in-solid test.

### 1.3 Classification domain

For each operand surface, the component must classify the complement of the authoritative intersection/contact complex on that surface.

The concrete provider may represent this domain using:

- source vertex sectors;
- source-edge open intervals between ordered events;
- source triangle or source-facet corner sectors;
- source-facet interior regions separated by carrier arcs;
- oriented halfedge-side records;
- a combinatorial arrangement graph; or
- another exact indexed decomposition.

Regardless of representation, the classification atoms must be complete enough that:

- every positive-area source-surface location belongs to exactly one atom away from shared boundaries;
- every intersection curve, contact delimiter, coplanar overlap boundary, and coincident-region boundary appears in atom adjacency;
- each atom has one well-defined opposite-operand classification under the frozen numerical and symbolic policies;
- internal triangulation diagonals do not split a source-facet semantic region unless an authoritative intersection descriptor crosses them; and
- zero-measure contacts required for occurrence separation remain represented.

The provider must publish a reconstruction contract from source topology and Component 08 records so an independent verifier can reproduce the atom domain.

### 1.4 Classification state model

The classification state must distinguish at least:

- strict outside with integer winding zero;
- strict inside with integer winding one under the default regular-solid policy;
- boundary contact with operation-neutral numeric state;
- tangent contact with zero net crossing;
- coplanar overlap boundary;
- coincident same-orientation sheet;
- coincident opposite-orientation sheet;
- symbolic negative-side and positive-side occupancy assignments;
- occurrence-separated point or edge contact;
- unresolved numerical ambiguity; and
- invalid or inconsistent state.

The implementation may preserve a general signed integer winding internally. For ordinary accepted regular solids, every non-boundary classified sample must ultimately map to winding zero or one under the validated solid policy. Values outside the accepted domain require a precise invariant or shell-semantics failure.

### 1.5 Capacity and lifetime preconditions

Before classification begins, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all classification atoms and source-feature memberships;
- all cut-aware adjacency edges and signed delta records;
- all union/group records and reverse maps;
- all propagation components and seed queries;
- all winding and boundary-side labels;
- all cycle-consistency and local-conservation evidence;
- all temporary graph, sorting, queue, union, and verification work;
- all diagnostics and replay data; and
- worst-case work up to configured atom, adjacency, group, seed, and propagation limits.

Published classification records may reference only immutable predecessor artifacts and immutable stage-owned storage whose lifetime covers Components 10-15.

## 2. Required behavior

### 2.1 Classification atom construction

The component must construct a canonical set of classification atoms for each operand surface.

Each atom must identify, directly or through immutable tables:

- operand and shell;
- source facet and source triangles contributing geometry;
- source vertices, source-edge intervals, event clusters, carrier arcs, and contact boundaries incident to the atom;
- orientation inherited from the source surface;
- whether the atom has positive area, is a boundary sector, or is a zero-measure occurrence descriptor;
- all Component 07/08 relation lineage that may affect classification; and
- a stable canonical key and ID.

Atom construction must be combinatorial and lineage-driven. It must not split or merge atoms because coordinates are close, because nominal carrier segments appear collinear, or because two event points round to one coordinate.

A valid empty operand must produce an empty atom set. A non-empty source surface with no cross-operand events must still produce enough atoms to classify each disconnected source-surface component.

### 2.2 Cut-aware adjacency

The component must build adjacency between classification atoms using exact source topology and Component 08 cut/contact descriptors.

Each potential adjacency must be classified as one of:

- uncut continuation with winding delta zero;
- crossing interface with a signed integer winding delta;
- tangent/contact continuation with zero delta;
- symbolic delimiter with operation-neutral and symbolic side metadata;
- coplanar overlap boundary;
- coincident-sheet adjacency;
- topology-separated point or edge contact;
- transparent internal triangulation adjacency;
- intentionally absent adjacency; or
- invalid/unresolved.

An adjacency may cross a source edge, a source-facet internal diagonal, a carrier arc, an event sector, or a source vertex fan transition. Its exact endpoints and local orientation must be independently recoverable.

Coordinate equality never creates adjacency. Two disconnected shells that touch at a point or edge remain separate atom graphs unless the source topology already connects them and the regularized contact policy authorizes continuation.

### 2.3 Transparent internal triangulation

Facet-internal triangulation diagonals are transparent to source-facet classification unless intersected by an authoritative event or carrier whose lineage crosses the source facet.

The component must:

- union atoms across uncut internal diagonals;
- preserve the common source-facet semantic region;
- avoid creating crossing deltas from bookkeeping edges;
- route source-facet carriers across triangle boundaries without changing classification; and
- verify legal alternative triangulations yield equivalent source-facet atom groups and labels.

An internal diagonal may provide local incidence for reconstruction but cannot be a reason to create a new classification group.

### 2.4 Crossing-delta derivation

Every adjacency that changes opposite-operand winding must obtain its signed delta from authoritative Component 07 relation records and Component 08 event/carrier aggregation.

The component must not recompute crossing signs through an independent edge-plane test.

For each directed adjacency, the record must contain:

- source and destination atom IDs;
- canonical local traversal orientation;
- signed numeric crossing delta;
- symbolic delta or side-order metadata where applicable;
- event, carrier, source-facet, shell, and relation provenance;
- local entering/leaving order; and
- a reverse adjacency whose delta is the exact additive inverse.

Tangent and zero-measure contact adjacencies normally have zero numeric delta. Coincident or exact-tie adjacencies may carry symbolic side assignments without pretending a geometric displacement occurred.

### 2.5 Zero-delta connectivity and classification groups

Atoms connected through exact uncut or permitted zero-delta adjacency must be placed in one classification group.

The grouping relation must be an exact equivalence relation:

- reflexive for every atom;
- symmetric for every permitted zero-delta adjacency;
- transitive through connected paths; and
- prohibited across crossing cuts or topology-separated contacts.

A provider may use deterministic union-find, graph traversal, canonical component labeling, or another in-tree method. Publication must assign group IDs from canonical member keys after grouping, not from union root address, insertion order, or worker timing.

Each group must publish:

- canonical member atom IDs;
- shell and source-component membership;
- boundary/contact categories present;
- representative candidates for seed construction;
- adjacency to other groups with signed deltas; and
- a canonical digest.

### 2.6 Quotient graph of classification groups

After zero-delta grouping, the component must construct a directed quotient graph whose nodes are classification groups and whose edges carry signed winding changes or symbolic boundary relations.

The quotient graph must satisfy:

- every crossing adjacency appears exactly once in each direction;
- reverse deltas are negatives;
- parallel edges with the same lineage are deduplicated without losing member evidence;
- several independent crossings between the same groups retain multiplicity evidence;
- zero-delta edges have already been absorbed unless they preserve a distinct boundary relation required downstream;
- self-loops have total delta zero or produce failure; and
- canonical node and edge order is independent of traversal.

The quotient graph is the propagation domain. Component 10 must not need to inspect triangle traversal order to recover classification transitions.

### 2.7 Deterministic classification seeds

Every connected component of the quotient graph must obtain enough anchored classifications to determine all group values.

The component should minimize expensive inclusion queries. It must not cast an independent ray for every vertex, edge, triangle, or atom. The default contract permits at most one successful independent seed query per otherwise unanchored connected quotient component, plus deterministic retries required to avoid a tied probe.

A seed record must identify:

- the target classification group;
- a canonical positive-area representative atom or a documented boundary-derived seed;
- a bounded representative point or side probe not lying on an unresolved opposite boundary;
- the opposite operand and shell set being queried;
- the selected seed-inclusion provider and version;
- exact or bounded crossing evidence;
- resulting integer winding and regularized inclusion;
- fallback attempts and tie reasons; and
- a deterministic proof that the final seed is admissible.

Zero-measure atoms should derive classification from adjacent positive-area groups and symbolic relations rather than forcing an unstable seed point.

### 2.8 Seed representative construction

A representative point for a positive-area group must be derived deterministically from its source lineage and bounded geometry. Possible providers may use:

- a source-facet interior witness;
- a bounded triangle interior point known to lie in the group;
- a point on a source-edge interval with an inward surface offset represented symbolically rather than by coordinate motion;
- a canonical aggregate of several vertices with a proof of region containment; or
- another versioned in-tree construction.

The point must not cross an intersection boundary within its uncertainty envelope. If no stable representative can be established, the provider must use propagation from another seeded group or return a typed geometric failure.

The component must not choose a nominal centroid that may lie outside a concave region without containment proof.

### 2.9 Seed inclusion provider

The seed inclusion provider may use a deterministic ray crossing, oriented winding accumulation, hierarchical shell query, exterior flood, or another in-tree bounded method selected later. Regardless of provider, it must:

- use Component 03 bounded arithmetic;
- use Component 02 shell orientation and occupied-side semantics;
- count source-facet crossings with a frozen half-open rule compatible with Component 07;
- use a canonical finite set or deterministic generation of probe directions if rays are used;
- reject or retry tied, tangent, coplanar, or uncertain probes rather than guessing;
- assign one canonical seed-query identity and compute it once;
- distinguish exact boundary tie from numerical uncertainty;
- produce a signed integer winding and regularized inclusion state;
- expose complete crossing and retry evidence; and
- fail if all permitted deterministic probes remain ambiguous beyond tolerance.

Random rays, wall-clock seeds, ambient thread state, and tolerance-based point-on-boundary guesses are prohibited.

### 2.10 Shell winding semantics

Seed and propagated winding values must respect the validated solid model from Component 02.

For the default outward-oriented alternating-shell policy:

- crossing an outer boundary from unoccupied to occupied increases winding as defined by the frozen convention;
- crossing a cavity boundary changes winding in the opposite direction;
- islands alternate again through the nesting forest;
- away from boundaries, total operand winding must be zero or one; and
- disconnected solids contribute through the same total oriented winding contract.

The component must retain shell-level contribution evidence so a verifier can distinguish a correct total from cancellation of incorrect shell labels.

A seed result inconsistent with Component 02 shell semantics must fail deterministically. It must not be clamped to zero or one.

### 2.11 Winding propagation

Starting from anchored seeds, the component must propagate winding over the quotient graph.

For every directed edge from group `g` to group `h` with numeric delta `d`, the propagation contract is:

```text
winding(h) = winding(g) + d
```

Symbolic boundary edges may additionally define perturbed side states without altering the operation-neutral numeric winding record.

Propagation must:

- use exact checked integer arithmetic;
- assign each group once or verify every repeated assignment agrees;
- process nodes and edges in canonical order or publish schedule-independent results;
- retain predecessor edge evidence for diagnostics;
- detect integer overflow;
- verify all graph components are anchored; and
- publish no partial label set.

The provider may use breadth-first traversal, depth-first traversal, constraint solving, or another deterministic method, but the final values must be path independent.

### 2.12 Cycle and path consistency

Every closed quotient-graph walk must have total numeric crossing delta zero.

The component must independently verify at least:

- reverse-edge antisymmetry;
- self-loop zero delta;
- agreement of all propagated paths to one group;
- zero sum around a canonical cycle basis or equivalent complete consistency proof;
- agreement between multiple seeds in one connected component;
- local crossing conservation around event clusters and source vertex sectors; and
- compatibility of symbolic side states around coincident and tangent contacts.

A non-zero cycle sum, conflicting seed, or path-dependent classification is an invariant failure or an upstream geometric-condition failure. The component must not choose the first path and ignore the contradiction.

### 2.13 Boundary and symbolic side classification

A group or atom that lies on the opposite operand boundary requires more than one bare integer winding.

The component must publish, as applicable:

- operation-neutral numeric winding away from the boundary;
- contact dimension;
- tangent, coplanar, coincident, or transverse boundary class;
- symbolic occupancy on the conceptual negative and positive sides of the contact;
- coincident ownership and cancellation metadata from Component 07;
- whether the point or edge contact must remain occurrence-separated;
- whether crossing the contact changes numeric winding; and
- whether the state is valid for Component 10 selection.

Symbolic side labels must be direct consequences of the frozen symbolic policy and relation lineage. The component must not invent a coordinate offset to sample them.

### 2.14 Per-atom opposite-operand labels

For every oriented positive-area source-surface atom, the artifact must provide the opposite-operand information needed to evaluate the Boolean result on both sides of that source surface.

At minimum, an atom label must identify:

- base opposite-operand winding or inclusion for the atom interior;
- opposite-operand occupancy on the source atom's oriented negative side;
- opposite-operand occupancy on the source atom's oriented positive side;
- whether those side values are numerically separated, symbolically assigned, or coincident-owned;
- all crossing/contact edges at the atom boundary;
- source shell occupied-side orientation; and
- classification group and seed/progression evidence.

For ordinary noncoincident atoms, the opposite occupancy may be equal on both infinitesimal sides. For coincident atoms, the two side values and ownership may differ and must be explicitly represented.

### 2.15 Zero-measure contact and occurrence separation

Point- and edge-touching configurations can have zero numeric crossing while still requiring separate output topology.

The component must preserve labels indicating:

- which source surface atoms meet only at a point or edge;
- whether positive-area continuation exists across the contact;
- whether the regularized operation considers the components connected through volume;
- which local sectors remain distinct classification occurrences; and
- which equal-coordinate event or source-vertex records must not be grouped.

Zero-measure contact must not create a false union in the classification graph. Conversely, a valid face-to-face volume connection whose internal shared surface is removed must provide the side labels needed for Component 10 to connect the surrounding retained boundary appropriately.

### 2.16 Coplanar and coincident region classification

Coplanar overlap may create regions where ordinary point-on-surface inclusion is not a single numeric state.

The component must classify each coplanar or coincident atom with:

- overlap support and source-facet pair;
- same or opposite orientation;
- overlap interior, overlap boundary, or disjoint coplanar region;
- numeric winding on each noncoincident neighboring region;
- symbolic sheet order and ownership;
- regularized inside/outside state on both conceptual sides; and
- occurrence-separation requirements.

The classification must be independent of source triangle diagonals and of which operand's edge discovered the overlap.

### 2.17 Deterministic grouping and parallel merge

Parallel atom construction or adjacency generation may use task-local buffers. Publication must:

- canonicalize atom and adjacency keys;
- merge exact duplicate requests by full key;
- group zero-delta connectivity deterministically;
- assign group IDs from canonical member sets;
- canonicalize quotient-graph edges;
- select seed targets and probe attempts deterministically;
- propagate or verify values independently of task completion order;
- select the same primary failure under all schedules; and
- commit only after complete verification.

Union-find parent choice, path compression timing, queue order, or hash iteration must not leak into group IDs, diagnostics, or digest.

### 2.18 Resource limits and pathological graphs

The component must account separately for:

- atoms;
- raw and canonical adjacency records;
- zero-delta unions;
- classification groups;
- quotient edges and member evidence;
- seed attempts and crossing work;
- propagation and cycle-verification work;
- boundary and side labels;
- diagnostics and replay storage; and
- persistent artifact bytes.

Highly intersected surfaces may produce large classification graphs. The component must use output-sensitive accounting and deterministic checkpoints. It must fail with `resource_limit` rather than merge across cuts, skip cycle checks, reduce seed evidence, or classify unvisited groups by default.

### 2.19 Cancellation and transactionality

Atom construction, adjacency generation, grouping, seed selection, seed inclusion, propagation, side labeling, and verification must occur in one stage transaction or private subtransactions that publish one final immutable artifact.

Cancellation must be polled at deterministic safe points during atom enumeration, adjacency generation, union/group construction, seed attempts, crossing accumulation, propagation, cycle verification, and publication checks.

On cancellation, all workers must join, all reservations must return, and no partial labels or groups may be visible. The result is `cancelled`.

### 2.20 Independent verification evidence

The component must publish enough evidence for an independent verifier to reconstruct and check:

- atom coverage of the source surface;
- atom key uniqueness and canonical order;
- cut-aware adjacency from source topology and Component 08 descriptors;
- transparent treatment of internal diagonals;
- exact zero-delta group membership;
- quotient-graph construction;
- crossing delta provenance and reverse antisymmetry;
- deterministic seed admissibility and inclusion results;
- shell-level winding contributions;
- propagation equations;
- path and cycle consistency;
- boundary and symbolic side states;
- per-atom opposite-operand labels;
- occurrence-separation constraints;
- deterministic digest inputs; and
- absence of coordinate-based connectivity.

For bounded fixtures, the verifier must compare group winding and side labels with an independently implemented exact rational ray/winding oracle and exhaustive source-surface decomposition. It must not call the producer's union, seed selection, propagation, or cycle-basis helpers as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `classification_complex` artifact containing or referencing:

- artifact, atom-domain, adjacency, seed-provider, winding, symbolic-policy, and serialization versions;
- canonical classification atoms for both operand surfaces;
- exact source-feature and event/carrier membership for every atom;
- canonical cut-aware adjacency records;
- signed numeric crossing deltas and symbolic side metadata;
- canonical zero-delta classification groups and reverse membership maps;
- canonical quotient graphs for A relative to B and B relative to A;
- deterministic seed inclusion records and retry evidence;
- integer winding values and regularized inclusion states;
- shell-level contribution records;
- cycle and path-consistency evidence;
- boundary, tangent, coplanar, coincident, and occurrence-separation labels;
- per-atom opposite-operand negative-side and positive-side occupancy;
- deterministic partitions permitted for Component 10 consumption;
- resource and structural statistics;
- independent-verification evidence;
- canonical input and output digests; and
- replay metadata sufficient to reproduce grouping, seeding, propagation, and labels.

The artifact must guarantee:

- every positive-area source-surface location is represented by exactly one atom away from boundaries;
- every atom belongs to exactly one classification group;
- members of one group share one opposite-operand numeric classification;
- no group crosses an authoritative winding-changing cut;
- no coordinate-coincident disconnected topology is joined by proximity;
- internal triangulation diagonals are transparent unless crossed by authoritative intersection lineage;
- every quotient edge uses Component 07 signed crossing data;
- all quotient components are anchored by deterministic valid seeds or boundary-derived constraints;
- propagated values are path independent and cycle consistent;
- non-boundary winding agrees with the validated regular-solid domain;
- boundary and symbolic states remain distinguishable from ordinary inside/outside;
- point- and edge-touching occurrence separation is preserved;
- legal re-triangulation and source subdivision preserve source-feature classification;
- group IDs, labels, diagnostics, and digest are independent of traversal and schedule; and
- Component 10 can apply the Boolean truth table without repeating inclusion or relation tests.

A valid empty operand pair must produce valid empty or one-sided classification artifacts according to the public empty-solid semantics and deterministic digests.

On failure, no classification complex is published. The typed error must identify the operand direction, atoms or groups, source features, event/carrier lineage, crossing deltas, seed probes, winding values, cycle witnesses, numerical bounds, policy versions, resource counters, and deterministic replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- classification atoms cover the source surface under the published decomposition contract;
- cut-aware adjacency is derived only from exact topology and authoritative relation descriptors;
- zero-delta connectivity defines canonical equivalence groups;
- crossing deltas are reused from Component 07 and never recomputed independently;
- reverse quotient edges have opposite deltas;
- every connected quotient component is deterministically anchored;
- each seed query is canonical and evaluated once;
- propagated winding is path independent;
- every closed graph cycle has zero total numeric delta;
- accepted non-boundary winding satisfies the validated solid policy;
- boundary, symbolic, and occurrence states remain explicit;
- coordinate equality never creates connectivity;
- internal diagonals do not change source-facet classification semantics;
- all artifacts are immutable, context-owned, transactional, deterministic, and independently verifiable; and
- ambiguity or resource exhaustion causes typed failure rather than default classification.

Prohibited behavior:

- one ray or point-in-solid query per source vertex, triangle, or atom when connectivity permits grouping;
- random probe directions or scheduling-dependent fallback order;
- using bounding-box containment as final inclusion classification;
- coercing an uncertain seed to inside or outside;
- clamping winding values to zero or one;
- recomputing crossing signs from raw geometry;
- unioning groups across a cut because coordinates are close;
- treating internal triangulation diagonals as semantic boundaries;
- connecting disconnected point- or edge-touching shells by coordinate coincidence;
- discarding zero-measure contact metadata;
- accepting path-dependent propagation by choosing one traversal result;
- assigning group IDs from union root addresses or insertion order;
- publishing partial label sets after cancellation or resource exhaustion; or
- calling an external graph, topology, exact-arithmetic, winding, ray-casting, or geometry library.

## 5. Test and validation specification

### 5.1 Atom and adjacency unit tests

Unit tests must cover:

- empty operands;
- one untouched tetrahedral shell;
- one shell cut by a single transverse loop;
- multiple events on one source edge;
- source-facet carriers crossing internal diagonals;
- tangent point and edge contact;
- coplanar overlap boundaries;
- coincident sheets;
- high-valence source vertices; and
- coordinate-coincident disconnected shells.

Every fixture must independently reconstruct atom coverage and adjacency.

### 5.2 Known-answer group tests

Commit exact expected atom groups for:

- two disjoint solids;
- strict containment without boundary intersection;
- one proper overlap loop;
- several disjoint cut loops on one shell;
- one cut splitting a large untouched region;
- cavities and nested islands;
- several disconnected shells;
- point-touching and edge-touching solids;
- face-touching solids; and
- equal operands.

Expected records must include group membership, quotient edges, seed targets, winding values, side labels, and occurrence-separation flags.

### 5.3 Seed inclusion tests

For every seed provider, test:

- clear exterior and interior probes;
- nested cavity and island probes;
- several disconnected opposite shells;
- a first probe direction hitting a vertex and a deterministic fallback succeeding;
- tangent and coplanar first attempts;
- all permitted attempts remaining ambiguous;
- large translations with small local features;
- signed zero and subnormal coordinates; and
- exact tolerance-boundary uncertainty.

Verify one canonical seed result per unanchored quotient component, stable retry order, crossing evidence, and precise failure.

### 5.4 Winding propagation and cycle tests

Construct quotient graphs with:

- one edge of delta `+1`;
- entering and leaving edges;
- several paths to one group;
- several parallel crossing edges;
- nested shell contributions;
- a valid nontrivial cycle with zero total delta;
- an injected non-zero cycle;
- conflicting seeds; and
- integer-overflow mutations.

The producer and independent verifier must agree on every value and reject every inconsistency.

### 5.5 Exact-oracle differential tests

Use the in-tree exact rational oracle on small integer-coordinate meshes to compare:

- source-surface atom classification;
- seed point winding;
- shell contributions;
- crossing deltas;
- group winding;
- inside/outside conversion; and
- boundary-side labels.

Test disjointness, containment, proper overlap, tangency, coplanar overlap, cavities, islands, and disconnected components.

### 5.6 Internal-diagonal and subdivision metamorphic tests

Apply several legal triangulations to the same polygonal source facets and topology-preserving subdivisions to source faces and edges.

Verify:

- source-facet atom groups remain equivalent;
- internal diagonals are transparent;
- seed count does not increase merely because triangulation changed;
- quotient graph and winding labels are equivalent;
- symbolic and occurrence-separation states are preserved; and
- downstream Component 10 selection is unchanged.

### 5.7 Contact and coincidence tests

Cover the complete operation-neutral classification matrix for:

- vertex-touching solids;
- edge-touching solids;
- face-touching solids;
- tangent surfaces;
- same-orientation coincident facets;
- opposite-orientation coincident facets;
- partial coplanar overlap;
- equal operands;
- several coordinate-coincident disconnected shells; and
- several sheets meeting at one event cluster.

Verify zero numeric deltas, symbolic side labels, group separation, and no accidental coordinate-based connectivity.

### 5.8 Metamorphic tests

Apply:

- operand exchange with classification-direction remapping;
- source vertex, edge, facet, shell, and component permutations;
- facet-ring rotation;
- legal source subdivision and re-triangulation;
- global orientation reversal with corrected shell semantics;
- axis permutation;
- sign flip;
- exactly representable translation;
- power-of-two scaling with precision scaling;
- thread counts 1, 2, and maximum; and
- forced task delays, union order changes, queue order changes, and reversed merges.

Canonical atoms where policy makes them unique, classification groups, quotient edges, seeds, winding, side labels, diagnostics, and digest must remain byte-identical after documented remapping.

### 5.9 Mutation tests

Corrupt valid artifacts by:

- deleting an atom;
- assigning one atom to two groups;
- adding a false adjacency across a cut;
- removing a valid zero-delta adjacency;
- treating an internal diagonal as a cut;
- unioning coordinate-coincident disconnected shells;
- flipping one crossing delta;
- making reverse deltas equal rather than opposite;
- changing a seed target or inclusion value;
- omitting one shell contribution;
- changing one propagated winding;
- hiding a non-zero cycle;
- discarding a boundary or occurrence-separation state;
- changing symbolic negative/positive side occupancy;
- scrambling canonical group IDs; and
- forging counts or digests.

Independent verification must reject every mutation.

### 5.10 Fuzzing and shrinking

Generate valid manifold operand pairs from exact templates, then vary:

- shell count, nesting, and genus;
- cut-loop count;
- event count and valence;
- facet triangulation and subdivision;
- tangent and coplanar contacts;
- coordinate duplication without topology merging;
- ULP perturbations;
- input precision and tolerance;
- seed probe directions and provider versions; and
- resource limits.

Every crash, nondeterministic grouping, oracle disagreement, path inconsistency, invalid seed, false union, or missing classification must serialize exact inputs, relation/event artifacts, graph evidence, policies, and counters and shrink while preserving the failure.

### 5.11 Resource, cancellation, and concurrency tests

For atoms, raw adjacencies, groups, quotient edges, seed attempts, propagation work, cycle evidence, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during atom construction, adjacency generation, union/grouping, seed selection, seed crossing evaluation, propagation, side labeling, and verification. Confirm all workers join, reservations return, and no partial artifact is visible.

Dense cut graphs must fail deterministically with `resource_limit` when configured below true output requirements. Raising limits must reveal the complete classification complex, not a simplified graph.

### 5.12 Definition of done

Component 09 is complete only when:

- the classification atom and adjacency domains are frozen and independently reconstructible;
- every source-surface atom belongs to one canonical cut-aware group;
- grouping never crosses authoritative cuts or coordinate-only contacts;
- crossing deltas are reused from Component 07;
- seed classification is deterministic, bounded, and minimized by connectivity;
- all quotient components are anchored;
- path and cycle consistency are independently verified;
- accepted non-boundary winding agrees with Component 02 shell semantics;
- boundary, coplanar, coincident, and occurrence-separation labels are complete;
- legal subdivision and re-triangulation preserve classification;
- mutation verification is effective;
- deterministic replay is byte-stable across schedules; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
