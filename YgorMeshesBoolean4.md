# `src/YgorMeshesBoolean4.cc` Algorithm Guide

This note documents the Boolean4 engine implemented by `src/YgorMeshesBoolean4.cc` and declared by `src/YgorMeshesBoolean4.h`.

Boolean4 no longer delegates to the BSP volume code. The implementation builds a deterministic split-facet arrangement of the input surface meshes, classifies arrangement facets against the opposite solid, selects facets with direct Boolean truth tables, and assembles the result through topology keys and halfedge validation.

## Public API Surface

`src/YgorMeshesBoolean4.h` declares `enum class MeshBooleanOperation4` with these operations:

- `Union`
- `Intersection`
- `Exclusion`
- `Subtraction`

The generic entry point is `BooleanMeshOp4(lhs, rhs, op)`. The convenience wrappers `BooleanUnion4`, `BooleanIntersection4`, `BooleanExclusion4`, and `BooleanSubtraction4` call `BooleanMeshOp4` with the corresponding operation.

All functions are templates over coordinate type `T` and face-index type `I`. `src/YgorMeshesBoolean4.cc` explicitly instantiates each API for:

- `float, uint32_t`
- `float, uint64_t`
- `double, uint32_t`
- `double, uint64_t`

## Supported Inputs

Boolean4 accepts finite facet-vertex surface meshes representing bounded closed solids. Empty meshes are accepted as the empty solid.

Non-empty operands are normalized and validated before Boolean evaluation:

- Non-finite coordinates are rejected.
- Face indices must be in range.
- Faces with fewer than three vertices are rejected.
- Polygonal faces are triangulated by fan triangulation during normalization.
- Normalized faces must be triangular.
- Degenerate triangles are rejected.
- Inputs must be closed manifold meshes.
- Inputs must have consistent orientation.
- Every connected closed component must have non-zero signed volume.

Diagnostics identify the failing operand as `lhs` or `rhs` where applicable. Invalid `MeshBooleanOperation4` values throw `std::invalid_argument` instead of silently returning an empty mesh.

## Coordinate And Topology Policy

The implementation uses floating-point coordinates for geometric storage and final output, but topology identity is not keyed directly by raw floating-point values. Arrangement and output vertices are assigned deterministic snap-rounded topology keys.

The topology key promotes `float` and `double` values to `long double`, rounds them onto a shared `1e-12` grid, and stores the rounded integer coordinates in `Boolean4SnapCoord`. The key is used for deterministic vertex identity during arrangement splitting and output assembly. Output coordinates are emitted from the associated approximate coordinate once, at mesh assembly time.

This policy is deterministic for the supported explicit instantiations and is guarded by tests for `float` and `double` coordinate types with both `uint32_t` and `uint64_t` indices. Outputs whose vertex or face counts cannot be represented by `I` are rejected.

## Pipeline

`BooleanMeshOp4` follows this high-level flow:

1. Validate the operation enum.
2. Normalize and validate the left operand.
3. Normalize and validate the right operand.
4. Handle empty-solid identities without entering the full arrangement path.
5. Use a deterministic cuboid cell arrangement for two detected axis-aligned cuboids.
6. Otherwise build the pairwise triangle arrangement.
7. Classify arrangement facets against the opposite solid.
8. Select facets with direct Boolean truth tables.
9. Assemble the output halfedge mesh from topology keys.
10. Run output postcondition checks.

Identity outputs, such as `A union empty`, `A subtraction empty`, and `empty exclusion B`, are emitted through the same normalized arrangement-to-output assembly path used by ordinary non-empty results. They are not returned as raw unvalidated input meshes.

## Input Arrangement

The arrangement path starts by computing deterministic axis-aligned bounding boxes for every normalized triangle. Candidate triangle pairs are enumerated by inclusive AABB overlap, so exactly touching boxes remain candidates. Candidate pairs are sorted by normalized face ids and source-face references before processing.

Each candidate pair is classified into one of these cases:

- disjoint
- proper segment intersection
- point contact
- shared edge or partially overlapping edge
- coplanar disjoint
- coplanar overlap area
- identical triangles
- reversed identical triangles

Non-coplanar segment intersections and coplanar contacts create constraint vertices and constraint segments. Each source triangle then gathers its own constraints plus its original boundary and is split into deterministic arrangement facets in a projected local 2D plane. Facets retain source operand, source face id, local split-face id, and local facet id.

## Facet Classification

Every split facet from `lhs` is classified relative to `rhs`, and every split facet from `rhs` is classified relative to `lhs`.

The classifier uses the facet centroid as an interior representative for triangular subfacets. It first checks whether the point lies on an opposite triangle. Boundary facets record whether the local surface orientation is same-facing or opposite-facing. Non-boundary facets are classified by deterministic ray tests using a fixed ordered set of non-axis-aligned ray directions; degenerate rays are skipped in order.

The classification result for each facet is one of:

- outside
- inside
- on-boundary with same-facing contact
- on-boundary with opposite-facing contact

Classification records are sorted deterministically before selection.

## Boolean Selection

Boolean4 selects arrangement facets directly, rather than composing operations through other operations or reconstructing surfaces from a volumetric tree.

The selection rules implement these predicates:

- Union emits facets bounding `inside(lhs) OR inside(rhs)`.
- Intersection emits facets bounding `inside(lhs) AND inside(rhs)`.
- Subtraction emits facets bounding `inside(lhs) AND NOT inside(rhs)` and reverses selected `rhs` facets.
- Exclusion emits facets bounding `inside(lhs) XOR inside(rhs)` directly.

Coincident same-facing facets are de-duplicated by keeping the deterministic `lhs` representative where needed and discarding the duplicate `rhs` facet. Opposite-facing contacts are handled explicitly for subtraction and exclusion so touching surfaces do not create doubled zero-thickness output faces.

Every selection decision records a reason code. Debug dumping can include arrangement records, facet classifications, and decision reason names.

## Output Assembly

Output assembly collects emitted facet candidates from both operands and sorts them deterministically. Repeated-vertex and zero-area facets are discarded before committing topology.

Vertices are created from deterministic topology keys, not from approximate coordinate welding. Faces are sorted by canonical vertex ids, duplicate faces are removed, and requested index type capacity is checked before assigning output indices.

The assembler builds directed halfedge candidates for every emitted triangle. Every output undirected edge must have exactly two incident faces, and paired directed halfedges must be oppositely oriented. Failure of either invariant throws `std::runtime_error` with the offending vertex ids.

After assembly, involved-face indices are rebuilt.

## Output Invariants

Every non-empty result is verified before it is returned:

- All vertices are finite.
- All faces are triangular.
- All face indices are valid.
- No face is degenerate.
- No duplicate triangular face exists.
- The result is a closed manifold.
- Face orientation is consistent.

If a postcondition fails, `BooleanMeshOp4` throws `std::runtime_error` naming the operation and failed invariant. Boolean4 does not use final orientation repair as the normal correctness mechanism; selected facets are expected to be oriented correctly by construction.

## Empty And Identity Cases

Empty meshes represent the empty solid after finite-coordinate validation. A mesh with vertices but no faces normalizes to the empty solid if the vertices are finite.

The empty-solid identities are:

- `empty union B = B`
- `A union empty = A`
- `empty intersection B = empty`
- `A intersection empty = empty`
- `empty subtraction B = empty`
- `A subtraction empty = A`
- `empty exclusion B = B`
- `A exclusion empty = A`

Identity outputs are normalized and verified before returning.

## Diagnostics And Debug Dumps

Internal diagnostics cover validation failures, triangle-pair classification, arrangement splitting, facet classification, and output assembly. Public API failures use standard exceptions.

Optional debug dumping is disabled by default. When compiled with `YGOR_MESHES_BOOLEAN4_ENABLE_DEBUG_DUMP`, setting `YGOR_MESHES_BOOLEAN4_DEBUG_DUMP` to a file path writes a deterministic text dump for the current Boolean operation.

## Unsupported Or Rejected Cases

Boolean4 rejects these inputs or outputs:

- Non-finite coordinates.
- Faces with invalid indices.
- Faces with fewer than three vertices.
- Degenerate normalized triangles.
- Non-manifold surfaces.
- Inconsistently oriented surfaces.
- Zero-volume closed components.
- Invalid Boolean operation enum values.
- Outputs too large for the requested index type.
- Any non-empty output that fails the final topology and orientation postconditions.

Boolean4 represents bounded closed solids only. It never returns a finite placeholder for all of space; an unexpected unbounded or topologically invalid state must fail rather than produce a plausible malformed mesh.

## Determinism

All containers used for observable output are sorted by stable ids, source ids, topology keys, or canonical vertex ids before emission. The Boolean4 path does not use randomized construction, process-dependent unordered traversal, approximate vertex welding as topology, or hard-coded finite output for unbounded solids.

The regression suite covers empty operands, deterministic repeated outputs, closed-mesh postconditions, cuboid cases with disjoint, overlapping, contained, identical and touching inputs, non-axis-aligned tetrahedron/cube cases, coplanar overlap, coincident faces, property identities, metamorphic transforms, and the supported explicit template instantiations.
