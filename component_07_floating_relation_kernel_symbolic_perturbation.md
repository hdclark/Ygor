# Component 07: Floating Relation Kernel and Symbolic Perturbation

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete predicate formulas, enclosure representation, projection strategy, relation-cache layout, symbolic-key encoding, and work-partitioning provider may change. The compute-once dependency graph, bounded-result categories, source-feature lineage, exact-tie separation, operation-aware symbolic semantics, determinism, verification, and failure contracts in this document are normative.

## 0. Purpose

This component is the authoritative narrow-phase relation engine for the Boolean pipeline. It consumes the conservative directed edge-triangle candidate stream from Component 06 and converts those candidates into one immutable, canonically keyed set of signed geometric relations.

Its purposes are to:

- determine whether canonical source features are definitely separated, in contact, overlapping, tangent, or crossing;
- evaluate every topology-affecting geometric question exactly once under one prescribed dependency graph;
- construct nominal intersection data together with conservative uncertainty and conditioning evidence;
- distinguish a true exact or representational tie from an unresolved numerical uncertainty;
- apply the frozen, operation-specific symbolic perturbation policy to exact ties and coincident configurations;
- assign signed crossing multiplicities and contact ownership facts needed by Components 08-10; and
- preserve source-feature lineage so later stages never recompute an equivalent relation through a different floating-point expression.

The component must treat exact indexed topology and bounded floating-point geometry as separate domains. A relation may state that two distinct topological features occupy the same nominal point or support. That does not merge their identities.

The component does not allocate canonical intersection-event IDs, merge events by coordinate, build cut connectivity, compute global winding, select retained Boolean surfaces, construct output topology, or perform cleanup. Component 08 owns event interning and shared construction lineage; Components 09 and 10 own classification and selection.

The principal output is an immutable `signed_feature_relations` artifact containing:

- canonical primitive and composite relation records;
- operation-neutral bounded numerical facts;
- operation-specific symbolic decisions for exact ties;
- signed crossing and contact contributions;
- canonical event-seed descriptions for Component 08; and
- complete dependency, provenance, resource, verification, and replay evidence.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `canonical_candidate_stream` from Component 06;
- the immutable `canonical_source_manifolds` from Component 05;
- source-facet triangle groups and triangulation provenance from Components 04 and 05;
- validated shell orientation, nesting, and occupied-side semantics from Component 02;
- the immutable `precision_context<T>`, bounded predicate services, construction services, and precision-ledger services from Component 03;
- the immutable Boolean context, operation truth table, frozen symbolic contact-policy matrix, stable identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, and transaction services from Component 01;
- the selected relation-artifact, dependency-graph, and symbolic-policy versions; and
- verification settings governing scalable checks and bounded exhaustive oracles.

The component must not read mutable caller meshes, mutate predecessor artifacts, use provider-local feature identities at its public boundary, or infer source-feature equivalence from coordinate values.

### 1.2 Required predecessor guarantees

The component may rely on the predecessor artifacts having established:

- every directed candidate identifies one canonical undirected edge representative from one operand and one canonical oriented source triangle from the opposite operand;
- both operand directions are represented completely under the frozen candidate-domain policy;
- candidate pruning has no false negatives under the conservative bound contract;
- every source triangle, halfedge, source edge, internal diagonal, source facet, shell, and source vertex has a stable context-owned identity;
- both incident halfedge uses of every undirected edge are recoverable;
- source edges and facet-internal triangulation diagonals are distinguishable without geometry;
- every source triangle carries total source-facet, shell, orientation, and bounded planar-support provenance;
- source-facet triangulations completely cover their accepted polygonal facets;
- all source points and construction inputs have finite conservative enclosures; and
- all arrays and keys are canonically ordered or have immutable canonical-order mappings.

The component must defensively validate context ownership, operand roles, ranges, versions, digests, and dependency references before use. A contradiction in a committed predecessor artifact is an `internal_invariant_error`; it must not be converted into a geometric no-contact relation.

### 1.3 Relation domain

The public relation domain must be expressed in canonical source-feature terms, even when discovery begins from triangle-level candidates. It must cover, directly or through composite relations, at least:

- source vertex against opposite source-facet support and region;
- source edge against opposite source edge;
- source edge against opposite source facet;
- source facet against opposite source facet where required for coplanarity, coincidence, or a shared transverse carrier;
- triangle-local subrelations needed to prove complete source-facet coverage; and
- symbolic contact relations involving source vertices, source edges, source facets, and their oriented uses.

Facet-internal triangulation diagonals may participate as bookkeeping witnesses when required for complete candidate coverage, but they must not acquire original source-feature ownership, independent symbolic ownership, or final event identity merely because a candidate was discovered through that diagonal.

A concave source facet may intersect one source edge in more than one point or interval. The relation model must therefore permit a composite source-edge/source-facet relation to own zero, one, or several ordered local event seeds and contact intervals.

### 1.4 Accepted relation cases

The component must support all cases admitted by the broad plan, including:

- definite separation;
- proper transverse edge-face crossings;
- endpoint crossings;
- vertex-on-face and vertex-on-edge contact;
- proper and endpoint edge-edge contact;
- tangency with zero net crossing;
- coplanar disjointness;
- coplanar point or segment contact;
- coplanar partial overlap;
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

Exact contact and coincidence are ordinary relation categories. They must not be converted into generic numerical failure merely because a denominator is zero in a transverse-intersection formula.

### 1.5 Frozen symbolic policy input

Before evaluation begins, Component 01 must provide a total, immutable, versioned symbolic contact-policy matrix. The matrix must specify, for every Boolean operation and operand role:

- conceptual expansion, contraction, or ordered offset direction used to break exact ties;
- source-feature priority and total tie keys;
- vertex-on-face, vertex-on-edge, edge-on-face, edge-edge, and equal-edge behavior;
- same-orientation and opposite-orientation coincident-facet behavior;
- half-open ownership rules used to avoid duplicate crossing counts at vertices and edges;
- ownership of retained coincident surfaces;
- behavior for equal operands;
- behavior for point-, edge-, and face-touching solids;
- operand-remapping behavior under exchange of A and B; and
- the expected regularized semantics of each tie category.

The matrix must distinguish operation-neutral geometric facts from operation-specific selection consequences. The implementation may encode symbolic rules procedurally or as tables, but the externally observable policy must be total, deterministic, replayable, and independently testable.

### 1.6 Capacity and lifetime preconditions

Before relation evaluation, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all candidate-to-relation references;
- all canonical primitive relation keys;
- all composite source-edge/source-facet and source-facet/source-facet records;
- all dependency edges and reverse-consumer lists;
- all bounded predicate and construction records;
- all symbolic decision records;
- all event-seed descriptors and contact intervals;
- all task-local and merge storage;
- all verification evidence, diagnostics, and replay data; and
- worst-case work up to configured relation, construction, and predicate limits.

Published relation records may reference only immutable predecessor artifacts and immutable stage-owned storage whose lifetime covers Components 08-10 and independent verification.

## 2. Required behavior

### 2.1 Canonical compute-once dependency graph

The component must define a fixed, versioned dependency graph for authoritative relation evaluation. Equivalent mathematical questions must map to one canonical relation key and one producer.

At minimum, the graph must separate:

1. primitive bounded support-side predicates;
2. projected or intrinsic source-facet region predicates;
3. source edge/source edge relations;
4. source edge/source facet relations;
5. source facet/source facet support and carrier relations;
6. composite crossing, tangency, coplanarity, and coincidence classification;
7. bounded construction records; and
8. symbolic decisions for exact ties.

A later relation may consume earlier immutable relation records. It must not recompute their arithmetic independently.

For example, when several triangle candidates require the side of one source vertex relative to one opposite source facet, all consumers must reference the same canonical vertex/facet support relation. When several triangles of one polygonal facet discover the same source-edge/facet event, they must feed one composite source-feature relation rather than create numerically independent public facts.

The dependency graph must be acyclic, canonically ordered, and independently serializable. Cycles, duplicate producers, missing dependencies, or two authoritative values for one canonical key prevent publication.

### 2.2 Canonical relation identity

Every relation must have a complete collision-safe key containing enough information to distinguish:

- relation family;
- operand roles;
- canonical source-feature identities;
- directed source-feature use where orientation matters;
- source-facet or triangle-local role;
- dependency-graph version;
- arithmetic/provider version where observable;
- symbolic-policy version where applicable; and
- any occurrence discriminator required to preserve distinct conceptual contacts.

Hashes may accelerate lookup but full keys determine equality. Relation IDs must be assigned only after deterministic canonicalization. They must not depend on candidate discovery order, worker number, hash insertion order, pointer address, or nominal coordinate sorting.

Triangle-local relation records must retain a total mapping to their source-facet composite relation. Internal-diagonal records must remain explicitly marked as bookkeeping.

### 2.3 Primitive bounded side predicates

The component must obtain all topology-affecting scalar signs through Component 03 bounded services. Primitive side predicates must return at least:

- definitely negative;
- exact or representational tie;
- definitely positive;
- unresolved uncertainty; or
- invalid.

Each record must include:

- nominal scalar value;
- conservative enclosure;
- sign margin when definite;
- source and operation provenance;
- arithmetic trace or stable trace identifier;
- conditioning and tolerance disposition;
- selected deterministic formulation; and
- precision-ledger references.

An unresolved enclosure containing zero is not automatically an exact tie. Symbolic perturbation may resolve only a tie category admitted by the frozen policy and supported by exact topological or representational evidence. Ordinary numerical uncertainty must trigger a deterministic alternate bounded formulation if one is contractually available, or a typed geometric failure.

### 2.4 Source-facet support and region relations

A source vertex or constructed candidate point tested against a source facet requires two conceptually separate facts:

- relation to the accepted oriented planar support; and
- relation to the bounded polygonal region of the source facet on that support.

The region test must be deterministic, bounded, and independent of the current source triangulation's internal diagonals. Triangle-local tests may provide coverage witnesses, but the final source-facet relation must distinguish:

- strictly inside facet region;
- on an original source edge;
- on an original source vertex;
- outside facet region;
- on a facet-internal triangulation diagonal only;
- coplanar overlap interval or region; and
- unresolved or invalid.

A point lying on an internal diagonal but inside the source polygon remains interior to the source facet unless another original source feature also owns the location.

The component must preserve all original boundary-feature identities involved in a boundary relation. It must not choose one edge or vertex from approximate coordinate proximity.

### 2.5 Source edge/source edge relations

The component must provide canonical relations for potentially interacting source edges, including:

- definite skew or planar separation;
- proper point intersection;
- endpoint/interior contact;
- endpoint/endpoint contact;
- parallel disjointness;
- collinear point contact;
- partial collinear overlap;
- equal geometric support with distinct topological identities;
- equal source endpoints by identity where legitimately shared within one operand; and
- uncertain or ill-conditioned cases.

For point relations, the output must identify both carrier parameters with conservative bounds and the owning source features on each operand. For overlap, the output must identify a bounded interval on each source edge, orientation agreement or opposition, and canonical endpoint seeds without collapsing distinct topological occurrences.

An edge-edge relation discovered through triangle candidates must be canonicalized by original source-edge identities. A facet-internal diagonal may serve only as a witness that a point lies inside a source facet; it cannot become the owner of a cross-operand source-edge event.

### 2.6 Source edge/source facet relations

For every required canonical source-edge/source-facet pair, the component must classify the complete relation of the closed source edge to the bounded polygonal facet. The result may contain:

- no contact;
- one or more proper crossings;
- endpoint contacts;
- tangent contacts;
- coplanar point or segment contacts;
- one or more coplanar overlap intervals for concave facets;
- complete containment of an edge segment in the facet region;
- contacts owned by original facet edges or vertices; and
- bookkeeping-only triangle-boundary witnesses.

A transverse event must include:

- a bounded edge parameter;
- a bounded point construction or stable endpoint reference;
- facet-region classification;
- source-feature ownership at the event;
- oriented side state before and after the event along the canonical directed edge;
- signed crossing multiplicity; and
- residual evidence against the source edge and facet support.

A tangent event must have zero net crossing multiplicity unless the frozen symbolic policy intentionally converts an exact tie into a directed conceptual crossing. Such a symbolic contribution must remain separately marked from the operation-neutral numeric multiplicity.

The relation must account for the complete source facet, not merely the one source triangle that admitted the candidate. Multiple triangle-local hits representing one source-feature event must collapse through lineage, while truly separate crossings of a concave facet must remain separate event seeds.

### 2.7 Source facet/source facet support relations

Where edge/facet relations indicate a common face pair, the component must provide one canonical source-facet/source-facet support relation sufficient for Component 08 to organize carriers and coincident regions.

It must distinguish:

- definitely non-coplanar supports with a stable transverse carrier;
- parallel separated supports;
- exact coplanarity;
- bounded uncertainty that cannot establish either separation or coplanarity;
- same or opposite oriented support directions; and
- invalid or unrepresentable carrier construction.

For a stable non-coplanar pair, the relation must provide or reference a bounded face-face carrier representation, a canonical carrier orientation derived from source identities and oriented support, conditioning evidence, and residual checks.

For coplanar facets, the relation must not invent a transverse line. It must preserve a coplanar support identity and route overlap boundaries through original source edges and canonical coplanar region relations.

Approximate normal similarity or plane coefficient proximity is insufficient to declare coplanarity. The decision must be a bounded relation under the qualified arithmetic model or an exact tie category supported by the source provenance.

### 2.8 Signed crossing multiplicity

Every event that can change opposite-operand winding must carry one authoritative signed integer crossing contribution.

The sign convention must be frozen and documented. The default conceptual convention is:

- traverse the canonical directed source edge from parameter zero to one;
- interpret the opposite source facet using its validated occupied and unoccupied sides;
- `+1` means the opposite-operand winding after the event is one greater than before the event;
- `-1` means it is one smaller; and
- `0` means no net crossing.

Endpoint, edge, and vertex ties must use the frozen half-open symbolic ownership rules so a geometric crossing shared by several incident facets contributes exactly the intended total multiplicity. The sum must not depend on which triangle of a source facet discovered the event or on the triangulation of either facet.

The artifact must distinguish:

- numeric transverse multiplicity;
- numeric zero-multiplicity contact;
- symbolic multiplicity used only to define an infinitesimal tie order; and
- invalid or inconsistent multiplicity.

Component 09 must be able to reconstruct the winding delta associated with every classification adjacency without re-running relation predicates.

### 2.9 Tangency and zero-measure contact

Tangency, point contact, and edge contact must be represented explicitly even when their net crossing multiplicity is zero.

The relation record must state:

- contact dimension;
- owning source features on both operands;
- local orientation or fan evidence;
- whether the contact separates source-surface classification regions;
- whether the symbolic policy introduces distinct conceptual sides;
- whether the contact can contribute a retained coincident surface; and
- whether topology must remain separated despite coordinate coincidence.

A zero crossing must not be discarded merely because it does not change winding. Components 08-10 require zero-measure contact information to preserve regularized semantics and avoid accidental welding.

### 2.10 Coplanar and coincident relations

Coplanar geometry must be handled through explicit source-feature relations rather than tolerance-based snapping or random ownership.

The component must represent:

- coplanar but disjoint facets;
- point or edge contact on a common support;
- partial area overlap;
- one facet region contained in another;
- equal facet regions;
- overlapping source edges with equal or opposite direction;
- same-orientation coincident surfaces;
- opposite-orientation coincident surfaces; and
- several topologically distinct coincident sheets.

Operation-neutral records must preserve geometric overlap, orientation, boundary-feature lineage, and multiplicity. Operation-specific symbolic decisions must then provide:

- conceptual relative ordering of the sheets;
- ownership of any retained coincident boundary;
- cancellation or suppression of internal coincident surfaces;
- half-open rules at overlap boundaries; and
- occurrence-separation requirements for coordinate-coincident but topologically distinct sheets.

The symbolic decision changes classification, counting, and ownership. It must not move, average, snap, or perturb the stored nominal coordinates.

### 2.11 Symbolic perturbation eligibility

Symbolic perturbation is permitted only when the numerical relation is an exact or representational tie under the qualified model, or when an explicitly versioned coincident-source contract supplies equivalent exact lineage evidence.

It must not be used to hide:

- an enclosure too wide to determine a sign;
- a near-parallel construction whose uncertainty exceeds tolerance;
- inconsistent duplicate relation evaluations;
- contradictory source-facet coverage;
- an unrepresentable finite bound; or
- resource exhaustion.

The relation record must state why symbolic policy was eligible. Eligible reasons must be stable codes such as exact zero residual, exact equal endpoint identity, exact collinear support, exact coincident plane representation, or versioned representational tie.

An ineligible unresolved relation must return `geometric_condition_exceeds_tolerance` or another precise typed failure rather than an arbitrary symbolic answer.

### 2.12 Symbolic decision production

For every eligible tie, the component must create one immutable symbolic decision record with:

- a canonical symbolic-decision ID and key;
- the relation ID it resolves;
- operation and operand roles;
- policy version and rule identifier;
- complete source-feature tie key;
- conceptual perturbed ordering or side assignment;
- contact ownership;
- symbolic crossing contribution if any;
- occurrence-separation requirements;
- operand-swap remapping; and
- deterministic explanatory diagnostics.

A symbolic decision must be total for the relation category. It must not consult traversal order, worker number, hash order, memory address, or an unversioned implementation detail.

The same relation under a different Boolean operation may legitimately receive a different symbolic ownership decision. The operation-neutral numeric relation must remain reusable and unchanged.

### 2.13 Bounded construction records

Whenever a relation requires a nominal point, parameter, direction, or carrier, the component must obtain it through Component 03 construction services and publish the complete bounded result.

A point construction must include:

- nominal coordinate in `T`;
- axis and radial enclosure or equivalent conservative representation;
- source feature lineage;
- parameter bounds on every defining carrier;
- residual against every defining source edge, facet support, or carrier;
- conditioning classification;
- finite and tolerance disposition;
- precision-ledger reference; and
- one authoritative construction producer relation.

Endpoint events should reference the accepted source point and its precision lineage rather than recompute the same coordinate by interpolation.

Two consumers of one canonical construction must share the same construction record. Equivalent formulas may be evaluated for independent verification, but they must not create a second authoritative nominal coordinate.

### 2.14 Event-seed production

The component must emit canonical event seeds for Component 08. An event seed is a lineage and incidence description, not yet an event ID.

Each seed must identify:

- the authoritative relation and construction record;
- source feature owners on both operands;
- whether the point is an existing source vertex or a constructed point;
- every source edge, facet, triangle, and halfedge use that consumes the event;
- contact dimension and relation class;
- crossing multiplicity and symbolic contribution;
- candidate and triangle-local discovery provenance;
- canonical event-equivalence key supplied by relation lineage;
- a distinct-occurrence key when equal coordinates must remain separate; and
- expected carrier memberships and bounded parameters.

The event-equivalence key must not be derived from coordinate equality, spatial buckets, or overlapping uncertainty envelopes. Only exact source identity and canonical relation lineage may authorize interning.

### 2.15 Candidate coverage and duplicate discovery

Every candidate from Component 06 must end in exactly one documented disposition:

- definitely separated;
- absorbed as duplicate discovery of a canonical source-feature relation;
- contributed a primitive dependency;
- contributed one or more event seeds;
- contributed a coplanar or coincident relation;
- retained as a zero-measure contact; or
- caused a typed failure.

No candidate may disappear without a stable disposition code. Duplicate triangle-level discovery must be recorded sufficiently for exhaustive verification of source-facet coverage.

The component must verify that all triangle-local relations belonging to one composite source-facet relation are mutually consistent. Missing coverage, contradictory signs, incompatible constructions, or unexplained duplicate seeds prevent publication.

### 2.16 Deterministic evaluation and parallel merge

Parallel relation evaluation may use task-local caches and buffers. Publication must:

- canonicalize primitive relation keys before assigning IDs;
- evaluate each authoritative key once;
- record duplicate requests as consumers rather than additional producers;
- merge task outputs by complete canonical key;
- select one deterministic bounded formulation where alternatives exist;
- detect conflicting duplicate results;
- assign relation, construction, symbolic-decision, and seed ordinals canonically;
- choose the same primary failure under every schedule; and
- commit only after complete verification.

A task may speculatively request a relation already being evaluated elsewhere, but the public result must have one producer and byte-identical content independent of which task finished first.

### 2.17 Resource limits, cancellation, and transactionality

The component must account separately for:

- candidate requests;
- primitive and composite relation records;
- bounded predicate operations;
- alternate bounded formulations;
- construction records;
- dependency edges and consumer lists;
- symbolic decisions;
- event seeds and overlap intervals;
- task-local caches and merge storage;
- verification work; and
- persistent artifact bytes.

Limit exhaustion must produce a deterministic `resource_limit` or `index_overflow` without dropping relations, simplifying the symbolic matrix, or truncating event seeds.

Cancellation must be polled at deterministic safe points during key generation, primitive evaluation, composite assembly, bounded construction, symbolic resolution, merge, and verification. All workers must join and all reservations must roll back before returning `cancelled`.

No partial relation artifact may become visible to Component 08.

### 2.18 Independent verification evidence

The component must publish enough evidence for an independent verifier to check:

- candidate-to-disposition completeness;
- uniqueness and total ordering of canonical relation keys;
- acyclicity and completeness of the dependency graph;
- primitive bounded predicate categories and enclosures;
- source-facet coverage independent of internal triangulation diagonals;
- construction residuals and conditioning classifications;
- event-seed lineage and occurrence keys;
- crossing multiplicities and local conservation at shared vertices and edges;
- symbolic eligibility and exact policy-rule lookup;
- operand-swap remapping;
- coplanar and coincident ownership facts;
- deterministic digest inputs; and
- absence of coordinate-based merging.

For bounded fixtures, the verifier must compare the complete relation set against an independently controlled in-tree exact rational oracle and exhaustive source-feature enumeration. Production-scale verification may be structural and sampled where exhaustive arithmetic is infeasible, but release qualification requires exhaustive bounded domains.

The verifier must not call the producer's relation-cache lookup, dependency scheduler, symbolic dispatcher, or event-seed deduplication helper as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `signed_feature_relations<T>` artifact containing or referencing:

- artifact, arithmetic, dependency-graph, symbolic-policy, and serialization versions;
- canonical primitive and composite relation tables;
- canonical relation IDs, keys, and total order;
- a complete acyclic dependency graph and reverse-consumer mappings;
- candidate-to-disposition and candidate-to-relation mappings;
- bounded support-side, region, edge-edge, edge-facet, and facet-facet relation records;
- operation-neutral contact, coplanarity, coincidence, tangency, and crossing classifications;
- bounded construction records with precision-ledger references;
- authoritative signed crossing multiplicities;
- symbolic-decision records for every eligible exact tie;
- canonical event seeds and distinct-occurrence keys for Component 08;
- source-feature, triangle-local, source-facet, shell, and operand provenance;
- deterministic work partitions permitted for Component 08 consumption;
- resource and structural statistics;
- independent-verification evidence;
- canonical input and output digests; and
- replay metadata sufficient to reproduce every numerical and symbolic disposition.

The artifact must guarantee:

- every Component 06 candidate has one documented disposition;
- every canonical topology-affecting geometric question has one producer;
- every dependent consumer references the same immutable relation and construction records;
- exact ties are distinguishable from unresolved numerical uncertainty;
- unresolved uncertainty is never silently resolved by symbolic policy;
- every non-zero crossing has a frozen orientation and signed multiplicity;
- zero-measure contacts remain represented even when multiplicity is zero;
- source-facet semantics are independent of facet-internal triangulation diagonals;
- all event seeds are keyed by lineage rather than coordinate equality;
- distinct conceptual events may have identical nominal coordinates and overlapping bounds;
- coincident-surface ownership is operation-aware and versioned;
- symbolic decisions affect classification and ownership, not stored nominal geometry;
- operand exchange and operation remapping produce the documented remapped relation set;
- relation IDs, ordering, diagnostics, and digest are independent of worker schedule; and
- Component 08 can intern events without repeating any authoritative geometric computation.

A valid empty candidate stream must produce a valid empty relation artifact with canonical versions, zero counts, and a deterministic digest.

On failure, no relation artifact is published. The typed error must identify the canonical relation or candidate witnesses, operand roles, source features, numerical enclosures, conditioning and tolerance evidence, symbolic eligibility where relevant, policy versions, resource counters, and deterministic replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- topology-affecting arithmetic is bounded and uncertainty-aware;
- one canonical relation key has one authoritative producer and value;
- relation dependencies are acyclic and complete;
- source-feature lineage survives triangle-level discovery;
- internal diagonals never own original source-feature semantics;
- a definite sign is backed by a conservative margin;
- an exact tie and an unresolved uncertainty are separate states;
- symbolic policy is total, versioned, operation-aware, and applied only when eligible;
- symbolic perturbation never changes nominal coordinates;
- each non-zero crossing has signed integer multiplicity;
- duplicate triangle discoveries do not duplicate source-feature events;
- distinct conceptual events are never merged by coordinate coincidence;
- all constructions carry finite conservative bounds and residual evidence;
- all artifacts are immutable, context-owned, transactional, deterministic, and independently verifiable; and
- resource or conditioning limits cause typed failure rather than guessed classification.

Prohibited behavior:

- raw `==`, `<`, or `>` on topology-affecting floating values without the bounded predicate contract;
- treating every enclosure containing zero as an exact symbolic tie;
- using tolerance as a universal equality, coplanarity, or ownership rule;
- recomputing one relation through different formulas in different consumers;
- averaging or choosing among inconsistent duplicate constructions;
- assigning event equivalence from coordinate equality, distance, bounds overlap, hash, or spatial cell;
- allowing an internal triangulation diagonal to own a source contact;
- choosing symbolic ownership from traversal order or triangle order;
- random perturbation, stochastic rays, or unversioned lexicographic hacks;
- converting near-parallel uncertainty beyond tolerance into a fabricated point;
- discarding tangencies or point/edge contacts because their crossing multiplicity is zero;
- publishing partial relation tables after cancellation or resource exhaustion; or
- calling an external predicate, exact-arithmetic, mesh, collision, or geometry library.

## 5. Test and validation specification

### 5.1 Primitive predicate unit tests

Unit tests must cover every bounded sign category for:

- vertex/facet support side;
- projected edge side;
- orientation determinants;
- segment parameters;
- line and plane residuals;
- edge-edge support relations;
- facet/facet parallelism and carrier conditioning; and
- interval ordering.

Cases must include positive and negative zero, subnormals, adjacent representable values, cancellation, large translations, power-of-two scales, extreme finite exponents, and denominator enclosures that contain zero.

### 5.2 Relation matrix known-answer tests

Commit hand-auditable expected relation artifacts for:

- definite separation;
- one proper edge/facet crossing;
- edge endpoint on facet interior;
- edge crossing through a facet source edge;
- edge crossing through a facet source vertex;
- proper edge-edge crossing;
- endpoint/interior and endpoint/endpoint edge contact;
- tangent vertex/facet and edge/facet contact;
- coplanar disjoint edges and facets;
- collinear point contact, overlap, and equal edges;
- coplanar partial facet overlap;
- equal facets with same orientation;
- equal facets with opposite orientation;
- several incident triangles discovering one event;
- a concave facet with several edge crossings; and
- distinct events that round to one coordinate.

Expected artifacts must include keys, dependencies, categories, construction bounds, multiplicities, symbolic rule IDs, and event-seed lineage.

### 5.3 Symbolic policy matrix tests

For every operation and both operand roles, exhaustively test:

- equal operands;
- same- and opposite-orientation coincident facets;
- vertex-on-face;
- vertex-on-edge;
- vertex-on-vertex;
- edge-on-face;
- equal and partially overlapping edges;
- point-touching solids;
- edge-touching solids;
- face-touching solids; and
- half-open crossing ownership at shared vertices and edges.

The matrix must be total. Unknown relation categories or missing rule entries must fail deterministically during context qualification or relation evaluation.

Verify that symbolic decisions change ownership or conceptual side only and leave all nominal coordinate bits unchanged.

### 5.4 Exact-oracle differential tests

Normative tests must use the in-tree exact rational oracle from the broad plan for bounded integer-coordinate fixtures. Compare:

- exact support signs;
- exact edge-edge and edge-facet relation categories;
- exact event parameters;
- exact coplanarity and orientation;
- exact crossing multiplicities;
- exact source-facet region ownership; and
- containment of every exact construction in its published enclosure.

Production code must not depend on the oracle.

### 5.5 Dependency and compute-once tests

Instrument relation requests and verify:

- repeated requests for one vertex/facet side produce one authoritative record;
- multiple triangle candidates of one source facet share composite relations;
- legal re-triangulation changes bookkeeping requests but not source-feature relations;
- every dependency edge references an earlier canonical record;
- no duplicate producer exists;
- candidate iteration permutations preserve relation IDs and bytes; and
- an injected second evaluation with a changed nominal result is rejected.

### 5.6 Crossing conservation tests

Construct vertices and edges where several incident facets share one geometric crossing. Verify:

- half-open symbolic ownership counts the crossing exactly once where required;
- tangencies sum to zero;
- entering and leaving crossings have opposite sign;
- reversing the canonical edge direction negates multiplicity under the documented remap;
- reversing a shell orientation with corrected occupied-side semantics remaps signs consistently;
- sums around closed local fans are consistent; and
- alternative facet triangulations preserve total source-feature multiplicity.

### 5.7 Coplanar and coincidence tests

Include:

- disjoint coplanar polygons;
- point and edge contact;
- partial area overlap;
- one facet contained in another;
- equal facets;
- same and opposite orientation;
- overlapping concave facets with several disjoint overlap intervals;
- multiple coordinate-coincident sheets with distinct topology; and
- overlap boundaries passing through original vertices and edges.

Verify source-feature ownership, event-seed keys, symbolic owner selection, occurrence-separation flags, and invariance under triangle re-triangulation.

### 5.8 Conditioning boundary tests

For near-parallel edge-plane and facet-facet constructions, test cases:

- comfortably conditioned;
- just inside available tolerance;
- exactly at the threshold;
- just outside the threshold;
- denominator enclosure containing zero with true coplanarity;
- denominator enclosure containing zero without exact-tie evidence;
- large translation with small local geometry; and
- extreme finite scales.

The category transition, nominal construction, enclosure, and typed failure must be deterministic. Symbolic policy must not rescue ineligible uncertainty.

### 5.9 Metamorphic tests

Apply:

- operand exchange with operation remapping;
- source vertex, edge, facet, shell, and component permutations;
- facet-ring rotation;
- legal source subdivision and alternative triangulation;
- axis permutation;
- sign flip with corrected orientation;
- exactly representable translation;
- power-of-two scaling with precision scaling;
- canonical edge-direction reversal with documented sign remap;
- thread counts 1, 2, and maximum; and
- forced task delays and reversed merge order.

For a fixed policy and provider version, canonical relation keys, dependencies, nominal values, bounds, symbolic decisions, multiplicities, event seeds, diagnostics, and digest must be byte-identical after the documented remapping.

### 5.10 Mutation tests

Corrupt valid artifacts by:

- deleting a candidate disposition;
- duplicating a relation producer;
- introducing a dependency cycle;
- changing a source-facet relation to a triangle-local owner;
- shrinking a construction enclosure;
- changing one residual or conditioning category;
- coercing uncertainty to a definite sign;
- marking uncertainty as symbolically eligible;
- changing a symbolic rule ID or owner;
- flipping a crossing sign;
- changing multiplicity from zero to non-zero or vice versa;
- merging two event seeds by coordinate;
- splitting one canonical event seed across triangle candidates;
- changing an internal diagonal into a source edge;
- scrambling canonical ordering; and
- forging counts or digests.

Independent verification must reject every mutation.

### 5.11 Fuzzing and shrinking

Generate valid manifold operand pairs from exact templates, then vary:

- feature valence;
- facet concavity and triangulation;
- overlap dimension;
- shell count and nesting;
- coordinate duplication without identity merging;
- ULP perturbations;
- near-parallel angles;
- translation and scale;
- input precision and tolerance; and
- symbolic policy categories.

Every crash, nondeterministic result, oracle disagreement, invalid symbolic eligibility, conflicting duplicate relation, or incorrect multiplicity must serialize exact source bits, candidates, policy versions, and relation traces and shrink while preserving the failure.

### 5.12 Resource, cancellation, and concurrency tests

For predicate work, relation records, dependency edges, constructions, symbolic decisions, event seeds, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during key generation, primitive predicates, composite relation assembly, construction, symbolic resolution, canonical merge, and verification. Confirm all workers join, reservations return, and no partial relation artifact is visible.

Run delayed and reordered tasks under several thread counts. The same input must produce identical artifacts and the same primary failure.

### 5.13 Definition of done

Component 07 is complete only when:

- the canonical dependency graph and relation key domains are frozen and versioned;
- every Component 06 candidate has a verified disposition;
- exact ties and unresolved uncertainty are never conflated;
- all supported contact and coincidence categories have total symbolic rules;
- every authoritative relation and construction is computed once;
- crossing multiplicities agree with exact bounded oracles and local conservation tests;
- legal re-triangulation preserves source-feature relations and event seeds;
- operand-remapped metamorphic tests pass for every operation;
- independent mutation verification is effective;
- deterministic replay is byte-stable across schedules; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
