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

This slice is deliberately not yet wired into the candidate-derived request
graph or final non-empty relation artifact. `RelationBuild` therefore continues
to fail closed at `coplanar_overlay_evaluation` until complete facet-boundary
request closure, canonical overlay-stage integration, crossing-component
assembly, and the later multiplicity/symbolic/publication obligations are
available.

Focused local validation completed successfully for the strict Component 07
library, `mesh_boolean_bounded_component07_facet_facet_relations`, and
`mesh_boolean_bounded_component07_coplanar_overlay`. The repository's unrelated
legacy public-facade rebuild exceeded the available execution window when broader
targets were requested; no failed Component 07 provider result was observed.

## Outstanding before Component 07 can be checked complete

The tracker remains unchecked. The following Plan 07 work is still required:

1. Replace the composite provider's local support reconstruction with unified
   canonical primitive support requests in the frozen relation graph, then
   integrate primitive truth, source-facet records, source-edge stages, and
   source-edge/source-facet records into the final non-empty canonical artifact,
   codec, independent verifier, and decode/replay surface.
2. Complete source-edge/source-facet source-fan ownership, admitted clustered
   multi-event ordering, complete occurrence tie keys, numeric crossing
   conservation, and all symbolic fields that depend on those results.
3. Integrate the implemented coplanar polygon classifier into candidate-derived
   complete facet-boundary request closure, assemble canonical crossing/overlap
   components and oriented arcs, and publish the resulting overlay table in the
   final artifact while retaining coordinate-coincident distinct occurrences.
4. Complete authoritative construction selection and deduplication, residual and
   conditioning evidence, numeric crossing multiplicity, half-open source-fan
   ownership, and local conservation across all relation families.
5. Complete all symbolic eligibility categories, frozen matrix lookups, event
   seeds, incidence, candidate dispositions, canonical partitions, replay
   diagnostics, and decode support.
6. Add the independent exact-rational/exhaustive oracle, complete known-answer
   and mutation suites, fuzz/shrink campaigns, resource/cancellation matrices,
   structural performance gates, and serial/parallel equivalence evidence.
7. Complete Component 08-10 handoff qualification proving no authoritative
   geometric recomputation and that Component 10 alone performs final selection.

No approximation, legacy predicate, tolerance-based welding, coordinate-keyed
identity, or external dependency has been introduced to bridge the outstanding
work. Non-empty inputs still cannot publish a partial relation artifact.
