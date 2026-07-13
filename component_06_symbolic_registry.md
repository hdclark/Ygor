# Component 6: Symbolic Entity Registry and Exact Ordering

## 0. Purpose

Turn redundant raw derivations into one canonical symbolic complex. Establish identity, incidence, and strict exact order for all original and constructed vertices/curves before any facet is refined.

## 1. Input contract

Accept validated original features, complete raw event set, exact kernel, stable-ID services, and immutable provenance records.

Raw records may describe the same mathematical point through different facet pairs or feature combinations. Equality may be declared only after exact proof or by a construction key whose canonical equivalence is proven.

## 2. Required behavior

### Canonical interning

- Pre-register every original vertex as a symbolic vertex.
- Normalize construction/provenance keys without relying solely on syntactic expression equality.
- Group candidates using cheap hashes/bounds, then prove exact coordinate equality and compatible incidence before merging.
- Ensure an intersection exactly at an original vertex receives that original vertex's identity.
- Intern coincident curve intervals and overlap carriers while preserving all source ownership/orientation records.

Distinct exact points must never merge because their `T` approximations coincide. Equal exact points must never retain separate identities merely because discovered from different pairs.

### Incidence closure

Compute complete sets of source vertices, edges, facets, planes, and overlap regions incident to each symbol. Propagate endpoint incidence through twins and adjacent facets. Reject contradictory claims rather than choosing one.

### Exact ordering

- For every source edge, sort incident symbolic vertices by exact edge parameter from its canonical endpoint.
- Merge equal parameters only after exact point equality; inconsistent equal-parameter/different-point data is an invariant error.
- For each line/carrier, establish canonical direction and exact total order.
- For planar angular/radial orders needed later, use exact quadrant/orientation comparison and stable-ID tie rules only after geometric equality/collinearity is established.

### Deterministic publication

Assign final IDs from sorted canonical equivalence classes. Parallel discovery and hash bucket order cannot influence IDs. Record all raw-to-canonical mappings.

## 3. Output contract

Produce a `symbolic_complex` containing canonical symbolic vertices, curves/intervals, exact edge split sequences, incidence closure, provenance, orientation metadata, and raw-event mappings.

Invariants:

- One symbolic vertex exists per exact mathematical point represented by discovered events or original vertices.
- Every source-edge sequence begins/ends at its original endpoints and is strictly increasing internally.
- Incidence is symmetric: if an entity lists another as incident, the reverse relation is present where defined.
- Every raw event maps to canonical entities without loss of relation kind or provenance.
- IDs and serialized order are deterministic.

Failure conditions are resource exhaustion, exact-kernel failure, contradictory raw events, or registry invariant defects. Hash collision is never a correctness failure because equality is proven exactly.

## 4. Verification and definition of done

- Feed the same events under all permutations and parallel partitions; canonical serialization is identical.
- Generate many syntactically different constructions of equal points and prove one identity.
- Generate points that round to the same `T` and prove they remain distinct.
- Independently recompute every edge order and incidence relation.
- Registry round-trip tests retain complete provenance and stable IDs.
