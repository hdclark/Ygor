# YgorMeshesBSPTree.cc Implementation Guide

This document describes the algorithm exactly as implemented in
`src/YgorMeshesBSPTree.cc` and its public interface in
`src/YgorMeshesBSPTree.h`. It is not a replacement design. It records what the
current code does, what state is expected at each stage, how errors are handled,
and where the implementation is numerically or topologically fragile.

The implementation intends to represent a closed 3D solid as a binary space
partitioning tree. Internal nodes store an oriented plane. Leaf nodes classify
the convex cell reached by the path through the planes as `In` or `Out`. The
boolean engine in `src/YgorMeshesBoolean4.cc` converts two facet-vertex meshes
to BSP volumes, applies one of the BSP boolean operations, and converts the
resulting BSP volume back to a facet-vertex mesh.

## Public Data Model

`bsp_plane<T>` stores exactly three anchor vertices. Its orientation is the
right-handed orientation of `(anchors[0], anchors[1], anchors[2])`.

- `normal()` returns `(b - a) x (c - a)`.
- `unit_normal()` normalizes `normal()` with `vec3<T>::unit()`.
- `centroid()` returns the arithmetic mean of the three anchors.

There is no validity flag on `bsp_plane`. A degenerate plane can still be stored;
callers are expected to prevent that. If the plane is degenerate, `unit_normal()`
may be undefined or produce invalid values depending on `vec3<T>::unit()`.

`bsp_tree_volume<T, I>::Node` has three states:

- `Partition`: the node stores a `partition_plane` and has `front` and `back`
  child subtrees.
- `In`: the cell is solid.
- `Out`: the cell is empty.

The front side is the side where `adaptive_predicate::orient3d(pa, pb, pc, pd)`
is positive for the node plane anchors `pa`, `pb`, and `pc`. Points with zero
orientation are routed to the front side in point classification and clipping.

Null child pointers are sometimes treated as `Out` by helper functions, but not
uniformly. Empty `bsp_tree_volume` objects have `root == nullptr`; an explicit
empty volume can also be represented by a non-null `Out` root.

## Numeric Primitives

### `plane_threshold<T>()`

This returns `epsilon(T) * 1024`. It is an absolute tolerance, not scaled to the
geometry except in a few downstream calculations. It is used for ray-triangle
intersection tests, offsetting sample points from partition planes, and some
degenerate-triangle checks.

This threshold is not robust across large or tiny coordinate scales. For large
coordinates it may be too small to separate sample points from planes. For tiny
geometry it may be large relative to the model.

### `classify_point(P, v)`

The code converts the three plane anchors and query point into C arrays and calls
`adaptive_predicate::orient3d(pa, pb, pc, pd)`. It returns:

- `+1` if the predicate is positive.
- `-1` if the predicate is negative.
- `0` if the predicate returns exactly zero.

This is the strongest numerical component in the BSP code. The sign of the
orientation determinant is computed by the adaptive predicate, which uses a fast
floating-point determinant when separated from roundoff and expansion arithmetic
near degeneracy. However, the predicate only gives a sign. It does not solve all
downstream problems caused by constructing new intersection vertices in ordinary
floating point.

### `classify_triangle(P, a, b, c)`

Each vertex is classified independently with `classify_point`. The triangle is:

- `Coplanar` if all three signs are zero.
- `Spanning` if at least one sign is positive and at least one sign is negative.
- `Back` if there are no positive signs and at least one negative sign.
- `Front` otherwise.

Vertices on the plane are treated as compatible with both sides for the purpose
of non-spanning classification. For example, signs `{+, 0, 0}` classify as
`Front`; signs `{- , 0, 0}` classify as `Back`.

### `intersect_edge_with_plane(P, a, b)`

This computes the signed orientation values `d0` and `d1` of edge endpoints `a`
and `b` against plane `P`, then computes `t = d0 / (d0 - d1)` and returns
`a + (b - a) * t`.

If `abs(d0 - d1) < epsilon(T) * (abs(d0) + abs(d1))`, it returns the midpoint of
the edge. This is a numeric fallback, not a topological guarantee. It can place
the returned point away from the true plane. The returned intersection point is
ordinary floating-point geometry and is not represented by exact constructions.

### `split_triangle(P, a, b, c, front, back)`

The triangle vertices are classified with `classify_point`.

If all signs are non-negative, the triangle is pushed to `front` only if at least
one vertex is strictly positive. If all signs are zero, it is discarded by this
helper.

If all signs are non-positive, the triangle is pushed to `back` only if at least
one vertex is strictly negative. If all signs are zero, it is discarded.

For a true spanning triangle, vertices with sign `>= 0` are placed in `pos` and
vertices with sign `< 0` are placed in `neg`. This means vertices exactly on the
plane are grouped with the front side.

If there is one `pos` vertex and two `neg` vertices, two intersection points are
computed from the positive vertex to each negative vertex. The positive side gets
one triangle. The negative side gets two triangles.

If there are two `pos` vertices and one `neg` vertex, two intersection points are
computed from the negative vertex to each positive vertex. The positive side gets
two triangles. The negative side gets one triangle.

The splitter does not preserve original vertex identity or exact edge topology.
It also does not explicitly remove zero-area split fragments.

## Ray Casting Used During Tree Construction

`ray_intersects_triangle` implements a Moller-Trumbore-style ray-triangle test
with direction supplied by the caller. The BSP code always uses `(+1, 0, 0)`.

The function rejects:

- Rays nearly parallel to the triangle, using `plane_threshold<T>()` on the
  determinant `a`.
- Intersections where barycentric `u` is less than the threshold or greater than
  `1 - threshold`.
- Intersections where barycentric `v` is less than the threshold or where
  `u + v > 1 - threshold`.
- Intersections with `t <= plane_threshold<T>()`.

This intentionally avoids counting hits on triangle edges, triangle vertices,
and hits extremely close to the ray origin. It also means the inside/outside
decision can be wrong when the fixed `+x` ray crosses exactly through mesh
vertices, lies in a face plane, or when the mesh scale makes the absolute
threshold inappropriate.

`count_ray_intersections` counts all triangles that pass this test.

`point_is_inside_mesh` returns odd parity of the count. It assumes a closed mesh
and uses no acceleration structure. It does not use winding number, exact segment
arrangements, randomized ray directions, or retry logic for ambiguous rays.

## Tree Ownership Helpers

`clone_node` deep-copies a node by calling `Node::clone()`.

`complement_tree` consumes a node and swaps `In` and `Out` leaves. Partition
nodes are kept with the same plane orientation; their front and back children are
recursively complemented. If both complemented children are non-partition leaves
of the same type, the partition is collapsed and one child is returned.

This complement is logically correct for a BSP cell classification tree: the
spatial partition does not need to change, only the leaf classifications.

## Partitioning One Tree By A Plane

`partition_tree(P, node)` consumes `node` and returns two trees:
`(front_fragment, back_fragment)`. The intent is to split an existing BSP volume
by a new plane `P` so a boolean merge can recurse with compatible spatial
subdivision.

If `node` is null, both fragments are null.

If `node` is a leaf, both fragments are new leaves of the same type. This means a
uniform cell remains uniform after subdivision.

If `node` is a partition node with plane `Q`, the code first tries to detect
coplanar planes:

- It computes unit normals for `P` and `Q`.
- It considers them parallel when `abs(dot) > 1 - epsilon(T) * 128`.
- If parallel, it classifies `Q.centroid()` against `P`.
- If the centroid is exactly on `P`, the planes are treated as coplanar.

For detected coplanar planes, the code partitions both children of `Q` by `P`
and then flattens `Q` out of the result. If normals are aligned, `Q.front` is
mapped to `P.front` and `Q.back` to `P.back`. If normals are anti-aligned, the
mapping is swapped. Missing fragments become `Out` leaves.

This coplanar flattening is heuristic. It uses unit-normal dot products and one
centroid classification. It does not prove two finite-precision planes are the
same plane under an exact plane identity scheme. It also combines child
fragments by choosing one non-null fragment, rather than computing a spatial
union of fragments if both are meaningful.

For non-coplanar planes, the code recursively partitions both children of `Q` by
`P`:

- `ff` and `fb` are the front and back fragments of `Q.front`.
- `bf` and `bb` are the front and back fragments of `Q.back`.

It builds the returned front fragment as plane `Q` with children `(ff, bf)`. It
builds the returned back fragment as plane `Q` with children `(fb, bb)`. Each
fragment is collapsed to a single leaf if both children are non-partition and
both are effectively the same with respect to `Out` status. The collapse test is
implemented as `ff_is_out == bf_is_out`, where null is treated as `Out`. Because
`ff_is_out` is false for `In`, this also collapses two `In` leaves. It does not
explicitly check equality of arbitrary non-partition leaf types beyond this
boolean.

The function does not geometrically clip the plane `Q`; it only rearranges BSP
subtrees. That is a standard part of BSP merging, but correctness depends on
`partition_tree` preserving the exact function represented by the original tree
on each side of `P`. The coplanar flattening and null handling are the fragile
parts.

## Uniform Collapse Helpers

`collapse_uniform` recursively collapses a partition when both children are
non-partition and have the same `Out` status. It is defined but not called by the
public operations in this file.

`subtree_uniform_eval` returns `In` or `Out` if an entire subtree evaluates to
one leaf type. It returns `Partition` as a sentinel for mixed subtrees. Null is
treated as `Out`.

`collapse_deep_uniform` recursively collapses a partition when both children
evaluate to the same uniform leaf type, even if a child still contains partition
nodes. Public boolean operations call this after `merge_bsp`.

## Boolean Merge

`merge_bsp(A, B, op)` consumes two trees and returns a new tree. `op` is:

- `0`: union.
- `1`: intersection.
- `2`: subtraction `A - B`.

Leaf cases are handled first.

If `A` is null or not a partition, `a_in` is true only for an explicit `In` leaf.
Null is therefore treated as `Out`.

- Union: `In union B` returns `In`; `Out union B` returns a clone of `B` or null.
- Intersection: `In intersection B` returns a clone of `B` or null;
  `Out intersection B` returns `Out`.
- Subtraction: `In - B` returns the complement of `B`; `Out - B` returns `Out`.

If `B` is null or not a partition, `b_in` is true only for explicit `In`.

- Union: `A union In` returns `In`; `A union Out` returns a clone of `A` or null.
- Intersection: `A intersection In` returns a clone of `A` or null;
  `A intersection Out` returns `Out`.
- Subtraction: `A - In` returns `Out`; `A - Out` returns a clone of `A` or null.

If both inputs are partition nodes, the algorithm selects `A`'s partition plane
`P`, partitions `B` by `P`, and recursively merges `A.front` with the front
fragment of `B` and `A.back` with the back fragment of `B`. It then creates a new
partition node with plane `P` and the two merged children, unless the children
are collapsible by the same non-partition same-`Out`-status test.

This is structurally similar to Naylor-style BSP merge. The main correctness
dependency is that both input trees already represent exact solids and that
`partition_tree` produces correct restricted functions. No explicit plane
selection optimization is performed during boolean merge; it always takes the
next plane from `A`.

## Mesh To BSP Conversion

`bsp_tree_volume<T, I>::from_fv_surface_mesh(mesh, seed)` converts a
facet-vertex mesh to a BSP volume.

### Input preprocessing and validation

If `mesh.faces` or `mesh.vertices` is empty, it returns an empty tree
(`root == nullptr`).

The input mesh is copied to `working_mesh`. The copy is triangulated by
`fv_surface_mesh::convert_to_triangles()`. That method uses a fan from vertex 0
for faces with more than three vertices, assumes those faces are planar, drops
faces with fewer than three vertices, and does not enforce convexity beyond the
class comment that faces are simplices.

The code checks that all vertices are finite after triangulation. If not, it
throws `std::invalid_argument`.

It removes degenerate faces with `remove_degenerate_faces()`. That removes faces
with fewer than three vertices, repeated first-three indices, or triangles
reported degenerate by `TriangleIsDegenerate`.

If all faces were removed, it returns an empty tree.

It then validates:

- All faces are triangles.
- All face indices are within `vertices.size()`.
- No degenerate faces remain.
- The mesh is a closed manifold by undirected edge counts: every unique edge
  must appear exactly twice, and there must be at least one unique edge.
- Face orientation is consistent by directed-edge pairing: paired manifold edges
  must appear in opposite directed order.

Failures throw `std::invalid_argument` with a message identifying the failed
condition.

These checks do not verify global outward orientation. A consistently inward
closed shell can pass. They also do not verify absence of self-intersection,
duplicated coincident shells, nested shell semantics, or geometric planarity of
pre-triangulation polygonal faces.

### Triangle record construction

The code iterates all triangular faces. For each face it loads the three vertex
positions and computes a cross-product normal. If `normal.sq_length()` is less
than `plane_threshold<T>()`, it performs an additional orient3d-based test using
`pd = v0 + dir`, where `dir` is the normal or `(1, 0, 0)` if the normal is also
too small. If that orientation is exactly zero, the face is skipped.

For all kept faces, it appends the triangle geometry to `all_tris` and appends a
`TriangleRec` containing the same vertices plus `bsp_plane(v0, v1, v2)` to
`tri_recs`.

If no triangle records remain, the function returns an empty tree.

Finally, it calls `build_bsp_from_triangles(std::move(tri_recs), all_tris, 0,
seed)` and wraps the returned root.

### `build_bsp_from_triangles`

This is the core mesh-to-tree builder. It takes the current triangle records,
the original complete triangle list for ray-cast classification, current depth,
and optional seed.

If the current triangle list is empty or `depth > 64`, it returns an `Out` leaf.
The depth limit silently classifies any unresolved region as outside. It does
not throw or report that construction failed.

At depth 0 only, it chooses a random seed. If the caller supplied `seed`, that
seed is used; otherwise `std::random_device` is used. It shuffles the input
triangles. This means tree shape and downstream numerical behavior are
non-deterministic unless a seed is supplied. Recursive calls pass the same seed
but do not shuffle again because `depth != 0`.

The first triangle in the current list supplies the partition plane `P`.

The code classifies every triangle against `P`:

- `Front`: append to `front_tris` and increment `front_non_coplanar`.
- `Back`: append to `back_tris` and increment `back_non_coplanar`.
- `Coplanar`: append to `front_tris` and increment `coplanar_cnt`.
- `Spanning`: split the triangle. Front split pieces are converted to new
  `TriangleRec` objects using their split vertices and a newly computed plane;
  each increments `front_non_coplanar`. Back split pieces are handled similarly
  and each increments `back_non_coplanar`.

Coplanar triangles are always stored on the front side for recursion bookkeeping.
They are not stored as boundary polygons at the current node. The current node's
plane itself is the only lasting record of the selected coplanar triangle plane.

The builder then handles several degenerate recursion cases.

If every triangle in the current list was coplanar with `P`, it samples two
points:

- `front_pt = P.centroid() + P.unit_normal() * plane_threshold<T>() * 2`.
- `back_pt = P.centroid() - P.unit_normal() * plane_threshold<T>() * 2`.

It classifies those points using odd-even ray casting against `all_tris`. It
creates `In` or `Out` leaves for each side. If both sides have the same leaf
type, it returns that leaf. Otherwise it returns a partition node at `P` with the
classified leaves.

The immediately following branch `front_non_coplanar == 0 && back_non_coplanar ==
0` performs the same sampling logic. Given the counting above, this is largely
redundant with the all-coplanar case.

If `front_non_coplanar == 0`, the front side has only coplanar triangles. The
back child is either built recursively from `back_tris` or, if `back_tris` is
empty, classified by ray-casting a back sample point. The front child is
classified by ray-casting the front sample point. If both children are identical
non-partition leaves, the leaf is returned; otherwise a partition node is
returned.

If `back_non_coplanar == 0`, the back side is classified by ray-casting the back
sample point. Before recursing on the front side, the code erases triangles from
`front_tris` that are still coplanar with `P`; the comment says this avoids an
infinite chain. The front child is either built recursively from the remaining
front triangles or ray-classified if none remain. Identical non-partition leaves
are collapsed.

If both sides have non-coplanar triangles, the builder recursively builds both
children. If both are identical non-partition leaves, it returns one child;
otherwise it returns a partition node with plane `P`.

### Builder soundness notes

The implemented builder is not a complete constructive solid geometry
classification algorithm for arbitrary closed triangle meshes. It uses surface
triangles to choose partition planes, but it does not maintain exact boundary
fragments as first-class topology. Coplanar faces are mostly used to choose a
plane and then discarded from recursion or assigned to the front side. Leaf
classification is recovered by ray-casting small offsets from selected planes
against the original mesh.

The result can be correct for simple, well-oriented, closed meshes under benign
plane orders, but it is not sound by construction. Main issues are:

- Leaf classification relies on a fixed-direction ray test against floating
  triangles rather than exact arrangement topology.
- The offset from a plane is `2 * epsilon * 1024`, not scaled by mesh extent in
  `build_bsp_from_triangles`, even though `compute_all_tris_extent` exists and
  is not used by the builder.
- Splitting creates approximate vertices and planes; subsequent exact
  orientation predicates operate on those approximate constructions.
- The 64-level recursion limit silently returns `Out`.
- The builder does not prove global outside/inside orientation or handle
  self-intersecting closed meshes.
- Random plane order means the same input can produce different trees unless a
  seed is provided.

## BSP To Mesh Conversion

`bsp_tree_volume<T, I>::to_fv_surface_mesh()` extracts a surface mesh from a BSP
classification tree.

If `root` is null, it returns an empty mesh. If `root` is an `Out` leaf, it also
returns an empty mesh.

If `root` is an `In` leaf, the represented solid is all of space, which is
unbounded. The implementation instead returns a hard-coded cube from `[-1, 1]`
on each axis, triangulates its six faces into twelve triangles, calls
`OrientFaces(mesh)`, rebuilds `involved_faces`, and returns it. This is a
placeholder bounded proxy, not an exact mesh for the represented volume.

For a partition tree, it computes `max_extent` from partition-plane centroids
only. It then sets:

`bbox_size = max_extent + max(max_extent * 0.1, 100)`.

This size is used to create a large initial square on each partition plane before
clipping. The value is not derived from all actual boundary vertices because the
BSP tree does not store boundary vertices directly. The minimum added size of
100 can create very large artifacts for small models.

### Boundary face extraction

`extract_boundary_faces(node, ancestors, side_stack, bbox_size, faces)` traverses
partition nodes.

For the current partition node, it evaluates each child subtree using a local
recursive lambda:

- Null is `Out`.
- `In` leaves are `In`.
- `Out` leaves are `Out`.
- A partition whose children evaluate to the same value evaluates to that value.
- Otherwise it evaluates to `Mixed`.

The current partition plane is considered a boundary candidate whenever the
front and back evaluations differ, including all `Mixed` combinations except
identical non-mixed cases.

For a boundary candidate, it creates a large square polygon on the current
partition plane using `make_initial_polygon`. The square is centered at the
plane centroid. Its in-plane axes are chosen by crossing the unit normal with
the x-axis unless the normal is too aligned with x, in which case the y-axis is
used.

The polygon is clipped against every ancestor plane. `side_stack[i]` records
whether traversal reached the current node through the ancestor front or back
child. If it went front, the front clipping result is kept; if it went back, the
back clipping result is kept.

After ancestor clipping, the code generates final fragments:

- `In` versus `Out`, either direction: the whole clipped polygon is kept.
- `In` versus `Mixed`: clip the mixed side to `Out` leaves.
- `Out` versus `Mixed`: clip the mixed side to `In` leaves.
- `Mixed` versus `In`: clip the mixed front side to `Out` leaves.
- `Mixed` versus `Out`: clip the mixed front side to `In` leaves.
- `Mixed` versus `Mixed`: keep front-`In`/back-`Out` fragments and
  front-`Out`/back-`In` fragments by applying `clip_polygon_to_leaf` to one side
  and then the other.

Each surviving fragment with at least three vertices is stored as a `PolyFace`.
The recorded normal is `node->partition_plane.unit_normal()` if the front side
evaluates to `In` or `Mixed`; otherwise it is the negative normal. This normal is
not used by the later triangulation step.

The traversal then recurses into front and back children, appending the current
node to the ancestor stack and appending the side taken to `side_stack`.

### Polygon clipping helpers

`clip_polygon_by_plane(P, poly, front, back)` walks each polygon edge. It pushes
the current vertex to `front` if its sign is non-negative and to `back` if its
sign is non-positive. If an edge changes sign strictly, it computes one
intersection point and appends it to both outputs.

Coplanar vertices are duplicated into both outputs. Edges with one endpoint
coplanar and the other off-plane do not add a separate intersection point.

`bsp_subtree_contains(sub, t)` returns whether a subtree contains a leaf of type
`t`; null is considered to contain `Out`.

`clip_polygon_to_leaf(sub, poly, target, out)` keeps regions of `poly` where the
subtree evaluates to `target`.

- If the subtree itself is the target leaf, it appends the full polygon.
- If the subtree is a different leaf, it discards the polygon.
- If the subtree is a partition, it clips the polygon by the partition plane and
  recurses on valid front and back polygons.
- If clipping produces no valid polygon, it classifies all original vertices. If
  all are on the front side only, it recurses front; if all are on the back side
  only, it recurses back.
- Otherwise it asks which child subtrees contain the target leaf and recurses
  into containing sides, possibly both.

The fallback using `bsp_subtree_contains` is heuristic. It can duplicate a
coplanar polygon into both child paths without an exact lower-dimensional cell
arrangement.

### Triangulation and vertex welding

After boundary extraction, each polygon fragment is triangulated by fan
triangulation from vertex 0. A triangle is skipped if `(b - a) x (c - a)` has
square length less than `epsilon(T) * scale * scale`, where `scale` is the max of
the three squared vertex lengths and 1. This is a geometric degeneracy filter.

If no triangles remain, the function returns an empty mesh.

Vertices are then welded by quantizing coordinates. The code computes a bounding
box over generated triangles, sets `scale = max(extent.x, extent.y, extent.z,
1)`, and sets `weld_eps = sqrt(epsilon(T)) * scale`. Each coordinate is rounded
to the nearest multiple of `weld_eps`; the rounded triple is used as a `std::map`
key. The original unrounded vertex is stored for the first occurrence of each
key. Faces are emitted as triples of welded vertex indices.

Finally, `OrientFaces(mesh)` is called, `involved_faces` is rebuilt, and the mesh
is returned.

### Extraction soundness notes

The extraction algorithm reconstructs boundary polygons by intersecting each BSP
partition plane with a large artificial square and clipping it by ancestor and
descendant planes. This can work for bounded convex cells, but it is not an exact
arrangement extraction algorithm.

Known fragilities include:

- The bounding square size is heuristic and can produce artifacts for disjoint
  intersections or unbounded/misclassified regions.
- The code does not use the stored `PolyFace::normal` during triangulation, so
  orientation is deferred entirely to `OrientFaces`.
- Fan triangulation assumes each extracted polygon is simple and suitably
  ordered. It does not handle holes in a polygon fragment.
- Vertex welding uses coordinate quantization, which can merge distinct nearby
  vertices or fail to merge geometrically identical vertices if roundoff differs
  across quantization boundaries.
- Coplanar clipping paths can duplicate polygons or keep ambiguous regions based
  on subtree leaf containment rather than exact planar topology.

The existing tests acknowledge this: disjoint intersections may leave a
non-empty BSP tree and mesh extraction may produce large-scale clipping
artifacts.

## Boolean4 Integration

`BooleanMeshOp4(lhs, rhs, op)` is a thin wrapper:

- If the left mesh is empty, union and exclusion return `rhs`; intersection and
  subtraction return an empty mesh.
- If the right mesh is empty, union and exclusion return `lhs`; intersection and
  subtraction return an empty mesh.
- Otherwise both meshes are converted with `bsp_tree_volume::from_fv_surface_mesh`.
- The requested boolean operation is applied to the two BSP trees.
- The result is converted back with `to_fv_surface_mesh()`.

This wrapper does not pass a deterministic seed. Therefore non-empty conversions
use `std::random_device` at the root of each conversion, so boolean results can
vary between runs.

The wrapper performs no additional validation before conversion. Validation
errors from `from_fv_surface_mesh` propagate as exceptions.

## Tests In This Repository

`tests2/YgorMeshesBSPTree.cc` checks construction, empty cases, simple
tetrahedron and cube round trips, boolean operations on overlapping tetrahedra
and cubes, some chain operations, nested cube shells, near-degenerate geometry,
and separated cubes.

The tests mostly assert that outputs are non-empty and sometimes have positive
absolute signed volume. They do not verify exact expected topology, exact volume,
closedness, manifoldness, absence of self-intersections, deterministic output,
or exact boolean semantics. Some comments explicitly allow known extraction
artifacts for disjoint intersections.

## Failure Handling Summary

Explicit exceptions are thrown only during mesh-to-BSP input validation:

- Non-finite vertices.
- Non-triangular faces after triangulation.
- Out-of-range face indices.
- Degenerate faces after removal.
- Mesh is not a closed manifold.
- Face orientations are not consistently paired across edges.

Silent fallback or silent failure cases include:

- Empty input or all removed triangles returns an empty tree.
- Builder recursion deeper than 64 returns `Out`.
- Degenerate or near-degenerate edge-plane intersections can return edge
  midpoints.
- Ambiguous ray intersections are skipped rather than retried.
- A root `In` tree converts to a placeholder cube.
- Mesh extraction can return an empty mesh when no triangles survive.

## Impedance Mismatches

The surrounding Ygor mesh type is a facet-vertex boundary representation. It can
store arbitrary polygonal faces, disconnected vertices, optional normals and
colors, and faces whose orientation is not globally enforced by the type.

The BSP converter requires a stricter object: finite, triangular after fan
triangulation, non-degenerate, closed, manifold, and consistently oriented.
Disconnected vertices are allowed because validation and conversion only inspect
faces and referenced vertices.

The adaptive predicate subsystem gives robust signs for orientation tests, but
the BSP implementation also needs robust constructions and robust topology. It
constructs split points, ray sample points, large clipping polygons, and welded
vertices using ordinary floating-point arithmetic. This is the main mismatch:
exact or adaptive predicates are used to ask questions about geometry that has
already been perturbed by inexact constructions.

The verification helpers check local edge manifoldness and orientation
consistency by indices. They do not check geometric self-intersection. A
self-intersecting mesh can satisfy index manifold checks while not bounding a
well-defined solid under odd-even ray casting.

## Fundamental Soundness Assessment

The implemented boolean pipeline is not fundamentally sound for high-precision
CAD as written. It is a plausible experimental BSP classifier/extractor for
simple closed triangle meshes, but it does not provide a topologically exact
solid arrangement.

The most important unsound elements are:

- Mesh-to-BSP leaf classification depends on ray casting against the original
  mesh instead of constructing exact volumetric cells from the triangle plane
  arrangement.
- Boolean merge assumes the BSP trees already represent exact inside/outside
  functions, but the builder can misclassify leaves and the extractor can create
  artifacts.
- Split vertices and clipped polygons are floating-point constructions without
  symbolic identity, exact rational coordinates, snap rounding guarantees, or
  topological reconstruction.
- Coplanar and near-coplanar cases are handled by heuristics rather than exact
  planar arrangements.
- Failure modes often return `Out`, empty meshes, placeholder bounded meshes, or
  artifacts rather than reporting a clear end-user failure.

For CAD use, the code should be treated as diagnostic or prototype quality until
the construction and extraction algorithms are replaced or substantially
rewritten.

## Improvement Directions

The best replacement would make topology primary and floating-point coordinates
secondary. Possible directions:

- Build an explicit plane arrangement or cell complex. Classify cells by robust
  topology, then extract boundary faces from adjacent `In`/`Out` cells. This
  avoids repeated ray classification of small offsets.
- Use exact or filtered exact constructions for plane-plane-plane vertices and
  edge-plane intersections, not only exact predicates.
- Represent split vertices symbolically, for example by references to original
  primitives or planes, so coincident constructions share identity by design.
- For triangle meshes, first build a robust triangle-triangle intersection graph
  and planar subdivisions per face, then classify resulting surface patches and
  stitch them into a closed boundary. This is closer to a robust B-rep boolean
  than the current BSP extraction.
- If retaining BSP, store boundary fragments on partition planes and maintain
  exact planar subdivisions during insertion. Do not recover all leaves by
  ray-casting sample points.
- Replace fixed `+x` ray casting with an exact winding-number or topological
  shell classification routine. If ray casting remains, use scale-aware offsets,
  randomized or certified non-degenerate rays, and explicit ambiguous-hit retry.
- Replace silent `Out` at depth limit with a reported construction failure that
  includes recursion depth, triangle count, and the active plane.
- Make `BooleanMeshOp4` pass a deterministic seed or expose one to callers so
  failures are reproducible.
- Strengthen tests to verify expected volumes, closed manifold output,
  orientation, connected components, and known exact cases such as disjoint
  intersection producing an empty mesh.
