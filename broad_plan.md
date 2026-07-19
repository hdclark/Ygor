# Bounded Floating-Point Surface Mesh Boolean Engine: Broad Plan

## 0. Purpose

Design and implement a fast, dependency-free surface-mesh Boolean engine inspired by the central architectural ideas of the Manifold algorithm:

- topology is represented and maintained exactly with integer identities and connectivity;
- geometry is represented with bounded floating-point values;
- each geometric question is answered once and every dependent decision reuses that answer;
- exact-coordinate equality is never used to infer topology;
- symbolic perturbation provides deterministic semantics for exact ties and coincident geometry;
- degeneracies are removed or separated by topology-preserving operations within an explicit tolerance budget; and
- a mesh is published only when its indexed topology is valid and its geometry passes the required tolerance-aware checks.

The engine accepts two facet-vertex surface meshes:

```cpp
template <class T, class I>
bounded_boolean_result<T, I> bounded_boolean(
    const fv_surface_mesh<T, I>& a,
    const fv_surface_mesh<T, I>& b,
    boolean_operation operation,
    const bounded_boolean_options<T>& options);
```

Each operand may contain multiple disconnected closed shells. Shells may be nested to represent cavities and islands. Facets may be polygons rather than triangles. The output is a topologically valid `fv_surface_mesh<T, I>`; the initial output policy may use triangles exclusively even when the input contains polygonal facets.

This document defines the contract, architecture, runtime stages, component boundaries, invariants, test strategy, and implementation order. It intentionally does not prescribe application-specific integration beyond the required public mesh type.

## 1. Non-goals

The first production version does not promise any of the following:

- exact point-set equality with the mathematical Boolean of the binary floating-point inputs;
- exact rational output coordinates;
- automatic repair of arbitrary open, non-manifold, inconsistently oriented, or severely self-intersecting input soups;
- preservation of geometric features smaller than the caller-authorized tolerance;
- polygonal output with minimal face count;
- analytic surfaces, curves, or NURBS output;
- topology inferred by snapping nearby, independently created vertices; or
- success for every finite input.

Failure is preferable to publishing a mesh that violates the declared contract.

## 2. Fundamental model

### 2.1 Topology and geometry are separate

Topology consists of integer-valued facts:

- vertex-occurrence identities;
- directed and undirected edges;
- paired halfedges;
- face cycles;
- component membership;
- orientation;
- source provenance; and
- continuation of an intersection through adjacent faces.

Geometry consists of bounded floating-point facts:

- vertex coordinates;
- normals and projection frames;
- intersection parameters;
- uncertainty bounds;
- distance and residual estimates; and
- caller-authorized cleanup displacements.

Two topological vertices may have bit-identical coordinates. This is required for point-touching and edge-touching solids, pinched configurations, coincident but distinct sheets, and topology-preserving cleanup. Coordinate equality must never weld vertices or create adjacency.

### 2.2 Topological manifoldness

A successful output must be an oriented indexed two-manifold under the following definition:

1. Every output face has at least three distinct consecutive vertex indices and a non-zero oriented area under the output validity threshold.
2. Every directed face edge has exactly one paired edge with the same two vertex indices in reverse order.
3. Every undirected edge has exactly two incident face uses.
4. The incident face fan around each topological vertex occurrence forms one closed cycle.
5. Face orientation is globally consistent on each connected shell.
6. Separate topological occurrences are not merged solely because their coordinates coincide.
7. The output may contain any number of disconnected shells.

A union of two solids that touch only at a point or edge is represented as two topologically separate components with duplicated coordinates at the contact. This preserves a two-manifold indexed result without inventing a geometric gap.

### 2.3 Geometric validity

The geometry contract is tolerance-bounded and conditional.

An input is `epsilon-valid` when there exists a perturbation of each input vertex by less than its declared input precision such that the resulting oriented indexed mesh is embedded and represents the same intended solid under the selected shell semantics.

A successful output is `tolerance-checked` when:

- its indexed topology satisfies Section 2.2;
- its vertices and cleanup operations remain within the declared global and local displacement budgets;
- no output triangle is inverted or collapsed beyond the accepted degeneracy policy;
- no forbidden new triangle-triangle intersection is detected outside the output uncertainty envelopes;
- sampled and structural side-classification checks agree with the requested Boolean operation; and
- the output can be re-ingested by the input validator with its published precision metadata.

This is not a proof of exact Boolean point-set equality. It is a strong a posteriori plausibility contract for bounded floating-point geometry.

## 3. Public contract

### 3.1 Operations

The engine supports regularized operations on finite solids:

- union;
- intersection;
- `A - B`;
- `B - A`; and
- symmetric difference.

Regularization removes isolated points and curves that do not bound volume. Touching at a point or edge does not create an occupied connection unless a positive-volume connection exists.

### 3.2 Input domain

The default accepted input domain is:

- `T` is a supported IEC 60559 binary floating-point type, initially `float` or `double`;
- `I` is an unsigned index type with enough capacity for the result;
- all coordinates are finite;
- every facet has a simple polygonal boundary after removal of consecutive duplicate indices;
- every facet is planar within the input precision contract;
- every undirected topological edge has exactly two oppositely directed uses;
- every topological vertex occurrence has one cyclic link;
- face orientation is consistent;
- each operand represents a regular closed solid under the selected shell policy;
- multiple disconnected shells, cavities, nested islands, and multiple solids are allowed;
- duplicate coordinates are allowed when indices and topology intentionally distinguish them; and
- geometry is epsilon-valid or the caller explicitly requests a diagnostic topology-only attempt.

The default solid policy is outward-oriented occupied boundaries with alternating nested shells. Away from the boundary, the total oriented winding must be either zero or one. Cavities reverse orientation; islands inside cavities reverse again.

Inputs outside this domain return a typed error. A separate normalization or repair API may be added later, but Boolean evaluation itself must not silently repair unknown topology.

### 3.3 Options

At minimum, `bounded_boolean_options<T>` contains:

```cpp
struct bounded_boolean_options {
    T tolerance;                    // Maximum authorized geometric deviation.
    T input_precision_a;            // Accrued uncertainty of operand A.
    T input_precision_b;            // Accrued uncertainty of operand B.
    solid_policy solids;
    contact_policy contacts;        // Versioned symbolic perturbation rules.
    output_policy output;           // Triangulated manifold output in v1.
    verification_level verification;
    determinism_policy determinism;
    execution_policy execution;
    resource_policy resources;
    diagnostic_policy diagnostics;
};
```

`tolerance` is a maximum authorization, not a general-purpose equality epsilon. It may permit removal of features, collapse of short edges, and local displacement only through explicitly documented cleanup operations. It must not be used to create arbitrary adjacency between unrelated features.

### 3.4 Result

A successful result contains:

```cpp
struct bounded_boolean_success {
    fv_surface_mesh<T, I> mesh;
    T output_precision;
    T maximum_authorized_tolerance;
    T maximum_realized_displacement;
    topology_report topology;
    geometry_report geometry;
    provenance_report provenance;
    deterministic_digest digest;
};
```

The output mesh is always topologically valid. The default public policy requires `geometry.status == tolerance_checked` before success is returned.

A diagnostic policy may permit a topology-only artifact to be retained internally, but it must not be exposed through the ordinary success type.

### 3.5 Typed failures

Required failure categories include:

- `input_contract_error`;
- `input_geometry_not_epsilon_valid`;
- `unsupported_platform`;
- `invalid_tolerance`;
- `ambiguous_shell_semantics`;
- `geometric_condition_exceeds_tolerance`;
- `cleanup_budget_exceeded`;
- `result_geometry_not_validated`;
- `index_overflow`;
- `resource_limit`;
- `cancelled`; and
- `internal_invariant_error`.

Every error records the stage, operand and feature identities, numerical bounds, tolerance, and a deterministic replay payload.

## 4. Precision and tolerance model

### 4.1 Distinct quantities

The implementation must keep these quantities separate:

- **Machine roundoff floor:** the scale-dependent floating-point uncertainty implied by `T` and the coordinate magnitude.
- **Input precision:** accrued uncertainty supplied with each input or conservatively derived on import.
- **Construction uncertainty:** a conservative forward-error estimate for each newly constructed intersection coordinate.
- **Cleanup displacement:** actual movement introduced by an authorized collapse, projection, or deduplication operation.
- **Output precision:** a conservative aggregate uncertainty attached to the result.
- **User tolerance:** the maximum displacement or feature removal authorized by the caller.

No component may treat all six as one undifferentiated epsilon.

### 4.2 Precision envelopes

Every original and constructed vertex carries a precision envelope:

```cpp
struct bounded_point {
    vec3<T> nominal;
    vec3<T> axis_error;
    T radial_error;
    provenance_id provenance;
};
```

The exact internal representation may be more compact, but it must support conservative overlap, separation, and residual tests.

For an edge-face intersection, the producer computes:

- the nominal intersection;
- an error bound based on operand coordinate uncertainty, operation count, and conditioning;
- residuals against the source edge and face plane;
- whether the result is interior, endpoint, near-parallel, or symbolically tied; and
- whether the uncertainty exceeds the caller's tolerance.

Near-parallel or ill-conditioned constructions must not produce unbounded coordinates. They are either interpreted as contact through the symbolic policy, handled as a coplanar case, or rejected with `geometric_condition_exceeds_tolerance`.

### 4.3 Propagation

For every stage:

```text
output_precision >= inherited_input_precision
output_precision >= construction_roundoff_bound
output_precision >= maximum_cleanup_displacement
output_precision <= user_tolerance for ordinary success
```

Repeated Boolean operations therefore carry their uncertainty forward. A downstream operation must not reset precision merely because the coordinates fit in `T`.

### 4.4 Strict floating-point environment

Topology-affecting arithmetic requires:

- nearest-even rounding;
- no fast-math;
- no reassociation;
- no implicit finite-only assumptions;
- a documented floating contraction policy;
- prescribed rounding points to `T`;
- finite checks after every construction boundary; and
- no transcendental functions in authoritative topology decisions.

Supported platforms must pass bit-pattern conformance tests. Unsupported environments return `unsupported_platform`.

## 5. Guiding principles

1. **Topology is authoritative.** Integer connectivity, not coordinate proximity, defines the result.
2. **Compute each question once.** A canonical feature relation or intersection event has one producer and immutable identity.
3. **Reuse lineage.** Adjacent faces reference the same intersection vertex and event rather than independently recomputing it.
4. **No tolerance-based welding.** Close features remain distinct unless their shared lineage or an explicit cleanup proof permits merging.
5. **Symbolic perturbation breaks ties.** Exact floating-point equality and coincident geometry use versioned operation-specific rules.
6. **Tolerance is a budget.** It authorizes bounded geometric simplification; it does not decide topology opportunistically.
7. **Duplicate coordinates are normal.** Distinct topological occurrences may occupy the same point.
8. **Manifoldness is constructed, not repaired at the end.** Every output edge is emitted with its pair.
9. **Cleanup preserves manifoldness.** Edge collapse and degenerate removal split occurrences when needed rather than creating non-manifold edges.
10. **Determinism is explicit.** Feature IDs, stable sorts, tie keys, and canonical output remove traversal and scheduling dependence.
11. **Verification is independent.** Verifiers reconstruct required facts instead of trusting producer-owned counts or classifications.
12. **Fail closed.** A topology-valid but geometrically implausible artifact is not ordinary success.
13. **No external dependencies.** Production and normative tests are implemented in-tree using the language and standard library facilities permitted by the project.

## 6. Runtime pipeline

1. Freeze the operation, tolerance, symbolic policy, limits, floating environment, and determinism policy.
2. Validate each facet-vertex operand and canonicalize stable feature identities.
3. Establish shell orientation and nesting semantics for disconnected shells, cavities, and islands.
4. Triangulate polygonal facets while preserving source-face provenance and shared topological edges.
5. Build the internal oriented halfedge representation and precision metadata.
6. Build conservative broad-phase acceleration structures.
7. Enumerate edge-triangle candidate pairs in both operand directions.
8. Evaluate each canonical relation once using bounded floating arithmetic and symbolic perturbation.
9. Intern all intersection events and share constructed vertices across every dependent face.
10. Mark cut edges and use connectivity to group vertices and regions whose winding classification is shared.
11. Compute winding or inclusion values for connected groups and convert them through the requested Boolean truth table.
12. Allocate retained and constructed output vertex occurrences, duplicating coordinates where topology requires separate sheets.
13. Assemble retained whole edges, split source edges, and new intersection edges as paired halfedges.
14. Build polygonal output-face cycles, including multiple contours and nested holes.
15. Triangulate each output polygon with a tolerance-aware, manifold-preserving triangulator.
16. Remove degeneracies and sub-tolerance features through budgeted topological cleanup.
17. Assemble and canonically order the `fv_surface_mesh<T, I>`.
18. Run mandatory topology and geometry verification.
19. Publish only if every required gate succeeds.

## 7. Components

| No. | Component | Principal artifact |
|---:|---|---|
| 1 | Contract, context, IDs, errors, and resources | immutable Boolean context |
| 2 | Input topology and shell semantics | validated operands |
| 3 | Precision, tolerance, and bounded arithmetic | precision ledger and bounded constructions |
| 4 | Polygon facet triangulation and provenance | source triangle complex |
| 5 | Canonical halfedge topology | immutable internal manifolds |
| 6 | Broad-phase collision enumeration | canonical candidate stream |
| 7 | Floating relation kernel and symbolic perturbation | signed feature relations |
| 8 | Intersection event registry | canonical shared intersection complex |
| 9 | Connectivity and winding classification | inclusion values and region labels |
| 10 | Boolean selection and occurrence accounting | retained surface uses |
| 11 | Paired output edge and face-cycle construction | polygonal output complex |
| 12 | Degeneracy-tolerant polygon triangulation | triangulated manifold complex |
| 13 | Budgeted cleanup and topological simplification | cleaned manifold complex |
| 14 | Output assembly and canonical serialization | `fv_surface_mesh<T, I>` |
| 15 | Verification, diagnostics, and replay | verification reports |
| 16 | Test infrastructure and qualification | permanent corpus and release evidence |
| 17 | Performance and deterministic concurrency | bounded execution services |

## 8. Component specifications and tests

### Component 1: Contract, context, IDs, errors, and resources

#### Responsibilities

- Freeze all policies before work begins.
- Assign stable operand, shell, source-vertex, source-edge, source-face, triangle, candidate, event, output-occurrence, edge, and face IDs.
- Define operation truth tables and symbolic perturbation versions.
- Track memory, entity, candidate, work, and output limits.
- Provide cancellation and deterministic error selection.
- Produce replay metadata from input bit patterns and options.

#### Invariants

- IDs are immutable and never reused within a context.
- Errors are typed; expected geometric difficulty never becomes `internal_invariant_error`.
- Resource exhaustion cannot publish a partial artifact.
- Parallel schedules choose the same first canonical failure.

#### Tests

- Unit tests for every operation truth table and error code.
- ID overflow, stale-handle, wrong-owner, and duplicate-ID mutation tests.
- Limit-minus-one, limit, and limit-plus-one tests for each resource class.
- Cancellation before, during, and after every transactional stage.
- Replay byte stability under thread-count and traversal permutations.

### Component 2: Input topology and shell semantics

#### Responsibilities

- Validate indices, facets, directed edge use, vertex links, orientation, and finiteness.
- Preserve duplicate coordinates as distinct topology unless indices already identify the same vertex.
- Identify disconnected shells.
- Establish nesting and orientation for cavities and islands.
- Reject shell arrangements whose occupied-side semantics cannot be determined within the input precision.
- Detect severe self-intersection or non-epsilon-valid geometry when mandatory input geometry validation is requested.

#### Invariants

- Every validated edge has two opposite uses.
- Every validated topological vertex occurrence has one cyclic fan.
- Each shell has a consistent orientation.
- Away from the boundary, the operand winding is zero or one under the selected solid policy.
- Canonical feature IDs do not depend on input array order.

#### Tests

- Closed tetrahedra, boxes, concave solids, multiple components, cavities, and nested islands.
- Vertex-touching and edge-touching disconnected shells with duplicate and shared coordinate values.
- Open edges, three-use edges, bow-tie vertices, reversed faces, zero-area facets, non-finite coordinates, and index overflow.
- Shell nesting permutations and orientation reversals.
- One-ULP gaps and overlaps around the declared input precision.
- Property tests that random facet, ring, vertex, and component permutations preserve the canonical validated artifact.
- Mutation tests that merge distinct coordinate-coincident occurrences and require rejection.

### Component 3: Precision, tolerance, and bounded arithmetic

#### Responsibilities

- Compute the machine roundoff floor from coordinate scale.
- Accept or derive input precision.
- Implement bounded `+`, `-`, `*`, `/`, interpolation, dot, cross, plane residual, and projection operations.
- Calculate construction conditioning and conservative uncertainty.
- Enforce tolerance budgets.
- Record every authorized cleanup displacement.

#### Invariants

- Every nominal constructed value lies inside its published enclosure under the supported arithmetic model.
- Bounds never shrink without proof.
- A construction with an enclosure larger than the available tolerance is not silently accepted.
- Precision propagation is monotonic across repeated operations.

#### Tests

- Known-answer bit-pattern tests across normals, subnormals, signed zero, extreme exponents, and adjacent floats.
- Differential tests against an in-tree exact rational oracle on bounded integer-coordinate fixtures.
- Random arithmetic tests that verify exact oracle values fall inside published bounds.
- Near-parallel line-plane and edge-face cases whose conditioning crosses the tolerance threshold.
- Repeated transform and Boolean chains to verify monotonic precision accumulation.
- Forced rounding-mode and fast-math configuration rejection tests.

### Component 4: Polygon facet triangulation and provenance

#### Responsibilities

- Validate each source facet's polygonal ring.
- Choose a deterministic projection.
- Triangulate simple planar facets without introducing cracks along shared source edges.
- Preserve mappings from each triangle edge and vertex to its source facet, edge, and vertex.
- Treat internal triangulation edges as non-geometric bookkeeping.

#### Invariants

- Triangles exactly cover the source facet under the bounded input geometry model.
- Shared source edges use the same endpoint indices and opposite direction.
- Internal diagonals cannot alter Boolean semantics.
- Re-triangulating a source polygon must not change the final Boolean topology.

#### Tests

- Convex, concave, thin, repeated-collinear, and large-vertex-count facets.
- Deterministic output under ring rotation and reversal with corrected orientation.
- Independent area and boundary coverage checks.
- Alternative legal triangulations used as metamorphic input variants.
- Mutation tests for a missing triangle, overlapping triangles, and a diagonal incorrectly marked as a source edge.

### Component 5: Canonical halfedge topology

#### Responsibilities

- Convert source triangles into an immutable halfedge structure.
- Pair all topological edges by source identity.
- Store source-face groups and coplanar provenance.
- Compute non-authoritative normals and conservative bounds.
- Canonically sort geometry for deterministic acceleration and output.

#### Invariants

- Pairing is reciprocal.
- Paired edges reverse their endpoints.
- Triangle cycles close.
- Internal triangulation edges are distinguished from source-feature edges.
- Coordinate-coincident but topologically distinct vertices remain distinct.

#### Tests

- Exhaustive halfedge consistency tests.
- Duplicate coordinate and duplicate face-provenance tests.
- Stable canonical bytes under input permutations.
- Mutation tests for one-way pairing, mismatched endpoints, and incorrect source labels.

### Component 6: Broad-phase collision enumeration

#### Responsibilities

- Build a deterministic BVH, Morton hierarchy, or equivalent conservative spatial index.
- Enumerate potentially intersecting edge-triangle pairs in both directions.
- Inflate bounds by the combined precision envelopes required for no false negatives.
- Exclude source-adjacent relations only by topology, never by a geometric epsilon.
- Produce a canonical candidate order independent of worker schedule.

#### Invariants

- No true bounded interaction is omitted.
- False positives are allowed.
- Candidate identity and order are deterministic.
- Broad-phase pruning never affects symbolic tie semantics.

#### Tests

- Exhaustive all-pairs comparison for bounded fixtures.
- One-ULP separation, exact-boundary contact, subnormal coordinates, extreme scales, and inflated uncertainty boxes.
- Random manifold meshes compared against exhaustive enumeration.
- BVH build-order, thread-count, and axis-tie permutations.
- Large disjoint and clustered benchmarks to detect accidental quadratic behavior.

### Component 7: Floating relation kernel and symbolic perturbation

#### Responsibilities

- Evaluate canonical vertex-face, edge-edge, and edge-face relations.
- Use a fixed dependency graph so each lower-dimensional result feeds later results.
- Return signed crossing multiplicity and nominal intersection coordinates with bounds.
- Resolve exact floating-point ties through a versioned operation-specific symbolic perturbation.
- Handle coplanar and coincident relations without tolerance-based random choices.

#### Symbolic policy requirements

The policy must specify, for every operation and operand role:

- conceptual expansion or contraction direction;
- behavior for coincident coplanar faces with same and opposite orientation;
- vertex-on-face, vertex-on-edge, edge-on-face, and equal-edge ties;
- ownership of retained coincident surfaces;
- deterministic ordering when feature IDs are otherwise symmetric; and
- expected regularized result for equal operands and touching components.

The symbolic decision changes classification, not stored coordinates.

#### Invariants

- The same feature relation is never recomputed through a mathematically equivalent but numerically different expression.
- Every non-zero crossing has an orientation and multiplicity.
- A symbolic tie is distinguishable from a numerically separated relation.
- Swapping operands and remapping the operation produces the corresponding remapped relation set.

#### Tests

- Full relation matrix for vertex, edge, face, coplanar, coincident, tangent, and transverse cases.
- Equal boxes, face-touching boxes, edge-touching boxes, and vertex-touching boxes for every operation.
- Near-parallel cases immediately above and below the construction-condition threshold.
- Operand-swap, axis-permutation, sign-flip, and power-of-two scale metamorphic tests.
- Test-only exact rational predicates for low-complexity expected signs.
- Mutation tests for a flipped crossing sign, different symbolic owner, and duplicate evaluation with inconsistent results.

### Component 8: Intersection event registry

#### Responsibilities

- Intern each canonical edge-triangle event once.
- Give each constructed coordinate one identity and one precision envelope.
- Share that identity across both source operands and every adjacent face.
- Order multiple events along a source edge or new face-face carrier.
- Preserve multiplicity and distinct topological occurrences when coordinates coincide.

#### Invariants

- One event identity has one nominal coordinate and one bound.
- Equal coordinates do not imply equal event identity.
- Events derived from the same canonical relation are never duplicated.
- Adjacent faces use the same event vertex rather than recomputing it.
- Ordered event sequences use a deterministic parameter and tie key.

#### Tests

- Several triangles meeting the same intersection point.
- Distinct exact or conceptual events rounding to the same `T` coordinate.
- Multiple intersections along one edge, including equal projected parameters.
- Hash-collision and ordering-tie injection.
- Mutation tests that duplicate an event, merge unrelated events, or change one consumer's vertex identity.

### Component 9: Connectivity and winding classification

#### Responsibilities

- Mark source edges broken by intersection events.
- Union vertices and surface regions connected through unbroken topology.
- Compute one winding or inclusion value per connected classification group.
- Propagate the value to group members.
- Use crossing multiplicities from Component 7; do not independently repeat geometric ray tests for every vertex.
- Support disconnected shells and nested cavities.

#### Invariants

- Members of one uncut connected group share the same classification.
- Crossing an oriented boundary updates winding consistently.
- Input shell winding agrees with the validated solid semantics.
- Classification does not depend on arbitrary ray direction or traversal order.

#### Tests

- Disjoint solids, containment, nested cavities, islands, and multiple components.
- Groups split by one cut edge and groups connected around large untouched regions.
- Known analytic winding fixtures.
- Comparison with an independent exact ray oracle on small integer-coordinate meshes.
- Mutation tests for a missing union, false union across a cut, and one incorrect crossing multiplicity.
- Metamorphic tests under source subdivision and alternative triangulation.

### Component 10: Boolean selection and occurrence accounting

#### Responsibilities

- Convert operand winding values and intersection multiplicities into retained surface uses for the requested operation.
- Determine orientation of retained pieces.
- Allocate multiple topological copies when inclusion magnitude or coincident sheets require them.
- Remove internal coincident surfaces according to symbolic ownership.
- Preserve point-touching and edge-touching components as distinct occurrences.

#### Invariants

- Selection is exactly the operation truth table applied to classified sides under the symbolic policy.
- Every retained surface has a prescribed outward orientation.
- No internal two-sided surface remains.
- Multiplicity is represented with duplicate occurrences rather than non-manifold edge use.

#### Tests

- Complete operation matrix for disjoint, equal, containment, proper overlap, and all contact dimensions.
- `A-A`, commutativity where applicable, directed difference swap, idempotence, and absorption identities.
- Vertex- and edge-touching unions must remain topologically separate with coordinate coincidence allowed.
- Mutation tests for reversed retained orientation, omitted multiplicity, and accidental welding.

### Component 11: Paired output edge and face-cycle construction

#### Responsibilities

- Allocate output vertices from retained source vertices and shared intersection events.
- Build whole retained edges, split source edges, and new face-face intersection edges.
- Pair output halfedges at creation time.
- Sort starts and ends along each source edge or new carrier and pair them deterministically.
- Form one or more oriented boundary cycles for each retained source-face region.
- Represent holes and nested contours explicitly.

#### Invariants

- No unpaired edge is ever published to the next stage.
- Starts and ends balance on every carrier.
- Carrier ordering is geometrically admissible within precision envelopes.
- If ordering is ambiguous enough to threaten geometry beyond tolerance, the stage fails rather than using an arbitrary heuristic.
- Face cycles close and do not reuse a directed edge.

#### Tests

- Multiple intersections on one edge and on one face pair.
- Annular faces, multiple holes, nested contours, thin corridors, and repeated projected coordinates.
- Exact coordinate ties with distinct collision IDs.
- Independent cycle reconstruction from halfedges.
- Mutation tests for one missing endpoint, wrong pairing, crossed pairing, and incomplete cycle.
- Fuzzed small polygon complexes checked against exhaustive start/end pairing oracles.

### Component 12: Degeneracy-tolerant polygon triangulation

#### Responsibilities

- Triangulate output polygons that may contain repeated coordinates, zero-length topological edges, nested contours, and features within tolerance.
- Preserve all boundary halfedges and their pairings.
- Use deterministic projection and ear ordering.
- When a local orientation test is uncertain, walk neighboring vertices or use bounded aggregate evidence until the decision is stable.
- Terminate with either a valid triangulation or a typed tolerance/geometry failure.

#### Invariants

- Every polygon boundary edge appears exactly once in the triangulation boundary.
- Internal diagonals are paired.
- Triangles cover the accepted polygon region without positive-area overlap under the bounded model.
- Triangulation never changes cross-face topology.
- Degenerate loops are handed to cleanup with complete provenance.

#### Tests

- Curated corpus of polygons emitted by Boolean stages, not only hand-authored simple polygons.
- Convex, concave, annular, deeply nested, self-touching within tolerance, repeated-coordinate, and needle-like polygons.
- Boundary preservation and independent area/coverage checks.
- Ring rotation, contour permutation, and projection-axis metamorphic tests.
- Automatic serialization and minimization of every fuzz failure.
- Mutation tests for missing ears, crossed diagonals, and lost hole boundaries.

### Component 13: Budgeted cleanup and topological simplification

#### Responsibilities

- Remove zero-area triangles, zero-length edges, collinear chains, and sub-tolerance artifacts.
- Collapse or swap edges only when the movement and local geometry remain within budget.
- Split pinched vertices or duplicate edge occurrences when a collapse would otherwise create a non-manifold edge.
- Permit removal of an entire sub-tolerance component only when authorized by policy and recorded in the report.
- Recompute local precision envelopes after every cleanup operation.

#### Invariants

- Manifoldness is preserved after every atomic cleanup step.
- Total displacement never exceeds the caller's tolerance.
- Cleanup never welds unrelated features solely because they are close.
- Genus or component changes are explicitly reported.
- Every cleanup action is replayable and has a local before/after certificate.

#### Tests

- Short-edge collapse cases that would produce three- or four-use edges without vertex splitting.
- Pinched vertices, tetrahedra collapsing below tolerance, thin handles, tiny disconnected components, and near-collinear fans.
- Budget boundary tests just below, at, and above tolerance.
- Independent topology verification after every mutation in debug tests.
- Mutation tests that omit required splitting or under-report displacement.
- Repeated cleanup idempotence.

### Component 14: Output assembly and canonical serialization

#### Responsibilities

- Convert the cleaned internal triangle manifold to `fv_surface_mesh<T, I>`.
- Preserve duplicate coordinates where topology requires them.
- Canonically order components, vertices, and faces.
- Check index capacity.
- Attach output precision, tolerance usage, and provenance to the success wrapper.
- Avoid polygon merging in v1 unless a later proof-preserving component is added.

#### Invariants

- Every output index is valid.
- Canonical ordering is independent of input permutations and thread scheduling.
- Serializing and re-ingesting the result preserves topology.
- Output precision is not smaller than any contributing bound.

#### Tests

- `float`/`double` and `uint32_t`/`uint64_t` matrices.
- Index-capacity boundary tests.
- Byte-identical output under repeated execution, thread counts, and input permutations.
- Round-trip through `fv_surface_mesh` validation.
- Duplicate-coordinate preservation tests.

### Component 15: Verification, diagnostics, and replay

#### Responsibilities

- Independently verify every published stage artifact.
- Reconstruct halfedge pairing, vertex links, event sharing, winding consistency, face cycles, and output topology.
- Verify construction residuals, precision envelopes, cleanup budgets, triangle orientation, and forbidden intersections.
- Produce deterministic diagnostic reports and replay payloads.
- Separate mandatory scalable checks from exhaustive bounded test oracles.

#### Mandatory final checks

- valid indices and finite coordinates;
- reciprocal paired halfedges;
- exactly two uses per undirected edge;
- one cyclic fan per topological vertex occurrence;
- consistent shell orientation;
- no degenerate output triangle outside the accepted policy;
- no forbidden non-adjacent triangle intersection beyond uncertainty envelopes;
- side probes agree with Boolean occupancy on both sides of representative output triangles;
- total displacement and precision stay within tolerance;
- successful output re-ingests under the same solid contract; and
- deterministic digest matches canonical content.

#### Tests

- Producer-shaped mutation artifacts with corrected counts and digests.
- Missing event, wrong winding, wrong edge pair, wrong vertex occurrence, invalid cleanup bound, and injected intersection mutations.
- Standalone verifier targets that cannot call producer grouping or pairing helpers.
- Replay compatibility and minimization tests.

### Component 16: Test infrastructure and qualification

#### Responsibilities

- Provide deterministic fixtures, exact low-complexity oracles, valid-manifold generators, invalid-input mutators, replay storage, shrinking, and coverage inventories.
- Map every contract clause and degeneracy category to executable tests.
- Maintain permanent regression cases for every discovered failure.

This component is specified in detail in Section 10.

### Component 17: Performance and deterministic concurrency

#### Responsibilities

- Provide bounded worker pools and private task outputs.
- Merge results in canonical key order.
- Parallelize broad phase, relation evaluation, and independent per-face work.
- Keep topology assembly phases serial where deterministic dependency ordering is simpler and safer.
- Instrument time, memory, candidate counts, exact pair checks, triangulation work, and cleanup work.

#### Invariants

- Thread count does not change canonical results or errors.
- Cancellation joins all workers before rollback.
- No parallel task mutates a shared artifact without deterministic ownership.
- Performance optimization cannot weaken candidate conservatism or verification.

#### Tests

- Thread counts 1, 2, and maximum.
- Forced scheduling permutations and task delays.
- Thread sanitizer and cancellation stress tests.
- Structural performance assertions preventing accidental all-pairs behavior.

## 9. Cross-component invariants

The following invariants apply throughout the pipeline:

- Every topological entity has one immutable owner and stable ID.
- Every canonical geometric relation is evaluated once.
- Every constructed vertex is shared by identity across all consumers.
- Coordinate equality never creates topology.
- Tolerance never creates adjacency without lineage or an explicit cleanup certificate.
- Symbolic perturbation affects classification and ownership, not nominal coordinates.
- Every output edge is born with its reverse pair.
- Every stage is transactional and publishes only a verified immutable artifact.
- Precision bounds are monotonic.
- Cleanup displacement is accumulated and audited.
- Every success is a topologically valid manifold mesh.
- Ordinary success additionally passes tolerance-aware geometric verification.
- Deterministic policies produce canonical output and diagnostics.

## 10. Test architecture

Testing is not a final phase. Every component is developed against its tests and verifier before the next production component begins.

### 10.1 Test levels

Each component must have:

1. **Unit tests** for local contracts and edge cases.
2. **Known-answer tests** with hand-auditable expected topology and geometry.
3. **Property tests** over generated valid inputs.
4. **Metamorphic tests** where equivalent input presentations must preserve results.
5. **Adversarial tests** focused on floating-point and degeneracy boundaries.
6. **Mutation tests** that prove independent verifiers reject plausible corrupt artifacts.
7. **Replay tests** for deterministic reproduction.
8. **Performance tests** with structural counters, not time alone.

### 10.2 In-tree exact test oracle

Although production uses bounded floating arithmetic, normative tests require a deliberately slow, in-tree exact oracle for low-complexity cases. It may use a small arbitrary-precision integer/rational implementation restricted to tests.

The oracle covers:

- orientation and side predicates;
- segment-plane and edge-face relation categories;
- exact event order on simple carriers;
- exact winding for small meshes;
- analytic convex-polytope Boolean expectations; and
- whether a nominal construction lies inside its published floating bound.

Production code must not depend on this oracle.

### 10.3 Required degeneracy matrix

At minimum, the permanent suite covers all operations and operand orders for:

- disjoint solids;
- strict containment;
- equal operands;
- coincident shells with same and opposite orientation;
- vertex-vertex, vertex-edge, vertex-face, edge-edge, edge-face, and face-face contact;
- proper transverse intersections;
- tangency;
- coplanar partial and complete overlap;
- intersection through original vertices and edges;
- several events at one coordinate;
- distinct events with the same rounded coordinate;
- concave facets and multiple contours;
- cavities, islands, and disconnected shells;
- radically different source triangulations;
- long thin facets and sliver triangles;
- signed zero, subnormal values, adjacent floats, extreme exponents, and large translations;
- near-parallel constructions;
- features smaller than, equal to, and larger than tolerance; and
- repeated Boolean chains with accumulated precision.

### 10.4 Metamorphic laws

Where applicable, test:

- commutativity of union, intersection, and symmetric difference;
- operand-remapped equivalence of directed differences;
- idempotence;
- `A - A` is empty;
- identity with empty;
- absorption;
- consistent results under facet-ring rotation;
- vertex, facet, component, and shell permutation;
- legal source subdivision and re-triangulation;
- axis permutations;
- sign flips with corrected orientation;
- exactly representable translations and power-of-two scales;
- thread-count and work-partition invariance; and
- repeated execution byte stability.

When tolerance permits multiple geometrically plausible triangulations, compare canonical topology, occupied-side evidence, precision bounds, and tolerance usage rather than requiring identical non-authoritative internal diagonals unless deterministic output policy requires them.

### 10.5 Valid manifold generators

Random testing must generate valid closed manifolds rather than relying only on corrupt index soups. Required generator families include:

- boxes and orthogonal extrusions;
- convex polytopes;
- star-shaped radial solids;
- polygonal extrusions with holes;
- nested shell trees;
- disconnected multi-solid operands;
- topology-preserving subdivisions;
- vertex perturbations by selected ULP counts; and
- controlled near-contact and near-parallel pairs.

Expected relations are generated from exact rational templates whenever possible before conversion to representable source coordinates.

### 10.6 Invalid input fuzzing

Target:

- open boundaries;
- repeated directed edges;
- non-manifold edges;
- bow-tie vertices;
- inconsistent winding;
- malformed nesting;
- non-planar facets;
- self-crossing facet rings;
- non-finite coordinates;
- out-of-range indices; and
- severe self-intersections.

The engine must reject them precisely without undefined behavior, leaked partial artifacts, or nondeterministic error classification.

### 10.7 End-to-end qualification corpus

A production release requires a committed, versioned corpus containing:

- all minimized regressions;
- representative CAD-like mechanical parts;
- nested and disconnected solids;
- repeated CSG chains;
- difficult coplanar and tangent cases;
- extreme-scale cases; and
- tolerance-boundary cases.

Each record stores source coordinate bits, indices, operation, options, expected success or typed failure, output topology digest, and geometry-check expectations.

### 10.8 Success criteria

Before production release:

- every successful result re-ingests and passes independent topology verification;
- no valid test expects `internal_invariant_error`;
- all low-complexity exact-oracle relations agree;
- no broad-phase false negative is found by exhaustive bounded tests;
- no mutation fixture survives the verifier intended to catch it;
- all output precision and displacement bounds are conservative in oracle tests;
- sanitizers and supported compiler/platform matrices pass;
- deterministic replay succeeds across thread counts; and
- performance targets are met without disabling verification or degeneracy coverage.

## 11. Implementation order and gates

Implementation proceeds sequentially. A later component does not enter production integration until the previous artifact, verifier, and required tests pass.

1. Component 1: contract, IDs, errors, resources, transactions, and replay.
2. Component 3: precision model and bounded arithmetic, together with the exact test oracle.
3. Component 2: input topology, shell semantics, and independent validation.
4. Component 4: source polygon triangulation and provenance.
5. Component 5: canonical halfedge topology.
6. Component 6: broad phase, gated against exhaustive enumeration.
7. Component 7: relation kernel and frozen symbolic perturbation matrix.
8. Component 8: canonical event registry and shared construction lineage.
9. Component 9: connectivity and winding classification.
10. Component 10: Boolean selection and topological occurrence accounting.
11. Component 11: paired output edges and polygonal face cycles.
12. Component 12: degeneracy-tolerant triangulation.
13. Component 13: budgeted cleanup and simplification.
14. Component 14: output assembly and canonical serialization.
15. Component 15: full independent final verification and replay diagnostics.
16. Component 17: performance optimization and deterministic concurrency.
17. Component 16: full qualification matrix and release report.

Verification and tests are implemented alongside every stage, not deferred to steps 15 and 17.

## 12. Release definition of done

The engine is ready for production use only when:

- the public topology and tolerance contracts are frozen and documented;
- all input-domain cases have explicit success or typed-failure semantics;
- every component artifact has an independent verifier;
- every contract clause maps to executable tests;
- the complete contact, degeneracy, operation, operand-order, type, and tolerance matrices pass;
- output topology is guaranteed by construction and independently verified;
- every ordinary success passes tolerance-aware geometric validation;
- no operation silently welds coordinate-near but unrelated features;
- point-touching and edge-touching solids retain separate topological occurrences;
- repeated Boolean chains propagate precision correctly;
- all fuzz failures are deterministic, minimized, and retained permanently;
- sanitizer, compiler, architecture, debug-library, and thread-count matrices pass;
- controlled benchmarks demonstrate acceptable scaling and memory use; and
- the implementation and normative tests have no external dependencies.

## 13. Design cautions

- A user tolerance must never become a universal `abs(a-b) < tolerance` rule.
- A manifold connectivity guarantee alone is insufficient for ordinary success; geometric checks remain mandatory.
- Conversely, geometric proximity must never override correct indexed topology.
- Symbolic perturbation must be operation-aware, versioned, and exhaustively tested.
- Coplanar overlap, duplicate coordinates, and touching solids are normal cases, not exceptional cleanup paths.
- A triangulator and a degenerate-removal system are core correctness components, not post-processing utilities.
- Independent verification must not repeat the producer's control flow.
- Performance work begins only after exhaustive small-case oracles and mutation tests are effective.

## 14. Design lineage

This architecture is inspired by the Manifold approach of exact indexed topology, bounded floating geometry, symbolic perturbation, shared computational lineage, winding-based selection, manifold-by-construction edge pairing, and tolerance-aware degenerate cleanup. It deliberately strengthens the publication contract by requiring explicit precision propagation, tolerance budgets, typed geometric failures, and mandatory a posteriori geometry verification.
