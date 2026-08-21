# Plan 08: Intersection Event Registry

## 0. Scope and non-negotiable constraints

Implement **only Component 08** from `component_08_intersection_event_registry.md`. This component consumes the immutable `signed_feature_relations<T,I>` artifact produced by Component 07 together with the verified source-facet, source-manifold, shell-semantics, precision, and Boolean-context artifacts, and publishes exactly one immutable `canonical_intersection_complex<T,I>` artifact for Components 09-15.

The V1 implementation is fixed by this plan as a deterministic **lineage-keyed conceptual-event registry with separate topology-occurrence records, canonical source-edge and carrier arrangements, and independent reconstruction verification**. The executable serial implementation is the semantic reference. Parallel work may produce only private normalization, incidence, and carrier fragments; canonical grouping, ID assignment, bounded ordering, failure arbitration, bytes, diagnostics, and digests must reproduce the serial reference exactly.

The implementation must:

- intern each canonical conceptual event exactly once from Component 07 lineage;
- represent topology-separate occurrences independently from the conceptual event and independently from coordinates;
- attach exactly one authoritative bounded point reference to each geometric conceptual event;
- make all consumers of one event reuse that same bounded point and construction lineage;
- preserve every Component 07 seed, relation, candidate, source-feature, triangle, and halfedge consumer through complete forward and reverse mappings;
- construct complete deterministic source-edge event sequences and interval partitions using Component 03 bounded parameter ordering;
- construct canonical transverse face-face carriers without connecting disjoint relation intervals that merely share an infinite support;
- construct coplanar and collinear overlap carriers from original source-feature lineage without inventing transverse supports or welding source edges;
- preserve exact-equal-parameter and coordinate-coincident clusters as ordered collections of distinct occurrences;
- aggregate crossing, tangent, contact, overlap, coincidence, and symbolic ownership evidence without erasing the contributing records;
- publish exact cut/contact descriptors sufficient for Component 09 to build classification atoms and cut-aware adjacency without repeating relation geometry;
- preserve source-facet semantics across internal triangulation diagonals and legal alternative triangulations;
- fail closed before publication whenever equivalence, authoritative construction, incidence, bounded order, carrier activation, overlap ownership, resources, cancellation, or independent verification remains unresolved; and
- remain strict portable C++17 with no external, vendored, downloaded, optional, or runtime-invoked dependency.

The component must not:

- call, adapt, copy from, or depend on `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- reread mutable caller meshes or reconstruct source topology from coordinates;
- call legacy `vec2`/`vec3`, `line`, `line_segment`, `plane`, contour, mesh-intersection, or adaptive-predicate APIs for authoritative identity, order, incidence, or connectivity decisions;
- recompute a Component 07 point, edge parameter, overlap endpoint, face-face carrier, residual, crossing multiplicity, or symbolic decision through a second formula;
- infer event equivalence from nominal-coordinate equality, coordinate proximity, intersecting uncertainty bounds, equal nominal parameters, spatial bins, hashes, Morton codes, adjacency, or expected downstream simplification;
- infer topological occurrence equivalence from conceptual-event equivalence; every distinct-occurrence key must survive explicitly;
- sort topology-affecting values with ordinary floating `<`, subtraction, a normalized direction, or nominal values when bounded intervals overlap;
- use caller tolerance as an event-welding epsilon, source-edge deduplication epsilon, carrier-equivalence epsilon, or order tie breaker;
- allow a facet-internal triangulation diagonal to own a public event, source contact, overlap boundary, cut, carrier, or classification delimiter;
- connect every event on the same infinite line or coplanar support;
- collapse an interval because endpoint nominal coordinates or nominal parameters are equal;
- discard zero-net tangencies, point contacts, edge contacts, coincident occurrences, or zero-measure delimiters;
- choose among inconsistent duplicate seeds, constructions, incidences, or multiplicities by first writer, majority vote, smallest uncertainty, or widest combined bound;
- allocate final output vertices, pair output halfedges, classify global winding, apply the Boolean truth table, triangulate output polygons, perform cleanup, or publish a public mesh;
- truncate required seeds, incidences, memberships, clusters, intervals, or diagnostics to satisfy a resource limit;
- publish a partial event registry, partial source-edge sequence, partial carrier arrangement, or partial mapping after failure, cancellation, verifier rejection, or resource exhaustion;
- use exceptions for expected contract, geometry, resource, cancellation, codec, or verification failures;
- serialize raw structs, padding, pointers, `size_t`, implementation-defined enums, unordered iteration, or `std::hash` values; or
- make worker schedule, pointer address, hash bucket, allocation order, or input traversal order observable.

Use Component 01 for owner tokens, strong IDs, checked count/byte/index arithmetic, stage/checkpoint registration, typed outcomes and errors, resource reservations, cancellation, deterministic failure arbitration, diagnostics, replay, canonical bytes, SHA-256, transactions, immutable publication, and execution-policy validation. Use Component 02 as the sole authority for shell orientation, shell nesting, occupied-side semantics, and source boundary ownership. Use Component 03 as the sole authority for bounded points, parameters, intervals, interval comparisons, exact-nominal evidence, construction/residual records, conditioning, precision-ledger references, tolerance dispositions, and canonical floating-bit encoding. Use Component 04 as the sole authority for source-facet semantic rings, projection/support identity, triangle groups, boundary provenance, and internal-diagonal bookkeeping. Use Component 05 as the sole authority for canonical source vertices, edges, halfedges, triangles, facets, shells, edge directions, incidence, and immutable query views. Use Component 06 as the sole authority for canonical candidate identity and disposition coverage. Use Component 07 as the sole authority for relation identity, constructions, event seeds, event-equivalence keys, distinct-occurrence keys, crossing contributions, symbolic ownership, overlap intervals, carrier constructions, and relation/candidate/feature provenance.

No failed, cancelled, partially encoded, or verifier-rejected artifact may publish. Mark Component 08 complete in `tracker.md` only after every requirement in Section 27 is represented by an implementable instruction and qualification gate.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 `fv_surface_mesh` is a public boundary type, not an event-registry provider

`src/YgorMath.h` defines `fv_surface_mesh<T,I>` as a deliberately simple face-vertex container. Its `involved_faces` index is optional and may be stale unless explicitly maintained; the class imposes few manifold or semantic constraints; and `merge_duplicate_vertices(distance_eps)` intentionally merges geometry using distance tolerance. These properties are incompatible with Component 08 identity and occurrence semantics.

Therefore:

- do not store the registry in a temporary `fv_surface_mesh`;
- do not use `vertices`, `faces`, or `involved_faces` as authoritative event, source-feature, or reverse-incidence tables;
- do not call `merge_duplicate_vertices`, `convert_to_triangles`, `remove_degenerate_faces`, or any other mutating mesh helper during this stage;
- do not represent a conceptual event by adding a temporary public-mesh vertex index;
- do not use a public vertex index as an `event_id` or `event_occurrence_id`; and
- reserve `fv_surface_mesh` conversion for Component 14 after topology construction, triangulation, and cleanup.

The existing mesh type remains useful in tests for fixture import/export and public round-trip checks, but no Component 08 semantic decision may depend on its mutable convenience indexes.

### 1.2 Reuse existing vector types only as nominal payload carriers

`vec2<T>` and `vec3<T>` provide ordinary arithmetic, exact operator equality/order, normalization, lengths, distances, and angle operations. `line<T>`, `line_segment<T>`, and `plane<T>` store normalized geometric forms and expose bare-boolean intersection routines. These APIs do not carry inherited uncertainty, construction lineage, owner identity, conditioning, deterministic formula selection, typed failure, or replay evidence.

Therefore:

- permit `vec3<T>` only inside Component 03 bounded-point records as the immutable nominal coordinate payload already selected by Component 07;
- permit `vec2<T>` only for immutable nominal projected payloads already owned by predecessor artifacts;
- never call vector arithmetic operators, `Dot`, `Cross`, `unit`, `length`, `distance`, `angle`, vector `operator==`, or vector `operator<` for event identity, ordering, carrier grouping, or interval activation;
- never call `line`/`plane` intersection, projection, distance, or equality routines;
- never construct a normalized face-face line in Component 08; consume the bounded carrier construction from Component 07; and
- leave all existing public geometry behavior source-compatible.

### 1.3 Reuse adaptive arithmetic only through Component 03

`YgorMeshesAdaptivePredicates.h/.cc` and the `orient_sign`/segment helpers exposed through `YgorMath.h` are not direct Component 08 dependencies. Component 03 already owns the audited strict-target bounded and exact-nominal arithmetic services needed by this stage.

Component 08 must consume only Component 03 views and operations for:

- five-way bounded parameter comparison;
- finite interval validation and domain containment;
- exact-nominal equality evidence;
- conservative interval hull/intersection when explicitly authorized;
- positive/zero/uncertain interval-length classification;
- source-edge and carrier parameter remapping under orientation reversal;
- authoritative bounded-point, residual, and conditioning validation; and
- canonical `T` bit encoding.

Do not call legacy predicates directly and do not add a second arithmetic subsystem. A missing bounded comparison or remap capability must be added to Component 03's versioned capability registry and independently tested there.

### 1.4 Existing thread pool is not a deterministic stage executor

`src/YgorThreadPool.h` supplies a general FIFO `work_queue`, but it catches and discards task exceptions, has no typed failure arbitration, no stage transaction, no resource accounting, no deterministic cancellation checkpoints, and no canonical private-result merge contract. It must not be used directly as Component 08's semantic executor.

Use the Component 01/17 execution capabilities instead. If a later integration layer adapts `work_queue`, the adapter must still provide private task outputs, join-before-rollback, deterministic primary failure, strict floating-environment qualification, and canonical merge. The executable serial path remains mandatory and authoritative.

### 1.5 Legacy and generic mesh algorithms are not providers

The explicitly excluded Boolean files, generic mesh verification, BSP, hole filling, remeshing, convex hull, Delaunay, contour, and slicing code may be studied for fixture generation or non-authoritative benchmark comparison only. Coordinate-keyed maps, epsilon equality, mutable topology, random or centroid/ray behavior, and exception-based failures do not satisfy this component's contract.

Do not retrofit a legacy Boolean vertex cache or edge-intersection map. Implement Component 08 as new bounded-subsystem code.

### 1.6 Mandatory predecessor reuse

Reuse, rather than duplicate:

- Component 01 context owner, operation identity, strong-ID domains, checked storage arithmetic, resource kinds, stage transaction, cancellation, deterministic partitions/failure ordering, canonical codec, SHA-256, diagnostics, replay, and verification-level dispatch;
- Component 02 source-shell/facet orientation, occupied-side semantics, nesting identity, original source-boundary records, and caller provenance;
- Component 03 bounded point/scalar/parameter/interval views, five-way comparisons, exact-equality evidence, construction/residual/conditioning/precision-ledger records, endpoint classification, remap operations, and tolerance disposition;
- Component 04 source-facet semantic ring, source triangles, original-boundary/internal-diagonal labels, projection/support identity, semantic digest, and exact-triangulation digest;
- Component 05 canonical source vertices, edges, internal diagonals, directed halfedge uses, canonical edge representatives, triangles, source-facet groups, shell groups, vertex fans, and checked incidence views;
- Component 06 candidate IDs, canonical candidate order, and disposition identity; and
- Component 07 complete relation/construction/seed keys, event-equivalence keys, distinct-occurrence keys, authoritative producer references, source-feature incidence, crossing contributions, symbolic rule/ownership records, carrier and overlap constructions, candidate mappings, and deterministic partitions.

Do not create a second source-feature registry, source-edge direction policy, shell classifier, precision ledger, symbolic matrix, relation cache, construction table, candidate map, source-facet semantic model, or digest provider.

### 1.7 Permitted implementation machinery

Use strict portable C++17 standard-library facilities such as fixed-width integers, `std::array`, `std::vector`, `std::optional`, `std::variant`, `std::sort`, `std::lower_bound`, deterministic prefix sums, sorted-vector grouping, and explicit iterative graph traversals. Prefer contiguous immutable arrays with CSR-style ranges and complete-key sort/scan.

`std::unordered_*` is unnecessary for the serial reference and must not determine equality, canonical order, duplicate resolution, diagnostics, or bytes. If a private performance cache is added later, every hit must verify the complete key and publication must still canonicalize by full-key sort.

## 2. Fixed V1 semantic model and provider

### 2.1 Provider identity

Freeze the production provider as:

```text
canonical_lineage_event_arrangement_v1
```

Freeze the conceptual-event/occurrence policy as:

```text
shared_event_separate_occurrence_v1
```

Freeze the carrier-ordering policy as:

```text
bounded_interval_sweep_cluster_order_v1
```

Freeze the source-edge partition policy as:

```text
endpoint_cluster_open_interval_partition_v1
```

Freeze the transverse-carrier activation policy as:

```text
relation_interval_supported_connectivity_v1
```

Changing any event-key field, conceptual-event/occurrence split, authoritative-point ownership rule, incidence ownership rule, carrier key, bounded-ordering rule, cluster eligibility rule, interval activation rule, cut/contact descriptor rule, canonical order, or observable record layout requires an explicit provider/policy/schema version change.

### 2.2 Conceptual event versus topology occurrence

V1 must use two distinct immutable domains:

1. A **conceptual event** is identified solely by the normalized Component 07 event-equivalence lineage. It owns the one authoritative bounded point reference and common relation meaning.
2. An **event occurrence** is identified by `(event key, distinct-occurrence key)`. It is the identity consumed by source-edge sequences, carrier sequences, clusters, interval boundaries, cut/contact descriptors, and later topology construction.

Rules:

- one normalized event-equivalence key produces exactly one `event_id`;
- every distinct-occurrence key within that event produces exactly one `event_occurrence_id`;
- if a seed has no additional occurrence discriminator, normalize it to the explicit `single_occurrence` discriminator rather than an absent/nullable semantic;
- several seeds may map to the same event and occurrence when they are duplicate consumers of the same conceptual/topological occurrence;
- several occurrences may reference one event and therefore one authoritative bounded point;
- different events may reference bit-identical nominal coordinates and equal bounds while remaining unrelated;
- different occurrences of one event may be topologically separate even though all share the same point record;
- all downstream topology-facing records refer to `event_occurrence_id`; records concerned with shared geometry and conceptual relation may additionally refer to `event_id`; and
- neither ID is a public mesh vertex index.

This split is mandatory. Do not encode occurrence separation as a boolean on a merged event, and do not duplicate bounded points per occurrence.

### 2.3 Fixed serial workflow

The serial semantic reference must execute these phases in order:

1. validate context, strict floating profile, versions, owner tokens, predecessor artifact identities, digests, and verification dispositions;
2. preflight all counts, products, work, bytes, ID domains, and configured limits;
3. scan Component 07 event seeds in canonical seed order and normalize complete event and occurrence keys into private proposals;
4. sort proposals by complete event key then occurrence key, group exact keys, and verify duplicate compatibility;
5. assign conceptual-event IDs and occurrence IDs in canonical order;
6. attach exactly one authoritative bounded point reference and verify all secondary witnesses;
7. construct complete seed/relation/candidate/source-feature/triangle/halfedge forward and reverse incidence tables;
8. construct source-edge membership proposals and canonical per-edge sequences;
9. form exact-equal or explicitly clusterable occurrence clusters and source-edge interval partitions;
10. construct transverse carrier proposals, memberships, clusters, active spans, and ordered sequences;
11. construct coplanar support, collinear-overlap carrier, overlap interval, and coplanar-region records;
12. reconstruct crossing/contact aggregates from immutable member contributions;
13. derive source-edge, source-vertex-sector, source-facet-adjacency, and carrier-span cut/contact descriptors;
14. reconcile triangle-local incidence to source-facet semantics and prove internal-diagonal transparency;
15. canonicalize all remaining IDs, ranges, maps, statistics, and deterministic partitions;
16. run producer invariant checks;
17. canonical-encode sections, compute section and complete digests, and finalize replay evidence;
18. run the independent verifier, including exhaustive mode when requested;
19. reconcile resources and perform the final deterministic cancellation check; and
20. commit one immutable artifact transactionally.

No authoritative geometry is evaluated in this workflow. Every parameter, interval, point, carrier, residual, and crossing contribution is imported by immutable reference and only validated, ordered, grouped, or aggregated.

## 3. Exact file and target layout

Add under `src/YgorMeshesBooleanBounded/`:

- `IntersectionTypes.h` — stable Component 08 enums, policy constants, strong-ID aliases, compact status/result tags, resource/statistic records, and descriptor categories;
- `IntersectionKeys.h/.cc` — normalized event, occurrence, source-edge membership, carrier, cluster, interval, and descriptor keys; total comparators; operand/source-direction remaps; and canonical key encoding;
- `IntersectionPreflight.h/.cc` — predecessor validation, count/work/byte/index arithmetic, per-feature incidence bounds, active-set bounds, resource estimates, and capacity guards;
- `EventNormalization.h/.cc` — Component 07 seed normalization, complete-key validation, grouping proposals, and coordinate-derived-key rejection checks;
- `EventInterning.h/.cc` — conceptual-event and occurrence grouping, duplicate compatibility verification, canonical ID assignment, and seed mappings;
- `EventCoordinates.h/.cc` — authoritative bounded-point attachment, existing-source-point reuse, secondary witness compatibility, construction lineage validation, and precision references;
- `EventIncidence.h/.cc` — canonical incidence proposal collection, CSR forward/reverse maps, source-feature ownership validation, and member-preserving aggregation inputs;
- `BoundedCarrierOrdering.h/.cc` — Component 03 interval comparison adapter, deterministic interval sweep, overlap-active-set validation, cluster eligibility, proven precedence, canonical order certificates, and failure witnesses;
- `SourceEdgeArrangements.h/.cc` — source-edge memberships, endpoint sentinels, occurrence clusters, event sequences, open-interval partitions, boundary deltas, and source-edge descriptors;
- `TransverseCarrierArrangements.h/.cc` — face-face carrier grouping, orientation, memberships, active relation spans, clusters, ordered sequences, and no-false-connectivity checks;
- `CoplanarCarrierArrangements.h/.cc` — original source-edge carriers, collinear overlap pairs, parameter remapping, overlap endpoint occurrences, symbolic ownership, coplanar support/region incidence, and separate-sheet preservation;
- `IntersectionAggregation.h/.cc` — crossing, tangency, contact, coincidence, and symbolic ownership aggregates reconstructed from immutable Component 07 contributions;
- `IntersectionDescriptors.h/.cc` — source edge/interval, source vertex sector, source-facet adjacency, carrier-span, overlap, and occurrence-separation cut/contact descriptors for Components 09-11;
- `IntersectionCanonicalization.h/.cc` — final key ordering, ID remap, sorted ranges, deterministic partitions, statistics, resource reconciliation inputs, and immutable publication assembly;
- `CanonicalIntersectionComplex.h` — immutable Component 08 artifact schemas and owner-checked read-only views;
- `IntersectionQueries.h` — narrow downstream query interfaces for Components 09-11 and diagnostics;
- `IntersectionBuild.h/.cc` — typed stage entrypoint and exact phase orchestration;
- `IntersectionCodec.h/.cc` — canonical encoding/decoding, section digests, replay integration, and strict private decode;
- `IntersectionVerifier.h/.cc` — independent seed regrouping, incidence reconstruction, bounded-order verification, carrier/partition/descriptor reconstruction, exact-oracle hooks, and mutation rejection; and
- `IntersectionDebugDump.h/.cc` — optional deterministic human-readable rendering from completed records only; never used by semantic code.

Extend existing bounded-subsystem registries rather than creating parallel infrastructure:

- `ContractVersions.h` for Component 08 provider, conceptual-event/occurrence, key, schema, ordering, cluster, partition, carrier, aggregation, descriptor, codec, and verifier versions;
- Component 01 stage/checkpoint, strong-ID-domain, error-subcode, resource-kind, diagnostic, replay, and execution registries;
- Component 03 capability registry only if a bounded parameter remap, interval-length disposition, or ordering-certificate operation required by this plan is genuinely missing; and
- Component 07 downstream query schema only if a required immutable seed/construction/carrier/overlap field is not exposed, without changing Component 07 semantics.

Add under `tests/mesh_boolean_bounded/`:

- `TestIntersectionKeys.cc`;
- `TestEventInterning.cc`;
- `TestEventCoordinates.cc`;
- `TestEventIncidence.cc`;
- `TestBoundedCarrierOrdering.cc`;
- `TestSourceEdgeArrangements.cc`;
- `TestTransverseCarrierArrangements.cc`;
- `TestCoplanarCarrierArrangements.cc`;
- `TestIntersectionAggregation.cc`;
- `TestIntersectionDescriptors.cc`;
- `TestIntersectionKnownArtifacts.cc`;
- `TestIntersectionExactOracle.cc`;
- `TestIntersectionAlternativeTriangulation.cc`;
- `TestIntersectionCanonicalization.cc`;
- `TestIntersectionCodec.cc`;
- `TestIntersectionMutation.cc`;
- `TestIntersectionProperties.cc`;
- `TestIntersectionAdversarial.cc`;
- `TestIntersectionResourcesCancellation.cc`;
- `TestIntersectionStructuralPerformance.cc`;
- `IntersectionFixtures.h/.cc`;
- `IntersectionExactOracle.h/.cc`;
- `IntersectionMutationSupport.h/.cc`; and
- `GoldenIntersectionComplexV1.h`.

Register separate CTest cases for keys/interning, coordinates/witnesses, incidence, bounded ordering, source-edge arrangements, transverse carriers, coplanar arrangements, aggregation/descriptors, known artifacts, exact oracle, alternative triangulation, canonicalization/codec, mutation, properties/fuzz, adversarial floating point, resources/cancellation, and structural performance. Apply `ygor_apply_mesh_boolean_strict_fp` to every production and normative-test translation unit. No network discovery or optional test package is permitted.

Keep mutable proposals, active-set nodes, temporary maps, sort buffers, graph workspaces, verifier scratch, exact-oracle integers, and mutation helpers out of installed/public headers. Templates must remain header-defined or be explicitly instantiated only for supported `float`/`double` and `uint32_t`/`uint64_t` combinations.

## 4. Stable versions, stages, checkpoints, and failure subcodes

### 4.1 Version registry

Add explicit nonzero V1 constants for:

- intersection-registry provider and semantic policy;
- conceptual-event/occurrence split and default occurrence normalization;
- event-equivalence and occurrence-key normalization;
- event, occurrence, incidence, source-edge membership, source-edge sequence, source-edge interval, carrier, membership, cluster, overlap, aggregate, and descriptor schemas;
- source-edge canonical orientation and endpoint-sentinel policy;
- bounded interval sweep and cluster-eligibility policy;
- transverse carrier key/orientation/activation policy;
- coplanar support, collinear overlap, and region-incidence policy;
- crossing/contact aggregation and member-retention policy;
- internal-diagonal transparency and source-facet reconciliation policy;
- canonical ID/order and deterministic partition policy;
- canonical encoding and section-digest layout; and
- intersection verifier and exhaustive-oracle evidence schema.

Zero is invalid/uninitialized. Unknown required versions, unsupported closed-enum values, mismatched predecessor versions, nonzero reserved bits, or unrecognized policy IDs are typed failures. Encode applicable versions in the artifact header, records, canonical bytes, diagnostics, replay, and verifier evidence.

### 4.2 Fixed logical checkpoints

Use the Component 08 stage reserved by Component 01. Define stable checkpoints in this order:

1. context, owner, strict-environment, execution, and capability validation;
2. predecessor artifact/version/digest/verification validation;
3. seed/relation/candidate/source-feature count and representability preflight;
4. resource/work reservation for normalization, interning, incidence, ordering, carriers, verification, and persistent storage;
5. seed normalization and complete-key validation;
6. event-key sorting, exact grouping, and duplicate compatibility verification;
7. conceptual-event and occurrence ID assignment;
8. authoritative-point attachment and secondary-witness validation;
9. forward and reverse incidence proposal construction;
10. canonical incidence sort/group/range publication;
11. source-edge membership proposal generation;
12. source-edge bounded ordering and cluster formation;
13. source-edge interval partition and descriptor construction;
14. transverse carrier grouping, validation, ordering, clustering, and activation spans;
15. coplanar support/overlap carrier and region-incidence construction;
16. crossing/contact/coincidence/symbolic aggregate reconstruction;
17. cut/contact descriptor derivation;
18. source-triangle/source-facet reconciliation and internal-diagonal transparency verification;
19. final canonical ordering, ID/reference remap, partitions, and statistics;
20. producer invariant checks;
21. canonical encoding, section digests, and replay finalization;
22. independent artifact verification, including exhaustive bounded mode when requested;
23. resource reconciliation and pre-publication cancellation check; and
24. transaction commit.

Do not renumber released checkpoints. Future optional providers require reserved gaps or a new version.

### 4.3 Required Component 08 failure subcodes

Allocate a disjoint Component 08 range with explicit values for at least:

- unsupported registry provider/policy/schema/order/codec/verifier version;
- wrong/stale context owner, operation, operand, role, ID domain, or artifact handle;
- predecessor artifact, version, digest, verification disposition, semantic digest, or exact-triangulation digest mismatch;
- malformed event seed, event-equivalence key, distinct-occurrence key, relation reference, construction reference, carrier reference, overlap reference, or consumer range;
- event/occurrence key contains forbidden coordinate-, hash-, pointer-, traversal-, or worker-derived identity material;
- seed/relation/candidate/source entity/count/byte/index/work overflow;
- event, occurrence, incidence, membership, cluster, interval, carrier, overlap, descriptor, or evidence capacity exceeded;
- duplicate event-key group incompatible in class, owner, relation lineage, construction producer, contact semantics, multiplicity, symbolic policy, or occurrence rules;
- one normalized event key mapped to multiple authoritative constructions without a valid producer/witness proof;
- one occurrence key mapped to incompatible event semantics;
- distinct-occurrence key merged or identical occurrence key split;
- missing or multiple authoritative bounded point;
- source-vertex event incorrectly interpolated or wrong source bounded point referenced;
- secondary witness enclosure, parameter, residual, carrier, endpoint ownership, conditioning, or lineage incompatible;
- seed missing event/occurrence mapping or mapped more than once;
- relation/candidate/source-feature/triangle/halfedge incidence missing, duplicated incompatibly, out of range, or wrong owner;
- unrelated source feature attached without lineage authority;
- internal diagonal assigned public source ownership;
- source-edge membership missing parameter, role, source uses, relation lineage, or ordering evidence;
- parameter interval invalid, non-finite, out of source-edge/carrier domain, or wrong orientation remap;
- bounded-order comparison invalid or contradictory;
- exact-equal parameter lacks exact evidence;
- unresolved overlap incorrectly ordered by nominal value;
- unresolved topology-relevant order or non-transitive cluster eligibility;
- source-edge sequence incomplete, noncanonical, or inconsistent with endpoint sentinels;
- source-edge partition has gap, duplicate coverage, reversed bounds, unsupported uncertain span, or wrong boundary delta;
- transverse carrier key, orientation, support, residual, conditioning, source-facet pair, or relation provenance inconsistent;
- events on common infinite support connected without relation-interval evidence;
- transverse active span open, contradictory, multiply owned, or outside source-facet regions;
- coplanar relation routed through a transverse carrier;
- collinear overlap orientation, dual-edge parameter intervals, endpoint occurrences, symbolic ownership, or half-open rule inconsistent;
- separate source edges/sheets welded by geometric equality;
- event cluster missing member, duplicate member, wrong tie order, or equivalence/coincidence conflation;
- crossing/contact/tangent/coincidence aggregate differs from member reconstruction;
- member contribution erased after aggregation;
- cut/contact descriptor category, source topology, boundary delta, occurrence-separation flag, or provenance inconsistent;
- triangle-local incidence missing source-facet coverage, lost at triangle boundary, or dependent on legal retriangulation;
- canonical key collision, ordering, ID, range, partition, or reference remap error;
- codec tag/length/count/version/reserved/trailing-data error;
- section or complete digest mismatch;
- intersection verifier rejection;
- resource reservation/reconciliation failure;
- cancellation; and
- internal construction invariant failure.

Map unresolved topology-affecting bounded order or incompatible geometric witness evidence to `geometric_condition_exceeds_tolerance` when the predecessor evidence itself is geometrically admissible but insufficient for a safe arrangement. Map representability to `index_overflow`, configured accounting exhaustion to `resource_limit`, cancellation to `cancelled`, malformed committed predecessor contradictions and producer/verifier disagreement to `internal_invariant_error`. Every error must include the least canonical seed/event/occurrence/relation/source-feature/carrier witnesses, exact nominal bits and intervals, comparison dispositions, construction/residual references, policy versions, resource counters, and deterministic replay identity.

## 5. Public entrypoint and capability boundaries

### 5.1 Typed stage entrypoint

Provide an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<canonical_intersection_complex<T,I>>
build_canonical_intersection_complex(
    const boolean_context<T,I>& context,
    const precision_context<T>& precision,
    const validated_operands<T,I>& validated,
    const source_triangle_complexes<T,I>& source_triangles,
    const canonical_source_manifolds<T,I>& manifolds,
    const canonical_candidate_stream<T,I>& candidates,
    const signed_feature_relations<T,I>& relations);
```

The exact wrapper naming may follow Component 01 conventions. Observable behavior must:

- validate all inputs before allocating authoritative registry state;
- support either or both operands empty and an empty relation/seed set;
- execute all phases under one stage transaction;
- select the same primary failure for every allowed traversal, partition, and worker schedule;
- join all private work before rollback;
- expose no partial event, incidence, source-edge, or carrier artifact; and
- commit only after the independent verifier accepts the complete artifact.

Lower-level test entrypoints may normalize one seed group, order one carrier, or build one source-edge partition. Ordinary pipeline publication remains all-or-nothing.

### 5.2 Required predecessor views

Consume narrow owner-checked immutable views.

From Component 07 require:

- artifact header, versions, operation, predecessor identities/digests, verification disposition, and deterministic partitions;
- canonical event-seed count, iteration, checked access, complete event-equivalence key, complete distinct-occurrence key, event class, contact class, operand roles, and authoritative relation;
- authoritative construction or accepted-source-point reference, secondary witnesses, bounded parameters, intervals, residuals, conditioning, precision-ledger, and tolerance disposition;
- complete source vertex/edge/facet/triangle/halfedge/shell incidence expected for each seed;
- relation, candidate, crossing-contribution, symbolic-eligibility, symbolic-decision, overlap, and carrier references;
- signed numeric and symbolic crossing contributions and half-open ownership evidence;
- transverse facet-pair carrier records and relation-supported parameter intervals;
- coplanar support, collinear overlap, overlap interval/component, orientation, endpoint, and region-incidence records;
- candidate-to-relation/seed and source-feature reverse maps; and
- no mutation, lazy arithmetic, or permission to choose a second construction.

From Component 06 require candidate identity/order and checked lookup only for validating Component 07 candidate mappings. Do not rescan geometry or broad-phase bounds.

From Component 05 require canonical source vertices and bounded-point attachments; source edges, endpoints, canonical direction, reciprocal halfedges, incident facets/triangles/shells, and original/internal-diagonal status; source triangles and local edge roles; source-facet groups, rings, boundary uses, semantic identity, triangle membership, and support/projection identity; source shell groups; and checked reverse incidence.

From Component 04 require semantic source-facet rings, triangle-local boundary/internal-diagonal provenance, triangle coverage evidence, semantic digest, and exact-triangulation digest.

From Component 03 require owner-checked bounded points/parameters/intervals/residuals/constructions; five-way interval comparison; exact-nominal equality evidence; interval-domain containment; orientation remapping; interval-length disposition; finite-bound checks; conditioning/tolerance classification; and canonical scalar-bit services.

From Component 02 require source shell/facet orientation, nesting, occupied-side semantics, original boundary ownership, and source winding contract. From Component 01 require complete key comparison, strong-ID publication, checked arithmetic, resources, cancellation, deterministic execution/failure scopes, diagnostics, replay, canonical bytes, SHA-256, and transactions.

Do not expose mutable predecessor arrays, raw ordinals without owner/range validation, provider pointers, or callbacks that can recompute predecessor facts.

### 5.3 Downstream capability

Components 09-11 receive a `canonical_intersection_complex_view<T,I>` that supports:

- header/version/predecessor-digest inspection;
- checked canonical iteration and random access by event, occurrence, incidence, source-edge sequence/interval, carrier, membership, cluster, overlap, aggregate, and descriptor ID;
- event key, occurrence key, event-to-occurrence range, and occurrence-to-event lookup;
- one authoritative bounded-point reference per geometric event;
- seed/relation/candidate/source-feature forward and reverse mappings;
- complete source-feature incidence and original/internal-diagonal provenance;
- source-edge sequences, endpoint sentinels, clusters, open intervals, boundary deltas, and continuation flags;
- transverse carrier orientation/support references, ordered occurrence memberships, relation-supported active spans, and source-facet region provenance;
- coplanar support, collinear overlap, dual source-edge parameters, endpoint occurrences, symbolic ownership, and region incidence;
- crossing/contact/tangent/coincidence aggregates plus immutable member ranges;
- exact cut/contact/occurrence-separation descriptors for classification and selection;
- deterministic partitions with no mutation, allocation, hidden traversal, or lazy arithmetic; and
- canonical section and artifact digests.

Downstream components must not receive raw seeds plus permission to re-intern, recompute coordinates, infer event order, or connect carrier events geometrically.

## 6. Strong IDs, closed enums, and complete keys

### 6.1 Strong ID domains

Add distinct Component 08 domains for:

- `event_id`;
- `event_occurrence_id`;
- `event_seed_binding_id`;
- `event_incidence_id`;
- `source_edge_membership_id`;
- `source_edge_sequence_id`;
- `source_edge_cluster_id`;
- `source_edge_interval_id`;
- `transverse_carrier_id`;
- `coplanar_support_id`;
- `collinear_overlap_carrier_id`;
- `carrier_membership_id`;
- `carrier_cluster_id`;
- `carrier_active_span_id`;
- `coplanar_overlap_record_id`;
- `coplanar_region_incidence_id`;
- `crossing_aggregate_id`;
- `contact_aggregate_id`;
- `intersection_descriptor_id`;
- `ordering_certificate_id`; and
- verifier-evidence IDs where generic evidence cannot express the domain safely.

Do not alias these domains to one another, predecessor IDs, `I`, `size_t`, or raw offsets. Dense ordinals are checked storage details only.

### 6.2 Event and occurrence categories

Use explicit nonzero closed values, including at least:

```cpp
enum class intersection_event_class : std::uint8_t {
    source_vertex_contact = 1,
    edge_facet_point = 2,
    edge_edge_point = 3,
    overlap_endpoint = 4,
    tangent_point = 5,
    multi_feature_meeting = 6,
    symbolic_tie = 7,
    coincident_cluster_member = 8
};

enum class occurrence_role : std::uint8_t {
    single_occurrence = 1,
    topology_separated_contact = 2,
    coincident_sheet_member = 3,
    symbolic_side_occurrence = 4,
    overlap_boundary_occurrence = 5,
    multiplicity_occurrence = 6
};
```

The exact payload design may use tagged variants, but zero/unknown values fail and contradictory field combinations must be unrepresentable.

### 6.3 Membership and descriptor categories

Define closed enums for:

- endpoint/interior/overlap-start/overlap-end/interval-member membership role;
- source-edge endpoint sentinel side;
- definitely-before/exact-equal/definitely-after/unresolved-overlap/invalid comparison result, matching Component 03 without lossy translation;
- definitely-positive/exact-zero/uncertain/overlap interval-length status;
- transverse/coplanar/original-source-edge/collinear-overlap carrier class;
- proper crossing, endpoint crossing, tangent, contact delimiter, coplanar overlap boundary, coincident-sheet boundary/interior, bookkeeping-only, topology-separated contact, no influence, unresolved, and invalid descriptor categories;
- cluster equivalence status: exact parameter coincidence, exact coordinate coincidence, lineage-authorized unresolved cluster, or mixed symbolic tie; and
- span activation: inactive, active transverse intersection, active overlap boundary, active coincident boundary, contact-only, unresolved, or invalid.

Every serialized enum requires explicit underlying type and stable numeric values.

### 6.4 Complete event key

Normalize Component 07 event-equivalence input into:

```text
(
  context semantic owner namespace,
  event class,
  directed operand roles,
  complete canonical source-feature owners on both operands,
  authoritative public relation key,
  authoritative construction/source-point role,
  source-vertex owner when reused,
  transverse/coplanar/overlap carrier role when semantic,
  contact and symbolic rule identity when occurrence equivalence depends on it,
  event-equivalence policy version,
  event-key schema version
)
```

The key must not contain nominal coordinates, uncertainty widths, parameter values, hashes, dense IDs whose meaning changes under canonical remap, candidate discovery order, triangle ID when declared bookkeeping-only, worker/task identity, pointer, or allocation order.

Triangle-local provenance may be retained in seed bindings and incidence, but event identity must reduce to original source-feature lineage wherever Component 07 marks the triangle/internal diagonal as bookkeeping.

### 6.5 Complete occurrence key

Define:

```text
(
  complete normalized event key,
  occurrence role,
  complete Component 07 distinct-occurrence discriminator,
  source shell/sheet occurrence identities when semantic,
  symbolic side/priority discriminator when semantic,
  multiplicity slot when explicitly supplied by Component 07,
  occurrence policy version,
  occurrence-key schema version
)
```

Never synthesize a multiplicity or sheet slot from insertion order. The Component 07 seed must supply the semantic discriminator. `single_occurrence` is an explicit stable value, not an omitted field.

### 6.6 Source-edge membership key

Use:

```text
(
  canonical source edge key,
  event occurrence key,
  membership role,
  authoritative parameter record key,
  relation/overlap lineage,
  source-facet-use role,
  membership schema version
)
```

The parameter value does not establish membership identity. It is authoritative evidence attached to the lineage-keyed membership.

### 6.7 Carrier keys

A transverse carrier key contains:

```text
(
  canonical ordered source-facet pair,
  Component 07 facet-pair carrier construction key,
  canonical orientation policy result,
  support/provider version,
  carrier-key schema version
)
```

Two carrier constructions may group only when Component 07 exposes the same canonical carrier lineage or an explicit designated-producer/witness relation. Equal line coordinates or directions are insufficient.

A collinear-overlap carrier key contains:

```text
(
  canonical ordered source-edge pair,
  coplanar support identity,
  overlap relation/component lineage,
  same/opposite source-edge direction relation,
  symbolic ownership/half-open policy identity,
  carrier-key schema version
)
```

Separate source edges remain separate carriers linked by this record; they are never replaced by one welded source edge.

### 6.8 Cluster, interval, and descriptor keys

- Cluster keys contain carrier/source-edge identity, proven common-location class, complete sorted member occurrence keys, tie-order policy, and schema version.
- Interval keys contain parent sequence key, ordered left/right boundary references (sentinel or cluster), interval ordinal derived only after canonical boundary order, interval class, and schema version.
- Active-span keys contain parent carrier key, relation-supported interval lineage, ordered boundary clusters, source-facet region incidence, activation class, and schema version.
- Descriptor keys contain source-topology locus, exact boundary/interval/sector identity, relation-derived category, orientation role, and descriptor version.

Hashes may accelerate lookup, but equality and ordering compare complete keys. IDs are assigned only after final canonical sorting.

### 6.9 Operand and direction remapping

Every key, category, parameter, crossing sign, carrier orientation, membership role, span activation, and descriptor must define:

- operand exchange;
- canonical source-edge direction reversal;
- source-facet orientation reversal only where predecessor semantics permit it;
- axis permutation effects limited to bounded-value payloads, never identity; and
- exact involution tests.

Parameter remapping uses Component 03's versioned operation, conceptually `[lo,hi] -> [1-hi,1-lo]` for normalized edge parameters, with exact domain and signed-zero policy. Do not implement ad hoc subtraction in Component 08.

## 7. Immutable artifact and table schemas

### 7.1 Artifact header

`canonical_intersection_complex<T,I>` stores:

- context owner, operation, operand IDs, and ordinary-publication eligibility;
- Component 08 provider/policy/schema/key/order/codec/verifier versions;
- required Component 01-07 versions, artifact IDs, and digests;
- source semantic and exact-triangulation digests for both operands;
- strict floating profile and precision-capability references;
- counts/ranges for every table and closed category;
- counts of seeds, events, occurrences, incidences, source-edge memberships/sequences/clusters/intervals, transverse/coplanar carriers, memberships/clusters/spans, overlap records, aggregates, and descriptors;
- deterministic partition records;
- resource/statistics and verification-evidence ranges;
- replay reference;
- separate section digests and complete artifact digest; and
- zero reserved fields.

No mutable cache, task-local handle, allocator, callback, caller pointer, or provider workspace may escape.

### 7.2 Event record

Each `canonical_event_record<T>` contains:

- `event_id`, complete event key, event class, owner, and versions;
- canonical operand/source-feature owner ranges;
- authoritative public relation ID/key;
- one `bounded_point_reference` tagged as either accepted source bounded point or Component 07 construction;
- precision-ledger, residual, conditioning, and tolerance-disposition references;
- sorted secondary construction-witness range;
- sorted occurrence range;
- sorted seed-binding range;
- sorted relation/candidate/source-feature/triangle/halfedge/shell incidence ranges;
- contact/crossing/symbolic summary references that remain reconstructible from members;
- deterministic diagnostic/replay evidence; and
- reserved fields set to zero.

Non-geometric future event classes may use an explicit `no_point` tagged alternative only after a version change. V1 event classes listed in the specification that represent a point must have exactly one point reference.

### 7.3 Event occurrence record

Each occurrence contains:

- `event_occurrence_id` and parent `event_id`;
- complete occurrence key and role;
- complete distinct-occurrence discriminator;
- source shell/sheet and symbolic/multiplicity discriminator fields when applicable;
- sorted seed-binding and incidence ranges specific to this occurrence;
- flags stating whether the occurrence may share an output coordinate, must remain topology-separate, may participate in one local traversal cluster, and requires point/edge-contact separation;
- membership ranges on source edges and carriers;
- cluster memberships;
- aggregate contribution ranges; and
- descriptor ranges.

An occurrence record does not duplicate nominal coordinates or bounds. Access to geometry is through its parent event.

### 7.4 Seed binding record

For every Component 07 `event_seed_id`, publish exactly one binding containing:

- seed ID/key and canonical seed ordinal;
- target event and occurrence IDs;
- authoritative relation/construction/source-point references;
- complete source-feature and discovery provenance;
- expected membership/carrier/overlap roles;
- crossing/symbolic contribution references;
- duplicate-consumer or designated-witness status; and
- compatibility evidence for its grouped event/occurrence.

The binding table remains in Component 07 seed order for direct completeness checks, while reverse ranges are sorted by complete consumer key.

### 7.5 Incidence record

Use one closed tagged `event_incidence_record` with explicit payload variants for:

- source vertex;
- source edge;
- source edge interval endpoint/member;
- source facet;
- source triangle;
- oriented halfedge;
- source shell;
- relation;
- candidate;
- transverse carrier;
- collinear/coplanar carrier;
- overlap component/interval;
- crossing contribution;
- symbolic decision; and
- contact/cut descriptor precursor.

Each record includes event and occurrence IDs as applicable, complete predecessor key/reference, role, orientation, multiplicity/contact metadata, and provenance. Build sorted CSR indexes by event, occurrence, seed, relation, candidate, and each source-feature domain. Do not duplicate semantic incidence merely to simplify lookup; multiple indexes may reference one canonical incidence record.

### 7.6 Source-edge membership and sequence

A source-edge membership contains:

- membership ID/key;
- canonical source edge and directed representative;
- event occurrence and parent event;
- Component 03 bounded parameter reference on the canonical edge direction;
- endpoint/interior/overlap-start/overlap-end role;
- relation and overlap lineage;
- crossing/contact/tangent/coincidence/symbolic contribution ranges;
- both incident source-facet use references and orientation roles;
- exact-equal/cluster eligibility evidence;
- ordering-certificate reference; and
- source-triangle/internal-diagonal discovery evidence retained only as bookkeeping.

A source-edge sequence contains:

- sequence ID and source edge;
- explicit start/end endpoint sentinel records tied to source vertex identities when no event occurrence is required at an endpoint;
- ordered cluster range;
- ordered membership range by cluster then tie key;
- interval partition range;
- aggregate and descriptor ranges;
- canonical orientation/remap evidence; and
- sequence digest/statistics.

Endpoint sentinels are not events and cannot carry opposite-operand contact semantics. If Component 07 supplies an endpoint event, that occurrence appears in a cluster at parameter zero/one and the sentinel still denotes the source-edge domain boundary.

### 7.7 Cluster record

Each source-edge or carrier cluster contains:

- cluster ID/key and parent sequence/carrier;
- cluster class;
- sorted distinct occurrence members and membership IDs;
- authoritative parameter records for every member;
- exact-equality or explicitly permitted unresolved-cluster evidence;
- complete frozen tie keys and deterministic local order;
- member crossing/contact contribution ranges;
- flags for shared output coordinate and required separate output occurrences;
- predecessor/successor cluster references after final order; and
- ordering-certificate references.

A cluster is not an event merge. It never changes event or occurrence identity and never cancels contributions by coordinate count.

### 7.8 Source-edge interval record

Each open interval contains:

- interval ID/key and parent sequence/source edge;
- left/right boundary references, each sentinel or cluster;
- source edge canonical direction;
- Component 03 parameter-domain evidence for the open span;
- length disposition: definitely positive, exact zero, uncertain, or overlap interval;
- inherited incident source-facet uses;
- left/right and accumulated crossing deltas;
- tangent/contact/coincidence/symbolic states;
- whether Component 09 may propagate classification through the interval;
- whether Components 10/11 may retain, split, duplicate, or suppress the interval;
- relation/overlap provenance; and
- verifier reconstruction evidence.

Do not drop exact-zero or equal-nominal-coordinate intervals. An uncertain interval that changes possible adjacency or classification is a typed failure unless the exact contract marks it as a preserved coincident/overlap interval.

### 7.9 Transverse carrier record

Each transverse carrier contains:

- carrier ID/key;
- canonical source-facet pair and operand roles;
- Component 07 bounded carrier construction/reference;
- canonical orientation and orientation derivation evidence;
- conditioning, residual, precision-ledger, and tolerance references;
- sorted relation/candidate provenance;
- ordered membership/cluster ranges;
- active span range;
- source-facet region incidence range;
- aggregate and descriptor ranges; and
- carrier digest/statistics.

A carrier record represents common support, not automatic connectivity.

### 7.10 Carrier active-span record

Each span contains:

- parent carrier;
- ordered left/right boundary clusters;
- activation class;
- exact Component 07 relation interval/component lineage proving activation;
- source-facet region membership for both facets;
- crossing/contact/coincidence ownership;
- whether Component 09 treats the span as a classification cut, contact delimiter, or inactive gap;
- whether Component 11 may construct an output intersection edge on it; and
- consistency evidence that no unsupported gap was bridged.

Several disconnected active spans may exist on one carrier. An interval between adjacent carrier clusters is inactive unless relation evidence proves otherwise.

### 7.11 Coplanar and collinear overlap records

A coplanar support record contains source-facet pair/support identity, orientation relation, symbolic policy, source-boundary carrier ranges, overlap components, region incidences, and provenance.

A collinear overlap carrier record contains:

- both original source edge identities and canonical directions;
- same/opposite direction relation;
- bounded overlap parameter intervals on each edge;
- parameter remap correspondence;
- start/end event occurrences;
- symbolic ownership and half-open boundary rules;
- distinct-occurrence/separate-sheet requirements;
- source-facet/shell provenance; and
- member-preserving contribution/descriptor ranges.

A coplanar region-incidence record contains overlap component identity, source facets/triangles, boundary carriers/events, interior/contained/equal-region status, orientation/coincidence class, symbolic owner, and source-facet semantic digest evidence.

### 7.12 Aggregate records

Crossing/contact aggregate records store:

- aggregation locus: event occurrence, cluster, source edge, source-edge boundary, carrier cluster, carrier span, overlap boundary, source facet, or shell;
- numeric signed crossing sum;
- symbolic crossing/ownership state;
- zero-net tangent/contact state;
- sorted immutable member contribution IDs;
- per-source-facet and per-shell subtotals;
- entering/leaving order under the canonical directed locus;
- cancellation/conservation evidence; and
- verifier reconstruction reference.

Never store an aggregate without members. Zero sum does not imply no contact.

### 7.13 Cut/contact descriptor record

Each descriptor identifies one exact source-topology locus and one category. Required loci include:

- whole source edge with no events;
- source-edge open interval;
- source-edge event cluster boundary;
- source vertex sector/fan transition;
- source-facet adjacency across an original edge;
- transparent internal-diagonal adjacency;
- transverse carrier active span;
- transverse inactive gap;
- coplanar overlap boundary/interior;
- coincident sheet boundary/interior; and
- topology-separated point/edge contact occurrence.

Each descriptor contains orientation, signed crossing delta where applicable, continuation permission, occurrence-separation requirement, symbolic ownership/rule, relation/event/cluster/span provenance, and downstream-consumption flags. Descriptors are relation-derived connectivity constraints, not global winding labels.

## 8. Preflight, validation, and resource accounting

### 8.1 Defensive predecessor validation

Before semantic allocation, validate:

- owner/context/operand/operation consistency across all artifacts;
- exact required version and strict-floating-profile compatibility;
- predecessor section and complete digests;
- verified-success disposition and ordinary-publication eligibility;
- all seed, relation, candidate, construction, parameter, interval, overlap, carrier, source-feature, and contribution ID ranges;
- every Component 07 seed has a nonzero supported event class, complete event-equivalence key, complete occurrence discriminator, authoritative relation, and point/source reference;
- every construction/source point belongs to the expected context and source lineage;
- every source feature exists and internal-diagonal status agrees across Components 04/05/07;
- every candidate mapping references a valid Component 06 candidate and complete disposition;
- every parameter/interval is finite, owner-correct, and in its declared domain;
- every carrier/overlap record has complete source-facet/source-edge ownership and conditioning disposition; and
- all reserved fields are zero.

A contradiction in a committed predecessor artifact is `internal_invariant_error`; do not repair it.

### 8.2 Exact and worst-case count arithmetic

Use Component 01 checked arithmetic for at least:

- seeds and seed bindings;
- unique event and occurrence upper bounds;
- total seed-consumer/source-feature/relation/candidate incidence entries;
- reverse-index entries for every domain;
- source-edge membership proposals and affected-edge count;
- endpoint sentinels, clusters, and interval partitions;
- transverse carrier proposals, memberships, clusters, and pairwise active-overlap checks;
- coplanar supports, collinear overlap carriers, endpoint memberships, overlap records, and region incidences;
- aggregate member references and descriptor records;
- canonicalization/remap tables;
- ordering active-set, cluster, and certificate work;
- codec/diagnostic/replay bytes;
- independent verifier scratch; and
- persistent artifact bytes.

Use conservative bounds from actual Component 07 record counts and declared per-record incidence ranges; do not use unbounded products when a tighter audited sum is available. Every multiplication/addition must have a named failure witness and resource kind.

### 8.3 Resource categories

Account separately for:

- input seed scan work;
- event/occurrence proposals;
- event/occurrence persistent records;
- incidence proposals and persistent records;
- forward/reverse index entries;
- source-edge memberships, clusters, intervals, and ordering comparisons;
- transverse memberships, clusters, spans, and ordering comparisons;
- coplanar supports/carriers/overlaps/regions;
- aggregate members and descriptors;
- canonicalization/remap work;
- diagnostics/replay;
- verifier work and exact-oracle test work; and
- persistent bytes.

Reserve before each major phase under the stage transaction, reconcile actual usage after the phase, and release temporary reservations deterministically. Never continue with truncated incidence or a partial high-valence event.

### 8.4 Pathological incidence policy

High-valence events, many equal-coordinate occurrences, many events on one source edge, and many disjoint spans on one carrier are genuine output complexity. V1 must remain output-sensitive and may return deterministic `resource_limit` when configured below the true requirement. It must not merge occurrences, drop contributors, or collapse clusters to hide complexity.

## 9. Seed normalization and event/occurrence interning

### 9.1 Normalization procedure

For each Component 07 seed in canonical order:

1. validate owner, class, relation, construction/source-point, source-feature, carrier/overlap, symbolic, and occurrence fields;
2. map triangle-local/internal-diagonal discovery fields to original source-feature ownership exactly as Component 07 declares;
3. construct the complete normalized event key using only semantic lineage;
4. construct the complete occurrence key with an explicit occurrence role/discriminator;
5. construct a seed-binding proposal carrying all non-identity provenance and expected incidences;
6. encode key fields canonically for collision/ordering tests; and
7. record a deterministic failure key before moving to the next seed.

Add debug/test assertions that serialized event/occurrence keys contain no `T` bit payload, parameter endpoint, uncertainty width, hash value, raw pointer, worker ID, or input ordinal except stable semantic ordinals explicitly supplied by predecessor keys.

### 9.2 Canonical grouping

Sort proposals by full event key, then full occurrence key, then complete seed key. Scan groups:

- an event-key group creates one conceptual event proposal;
- within it, each occurrence-key subgroup creates one occurrence proposal;
- duplicate seed keys are invalid unless Component 07 explicitly marks one as duplicate consumer evidence under a unique predecessor record model;
- all members of an event group must agree on event class, owners, authoritative relation/construction role, contact class, and point semantics;
- all members of an occurrence subgroup must agree on occurrence role, topology-separation flags, symbolic side, sheet/multiplicity discriminator, and expected memberships; and
- conflicting records fail before IDs are assigned.

No hash table winner or first record is authoritative. The designated Component 07 producer relation is authoritative only because the predecessor record says so and every group member verifies that designation.

### 9.3 Canonical ID assignment

After all groups validate:

- assign `event_id` in strict normalized event-key order;
- assign `event_occurrence_id` in strict `(event key, occurrence key)` order;
- assign each event a contiguous occurrence range;
- assign seed bindings in Component 07 seed order for direct mapping, with separate canonical reverse ranges; and
- create checked dense-ordinal maps from predecessor seed IDs to event/occurrence IDs.

IDs must be independent of input permutations after predecessor canonicalization, worker schedule, and hash implementation.

### 9.4 Required interning invariants

Verify:

- every seed maps to exactly one event and one occurrence;
- every event has at least one occurrence and one seed binding;
- every occurrence belongs to exactly one event and has at least one seed binding;
- event keys are unique and strictly ordered;
- occurrence keys are unique within and globally ordered by parent event key;
- equal event keys never produce two event IDs;
- different occurrence keys never produce one occurrence ID;
- different event keys never merge even if every geometric payload bit is equal; and
- no key references an internal diagonal as public owner.

## 10. Authoritative coordinates and witness validation

### 10.1 Point reference model

Use a tagged immutable `bounded_point_reference`:

- `source_point`: `(operand, canonical source vertex, Component 03/05 bounded-point ID)`; or
- `constructed_point`: `(Component 07 relation construction ID, precision-ledger ID)`.

The event record stores the reference, not a recomputed coordinate. Accessors may expose the referenced bounded point through predecessor views while preserving owner checks.

### 10.2 Existing source vertex events

When Component 07 marks an accepted source vertex as authoritative:

- verify the source vertex identity is one of the event's authorized owners;
- verify the bounded-point reference exactly matches Component 05/03;
- verify every endpoint parameter witness is exact endpoint evidence under Component 03;
- reject any interpolated construction for the same event unless it is explicitly a non-authoritative witness; and
- preserve separate events/occurrences at the same source vertex when their keys differ.

Do not construct `p0 + 0*(p1-p0)` or `p0 + 1*(p1-p0)`.

### 10.3 Constructed events

For a constructed event:

- verify one and only one designated Component 07 construction producer;
- verify finite nominal coordinate and conservative enclosure;
- verify source-feature lineage and relation role;
- verify all parameter records and domains;
- verify support/edge/facet residual references and accepted conditioning/tolerance disposition;
- verify precision-ledger ownership and monotonicity; and
- attach the same reference to all occurrences and consumers.

Component 08 does not copy the coordinate into occurrence/membership records.

### 10.4 Secondary construction witnesses

For every non-authoritative witness required by Component 07 policy, independently check through Component 03 capabilities:

- authoritative nominal point is contained in the required witness enclosure;
- witness and authoritative records refer to compatible source supports/carriers;
- parameter intervals overlap or relate exactly as the witness role requires;
- endpoint/source-vertex ownership agrees;
- support residuals and conditioning classifications are compatible;
- witness does not claim a second authoritative producer; and
- any difference is explainable under the documented bounded-combination rule.

Do not average, snap, choose the smallest bound, or widen all bounds. If the prescribed compatibility proof fails, return the deterministic relation/event witness error.

### 10.5 Shared-coordinate invariant

Instrument test builds so every geometry accessor for an occurrence, source-edge membership, carrier membership, cluster member, or descriptor resolves through the parent event's one point reference. Mutation tests must detect any copied and altered nominal value or bound.

## 11. Incidence and reverse mappings

### 11.1 Proposal collection

Emit private incidence proposals from every seed binding and referenced Component 07 relation record. Each proposal contains a complete incidence key and predecessor proof reference. Required incidence includes all applicable:

- source vertices represented/touched;
- source edges containing the occurrence;
- source facets containing/bounding the occurrence;
- source triangles and oriented halfedges that discovered/consume it;
- shells and operand roles;
- relation and candidate IDs;
- crossing contribution IDs and symbolic decisions;
- transverse/coplanar carriers and overlap components;
- contact/cut/tangent/coincidence categories; and
- endpoint/interval roles.

### 11.2 Canonicalization and duplicate rules

Sort by complete incidence key. Exact duplicate proposals may coalesce only when they refer to the same semantic incidence and compatible proof references. Preserve multiple distinct member contributions even when their source feature and numeric value coincide.

Assign incidence IDs in canonical key order. Build CSR-style ranges by event, occurrence, seed, relation, candidate, source vertex, source edge, source facet, source triangle, halfedge, shell, carrier, and overlap record using deterministic prefix sums.

### 11.3 Completeness checks

Verify bidirectionally:

- every seed's declared incidence appears in its event/occurrence;
- every event/occurrence incidence has at least one authorizing seed/relation record;
- every relation-to-event and candidate-to-event mapping agrees with Component 07;
- every source-edge membership has source-edge incidence;
- every carrier membership has carrier/source-facet incidence;
- every original source-boundary event retains original vertex/edge ownership;
- no unrelated feature is attached merely because geometry is coincident; and
- internal diagonals appear only in bookkeeping discovery incidence and never as public source owner.

## 12. Bounded ordering provider

### 12.1 Why ordinary sorting is forbidden

A comparator over overlapping bounded intervals is not necessarily a strict weak order. Do not pass a comparator that returns nominal `<` or treats unresolved overlap as equality to `std::sort`. V1 must use a two-stage deterministic interval-sweep provider that explicitly validates every potentially overlapping pair.

### 12.2 Candidate pre-order for sweep only

For memberships on one directed carrier/source edge:

1. validate every parameter record and domain;
2. create a non-semantic sweep key from Component 03 canonical total ordering of lower endpoint bits, upper endpoint bits, then complete membership key;
3. sort by this sweep key only to enumerate potentially overlapping intervals; and
4. never publish this order as topology order without comparison certificates.

The sweep key may use parameter bits for work ordering, not identity or semantic order.

### 12.3 Active-set comparison

Maintain an active set of intervals whose upper bound is not definitely before the next lower bound under Component 03's certified comparison. For every new membership, compare it against every active membership using the five-way bounded parameter service.

Record one ordering certificate per relevant pair:

- `definitely_before` or `definitely_after`: add a proven precedence relation;
- `exact_equal`: require exact-equality evidence and mark cluster-compatible subject to occurrence rules;
- `unresolved_overlap`: call the explicit cluster-eligibility rule;
- `invalid`: fail.

The cluster-eligibility rule may return true only when Component 07 lineage/symbolic evidence proves that either local order yields the same topological partition and the members are required to remain a coincident cluster. If adjacency, interval length, crossing sequence, cut topology, or active-span structure could differ, fail with `geometric_condition_exceeds_tolerance`.

### 12.4 Cluster equivalence construction

Build cluster components only from exact-equal or explicitly clusterable pair relations. Verify the relation is symmetric and that every pair within a resulting cluster is mutually cluster-compatible; do not rely on transitive union alone. A-B and B-C clusterability does not imply A-C.

For each valid cluster, sort members by the complete frozen occurrence tie key supplied by Component 01/07. The tie order affects traversal only; it does not merge identities or alter numeric parameters.

### 12.5 Proven cluster order

Collapse valid clusters to nodes. Derive precedence from certified pair relations. Use a deterministic topological sort with complete cluster key as the queue tie breaker. Require a unique semantic linear order in this sense:

- every adjacent output cluster pair must have a certified definite order;
- no cycle may exist;
- no incomparable cluster pair may affect interval adjacency; and
- a canonical-key tie break may select among incomparable nodes only if an explicit proof says their interchange leaves all partitions, deltas, spans, and descriptors identical.

In V1, prefer failure over such an interchange unless the cluster/independence policy explicitly covers it.

### 12.6 Ordering certificate

Publish enough evidence to independently reconstruct:

- input memberships and parameters;
- sweep enumeration;
- every active overlapping pair comparison;
- cluster eligibility and all-pairs cluster validation;
- precedence graph;
- topological order; and
- final tie-key member order.

The independent verifier must not trust producer ordinals or a stored `sorted=true` flag.

## 13. Source-edge arrangements

### 13.1 Membership generation

For each original source edge, collect memberships authorized by Component 07 incidence, including isolated events, overlap endpoints, and occurrences at source vertices. Exclude internal triangulation diagonals from public source-edge sequences; their discoveries must map to the containing source facet or original boundary feature.

Each membership uses the source edge's Component 05 canonical directed representative. If Component 07 supplies a parameter in the opposite direction, remap it through Component 03 and retain remap evidence.

### 13.2 Endpoint handling

Every affected source edge sequence contains immutable start/end sentinels. Rules:

- sentinel 0 references the canonical start source vertex and exact parameter zero;
- sentinel 1 references the canonical end source vertex and exact parameter one;
- endpoint event occurrences form clusters at the corresponding sentinel location but remain separate records;
- a source vertex can host several distinct event/occurrence members;
- no endpoint event is invented merely to partition the edge; and
- downstream APIs distinguish sentinel boundaries from event clusters.

### 13.3 Sequence ordering and clusters

Run the bounded ordering provider over memberships. Add endpoint sentinels as fixed boundary nodes and verify every membership is within `[0,1]` and correctly classified endpoint/interior/overlap-end. Exact-equal parameter memberships form explicit clusters with deterministic occurrence tie order.

### 13.4 Interval partition construction

Given ordered boundary nodes `[start sentinel, zero or more clusters, end sentinel]`, create one open interval between every adjacent pair, plus explicit cluster-location records. For each interval:

- derive parameter-domain evidence from boundary parameter records;
- classify length through Component 03 without using nominal subtraction;
- attach inherited source-facet uses from Component 05;
- aggregate left/right crossing and contact contributions from member records;
- identify overlap intervals from Component 07 overlap lineage;
- derive continuation/split/duplicate/suppress permissions; and
- retain exact provenance.

A definitely positive ordinary interval is traversable unless a cut/contact descriptor says otherwise. Exact-zero, uncertain, and overlap intervals require explicit descriptor handling; never silently erase them.

### 13.5 Crossing delta convention

Use Component 07's canonical source-edge traversal sign. At each cluster, retain member contributions and compute the aggregate delta in deterministic member order using checked integer arithmetic. Verify traversal reversal negates/remaps the delta exactly and endpoint half-open ownership prevents duplicate crossing count.

### 13.6 Unaffected edges

The artifact need not allocate a full sequence for every source edge if a canonical implicit descriptor represents an unaffected edge. However, Component 09 must be able to query any source edge and receive one of:

- explicit affected sequence; or
- verified implicit `no_intersection_influence` record tied to complete Component 07 reverse-map coverage.

The verifier must prove no seed/overlap incidence was omitted from an implicitly unaffected edge.

## 14. Transverse carrier arrangements

### 14.1 Carrier grouping

Collect Component 07 non-coplanar facet-pair carrier proposals. Group only by complete transverse carrier key. Verify designated producer/witness consistency, source-facet pair, canonical orientation, support construction, residuals, conditioning, precision, and tolerance disposition.

Do not group carriers from equal nominal line supports without common lineage.

### 14.2 Canonical orientation

Derive orientation solely from the frozen Component 07/05 source-facet identity and oriented-support rule. Store the derivation evidence. Every membership parameter must be expressed in this orientation through Component 03 remapping. Operand exchange and facet-order remap must be total and involutive.

### 14.3 Memberships and order

Collect event occurrences and overlap endpoints authorized on the carrier. Validate parameter/support/source-facet-region incidence. Order and cluster them using the same bounded ordering provider, with carrier-specific cluster eligibility.

Several disconnected intersections may share one carrier. The ordered membership sequence alone does not declare active connectivity.

### 14.4 Active-span reconstruction

Project no new geometry. Instead, consume Component 07 relation-supported parameter intervals and source-facet region incidence to determine each adjacent cluster interval's activation.

For every adjacent cluster pair:

- gather all Component 07 relation intervals covering the open carrier span;
- verify both source facets contain the span under predecessor region evidence;
- classify active transverse intersection, contact-only, overlap boundary, inactive gap, or invalid;
- preserve contributing relation intervals and ownership;
- reject contradictory open/closed endpoint rules or unsupported activation; and
- never bridge an inactive gap merely because the same infinite carrier continues.

If a relation interval begins/ends without a matching event occurrence or exact sentinel policy, fail.

### 14.5 Carrier endpoint and unbounded support policy

The carrier itself may be mathematically unbounded, but published active spans must be bounded by event clusters derived from finite source-facet regions. V1 must not publish unbounded active intersection edges. A relation claiming active support without finite event boundaries is inconsistent or outside the accepted component contract.

## 15. Coplanar and collinear arrangements

### 15.1 No transverse-carrier fallback

Coplanar facet relations must route through original source-edge carriers, collinear overlap carriers, coplanar support identities, overlap components, and region incidence. Creating an arbitrary line from cross products or projected coordinates is prohibited.

### 15.2 Collinear overlap carrier construction

For each Component 07 collinear overlap relation:

- validate both original source edges and directions;
- validate bounded overlap intervals on both edge parameter domains;
- validate same/opposite direction and parameter correspondence;
- resolve start/end event occurrences from canonical endpoint seeds;
- validate source-vertex ownership at exact endpoints;
- attach symbolic ownership and half-open boundary rules;
- preserve separate source edges and occurrences; and
- create one canonical overlap carrier record keyed by relation lineage.

Nested or repeated overlap intervals remain separate records unless Component 07 explicitly groups them into one overlap component with compatible ownership.

### 15.3 Point contact and zero-length overlap

A collinear relation whose accepted overlap is exact zero is represented as a point-contact event/occurrence and, when needed for provenance, a zero-length overlap record. It is not promoted to a positive-length edge and not discarded.

### 15.4 Coplanar support and region incidence

For each coplanar facet pair, publish:

- support identity and same/opposite orientation;
- all original boundary carriers and overlap records;
- ordered boundary event sequences;
- overlap components and region-incidence records;
- containment/equal-region/coincident-sheet classifications from Component 07;
- symbolic owner and half-open policy; and
- source-facet semantic digest evidence.

Internal triangulation edges may appear only as coverage witnesses. They cannot split the coplanar semantic region or own an overlap boundary.

### 15.5 Multiple coincident sheets

When several source shells/sheets share geometry:

- preserve every event occurrence and sheet/source-topology identity;
- preserve same/opposite orientation and symbolic owner per relation;
- never reduce sheets by coordinate count or net crossing alone;
- expose separate occurrence and descriptor records for Component 09/10; and
- verify operand exchange and source permutation stability.

## 16. Aggregation and cut/contact descriptors

### 16.1 Member-preserving aggregation

Aggregate only after all canonical member records exist. For each locus, sort member contribution IDs by complete contribution key and compute checked sums/subtotals in that order. Store the member range and aggregate.

Required aggregates include:

- per event and occurrence;
- per source-edge cluster and boundary;
- per source edge and interval partition;
- per transverse carrier cluster and active span;
- per collinear overlap endpoint/boundary;
- per source facet and shell where needed for local conservation; and
- per coplanar/coincident component.

### 16.2 Tangent and zero-net contacts

A numeric crossing sum of zero does not erase:

- tangent contact;
- equal and opposite member crossings at a high-valence event;
- symbolic delimiter;
- coplanar overlap boundary;
- coincident-sheet occurrence; or
- point/edge contact requiring topology separation.

Record explicit contact classes and members.

### 16.3 Descriptor derivation table

Implement one versioned total derivation table from predecessor relation/member categories plus local topology to descriptor category. The table must cover every valid combination and reject unknown/contradictory combinations. At minimum:

- no relation influence -> uncut continuation, delta zero;
- proper transverse crossing -> cut, signed delta;
- endpoint crossing -> cut with half-open endpoint ownership;
- tangent -> continuation or contact delimiter exactly as symbolic policy states, delta zero;
- point/edge contact requiring separate occurrences -> absent topological continuation across occurrences;
- coplanar overlap boundary -> coplanar delimiter with symbolic owner;
- coincident sheet boundary/interior -> occurrence-preserving coincident descriptor;
- internal diagonal without authoritative crossing -> transparent continuation;
- inactive carrier gap -> no carrier adjacency; and
- unresolved/invalid evidence -> failure.

Do not infer descriptors from coordinates or from aggregate sum alone.

### 16.4 Source vertex sectors

At high-valence source vertices, derive sector/fan-transition descriptors from Component 05 cyclic vertex fan plus incident event occurrences and Component 07 local contributions. Preserve separate shells and occurrences. Verify a full fan traversal conserves the expected crossing delta and that internal-diagonal transitions remain transparent.

### 16.5 Downstream sufficiency

Add contract tests in which a minimal Component 09 atom/adjacency reconstruction consumes only Component 05 topology plus Component 08 queries and can determine every potential continuation/cut locus without asking Component 07 to recompute geometry or order.

## 17. Source-facet and triangulation reconciliation

### 17.1 Triangle-local coverage

For every source-facet event/carrier incidence:

- collect all Component 07 triangle-local discovery witnesses;
- verify at least one member triangle covers an interior event under the accepted triangulation model;
- verify boundary events retain original source edge/vertex ownership;
- verify discoveries on a shared internal diagonal map to one source-facet semantic event/occurrence;
- verify no event is lost between adjacent source triangles;
- verify no internal diagonal creates a source-edge sequence, carrier break, cut, or classification delimiter by itself; and
- verify all candidate/relation consumers are accounted for.

### 17.2 Alternative triangulation invariance

Golden/metamorphic tests must build legal alternative triangulations of the same source facet. After remapping triangle-local bookkeeping IDs:

- public event and occurrence keys/IDs;
- authoritative points;
- original source-feature incidence;
- source-edge arrangements;
- transverse/coplanar carriers and active spans;
- aggregates/descriptors; and
- semantic section digests

must be identical. Only explicitly triangulation-specific evidence sections may differ, and the complete artifact digest policy must either canonicalize that evidence or provide separate semantic/exact-triangulation digests exactly as frozen by version.

### 17.3 Contradiction policy

Do not resolve contradictory triangle-local incidence by majority vote, nearest triangle, or coordinate coverage. Return `internal_invariant_error` with the least source-facet/event/relation/triangle witness.

## 18. Deterministic execution and canonical publication

### 18.1 Serial semantic reference

Implement and keep enabled a serial path covering normalization, grouping, point attachment, incidence, source-edge/carrier arrangements, aggregation, descriptors, canonicalization, codec, and verifier invocation. Normative tests compare every parallel run to this path.

### 18.2 Parallel-ready boundaries

Parallel execution may partition immutable ranges for:

- seed normalization;
- incidence proposal generation;
- per-source-edge arrangement proposals;
- per-transverse-carrier arrangement proposals;
- per-coplanar-support arrangement proposals;
- aggregate-member collection; and
- verifier scans.

Each task writes only private outputs and structured failures. Tasks must not allocate final IDs, mutate shared semantic maps, commit shared resource order, or publish partial records.

### 18.3 Canonical merge

Merge private outputs by:

1. concatenating in deterministic partition-key order;
2. sorting complete keys;
3. validating exact duplicate compatibility;
4. assigning IDs only after values are final;
5. remapping references through checked canonical maps;
6. building CSR ranges with deterministic prefix sums; and
7. selecting the least canonical structured failure under Component 01 arbitration.

Forced delays, reversed task completion, and worker counts 1, 2, and maximum must reproduce serial bytes, counters, diagnostics, and failure.

### 18.4 Final canonical order

Assign IDs in this order after relevant validation:

1. conceptual events by complete event key;
2. occurrences by parent event key then occurrence key;
3. incidences by complete incidence key;
4. seed bindings remain in Component 07 seed order;
5. source-edge memberships by source edge then membership key;
6. source-edge clusters by source edge and proven sequence order;
7. source-edge intervals by source edge and boundary order;
8. transverse carriers by complete carrier key;
9. coplanar supports and collinear carriers by complete keys;
10. carrier memberships by parent carrier then membership key;
11. carrier clusters by parent carrier and proven order;
12. carrier active spans by parent carrier and boundary order;
13. overlap and region-incidence records by complete keys;
14. aggregates by locus key; and
15. descriptors by source-topology locus key and category.

Canonical IDs may not be assigned from task-local ordinals.

## 19. Producer invariants

Before encoding, reconstruct and verify at least:

- every complete key is valid, unique in its domain, and strictly ordered;
- every Component 07 seed has exactly one binding/event/occurrence;
- one event key has one event ID and one authoritative bounded-point reference;
- every occurrence key is preserved and belongs to exactly one event;
- all event consumers resolve the same point reference;
- source-vertex events use accepted source points rather than interpolation;
- secondary witnesses are compatible under the exact prescribed proof;
- incidence and reverse maps are complete and reciprocal;
- no coordinate/hash/tolerance-derived merge occurred;
- every affected source edge has a complete ordered sequence and partition;
- every topology-relevant order has bounded comparison evidence;
- every cluster preserves distinct occurrence members and complete tie keys;
- every interval/span has valid ordered boundaries and activation evidence;
- transverse carriers connect only relation-supported active spans;
- coplanar relations use source-feature carriers and preserve separate edges/sheets;
- every aggregate equals the checked sum/state reconstructed from members;
- every cut/contact descriptor follows the total derivation table;
- internal diagonals own no public event, carrier, cut, or overlap boundary;
- source-facet semantics are triangulation-consistent;
- all counts/ranges/resources match exact records;
- no task-local/private handle remains; and
- empty inputs produce canonical empty records and digests.

Producer checks supplement but do not replace the independent verifier.

## 20. Canonical encoding, diagnostics, replay, and decode

### 20.1 Canonical encoding

Use Component 01 `CanonicalBytes`. Encode explicit framed sections for:

- artifact header, versions, operation, policies, and predecessor identities/digests;
- event and occurrence keys/records;
- seed bindings;
- incidence records and every reverse index;
- source-edge memberships, endpoint sentinels, sequences, clusters, intervals, and ordering certificates;
- transverse carriers, memberships, clusters, active spans, and support evidence;
- coplanar supports, collinear overlap carriers, overlap records, and region incidence;
- aggregate records and immutable members;
- cut/contact descriptors;
- deterministic partitions, statistics, resources, verifier evidence, and replay references.

Never serialize raw object memory, padding, pointers, `size_t`, container capacity, locale-dependent text, compiler type names, or hash values. Exact `T` bits and intervals use Component 03 canonical bit policy, preserving nominal signed zero where required.

Compute separate SHA-256 digests for:

- header/versions/predecessors;
- event/occurrence keys and records;
- seed bindings/incidence/maps;
- source-edge arrangements/order certificates;
- transverse carriers/spans;
- coplanar/overlap structures;
- aggregates/descriptors;
- evidence/statistics/resources; and
- the complete artifact.

### 20.2 Diagnostics

Structured failures and retained findings include, within policy limits:

- component/stage/checkpoint/subcode;
- complete seed/event/occurrence/relation/source-feature/carrier keys;
- exact nominal bits, parameter intervals, and comparison dispositions;
- authoritative and witness construction/residual/conditioning references;
- cluster members/tie keys and precedence evidence;
- interval/span boundaries and activation provenance;
- crossing/contact contributors and symbolic rule IDs;
- source semantic/exact-triangulation lineage;
- resource limits/counters and cancellation progress; and
- deterministic replay identity.

Core registry code does not log. Human rendering occurs from completed structured records at an API boundary.

### 20.3 Replay

Replay must reproduce seed normalization, event/occurrence grouping, authoritative point selection, witness checks, incidence, source-edge/carrier memberships, bounded comparisons, clusters, partitions, active spans, overlap structures, aggregates, descriptors, IDs, bytes, digests, counters, and primary failure under the same supported platform profile. Worker count and schedule are non-semantic.

### 20.4 Decode

Decode privately and fail closed. Validate tags, lengths, versions, counts, limits, owners, predecessor digests, reserved fields, and exact bit patterns before allocation/publication. Reconstruct every table and index, recompute section/complete digests, and run the same independent verifier. Reject trailing bytes, unknown required fields, duplicate singleton sections, invalid IDs, or task-local references.

## 21. Independent verifier

### 21.1 Independence requirements

`IntersectionVerifier` consumes immutable Component 08 records plus narrow predecessor and Component 03 capabilities. It must not call the producer's event grouping table, occurrence builder, incidence canonicalizer, bounded-ordering sweep, cluster builder, interval partitioner, carrier activation builder, aggregate reducer, or descriptor dispatcher as its sole truth.

It may share stable schemas, complete-key comparators, Component 01 checked arithmetic/canonical bytes/SHA-256, Component 03 public bounded operations, and immutable predecessor views, but must use independent traversal and control flow.

### 21.2 Required reconstruction

The verifier must:

1. rescan every Component 07 seed and independently normalize event/occurrence keys;
2. independently sort/group keys and compare event/occurrence partitions;
3. verify one authoritative point per event and all witness compatibility;
4. reconstruct seed bindings and all forward/reverse incidence from predecessor lineage;
5. independently collect source-edge memberships and validate endpoint/source-feature ownership;
6. independently compare every potentially overlapping parameter pair and reconstruct clusters/order;
7. reconstruct source-edge endpoint sentinels and interval partitions;
8. independently group transverse carriers by lineage and verify orientation/support evidence;
9. reconstruct carrier memberships, order, clusters, and relation-supported active spans;
10. reconstruct coplanar supports, collinear overlap carriers, endpoint occurrences, dual parameters, orientation, ownership, and region incidence;
11. independently sum crossing/contact members and compare aggregates;
12. independently apply the versioned descriptor derivation table;
13. reconstruct triangle-to-source-facet coverage and internal-diagonal transparency;
14. validate all owners, ranges, versions, reserved fields, resources, canonical ordering, and deterministic partitions; and
15. re-encode semantic records and compare all digests.

For bounded fixtures under `exhaustive_test_only`, compare event lineage grouping, exact parameters/order, overlap endpoints, source-edge partitions, carrier active spans, and exact point containment against an independent in-tree exact rational oracle and exhaustive relation-lineage enumeration. Production code never depends on that oracle.

### 21.3 Mutation rejection

Required mutations include:

- merge two events by coordinate;
- split one event key into two events;
- merge or split occurrence keys;
- change an event's point reference or give one event two points;
- replace a source-point event with an interpolation;
- alter a witness enclosure/residual/producer;
- delete/duplicate/remap a seed binding or incidence;
- attach an unrelated source feature;
- assign public ownership to an internal diagonal;
- change a parameter or direction remap;
- swap two topology-relevant clusters;
- mark unresolved overlap as definitely ordered;
- delete a cluster member or tie key;
- delete/bridge a source-edge interval;
- connect inactive spans on one infinite carrier;
- route a coplanar relation through a transverse carrier;
- weld separate source edges/sheets;
- change overlap endpoint ownership or half-open rule;
- alter a crossing/contact member or aggregate only;
- erase a zero-net tangent/contact;
- change a cut/contact descriptor or continuation permission;
- forge counts, ranges, versions, resources, partitions, bytes, reserved fields, or digests; and
- permute canonical arrays without updating every map.

Every mutation must be rejected deterministically.

## 22. Tests and qualification

### 22.1 Event-key and interning unit tests

Cover:

- one seed -> one event -> one occurrence;
- several duplicate consumers of one relation;
- several triangle-local discoveries -> one source-feature event;
- one event with several topology occurrences;
- one source vertex referenced by several different events;
- different event keys with bit-identical coordinates/bounds;
- same event key with different occurrence keys;
- explicit `single_occurrence` normalization;
- hash-collision injection;
- malformed coordinate-derived key fields;
- wrong owner/operand/context/version/stale IDs; and
- canonical ID stability under seed permutations/partitions.

### 22.2 Authoritative coordinate tests

Include:

- stable edge/facet interior construction;
- source endpoint reuse at parameter zero/one;
- edge-edge construction;
- coplanar overlap endpoint;
- several compatible witnesses;
- incompatible enclosure, residual, producer, parameter, endpoint owner, and conditioning mutations;
- distinct events sharing exact nominal bits;
- multiple occurrences sharing one event point; and
- large translation, extreme scale, subnormal, signed-zero, and adjacent-float cases.

### 22.3 Incidence tests

Verify complete forward/reverse maps for source vertex, edge, facet, triangle, halfedge, shell, relation, candidate, carrier, overlap, crossing contribution, and symbolic decision. Include high-valence events, duplicate triangle discovery, internal-diagonal bookkeeping, unrelated geometrically coincident feature rejection, and missing/duplicate consumer mutations.

### 22.4 Bounded ordering known answers

Commit exact comparison/order certificates for:

- definitely ordered wide margin;
- one representable value separation;
- exact equal parameter;
- distinct occurrences with exact equal parameter;
- overlapping intervals safely clusterable by explicit lineage;
- overlapping intervals whose order changes adjacency;
- nominal order opposite the certified bounded order;
- non-transitive attempted cluster triple;
- precedence cycle injection;
- canonical edge/carrier direction reversal; and
- threshold cases just inside, exactly at, and just outside accepted conditioning.

No test may pass because nominal sort happened to agree.

### 22.5 Source-edge arrangement goldens

Commit complete expected sequences, clusters, sentinels, intervals, deltas, descriptors, and incidence for:

- no events;
- one interior event;
- endpoint plus interior events;
- several definitely ordered events;
- equal-parameter cluster;
- distinct events with equal nominal coordinates;
- one overlap interval;
- nested and disjoint overlap intervals;
- concave opposite facet with several crossings;
- zero-nominal-length but topology-preserved interval;
- tangent and point-contact occurrences;
- high-valence endpoint cluster; and
- implicit unaffected edge query.

### 22.6 Transverse carrier tests

Include:

- one face-face segment;
- several disjoint active spans on one carrier;
- inactive gap between spans;
- several exact-equal event clusters;
- near-parallel accepted carrier;
- conditioning failure;
- duplicate triangle candidates sharing one canonical carrier lineage;
- equal nominal line supports with different lineage that must not merge;
- events on the infinite support but outside one facet region;
- missing relation-supported endpoint;
- operand exchange and carrier-orientation remap; and
- direction reversal with parameter remap.

Verify no unsupported connectivity.

### 22.7 Coplanar overlap tests

Cover:

- collinear disjoint edges;
- point contact;
- partial overlap;
- equal edges same direction;
- equal edges opposite direction;
- nested overlap;
- several disjoint overlap intervals from concave boundaries;
- partial/equal facets;
- same/opposite coincident orientation;
- multiple coincident sheets with distinct topology;
- overlap boundaries crossing internal diagonals; and
- separate equal-coordinate source edges that must not weld.

Verify endpoint occurrences, dual parameters, direction relation, symbolic ownership, half-open rules, region incidence, and triangulation invariance.

### 22.8 High-valence and coincident-cluster tests

Include:

- several source edges at one vertex meeting an opposite face;
- several opposite facets incident to one source-edge event;
- two shells touching at one point;
- several conceptual events rounding to one coordinate;
- several occurrences of one event;
- exact-equal carrier parameters;
- coincident sheets requiring separate occurrences; and
- one source vertex participating in transverse, tangent, and coplanar relations.

Verify complete incidence, all-pairs cluster compatibility, deterministic tie order, multiplicity preservation, and no merge.

### 22.9 Aggregate and descriptor tests

Construct known member contribution sets for proper crossing, endpoint crossing, tangent zero, equal/opposite contributions, symbolic delimiter, coplanar boundary, coincident interior, inactive carrier gap, transparent internal diagonal, and topology-separated contact. Verify aggregate reconstruction and the complete descriptor derivation table.

### 22.10 Exact rational differential oracle

For bounded integer-coordinate fixtures, use the in-tree exact rational test oracle to compare:

- event equivalence from relation lineage;
- exact source-edge and carrier parameters;
- exact order and equality clusters;
- overlap interval endpoints/correspondence;
- source-edge interval partitions;
- transverse active spans from exact facet-region intervals;
- exact point containment in published bounds; and
- source-facet triangulation-independent incidence.

The production registry must not link or call the oracle.

### 22.11 Metamorphic tests

Apply:

- source vertex/edge/facet/triangle/shell/component permutations;
- legal alternative source-facet triangulations;
- source-edge canonical direction reversal;
- operand exchange and operation remapping;
- axis permutation;
- sign flip with corrected orientation;
- exactly representable translation;
- power-of-two scaling with precision scaling;
- repeated execution;
- worker counts 1, 2, and maximum;
- forced task delays; and
- reversed private-result merge order.

After documented remapping, semantic event/occurrence keys, IDs, point references, incidence, sequences, carriers, spans, aggregates, descriptors, diagnostics, and digests must be byte-identical.

### 22.12 Fuzzing and shrinking

Generate valid exact-template relation artifacts varying:

- seeds/events/occurrences per edge;
- event valence;
- facet concavity and disjoint carrier intervals;
- coplanar overlap count and orientation;
- coordinate coincidence without identity equivalence;
- equal and nearly equal parameters;
- ULP perturbations;
- shell count/nesting;
- source triangulation;
- inherited precision/tolerance thresholds;
- resource limits; and
- thread partitions.

Every crash, nondeterministic result, invalid merge, missing incidence, unsafe order, unsupported carrier connection, oracle disagreement, or unjustified geometric failure must serialize exact source bits, predecessor keys/records, comparisons, policies, counters, and replay identity and shrink while preserving the failure.

### 22.13 Resource boundary tests

For every separately accounted category, test limit-minus-one, limit, and limit-plus-one. Include dense equal-coordinate occurrences, one edge with many events, one carrier with many active/inactive spans, many overlap records, and high-valence incidence. Below-limit failure must be deterministic; raising the limit must reveal the complete complex, not a differently merged prefix.

### 22.14 Cancellation tests

Inject cancellation at every checkpoint and inside long loops for:

- seed normalization;
- group verification;
- point/witness validation;
- incidence collection/canonicalization;
- active-set ordering;
- all-pairs cluster verification;
- source-edge partitioning;
- carrier grouping/ordering/span activation;
- coplanar arrangement;
- aggregation/descriptors;
- canonicalization/codec; and
- verifier reconstruction.

Confirm all workers join, all reservations return, no partial artifact is visible, and repeated cancellation selects the same progress/failure witness.

### 22.15 Structural performance gates

Instrument and gate, without wall-clock-only assertions:

- seed normalization/grouping uses sort/scan and no quadratic global lookup;
- incidence construction is linear plus canonical sorting in emitted entries;
- bounded ordering compares only active overlapping intervals plus deterministic verification, with comparison count reported;
- disjoint well-separated memberships exhibit near `O(k log k)` behavior;
- genuine dense overlap reports/output-sensitive `O(k^2)` comparison work and respects limits;
- source-edge/carrier tasks allocate no final IDs;
- no production path calls public geometry intersection helpers;
- no production path uses coordinate keys for identity; and
- persistent bytes scale with published events/occurrences/incidence/memberships/spans.

### 22.16 Sanitizers and portability

Run supported debug/ASan/UBSan/TSan configurations, strict warnings, and all `float`/`double` × `uint32_t`/`uint64_t` instantiations. Verify no signed overflow, invalid iterator/reference after vector growth, data race, unjoined worker, unqualified floating contraction, or implementation-defined serialization.

## 23. Implementation sequence and reviewable commits

Implement in this exact dependency order. Each step should be a focused reviewable commit and must keep all previously added tests passing.

1. Add Component 08 versions, strong IDs, closed enums, stage/checkpoint/resource/error registries, and empty artifact/view skeleton.
2. Add complete key schemas, canonical comparators/encoding, operand/direction remaps, and key known-answer tests.
3. Add predecessor validation/preflight/resource accounting and empty-artifact success path.
4. Add seed normalization, forbidden-coordinate-key checks, conceptual-event/occurrence grouping, and interning tests.
5. Add authoritative point references, source-point reuse, witness validation, and coordinate mutation tests.
6. Add incidence proposal schemas, canonicalization, CSR forward/reverse maps, and completeness tests.
7. Add bounded ordering certificates, sweep/active-set provider, all-pairs cluster validation, and adversarial ordering tests.
8. Add source-edge memberships, endpoint sentinels, sequences, clusters, intervals, crossing deltas, and known artifacts.
9. Add transverse carrier grouping/orientation/memberships/order/active spans and no-false-connectivity tests.
10. Add coplanar support, collinear overlap carriers, dual parameters, overlap/region records, and coincident-sheet tests.
11. Add member-preserving aggregates and total descriptor derivation table, including vertex-sector descriptors.
12. Add source-facet/triangle reconciliation and legal-retriangulation metamorphic tests.
13. Add final canonicalization, deterministic partitions, statistics, and serial replay.
14. Add canonical codec/decode, section/complete digests, and corruption tests.
15. Add independent verifier and required mutation suite.
16. Add exact-rational differential hooks/goldens, fuzz/shrink, resource/cancellation, concurrency determinism, and structural performance gates.
17. Integrate the stage into the bounded Boolean pipeline only after Components 01-07 interfaces compile and every Component 08 qualification gate passes.

Do not begin Component 09 implementation in these commits.

## 24. Contract matrix for downstream teams

### 24.1 Component 09 may rely on

- exact source-edge interval boundaries and continuation/cut descriptors;
- exact occurrence separation at point/edge contacts;
- complete crossing deltas and member provenance;
- transparent internal-diagonal descriptors;
- carrier active spans and inactive gaps;
- coplanar/coincident boundary/interior descriptors; and
- canonical queries that require no coordinate-based adjacency inference.

### 24.2 Component 10 may rely on

- preserved symbolic ownership and distinct sheet/occurrence identities;
- member-preserving crossing/contact/coincidence aggregates;
- interval/span retain/split/duplicate/suppress permissions; and
- no loss of zero-measure contact semantics.

### 24.3 Component 11 may rely on

- one shared point reference per conceptual event;
- separate occurrence IDs for topology allocation;
- complete ordered source-edge and carrier clusters;
- relation-supported active spans only;
- overlap endpoint occurrence identities; and
- no unverified or partial sequence.

### 24.4 Components 15-16 may rely on

- complete canonical bytes and digests;
- predecessor and member evidence sufficient for independent reconstruction;
- deterministic structured failures and replay;
- mutation-sensitive forward/reverse maps; and
- exhaustive bounded-oracle hooks that are test-only.

## 25. Prohibited shortcuts checklist

Code review must reject any implementation that:

- uses a coordinate or parameter as an event-map key;
- stores one record per rounded coordinate;
- uses `std::map<vec3<T>,...>` or `unordered_map` hash equality for identity;
- calls `merge_duplicate_vertices`;
- interpolates endpoint events;
- uses ordinary floating sort for semantic order;
- treats unresolved interval overlap as equality without lineage proof;
- relies on a non-strict comparator in `std::sort`;
- connects adjacent ordered carrier events without relation-span activation evidence;
- turns a coplanar relation into an arbitrary line;
- omits endpoint sentinels or conflates them with events;
- stores aggregate sums without members;
- removes zero-net contacts;
- lets internal diagonals own events/cuts;
- mutates published records;
- catches and discards worker failures;
- serializes raw memory/hash values; or
- publishes before independent verification.

## 26. Required documentation

Add developer documentation adjacent to the implementation that explains:

- conceptual event versus occurrence with diagrams for equal-coordinate distinct topology;
- authoritative point ownership and source-endpoint reuse;
- complete event/occurrence key fields and forbidden identity inputs;
- bounded interval sweep, cluster eligibility, and why ordinary sort is invalid;
- source-edge sentinels, clusters, and interval partition semantics;
- transverse carrier support versus active connectivity spans;
- coplanar/collinear overlap and separate source-edge semantics;
- crossing/contact aggregate member retention;
- cut/contact descriptor categories consumed by Component 09;
- deterministic ID/canonical byte rules;
- resource and cancellation behavior; and
- replay and verifier independence.

Documentation examples must use stable symbolic IDs and exact bit/interval displays, not decimal-only approximate diagrams that imply coordinate welding.

## 27. Definition of done

Component 08 is complete only when all of the following are true:

- the V1 provider, conceptual-event/occurrence policy, complete key domains, ordering/cluster/partition/carrier policies, schemas, codec, and verifier versions are frozen and nonzero;
- every Component 07 seed maps to exactly one verified event and one verified occurrence;
- one event-equivalence key has exactly one conceptual event identity;
- every distinct-occurrence key remains distinct through incidence, ordering, clusters, descriptors, codec, and downstream queries;
- every geometric event has exactly one authoritative bounded-point reference shared by all consumers;
- existing source vertex events reuse the accepted source bounded point and are never independently interpolated;
- duplicate construction witnesses are independently validated and contradictions fail without averaging/snapping/widening shortcuts;
- coordinate equality, proximity, bounds overlap, equal nominal parameter, hashes, spatial cells, or tolerance never establish event or occurrence identity;
- complete seed/relation/candidate/source-feature/triangle/halfedge forward and reverse mappings are present and reciprocal;
- every affected source edge has a complete deterministic bounded sequence, endpoint sentinels, explicit clusters, and interval partition;
- every topology-affecting order has a published comparison certificate and unresolved unsafe order causes typed failure;
- exact-equal or lineage-clustered members preserve all distinct occurrences and deterministic tie keys;
- transverse carrier records preserve one canonical support/orientation and connect only relation-supported active spans;
- coplanar/collinear overlap records use original source-feature lineage, preserve dual edge parameters/orientation/ownership, and never weld separate edges or sheets;
- crossing/contact/tangent/coincidence/symbolic aggregates are exactly reconstructible from immutable members;
- cut/contact/occurrence-separation descriptors are complete and sufficient for Component 09 without geometric recomputation;
- internal triangulation diagonals remain bookkeeping-only and legal retriangulation preserves the public semantic intersection complex;
- high-valence and genuine output complexity are preserved or fail deterministically with `resource_limit`, never truncated or proximity-merged;
- cancellation joins all work, releases reservations, and publishes nothing;
- serial and all supported parallel schedules produce byte-identical artifacts, diagnostics, counters, and primary failures;
- canonical decode validates privately and runs the same independent verifier;
- the independent verifier reconstructs event grouping, incidence, ordering, partitions, carriers, aggregates, and descriptors without using producer helpers as sole truth;
- every required mutation is rejected;
- bounded exact-oracle, metamorphic, fuzz/shrink, adversarial, resource, cancellation, sanitizer, portability, and structural performance tests pass;
- no excluded legacy Boolean or external dependency is referenced;
- all production and normative-test code is strict portable C++17 and follows local Ygor style/conventions; and
- `tracker.md` is marked complete for Component 08 only after all preceding gates are satisfied by the plan and eventual implementation.
