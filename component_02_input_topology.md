# Component 2: Input Topology Validation and Canonicalization

## 0. Purpose

Convert each raw index mesh into a certified, immutable B-rep operand with explicit halfedge adjacency, shells, orientation, exact source geometry, and stable IDs. Reject data that does not define the operation contract; do not heal it by tolerance.

## 1. Input contract

Accept an immutable `fv_surface_mesh<T, I>`, operand role, frozen context, and exact kernel.

The raw shape may contain multiple disconnected closed shells and polygonal facets. It may use arbitrary valid index ordering and may contain unused vertices if policy permits their deterministic removal. It must not rely on approximate planarity or implicit welding.

No downstream component may consume the raw mesh directly after this stage.

## 2. Required behavior

### Structural audit

- Check index range before dereference and detect `I`/`size_t` conversion overflow.
- Reject facets with fewer than three effective vertices, consecutive duplicate indices, repeated boundary vertices that make a non-simple ring, or zero exact area.
- Convert every finite `T` coordinate losslessly to the exact dyadic representation. Reject NaN, infinity, and unsupported encodings.
- Preserve an input-to-canonical provenance map.

### Exact facet audit

- Establish a support plane from a proven non-collinear triple.
- Prove every facet vertex lies on that plane with exact orientation predicates.
- Project onto a deterministic dominant coordinate plane selected without unsafe rounding.
- Prove the polygon boundary is simple, including endpoint-touch rules, and has non-zero signed area.
- Record exact plane, orientation, projected ring, and a deterministic internal triangulation used only for acceleration/predicates. Triangulation diagonals must lie in the polygon and must not alter the public facet identity.

### Topology construction

- Build directed halfedge uses and canonical undirected edges from index pairs.
- Require exactly two oppositely directed uses per edge.
- Build ordered facet adjacency and vertex links. Require every manifold vertex link to be a single cycle within its shell.
- Split disconnected adjacency components into shells. Mere coordinate coincidence does not create adjacency.
- Detect duplicate facets and duplicate edge uses exactly.

### Embeddedness and solid audit

Using the exact kernel and a conservative self-broad-phase:

- Reject non-adjacent self-intersections and positive-area self-overlaps unless a separately specified normalization contract handles them.
- Check adjacent facets intersect only in their shared topological feature, accounting for coplanar adjacent facets.
- Check distinct shells satisfy the selected nesting/contact policy.
- Establish shell containment with exact point location and symbolic perturbation.
- Verify consistent orientation. Under the default contract, verify/normalize only explicitly allowed shell orientation conventions so occupied side is unambiguous; never silently reinterpret contradictory shells.

### Canonicalization

Create stable feature IDs by deterministic structural keys. Canonicalization must not merge distinct vertices merely because coordinates are equal. Any optional removal of unused vertices records provenance and occurs before IDs freeze.

## 3. Output contract

Produce one `validated_operand` containing:

- Exact original vertex coordinates and raw `T` coordinates.
- Facet rings, support planes, projected forms, and proven triangulations.
- Halfedges, edge twins, ordered vertex links, facet adjacency, shells, and nesting.
- Consistent occupied-side orientation and operand role.
- Stable IDs and complete source provenance.
- A validation certificate and conservative bounds for acceleration.

Invariants:

- Every halfedge has one twin and one incident facet; every edge has exactly two uses.
- Facet rings and vertex links are closed and consistently oriented.
- Every facet is exactly planar, simple, and non-degenerate.
- Every shell is connected, closed, orientable, and embedded under the accepted policy.
- Occupied side is defined everywhere away from the boundary.
- Internal triangulations exactly partition their source polygon and introduce no semantic boundary.

Failure conditions identify the smallest known offending source features and exact predicate evidence. No invalid input is converted to a best-effort solid.

## 4. Verification and definition of done

- Unit tests cover index overflow, duplicate uses, pinched vertices, bow-tie links, non-planarity, self-crossing polygons, zero area, shell nesting, cavities, and equal-coordinate/distinct-index cases.
- Random valid meshes survive index/facet permutation with equivalent canonical operands.
- Self-intersection detection is checked against exhaustive facet pairs.
- Reversing all facets has the documented effect under each shell policy.
- The verifier reconstructs every adjacency relation independently from emitted facet rings.
