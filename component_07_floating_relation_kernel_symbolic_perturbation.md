# Component 07: Floating Relation Kernel and Symbolic Perturbation

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete predicate formulas, enclosure representation, source-polygon query provider, relation-cache layout, symbolic-key encoding, and work-partitioning implementation may change. The compute-once dependency graph, three-layer numerical truth model, source-feature lineage, exact-tie eligibility, operation-aware symbolic semantics, downstream ownership boundary, determinism, verification, and failure contracts in this document are normative.

`tracker.md` records completion of this planning and independent-review step. It does not assert that Component 07 has been implemented or qualified. The future implementation definition of done remains Section 5.13.

## 0. Purpose

This component is the authoritative narrow-phase relation engine for the Boolean pipeline. It consumes the conservative directed edge-triangle candidate stream from Component 06 and converts those candidates into one immutable, canonically keyed set of signed geometric relations.

Its purposes are to:

- determine whether canonical source features are definitely separated, in contact, overlapping, tangent, coincident, or crossing;
- evaluate every topology-affecting geometric question exactly once under one prescribed dependency graph;
- preserve three separate numerical truth layers for every topology-affecting predicate: rounded nominal bits, exact stored-coordinate relation evidence, and the conservative uncertainty enclosure;
- construct nominal intersection data together with conservative uncertainty, residual, conditioning, and precision evidence;
- distinguish an exact stored-coordinate or exact-lineage tie from unresolved bounded uncertainty;
- apply the frozen operation-specific symbolic contact policy only to eligible ties and coincident configurations;
- assign authoritative signed numeric crossing multiplicities and separately identified symbolic side or crossing contributions needed by Components 08 and 09;
- publish coincidence ordering, owner-ranking eligibility, side-assignment, and occurrence-separation evidence needed by Components 09 and 10 without performing final Boolean surface selection; and
- preserve source-feature lineage so later stages never recompute an equivalent relation through a different floating-point expression.

The component treats exact indexed topology and bounded floating-point geometry as separate domains. A relation may state that distinct topological features occupy the same nominal point or support. That fact does not merge their identities.

The component does not allocate canonical intersection-event IDs, merge events by coordinate, build cut connectivity, compute global winding, apply the Boolean truth table to positive-area surface atoms, select retained surfaces, prescribe final output orientation, construct output topology, or perform cleanup. Component 08 owns event interning and carrier ordering, Component 09 owns cut-aware classification and side labels, and Component 10 owns final retain/discard/suppress/orient decisions and output occurrence accounting.

The principal output is an immutable `signed_feature_relations<T,I>` artifact containing:

- canonical primitive and composite relation records;
- orthogonal rounded-nominal, exact-relation, and uncertainty evidence;
- operation-neutral bounded geometric facts;
- operation-specific symbolic relation decisions for eligible exact ties;
- signed numeric crossing multiplicities and separate symbolic side/crossing metadata;
- canonical event-seed descriptions for Component 08;
- coincidence grouping, orientation, owner-ranking eligibility, and occurrence-separation evidence for Components 09 and 10; and
- complete dependency, provenance, resource, verification, digest, and replay evidence.

### 0.1 Independent review corrections

This reviewed specification makes four integration rules explicit.

1. **Three numerical truth layers are mandatory.** A rounded nominal zero is not an exact geometric relation. Exact expansion or equivalent evidence proves an exact relation over stored nominal bits, not the result of the rounded operation graph and not every realization within inherited uncertainty. Every consumer must see all applicable layers.
2. **Runtime owner tokens are validation state, not semantic identity.** Handles and artifacts remain owner-checked, but raw owner-token values never enter semantic keys, canonical order, canonical bytes, digests, replay-equivalence, or deterministic primary-failure ordering.
3. **Component 07 does not perform final Boolean selection.** It may look up relation-specific symbolic rules and publish owner ranking or conceptual side order. Components 09 and 10 remain responsible for per-atom side classification and final truth-table selection respectively.
4. **Existing Ygor arithmetic and polygon functionality is reused only through qualified bounded capabilities.** Component 07 does not create a second exact-arithmetic subsystem or call legacy predicates directly. Missing formulas are added to Component 03; complete source-polygon region queries reuse or narrowly extend the pure `BoundedSourcePolygonKernel` established by Components 02 and 04.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `canonical_candidate_stream<T,I>` from Component 06;
- the immutable `canonical_source_manifolds<T,I>` from Component 05;
- source-facet semantic rings, triangle groups, projection records, and triangulation provenance from Components 02, 04, and 05;
- validated shell orientation, nesting, and occupied-side semantics from Component 02;
- the immutable `precision_context<T>` and narrow Component 03 services for bounded predicates, exact stored-coordinate relations, constructions, residuals, conditioning, interval ordering, precision lineage, and tolerance disposition;
- the immutable Boolean context, operation, truth-table version, frozen symbolic contact-policy matrix, stable semantic identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, transactions, and execution services from Component 01;
- the selected relation-artifact, dependency-graph, formula, symbolic-policy, event-seed, codec, and verifier versions; and
- verification settings governing scalable checks and bounded exhaustive oracles.

The component must not read mutable caller meshes, mutate predecessor artifacts, use provider-local identities at its public boundary, infer source-feature equivalence from coordinate values, or place runtime owner tokens into semantic records.

### 1.2 Required predecessor guarantees

The component may rely on predecessor artifacts having established:

- every directed candidate identifies one canonical undirected edge representative from one operand and one canonical oriented source triangle from the opposite operand;
- both operand directions are represented completely under the frozen Component 06 candidate-domain policy;
- candidate pruning has no false negatives under the conservative bound contract;
- every source triangle, halfedge, canonical edge, source edge, internal diagonal, source facet, shell, and source vertex has a stable context-owned handle and an owner-free complete semantic key;
- both incident halfedge uses of every undirected edge are recoverable;
- source edges and facet-internal triangulation diagonals are distinguishable without geometry;
- every source triangle carries total source-facet, shell, orientation, geometry-basis, and bounded planar-support provenance;
- source-facet triangulations completely cover their accepted polygonal facets;
- all source points and construction inputs have finite conservative enclosures;
- Component 03 exposes rounded nominal, exact stored-coordinate relation, and uncertainty evidence as separate fields; and
- all predecessor arrays and keys are canonically ordered or have immutable canonical-order mappings.

The component must defensively validate runtime ownership, operand roles, ranges, versions, digests, verification dispositions, formula capabilities, and dependency references before use. A contradiction in a committed predecessor artifact is an `internal_invariant_error`; it must not be converted into a no-contact relation.

### 1.3 Runtime ownership and semantic identity

Every checked handle or view must validate the invocation-owned runtime owner token before dereference. Wrong, stale, or cross-context handles fail deterministically.

Raw runtime owner-token values must not appear in:

- relation, construction, symbolic-decision, seed, or disposition semantic keys;
- canonical table ordering or ID assignment;
- canonical bytes or digests;
- replay-equivalence comparisons;
- deterministic primary-failure ordering; or
- user-visible canonical diagnostics.

Semantic keys use stable context, operand, source-feature, formula, and policy identities, including the stable context digest or equivalent owner-free namespace where needed. Two semantically identical invocations under different runtime owner anchors must produce identical Component 07 semantic bytes and IDs while cross-owner access still fails.

### 1.4 Relation domain

The public relation domain is expressed in original source-feature terms even when discovery begins from triangle-level candidates. It must cover, directly or through composites:

- source vertex against opposite source-facet support and region;
- source edge against opposite source edge;
- source edge against opposite source facet;
- source facet against opposite source facet where required for coplanarity, coincidence, or a shared transverse carrier;
- triangle-local subrelations needed to prove candidate and source-facet coverage; and
- symbolic contact relations involving source vertices, source edges, source facets, and their oriented uses.

Facet-internal triangulation diagonals may participate as bookkeeping witnesses under the Component 06 V1 candidate-domain policy. They must not acquire original source-feature ownership, independent symbolic priority, crossing-barrier ownership, or final event identity merely because discovery passed through a diagonal.

A concave source facet may intersect one source edge in more than one point or interval. The relation model therefore permits a composite source-edge/source-facet relation to own zero, one, or several canonically ordered local event seeds and contact intervals.

### 1.5 Accepted relation cases

The component must support all cases admitted by the broad plan, including:

- definite separation;
- proper transverse edge-face crossings;
- endpoint crossings;
- vertex-on-face, vertex-on-edge, and vertex-on-vertex contact;
- proper and endpoint edge-edge contact;
- tangency with zero numeric net crossing;
- coplanar disjointness;
- coplanar point or segment contact;
- coplanar partial overlap and containment;
- equal or coincident source edges;
- partially or fully coincident source facets;
- coincident facets with the same or opposite orientation;
- several source features meeting at one nominal coordinate;
- distinct conceptual events rounding to the same `T` coordinate;
- near-parallel constructions whose conditioning remains within tolerance;
- constructions whose conditioning exceeds tolerance;
- coordinate-coincident but topologically distinct shells;
- signed zero, subnormal values, adjacent representable values, large translations, and extreme finite scales; and
- empty candidate streams.

Exact contact and coincidence are ordinary relation categories. They must not be converted into generic numerical failure merely because a transverse-intersection denominator is exactly zero. Conversely, a zero rounded denominator or an uncertainty interval containing zero is not by itself proof of exact contact or coplanarity.

### 1.6 Frozen symbolic policy input and downstream boundary

Before evaluation begins, Component 01 must provide a total, immutable, versioned symbolic contact-policy matrix. For every supported operation, operand role, relation family, orientation state, boundary role, and transition role, the matrix must specify as applicable:

- conceptual expansion, contraction, or ordered offset direction used to break an eligible exact tie;
- source-feature priority and total tie keys;
- vertex-on-face, vertex-on-edge, vertex-on-vertex, edge-on-face, edge-edge, and equal-edge behavior;
- same-orientation and opposite-orientation coincident-facet ordering behavior;
- half-open ownership rules used to avoid duplicate crossing counts at vertices and edges;
- owner ranking or eligibility for a potentially retained coincident boundary;
- symbolic negative-side and positive-side assignments needed by Component 09;
- behavior for equal operands and point-, edge-, and face-touching solids;
- occurrence-separation requirements;
- operand-remapping behavior under exchange of A and B; and
- the expected regularized semantics of each tie category.

The matrix must distinguish operation-neutral geometry from operation-specific symbolic relation consequences. A Component 07 symbolic decision may identify conceptual ordering, side assignment, half-open crossing ownership, owner rank or eligibility, and occurrence separation. It must not claim that a positive-area atom is finally retained, discarded, suppressed, or output-oriented. Component 10 performs those decisions after Component 09 supplies complete per-atom side labels and after the Component 01 truth table is applied exactly at the selection boundary.

### 1.7 Capacity and lifetime preconditions

Before relation evaluation, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all candidate-to-relation and candidate-to-disposition references;
- all canonical primitive and composite relation keys;
- all source-edge/source-facet and source-facet/source-facet records;
- all dependency edges and reverse-consumer lists;
- all bounded predicate, exact-relation, and construction records;
- all symbolic eligibility and decision records;
- all event-seed descriptors, overlap intervals, and incidence records;
- all task-local and canonical merge storage;
- all verification evidence, diagnostics, canonical bytes, and replay data; and
- worst-case work up to configured relation, construction, predicate, overlay, seed, and verifier limits.

Published records may reference only immutable predecessor artifacts and immutable stage-owned storage whose lifetime covers Components 08-15.

## 2. Required behavior

### 2.1 Canonical compute-once dependency graph

The component must define a fixed, versioned, acyclic dependency graph for authoritative relation evaluation. Equivalent mathematical questions map to one canonical relation key and one producer.

At minimum, the graph separates:

1. imported bounded points, supports, frames, and canonical directions;
2. primitive bounded support-side predicates;
3. primitive exact stored-coordinate relation requests;
4. projected or intrinsic source-facet region predicates;
5. source edge/source edge relations;
6. source edge/source facet relations;
7. source facet/source facet support and carrier relations;
8. coplanar overlay and coincidence relations;
9. composite crossing, tangency, contact, and coincidence classification;
10. bounded construction records;
11. numeric crossing multiplicity and conservation reducers;
12. symbolic eligibility records;
13. operation-specific symbolic relation decisions; and
14. event-seed and candidate-disposition records.

A later relation may consume earlier immutable records. It must not recompute their arithmetic independently. Repeated requests for one source vertex versus one source-facet support share one record. Several triangle candidates that discover one source-edge/source-facet event feed one source-feature composite rather than numerically independent facts.

The graph must be canonically ordered and independently serializable. Cycles, duplicate producers, missing dependencies, forward dependencies not admitted by the family policy, or two authoritative values for one canonical key prevent publication.

### 2.2 Canonical relation identity

Every relation has a collision-safe complete semantic key containing enough information to distinguish:

- relation family and schema version;
- stable context digest or equivalent owner-free semantic namespace;
- operand roles;
- canonical source-feature semantic identities;
- directed source-feature use where orientation matters;
- source-facet or triangle-local role;
- dependency-graph and formula/provider versions where observable;
- symbolic-policy version for symbolic records; and
- an occurrence discriminator only when required to preserve genuinely distinct contacts.

Hashes may accelerate lookup, but complete keys determine equality. Relation IDs are assigned only after deterministic canonicalization. They do not depend on candidate discovery order, worker number, hash insertion order, pointer address, runtime owner token, nominal coordinate order, or uncertainty width.

Triangle-local records retain a total mapping to a source-facet composite relation or a documented no-public-relation disposition. Internal-diagonal records remain explicitly bookkeeping-only.

### 2.3 Primitive predicate truth model

Every topology-affecting primitive predicate must preserve four distinct kinds of evidence:

1. **rounded nominal:** exact bits produced by the prescribed rounded `T` operation graph;
2. **bounded sign:** definitely negative, definitely positive, overlaps the decision boundary, or invalid under the conservative uncertainty enclosure;
3. **exact stored-coordinate relation:** exact negative, exact zero, exact positive, unavailable, or invalid for the separately versioned algebraic relation over stored nominal bits; and
4. **consumer disposition:** accept a definite numeric sign, retain exact-tie evidence for relation-specific eligibility, try one permitted deterministic alternate formulation, route to coplanar/coincident handling, fail unresolved, or fail invalid.

A compatibility five-way presentation may expose definitely negative, exact-relation tie evidence, definitely positive, uncertain, or invalid, but it must not erase the orthogonal source fields.

Each primitive record includes:

- rounded nominal scalar bits;
- finite conservative enclosure;
- exact-relation formula and evidence when requested;
- sign margin when bounded definite;
- uncertainty contributors and width when non-definite;
- source and operation provenance;
- rounded-operation and exact-relation trace identifiers;
- conditioning and tolerance disposition;
- selected deterministic formulation and any admitted alternate attempt; and
- precision-ledger references.

A rounded nominal zero is diagnostic only. It does not establish an exact tie. An exact stored-coordinate zero may coexist with an uncertainty enclosure that permits both signs; that combination remains visible and is not automatically symbolically eligible. Ordinary unresolved uncertainty triggers the one contractually permitted alternate formulation, coplanar routing when independently established, or a typed geometric failure.

### 2.4 Source-facet support and region relations

A source vertex or constructed candidate point tested against a source facet requires separate support and region facts:

- relation to the accepted oriented planar support; and
- relation to the complete bounded polygonal region of the source facet on that support.

Region classification is deterministic, bounded, based on the source facet's committed geometry basis and original semantic boundary, and independent of facet-internal triangulation diagonals. Triangle-local tests may supply coverage witnesses, but the final source-facet relation distinguishes:

- strictly inside the facet region;
- on an original source edge;
- on an original source vertex;
- outside the facet region;
- on a facet-internal triangulation diagonal only;
- coplanar overlap interval or region;
- unresolved; and
- invalid.

A point on an internal diagonal but inside the source polygon remains interior unless an original source feature also owns the location. Complete original boundary-feature ownership is preserved; the implementation must not choose one owner by coordinate proximity.

### 2.5 Source edge/source edge relations

The component provides canonical relations for potentially interacting original source edges, including:

- definite skew or planar separation;
- proper point intersection;
- endpoint/interior and endpoint/endpoint contact;
- parallel disjointness;
- collinear point contact;
- partial overlap, containment, or equality;
- equal geometric support with distinct topological identities;
- same or opposite canonical direction; and
- uncertain, ill-conditioned, or invalid cases.

For point relations, output identifies bounded parameters on both carriers, exact endpoint identity where applicable, one authoritative source point or construction, all source-feature owners, and residual evidence. For overlap, output identifies bounded closed parameter intervals on both edges, orientation agreement or opposition, and canonical endpoint seeds without collapsing distinct occurrences.

A public edge-edge relation is keyed by original source-edge identities. A facet-internal diagonal may be a discovery or coverage witness but cannot own a cross-operand source-edge event.

### 2.6 Source edge/source facet relations

For every required canonical source-edge/source-facet pair, the component classifies the complete relation of the closed source edge to the bounded source polygon. Results may contain:

- no contact;
- one or more proper crossings;
- endpoint contacts;
- tangent contacts;
- coplanar point or segment contacts;
- one or more coplanar overlap intervals for concave facets;
- complete containment of an edge segment in the facet region;
- contacts owned by original facet edges or vertices; and
- bookkeeping-only triangle-boundary witnesses.

A transverse local event includes:

- a bounded edge parameter;
- an accepted source point or one authoritative bounded point construction;
- complete source-facet region classification;
- source-feature ownership at the event;
- occupied/unoccupied side state before and after the event along the canonical directed edge;
- signed numeric crossing multiplicity;
- separate symbolic side or crossing metadata when eligible; and
- residual, conditioning, tolerance, and precision evidence.

A tangent event has zero numeric net crossing unless the half-open source-fan rule identifies a true boundary transition owned by one incident occurrence. A purely symbolic contribution remains separate from numeric multiplicity.

The composite relation accounts for the full source facet, not merely the triangle that admitted the candidate. Duplicate triangle-local hits collapse only through common lineage. Truly separate crossings or intervals of a concave facet remain separate ordered occurrences.

### 2.7 Source facet/source facet support and overlay relations

Where edge/facet relations indicate a common source-facet pair, the component provides one canonical support relation that distinguishes:

- definitely nonparallel supports with a stable transverse carrier;
- parallel separated supports;
- exact stored-coordinate coplanarity supported by qualified relation evidence;
- bounded uncertainty that cannot establish separation or coplanarity;
- same or opposite oriented support directions; and
- invalid or unrepresentable carrier construction.

For a stable nonparallel pair, the relation provides one bounded carrier representation, canonical orientation derived from source semantic keys and oriented supports, conditioning evidence, and residual checks.

For coplanar facets, the relation preserves common-support identity and builds overlap evidence from original source boundaries. It must represent disjointness, point/segment contact, partial area overlap, containment, equal regions, same/opposite orientation, and topologically distinct coincident sheets. It must not invent a transverse line or use approximate normal/plane proximity as coplanarity proof.

### 2.8 Signed crossing multiplicity

Every event that can change opposite-operand winding carries one authoritative signed integer numeric crossing contribution.

The frozen convention is:

- traverse the canonical directed source edge from parameter zero to one;
- interpret the opposite source facet through validated occupied/unoccupied sides;
- `+1` means opposite-operand winding after the event is one greater than before;
- `-1` means it is one smaller; and
- `0` means no numeric net crossing.

Endpoint, edge, and vertex ties use the frozen source-fan half-open ownership rule so a crossing shared by incident facets contributes exactly the intended total. The sum must not depend on triangle discovery or source-facet triangulation.

The artifact distinguishes:

- numeric transverse multiplicity;
- numeric zero-multiplicity contact;
- symbolic side or crossing contribution used only to define eligible tie semantics; and
- invalid or inconsistent multiplicity.

Component 09 must reconstruct every adjacency delta without re-running relation predicates.

### 2.9 Tangency and zero-measure contact

Tangency, point contact, and edge contact remain explicit even when numeric multiplicity is zero. Each record states:

- contact dimension;
- owning source features on both operands;
- local orientation or fan evidence;
- whether the contact separates source-surface classification regions;
- whether symbolic policy introduces distinct conceptual sides;
- coincidence or owner-ranking eligibility;
- occurrence-separation requirements; and
- whether topology remains separated despite coordinate coincidence.

Zero-measure contacts are not discarded by Component 07. Components 08-10 need them for cut structure, side labels, regularized semantics, and prevention of accidental welding.

### 2.10 Coplanar and coincident evidence for downstream classification and selection

Coplanar geometry is handled through explicit source-feature relations, not tolerance snapping or arbitrary ownership. Operation-neutral records preserve:

- overlap components and dimensions;
- support orientation;
- source-boundary lineage;
- sheet and occurrence identities;
- side occupancy supplied by each validated source shell; and
- numeric crossing/contact information.

For each eligible exact tie or coincident component, Component 07 may publish operation-specific symbolic relation evidence:

- conceptual relative ordering of sheets;
- symbolic negative-side and positive-side assignments;
- half-open boundary ownership;
- owner rank or eligibility among source-sheet occurrences;
- symbolic crossing contribution where the policy requires it; and
- occurrence-separation constraints.

These fields are constraints and evidence for Components 09 and 10. Component 07 must not:

- evaluate final per-atom Boolean result occupancy;
- declare a positive-area atom finally retained, discarded, suppressed, or cancelled;
- prescribe final output orientation; or
- collapse several source partitions into one final output surface.

Component 09 reconstructs atom side labels from relation and event structure. Component 10 applies the truth table and resolves final coincident-sheet ownership jointly over complete positive-area atoms.

### 2.11 Symbolic perturbation eligibility

Symbolic perturbation is permitted only when the relation has a stable eligible reason, such as:

- exact stored-coordinate zero under a qualified exact-relation formula plus a supported structural relation category;
- exact shared source endpoint identity;
- exact collinear source-edge lineage;
- exact coplanar source-facet lineage;
- exact equal source-feature lineage;
- a versioned representational-tie contract explicitly admitted by Components 01 and 03; or
- a versioned coincident-source contract with equivalent exact evidence.

Rounded nominal zero alone is never eligible. An uncertainty enclosure containing zero alone is never eligible. Exact stored-coordinate zero is necessary for some categories but not sufficient by itself: the record must also show the structural relation category and why applying the symbolic rule is compatible with the accepted bounded representative and tolerance contract.

Symbolic policy must not hide:

- an enclosure whose possible signs would change contact dimension or crossing topology without an admitted tie contract;
- a near-parallel construction whose uncertainty exceeds tolerance;
- inconsistent duplicate relations;
- contradictory source-facet coverage;
- an unrepresentable finite bound;
- resource exhaustion; or
- missing exact-relation capability.

An ineligible unresolved relation returns `geometric_condition_exceeds_tolerance` or another precise typed failure.

### 2.12 Symbolic decision production

For every eligible tie, the component creates one immutable symbolic decision record containing:

- canonical symbolic-decision ID and owner-free semantic key;
- the relation ID it resolves;
- operation and operand roles;
- policy version and rule identifier;
- exact eligibility reason and evidence references;
- complete source-feature tie key;
- conceptual ordering or side assignment;
- half-open crossing owner or symbolic contribution where applicable;
- coincident owner rank or eligibility, not final retained ownership;
- occurrence-separation requirements;
- operand-swap remapping; and
- deterministic explanatory diagnostics.

A decision is total for its relation category. It does not consult traversal order, worker number, hash order, pointer address, runtime owner token, or an unversioned implementation detail.

The same operation-neutral relation may receive different symbolic decisions under different Boolean operations. Its numeric relation and stored nominal geometry remain unchanged.

### 2.13 Bounded construction records

Whenever a relation requires a point, parameter, interval endpoint, direction, or carrier, the component obtains it through Component 03 construction services and publishes the complete bounded result.

A point construction includes:

- nominal coordinate bits in `T`;
- finite axis and radial enclosure or equivalent conservative representation;
- source-feature lineage;
- bounded parameters on every defining carrier;
- residuals against every defining source edge, support, or carrier;
- conditioning classification;
- tolerance disposition;
- precision-ledger reference; and
- one authoritative producer relation.

Endpoint events reference the accepted source bounded point rather than recomputing interpolation at parameter zero or one. Equivalent formulas may be evaluated only as explicitly non-authoritative verification witnesses. They must not create a second authoritative nominal coordinate.

### 2.14 Event-seed production

The component emits canonical event seeds for Component 08. A seed is a lineage and incidence description, not yet an event ID.

Each seed identifies:

- authoritative relation and construction record or accepted source point;
- source-feature owners on both operands;
- every source edge, facet, triangle, and oriented halfedge use that consumes the event;
- contact dimension and relation class;
- numeric crossing multiplicity and separate symbolic metadata;
- candidate and triangle-local discovery provenance;
- canonical event-equivalence key supplied by relation lineage;
- a distinct-occurrence key when equal coordinates must remain separate;
- expected source-edge, transverse-carrier, or coplanar-carrier memberships; and
- bounded parameters, residuals, conditioning, and precision references.

Event-equivalence is never derived from coordinate equality, distance, spatial buckets, equal nominal parameters, or overlapping uncertainty envelopes. Only exact source identity and canonical relation/construction lineage authorize interning.

### 2.15 Candidate coverage and duplicate discovery

Every Component 06 candidate ends in exactly one documented disposition:

- definitely separated;
- absorbed as duplicate discovery of a canonical source-feature relation;
- contributed a primitive dependency;
- contributed one or more event seeds;
- contributed a coplanar or coincident relation;
- retained as zero-measure contact;
- retained as bookkeeping-only internal-diagonal coverage evidence; or
- caused a typed failure.

No candidate disappears without a stable disposition code. Duplicate triangle-level discovery is retained sufficiently for exhaustive candidate and source-facet coverage verification.

All triangle-local relations belonging to one source-facet composite must be mutually consistent. Missing coverage, contradictory signs, incompatible constructions, unexplained duplicate seeds, or triangulation-dependent public semantics prevent publication.

### 2.16 Deterministic evaluation and parallel merge

The executable serial implementation is the semantic reference. Parallel evaluation may use only immutable inputs, task-local caches, and private output fragments. Publication must:

- canonicalize complete request keys before assigning IDs;
- evaluate each authoritative key once;
- record duplicate requests as consumers;
- merge task outputs by complete semantic key;
- select one deterministic bounded formulation where alternatives are contractually available;
- detect conflicting duplicate values;
- assign relation, construction, symbolic-decision, and seed ordinals canonically;
- replay Component 03 trace fragments in canonical order;
- choose the same primary failure under every schedule; and
- commit only after complete producer and independent verification.

No shared cache winner, allocation race, worker number, or completion order may affect semantics.

### 2.17 Resource limits, cancellation, and transactionality

The component separately accounts for:

- candidate requests and dispositions;
- primitive and composite relations;
- bounded predicate and exact-relation operations;
- alternate formulations;
- construction records;
- dependency edges and consumer lists;
- coplanar overlay points, arcs, intervals, and components;
- crossing contributions and conservation evidence;
- symbolic eligibility and decisions;
- event seeds and incidence;
- task-local caches and canonical merge storage;
- codec, diagnostics, replay, and verifier work; and
- persistent artifact bytes.

Limit exhaustion produces deterministic `resource_limit` or `index_overflow` without truncating relations, weakening the symbolic matrix, dropping evidence, or simplifying event seeds.

Cancellation is polled at deterministic safe points during validation, preflight, request discovery, closure, each relation family, construction, symbolic resolution, merge, encoding, and verification. All workers join and all private reservations roll back before returning `cancelled`. No partial artifact becomes visible to Component 08.

### 2.18 Independent verification evidence

The artifact publishes enough evidence for an independent verifier to check:

- candidate-to-disposition completeness;
- uniqueness and total ordering of owner-free semantic keys;
- runtime owner validation without owner-token participation in semantics;
- acyclicity and completeness of the dependency graph;
- all rounded-nominal, exact-relation, bounded-sign, and disposition fields;
- source-facet coverage independent of internal diagonals;
- construction residuals and conditioning classifications;
- event-seed lineage and occurrence keys;
- numeric crossing multiplicities and local conservation;
- symbolic eligibility and exact policy lookup;
- absence of final Component 10 selection decisions;
- operand-swap remapping;
- coplanar/coincident grouping, side, rank, and occurrence evidence;
- deterministic digest inputs; and
- absence of coordinate-based identity.

For bounded fixtures, the verifier compares the complete relation set against independently controlled in-tree exact rational and exhaustive source-feature oracles. Production-scale verification may use structural and configured sampled arithmetic checks, but release qualification requires exhaustive bounded domains.

The verifier must not use the producer's request-cache lookup, dependency scheduler, composite assembler, multiplicity reducer, symbolic dispatcher, or seed deduplicator as its sole source of truth.

## 3. Output contract

On success, the component produces one immutable `signed_feature_relations<T,I>` artifact containing or referencing:

- artifact, formula, dependency-graph, symbolic-policy, seed, codec, and verifier versions;
- stable context and predecessor semantic digests plus runtime owner-checked handles whose raw tokens are noncanonical;
- canonical primitive and composite relation tables;
- canonical relation IDs, owner-free complete keys, and total order;
- a complete acyclic dependency graph and reverse-consumer mappings;
- candidate-to-disposition and candidate-to-relation mappings;
- rounded-nominal, bounded-sign, exact-relation, and consumer-disposition evidence;
- bounded support-side, region, edge-edge, edge-facet, facet-facet, coplanar, and composite records;
- operation-neutral contact, coplanarity, coincidence, tangency, and crossing classifications;
- bounded construction records with precision-ledger references;
- authoritative signed numeric crossing multiplicities;
- symbolic eligibility and decision records for every applicable exact tie;
- coincidence side/order/rank and occurrence-separation evidence for Components 09 and 10;
- canonical event seeds and distinct-occurrence keys for Component 08;
- source-feature, triangle-local, source-facet, shell, and operand provenance;
- deterministic downstream work partitions;
- resource and structural statistics;
- independent-verification evidence;
- canonical section and complete digests; and
- replay metadata sufficient to reproduce every numerical and symbolic disposition.

The artifact guarantees:

- every Component 06 candidate has one documented disposition;
- every topology-affecting geometric question has one producer;
- every consumer references the same immutable relation and construction;
- rounded nominal zero, exact stored-coordinate zero, and unresolved bounded uncertainty remain distinct;
- unresolved uncertainty is never silently resolved by symbolic policy;
- every non-zero numeric crossing has a frozen orientation and signed multiplicity;
- zero-measure contacts remain represented;
- source-facet semantics are independent of internal diagonals;
- all event seeds are keyed by lineage rather than coordinates;
- distinct conceptual events may have identical nominal coordinates and overlapping bounds;
- symbolic decisions affect relation classification, conceptual side, ranking, and occurrence constraints, not stored nominal geometry;
- no record claims final positive-area surface retention, suppression, or output orientation;
- operand exchange and operation remapping produce the documented remapped relation set;
- semantic IDs, ordering, diagnostics, and digests exclude runtime owner-token values and are schedule-independent; and
- Components 08 and 09 can consume the artifact without repeating authoritative geometry.

A valid empty candidate stream produces a canonical empty relation artifact with valid versions, predecessor digests, zero counts, and deterministic bytes. It does not invent containment relations; Component 09 handles disconnected global containment through classification seeding.

On failure, no relation artifact is published. The typed error identifies canonical relation or candidate witnesses, operand roles, source features, rounded nominal bits, enclosures, exact-relation evidence, conditioning and tolerance state, symbolic eligibility where relevant, policy versions, resource counters, and deterministic replay identity.

## 4. Required invariants and prohibited behavior

Required invariants:

- topology-affecting arithmetic is bounded and uncertainty-aware;
- rounded nominal, exact stored-coordinate relation, and uncertainty evidence remain orthogonal;
- one canonical relation key has one authoritative producer and value;
- relation dependencies are acyclic and complete;
- semantic keys and bytes exclude raw runtime owner tokens;
- source-feature lineage survives triangle-level discovery;
- internal diagonals never own original source-feature semantics;
- a definite bounded sign is backed by a conservative margin;
- symbolic policy is total, versioned, operation-aware, and applied only when eligible;
- symbolic decisions never change nominal coordinates or perform final Boolean selection;
- each non-zero numeric crossing has signed integer multiplicity;
- duplicate triangle discoveries do not duplicate source-feature events;
- distinct conceptual events are never merged by coordinate coincidence;
- all constructions carry finite conservative bounds, residuals, conditioning, and precision evidence;
- all artifacts are immutable, transactional, deterministic, and independently verifiable; and
- resource or conditioning limits cause typed failure rather than guessed classification.

Prohibited behavior:

- raw `==`, `<`, or `>` on topology-affecting floating values outside Component 03's qualified contract;
- treating rounded nominal zero as exact relation evidence;
- treating every enclosure containing zero as an exact symbolic tie;
- using tolerance as a universal equality, coplanarity, ownership, snapping, or welding rule;
- recomputing one relation through different formulas in different consumers;
- averaging or selecting among inconsistent constructions;
- assigning event equivalence from coordinate equality, distance, bounds overlap, hashes, or spatial cells;
- allowing an internal diagonal to own a source contact;
- choosing symbolic behavior from traversal or triangle order;
- encoding runtime owner tokens in semantic keys, bytes, digests, or replay;
- random perturbation, stochastic rays, or unversioned lexicographic hacks;
- converting near-parallel uncertainty beyond tolerance into a fabricated point;
- discarding tangencies or contact because numeric multiplicity is zero;
- applying the final Boolean truth table to select positive-area atoms in Component 07;
- declaring final retained/suppressed/output-oriented surfaces;
- publishing partial relation tables after cancellation or resource exhaustion; or
- calling an external predicate, exact-arithmetic, mesh, collision, or geometry library.

## 5. Test and validation specification

### 5.1 Three-layer predicate unit tests

Unit tests cover every combination that is valid for:

- rounded nominal negative, zero, and positive bits, including signed zero;
- bounded sign definitely negative, definitely positive, overlaps boundary, and invalid;
- exact relation negative, zero, positive, unavailable, and invalid; and
- consumer dispositions including definite, alternate, coplanar route, exact-tie candidate, unresolved failure, and invalid failure.

Predicate families include support residual, projected orientation, segment parameter, interval order, edge-edge support, facet/facet parallelism, and construction residual. Cases include subnormals, adjacent values, cancellation, large translation, extreme finite exponents, and denominator enclosures containing zero.

Explicitly test:

- rounded zero with exact nonzero relation;
- rounded nonzero with exact zero relation where the rounded graph differs;
- exact zero with a sign-spanning uncertainty enclosure;
- uncertainty overlap with unavailable exact relation;
- definite bounded sign with supplemental exact relation; and
- rejection of symbolic eligibility based only on rounded zero.

### 5.2 Relation matrix known-answer tests

Commit hand-auditable expected artifacts for:

- definite separation;
- proper and endpoint edge/facet crossing;
- edge crossing through a source edge or vertex;
- proper edge-edge crossing;
- endpoint/interior and endpoint/endpoint contact;
- tangent vertex/facet and edge/facet contact;
- coplanar disjoint edges and facets;
- collinear point contact, overlap, containment, and equal edges;
- coplanar partial facet overlap;
- equal facets with same and opposite orientation;
- several incident triangles discovering one event;
- a concave facet with several crossings or intervals;
- distinct events that round to one coordinate; and
- empty candidate streams.

Expected artifacts include complete keys, dependencies, all truth layers, constructions, bounds, multiplicities, symbolic rule IDs, owner-rank evidence, occurrence constraints, event-seed lineage, dispositions, and digests.

### 5.3 Symbolic policy matrix tests

For every operation and operand role, exhaustively test:

- equal operands;
- same- and opposite-orientation coincident facets;
- vertex-on-face, vertex-on-edge, and vertex-on-vertex;
- edge-on-face, equal edges, and partial edge overlap;
- point-, edge-, and face-touching solids; and
- half-open crossing ownership at shared vertices and edges.

Verify matrix totality, unique lookup, operand exchange, exact eligibility, coordinate-bit preservation, conceptual side assignment, owner-rank/eligibility output, and occurrence separation.

Verify Component 07 emits no final per-atom retain/discard/suppress/output-orientation field and never calls the final selection truth-table path.

### 5.4 Exact-oracle differential tests

Normative tests use the in-tree exact rational oracle from Component 16 infrastructure for bounded integer-coordinate fixtures. Compare exact:

- support and orientation relations;
- edge-edge and edge-facet categories;
- event parameters;
- coplanarity and orientation;
- source-facet region ownership;
- numeric crossing multiplicities; and
- containment of every exact construction in its enclosure.

Production code does not depend on the oracle.

### 5.5 Dependency and compute-once tests

Instrument requests and verify:

- repeated vertex/facet support consumers create one producer;
- repeated exact-relation requests create one producer distinct from rounded-operation evidence;
- multiple triangle candidates of one source facet share composites;
- legal retriangulation changes bookkeeping requests but not public relations;
- every dependency references an earlier admitted record;
- no duplicate producer exists;
- candidate permutations preserve IDs and bytes; and
- an injected second evaluation with changed nominal, enclosure, exact relation, or category is rejected.

### 5.6 Crossing conservation tests

Construct high-valence source vertices and shared edges and verify:

- half-open ownership counts each true crossing exactly once;
- tangencies sum to zero;
- entering and leaving crossings have opposite sign;
- canonical edge reversal negates numeric multiplicity under the documented remap;
- shell orientation reversal with corrected occupied-side semantics remaps signs consistently;
- closed local fans are conservative; and
- alternative source-facet triangulations preserve total source-feature multiplicity.

### 5.7 Coplanar and coincidence tests

Include disjoint coplanar polygons, point/edge contact, partial area overlap, containment, equal facets, same/opposite orientation, concave overlap with multiple components, coordinate-coincident distinct sheets, and boundaries through original vertices/edges.

Verify source-feature ownership, event-seed keys, symbolic side/order and owner-rank evidence, occurrence separation, and triangulation invariance. Then pass the artifacts through Component 09/10 test doubles and verify final selection is made there, not precommitted by Component 07.

### 5.8 Conditioning boundary tests

For near-parallel edge-plane, edge-edge, facet-carrier, projected-region, and parameter-order operations, test comfortably conditioned, just inside tolerance, exactly at threshold, just outside, exact relation zero with zero-containing denominator, zero-containing denominator without exact evidence, large translation, and extreme finite scales.

Category, formula, nominal construction, enclosure, residual, and typed failure are deterministic. Symbolic policy does not rescue ineligible uncertainty.

### 5.9 Owner and semantic-identity tests

Run semantically identical fixtures under distinct runtime owner anchors. Require identical semantic relation keys, IDs, canonical bytes, digests, replay-equivalence records, and canonical failure ordering. Require wrong-owner, stale-owner, and cross-owner handles to fail before dereference.

Mutate encoded data to include an owner-token-derived field and require codec/verifier rejection.

### 5.10 Metamorphic tests

Apply operand exchange with operation remapping, source vertex/edge/facet/shell permutations, facet-ring rotation, legal subdivision and retriangulation, axis permutation, sign flip with corrected orientation, exactly representable translation, power-of-two scaling with precision scaling, canonical edge reversal, thread counts 1/2/maximum, forced delays, and reversed merge order.

After documented remapping, public relation keys, all truth layers, constructions, bounds, symbolic decisions, multiplicities, seeds, diagnostics, and digests are byte-identical for a fixed semantic provider/policy.

### 5.11 Mutation tests

Corrupt valid artifacts by:

- deleting a candidate disposition;
- duplicating a relation producer;
- introducing a dependency cycle;
- changing a source-facet owner to an internal diagonal;
- shrinking a construction enclosure;
- changing a residual or conditioning category;
- coercing uncertainty to a definite sign;
- replacing exact-relation evidence with rounded-zero evidence;
- marking ineligible uncertainty symbolically eligible;
- changing a symbolic rule, side assignment, rank, or occurrence constraint;
- inserting a final retained/discarded/output-orientation decision into a Component 07 symbolic record;
- flipping a crossing sign or zero/nonzero status;
- merging event seeds by coordinate;
- splitting one canonical lineage event;
- scrambling canonical ordering;
- inserting a raw owner token into semantic bytes; and
- forging counts or digests.

Independent verification rejects every mutation.

### 5.12 Fuzzing, resources, cancellation, and concurrency

Generate valid manifold operand pairs from exact templates while varying valence, concavity, triangulation, overlap dimension, shell nesting, duplicate coordinates without identity merging, ULP perturbations, near-parallel angles, translation, scale, input precision, tolerance, and symbolic categories.

Every crash, nondeterministic result, oracle disagreement, truth-layer conflation, invalid symbolic eligibility, conflicting duplicate relation, bad multiplicity, owner-token semantic leak, or downstream-boundary violation serializes exact source bits, candidates, policy versions, and relation traces and shrinks while preserving the failure.

For every resource class, test limit-minus-one, limit, and limit-plus-one. Cancel during each checkpoint and long-loop family. Confirm all workers join, reservations return, no partial artifact is visible, and retry produces canonical bytes. Reordered tasks under all supported worker counts produce the serial bytes and primary failure.

### 5.13 Definition of done

Component 07 implementation is complete only when:

- the canonical dependency graph, relation-key domains, formula precedence, and public/bookkeeping reduction policy are frozen and versioned;
- runtime owner validation is complete and raw owner tokens are absent from semantic identity and bytes;
- every Component 06 candidate has a verified disposition;
- rounded nominal bits, exact stored-coordinate relation evidence, conservative uncertainty, and consumer disposition remain separate throughout the artifact;
- rounded zero never authorizes symbolic policy;
- all supported contact and coincidence categories have total symbolic rules;
- every authoritative relation and construction is computed once;
- source-facet semantics are independent of internal triangulation;
- crossing multiplicities agree with exact bounded oracles and local conservation tests;
- symbolic decisions provide only relation-level side/order/rank/occurrence evidence and never final Component 10 selection;
- event seeds provide complete lineage and incidence for Component 08 without coordinate equivalence;
- Components 08 and 09 consume the artifact without recomputing authoritative geometry;
- Component 10 test integration confirms final truth-table selection remains downstream;
- operand-remapped, owner-anchor, retriangulation, and schedule metamorphic tests pass;
- independent verification rejects every required mutation;
- deterministic replay is byte-stable; and
- all production and normative-test code is strict portable C++17, self-contained within Ygor, and uses no external dependency.
