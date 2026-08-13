# Component 08 implementation status

Component 08 remains **in progress**.  `tracker.md` must stay unchecked until
all Plan 08 definition-of-done gates and the pipeline integration are complete.

## Implemented and qualified

- frozen provider, policy, schema, codec, verifier, ID, error, resource, and
  checkpoint domains;
- lineage-only event and occurrence keys, normalization, canonical interning,
  authoritative point references, and complete incidence tables;
- bounded source-edge ordering, clusters, endpoint sentinels, interval
  partitions, transverse arrangements, coplanar/collinear arrangements,
  member-preserving aggregates, and total descriptor derivation providers;
- source-facet reconciliation and internal-diagonal transparency;
- final canonicalization, statistics, sectioned codec/private decode, digests,
  and independent verification/publication;
- transaction-safe Component 08 stage orchestration for the currently
  representable no-intersection path, with all 24 cancellation checkpoints and
  checked resource-category preflight/reconciliation;
- an out-of-line, explicitly instantiated stage API for all four supported
  `float`/`double` by `uint32_t`/`uint64_t` combinations;
- exact-rational source-edge order/cluster/partition differential tests;
- construction-scoped Component 07 source-edge parameter adaptation, including
  canonical zero-trace evidence lineage and direct contribution/facet-use
  ranges, qualified on a non-empty crossing fixture and authority mutations;
- Component 07 facet/facet transverse carrier adapter ingestion wired into the
  stage, including verified-stage/request/support/construction validation and a
  qualified fail-closed check for the predecessor's currently missing nonzero
  carrier geometric lineage;
- public-view-only Component 07 coplanar/collinear adapter ingestion, including
  representative/request-lineage node mapping, separate-sheet occurrence
  preservation, shared-boundary carriers with direct bounded endpoint evidence,
  interior component arcs, exact source-facet partition commitments, independent
  proposal reconstruction, and non-empty full-stage qualification;
- preflight and resource reconciliation for coplanar supports, collinear
  carriers, overlap components, regions, and their nested indexes;
- committed semantic golden projection, deterministic fixed-seed fuzzing and
  shrinking, capability/manager resource boundaries, complete cancellation
  matrix, execution-mode/worker-count identity, and sparse/dense structural
  work gates.

## Remaining Plan 08 work

1. Complete the transverse adapter after Component 07 publishes a nonzero
   immutable geometric lineage for its facet/facet carrier construction, then
   the Plan 07 section 15.5 bounded carrier membership parameters and required
   source-facet-region ownership evidence.  Component 08 intentionally fails
   closed rather than replacing either missing lineage or missing parameters
   with hashes, dense-ID surrogates, or coordinate-derived reprojection.
2. Adapt immutable Component 07 coplanar event/arc/component lineage into
   coplanar support, collinear overlap, dual-parameter, ownership, and region
   proposals.
3. Remove the explicit fail-closed proposal-ingestion gates only after the
   resulting non-empty stage artifacts pass exact-oracle, mutation,
   cancellation-inside-loop, resource, metamorphic, and structural tests.
4. Extend exact-oracle/fuzz coverage from the qualified source-edge provider to
   non-empty transverse and coplanar stage artifacts, and complete the remaining
   adversarial/platform sanitizer matrix where supported by the repository
   qualification runner.
5. Integrate the verified Component 08 stage into the bounded Boolean pipeline
   as Plan 08 implementation step 17; do not begin Component 09 here.
6. Mark Component 08 complete in `tracker.md` only after every preceding gate
   passes.

## Component 07 coplanar/contact lineage hardening (in progress)

The coplanar/collinear adapter is gated on Component 07 publishing bounded
coplanar lineage for coplanar/contact fixtures (touching boxes, coincident
sheets).  This commit advances that precondition by hardening the Component 07
coplanar/contact path, which previously rejected every touching/coincident
fixture before the source-facet segment partition could run:

- cross-operand source-vertex ownership is now recorded from exact
  stored-nominal coincidence rather than from a parameter-endpoint index, which
  can attach a contact to the wrong source vertex on a shared boundary edge;
- `partition_source_facet_segment` admits exact stored-coordinate boundary ties
  uniformly (segment endpoints may coincide with a polygon corner even without
  a declared owner) and treats canonical `[-0,+0]` parameters as one point;
- the coplanar boundary partition re-orients the query-edge relation
  parameterization to the source-polygon traversal (the relation follows the
  primitive edge direction, which can be reversed relative to the polygon);
- the collinear kernel emits exact singleton endpoint parameters instead of
  division-derived near-zero intervals for exact endpoint contacts; and
- the independent verifier reconstructs collinear categories from the sorted
  producer parameter order instead of assuming a direction-dependent raw order.

With these fixes the touching-boxes coplanar fixture now advances through the
edge/edge, edge/facet, facet/facet, coplanar-overlay, crossing, symbolic, and
event-seed stages and reaches the independent verifier (checkpoint 26), where
coincident/equal sheets still fail on an unresolved construction-authority
certificate inconsistency (`duplicate_authoritative_producer`,
`component07_exact_oracle`, `component07_type_index_matrix`, and
`component08_qualification_concurrency`).  Equal/coincident sheets therefore
still fail closed, but currently as an internal invariant rather than the
required typed geometric/resource failure; this is the remaining gate before
the coplanar adapter can be un-gated.
