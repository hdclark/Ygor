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

## Outstanding before Component 07 can be checked complete

The tracker remains unchecked. The following Plan 07 work is still required:

1. Complete all genuinely missing Component 03 formula/capability bindings and
   integrate the primitive truth adapter into the non-empty `RelationBuild`
   transaction and request graph.
2. Canonical source-edge/source-edge relations and bounded constructions.
3. Complete source-edge/source-facet composites, including endpoint, tangent,
   coplanar, and multi-event cases.
4. Source-facet/source-facet support, transverse carriers, coplanar overlay,
   containment, equality, and distinct sheet occurrences.
5. Authoritative construction selection, residual and conditioning evidence,
   numeric crossing multiplicity, half-open source-fan ownership, and local
   conservation.
6. Full symbolic eligibility categories, event seeds, incidence, candidate
   dispositions, canonical partitions, replay diagnostics, and decode support.
7. The independent exact-rational/exhaustive oracle, all known-answer and
   mutation suites, fuzz/shrink campaigns, resource/cancellation matrices,
   structural performance gates, and serial/parallel equivalence evidence.
8. Component 08-10 handoff qualification proving no authoritative geometric
   recomputation and that Component 10 alone performs final selection.

No approximation, legacy predicate, tolerance-based welding, coordinate-keyed
identity, or external dependency has been introduced to bridge the outstanding
work. Non-empty inputs still cannot publish a partial relation artifact.
