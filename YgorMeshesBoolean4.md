# `src/YgorMeshesBoolean4.cc` algorithm guide

This note documents the boolean engine exactly as implemented in
`src/YgorMeshesBoolean4.cc`, plus the BSP-tree machinery it delegates to in
`src/YgorMeshesBSPTree.{h,cc}`.  The implementation in `YgorMeshesBoolean4.cc`
itself is intentionally thin: it validates only trivial empty-input cases,
converts both input facet-vertex meshes to `bsp_tree_volume` objects, applies a
BSP boolean operation, and converts the BSP volume back to an `fv_surface_mesh`.

No implementation code was changed while preparing this document.

## Public API surface

`src/YgorMeshesBoolean4.h` declares:

- `enum class MeshBooleanOperation4 { Union, Intersection, Exclusion, Subtraction }`.
- `BooleanMeshOp4(lhs, rhs, op)`, the generic dispatcher.
- Four convenience wrappers: `BooleanUnion4`, `BooleanIntersection4`,
  `BooleanExclusion4`, and `BooleanSubtraction4`.

All functions are templates over coordinate type `T` and face-index type `I`.
`src/YgorMeshesBoolean4.cc` explicitly instantiates each API for:

- `float, uint32_t`
- `float, uint64_t`
- `double, uint32_t`
- `double, uint64_t`

## Exact control flow in `BooleanMeshOp4`

### 1. Empty-left-input shortcut

The function first checks only the raw containers:

```cpp
if(lhs.faces.empty() || lhs.vertices.empty()) { ... }
```

If the left mesh has no faces or no vertices, it returns immediately without
validating the right mesh or invoking the BSP code:

- `Union`: returns `rhs` unchanged.
- `Exclusion`: returns `rhs` unchanged.
- `Intersection`: returns a default-constructed empty `fv_surface_mesh<T,I>`.
- `Subtraction`: returns a default-constructed empty `fv_surface_mesh<T,I>`.

Topologically, this treats an empty left operand as the empty solid.  This is
consistent for union, intersection, and subtraction (`∅ - B = ∅`).  For
exclusion/XOR, `∅ xor B = B`, so the shortcut is also consistent.

Important implementation detail: returning `rhs` unchanged means the result is
not normalized, triangulated, validated, reoriented, or rebuilt.  If `rhs` is
invalid, polygonal rather than triangular, has stale auxiliary indices, or is
not closed/manifold, this shortcut preserves that state instead of enforcing the
contract that boolean outputs are manifold surface meshes.

### 2. Empty-right-input shortcut

The next raw-container check is:

```cpp
if(rhs.faces.empty() || rhs.vertices.empty()) { ... }
```

If the right mesh has no faces or no vertices, the function returns:

- `Union`: `lhs` unchanged.
- `Intersection`: empty mesh.
- `Subtraction`: empty mesh.
- `Exclusion`: `lhs` unchanged.

The union, intersection, and exclusion cases match the empty-solid algebra.
The subtraction case does not: mathematically, `A - ∅ = A`, but the
implementation returns an empty mesh.  This is a direct behavioral defect in
`BooleanMeshOp4`; it is not caused by the BSP backend.

As with the left shortcut, returning `lhs` unchanged bypasses all conversion,
validation, triangulation, orientation repair, and auxiliary-index rebuilding.

### 3. Mesh-to-BSP conversion

For non-empty raw inputs, the function calls:

```cpp
const auto lhs_tree = bsp_tree_volume<T, I>::from_fv_surface_mesh(lhs);
const auto rhs_tree = bsp_tree_volume<T, I>::from_fv_surface_mesh(rhs);
```

At this point `BooleanMeshOp4` stops inspecting mesh topology and delegates all
real validation and interpretation to `bsp_tree_volume::from_fv_surface_mesh`.
Any exception thrown there propagates directly to the caller; the boolean layer
does not catch it, decorate it with operand context, or convert it to a
user-facing diagnostic.

The BSP conversion is documented in detail below because it is the effective
implementation of the boolean engine.

### 4. BSP boolean dispatch

The function creates a default `bsp_tree_volume<T,I> result_tree;`, then assigns
it in a `switch(op)`:

- `Union`: `lhs_tree.boolean_union(rhs_tree)`
- `Intersection`: `lhs_tree.boolean_intersection(rhs_tree)`
- `Exclusion`: `lhs_tree.boolean_exclusion(rhs_tree)`
- `Subtraction`: `lhs_tree.boolean_subtraction(rhs_tree)`

There is no `default:` case.  For valid `MeshBooleanOperation4` enumerators this
is exhaustive.  If a caller casts an invalid integer to the enum, the switch
falls through, leaving `result_tree` default-constructed and therefore empty.

### 5. BSP-to-mesh conversion

Finally, the result tree is converted back:

```cpp
return result_tree.to_fv_surface_mesh();
```

Again, exceptions or malformed output from the BSP layer are not handled in the
boolean wrapper.  The wrapper assumes the BSP tree is a complete and correct
solid representation and that `to_fv_surface_mesh()` can recover the boundary.

## Convenience wrappers

Each named boolean wrapper simply calls `BooleanMeshOp4` with the corresponding
enum value.  No wrapper adds validation, normalization, or special handling:

- `BooleanUnion4(lhs, rhs)` -> `BooleanMeshOp4(lhs, rhs, Union)`
- `BooleanIntersection4(lhs, rhs)` -> `BooleanMeshOp4(lhs, rhs, Intersection)`
- `BooleanExclusion4(lhs, rhs)` -> `BooleanMeshOp4(lhs, rhs, Exclusion)`
- `BooleanSubtraction4(lhs, rhs)` -> `BooleanMeshOp4(lhs, rhs, Subtraction)`

## BSP representation used by the boolean layer

`bsp_tree_volume<T,I>` represents a solid as a binary space partition tree.
Each node is either:

- `Partition`: an oriented plane plus `front` and `back` subtrees.
- `In`: a leaf classified as solid.
- `Out`: a leaf classified as empty.

A partition plane is stored as three anchor points copied from a triangle.  The
plane normal is `(anchor1 - anchor0).Cross(anchor2 - anchor0)`, so all sidedness
and inside/outside semantics depend on the input face winding and on later
calls to `OrientFaces`.

The intended high-level approach is:

1. Convert each closed input surface mesh to a BSP solid.
2. Merge the two BSP solids by recursively partitioning one tree with the
   planes of the other and applying leaf-level boolean truth tables.
3. Extract the boundary between `In` and `Out` leaves back into polygons and
   triangulate those polygons.

That is a recognizable BSP boolean strategy, but the details below show several
places where the implementation is not topologically sound by construction.

## Mesh-to-BSP conversion details

`from_fv_surface_mesh(mesh, seed)` is the conversion called by
`BooleanMeshOp4`.  It performs the following steps.

### 1. Raw empty mesh handling

If the input has no faces or no vertices, it returns an empty BSP tree
(`root == nullptr`).

### 2. Local working copy and triangulation

The input mesh is copied into `working_mesh`, then `working_mesh.convert_to_triangles()`
is called.  This means the BSP layer can accept non-triangular faces from the
boolean wrapper, but only after whatever triangulation strategy
`fv_surface_mesh::convert_to_triangles()` implements.

Potential impedance mismatch: the boolean wrapper's public comments describe
closed triangular meshes, while the conversion actually accepts polygonal faces
and triangulates them silently.  If triangulation changes topology, introduces
sliver triangles, or chooses diagonals inconsistent with geometric constraints,
the boolean layer has no visibility into that.

### 3. Validation and cleanup

The conversion then:

1. Rejects non-finite vertices via `HasOnlyFiniteVertices`.
2. Calls `working_mesh.remove_degenerate_faces()`.
3. Returns an empty BSP if all faces were removed.
4. Requires `IsTriangularMesh`.
5. Requires valid face indices via `HasValidFaceIndices`.
6. Requires no degenerate faces via `HasNoDegenerateFaces`.
7. Requires `IsClosedManifold`.
8. Requires `HasConsistentOrientation`.

Failures throw `std::invalid_argument` with messages naming the BSP conversion
routine.  They do not identify whether the left or right boolean operand failed.

The conversion relies on Ygor's verification routines for topological validity.
The wrapper itself does not independently enforce manifoldness or orientation.

### 4. Triangle records and plane records

For each triangular face, the code extracts the three indexed vertices, computes
`normal = (v1 - v0).Cross(v2 - v0)`, and checks `normal.sq_length()` against
`plane_threshold<T>()` (`epsilon * 1024`).

If the squared normal is below this threshold, it attempts an adaptive
`orient3d` check using `pd = v0 + dir`, where `dir` is either the computed
normal or `(1,0,0)` if the normal is too small.  If the adaptive orientation is
exactly zero, the triangle is skipped.

This is numerically questionable:

- The previous `remove_degenerate_faces()` and `HasNoDegenerateFaces()` checks
  should already reject degenerates.  Skipping faces afterward can silently
  alter a closed manifold into an open surface.
- `orient3d(v0, v1, v2, v0 + normal)` for a nonzero normal should generally be
  proportional to squared area.  But the branch is entered specifically for
  very small squared normal, so the test mixes exact-ish sign predicates with a
  scale-dependent floating threshold.
- If `dir` is replaced by `(1,0,0)`, triangles whose plane is aligned so that
  this point is coplanar may be skipped even though another direction would
  reveal non-coplanarity.

Surviving triangles are stored twice:

- `all_tris`, used later for ray-cast inside/outside classification.
- `tri_recs`, where each record stores the triangle and its partition plane.

### 5. Recursive BSP build

`build_bsp_from_triangles(tri_recs, all_tris, 0, seed)` builds the tree.
At depth zero it shuffles the triangle order using either the optional seed or
`std::random_device`.  Because `BooleanMeshOp4` does not pass a seed, tree shape
and therefore numerical behavior can be nondeterministic across runs.

The builder chooses the first triangle's plane as the current partition plane,
then classifies every triangle against that plane using `classify_triangle`.

#### Point and triangle classification

`classify_point(P, v)` calls `adaptive_predicate::orient3d(P.a, P.b, P.c, v)`
and returns `+1`, `0`, or `-1` from the sign.  No tolerance is applied at this
stage; only exact zero from the adaptive predicate is coplanar.

This is the strongest numerical component in the code: using Shewchuk-style
adaptive predicates for plane sidedness is appropriate for robust decisions.
However, later intersection construction and welding use ordinary floating-point
arithmetic and scale-dependent thresholds, so exact classifications do not make
the overall algorithm exact.

`classify_triangle` classifies the three vertices:

- all zero -> `Coplanar`
- signs include both positive and negative -> `Spanning`
- any negative and no positive -> `Back`
- otherwise -> `Front`

Vertices on the plane are grouped with the nonnegative side for all-front logic
and with the nonpositive side for all-back logic in the splitting function,
which can duplicate plane vertices into both outputs.

#### Triangle splitting

Spanning triangles are split by `split_triangle`.

- If one vertex is on/above the plane and two are below, it creates one front
  triangle and two back triangles.
- Otherwise, it assumes one below and two on/above, creating two front triangles
  and one back triangle.

Intersections are computed by `intersect_edge_with_plane`, which evaluates two
adaptive orientation values `d0` and `d1`, then uses ordinary floating-point
linear interpolation with `t = d0 / (d0 - d1)`.  If the denominator is small
relative to `epsilon * (abs(d0) + abs(d1))`, it returns the edge midpoint.

This is not an exact construction.  Even if sidedness is robust, new vertices
are rounded floating-point points.  The midpoint fallback can put an intersection
away from the partition plane.  Repeated splitting can accumulate cracks,
slivers, or inconsistent vertices between adjacent triangles.

#### Coplanar and one-sided special cases

The builder has several special cases intended to terminate recursion when all
remaining triangles are coplanar with the chosen plane or when only one side has
non-coplanar triangles.  These cases classify leaf children by ray casting a
point slightly offset from the partition plane:

```cpp
P.centroid() +/- P.unit_normal() * plane_threshold<T>() * 2
```

The ray test casts in the fixed +X direction and counts Moller-Trumbore triangle
intersections, rejecting hits near edges and vertices using `plane_threshold<T>()`.

Risks:

- The offset is based only on machine epsilon, not on mesh scale in this code
  path, despite a `compute_all_tris_extent` helper existing but not being used.
- A fixed +X ray is vulnerable to degeneracies with triangles parallel to or
  nearly parallel to the ray and to points whose ray passes through edges or
  vertices.
- The ray test uses ordinary floating-point arithmetic and thresholding, not the
  adaptive predicates.
- For multiple disconnected shells, parity ray casting can still work for
  consistently oriented closed surfaces, but degeneracies can misclassify cells.

#### Depth limit

If `tris.empty()` or `depth > 64`, the builder returns an `Out` leaf.  Hitting
the depth limit silently discards any remaining solid classification as empty.
For CAD-quality booleans this is a dangerous failure mode: it produces a valid-
looking but topologically wrong result instead of a clear error.

## BSP boolean merge details

The core merge routine is `merge_bsp(A, B, op)`, where `op` is:

- `0`: union
- `1`: intersection
- `2`: subtraction (`A - B`)

Leaf cases apply ordinary boolean truth tables:

- `In union B = In`; `Out union B = B`
- `In intersection B = B`; `Out intersection B = Out`
- `In - B = complement(B)`; `Out - B = Out`
- Symmetric cases are handled when `B` is a leaf.

When both inputs are partition nodes, the routine chooses `A`'s plane, partitions
`B` by that plane, recursively merges `A.front` with `B_front` and `A.back` with
`B_back`, then collapses redundant equal leaf children.

This is a standard BSP-merge skeleton.  Its correctness depends critically on
`partition_tree` correctly cutting an entire BSP tree by an arbitrary plane.

### `partition_tree`

For leaves, `partition_tree` returns two copies of the same leaf type, because a
uniform region remains uniform on both sides of an additional cut.

For partition nodes, it first tries to detect coplanar partition planes by:

1. Computing unit normals with floating-point normalization.
2. Testing `abs(dot) > 1 - epsilon * 128` for parallelism.
3. Classifying `Q.centroid()` against `P` using the adaptive predicate.

If the planes are considered coplanar, it recursively partitions both children
and flattens away `Q`, choosing children based on whether the normals are aligned
or anti-aligned.

If not coplanar, it recursively partitions `Q`'s front and back children by `P`,
then builds two new fragments using `Q` as their partition plane.

Major topological issue: for non-coplanar planes, this routine does not
geometrically split the partition plane `Q` itself by `P`; it only repartitions
subtrees.  In a full BSP, partitioning a BSP by another plane requires careful
handling of the convex cell context and the interaction line between planes.
The current implementation may preserve tree logic in simple cases, but it is
not obviously a complete Naylor-style tree clipping implementation.

### Complement

`complement_tree` swaps `In` and `Out` leaves and recurses through partitions.
It does not reverse partition plane orientations.  For pure point-classification
semantics that is acceptable because the children are complemented in place; for
later boundary extraction, face-normal generation depends on child evaluations
and partition normals, so complemented solids can inherit normals whose direction
must be repaired by `OrientFaces` after mesh extraction.

### XOR/exclusion

`boolean_exclusion` computes:

1. `A - B`
2. `B - A`
3. union of those two results

This is algebraically valid for XOR but duplicates all weaknesses of subtraction
and union.  It is also more expensive than a direct leaf truth-table merge.

## BSP-to-mesh conversion details

`to_fv_surface_mesh()` converts the final BSP volume back to an
`fv_surface_mesh<T,I>`.

### Empty and universal leaves

- `root == nullptr` -> empty mesh.
- `root == Out` -> empty mesh.
- `root == In` -> returns a hard-coded cube from `[-1,1]^3`, calls
  `OrientFaces(mesh)`, rebuilds involved-face indices, and returns it.

The `In` case represents all of space, not a bounded solid.  Returning a unit
cube is not geometrically meaningful for boolean operations on CAD solids.  It
may occur if merging collapses a tree to uniform `In`, for example in some
pathological union/complement cases.

### Boundary extraction from partition nodes

For partition trees, the conversion computes a large finite square on each
boundary partition plane and clips it down:

1. `compute_tree_bbox_margin` inspects only partition plane centroids and returns
   the maximum absolute coordinate component.
2. `bbox_size = max_extent + max(max_extent * 0.1, 100)`.
3. `extract_boundary_faces` walks every partition node.
4. At each node, it evaluates the front and back subtrees as `In`, `Out`, or
   `Mixed`.
5. If the two sides differ in any way, including `Mixed` combinations, it creates
   an initial square on the node plane.
6. The square is clipped by ancestor planes according to the path from the root.
7. If one or both children are `Mixed`, the polygon is further clipped to leaf
   regions using descendant planes.
8. Surviving polygon fragments become `PolyFace` records.

This approach is not reconstructing the original mesh faces.  It reconstructs
faces implied by the BSP partition planes and the `In`/`Out` classification.
That is normal for BSP output, but correctness requires exact clipping and
consistent cell topology.  The implementation uses ordinary floating-point
polygon clipping and can duplicate coplanar vertices.

### Polygon clipping

`clip_polygon_by_plane` loops over polygon edges, classifies endpoints with the
adaptive predicate, pushes vertices to the front and/or back polygon when their
sign is `>= 0` or `<= 0`, and inserts an intersection point when signs strictly
cross.

Because coplanar vertices are pushed to both outputs, polygons lying on clipping
planes can be duplicated into both branches.  There is no symbolic perturbation,
no exact rational construction, and no explicit duplicate-vertex removal at the
polygon-fragment level.

### Triangulation

Each extracted polygon is triangulated by a fan from `poly[0]`.  Triangles whose
computed normal squared length is below `epsilon * scale^2` are skipped.

Fan triangulation is only guaranteed for convex polygons or suitably ordered
simple polygons.  BSP clipping of an initial convex square by halfspaces should
produce convex fragments in exact arithmetic, but duplicated coplanar vertices
and mixed-subtree fragment accumulation can produce degenerate or repeated
vertices that fan triangulation merely skips locally.

The `PolyFace::normal` field is computed but not used by `triangulate_fan`, so
triangle winding is inherited from the polygon vertex order rather than from the
stored intended face normal.  A final `OrientFaces(mesh)` is expected to repair
orientation.

### Vertex welding

The mesh builder welds vertices by rounding coordinates to a grid:

```cpp
weld_eps = sqrt(epsilon) * scale
key = round(coord / weld_eps) * weld_eps
```

This approximate welding can close small cracks, but it can also merge distinct
nearby CAD features.  It is scale-relative to the output bounding box, so the
same geometry at different scales can weld differently.  The key uses `T` values
inside `std::tuple<T,T,T>`, relying on exact equality of rounded floating values.

After faces are created, `OrientFaces(mesh)` is called and involved-face indices
are rebuilt.  There is no final explicit check that the output is closed,
manifold, non-self-intersecting, or geometrically equivalent to the boolean
operation.

## Interaction with adaptive predicates

The implementation does use Shewchuk-style adaptive predicates for these sign
decisions:

- point-vs-plane classification,
- triangle-vs-plane classification through point classifications,
- orientation checks in triangle filtering,
- edge-plane signed distances before interpolation.

However, adaptive predicates are used only for signs.  The algorithm still
constructs new geometry using floating-point interpolation, midpoint fallbacks,
polygon clipping, threshold-based ray casting, fan triangulation, and approximate
welding.  Therefore the engine is not robust in the exact-geometric-computation
sense.  It can make correct combinatorial decisions but still create coordinates
that do not satisfy those decisions exactly.

The most important impedance mismatch is that robust predicates and inexact
constructions are mixed without a mechanism such as rational coordinates,
filtered exact constructions, snap rounding with topology guarantees, or a
constrained arrangement data structure.  This is a common source of boolean
failures: the topology says an intersection vertex lies on a plane, but the
stored floating-point vertex may classify off that plane later.

## Failure handling

The code has three categories of failure behavior:

1. **Explicit exceptions during input conversion**: non-finite vertices,
   non-triangular faces after triangulation, invalid indices, degenerates,
   non-closed manifold, inconsistent orientation.
2. **Silent empty results**: raw empty shortcuts, all faces removed, empty BSP
   roots, `Out` roots, depth limit in BSP construction, invalid enum values.
3. **Silent approximate repair or degradation**: midpoint edge-plane fallback,
   skipped tiny triangles, approximate vertex welding, final orientation repair.

For high-precision CAD work, category 2 and 3 are problematic because they can
produce plausible but wrong meshes.  A CAD boolean engine should generally fail
loudly with enough context to diagnose the invalid input or unsupported
configuration.

## Fundamental soundness assessment

The high-level idea of BSP booleans is sound: represent solids as a partition of
space into `In` and `Out` cells, merge two classifications with boolean truth
tables, and extract their boundary.  But this implementation is not currently
sound enough for high-precision CAD use.

Key reasons:

- `BooleanMeshOp4` has an algebraic bug for `A - empty`, returning empty instead
  of `A`.
- The conversion uses randomized tree construction by default, making failures
  potentially nondeterministic.
- BSP building uses exact-ish predicates but inexact intersection construction.
- Ray-cast leaf classification is threshold-based, fixed-direction, and not
  robust against degeneracies.
- The depth limit returns `Out` silently.
- Boundary extraction reconstructs large clipped polygons from tree planes and
  relies on floating clipping, approximate welding, and final orientation repair.
- There is no final verifier proving the output is closed, manifold, and
  correctly oriented.
- The implementation appears to contain textual/code-quality defects in the BSP
  backend around `complement_tree` and `extract_boundary_faces` in the inspected
  source: an extra closing brace appears after the uniform-child collapse in
  `complement_tree`, and `fragments` is declared twice consecutively in
  `extract_boundary_faces`.  If these are present in the compiled source, the
  backend cannot compile as written; if hidden by local edits or preprocessor
  state elsewhere, they should still be cleaned up before reasoning about
  runtime behavior.

## Obvious improvement directions

The best replacement would be topologically sound by construction: construct the
arrangement of both input triangle meshes, classify cells, and emit boundary
faces from the classified arrangement.  In practice that means:

1. **Exact predicates plus exact or topology-preserving constructions**:
   use rational coordinates, expansion-backed constructions, or a controlled
   snap-rounding scheme with formal topology guarantees.
2. **Triangle-triangle intersection arrangement first**:
   split both meshes along all intersection curves so that output faces are
   selected from arrangement facets, not reconstructed from approximate BSP
   clipping after the fact.
3. **Halfedge or combinatorial surface representation**:
   maintain explicit vertices, directed edges, faces, twins, and shells so
   manifoldness can be checked and preserved locally.
4. **Cell classification with robust point location**:
   classify connected volumetric regions or surface facets using robust winding
   numbers / exact ray predicates with degeneracy handling, not ad-hoc fixed
   +X parity rays.
5. **Deterministic operation and diagnostics**:
   remove random tree order by default, replace silent depth-limit `Out` leaves
   with errors, and report operand-specific validation failures.
6. **Postcondition verification**:
   after every boolean, check finite vertices, valid indices, no degenerates,
   closed manifoldness, consistent orientation, and ideally absence of
   self-intersections.  If any postcondition fails, return a clear error rather
   than a malformed mesh.

If retaining the BSP approach, the minimum repairs should include fixing the
empty-right subtraction shortcut, making BSP construction deterministic by
default, replacing midpoint intersection fallbacks with a robust construction
policy, making depth overflow fatal, using scale-aware and degeneracy-aware
inside/outside classification, and adding final output verification.
