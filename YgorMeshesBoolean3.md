# `src/YgorMeshesBoolean3.cc` implementation notes

This document describes the boolean engine exactly as implemented in `src/YgorMeshesBoolean3.cc` at the time this file was written. It is intentionally descriptive rather than prescriptive: where the implementation is fragile, the fragility is called out as part of the algorithm that actually exists.

## Public contract and data model

The public entry points are declared in `src/YgorMeshesBoolean3.h` and explicitly instantiated at the end of `src/YgorMeshesBoolean3.cc` for `float`/`double` meshes with `uint32_t`/`uint64_t` indices. The API exposes:

- `BooleanMeshOp3(lhs, rhs, op, options)` for `Union`, `Intersection`, `Exclusion`, and `Subtraction`.
- Convenience wrappers `BooleanUnion3`, `BooleanIntersection3`, `BooleanExclusion3`, and `BooleanSubtraction3`.
- `Boolean3Plane`, `SymbolicVertex`, and helper functions for plane construction, symbolic vertex evaluation, and symbolic vertex orientation.

The mesh type is `fv_surface_mesh<T, I>`, a facet-vertex surface representation: `mesh.vertices` stores coordinates and `mesh.faces` stores per-face vectors of vertex indices. This implementation assumes all faces used by the boolean path are triangles, and it uses three-entry face vectors throughout after preparation.

`MeshBoolean3Options<T>` has two controls:

- `snap_eps`: if positive, this absolute distance is used for point deduplication, snapping, cleanup, and several approximate geometric tests; otherwise it is derived from the combined bounding box as `sqrt(machine_epsilon) * max_extent`, with a minimum scale of `1`.
- `fast_path_weld_eps`: a distance threshold that forces otherwise clean fast-path intersections near existing vertices onto the robust path. The default in the header is `1.0e-7`, an absolute coordinate-space tolerance rather than a scale-normalized tolerance.

## High-level pipeline

`boolean_mesh_op_impl` is the main algorithm. Its implemented flow is:

1. Compute bounding boxes and choose `eps`.
2. Prepare each input mesh: validate closure, copy, triangulate, remove degenerates, orient faces, build involved-face adjacency, build an R-tree over triangle bounding boxes, and compute a `Boolean3Plane` for each face.
3. If both prepared meshes look like axis-aligned boxes with exactly 12 triangles and 8 bounding-box corners, use a special exact box-grid path.
4. Otherwise build one per-face planar arrangement for every face of both meshes.
5. Use an R-tree broad phase to find candidate face pairs.
6. For each candidate pair, classify the pair using orientation signs and route it to discard, fast non-degenerate segment intersection, or robust degenerate/coplanar handling.
7. Insert intersection segments and coplanar-overlap boundary segments into the two per-face arrangements.
8. Split every original face arrangement into `split_triangle` pieces, using simple fan splitting for simple cases and `Constrained_Delaunay_Triangulation_2` otherwise.
9. Classify each split piece as inside, outside, or coplanar relative to the opposite mesh. Most pieces are classified by an adjacency flood fill seeded with ray-casting parity tests; explicitly coplanar pieces are classified earlier by overlap regions.
10. Select pieces according to the requested boolean operation. For subtraction, included pieces from `rhs` are reversed.
11. Append every selected piece as three fresh output vertices and one face.
12. Merge snap-equivalent output vertices, remove duplicate and degenerate faces, remove disconnected vertices, validate closedness, orient the output, and return it.

The pipeline is a hybrid floating-point/symbolic arrangement algorithm. It uses exact/adaptive orientation predicates for many topological side tests, but most coordinates inserted into arrangements are ordinary floating-point coordinates, sometimes computed with `long double` intermediates and then snapped. Consequently the implementation is not topologically exact by construction.

## Internal structures

### Relation and routing enums

`boolean_face_relation` stores the final classification of a split face piece relative to the other mesh:

- `Outside`
- `Inside`
- `CoplanarSame`
- `CoplanarOpposite`

`triangle_intersection_kind` distinguishes no intersection, a segment intersection, and a coplanar polygon overlap.

`face_pair_route` controls candidate-pair handling:

- `Discard`: no intersection work is done.
- `FastPath`: clean non-coplanar intersection with no vertices exactly on the opposite plane.
- `RobustPath`: coplanar, vertex-on-plane, edge-touching, vertex-touching, near-welded, or otherwise degenerate cases.

### Prepared mesh

`prepared_mesh<T, I>` stores:

- a copied and modified `fv_surface_mesh<T, I>`;
- one `Boolean3Plane<T>` per face;
- an R-tree over per-triangle bounding boxes;
- a mesh bounding box.

The prepared mesh is the version used for all later calculations. The original input is not mutated.

### Per-face arrangement

`face_arrangement<T>` stores the planar subdivision constraints for one original triangle:

- `tri`: the original three 3D vertices of the face;
- `points`: arrangement vertices, initially the three triangle corners;
- `segments`: all subdivision edges, initially the three triangle boundary edges;
- `constrained_segments`: intersection/contact edges that should flip inside/outside parity during classification;
- `overlaps`: coplanar overlap polygons, each with the other face normal.

### Split triangle

`split_triangle<T>` is an emitted piece of an original face arrangement. It stores:

- three 3D vertices;
- three booleans telling whether each edge is a constrained intersection edge;
- the source face index;
- the relation to the opposite mesh;
- whether that relation was explicitly set by coplanar-overlap handling rather than flood fill.

## Step-by-step implementation details

### Mesh validation and preparation

`validate_closed_triangular_mesh` first logs checkpoint statistics, then accepts an empty-face mesh immediately. For non-empty meshes it enforces:

- `(3 * number_of_faces)` is even;
- every vertex is finite;
- every face has exactly three indices;
- no face has repeated indices;
- every face index is in range;
- every undirected edge appears exactly twice.

The edge test is based on undirected edge counts. It detects open boundaries and non-manifold edge counts, but it does not prove the mesh is orientable, connected, consistently oriented, self-intersection-free, or free of non-manifold vertices. The `3F = 2E` handshake test is redundant with the later edge-count check for a triangular closed surface, but catches impossible parity early.

`prepare_mesh` calls this validator before it calls `convert_to_triangles()` and `remove_degenerate_faces()`. Because validation happens first, the boolean engine rejects non-triangular input even though the next line would triangulate it. After copying the mesh, preparation:

1. Calls `convert_to_triangles()`.
2. Calls `remove_degenerate_faces()`.
3. Calls `OrientFaces(out.mesh, eps)` and throws if it fails.
4. Calls `recreate_involved_face_index()`.
5. Builds `bounds` from all prepared vertices.
6. Inserts each face's axis-aligned triangle bounding box into a new `rtree<T>` with the face index in `aux_data`.
7. Constructs a `Boolean3Plane` for each face from the face's prepared vertices.

Important impedance mismatch: the input is validated before preparation mutates it. If `remove_degenerate_faces()` removes faces after validation, the prepared mesh is not revalidated before the algorithm proceeds. If `OrientFaces` changes winding, face planes are built after orientation and therefore match the prepared winding.

### Plane and symbolic-vertex support

`make_boolean3_plane_impl(a, b, c)` computes the unnormalized triangle normal `(b - a) x (c - a)`. It throws on zero squared length. It stores:

- `normal`: the unnormalized normal;
- `offset`: `normal dot a`;
- `geometric`: a normalized `plane<T>` made from `normal.unit()` and `a`.

Most boolean code uses the unnormalized `normal` and `offset`, not the normalized `plane`.

`make_edge_support_plane(a, b, face_normal)` constructs a plane through an owner triangle edge and perpendicular to the owner face by using `(b - a) x face_normal` as the support-plane normal. This plane is used as the third support plane for symbolic edge/plane intersection vertices. It throws on a degenerate edge or normal combination.

`evaluate_symbolic_vertex_impl` intersects three support planes using Cramer's rule. It returns `nullopt` if the determinant has zero floating-point sign. Otherwise it caches and returns the floating-point intersection point. This is exact in intent but not exact in coordinate output: normals and offsets are `T`, the determinant sign is tested with ordinary `T`, and the emitted coordinate is `T`.

`orient_symbolic_vertex_against_plane_impl` evaluates the sign of a symbolic vertex against a fourth plane using expansion arithmetic. It builds scalar-triple-product expansions and combines them into the determinant expression, then returns the sign of the largest compressed expansion component. This is the most exact-style part of the file, but the main boolean pipeline does not actually call this function for classification of arrangement pieces; it is only exposed via the public helper wrapper.

### Bounding boxes, tolerances, and quantization

`triangle_bbox` and `mesh_bbox` build axis-aligned boxes from floating-point coordinates. The R-tree candidate search is therefore bounding-box based and depends on exact coordinate extrema; no epsilon expansion is applied to triangle boxes.

`mesh_coord_eps(bounds)` chooses `sqrt(epsilon) * max(max_extent, 1)`. This tolerance is used for many unrelated purposes: snapping, deduplication, degenerate segment rejection, coplanar overlap deduplication, segment refinement, vertex merging, and validation cleanup.

`quantize_point_key(p, eps)` rounds each coordinate to an integer grid with spacing `max(eps, machine_epsilon)`. `make_quantized_edge_key` sorts two quantized endpoints to make an undirected key. Flood-fill adjacency is based on these quantized edge keys, not on exact mesh indices.

This is a major robustness compromise. It can connect edges that are geometrically close but topologically distinct, and it can fail to connect edges that should match but round differently.

### Point-in-mesh classification

`point_inside_mesh(p, prep, far_distance)` classifies a point against a closed mesh as follows:

1. Empty meshes are outside.
2. If `point_on_mesh_boundary` finds any candidate face whose triangle contains `p`, the point is treated as inside.
3. Otherwise three fixed oblique ray directions are tried.
4. Each ray is a segment from `p` to `q = p + unit(dir) * far_distance`.
5. `cast_parity_ray` counts strict interior intersections with candidate triangles found by searching the R-tree over `index_bbox<T>(p, q)`.
6. The point is inside if at least two of the three rays have odd crossing count.

`segment_intersects_triangle_interior` rejects all cases where either ray endpoint is coplanar with a triangle plane or where any edge-orientation test is zero. This deliberately avoids ambiguous hits on triangle edges or vertices, but it also means a ray aligned with degeneracies may undercount. The three-direction majority vote partially mitigates this.

Potential issue: the R-tree search uses the bounding box of the segment endpoints. If `index_bbox<T>(p, q)` only stores min/max componentwise, this is a broad enough segment AABB. If it assumes constructor arguments are ordered, rays in negative directions would be problematic, but all three chosen ray directions are positive in all components.

### Candidate pair collection and face-pair classification

`collect_candidate_face_pairs` iterates every face of `prep_a`, searches `prep_b.face_index` with the exact triangle bounding box of the `a` face, and returns `(face_a_idx, face_b_idx)` for every box hit. There is no epsilon-inflation. Near intersections whose triangle boxes miss due to rounding are not considered.

Before exact route classification, `build_face_arrangements_hybrid` applies `passes_structural_proximity_check`, which compares centroid distance to the sum of centroid-based bounding radii plus `eps`. This should be redundant for true bounding-box candidates, but it is another approximate filter.

`classify_face_pair` evaluates all three vertices of triangle A against B's plane and all three vertices of triangle B against A's plane using `orient_sign(vec3, vec3, vec3, vec3)`. It then routes:

- If all vertices of either triangle are strictly on one side of the other triangle's plane, discard.
- If any tested sign is zero, route robust.
- Otherwise route fast only if each triangle has vertices on both sides of the other's plane.
- Any remaining case is discarded.

This classification is necessary but not sufficient for triangle intersection. The later fast path computes segment overlap along the intersection line; if none exists, no split is inserted.

### Fast path: clean non-degenerate triangle-triangle segment intersection

`compute_fast_path_intersection` handles non-coplanar triangle pairs with no vertex exactly on the opposite plane.

It computes normals and the intersection-line direction `n_a x n_b`. If the line direction is too small relative to `snap_eps`, it marks `welded_to_existing_vertex = true`, which forces robust handling by the caller.

The local `plane_cut` helper clips one triangle by the other triangle's plane:

- vertices with zero sign are added, though the fast path normally has no zeros;
- each edge whose endpoint signs differ is intersected with the other plane using `line_plane_intersection_with_t`;
- if the intersection point is within `weld_eps` of either edge endpoint, the whole pair is marked welded and routed to the robust path;
- otherwise the point is deduplicated with `snap_eps`.

After obtaining two plane-cut segments, the function projects both onto the common line direction and intersects their 1D intervals. If the interval overlap length is less than or equal to `snap_eps`, no intersection is returned. Otherwise the overlap endpoints are reconstructed in 3D, snapped to nearby vertices or edges using `snap_intersection_point`, checked again against `weld_eps` for vertex proximity, and returned as a `Segment` if distinct.

Numerical character: the signs come from `orient_sign`, but the segment endpoints are floating-point line-plane intersections. `line_plane_intersection_with_t` uses `long double` intermediates, but returns `T` and treats only exactly zero denominator as failure. There is no interval arithmetic or exact construction.

### Robust path: degenerate and coplanar pair handling

`compute_robust_path_intersection` is used for any candidate with a zero orientation sign and for fast-path near-weld fallback.

For exactly coplanar face pairs, where all six vertex-plane signs are zero, it calls `compute_coplanar_intersection`.

For non-coplanar degeneracies it:

1. Calls `append_symbolic_or_sampled_points` for A clipped by B and B clipped by A.
2. Adds owner vertices that lie on the other triangle.
3. Sorts collected points along `normal_a x normal_b` if this direction is non-degenerate.
4. Deduplicates by `eps`.
5. If at least two points remain, returns the first and last as a `Segment`; otherwise returns no intersection.

`append_symbolic_or_sampled_points` adds vertices exactly on the opposite plane and constructs symbolic edge/plane intersection vertices for sign-changing owner edges. However, it immediately calls `evaluate_symbolic_vertex_impl` and then snaps the result into floating-point coordinates. The robust path therefore reconstructs contact geometry with symbolic support planes only transiently; the arrangement itself is still approximate.

This robust path does not perform a full local topological arrangement of a degenerate cluster despite the comment saying it handles localized clusters. It processes one face pair at a time and reduces any non-coplanar degenerate contact with more than two points to the extreme segment endpoints. That can lose topology in multi-face or vertex-contact clusters.

### Coplanar overlap handling

`compute_coplanar_intersection` projects both triangles to 2D by dropping the dominant normal axis of triangle A. It builds two 2D/3D point lists, forces both projected polygons to counter-clockwise order by signed area, and clips A by B with Sutherland-Hodgman clipping.

The clipping `inside` test uses `orient_sign(a, b, p) >= 0`. Intersections are computed by `interpolate_clip_point`, which solves a 2D line intersection using `long double`, clamps the interpolation parameter to `[0, 1]`, and linearly interpolates the original 3D points. The clipped points are snapped/deduplicated. If at least three distinct points remain, the result is a `CoplanarPolygon`.

Limitations:

- The algorithm assumes convex triangles, which is fine here.
- It returns no result for coplanar edge-only or vertex-only contact because fewer than three points are discarded.
- The overlap polygon is stored as an ordered list but later logic mostly uses it for boundary segment insertion and centroid-in-polygon tests.

`finalize_coplanar_overlap_segments` adds every overlap polygon vertex to the arrangement. For each overlap polygon edge, it computes the midpoint and counts how many overlap polygons contain that midpoint. It inserts the edge as a constrained segment only if the midpoint is contained in one or fewer overlap polygons. This attempts to keep only the boundary of the union of overlapping coplanar regions on that face.

This boundary extraction is approximate and local to one original face. It uses midpoint containment and cannot represent all possible overlapping-region arrangements robustly if multiple coplanar overlaps cross each other in complicated ways.

### Arrangement construction

`make_face_arrangement` initializes each face arrangement with the triangle corners and three boundary segments.

`build_face_arrangements_hybrid` creates arrangements for every prepared face of both meshes, collects candidate pairs, routes each pair, and inserts results:

- Segment intersections are inserted as constrained segments into both corresponding face arrangements.
- Coplanar polygons are pushed into each face's `overlaps`, with the other face normal recorded for later same/opposite classification.
- After all pairs are processed, `finalize_coplanar_overlap_segments` is run for every arrangement.

Only segment endpoints and coplanar-overlap polygon vertices become arrangement points. If two intersection segments cross inside a face but the crossing point was not generated by a triangle-triangle pair endpoint, the later `refine_arrangement_segments` can split segments at points that are near them, but it does not compute new segment-segment crossing vertices. Therefore the arrangement is not guaranteed to be a complete planar straight-line graph.

### Splitting a face arrangement

`split_face_with_arrangement(arr, source_face_idx)` projects all arrangement points to 2D by dropping the dominant axis of the original triangle normal. It refines all segments and constrained segments by inserting existing points that lie near each segment. It does not compute missing intersections between segments.

It constructs a list of edge constraints for CDT from refined segments. It also constructs a boundary cycle by finding all points near each of the three original triangle edges and concatenating the three sorted chains.

There are three splitting modes:

1. **No internal segments.** The boundary cycle is fan-triangulated from its first vertex.
2. **Exactly one internal segment whose endpoints are both on the boundary cycle.** The segment cuts the boundary cycle into two polygons, and both are fan-triangulated.
3. **General case.** It calls `Constrained_Delaunay_Triangulation_2(verts2, edges, false)` and filters the returned triangles to those with nonzero projected orientation and centroids inside the original projected triangle.

For every emitted triangle it sets `constrained_edge[i]` by checking whether that 2D edge is in the refined constrained-segment set. If `classify_coplanar_split_triangle` finds the piece centroid inside any stored coplanar overlap polygon, it sets `relation` to `CoplanarSame` or `CoplanarOpposite` depending on the dot product of the original face normal and stored other normal, and marks `relation_explicit = true`.

Important limitations:

- Fan triangulation assumes the polygon is simple and fan-visible from vertex 0. For a boundary cycle with collinear inserted points this often works, but it is not a general polygon triangulation.
- The single-internal-segment special case only handles endpoints on the boundary cycle.
- The CDT path depends on the constrained Delaunay implementation accepting all segments and preserving them. The boolean code does not verify that every constrained segment appears in the triangulation.
- Centroid filtering can keep or discard triangles incorrectly if invalid crossed constraints produce triangles whose centroids are inside the original triangle but outside the intended arrangement cell.

### Flood-fill classification of split pieces

`classify_split_pieces_via_flood_fill` classifies non-explicit pieces relative to the opposite prepared mesh.

It first builds an `edge_map` from quantized undirected piece edges to occurrences. Explicit coplanar pieces are skipped entirely and therefore do not participate in flood-fill adjacency.

For each quantized edge group:

1. Occurrences are grouped by `source_face_idx`. If a source face contributes exactly two occurrences, the corresponding pieces are adjacent. The adjacency has `invert = true` if either occurrence marks the edge as constrained.
2. Non-constrained occurrences from distinct source faces are also connected with `invert = false` when exactly two distinct source faces share the quantized edge. This attempts to connect pieces across original mesh edges.
3. Groups with more than two occurrences in relevant contexts are counted as ambiguous and only logged at debug level.

It then finds connected components ignoring inversion. For each component:

1. It prefers a seed piece with no constrained edges; otherwise it uses the first component piece.
2. It classifies the seed centroid with `point_inside_mesh` against the other mesh.
3. It propagates parity through adjacency. Crossing a constrained edge flips parity; crossing an unconstrained edge preserves parity.
4. If a conflict is found, or any piece remains unclassified, it falls back to an independent `point_inside_mesh` centroid test for each affected piece.

Finally it writes `Inside` or `Outside` into every non-explicit piece. Explicit coplanar pieces keep their `CoplanarSame`/`CoplanarOpposite` relation.

The conceptual purpose is sound for a correctly constructed arrangement: crossing a true intersection curve changes inside/outside state, while moving across ordinary subdivision edges does not. The implementation's reliability depends on every intersection curve being represented by constrained segments and every adjacency edge being correctly paired. Since arrangements are approximate and quantized, this is a common failure point.

### Boolean face selection

`include_face_from_a` and `include_face_from_b` implement output selection:

| Operation | Keep A pieces | Keep B pieces | B orientation |
| --- | --- | --- | --- |
| Union | `Outside`, `CoplanarSame` | `Outside` | unchanged |
| Intersection | `Inside`, `CoplanarSame` | `Inside` | unchanged |
| Subtraction | `Outside`, `CoplanarOpposite` | `Inside` | reversed |
| Exclusion | `Outside`, `Inside` | `Outside`, `Inside` | unchanged |

Coplanar ownership is asymmetric: coplanar same-facing pieces are kept from A for union and intersection, while coplanar opposite-facing pieces are kept from A for subtraction. Coplanar pieces from B are not kept by either same/opposite relation. This avoids duplicate coincident output faces in many cases, but it relies on A-side overlap classification being complete.

`append_triangle` appends three fresh vertices for every selected triangle. Shared topology is reconstructed later only by coordinate merging.

### Cleanup and validation

After assembly, the output is logged. If there are no faces, it returns immediately.

For non-empty output, cleanup is:

1. `merge_duplicate_vertices(eps)`.
2. `deduplicate_faces(out)`.
3. `remove_degenerate_faces()`.
4. `remove_disconnected_vertices()`.
5. `recreate_involved_face_index()`.
6. log checkpoint.
7. `validate_closed_triangular_mesh(out, "BooleanMeshOp3 output")`.
8. `OrientFaces(out, eps * 8)`, throwing if it fails.
9. `recreate_involved_face_index()` again.

`deduplicate_faces` only removes faces with the same cyclic ordering of vertex indices up to rotation. It does not remove reversed duplicates because `{i0, i1, i2}` and `{i0, i2, i1}` produce different keys unless vertex ordering happens to rotate into the same sequence. Since vertex merging happens before face deduplication, duplicate same-oriented faces may be removed; opposite-oriented duplicate faces may remain and then fail validation or orientation.

Validation occurs before the final orientation call. A topologically closed but inconsistently oriented mesh can pass validation, then be oriented. An invalid open/non-manifold mesh throws before orientation can attempt repair.

### Axis-aligned box fast path

Before the general path, `boolean_mesh_op_impl` tries `extract_axis_aligned_box` on both prepared meshes. A mesh is considered a box if:

- it has exactly 12 faces;
- its bounding box has positive extent in all axes greater than `eps`;
- every vertex lies within `eps` of either min or max on every axis;
- the set of observed corner classifications has size 8.

It does not verify that the faces actually form the six box sides, that triangle windings are correct, or that there are exactly 8 vertices. The earlier preparation validator and orientation step provide some protection.

`exact_axis_aligned_box_boolean` collects the two boxes' min/max coordinates in each dimension, builds the induced rectangular grid, marks each cell occupied according to the requested boolean set operation, and emits boundary quads between occupied and unoccupied cells. Each quad is split into two triangles. Vertices are deduplicated exactly by `std::map<vec3<T>, I>`, using exact coordinate values from the input min/max coordinate lists. It orients and validates non-empty output.

This fast path is topologically much more reliable for true axis-aligned boxes than the general path, because it constructs an output boundary from a volumetric cell occupancy grid.

## Failure handling

The implementation uses exceptions for hard failures:

- invalid inputs throw `std::invalid_argument` from validation or degenerate plane construction;
- failed input orientation throws `std::runtime_error`;
- failed output validation throws `std::invalid_argument`;
- failed output orientation throws `std::runtime_error`;
- failed axis-aligned box output validation/orientation throws.

The general hybrid path is wrapped in a `try/catch` that logs a warning and rethrows. There is no fallback boolean algorithm after hybrid failure.

Several serious inconsistencies only produce debug logs, not errors:

- ambiguous split-edge groups during flood-fill adjacency;
- flood-fill parity conflicts before centroid fallback;
- checkpoint statistics showing boundary or non-manifold edges before final validation.

For high-precision CAD, this means some topological defects may be silently handled by centroid fallback or only exposed later as a generic invalid-output exception.

## Robustness and numerical analysis

### What is robust

- Many sign decisions use `orient_sign`, which in Ygor is intended to be an adaptive predicate interface.
- Symbolic vertex orientation against a plane is implemented with expansion arithmetic.
- Closed triangular manifold validation catches obvious boundary and non-manifold edge-count defects.
- The output is validated for closed triangular manifold edge counts before return.
- The axis-aligned box fast path is grid-based and topologically simple.

### What is not robust

- The main arrangement stores floating-point coordinates, not exact constructions.
- Symbolic vertices are evaluated to floating-point coordinates before insertion into arrangements.
- `OrientSymbolicVertexAgainstPlane` is not used by the main classification pipeline.
- R-tree broad phase uses unexpanded floating-point AABBs.
- Snap tolerance is global and absolute after scale derivation; `fast_path_weld_eps` defaults to a hard-coded absolute value.
- Multiple topological concepts share one `eps`, even though snapping, degeneracy, adjacency, and classification have different tolerance needs.
- Arrangement completion does not compute all segment-segment intersections inside a face.
- CDT output is trusted without verification that constraints are represented exactly.
- Flood-fill adjacency is reconstructed from quantized coordinates rather than from an explicit topological arrangement graph.
- Point-in-mesh uses finite ray segments and strict triangle-interior crossings, then majority vote, rather than a fully symbolic winding-number or exact ray-stabbing method.
- Coplanar edge-only/vertex-only contacts are largely ignored as overlap regions.

## Fundamental soundness assessment

The implemented algorithm follows a recognizable surface-boolean strategy: split both input surfaces along all intersection curves, classify resulting surface patches, select patches according to the boolean operation, and weld the selected patches into an output surface. That strategy is fundamentally sound if the splitting arrangement and classification are topologically exact.

This implementation is not topologically sound by construction. It attempts to recover topological consistency from approximate geometric arrangements using snapping, quantized adjacency, centroid tests, and final validation. Those techniques can work for simple, well-separated cases, but they are not sufficient for high-precision CAD robustness. The most important structural weakness is that exact/adaptive predicates are used for some side decisions, while the constructed arrangement graph is still floating-point and incomplete. A single missed intersection vertex, mis-snapped endpoint, or wrong quantized edge pairing can invalidate the flood-fill parity model.

The final closed-manifold validation is valuable: it prevents many invalid results from being returned. However, it is a late failure detector, not a proof of correctness. A closed manifold can still represent the wrong boolean set, and many failure modes will surface only as generic exceptions rather than explanatory user-facing diagnostics.

## Obvious replacement directions

The strongest replacement would be an algorithm whose topology is constructed independently of floating-point coordinate accidents. Options include:

1. **Exact corefinement with rational or expansion-backed constructions.** Use exact predicates and exact intersection vertices, build an explicit halfedge/face arrangement, split all intersected faces, and only round coordinates at controlled output stages.
2. **Nef polyhedra or exact B-rep arrangements.** Libraries such as CGAL's Nef polyhedra or exact polygon mesh corefinement are designed to make boolean topology robust by construction.
3. **Volumetric/cell complex arrangement for CAD-critical paths.** Build a cell complex induced by all input planes/faces with exact signs, classify cells, and extract the boundary. This is more expensive but avoids patch-level parity ambiguity.
4. **Constrained remeshing with verified PSLGs.** If retaining the current architecture, first construct a complete planar straight-line graph per face: compute every segment-segment intersection, split constraints exactly, verify every constraint edge appears in CDT output, and carry explicit adjacency from this graph into flood-fill instead of reconstructing it by quantized coordinates.
5. **Exact winding-number classification.** Replace finite majority ray casting with robust winding/solid-angle classification or exact ray stabbing that symbolically handles vertex/edge hits.

A minimally invasive evolution of this file would keep the high-level split/classify/select pipeline but replace the per-face arrangement and adjacency layers with an explicit, verified topology graph. That is the area where the present implementation most directly loses the guarantees promised by its adaptive predicates.
