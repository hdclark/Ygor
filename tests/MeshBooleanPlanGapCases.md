# Mesh Boolean Plan-Gap Regression Cases

This file turns the feasibility and correctness concerns identified during review of
`broad_plan.md` into concrete TDD targets. The cases are intentionally written at
the contract boundary first. Some cannot be registered in CTest until the public
result and failure contracts are revised, because the existing API has no correct
expected value for them.

For every case:

- use exact input bit patterns and serialize failures with those bits;
- run both operand orders where meaningful;
- run all five Boolean operations when the expected result differs by operation;
- run with `float`/`double` and `uint32_t`/`uint64_t` where the geometry is
  representable as input;
- require deterministic results under facet/ring permutation, thread count, and
  exact-filter path changes;
- never accept `internal_invariant_error` as the expected outcome of a valid
  mathematical input unless the test is explicitly a malformed-artifact test.

## Summary matrix

| ID | Primary gap | Test level | Corrected expected result |
|---|---|---|---|
| G1 | Manifold output is not closed under Boolean operations | End-to-end | Exact stratified result or typed unsupported-result-topology failure |
| G2 | Rounded `T` output cannot retain exact set semantics | End-to-end / realization | Exact-in-`T` failure, or explicitly approximate certified result |
| G3 | Geometric point identity is not topological vertex identity | Registry / arrangement | One geometric point may own several topological link occurrences |
| G4 | Carrier radial order does not replace a spherical vertex link | Arrangement | Complete link sectors and continuations at every contact vertex |
| G5 | Patch-side probes may remain on adjacent boundary planes | Arrangement / classification | Every seed descriptor denotes an open 3D point |
| G6 | Cell propagation and independent side classification are mixed | Arrangement / classification | One explicitly selected, testable classification strategy |
| G7 | Realization certificates omit defining incidences and orders | Realization / verifier | Candidate bits satisfy every required exact construction relation |
| G8 | Global exhaustive realization search is not scalable | Realization solver | Constraint-component decomposition and bounded pair generation |
| G9 | Producer-shaped verification is not independent verification | Verifier mutation | Semantically wrong but self-consistent artifacts are rejected |

## Shared fixture helpers

The existing test helpers can express several cases directly:

```cpp
auto a = cube<double, std::uint32_t>();
auto b = cube<double, std::uint32_t>();
translate(b, dx, dy, dz);
auto c = classification_test::context(
    a, b, output_test::registry(), operation::regularized_union);
```

Add the following reusable fixture.

### `third_intersection_prism<T, I>()`

A triangular prism whose slanted side is `y = 3*x`, using only integer input
coordinates:

```text
v0 = (0,0,0)   v3 = (0,0,1)
v1 = (1,3,0)   v4 = (1,3,1)
v2 = (0,3,0)   v5 = (0,3,1)

faces:
  [0,2,1]       bottom
  [3,4,5]       top
  [0,1,4,3]
  [1,2,5,4]
  [2,0,3,5]
```

Intersecting this prism with the unit cube creates exact boundary vertices

```text
(1/3, 1, 0)
(1/3, 1, 1)
```

even though every input coordinate is exactly representable in binary floating
point.

---

## G1. Valid manifold operands can produce a non-manifold result

### G1a. Vertex-touching cubes

**Fixture**

```cpp
auto a = cube<double, std::uint32_t>();       // [0,1]^3
auto b = cube<double, std::uint32_t>();
translate(b, 1.0, 1.0, 1.0);                 // [1,2]^3
```

The operands share exactly the point `(1,1,1)` and have disjoint interiors.

**Operations and expected mathematics**

| Operation | Exact regularized result |
|---|---|
| union | two cubes joined at one boundary point |
| symmetric difference | same as union |
| intersection | empty |
| `A-B` | `A` |
| `B-A` | `B` |

At the shared point, the boundary link is two disjoint circles. It is not a
single circle, so the union/xor boundary is not a 2-manifold.

**TDD assertions**

1. Input validation succeeds for both operands.
2. Event discovery records an exact vertex contact.
3. Exact selection for union/xor succeeds as a stratified boundary.
4. The selected result records two topological link components at the shared
   geometric point.
5. A manifold-only `fv_surface_mesh` publication returns a dedicated typed
   result-topology failure, for example `result_topology_not_supported`.
6. It must not return `input_contract_error`, `output_not_representable`, or
   `internal_invariant_error`.
7. Intersection returns a successful empty mesh; differences return the
   untouched operand.

A temporary test against the current API may assert only that union/xor do not
publish a mesh. Replace that assertion when the typed topology result is added.

### G1b. Edge-touching cubes

**Fixture**

```cpp
auto a = cube<double, std::uint32_t>();       // [0,1]^3
auto b = cube<double, std::uint32_t>();
translate(b, 1.0, 1.0, 0.0);                 // [1,2]x[1,2]x[0,1]
```

The operands share the complete edge `(1,1,z)`, `0 <= z <= 1`, and have disjoint
interiors.

**TDD assertions**

For union/xor, an interior point of the shared edge has a non-circular boundary
link. Require the same exact-stratified-success/manifold-output-failure behavior
as G1a. Intersection remains empty and both differences remain unchanged.

This case prevents a fix that handles only isolated point contacts.

---

## G2. Exact set semantics and rounded realization must be separate contracts

### G2a. One-third intersection coordinate

**Fixture**

```cpp
auto a = cube<double, std::uint32_t>();
auto b = third_intersection_prism<double, std::uint32_t>();
auto c = classification_test::context(
    a, b, output_test::registry(), operation::regularized_intersection);
```

**Exact fact**

The selected exact boundary contains `(1/3,1,0)` and `(1/3,1,1)`. No finite
binary32 or binary64 value equals `1/3`.

**TDD assertions**

Under an `exact_in_T` output policy:

1. Components 1-10 succeed.
2. The selected symbolic vertices equal the exact rationals above.
3. Component 11 returns `output_not_representable`.
4. The failure identifies the exact symbols and the unsatisfied coordinate or
   incidence constraints.

Under a future `certified_approximate_embedding` policy:

1. Realization may succeed with nearby `T` values.
2. The public result kind explicitly states that the represented point set is
   not the exact Boolean set.
3. The certificate records the exact target, accepted bits, and non-zero
   displacement/error bound.
4. No field or documentation may claim exact set semantics for that mesh.

A single success type that reports exact set semantics after moving either
one-third vertex fails this test.

---

## G3. Separate geometric-point keys from topological vertex occurrences

Reuse G1a.

**TDD assertions**

1. The registry may intern the coordinate `(1,1,1)` once as a geometric point.
2. The global arrangement stores two distinct topological occurrence/link
   records associated with that point, one for each cube boundary sheet.
3. No adjacency, patch mate, sector continuation, or classification transition
   is created solely because the coordinates are equal.
4. A stratified output can preserve both occurrences while sharing an exact
   geometric point.
5. A manifold output rejects the result rather than welding the two links.

Add a mutation test that merges the two topological occurrence records while
leaving the exact point and all coordinate bytes unchanged. The independent
arrangement verifier must reject the artifact.

---

## G4. Build and verify the spherical link at contact vertices

### G4a. Isolated contact with no carrier radial order

Reuse G1a. The shared point has no positive-length intersection seam, so there
is no carrier around which a radial order can encode the local topology.

**TDD assertions**

1. A vertex-link record exists despite the absence of a seam edge.
2. Its exact link arrangement has two disconnected components.
3. Every incident patch-side sector appears exactly once.
4. No sector continuation crosses from one cube to the other.
5. The topology classifier reports a non-manifold selected link for union/xor.

### G4b. Synthetic multi-plane vertex star

Build a Component-8 fixture at the origin with oriented sheet germs on the exact
planes

```text
x = 0
y = 0
z = 0
x + y + z = 0
```

and explicit incident patch wedges. The fixture need not be accepted as a full
operand; it is a local arrangement-unit fixture.

**TDD assertions**

1. The independently constructed spherical arrangement contains every great
   circle/ray intersection implied by the germs.
2. Directed link rays have correct antipodes.
3. Link arcs are split at every crossing.
4. Open sectors have exact witness directions satisfying all incident signs.
5. Seam continuations match through the vertex without relying on carrier ID
   ordering.
6. Permuting planes and patch occurrences produces the same canonical link.

This catches implementations that sort planes around each edge but do not solve
the endpoint link problem.

---

## G5. A patch-side seed must be an open 3D point

### G5a. Cube-corner normal probe

Use two disjoint cubes so the arrangement contains untouched source patches:

```cpp
auto a = cube<double, std::uint32_t>();
auto b = cube<double, std::uint32_t>();
translate(b, 3.0, 0.0, 0.0);
auto c = classification_test::context(
    a, b, arrangement_test::registry(), operation::regularized_union);
auto g = build_global_arrangement(*c);
```

Find a patch-side probe whose base is cube vertex `(0,0,0)` on the face `x=0`.
A first-order direction normal to that face is parallel to the adjacent planes
`y=0` and `z=0`.

For a formal point `p + epsilon*d`, evaluate every incident plane as

```text
plane(p) + epsilon * dot(plane_normal, d)
```

**TDD assertions**

1. The first non-zero coefficient is non-zero for every operand boundary plane
   incident to the base stratum.
2. The descriptor includes all incident constraints, not only the source patch
   plane.
3. Independent exact point location never receives a formal point that is still
   on an adjacent boundary plane.
4. The corrected implementation may use an exact patch-interior base point, a
   strict cone direction satisfying all incident planes, or a documented
   higher-order infinitesimal.

The current-style descriptor `(corner vertex) + epsilon*(single face normal)`
must fail descriptor verification before classification.

---

## G6. Freeze one classification strategy and test its complete invariant

Use two disjoint cubes, with one cube represented using two coplanar triangular
facets per square face. This creates many patch-side fragments that belong to
only three open cells: exterior, interior of `A`, and interior of `B`.

### Strategy A: side-graph propagation

If the plan chooses a complete conservative side graph:

1. Component 8 connects coplanar, source-edge, seam, and vertex-sector fragments
   through region-preserving transitions.
2. The disjoint-cube fixture has exactly three region components.
3. Component 9 uses exactly one seed per component.
4. Removing one required transparent/source-edge/vertex transition causes the
   independent verifier to fail.
5. Subdividing each face again does not change the quotient region graph.

### Strategy B: independent patch-side classification

If the plan chooses independent classification:

1. Every atomic patch side has its own certified open probe.
2. Every side is classified directly; propagation is not cited as the proof of
   labels.
3. Region IDs are explicitly conservative fragments, not claimed 3D cells.
4. Graph transitions are optional cross-checks only and do not appear in the
   correctness argument as complete connectivity.

Add a contract test that requires a frozen strategy enum/version and rejects a
hybrid artifact that has one seed per patch side while claiming complete
cell-propagation certification.

---

## G7. Realization must preserve defining exact incidences

### G7a. Rounded vertex leaves its construction carrier

Use a synthetic selected triangle with vertices

```text
a = (0,0,0)
q = (1/3,1,0)
b = (0,1,0)
```

and record that `q` is defined by the exact carrier/plane relation

```text
3*q.x - q.y = 0
```

Nearest binary rounding of `q.x` leaves the triangle non-degenerate and
embedded, but makes the defining relation non-zero.

**TDD assertions**

1. The nearest candidate passes triangle-orientation and pair-intersection
   checks, proving those checks alone are insufficient.
2. The realization obligation set contains the defining carrier/plane
   incidence.
3. Exact substitution of accepted `T` bits into `3*x-y` is required to equal
   zero under an exact-in-`T` policy.
4. The realization fails when no candidate in the configured domain satisfies
   all defining incidences.
5. Under an approximate policy, the certificate records that this incidence is
   approximated rather than exact and cannot claim exact set semantics.

### G7b. Verifier mutation

Starting from an accepted realized fixture, move one constructed vertex by one
ULP along an axis chosen so that:

- every triangle remains oriented;
- all triangle-pair intersection types are unchanged;
- a defining source-plane or carrier equation is violated.

Regenerate producer-owned counts and serialization through a test-only artifact
builder. The independent verifier must reject the artifact by recomputing the
construction relation, not by noticing stale bytes.

---

## G8. Realization search must decompose independent constraints

Factor the deterministic assignment solver behind a testable internal
interface. Give it `N` disconnected constraint components. Each component has
one variable with candidate domain `{0,1}` and a unary constraint accepting
only `1`.

A complete monolithic solver that validates only full assignments explores
approximately `2^N` assignments before reaching all ones. A component solver
explores at most a constant number per component.

**TDD assertions**

For `N = 20`:

1. The accepted assignment is all ones.
2. `visited_nodes` and `complete_assignments` are bounded by a documented
   linear function of `N`, not exponential in `N`.
3. Permuting component and variable order leaves accepted assignment and
   canonical certificate unchanged.
4. A work limit selected between the linear and exponential bounds succeeds
   with decomposition.
5. The verifier replays component certificates rather than repeating a global
   exponential DFS.

Add a separate broad-phase performance property with many spatially disjoint
realization triangles. Exact triangle-pair relation checks must be generated
from a conservative candidate structure rather than all `n*(n-1)/2` pairs.

---

## G9. Independent verifiers must reject common-mode producer errors

### G9a. Radial-order mutation with self-consistent encoding

Create a valid transverse seam fixture. Produce a mutated arrangement in which
two radial layers are exchanged, then rebuild IDs, sectors, counts, canonical
bytes, and digests consistently using a test-only producer-style artifact
builder.

**TDD assertion**

The Component-8 verifier rejects the artifact by independently recomputing
radial order from the exact planes. A verifier that only validates ranges and
re-encodes stored order fails this test.

### G9b. Missing spherical-link continuation

Remove one vertex-sector seam continuation, then update all stored counts and
serialization consistently.

**TDD assertion**

The verifier reconstructs the local spherical link from source germs and
detects the missing continuation.

### G9c. Realization common-mode mutation

Use G7b and rebuild all producer-shaped obligation records with
`actual=embedded`.

**TDD assertion**

The Component-11 verifier independently substitutes accepted coordinate bits
into source constructions and rejects the artifact. An obligation record whose
expected and actual values are merely copied from the artifact is not a proof.

### G9d. Oracle independence guard

In test builds, tag producer and verifier helper functions by implementation
family. The mandatory verifier test target must fail to link if it calls
producer grouping, ordering, triangulation, assignment-validation, or canonical
encoding helpers that it is intended to independently reproduce.

---

## Registration plan

1. Add executable tests that already have a well-defined current contract to
   the relevant component test targets.
2. Keep tests requiring a new public enum/result type as compile-ready fixtures
   behind a clearly named feature macro, not silently skipped assertions.
3. Once the contract is revised, remove the feature guard and add all cases to
   CTest.
4. Add the two non-manifold cases and the one-third case to the permanent fuzz
   seed/replay corpus.
5. Treat this file as a release gate: every case must be executable, with an
   explicit expected success or typed failure, before the broad plan can claim
   completion.
