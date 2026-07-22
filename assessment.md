# Assessment of the Surface Mesh Boolean Engine in PR 89

**Reviewed branch:** `boolean_symbolic`  
**Reviewed head:** `b2f084a03f9eea860bf4bc377685351e1ca17908`  
**Assessment date:** 2026-07-18

## Executive conclusion

The proposed engine has an unusually careful and intellectually strong design. Its central principles are correct: make geometric decisions exactly, keep topology separate from rounded coordinates, treat difficult contacts as normal cases, make results deterministic, verify each stage, and return a typed failure rather than silently publishing a plausible but wrong mesh.

However, the implementation is **not ready to be the sole Boolean backend for a production CAD engine**. The main problem is not one isolated bug. It is the combination of:

1. a very large, newly written computational-geometry stack;
2. an input contract that excludes many meshes encountered in practice;
3. a public output policy that rejects many ordinary intersections because their exact coordinates cannot be represented by `float` or `double`;
4. limited end-to-end evidence, with a qualification corpus dominated by boxes and only a handful of permanent cases; and
5. an incomplete final qualification workstream.

The likely behavior on real-world data is therefore mixed:

- The engine is deliberately designed to **fail safely** instead of returning a known-wrong answer. That is a major strength.
- It is not yet demonstrated that every reported success is correct over a broad real-world corpus.
- A large fraction of valid CAD-like operations can be expected to return `output_not_representable` under the only implemented realization semantics.
- Unknown-provenance meshes will often be rejected before the Boolean because no import-healing or normalization layer is provided.

## 1. High-level overview

### What the engine is intended to do

The engine takes two closed surface meshes that each describe a solid and computes one of five operations:

- union;
- intersection;
- subtraction of either operand from the other; or
- symmetric difference, which keeps the parts belonging to exactly one operand.

It uses regularized solid operations. In plain language, isolated points and isolated lines are not treated as solid output. The result is the boundary of the three-dimensional material that remains.

The intended public result is a closed, outward-facing triangle mesh. The engine can also discover that the mathematically correct boundary is not representable by that mesh type and return a specific error instead.

### How it works

The broad design is described in [broad_plan.md](broad_plan.md). In simplified terms, the engine follows these stages.

1. **Freeze the rules for the operation.** It records which Boolean operation is requested, how shells and cavities are interpreted, how much work is allowed, how results are ordered, and which verification rules apply.

2. **Validate both input solids.** It checks finite coordinates, face planarity, valid face loops, closed edges, consistent orientation, valid shell nesting, and the absence of self-intersections. It also assigns stable identities to input vertices, edges, and faces.

3. **Find pairs of faces that might meet.** A spatial search removes obviously unrelated pairs while promising not to omit a genuine interaction.

4. **Compute all intersections exactly.** Candidate face pairs are examined using exact arithmetic. The engine records crossings, contacts, and overlaps symbolically rather than immediately rounding new points to `float` or `double`.

5. **Merge duplicate discoveries.** The same geometric point or line can be discovered through several pairs of features. The engine gives each exact geometric object one canonical identity while keeping separate surface occurrences where the topology requires them.

6. **Split each affected input face.** Intersection lines and points are inserted into the original faces, producing small face pieces whose interiors do not cross an intersection.

7. **Join the face pieces into one global structure.** Matching edges and contacts are stitched together. Special records describe what happens around difficult vertices and coincident surfaces.

8. **Determine what is inside each solid.** The engine takes an exact sample on each side of each small face piece and determines whether that side lies inside operand A and operand B.

9. **Apply the Boolean truth table.** A face piece is kept when one side belongs to the result and the other side does not. It is oriented so that the result's interior is on the correct side.

10. **Check whether the selected boundary can be published.** Empty output is allowed. A valid boundary that is not a closed two-manifold is rejected by the current public output policy.

11. **Convert exact points to output coordinates.** Under the only implemented public semantics, every exact selected coordinate must be represented *exactly* by the output floating-point type. If even one required coordinate is not exactly representable, the operation returns `output_not_representable`.

12. **Build and verify the output mesh.** The result is triangulated, canonically ordered, and checked for closure, orientation, embedding, index capacity, and consistency with the selected exact boundary.

### Core features supported by the design

The design and code attempt to support:

- closed, oriented solids with multiple shells and cavities;
- polygonal input faces, provided that each face is exactly planar and simple;
- all five common regularized Boolean operations;
- proper intersections, touching, containment, equal operands, coplanar overlap, and coincident features;
- exact decisions based on the input floating-point bit patterns;
- deterministic output and deterministic error reports;
- typed failures for invalid input, resource exhaustion, unsupported result topology, index overflow, and unrepresentable output geometry;
- component-level verification, mutation tests, replay data, and resource accounting; and
- `float` or `double` coordinates with 32-bit or 64-bit indices.

The design also correctly recognizes two cases that many mesh Boolean implementations mishandle:

- Two valid solids can produce a mathematically valid boundary that is not a manifold mesh, such as cubes touching only at a vertex or edge.
- Exact intersections of binary floating-point inputs need not themselves be binary floating-point values. The one-third fixture in [tests/MeshBooleanPlanGapCases.md](tests/MeshBooleanPlanGapCases.md) is a clear example.

## 2. Assessment of overall quality

### 2.1 Quality of the plan

The plan is strong as a research-grade correctness specification. The following choices should be retained in any replacement or revised implementation.

**Exact decisions control topology.** The plan does not use an arbitrary epsilon to decide whether things intersect, coincide, or change order. Fast floating-point tests are permitted only when they can certify their result and otherwise fall back to exact arithmetic.

**Topology is decided before coordinates are rounded.** This prevents a rounded intersection point from changing connectivity or causing two distinct exact points to be welded accidentally.

**Geometric equality is not confused with topological identity.** A single location in space can belong to multiple disconnected surface sheets. The explicit occurrence and link model is a sophisticated and necessary correction to a common mistake.

**Difficult contacts are part of the contract.** Coplanar overlap, tangency, vertex and edge contacts, equal operands, and non-manifold results are not dismissed as undefined behavior.

**Failures are explicit.** `result_topology_not_supported` and `output_not_representable` are much more honest than returning a damaged mesh.

**Stages are transactional and verified.** A downstream stage consumes a frozen artifact rather than a half-mutated data structure. The producer/verifier separation and deterministic replay goals are valuable.

These principles align closely with established exact geometry systems. In particular, the plan's exact internal set, local vertex neighborhoods, global incidence structure, regularization, and manifold-conversion gate strongly resemble the problem already addressed by exact Nef polyhedra.

### 2.2 Quality of the implementation

The implementation shows substantial engineering effort. It includes strong IDs, immutable artifacts, deterministic ordering, resource limits, exact arithmetic, filtered predicates, conservative spatial searches, explicit error provenance, bounded worker execution, and numerous component tests. The code is clearly trying to encode the invariants described by the documents rather than merely imitate their vocabulary.

That said, confidence in a geometry engine cannot be inferred from architecture, line count, or the number of test executables. PR 89 adds roughly 42,000 lines across 143 files and implements an exact number system, geometric predicates, planar arrangements, three-dimensional stitching, local spherical topology, point classification, output realization, triangulation, and independent verification. Each of those is a difficult geometry project in its own right. Their composition creates a very large surface for subtle common-mode errors.

The PR history already contains a useful warning. A Component 8 patch-interior witness originally used an incomplete centroid heuristic that failed on an annular patch, while the verifier also failed to exclude hole interiors. The fix replaced the heuristic with exact vertical decomposition and strengthened the verifier. That is the right correction, but the incident demonstrates how a producer and supposedly independent verifier can share the same blind spot. The current head also contains a subtle determinism fix involving argument evaluation and moved state. These are not reasons to discard the work, but they are evidence that source inspection and synthetic unit tests are not yet enough to call the implementation mature.

### 2.3 Testing and qualification evidence

The component test strategy described in [component_14_testing.md](component_14_testing.md) is excellent. The implemented release evidence falls well short of that specification.

Important observations include:

- [tracker.md](tracker.md) leaves P13, the final qualification and frozen performance gate, incomplete.
- The permanent corpus manifest contains only six entries: disjoint boxes, equal boxes, replay encoding, vertex-touching boxes, edge-touching boxes, and the one-third failure case.
- The degeneracy inventory contains only seven rows. It does not demonstrate the broad matrix promised by Component 14, including arbitrary concave shapes, multiple holes, high-valence contacts, extensive coplanar overlap, nested cavities, radically different subdivisions, and extreme-coordinate combinations.
- [tests/Test_MeshesBooleanEndToEnd.cc](tests/Test_MeshesBooleanEndToEnd.cc) primarily covers disjoint and equal boxes.
- [tests/Test_MeshesBooleanFuzz.cc](tests/Test_MeshesBooleanFuzz.cc) generates only disjoint axis-aligned boxes. The default continuous tier runs eight generated cases; even the qualification tier requests only 128 of this narrow case family.
- [tests/Test_MeshesBooleanMetamorphic.cc](tests/Test_MeshesBooleanMetamorphic.cc) is also box-based.
- [tests/MeshBooleanQualification.cc](tests/MeshBooleanQualification.cc) mainly verifies that inventory files are non-empty and well formed. It does not itself execute the broad qualification matrix or prove that the listed coverage is adequate.
- The branch contains useful GitLab CI definitions for multiple compilers and sanitizers, but no final P13 report with exact commands, platforms, outcomes, performance measurements, corpus size, fuzz duration, or unresolved failures is committed.

There are many worthwhile component-level tests, including properties, adversarial fixtures, mutation tests, and plan-gap cases. The concern is specifically that the overall engine has not been demonstrated on a sufficiently broad set of complete operations.

### 2.4 Likely behavior on real-world inputs

It is important to separate three questions.

#### Will the engine avoid knowingly returning a wrong mesh?

The design is unusually strong on this point. Its intended answer is yes: reject uncertainty, verify the result, and fail closed. Whether every implementation path satisfies that intention remains unproven, but the contract is correct.

#### Will the engine accept typical imported meshes?

Often no. The accepted inputs must be exactly closed, edge-manifold, consistently oriented, embedded, free of self-intersections, and composed of exactly planar simple faces. The Boolean operation does not heal cracks, merge nearby vertices, repair winding, remove duplicate facets, resolve self-overlap, or reinterpret nearly coplanar faces. The broad plan says normalization should be a separate API, but this PR does not provide a production normalization pipeline.

This strict boundary is reasonable for an exact kernel, but it is not a complete application-facing solution for meshes of unknown provenance. STL, OBJ, tessellated CAD exports, scan-derived meshes, and hand-edited meshes commonly contain precisely the defects this contract rejects.

#### Will valid inputs usually produce a usable `float` or `double` mesh?

No, not under the current realization contract. Every finite binary floating-point number is a rational number whose denominator is a power of two. Intersections of planes and edges require division, so exact results commonly have other denominators. A coordinate such as `1/3` is only the simplest example.

The implementation exposes only `realization_semantics::exact_in_T`. Although a neighboring-values strategy exists in the policy type, the realization code deliberately does not search neighboring values under exact-in-`T` semantics. That is mathematically consistent, but it means many ordinary rotated, slanted, or differently scaled CAD meshes will complete the expensive exact topology stages and then fail at publication.

A typed failure is safer than a wrong mesh, but a Boolean engine that safely rejects a large share of normal operations is not a sufficient CAD engine.

### 2.5 Readiness summary

| Area | Assessment |
|---|---|
| Mathematical plan | Strong and unusually explicit |
| Error model | Strong |
| Determinism design | Strong goal; implementation still receiving fixes |
| Component engineering | Substantial and promising |
| Input compatibility | Narrow |
| Successful floating-point output coverage | Fundamentally narrow under `exact_in_T` |
| End-to-end qualification | Insufficient |
| Real-world corpus | Insufficient |
| Performance evidence | Incomplete |
| Maintainability | High risk because of size and novelty |
| Suitability as sole CAD backend | Not ready |

## 3. Major limitations

### 3.1 Exact-in-`T` is not a practical default output contract

This is the most important limitation. The plan correctly proves that exact topology and exact floating-point output cannot both be guaranteed for every input. The current implementation responds by supporting only the rare case where every selected exact coordinate is already representable by `T`.

That policy is useful for a certified mode and for testing. It is not a practical default for CAD. The engine needs at least one of the following:

1. an exact-coordinate output mesh whose points are rationals or exact constructions;
2. a separately labeled, certified approximate embedding in `float` or `double`; or
3. an application-level tolerance model that constructs and validates a nearby B-rep while explicitly abandoning exact point-set equality.

Without one of these, the engine's success rate is structurally limited, regardless of how many topology bugs are fixed.

### 3.2 The input contract requires a separate healing and normalization product

The Boolean engine intentionally does not perform tolerance-based repair. That is defensible, but a downstream CAD application still needs a robust path from its actual data to the Boolean contract.

The missing layer should address at least:

- duplicate and near-duplicate vertices;
- cracks and small gaps;
- inconsistent winding;
- duplicate or overlapping facets;
- non-planar polygon loops;
- self-intersections;
- nested shell orientation;
- tiny sliver faces and edges;
- attribute seams; and
- an application-defined model tolerance.

Normalization must not be hidden inside the exact Boolean. It should be an explicit operation with a report explaining what changed. Nonetheless, without it the PR does not directly solve the end-user's unknown-provenance workflow shown in the example.

### 3.3 The public result type supports only closed embedded manifold triangles

A regularized Boolean of valid manifold operands can have a non-manifold boundary. The implementation now detects this correctly, but the only public result policy rejects it. The exact selected structure is internal and is not a generally usable public B-rep.

This is acceptable if the downstream CAD contract also rejects such results. Otherwise, the API needs a public stratified or non-manifold B-rep type, or a documented operation that separates touching components without pretending they are one manifold surface.

### 3.4 Polygonal and attribute interoperability is incomplete

Input polygons are accepted, but output is always triangulated and the configured output policy performs no simplification. I found no complete application-level design for round-tripping:

- original polygon boundaries;
- face and body identities;
- materials;
- texture coordinates;
- vertex normals;
- sharp-edge tags;
- per-face metadata; or
- downstream CAD feature history.

Compact provenance exists as an option, and the internal code tracks extensive provenance, but a CAD integration needs a stable, documented mapping contract rather than only geometric output.

### 3.5 Exact bit semantics can overreact to modeling noise

Treating every input bit as exact is ideal for reproducibility. It does not express the intent of most CAD models, which are built under a model tolerance. Two surfaces intended to be coincident but separated by a few units in the last place become distinct exact surfaces. This can create tiny wedges, slivers, extra events, large exact numbers, or a result that differs from the user's intended model.

A practical CAD system needs both:

- an exact computational layer that never makes inconsistent decisions; and
- an explicit tolerance/normalization layer that defines which nearby models are considered equivalent.

The PR supplies the first concept but not the second.

### 3.6 Complexity and maintenance risk are very high

The engine implements its own arbitrary-precision integer and rational arithmetic, predicate filters, planar subdivision, global arrangement, local spherical topology, point location, constraint solver, triangulation, and verifier reconstructions. This makes the code self-contained, but it also gives the project sole responsibility for decades of difficult edge cases.

Independent verifiers increase confidence but also double some of the algorithmic surface. If producer and verifier are written from the same specification, by the same implementation effort, and tested with the same fixtures, they can still share a conceptual error.

For a small team or a single maintainer, long-term reliability risk is high even if the initial test suite passes.

### 3.7 Performance is not yet qualified

The plan performs exact work at many stages and directly classifies both sides of every atomic patch. Mandatory verification repeats substantial reconstruction. These are defensible correctness choices, but they can be expensive in memory and time on dense tessellations or extensive coplanar overlap.

The performance workstream contains many optimizations, but the final P13 benchmark and platform gate is incomplete. No production decision should be based on performance claims until representative CAD workloads, worst-case growth, peak memory, cancellation behavior, and verifier overhead are published.

### 3.8 The application-facing API is too burdensome

The example asks callers to instantiate an exact kernel, register ten verifier implementations, freeze the verifier service, construct a context, validate operands, and then assemble output. That is useful for internal testing but too much ceremony for the ordinary supported path.

A production API should provide a one-call default service with advanced dependency injection available separately. It should also expose validation and normalization reports in an application-oriented form.

## 4. Is the overall plan valid?

### What is valid

The following high-level model is correct:

1. preserve exact input meaning;
2. make all topology-changing decisions exactly;
3. represent constructed points symbolically or exactly;
4. construct the complete subdivided boundary;
5. classify both sides;
6. select the Boolean boundary;
7. distinguish exact set semantics from approximate coordinates;
8. reject unsupported topology explicitly; and
9. verify before publication.

This is a sound specification for a robust Boolean engine.

### What should change

The plan should not assume that the project must implement every layer itself. “Portable, C++17, and in-tree with no external dependency” is an implementation preference, not a correctness principle. For a production-critical CAD dependency, it is probably the wrong tradeoff.

The design should be reframed around three separable concerns:

1. **Input preparation:** validation, orientation, repair, tolerance handling, triangulation, and provenance capture.
2. **Authoritative Boolean backend:** an established exact or robust engine with a narrow adapter.
3. **Output realization:** exact-coordinate output, certified approximate floating output, topology policy, simplification, and application metadata.

The current Components 1, 2, 11, 12, and 13 contain ideas that remain valuable around an external backend. Most of Components 3 through 10 duplicate the central work of mature geometry libraries.

## 5. Recommended architecture for Ygor and the downstream CAD engine

### Recommended production path

1. **Define the actual downstream contract first.** Decide whether users need exact point-set semantics, a valid manifold mesh within a model tolerance, or an analytic CAD B-rep. These are different products.

2. **Add an explicit normalization API.** It should accept an application tolerance, return a repaired validated solid plus a detailed change report, and never be confused with exact Boolean evaluation.

3. **Provide at least two output modes.** One should return exact rational/construction coordinates. The other should return a floating mesh with a documented displacement bound and complete post-rounding verification. Exact-in-`T` can remain as a strict certified mode.

4. **Keep a manifold publication gate.** If `fv_surface_mesh` cannot represent the exact topology, return a dedicated result containing either the exact backend object, a stratified boundary type, or the existing typed failure.

## 6. Required improvements for the current implementation

The following are release blockers for a rock-solid CAD dependency.

### P0: product-contract blockers

1. **Implement a usable output policy.** Add exact-coordinate output or a separately typed certified approximate embedding. Do not present `exact_in_T` as the general CAD path.

2. **Provide the normalization pipeline.** Unknown-provenance input cannot be a documented use case without a supported preparation step.

3. **Publish the stratified exact result or define its lifetime.** A valid non-manifold result should not disappear behind an error if downstream code can use a richer B-rep.

4. **Define attribute and provenance behavior.** CAD integration requires more than vertex and face arrays.

### P0: qualification blockers

1. **Complete P13 and commit a reproducible report.** Include exact compiler versions, architectures, build modes, sanitizer runs, thread counts, filter modes, test tiers, seeds, fuzz durations, benchmark hardware, peak memory, failures, and performance results.

2. **Build a substantial permanent end-to-end corpus.** It should include many cases (thousands!) from generated exact polyhedra and real tessellated CAD models, not only boxes. The cases should reflect real applications, not just simple boxes.

3. **Expand full-operation degeneracy coverage.** Include rotated and skewed convex solids, concave solids, multiple cavities, nested shells, coplanar partial overlaps, repeated cuts, high-valence intersections, long thin features, severe scale differences, and alternate triangulations.

4. **Fuzz valid geometry, not only disjoint boxes.** Use topology-preserving generators and exact rational construction so expected relations are known. Run continuously under sanitizers and preserve every failure.

5. **Test chains of operations.** CAD workloads perform many Booleans in sequence. Every successful output should be re-ingested and used in further operations under varied orderings.

6. **Measure false success, not only crashes and failures.** The most dangerous outcome is a verified-looking but incorrect solid. Independent point classification, exact volume comparisons where applicable, and comparisons with external providers/projects/samples are essential.

### P1: engineering improvements

- Wrap verifier and kernel setup in a simple default service.
- Separate public headers from large internal artifact types.
- Reduce the number of policy combinations until each has a complete test matrix.
- Document complexity and default resource limits.
- Add stable serialization for exact and stratified results.
- Establish a long-term compatibility policy for replay schemas and canonical bytes.
- Document which compiler floating-point modes are supported and enforce them in consumer builds.

## Final verdict

The PR is a serious and thoughtful research implementation, not a superficial mesh Boolean. Its broad plan is valuable and many of its correctness principles should become requirements for any chosen backend.

The recommended decision is:

1. accept and retain this engine as an experimental API until more evidence can be gathered to validate it; and
2. make normalization, exact output, certified approximate output, provenance, and qualification first-class parts of the product architecture.

A production claim should be reconsidered only after the downstream application's representative corpus passes a published, reproducible, multi-backend qualification program with an acceptable success rate, no unexplained disagreements, and measured performance and memory bounds.
