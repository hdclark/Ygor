# Component 08: Intersection Event Registry

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete registry layout, key interning strategy, carrier representation, interval-ordering provider, incidence compression, and parallel merge implementation may change. The source-lineage equivalence, one-construction-per-event, distinct-occurrence preservation, carrier ordering, deterministic publication, verification, and failure contracts in this document are normative.

## 0. Purpose

This component converts the canonical relation and event-seed records from Component 07 into one immutable shared intersection complex.

Its purposes are to:

- intern every conceptual intersection or contact event exactly once by canonical lineage;
- associate one authoritative nominal coordinate and one conservative precision envelope with each event identity;
- make every consuming source edge, source facet, source triangle, and oriented halfedge use reference that same event;
- preserve distinct topological or conceptual events even when their nominal coordinates and bounds are equal;
- order events deterministically along original source edges and newly identified face-face carriers;
- represent point, segment, interval, coplanar, coincident, tangent, and transverse incidences without inferring connectivity from proximity;
- expose cut and contact structure required by Component 09 classification and Component 10 selection; and
- provide independently verifiable lineage and ordering evidence for later output construction.

The registry is not a coordinate deduplicator. It must merge records only when Component 07 supplies the same canonical event-equivalence lineage or when an exact source-identity rule explicitly proves equivalence. Coordinate equality, overlapping uncertainty envelopes, equal carrier parameters, spatial hashing, or tolerance do not establish event identity.

The component does not recompute geometric relations, choose a different construction formula, classify global winding, apply the Boolean truth table, allocate final output vertex occurrences, pair output edges, or triangulate faces.

The principal output is an immutable `canonical_intersection_complex<T>` containing:

- canonical event records;
- relation-to-event and candidate-to-event mappings;
- source-feature incidence tables;
- ordered source-edge event sequences;
- canonical transverse and coplanar carrier records;
- ordered carrier event sequences and coincident clusters;
- contact and cut descriptors; and
- complete provenance, precision, resource, verification, digest, and replay metadata.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `signed_feature_relations<T>` artifact from Component 07;
- the immutable `canonical_source_manifolds` from Component 05;
- source-facet triangle groups and boundary provenance from Components 04 and 05;
- validated shell and occupied-side semantics from Component 02;
- the immutable precision context, bounded parameter-ordering services, construction-ledger records, and residual services from Component 03;
- the immutable Boolean context, identity domains, deterministic comparators, resource, cancellation, diagnostics, replay, and transaction services from Component 01;
- the selected event-key, carrier, ordering, artifact, and serialization versions; and
- verification settings controlling scalable and exhaustive registry checks.

The component must consume Component 07 constructions by immutable reference. It must not independently interpolate a second coordinate for the same event seed.

### 1.2 Required predecessor guarantees

The component may rely on Component 07 having established:

- every Component 06 candidate has one disposition;
- every topology-affecting relation has one canonical producer and stable relation ID;
- every event seed identifies one authoritative relation and bounded construction or accepted source point;
- every event seed includes canonical source-feature owners on both operands;
- every event seed includes a canonical event-equivalence key and, where needed, a distinct-occurrence key;
- triangle-local discoveries map to source-facet and source-feature lineage;
- internal triangulation diagonals are identified as bookkeeping and cannot own source contacts;
- every crossing contribution has signed multiplicity and symbolic metadata;
- every bounded construction is finite, conditioned, and within the applicable tolerance contract;
- coplanar and coincident relations include complete ownership and interval descriptors; and
- relation and seed tables have deterministic canonical order.

The component must defensively verify owner tokens, versions, key well-formedness, construction references, relation dispositions, source-feature ranges, and artifact digests. Contradictory committed relation records are an `internal_invariant_error`, not an excuse to choose one record by insertion order.

### 1.3 Event model

An event is a canonical conceptual occurrence at which the relation complex changes or must be referenced consistently. Supported event classes must include, at minimum:

- an existing source vertex lying on or crossing the opposite operand;
- a constructed source-edge/source-facet intersection point;
- a source-edge/source-edge point intersection;
- an endpoint of a coplanar overlap interval;
- a tangent contact point;
- a point at which several source features meet the same conceptual event;
- a symbolic tie event with no coordinate displacement;
- a coincident cluster member that must remain a distinct occurrence; and
- any future versioned event class required by the relation kernel.

An overlap interval or coincident region is not itself forced to be a single point event. It may reference endpoint events, interval records, source-edge carriers, coplanar support records, and region-incidence records.

### 1.4 Event equivalence preconditions

Two seeds may be interned to one event only when one of the following holds:

- their complete canonical event-equivalence keys are equal by full-key comparison;
- one seed is an explicit duplicate consumer of the same Component 07 relation and construction;
- they reference the same accepted source vertex identity under a documented event class and occurrence rule; or
- a versioned exact lineage rule proves that several triangle-local or source-facet-local discoveries represent the same source-feature event.

The registry must not infer equivalence from:

- equal or nearby nominal coordinates;
- intersecting or nested uncertainty envelopes;
- equal projected or edge parameters;
- the same spatial cell, hash, Morton code, or sort key;
- common incident triangle coordinates without common source lineage;
- a tolerance-based weld; or
- a later expectation that merging would simplify output.

### 1.5 Distinct-occurrence preconditions

Seeds with different distinct-occurrence keys must remain distinct events or distinct event occurrences even when they share:

- the same nominal coordinate bits;
- the same conservative point enclosure;
- the same source vertex coordinate;
- the same carrier parameter;
- the same contact dimension; or
- the same pair of geometric supports.

The event model must support coincident clusters whose members have a deterministic order and separate topology identities.

### 1.6 Capacity and lifetime preconditions

Before interning begins, the component must validate with overflow-safe arithmetic that it can represent and account for:

- all event seeds and seed-consumer incidences;
- all unique event records and coincident occurrence records;
- all source vertex, source edge, source facet, source triangle, halfedge, relation, and candidate reverse mappings;
- all source-edge event sequences and interval partitions;
- all face-face and coplanar carrier records;
- all carrier memberships, ordered sequences, equal-parameter clusters, and interval records;
- all cut/contact descriptors consumed by Components 09 and 10;
- all canonicalization, sorting, verification, diagnostics, and replay storage; and
- worst-case work up to configured event, incidence, carrier, and ordering limits.

Published records may reference only immutable predecessor artifacts and immutable stage-owned storage whose lifetime covers Components 09-15.

## 2. Required behavior

### 2.1 Canonical event-key normalization

The component must normalize every event seed into one complete canonical event key before assigning event IDs.

A key must encode enough information to distinguish:

- event class;
- operand roles;
- canonical source-feature owners on both operands;
- authoritative relation and construction lineage;
- source-vertex ownership when an existing endpoint is reused;
- coplanar or transverse carrier role;
- symbolic rule or contact class where it affects occurrence identity;
- distinct-occurrence discriminator; and
- event-key and artifact versions.

The key encoding must be total, collision-safe through full comparison, and independent of discovery order. Hashes may accelerate lookup but cannot determine equality.

Triangle IDs and internal-diagonal IDs may appear as discovery provenance, but public event identity must be reduced to authoritative source-feature lineage wherever Component 07 declares them bookkeeping-only.

### 2.2 Event interning

For each normalized key, the component must create exactly one canonical event record or one canonical occurrence record according to the event model.

Interning must:

- gather every seed and consumer that references the key;
- verify that all seeds agree on event class, source owners, construction producer, contact semantics, multiplicity, symbolic policy, and occurrence rules;
- choose no value by first-writer or majority vote;
- assign the event ID only after canonical key sorting;
- preserve all reverse-consumer mappings needed for verification and diagnostics; and
- reject inconsistent duplicate records before publication.

If several seeds use one canonical event-equivalence key but refer to different authoritative construction records, the component must require a Component 07 proof that one is the designated producer and the others are non-authoritative verification witnesses. Otherwise the relation artifact is inconsistent.

### 2.3 One authoritative coordinate and precision envelope

Every event identity that has a geometric point must reference exactly one authoritative bounded point record.

For a constructed event, the record must come from Component 07 and include:

- nominal coordinate;
- conservative axis/radial enclosure or equivalent;
- source feature lineage;
- construction parameters;
- residual evidence;
- conditioning classification;
- precision-ledger reference; and
- tolerance disposition.

For an event at an existing source vertex, the registry must reference the accepted source bounded point from Component 03/05 rather than recomputing the coordinate.

The registry must not average several constructions, snap a construction to a nearby endpoint, narrow a bound by intersecting inconsistent enclosures without proof, or pick the smallest uncertainty record opportunistically.

All consumers of one event must obtain the same nominal bits and bounded point reference.

### 2.4 Duplicate construction verification

When Component 07 provides secondary construction witnesses for one event, the component must verify at least:

- the authoritative nominal point lies inside every witness enclosure required by policy;
- each witness is compatible with the same source carriers and support residuals;
- parameter intervals overlap consistently where they describe the same conceptual location;
- endpoint/source-vertex ownership agrees; and
- the difference is explainable by bounded arithmetic rather than contradictory relation lineage.

A contradiction must produce `internal_invariant_error` or a precise geometric-condition failure attributed to the producing relation. The registry must not conceal disagreement by widening all bounds without a proven combination rule.

### 2.5 Event incidence

Every event record must publish total incidence to all relevant predecessor features. Incidence may include:

- source vertices represented by or touching the event;
- source edges containing the event;
- source edge intervals beginning or ending at the event;
- source facets whose region contains or bounds the event;
- source triangles and oriented halfedges that discovered or consume the event;
- source shells and operand roles;
- relation IDs and candidate IDs;
- crossing multiplicities and symbolic decisions;
- transverse or coplanar carriers; and
- contact, cut, tangent, overlap, and coincidence classifications.

Incidence must be based on Component 07 lineage and exact Component 05 topology. The registry must not attach a feature because its geometry happens to pass through the same nominal point unless the relation artifact authorizes that incidence.

### 2.6 Existing source vertices and endpoint reuse

When an event is exactly owned by an accepted source vertex, the registry must preserve both facts:

- the event is a Boolean relation occurrence with its own event identity; and
- its geometric point is the existing source vertex bounded point.

Several event identities may reference one source bounded point when distinct opposite-operand contacts or symbolic occurrences happen at the same source vertex. Conversely, several triangle-local seeds for one source-feature contact may intern to one event.

The registry must not allocate a second nominal point by interpolating parameter zero or one. It must not merge all events at one source vertex into one occurrence unless their event-equivalence and occurrence keys authorize it.

### 2.7 Source-edge membership and event sequences

For every source edge containing one or more events or overlap interval endpoints, the component must build one canonical ordered event sequence along the edge's canonical directed representative.

Each membership must include:

- event or occurrence ID;
- bounded parameter interval on the source edge;
- endpoint/interior/interval-end role;
- crossing and contact metadata;
- relation lineage;
- source-facet use incidence on both sides of the edge; and
- deterministic ordering evidence.

The sequence must include source endpoints when required by downstream partitioning, either as explicit endpoint events or immutable sentinel records tied to source vertex identities.

The component must distinguish:

- an isolated event;
- several definitely ordered events;
- a cluster of distinct events with the same exact parameter;
- overlapping parameter enclosures whose order is resolved by exact lineage or symbolic tie policy;
- an overlap interval with start and end events; and
- an unresolved ordering that threatens topology.

### 2.8 Bounded parameter ordering

Event ordering on a carrier is topology-affecting and must use Component 03 bounded parameter comparisons.

For two distinct memberships, the ordering result may be:

- definitely before;
- exact equal parameter;
- definitely after;
- unresolved overlap; or
- invalid.

A strict order may be published when the intervals are definitely separated. Exact equal parameters may be ordered by the frozen occurrence tie key while remaining marked as one coincident cluster.

An unresolved overlap may be resolved by lineage only when both orders represent the same topological partition and the contract explicitly permits a cluster. If choosing one order would change adjacency, interval length, crossing sequence, or cut topology, the component must fail with `geometric_condition_exceeds_tolerance` rather than use nominal parameter order.

Parameter sorting must not subtract nearly equal values without bounded comparison, normalize by an uncertain carrier length, or use pointer order as a tie breaker.

### 2.9 Source-edge interval partition

From each ordered source-edge sequence, the component must define a canonical partition into open intervals and endpoint/cluster locations.

Each interval record must identify:

- bounding event, cluster, or source endpoint records;
- source edge and canonical orientation;
- whether its geometric length is definitely positive, exact zero, uncertain, or an overlap interval;
- inherited source-facet uses;
- crossing delta accumulated at its boundaries;
- contact and coincidence states;
- whether Component 09 may propagate classification through the interval; and
- whether Components 10 and 11 may retain, split, duplicate, or suppress the interval.

The registry must not collapse a zero-nominal-length interval solely because endpoint coordinates are equal. Distinct topology may require separate occurrence records and later cleanup accounting.

### 2.10 Transverse face-face carriers

For each canonical non-coplanar source-facet pair that has event incidence, the component must create or reference one transverse carrier record derived from Component 07.

A carrier record must include:

- the canonical source-facet pair;
- bounded line or equivalent one-dimensional support;
- canonical orientation derived from source identities and oriented supports;
- conditioning and residual evidence;
- all incident event and overlap records;
- bounded parameters for each event on the carrier;
- relation and candidate provenance; and
- deterministic carrier ID and key.

Several disconnected intersection segments may lie on one carrier. The carrier sequence must preserve all event clusters and interval activation metadata needed to distinguish those segments.

The registry must not connect two events merely because they lie on the same infinite carrier. Connectivity requires compatible relation intervals and source-facet region incidence.

### 2.11 Coplanar carriers and overlap records

For coplanar source-facet relations, the component must not create an arbitrary transverse line. It must organize the intersection complex through:

- original source-edge carriers;
- canonical collinear-overlap carrier records;
- coplanar support identities;
- overlap boundary event sequences;
- bounded overlap intervals; and
- coplanar region-incidence records.

A coplanar carrier record must preserve:

- both source-edge identities and directions where applicable;
- same/opposite orientation;
- overlap parameter intervals on each source edge;
- endpoint event identities;
- symbolic ownership and half-open boundary rules;
- distinct occurrence requirements; and
- all source-facet and shell provenance.

Overlapping edges with equal coordinates but separate topology must remain separate source carriers linked by one relation record, not welded into one source edge.

### 2.12 Carrier event clusters

Events with exact equal carrier parameters or exact coordinate coincidence may form a canonical cluster. A cluster must:

- retain every member event or occurrence ID;
- retain the complete ordering tie keys;
- distinguish equivalence from coincidence;
- preserve all crossing and contact contributions;
- expose a deterministic local order for traversal where required;
- record whether members may share an output coordinate but require separate output occurrences; and
- prevent cancellation or deduplication by coordinate count alone.

Cluster order may be symbolic or topological, but it must be versioned and independently verifiable.

### 2.13 Crossing and contact aggregation

The registry must aggregate Component 07 contributions by event, source edge, and carrier without changing their authoritative values.

Aggregation must expose:

- numeric signed crossing multiplicity;
- symbolic crossing contribution;
- zero-net tangent and contact contributions;
- per-source-facet and per-shell provenance;
- local entering/leaving order along each directed carrier;
- ownership of coincident boundaries; and
- consistency checks for duplicate discoveries.

The component may precompute sums for downstream efficiency, but the verifier must be able to reconstruct them from immutable member records.

An aggregate sum does not erase the member contributions. This is necessary for local fan verification and mutation detection.

### 2.14 Cut and contact descriptors

The component must publish source-topology descriptors sufficient for Component 09 to determine which adjacency may propagate one classification.

For each relevant source edge, source edge interval, source vertex sector, source-facet adjacency, and carrier interval, the registry must indicate:

- no intersection influence;
- proper crossing cut;
- endpoint crossing cut;
- tangent contact without classification separation;
- contact delimiter required by symbolic policy;
- coplanar overlap boundary;
- coincident-sheet boundary or interior;
- bookkeeping-only internal-diagonal incidence; and
- unresolved or invalid.

These descriptors are not global winding labels. They are exact relation-derived constraints on connectivity.

The registry must preserve zero-measure contacts because point- and edge-touching regularized semantics depend on whether source-surface occurrences remain separate.

### 2.15 Source-facet and triangle incidence consistency

All triangle-local consumers belonging to one source facet must agree with the source-facet event and carrier records.

The component must verify:

- every event classified inside a source facet is covered by at least one member triangle under the accepted triangulation model;
- internal-diagonal duplicate discoveries collapse through source-facet lineage;
- an event on an original facet edge or vertex retains that source feature's ownership;
- no event is lost at a triangle boundary;
- no internal diagonal splits a source-facet carrier or classification region by itself; and
- legal alternative triangulations produce equivalent source-feature event complexes.

Contradictory triangle-local incidence is an invariant failure and must not be resolved by coordinate-based majority vote.

### 2.16 Deterministic interning and parallel merge

Parallel processing may normalize seeds and build task-local incidence records. Publication must:

- sort complete event keys canonically;
- group equal full keys;
- verify every group before assigning IDs;
- assign event, occurrence, carrier, cluster, and interval IDs in canonical order;
- merge reverse mappings deterministically;
- order source-edge and carrier memberships through bounded comparisons and frozen tie keys;
- select the same primary failure under all schedules; and
- commit only after complete verification.

No task may mutate a published event record. `std::unordered_*` may be used only for temporary lookup followed by deterministic full-key sorting.

### 2.17 Resource limits and pathological incidence

The component must account separately for:

- input seeds;
- unique events and distinct occurrences;
- source-feature incidence entries;
- reverse relation and candidate mappings;
- source-edge and carrier memberships;
- interval and cluster records;
- sorting and bounded-ordering work;
- verification and diagnostics; and
- persistent artifact bytes.

Pathological cases may contain very high event valence, many exact-coordinate occurrences, or many events on one edge or carrier. The component is not required to hide genuine output complexity. It must use output-sensitive accounting and fail deterministically with `resource_limit` rather than truncate incidence, merge by proximity, or drop cluster members.

### 2.18 Cancellation and transactionality

Interning, incidence construction, carrier grouping, bounded ordering, interval partitioning, and verification must occur in one stage transaction or private subtransactions that publish one final immutable artifact.

Cancellation must be polled at deterministic safe points during seed normalization, group verification, event allocation, reverse-map construction, edge/carrier ordering, cluster formation, interval partitioning, and verification.

On cancellation, all workers must join, reservations must be released, and every unpublished record must be discarded. The result is `cancelled`, never a partial intersection complex.

### 2.19 Independent verification evidence

The component must publish enough evidence for an independent verifier to reconstruct and check:

- normalization of every event seed;
- event-key uniqueness and canonical order;
- seed-to-event and relation-to-event completeness;
- agreement of all seeds interned to one event;
- one authoritative bounded point per event;
- preservation of distinct-occurrence keys;
- source-feature incidence and reverse mappings;
- bounded parameter order on every source edge and carrier;
- equal-parameter clusters and tie-key order;
- source-edge interval partitions;
- transverse carrier conditioning and residuals;
- coplanar overlap intervals and ownership;
- crossing/contact aggregates from member records;
- cut/contact descriptor derivation;
- triangle-to-source-facet consistency;
- deterministic digest inputs; and
- absence of coordinate-based merging.

For bounded fixtures, the verifier must compare the complete event and carrier complex with an independently implemented exact rational oracle and exhaustive relation-lineage grouping. It must not call the producer's interning table, ordering helper, cluster builder, or interval partitioner as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `canonical_intersection_complex<T>` artifact containing or referencing:

- artifact, event-key, carrier, ordering, symbolic-policy, and serialization versions;
- canonical event and distinct-occurrence records;
- canonical event IDs, keys, and total order;
- one authoritative bounded point and precision-ledger reference per geometric event;
- complete seed-to-event, relation-to-event, candidate-to-event, and event-to-consumer mappings;
- source vertex, edge, facet, triangle, halfedge, shell, and operand incidence;
- ordered event sequences on every affected source edge;
- source-edge interval partitions;
- canonical transverse face-face carriers and ordered memberships;
- canonical coplanar support, collinear-overlap, and overlap-region records;
- equal-parameter and coordinate-coincident clusters;
- crossing, contact, tangency, coincidence, and symbolic ownership aggregates;
- cut and contact descriptors for Components 09 and 10;
- deterministic partitions permitted for parallel downstream consumption;
- resource and structural statistics;
- independent-verification evidence;
- canonical input and output digests; and
- replay metadata sufficient to reproduce event grouping and ordering.

The artifact must guarantee:

- every Component 07 event seed maps to exactly one event or occurrence record;
- one event-equivalence key has one canonical event identity;
- different distinct-occurrence keys remain distinct;
- one event has one authoritative nominal coordinate and conservative bound;
- all consumers of one event reference that same bounded point record;
- no event identity was inferred from coordinate equality, proximity, bounds overlap, hash, or tolerance;
- every affected source edge has a complete deterministic event sequence and interval partition;
- carrier ordering is bounded, deterministic, and fails when topology-relevant ambiguity remains;
- events on one infinite carrier are not connected without relation-interval evidence;
- coplanar overlap uses original source-feature lineage and does not invent a transverse carrier;
- internal triangulation diagonals cannot own events or split source-facet semantics;
- crossing and contact aggregates remain reconstructible from member records;
- point- and edge-touching occurrence-separation requirements are preserved;
- legal source-facet re-triangulation preserves the source-feature event complex; and
- Components 09 and 10 can consume the artifact without recomputing coordinates, relations, or event order.

A valid empty relation artifact must produce a valid empty intersection complex with canonical metadata, zero counts, and a deterministic digest.

On failure, no event registry or intersection complex is published. The typed error must identify the event seed, relation, source features, carrier, parameter bounds, ordering ambiguity, construction inconsistency, resource counters, policy versions, and deterministic replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- event identity is established only by canonical lineage and explicit occurrence rules;
- one event has one authoritative bounded point;
- all event consumers share that point by identity;
- distinct conceptual or topological events may occupy the same coordinate;
- all source-feature incidence is total and exact;
- every event seed is consumed exactly once;
- source-edge and carrier sequences are complete and canonically ordered;
- topology-affecting ordering uses bounded parameter comparisons;
- exact equal parameters form explicit clusters rather than accidental merges;
- unresolved topology-relevant ordering causes typed failure;
- internal diagonals remain bookkeeping-only;
- carrier connectivity follows relation intervals, not common geometric support alone;
- all aggregate multiplicities are reconstructible from immutable members;
- all artifacts are immutable, context-owned, transactional, deterministic, and independently verifiable; and
- resource limits cause failure rather than incidence truncation.

Prohibited behavior:

- coordinate-based vertex or event deduplication;
- tolerance-based welding of events or source features;
- averaging inconsistent constructions;
- interpolating a second coordinate for an endpoint event;
- merging events because parameter intervals overlap;
- sorting topology-affecting parameters by nominal value alone when bounds overlap;
- connecting all events that share an infinite line or coplanar support;
- allowing an internal diagonal to own a cross-operand event;
- erasing member multiplicities after aggregation;
- dropping zero-multiplicity tangencies or contacts;
- using hash iteration, pointer address, worker completion order, or random tie breaking;
- publishing partial sequences or carriers after cancellation or resource exhaustion; or
- calling an external geometry, graph, spatial, exact-arithmetic, or mesh library.

## 5. Test and validation specification

### 5.1 Event-key and interning unit tests

Unit tests must cover:

- one seed producing one event;
- several duplicate consumers of one relation;
- several triangle-local seeds mapping to one source-feature event;
- one existing source vertex referenced by several distinct events;
- equal coordinates with different event-equivalence keys;
- equal event-equivalence keys with different occurrence keys;
- hash-collision injection;
- canonical key ordering; and
- wrong-context, wrong-operand, stale, and malformed keys.

Verify that only full lineage keys establish equivalence.

### 5.2 Authoritative coordinate tests

Include:

- stable interior edge/facet constructions;
- endpoint events using source bounded points;
- edge-edge constructions;
- coplanar overlap endpoints;
- several witnesses for one event;
- distinct events with bit-identical nominal coordinates; and
- large-translation and extreme-scale cases.

Mutate one witness enclosure, residual, producer relation, or nominal value and require deterministic rejection.

### 5.3 Source-edge sequence known-answer tests

Commit exact expected source-edge sequences and interval partitions for:

- no events;
- one interior event;
- endpoint plus interior events;
- several definitely ordered events;
- exact equal-parameter event clusters;
- distinct events with equal nominal coordinates;
- one overlap interval;
- several disjoint overlap intervals on a long edge;
- a concave opposite facet producing several crossings; and
- an event sequence whose nominal order differs from bounded definite order.

Expected records must include parameter bounds, tie keys, clusters, interval descriptors, crossing deltas, and incidence.

### 5.4 Ordering ambiguity tests

Construct parameter pairs that are:

- definitely ordered with a wide margin;
- separated by one representable value;
- exact equal;
- overlapping but safely clusterable by lineage;
- overlapping with order affecting adjacency;
- just inside the conditioning/tolerance threshold;
- exactly at the threshold; and
- just beyond the threshold.

The component must publish the documented order or fail precisely. It must never use nominal order as an undocumented fallback.

### 5.5 Transverse carrier tests

Include:

- one face-face intersection segment;
- several disjoint segments on one carrier from concave facets;
- several event clusters on one carrier;
- near-parallel but accepted carriers;
- conditioning failure;
- same carrier support generated by different triangle candidates;
- event points outside one source-facet region that must not connect; and
- operand exchange with carrier-orientation remapping.

Verify carrier keys, residuals, orientation, ordered memberships, active intervals, and source-facet incidence.

### 5.6 Coplanar overlap tests

Cover:

- collinear disjoint edges;
- point contact;
- partial overlap;
- equal edges with same direction;
- equal edges with opposite direction;
- one overlap interval nested in another;
- several overlap intervals from concave boundaries;
- equal and partially overlapping facets;
- multiple coincident sheets with distinct topology; and
- overlap boundaries crossing triangle internal diagonals.

Verify endpoint events, source-edge carriers, orientation, symbolic ownership, occurrence separation, and triangulation invariance.

### 5.7 High-valence and coincident-cluster tests

Include events where:

- several source edges from one vertex meet an opposite face;
- several opposite facets meet one source edge event;
- two shells touch at one point;
- several distinct events round to one coordinate;
- several events have exact equal carrier parameters;
- coincident sheets require separate occurrences; and
- a source vertex participates in both transverse and tangent relations.

Verify complete incidence, deterministic cluster ordering, multiplicity preservation, and no accidental merge.

### 5.8 Exact-oracle differential tests

For bounded integer-coordinate fixtures, use the in-tree exact rational oracle to compare:

- event equivalence from relation lineage;
- exact event parameters;
- exact carrier order;
- overlap interval endpoints;
- exact equal-parameter clusters;
- source-edge partitions; and
- containment of exact points in published bounds.

The production registry must not depend on the oracle.

### 5.9 Metamorphic tests

Apply:

- source vertex, edge, facet, triangle, shell, and component permutations;
- legal alternative source-facet triangulations;
- source-edge canonical direction reversal with sequence remapping;
- operand exchange and operation remapping;
- axis permutation;
- sign flip with corrected orientation;
- exactly representable translation;
- power-of-two scaling with precision scaling;
- thread counts 1, 2, and maximum; and
- forced task delays and reversed merge order.

Event equivalence, occurrence partition, canonical IDs, carrier structure, source-edge sequences, aggregates, diagnostics, and digest must remain byte-identical after the documented remapping.

### 5.10 Mutation tests

Corrupt valid artifacts by:

- merging two events by coordinate;
- splitting one event-equivalence key into two events;
- merging different occurrence keys;
- assigning one event two authoritative coordinates;
- changing a source vertex event into an interpolated point;
- omitting one seed consumer;
- attaching an unrelated source feature by proximity;
- changing a source-edge parameter;
- swapping two topology-relevant ordered events;
- deleting a cluster member;
- connecting disjoint intervals on one carrier;
- letting an internal diagonal own an event;
- changing one crossing contribution before aggregation;
- forging interval, count, or digest data; and
- permuting canonical arrays without updating maps.

Independent verification must reject every mutation.

### 5.11 Fuzzing and shrinking

Generate valid exact-template relations and vary:

- event count per edge;
- event valence;
- concavity and number of disjoint carrier intervals;
- coplanar overlap count;
- coordinate coincidence without identity equivalence;
- equal and nearly equal parameters;
- ULP perturbations;
- shell count and nesting;
- source triangulation;
- resource limits; and
- thread partitions.

Every crash, nondeterministic order, invalid merge, missing incidence, oracle disagreement, or unjustified geometric failure must serialize exact source bits, relation records, event keys, parameters, policies, and counters and shrink while preserving the failure.

### 5.12 Resource, cancellation, and concurrency tests

For seeds, events, occurrences, incidence entries, carrier memberships, clusters, intervals, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during seed normalization, group verification, coordinate attachment, incidence construction, source-edge sorting, carrier sorting, clustering, interval partitioning, and verification. Confirm all workers join, reservations return, and no partial artifact is visible.

Dense equal-coordinate and high-valence fixtures must fail deterministically with `resource_limit` when configured below true output requirements. Raising limits must reveal the complete complex, not a truncated or differently merged prefix.

### 5.13 Definition of done

Component 08 is complete only when:

- event and occurrence key domains are frozen and versioned;
- every Component 07 seed maps to exactly one verified event or occurrence;
- one event has one authoritative bounded point shared by all consumers;
- coordinate equality never establishes event identity;
- distinct equal-coordinate occurrences survive every path;
- every affected source edge and carrier has a complete bounded deterministic sequence;
- topology-relevant ordering ambiguity produces typed failure;
- coplanar and transverse carrier semantics are independently verified;
- legal re-triangulation preserves the source-feature intersection complex;
- mutation tests prove the verifier detects plausible corrupt registries;
- deterministic replay is byte-stable across schedules; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
