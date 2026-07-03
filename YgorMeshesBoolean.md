# `src/YgorMeshesBoolean.cc` implementation guide

This document describes the boolean engine exactly as implemented in
`src/YgorMeshesBoolean.cc` at the time this note was written. It is not a
specification of an ideal CAD boolean. The current implementation is a uniform
voxel classifier and boundary extractor: it does **not** compute exact triangle
intersection curves, does **not** preserve input vertices or faces, and does
**not** emit an exact b-rep boolean result. It samples the boolean solid on a
fixed Cartesian grid, marks grid cells as occupied or empty, and emits the
axis-aligned boundary of the occupied cells as triangles.

## Executive summary

The implemented algorithm is:

1. Validate `max_depth` and `boundary_scale`.
2. Handle empty-input special cases.
3. Copy each non-empty input mesh, triangulate it, remove degenerate faces,
   validate finite vertices and closed manifold edge counts, orient faces, and
   build an R-tree over triangle axis-aligned bounding boxes.
4. Build a cubic computational domain around the union of both mesh bounding
   boxes.
5. Split the cube into `2^max_depth` cells per axis. Because `max_depth` is
   constrained to `[1, 8]`, the grid is at most `256^3 = 16,777,216` cells.
6. For every cell, determine whether its axis-aligned box touches any triangle
   in the left or right mesh.
7. Flood-fill all connected regions of cells that do **not** touch either mesh.
   Classify one representative point per such region by ray-parity tests
   against each mesh, then apply the requested boolean truth table to the whole
   component.
8. Classify mesh-touching cells individually by their cell centre.
9. Emit one quad, split into two triangles, for each exposed face between an
   occupied cell and an unoccupied or out-of-domain neighbour.
10. Remove disconnected vertices, recreate the `involved_faces` index, compute
    vertex normals, and attach metadata.

Topologically, the result is the boundary of a union of closed grid voxels.
This makes the output closed for the occupied-cell set under normal execution,
but it is a blocky approximation of the desired boolean and can change topology
relative to the true input solids whenever the grid resolution is insufficient,
when a thin feature falls between sample points, or when the touch-cell barrier
is not an adequate separator of inside/outside free-space components.

## Types and conventions used by the implementation

### `fv_surface_mesh<T, I>`

The boolean functions operate on `fv_surface_mesh<T, I>`. The mesh stores:

- `vertices`: 3D coordinates.
- `faces`: a vector of face vertex-index lists. Face orientation is not enforced
  by the data structure itself.
- `involved_faces`: an optional derived vertex-to-face adjacency index.
- `metadata`: string metadata.

`convert_to_triangles()` rewrites polygonal faces as a fan from vertex 0 and
silently skips faces with fewer than 3 vertices. The conversion assumes planar
facets and does not introduce intersection-aware vertices. `remove_degenerate_faces()`
then removes sub-triangle facets, while the boolean validator separately rejects
triangles with repeated vertex indices.

### Predicate functions

`point_on_triangle()` and `segment_intersects_triangle_interior()` use
`orient_sign()`. In `YgorMath.cc`, the 3D overload delegates to an adaptive
Shewchuk-style `orient3d` predicate and returns only its sign. The 2D overload
embeds the points in the `z = 0` plane and also uses `orient3d`. These signs are
robust for orientation decisions on the floating-point input coordinates, but
this boolean engine combines them with non-exact grid construction, AABB tests,
ray direction choices, and centre sampling. Therefore exact orientation
predicates do not make the overall boolean exact.

### R-tree use

Each prepared mesh owns an `rtree<T>` containing one entry per triangle, keyed by
that triangle's axis-aligned bounding box. Queries are broad-phase filters only;
all geometric decisions after a query are made by local triangle tests.

## Helper functions, in source order

### `prepared_mesh<T, I>`

`prepared_mesh` bundles a copied/preprocessed mesh, an R-tree face index, and a
mesh bounding box. This avoids modifying caller-owned input meshes and provides
fast candidate lookup for point, ray, and cell queries.

State invariant after `prepare_mesh()` succeeds for a non-empty mesh:

- `mesh.faces` are triangles.
- degenerate faces removed by the mesh utility are gone.
- every face index is in range.
- every undirected edge occurs exactly twice according to vertex indices.
- faces have been reoriented consistently by `OrientFaces()` if possible.
- `mesh.involved_faces` has been rebuilt.
- `face_index` contains every triangle bbox.

### `make_undirected_edge(I a, I b)`

This canonicalizes an edge by sorting the two endpoint indices. It is used only
for topological edge counting in `validate_closed_triangular_mesh()`.

Important limitation: this is index-based, not coordinate-based. Two coincident
vertices with different indices are treated as different vertices and therefore
do not close an edge. Conversely, duplicate faces sharing the same indices can
satisfy some edge-count patterns even if the geometry is invalid in other ways.

### `triangle_bbox(mesh, face_idx)`

Fetches the three vertices of a triangular face and returns the componentwise
minimum and maximum corners. It uses `.at()` for bounds checking. It assumes the
face has exactly three entries; the caller enforces this during preparation.

Geometric purpose: broad-phase acceleration for triangle candidate searches.
The bbox is exact with respect to the rounded coordinate values stored in the
mesh, but it has no tolerance expansion. A point or cell exactly outside the
floating-point bbox is not considered a candidate.

### `mesh_bbox(mesh)`

Computes the componentwise bounding box of all vertices, including disconnected
vertices. If `vertices` is empty it returns a default `index_bbox<T>()`.

Implication: because `prepare_mesh()` does not call `remove_disconnected_vertices()`
on non-empty meshes, disconnected vertices in the input can expand the boolean
computational domain even though they do not participate in any face.

### `validate_closed_triangular_mesh(mesh, name)`

Validation is performed after triangulation and degenerate-face removal.

The function:

1. Allows an empty face list without further checks.
2. Rejects any non-finite vertex.
3. Requires every face to have exactly three indices.
4. Requires every referenced vertex index to be in range.
5. Rejects triangles with repeated vertex indices.
6. Counts every undirected triangle edge.
7. Requires the number of faces to be even. This follows from the closed
   triangular-manifold relation `3F = 2E`, but is only a necessary condition.
8. Computes `expected_edges = faces.size() * 3 / 2` and requires the number of
   unique undirected index-edges to equal that value.
9. Requires every undirected edge count to be exactly 2.

Failure handling: violations throw `std::invalid_argument` with a brief message
that names the left or right mesh.

Robustness and topology notes:

- The check verifies closedness in an index-incidence sense, not a full embedded
  2-manifold. It does not detect self-intersections, inverted components,
  duplicated coincident surfaces with distinct indices, zero-area triangles with
  distinct indices, bow-tie vertices, inconsistent shell nesting, or coordinate
  cracks that are not represented by shared indices.
- The function rejects boundary and non-manifold edges.
- It does not require a single connected component, so multiple disconnected
  closed surfaces are allowed.

### `mesh_coord_eps(bounds)`

Computes `sqrt(epsilon) * scale`, where `scale` is the largest box extent but at
least 1. This epsilon is passed to `OrientFaces()`.

The boolean classifier itself mostly does not use this epsilon. Its main effect
is to allow the orientation pass to merge/compare nearby vertices according to
that routine's own logic.

### `dominant_axis(n)` and `project_drop_axis(v, axis)`

`dominant_axis()` selects the coordinate axis with the largest absolute normal
component. `project_drop_axis()` projects 3D coordinates to 2D by dropping that
axis.

Purpose: when a point is known to be coplanar with a triangle, the code tests
whether the point lies in the triangle by projecting to the coordinate plane
where the triangle has the largest projected area. This avoids the worst
projection for non-degenerate triangles.

Limitation: if the triangle normal is zero, axis 0 is selected by the tie logic,
and the subsequent 2D triangle test returns false if the projection is
collinear. `validate_closed_triangular_mesh()` rejects repeated indices, but it
does not reject distinct collinear or coincident triangle vertices.

### `point_on_triangle(p, a, b, c)`

This tests whether point `p` is on a triangle including its boundary:

1. `orient_sign(a, b, c, p) != 0` returns false, so exact coplanarity under the
   adaptive predicate is required.
2. Compute the triangle normal `(b - a).Cross(c - a)` using floating-point
   arithmetic.
3. Drop the dominant normal axis.
4. Use `point_in_triangle_or_on_boundary()` on the 2D projection.

Geometric purpose: classify cell centres that lie exactly on the mesh boundary
as inside, and support point-on-boundary queries before ray casting.

Numerical note: this is exact-ish for orientation signs but not tolerant. A
point that should be on a surface in real arithmetic but is slightly off because
of previous floating-point construction will not be considered on the boundary.
For cell centres generated from the grid, exact boundary hits are possible but
not guaranteed.

### `segment_intersects_triangle_interior(p, q, a, b, c)`

This is the ray/segment crossing primitive. It returns true only for a strict
intersection through the open interior of triangle `abc`:

1. Compute the signs of `p` and `q` relative to the triangle plane.
2. If either endpoint is coplanar with the triangle plane, return false.
3. If both endpoints are on the same side of the plane, return false.
4. Compute orientation signs of the triangle vertices relative to the directed
   segment and each triangle edge plane: `orient_sign(p, q, a, b)`,
   `orient_sign(p, q, b, c)`, and `orient_sign(p, q, c, a)`.
5. If any of these signs is zero, return false. This excludes intersections on
   triangle edges or vertices.
6. Return true if all three signs are positive or all three signs are negative.

Purpose: count parity crossings without double-counting a ray that hits a mesh
edge shared by two triangles. The implementation avoids ambiguous boundary
crossings by ignoring all non-strict triangle hits.

Important limitation: if a ray passes through a vertex, along an edge, or lies
in a triangle plane, crossings are ignored rather than perturbed or resolved
symbolically. The caller mitigates this by voting across three fixed ray
directions, but there is no guarantee that two or three rays cannot be
ambiguous for adversarial CAD geometry.

### `point_on_mesh_boundary(p, prep)`

Queries the R-tree with the degenerate bbox `[p, p]`, then tests candidate
triangles with `point_on_triangle()`. It returns true on the first containing
triangle.

Implications:

- This depends on the R-tree `intersects` logic considering a point bbox to
  intersect a triangle bbox when the point is exactly on a bbox boundary.
- No tolerance expansion is applied.
- It treats a boundary point as inside in `point_inside_mesh()`.

### `cast_parity_ray(p, q, prep)`

Queries the R-tree with `index_bbox<T>(p, q)`, counts strict triangle-interior
intersections between segment `p -> q`, and returns true if the count is odd.

The query box is built from two corners `p` and `q`; it is assumed that
`index_bbox<T>(p, q)` normalizes min/max internally. If it does not, rays in
some directions could query an invalid or inverted box. The three implemented
ray directions are all positive in x, y, and z, so `q` is componentwise greater
than `p`; this avoids the issue for current code.

### `point_inside_mesh(p, prep, far_distance)`

Classifies a point as inside or outside a prepared mesh:

1. An empty mesh is outside.
2. A point exactly on the mesh boundary is inside.
3. Three fixed non-axis-aligned directions are normalized.
4. For each direction, cast a segment from `p` to `p + dir * far_distance` and
   take parity of strict triangle crossings.
5. Return inside if at least two of the three rays are odd.

`far_distance` is set by the main algorithm to `4 * side`, where `side` is the
expanded cubic domain side length. Since the domain encloses both meshes, this
should send the endpoint well outside the domain for the positive ray
directions.

Robustness notes:

- This is a parity classifier. It does not use face normals and therefore does
  not require outward orientation for point containment.
- It assumes closed embedded surfaces. Self-intersections make parity semantics
  ambiguous.
- Treating boundary as inside affects boolean semantics near coincident or
  touching surfaces.
- Fixed ray directions can fail on geometries specially aligned to those rays.
  A robust CAD boolean would need symbolic perturbation, exact arrangement
  construction, or a topological point-location method in a known cell complex.

### `triangle_intersects_aabb(v0, v1, v2, box_min, box_max)`

This is a separating-axis test between a triangle and an axis-aligned box:

1. Translate the triangle so the box centre is the origin.
2. Compute box half extents and triangle edge vectors.
3. Test separation on the x, y, and z box axes.
4. Test separation on the triangle normal.
5. Test separation on the 9 cross-product axes between box axes and triangle
   edges, skipping near-zero axes using `numeric_limits<T>::min()`.
6. If no separating axis is found, return true.

Purpose: mark grid cells whose volume intersects or touches an input triangle.
This creates a one-cell-thick barrier around the mesh that prevents flood-fill
components from crossing the surface.

Numerical notes:

- The SAT computations are ordinary floating-point arithmetic, not adaptive
  predicates.
- Degenerate triangles can produce a zero normal and many skipped axes,
  potentially over-reporting touches.
- Touching is considered intersection because separation uses strict `lo > r`
  and `hi < -r` checks.

### `cell_touches_mesh(cell_min, cell_max, prep)`

Queries triangle bboxes overlapping the cell bbox and applies
`triangle_intersects_aabb()` to each candidate. Returns true on the first true
intersection.

Purpose: decide whether a grid cell is part of the mesh-touching barrier. These
cells are not flood-filled with free-space components; they are classified
individually by their centre.

### `prepare_mesh(input, name)`

The preparation pipeline is:

1. Copy the input mesh.
2. Convert copied faces to triangles.
3. Remove degenerate faces.
4. Allocate an empty R-tree.
5. If no faces remain, compute bounds and return.
6. Validate the closed triangular mesh.
7. Compute bounds.
8. Compute orientation epsilon from bounds.
9. Call `OrientFaces(out.mesh, orient_eps)` and throw if it returns false.
10. Recreate `involved_faces`.
11. Insert every triangle bbox into the R-tree, storing the face index as
    auxiliary data.
12. Return the prepared mesh.

Important impedance mismatches:

- The boolean's closed-manifold validator is index-based, while `OrientFaces()`
  uses an epsilon-dependent representative map. A mesh may pass one notion of
  topology and be modified or interpreted under another.
- Face orientation is computed but the inside/outside ray parity classifier does
  not rely on normals. Orientation mainly affects normals on copied empty-case
  outputs and any downstream consumers.
- Triangulating a non-triangular face by a simple fan can introduce diagonals
  that affect closed-manifold edge counts. A closed quad mesh can become a
  closed triangle mesh only if the introduced internal diagonals occur twice in
  compatible places; otherwise the triangulated representation may fail
  validation or alter topology.

### `copy_mesh_for_empty_case(input)`

Used when one boolean operand has no faces and the result should be the other
mesh. It:

1. Copies the mesh.
2. Triangulates.
3. Removes degenerate faces.
4. Removes disconnected vertices.
5. If faces remain, orients faces, rebuilds `involved_faces`, and computes
   vertex normals.
6. Returns the mesh.

This path preserves the surviving input geometry after triangulation; it does
not voxelize. Note that callers sometimes pass `rhs_prep.mesh` or
`lhs_prep.mesh`, so the mesh may have already been triangulated, validated, and
oriented before this copy.

### `eval_boolean(inside_lhs, inside_rhs, op)`

Truth table:

- `Union`: `inside_lhs || inside_rhs`.
- `Intersection`: `inside_lhs && inside_rhs`.
- `Exclusion`: `inside_lhs != inside_rhs` (symmetric difference/XOR).
- Any other operation value, including the intended `Subtraction`: `inside_lhs && !inside_rhs`.

The template parameter `T` is unused.

### `emit_boundary_mesh(occupied, resolution, domain_min, h)`

This converts occupied grid cells to a triangle mesh:

1. Return an empty mesh if the occupancy array is empty.
2. Define linear indexing and neighbour occupancy lookup. Out-of-domain
   neighbours are treated as unoccupied.
3. Maintain a map from integer lattice coordinates `(i, j, k)` to output vertex
   indices. This shares vertices exactly between adjacent emitted quads.
4. For every occupied cell:
   - create or fetch the eight lattice vertices at its corners;
   - for each of the six directions, if the neighbour cell is unoccupied or
     outside the domain, emit that cell face as a quad split into two triangles.
5. Remove disconnected vertices, recreate `involved_faces`, compute normals,
   and return.

The emitted mesh is axis-aligned and lies on voxel cell faces. Every quad is
split along a fixed diagonal. The function does not call `OrientFaces()` on the
output, so the correctness of final face winding depends entirely on the hard-
coded vertex order for each of the six emitted quads.

Topological consequence: if the occupied array is any finite set of voxels, the
emitted boundary is the cubical-complex boundary triangulated into faces. This
is a closed surface in the combinatorial sense, but it may be non-manifold at
vertices or edges for certain voxel configurations. For example, cells touching
only at a corner can produce disconnected surface sheets sharing a vertex
coordinate after vertex sharing by lattice coordinate, which can be problematic
for strict 2-manifold CAD requirements.

## Main algorithm: `boolean_mesh_op_impl()`

### Parameter validation

The implementation rejects `max_depth < 1` and `max_depth > 8`. The grid
resolution is later computed as `1 << max_depth`, so valid resolutions are
2, 4, 8, 16, 32, 64, 128, and 256 cells per axis.

It also rejects negative `boundary_scale`. A zero boundary scale is accepted.

### Empty mesh cases

The function checks the original `lhs.faces` and `rhs.faces`, not the prepared
post-triangulation face lists.

- If both face lists are empty, return an empty mesh.
- If `lhs.faces` is empty:
  - `Intersection` and `Subtraction` return empty.
  - `Union` and `Exclusion` prepare `rhs` and return a copied/triangulated/
    oriented version of `rhs`.
- If `rhs.faces` is empty:
  - `Intersection` returns empty.
  - `Union`, `Exclusion`, and `Subtraction` prepare `lhs` and return a copied/
    triangulated/oriented version of `lhs`.

Semantic note: XOR with an empty solid is the non-empty solid, and subtraction
by empty is the left solid, so these special cases match set-theoretic boolean
semantics. However, if a mesh has non-empty original faces that are all removed
by triangulation/degenerate-face removal, it bypasses these special cases and is
handled as an empty prepared mesh later.

### Preparing both inputs

For non-empty original face lists, both operands are prepared with the pipeline
described above. Any preparation failure is propagated as `std::invalid_argument`.

At this point, no triangle-triangle intersections have been computed. The two
meshes remain independent; there is no arrangement, splitting, snapping, or
coincident-surface handling.

### Computational domain construction

The algorithm computes the union of prepared mesh bounding boxes, then chooses a
cubic domain:

1. `extent = combined.max - combined.min`.
2. `side = max(extent.x, extent.y, extent.z)`.
3. Reject if `side` is not positive.
4. `expansion = side * boundary_scale`.
5. `centre = (combined.min + combined.max) / 2`.
6. `side += 2 * expansion`.
7. `domain_min = centre - (side, side, side) / 2`.
8. `resolution = 2^max_depth`.
9. `h = side / resolution`.
10. `far_distance = 4 * side`.

The cubic domain is centred on the combined input bbox. `boundary_scale` expands
it uniformly. With `boundary_scale == 0`, the outermost input surfaces may lie
on the domain boundary; emitted voxel faces also use out-of-domain as empty.

### Arrays

Four `uint8_t` arrays of length `resolution^3` are allocated:

- `occupied`: final boolean occupancy per cell.
- `touches_lhs`: whether the cell intersects/touches any left triangle.
- `touches_rhs`: whether the cell intersects/touches any right triangle.
- `visited`: flood-fill bookkeeping.

At maximum depth 8, these arrays together use about 64 MiB before allocator
and vector overhead; runtime is dominated by cell-to-mesh touch checks and
point classifications.

### Indexing utilities

`linear_index(i, j, k)` maps grid coordinates to a flat index as
`(k * resolution + j) * resolution + i`.

`cell_min_for(i, j, k)` returns the world-space minimum corner of a cell.

`cell_centre_for(i, j, k)` returns the cell centre by adding `(h, h, h) / 2`.

`unpack_index(idx, i, j, k)` reverses the flat index mapping.

### Touch classification pass

The triple loop over all grid cells:

1. Computes each cell bbox.
2. Calls `cell_touches_mesh()` against the left mesh; if true, sets
   `touches_lhs[idx] = 1`.
3. Calls `cell_touches_mesh()` against the right mesh; if true, sets
   `touches_rhs[idx] = 1`.

This pass creates the barrier used by later flood filling. It is conservative
with respect to the SAT result but limited by floating-point robustness. It does
not determine occupied status directly.

### Occupancy classification and flood fill

The algorithm iterates every cell index as a possible seed.

#### Mesh-touching seed cells

If a cell touches either mesh, it is not flood-filled. The code:

1. Marks it visited.
2. Computes its centre point.
3. Classifies that centre against the left mesh.
4. Classifies that centre against the right mesh.
5. Applies `eval_boolean()`.
6. Marks only that cell occupied if the truth table returns true.

Thus all mesh-touching cells are classified independently by centre sampling.
A triangle can pass through a cell whose centre is on either side of the true
surface; the whole cell is nevertheless assigned one boolean value.

#### Non-touching seed cells

For cells touching neither mesh, the code performs a depth-first flood fill over
6-connected neighbours that also touch neither mesh and are unvisited.

After collecting a component:

1. It picks `component.front()` as the representative cell.
2. It classifies that cell centre against both meshes.
3. It applies `eval_boolean()`.
4. If true, it marks every cell in the component occupied.

Purpose: avoid expensive point-in-mesh classification for every free-space cell.
The touch-cell barrier is assumed to separate regions whose inside/outside
classification cannot change without crossing an input surface.

Soundness caveat: this assumption is only as good as the touch barrier. If a
surface is missed by `triangle_intersects_aabb()` or if topology requires more
than 6-connected cell separation, a component can leak between regions and be
classified incorrectly. Also, all cells touching a mesh are removed from the
flood-fill graph, which thickens surfaces by up to one cell and shifts the final
boundary relative to the intended surface.

### Boundary emission and metadata

After occupancy is complete, `emit_boundary_mesh()` converts the occupied cells
to triangles. The output metadata records:

- `BooleanMaxDepth` as the integer depth string.
- `BooleanBoundaryScale` as the boundary scale string.
- `BooleanOperation` as `Union`, `Intersection`, `Exclusion`, or `Subtraction`.

No validation is run on the emitted mesh before return.

## Public API wrappers and explicit instantiations

The public template functions are thin wrappers:

- `BooleanMeshOp()` calls `boolean_mesh_op_impl()`.
- `BooleanUnion()` calls `BooleanMeshOp(..., MeshBooleanOperation::Union, ...)`.
- `BooleanIntersection()` uses `Intersection`.
- `BooleanExclusion()` uses `Exclusion`.
- `BooleanSubtraction()` uses `Subtraction`.

Unless `YGOR_MESHES_BOOLEAN_DISABLE_ALL_SPECIALIZATIONS` is defined, the file
explicitly instantiates these functions for:

- `float, uint32_t`
- `float, uint64_t`
- `double, uint32_t`
- `double, uint64_t`

## Failure handling summary

The implementation throws `std::invalid_argument` for:

- `max_depth` outside `[1, 8]`.
- negative `boundary_scale`.
- non-finite vertices during validation.
- non-triangular faces after conversion.
- out-of-range face indices.
- repeated indices in a triangle.
- odd face count in a supposed closed triangular manifold.
- incorrect unique-edge count.
- any edge count other than 2.
- failure of `OrientFaces()` in preparation or empty-case copying.
- non-positive combined bounding-domain side length.

Other potential failures are not converted to user-oriented errors:

- `.at()` can throw `std::out_of_range` if an invariant is broken internally.
- `std::any_cast<size_t>()` can throw if R-tree auxiliary data has unexpected
  type, though this file inserts only face indices.
- Memory allocation can fail for the grid arrays.
- Arithmetic overflow of `resolution^3` is avoided by the `max_depth <= 8`
  guard for normal platforms, but the code does not explicitly check it.

## Is the implemented algorithm fundamentally sound for high-precision CAD?

For exact b-rep CAD booleans, no. The implemented algorithm is fundamentally a
voxelization/resampling algorithm. It can be useful as an approximate solid
boolean or as a fallback visualization/preview method, but it cannot guarantee
an exact manifold b-rep result corresponding to the two input meshes.

Main reasons:

1. **No exact arrangement is constructed.** Triangle intersections, overlap
   curves, coincident patches, and split faces are never computed.
2. **Resolution controls correctness.** Features thinner than a cell, gaps
   smaller than a cell, and close surfaces can disappear, merge, or change
   topology.
3. **Output geometry is unrelated to input faces.** Returned surfaces are
   axis-aligned voxel boundaries, not subsets or subdivisions of the original
   b-rep surfaces.
4. **Centre sampling is not conservative.** Mesh-touching cells are classified
   by one point, so a cell cut by the actual boundary is assigned entirely to
   one side.
5. **Ray casting has degeneracy cases.** Fixed rays plus strict interior
   crossing tests avoid some double-counting but do not provide symbolic
   perturbation guarantees.
6. **Input validation is incomplete for embedded manifoldness.** The edge
   handshake catches many combinatorial boundary/non-manifold problems but not
   self-intersection, zero-area distinct-index faces, or invalid shell nesting.
7. **Voxel boundary manifoldness is not guaranteed in the strict b-rep sense.**
   Cubical boundaries can share vertices in ways that are not acceptable for
   all manifold-surface consumers.

## Obvious replacement directions

A replacement intended for high-precision CAD should be topologically sound by
construction. Possible directions:

1. **Exact/filtered arrangement boolean on triangle meshes.**
   - Detect all triangle-triangle intersections with robust predicates.
   - Split triangles along intersection segments and coincident overlaps.
   - Build a surface arrangement with explicit halfedges.
   - Classify arrangement faces by exact/topological point location.
   - Select faces according to the boolean operation.
   - Stitch and orient the selected faces, reporting non-manifold or ambiguous
     cases explicitly.

2. **Nef polyhedra / exact rational kernel.**
   Use a representation that stores exact halfspace/plane arrangements and is
   closed under boolean operations. This is slower and may require coordinate
   growth management, but is the most direct route to topological guarantees.

3. **Constrained tetrahedralization / volumetric cell complex.**
   Construct a tetrahedral complex conforming to both input surfaces and their
   intersections, classify tetrahedral cells, and extract the selected boundary.
   If generated with robust predicates and exact snapping policies, topology is
   represented explicitly rather than inferred by floating-point rays on a grid.

4. **If voxel output is desired, use an adaptive signed/topological grid.**
   An octree with conservative interval predicates, feature preservation, and
   dual contouring could produce better approximations than the current uniform
   cell-centre classifier, but it would still not be an exact b-rep boolean.

## Practical debugging implications for the current implementation

When debugging current failures, check these questions first:

- Did the input survive `convert_to_triangles()` and `remove_degenerate_faces()`
  as a closed index-manifold triangle mesh?
- Are disconnected vertices expanding the domain and reducing effective
  resolution near the actual surface?
- Is `boundary_scale` large enough that outside rays and voxel boundaries are
  not interacting with domain limits?
- Is `max_depth` high enough for the smallest feature, gap, or overlap that
  must be preserved?
- Are failures caused by the touch-cell barrier, the centre classifier, or
  ambiguous ray crossings?
- Does the downstream consumer require a strict 2-manifold, and can the emitted
  voxel boundary violate that requirement at corner/edge contacts?

