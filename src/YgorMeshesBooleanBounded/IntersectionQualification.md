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

## Current fail-closed integration boundary

The stage driver publishes fully verified empty/no-intersection complexes and
source-topology descriptors.  The complete Component 07-to-Component 08
proposal adapters are not yet integrated.  A non-empty crossing fixture is
therefore required to fail at the earliest missing source-edge parameter
handoff (`parameter_invalid`, checkpoint 11); later explicit transverse and
coplanar gates remain as secondary protection.  No partial event registry is
published and every reservation is released.  The remaining Plan 08 work must
supply those adapters, add non-empty exact/metamorphic qualification, and only
then remove the gates and integrate the stage into the bounded Boolean
pipeline.
