# `src/YgorMeshesBoolean2.cc` implementation guide

This document describes the Boolean engine exactly as implemented in `src/YgorMeshesBoolean2.cc` at the time of writing. It is intentionally descriptive rather than prescriptive: the implementation is **not** changed here. The goal is to make the current algorithm debuggable by exposing the geometry, topology, state transitions, error handling, and likely impedance mismatches with the surrounding Ygor mesh and predicate infrastructure.

## 1. Public contract and actual entry points

The public header `src/YgorMeshesBoolean2.h` advertises `BooleanMeshOp2(lhs, rhs, op, snap_eps)` plus wrappers for union, intersection, exclusion/symmetric difference, and subtraction. The implementation explicitly instantiates `float`/`double` with `uint32_t`/`uint64_t` vertex indices.

The actual call chain is:

1. `BooleanUnion2`, `BooleanIntersection2`, `BooleanExclusion2`, or `BooleanSubtraction2` forwards to `BooleanMeshOp2` with a `MeshBooleanOperation2` value.
2. `BooleanMeshOp2` calls the internal `boolean_mesh_op_impl`.
3. `boolean_mesh_op_impl` prepares both inputs, possibly dispatches a special exact axis-aligned-box path, otherwise builds per-face arrangements, splits faces, classifies pieces, emits selected triangles, cleans the output, validates closedness, or throws.

The implementation assumes the input meshes are closed triangular manifold surfaces after preparation, although `prepare_mesh` first validates the original mesh before converting any non-triangular facets.

## 2. Important local data structures

All helper types are in an anonymous namespace.

### `boolean_face_relation`

A split output triangle is classified relative to the opposite solid as one of:

- `Outside`: representative point is outside the other mesh.
- `Inside`: representative point is inside or on the other mesh.
- `CoplanarSame`: representative point lies in a recorded coplanar overlap region and the source face normal has non-negative dot product with the overlapping opposite face normal.
- `CoplanarOpposite`: same, but the normals have negative dot product.

This relation is later mapped to inclusion/exclusion rules for each Boolean operation.

### `triangle_intersection_kind` and `triangle_intersection_result`

Triangle-pair intersection returns:

- `None`: no usable intersection.
- `Segment`: a non-coplanar intersection segment with two endpoints.
- `CoplanarPolygon`: a polygonal overlap region with at least three points.

Point-only, edge-touch-only, and zero-length intersections are discarded by design.

### `prepared_mesh<T, I>`

Preparation stores:

- A copied and oriented `fv_surface_mesh<T, I>`.
- An R-tree of face bounding boxes. Each R-tree entry stores the face index in `std::any`.
- A mesh bounding box.

### `face_arrangement<T>`

Each original triangle gets a local planar arrangement:

- `tri`: the original triangle coordinates.
- `points`: local 3D points that lie on the face; initialized with the three triangle vertices.
- `segments`: undirected pairs of point indices; initialized with the three triangle boundary edges and augmented by intersection segments.
- `overlaps`: coplanar polygon regions contributed by opposite mesh faces; used both for coplanar classification and for adding overlap boundary segments.

### `split_triangle<T>`

A generated triangle plus its `boolean_face_relation`.

## 3. Mesh preparation and input validation

### Bounding boxes and tolerances

`mesh_bbox` computes an axis-aligned bounding box over every vertex. Empty meshes receive the default `index_bbox<T>()`, whose semantics depend on `YgorIndex`.

`mesh_coord_eps(bounds)` chooses an automatic coordinate tolerance as:

```cpp
sqrt(std::numeric_limits<T>::epsilon()) * max(extent.x, extent.y, extent.z, 1)
```

This is a scale-relative snapping tolerance, not an exact arithmetic tolerance. It is used for distinct purposes: snapping, duplicate detection, collinearity-near-segment tests, box recognition, and orientation eps values. This overload of roles is a major robustness risk.

### `validate_closed_triangular_mesh`

Before preparation proceeds, the input mesh is checked as follows:

1. Empty face lists are accepted.
2. `3F` must be even; otherwise the mesh cannot have every triangular edge paired.
3. Every vertex must be finite.
4. Every face must have exactly three indices.
5. Every face index must be in range.
6. No face may repeat a vertex index.
7. Undirected edge counts are accumulated.
8. The number of unique undirected edges must equal `3F/2`.
9. Every undirected edge must appear exactly twice.

Topological implications:

- This checks closed edge incidence but does **not** prove orientability, connectedness, geometric non-self-intersection, positive area, or absence of duplicate geometrically coincident vertices with different indices.
- It rejects non-triangular closed meshes before `convert_to_triangles` can triangulate them, despite the later call to `convert_to_triangles`.
- It permits two triangles with the same three vertex indices but opposite winding only if the edge-count test still passes; duplicate coincident facets generally break manifoldness or later duplicate-face handling.

### `prepare_mesh`

`prepare_mesh(mesh, name, eps)` performs:

1. `validate_closed_triangular_mesh(mesh, name)` on the original input.
2. Copy to `out.mesh`.
3. `convert_to_triangles()` even though validation already required triangles.
4. `remove_degenerate_faces()`; this removes facets with fewer than three vertices, not necessarily zero-area triangles.
5. `OrientFaces(out.mesh, eps)`; if it returns false, preparation throws `runtime_error`.
6. `recreate_involved_face_index()`.
7. Compute mesh bounds.
8. Build an R-tree with each triangle's exact axis-aligned bounding box.

Impedance notes:

- `OrientFaces` documents that it handles disconnected components and uses an `eps` duplicate-vertex tolerance. This Boolean code relies on it to make the input represent outward-oriented solids, but the subsequent inside/outside ray tests do not use normals; orientation matters mainly for final output winding and coplanar same/opposite classification.
- `remove_degenerate_faces()` does not necessarily remove zero-area triangular facets. Many later operations assume nonzero normals.

## 4. Geometric predicates and projection helpers

The implementation uses Ygor's `orient_sign` wrappers for 3D orientation and 2D orientation. These are described in `YgorMath.h` as adaptive arithmetic adaptors, so sign decisions for orientation are intended to be robust.

However, many constructed coordinates are computed in floating point or `long double` and then cast back to `T`:

- Line-plane intersections.
- 2D clipping interpolation.
- Segment projection and snapping.
- Centroid classification points.

Thus, predicate signs may be robust for the coordinates supplied, but the coordinates supplied may already be rounded or snapped inconsistently across adjacent faces.

`dominant_axis` chooses the largest absolute normal component and `project_drop_axis` projects 3D points to 2D by dropping that coordinate. This is used for point-in-triangle tests, coplanar clipping, and per-face retriangulation. Degenerate or near-degenerate normals can choose unstable projection axes.

`point_on_triangle` first requires exact 3D coplanarity via `orient_sign(a,b,c,p)==0`, then tests the projected point against the projected triangle boundary.

## 5. Inside/outside classification

### Boundary test

`point_on_mesh_boundary(p, prep)` queries the R-tree with the zero-volume bounding box `[p,p]`. For each candidate triangle, it calls `point_on_triangle`.

Potential issue: whether an R-tree point query finds triangles whose bounding boxes merely contain the point depends on `rtree::search` semantics. If search requires bbox overlap, this works. If it has strict interval behavior around degenerate boxes, boundary points can be missed.

### Ray parity test

`cast_parity_ray(p, q, prep)` searches faces whose bounding boxes overlap the segment bbox `[p,q]`, then counts triangles where `segment_intersects_triangle_interior(p,q,a,b,c)` is true.

`segment_intersects_triangle_interior`:

1. Rejects if either endpoint lies exactly on the triangle plane.
2. Rejects if both endpoints are on the same side.
3. Computes three orientation signs for tetrahedra `(p,q,a,b)`, `(p,q,b,c)`, `(p,q,c,a)`.
4. Rejects if any of these signs is zero.
5. Counts only strict interior crossings where all three signs have the same sign.

This deliberately avoids double-counting ray hits on triangle edges or vertices, but it also means a ray through an edge, vertex, or coplanar patch contributes no crossing. The caller compensates partially by majority voting three fixed skew directions.

### `point_inside_mesh`

1. Empty mesh is outside.
2. A boundary point is treated as inside.
3. Three hard-coded ray directions are used.
4. A far endpoint is `p + dir * far_distance`.
5. Inside if at least two of three rays have odd crossing parity.

Robustness concerns:

- `far_distance` is based on the combined input bounding-box extent, but rays begin from arbitrary split-triangle centroids. It should usually exit the mesh, but there is no explicit proof that the segment endpoint is outside every disconnected component in the ray direction.
- Fixed ray directions can still be degenerate for adversarial CAD geometry.
- Boundary is treated as inside, except coplanar overlaps are classified before ray tests.

## 6. Triangle-triangle intersection

### Broad-phase candidate selection

For each face of A, `build_face_arrangements` queries B's R-tree using A's triangle bounding box. It then performs an additional centroid/radius proximity check:

```cpp
|centroidA - centroidB| <= radiusA + radiusB + eps
```

This check is mathematically redundant if bbox overlap is correct, but should not reject true intersections for exact bounding spheres around centroids. Because radii and centroids are rounded, it may reject near-tangent cases if `eps` is too small.

### Non-coplanar intersection

`intersect_triangles` first classifies all vertices of A against B's plane and all vertices of B against A's plane using `orient_sign`. If all vertices of either triangle are strictly on one side of the other plane, it returns no intersection. If both sign arrays are all zero, it dispatches to coplanar intersection; otherwise to non-coplanar intersection.

`compute_noncoplanar_intersection`:

1. Computes raw normals and `line_dir = n_a x n_b`.
2. If `|line_dir|^2 <= eps^2`, returns none. This is a tolerance-based near-parallel rejection even though the earlier coplanar test is exact-sign based.
3. Computes signs of A vertices to B plane and B vertices to A plane.
4. `plane_cut` collects triangle vertices on the opposite plane and interpolated edge-plane crossings for sign-changing edges.
5. If either triangle contributes fewer than two cut points, returns none.
6. Projects the two segment endpoints from each triangle onto the intersection-line direction.
7. Intersects the 1D intervals.
8. If overlap length `hi-lo <= eps`, returns none.
9. Reconstructs two 3D endpoints, snaps them to nearby vertices/edges of both source triangles, and rejects if they collapse.
10. Returns a `Segment`.

Important limitation: point intersections and edge-touch intersections are discarded. For a full arrangement this is acceptable only if all topological events are still represented through adjacent face-pair segment intersections. In CAD-like degeneracies this assumption is unsafe.

### Coplanar intersection

`compute_coplanar_intersection`:

1. Uses A's normal to choose a projection axis.
2. Projects both triangles to 2D.
3. Ensures both projected polygons are counter-clockwise by signed area.
4. Clips A's triangle polygon by B's triangle polygon using Sutherland-Hodgman.
5. Deduplicates output points by 3D `eps` distance.
6. If at least three unique points remain, returns `CoplanarPolygon`; otherwise returns none.

Limitations:

- Line or point overlaps are ignored.
- Sutherland-Hodgman assumes convex clip polygons, which triangles satisfy.
- The 2D `inside` test uses `orient_sign(a,b,p) >= 0` with no tolerance. Intersections are interpolated in floating point, so points that should lie exactly on clip lines may classify inconsistently.
- The returned polygon is the overlap boundary, but it is not guaranteed to include all pre-existing arrangement vertices from either triangle that lie inside the overlap unless clipping emits them.

## 7. Face arrangement construction

Each face arrangement starts with only the source triangle boundary. For every intersecting pair:

- Non-coplanar `Segment`: the same 3D segment is added to both source face arrangements.
- Coplanar `CoplanarPolygon`: no segments are added immediately. Instead, each face stores an overlap region with the overlap polygon and the other face's normal.

After all pairwise intersections, `finalize_coplanar_overlap_segments` runs independently on every face arrangement:

1. Adds every polygon vertex from every overlap region as an arrangement point.
2. For every edge of every overlap polygon, takes the midpoint.
3. Counts how many overlap polygons contain that midpoint after projection.
4. Adds the polygon edge as an arrangement segment only if the midpoint is contained in at most one overlap polygon.

This appears intended to keep only the boundary of the union of coplanar overlap regions on a face, suppressing internal edges where two opposite faces produce adjacent/overlapping coplanar regions.

Concern: midpoint containment is not a complete polygon-union arrangement algorithm. It can fail for overlapping coplanar polygons whose boundaries cross, nested overlaps, duplicate nearly coincident boundaries, or multiple components. It also does not split crossing coplanar-overlap boundary edges against one another before the midpoint count.

## 8. Splitting a face arrangement into triangles

`split_face_with_arrangement` converts one face's arrangement into split triangles.

### Projection

All arrangement points are projected into the face's 2D coordinate system by dropping the dominant normal axis. All subsequent planar topology is done in this projection.

### Segment refinement

For every arrangement segment, the code finds every arrangement point that lies near that 3D segment using `point_near_segment` and a segment-local tolerance:

```cpp
max(1, segment_length) * sqrt(epsilon) * 8
```

It sorts those points by parametric coordinate along the segment and inserts refined subsegments between consecutive points. This is the implementation's main mechanism for splitting intersection segments at vertices that were discovered by other pairwise intersections.

Risks:

- Nearness is Euclidean 3D distance, not exact collinearity in the face plane.
- The refinement tolerance differs from `eps` and is hard-coded from machine epsilon.
- It only splits where an existing arrangement point is near an existing segment. It does not compute intersections between two arrangement segments that cross without an existing point at the crossing.

### Boundary cycle construction

The original triangle boundary is reconstructed by finding all points near original edges `(0,1)`, `(1,2)`, `(2,0)` and sorting each chain along that edge. The chains are concatenated, omitting the duplicated last endpoint of each chain, to form `boundary_cycle`.

This assumes arrangement point indices `0`, `1`, and `2` are still the original triangle vertices, which is true because `make_face_arrangement` initializes points from the triangle vertices and later additions append or reuse.

### Fast simple cases

The code separates refined segments into boundary and internal segments.

- If there are no internal segments, it treats the whole boundary cycle as one polygon and fan-triangulates from vertex 0.
- If there is exactly one internal segment and both endpoints appear on the boundary cycle, it splits the boundary cycle into two polygons along that chord and fan-triangulates both.

The fan triangulator rejects degenerate projected triangles and triangles whose projected centroid is outside the original projected triangle. Each emitted triangle is classified by `classify_split_triangle`.

Concern: fan triangulation is only valid for convex or star-shaped polygons with respect to vertex 0. Boundary cycles can include collinear subdivision points and, with coplanar overlap boundaries, potentially non-convex polygons.

### General constrained triangulation

If there are multiple internal segments, the code calls:

```cpp
Constrained_Delaunay_Triangulation_2<T, size_t>(verts2, edges, false)
```

where `edges` contains all refined segments, including boundary and internal constraints. The returned triangulation is filtered by:

1. Face has exactly three indices.
2. Indices are distinct.
3. Projected orientation is nonzero.
4. Projected centroid lies inside/on the original triangle.

Then each triangle is lifted back to 3D by original arrangement point coordinates and classified.

Impedance mismatch with CDT:

- The CDT function warns on exactly duplicate 2D vertices but does not merge epsilon-near duplicates. This Boolean code deduplicates 3D points by `eps`, but projection can collapse distinct 3D points for degenerate faces or create exact duplicates after dropping an axis.
- `retain_only_constraint_faces=false` means the CDT may include triangles outside constraint loops; the Boolean code relies on centroid-in-original-triangle filtering rather than extracting constrained regions. Internal coplanar-overlap boundaries therefore do not define holes or nested regions by themselves.
- The CDT receives no explicit tolerance matching the Boolean's snapping tolerance.

If CDT throws, the Boolean code throws a `runtime_error` including all projected points and segments.

## 9. Classification of split triangles

`classify_split_triangle(arr, tri, other_prep, far_distance)`:

1. Uses the original source face normal and projection axis.
2. Computes the 3D centroid of the split triangle and projects it.
3. Checks every coplanar overlap polygon on the source face. If the centroid lies inside/on an overlap polygon, returns `CoplanarSame` or `CoplanarOpposite` according to the sign of `normal dot other_normal`.
4. Otherwise calls `point_inside_mesh(centroid, other_prep, far_distance)` and returns `Inside` or `Outside`.

This is a representative-point classifier. It is sound only if the face arrangement has fully split the source face along every intersection and coplanar-overlap boundary so that each split triangle has constant relation to the other solid. Any missing segment split can produce a triangle whose centroid has one classification while part of the triangle belongs to another region.

## 10. Boolean selection rules

For pieces originating from A:

| Operation | Keep A piece when relation is |
| --- | --- |
| Union | `Outside` or `CoplanarSame` |
| Intersection | `Inside` or `CoplanarSame` |
| Subtraction | `Outside` or `CoplanarOpposite` |
| Exclusion | `Outside` or `Inside` |

For pieces originating from B:

| Operation | Keep B piece when relation is |
| --- | --- |
| Union | `Outside` |
| Intersection | `Inside` |
| Subtraction | `Inside` |
| Exclusion | `Outside` or `Inside` |

Notable behavior:

- For subtraction, kept B pieces are not explicitly reversed when appended. The code later calls `OrientFaces(out, eps*8)` to make the final mesh consistent. This can work if the emitted boundary is topologically closed, but it defers a semantic orientation requirement to a heuristic global orientation pass.
- For coplanar same surfaces, only A contributes to union/intersection, suppressing duplicate coincident faces from B.
- For coplanar opposite surfaces, A contributes to subtraction.
- Symmetric difference keeps all non-coplanar inside and outside pieces and drops coplanar pieces. This is plausible for coincident surfaces but sensitive to whether coplanarity was detected exactly.

## 11. Output assembly and cleanup

`append_triangle` appends three new vertices for every kept triangle and creates one face. No vertex sharing is preserved at emission time.

After all A and B pieces are processed:

1. Empty output is returned immediately.
2. `merge_duplicate_vertices(eps * 32)` merges geometrically nearby vertices.
3. `deduplicate_faces` removes duplicate faces by cyclically rotating the smallest index to the front; it is orientation-sensitive and does not remove reversed duplicate faces.
4. `remove_degenerate_faces()` removes sub-triangle facets.
5. `remove_disconnected_vertices()` compacts vertices.
6. `recreate_involved_face_index()`.
7. `validate_closed_triangular_mesh(out, "BooleanMeshOp2 output")`; throws if not closed triangular manifold by edge counts.
8. `OrientFaces(out, eps * 8)`; throws if inconsistent.
9. Recreates involved-face index again and returns.

The output validation is a valuable fail-fast guard. However, it can only report generic closed-manifold errors, not the geometric event that caused the invalid topology.

## 12. Axis-aligned box special path

Before the general algorithm runs, both prepared meshes are tested by `extract_axis_aligned_box`.

A mesh is recognized as an axis-aligned box if:

1. It has exactly 12 triangular faces.
2. Its bounding-box extents are all larger than `eps`.
3. Every vertex lies within `eps` of one of the eight bbox corners.
4. All eight corners are represented.
5. Every triangle lies on one of the six bbox planes.

If both inputs pass, `exact_axis_aligned_box_boolean` is used.

That path:

1. Forms sorted unique coordinate cuts from the min/max coordinates of both boxes along x/y/z.
2. Builds a small rectilinear cell grid.
3. Classifies each cell center as inside lhs/rhs boxes.
4. Marks occupied cells according to the Boolean operation.
5. Emits exposed cell faces as quads split into two triangles, sharing vertices through `std::map<vec3<T>, I>` exact coordinate keys.
6. Orients faces if nonempty and returns.

This is topologically more reliable for exact axis-aligned boxes because it constructs a cell complex and emits boundary faces between occupied and unoccupied cells. It is still limited to boxes and uses exact coordinate equality for unique cuts, not `eps` clustering.

## 13. Error handling

Input errors and output topology failures are thrown as exceptions. The outer general path catches `std::exception`, logs:

```text
BooleanMeshOp2 falling back to legacy BooleanMeshOp: <message>
```

and then immediately rethrows. There is no actual fallback in this implementation.

Therefore callers should expect exceptions for:

- Non-triangular, non-closed, non-manifold, or non-finite inputs.
- Failure to orient either input.
- CDT failure during face splitting.
- Non-closed output.
- Failure to orient the output.

The message can be detailed for CDT failures but generic for most topology failures.

## 14. Fundamental soundness assessment

The implemented algorithm has the outline of a classical surface Boolean:

1. Validate and orient closed input surfaces.
2. Find triangle-triangle intersections.
3. Insert intersection curves into per-face arrangements.
4. Retriangulate faces along those curves.
5. Classify split pieces.
6. Select pieces according to Boolean semantics.
7. Merge and orient the resulting boundary.

That high-level plan is standard. The current implementation is **not topologically sound by construction**, because the arrangement-building and retriangulation stages are local, tolerance-based, and incomplete for several classes of events:

- Segment-segment crossings inside a face are not computed unless a crossing point already exists from a triangle-triangle intersection.
- Coplanar overlap boundaries are approximated by midpoint containment rather than a full planar polygon arrangement/union.
- Point and edge-only triangle intersections are discarded, which can remove necessary topological vertices in coincident/tangent cases.
- Adjacent source faces may compute and snap geometrically identical intersection vertices differently, then rely on final `merge_duplicate_vertices` to repair global connectivity.
- The CDT and Boolean tolerances are not unified.
- Classification uses centroids and parity rays; it is not exact volumetric cell classification.

For high-precision CAD work, this means the implementation may succeed on many generic-position meshes, especially non-coplanar transversal intersections, but can fail on common CAD degeneracies: coincident faces, shared edges, vertex-on-face contacts, near-coplanar cuts, sliver triangles, nested disconnected shells, and repeated or nearly coincident vertices.

## 15. Obvious replacement directions

The most robust replacement would be an algorithm that constructs a topological cell complex first and uses exact predicates/controlled constructions throughout. Options include:

1. **Exact/filtered Nef polyhedra or arrangement-based core**: build the full arrangement of all input facets in exact arithmetic, classify 3D cells, and extract the requested boundary. This is topologically sound but requires substantial infrastructure.
2. **Plane-based BSP/corefinement with exact predicates**: split polygons by planes using exact rational or symbolic coordinates, maintain adjacency explicitly, classify cells, and emit manifold boundaries.
3. **Triangle mesh corefinement with exact predicates and constrained exact constructions**: globally corefine both meshes so intersection curves are shared by both operands, then classify connected face patches. This is closer to the current code, but the corefinement must be global and adjacency-aware, not independent per face.
4. **Voxel/cell-complex fallback for axis-aligned or rectilinear CAD primitives**: the existing box path is an example of a topologically constructed cell-complex method for a narrow case.

A pragmatic evolution of the current code would start by replacing per-face ad hoc arrangements with a real planar arrangement per source face that computes all segment intersections, handles coplanar polygon overlay exactly, and produces a constrained triangulation whose constraints and vertices are shared consistently across adjacent faces.

## 16. Quick map of helper functions by responsibility

- Validation/prep: `validate_closed_triangular_mesh`, `prepare_mesh`, `mesh_bbox`, `triangle_bbox`, `mesh_coord_eps`.
- Predicates/projection: `dominant_axis`, `project_drop_axis`, `triangle_normal`, `triangle_centroid`, `point_on_triangle`.
- Ray classification: `segment_intersects_triangle_interior`, `point_on_mesh_boundary`, `cast_parity_ray`, `point_inside_mesh`.
- Intersection construction: `line_plane_intersection`, `snap_intersection_point`, `compute_noncoplanar_intersection`, `compute_coplanar_intersection`, `intersect_triangles`.
- Arrangement construction: `make_face_arrangement`, `arrangement_add_point`, `arrangement_add_segment`, `build_face_arrangements`, `finalize_coplanar_overlap_segments`.
- Face splitting: `split_face_with_arrangement`.
- Boolean semantics: `classify_split_triangle`, `include_face_from_a`, `include_face_from_b`.
- Output assembly: `append_triangle`, `deduplicate_faces`.
- Special case: `extract_axis_aligned_box`, `exact_axis_aligned_box_boolean`.
