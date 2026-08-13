# Component 08 qualification and stage publication

`IntersectionBuild.h` is the transactional serial reference for publishing a
`canonical_intersection_complex<T,I>`.  It consumes only the immutable
Component 07 relation artifact and predecessor context/precision views.  Every
phase is mapped to one of the 24 frozen `intersection_checkpoint` values.  A
cancelled, resource-exhausted, contradictory, or verifier-rejected execution
returns a typed error and exposes no artifact.

The stage driver deliberately assigns no task-local semantic IDs.  Event,
occurrence, incidence, source-edge, aggregate, descriptor, codec, and verifier
providers retain their complete-key sort/scan and canonical-ID rules.  The
`deterministic_parallel_v1` execution profile is accepted only as a
non-semantic policy at this layer; the current provider runs the serial
reference path.  Qualification compares serial, one-worker, two-worker, and
multi-worker contexts byte-for-byte so a later private-fragment executor cannot
change the published contract.

## Resource model

`preflight_intersection_events` computes checked upper bounds for every
Component 08 publication category:

- events, occurrences, seed bindings, and incidence;
- source-edge/carrier memberships, clusters, intervals, and ordering
  certificates;
- transverse carriers and coplanar/collinear overlap records;
- member-preserving aggregates and cut/contact descriptors;
- temporary sort/reconstruction storage, work units, persistent bytes, and
  canonical bytes.

The stage reserves each category before semantic construction, reconciles the
actual immutable statistics after independent verification, and commits only
at the transaction boundary.  Limit-minus-one tests must fail without a live
reservation.  Exact and limit-plus-one configurations must publish the same
canonical bytes; limits never authorize truncation or a different merge.

## Exact oracle and deterministic campaigns

The permanent Component 08 qualification target imports binary floating-point
parameters into the in-tree `ExactRational` oracle.  Source-edge sequence order,
exact-equality clusters, and endpoint-sentinel interval partitions are checked
against exact dyadic values rather than nominal subtraction or tolerance.
The committed known-answer projection includes stable source-edge memberships,
cluster membership, predecessor/successor links, interval classes, ordering
certificates, and structural counters.

A fixed-seed campaign generates exact dyadic memberships with repeated values,
permutes proposal order, and requires byte-identical semantic projections.  Its
shrinker repeatedly removes records while preserving the typed failure and is
qualified with an exact-equal-without-evidence case that minimizes to the two
records required to reproduce the defect.

## Structural gates

Well-separated memberships must perform exactly one certified comparison per
adjacent ordered pair after the sweep pre-order.  A genuinely dense exact-equal
cluster must report the complete `k*(k-1)/2` all-pairs work and retain all `k`
distinct occurrences.  Persistent table storage is checked to grow with
published records rather than an unbounded global geometry grid or coordinate
cache.

## Non-empty transverse publication

After the Component 07a-07f lineage work, the frozen Component 07 carrier
construction now publishes a nonzero `geometric_lineage`, and the Component 08
transverse adapter consumes it directly.  The overlapping-box fixture is driven
through the complete transactional stage: seed normalization, event/occurrence
interning, authoritative-point attachment, incidence publication, source-edge
arrangements, transverse carrier grouping/orientation, bounded carrier
membership ordering, relation-supported active spans, member-preserving
aggregation, and cut/contact descriptor derivation.  The stage then
canonicalizes, encodes, and independently verifies the resulting
`canonical_intersection_complex` before committing.

`test_nonempty_transverse_stage_publishes` runs that fixture under serial and
parallel (one, two, and eight worker) execution profiles and requires
byte-identical canonical bytes and digests, one interned event and occurrence per
seed, one source-edge membership per seed, non-empty transverse carrier/span
tables, non-empty cut/contact descriptors, and one shared authoritative point
reference per event.  The still-frozen fail-closed gate remains separately
verified: clearing the Component 07 carrier `geometric_lineage` must reject at
checkpoint `transverse_carriers` with `transverse_carrier_invalid` and no leaked
reservations.

## Remaining fail-closed boundary: coplanar arrangements

The coplanar proposal ingestion (`build_coplanar`) is still fail-closed.  When
the Component 07 artifact carries coplanar lineage (coincident facets, coplanar
event nodes, oriented arcs, or overlap components) the stage rejects at
checkpoint `coplanar_carriers` with `membership_incomplete` rather than welding
or approximating coplanar relations.  The collinear-overlap and coplanar-support
arrangement machinery (`build_coplanar_carrier_arrangements`) is implemented and
independently tested against synthetic proposals; only the Component 07 record
adapter is deferred.

The coplanar adapter remains gated on the predecessor contract, not on
Component 08 machinery.  Component 07 currently returns typed failures for
coplanar and contact fixtures before Component 08 runs: full-face overlap and
partial coplanar overlap exceed the conservative `persistent_bytes` reservation
(the coplanar request/dependency upper bounds reach ~1.36M requests and ~18M
dependencies for two face-touching boxes), while vertex- and edge-touching
fixtures are rejected with unresolved half-open sweep ordering and source-facet
boundary ownership.  Component 08 therefore cannot consume valid coplanar
lineage yet.  Component 08 does not project retained point coordinates onto a
carrier or invent coplanar supports as a fallback; the coplanar adapter and
non-empty coplanar qualification are integrated only after Component 07
publishes bounded coplanar lineage for these degenerate cases.
