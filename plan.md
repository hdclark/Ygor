# Repair Plan For `src/YgorMeshesBSPTree.cc`

## Goal

Repair `src/YgorMeshesBSPTree.cc` so BSP booleans over valid, closed, consistently oriented, non-self-intersecting b-rep surface meshes are deterministic, topologically valid by construction, and robust to numerical issues. The repaired implementation must not rely on random plane order, fixed-direction ray casts, heuristic sample offsets, midpoint fallbacks, artificial bounding squares, or coordinate-quantized welding for correctness.

The safest path is a topology-first rewrite of the implementation internals while preserving the public `bsp_tree_volume<T, I>` API. The BSP tree should become the classification structure for a deterministically constructed exact cell arrangement, and mesh extraction should read boundary topology from that arrangement rather than reconstructing it heuristically from infinite planes.

## Step 1: Define The Supported Input Contract Explicitly

1. In `from_fv_surface_mesh`, keep accepting finite, closed, consistently oriented triangular or triangulatable meshes.
2. Add an internal documented contract that the exact-result guarantee applies when the input is a valid, topologically well-defined b-rep solid: no self-intersections, no non-manifold geometric overlaps, no zero-area faces after triangulation, no contradictory coincident shells, and a bounded solid under the chosen orientation convention.
3. Add explicit exceptions for inputs that violate this contract once detection is implemented. Do not silently return `Out`, empty meshes, or placeholder geometry for construction failures.
4. Treat `root == nullptr` and explicit `Out` as empty only at API boundaries. Internally canonicalize empty volumes to an explicit `Out` root to avoid inconsistent null handling.

## Step 2: Add A Deterministic Exact Geometry Kernel Layer

1. Introduce internal helper types in the anonymous namespace of `YgorMeshesBSPTree.cc`; do not expose them in the header unless tests need direct access.
2. Add `ExactScalar`, initially using an exact rational type available in the project if one exists; otherwise use a local wrapper around a multiprecision rational implementation.
3. Add `ExactPoint3`, `ExactPlane3`, `ExactLine3`, and `ExactPoint2` types.
4. Store original floating coordinates converted exactly from their binary representation, not rounded decimal strings.
5. Implement filtered predicates that first use the existing adaptive predicates when safe and fall back to exact arithmetic when the sign is uncertain or zero-sensitive.
6. Implement exact constructions for line-plane, segment-plane, plane-plane-line, and plane-plane-plane intersections.
7. Represent constructed vertices symbolically as well as geometrically. At minimum, store the sorted set of defining primitives: original vertex id, original edge id plus splitting plane id, or triple of plane ids.
8. Define equality and ordering of constructed vertices by symbolic identity first, exact coordinates second. Never key topology by rounded floating coordinates.

## Step 3: Canonicalize Mesh Input Before BSP Construction

1. Copy and triangulate the input mesh as the current implementation does, but then build an internal indexed triangle list with stable ids.
2. Remove duplicate vertices only when exact coordinates are identical. Preserve distinct vertices that are merely close.
3. Remove duplicate faces with identical vertex cycles only if they have the same orientation and are true duplicates; reject opposite-orientation duplicate faces as invalid overlapping boundary.
4. Verify all face indices are valid after canonicalization.
5. Verify every triangle has non-zero exact area by checking exact orientation against at least one non-collinear support direction or exact cross product.
6. Verify edge manifoldness by exact vertex ids: every undirected edge appears exactly twice.
7. Verify orientation consistency by paired directed edges.
8. Compute shell components and signed volume using exact arithmetic. If all shells are inward, reverse them deterministically. If mixed or ambiguous shell containment is detected, classify shells using exact point-in-polyhedron logic and reject contradictions.
9. Detect geometric self-intersections before building the BSP. Use broad-phase bounding boxes for speed, but confirm triangle-triangle intersections exactly. Reject intersections that are not shared topological vertices or edges.

## Step 4: Replace Random Plane Ordering With Deterministic Plane Selection

1. Remove use of `std::random_device`, `std::mt19937_64`, and `std::shuffle` from the builder.
2. Build a canonical list of unique oriented support planes from input triangles. Plane identity must be exact: two planes are the same if their normalized exact coefficients are proportional with the same offset.
3. For each plane, collect all input triangles lying on it and their orientation relative to the plane.
4. Define a stable total order for planes using normalized exact coefficients and the minimum original face id on the plane.
5. When choosing a BSP partition plane, use a deterministic score: minimize exact split count, then minimize balance difference, then prefer original face id, then plane coefficient order.
6. Do not impose a silent maximum depth that changes classification. If construction exceeds a defensive limit, throw an exception with the active cell, depth, plane count, and fragment count.

## Step 5: Build A Planar Surface Arrangement Per Support Plane

1. For every unique support plane, project all coplanar input triangles on that plane into an exact 2D coordinate system.
2. Add all triangle edges to a planar straight-line graph for the plane.
3. Intersect coplanar and crossing segments exactly in 2D and split edges at every exact intersection.
4. Create canonical 2D vertices by exact coordinate or symbolic identity; never by tolerance.
5. Build half-edges with twin, next, previous, face, and source primitive metadata.
6. Walk the half-edge graph to extract all bounded planar cells.
7. For each planar cell, determine whether it is covered by the original mesh surface and record the oriented material side: front-solid/back-empty or front-empty/back-solid.
8. Reject overlapping coplanar faces with contradictory material side unless they cancel exactly under a documented shell rule.
9. Triangulate planar cells only as an output detail; keep the planar cell polygon as the primary topology.

## Step 6: Construct A Global 3D Cell Arrangement

1. Insert all unique support planes into a deterministic BSP or arrangement builder.
2. Each BSP cell must track the set of half-space signs against all inserted planes that define it.
3. Split cells exactly when inserting a plane. Cell vertices are exact plane-plane-plane intersections or inherited original vertices.
4. Maintain adjacency between neighboring 3D cells across each planar facet.
5. Clip planar surface cells from Step 5 into the current 3D cells using exact predicates and exact intersections.
6. Attach each clipped surface patch to the two adjacent 3D cells it separates.
7. Do not classify 3D cells by ray casting. Classify by crossing oriented boundary patches: if a patch normal points from inside to outside, mark the adjacent cells consistently.
8. Seed the unbounded outside cell as `Out` by construction.
9. Flood-fill through cell adjacencies not blocked by material boundary patches to assign `Out`; cells across boundary patches become `In` or `Out` according to patch orientation.
10. If classification produces a contradiction, throw an invalid-input exception that identifies the conflicting patches or cells.

## Step 7: Rebuild `build_bsp_from_triangles` Around Classified Cells

1. Replace the current triangle-recursive builder with a builder that consumes the exact classified cell arrangement.
2. The builder should create a BSP node for the selected plane and partition the current set of classified cells into front and back subsets.
3. If all cells in a subset have the same classification, emit an `In` or `Out` leaf.
4. If a subset is mixed, recursively split it with the next deterministic plane that separates remaining mixed cells.
5. Store only valid non-degenerate planes in `Node::partition_plane`; convert exact plane anchors to `T` only after choosing stable representative points from exact arrangement vertices.
6. Ensure every `Partition` node has non-null `front` and `back` children.
7. Run a structural validator after construction: no degenerate planes, no null children, no redundant same-leaf partitions, and every arrangement cell reaches the expected leaf.

## Step 8: Make Boolean Merge Exact Over Classified BSP Trees

1. Keep the public `boolean_union`, `boolean_intersection`, `boolean_subtraction`, and `boolean_exclusion` methods.
2. Replace `partition_tree` coplanar detection based on unit-normal dot products and centroids with exact plane identity using normalized plane coefficients.
3. If public `Node` still stores only three anchors, create an internal exact plane cache during merge by converting anchors to exact plane coefficients and validating non-degeneracy.
4. Implement `restrict_tree_to_halfspace(node, plane, side)` as an exact operation over the tree's planes. It must preserve the represented classification function on that half-space.
5. For coplanar planes, combine aligned or anti-aligned children by exact side mapping. Never choose one non-null fragment when both fragments are meaningful; merge the fragments with union/intersection semantics appropriate to restriction.
6. Implement boolean leaf truth tables explicitly over `In` and `Out`; do not rely on null-as-out inside merge.
7. Collapse only when both children are explicitly equal leaves or when a recursive uniform evaluator proves equality.
8. After every boolean operation, validate the resulting tree against sampled exact arrangement cells from both operands if available.

## Step 9: Replace Mesh Extraction With Boundary Patch Extraction

1. Remove the artificial root-`In` cube output. A bounded mesh cannot represent all space. Throw for `In` root unless the API explicitly documents an unbounded-solid sentinel.
2. Remove `make_initial_polygon`, heuristic `bbox_size`, and large-square clipping from correctness paths.
3. Extract boundary patches from the classified arrangement: every facet between an `In` cell and an `Out` cell is output exactly once.
4. For each boundary facet, use the existing planar cell polygon from the arrangement, clipped to the actual adjacent 3D cells.
5. Orient each polygon so its normal points from `In` to `Out`.
6. Triangulate each exact polygon deterministically in its 2D plane using an exact constrained triangulation or deterministic ear clipping with exact orientation predicates.
7. Convert exact output coordinates to `T` only at the final mesh emission boundary.
8. Preserve topological identity by assigning one mesh vertex per exact arrangement vertex. Do not weld by coordinate quantization.
9. Rebuild `involved_faces` after mesh creation and run manifold/orientation verification. Treat verification failure as an internal logic error.

## Step 10: Remove Heuristic Numeric Fallbacks

1. Delete or quarantine `plane_threshold<T>()` so it is not used for topological decisions.
2. Replace `intersect_edge_with_plane` midpoint fallback with exact segment-plane intersection. If the segment is coplanar, route it through planar arrangement logic; if disjoint, report no intersection.
3. Delete `point_is_inside_mesh`, `count_ray_intersections`, and `ray_intersects_triangle` from construction correctness paths.
4. If any ray or sample classification remains for diagnostics, make it explicitly non-authoritative.
5. Replace `classify_point` exact-zero routing rules with caller-specific policies. A point on a partition plane should be handled as a lower-dimensional cell, not automatically routed front.

## Step 11: Strengthen Error Reporting And Determinism

1. Every failure that prevents exact construction must throw a deterministic exception with enough context to reproduce the issue.
2. Include input face ids, plane ids, cell ids, and predicate/construction type in exception messages where practical.
3. Ensure two identical inputs produce byte-for-byte identical output face index order and vertex order.
4. Sort output vertices by exact coordinate or first creation in deterministic arrangement traversal.
5. Sort output faces by support plane id, planar cell id, then triangulation-local id.

## Step 12: Add Internal Validators

1. Add `validate_bsp_tree` to verify structural invariants: non-null children for partition nodes, valid planes, canonical leaves, and no reducible same-leaf partition.
2. Add `validate_arrangement` to verify every half-edge has a twin, every face cycle is closed, every 3D facet has valid adjacent cells, and every cell classification is assigned.
3. Add `validate_output_mesh` to call existing verification helpers and also check exact expected edge counts before floating conversion when possible.
4. Call validators in debug builds and in tests. Keep release runtime validation for input contract failures and internal contradiction detection.

## Step 13: Update Integration In `YgorMeshesBoolean4.cc`

1. Remove dependence on an optional random seed for correctness. The seed parameter may remain for API compatibility but should be ignored or used only for deterministic tie-breaking when explicitly supplied.
2. Ensure `BooleanMeshOp4` receives deterministic BSP results without passing a seed.
3. Decide how to report invalid operands: propagate `std::invalid_argument` for invalid input and use `std::runtime_error` for internal construction failures.

## Step 14: Build A Comprehensive Test Suite

1. Add deterministic tests: run the same boolean operation many times and assert identical vertices, faces, and volumes.
2. Add exact primitive tests for point-plane classification, segment-plane intersection, plane identity, plane-plane-plane vertices, and symbolic vertex equality.
3. Add planar arrangement tests with overlapping coplanar triangles, shared edges, T-junctions, and duplicate faces.
4. Add mesh-to-BSP tests for tetrahedron, cube, nested shells, disjoint components, and inward-oriented inputs.
5. Add boolean tests with known exact results: disjoint intersection is empty, identical subtraction is empty, identical union is identical, containment subtraction creates a cavity, touching at a face/edge/vertex follows documented closed-set semantics, and coplanar overlapping faces do not create duplicate sheets.
6. Add robustness tests at very large and very small coordinate scales.
7. Add invalid-input tests for self-intersections, non-manifold edges, inconsistent orientation, duplicate opposite faces, zero-area faces, and non-finite vertices.
8. For every non-empty output, assert finite vertices, valid face indices, closed manifoldness, consistent orientation, expected signed volume, and no duplicate contradictory faces.

## Step 15: Stage The Implementation Safely

1. First land validators and deterministic ordering while keeping behavior otherwise unchanged.
2. Next add exact kernel types and unit tests.
3. Then replace mesh preprocessing and invalid-input detection.
4. Then implement planar arrangements and test them independently.
5. Then implement global cell classification and BSP construction from cells.
6. Then replace boolean merge coplanar/restriction logic.
7. Then replace mesh extraction with boundary patch extraction.
8. Finally delete obsolete ray-cast, offset, randomization, large-square clipping, and quantized-welding code.

## Step 16: Acceptance Criteria

1. No use of randomness in `from_fv_surface_mesh` or boolean operations.
2. No fixed epsilon controls a topological decision.
3. No midpoint fallback for geometric intersections.
4. No ray cast determines BSP leaf classification.
5. No artificial bounding square is used to discover output faces.
6. No rounded-coordinate welding is needed for topological correctness.
7. Empty, bounded, and invalid inputs have explicit deterministic behavior.
8. Boolean outputs are closed, consistently oriented, manifold meshes whenever the exact result is a bounded, topologically well-defined solid.
9. Tests prove deterministic output and exact semantics for representative valid cases.
10. Construction failures are reported with exceptions, never silently converted to `Out`, empty meshes, or placeholder geometry.
