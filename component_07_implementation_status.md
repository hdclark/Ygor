# Component 07 implementation status

## Implemented relation-graph foundation

The foundation establishes the fail-closed first implementation slice required by
`plan_07_floating_relation_kernel_symbolic_perturbation.md`:

- frozen Component 07 provider, graph, truth, key, codec, verifier, stage,
  checkpoint, strong-ID, and typed-error domains;
- complete owner-free source-feature request and event-seed key encodings with
  operand remapping and explicit rejection of public internal-diagonal ownership;
- deterministic sort/group/deduplicate request publication, fixed family
  precedence, closed dependency validation, reverse-consumer ranges, candidate
  witness union, canonical IDs, semantic bytes, and semantic digest;
- independent request-graph verification and mutation-sensitive range,
  dependency, identity, ordering, and digest checks;
- exact-tie symbolic eligibility and unique lookup in the frozen Component 01
  symbolic matrix, publishing only side/order/rank/occurrence evidence and never
  final Boolean selection or output orientation;
- immutable `signed_feature_relations<T,I>` foundation schema, owner-checked
  queries, owner-free canonical encoding, section handshakes, statistics, and an
  independent verifier that checks runtime-owner exclusion from semantic bytes;
- transactional Component 7 entrypoint for the completely classified empty
  candidate stream; and
- explicit typed fail-closed rejection for every non-empty candidate stream
  until the remaining numerical relation families are implemented.

## Implemented primitive truth and point-region slice

The next Plan 07 slice is now implemented:

- Component 03 exposes `valid_predicate_result` so downstream consumers can
  independently reconstruct and validate predicate schema, owner binding,
  bounded sign, disposition, width, margin, uncertainty contributors, and trace
  identity without reaching into Component 03 implementation details;
- `PrimitiveRelationKernel` adapts Component 03 bounded scalar and exact-relation
  evidence without collapsing the four independent truth layers: rounded nominal
  bits, bounded sign, exact stored-coordinate relation, and consumer disposition;
- formula registration, owner compatibility, exact-evaluation status, expansion
  capacity, bounded-sign reconstruction, exact/bounded consistency, reserved
  fields, and disposition invariants are validated fail closed;
- an unavailable exact relation is representable only when no exact formula was
  requested, and rounded nominal zero is never converted into exact-tie evidence;
- `SourceFacetRegionKernel` classifies a point against the complete original
  source-facet boundary with a fixed Y-axis lower-inclusive/upper-exclusive
  parity rule and one retained orientation record per boundary edge;
- the complete polygon orientation is independently reconstructed and required
  to agree with the frozen source-facet orientation supplied by the predecessor;
- original source-edge and source-vertex ownership is retained canonically,
  including both incident boundary edges at a source vertex;
- source identity that disagrees with accepted projected geometry is rejected;
- inherited uncertainty that can change boundary, parity, or region semantics is
  rejected rather than guessed; and
- independent record-invariant checks and focused strict C++17 tests cover exact
  truth-layer preservation, formula/evaluation failures, mutated predicate
  evidence, convex and concave polygons, reversed ring orientation, frozen
  orientation mismatch, edge/vertex ownership, and uncertain boundary rejection.

No new low-level numerical formula was embedded in Component 07. The primitive
adapter consumes the registered Component 03 truth services, and the point-region
classifier consumes `BoundedSourcePolygonKernel::orientation` and its exact
stored-coordinate relation evidence.

## Implemented source-facet segment partition and reconciliation slice

The next coherent Plan 07 slice is also implemented:

- source-facet segment partitioning consumes a complete set of canonical
  original-boundary contact constructions and explicitly does not recompute
  source-edge/source-edge predicates inside the polygon kernel;
- the closed query-segment parameter domain is partitioned by point contacts and
  boundary-overlap endpoints while retaining authoritative contact lineage,
  bounded parameter enclosures, projected constructions, source-vertex owners,
  and original source-edge owners;
- canonical breakpoint order is accepted only when complete parameter
  enclosures prove strict separation, or when exact singleton parameters and
  projected constructions are identical; overlapping unresolved enclosures fail
  closed with a typed Component 07 error rather than being nominally sorted;
- every open parameter cell receives a deterministic certified dyadic witness
  from a fixed finite sequence, and the witness is reclassified against the
  complete original source polygon as interior, outside, or an explicit
  original-edge overlap;
- overlap endpoints, zero-length point contacts, endpoint masks, complete
  boundary traversal, and original-boundary ownership are preserved in typed
  partition records with mutation-sensitive independent invariants;
- triangle-local witnesses are absorbed only into matching public breakpoints or
  intervals, while internal diagonals are restricted to bookkeeping-only
  witnesses and are forbidden from becoming public owners;
- canonical semantic bytes and the semantic digest deliberately exclude
  triangle-local triangulation bookkeeping, while retaining the full public
  source-feature partition; and
- focused strict C++17 qualification covers transverse partitions, boundary
  overlaps, unresolved parameter order, digest/invariant mutation, illegal
  internal-diagonal ownership, and equal public semantics under distinct legal
  triangulations.

The segment partition implementation is decomposed into private Component 07
headers for records, canonicalization, validation, and construction. The strict
Component 07 translation unit includes the complete provider, and the public
point-region header and behavior remain source compatible.

## Implemented canonical source-edge/source-edge numerical slice

The Plan 07 source-edge/source-edge numerical kernel is now implemented as a
self-contained, fail-closed provider:

- exact stored-coordinate bindings classify direction parallelism, point/line
  collinearity, and four-point coplanarity without treating rounded differences
  or rounded zero as exact evidence;
- only canonical, opposite-operand, original source-edge pairs are accepted;
  internal triangulation diagonals cannot publish a public source-edge relation;
- both source directions must have definitely positive bounded squared norms;
  degenerate or uncertainty-overlapping directions fail with stable typed errors;
- nonparallel support is split into skew separation and exact coplanar ties using
  independent bounded and exact truth records;
- the solve plane is selected by the greatest definitely-positive squared lower
  bound of the cross-product component, with deterministic X/Y/Z tie order;
- the fixed 2x2 solve retains bounded parameter enclosures, exact zero/one
  relations, domain margins, uncertainty contributors, and trace roots;
- proper crossings, endpoint crossings, and non-contact solves are distinguished
  without nominal fallback when a parameter enclosure overlaps a topology
  boundary;
- accepted source vertices are reused exactly, otherwise one bounded carrier
  construction is produced and independently checked against both source edges;
- parallel support is split into separated and exactly collinear cases, then the
  dominant stable source-edge axis is used for a complete one-dimensional
  interval classification;
- disjoint, point contact, partial overlap, first-contains-second,
  second-contains-first, equal-same-orientation, and equal-opposite-orientation
  cases retain canonical endpoint parameters, source ownership masks, overlap
  constructions, and orientation evidence;
- every published construction retains per-axis residual intervals against both
  carriers and is rejected unless zero is enclosed within the configured
  residual boundary;
- canonical semantic bytes, semantic digests, reserved-field checks, category
  invariants, parameter invariants, construction invariants, and mutation-sensitive
  validation are provided; and
- a strict-FP C++17 qualification target covers float and double known-answer
  cases for every support/contact category, exact-binding checks, wrong-owner and
  internal-diagonal rejection, record mutation, and unresolved-direction failure.

## Implemented candidate-derived source-edge request integration slice

The standalone source-edge/source-edge provider is now connected to the
non-empty transactional path without permitting a partial Component 07 artifact:

- every canonical broad-phase edge/source-triangle candidate is validated against
  the immutable primitive tables and canonical manifold predecessor;
- public candidate source edges are paired with every original source edge on the
  complete opposite source-facet boundary, while facet-internal diagonals are
  excluded from public ownership and produce no public source-edge request;
- candidate discoveries are emitted as canonical request proposals, then sorted,
  grouped, and deduplicated by the existing relation request graph so duplicate
  discoveries share exactly one numerical producer;
- canonical manifold vertex bounds are imported as owner-bound bounded points
  with deterministic provenance, lineage, uncertainty contributors, and stable
  source-edge feature keys;
- every unique source-edge/source-edge request is evaluated exactly once by the
  qualified numerical kernel, and each candidate receives a deterministic range
  of all relations discovered from its complete opposite source-facet boundary;
- owner-free integration semantics cover the graph, detailed numerical records,
  candidate coverage ranges, evaluation count, and a mutation-sensitive digest;
- an independent verifier reconstructs candidate proposals, rebuilds the request
  graph, reimports endpoint geometry, re-evaluates every numerical relation, and
  checks complete candidate coverage, canonical ranges, digest integrity, and
  runtime-owner exclusion; and
- a focused C++17 qualification target covers overlapping and separated fixtures,
  compute-once producer sharing, record mutation rejection, independent
  reconstruction, owner exclusion, and the non-empty no-partial-publication gate.

The candidate-derived source-edge stage now supplies the complete original
source-facet boundary dependency set required by the source-edge/source-facet
composite provider. Internal triangulation diagonals are filtered by predecessor
semantics rather than by a potentially colliding `(primary, secondary)` feature
pair, and reconstruction locates original-edge primitives by their canonical
manifold-edge identity instead of assuming that semantic IDs are vector indices.


## Implemented source-edge/source-facet numerical and candidate slice

The next coherent Plan 07 slice implements the source-edge/source-facet
composite through the first unavailable source-facet/source-facet handoff:

- each candidate source edge is paired with the complete opposite original
  source-facet boundary and references every canonical source-edge/source-edge
  dependency used by the composite relation;
- facet boundary halfedges are reconstructed into the unique directed source
  polygon cycle from predecessor source-vertex order; the canonically sorted
  feature-group storage order is never mistaken for polygon order;
- endpoint support truth retains rounded residual bits, bounded sign, exact
  stored-coordinate relation, disposition, trace, and owner binding; same-side,
  opposite-side, one-endpoint support ties, and exact coplanar support are
  distinguished fail closed;
- opposite-side support constructs one bounded edge/plane parameter and point,
  checks edge-carrier and plane residuals against the authorized boundary, and
  classifies the result against the complete semantic source polygon;
- exact endpoint support ties reuse the accepted source endpoint and distinguish
  endpoint contact from zero-multiplicity tangency; source-fan ownership remains
  explicit and boundary crossings do not claim a numeric transition before the
  complete fan rule is available;
- exact coplanar support invokes the complete source-polygon segment partition,
  preserving point contacts, source-boundary overlaps, containment, original
  source ownership, and every canonical boundary relation request;
- local event order is accepted only from proven parameter separation, event
  records receive deterministic occurrence values after order is established,
  and unsupported overlapping multi-event order fails closed;
- owner-free canonical bytes and semantic digests cover the composite records,
  complete boundary dependencies, candidate ranges, request graph, and exact
  evaluation count;
- an independent verifier reconstructs the candidate proposals, directed facet
  boundary cycle, support input, complete dependency bindings, numerical
  relations, candidate ranges, digest, and runtime-owner exclusion; and
- relation preflight now accounts for the maximum complete source-facet boundary
  in request, dependency, work, and temporary-memory upper bounds rather than a
  fixed triangle-edge multiplier.

The shared source-facet segment provider was also hardened for canonical
`[-0,+0]` exact-zero intervals. Source-region exact-point comparisons now treat
that representation as one numerical point, coincident endpoint/contact seeds
can merge canonically, endpoint validation accepts the canonical signed-zero
pair, and projected interpolation uses the same convex-combination operation
shape for nominal and enclosure construction. Qualification fixtures now use a
conservative parameter enclosure when a decimal binary value represents an
exact rational contact such as one-third or two-thirds. The source-edge
qualification also derives a precision-appropriate residual boundary for
`float` containment cases instead of requesting a sub-ULP tolerance.

`RelationBuild` now computes and independently verifies both the complete
source-edge/source-edge stage and this source-edge/source-facet stage for every
non-empty candidate stream. It then fails closed at the next unavailable family,
`source_facet_source_facet`; both verified transaction-local stages are discarded
and no incomplete `signed_feature_relations` artifact is published.

Focused validation executed successfully in the exported PR workspace:

- strict Component 07 library build, including the new provider translation unit;
- compile-only instantiation of candidate integration and the publication gate;
- float/double source-edge/source-facet kernel qualification covering same-side,
  transverse interior/outside/boundary, endpoint, tangent, coplanar point,
  containment, boundary overlap, malformed dependency, ownership, and mutation
  cases;
- candidate-stream source-edge/source-facet integration, compute-once sharing,
  complete boundary coverage, independent reconstruction, preflight bounds,
  owner exclusion, mutation rejection, empty-stage handling, and the
  source-facet/source-facet fail-closed gate;
- candidate-derived source-edge integration after complete-facet expansion;
- the source-facet point/segment region qualification, including signed-zero
  endpoint handling and conservative rational-contact parameter enclosures; and
- the float/double source-edge known-answer suite with a scalar-precision-aware
  residual budget.


## Implemented source-facet/source-facet support and carrier slice

The next Plan 07 handoff gate is now implemented through stable support
classification and transverse carrier construction:

- each canonical broad-phase candidate induces the complete set of opposite-
  operand source-facet/source-facet support requests for the candidate source
  edge's incident source facets and the opposite source-triangle facet;
- duplicate discoveries are grouped by the canonical request graph, every
  unique support pair has exactly one numerical producer, and every candidate
  receives a deterministic complete range of the relations it witnesses;
- immutable support inputs retain the three accepted source points, accepted
  unnormalized bounded plane, source facet/ring/shell/material-side identity,
  frozen projection axis, coherent-basis digest, and relevant edge/facet
  consumer requests;
- Component 03's exact-formula capability is advanced and extended with stable
  formula IDs for unnormalized support-normal parallelism, support-normal dot
  orientation, and oriented plane-point residual, including independent replay
  reconstruction with the same sign convention as the bounded plane residual;
- bounded normal cross products and exact stored-coordinate normal relations
  distinguish definitely nonparallel support, eligible exact parallel support,
  unresolved support, and invalid or degenerate supports without approximate
  normal similarity;
- exact parallel support evaluates an accepted opposite anchor and distinguishes
  parallel separation from eligible exact coplanarity, then determines same or
  opposite support orientation from exact normal-dot evidence while preserving
  shell occupied-side semantics separately;
- definitely nonparallel support constructs exactly one unnormalized carrier,
  with direction ordered from canonical semantic feature keys and a fixed
  bounded closed-form point construction;
- carrier direction conditioning, point-on-both-planes residuals, and
  direction-orthogonality residuals are retained and checked against the
  authorized tolerance boundary before publication;
- owner-free canonical bytes and semantic digests cover detailed support truth,
  carrier construction, consumer links, graph, candidate ranges, and exact
  evaluation count; runtime owner identity is independently proven absent from
  semantic bytes;
- an independent verifier rebuilds proposals, request grouping, support inputs,
  exact and bounded support classification, carrier construction, consumer
  links, candidate coverage, digest, and owner exclusion; and
- relation preflight now reserves the candidate-derived facet/facet requests and
  their deterministic work and storage bounds in addition to the complete
  edge-edge and edge-facet dependency families.

`RelationBuild` now computes and independently verifies source-edge/source-edge,
source-edge/source-facet, and source-facet/source-facet support stages for every
non-empty candidate stream. It fails closed at
`coplanar_overlay_evaluation`; all transaction-local stages are discarded and no
partial `signed_feature_relations` artifact is published.

Focused validation in the exported PR workspace includes the strict C++17
library, compile-only full candidate-stage instantiation, float/double known
answers for transverse, parallel-separated, coplanar-same, and
coplanar-opposite supports, exact formula replay, carrier residual and
conditioning checks, malformed support/owner rejection, owner-free semantics,
mutation rejection, compute-once grouping, deterministic candidate ranges, and
independent stage reconstruction.

## Implemented coplanar facet overlay classification slice

The next Plan 07 numerical slice now provides a self-contained coplanar
source-facet overlay classifier without advancing final Boolean selection:

- the provider accepts exactly one already-qualified coplanar facet/facet support
  relation, two complete original source-polygon boundaries in one projection,
  and a complete canonical matrix of existing source-edge/source-edge relation
  records; it does not recompute boundary intersections or infer identity from
  coordinates;
- every source vertex is independently classified against the complete opposite
  semantic polygon using the Component 07 source-facet region kernel, retaining
  complete boundary traversal and original-boundary ownership evidence;
- every cross-operand boundary-edge pair is required exactly once, validated
  against its canonical source-edge features and runtime owner, and retained as
  compute-once dependency evidence; missing, duplicated, or malformed pairs fail
  closed;
- canonical classification distinguishes disjoint facets, point-only contact,
  segment contact, positive-area overlap, directed containment, equal
  same-orientation sheets, and equal opposite-orientation sheets;
- equality requires complete boundary coverage proven from authoritative overlap
  parameter intervals, rather than matching vertex counts, coordinates, or
  triangulations; unresolved interval coverage fails closed;
- coordinate-coincident equal sheets remain explicitly distinct occurrences, and
  the record contains no retain/discard/suppress decision or output orientation;
- owner-free canonical bytes and semantic digests include complete polygon,
  vertex-region, boundary-relation, contact, classification, and occurrence
  evidence while excluding runtime owner tokens;
- structural validation rejects malformed ranges, contact projections, ownership,
  counts, reserved fields, and digest mutations, while reconstruction verification
  reruns the complete classification and rejects a forged classification even
  after its digest is recomputed; and
- focused strict C++17 float/double qualification covers disjoint, point, segment,
  crossing-area, containment, equal-same, equal-opposite, missing dependency,
  wrong-owner, digest, forged-classification, and owner-exclusion cases.

The standalone classifier was initially kept behind the non-empty publication
gate so candidate-derived request closure could be added without accepting a
partial artifact.

## Implemented candidate-derived coplanar overlay integration slice

The classifier is now connected to the transaction-local candidate pipeline:

- source-edge request discovery closes every candidate-derived source-facet pair
  over the complete Cartesian product of both original semantic boundaries,
  including both source facets incident to the candidate edge; collision-local
  edge pairs are no longer mistaken for complete overlay dependencies;
- the checked preflight uses the corresponding quadratic boundary-pair bound for
  request, dependency, work, and temporary-byte accounting, with overflow and
  configured-capacity failures before numerical evaluation;
- one canonical coplanar overlay record is produced for every coplanar
  source-facet/source-facet support relation in request order, with a typed link
  back to the compute-once support producer and no record for separated or
  transverse support;
- source polygons and their ordered original boundary-edge identities are
  reconstructed from the immutable canonical manifold and primitive tables,
  while all boundary relations are looked up from the already-verified
  source-edge stage rather than recomputed;
- stage bytes and digest are owner free, the independent verifier reconstructs
  the complete stage from predecessor artifacts, and mutation tests reject
  altered overlay records, links, counts, and digests;
- `RelationBuild` executes and verifies this stage transactionally for every
  candidate stream, then continues to fail closed at
  `coplanar_overlay_evaluation` because canonical overlap-component/oriented-arc
  assembly and the later multiplicity, symbolic, event, codec, and publication
  obligations are not yet complete; and
- the focused runtime targets no longer link the unrelated legacy Ygor geometry
  and Boolean monolith. A test-only carrier translation unit provides the exact
  trivial `vec3<double>` and `fv_surface_mesh<double,uint32_t>` construction and
  copy definitions used by `YgorMath.cc`; every authoritative topology and
  geometry decision remains in the strict bounded provider.

Focused strict validation now passes
`mesh_boolean_bounded_component07_candidate_source_edge_relations`,
`mesh_boolean_bounded_component07_edge_facet_relations`,
`mesh_boolean_bounded_component07_edge_facet_integration`,
`mesh_boolean_bounded_component07_facet_facet_relations`, and
`mesh_boolean_bounded_component07_coplanar_overlay`. The integration suite
covers empty and non-empty streams, deterministic compute-once closure,
owner-free semantics, mutation rejection, fail-closed publication, and a
positive-area coplanar containment case derived from touching closed boxes.


## Implemented complete coplanar boundary-partition slice

The candidate-derived overlay stage now performs the next Plan 07 source-boundary
reduction step before component and arc assembly:

- every original boundary edge of both coplanar source facets is partitioned
  against the complete opposite semantic polygon through the existing certified
  `partition_source_facet_segment` provider; no second point-in-polygon,
  clipping, or tolerance kernel was introduced;
- the partition input is assembled solely from the already verified compute-once
  source-edge/source-edge relation matrix, including point contacts and overlap
  endpoints with authoritative lineage and original-boundary ownership;
- exact source endpoints take canonical precedence at query parameters zero and
  one, preventing independently solved edge-pair parameter enclosures from
  creating ambiguous duplicate endpoint breakpoints while preserving every
  opposite-boundary owner;
- every closed boundary parameter domain is represented by canonical breakpoints
  and every open cell carries the provider's deterministic certified dyadic
  witness classified as interior, outside, or an explicit opposite-boundary
  overlap;
- opposite-operand dense source vertex identifiers are never reinterpreted as
  query-endpoint identity, and projected query geometry is independently checked
  against the source boundary edge before publication;
- the overlay record requires complete sorted one-per-edge partition coverage,
  a common qualified projection axis, reconciled partition invariants, zero
  reserved fields, owner-free semantic bytes, and mutation-sensitive nested and
  outer digests; and
- strict qualification covers disjoint boundaries, point/segment contact,
  transverse positive-area overlap, strict containment, equality, complete
  interval counts, and a matched nested-partition mutation whose outer digest is
  recomputed.

The stage still fails closed before canonical overlap-component and oriented-arc
assembly. The partitions are immutable authoritative input for that next slice;
they do not themselves infer event equivalence, final sheet selection, or output
orientation.

## Implemented canonical coplanar event, arc, and component slice

The complete certified boundary partitions now feed a canonical, lineage-keyed
coplanar topology reduction:

- every relevant partition breakpoint is represented by one source-boundary
  occurrence carrying its exact source-vertex incidence and ordered
  compute-once contact-lineage/endpoint-role evidence;
- event nodes are equivalence classes formed only by exact same-source-vertex
  incidence on one source polygon or by identical relation lineage and endpoint
  role, with transitive closure, compatible projected enclosures, canonical
  occurrence ordering, and no coordinate, proximity, hash, or tolerance key;
- every certified interior or opposite-boundary-overlap interval appears exactly
  once as an oriented boundary-arc occurrence, using the frozen source-facet
  orientation to select traversal direction;
- mirrored shared-boundary intervals are merged into one conceptual arc only
  through their common compute-once source-edge relation lineage, while both
  source-sheet occurrences remain explicit and distinct;
- deterministic connected components distinguish isolated point contact,
  boundary-segment contact, positive-area overlap/containment boundary cycles,
  and coincident same/opposite-orientation sheet cycles without performing final
  Boolean selection or output orientation;
- component closure, node degrees, exact interval coverage, occurrence sheet
  masks, canonical node/arc/component order, and zero reserved fields are
  reconstructed and validated independently of the outer semantic digest;
- matched mutations reject split event-equivalence classes, forged shared
  relation lineage, self-loop arcs, open area/coincident components, missing
  interval coverage, and digest repair attempts;
- equality qualification includes distinct legal source-boundary tessellations,
  proving that conceptual arcs are reduced from exact relation lineage rather
  than matching source-edge counts or coordinates; and
- the candidate-derived transaction stage consumes these records directly and
  continues to fail closed before crossing multiplicity, source-fan ownership,
  symbolic policy, event seeds, final artifact/codec/replay publication, or any
  Component 10 selection.

Focused strict qualification passes the complete five-test predecessor chain for
candidate source-edge closure, source-edge/source-facet relations, candidate
integration, source-facet support, and coplanar overlay topology.

## Implemented canonical final artifact, codec, replay, and resource slice

The verified numerical and coplanar predecessor stages now publish one complete
immutable Component 07 artifact for supported candidate streams:

- the frozen fifteen-family request DAG closes over source relations, bounded
  constructions, crossing multiplicity, symbolic eligibility and decisions,
  event seeds, and candidate dispositions without downstream selection state;
- the final relation, truth, construction, crossing, seed, incidence, and
  candidate-disposition tables receive canonical IDs and owner-free semantic
  encoding, with all detailed predecessor stages retained for audit and replay;
- non-empty artifacts are independently reconstructed from the candidate stream
  and detailed stages, and decode rebuilds the artifact transactionally before
  accepting canonical bytes;
- persistent, replay, temporary, and work usage use checked candidate-scaled
  preflight bounds, deterministic final-size reconciliation, exact resource
  commits, cancellation rollback, and limit-minus-one qualification; and
- matched mutations of dispositions, crossings, source-fan cardinality, codec
  framing, and repaired complete digests are rejected independently.

## Implemented source-fan crossing conservation slice

Boundary contacts now pass through a topology-authoritative fan reduction:

- source-edge groups are required to cover exactly both Component 05 incident
  source facets, while source-vertex groups cover the complete canonical fan;
- local before/after occupancy transitions remain distinct from symbolic
  contributions, tangent groups conserve to zero, and every nonzero numeric
  total has exactly one canonical owning occurrence;
- compute-once boundary relations may resolve a conservative point-region bound
  only through exact original-feature lineage, never coordinate proximity; and
- the independent verifier reconstructs fan grouping, expected incidence, local
  transitions, group totals, owner choice, and cardinality from predecessor
  topology and relation records.

## Implemented evidence-driven symbolic eligibility slice

Symbolic matrix lookup is no longer authorized by a blanket exact-zero stamp:

- each symbolic request carries a stable eligibility reason, evidence formula or
  kernel version, exact-lineage status, inherited uncertainty, tolerance
  compatibility, original-source ownership, and an explicit assertion that
  separated realizations are unavailable under the admitted tie contract;
- eligibility is reconstructed from the authoritative source-edge collinearity
  or coplanarity truth, edge/facet boundary ownership or endpoint support truth,
  facet/facet coplanarity truth, or the coplanar-overlay support relation;
- derived symbolic keys include the authoritative source relation family, so a
  facet support and its overlay may publish distinct decisions for the same
  source-feature pair without key collision;
- the independent verifier finds the exact source and optional construction
  dependencies, reconstructs every eligibility field from predecessor truth,
  and rejects repaired-digest evidence mutations; and
- strict qualification covers exact formula, coplanar-facet-lineage, and
  coincident-overlay categories, plus reasonless exact-zero, rounded-zero-only,
  and potentially separated tie rejection.

## Implemented first-class coplanar topology publication slice

The canonical coplanar reduction is now a first-class part of the immutable
Component 07 artifact rather than only nested audit data:

- every qualified coplanar event node, oriented arc, and overlap component is
  published in a versioned canonical table with a globally sequential strong ID
  assigned in canonical overlay-relation order;
- event records retain the representative authoritative construction, every
  source-boundary occurrence, source-vertex incidence, ordered contact lineage
  and endpoint role, sheet mask, and coordinate-coincident distinct-sheet flag;
- arc records retain globally remapped endpoint nodes, every directed
  source-boundary occurrence, source traversal direction, conceptual arc kind,
  sheet mask, and exact compute-once source-edge producer IDs for shared
  boundary lineage;
- component records retain globally remapped node and arc membership, component
  kind, closure, sheet mask, and distinct-sheet semantics without publishing any
  Boolean selection or output orientation;
- canonical bytes, decode-by-rebuild, section handshakes, statistics, persistent
  byte reconciliation, and nested occurrence/lineage resource accounting cover
  the complete tables;
- the independent verifier reconstructs the overlay key from the exact
  facet/facet support link, then reconstructs every global ID and table field
  from the independently verified detailed overlay stage without coordinate,
  proximity, or feature-pair fallback; and
- strict qualification checks exact table cardinality, byte-identical decode,
  and repaired-codec mutations of node sheet masks, arc endpoints, and component
  closure.

## Implemented canonical primitive-support publication foundation

The final relation graph now exposes the primitive truth lineage that was
previously visible only inside composite relation records:

- every authoritative source feature used by a final relation has one canonical
  imported-geometry producer, with public/bookkeeping scope retained and no
  runtime owner identity in semantic bytes;
- every retained final truth record has one first-class rounded/bounded
  primitive request and record carrying nominal bits, bounded sign, consumer
  disposition, formula version, source relation, and local truth ordinal;
- every requested exact stored-coordinate evaluation has one first-class exact
  producer and record carrying exact status, formula version, source relation,
  and local truth ordinal, while unavailable exact evaluations have no invented
  producer;
- explicit truth-lineage records bind the raw four-layer truth record to its
  bounded producer and optional exact producer, and the authoritative composite
  relation depends on both compute-once primitive requests;
- bounded and exact primitive producers depend on the canonical imported source
  features used by the relation, closing request families 01-03 ahead of the
  existing relation families without coordinate or tolerance identity;
- canonical codec/decode, statistics, checked preflight bounds, exact persistent
  accounting, and independent verification cover all four new tables; and
- repaired-codec mutations of imported features, rounded evidence, exact formula
  identity, and truth lineage are independently rejected.

## Implemented first-class interval and source-facet-region publication slice

The previously qualified bounded enclosures and complete source-polygon
classifications are now canonical family-04 graph producers rather than nested
composite-only audit data:

- every source-edge parameter, carrier residual, edge/facet event parameter,
  support residual, facet/facet conditioning interval, plane residual, segment
  contact parameter, breakpoint parameter, certified open-cell witness, and
  triangle reconciliation witness is published with a stable evidence kind,
  occurrence, component, authoritative source relation, and canonical request;
- parameter records retain rounded bits, complete enclosures, domain status and
  margin, exact zero/one status, all eight uncertainty contributors, and the
  precision trace root, while residual and conditioning records retain the
  exact authorized comparison boundary and independently reconstructed
  acceptance result;
- every edge/facet event, coplanar partition breakpoint/open-cell witness,
  overlay source vertex, and overlay boundary-partition witness publishes its
  query enclosure together with the complete source-facet region record,
  including polygon orientation, full boundary traversal evidence, parity,
  original source-vertex/source-edge owners, and source-identity validity;
- each family-04 producer depends exactly on the imported, bounded, and optional
  exact primitive producers for its authoritative relation, and the relation
  depends on every family-04 record it consumes; no region identity is inferred
  from coordinates, proximity, or vector position;
- versioned owner-free codec/decode sections, table statistics, checked
  candidate-scaled preflight limits, exact persistent accounting for nested
  owner/orientation vectors, and work reconciliation cover both new tables;
- the independent verifier reconstructs every key, occurrence, bit pattern,
  parameter metadata field, residual decision, query enclosure, complete region
  semantic record, and dependency range directly from the four verified
  predecessor stages; and
- focused qualification checks non-empty completeness, canonical decode, exact
  persistent limit-minus-one rollback, and repaired-codec mutations of both an
  interval enclosure and a region query.

## Implemented canonical clustered edge/facet event ordering

The source-edge/source-facet relation stage now completes the Plan 07 ordering
and occurrence slice for admitted multi-event contacts:

- first-class canonical ordered-event and event-cluster tables cover every
  detailed edge/facet event exactly once and are part of the stage semantic
  digest and final relation-artifact contract;
- event order along each canonical query-edge direction is accepted only from
  definitely separated bounded parameter intervals, while overlapping intervals
  fail closed unless exact endpoint or original source-boundary lineage proves an
  admitted equality cluster;
- admitted equal-parameter clusters retain equality and distinguish exact query
  endpoints, common original source vertices, and common original source edges
  without using nominal coordinates, tolerance proximity, candidate traversal
  order, or runtime-owner identity;
- a complete owner-free occurrence tie key records the query edge, exact
  parameter role, original boundary kind and owners, opposite source facet,
  event kind, side transition, numeric crossing, and policy identity;
- every cluster is independently checked to be order-independent for the
  supported contact semantics, tie keys must be unique and strictly canonical,
  and canonical occurrences are assigned only after the cluster partition and
  strict inter-cluster order have been proved;
- relation construction requests, symbolic decisions, source-fan keys, crossing
  publication, event seeds, semantic encoding, and independent final-artifact
  verification now consume the canonical occurrence rather than a
  relation-local event ordinal;
- independent reconstruction validates complete coverage, cluster ranges, exact
  equality lineage, strict inter-cluster order, tie-key bytes, and occurrence
  sequences, including self-consistent repaired-digest mutations; and
- focused qualification covers exact query-endpoint, common source-vertex, and
  shared-source-edge clusters, plus tie-order, occurrence, cluster-range, and
  coordinate-equal-but-lineage-distinct rejection cases.

## Implemented authoritative construction registry and evidence ledger

Construction publication now follows the fixed Plan 07 producer precedence and
retains every secondary witness needed to audit that choice:

- a standalone owner-free policy reconstructs canonical construction authority
  from accepted source-vertex identity, compute-once source-edge/source-edge
  lineage, source-edge/source-facet lineage, coplanar endpoint lineage, and
  facet/facet carrier lineage, in that exact precedence order;
- accepted source vertices are resolved through Component 05 primitive and
  topology tables rather than misinterpreting source-edge feature fields as
  endpoint IDs, and identical source-vertex authorities deduplicate across
  edge/facet and coplanar-overlay relation families;
- every canonical construction publishes one synthetic authority ledger entry
  followed by all producer and verification-witness entries, with coordinate
  space, nominal and enclosure bits, source provenance, geometric lineage,
  tolerance boundary, truth ranges, interval/region evidence ranges, and
  independently reconstructed compatibility dispositions;
- authority/witness compatibility checks require finite ordered bounds, admitted
  tolerance, source-lineage agreement, enclosure containment, parameter and
  residual compatibility, and complete precision evidence without coordinate or
  proximity identity and without unioning contradictory enclosures;
- relation requests, reverse dependencies, consumers, event seeds, codec/decode,
  statistics, preflight capacity, persistent accounting, and verification work
  now include the canonical registry and evidence ledger; event seeds remain
  occurrence-specific even when several witnesses share one construction;
- the independent verifier rebuilds all authority groups and witness records from
  predecessor stages, verifies exact request/dependency closure, rejects missing
  seed-table publication, and does not reuse the mutable assembly registry; and
- focused qualification covers cross-family source-vertex deduplication, repaired
  authority and ledger mutations, missing event-seed publication, exact ledger
  limit-minus-one rollback, byte-identical decode, and the complete thirteen-test
  Component 07 predecessor chain.

## Implemented canonical event-seed provenance and candidate coverage slice

The final artifact now publishes the complete candidate-facing evidence required
by Plan 07 Sections 19 and 20 rather than only a lineage key and a coarse
candidate disposition:

- every event seed retains its contact category and dimension, authoritative
  relation and construction, accepted source-vertex reuse, complete truth and
  construction-ledger ranges, symbolic rule/rank/side evidence, numeric and
  symbolic crossing contributions, half-open owner, precision completeness, and
  independently reconstructed occurrence-separation requirement;
- source incidence remains canonically sorted by complete owner-free feature key,
  while a separate one-per-candidate incidence table records the candidate edge,
  opposite source triangle, both canonical edge halfedge uses, all three triangle
  halfedge uses, internal-diagonal discovery status, and original-source
  ownership without promoting triangle-local identity into event equivalence;
- duplicate seed discoveries merge only when all semantic evidence agrees, must
  retain complete candidate incidence, and remain occurrence-specific when one
  construction authority serves several topological uses;
- each Component 06 candidate publishes exactly one of the seven successful
  dispositions together with complete canonical relation and event-seed ranges,
  explicit coverage flags, the selected operation-neutral public relation when
  present, and its candidate-disposition request producer;
- first-class candidate partition records reproduce every Component 06 canonical
  partition and its candidate, disposition, relation, seed, and maximum-record
  ranges without traversal-order inference;
- codec/decode, semantic bytes, statistics, preflight limits, exact persistent
  accounting, and verifier work include all new incidence, coverage, and
  partition tables;
- an independent strict translation unit reconstructs event-seed request closure,
  truth and precision lineage, symbolic/crossing metadata, coplanar distinct-sheet
  occurrence semantics, every candidate primitive and halfedge incidence,
  candidate coverage/disposition, and partition correspondence directly from
  predecessor artifacts; and
- focused qualification rejects repaired mutations of candidate halfedges,
  relation coverage, partition ranges, and occurrence separation, exercises both
  new limit-minus-one boundaries, and passes the complete thirteen-test Component
  07 predecessor chain under the strict Clang C++17 profile.

## Implemented complete symbolic policy and operand-exchange slice

The reviewed Component 01/07 symbolic contract is now represented by one total,
versioned, operation-specific matrix rather than the earlier four-field subset:

- the pure policy generator materializes all 34,560 combinations of Boolean
  operation, acting operand, relation family, orientation relation, source-feature
  ownership role, half-open endpoint/edge role, transition orientation, and
  occurrence class with direct canonical ordinal lookup and no fallback path;
- every rule publishes conceptual offset/order, symbolic crossing, contact class,
  expected disposition, half-open owner, coincident owner preference, feature
  priority, owner-rank eligibility, occurrence separation, stable explanation,
  and an explicit six-component owner-ranking key description covering final
  orientation capability, operand priority, symbolic feature priority, complete
  canonical source feature, directed use, and occurrence;
- operand exchange remaps the operation, acting operand, ownership role, and
  transition, records the exchanged rule ordinal, is involutive over the complete
  domain, reverses nonzero conceptual order and crossing contribution, and retains
  all exchange-invariant contact and ranking consequences;
- Component 07 symbolic decisions now carry the complete rule key, exact relation,
  event-occurrence or coplanar-component subject, full ranking descriptor,
  conceptual side/order, half-open and crossing consequences, owner rank or
  eligibility, occurrence constraint, explanation, and exchanged rule without
  modifying nominal geometry or performing final Boolean selection;
- edge/edge, edge/facet, facet/facet support, and coplanar overlay discovery build
  the exact matrix key from predecessor evidence; coplanar and coincident subjects
  publish both operand-role decisions, while lower-dimensional contacts retain the
  documented transform for the exchanged invocation without inventing a second
  subject in the current artifact;
- event-seed requests depend on the exact symbolic subject and acting operand,
  removing the former ambiguous occurrence-only lookup, and seeds retain the
  complete symbolic consequence needed by Components 08-10;
- the independent verifier reconstructs every key dimension, subject identity,
  decision request and dependency, complete expected subject population, frozen
  rule, ranking descriptor, seed consequence, and operand-exchange relation from
  predecessor artifacts and the Component 01 policy digest; and
- exhaustive Component 01 qualification covers all 34,560 keys, ordinal and
  exchange round trips, byte/rule mutations, conceptual-order and crossing
  reversal, while Component 07 matched mutations reject repaired ranking-key and
  event-seed consequences and the complete thirteen-test strict Clang C++17 chain
  passes.

## Completed canonical replay and retained diagnostic slice

Component 07 now publishes and independently reconstructs the bounded replay and
machine-readable diagnostic evidence required by Plan 07 Section 22:

- seventeen fixed semantic replay checkpoints cover predecessor validation,
  every relation-family stage, graph/truth/region/construction/crossing/symbolic
  assembly, seed/disposition reconciliation, canonical remap, downstream
  selection-boundary audit, producer verification, canonical encoding, and
  resource reconciliation;
- every checkpoint records versioned input/output counts, cumulative deterministic
  work, a chained semantic digest, and a canonical completed disposition without
  schedule, worker, thread, path, pointer, or runtime-owner data;
- four bounded retained findings record the owner-exclusion,
  selection-boundary, replay-completeness, and resource-reconciliation audits in
  a fixed owner-free schema;
- replay checkpoints, diagnostics, replay evidence, statistics, codec sections,
  exact persistent accounting, decode, and the complete artifact digest are
  published transactionally and rejected on unknown versions, malformed ranges,
  repaired digest corruption, trailing bytes, or inconsistent resource counts;
- the independent replay verifier reconstructs the input-equivalence projection,
  base artifact, every checkpoint count and chain digest, every retained finding,
  and all aggregate replay digests without reusing producer summaries as truth;
  and
- failures expose a domain-separated replay identity derived from the invocation
  replay digest and canonical primary error, while failed and cancelled builds
  publish no partial artifact.

## Completed semantic identity and deterministic execution slice

Invocation-only controls no longer contaminate Component 07 semantic identity:

- the Component 01 context digest used by semantic artifacts is a versioned
  projection that excludes resource ceilings, diagnostic capacity, execution
  mode, worker count, and cancellation services while the complete invocation
  replay digest retains those controls;
- the Component 03 handoff is reduced through a separate versioned relation
  precision projection that retains all scalar, tolerance, scale, source, and
  Boolean arithmetic semantics but excludes Component 03 preflight and
  invocation replay identity;
- identical semantic inputs under distinct runtime owner anchors, serial and
  deterministic-parallel execution modes, and different supported worker counts
  produce byte-identical Component 07 semantic artifacts, section digests,
  retained diagnostics, and replay-equivalence evidence; and
- wrong-owner access remains rejected before dereference through the immutable
  downstream view, while owner tokens remain absent from keys, bytes, digests,
  diagnostics, replay evidence, and failure ordering.

## Completed exact-oracle, campaign, and downstream handoff slice

The permanent Component 07 qualification target now combines the existing
family-specific known-answer, truth-layer, mutation, retriangulation, coplanar,
and symbolic-matrix suites with additional independent end-to-end evidence:

- a test-only arbitrary-precision exact-rational source-edge oracle independently
  enumerates every compute-once source-edge relation in the bounded deterministic
  campaign, classifies support/category/parameters without production grouping
  or formula dispatch, and proves exact results lie inside published enclosures;
- deterministic valid fixtures vary source permutations, owner anchors, execution
  profiles, worker counts, translations, scales, endpoint/parallel/collinear
  categories, and candidate order; disagreements retain a canonical case and a
  deterministic shrink path;
- structural counters gate candidate scan, unique request evaluation, graph
  comparison growth, relation-family work, verifier work, and sparse-fixture
  exclusion of an all-feature Cartesian product;
- the allocation-free, immutable, owner-checked
  `signed_feature_relations_view` exposes only verified Component 07 evidence and
  checked ranges, never predecessor geometry or arithmetic services; and
- Component 08, 09, and 10 test doubles consume only that view: Component 08
  preserves construction/seed lineage, Component 09 derives numeric and symbolic
  side labels, and the instrumented final truth-table service proves Component 10
  alone performs retain/discard and orientation selection.

## Completed resource, cancellation, and publication slice

The final artifact suite now exercises the complete Component 07 publication
boundary deterministically:

- all fifteen count/work capabilities are tested at limit-minus-one, exact limit,
  and limit-plus-one, with exact and plus-one successful publications producing
  identical bytes;
- canonical byte capacity and invocation-owned persistent, temporary, and work
  reservations are tested at their exact boundaries, including exact replay-byte
  commitment and complete lease reconciliation;
- a versioned test-only cancellation observer injects cancellation at every one of
  the twenty-eight published Component 07 phase checkpoints without timing races;
- every injected cancellation reports the exact stable checkpoint and primary
  category, releases every lease, publishes no artifact, and is followed by a
  canonical byte-identical retry; and
- the dependency-closure cancellation path now reports its correct checkpoint
  rather than the initial-grouping checkpoint.

## Component 07 completion evidence

Component 07 is complete against the reviewed Plan 07 contract. All fourteen
strict optimized Clang 17 C++17 Component 07 tests pass, covering source-edge,
candidate/source-edge, edge/facet,
facet/facet, final-artifact, exact-oracle/handoff, foundation graph/owner/
canonical/symbolic, truth-layer, source-facet-region, and coplanar-overlay suites.
The implementation remains self-contained within Ygor and introduces no external
dependency, approximation predicate, tolerance-based welding, coordinate-keyed
identity, or topology inferred from nominal proximity. Unsupported, unresolved,
over-limit, cancelled, decode-invalid, or independently rejected artifacts still
fail closed and never publish.
