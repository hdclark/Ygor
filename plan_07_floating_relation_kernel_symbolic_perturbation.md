# Plan 07: Floating Relation Kernel and Symbolic Perturbation

## 0. Scope, review status, and non-negotiable constraints

Implement **only Component 07** from `component_07_floating_relation_kernel_symbolic_perturbation.md`. The stage accepts the immutable `canonical_candidate_stream<T,I>` from Component 06 together with the verified source-manifold, source-facet, shell-semantics, precision, and Boolean-context artifacts, evaluates every topology-affecting narrow-phase relation through one fixed compute-once dependency graph, and publishes exactly one immutable `signed_feature_relations<T,I>` artifact for Components 08-10.

The V1 implementation is fixed as a deterministic **canonical relation-request graph with staged sort/deduplicate/close/evaluate/canonicalize publication**. The executable serial implementation is the semantic reference. Parallel work may create only private request and result fragments; canonical merge, relation identity, dependency order, primary failure, bytes, and diagnostics must reproduce the serial reference exactly.

The implementation must:

- preserve exact indexed source-feature identity independently from floating geometry;
- preserve the four predicate record dimensions required by Component 03: rounded nominal bits, bounded-sign status, exact stored-coordinate relation status, and consumer disposition;
- evaluate each canonical support, region, edge-edge, edge-facet, facet-facet, construction, multiplicity, and symbolic question once;
- retain complete triangle-local discovery and coverage evidence while reducing public ownership to original source vertices, source edges, and source facets;
- construct one authoritative bounded point, parameter, interval, or carrier per canonical relation lineage;
- produce operation-neutral numerical relations first and operation-specific symbolic relation evidence only for eligible exact ties;
- assign signed numeric crossing multiplicity under one frozen orientation and source-fan half-open convention;
- represent tangency, point contact, edge contact, coplanar overlap, coincidence, and coordinate-coincident distinct occurrences explicitly;
- publish symbolic side/order, half-open ownership, owner-rank eligibility, and occurrence-separation constraints without performing final Component 10 Boolean selection;
- produce canonical event seeds keyed by lineage rather than coordinates; and
- fail closed before publication whenever numerical conditioning, source-facet coverage, symbolic eligibility, multiplicity conservation, resources, cancellation, or independent verification remains unresolved.

The implementation must not:

- call, adapt, copy from, or depend on `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- reread mutable caller meshes or rebuild source topology from coordinate equality;
- call legacy `vec2`/`vec3`, `line`, `line_segment`, `plane`, contour, mesh-intersection, or adaptive-predicate APIs for authoritative decisions;
- call `orient_sign`, `point_on_*`, `segments_intersect_*`, `plane::Intersects_*`, projected point-in-polygon helpers, or epsilon comparisons directly;
- use normalized directions, Euclidean distance, angles, square roots, `atan2`, random rays, arbitrary perturbations, `long double`, fast-math, reassociation, or unqualified floating contraction in authoritative work;
- use caller tolerance as a universal equality, coplanarity, intersection, ownership, or welding epsilon;
- treat a rounded nominal zero as exact relation evidence;
- treat an uncertainty enclosure containing zero as an exact tie without exact-relation or exact-lineage evidence;
- resolve ordinary uncertainty through symbolic policy;
- declare two source features, relations, constructions, or events equivalent from equal or nearby coordinates, overlapping bounds, equal nominal parameters, hashes, spatial cells, or traversal adjacency;
- allow a facet-internal triangulation diagonal to own an original source contact, symbolic priority, public event, crossing barrier, or retained surface;
- compute a source-edge/source-facet relation independently for each triangle and choose or average conflicting answers;
- discard zero-multiplicity contacts, tangencies, or coincident-sheet occurrences merely because they do not change numeric winding;
- call the final Boolean truth-table selection path, decide final retain/discard/suppress status for positive-area atoms, or prescribe final output orientation;
- emit final event IDs, construct Component 08 registry topology, compute global winding, select Boolean surfaces, construct output topology, clean geometry, or publish a public mesh;
- place raw runtime owner-token values in semantic keys, canonical order, canonical bytes, digests, replay-equivalence records, or deterministic failure ordering;
- truncate required relations or seeds to satisfy a limit;
- publish a partial relation graph, construction table, symbolic table, or candidate-disposition table after failure, cancellation, or resource exhaustion;
- use exceptions for expected contract, geometry, resource, cancellation, codec, or verification failures;
- serialize raw structs, padding, pointers, `size_t`, implementation-defined enums, unordered iteration, or `std::hash` values; or
- introduce any external, vendored, downloaded, optional, subprocess, or runtime-invoked dependency.

Use Component 01 for runtime owner validation, strong IDs, checked count/byte/index arithmetic, stage/checkpoint registration, typed outcomes and errors, resource reservations, cancellation, deterministic failure arbitration, symbolic-policy lookup, diagnostics, replay, canonical bytes, SHA-256, transactions, immutable publication, and execution-policy validation. Use Component 02 as the sole authority for shell orientation, nesting, occupied-side semantics, source-facet boundary identity, and coherent geometry basis. Use Component 03 as the sole authority for bounded arithmetic, exact stored-coordinate relations, finite intervals, residuals, parameters, constructions, conditioning, precision-ledger records, and tolerance disposition. Use Component 04 as the sole authority for source-facet projection/orientation, semantic polygon rings, triangle groups, boundary/internal-diagonal provenance, and semantic versus exact-triangulation lineage. Use Component 05 as the sole authority for canonical vertices, edges, halfedges, triangles, source-facet groups, shell groups, canonical edge direction, incidence, and conservative geometry attachments. Use Component 06 as the sole authority for the complete directed canonical-edge/opposite-triangle candidate stream under its frozen V1 candidate-domain policy.

`tracker.md` records completion of this **planning and independent-review step**, not future implementation completion. Mark Component 07 complete after the reviewed specification and this concrete plan are mutually consistent with the broad plan and Components 01-10, 15-17. Section 26 remains the future implementation definition of done.

## 1. Independent review conclusions and required corrections

The prior specification and plan had a strong compute-once, source-lineage, bounded-construction, multiplicity, symbolic-policy, testing, and verification foundation. This review identified four material integration defects and one clarification needed for existing-code reuse.

### 1.1 Preserve Component 03's three numerical truth layers

The prior plan repeatedly used “exact nominal zero” or “exact nominal tie.” Component 03 now explicitly separates:

1. `rounded_nominal`: the exact bits produced by a prescribed rounded `T` operation graph;
2. `exact_relation`: the exact sign or zero of a separately versioned algebraic relation evaluated over stored nominal bits; and
3. `uncertainty_enclosure`: every admitted realization after inherited precision and operation error.

These are not interchangeable. V1 must expose them orthogonally, together with a fourth consumer-disposition field. In particular:

- rounded nominal zero is diagnostic only;
- exact expansion arithmetic proves `exact_relation`, not the rounded graph and not every realization in the uncertainty enclosure;
- exact relation zero may coexist with an enclosure admitting both signs;
- exact relation zero is not automatically symbolically eligible;
- structural relation category, exact lineage or admitted representational-tie evidence, and tolerance compatibility are also required; and
- uncertainty that could change contact dimension or crossing topology without an admitted tie contract fails rather than receiving an arbitrary symbolic answer.

All schemas, formula registries, diagnostics, codecs, verifiers, tests, and downstream queries must use this terminology. Any missing exact algebraic relation is added to Component 03's exact-relation capability; Component 07 must not emulate it with rounded arithmetic or call legacy adaptive predicates directly.

### 1.2 Keep final Boolean selection in Component 10

The prior plan's coincident-facet section evaluated the Boolean truth table and declared a retained or suppressed coincident boundary with output orientation. That duplicates and preempts Component 10, whose reviewed contract requires one truth-table evaluation per positive-area atom and joint coincident-sheet selection after Component 09 has produced complete side labels.

Correct V1 boundary:

- Component 07 establishes operation-neutral overlap/coincidence geometry, source-side occupancy provenance, orientation relation, and exact occurrence grouping;
- Component 07 performs relation-specific symbolic matrix lookup and publishes conceptual sheet ordering, symbolic side assignments, source-fan half-open ownership, owner rank or eligibility, symbolic crossing metadata, and occurrence-separation constraints;
- Component 09 uses Component 07/08 evidence to construct classification atoms and negative/positive side labels;
- Component 10 applies the Component 01 truth table to complete atom side tuples, jointly resolves coincident sheet cells, chooses final retained owner or suppression, and prescribes final output orientation.

No Component 07 record may claim final positive-area retain/discard/suppress/cancel status or output orientation. Tests must enforce the boundary by instrumenting the Component 01 truth-table selection service and by rejecting a mutated Component 07 artifact containing final selection fields.

### 1.3 Remove runtime owner tokens from semantic identity

Components 01, 04, 05, and 06 distinguish runtime owner validation from deterministic semantic identity. The prior Component 07 plan included a “context semantic owner namespace” in complete relation keys and ambiguously included owner in artifact bytes.

Correct V1 rule:

- checked handles, views, and artifact wrappers carry or reference runtime owner tokens;
- every dereference validates runtime ownership;
- semantic keys use the stable context digest or an equivalent owner-free namespace, stable operand roles, complete source semantic keys, and versioned policy/formula fields;
- raw owner-token values are excluded from semantic IDs, ordering, canonical bytes, digests, replay-equivalence, canonical diagnostics, and primary-failure ordering; and
- semantically identical invocations under different runtime owner anchors produce identical Component 07 semantic artifacts, while stale or cross-owner handles still fail before use.

### 1.4 Tracker completion is a planning gate

The prior plan tied the tracker checkbox to future implementation and qualification. The existing tracker instead records completion of each component's specification/plan review.

Mark Component 07 checked when this reviewed specification and plan are committed and consistent. Future implementation acceptance remains Section 26 and executable evidence.

### 1.5 Reuse existing Ygor functionality through qualified boundaries

The existing-code audit remains valid with one refinement:

- `YgorMeshesAdaptivePredicates` contains useful in-tree expansion primitives and orientation methods, but its public API returns bare values/signs, assumes exact coordinates, lacks inherited uncertainty/provenance/resources/replay, and is compiled in the ordinary target. Component 03 already requires extracting or hardening a strict dependency-free expansion core. Component 07 consumes only Component 03 exact-relation and bounded-predicate capabilities.
- Existing `vec2`, `vec3`, `line`, `line_segment`, `plane`, contour, triangulation, and generic intersection methods are useful for compatibility, fixtures, visualization, or non-normative comparison, but not authoritative relations.
- `BoundedSourcePolygonKernel`, established by Components 02 and 04 as a pure Component 03 adapter, is the required reuse point for complete source-polygon region queries. It may be narrowly extended with operation-neutral point/segment region, boundary ownership, half-open parity, and interval-partition primitives.
- Legacy Boolean files, temporary public meshes, coordinate-keyed maps, generic mesh cleanup, spatial indexes, and generic ray/centroid tests remain unsuitable providers.

Before adding any low-level formula, the implementer must document whether Component 03 or `BoundedSourcePolygonKernel` already supplies it. Duplicate arithmetic or polygon semantics are prohibited.

## 2. Existing Ygor assessment and mandatory reuse decisions

### 2.1 Reuse geometry types only as nominal carriers

`YgorMath.h` provides `vec2<T>`, `vec3<T>`, `line<T>`, `line_segment<T>`, `plane<T>`, and contour utilities. Preserve their public behavior. For Component 07:

- permit `vec2<T>` and `vec3<T>` only as immutable nominal payloads inside records whose authoritative enclosure and lineage come from Component 03;
- do not call vector arithmetic operators, `Dot`, `Cross`, `length`, `unit`, `distance`, `angle`, `operator==`, or `operator<` for relation decisions;
- do not call `plane<T>::Get_Signed_Distance_To_Point`, line/segment/plane intersection methods, plane-plane intersection, or projection methods;
- do not call contour point-in-polygon, duplicate removal, simplification, least-squares plane, or epsilon-equality helpers; and
- do not modify existing public math semantics to carry Boolean-specific owner, policy, or replay state.

### 2.2 Reuse adaptive expansion arithmetic only through Component 03

`src/YgorMeshesAdaptivePredicates.h/.cc` contains expansion primitives and robust `orient3d`/`insphere` logic. It is not directly sufficient because it:

- returns a bare `T` or sign;
- does not propagate inherited uncertainty;
- does not separate rounded nominal, exact relation, and uncertainty enclosure;
- lacks owner, source lineage, formula versions, conditioning, resource accounting, typed failure, canonical bytes, or replay;
- uses fixed implementation buffers and ordinary-target compilation; and
- is not a proof that a relation is symbolically eligible under the Boolean contact policy.

Component 03's reviewed plan extracts or wraps an audited strict-target expansion core and publishes exact-relation records with separate bounded-enclosure evidence. Component 07 must consume those capabilities only. Any new determinant, cross-product zero relation, collinearity numerator, coplanarity numerator, or parameter endpoint numerator required below is registered and implemented in Component 03, with capacity/resource guards and independent tests.

Do not copy expansion code into Component 07, call the legacy object, or interpret a returned zero from a rounded public predicate as eligibility.

### 2.3 Reuse and narrowly extend `BoundedSourcePolygonKernel`

Components 02 and 04 establish `BoundedSourcePolygonKernel.h/.cc` as a pure owner-checked adapter over Component 03 projected orientation, segment relation, local-cone, point-in-region, and interval services.

Reuse it for:

- projected point versus the complete semantic source polygon;
- projected segment versus original source boundary and interior;
- complete original boundary-feature ownership at vertices and edges;
- deterministic lower-inclusive/upper-exclusive parity or winding contributions for exact boundary ties;
- certified interval witnesses; and
- bounded interval partitioning of a segment against a simple polygon.

The shared kernel must remain operation-neutral and artifact-neutral. It must not allocate Component 07 IDs, apply Boolean symbolic ownership, assign event equivalence, choose 3D crossing multiplicity, call final truth-table selection, or act as the Component 07 independent verifier. Component 02/04 regression tests must pass unchanged after extensions.

### 2.4 Existing mesh, Boolean, and intersection code is not a provider

Do not route relations through a temporary `fv_surface_mesh`, split meshes geometrically, invoke generic triangle-triangle intersection helpers, or retrofit a legacy Boolean class. Existing Boolean, BSP, slicing, tetrahedralization, Delaunay, remeshing, hole, orientation, verification, and mesh-intersection code may be studied only for fixtures or non-normative benchmark comparisons.

Their coordinate-keyed identity, tolerance heuristics, mutable topology, normalized vectors, centroid/ray methods, exception behavior, incomplete lineage, and missing bounded verification do not satisfy this contract.

### 2.5 Mandatory predecessor reuse

Reuse, rather than duplicate:

- Component 01 truth table, symbolic matrix, operand remapping, complete tie-key comparator, strong relation/symbolic IDs, owner validation, resources, transactions, codec, SHA-256, diagnostics, replay, and deterministic failure arbitration;
- Component 02 coherent geometry basis, source shell/facet orientation, nested-shell occupied-side semantics, canonical source boundary records, and caller provenance;
- Component 03 bounded points, planes, vectors, scalars, parameters, determinants, residuals, interval comparisons, exact stored-coordinate relation evidence, edge-plane/line-line/carrier constructions, conditioning, precision ledger, and finite-bound services;
- Component 04 source-facet projection frame, semantic polygon ring, source triangles, boundary/internal-diagonal labels, coverage evidence, semantic digest, and exact-triangulation digest;
- Component 05 canonical source vertices, source edges, internal diagonals, directed halfedge uses, canonical edge representatives, triangles, vertex fans, source-facet groups, shell groups, and checked immutable query views; and
- Component 06 candidate IDs, complete semantic keys, directed roles, canonical edge/triangle witnesses, partitions, and canonical order.

Do not create a second source-feature registry, shell classifier, source-facet plane, coherent realization, projection chooser, edge-direction policy, precision ledger, exact-relation engine, symbolic matrix, candidate enumerator, digest provider, event registry, or selection truth-table evaluator.

### 2.6 Permitted implementation machinery

Use strict portable C++17 standard-library facilities: fixed-width integers, `std::array`, `std::vector`, `std::optional`, `std::variant`, `std::sort`, `std::lower_bound`, deterministic prefix sums, and sorted-vector grouping.

Prefer contiguous immutable arrays, checked CSR ranges, and complete-key sort/scan. `std::unordered_*` is unnecessary for the serial reference and must not determine equality, output order, duplicate resolution, diagnostics, or bytes. A future private performance cache must compare complete keys and publish through canonical sorting.

## 3. Fixed V1 provider and semantic decomposition

### 3.1 Provider identities

Freeze the production provider as:

```text
canonical_source_feature_relation_graph_v1
```

Freeze the dependency-graph policy as:

```text
support_region_edge_edge_edge_facet_facet_symbolic_dag_v1
```

Freeze the public reduction policy as:

```text
triangle_discovery_source_feature_ownership_v1
```

Freeze the numerical truth policy as:

```text
rounded_exact_relation_uncertainty_orthogonal_v1
```

Freeze the symbolic downstream boundary as:

```text
relation_side_rank_evidence_no_final_selection_v1
```

Changing relation families, complete key fields, dependency precedence, formula selection, boundary ownership, multiplicity, symbolic eligibility, event equivalence, semantic owner exclusion, or downstream selection boundary requires an explicit provider/policy/schema version change.

### 3.2 Staged request closure

The serial semantic reference uses this workflow:

1. validate context, owner, operation, policies, strict floating profile, predecessor artifacts, and required capabilities;
2. preflight counts, products, dependency limits, construction limits, overlay limits, bytes, and abstract work;
3. scan Component 06 candidates in canonical order and emit private primitive/composite requests plus pending candidate-disposition proposals;
4. sort requests by complete owner-free semantic key, group exact duplicates, validate request compatibility, and assign one private producer slot per key;
5. close dependencies in the fixed family order, adding derived requests through deterministic sort/group rounds until no new key appears;
6. prove the finite closure bound and reject duplicate producers, cycles, unsupported recursion, or work exhaustion before authoritative arithmetic;
7. evaluate primitive bounded and exact-relation records in dependency order;
8. assemble source-feature composites from triangle-local and primitive records;
9. compute numeric crossing multiplicities and conservation evidence;
10. construct symbolic eligibility from exact-relation/lineage plus structural and tolerance evidence;
11. look up relation-specific symbolic rules and produce side/order/rank/occurrence evidence only;
12. produce event-seed proposals and complete candidate dispositions;
13. canonicalize all public IDs, ranges, trace fragments, and reverse maps after values are final;
14. run producer checks, canonical encoding, and the independently controlled verifier; and
15. publish one immutable artifact transactionally.

A request may be discovered from many candidates, triangles, halfedges, or composites. Discovery creates consumers and witnesses, not multiple producers. Arithmetic begins only after the applicable dependency layer is closed.

### 3.3 Fixed dependency-family precedence

Use this topological family order:

1. imported source bounded points, planes, projection frames, and canonical directions;
2. rounded bounded support residual and projected primitive operations;
3. exact stored-coordinate relation records for prescribed algebraic formulas;
4. source-point/source-facet complete region predicates;
5. canonical original-source-edge/source-edge relations;
6. canonical source-edge/source-facet relations;
7. source-facet/source-facet support and carrier relations;
8. coplanar source-facet overlay and coincidence relations;
9. composite contact/crossing/tangency classifications;
10. authoritative bounded point, interval, parameter, and carrier constructions;
11. signed numeric multiplicity and local conservation reducers;
12. symbolic eligibility records;
13. operation-specific symbolic relation decisions;
14. event-seed records; and
15. candidate-disposition records.

Dependencies point only to an earlier family or to an explicitly documented earlier key in a same-family acyclic subgraph. Same-family recursion is prohibited as an implementation convenience. Every public relation stores its ordered dependency range.

### 3.4 Public versus bookkeeping relations

Public semantic relations are keyed by original source features:

- source vertex/source facet;
- source edge/source edge;
- source edge/source facet;
- source facet/source facet; and
- symbolic contact among original source features or sheet occurrences.

Triangle-local and internal-diagonal relations remain bookkeeping evidence needed for candidate coverage, region coverage, or source-facet composite reconciliation. They carry `bookkeeping_only` status and a mandatory map to a public relation or a documented no-public-relation disposition. They cannot own public event equivalence, source contact, symbolic priority, crossing barriers, or selected surfaces.

The Component 06 V1 candidate policy includes internal diagonals. Component 07 must consume them and prove they are semantically transparent. A future candidate policy that omits them requires its own Component 06 qualification and does not change Component 07 source-feature ownership.

## 4. Exact file and target layout

Add under `src/YgorMeshesBooleanBounded/`:

- `RelationTypes.h` — closed enums, policy constants, strong-ID aliases, numerical truth-layer types, relation/status tags, and resource/statistic records;
- `RelationKeys.h/.cc` — owner-free complete primitive/composite/symbolic/construction/seed keys, total comparators, operand remapping, and known-answer encoding helpers;
- `RelationPreflight.h/.cc` — predecessor validation, checked count/byte/work bounds, capability checks, and closure guards;
- `RelationRequestGraph.h/.cc` — private requests, consumers/witnesses, deterministic sort/group closure, dependency edges, family order, and cycle detection;
- `PrimitiveRelationKernel.h/.cc` — narrow Component 03 adapters for bounded operations, exact-relation requests, orthogonal truth records, and fixed formula dispatch;
- `SourceFacetRegionKernel.h/.cc` — complete source-polygon point/segment classification, boundary ownership, interval partitioning, and triangle-coverage reconciliation using `BoundedSourcePolygonKernel`;
- `EdgeEdgeRelations.h/.cc` — canonical 3D original-source-edge/source-edge classification, construction requests, overlap intervals, ownership, and residual evidence;
- `EdgeFacetRelations.h/.cc` — complete source-edge/source-facet transverse, endpoint, tangent, coplanar, and multi-event composite assembly;
- `FacetFacetRelations.h/.cc` — bounded support relation, transverse carrier, orientation relation, and source-facet pair composites;
- `CoplanarRelationOverlay.h/.cc` — source-boundary-driven coplanar overlay, containment witnesses, overlap components, equality, coincidence orientation, and sheet occurrence records;
- `RelationConstructions.h/.cc` — authoritative point, parameter, overlap interval, and face-face carrier records plus Component 03 conditioning/residual validation;
- `CrossingMultiplicity.h/.cc` — frozen edge traversal sign, source-fan half-open ownership, local reduction, and conservation evidence;
- `SymbolicPerturbation.h/.cc` — eligibility, exact matrix lookup, conceptual side/order, half-open owner, owner rank/eligibility, occurrence separation, operand swap, and explicit no-final-selection enforcement;
- `RelationCanonicalization.h/.cc` — final ordering, ID assignment, dependency remap, trace-fragment replay, duplicate validation, reverse maps, and partitions;
- `RelationEventSeeds.h/.cc` — lineage equivalence, distinct-occurrence keys, incidence aggregation, and canonical seed production;
- `SignedFeatureRelations.h` — immutable artifact schema and owner-checked read-only views;
- `RelationQueries.h` — narrow downstream queries for Components 08-10 and diagnostics;
- `RelationBuild.h/.cc` — typed stage entrypoint and phase orchestration;
- `RelationCodec.h/.cc` — canonical owner-free semantic encoding/decoding, section digests, and replay integration; and
- `RelationVerifier.h/.cc` — independent request reconstruction, truth-layer verification, semantic relation reconstruction, seed reconstruction, downstream-boundary checks, and mutation rejection.

Extend existing registries rather than creating parallel infrastructure:

- `ContractVersions.h` for Component 07 provider, policy, family, key, formula, schema, graph, construction, multiplicity, symbolic, seed, codec, and verifier versions;
- Component 01 stage/checkpoint, strong-ID-domain, error-subcode, resource-kind, diagnostic, replay, and execution registries;
- Component 03 operation, exact-relation formula, construction, and capability registries only for genuinely missing operations; and
- `BoundedSourcePolygonKernel` formula/version registry only for operation-neutral source-polygon queries added here.

Add under `tests/mesh_boolean_bounded/`:

- `TestRelationTruthLayers.cc`;
- `TestRelationKeysGraph.cc`;
- `TestPrimitiveRelations.cc`;
- `TestSourceFacetRegionRelations.cc`;
- `TestEdgeEdgeRelations.cc`;
- `TestEdgeFacetRelations.cc`;
- `TestFacetFacetRelations.cc`;
- `TestCoplanarRelationOverlay.cc`;
- `TestCrossingMultiplicity.cc`;
- `TestSymbolicPerturbation.cc`;
- `TestRelationSelectionBoundary.cc`;
- `TestRelationOwnerSemantics.cc`;
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
- `RelationExactOracle.h/.cc` or the promoted Component 16 qualification interface;
- `RelationMutationSupport.h/.cc`; and
- `GoldenRelationsV1.h`.

Register focused CTest cases for truth layers, graph/keys, primitive predicates, source-facet region, edge-edge, edge-facet, facet-facet/coplanar, multiplicity, symbolic policy, selection boundary, owner semantics, event seeds, known artifacts, exact oracle, compute-once/retriangulation, codec, mutation, properties/fuzz, adversarial floating point, resources/cancellation, and structural performance.

Apply `ygor_apply_mesh_boolean_strict_fp` to every production and normative-test translation unit. No network discovery or optional test package is permitted. Keep mutable builders, caches, overlay sweeps, task-local constructions, verifier scratch, exact-oracle integers, and mutation helpers out of installed/public headers.

## 5. Stable versions, stages, checkpoints, and failure subcodes

### 5.1 Version registry

Add explicit nonzero V1 constants for:

- relation provider and semantic policy;
- numerical truth-layer policy;
- dependency graph and family precedence;
- primitive relation, source-facet region, edge-edge, edge-facet, facet-facet, coplanar overlay, composite contact, and disposition schemas;
- each complete owner-free relation-key family and canonical-order policy;
- request/consumer/dependency graph records;
- rounded-operation and exact-relation formula bindings;
- construction and construction-selection policy;
- crossing sign, source-fan half-open ownership, multiplicity-reduction, and conservation policies;
- symbolic eligibility, symbolic decision, and downstream-selection-boundary policies;
- event-seed equivalence and distinct-occurrence policies;
- artifact and downstream-query schema;
- canonical encoding and section-digest layout; and
- verifier and exhaustive-oracle evidence schemas.

Zero is invalid/uninitialized. Unknown required versions, unsupported enum values, mismatched predecessor versions, nonzero reserved bits, unrecognized formula IDs, or a symbolic record carrying final selection state are typed failures.

Runtime owner tokens are not versioned semantic fields. Owner-check policy version may be recorded, but the token value itself remains noncanonical.

### 5.2 Fixed logical checkpoints

Use the Component 07 stage reserved by Component 01. Define stable checkpoints:

1. runtime owner, context, operation, policy, strict-environment, and capability validation;
2. predecessor artifact/version/digest/verification validation;
3. candidate/source-feature count and representability preflight;
4. resource/work reservation for discovery and closure;
5. candidate scan and initial request/disposition proposals;
6. request sorting, duplicate validation, and first closure;
7. derived dependency generation and repeated closure;
8. graph finalization and independent acyclicity precheck;
9. rounded bounded primitive evaluation;
10. exact stored-coordinate relation evaluation;
11. orthogonal truth-record assembly and disposition;
12. source-facet region evaluation;
13. source-edge/source-edge evaluation;
14. source-edge/source-facet composite evaluation;
15. source-facet/source-facet support and carrier evaluation;
16. coplanar overlay and coincidence evaluation;
17. construction validation and authoritative-producer selection;
18. numeric crossing multiplicity and local conservation;
19. symbolic eligibility;
20. relation-specific symbolic matrix lookup and side/order/rank production;
21. downstream-selection-boundary audit;
22. event-seed proposal and candidate-disposition reconciliation;
23. canonical ordering, ID assignment, dependency/reference remap, and reverse maps;
24. producer invariant and source-facet coverage verification;
25. canonical encoding, section digests, statistics, and replay;
26. independent artifact verification, including exhaustive bounded mode when requested;
27. resource reconciliation and final cancellation check; and
28. transaction commit.

Do not renumber released checkpoints. Future providers require reserved gaps or a new version.

### 5.3 Required Component 07 failure subcodes

Allocate a disjoint Component 07 range covering at least:

- unsupported provider/policy/schema/formula;
- wrong, stale, or cross-owner handle;
- raw owner token found in semantic key/bytes/digest/replay;
- predecessor artifact, version, digest, verification, or semantic-digest mismatch;
- malformed candidate key, role, edge, triangle, witness, partition, or provenance;
- entity/count/byte/index/work overflow;
- relation, dependency, construction, overlay, symbolic, interval, seed, or consumer capacity exceeded;
- malformed request key or incompatible duplicate request;
- duplicate authoritative producer;
- missing, forward, unsupported same-family, cyclic, or unclosed dependency;
- bounded operation missing, invalid, non-finite, wrong formula, or owner-inconsistent;
- exact-relation operation missing, invalid, capacity-exhausted, wrong formula, or inconsistent;
- rounded nominal improperly treated as exact relation;
- bounded definite sign conflicting with its enclosure;
- exact relation conflicting with formula evidence;
- uncertainty incorrectly marked definite or exact;
- exact relation zero lacking structural symbolic eligibility;
- source-facet support, projection, geometry basis, orientation, ring, boundary, or semantic digest inconsistent;
- source-facet region classification unresolved;
- internal diagonal assigned source ownership;
- edge-edge relation unsupported, contradictory, or ill-conditioned;
- edge-edge point/overlap parameter, residual, or ownership inconsistent;
- edge-facet endpoint residual pair inconsistent;
- edge-facet construction ill-conditioned or over tolerance;
- edge-facet region coverage missing, duplicated, contradictory, or triangulation-dependent;
- facet-facet support uncertain, carrier unavailable, or residual failed;
- coplanarity/coincidence evidence incomplete or contradictory;
- coplanar overlay open, crossing-inconsistent, multiply owned, or coverage-incomplete;
- overlap containment witness unavailable or ambiguous;
- construction missing, non-finite, out of domain, or multiply authoritative;
- candidate missing/duplicate/contradictory disposition;
- crossing orientation mismatch;
- crossing multiplicity invalid, inconsistent, or nonconservative;
- tangent/contact incorrectly assigned numeric crossing;
- symbolic matrix lookup missing, duplicated, invalid, or operand-swap inconsistent;
- symbolic policy applied to rounded zero or ineligible uncertainty;
- symbolic decision changes nominal geometry;
- symbolic decision contains final retain/discard/suppress/output-orientation state;
- coincidence side/order/rank evidence incomplete or orientation-inconsistent;
- event-seed equivalence malformed or coordinate-derived;
- distinct occurrence merged or canonical event split;
- incidence, consumer, or reverse map incomplete;
- canonical key collision/order/ID/reference error;
- codec tag/length/count/version/reserved/trailing-data error;
- section or complete digest mismatch;
- verifier rejection;
- resource reservation/reconciliation failure;
- cancellation; and
- internal construction invariant failure.

Map unresolved conditioning or semantic bounded uncertainty to `geometric_condition_exceeds_tolerance`; representability to `index_overflow`; configured accounting exhaustion to `resource_limit`; cancellation to `cancelled`; malformed committed predecessors and producer/verifier contradictions to `internal_invariant_error`.

Every error includes the least canonical relation/candidate/source-feature witnesses, rounded nominal bits, finite intervals, exact-relation evidence, formulas/traces, precision/tolerance evidence, policy/rule IDs, resource counters, and replay identity. Raw owner-token values never become canonical witnesses.

## 6. Public entrypoint and capability boundaries

### 6.1 Typed stage entrypoint

Provide an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const signed_feature_relations<T,I>>>
build_signed_feature_relations(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const validated_operands_view<T,I>& validated,
    const source_triangle_complexes_view<T,I>& source_triangles,
    const canonical_source_manifolds_view<T,I>& manifolds,
    const canonical_candidate_stream_view<T,I>& candidates,
    const relation_capabilities<T,I>& capabilities);
```

Observable behavior:

- validate runtime ownership and every input before authoritative allocation;
- support either or both operands empty and an empty candidate stream;
- execute all relation families in one transaction;
- use the frozen operation and symbolic matrix from the context;
- never call final positive-area atom selection;
- select the same primary failure under every allowed traversal, partition, and schedule;
- join all private work before rollback;
- expose no partial primitive or composite artifact; and
- commit only after independent verification.

Lower-level test entrypoints may evaluate one primitive relation, source-feature pair, request graph, or symbolic rule. Ordinary publication remains all-or-nothing.

### 6.2 Required predecessor views

From Component 06 require:

- artifact versions/digest and verified disposition;
- candidate-domain policy version;
- candidate count, canonical iteration, checked random access, and deterministic partition ranges;
- owner-free complete candidate key and directed role;
- canonical edge ID/class/representative/endpoints/halfedges/incidences;
- opposite triangle ID/orientation/source facet/shell/provenance;
- overlap witness and precision references; and
- no hidden traversal, lazy discovery, or mutable cache.

From Component 05 require:

- canonical source vertices and bounded-point attachments;
- canonical edges, original-source-edge/internal-diagonal class, endpoint identity, canonical direction, reciprocal halfedges, incident triangles/facets/shells, and segment bounds;
- oriented source triangles and local edge-use roles;
- source-facet groups with semantic ring, geometry basis, boundary uses, projection/support/orientation, triangle/diagonal membership, semantic digest, and exact-triangulation digest;
- source shell groups with occupied-side semantics;
- vertex fans and reverse maps; and
- owner-checked lookup by strong ID plus owner-free complete semantic keys.

From Component 04 require semantic rings, projection formulas, triangle-local boundary/diagonal provenance, coverage evidence, semantic/exact digests, and operation-neutral bounded source-polygon capabilities.

From Component 03 require:

- owner-checked bounded point/plane/vector/scalar/parameter/residual views;
- fixed grouped dot, cross, determinant, residual, interpolation, division, interval, projection, and carrier construction services;
- exact stored-coordinate relation services for prescribed formulas;
- separate rounded-operation and exact-relation formula IDs;
- bounded-sign and exact-relation orthogonal result fields;
- admitted alternate-formula dispositions;
- edge-plane and line-line or equivalent construction services;
- conditioning/tolerance classification and precision-ledger references;
- finite interval hull/intersection under proof, endpoint classification, and residual checks; and
- exact scalar bit/finite-total-order services for canonical diagnostics only.

From Component 02 require coherent geometry-basis references, shell orientation/nesting/occupied sides, source facet/ring identities, and boundary provenance. From Component 01 require symbolic matrix lookup, truth-table version reference without final selection invocation, operand exchange transforms, complete tie-key comparison, owner checking, strong-ID publication, resources, cancellation, execution scopes, diagnostics, replay, canonical bytes, SHA-256, and transactions.

### 6.3 Downstream capability

Component 08 receives a `signed_feature_relations_view<T,I>` supporting:

- header/version/predecessor-digest inspection;
- checked canonical iteration and random access by relation, construction, symbolic-decision, and seed ID;
- complete owner-free semantic keys and family/category lookup;
- dependency and reverse-consumer ranges;
- candidate dispositions and mappings;
- source-feature and triangle-local provenance;
- rounded nominal, exact relation, enclosure, and consumer disposition evidence;
- bounded constructions, parameters, residuals, conditioning, and precision lineage;
- signed numeric and symbolic crossing metadata;
- symbolic eligibility, rule ID, side/order/rank/occurrence evidence;
- event-equivalence and distinct-occurrence keys;
- source edge/facet/triangle/halfedge incidence; and
- deterministic partitions with no mutation, allocation, or lazy arithmetic.

Component 09 receives the relation and event evidence needed for cut deltas, symbolic side labels, coincidence descriptors, and occurrence separation. Component 10 may inspect symbolic rank/eligibility and coincidence lineage but must derive final selection from Component 09 atom side labels and Component 01 truth-table evaluation.

No downstream component receives raw relation inputs plus permission to recompute coordinates or infer event equivalence geometrically.

## 7. Strong IDs, closed enums, and complete keys

### 7.1 Strong ID domains

Use Component 01 `relation_id` and `symbolic_decision_id`. Add distinct Component 07 domains for:

- `relation_request_id`;
- `relation_dependency_edge_id`;
- `relation_consumer_id`;
- `primitive_operation_record_id`;
- `exact_relation_record_id`;
- `source_facet_region_record_id`;
- `relation_construction_id`;
- `overlap_interval_id`;
- `coplanar_overlap_component_id`;
- `crossing_contribution_id`;
- `multiplicity_evidence_id`;
- `symbolic_eligibility_id`;
- `event_seed_id`;
- `candidate_disposition_id`; and
- verifier-evidence IDs where generic evidence is unsafe.

Do not alias domains to one another, predecessor IDs, `I`, `size_t`, or raw offsets. Dense ordinals are checked storage details.

### 7.2 Relation families

Define closed explicit values including at least:

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

Use separate closed enums for bounded-sign status, exact-relation status, consumer disposition, contact dimension, relation category, construction role, multiplicity type, symbolic eligibility reason, symbolic consequence kind, owner-rank eligibility, occurrence-separation class, seed class, and candidate disposition.

No enum value may overload nullable fields whose interpretation changes by context. Unknown or zero values fail decoding.

### 7.3 Orthogonal truth record

Use a record conceptually equivalent to:

```cpp
struct primitive_truth_record {
    rounded_nominal_bits nominal;
    finite_interval enclosure;
    bounded_sign_status bounded_sign;
    exact_relation_status exact_relation;
    consumer_disposition disposition;
    operation_formula_id rounded_formula;
    exact_relation_formula_id exact_formula;
    operation_trace_id rounded_trace;
    exact_relation_record_id exact_trace;
};
```

The exact layout may differ, but no compatibility enum may erase the constituent fields. Exact relation may be unavailable when no supported formula is requested; that is distinct from exact zero and invalid.

### 7.4 Complete semantic keys

A complete public relation key contains, as applicable:

```text
(
  stable context digest or owner-free semantic namespace,
  relation family,
  directed operand role or canonical unordered pairing,
  complete source-feature semantic identities,
  directed source-feature use where orientation matters,
  source-facet semantic identity,
  occurrence discriminator for genuinely distinct contacts,
  dependency-graph version,
  relation formula/provider version,
  symbolic-policy version for symbolic records,
  key-schema version
)
```

Raw runtime owner-token values are forbidden. Public identity uses complete source-feature keys, not only dense IDs. Triangle-local keys include triangle/local-slot/diagonal discovery lineage and the public composite key they feed. Construction keys include authoritative producer relation and role. Symbolic keys include resolved relation, operation, operand roles, exact rule key, and source-feature tie key. Seed keys follow Section 19.

Hashes may accelerate lookup, but equality and ordering compare full keys. IDs are assigned after final canonical sorting. Candidate order, task IDs, pointers, hashes, coordinates, uncertainty width, or completion order do not enter identity.

### 7.5 Operand exchange

Every key, relation category, numeric multiplicity, symbolic side assignment, owner-rank record, occurrence key, and diagnostic defines a total operand-exchange transform. Union, intersection, and symmetric difference retain operation; differences exchange. Tests prove exchange is involutive.

## 8. Immutable artifact and table schemas

### 8.1 Artifact header and owner separation

`signed_feature_relations<T,I>` stores canonical semantic fields:

- stable context digest, operation, operand semantic IDs, and ordinary-publication eligibility;
- Component 07 provider/policy/schema/formula/graph/codec/verifier versions;
- required Component 01-06 versions, artifact IDs, and digests;
- source semantic and exact-triangulation digests;
- symbolic matrix and truth-table version/digest references;
- strict floating profile and precision-capability references;
- counts/ranges for every table and family;
- dependency node/edge/reverse-consumer counts;
- candidate disposition totals;
- construction, interval, overlay, multiplicity, symbolic, seed, and incidence counts;
- deterministic partitions;
- resource/statistics and verifier-evidence ranges;
- replay reference;
- section and complete semantic digests; and
- zero reserved fields.

The artifact handle or wrapper also carries runtime owner validation state. Raw token values do not enter semantic content or codec. No mutable cache, task-local handle, allocator, callback, caller pointer, or provider workspace escapes.

### 8.2 Primitive operation and exact-relation records

Each primitive bounded-operation record contains:

- strong ID, complete key, family, rounded formula/version;
- ordered bounded inputs;
- Component 03 operation trace and precision references;
- rounded nominal bits and finite enclosure;
- bounded-sign status and definite margin;
- uncertainty contributor summary;
- admitted alternate-formula attempt/disposition;
- conditioning/tolerance state; and
- ordered consumers/dependencies.

Each exact-relation record contains:

- distinct strong ID and complete key;
- exact relation formula/version and ordered stored nominal inputs;
- exact negative/zero/positive/unavailable/invalid result;
- normalization, capacity, and resource evidence;
- exact trace/reference; and
- consumers.

A composite `primitive_truth_record` references both where applicable and stores the consumer disposition. Component 07 never copies only a sign while dropping evidence.

### 8.3 Source-facet region record

A region record contains:

- queried source or constructed point/segment lineage;
- source facet/ring/support/projection/orientation/geometry-basis identity;
- projected bounded point/segment references;
- complete original polygon boundary traversal evidence;
- classification: interior, original edge, original vertex, outside, internal-diagonal-only witness, overlap interval/region, unresolved, or invalid;
- every original boundary owner at an exact tie;
- triangle-local coverage witnesses and composite map;
- half-open parity/winding evidence where used;
- semantic digest independent of triangulation; and
- ordered dependencies/consumers.

### 8.4 Edge-edge relation record

Each canonical original-source-edge/source-edge record contains:

- both source edge semantic identities, canonical directions, endpoints, incident uses/facets/shells, and bounds;
- support, parallelism, coplanarity, collinearity, and projection truth records;
- selected stable projection/formula;
- one tagged category;
- point-contact bounded parameters, authoritative point/source endpoint, ownership, and residuals; or
- overlap bounded intervals, endpoint seeds, direction agreement/opposition, source-vertex ownership, and interval disposition;
- numeric contact dimension and crossing relevance;
- candidate/triangle consumers, including bookkeeping witnesses;
- event-equivalence lineage; and
- symbolic eligibility references.

### 8.5 Edge-facet relation record

Each canonical source-edge/source-facet record contains:

- source edge semantic identity/canonical direction and opposite source facet identity;
- all candidate and triangle-local witnesses;
- endpoint support truth records;
- complete facet region dependencies;
- ordered local event records, possibly several;
- coplanar overlap intervals and boundary ownership;
- occupied/unoccupied side state before/after each event;
- numeric signed crossing per event and aggregate;
- separate symbolic side/crossing metadata;
- tangency/contact/coincidence metadata;
- construction/residual/conditioning references;
- full source-facet coverage reconciliation; and
- canonical occurrence discriminators.

### 8.6 Facet-facet and coplanar overlay records

A facet-facet support record contains:

- both semantic facet identities, supports, orientations, shells, geometry bases, and bounds;
- parallelism/coplanarity truth records;
- same/opposite support orientation;
- stable transverse carrier when definitely nonparallel;
- carrier construction, orientation, parameterization, residual, and conditioning;
- exact coplanar support evidence when tied; and
- links to edge-facet and edge-edge consumers.

A coplanar overlay record contains overlap components, boundary arcs/points, containment/equality witnesses, original source-boundary ownership, same/opposite sheet relation, source-side occupancy provenance, sheet occurrence identities, and occurrence-separation constraints. It does not contain final retained owner or output orientation.

### 8.7 Construction record

Every authoritative construction contains:

- construction ID/key, one producer relation, role, formula, dependencies;
- accepted source bounded point or Component 03 constructed bounded point;
- nominal bits and finite enclosure;
- bounded parameters on defining carriers;
- source-feature lineage;
- residuals against every defining edge/support/carrier;
- conditioning and tolerance disposition;
- precision-ledger reference;
- optional non-authoritative verification witnesses; and
- consumer range.

Endpoint contacts reference accepted source points rather than interpolating zero or one.

### 8.8 Crossing, symbolic, seed, and disposition records

Each crossing contribution stores relation/event-local lineage, canonical edge traversal, opposite occupied-side convention, numeric contribution, boundary ownership, dependencies, and conservation witness. Symbolic contributions are separate fields and never overwrite numeric evidence.

Each symbolic eligibility record stores:

- exact reason code;
- exact-relation/lineage evidence;
- uncertainty compatibility and tolerance disposition;
- structural relation category;
- policy key domain; and
- eligibility or precise failure.

Each symbolic decision stores rule ID/key, operation, conceptual side/order, half-open owner, symbolic crossing where applicable, coincident owner rank or eligibility, occurrence separation, operand exchange, and explanation. It stores no changed coordinate and no final positive-area selection state.

Each event seed follows Section 19. Each candidate disposition stores one candidate, one stable disposition, relation/seed ranges, and complete coverage evidence.

## 9. Count, capacity, dependency, and resource preflight

### 9.1 Checked predecessor counts and worst-case products

Before request discovery, read checked counts for candidates, canonical vertices/edges/triangles/facets/halfedges, facet boundary uses, and triangle memberships. Validate representability of at least:

- one candidate disposition per candidate;
- up to two endpoint/facet support requests per candidate edge/opposite facet;
- bounded-operation and exact-relation records independently;
- all candidate-induced edge/facet composites;
- candidate-induced source-edge/source-edge requests against original opposite triangle boundary edges;
- facet/facet requests induced by edge/facet relations;
- request consumers and dependency edges;
- triangle-local coverage records;
- possible point events, overlap intervals, and coplanar overlay elements under limits;
- symbolic eligibility/decision records;
- event seeds and incidences;
- task fragments, sort workspaces, reverse maps, verifier duplication, canonical bytes, diagnostics, and replay.

Use Component 01 checked arithmetic. Distinguish representability failure from configured resource limits. Do not allocate a full all-feature Cartesian product when candidate-driven closure is output-sensitive, but prove the configured worst-case count before mutation.

### 9.2 Request-closure guard

Define an architecture-independent upper bound on unique canonical requests derivable from predecessor entity counts, candidate incidence, and finite V1 family rules. Charge every proposal, complete-key comparison, group, dependency insertion, closure round, bounded operation, exact relation, construction, polygon boundary test, overlay step, multiplicity reduction, symbolic lookup, seed incidence, and verifier operation to abstract work.

Closure terminates when a full round adds no new key. Fail if a round adds a key in a closed earlier family, reveals a cycle, exceeds the derived unique-key bound, or exhausts work. Do not use an arbitrary uncharged iteration count.

### 9.3 Resource classes and reservation phases

Account separately for:

- request proposals and canonical request records;
- primitive bounded-operation and exact-relation records;
- relation records by family;
- dependencies and reverse consumers;
- projected region workspaces;
- edge-edge/edge-facet/facet-facet workspaces;
- coplanar overlay points/arcs/components;
- constructions, parameters, intervals, residuals;
- crossing contributions and conservation evidence;
- symbolic eligibility/decisions;
- event seeds and incidence;
- dispositions and mappings;
- canonical sort/merge and trace replay;
- task descriptors/private buffers;
- codec/digest/replay/diagnostic bytes;
- verifier/exact-oracle work; and
- persistent artifact bytes.

Reserve fixed predecessor-derived tables and discovery work before scanning. After closure yields exact unique counts, reserve exact persistent and evaluation workspace before authoritative evaluation. Reconcile actual use before commit. Never omit evidence or change formulas because a limit is tight.

## 10. Predecessor validation and candidate request generation

### 10.1 Cross-artifact validation

Before relation work, validate:

1. runtime owner compatibility without copying token values into semantics;
2. context operation, symbolic/truth policy versions, strict profile, and execution mode;
3. Component 02-06 artifact owners, versions, digests, and verification dispositions;
4. candidate predecessor digests equal the exact Component 05 artifacts supplied;
5. source semantic and exact-triangulation digests agree across Components 04-06;
6. every candidate edge/triangle/witness resolves to the advertised canonical record and precision attachment;
7. every source-edge/internal-diagonal role agrees with Components 04-06;
8. source-facet geometry basis, support, projection, orientation, ring, and shell occupied-side records are complete;
9. required Component 03 bounded-operation and exact-relation capabilities are finite, owner-correct, version-valid, and ordinary-publication eligible; and
10. no task-local, stale, wrong-domain, or out-of-range ID appears.

A committed predecessor contradiction is `internal_invariant_error`; do not choose one artifact as newer or reinterpret geometry.

### 10.2 Initial candidate-derived requests

For each canonical candidate `(edge E, opposite triangle T)`:

- resolve opposite source facet `F`, triangle edge uses, and edge class;
- request endpoint/facet support bounded operations and exact-relation evidence as separate records for both canonical endpoints of `E` against `F`;
- request canonical source-edge/source-facet composite `(E_source,F)` when `E` is an original source edge;
- when `E` is an internal diagonal, request only triangle-local bookkeeping edge/facet relations and source-facet composites needed to absorb evidence;
- request original-source-edge/source-edge relations for opposite original boundary edges when required for boundary ownership or coplanar partitioning;
- retain internal-diagonal relations only as coverage witnesses;
- request source-facet/source-facet support for source facets incident to `E` and `F` when carrier or coplanar organization may be needed;
- attach the candidate as a consumer to every request; and
- emit one pending disposition.

Request generation uses topology and provenance, not nominal geometry beyond the broad-phase retention.

### 10.3 Derived request closure

Derived requests include:

- point/facet region tests for accepted endpoints or constructed points;
- projected bounded operations and exact orientation/collinearity relations required by region tests;
- facet/facet support when an edge/facet pair is not definitely separated;
- original source-edge/source-edge relations encountered during region or coplanar partitioning;
- authoritative construction requests after a category proves a point/interval/carrier is needed;
- coplanar overlay after exact coplanarity eligibility is established;
- multiplicity reducers after complete local contacts are known;
- symbolic eligibility and decision requests after exact-tie categories are established;
- event-seed requests after ownership, construction, multiplicity, side/order/rank, and occurrence facts are final.

Every derived request records its requesting relation. The graph shows why each relation exists even without a direct candidate consumer.

## 11. Primitive bounded relations and formula dispatch

### 11.1 Support-side evaluation

For a source point `P` and oriented source facet support `F`, request exactly one Component 03 bounded residual operation using the accepted unnormalized plane and one exact-relation request for the corresponding supported numerator or determinant when required.

Do not reconstruct or normalize the plane. Store:

- rounded residual bits;
- finite residual enclosure;
- bounded-sign status;
- exact stored-coordinate relation status;
- raw support-side interpretation;
- occupied/unoccupied interpretation from Component 02; and
- consumer disposition.

If the primary enclosure overlaps zero, use only the one alternate bounded formulation explicitly authorized by Component 03 for this operation and provenance. Both attempts remain under one trace root. Exact relation zero does not itself choose symbolic policy; it forwards eligibility evidence to the semantic relation. If unresolved and no admitted route exists, fail.

### 11.2 Projected orientation and interval predicates

Use the frozen source-facet projection frame and Component 03 bounded 2D determinant, parameter, and interval services. Every projected point/frame and relation formula has one canonical key. Do not derive a second projected coordinate for the same lineage/frame.

For exact projected ties, preserve reason:

- exact source vertex identity;
- exact point-on-original-edge lineage;
- exact collinearity under a supported exact-relation formula;
- versioned representational tie; or
- no structural ownership despite rounded nominal zero.

Only admitted structural categories may feed boundary or symbolic rules.

### 11.3 Definite, exact, and unresolved decisions

A definite bounded sign requires an enclosure excluding zero. An exact stored-coordinate sign cannot override inherited uncertainty for bounded separation. A rounded sign cannot override either.

An exact stored-coordinate zero is retained as exact-relation evidence. The semantic relation then decides whether structural lineage and tolerance permit exact-tie handling. If possible uncertainty realizations would change relation topology beyond the accepted tie contract, fail rather than symbolically guess.

No caller tolerance comparison directly replaces a sign predicate. Tolerance is consulted through Component 03 conditioning and publication eligibility.

### 11.4 Formula extension gate

When an implementation step needs a formula not exposed by Component 03:

1. specify the algebraic relation and ordered inputs;
2. specify rounded-operation graph separately from exact-relation formula;
3. add stable formula IDs and capability version in Component 03;
4. implement finite enclosure, exact-relation, capacity/resource, trace, diagnostic, replay, and verifier support there;
5. add Component 03 known-answer and exact-oracle tests; and
6. consume the new narrow capability in Component 07.

Never embed an unregistered determinant or exact arithmetic fallback in relation code.

## 12. Complete source-facet region kernel

### 12.1 Point versus complete source polygon

Classify a source or constructed point against a source facet in two dimensions:

1. support relation to the accepted oriented plane; and
2. region relation to the complete semantic polygon in the frozen projection and coherent geometry basis.

Use a deterministic half-open parity/winding algorithm over original projected boundary edges:

- primary sweep ordinate and crossing coordinate are frozen by kernel policy;
- each boundary edge uses one lower-inclusive/upper-exclusive convention derived from exact endpoint identity and bounded order;
- bounded orientation determines definite crossing contribution;
- exact point-on-edge/vertex ownership is collected before parity reduction;
- one versioned alternate sweep axis may be used when qualified by Component 03; and
- fail if possible outcomes change interior/boundary/outside semantics.

No arbitrary ray endpoint, random direction, nominal-only crossing, or epsilon is permitted.

### 12.2 Boundary ownership

If exactly on an original source edge, record that edge and both directed uses. If exactly on an original source vertex, record the vertex and every incident original boundary use required by the relation. Preserve all valid owners in canonical order.

An internal diagonal may remain a coverage witness but cannot convert interior into source-boundary ownership.

### 12.3 Triangle-local reconciliation

For each triangle-local hit or candidate:

- classify through the semantic source-polygon provider;
- map to one source-facet composite or documented no-public relation;
- compare triangle-local barycentric/edge evidence with original boundary ownership;
- coalesce duplicate discovery only through common source-feature/construction lineage;
- preserve distinct ordered crossings of concave facets; and
- prove every candidate has an absorption or event contribution.

Legal retriangulation may alter bookkeeping records and exact-triangulation digests but not public region classes, event seeds, numeric crossing totals, symbolic decisions, or semantic digests.

### 12.4 Segment versus source polygon

For coplanar edge/facet relations, partition the closed source-edge parameter domain by all original source-boundary contacts. Use canonical edge-edge relations and proof-producing bounded parameter comparisons to create breakpoints.

For each open interval, classify one deterministic certified interior witness. Prefer a Component 03 construction at an exact rational midpoint when certified. Otherwise use a finite versioned dyadic interior sequence with checked work. Preserve interval endpoint owners, source-edge overlaps, exact zero-length contacts, and unresolved ordering. Do not infer interval topology from nominal parameter sort.

## 13. Canonical source-edge/source-edge relations

### 13.1 Fixed 3D support classification

For canonical directed original source edges `A: a0->a1` and `B: b0->b1`, obtain bounded direction vectors and fixed Component 03 operations:

```text
u = a1 - a0
v = b1 - b0
w = b0 - a0
n = cross(u, v)
parallel_measure = dot(n, n)
coplanarity_measure = dot(w, n)
```

Request exact relations for the supported polynomial zero tests separately. Require definitely positive bounded direction squared norms.

Classify nonparallel, exact parallel, unresolved, or invalid from orthogonal evidence. For definitely nonparallel edges, require coplanarity support before point intersection. Definite nonzero coplanarity means skew/separated. Exact coplanarity routes to the stable solve. Unresolved semantic outcome fails.

### 13.2 Stable nonparallel parameter solve

Select the 2D minor from the component of `n` whose squared absolute lower bound is greatest and definitely positive; tie X/Y/Z. If no minor is definitely invertible, use only the registered alternate Component 03 construction formula, then fail if unresolved.

Solve bounded parameters with fixed 2x2 determinant formulas. Classify complete intervals against `[0,1]`. Construct one point through the selected endpoint formula. Validate residuals against both carriers. Classify interior/interior, endpoint/interior, endpoint/endpoint, or no-contact.

A parameter rounded to zero or one does not prove endpoint identity. Reuse a source endpoint only when exact parameter numerator/identity and residual evidence support it.

### 13.3 Parallel and collinear handling

For exact parallel support eligibility, test collinearity with bounded residuals and exact-relation evidence. Distinguish definite parallel separation, exact collinearity, unresolved, and invalid.

For exact collinearity:

- choose a carrier axis from the greatest definitely positive direction-component lower bound, tie X/Y/Z;
- parameterize both edges against canonical edge A through Component 03 bounded division;
- collect A endpoints and B projected endpoints;
- compute closed interval intersection with proof-producing interval operations;
- classify disjoint, point contact, partial overlap, containment, or equality;
- retain orientation agreement/opposition; and
- use accepted source vertices for endpoint seeds whenever exact identity permits.

Unresolved ordering that changes overlap topology fails. Do not use distance-to-line, normalized directions, or coordinate epsilon.

### 13.4 Ownership and duplicate discovery

A public relation exists only for original source edges from opposite operands. Internal-diagonal relations remain bookkeeping. Candidate discoveries through multiple triangles/facets attach to one producer. Verify duplicate consumers agree on category, parameters, point/interval lineage, ownership, and all truth layers.

## 14. Canonical source-edge/source-facet relations

### 14.1 Endpoint support pair

For source edge `E: p0->p1` and opposite facet `F`, consume the two canonical support truth records. Interpret bounded side through `F`'s occupied/unoccupied semantics.

Classify:

- both endpoints definitely same side: no transverse crossing, while exact endpoint/contact evidence may still require boundary handling;
- endpoints definitely opposite: one transverse support crossing candidate;
- one endpoint eligible exact support tie and the other definite: endpoint contact/crossing candidate;
- both endpoints eligible exact coplanar ties: coplanar edge/facet handling;
- unresolved: use prescribed alternate then fail if semantics remain uncertain; and
- invalid: typed failure.

Rounded zero alone never enters tie branches.

### 14.2 Transverse construction and region classification

For opposite-side endpoints, request Component 03 edge-plane construction using existing residuals. Require finite parameter/point enclosure, stable interior or exact endpoint classification, edge/support residuals, tolerance eligibility, and one authoritative producer.

Classify the point against the complete source polygon. Outside means no contact. Interior means proper face crossing. Original boundary edge/vertex means boundary crossing/contact requiring source-fan ownership. Internal-diagonal-only witness remains facet interior.

### 14.3 Endpoint and tangent contacts

When an endpoint has eligible exact support relation, reuse the source bounded point and classify against the source polygon. Determine adjacent side state from the other endpoint and incident source-fan relations. Record endpoint contact, tangency, or half-open crossing without fabricating an out-of-domain parameter.

A tangent has numeric multiplicity zero unless the source-fan rule identifies one true boundary transition. Symbolic contribution remains separate.

### 14.4 Coplanar complete relation

When both endpoint support relations have eligible exact coplanar evidence, invoke segment/source-polygon partitioning. Produce isolated point contacts, boundary overlaps, interior overlaps, containment, original source ownership, and zero numeric transverse crossing for intervals.

Reference every canonical boundary edge-edge relation used in partitioning. Triangle-local coplanar hits are witnesses only.

### 14.5 Composite occurrence order

Order local events along canonical edge direction using bounded parameter comparisons. Definitely separated intervals establish strict order. Exact equal parameters form a cluster ordered by complete occurrence tie key while retaining equality. Unresolved overlapping intervals may cluster only if all admitted orders yield the same partition and crossing sequence; otherwise fail.

Assign occurrence discriminators only after order is proved. Never collapse by coordinate equality.

## 15. Source-facet/source-facet support and coplanar overlay

### 15.1 Support relation

Use accepted unnormalized bounded planes. Compute bounded normal cross product and separate exact-relation evidence.

Classify definitely nonparallel, eligible exact parallel, unresolved, or invalid. For nonparallel supports, request one bounded carrier. Orient the carrier from source semantic keys and oriented cross product without normalization. Validate residuals and conditioning.

For exact parallel eligibility, evaluate accepted anchors against opposite supports. Distinguish parallel separated, eligible exact coplanar, unresolved offset, and invalid. Approximate normal/offset similarity is insufficient.

### 15.2 Orientation relation

For exact coplanarity, determine same/opposite support orientation from bounded and exact-relation evidence plus accepted source orientation. Preserve shell occupied-side semantics separately. Unresolved orientation fails.

### 15.3 Coplanar boundary overlay

Build overlap from original source boundaries:

1. gather canonical cross-operand source-edge/source-edge point and overlap relations for the facet pair;
2. gather source vertices of each facet classified against the other polygon;
3. partition original boundary edges by canonical event parameters;
4. label open boundary intervals outside/on/inside using certified witnesses;
5. assemble boundary arcs/points into overlap components using exact source incidence and event lineage;
6. classify area overlap, segment/point contact, containment, or equality;
7. verify each overlap boundary is covered exactly by authorized oriented arcs; and
8. preserve coordinate-coincident distinct sheet occurrences.

Do not triangulate the overlap region as authority. Triangle coverage is supplemental only.

### 15.4 Containment and equality witnesses

If no boundary crossing exists, use one deterministic certified interior witness per facet. Prefer a predecessor-published accepted witness. Otherwise derive one from the least canonical source triangle through Component 03 bounded barycentric construction with exact rational weights, and verify support and strict polygon interior. Use a finite versioned dyadic sequence if needed.

Facet regions are equal only when boundaries are completely covered, orientations and ownership are consistent, and neither facet has an outside witness. Area equality or equal counts are insufficient.

### 15.5 Transverse carrier membership

For nonparallel facet pairs, all edge-facet transverse events reference the same carrier. Project constructions to one bounded carrier parameter through Component 03. Store expected membership and parameter for Component 08. Contradictory residuals or incompatible parameters fail.

## 16. Authoritative construction policy

### 16.1 One producer precedence

Every construction key has one producer chosen by fixed precedence:

1. accepted source vertex point;
2. canonical source-edge/source-edge point relation;
3. canonical source-edge/source-facet point relation;
4. canonical coplanar overlap endpoint relation;
5. canonical facet-facet carrier construction; and
6. test-only verification witness.

When several relations describe one conceptual point, explicit lineage names the producer and others become consumers or verification witnesses. Do not choose smallest enclosure, first candidate, or first completion.

### 16.2 Secondary witness compatibility

For every required secondary witness verify:

- authoritative nominal point lies in the witness enclosure when they claim one real occurrence;
- parameters and endpoint ownership are compatible;
- residuals reference the same carriers/supports;
- precision discrepancy is explained by recorded operation graphs; and
- no witness implies another occurrence.

Do not union contradictory enclosures to hide conflict.

### 16.3 Finite/tolerance gate

No construction publishes unless all nominal values/enclosure endpoints are finite, denominators are definitely separated from zero or routed to an eligible exact case, parameters and residuals satisfy Component 03, and aggregate precision remains within tolerance. Symbolic policy cannot rescue unrepresentable or over-budget construction.

## 17. Signed crossing multiplicity and local conservation

### 17.1 Frozen sign convention

For each event on a canonical directed source edge:

- traverse Component 05 direction from parameter 0 to 1;
- interpret opposite support sides through Component 02 occupied/unoccupied semantics;
- numeric `+1` means opposite winding increases after the event;
- numeric `-1` means it decreases; and
- numeric `0` means no net crossing.

Proper interior crossing sign comes from definite endpoint side states. Edge-direction reversal negates numeric multiplicity under key remap.

### 17.2 Source-fan half-open ownership

For events on an opposite source edge or vertex, gather all incident original source-facet uses through Component 05. V1 rule:

1. evaluate local support transitions using existing relation records;
2. exclude internal diagonals;
3. order eligible exact ties by Component 01 half-open rule key;
4. assign numeric crossing to exactly one incident occurrence representing the real occupied/unoccupied transition;
5. assign zero to duplicate consumers and tangential fan contacts; and
6. store complete contribution list and sum.

Ordinary closed two-manifold crossing total is `-1`, `0`, or `+1`. Multi-sheet aggregate may be wider only when constituents are listed and overflow-checked.

### 17.3 Tangency and conservation

Verify local fan total is order- and triangulation-independent, every nonzero contribution has one owner, tangencies total zero unless a true boundary transition exists, traversal reversal negates constituents, and closed-fan side states are consistent.

Symbolic side/crossing metadata is stored separately and does not overwrite numeric conservation.

## 18. Symbolic perturbation and downstream selection boundary

### 18.1 Eligibility

Create eligibility only for stable reasons:

- exact relation zero under a qualified formula plus a supported structural category;
- exact shared source endpoint identity;
- exact collinear source-edge lineage;
- exact coplanar source-facet lineage;
- exact equal source-feature lineage;
- versioned representational tie admitted by Components 01/03; or
- versioned coincident-source contract with equivalent exact evidence.

Record rounded nominal bits separately, inherited uncertainty, whether separated realizations remain possible, structural category, and tolerance compatibility. Rounded zero, over-wide intervals, near-parallel failure, contradictory duplicates, unrepresentable bounds, missing formulas, and resource failures are ineligible.

### 18.2 Total matrix lookup

For every eligible relation construct the exact `symbolic_rule_key` required by Component 01, including operation, operand role, relation family, orientation, source-feature ownership role, half-open endpoint/edge role, transition orientation, and occurrence class. Lookup returns exactly one rule. No default or local fallback.

Produce:

- rule ID and complete tie key;
- conceptual side/order assignment;
- half-open crossing owner or symbolic contribution;
- owner rank or eligibility among coincident occurrences;
- occurrence separation;
- explanation code; and
- operand-exchange transform.

Verify rule content against the frozen matrix digest.

### 18.3 Lower-dimensional contact semantics

For point- and edge-touching regular solids:

- preserve explicit contacts and distinct topology occurrences;
- do not create positive-volume connectivity from coordinate coincidence;
- use symbolic ordering only for downstream classification and ownership constraints;
- retain numeric crossing zero for tangent/contact geometry; and
- require Component 08-11 occurrence separation sufficient to prevent welding.

### 18.4 Coincident component relation evidence

For each coplanar overlap component:

1. identify common support and its two conceptual sides;
2. preserve each source sheet's occupied-side provenance from Component 02;
3. establish same/opposite orientation and exact overlap grouping;
4. look up operation-specific symbolic sheet order and owner-rank eligibility;
5. publish symbolic negative/positive side metadata needed by Component 09;
6. publish half-open overlap-boundary ownership and occurrence separation;
7. preserve all source-facet partition and sheet-member lineage; and
8. do not evaluate final atom truth tuples or choose final retained owner/output orientation.

Component 09 creates complete classification atoms and side labels. Component 10 evaluates the truth table and resolves final external/internal/cancelled/non-owner status jointly over complete sheet cells. Component 07's owner rank is a policy constraint, not a final selection.

### 18.5 Selection-boundary enforcement

Implement an explicit structural audit before publication:

- symbolic consequence enums contain no final retained/discarded/suppressed/output-oriented values;
- no Component 10 atom, sheet-cell, retained-use, or output-occurrence ID appears;
- no final truth-table result bits are stored;
- Component 01 truth-table selection callback is not invoked by Component 07;
- queries expose symbolic inputs/evidence, not a preselected answer; and
- the verifier rejects forbidden fields/tags even if digests are recomputed.

### 18.6 Operand exchange and determinism

Store expected exchanged rule key/value or a verifier reference. Exchange/remap produces documented side/order/rank/crossing/occurrence semantics. No rule consults traversal, triangle order, worker, pointer, hash bucket, runtime owner token, or unversioned detail.

## 19. Event-seed production

### 19.1 Seed classes

Emit seeds for:

- existing source vertex/facet contact or crossing;
- constructed edge/facet point;
- edge/edge point;
- coplanar overlap endpoint;
- tangent contact point;
- one event discovered by several triangles;
- symbolic tie occurrence without coordinate movement; and
- distinct coincident-sheet occurrence.

Intervals and area components reference endpoint seeds and interval/component records; they are not forced into one point event.

### 19.2 Equivalence key

The key contains:

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

Triangle/internal-diagonal IDs appear only in discovery provenance after source-feature reduction. Coordinates, bounds, nominal parameters, spatial keys, hashes, and owner tokens do not enter equivalence.

### 19.3 Distinct occurrence key

Add a distinct-occurrence key whenever one coordinate/construction represents separate topological or conceptual occurrences. Include source sheet/facet use, contact-side role, symbolic occurrence rank, and canonical local occurrence discriminator. Different occurrence keys remain separate even when nominal bits, bounds, source point, and parameters are equal.

### 19.4 Complete incidence

Each seed records authoritative relation and point, all source owners, candidate/triangle/diagonal provenance, every consuming source triangle and halfedge use, contact category, numeric/symbolic crossing metadata, candidate dispositions, carrier memberships, parameters, equivalence/occurrence keys, and precision evidence.

Sort incidence by complete feature/use key and reject missing, duplicate, contradictory, or coordinate-inferred entries.

## 20. Candidate disposition and coverage reconciliation

Every Component 06 candidate receives one final disposition:

- definitely separated by canonical relation;
- duplicate triangle discovery absorbed into source-feature relation;
- primitive dependency consumer only;
- contributed event seeds;
- contributed coplanar/coincident relation or overlap interval;
- retained zero-measure contact;
- bookkeeping internal-diagonal witness absorbed into source-facet coverage; or
- caused canonical typed failure.

A success artifact has no pending/failed disposition. Verify each candidate witness appears in required consumer maps and each source-edge/source-facet composite accounts for the union of triangle-local discoveries. Missing coverage, contradictory local truth records, incompatible constructions, or a candidate mapped to incompatible public relations prevents publication.

Empty candidate streams publish a canonical empty relation graph with no invented containment. Component 09 classifies disconnected surfaces globally.

## 21. Deterministic request evaluation and canonical publication

### 21.1 Serial semantic reference

Implement serial request generation, closure, evaluation, composites, multiplicity, symbolic evidence, seeds, and canonicalization first. It remains executable in normative tests and is Component 17's semantic oracle.

Within each family evaluate sorted complete keys. Component 03 may create private trace fragments, but final relation IDs and ledger references are assigned through canonical merge. Failure keys include family and complete semantic relation key.

### 21.2 Parallel-ready boundaries

Parallel execution may partition immutable request ranges. Tasks write private results and fragments. They must not:

- insert into timing-dependent shared semantic caches;
- allocate final relation/construction/symbolic/seed IDs;
- commit precision-ledger order directly;
- choose symbolic rules from local traversal context;
- access final selection services; or
- publish partial families.

Canonical merge sorts complete keys, validates duplicates, assigns IDs, remaps dependencies, and replays Component 03 trace fragments in canonical order. All schedules reproduce serial bytes and primary failure.

### 21.3 Final canonical order

Assign IDs after final validation in this order:

1. bounded primitive operation records;
2. exact-relation records;
3. composite primitive truth records;
4. source-facet region records;
5. edge-edge relations;
6. edge-facet relations;
7. facet-facet support records;
8. coplanar overlay components;
9. composite contacts;
10. constructions and intervals;
11. crossing contributions/evidence;
12. symbolic eligibility and decisions;
13. event seeds; and
14. candidate dispositions remain in Component 06 candidate order with canonical relation/seed ranges.

Dependencies store final IDs after remap. Reverse consumers sort by full key.

### 21.4 Producer invariant checks

Before encoding, reconstruct and verify:

- unique strictly ordered owner-free keys;
- one producer per request and duplicate consumers;
- complete acyclic family-valid graph;
- all rounded nominal/enclosure/bounded-sign/exact-relation/disposition combinations;
- rounded zero not used as exact eligibility;
- semantic source-polygon boundaries and geometry basis;
- internal diagonals own no public semantics;
- one finite valid construction per key;
- complete triangle-local/candidate coverage;
- crossing orientation and local conservation;
- symbolic eligibility, exact rule lookup, coordinate preservation, and operand remap;
- absence of final selection fields or service calls;
- event lineage and distinct occurrences;
- owner-token exclusion from semantic content;
- counts/ranges/resources; and
- no task-local/private handle escape.

## 22. Canonical encoding, diagnostics, replay, and decode

### 22.1 Canonical encoding

Use Component 01 `CanonicalBytes`. Encode framed sections for:

- semantic header, versions, operation, policies, predecessor identities/digests;
- owner-free relation family/key tables;
- rounded operations and Component 03 traces;
- exact-relation records and formulas;
- composite truth records;
- request/dependency/reverse-consumer graph;
- region, edge-edge, edge-facet, facet-facet, coplanar, and composite records;
- construction and interval records;
- crossing and conservation evidence;
- symbolic eligibility/decision and matrix references;
- seeds, incidences, occurrence keys;
- candidate dispositions/mappings;
- partitions, statistics, resources, verifier evidence, and replay references.

Never serialize raw object memory, padding, pointers, runtime owner tokens, `size_t`, container capacity, locale text, compiler names, or hash values. Exact `T` bits preserve signed zero.

Compute domain-separated SHA-256 digests for keys/graph, bounded operations, exact relations, composites, constructions, multiplicity, symbolic evidence, seeds/dispositions, statistics/evidence, and complete artifact.

### 22.2 Diagnostics

Failures and retained findings include bounded stable fields:

- component/stage/checkpoint/subcode;
- candidate/relation/source-feature semantic keys and roles;
- rounded bits, enclosures, margins, exact-relation status;
- operation/exact formula and trace IDs;
- region/parameter/residual/conditioning evidence;
- source semantic/exact-triangulation lineage;
- crossing contributors and symbolic rule evidence;
- selection-boundary violation evidence where applicable;
- resource limits/counters and cancellation progress; and
- replay identity.

Core relation code does not log. Rendering occurs at API boundary. Raw owner tokens, pointers, build paths, and thread IDs are never canonical diagnostics.

### 22.3 Replay

Replay reproduces request discovery/closure, keys, graph order, rounded traces, exact-relation records, truth dispositions, region classes, relations, constructions, overlays, multiplicities, symbolic evidence, seeds, dispositions, IDs, bytes, digests, counters, and primary failure. Worker count, schedule, and runtime owner anchor are nonsemantic.

### 22.4 Decode

Decode privately and fail closed. Validate tags, lengths, versions, counts, resource limits, semantic owners/digests, reserved fields, exact bit patterns, and explicit absence of runtime owner-token/final-selection fields before allocation/publication. Reconstruct tables and rerun the independent verifier. Reject trailing bytes, unknown required fields, duplicate singleton sections, task-local IDs, or forbidden tags.

## 23. Independent verifier

### 23.1 Independence requirements

`RelationVerifier` consumes immutable Component 07 records plus narrow predecessor and Component 03 capabilities. It must not call producer request closure, relation cache, source-facet assembler, multiplicity reducer, symbolic dispatcher, seed deduplicator, or stored summary booleans as sole truth.

It may share schemas, complete-key comparators, Component 01 checked arithmetic/canonical bytes/SHA-256, Component 03 public bounded/exact capabilities, and pure polygon primitives, but uses independent traversal and dispatch control flow.

### 23.2 Required reconstruction

The verifier must:

1. validate runtime owner access and independently confirm owner-token exclusion from semantic bytes;
2. rescan every candidate and reconstruct initial request keys;
3. independently derive finite dependency closure and compare key/edge sets;
4. detect cycles without trusting producer ordinals;
5. reconstruct every rounded operation record from Component 03 inputs/traces;
6. reconstruct every exact-relation record from formula/input references;
7. validate all orthogonal truth combinations and dispositions;
8. classify point/facet regions from original boundaries and geometry basis;
9. reconstruct edge-edge and edge-facet categories, parameters, ownership, and coverage;
10. reconstruct facet-facet support, carriers, coplanar overlay, containment/equality, orientation, and sheet occurrences;
11. verify authoritative construction and witness residual/conditioning compatibility;
12. independently sum source-fan numeric crossings and reversal;
13. reconstruct symbolic eligibility and exact matrix lookup;
14. verify symbolic side/order/rank/occurrence output and coordinate preservation;
15. prove no final selection state or truth-table result is present;
16. reconstruct event equivalence, distinct occurrence, and incidence;
17. compare candidate dispositions;
18. validate owners, ranges, versions, reserved fields, resources, ordering; and
19. re-encode semantic records and compare digests.

For bounded `exhaustive_test_only` fixtures, compare complete source-feature relations with independent exact rational and exhaustive source-feature enumeration. Production never links the oracle.

### 23.3 Mutation rejection

Required mutations include deleting/duplicating/reordering requests, producers, dependencies, consumers, dispositions, relations, constructions, crossings, symbolic records, or seeds; introducing cycles; changing rounded bits/enclosure/bounded sign/exact relation/disposition; substituting rounded zero for exact evidence; assigning source ownership to an internal diagonal; changing semantic digests; splitting/merging discoveries; shrinking bounds; changing parameters/residuals/conditioning; flipping crossing; changing half-open owner; applying symbolic policy to uncertainty; changing side/order/rank/occurrence; adding final selection or output orientation; changing nominal coordinate under symbolic policy; merging seeds by coordinate; corrupting incidence/order/versions/resources/bytes/digests; and inserting runtime owner-token-derived semantics. Reject all deterministically.

## 24. Tests and qualification

### 24.1 Truth-layer known answers

Cover every valid combination of rounded nominal sign/zero, bounded-sign state, exact-relation state, and disposition for support residual, projected orientation, segment relation, parameter comparison, interval order, parallelism, coplanarity, and residual checks.

Include signed zero, subnormals, adjacent values, cancellation, extreme exponents, large translations, and denominator intervals containing zero. Commit goldens for rounded zero/exact nonzero, rounded nonzero/exact zero, exact zero/sign-spanning uncertainty, exact unavailable/uncertain, and definite bounded sign.

### 24.2 Region known answers

Cover convex/concave facets; interior, exterior, original edge, original vertex, internal diagonal only; collinear boundary intervals; multiple inside intervals; and exact half-open vertex crossings. Verify ring rotation, source permutation, coherent geometry-basis use, and legal retriangulation invariance.

### 24.3 Relation matrix fixtures

Commit complete golden artifacts for separation, proper/endpoint edge-facet crossing, source-edge/source-vertex boundary crossing, edge-edge crossing, endpoint contacts, tangencies, skew/parallel edges, collinear point/overlap/containment/equality, coplanar disjoint/point/edge/area overlap, facet containment/equality, same/opposite coincidence, distinct coincident sheets, duplicate triangle discovery, concave multi-events, equal nominal coordinates, and empty streams.

Goldens include complete keys, all truth layers, dependencies, construction bits/enclosures, parameters, residuals, multiplicities, symbolic side/order/rank, occurrence constraints, seeds, dispositions, and digests.

### 24.4 Symbolic matrix and selection-boundary qualification

Exhaustively enumerate every valid Component 01 symbolic rule key for all operations and operand roles. Test equal operands; point/edge/face touching; vertex-on-vertex/edge/face; edge-on-face; equal/overlapping edges; same/opposite coincident facets; half-open ownership; and operand exchange.

Prove totality, unique lookup, exact eligibility, coordinate-bit preservation, side/order/rank output, and occurrence separation.

Instrument the final Component 01 truth-table selection service and require zero Component 07 calls. Run Component 07 artifacts through Component 09/10 test doubles and require final retention/orientation to arise only downstream. Mutated final-selection fields must fail decode/verifier.

### 24.5 Owner semantics qualification

Run identical semantic inputs under different runtime owner anchors. Require identical semantic IDs, bytes, digests, replay-equivalence records, and primary errors. Wrong/stale/cross-owner handles fail before dereference. Inject owner-token bits into keys, diagnostics, codec sections, or digest input and require rejection.

### 24.6 Exact rational differential oracle

Use Component 16's test-only arbitrary-precision signed integer/rational infrastructure. For bounded integer fixtures compare exact support/orientation, edge-edge/edge-facet categories, parameters, source-polygon ownership, coplanarity/orientation/overlap, numeric crossing, and event incidence. Verify exact points lie in published enclosures. The oracle shares no producer grouping or formula dispatch and is not linked into production.

### 24.7 Compute-once and dependency tests

Instrument requests and verify one producer for repeated support, exact-relation, source-facet composite, construction, multiplicity, and symbolic questions; original-source ownership reduction; family-valid dependencies; rejection of conflicting duplicate results; candidate permutation invariance; and retriangulation changes only bookkeeping.

### 24.8 Crossing conservation tests

Construct high-valence vertices and shared edges. Verify unique half-open owner, tangent zero, entering/leaving signs, reversal, shell orientation remap, closed-fan conservation, retriangulation invariance, and exact-oracle agreement.

### 24.9 Conditioning boundaries

Test edge-plane, line-line, facet-carrier, projected-region, and parameter-order cases comfortably conditioned, at tolerance boundaries, exact parallel/coplanar zero, zero-containing denominator without exact evidence, large translation/small geometry, subnormal scale, and maximum finite values. Category, formula, bounds, residuals, and failure are deterministic.

### 24.10 Metamorphic and determinism tests

Apply operand exchange/remapping; source permutations; ring rotation; legal subdivision/retriangulation; axis permutation; sign flip with corrected orientation; exactly representable translation; power-of-two scaling with precision scaling; edge-direction reversal; repeated runs; owner-anchor changes; thread counts; forced delays; and reversed merge.

After remapping, semantic keys, truth layers, relations, constructions, multiplicities, symbolic evidence, seeds, dispositions, diagnostics, and digests are byte-identical.

### 24.11 Fuzzing and shrinking

Generate valid exact-template manifolds varying valence, concavity/triangulation, shells/nesting, overlap dimension/orientation, coordinate duplication, ULP perturbations, near-parallel angles, translation, scale, inherited precision, tolerance, and symbolic categories. Compare bounded fixtures with exhaustive candidate/source-feature and exact rational oracles.

Every crash, nondeterminism, oracle disagreement, truth-layer conflation, invalid eligibility, duplicate conflict, bad crossing, triangulation dependence, owner semantic leak, or selection-boundary violation serializes exact inputs, predecessor digests, candidate stream, policies, traces, and replay. Shrink preserving canonical failure category and witness.

### 24.12 Resource, cancellation, and concurrency tests

For every Section 9 resource class test limit-minus-one, limit, and limit-plus-one. Confirm no truncation, exact primary witness, no partial artifact, and complete lease reconciliation.

Cancel during validation, discovery, closure, each operation/relation family, construction, overlay, multiplicity, symbolic lookup, boundary audit, seed generation, canonical merge, codec, and verifier. Confirm all workers join, reservations return, no artifact publishes, and retry produces canonical bytes.

### 24.13 Structural performance gates

Use deterministic counters. Require:

- candidate scan linear in candidate count plus request proposals;
- request grouping within documented `O(R log R)` comparison growth;
- one evaluation per unique key;
- polygon work proportional to queried boundary size or qualified deterministic index candidates;
- edge/facet and coplanar work output-sensitive with explicit worst-case quadratic guards;
- no ordinary sparse fixture evaluating all cross-operand feature pairs;
- memory proportional to requests, dependencies, relations, constructions, consumers, seeds, and verifier evidence; and
- exhaustive source-feature work only in test mode or documented worst-case overlap.

Counter ceilings are architecture-independent and require reviewed justification to change.

## 25. Implementation sequence and handoff gates

Implement in this dependency order:

1. register versions, truth-layer enums, IDs, checkpoints, subcodes, resource kinds, and strict-target files;
2. define owner-free complete keys, comparators, operand remaps, and golden encodings;
3. define immutable artifact/query views with runtime owner checking separated from semantics;
4. implement predecessor validation and count/resource preflight;
5. implement serial initial request discovery and dispositions;
6. implement deterministic sort/group, dependency closure, and cycle detection;
7. expose missing Component 03 rounded/exact formulas through reviewed capability extensions and tests;
8. extend pure source-polygon kernel and prove Component 02/04 regressions;
9. implement bounded primitive and exact-relation records plus orthogonal dispositions;
10. implement complete point/segment source-facet region classification;
11. implement canonical original-source-edge/source-edge relations and constructions;
12. implement edge-facet transverse/endpoint/tangent/coplanar composites;
13. implement facet-facet support and carrier construction;
14. implement coplanar overlay, containment, equality, and sheet occurrences;
15. implement authoritative construction selection and witness compatibility;
16. implement numeric crossing multiplicity, source-fan ownership, and conservation;
17. implement symbolic eligibility and total matrix lookup;
18. implement symbolic side/order/rank/occurrence output and selection-boundary audit;
19. implement event seeds, incidence, distinct occurrences, and dispositions;
20. implement canonical IDs, dependency remap, reverse maps, partitions, and producer checks;
21. implement owner-free codec, digests, diagnostics, and replay;
22. implement independent graph/truth/relation/construction/multiplicity/symbolic/seed/boundary verifier;
23. integrate shared bounded exhaustive and exact-rational qualification infrastructure;
24. add known-answer, symbolic, boundary, owner, compute-once, retriangulation, mutation, metamorphic, adversarial, resource, cancellation, fuzz, and performance suites;
25. expose private parallel boundaries and prove serial equivalence under Component 17;
26. run all supported scalar/index profiles and strict floating configurations;
27. verify Component 08 can consume seeds/constructions/incidence without recomputing geometry;
28. verify Component 09 can derive complete numeric/symbolic side labels;
29. verify Component 10 alone performs final truth-table selection and orientation; and
30. retain Section 26 as the future implementation acceptance gate.

The planning tracker checkbox is updated when this reviewed document is committed, not after these implementation steps execute.

## 26. Definition of done

Component 07 implementation is complete only when all of the following are true:

- V1 provider, dependency graph, family precedence, complete owner-free key domains, formula selection, truth-layer policy, and public/bookkeeping reduction are frozen and versioned;
- all Component 01-06 owners, versions, digests, capabilities, and predecessor invariants are validated before authoritative work;
- runtime owner tokens are validated but absent from semantic identity, bytes, digests, replay-equivalence, and failure ordering;
- every Component 06 candidate receives exactly one verified final disposition;
- every topology-affecting primitive/composite question has one producer and immutable value;
- duplicate requests are consumers and conflicts are rejected;
- the dependency graph is complete, acyclic, canonically ordered, serializable, and independently reconstructed;
- all authoritative arithmetic comes through Component 03 bounded and exact-relation services under strict C++17 floating rules;
- rounded nominal bits, bounded sign, exact stored-coordinate relation, uncertainty, and consumer disposition remain distinct;
- rounded zero never authorizes symbolic handling;
- exact relation zero is used only with structural and tolerance eligibility;
- source-facet support and polygon semantics are independent of internal triangulation;
- internal diagonals never own public contact, symbolic priority, crossing barriers, event equivalence, or retained surfaces;
- edge-edge relations cover proper, endpoint, parallel, collinear, overlap, containment, equality, uncertain, and invalid cases with bounded parameters/residuals;
- edge-facet relations cover transverse, endpoint, tangent, coplanar, multi-crossing, overlap, containment, and no-contact cases over the full facet;
- facet-facet relations distinguish stable carrier, parallel separation, exact coplanarity, orientation, uncertainty, and invalid construction;
- coplanar overlays use original boundaries, preserve overlap components and distinct sheets, and prove containment/equality without proximity or area shortcuts;
- every point, parameter, interval, and carrier has one authoritative finite bounded construction with complete precision/residual/conditioning/tolerance/lineage evidence;
- every nonzero numeric crossing has frozen orientation and signed multiplicity;
- source-fan half-open ownership is deterministic, source-feature based, triangulation-independent, and locally conservative;
- tangencies and zero-measure contacts remain explicit;
- Component 01 symbolic matrix is total and every lookup is recorded by stable rule ID;
- symbolic decisions publish only conceptual side/order, half-open owner, owner rank/eligibility, crossing metadata, and occurrence separation;
- no Component 07 record or code path performs final positive-area retain/discard/suppress/output-orientation selection;
- Component 09 derives side labels and Component 10 alone applies final truth-table selection;
- operand exchange produces documented remapped numeric relations, symbolic evidence, seeds, diagnostics, and digests;
- event seeds are keyed by exact relation/source lineage, never coordinates, bounds, nominal parameters, hashes, or spatial cells;
- distinct occurrences remain separate despite identical nominal coordinates and enclosures;
- Components 08 and 09 consume the artifact without repeating authoritative geometry;
- producer checks reconstruct coverage, dependencies, truth layers, constructions, multiplicity, symbolic eligibility, incidence, ranges, resources, owner exclusion, and downstream boundary;
- the independent verifier uses separate control flow and rejects every required mutation;
- exact-rational, conservation, symbolic, selection-boundary, owner-anchor, retriangulation, conditioning, metamorphic, replay, fuzz/shrink, resource/cancellation, and performance tests pass;
- serial and every supported deterministic parallel schedule produce byte-identical semantic artifacts and primary failures;
- no failed, cancelled, over-limit, uncertain, partially encoded, or verifier-rejected artifact publishes;
- canonical bytes, section digests, complete digest, diagnostics, and replay are deterministic and versioned; and
- production and normative-test code is strict portable C++17, self-contained within Ygor, and uses no external dependency.
