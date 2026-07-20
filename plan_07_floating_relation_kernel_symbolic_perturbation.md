# Plan 07: Floating Relation Kernel and Symbolic Perturbation

## 0. Scope and non-negotiable constraints

Implement **only Component 07** from `component_07_floating_relation_kernel_symbolic_perturbation.md`. This component accepts the immutable `canonical_candidate_stream<T,I>` from Component 06 together with the verified source-manifold, source-facet, shell-semantics, precision, and Boolean-context artifacts, evaluates every topology-affecting narrow-phase relation through one fixed compute-once dependency graph, and publishes exactly one immutable `signed_feature_relations<T,I>` artifact for Component 08.

The V1 implementation is fixed by this plan as a deterministic **canonical relation-request graph with staged sort/deduplicate/evaluate/canonicalize publication**. The executable serial implementation is the semantic reference. Parallel work may create only private request and result fragments; canonical merge, relation identity, dependency order, primary failure, bytes, and diagnostics must reproduce the serial reference exactly.

The implementation must:

- preserve exact indexed source-feature identity independently from floating geometry;
- evaluate each canonical support, region, edge-edge, edge-facet, facet-facet, construction, multiplicity, and symbolic question once;
- distinguish definite bounded signs, exact-nominal or exact-lineage ties, unresolved uncertainty, and invalid evidence;
- retain complete triangle-local discovery and coverage evidence while reducing public ownership to original source vertices, source edges, and source facets;
- construct one authoritative bounded point, parameter, or carrier per canonical relation lineage;
- produce operation-neutral numerical relations first and operation-specific symbolic decisions only for eligible ties;
- assign signed crossing multiplicity under one frozen orientation and half-open ownership convention;
- represent tangency, point contact, edge contact, coplanar overlap, coincidence, and coordinate-coincident distinct occurrences explicitly;
- produce canonical event seeds keyed by lineage rather than coordinates; and
- fail closed before publication whenever numerical conditioning, source-facet coverage, symbolic eligibility, multiplicity conservation, resources, cancellation, or independent verification is unresolved.

The component must not:

- call, adapt, copy from, or depend on `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- reread mutable caller meshes or rebuild source topology from coordinate equality;
- call legacy `vec2`/`vec3`, `line`, `line_segment`, `plane`, contour, mesh-intersection, or adaptive-predicate APIs for authoritative decisions;
- call `orient_sign`, `point_on_*`, `segments_intersect_*`, `plane::Intersects_*`, projected point-in-polygon helpers, or any epsilon comparison directly;
- use normalized directions, Euclidean distance, angles, square roots, `atan2`, random rays, arbitrary perturbations, `long double`, fast-math, reassociation, or unqualified floating contraction in authoritative work;
- use caller tolerance as a universal equality, coplanarity, intersection, ownership, or welding epsilon;
- treat an enclosure containing zero as an exact tie without exact-nominal or exact-lineage evidence;
- resolve ordinary uncertainty through symbolic policy;
- declare two source features, relations, constructions, or events equivalent from equal or nearby coordinates, overlapping bounds, equal nominal parameters, hashes, spatial cells, or traversal adjacency;
- allow a facet-internal triangulation diagonal to own an original source contact, symbolic decision, public event, crossing barrier, or retained surface;
- compute a source-edge/source-facet relation independently for each triangle and then choose or average among conflicting answers;
- discard zero-multiplicity contacts, tangencies, or coincident-sheet occurrences merely because they do not change winding;
- emit event IDs, construct Component 08 registry topology, compute global winding, select Boolean surfaces, construct output topology, clean geometry, or publish a public mesh;
- truncate required relations or seeds to satisfy a limit;
- publish a partial relation graph, partial construction table, partial symbolic table, or partial candidate-disposition table after failure, cancellation, or resource exhaustion;
- use exceptions for expected contract, geometry, resource, cancellation, codec, or verification failures;
- serialize raw structs, padding, pointers, `size_t`, implementation-defined enums, unordered iteration, or `std::hash` values; or
- introduce any external, vendored, downloaded, optional, or runtime-invoked dependency.

Use Component 01 for owner tokens, strong IDs, checked count/byte/index arithmetic, stage/checkpoint registration, typed outcomes and errors, resource reservations, cancellation, deterministic failure arbitration, symbolic-policy lookup, diagnostics, replay, canonical bytes, SHA-256, transactions, immutable publication, and execution-policy validation. Use Component 02 as the sole authority for shell orientation, nesting, occupied-side semantics, and source-facet boundary identity. Use Component 03 as the sole authority for bounded arithmetic, exact-nominal sign/tie evidence, finite intervals, residuals, parameters, constructions, conditioning, precision-ledger records, and tolerance disposition. Use Component 04 as the sole authority for source-facet projection/orientation, triangle groups, boundary/internal-diagonal provenance, and semantic versus exact-triangulation lineage. Use Component 05 as the sole authority for canonical vertices, edges, halfedges, triangles, source-facet groups, shell groups, canonical edge direction, incidence, and conservative geometry attachments. Use Component 06 as the sole authority for the complete directed edge/opposite-triangle candidate stream.

No failed, cancelled, partially evaluated, partially encoded, or verifier-rejected artifact may publish. Mark Component 07 complete in `tracker.md` only after every requirement in Section 26 is represented by an implementable instruction and qualification gate.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Reuse existing geometry types only as nominal carriers

`YgorMath.h` provides `vec2<T>`, `vec3<T>`, `line<T>`, `line_segment<T>`, `plane<T>`, and contour utilities. These types are useful for public compatibility, fixture construction, visualization, and storing nominal coordinates, but their geometric operations are not suitable as Component 07 authority because they use raw arithmetic, normalized vectors, square roots, bare booleans, ordinary comparisons, and optional epsilon conventions without inherited precision, operation traces, conditioning, source lineage, typed failure, deterministic replay, or independent proof evidence.

Therefore:

- permit `vec2<T>` and `vec3<T>` only as immutable nominal coordinate carriers inside records whose authoritative enclosures and lineage are supplied by Component 03;
- do not call `vec2`/`vec3` arithmetic operators, `Dot`, `Cross`, `length`, `unit`, `distance`, `angle`, `operator==`, or `operator<` for a relation decision;
- do not call `plane<T>::Get_Signed_Distance_To_Point`, `Intersects_With_Line_Once`, `Intersects_With_Line_Segment_Once`, `Intersects_With_Plane_Along_Line`, or `Project_Onto_Plane_Orthogonally`;
- do not call contour projected point-in-polygon, duplicate removal, simplification, least-squares plane, or epsilon-equality helpers; and
- leave all existing public geometry behavior source-compatible.

### 1.2 Reuse adaptive expansion arithmetic only through Component 03

`YgorMeshesAdaptivePredicates.h/.cc` contains useful in-tree expansion arithmetic and adaptive orientation logic. Component 03 already requires an audited strict-target `ExactFloatExpansion` provider and enclosure-aware `PredicateResults` capability. Component 07 must consume only those Component 03 capabilities.

Do not call legacy adaptive predicate functions directly. A bare exact nominal sign is insufficient because Component 07 must account for inherited coordinate uncertainty, distinguish exact nominal zero from an uncertain interval containing zero, record conditioning and tolerance, preserve the prescribed operation graph, and expose immutable proof records. Any additional determinant or exact-nominal formula needed by this plan must be added as a versioned Component 03 operation/capability with an independently verified enclosure path, not embedded as a second arithmetic subsystem in Component 07.

### 1.3 Reuse and narrowly extend `BoundedSourcePolygonKernel`

Component 04 plans `BoundedSourcePolygonKernel.h/.cc` as a pure, owner-checked adapter over Component 03 projected orientation, segment relation, and point-in-region predicates. Reuse this kernel for source-facet region questions when its existing contract is sufficient. Extend it only with operation-neutral, source-facet-semantic queries required here:

- projected point versus complete source polygon region;
- projected segment versus complete source polygon boundary and interior;
- canonical boundary-feature ownership at vertices and edges;
- deterministic half-open winding/parity contributions for exact boundary ties; and
- bounded interval partitioning of a segment against a simple polygon.

The shared kernel must remain pure and artifact-neutral. It must not allocate relation IDs, apply Boolean symbolic ownership, assign event equivalence, choose crossing multiplicity across a 3D oriented surface, or act as the independent Component 07 verifier. Component 04 regression tests must continue to pass unchanged after any extension.

### 1.4 Existing mesh and Boolean intersection code is not a provider

Legacy Boolean files are explicitly excluded. Generic mesh verification, slicing, tetrahedralization, hole, orientation, refinement, remeshing, Delaunay, and spatial-index code may be studied only for non-authoritative fixtures or benchmark comparisons. Their coordinate-keyed maps, tolerance heuristics, mutable topology, centroid/ray tests, exception behavior, and incomplete lineage do not meet this component's contract.

Do not route relations through a temporary `fv_surface_mesh`, split a mesh geometrically, invoke a generic triangle-triangle intersection helper, or retrofit a legacy Boolean class. Implement the relation engine as new bounded-subsystem code.

### 1.5 Mandatory predecessor reuse

Reuse, rather than duplicate:

- Component 01 symbolic rule matrix, truth table, operand remapping, complete tie-key comparator, strong relation/symbolic-decision IDs, resources, transactions, codec, SHA-256, diagnostics, replay, and deterministic failure arbitration;
- Component 02 source shell/facet orientation, nested-shell occupied-side semantics, canonical source boundary records, and caller provenance;
- Component 03 bounded points, planes, vectors, scalars, parameters, determinants, residuals, interval comparisons, exact-nominal tie evidence, edge-plane/line-line constructions, conditioning, precision ledger, and finite-bound services;
- Component 04 source-facet projection frame, semantic polygon ring, source triangles, boundary/internal-diagonal labels, triangle-local coverage evidence, semantic digest, and exact triangulation digest;
- Component 05 canonical source vertices, source edges, internal diagonals, directed halfedge uses, canonical edge representatives, triangles, vertex fans, source-facet groups, shell groups, and checked immutable query views; and
- Component 06 candidate IDs, complete keys, directed roles, edge/triangle witnesses, canonical partitions, and candidate order.

Do not create a second source-feature registry, shell classifier, source-facet plane, projection-frame chooser, edge-direction policy, precision ledger, symbolic matrix, candidate enumerator, digest provider, or event registry.

### 1.6 Permitted implementation machinery

Use strict portable C++17 standard-library facilities such as fixed-width integers, `std::array`, `std::vector`, `std::optional`, `std::variant`, `std::sort`, `std::lower_bound`, deterministic prefix sums, and sorted-vector grouping. Prefer contiguous immutable arrays and complete-key sort/scan. `std::unordered_*` is unnecessary for the serial reference and must not determine equality, output order, duplicate resolution, diagnostics, or bytes. If used as a private performance cache later, every hit must compare the complete key and publication must still sort canonically.

## 2. Fixed V1 provider and semantic decomposition

### 2.1 Provider identity

Freeze the production provider as:

```text
canonical_source_feature_relation_graph_v1
```

Freeze the dependency-graph policy as:

```text
support_region_edge_edge_edge_facet_facet_symbolic_dag_v1
```

Freeze the public source-feature reduction policy as:

```text
triangle_discovery_source_feature_ownership_v1
```

Changing any relation family, canonical key field, dependency precedence, formula-selection rule, boundary ownership convention, multiplicity convention, symbolic application rule, event-seed equivalence rule, or observable record layout requires an explicit provider/policy/schema version change.

### 2.2 Staged request closure

The serial semantic reference must use this fixed high-level workflow:

1. validate all predecessor artifacts and required capabilities;
2. preflight counts, pair products, dependency limits, construction limits, and storage;
3. scan Component 06 candidates in canonical order and emit private primitive/composite relation requests plus candidate-disposition proposals;
4. sort requests by complete canonical key, group exact duplicates, validate duplicate request compatibility, and assign one private producer slot per key;
5. close dependencies in the fixed family precedence from Section 2.3, adding any derived requests through repeated deterministic sort/group rounds until no new key is introduced;
6. detect duplicate producers, cycles, unsupported request recursion, or closure-work exhaustion before evaluating authoritative arithmetic;
7. evaluate primitive relation families in dependency order, using Component 03 immutable operation traces and constructions;
8. assemble source-feature composite relations from triangle-local and primitive records;
9. compute signed numeric crossing multiplicities and conservation evidence;
10. apply the frozen symbolic matrix only to eligible exact ties and coincident relations;
11. produce event-seed proposals and complete candidate dispositions;
12. canonicalize all public IDs and table ranges after values are final;
13. run producer structural checks, canonical encoding, and the independent verifier; and
14. publish one immutable artifact transactionally.

A request may be discovered from many candidates, triangles, halfedges, or composites. Discovery creates consumers and witnesses, not multiple producers. Arithmetic begins only after the current dependency layer's canonical request set is closed.

### 2.3 Fixed dependency-family precedence

Use this topological family order:

1. imported source bounded points, planes, projection frames, and canonical directions;
2. primitive vertex/facet support residual predicates;
3. primitive projected 2D orientations, segment parameters, and interval comparisons;
4. source-point/source-facet region predicates;
5. canonical source-edge/source-edge relations;
6. canonical source-edge/source-facet relations;
7. source-facet/source-facet support and carrier relations;
8. coplanar source-facet overlay and coincidence relations;
9. composite contact/crossing/tangency classifications;
10. authoritative bounded point, interval, and carrier constructions;
11. signed numeric multiplicity and local conservation reducers;
12. symbolic eligibility records;
13. operation-specific symbolic decision records; and
14. event-seed and candidate-disposition records.

Dependencies may point only to an earlier family, or to an earlier canonical key in a specifically documented same-family acyclic subgraph. Same-family recursion must not be used for convenience. Every relation record stores its ordered dependency range. The graph must be independently serializable and acyclic.

### 2.4 Public versus bookkeeping relations

Public semantic relations are keyed by original source features:

- source vertex/source facet;
- source edge/source edge;
- source edge/source facet;
- source facet/source facet; and
- exact symbolic contact ownership among original source features.

Triangle-local relations and internal-diagonal relations are retained as bookkeeping evidence when needed to prove candidate coverage, region coverage, or a source-facet composite. They have explicit `bookkeeping_only` status and a mandatory map to a public source-feature relation or a documented no-public-relation disposition. They cannot own public event equivalence, source contact, symbolic priority, or crossing barriers.

## 3. Exact file and target layout

Add under `src/YgorMeshesBooleanBounded/`:

- `RelationTypes.h` — stable enums, fixed policy constants, strong-ID aliases, relation/status tags, compact result types, and resource/statistic records;
- `RelationKeys.h/.cc` — complete primitive/composite/symbolic/construction/event-seed keys, total comparators, operand remapping, and known-answer encoding helpers;
- `RelationPreflight.h/.cc` — predecessor validation, exact/worst-case count arithmetic, dependency/work bounds, resource estimates, and request-closure guards;
- `RelationRequestGraph.h/.cc` — private requests, consumer/witness accumulation, deterministic sort/group closure, dependency edges, topological ordering, and cycle detection;
- `PrimitiveRelationKernel.h/.cc` — Component 03 adapters for support residuals, determinants, parameter comparisons, exact-tie evidence, and fixed formula dispatch;
- `SourceFacetRegionKernel.h/.cc` — complete source-polygon point/segment region classification, boundary-feature ownership, interval partitioning, and triangle-coverage reconciliation using `BoundedSourcePolygonKernel`;
- `EdgeEdgeRelations.h/.cc` — canonical 3D source-edge/source-edge classification, point/overlap construction requests, ownership, and residual evidence;
- `EdgeFacetRelations.h/.cc` — complete source-edge/source-facet transverse, tangent, coplanar, and multi-event relation assembly;
- `FacetFacetRelations.h/.cc` — bounded support relation, transverse carrier, orientation relation, coplanar routing, and source-facet pair composites;
- `CoplanarRelationOverlay.h/.cc` — source-feature-driven coplanar boundary overlay, containment witnesses, overlap components, equal-region detection, and coincidence orientation;
- `RelationConstructions.h/.cc` — authoritative point, parameter, overlap interval, and face-face carrier records plus Component 03 conditioning/residual validation;
- `CrossingMultiplicity.h/.cc` — frozen edge traversal sign convention, half-open boundary ownership, local fan/source-facet reduction, and conservation evidence;
- `SymbolicPerturbation.h/.cc` — symbolic eligibility, exact matrix lookup, conceptual ordering, coincident ownership, occurrence separation, and operand-swap validation;
- `RelationCanonicalization.h/.cc` — final table ordering, ID assignment, dependency remap, duplicate-value validation, reverse-consumer tables, and partition construction;
- `RelationEventSeeds.h/.cc` — lineage-equivalence keys, distinct-occurrence keys, incidence aggregation, and canonical seed production for Component 08;
- `SignedFeatureRelations.h` — immutable Component 07 artifact schemas and owner-checked read-only views;
- `RelationQueries.h` — narrow downstream queries for Components 08-10 and diagnostics;
- `RelationBuild.h/.cc` — typed stage entrypoint and exact phase orchestration;
- `RelationCodec.h/.cc` — canonical encoding/decoding, section digests, and replay integration; and
- `RelationVerifier.h/.cc` — independent request reconstruction, dependency verification, relation/seed reconstruction, exact bounded oracle hooks, and mutation rejection.

Extend existing bounded-subsystem registries rather than creating parallel infrastructure:

- `ContractVersions.h` for Component 07 provider, policy, family, key, schema, graph, construction, multiplicity, symbolic, seed, codec, and verifier versions;
- Component 01 stage/checkpoint, strong-ID-domain, error-subcode, resource-kind, diagnostic, replay, and execution registries;
- Component 03 operation/formula/capability registries only for genuinely missing bounded formulas required by Sections 10-14; and
- `BoundedSourcePolygonKernel` formula/version registry only for operation-neutral source-polygon queries added by this plan.

Add under `tests/mesh_boolean_bounded/`:

- `TestRelationKeysGraph.cc`;
- `TestPrimitiveRelations.cc`;
- `TestSourceFacetRegionRelations.cc`;
- `TestEdgeEdgeRelations.cc`;
- `TestEdgeFacetRelations.cc`;
- `TestFacetFacetRelations.cc`;
- `TestCoplanarRelationOverlay.cc`;
- `TestCrossingMultiplicity.cc`;
- `TestSymbolicPerturbation.cc`;
- `TestRelationEventSeeds.cc`;
- `TestRelationKnownArtifacts.cc`;
- `TestRelationExactOracle.cc`;
- `TestRelationComputeOnce.cc`;
- `TestRelationAlternativeTriangulation.cc`;
- `TestRelationCanonicalization.cc`;
- `TestRelationMutation.cc`;
- `TestRelationProperties.cc`;
- `TestRelationAdversarial.cc`;
- `TestRelationResourcesCancellation.cc`;
- `TestRelationStructuralPerformance.cc`;
- `RelationFixtures.h/.cc`;
- `RelationExactOracle.h/.cc`;
- `RelationMutationSupport.h/.cc`; and
- `GoldenRelationsV1.h`.

Register separate CTest cases for graph/keys, primitive predicates, source-facet region, edge-edge, edge-facet, facet-facet/coplanar, multiplicity, symbolic policy, event seeds, known artifacts, exact oracle, compute-once/alternative triangulation, canonicalization/codec, mutation, properties/fuzz, adversarial floating point, resources/cancellation, and structural performance. Apply `ygor_apply_mesh_boolean_strict_fp` to every production and normative-test translation unit. No network discovery or optional test package is permitted.

Keep mutable request proposals, dependency builders, caches, projected workspaces, interval sweep state, sort buffers, task-local constructions, verifier scratch, exact-oracle integers, and mutation helpers out of installed/public headers. Templates must remain header-defined or be explicitly instantiated only for supported `float`/`double` and `uint32_t`/`uint64_t` combinations.

## 4. Stable versions, stages, checkpoints, and failure subcodes

### 4.1 Version registry

Add explicit nonzero V1 constants for:

- relation provider and semantic policy;
- dependency graph and family precedence;
- primitive relation, source-facet region, edge-edge, edge-facet, facet-facet, coplanar overlay, composite contact, and candidate-disposition schemas;
- each complete relation-key family and canonical-order policy;
- request/consumer/dependency graph records;
- bounded construction and construction-selection policy;
- crossing sign, half-open ownership, multiplicity-reduction, and conservation policies;
- symbolic eligibility and symbolic-decision schemas;
- event-seed equivalence and distinct-occurrence policies;
- signed-feature-relations artifact and downstream query schema;
- canonical encoding and section-digest layout; and
- relation verifier and exhaustive-oracle evidence schemas.

Zero is invalid/uninitialized. Unknown required versions, unsupported enum values, mismatched predecessor versions, nonzero reserved bits, or unrecognized formula IDs are typed failures. Encode all versions in the artifact header, each applicable record, canonical bytes, diagnostics, replay, and verifier evidence.

### 4.2 Fixed logical checkpoints

Use the Component 07 stage reserved by Component 01. Define stable checkpoints in this order:

1. context, operation, owner, policy, strict-environment, and capability validation;
2. predecessor artifact/version/digest/disposition validation;
3. candidate/source-feature count and representability preflight;
4. resource/work reservation for request discovery and closure;
5. candidate scan and initial request/disposition proposal generation;
6. request-key sorting, duplicate validation, and first canonical closure;
7. derived dependency request generation and repeated closure;
8. dependency graph finalization and independent acyclicity precheck;
9. primitive support and projected relation evaluation;
10. source-facet region relation evaluation;
11. source-edge/source-edge relation evaluation;
12. source-edge/source-facet composite evaluation;
13. source-facet/source-facet and carrier evaluation;
14. coplanar overlay and coincidence evaluation;
15. bounded construction validation and authoritative producer selection;
16. numeric crossing multiplicity and local conservation reduction;
17. symbolic eligibility and total policy lookup;
18. operation-specific symbolic decision production;
19. candidate disposition reconciliation and event-seed proposal generation;
20. canonical table ordering, ID assignment, dependency/reference remap, and reverse maps;
21. producer invariant and source-facet coverage verification;
22. canonical encoding, section digests, statistics, and replay finalization;
23. independent artifact verification, including exhaustive bounded mode when requested;
24. resource reconciliation and pre-publication cancellation check; and
25. transaction commit.

Do not renumber released checkpoints. Future optional formulas or providers require reserved gaps or a new version.

### 4.3 Required Component 07 failure subcodes

Allocate a disjoint Component 07 range with explicit values for at least:

- unsupported relation provider/policy/schema/formula;
- wrong/stale context owner, operand, role, ID domain, or artifact handle;
- predecessor artifact, version, digest, verification disposition, or semantic digest mismatch;
- malformed candidate key, role, edge, triangle, witness, partition, or provenance reference;
- candidate/source entity/count/byte/index/work overflow;
- relation, dependency, construction, symbolic-decision, interval, overlap-component, seed, or consumer capacity exceeded;
- request key malformed or duplicate request incompatible;
- duplicate authoritative producer;
- missing dependency, forward dependency, unsupported same-family dependency, graph cycle, or closure guard exceeded;
- primitive bounded value/predicate missing, invalid, non-finite, wrong formula, or owner-inconsistent;
- definite sign conflicts with exact-nominal evidence;
- exact tie lacks symbolic-eligibility evidence;
- uncertainty incorrectly marked exact or definite;
- source-facet support, projection, orientation, ring, boundary, or semantic digest inconsistent;
- source-facet region classification unresolved;
- internal diagonal incorrectly assigned source ownership;
- edge-edge relation unsupported, contradictory, or ill-conditioned;
- edge-edge point/overlap parameter, residual, or ownership inconsistent;
- edge-facet endpoint residual pair inconsistent;
- edge-facet construction ill-conditioned or exceeds tolerance;
- edge-facet region coverage missing, duplicated, contradictory, or triangulation-dependent;
- facet-facet support relation uncertain, carrier unavailable, or carrier residual failed;
- coplanarity/coincidence evidence incomplete or contradictory;
- coplanar boundary overlay open, crossing-inconsistent, multiply owned, or coverage-incomplete;
- overlap containment witness unavailable or ambiguous;
- constructed point/parameter/interval/carrier missing, non-finite, out of domain, or multiply authoritative;
- candidate missing disposition, duplicate disposition, or disposition/relation contradiction;
- crossing orientation convention mismatch;
- crossing multiplicity outside supported range, inconsistent across duplicate discovery, or local conservation failed;
- tangent/contact incorrectly given numeric crossing;
- symbolic matrix lookup missing, duplicated, invalid, or operand-swap inconsistent;
- symbolic policy applied to ineligible uncertainty;
- symbolic decision changes nominal geometry;
- coincident ownership incomplete, duplicated, orientation-inconsistent, or truth-table inconsistent;
- event-seed equivalence key malformed or coordinate-derived;
- distinct occurrence improperly merged or canonical event improperly split;
- source-feature incidence, consumer, or reverse map incomplete;
- canonical key collision/ordering/ID/reference error;
- codec tag/length/count/version/reserved/trailing-data error;
- section or complete digest mismatch;
- relation verifier rejection;
- resource reservation/reconciliation failure;
- cancellation; and
- internal construction invariant failure.

Map unresolved conditioning or bounded relation uncertainty to `geometric_condition_exceeds_tolerance`; representability to `index_overflow`; configured accounting exhaustion to `resource_limit`; cancellation to `cancelled`; malformed committed predecessor contradictions and producer/verifier disagreement to `internal_invariant_error`. Every error must include the least canonical relation/candidate/source-feature witnesses, exact nominal bits and intervals, formulas/traces, precision and tolerance evidence, policy versions/rule IDs, resource counters, and deterministic replay identity.

## 5. Public entrypoint and capability boundaries

### 5.1 Typed stage entrypoint

Provide an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<signed_feature_relations<T,I>>
build_signed_feature_relations(
    const boolean_context<T,I>& context,
    const precision_context<T>& precision,
    const validated_operands<T,I>& validated,
    const source_triangle_complexes<T,I>& source_triangles,
    const canonical_source_manifolds<T,I>& manifolds,
    const canonical_candidate_stream<T,I>& candidates);
```

The exact wrapper naming may follow Component 01 conventions. Observable behavior must:

- validate every input before allocating authoritative relation state;
- support either or both operands empty and an empty candidate stream;
- execute all relation families under one transaction;
- use the frozen operation and symbolic matrix from the context;
- select the same primary failure for every allowed traversal, partition, and thread schedule;
- join all private work before rollback;
- expose no partial primitive or composite relation artifact; and
- commit one immutable artifact only after the independent verifier accepts it.

Lower-level test entrypoints may evaluate one primitive relation, one source-feature pair, or one request graph. Ordinary pipeline publication remains all-or-nothing.

### 5.2 Required predecessor views

Consume narrow owner-checked immutable views.

From Component 06 require:

- artifact versions/digest and verified disposition;
- candidate count, canonical iteration, checked random access, and deterministic partition ranges;
- complete candidate key and directed role;
- canonical edge ID/class/representative/endpoints/halfedges/incidences;
- opposite triangle ID/orientation/source facet/shell/provenance;
- overlap witness and precision references; and
- no hidden traversal, lazy discovery, or mutable cache.

From Component 05 require:

- canonical source vertices and bounded-point attachments;
- canonical source edges, internal diagonals, endpoint identity, canonical direction, reciprocal halfedges, incident triangles/facets/shells, and segment bounds;
- oriented source triangles and local edge-use roles;
- source-facet groups with ring, boundary uses, projection/support/orientation, triangle/diagonal membership, semantic and exact-triangulation digests;
- source shell groups with occupied-side semantics;
- vertex fans and source-feature reverse maps; and
- constant-time or documented logarithmic checked lookup by strong ID.

From Component 04 require:

- source-facet semantic ring and projection formula;
- triangle-local source-boundary/internal-diagonal provenance;
- facet triangle coverage evidence and accepted orientation references;
- semantic and exact-triangulation digests; and
- operation-neutral bounded source-polygon kernel capabilities.

From Component 03 require:

- owner-checked bounded point/plane/vector/scalar/parameter/residual views;
- fixed grouped dot, cross, determinant, residual, interpolation, division, interval comparison, projection, and carrier construction services;
- exact-nominal sign/zero evidence for prescribed formulas;
- five-way bounded predicate results and alternate-formula dispositions;
- edge-plane and line/line or equivalent parameter construction services;
- conditioning/tolerance classification and precision-ledger references;
- finite interval hull/intersection under proof, endpoint classification, and conservative residual checks; and
- exact scalar bit/total-order services for canonical diagnostics only.

From Component 02 require shell orientation, nesting, occupied/unoccupied side semantics, source facet/ring identities, and validated boundary provenance. From Component 01 require complete symbolic matrix lookup, truth table, operand exchange transforms, full tie-key comparison, strong-ID publication, resources, cancellation, execution scopes, diagnostics, replay, canonical bytes, SHA-256, and transactions.

Do not expose mutable predecessor arrays, raw ordinals without owner/range validation, provider pointers, or permission to recompute predecessor facts.

### 5.3 Downstream capability

Component 08 receives a `signed_feature_relations_view<T,I>` that supports:

- header/version/predecessor-digest inspection;
- checked canonical iteration and random access by relation, construction, symbolic-decision, and seed ID;
- complete relation keys and family/category lookup;
- dependency and reverse-consumer ranges;
- candidate disposition and candidate-to-relation/seed mappings;
- source-feature and triangle-local provenance;
- bounded construction references, parameters, residuals, conditioning, and precision ledger;
- signed numeric and symbolic crossing contributions;
- symbolic eligibility, rule ID, ownership, occurrence-separation, and operand-remap evidence;
- event-equivalence and distinct-occurrence seed keys;
- source edge/facet/triangle/halfedge incidence expected by the registry; and
- deterministic partitions with no mutation, allocation, or lazy arithmetic.

Component 08 must not receive raw relation inputs plus permission to recompute a coordinate or decide equivalence by geometry.

## 6. Strong IDs, closed enums, and complete keys

### 6.1 Strong ID domains

Use Component 01 `relation_id` and `symbolic_decision_id`. Add distinct Component 07 domains for:

- `relation_request_id` for private/canonical request evidence where a generic task-local ID is insufficient;
- `relation_dependency_edge_id`;
- `relation_consumer_id`;
- `primitive_predicate_record_id`;
- `source_facet_region_record_id`;
- `relation_construction_id`;
- `overlap_interval_id`;
- `coplanar_overlap_component_id`;
- `crossing_contribution_id`;
- `multiplicity_evidence_id`;
- `symbolic_eligibility_id`;
- `event_seed_id`;
- `candidate_disposition_id`; and
- verifier-evidence IDs where generic evidence cannot express the domain safely.

Do not alias these domains to one another, Component 05/06 IDs, `I`, `size_t`, or raw offsets. Dense ordinals are checked storage details only.

### 6.2 Relation families

Define explicit nonzero stable values, including at least:

```cpp
enum class relation_family : std::uint8_t {
    vertex_facet_support = 1,
    point_facet_region = 2,
    edge_edge = 3,
    edge_facet = 4,
    facet_facet_support = 5,
    coplanar_facet_overlay = 6,
    composite_contact = 7,
    crossing_multiplicity = 8,
    symbolic_contact = 9,
    event_seed_lineage = 10,
    triangle_local_bookkeeping = 11
};
```

The exact enum may use separate primitive/composite tags, but every serialized category must be closed, explicit, versioned, and total. Unknown or zero values fail.

### 6.3 Core relation categories

Use closed tagged categories sufficient to distinguish:

- definitely separated;
- proper transverse crossing;
- endpoint crossing;
- vertex-on-facet-interior;
- vertex-on-source-edge;
- vertex-on-source-vertex;
- proper edge-edge point contact;
- endpoint/interior and endpoint/endpoint edge contact;
- tangent point or edge contact with zero numeric crossing;
- parallel separated;
- collinear point contact;
- collinear partial overlap;
- equal geometric edge with same or opposite canonical direction;
- coplanar disjoint facets;
- coplanar point or segment contact;
- partial area overlap;
- region containment;
- equal facet region;
- same-orientation coincidence;
- opposite-orientation coincidence;
- coordinate-coincident distinct sheet occurrence;
- unresolved bounded uncertainty;
- invalid evidence; and
- bookkeeping-only triangle discovery.

Do not overload one enum value with nullable fields whose interpretation depends on context. Prefer tagged payloads with impossible contradictory combinations unrepresentable.

### 6.4 Complete relation keys

A complete public relation key must contain, as applicable:

```text
(
  context semantic owner namespace,
  relation family,
  directed operand role or canonical unordered operand pairing,
  complete source-feature identities,
  directed source-feature use when orientation matters,
  source-facet semantic identity,
  occurrence discriminator when one feature pair has several ordered contacts,
  dependency-graph version,
  relation formula/provider version,
  symbolic-policy version for symbolic records,
  key-schema version
)
```

Public identity must use complete source-feature keys, not only dense IDs. Triangle-local keys additionally contain exact triangle/local-slot/internal-diagonal discovery lineage and the public composite key they feed. Construction keys contain their authoritative producer relation and construction role. Symbolic keys contain the resolved relation key, operation, operand roles, exact symbolic rule key, and complete source-feature tie key. Event-seed keys follow Section 18.

Hashes may accelerate lookup, but equality and ordering compare full keys. IDs are assigned only after final canonical sorting. Provider-local request IDs, candidate discovery order, worker number, task number, pointer, hash bucket, nominal coordinate, uncertainty width, or construction completion order do not enter semantic identity.

### 6.5 Operand exchange

Every key and closed category must define a total operand-exchange transform. For directed edge/facet relations, exchange also maps canonical edge traversal and opposite facet orientation evidence. Union, intersection, and symmetric difference remain the same operation; differences exchange. Tests must prove exchange is involutive and maps numeric multiplicity, symbolic ownership, coincident orientation, seeds, and diagnostics exactly as documented.

## 7. Immutable artifact and table schemas

### 7.1 Artifact header

`signed_feature_relations<T,I>` stores:

- context owner, operation, operand IDs, and ordinary-publication eligibility;
- Component 07 provider/policy/schema/key/graph/codec/verifier versions;
- required Component 01-06 versions, formula sets, artifact IDs, and digests;
- source semantic and exact-triangulation digests for both operands;
- frozen symbolic matrix and truth-table digests;
- strict floating profile and precision-capability references;
- counts/ranges for every table and relation family;
- dependency graph node/edge/reverse-consumer counts;
- candidate counts and disposition totals by role/class/category;
- construction, interval, overlap, multiplicity, symbolic, seed, and incidence counts;
- deterministic partition records;
- resource/statistics and verification-evidence ranges;
- replay reference;
- separate section digests and complete artifact digest; and
- zero reserved fields.

No mutable cache, task-local handle, allocator, callback, caller pointer, or provider workspace may escape.

### 7.2 Primitive predicate record

Each primitive predicate record contains:

- strong ID, complete key, owner, family, and formula/version;
- ordered source-feature and bounded-value inputs;
- Component 03 operation-trace and precision-ledger references;
- nominal scalar bits and finite enclosure;
- one of definitely negative, exact nominal tie, definitely positive, uncertain, or invalid;
- definite separation margin where applicable;
- exact-nominal sign/zero evidence and structural symbolic-eligibility flags;
- alternate-formula attempt/disposition when contractually permitted;
- conditioning and tolerance classification;
- ordered consumers and dependency references; and
- deterministic diagnostic/replay evidence.

Component 07 must not copy only the sign while dropping the immutable Component 03 proof reference.

### 7.3 Source-facet region record

A region record contains:

- queried point or segment lineage;
- source facet/ring/support/projection/orientation identity;
- projected bounded point/segment references;
- complete polygon boundary traversal evidence;
- classification: interior, original source edge, original source vertex, outside, internal diagonal only, coplanar overlap interval/region, unresolved, or invalid;
- every owning original boundary feature at an exact vertex/edge tie;
- triangle-local coverage witnesses and source-facet composite mapping;
- half-open winding/parity evidence where used;
- semantic digest reference independent of triangulation; and
- ordered dependencies/consumers.

A point on an internal triangulation diagonal but not on the original boundary is `interior`, with the diagonal retained only as discovery provenance.

### 7.4 Edge-edge relation record

Each canonical source-edge/source-edge record contains:

- both source edge identities, canonical directions, endpoints, incident halfedges/facets/shells, and bounds;
- support/coplanarity predicates and selected stable projection/formula;
- one tagged relation category;
- for point contact: bounded parameters on both edges, authoritative existing endpoint or constructed point, endpoint/interior ownership, and residual evidence;
- for collinear overlap: bounded closed parameter interval on each edge, canonical endpoint seeds, direction agreement/opposition, source-vertex ownership at endpoints, and nonzero/zero/uncertain interval disposition;
- numeric contact dimension and crossing relevance;
- triangle/candidate discovery consumers, including bookkeeping internal-diagonal witnesses;
- source-feature event-equivalence lineage; and
- symbolic-eligibility references for exact ties.

### 7.5 Edge-facet relation record

Each canonical source-edge/source-facet record contains:

- source edge identity/canonical direction and opposite source facet semantic identity;
- all candidate and triangle-local discovery witnesses contributing to the pair;
- endpoint support predicates;
- facet support and complete region dependencies;
- ordered local event records, possibly zero, one, or several for a concave facet;
- coplanar overlap interval records and boundary ownership;
- side state before/after each event along the canonical edge;
- numeric signed crossing multiplicity per event and total relation contribution;
- tangency/contact/coincidence metadata;
- construction and residual references;
- coverage reconciliation against the full source facet rather than one triangle; and
- canonical occurrence discriminators for distinct ordered events.

### 7.6 Facet-facet and coplanar overlay records

A facet-facet support record contains:

- both source facet semantic identities, supports, orientations, shells, and bounds;
- bounded parallelism/coplanarity predicates;
- same/opposite support orientation;
- stable transverse carrier when definitely nonparallel;
- carrier construction, orientation, parameterization, residual, and conditioning evidence;
- exact coplanar support identity when tied; and
- links to all edge-facet and edge-edge consumers.

A coplanar overlay record additionally contains canonical overlap components, boundary arcs/points, containment/equality witnesses, source boundary ownership, same/opposite oriented sheet relation, and occurrence-separation requirements. It must remain independent of facet triangulation.

### 7.7 Construction record

Every authoritative construction record contains:

- construction ID/key, one producer relation, role, formula, and ordered dependencies;
- either an existing source bounded point reference or a Component 03 constructed bounded point;
- nominal bits and finite axis/radial enclosure;
- bounded parameters on all defining carriers;
- source-feature lineage;
- residuals against every defining edge, support, or carrier;
- conditioning and tolerance disposition;
- precision-ledger reference;
- optional secondary verification witnesses explicitly marked non-authoritative; and
- consumer range.

Two authoritative constructions for one construction key are forbidden. Endpoint contacts reference the accepted source point rather than interpolating parameter zero or one.

### 7.8 Crossing, symbolic, seed, and disposition records

Each crossing contribution stores relation/event-local lineage, canonical edge traversal, opposite occupied-side convention, numeric `-1/0/+1` contribution or a documented larger integral aggregate, boundary ownership, dependencies, and conservation witness.

Each symbolic decision stores exact eligibility reason, relation, operation, rule key/ID, conceptual ordering/side assignment, ownership, symbolic crossing, occurrence separation, operand-swap transform, and explanation code. It stores no changed coordinate.

Each event seed stores Section 18 fields. Each candidate disposition stores one candidate, one stable disposition category, public/bookkeeping relation references, seed references, and all discovery coverage needed for exhaustive reconciliation.

## 8. Count, capacity, dependency, and resource preflight

### 8.1 Exact predecessor counts and worst-case products

Before request discovery, read checked counts for candidates, canonical vertices/edges/triangles/facets/halfedges, facet boundary uses, and source-facet triangle memberships. Validate representability of at least:

- one candidate disposition per Component 06 candidate;
- up to two endpoint/facet support requests per candidate edge/opposite facet;
- all candidate-induced edge/facet composite requests;
- all source-edge/source-edge requests induced by candidate edge versus the three opposite triangle edge uses, after public-source reduction;
- all facet/facet requests induced by edge/facet relations;
- all request consumers and dependency edges;
- all triangle-local coverage records;
- all possible point events and coplanar interval endpoints under configured relation limits;
- all symbolic decisions for eligible relation/operation combinations;
- all event seeds and feature incidences;
- sort workspaces, reverse maps, verifier duplication, canonical bytes, diagnostics, and replay.

Compute conservative upper bounds with Component 01 checked arithmetic. Distinguish representability failure from caller resource limits. Do not allocate a full all-feature Cartesian product when candidate-driven closure can remain output-sensitive, but prove every configured worst-case count before mutation.

### 8.2 Request-closure work guard

The V1 dependency family set is finite. Define an architecture-independent upper bound on the number of unique canonical requests derivable from predecessor entity counts and candidate incidence. Charge every request proposal, full-key comparison, group, dependency insertion, closure round, predicate operation, construction, region boundary test, overlay sweep step, multiplicity reduction, symbolic lookup, seed incidence, and verifier operation to Component 01 abstract work.

Closure terminates when a full round adds zero new keys. If a round adds a key in an already-closed earlier family, reveals a cycle, exceeds the derived maximum unique key count, or exceeds the configured work limit, fail. Do not use an arbitrary uncharged iteration count.

### 8.3 Resource classes

Account separately for:

- request proposals and canonical request records;
- relation records by family;
- dependency edges and reverse-consumer references;
- primitive predicate/evidence records;
- projected source-facet region workspaces;
- edge-edge/edge-facet/facet-facet workspaces;
- coplanar overlay points/arcs/components;
- constructions, parameters, intervals, and residuals;
- crossing contributions and conservation evidence;
- symbolic eligibility/decision records;
- event seeds and incidence references;
- candidate dispositions and mappings;
- canonical sort/merge work;
- task descriptors/private buffers;
- codec/digest/replay/diagnostic bytes;
- verifier and exact-oracle work; and
- persistent artifact bytes.

Reserve fixed predecessor-derived tables and request-discovery work before scanning candidates. After request closure provides exact unique counts, compute exact persistent and evaluation workspace requirements, reserve them transactionally, and only then evaluate authoritative relations. Reconcile actual versus reserved resources before commit. Never lower relation fidelity, omit witnesses, skip verification, or change formula because a limit is tight.

## 9. Predecessor validation and candidate request generation

### 9.1 Cross-artifact validation

Before relation work, validate:

1. context owner, operation, symbolic/truth matrix, strict floating environment, and supported execution mode;
2. Component 02-06 artifact owners, versions, digests, verifier dispositions, and operand roles;
3. candidate stream predecessor digests equal the exact Component 05 artifact supplied;
4. source semantic and exact triangulation digests agree across Components 04-06;
5. every candidate edge/triangle/witness resolves to the same canonical records and precision attachments advertised by Component 06;
6. every source edge/internal diagonal role agrees with Components 04/05 fixed semantics;
7. source-facet support/projection/orientation and shell occupied-side records are complete;
8. all required Component 03 bounded records and formula capabilities are finite, owner-correct, version-valid, and ordinary-publication eligible; and
9. no task-local, stale, cross-owner, or out-of-range ID appears.

A contradiction in a committed predecessor is `internal_invariant_error`; do not choose one artifact as newer or reinterpret geometry.

### 9.2 Initial candidate-derived requests

For each canonical candidate `(edge E, opposite triangle T)`:

- resolve the opposite source facet `F`, its three triangle edge uses, and the candidate edge class;
- request endpoint/facet support predicates for both canonical endpoints of `E` against `F`;
- request the canonical source-edge/source-facet composite `(E_source,F)` when `E` is an original source edge;
- when `E` is an internal diagonal, request only the triangle-local edge/facet bookkeeping relation and the source-facet composite(s) needed to absorb its evidence; it receives no original edge ownership;
- when `E` is an original source edge, request canonical source-edge/source-edge relations only for original opposite triangle boundary edges whose exact relation is needed to classify a boundary event; when `E` is internal, keep edge/edge requests triangle-local until a specific original boundary-owner pair is derived from exact source-facet region lineage;
- retain internal-diagonal/opposite-boundary and internal/internal triangle-local relations only as coverage witnesses where required, never by inventing a public source edge for the diagonal;
- request the source-facet/source-facet support relation for the source facet(s) incident to `E` and `F` whenever transverse carrier or coplanar organization may be needed;
- attach the candidate as a consumer to every emitted request; and
- emit a pending candidate disposition that must later resolve exactly once.

Request generation must use topology and complete provenance, not nominal geometry beyond the candidate's already-authorized broad-phase retention.

### 9.3 Derived request closure

Derived requests include:

- point/facet region tests for an accepted source endpoint or constructed point;
- projected primitive orientations and segment relations required by a region test;
- facet/facet support when an edge/facet pair is not definitely separated;
- edge/edge relations for source boundary ownership encountered during region classification or coplanar partitioning;
- authoritative construction requests after a relation category proves a point/interval/carrier is needed;
- coplanar overlay requests after exact coplanarity is established;
- multiplicity reducers after complete local contacts are known;
- symbolic eligibility/decision requests after exact ties are classified; and
- event-seed requests after all ownership, construction, multiplicity, and occurrence facts are final.

Every derived request records the relation that requested it. The request graph must show why each relation exists even when it has no direct Component 06 candidate consumer.

## 10. Primitive bounded relations and formula dispatch

### 10.1 Primitive support-side evaluation

For a source point `P` and oriented source facet support `F`, request exactly one Component 03 residual evaluation of the frozen unnormalized plane equation. Use the accepted Component 02/04/05 plane record; do not reconstruct or normalize it.

The result is one of:

- definitely on negative side with positive margin;
- exact-nominal or exact-lineage tie;
- definitely on positive side with positive margin;
- unresolved uncertainty; or
- invalid.

Record the occupied/unoccupied interpretation separately from raw sign. A facet's orientation and shell semantics determine which raw side is occupied; Component 07 does not assume “negative is inside” globally.

If the primary interval contains zero, use only the one alternate formula explicitly authorized by Component 03 for that operation and source provenance. Both attempts remain under one primitive record and trace root. If still uncertain and no exact tie evidence applies, fail the dependent semantic relation with `geometric_condition_exceeds_tolerance` rather than invoking symbolic policy.

### 10.2 Projected orientation and interval predicates

Use the frozen source-facet projection frame and Component 03 bounded 2D determinant services. Every projected orientation, segment parameter, endpoint order, and interval comparison has one canonical key based on source/construction lineage and formula. Do not derive a second projected coordinate for the same point/frame pair.

For an exact projected tie, preserve whether it is caused by:

- exact source vertex identity;
- exact point-on-original-edge lineage;
- exact collinearity under the prescribed nominal expression;
- a versioned representational tie; or
- merely a zero nominal result without sufficient structural ownership.

Only the admitted structural categories may feed symbolic or half-open boundary rules.

### 10.3 Definite versus unresolved decisions

A relation may use a definite predicate only when the published enclosure excludes zero. Exact nominal sign cannot override inherited uncertainty. Conversely, an exact nominal zero is not invalid merely because the interval spans both signs; it becomes an exact-tie record whose structural eligibility and tolerance status are explicit. Unsupported uncertainty remains a typed failure.

No caller tolerance comparison may directly replace a sign predicate. Tolerance is consulted only through Component 03 conditioning and ordinary-publication eligibility.

## 11. Complete source-facet region kernel

### 11.1 Point versus source facet

Classify a source or constructed point against a source facet in two independent dimensions:

1. support relation to the accepted oriented plane; and
2. region relation to the complete simple polygon in the frozen projection.

The region provider must scan or index the original source boundary, not triangle interiors as authority. Use a deterministic half-open winding/parity algorithm over projected bounded edges:

- choose the projection already frozen by Component 02/04;
- use the projected Y coordinate as the sweep ordinate and X as the crossing coordinate for V1, with axis names interpreted within that frame;
- treat each boundary edge under one versioned lower-inclusive/upper-exclusive convention based on exact source endpoint identity and projected bounded order;
- use bounded orientation to determine a definite crossing contribution;
- collect exact point-on-edge/vertex ownership before parity reduction;
- permit one explicitly versioned alternate sweep axis inside the same frozen frame when the primary ordinate comparison is unresolved and the alternate is qualified by Component 03; and
- fail if either possible classification would change interior/boundary/outside semantics.

Do not use an arbitrary ray endpoint, random direction, nominal-only crossing, or epsilon.

### 11.2 Boundary ownership

If a point is exactly on an original source edge, record that edge and both directed halfedge uses. If it is exactly on an original source vertex, record the vertex and every incident original boundary edge/facet use required by the relation. Several boundary owners may be valid at one point; preserve the complete canonical set rather than selecting the first.

A triangle edge marked `facet_internal_diagonal` may be recorded as a coverage witness but cannot convert an interior point into source-boundary ownership.

### 11.3 Triangle-local reconciliation

For each triangle-local hit or candidate:

- classify it through the source-facet semantic region provider;
- map it to one source-facet composite relation;
- compare triangle-local barycentric/edge evidence with original boundary ownership;
- coalesce duplicate discovery only through common source-feature/construction lineage;
- preserve truly distinct ordered crossings of a concave facet; and
- prove every triangle candidate has a documented absorption or event contribution.

Legal retriangulation may change triangle-local records and internal-diagonal witnesses, but must not change public source-facet region classification, event seeds, crossing totals, or symbolic ownership.

### 11.4 Segment versus source polygon

For coplanar edge/facet relations, partition the closed source edge parameter domain by all original source-boundary contacts. Use canonical edge-edge relations and bounded parameter ordering to create ordered breakpoints. For each open interval, classify one deterministic certified interior witness against the source polygon. Build the witness through Component 03 from the interval endpoints and a fixed exact rational parameter represented by the bounded construction service; prefer midpoint only when the formula and parameter are certified, and otherwise use a dyadic interior sequence with a versioned bounded work limit.

Produce zero, one, or several inside/on-boundary intervals. Preserve interval endpoint owners, overlap with source edges, exact zero-length contacts, and unresolved ordering. Do not infer interval topology from nominal parameter sorting.

## 12. Canonical source-edge/source-edge relations

### 12.1 Fixed 3D support classification

For canonical directed source edges `A: a0->a1` and `B: b0->b1`, obtain bounded direction vectors from Component 03 and compute once:

```text
u = a1 - a0
v = b1 - b0
w = b0 - a0
n = cross(u, v)
parallel_measure = dot(n, n)
coplanarity_measure = dot(w, n)
```

All expressions use fixed Component 03 grouping/formula IDs. Both source edges are validated nonzero topological edges; nevertheless require a definitely positive bounded direction squared norm. Classify:

- definitely nonparallel when a qualified component/minor of `n` or `parallel_measure` proves nonzero;
- exact parallel tie when exact nominal/lineage evidence proves `n == 0` under the prescribed expression;
- unresolved parallelism when the enclosure admits both and no tie evidence is sufficient; or
- invalid.

For definitely nonparallel edges, require coplanarity support before a 3D point intersection. A definitely nonzero coplanarity measure means skew/separated. An exact coplanar tie routes to the stable 2D solve. Unresolved coplanarity fails if contact semantics could change.

### 12.2 Stable nonparallel parameter solve

Select the 2D coordinate minor by the component of `n` whose squared absolute lower bound is greatest and definitely positive; break equal lower bounds X, then Y, then Z. If no minor is definitely invertible, use the one alternate Component 03 line/line construction formula, then fail if unresolved.

Solve bounded parameters `s` and `t` using the selected fixed 2x2 determinant formula. Require domain classification against `[0,1]` for both parameters. Construct the point once from the endpoint formula selected by Component 03 conditioning policy. Validate residuals against both edge carriers. Classify proper interior/interior, endpoint/interior, endpoint/endpoint, or outside/no-contact.

When a parameter is exactly zero or one with source-identity-compatible residuals, reference that accepted source vertex point. If both constructions nominally coincide but lineage says distinct conceptual occurrences, retain separate occurrence metadata; do not average coordinates.

### 12.3 Parallel and collinear handling

For exact parallel support, test collinearity using bounded cross products/residuals from one canonical endpoint to the opposite edge carrier. Distinguish definite parallel separation, exact collinearity, unresolved uncertainty, and invalid.

For exact collinearity:

- choose a carrier coordinate axis from the direction component with greatest definitely positive squared lower bound, tie X/Y/Z;
- parameterize both closed source edges against canonical edge A through Component 03 bounded division;
- collect both A endpoints and both projected B endpoints as bounded parameters;
- compute closed interval intersection with proof-producing interval operations;
- classify disjoint, point contact, partial overlap, A contained in B, B contained in A, or equal support interval;
- retain orientation agreement/opposition from canonical directions; and
- create canonical endpoint constructions/seeds from accepted source vertices whenever possible.

If projected parameter ordering is unresolved in a way that changes overlap topology, fail. Do not use distance-to-line, normalized directions, or coordinate epsilon.

### 12.4 Source ownership and duplicate discovery

A public edge-edge relation exists only for two original source edges, one from each operand. Relations involving an internal diagonal remain bookkeeping. Candidate discoveries through several opposite triangles or both incident facets attach as consumers to the same edge-edge producer. Verify all duplicate consumers agree on point/interval lineage, category, parameters, and ownership.

## 13. Canonical source-edge/source-facet relations

### 13.1 Endpoint support pair

For source edge `E: p0->p1` and opposite source facet `F`, consume the two canonical endpoint/facet support predicates. Let the ordered states be interpreted using `F`'s occupied/unoccupied side.

Classify the support pattern before region testing:

- both endpoints definitely on the same side: no transverse crossing, while boundary/tangent contact still requires exact endpoint or coplanar evidence;
- endpoints definitely on opposite sides: one transverse support crossing candidate;
- one endpoint exact tie and the other definite: endpoint contact/crossing candidate;
- both endpoints exact ties: coplanar edge/facet handling;
- any unresolved state: use only the prescribed alternate predicate path, then fail if semantic uncertainty remains; and
- invalid evidence: typed failure.

### 13.2 Transverse construction and region classification

For an opposite-side pair, request the Component 03 edge-plane construction using the already computed residuals. Use its deterministic endpoint formula selection. Require:

- finite parameter and point enclosure;
- stable interior or exact endpoint parameter classification;
- residual acceptance against edge carrier and facet support;
- aggregate precision within ordinary tolerance; and
- one authoritative construction producer.

Then classify the constructed point against the complete source facet region. A support crossing outside the polygon is no contact. Interior gives a proper face crossing. Original edge/vertex boundary gives a boundary crossing/contact requiring half-open multiplicity ownership. Internal diagonal only remains facet interior.

### 13.3 Endpoint and tangent contacts

When an edge endpoint is exactly on the facet support, reuse the source bounded point and classify it against the source facet region. Determine the local side state immediately after/before the endpoint from the other endpoint support plus incident source-edge/source-facet relations. Record endpoint contact, tangency, or half-open crossing without fabricating a parameter beyond `[0,1]`.

A tangent relation has numeric multiplicity zero unless exact local fan evidence and the frozen half-open convention assign one incident occurrence the unique crossing contribution. Symbolic conceptual crossing remains separate from numeric multiplicity.

### 13.4 Coplanar complete relation

When both endpoint support predicates are exact coplanar ties with eligible common-support evidence, invoke the segment/source-polygon partition from Section 11.4. Produce:

- isolated point contacts;
- one or more boundary overlap intervals;
- one or more interior overlap intervals for concave facets;
- complete containment of the edge in the facet region;
- source vertex/edge ownership at every breakpoint; and
- zero numeric transverse crossing for intervals.

The relation must reference all canonical edge-edge boundary relations used to partition it. Triangle-local coplanar hits are consumers/witnesses only.

### 13.5 Composite occurrence order

Order local events along the canonical directed source edge using bounded parameter comparisons. Definitely separated intervals establish strict order. Exact equal parameters form a canonical cluster ordered by complete source-feature/occurrence tie key while retaining equality. Overlapping unresolved intervals may be clustered only when every allowed order yields identical source-edge partition and crossing sequence; otherwise fail.

Assign an occurrence discriminator only after this canonical order is proven. Concave facets may have several distinct events or intervals. Never collapse them by coordinate equality.

## 14. Source-facet/source-facet support and coplanar overlay

### 14.1 Support relation

For source facets `FA` and `FB`, use their accepted unnormalized bounded planes. Compute a bounded cross product of normals and exact-nominal tie evidence once.

Classify:

- definitely nonparallel supports;
- exact parallel supports;
- unresolved parallelism; or
- invalid.

For definitely nonparallel supports, request one bounded carrier construction. Select carrier orientation from the oriented cross product, then canonicalize sign using complete facet keys: the lower complete facet key defines the first normal in the cross product; operand exchange applies the documented remap. Store a point/direction or equivalent bounded line representation without normalizing. Validate residuals against both supports and conditioning within tolerance.

For exact parallel supports, evaluate one accepted anchor from each facet against the opposite support. Distinguish definitely separated parallel planes, exact coplanarity, unresolved support offset, and invalid. Approximate normal/offset similarity is insufficient.

### 14.2 Orientation relation

For exact coplanarity, determine same versus opposite oriented supports from bounded normal dot/cross evidence and accepted source orientation. If orientation is unresolved, fail. Preserve shell occupied-side semantics separately; same normal direction does not alone imply identical occupied side when shell orientation provenance differs.

### 14.3 Coplanar boundary overlay

Build coplanar overlap from original source boundaries and exact source-feature relations:

1. gather every canonical cross-operand source-edge/source-edge point or overlap relation for the facet pair;
2. gather source vertices of each facet classified against the other complete source polygon;
3. partition original boundary edges by canonical event parameters;
4. label each open boundary interval as outside, on boundary, or inside the opposite facet using certified interval witnesses;
5. assemble canonical boundary arcs/points into overlap components using exact source boundary incidence and event lineage;
6. classify area overlap, segment/point contact, containment, or equality;
7. verify every overlap boundary is covered once by authorized source-feature arcs with orientation; and
8. preserve coordinate-coincident but topologically distinct sheet occurrences.

Do not triangulate the overlap region as authority. Triangle coverage may independently witness area overlap but cannot define public boundary ownership.

### 14.4 Containment and equality witnesses

If no boundary crossing exists, classify containment using one deterministic certified interior witness per source facet. Prefer a predecessor-published accepted interior witness when Component 02/04 provides one. Otherwise derive it from the least canonical source triangle using a Component 03 bounded barycentric construction with exact rational weights `(1/3,1/3,1/3)` represented through the qualified operation graph, and verify it is strictly inside the complete source polygon and on the support.

If the barycentric witness cannot be certified because its enclosure touches the boundary, try the finite versioned dyadic interior sequence associated with the same least triangle. Exhaustion is a typed geometric failure; do not use a nominal centroid or random point.

Facet regions are equal only when both boundaries are completely covered with matching canonical overlap arcs and each facet has no outside interval/area witness. Count equality or equal nominal area is insufficient.

### 14.5 Transverse carrier membership

For nonparallel facet pairs, all edge-facet transverse events between the pair must reference the same carrier. Project event constructions to one bounded carrier parameter through Component 03. Component 07 records expected carrier membership and parameter; Component 08 owns final interning and ordering. Contradictory residuals or incompatible carrier parameters prevent publication.

## 15. Authoritative construction policy

### 15.1 One producer

Every construction key has one producer relation chosen by fixed precedence:

1. accepted source vertex point;
2. canonical source-edge/source-edge point relation;
3. canonical source-edge/source-facet point relation;
4. canonical coplanar overlap endpoint relation;
5. canonical facet-facet carrier construction; and
6. test-only verification witness.

When several relations describe one conceptual point, relation lineage must explicitly name the authoritative producer and make others consumers/verification witnesses. Do not choose the smallest enclosure, earliest candidate, or first completed task.

### 15.2 Secondary witness compatibility

For every secondary witness required by policy, verify:

- the authoritative nominal point lies within the witness enclosure when the witness claims the same real occurrence;
- carrier parameters and endpoint ownership are compatible;
- residuals reference the same source supports/carriers;
- the precision/conditioning discrepancy is explainable by recorded operation graphs; and
- no witness implies a different source-feature occurrence.

Do not widen all witnesses into a union to conceal contradiction. A contradiction is a producer/predecessor invariant failure or a precise geometric-condition failure.

### 15.3 Finite/tolerance gate

No construction enters a published relation unless all nominal values and enclosure endpoints are finite, every required denominator is definitely separated from zero or routed to an exact tie case, residuals meet the Component 03 contract, and aggregate precision remains within tolerance for ordinary publication. Symbolic policy cannot rescue an unrepresentable or over-budget construction.

## 16. Signed crossing multiplicity and local conservation

### 16.1 Frozen sign convention

For every event on a canonical directed source edge:

- traverse from canonical parameter `0` to `1` as defined by Component 05's representative direction;
- interpret the opposite source facet's two support sides through its validated occupied/unoccupied semantics;
- numeric `+1` means opposite-operand winding after the event is one greater than before;
- numeric `-1` means it is one smaller;
- numeric `0` means no net crossing.

A proper interior crossing obtains sign directly from the definite endpoint occupied-side states. Reversing canonical edge direction negates numeric multiplicity under the documented key remap. Reversing a shell while consistently remapping occupied-side semantics produces the corresponding transformed result.

### 16.2 Boundary crossing ownership

For events on an opposite source edge or vertex, several incident facets may report the same geometric crossing. Use one versioned **source-fan half-open ownership rule**, not triangle discovery order.

The V1 rule must:

1. gather all opposite original source-facet uses incident to the owning source edge/vertex through Component 05 topology;
2. evaluate their local support transitions against the traversing canonical edge using already computed relations;
3. exclude internal diagonals from ownership;
4. order exact tie candidates by the Component 01 symbolic half-open rule key, including operand role, source-feature priority, incident directed use, and requested transition orientation;
5. assign the numeric crossing to exactly the canonical incident source-facet occurrence representing the actual occupied/unoccupied transition;
6. assign zero to duplicate consumers and tangential fan contacts; and
7. store the full fan contribution list and sum.

The total numeric contribution for one ordinary closed two-manifold source boundary crossing is `-1`, `0`, or `+1`. Coincident multi-sheet records may store a wider signed integer aggregate only when each constituent occurrence is separately listed and the sum is checked for overflow.

### 16.3 Tangency and conservation

A local fan that enters and leaves the same side without crossing has total zero. At every source vertex/edge event verify:

- the sum is independent of incident triangle order and legal facet triangulation;
- duplicate triangle discoveries do not alter the sum;
- every nonzero contribution has one owner;
- a tangent or zero-measure contact has zero numeric total unless the documented half-open traversal represents a true boundary crossing;
- reversing traversal negates all constituent contributions; and
- local closed-fan side states are consistent.

A symbolic contribution, if any, is stored separately and must not overwrite numeric conservation evidence.

## 17. Symbolic perturbation and coincident ownership

### 17.1 Eligibility

Create a symbolic eligibility record only when the numerical relation is tied under one of these stable reasons:

- exact nominal zero under the qualified operation graph plus a supported structural relation category;
- exact shared source endpoint identity;
- exact collinear source-edge support;
- exact coplanar source-facet support;
- exact equal source-feature lineage;
- versioned representational tie explicitly admitted by Component 01/03; or
- versioned coincident-source contract with equivalent exact evidence.

Record whether inherited uncertainty also admits separated realizations and whether tolerance remains sufficient. Ineligible uncertainty, over-wide intervals, near-parallel ill-conditioning, contradictory duplicates, unrepresentable bounds, and resource failures never invoke symbolic policy.

### 17.2 Total matrix lookup

For every eligible relation construct the exact `symbolic_rule_key` required by Component 01, including operation, acting operand, relation family, orientation relation, source-feature ownership role, half-open endpoint/edge role, and transition orientation. Lookup must return exactly one rule. No default branch or local fallback is permitted.

Produce a symbolic decision containing the rule ID, complete tie key, conceptual ordering/side, owner, symbolic crossing, retained/discard consequence, occurrence separation, explanation code, and operand-exchange transform. Verify the rule value itself against the frozen matrix digest.

### 17.3 Lower-dimensional contact semantics

For point- and edge-touching regular solids:

- preserve explicit contact records and distinct topology occurrences;
- do not create positive-volume connectivity merely because coordinates coincide;
- use symbolic ordering only to resolve classification/ownership needed by later stages;
- retain numeric crossing zero when the operation-neutral geometry is tangent/contact only; and
- duplicate occurrence requirements must ensure Components 08-11 cannot weld separate sheets.

### 17.4 Coincident facets and equal regions

Resolve coincident area ownership atomically from the two operation-side truth values, not by applying independent local offsets that create a fictitious finite slab.

For each coincident overlap component:

1. identify the two conceptual geometric sides of the common support;
2. derive operand A and B occupancy on each side from validated shell orientation and relation orientation;
3. evaluate the frozen Boolean truth table on both sides;
4. if truth values are equal, the coincident component does not bound the result and every coincident surface occurrence is suppressed from retained-boundary ownership, while contact provenance remains recorded;
5. if truth values differ, exactly one canonical source-facet occurrence owns the retained coincident boundary under the symbolic matrix, and its output orientation must point from result-occupied to result-empty according to the broader output convention;
6. same-orientation and opposite-orientation cases use distinct matrix rules;
7. equal operands for every operation must match the reviewed regularized expected result; and
8. coordinate-coincident distinct sheets not proven to be one coincident component remain separate occurrences.

The decision changes classification and ownership only. Nominal coordinates, bounded points, support planes, and construction records remain bit-identical.

### 17.5 Operand exchange and determinism

For every symbolic decision, compute and store the expected exchanged rule key/value or a compact verifier reference. Exchanging operands and remapping the operation must produce the documented exchanged owner, conceptual side, crossing, occurrence key, and diagnostics. No rule may consult traversal order, triangle order, worker, pointer, hash bucket, or unversioned implementation detail.

## 18. Event-seed production

### 18.1 Seed classes

Emit canonical seeds for at least:

- existing source vertex/facet contact or crossing;
- constructed edge/facet point;
- edge/edge point;
- coplanar overlap interval endpoint;
- tangent contact point;
- shared source-feature event discovered by several triangles;
- symbolic tie occurrence without coordinate movement; and
- distinct coincident-sheet occurrence.

An overlap interval or area component references endpoint seeds and interval/component records; it is not forced into one point event.

### 18.2 Equivalence key

The event-equivalence key contains:

```text
(
  seed class,
  authoritative producer relation/construction lineage,
  canonical source-feature owners on both operands,
  accepted source vertex identity when reused,
  transverse/coplanar carrier role,
  contact dimension/category,
  symbolic rule identity when it changes occurrence semantics,
  event-equivalence policy version
)
```

Triangle IDs/internal diagonals may appear only in discovery provenance after reduction to authoritative source-feature lineage. Coordinates, bounds, nominal parameters, spatial keys, and hashes do not enter equivalence.

### 18.3 Distinct occurrence key

Add a distinct-occurrence key whenever one coordinate/construction can represent separate topological or conceptual occurrences. The key contains the source sheet/facet use, contact-side role, symbolic occurrence rank, and canonical local occurrence discriminator. Seeds with different distinct-occurrence keys must remain separate even when all nominal bits, bounds, source point references, and carrier parameters are equal.

### 18.4 Complete incidence

Each seed records:

- authoritative relation and construction/source point;
- all source vertices, original source edges, source facets, shells, and operands that own the event;
- triangle/internal-diagonal candidate discovery provenance;
- every source triangle and oriented halfedge use expected to consume it;
- contact dimension and relation category;
- numeric and symbolic crossing contributions;
- candidate IDs/dispositions;
- expected source-edge and facet-facet carrier memberships with bounded parameters;
- event-equivalence and distinct-occurrence keys; and
- precision/residual/conditioning references.

Sort incidence by complete feature/use key and reject missing, duplicate, contradictory, or coordinate-inferred entries.

## 19. Candidate disposition and coverage reconciliation

Every Component 06 candidate receives exactly one final disposition:

- definitely separated by a canonical relation;
- duplicate triangle-level discovery absorbed into a canonical source-feature relation;
- primitive dependency consumer only;
- contributed one or more event seeds;
- contributed a coplanar/coincident relation or overlap interval;
- retained as explicit zero-measure contact;
- bookkeeping internal-diagonal witness absorbed into source-facet coverage; or
- caused the canonical typed failure.

A success artifact contains no pending or failed disposition. For each candidate verify its edge/triangle witness appears in all required consumer maps. For each source-edge/source-facet composite verify the union of triangle-local discoveries covers every admitted candidate and that no duplicate seed is unexplained. Missing coverage, contradictory local signs, incompatible constructions, or a candidate mapped to two incompatible public relations prevents publication.

Empty candidate streams publish a canonical empty relation graph with zero candidates/dispositions/relations/seeds, valid versions/digests, and no invented containment relations; Components 09 and later handle global containment without boundary contact.

## 20. Deterministic request evaluation and canonical publication

### 20.1 Serial semantic reference

Implement serial request generation, closure, evaluation, composite assembly, multiplicity, symbolic decisions, seed production, and canonicalization first. It is executable in all normative tests and remains the semantic oracle for Component 17.

Within each family, evaluate canonical request keys in sorted order. Component 03 operation traces may allocate private task-local records, but final relation IDs and ledger references are assigned through canonical merge. A failure key includes family and complete relation key so the same primary error is chosen independent of execution schedule.

### 20.2 Parallel-ready boundaries

Parallel execution may partition immutable canonical request ranges by family. Each task writes private results and consumer/dependency fragments. It must not:

- insert into a shared semantic cache whose winner is timing-dependent;
- allocate final relation/construction/symbolic/seed IDs;
- commit shared precision-ledger order directly;
- select a symbolic rule from local traversal context; or
- publish a partial family.

Canonical merge sorts complete keys, validates duplicate values byte-for-byte or through one documented semantic comparison, assigns IDs, resolves dependency references, and replays Component 03 ledger fragments in canonical order. Forced delays, reversed task completion, and all supported worker counts must reproduce serial bytes and primary failure.

### 20.3 Final canonical order

Assign IDs after final validation in this order:

1. primitive predicates by complete key;
2. source-facet region records;
3. edge-edge relations;
4. edge-facet relations;
5. facet-facet support records;
6. coplanar overlay components;
7. composite contact relations;
8. constructions and intervals by producer/key;
9. crossing contributions/evidence;
10. symbolic eligibility and decisions;
11. event seeds; and
12. candidate dispositions remain in Component 06 candidate order with canonical relation ranges.

Dependency edges store final IDs only after remap. Every relation's dependencies precede it under the graph order. Reverse-consumer ranges are sorted by full consumer key.

## 21. Producer invariant checks

Before encoding, reconstruct and verify at least:

- every canonical key is unique and tables are strictly ordered;
- every request has one producer and all duplicate requests are consumers;
- the dependency graph is complete, acyclic, and family-order valid;
- every primitive predicate category agrees with its Component 03 enclosure and exact-tie evidence;
- exact ties and unresolved uncertainty are never conflated;
- source-facet region records use original polygon boundaries and semantic digests;
- internal diagonals own no public contact, symbolic decision, crossing barrier, or seed;
- every construction has one producer, finite bounds, valid parameters/residuals, and precision eligibility;
- every source-feature composite accounts for all triangle-local consumers;
- every candidate has one disposition;
- crossing contributions obey the frozen orientation, sum correctly, and pass local fan conservation;
- symbolic decisions are eligible, total, matrix-backed, operation-aware, and coordinate-preserving;
- coincident ownership matches truth values on both geometric sides;
- event seeds use lineage equivalence and preserve distinct occurrences;
- operand-exchange mappings are complete;
- counts/ranges/resources match exact records; and
- no task-local/private handle remains.

Producer checks supplement but do not replace the independent verifier.

## 22. Canonical encoding, diagnostics, replay, and decode

### 22.1 Canonical encoding

Use Component 01 `CanonicalBytes`. Encode explicit framed sections for:

- artifact header, versions, operation, fixed policies, and predecessor identities/digests;
- relation family/key tables;
- primitive predicates and Component 03 trace references;
- request/dependency/reverse-consumer graph;
- region, edge-edge, edge-facet, facet-facet, coplanar, and composite records;
- exact nominal bits, finite interval endpoints, parameter/residual/conditioning references;
- construction and overlap interval records;
- crossing contributions and conservation evidence;
- symbolic eligibility/decision records and matrix references;
- event seeds, incidences, and occurrence keys;
- candidate dispositions/mappings;
- partitions, statistics, resources, verifier evidence, and replay references.

Never serialize raw object memory, padding, pointers, `size_t`, container capacity, locale-dependent text, compiler type names, or hash values as identity. Exact `T` bits use Component 03 canonical bit policy, preserving nominal signed zero.

Compute separate SHA-256 digests for keys/graph, primitive predicates, source-feature relations, constructions/intervals, multiplicity, symbolic decisions, seeds/dispositions, evidence/statistics, and the complete artifact. The complete digest covers all required versions and predecessor digests.

### 22.2 Diagnostics

Failures and retained findings include, within policy limits:

- component/stage/checkpoint/subcode;
- complete candidate/relation/source-feature keys and roles;
- exact nominal bits/enclosures/margins;
- predicate/construction formula and operation-trace IDs;
- support/region/parameter/residual/conditioning evidence;
- source semantic and exact-triangulation lineage;
- crossing contributors and symbolic eligibility/rule IDs;
- resource limits/counters and cancellation progress; and
- deterministic replay identity.

Core relation code does not log. Diagnostic rendering occurs at an API boundary from completed structured records.

### 22.3 Replay

Replay must reproduce request discovery and closure, canonical keys, graph order, primitive operation traces, exact categories, source-facet region classifications, edge/edge and edge/facet constructions, facet/facet carrier/overlay, multiplicities, symbolic decisions, seeds, dispositions, IDs, bytes, digests, counters, and primary failure under the same supported platform profile. Worker count and schedule are non-semantic.

### 22.4 Decode

Decode privately and fail closed. Validate tags, lengths, versions, counts, resource limits, owners, predecessor digests, reserved fields, and exact bit patterns before allocation or publication. Reconstruct keys, graph, relations, constructions, symbolic decisions, seeds, and dispositions; recompute all section/complete digests; then run the same independent verifier. Reject trailing bytes, unknown required fields, duplicate singleton sections, or task-local IDs.

## 23. Independent verifier

### 23.1 Independence requirements

`RelationVerifier` consumes immutable Component 07 records plus narrow predecessor and Component 03 capabilities. It must not call the producer's request-closure engine, relation cache, source-facet composite assembler, multiplicity reducer, symbolic dispatcher, seed deduplicator, or stored summary booleans as its sole truth.

It may share stable schemas, complete-key comparators, Component 01 checked arithmetic/canonical bytes/SHA-256, Component 03 public bounded operations, and the pure `BoundedSourcePolygonKernel` primitives, but must use independent traversal and dispatch control flow.

### 23.2 Required reconstruction

The verifier must:

1. rescan every Component 06 candidate and independently reconstruct the required initial request keys;
2. independently derive the finite V1 dependency closure and compare complete request/dependency sets;
3. verify topological order and detect cycles without trusting producer ordinals;
4. re-evaluate or reconstruct every primitive bounded predicate from Component 03 records and inputs;
5. independently classify source-point/facet regions from original source boundaries;
6. independently reconstruct edge-edge and edge-facet categories, parameters, ownership, and source-facet coverage;
7. independently reconstruct facet-facet support, carriers, coplanar boundary overlays, containment/equality, and orientation;
8. verify every authoritative construction and secondary witness residual/conditioning relation;
9. independently sum local fan crossing contributions and check traversal reversal;
10. reconstruct symbolic eligibility and perform exact Component 01 matrix lookup for every decision;
11. recompute coincident truth values on both sides and retained owner/orientation;
12. reconstruct event-equivalence/distinct-occurrence keys and complete incidences;
13. compare exact candidate disposition coverage;
14. validate all owners, ranges, versions, reserved fields, resources, and canonical ordering; and
15. re-encode semantic records and compare all digests.

For bounded fixtures under `exhaustive_test_only`, compare the complete public source-feature relation set with an independent in-tree exact rational oracle and exhaustive source-feature enumeration. Production code never depends on that oracle.

### 23.3 Mutation rejection

Required mutations include deleting/duplicating/reordering a request, producer, dependency, consumer, candidate disposition, relation, construction, crossing contribution, symbolic decision, or seed; introducing a cycle; changing a primitive sign/tie/interval/margin/formula; assigning source ownership to an internal diagonal; changing source-facet semantic digest; splitting or merging triangle discoveries incorrectly; shrinking a construction enclosure; changing parameter/domain/residual/conditioning; flipping crossing sign or zero/nonzero status; changing a half-open owner; marking uncertainty symbolically eligible; changing a rule ID/owner/conceptual side; changing nominal coordinate under symbolic policy; merging seeds by coordinate; splitting one lineage event; changing distinct occurrence; corrupting incidence, ordering, versions, resources, bytes, reserved fields, or digest. Every mutation must be rejected deterministically.

## 24. Tests and qualification

### 24.1 Primitive and region known-answer tests

Cover every five-way predicate category for support residual, projected orientation, segment relation, parameter comparison, interval order, parallelism, coplanarity, and residual checks. Include signed zero, subnormals, adjacent representable values, cancellation, extreme finite exponents, large translation with small local features, and denominator intervals containing zero.

Commit source-facet region known answers for convex/concave facets, interior, exterior, original edge, original vertex, internal diagonal only, collinear boundary intervals, multiple disjoint inside intervals, and exact half-open vertex crossings. Verify invariance under ring rotation, source permutation, and legal retriangulation.

### 24.2 Relation matrix fixtures

Commit complete golden artifacts for:

- definite separation;
- proper transverse edge/facet crossing;
- endpoint on facet interior;
- crossing through original source edge and source vertex;
- proper edge-edge crossing;
- endpoint/interior and endpoint/endpoint edge contact;
- tangent vertex/facet and edge/facet contacts;
- skew and parallel separated edges;
- collinear point contact, partial overlap, containment, and equal edges;
- coplanar disjoint facets;
- coplanar point/edge contact;
- partial area overlap;
- one facet contained in another;
- equal facets with same and opposite orientation;
- multiple topologically distinct coincident sheets;
- several incident triangles discovering one event;
- a concave facet with several edge crossings/overlap intervals;
- distinct conceptual events with equal nominal coordinates; and
- empty candidate streams.

Goldens include complete keys, dependencies, categories, construction bits/enclosures, parameters, residuals, multiplicities, symbolic rule IDs, seeds, dispositions, and digests.

### 24.3 Symbolic matrix qualification

Exhaustively enumerate every valid Component 01 symbolic rule key for all five operations and both operand roles. Test equal operands; point-, edge-, and face-touching solids; vertex-on-vertex/edge/face; edge-on-face; equal and overlapping edges; same/opposite coincident facets; half-open vertex/edge ownership; and operand exchange. Prove matrix totality, unique lookup, coordinate bit preservation, correct coincident truth-side ownership, and expected regularized outputs.

Unknown or missing categories fail during context qualification or relation evaluation; they never fall through.

### 24.4 Exact rational differential oracle

Implement test-only arbitrary-precision signed integer and rational arithmetic in-tree using C++17 standard library containers, as required by Component 16's later shared infrastructure. For bounded integer-coordinate fixtures compare exact:

- orientation/support signs;
- edge-edge and edge-facet categories;
- intersection parameters and exact point containment in published bounds;
- source-polygon region ownership;
- coplanarity/orientation and overlap topology;
- crossing multiplicity and local conservation; and
- event/source-feature incidence.

The exact oracle must not share producer formula dispatch or relation grouping. Production targets must not link it.

### 24.5 Compute-once and dependency tests

Instrument requests and verify:

- repeated vertex/facet support consumers create one producer;
- repeated triangle candidates of one source facet share one composite;
- edge-edge ownership is reduced to original source edges;
- every construction has one authoritative producer;
- every dependency points to an earlier valid record;
- no duplicate producer exists;
- an injected second result with changed nominal/enclosure/category is rejected;
- candidate permutations and partitioning preserve IDs/bytes; and
- legal retriangulation changes only bookkeeping consumers, not public relations/seeds.

### 24.6 Crossing conservation tests

Construct high-valence source vertices and shared edges where several facets discover one crossing. Verify unique half-open ownership, tangent total zero, entering/leaving opposite signs, traversal reversal, shell-orientation remap, closed-fan conservation, alternative triangulation invariance, and exact-oracle agreement.

### 24.7 Conditioning boundaries

For edge-plane, line-line, facet-carrier, projected region, and parameter-order constructions, test comfortably conditioned, just inside tolerance, exactly at threshold, just outside, exact coplanarity with zero denominator, zero-containing denominator without tie evidence, large translation/small geometry, subnormal scales, and maximum finite values. Category, chosen formula, bounds, residuals, and failure must be deterministic. Symbolic policy must not rescue ineligible uncertainty.

### 24.8 Metamorphic and determinism tests

Apply operand exchange with operation remapping; source vertex/edge/facet/shell/component permutations; facet-ring rotation; legal source subdivision and retriangulation; axis permutation; sign flip with corrected orientation; exactly representable translation; power-of-two scaling with precision scaling; canonical edge-direction reversal; repeated execution; thread counts 1, 2, and maximum; forced task delays; and reversed merge order.

After documented remapping, public relation keys, categories, constructions, bounds, dependencies, multiplicities, symbolic decisions, seeds, dispositions, diagnostics, and digests must be byte-identical for a fixed provider/policy.

### 24.9 Fuzzing and shrinking

Generate valid manifold operand pairs from exact templates varying feature valence, facet concavity/triangulation, shell count/nesting, overlap dimension, coplanar orientation, coordinate duplication without identity merge, ULP perturbations, near-parallel angles, translation, scale, inherited precision, tolerance, and symbolic categories. Compare bounded fixtures with exhaustive candidate/source-feature and exact-rational oracles.

Every crash, nondeterministic result, oracle disagreement, invalid tie eligibility, conflicting duplicate relation, incorrect multiplicity, source-facet triangulation dependence, or bad seed equivalence serializes exact source bits, predecessor digests, candidate stream, policy versions, relation traces, and replay. Shrink while preserving the same canonical failure category and witness.

### 24.10 Resource, cancellation, and concurrency tests

For every resource class in Section 8, test limit-minus-one, limit, and limit-plus-one. Confirm exact primary limit witness, no truncation, no partial artifact, and complete lease reconciliation.

Cancel during request generation, sort/group closure, each relation family, construction, coplanar overlay, multiplicity, symbolic lookup, seed generation, canonical merge, codec, and verifier. Confirm all workers join, reservations return, no relation artifact is visible, and retry produces canonical bytes.

### 24.11 Structural performance gates

Use deterministic counters rather than wall time alone. Require:

- candidate scan linear in candidate count plus emitted request proposals;
- request grouping within documented `O(R log R)` comparison growth;
- one evaluation per unique relation key;
- source-facet region work proportional to queried polygon boundary size or the versioned deterministic conservative index candidate count;
- edge/facet and coplanar work output-sensitive to actual boundary contacts, with explicit worst-case quadratic guards;
- no ordinary sparse fixture evaluating all cross-operand feature pairs;
- memory proportional to requests, dependencies, relations, constructions, consumers, seeds, and verifier evidence; and
- exhaustive pair/source-feature work only in test mode or documented worst-case overlap.

Counter ceilings are architecture-independent checked values. Regression updates require reviewed justification, not silent threshold inflation.

## 25. Implementation sequence and handoff gates

Implement in this exact dependency order:

1. register all Component 07 versions, enums, IDs, checkpoints, subcodes, resource kinds, and strict-target files;
2. define complete keys, tagged schemas, comparators, operand remaps, and golden key encodings;
3. define immutable artifact/query views and predecessor capability adapters;
4. implement predecessor validation and count/resource preflight;
5. implement serial initial request discovery and candidate-disposition proposals;
6. implement deterministic request sort/group, duplicate validation, dependency closure, and cycle detection;
7. expose any missing Component 03 bounded formulas through reviewed capability extensions and tests;
8. extend the pure source-polygon kernel and prove Component 04 regressions;
9. implement primitive support/projected predicate records;
10. implement complete point/segment source-facet region classification;
11. implement canonical edge-edge relations and constructions;
12. implement edge-facet transverse/endpoint/tangent/coplanar composites;
13. implement facet-facet support and stable carrier construction;
14. implement coplanar boundary overlay, containment, equality, and occurrence components;
15. implement authoritative construction selection and witness compatibility;
16. implement numeric crossing multiplicity, half-open fan ownership, and conservation checks;
17. implement symbolic eligibility, total matrix lookup, lower-dimensional contact, and atomic coincident ownership;
18. implement event seeds, incidence, distinct occurrences, and candidate disposition reconciliation;
19. implement final canonical ID assignment, dependency remap, reverse maps, partitions, and producer invariant checks;
20. implement canonical codec, section digests, diagnostics, and replay;
21. implement independent request/graph/relation/construction/multiplicity/symbolic/seed verifier control flow;
22. implement bounded exhaustive source-feature enumeration and exact-rational test oracle;
23. add all known-answer, symbolic matrix, compute-once, alternative-triangulation, mutation, metamorphic, adversarial, resource, cancellation, fuzz/shrink, and performance suites;
24. expose private parallel task boundaries and prove serial equivalence under Component 17 schedules;
25. run all supported scalar/index configurations and strict floating profiles;
26. verify Component 08 can consume every seed/construction/incidence without recomputing geometry or using coordinates for equivalence; and
27. update `tracker.md` only after every Definition of Done item below is fully represented and reviewed.

Do not begin Component 08 registry implementation, add an external predicate library, weaken uncertainty handling, or introduce an alternative relation provider while completing V1.

## 26. Definition of done

Component 07 is complete only when all of the following are true:

- the V1 canonical relation-request graph, family precedence, complete key domains, formula selection, and public/bookkeeping reduction policies are frozen and versioned;
- all Component 01-06 owners, versions, digests, capabilities, and predecessor invariants are validated before authoritative relation work;
- every Component 06 candidate receives exactly one verified final disposition;
- every topology-affecting primitive/composite question has exactly one canonical producer and immutable value;
- all duplicate requests are represented as consumers and conflicting duplicate results are rejected;
- the dependency graph is complete, acyclic, canonically ordered, serializable, and independently reconstructed;
- all authoritative arithmetic comes through Component 03 bounded services under the strict C++17 floating contract;
- exact ties, representational ties, unresolved uncertainty, definite signs, and invalid evidence remain distinct in every record and diagnostic;
- symbolic policy is applied only to explicitly eligible exact ties or coincident-lineage cases and never to ordinary uncertainty, conditioning failure, resource exhaustion, or unrepresentable bounds;
- source-facet support and complete polygon-region semantics are independent of facet-internal triangulation;
- internal diagonals remain bookkeeping-only and never own public contact, symbolic priority, crossing barriers, event equivalence, or retained surfaces;
- canonical edge-edge relations cover proper, endpoint, parallel, collinear, overlap, equal-edge, uncertain, and invalid cases with bounded parameters and residuals;
- canonical edge-facet relations cover transverse, endpoint, tangent, coplanar, multi-crossing concave, overlap, containment, and no-contact cases over the full source facet;
- facet-facet support relations distinguish stable transverse carrier, parallel separation, exact coplanarity, orientation, uncertainty, and invalid construction;
- coplanar overlays use original source boundaries, preserve all overlap components and distinct sheets, and prove containment/equality without coordinate proximity or area equality shortcuts;
- every required point, parameter, interval, and carrier has one authoritative finite bounded construction with complete precision, residual, conditioning, tolerance, and lineage evidence;
- every nonzero numeric crossing has the frozen orientation and signed integer multiplicity;
- half-open ownership at shared source edges/vertices is deterministic, source-feature based, triangulation-independent, and locally conservative;
- tangencies and zero-measure contacts remain explicit even when numeric multiplicity is zero;
- the Component 01 symbolic matrix is total for every supported relation/operation/operand role and every lookup is recorded by stable rule ID;
- coincident facet ownership is derived atomically from truth values on both geometric sides, preserves regularized semantics, and never changes nominal coordinates;
- operand exchange with operation remapping produces the documented remapped numeric relations, multiplicities, symbolic decisions, seeds, diagnostics, and digests;
- every event seed is keyed by exact relation/source-feature lineage, not coordinates, bounds, nominal parameters, hashes, or spatial cells;
- distinct conceptual/topological occurrences remain separate even when nominal coordinates and enclosures are identical;
- Component 08 can intern and order events using the published seeds/constructions/parameters without repeating authoritative geometric computation;
- producer invariant checks reconstruct candidate coverage, source-facet coverage, dependencies, constructions, multiplicity, symbolic eligibility, incidence, ranges, and resources rather than trusting summary flags;
- the independent verifier reconstructs the request graph and semantic relation set through separate control flow and rejects every required mutation;
- exact-rational differential tests, crossing conservation tests, symbolic matrix tests, legal-retriangulation tests, conditioning-boundary tests, metamorphic tests, deterministic replay tests, fuzz/shrink tests, resource/cancellation tests, and structural performance gates all pass;
- serial and every supported deterministic-parallel schedule produce byte-identical artifacts and the same canonical primary failure;
- no failed, cancelled, over-limit, uncertain, partially encoded, or verifier-rejected artifact publishes;
- all canonical bytes, section digests, complete digest, diagnostics, and replay are deterministic and versioned; and
- all production and normative-test code is strict portable C++17, self-contained within Ygor, and uses no external dependency.
