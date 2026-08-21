# Component 5: Exact Feature Intersection Events

## 0. Purpose

Resolve every broad-phase pair into a complete, exact description of how the two source boundaries interact. Emit symbolic events and carriers with provenance, never independently rounded Cartesian points.

## 1. Input contract

Accept canonical candidate facet pairs, validated operands, exact kernel, stable feature IDs, and operation-independent event policy.

Each facet is an exact simple planar polygon with explicit boundary features. Candidates are a superset and may be disjoint. The module must not assume general position.

## 2. Required behavior

### Pair classification

Classify support planes exactly as distinct non-parallel, parallel-disjoint, or coincident.

For non-coplanar facets:

- Compute the exact plane-plane intersection carrier when it exists.
- Intersect the carrier with each closed polygon, including concave polygons, obtaining exact ordered point/interval sets.
- Intersect those sets to identify proper crossing segments, endpoint touches, vertex-on-face, edge-on-plane, and tangential contacts.
- Attribute every endpoint to all incident source features, not merely the triangulation primitive that found it.

For coplanar facets:

- Build the exact 2D relation of polygon boundaries and interiors.
- Emit all boundary crossings, vertex-on-edge events, shared subsegments, coincident vertices/edges, and positive-area overlap descriptors.
- Preserve orientation and side provenance needed to resolve same-facing and opposite-facing coincident regions later.

### Completeness and triangulation independence

Internal source triangulations may accelerate queries, but events on artificial diagonals are merged away and cannot appear as source-boundary events. Concave facets may produce multiple disjoint intersection intervals; all are emitted.

### Event representation

Raw events describe mathematical constraints, for example:

- Original vertex incident to another operand's edge/facet.
- Proper edge-edge or edge-plane construction with exact parameters.
- Intersection endpoint on a plane-plane carrier.
- Coincident edge interval with exact endpoint keys.
- Coplanar overlap region bounded by source features.

Each event includes normalized source feature IDs, exact parameter/expression handles, relation kind, local orientation/sign data, and all derivations discovered for that candidate. Cartesian approximations are diagnostics only.

### Degeneracy discipline

Do not perturb real geometry during discovery. A vertex-on-vertex event remains exact coincidence. A tangent creates contact events even though regularized evaluation may later select no boundary there. Event discovery is independent of requested Boolean operation.

## 3. Output contract

Produce a deterministically sorted `raw_event_set` containing point events, overlap intervals/regions, carrier records, source incidence, and exact relation certificates. Disjoint candidates produce explicit trace classifications but no event.

Invariants:

- Every point or positive-dimensional component of `boundary(A) intersect boundary(B)` is covered by emitted events/overlap descriptors.
- Every emitted event satisfies all defining source incidences exactly.
- Event endpoints have exact order parameters on every supporting source edge/carrier.
- No topology depends on an approximate coordinate or epsilon.
- Reversing candidate traversal changes neither normalized events nor provenance content.

Failure conditions are exact-kernel/resource failures or an internal completeness/invariant defect. Degenerate intersection geometry is ordinary successful output.

## 4. Verification and definition of done

- Focused matrices cover disjoint, proper crossing, endpoint touch, collinear overlap, edge-on-face, coplanar overlap, containment, equal facets, opposite facets, and concave multi-interval cases.
- Subdividing or retriangulating a source facet yields equivalent source-level events.
- Swapping operands preserves symmetric event geometry while correctly swapping oriented provenance.
- Exact substitution verifies every endpoint and interval.
- A slow independent boundary-arrangement oracle checks low-complexity pairs.
