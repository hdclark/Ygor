# Plan to Repair `src/YgorMeshesBoolean4.cc`

## Goal

Repair the Boolean mesh implementation so it is deterministic, topologically valid by construction, and robust to numerical edge cases for valid, topologically well-defined closed input solids. The current `BooleanMeshOp4` wrapper delegates almost all behavior to `bsp_tree_volume`, whose current construction, merge, and extraction paths mix robust predicates with inexact constructions and approximate repair. The repair should therefore replace the effective algorithm used by `YgorMeshesBoolean4.cc`, not merely patch the wrapper.

The target implementation should use this invariant: build an explicit arrangement of the two input surface meshes, classify arrangement facets exactly, and emit only selected arrangement facets. Output topology should be assembled from the arrangement connectivity rather than reconstructed later by approximate BSP polygon clipping or welding.

## Step 1: Add a Failing Regression Baseline

1. Add Boolean4 tests before changing behavior.
2. Cover the public functions declared in `src/YgorMeshesBoolean4.h`: `BooleanMeshOp4`, `BooleanUnion4`, `BooleanIntersection4`, `BooleanExclusion4`, and `BooleanSubtraction4`.
3. Add a test for the existing algebraic bug: `BooleanSubtraction4(non_empty_mesh, empty_mesh)` must return a normalized copy of the left solid, not an empty mesh.
4. Add empty-operand tests for all operations:
   - `empty union B = B`
   - `A union empty = A`
   - `empty intersection B = empty`
   - `A intersection empty = empty`
   - `empty subtraction B = empty`
   - `A subtraction empty = A`
   - `empty exclusion B = B`
   - `A exclusion empty = A`
5. Add deterministic-output tests that run the same operation many times and compare normalized vertex and face output byte-for-byte.
6. Add topological postcondition tests that run `HasOnlyFiniteVertices`, `IsTriangularMesh`, `HasValidFaceIndices`, `HasNoDegenerateFaces`, `IsClosedManifold`, and `HasConsistentOrientation` on every non-empty result.
7. Add operation cases using pairs of closed triangular meshes:
   - disjoint cubes,
   - overlapping cubes,
   - one cube fully inside another,
   - identical cubes,
   - cubes touching at one face,
   - cubes touching at one edge,
   - cubes touching at one vertex,
   - a tetrahedron intersecting a cube with non-axis-aligned triangles.
8. Add tests for coplanar overlap and coincident faces because these are the main places where the current BSP path is ambiguous.
9. For every test, compare volume and surface area against expected values where those values are simple. Use exact combinatorial expectations where geometry is too awkward for scalar formulas.

## Step 2: Fix the Public Wrapper's Immediate Contract Bugs

1. In `BooleanMeshOp4`, replace the right-empty subtraction shortcut so `A - empty` returns the normalized left result.
2. Do not return `lhs` or `rhs` unchanged from any non-error shortcut unless the function contract explicitly allows preserving unnormalized input state. Instead, route identity results through the same normalization and validation path used by normal results.
3. Add a local operation validator that rejects invalid `MeshBooleanOperation4` values with `std::invalid_argument` rather than silently returning an empty mesh for an invalid enum cast.
4. Wrap input conversion failures with operand context: errors from the left operand should identify `lhs`, and errors from the right operand should identify `rhs`.
5. Keep this step minimal. It should make current behavior less wrong while the full replacement is implemented.

## Step 3: Define the Boolean4 Internal Data Model

1. Implement the new Boolean engine in `src/YgorMeshesBoolean4.cc` or in private helpers included only by that translation unit. Do not expose a new public API unless later steps prove it is necessary.
2. Define an internal exact coordinate type for constructed intersection points. Use one of these approaches:
   - a rational type over integer or arbitrary-precision numerator/denominator values if project dependencies allow it,
   - an expansion-backed exact construction type matching the adaptive predicate infrastructure,
   - a deterministic snap-rounding coordinate type with a documented topological guarantee.
3. Do not use floating-point interpolation results as identity keys for topology. Floating coordinates may be stored for final output, but topological identity must come from exact construction records.
4. Define internal records for:
   - input vertex references, including source mesh and vertex index,
   - exact constructed vertices from triangle-triangle intersections,
   - directed halfedges,
   - split face fragments,
   - shell or connected-component identifiers,
   - per-fragment source side, original face id, and orientation.
5. Make all containers deterministic. Prefer sorted vectors and stable ids over `std::map` keys based on rounded floating-point coordinates.

## Step 4: Normalize and Validate Inputs Explicitly

1. Copy each input mesh into a local working mesh.
2. Reject non-finite vertices before any topology changes.
3. Convert polygonal faces to triangles only if `BooleanMeshOp4` is intended to support polygonal inputs. If the API contract is strictly triangular, reject non-triangular input instead.
4. Rebuild involved-face indices after conversion or cleanup.
5. Reject degenerate faces. Do not silently remove faces from a non-empty input because removing one triangle can open a closed surface and change the solid.
6. Require valid face indices.
7. Require a closed manifold.
8. Require consistent orientation.
9. Verify that each connected component has non-zero signed volume. Reject zero-volume closed surfaces.
10. Preserve an exact mapping from every normalized triangle back to its source operand and source face.

## Step 5: Build a Deterministic Broad Phase

1. Compute exact or outward-rounded axis-aligned bounding boxes for every normalized triangle.
2. Use a deterministic acceleration structure to enumerate candidate triangle pairs between `lhs` and `rhs`.
3. Sort candidate pairs by `(lhs_face_id, rhs_face_id)` before processing.
4. Include pairs whose boxes touch exactly. Do not discard face-touching, edge-touching, or vertex-touching contacts.
5. Add tests proving the broad phase returns candidates for coplanar and merely touching triangles.

## Step 6: Implement Robust Triangle-Triangle Classification

1. Replace ad-hoc edge-plane interpolation with a triangle-triangle classifier based on exact orientation predicates.
2. For every candidate pair, classify it into exactly one case:
   - disjoint,
   - proper segment intersection,
   - point contact,
   - shared edge or partially overlapping edge,
   - coplanar disjoint,
   - coplanar overlap area,
   - identical triangles or reversed identical triangles.
3. For non-coplanar intersections, construct exact segment endpoints from plane-edge intersections using the exact coordinate model from Step 3.
4. For coplanar pairs, project onto the dominant 2D plane chosen deterministically from the exact normal, then perform exact 2D segment arrangement tests.
5. Store every intersection as topological constraints on both participating source triangles.
6. Do not use tolerance thresholds to decide topology. Tolerances may only be used when converting final exact coordinates to `T` after topology is complete.

## Step 7: Split Every Input Triangle Into Arrangement Facets

1. For each source triangle, gather all constraint vertices and constraint segments created by intersections with the opposite mesh.
2. Add the triangle's original three edges as constraints.
3. Build a constrained planar arrangement inside the source triangle using exact 2D coordinates in the triangle's local plane.
4. Split the triangle into simple subfacets whose interiors do not cross any triangle from the other operand.
5. Ensure that matching intersection geometry receives the same global vertex ids on both operands.
6. Preserve directed boundary cycles for every subfacet.
7. Triangulate each subfacet deterministically after the arrangement is complete. Ear clipping or constrained triangulation is acceptable only if it operates on the exact topology and produces stable output.
8. Reject any arrangement state that would create a non-manifold local edge for a valid input, because that indicates a classifier or construction bug.

## Step 8: Classify Arrangement Facets Against the Opposite Solid

1. For every split facet from `lhs`, classify its interior relative to `rhs` as inside, outside, or on-boundary.
2. For every split facet from `rhs`, classify its interior relative to `lhs` as inside, outside, or on-boundary.
3. Choose a representative point in the relative interior of the exact facet. Never choose a point on a facet edge or vertex.
4. Use a robust point-in-closed-surface classifier with degeneracy handling. Preferred method:
   - first check exact on-boundary status against the opposite arrangement,
   - if not on-boundary, cast a deterministic symbolic ray or use exact winding number evaluation,
   - if the first ray is degenerate, retry a fixed ordered set of symbolic directions and record which one succeeded.
5. For coincident boundary facets, determine whether the two solids have equal or opposite local orientation. Store this as same-facing or opposite-facing contact.
6. Cache classification by connected coplanar regions where possible, but only after the region's topology is known.

## Step 9: Select Output Facets by Boolean Truth Tables

1. Replace BSP tree truth tables with facet-selection truth tables over arrangement facets.
2. For each `lhs` facet, decide whether the material immediately outside that oriented facet changes across the Boolean result.
3. For union, emit facets bounding `inside(lhs) OR inside(rhs)`.
4. For intersection, emit facets bounding `inside(lhs) AND inside(rhs)`.
5. For subtraction, emit facets bounding `inside(lhs) AND NOT inside(rhs)`. Reverse orientation for selected `rhs` facets that become part of the subtraction boundary.
6. For exclusion, emit facets bounding `inside(lhs) XOR inside(rhs)` directly. Do not implement XOR as `(A - B) union (B - A)` unless tests prove the direct path and composed path match.
7. For coincident same-facing facets, drop duplicate internal facets for union and keep the correct boundary facets for intersection.
8. For coincident opposite-facing facets, handle subtraction and exclusion explicitly so touching surfaces do not produce doubled zero-thickness faces.
9. Record a reason code for every emitted or discarded facet. Use this for debugging failed tests.

## Step 10: Assemble the Output Halfedge Mesh

1. Create output vertices from exact arrangement vertex ids, not from approximate coordinate welding.
2. Convert exact coordinates to `T` once, at emission time, using deterministic rounding.
3. Before committing each output face, remove repeated consecutive vertices and reject zero-area exact facets.
4. Insert directed halfedges and link twins by exact vertex id pairs.
5. Require every output undirected edge to have exactly two incident faces unless the whole output is empty.
6. Require paired directed edges to be oppositely oriented.
7. Split disconnected shells into components internally, but return them in one `fv_surface_mesh` with deterministic component ordering.
8. Sort components, vertices, and faces deterministically. A stable ordering can use bounding boxes, exact coordinates, source ids, then local ids.
9. Rebuild involved-face indices before returning.

## Step 11: Orient and Verify by Construction, Not Repair

1. Use source facet orientation and Boolean truth-table side information to orient output facets during selection.
2. Remove dependence on `OrientFaces(mesh)` as a repair step for normal operation. It may remain as a debug assertion helper, but the emitted mesh should already be consistently oriented.
3. Run final postcondition verification:
   - finite vertices,
   - triangular faces,
   - valid indices,
   - no degenerate faces,
   - closed manifold,
   - consistent orientation,
   - no duplicate faces,
   - no self-intersections except shared manifold edges and vertices.
4. If final verification fails, throw `std::runtime_error` with the failed invariant and operation name. Do not return a plausibly shaped malformed mesh.
5. Add debug-only checks that classify a sample point on both sides of every output facet and verify that the Boolean predicate changes across the facet.

## Step 12: Remove Nondeterminism and Silent Approximation From Boolean4

1. Do not call `bsp_tree_volume::from_fv_surface_mesh` from `BooleanMeshOp4` once the arrangement implementation is complete.
2. Remove reliance on `std::random_device`, randomized triangle order, fixed `+X` parity rays without degeneracy handling, midpoint edge-plane fallbacks, approximate rounded vertex welding, and hard-coded cube output for uniform `In` BSP leaves from the Boolean4 path.
3. If `bsp_tree_volume` remains in the tree for other callers, leave it alone except for independent bug fixes. Boolean4 correctness should not depend on BSP extraction.
4. Ensure the Boolean4 implementation has no source of process-dependent ordering such as unordered containers without sorted emission.

## Step 13: Handle Empty, Identity, and Universal Cases Correctly

1. Treat an empty mesh as the empty solid only after verifying it is truly empty. A mesh with vertices but no faces should normalize to the empty solid only if the API accepts that representation.
2. For identity outputs, return a normalized copy produced through the same emission path used by arrangement results.
3. Never represent all of space as a finite cube. For bounded closed input meshes, valid Boolean outputs should be bounded or empty. If an internal state represents unbounded solid unexpectedly, throw.
4. Ensure subtraction from empty and intersection with empty never touches the arrangement code unnecessarily.

## Step 14: Add Diagnostic and Developer Instrumentation

1. Add internal diagnostic structs for validation failures, triangle pair classification, arrangement splitting, facet classification, and output assembly.
2. Include operand names, face ids, vertex ids, and exact predicate case names in diagnostic messages.
3. Keep the public API throwing standard exceptions unless the project already has a richer error type.
4. Add optional compile-time debug dumping of arrangement facets to help reproduce failures.
5. Ensure debug dumping is deterministic and disabled by default.

## Step 15: Expand Verification Tests to Property and Fuzz Cases

1. Add randomized but deterministic tests using fixed seeds.
2. Generate closed convex polyhedra and compare Boolean identities:
   - `A union B == B union A`,
   - `A intersection B == B intersection A`,
   - `A subtraction A == empty`,
   - `A exclusion A == empty`,
   - `A union empty == A`,
   - `A intersection empty == empty`.
3. Add metamorphic tests under translation, rotation, uniform scaling, and operand order where mathematically applicable.
4. Add near-degenerate but valid inputs:
   - very thin tetrahedra with nonzero volume,
   - triangles intersecting near vertices,
   - nearly coplanar but distinct faces,
   - large coordinate magnitudes,
   - very small coordinate magnitudes.
5. Every fuzz result must pass the full postcondition verifier. Failures should preserve the input seed and operation in the test output.

## Step 16: Validate Numerical Robustness Boundaries

1. Document the exact coordinate and rounding policy chosen in Step 3.
2. Define which input coordinate types are supported for exact construction. The current explicit instantiations are `float` and `double`; internally promote both to the same exact type if practical.
3. Prove through tests that results are deterministic for both `float` and `double` instantiations and for both `uint32_t` and `uint64_t` index types.
4. Add overflow checks when converting output vertex and face counts to `I`.
5. Reject outputs that cannot be indexed by the requested index type.

## Step 17: Replace `BooleanMeshOp4` With the New Pipeline

1. The final `BooleanMeshOp4` flow should be:
   - validate the enum,
   - normalize and validate `lhs`,
   - normalize and validate `rhs`,
   - handle empty identities,
   - build the pairwise triangle arrangement,
   - classify arrangement facets,
   - select facets by operation,
   - assemble the halfedge mesh,
   - run postcondition verification,
   - return the mesh.
2. Keep wrapper functions unchanged except for any needed include or namespace adjustments.
3. Keep explicit template instantiations in `src/YgorMeshesBoolean4.cc` for the four existing type combinations.
4. Ensure no Boolean4 path depends on `YgorMeshesBSPTree.h` after replacement unless intentionally retained for a compatibility test.

## Step 18: Update Documentation

1. Update comments in `src/YgorMeshesBoolean4.h` so they describe the arrangement-based implementation rather than a BSP-backed engine.
2. Update or replace `YgorMeshesBoolean4.md` with the new algorithm and the invariants it enforces.
3. Document unsupported inputs clearly:
   - non-finite coordinates,
   - non-manifold surfaces,
   - inconsistent orientation,
   - degenerate faces,
   - invalid indices,
   - outputs too large for `I`.
4. Document that valid, topologically well-defined closed inputs produce deterministic closed manifold outputs.

## Step 19: Run the Full Build and Test Suite

1. Build all explicit template instantiations.
2. Run the existing project tests.
3. Run the new Boolean4 regression tests.
4. Run deterministic fuzz tests for enough fixed seeds to exercise edge cases.
5. Run any sanitizer configuration available in the project.
6. Confirm that no test depends on randomized order or machine-local state.

## Step 20: Remove or Quarantine Obsolete BSP-Based Assumptions

1. Remove `YgorMeshesBSPTree.h` includes from `YgorMeshesBoolean4.cc` if unused.
2. Delete Boolean4-specific comments claiming BSP semantics.
3. If the BSP code remains available publicly, add tests that prevent future developers from accidentally switching Boolean4 back to the BSP path.
4. Keep the old BSP implementation only as an independent utility unless a separate task repairs it to the same robustness standard.

## Acceptance Criteria

The repair is complete only when all of these are true:

1. `BooleanSubtraction4(A, empty)` returns a normalized representation of `A`.
2. Boolean4 produces byte-for-byte deterministic output for repeated identical inputs.
3. All non-empty outputs are finite, triangular, validly indexed, non-degenerate, closed, manifold, consistently oriented, and free of duplicate faces.
4. Touching, overlapping, coincident, contained, disjoint, and non-axis-aligned test cases pass for union, intersection, exclusion, and subtraction.
5. The Boolean4 path no longer uses randomized BSP construction, midpoint plane intersections, fixed-direction threshold ray casting without degeneracy handling, approximate coordinate welding as topology, or hard-coded finite output for an unbounded `In` leaf.
6. Invalid inputs fail with clear operand-specific diagnostics instead of silent empty output or approximate repair.
7. The public API and explicit template instantiations remain source-compatible.
