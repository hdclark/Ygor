# Robust B-rep Boolean Engine: Broad Plan

## 0. Overall goal

Build a portable C++17 engine for regularized union, intersection, difference, and symmetric difference of closed, orientable polygonal B-rep solids represented by `fv_surface_mesh<T, I>`. The engine must derive every topological decision from exact predicates or exact constructions, assign stable identities to all constructed entities, and either return a certified manifold boundary of the mathematically defined result or return a precise failure. It must never guess, repair an ambiguous result heuristically, or silently emit a plausible but incorrect mesh.

"Exact result" has three distinct meanings:

- **Exact set semantics:** the selected boundary is the boundary of the requested regularized set operation.
- **Exact combinatorics:** incidence, ordering, coincidence, side labels, and orientation agree with the exact arrangement induced by the binary floating-point inputs.
- **Represented geometry:** every output coordinate is a `T` value and the represented facets realize those exact combinatorics without inversions, collapses, new intersections, or lost intersections.

The first two guarantees are achievable by treating each finite input float as an exact dyadic rational and retaining exact constructed geometry internally. The third is not achievable for every possible input when output is restricted to `vec3<T>`: an intersection of representable inputs need not be representable in `T`, and two distinct exact intersections can round to the same `T` point. No implementation can promise a valid `fv_surface_mesh<T, I>` in every such case. Therefore the public contract must expose a checked result:

```cpp
boolean_result<T, I> boolean_operation(...);
// success: certified fv_surface_mesh<T, I>
// failure: input_contract_error, resource_limit, or output_not_representable
```

Internally, an exact/symbolic result remains available so that a future exact-coordinate output type can avoid the last failure class. Returning a wrong `T` mesh is never an allowed fallback.

## 1. Domain and semantics

The phrase "valid B-rep" must mean more than index-manifoldness. The accepted domain is defined and checked by Components 1 and 2:

- Coordinates are finite IEC 60559 binary floating-point values supported by the exact conversion layer; no NaN or infinity.
- Every facet has at least three distinct consecutive vertices, is exactly planar, and has a simple, non-zero-area boundary in that plane.
- The mesh is closed and edge-manifold: every undirected edge has exactly two incident facet uses with opposite directions.
- Vertex links are single cycles per shell; touching only at a vertex or edge does not masquerade as one manifold shell.
- Each input boundary is embedded: non-adjacent facets of the same operand do not cross or overlap. Allowed contacts and duplicate shells must be assigned explicit semantics, not inferred opportunistically.
- Facet orientation is consistent. The API specifies whether orientation defines occupied side or whether disconnected shells are interpreted with an explicit shell policy. The default solid contract uses outward boundary orientation and supports cavities through nesting/winding.
- The represented solid is regular closed. Boolean operations are regularized: `closure(interior(A op B))`. Isolated points and curves are not emitted as B-rep boundary.

If callers want a broader notion of input, a separate normalization API must convert it to this contract and prove the conversion. Boolean evaluation itself does not perform tolerance-based healing.

## 2. Guiding principles

1. **Correctness before performance.** Optimize only after invariants and differential tests survive the change.
2. **Exact decisions.** Floating-point filters may accelerate a predicate, but an uncertain filter always falls back to exact arithmetic. No epsilon, snapping tolerance, or float equality controls topology.
3. **Topology first.** Discover symbolic events, establish exact order and incidence, build arrangements, classify cells, and only then realize coordinates.
4. **One identity per mathematical entity.** Original features and constructed intersections have canonical, stable keys. Coincident derivations are interned, not independently rounded.
5. **Degeneracies are ordinary cases.** Coplanar overlap, coincident edges/faces, vertex-on-edge/face, tangency, containment, equal operands, and empty/full results have specified outcomes.
6. **Determinism.** Results do not depend on pointer values, hash iteration order, thread scheduling, BVH shape, predicate filter path, or input facet traversal order. Canonical ordering resolves all ties.
7. **Conservative acceleration.** Broad-phase code may add false candidates but may never remove a true candidate.
8. **Explicit ownership.** Every entity has one owner and immutable provenance. Components exchange value objects or stable handles under documented lifetime rules.
9. **Transactional stages.** A stage either publishes a verified artifact or returns an error; downstream stages never observe partial mutation.
10. **No heuristic cleanup.** Cleanup may remove only structures proven redundant under exact semantics.
11. **Fail closed.** Contract violations, arithmetic/resource limits, index overflow, or unrepresentable output are errors, never approximate success.
12. **Continuous verification.** Cheap invariants run in normal builds; exhaustive stage checks, certificates, and trace capture are available in verification builds.
13. **Portable implementation.** All code is C++17 and in-tree, including arbitrary-precision integer/rational arithmetic and adaptive predicates. No external dependency is part of the design.
14. **Ignore existing YgorMeshBoolean implementations.** The existing files `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}` are known to be insufficient and will be removed in the future. Do not use them.
15. **Use existing Ygor logging and exceptions.** It is OK to use exceptions to communicate failures, broken invariants, broken assumptions, etc. Encourage use of the `YLOGWARN`/`YLOGINFO`/`YLOGDEBUG` mechanism to log information as needed.

## 3. Components

| No. | File | Responsibility | Principal artifact |
|---:|---|---|---|
| 1 | `component_01_contract_context.md` | Operation semantics, IDs, errors, limits, deterministic context | `boolean_context`, operation contract |
| 2 | `component_02_input_topology.md` | Validate and canonicalize each input B-rep | `validated_operand` |
| 3 | `component_03_exact_kernel.md` | Exact numbers, constructions, predicates, symbolic perturbation | `exact_kernel` |
| 4 | `component_04_broad_phase.md` | Conservatively enumerate potentially interacting features | canonical candidate stream |
| 5 | `component_05_intersection_events.md` | Resolve feature pairs into complete symbolic intersection events | raw event set |
| 6 | `component_06_symbolic_registry.md` | Intern equivalent events and establish exact order/incidence | canonical symbolic complex |
| 7 | `component_07_local_refinement.md` | Build exact constrained arrangements on input facets | refined facet patches |
| 8 | `component_08_global_arrangement.md` | Stitch patches into a globally consistent oriented arrangement | arrangement complex |
| 9 | `component_09_cell_classification.md` | Label both sides/cells by exact operand occupancy | labeled arrangement |
| 10 | `component_10_boolean_selection.md` | Apply the Boolean truth table and select oriented boundary patches | selected exact boundary |
| 11 | `component_11_geometry_realization.md` | Realize each symbolic vertex once and certify conversion to `T` | realized vertices or failure |
| 12 | `component_12_output_assembly.md` | Assemble, simplify safely, orient, and canonically serialize output | `fv_surface_mesh<T, I>` |
| 13 | `component_13_verification.md` | Verify stage invariants and produce diagnostic certificates | verification reports |
| 14 | `component_14_testing.md` | Unit, property, metamorphic, adversarial, and end-to-end testing | reproducible test corpus |

Components 1, 3, 13, and 14 are infrastructure used throughout. Verification is shown late in the table only because it verifies every artifact; it must be implemented early.

## 4. Runtime order

1. Freeze a `boolean_context`: operation, operand roles, shell semantics, deterministic policies, resource limits, and tracing.
2. Convert each input coordinate exactly and validate/canonicalize each operand. Assign stable original-feature IDs.
3. Build conservative acceleration structures and generate a canonical stream of cross-operand feature candidates. Self-candidates are also generated where required by input validation.
4. Evaluate candidates with exact predicates and constructions. Emit all proper, touching, and overlap events with provenance; do not emit Cartesian `T` coordinates.
5. Intern mathematically identical events and derive exact total orders along edges and within overlap carriers.
6. Construct a constrained planar arrangement on every affected source facet. Subdivide source edges and facet interiors using symbolic vertices and curves.
7. Stitch local patches through canonical edge/event identities into a global arrangement. Verify twin, cycle, and orientation invariants.
8. Classify arrangement cells, or equivalently both sides of every patch, as inside/outside each operand. Seed classification with exact point location and propagate labels through oriented surfaces.
9. Evaluate the requested regularized Boolean truth table. Keep exactly those patches separating selected and unselected volume, oriented with result interior on the prescribed side.
10. Realize exact symbolic coordinates once per canonical vertex. Find `T` representatives and certify all signs, orders, incidences, and non-incidences needed by the selected boundary. Return `output_not_representable` if no certified representation is found under the API policy.
11. Assemble indexed facets, perform only exact-proof simplifications, canonicalize output ordering, and check index capacity.
12. Run final topology, geometry, orientation, set-membership, determinism, and serialization checks. Publish only after all mandatory checks pass.

An empty selected boundary is a successful representation of the empty regularized solid. A "whole universe" result is outside the finite closed-solid API and is prevented by the supported operation/domain contract.

## 5. Implementation order

Runtime order is not development order. Build and gate components as follows:

1. Component 1 contracts, immutable IDs, diagnostics, and deterministic utilities.
2. Component 3 exact arithmetic and predicates, alongside Component 14 arithmetic/predicate oracles.
3. Component 13 artifact verifiers and Component 2 topology validation.
4. Component 4 broad phase, tested against exhaustive pair enumeration.
5. Components 5 and 6 symbolic events and registry.
6. Component 7 local arrangements, first on synthetic exact constraints and then on discovered events.
7. Component 8 global stitching and manifold/arrangement checks.
8. Component 9 classification, then Component 10 selection.
9. Component 11 checked realization and impossibility reporting.
10. Component 12 output assembly and canonicalization.
11. Full Component 14 adversarial, randomized, metamorphic, replay, and performance suites.

No later component begins production integration until the previous artifact's contract tests and verifier are passing.

## 6. Global invariants

- Every handle refers to exactly one immutable entity in its owning context.
- Every topological comparison is explained by an exact predicate result plus a deterministic tie rule.
- Every constructed vertex has exact provenance and one canonical registry entry.
- All event lists are strictly ordered by exact parameter; equal parameters are merged by proof.
- Local refinements cover each source facet exactly, without gaps or positive-area overlap.
- Global halfedges have valid twins; local cycles close; patch orientations agree with source provenance.
- Each arrangement patch has well-defined occupancy labels on both sides for both operands.
- Selected patches are exactly those across which the Boolean result occupancy changes.
- The selected exact boundary is closed and orientable before coordinate rounding.
- Realization preserves every predicate sign in the stage's explicit certificate set.
- The final indexed mesh is closed, embedded, consistently outward-oriented, and has no duplicate positive-area facets.
- Re-running with different thread counts, BVH construction choices, or enabled filters yields byte-identical canonical output and equivalent diagnostics.

## 7. Failure model

Expected failures are typed and carry stage, operand/feature IDs, predicate/event provenance, and a replay token:

- `input_contract_error`: malformed topology, non-finite coordinate, non-planar/simple facet, self-intersection, invalid orientation/solid semantics.
- `unsupported_platform`: `T` is not supported by exact bit-level conversion or required integer assumptions are false.
- `resource_limit`: caller-declared memory, exact-number size, event count, or work limit reached. Unlimited correctness mode has no artificial arithmetic cap.
- `index_overflow`: result cannot be indexed by `I`.
- `output_not_representable`: exact result exists, but no emitted `T` realization has been certified under the chosen realization policy.
- `internal_invariant_error`: implementation defect; includes a minimized/replayable trace whenever possible.

These are not alternate geometric answers. In particular, epsilon perturbation, dropping small facets, and returning a non-manifold mesh are prohibited recovery strategies.

## 8. Release definition of done

- All component contracts and invariants have executable checks.
- Exact arithmetic and predicate suites cover all signs, overflow paths, and filter/fallback equivalence.
- Exhaustive broad-phase comparison proves no missed candidates over the test corpus.
- Every documented degeneracy has focused tests for all operations and operand orders.
- Property and metamorphic tests validate identities such as commutativity where applicable, idempotence, absorption, difference identities, rigid-transform invariance, and subdivision invariance.
- Independent exact low-complexity oracles agree on classification and selected boundaries.
- Fuzz failures are automatically serialized and deterministic under replay.
- Fault injection demonstrates that violated invariants stop publication.
- Sanitizers, debug iterators where available, multiple compilers/architectures, and deterministic parallel schedules pass.
- Benchmarks establish performance without weakening any exact path or certificate.
- Public documentation states domain, regularization, orientation, determinism, error behavior, complexity limits, and the finite-`T` representability limitation.
