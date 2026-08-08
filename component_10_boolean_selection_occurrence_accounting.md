# Component 10: Boolean Selection and Occurrence Accounting

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete retained-use representation, local fan partitioning provider, coincidence-group layout, multiplicity encoding, and deterministic graph algorithms may change. The side-occupancy truth-table evaluation, operation-aware coincidence ownership, oriented retention, internal-surface suppression, topological occurrence partition, manifold-feasibility evidence, determinism, verification, and failure contracts in this document are normative.

## 0. Purpose

This component converts the operation-neutral source-surface classification from Component 09 into the exact set of oriented surface uses that must bound the requested regularized Boolean result.

Its purposes are to:

- apply the frozen Boolean truth table to the classified occupancy on both sides of every positive-area source-surface atom;
- retain a source-surface use exactly when the Boolean result changes occupancy across that use;
- prescribe whether the retained use keeps or reverses its source orientation;
- jointly resolve coincident and coplanar sheets using the symbolic ownership decisions from Component 07;
- suppress internal two-sided surfaces and zero-volume artifacts under the regularized operation contract;
- account explicitly for multiplicity when several valid topological boundary occurrences share one geometric location;
- partition retained incidences into separate output occurrence requirements wherever joining them would create a non-manifold edge, bow-tie vertex, or false connection through a point or edge contact;
- preserve complete selection and discard evidence for independent verification; and
- provide Component 11 with a deterministic retained-surface complex from which paired output edges and face cycles can be constructed without repeating Boolean classification.

The component selects and accounts for topology. It does not allocate final output vertex coordinates, create output halfedge pairs, build face cycles, triangulate polygons, move geometry, collapse degeneracies, or serialize the public mesh.

The principal output is an immutable `retained_surface_complex` containing:

- one audited selection disposition for every positive-area classification atom;
- canonical retained surface-use records;
- orientation and truth-table evidence;
- coincidence ownership and suppression records;
- retained source-edge interval and carrier-boundary incidences;
- local output occurrence partitions at source vertices, intersection events, edge intervals, and carrier clusters;
- multiplicity and topology-separation requirements; and
- complete manifold-feasibility, provenance, resource, verification, digest, and replay metadata.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `classification_complex` from Component 09;
- the immutable `canonical_intersection_complex<T>` from Component 08;
- the immutable `signed_feature_relations<T>` from Component 07;
- the immutable `canonical_source_manifolds` from Component 05;
- source-facet groups and source-boundary provenance from Components 04 and 05;
- validated source shell orientation and occupied-side semantics from Component 02;
- the immutable precision context and bounded evidence services from Component 03;
- the immutable Boolean context, operation truth table, operand-remapping rules, symbolic contact-policy matrix, identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, and transaction services from Component 01;
- the selected selection-domain, occurrence-partition, multiplicity, artifact, and serialization versions; and
- verification settings controlling scalable and exhaustive selection checks.

The component must not repeat point-in-solid tests, recompute crossing signs, infer coincidence from coordinates, or alter predecessor classifications to make output assembly easier.

### 1.2 Required predecessor guarantees

The component may rely on predecessor artifacts having established:

- complete positive-area classification atoms for both source surfaces;
- exact source-feature, event, carrier, shell, and operand lineage for every atom;
- opposite-operand numeric and symbolic occupancy on both oriented sides of every atom;
- cut-aware group and boundary states;
- complete point-, edge-, tangent-, coplanar-, and coincident-contact descriptors;
- operation-specific symbolic ownership for exact ties and coincident surfaces;
- canonical intersection events and distinct occurrence identities;
- ordered source-edge and carrier interval partitions;
- preserved occurrence-separation constraints for point- and edge-touching topology;
- consistent integer winding and regularized inclusion values; and
- deterministic canonical ordering and digests.

The component must defensively verify owner tokens, versions, atom coverage, side-label validity, truth-table state domains, event/carrier references, symbolic-rule references, and artifact digests. A contradiction in a committed predecessor artifact is an `internal_invariant_error`; it must not be hidden by discarding the affected atom.

### 1.3 Selection domain

The public selection domain consists of all positive-area source-surface atoms from both operands, together with the coincidence groups and boundary incidences required to evaluate them jointly.

Every positive-area atom must receive exactly one final disposition:

- retained with source orientation;
- retained with reversed orientation;
- discarded because result occupancy is equal on both sides;
- suppressed as an internal coincident surface;
- suppressed because another canonical coincident owner supplies the retained sheet;
- represented through a documented multiplicity or separate occurrence;
- invalid because side classification or ownership is contradictory; or
- failed because required topology cannot be represented within resource or contract limits.

No atom may be omitted from the audit table.

### 1.4 Source-side occupancy preconditions

For each oriented source-surface atom, the component must receive:

- the source operand's occupancy on the atom's oriented negative side;
- the source operand's occupancy on the atom's oriented positive side;
- the opposite operand's occupancy on the same two conceptual sides;
- whether each value is numeric, symbolic, coincident-owned, or boundary-derived;
- the source shell occupied-side convention;
- all relation and classification evidence; and
- an indication that the two side tuples are valid inputs to the frozen operation truth table.

The component must not assume that the source occupied side is always a particular normal direction without consulting Component 02 semantics. The convention must be explicit and versioned.

### 1.5 Coincidence and multiplicity preconditions

For source atoms participating in coplanar or coincident overlap, the component must receive canonical grouping and ownership evidence sufficient to distinguish:

- one geometric sheet discovered by several triangle atoms;
- two operand sheets representing the same boundary with the same orientation;
- opposite-oriented coincident sheets;
- partial overlap with noncoincident neighboring regions;
- several topologically distinct coincident shells;
- an internal shared face between volume-connected solids;
- a zero-volume contact sheet that regularization removes; and
- several equal-coordinate occurrences that must remain separate.

Coordinate equality alone is never sufficient to place atoms in one coincidence group.

### 1.6 Capacity and lifetime preconditions

Before selection begins, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all atom disposition records;
- all retained surface uses;
- all coincidence groups and ownership decisions;
- all retained boundary incidences on source edges and intersection carriers;
- all source-vertex, event, edge-interval, carrier-cluster, and local fan occurrence partitions;
- all multiplicity records and reverse mappings;
- all manifold-feasibility and balance evidence;
- all temporary grouping, sorting, graph, verification, diagnostics, and replay storage; and
- worst-case work up to configured retained-use, occurrence, incidence, and partition limits.

Published records may reference only immutable predecessor artifacts and immutable stage-owned storage whose lifetime covers Components 11-15.

## 2. Required behavior

### 2.1 Truth-table evaluation on oriented sides

For every positive-area source-surface atom, the component must form two complete operand-occupancy tuples:

- the tuple immediately on the atom's oriented negative side; and
- the tuple immediately on the atom's oriented positive side.

For an atom originating from operand A, each tuple contains A's source-side occupancy and B's Component 09 classification. For an atom originating from B, the roles are remapped consistently.

The component must evaluate the frozen Boolean operation truth table on both tuples:

```text
result_negative = operation(occupancy_A_negative, occupancy_B_negative)
result_positive = operation(occupancy_A_positive, occupancy_B_positive)
```

The truth-table service must be the Component 01 service. The component must not duplicate operation logic with ad hoc conditionals whose behavior may diverge for coincident or symbolic states.

Every evaluation must retain the exact truth-table cell, operand role, symbolic rule references, and input side labels for verification.

### 2.2 Basic retain, discard, and orientation rule

The basic operation-neutral surface-boundary rule is:

- if `result_negative == result_positive`, the atom is not a boundary of the regularized result and must be discarded or suppressed according to its coincidence state;
- if the values differ, exactly the required retained surface use or uses must be produced; and
- each retained use must be oriented so the result's occupied side is the output convention's negative side.

If the source orientation already places result occupancy on the required side, the use is retained with source orientation. Otherwise it is retained reversed.

The orientation decision must come from side occupancy, not from comparing normals, source operand priority, or the original operation name alone.

An invalid, missing, or contradictory side tuple must cause a typed failure. The component must not default to discard.

### 2.3 Selection disposition records

Every source atom must receive one immutable disposition record containing:

- atom, group, source facet, shell, and operand IDs;
- source orientation and occupied-side convention;
- negative-side and positive-side operand occupancy tuples;
- truth-table rule and outputs;
- retain/discard/suppress result;
- preserve/reverse orientation result;
- coincidence ownership and multiplicity references;
- boundary incidence consumed by the retained use;
- deterministic reason code; and
- relation, classification, and symbolic provenance.

Discard records are first-class verification evidence. They must not be omitted after retained uses are built.

### 2.4 Canonical retained surface uses

For every retained disposition, the component must create one or more canonical `retained_surface_use` records as required by multiplicity and occurrence rules.

A retained surface use must identify:

- source classification atom;
- source operand, shell, facet, triangle-group, and caller provenance;
- prescribed output orientation;
- result occupancy on both sides;
- source-edge intervals, event clusters, and carrier arcs forming its boundary;
- coincident ownership class;
- multiplicity and occurrence partition;
- permitted continuation to adjacent retained atoms; and
- a stable canonical key and ID.

The use may describe a polygonal source-facet region whose explicit boundary cycles are built later by Component 11. It must contain enough exact incidence that Component 11 does not repeat Boolean selection.

### 2.5 Coincident-sheet joint evaluation

Atoms in one canonical coincident-sheet group must be evaluated jointly after their individual truth-table side results are known.

Joint evaluation must:

- preserve all operation-neutral side tuples;
- apply the exact symbolic ownership rule from Component 07;
- determine whether the coincident sheet is external result boundary, internal surface, cancelled boundary, or duplicated topological occurrence;
- choose a canonical retained owner where one geometric boundary sheet is required;
- suppress non-owning duplicate source uses without losing provenance;
- preserve separate occurrences when the policy requires topologically distinct coincident sheets; and
- verify orientation compatibility among all retained members.

The component must not select the first operand, smaller triangle ID, or lower nominal normal as owner unless that rule is explicitly part of the frozen symbolic policy.

### 2.6 Same-orientation coincidence

For same-orientation coincident source sheets, the component must evaluate the operation-specific regularized semantics.

Examples that must be represented by the frozen policy include:

- union and intersection of equal solids retaining one outward boundary sheet;
- `A - A` and symmetric difference of equal solids retaining no boundary;
- partial overlap retaining only regions where result occupancy changes;
- coincident duplicate shells in separate topological occurrences; and
- same support but different source-facet partitioning.

A retained coincident boundary must have one canonical ownership lineage or a documented multiplicity. Several triangle atoms or source facets that cover the same selected sheet must not create several overlapping output faces merely because their source partitions differ.

### 2.7 Opposite-orientation coincidence

For opposite-oriented coincident sheets, the component must preserve source shell semantics and operation-specific symbolic order.

It must distinguish at least:

- internal shared faces between adjacent occupied volumes;
- cavity and outer-shell coincidence;
- equal geometric supports representing opposite occupied sides;
- contact without positive-volume continuation;
- cancellation under difference or symmetric difference; and
- separate topological sheets whose coordinates coincide but whose occupied-side semantics differ.

Internal two-sided sheets must be suppressed. If the selected result requires one orientation, exactly that orientation must be retained by canonical ownership. If both conceptual occurrences must remain separate, they must receive separate occurrence requirements rather than a non-manifold multi-use edge.

### 2.8 Partial coplanar overlap

Partial coplanar overlap requires region-local selection. The component must:

- select overlap-interior atoms jointly through their coincidence group;
- select nonoverlap atoms through ordinary side evaluation;
- preserve overlap-boundary event and source-edge incidence;
- apply half-open ownership at shared overlap boundaries;
- avoid double-retaining the same overlap area from both operands;
- retain source provenance for whichever canonical owner supplies the result; and
- keep the selection independent of source triangle diagonals and facet partitioning.

A source facet may contribute retained regions with different dispositions. The output is a set of retained surface uses, not one all-or-nothing decision per original facet.

### 2.9 Internal-surface suppression

A positive-area sheet is internal to the regularized result when result occupancy is equal on its two conceptual sides after all numeric and symbolic rules are applied.

Internal suppression must cover:

- overlapping interior faces;
- shared coincident faces between volume-connected solids;
- duplicate same-orientation sheets whose canonical owner already supplies the boundary;
- sheets cancelled by directed difference;
- symmetric-difference cancellation; and
- zero-thickness layers that do not bound volume under regularization.

Suppression must preserve an audit record explaining which side states made the sheet internal and, for coincident groups, which owner or cancellation rule applied.

The component must not suppress a point- or edge-touching component's external faces merely because another component has equal coordinates at the contact.

### 2.10 Multiplicity accounting

The component must represent every required boundary multiplicity explicitly.

Multiplicity may arise from:

- several topologically separate shells occupying the same coordinates;
- point- or edge-touching solids whose regularized union remains topologically disconnected;
- coincident but occurrence-distinct source sheets;
- local selection splitting one source vertex or event into several manifold fans;
- symbolic policy retaining a canonical sheet while preserving separate provenance occurrences; or
- future versioned solid policies with valid integer boundary multiplicity.

The default regular-solid output should have one oriented surface use per geometric boundary sheet, but the representation must not assume coordinate uniqueness.

Multiplicity must be expressed through separate retained-use or occurrence identities, not by allowing an undirected output edge to have more than two incident face uses.

### 2.11 Retained boundary incidence

For each retained surface use, the component must identify all boundary incidences inherited from:

- retained whole source edges;
- retained source-edge intervals split by events;
- transverse face-face carrier intervals;
- coplanar overlap boundaries;
- source vertices;
- intersection events and equal-parameter clusters;
- tangent/contact delimiters; and
- coincident-region boundaries.

Each incidence must include:

- local direction under the retained use orientation;
- start and end event/source-vertex occurrence keys;
- source or carrier provenance;
- adjacent retained-use candidates;
- multiplicity/occurrence partition;
- whether the incidence is expected to become an output boundary edge or an internal adjacency between retained atoms of one face region; and
- deterministic pairing/balance evidence for Component 11.

The component must not create output halfedge IDs. It supplies the exact retained incidence contract from which Component 11 creates paired edges.

### 2.12 Continuation across retained atom boundaries

Two retained surface uses or atom fragments may continue as one result surface region only when exact lineage and side classification prove compatible continuation.

Continuation may be permitted across:

- an uncut source edge whose adjacent retained atoms have compatible output orientation and occurrence partition;
- a transparent internal triangulation diagonal;
- a source-facet atom boundary introduced only by classification decomposition; or
- a coincident partition boundary where the canonical owner remains the same sheet.

Continuation must be prohibited across:

- a selected intersection carrier edge;
- a topology-separated point or edge contact;
- incompatible coincident ownership;
- opposite retained orientation;
- different multiplicity occurrences; or
- a boundary at which result occupancy changes in a different local fan.

Continuation is exact topology. It must not be inferred from coplanarity or coordinate proximity.

### 2.13 Local occurrence domains

The component must define local output occurrence requirements at every geometric location where retained topology may branch or coincide.

Occurrence domains must include, as applicable:

- accepted source vertices;
- canonical intersection events;
- equal-coordinate event clusters;
- source-edge interval endpoints;
- transverse carrier vertices;
- coplanar overlap endpoints;
- retained source-edge intervals; and
- carrier intervals that may need separate topological copies.

An occurrence requirement is a future output-topology slot. It identifies which retained surface incidences are allowed to share one output vertex or edge occurrence. Component 11 allocates actual output entities from these slots.

### 2.14 Vertex and event fan partitioning

At each source vertex, event, or coincident cluster, the component must build the local incidence graph of retained oriented surface sectors and partition it into one or more manifold fan requirements.

A valid fan requirement must:

- contain only incidences permitted to share one output occurrence;
- have a deterministic cyclic order from source topology and carrier/event lineage;
- represent one closed local two-manifold fan after Component 11 pairs edges;
- exclude sectors separated only by point or edge contact without positive-area continuation;
- separate coincident sheets when symbolic policy requires it;
- preserve multiplicity; and
- expose complete member and boundary evidence.

If joining all equal-coordinate sectors would create a bow-tie vertex, disconnected link, or non-manifold fan, the component must create multiple occurrence requirements.

The component must not create a geometric gap or move coordinates to separate those occurrences.

### 2.15 Edge and carrier occurrence partitioning

For each retained source-edge interval or intersection-carrier interval, the component must partition incident retained surface uses into output edge-occurrence requirements.

Each edge occurrence must be compatible with exactly two opposite directed face uses after Component 11 construction.

The partition must account for:

- source edge incidence from one or both original facets;
- split intervals between ordered events;
- transverse intersection boundaries contributed by both operands;
- coplanar overlap boundaries;
- same-coordinate but topology-distinct sheets;
- retained orientation; and
- local fan membership at both endpoints.

If more than two retained face uses meet one geometric carrier interval, they must be partitioned into separate output edge occurrences according to lineage and local fan compatibility. The component must not publish a planned three-use or four-use edge.

### 2.16 Start/end and carrier balance constraints

The component must provide Component 11 with deterministic balance constraints for every retained carrier and edge-occurrence domain.

At minimum, it must verify:

- every retained boundary incidence has a matching compatible opposite-directed incidence candidate within the same occurrence partition;
- start and end counts balance on closed carrier chains;
- no retained incidence is consumed twice;
- no required incidence is missing;
- local orientation around each carrier is compatible with outward result orientation;
- zero-length nominal intervals remain accounted for when topology requires them; and
- ambiguous pairing choices are exposed rather than resolved by proximity.

Component 11 owns final halfedge pairing and cycle construction, but Component 10 must reject a selection artifact that is combinatorially incapable of forming paired manifold output.

### 2.17 Point- and edge-touching regularization

For contacts that do not create positive-volume connection, the component must preserve separate topological occurrences.

For a union of solids touching only at a point or edge, the selection must:

- retain the external surface atoms of both solids according to the truth table;
- preserve coordinate coincidence at the contact;
- create separate vertex and, where applicable, edge occurrence requirements;
- prohibit cross-component fan continuation through the zero-measure contact; and
- avoid inventing a geometric gap.

For intersection of solids touching only at a point or edge, regularization ordinarily yields no positive-area boundary and therefore no retained surface uses for the contact alone.

All operations and contact dimensions must follow the frozen policy matrix and be covered by tests.

### 2.18 Face-touching and positive-volume continuation

Face contact may represent either a zero-volume touch or a shared interface between volume-connected occupied regions, depending on occupied-side semantics.

The component must use side occupancy to distinguish them.

For a volume-connected union across a shared face, the coincident internal face must be suppressed while surrounding retained surfaces receive occurrence partitions compatible with one manifold result.

For an operation that retains the shared face as external boundary, exactly one canonical oriented sheet must be selected.

The decision must not come from contact dimension alone.

### 2.19 Algebraic and operand-remapping consistency

Selection must obey Component 01's operation and operand-remapping rules.

The artifact must support independent checks of:

- commutativity for union, intersection, and symmetric difference;
- directed remapping of `A - B` and `B - A`;
- idempotence;
- `A - A` emptiness;
- identity with empty;
- absorption; and
- equal-operand coincidence behavior.

These are semantic verification rules. The component must not rewrite an operation into another form if doing so would change symbolic policy or evaluation lineage unless that rewrite is explicitly frozen and versioned.

### 2.20 Deterministic selection and parallel merge

Parallel atom evaluation may produce task-local disposition and incidence records. Publication must:

- evaluate truth-table cells from immutable side labels;
- canonicalize atom dispositions;
- group coincidence records by complete lineage keys;
- resolve ownership through the frozen symbolic matrix;
- assign retained-use IDs after canonical sorting;
- partition local vertex/event fans deterministically;
- partition edge/carrier occurrences deterministically;
- merge incidence and balance records by full keys;
- select the same primary failure under every schedule; and
- commit only after complete verification.

Union roots, graph traversal order, source triangle order, hash iteration, and worker completion order must not affect retained-use or occurrence IDs.

### 2.21 Resource limits and pathological multiplicity

The component must account separately for:

- atom dispositions;
- retained uses;
- coincidence-group membership;
- retained boundary incidences;
- vertex/event fan graph edges;
- vertex, edge, and carrier occurrence requirements;
- multiplicity records;
- pairing-feasibility and balance evidence;
- sorting and verification work;
- diagnostics and replay storage; and
- persistent artifact bytes.

A case with many coincident sheets or high-valence event clusters may require many separate occurrences. The component must use output-sensitive accounting and fail with `resource_limit` rather than merge occurrences, cap multiplicity, drop retained uses, or permit non-manifold incidence.

### 2.22 Cancellation and transactionality

Truth-table evaluation, coincidence resolution, retained-use construction, occurrence partitioning, balance checks, and verification must occur in one stage transaction or private subtransactions that publish one final immutable artifact.

Cancellation must be polled at deterministic safe points during atom disposition, coincidence grouping, ownership resolution, retained-use allocation, local fan construction, edge/carrier partitioning, balance verification, and publication checks.

On cancellation, all workers must join, reservations must return, and no partial selection artifact may be visible. The result is `cancelled`.

### 2.23 Independent verification evidence

The component must publish enough evidence for an independent verifier to reconstruct and check:

- one disposition for every positive-area classification atom;
- input side tuples and truth-table outputs;
- preserve/reverse orientation decisions;
- coincidence-group membership from lineage;
- exact symbolic owner or cancellation rule;
- internal-surface suppression;
- retained-use boundaries and continuation rules;
- multiplicity and separate occurrence requirements;
- local vertex/event fan partitions;
- edge/carrier occurrence partitions;
- pairing-feasibility and start/end balance constraints;
- point-, edge-, and face-contact regularization semantics;
- operand-remapping and algebraic identities on bounded fixtures;
- deterministic digest inputs; and
- absence of coordinate-based welding or ownership.

For bounded fixtures, the verifier must independently evaluate the Boolean truth table from Component 09 side labels and reconstruct local retained fans and edge-use multiplicities. It must not call the producer's selection dispatcher, coincidence owner chooser, fan partitioner, or occurrence allocator as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `retained_surface_complex` artifact containing or referencing:

- artifact, selection-domain, occurrence-partition, multiplicity, symbolic-policy, and serialization versions;
- one immutable selection disposition for every positive-area classification atom;
- canonical retained surface-use records and IDs;
- source operand, shell, facet, triangle-group, atom, and caller provenance;
- negative-side and positive-side operand occupancy tuples;
- truth-table rule IDs and result occupancies;
- preserve/reverse orientation decisions;
- coincidence-group, ownership, suppression, and cancellation records;
- multiplicity records;
- retained source-edge interval and carrier-boundary incidence;
- continuation constraints between retained atoms;
- canonical source-vertex, event, and cluster fan occurrence requirements;
- canonical source-edge and carrier edge-occurrence requirements;
- endpoint compatibility and start/end balance evidence;
- deterministic partitions permitted for Component 11 consumption;
- resource and structural statistics;
- independent-verification evidence;
- canonical input and output digests; and
- replay metadata sufficient to reproduce every retain, discard, ownership, orientation, and occurrence decision.

The artifact must guarantee:

- every positive-area source atom has exactly one audited disposition;
- a retained use exists exactly when the Boolean result changes occupancy across the selected sheet, subject to explicit coincident ownership and multiplicity rules;
- every retained use has a prescribed outward result orientation;
- no internal two-sided positive-area surface remains;
- no coincident geometric sheet is emitted more times than its documented multiplicity;
- non-owning coincident source atoms are suppressed without losing provenance;
- point- and edge-touching solids remain topologically separate when regularization requires it;
- coordinate-coincident retained occurrences may remain geometrically coincident;
- every planned output edge occurrence is compatible with exactly two opposite directed face uses;
- every planned output vertex occurrence has one closed local retained fan;
- no occurrence partition was inferred from coordinate proximity;
- internal triangulation diagonals do not affect source-facet selection semantics;
- legal source subdivision and re-triangulation preserve retained source-feature topology;
- retained-use IDs, occurrence IDs, diagnostics, and digest are independent of traversal and schedule; and
- Component 11 can allocate vertices, pair edges, and form face cycles without repeating Boolean classification.

A valid operation producing the empty set must publish a valid empty retained-surface complex with complete discard/suppression evidence and a deterministic digest.

On failure, no retained-surface complex is published. The typed error must identify the operation, operand roles, source atoms, side tuples, truth-table rule, coincident group, occurrence partition, carrier or event witnesses, multiplicity, resource counters, policy versions, and deterministic replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- every source atom is audited exactly once;
- truth-table evaluation uses the frozen Component 01 service;
- retain/discard is determined by result occupancy on both sides;
- retained orientation places result occupancy on the documented output side;
- coincident sheets are resolved jointly through canonical lineage and symbolic policy;
- internal positive-area surfaces are suppressed;
- multiplicity is explicit and never encoded as non-manifold edge use;
- coordinate equality never creates ownership, continuation, or occurrence sharing;
- every output occurrence requirement has one exact lineage-defined member set;
- every planned edge occurrence has exactly two compatible directed uses;
- every planned vertex occurrence has one closed retained fan;
- point- and edge-touching regularization preserves required topological separation;
- internal diagonals do not change source-facet selection;
- all artifacts are immutable, context-owned, transactional, deterministic, and independently verifiable; and
- ambiguity or resource exhaustion causes typed failure rather than heuristic selection.

Prohibited behavior:

- selecting faces from source operand priority without side evaluation;
- deciding orientation from nominal normal comparison alone;
- discarding an atom because its classification is missing or uncertain;
- inferring coincident groups from coordinates, plane proximity, or tolerance;
- retaining both operands' copies of one selected coincident boundary without documented multiplicity;
- welding point- or edge-touching components;
- using one output occurrence for disconnected local fans;
- planning an edge with three or more incident face uses;
- hiding internal surfaces by deleting arbitrary duplicates;
- treating a whole source facet uniformly when its atoms have different classifications;
- allowing triangle internal diagonals to affect ownership;
- choosing owners, occurrence roots, or fan order from hash iteration, pointer address, or worker timing;
- publishing partial selections after cancellation or resource exhaustion; or
- calling an external Boolean, graph, topology, mesh, exact-arithmetic, or geometry library.

## 5. Test and validation specification

### 5.1 Truth-table unit tests

For every Boolean operation, test all valid combinations of:

- source negative/positive occupancy;
- opposite negative/positive occupancy;
- numeric inside/outside;
- boundary and tangent states;
- same- and opposite-orientation coincidence;
- symbolic owner and non-owner states; and
- operand role.

Verify retain/discard, preserve/reverse orientation, owner, multiplicity, and stable rule IDs.

### 5.2 Known-answer operation matrix

Commit exact expected retained-use artifacts for:

- empty/empty;
- empty/non-empty in both operand orders;
- disjoint solids;
- strict containment in both directions;
- proper overlap;
- equal operands;
- vertex-touching solids;
- edge-touching solids;
- face-touching solids;
- tangent contact;
- partial coplanar overlap;
- same-orientation coincident shells;
- opposite-orientation coincident shells;
- cavities and islands; and
- several disconnected components.

Run the complete matrix for union, intersection, `A - B`, `B - A`, and symmetric difference.

### 5.3 Orientation tests

For retained atoms from both operands, verify:

- source orientation preserved when result occupied side matches;
- orientation reversed for difference where required;
- coincident owner orientation follows result occupancy rather than operand priority;
- global orientation reversal with corrected shell semantics remaps consistently;
- every retained use has different result occupancy on its two sides; and
- an injected reversed retained use is rejected independently.

### 5.4 Coincident ownership tests

Include:

- equal boxes with identical triangulation;
- equal boxes with radically different triangulation;
- partial coplanar overlap;
- one facet subdivided into many source facets;
- same-orientation coincident duplicate shells;
- opposite-orientation coincident shells;
- several topologically distinct coincident components; and
- overlap boundaries passing through original vertices and edges.

Verify one canonical owner where one sheet is required, correct cancellation where none is required, explicit multiplicity where several occurrences are required, and no coordinate-based grouping.

### 5.5 Point-, edge-, and face-contact tests

For every operation and operand order, test:

- vertex-only contact;
- edge-only contact;
- coplanar face contact with no positive-volume overlap;
- shared face between volume-connected occupied regions;
- tangent contact along a curve; and
- several contacts at one event cluster.

Verify regularized emptiness or retention, occurrence separation, internal-face suppression, and local fan partitions.

### 5.6 Occurrence partition tests

Construct local retained incidence requiring:

- one ordinary manifold vertex occurrence;
- two point-touching components sharing one coordinate;
- two edge-touching components sharing an edge geometry;
- a bow-tie that must split into two output occurrences;
- several coincident sheets with separate fan requirements;
- a high-valence transverse intersection event;
- a carrier interval with four retained face uses partitioned into two edge occurrences; and
- equal-coordinate source and constructed events that remain distinct.

Independently reconstruct every local fan and planned edge-use pair.

### 5.7 Carrier balance tests

Commit known-answer incidence and balance records for:

- one closed transverse intersection loop;
- several loops on one face pair;
- open local chains that close through source edges;
- coplanar overlap boundaries;
- equal-parameter event clusters;
- zero-nominal-length but topology-distinct intervals;
- one missing incidence mutation;
- one duplicate incidence mutation; and
- a crossed or incompatible pairing candidate set.

The component must reject selection artifacts incapable of paired manifold output.

### 5.8 Algebraic identity tests

For exact-template and bounded floating fixtures, verify:

- union, intersection, and symmetric-difference commutativity;
- operand-remapped directed difference;
- idempotence;
- `A - A` is empty;
- identity with empty;
- absorption;
- equal-operand ownership; and
- consistent retained topology under repeated operation evaluation.

Compare canonical retained topology and occurrence partitions after documented operand remapping.

### 5.9 Subdivision and triangulation metamorphic tests

Apply:

- source facet subdivision;
- source edge subdivision;
- alternative legal polygon triangulations;
- triangle order permutations;
- facet ring rotation; and
- different source-facet partitions of the same coplanar sheet.

Verify source-feature selection semantics, coincident ownership, retained-use topology, occurrence partitions, and Component 11 input equivalence remain unchanged.

### 5.10 Determinism and metamorphic tests

Apply:

- source vertex, facet, shell, and component permutations;
- operand exchange with operation remapping;
- global orientation reversal with corrected solid policy;
- axis permutation;
- sign flip;
- exactly representable translation;
- power-of-two scaling with precision scaling;
- thread counts 1, 2, and maximum;
- forced task delays;
- reversed coincidence-group discovery;
- changed union root choices; and
- reversed fan traversal starts.

For a fixed policy version, disposition records, retained-use keys and IDs, owner decisions, occurrence partitions, balance evidence, diagnostics, and digest must be byte-identical after documented remapping.

### 5.11 Mutation tests

Corrupt valid artifacts by:

- deleting an atom disposition;
- retaining an atom whose result occupancy is equal on both sides;
- discarding an atom whose result occupancy differs;
- reversing one orientation decision;
- selecting the wrong coincident owner;
- retaining both copies of one single-multiplicity coincident sheet;
- suppressing both copies when one is required;
- merging point-touching occurrence fans;
- splitting a valid connected fan without policy reason;
- assigning three face uses to one edge occurrence;
- omitting one carrier boundary incidence;
- changing one start/end role;
- altering multiplicity;
- allowing an internal diagonal to affect selection;
- scrambling canonical retained-use order; and
- forging counts or digests.

Independent verification must reject every mutation.

### 5.12 Fuzzing and shrinking

Generate valid exact-template classification complexes and vary:

- operation and operand order;
- shell count and nesting;
- contact dimension;
- coincident-sheet count and orientation;
- source triangulation and subdivision;
- event valence;
- point- and edge-touching component count;
- coordinate duplication without topology merging;
- ULP perturbations;
- symbolic policy categories; and
- resource limits.

Every crash, nondeterministic owner, algebraic-law violation, accidental weld, invalid occurrence partition, unpaired planned edge, or verifier disagreement must serialize exact inputs, classification and relation evidence, policies, and counters and shrink while preserving the failure.

### 5.13 Resource, cancellation, and concurrency tests

For dispositions, retained uses, coincidence membership, boundary incidences, fan graph edges, occurrence records, multiplicity, balance evidence, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during truth-table evaluation, coincidence grouping, ownership resolution, retained-use construction, fan partitioning, edge/carrier occurrence partitioning, balance checking, and verification. Confirm all workers join, reservations return, and no partial artifact is visible.

High-multiplicity coincident fixtures must fail deterministically with `resource_limit` when configured below the true required occurrence count. Raising limits must produce the complete separated topology, not a welded or truncated result.

### 5.14 Definition of done

Component 10 is complete only when:

- every positive-area classification atom has one independently verifiable disposition;
- retention is exactly the frozen truth table applied to both oriented sides;
- every retained use has the correct outward result orientation;
- coincident ownership and internal-surface suppression are total for every operation;
- multiplicity is explicit and never encoded as non-manifold incidence;
- point- and edge-touching regularization preserves separate occurrences where required;
- every planned vertex occurrence has one closed retained fan;
- every planned edge occurrence has exactly two compatible directed uses;
- legal source subdivision and re-triangulation preserve retained topology;
- algebraic and operand-remapping tests pass;
- independent mutation verification is effective;
- deterministic replay is byte-stable across schedules; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
