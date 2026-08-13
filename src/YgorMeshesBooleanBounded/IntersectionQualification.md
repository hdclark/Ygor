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

## Coplanar arrangement handoff

Component 08 consumes the checked Component 07 coplanar support, event-node,
oriented-arc, overlap-component, and partition-coverage records through a
public-view-only adapter. Node records map to interned occurrences by
authoritative construction representative and decoded public contact-request
lineage, never by coordinate matching. Shared-boundary arcs produce collinear
carriers with direct bounded endpoint intervals; interior arcs remain component
boundaries and do not acquire transverse support identities.

Region incidence commits to exact source-facet boundary partitions because the
public handoff does not publish triangle coverage. Internal diagonals remain
bookkeeping-only rather than fabricated coverage owners. Qualification uses the
successful touching-box Component 07 fixture and checks non-empty full-stage
publication, independent proposal reconstruction, mutation rejection,
fail-closed incomplete handoff behavior, and serial/worker canonical identity.
### Construction witness compatibility

The coordinate adapter intentionally does not require every secondary ledger
entry to name the same source relation as every seed grouped at an event. A
canonical construction may be shared by several independently admitted
relations. Safety instead follows from the stronger per-entry checks: every
ledger entry names the same construction, carries the construction's tolerance
boundary, has complete lineage/enclosure/parameter/residual compatibility, and
each seed must find a ledger use with its own source relation and occurrence.
Thus unrelated witnesses cannot authorize a seed, while compatible
cross-relation witnesses remain auditable. Focused mutation tests must reject a
missing matching use and any false compatibility field.
