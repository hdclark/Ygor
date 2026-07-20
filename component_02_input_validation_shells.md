# Component 02: Input Topology Validation and Shell Semantics

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The implementation strategy may change, but the observable contracts, failure behavior, invariants, and test obligations in this document are normative.

## 0. Purpose

This component converts each caller-supplied `fv_surface_mesh<T,I>` operand into a validated, canonically identified description of one or more closed oriented polyhedral shells. It determines whether the facet-vertex topology and declared geometric uncertainty are compatible with the engine's solid model, and it establishes occupied-side semantics for disconnected shells, cavities, and nested islands.

The component is intentionally a validator and semantic interpreter, not a general mesh-repair system. It may perform only explicitly contract-preserving canonicalization such as removing consecutive duplicate indices from a facet ring or rotating a ring to a canonical start. It must not silently weld nearby vertices, close cracks, split unknown self-intersections, infer missing faces, or change shell orientation without recording and validating the operation under an explicitly selected normalization policy.

Its principal output is a pair of immutable `validated_operand` artifacts consumed by Component 04 and later stages.

## 1. Input contract

### 1.1 Required inputs

For each operand, the component must accept:

- an immutable `fv_surface_mesh<T,I>`;
- the immutable Boolean context from Component 01;
- the operand's declared input precision from the frozen options;
- the selected solid and shell policy;
- the requested input-geometry verification level;
- resource and cancellation services from Component 01; and
- bounded arithmetic and enclosure services from Component 03.

The mesh representation must expose, directly or through an adapter:

- an ordered coordinate array;
- a sequence of facets;
- an ordered index ring for every facet;
- the concrete types `T` and `I`; and
- optional source metadata that can be carried as opaque provenance but must not affect topology unless explicitly specified.

### 1.2 Accepted input domain

The default accepted operand must satisfy all of the following:

1. every coordinate is finite;
2. every index is in range;
3. after removal of consecutive duplicate indices and a duplicate closing index, every facet has at least three distinct topological vertices;
4. every facet ring is simple under the facet's bounded planar model;
5. every facet is planar within the operand's input-precision contract;
6. every undirected topological edge has exactly two directed uses with opposite orientation;
7. the incident facet fan around each topological vertex occurrence forms exactly one closed cycle;
8. every connected shell has consistent orientation;
9. the shell collection has determinable nesting and occupied-side semantics under the selected policy;
10. away from boundaries, the total oriented winding represents a regular solid, normally zero or one;
11. duplicate coordinates are permitted and remain distinct when their indices are distinct;
12. multiple disconnected solids, cavities, islands, and nested shell trees are permitted; and
13. when mandatory geometric validation is enabled, the operand is epsilon-valid within its declared input precision.

The component must reject unsupported index layouts, malformed rings, open or non-manifold topology, ambiguous shell semantics, and geometry that cannot satisfy the requested input-validity contract.

### 1.3 Canonicalization permitted by this component

The following transformations may be performed because they do not alter indexed surface semantics:

- remove consecutive repetitions of the same vertex index in one ring;
- remove a final ring index equal to the first;
- rotate a ring to a canonical start while preserving orientation;
- canonicalize ordering of disconnected shells and source features;
- create strongly typed source identities and incidence records; and
- normalize equivalent metadata encodings.

The following are prohibited in the default validator:

- merging distinct vertex indices because coordinates are close or equal;
- moving coordinates;
- filling boundaries;
- deleting non-degenerate facets;
- splitting bow-tie vertices or non-manifold edges;
- choosing an orientation for an otherwise ambiguous shell based only on bounding-box volume or a tolerance heuristic; and
- resolving self-intersection by arbitrary local edits.

A separate future repair API may precede the Boolean engine, but repaired output must re-enter through this same validation contract.

## 2. Required behavior

### 2.1 Structural and scalar validation

The component must validate all array and scalar preconditions before building derived topology:

- coordinate count, facet count, ring lengths, and total index count fit resource and index limits;
- all indices convert safely to internal index types;
- no coordinate is NaN or infinite;
- declared input precision is finite, non-negative, and at least the applicable machine floor;
- empty operands have explicitly defined semantics and contain no malformed residual data; and
- arithmetic used to size incidence structures is overflow-safe.

Failures must identify the operand and canonical source location where possible.

### 2.2 Facet-ring validation

For each facet, the component must:

- canonicalize only permitted duplicate-index forms;
- reject fewer than three distinct topological vertices;
- reject repeated non-consecutive vertex indices unless the selected facet representation explicitly supports multiple contours, which v1 does not require;
- derive a conservative facet plane or bounded planar support using Component 03;
- verify every ring vertex lies within the accepted planar enclosure;
- select a deterministic projection frame;
- verify the projected ring is simple and has non-zero accepted orientation;
- distinguish geometric coordinate coincidence from topological repetition; and
- record source ring orientation and provenance.

An uncertain simplicity, orientation, or planarity decision must not be converted into acceptance by comparing to the global tolerance as a universal epsilon. It must use the bounded predicate contract and return a typed geometric-input failure if uncertainty exceeds the accepted input precision.

### 2.3 Edge incidence construction

The component must create canonical directed-edge-use and undirected-edge records from source indices. Pairing must be based on endpoint identities, not coordinates.

For every undirected source edge, it must verify:

- exactly two directed uses exist;
- the two uses have opposite directions;
- each use belongs to a valid facet ring position;
- no overflow or duplicate record is hidden by canonical sorting; and
- the pair is stable under facet and ring permutations.

An edge with one use is an open boundary. More than two uses is a non-manifold edge. Two same-direction uses indicate inconsistent orientation. Each condition requires a distinct diagnostic subcode.

### 2.4 Vertex-link validation

For each source vertex identity, the component must reconstruct the link graph from incident directed edge uses and facets. A valid closed two-manifold vertex occurrence must have one cyclic fan.

The validator must reject:

- multiple disjoint fans sharing one source vertex index;
- a fan with an open end;
- repeated incidence that prevents a simple cycle;
- an edge pair that is locally reciprocal but participates in a bow-tie vertex; and
- a source vertex whose incidence cannot be traversed deterministically because of corrupt topology.

Distinct indices with identical coordinates are validated independently and must not be joined.

### 2.5 Connected shell discovery

The component must identify connected shell components using topological adjacency through paired edges. It must assign stable shell IDs independent of input facet order.

For each shell it must record:

- member source vertices, edges, and facets;
- canonical representative features;
- a conservative bounding enclosure;
- signed-orientation evidence;
- component-level resource statistics;
- potential geometric contacts or overlaps with other shells; and
- a stable canonical digest.

Topological connectivity must not be inferred from coordinate contacts between separate shells.

### 2.6 Shell orientation

The component must establish that every shell has consistent local orientation from edge pairing. It must then determine the geometric side associated with that orientation using bounded volume, side-probe, or equivalent evidence that is independent of traversal order.

The method may vary, but it must:

- use conservative bounded computations;
- avoid relying solely on a numerically unstable signed-volume sum;
- detect when orientation evidence is indeterminate within input precision;
- produce witnesses identifying representative faces and probes;
- distinguish a reversed but internally consistent shell from a locally inconsistent shell; and
- follow the selected solid policy rather than silently reversing data.

If the policy permits canonical reorientation, the operation must be explicit, global to the shell, deterministic, and reflected in provenance. Under the default strict policy, wrong orientation is rejected.

### 2.7 Shell nesting and occupied-side semantics

The component must construct a deterministic containment/nesting relation among disconnected shells while accounting for precision envelopes and boundary contact.

The shell-semantic model must support:

- several disjoint outer solids;
- cavities inside solids;
- islands inside cavities;
- deeper alternating nesting;
- coincident but topologically separate shells when the policy defines them; and
- point- or edge-touching shells that remain topologically disconnected.

For each shell, the component must determine a parent shell or exterior-root status and a nesting depth. Under the default alternating-boundary policy:

- even-depth outward-oriented shells add occupied volume;
- odd-depth oppositely oriented shells bound cavities;
- depth alternates occupancy; and
- the total oriented winding away from boundaries must be zero or one.

Containment tests must use canonical probes and bounded uncertainty. A probe whose enclosure overlaps another boundary cannot be treated as a definite inside/outside result. The component must seek another certified probe or return `ambiguous_shell_semantics`.

Boundary contact between shells must be classified according to the solid/contact policy. The validator must not create parent-child nesting solely from touching at a point or edge.

### 2.8 Epsilon-valid geometry assessment

When required by the verification policy, the component must assess whether there exists a perturbation of every input vertex within its declared precision envelope that yields an embedded oriented manifold representing the same intended solid.

A complete mathematical decision for arbitrary input is not required, but the production contract must include conservative sufficient checks and typed rejection. At minimum, assessment must search for:

- non-adjacent triangle intersections not explainable by authorized shell contact;
- facets whose planar uncertainty is too large;
- collapsed or inverted local geometry;
- source edges whose incident facets become geometrically inconsistent;
- shell overlaps that make winding semantics exceed the supported regular-solid model;
- unresolved contacts whose uncertainty envelope spans incompatible topology; and
- gaps or overlaps larger than or contradictory to declared precision.

The component may reject difficult but potentially valid geometry. It must not accept a known contradiction merely because the indexed topology is manifold.

A diagnostic topology-only attempt may bypass some geometric checks only when the context explicitly requests it; such an artifact cannot by itself lead to ordinary success.

### 2.9 Canonical feature identities

The component must assign canonical source identities without depending on input array order. Canonicalization may use topology, exact coordinate bit patterns, ring sequences, shell structure, and stable refinement. Symmetric inputs may require deterministic tie keys based on complete canonical encodings rather than pointer or traversal order.

The artifact must preserve a reversible mapping between caller indices/facet positions and canonical source IDs for diagnostics and provenance.

### 2.10 Independent validation evidence

The component must produce enough evidence for Component 15 to independently reconstruct validation, including:

- normalized facet rings;
- directed and undirected edge incidence;
- vertex-link cycles;
- shell membership;
- shell orientation witnesses;
- nesting probes and bounded classifications;
- input-geometry findings;
- canonicalization maps; and
- precision values used in every uncertain geometric decision.

It must not provide only producer-owned booleans such as `is_manifold=true` without reconstructible support.

## 3. Output contract

For each accepted operand, the component must produce one immutable `validated_operand<T,I>` artifact containing at least:

- operand identity and source digest;
- immutable access to original coordinate bit patterns;
- normalized source vertex records;
- normalized facet rings with canonical facet and ring IDs;
- paired directed-edge-use and undirected-edge records;
- vertex-to-incidence and validated cyclic-link records;
- disconnected shell records and membership maps;
- shell orientation and occupied-side semantics;
- a deterministic shell nesting forest;
- canonical source feature order and caller-to-canonical mappings;
- conservative per-feature and per-shell bounds;
- declared and derived input precision metadata;
- epsilon-validity status and supporting findings;
- provenance required by Components 04, 05, 14, and 15; and
- a canonical digest.

The artifact must guarantee:

- every facet ring is structurally valid and bounded-planar;
- every undirected source edge has exactly two opposite directed uses;
- every source vertex occurrence has one cyclic fan;
- every shell is internally consistently oriented;
- shell semantics are unambiguous under the selected policy;
- duplicate-coordinate source identities remain separate unless the caller used the same index; and
- no mutable caller storage is referenced unless the public lifetime contract explicitly permits immutable borrowing.

On failure, no `validated_operand` is published. The error must identify the earliest canonical contract violation under Component 01's deterministic arbitration rules.

## 4. Required invariants and prohibited behavior

Required invariants include:

- canonical feature IDs do not depend on source array ordering;
- topology is derived only from indices and incidence;
- geometric validity uses bounded evidence, not ad hoc global epsilon comparisons;
- shell nesting is acyclic and deterministic;
- each accepted boundary separates a documented occupied and unoccupied side;
- input precision never decreases during validation;
- all source-to-canonical mappings are total for retained source records; and
- the artifact is immutable and independently verifiable.

The component must not:

- weld coordinate-near vertices;
- repair open or non-manifold topology silently;
- hide malformed facets by triangulating them first;
- use random rays without deterministic fallback and bounded ambiguity handling;
- infer containment from bounding boxes alone;
- accept an uncertain probe as definite;
- permit total winding outside the selected solid model; or
- call external geometry code.

## 5. Test and validation specification

### 5.1 Structural unit tests

Cover:

- empty operands;
- tetrahedra, boxes, concave shells, and high-valence vertices;
- valid facets with canonicalizable duplicate closing indices;
- out-of-range and overflowed indices;
- rings with fewer than three distinct indices;
- repeated non-consecutive indices;
- open boundaries;
- same-direction edge pairs;
- three-use and higher-use edges;
- bow-tie vertices;
- reversed individual facets;
- duplicate facets; and
- non-finite coordinates.

Each invalid category must produce its precise typed diagnostic.

### 5.2 Facet geometry tests

Include:

- convex and concave planar polygons;
- long thin facets;
- nearly collinear corners;
- self-crossing rings;
- repeated coordinates with distinct indices;
- planarity exactly inside, at, and outside the declared precision;
- signed-zero coordinates;
- subnormal offsets;
- large translations; and
- projection-axis ties.

Use an in-tree exact test oracle for low-complexity projected orientation and crossing expectations.

### 5.3 Manifold topology tests

Generate and hand-author:

- genus-zero and positive-genus closed shells;
- disconnected shells sharing coordinate values but not indices;
- vertex-touching and edge-touching shells;
- one source vertex index incorrectly shared across two otherwise closed components;
- non-manifold edge fans hidden by duplicate facets; and
- cyclic-link mutations that preserve edge pair counts but break the vertex manifold condition.

The independent validator must detect failures that simple edge-count checks miss.

### 5.4 Shell semantics tests

Cover:

- one outer shell;
- several disjoint solids;
- one cavity;
- islands inside cavities;
- deep alternating nesting;
- reversed outer and cavity orientations;
- coincident same- and opposite-orientation shells;
- point-, edge-, and face-contacting shells;
- shell order permutations; and
- probes whose uncertainty touches another boundary.

For accepted cases, verify nesting depth, parent relation, occupied side, and total winding. For ambiguous cases, require `ambiguous_shell_semantics` rather than heuristic selection.

### 5.5 Epsilon-validity tests

Create fixtures with:

- one-ULP gaps and overlaps;
- non-adjacent triangle intersections;
- contacts resolvable within declared precision;
- contradictions larger than declared precision;
- near-coplanar overlapping facets;
- collapsed triangles;
- severe self-intersection; and
- nested shells whose boundaries overlap within uncertainty.

Test mandatory, reduced, and diagnostic-only verification policies separately.

### 5.6 Metamorphic and property tests

For generated valid operands, apply:

- vertex-array permutations with index remapping;
- facet permutations;
- ring rotations;
- component and shell permutations;
- globally reversed orientation with corresponding policy adjustment;
- exactly representable translations;
- power-of-two scaling with precision scaling; and
- legal source subdivision that preserves the same boundary.

The canonical validated artifact and digest must remain equivalent according to the documented canonical policy.

### 5.7 Mutation tests

Starting from valid artifacts, inject:

- a missing edge use;
- a third edge use;
- a wrong pair direction;
- a second vertex fan;
- an incorrect shell parent;
- reversed occupied-side evidence;
- a merged coordinate-coincident occurrence;
- a reduced precision bound; and
- a source-to-canonical mapping inconsistency.

Component 15's independent verifier must reject every mutation.

### 5.8 Fuzzing and replay

Fuzz both valid generated manifolds and intentionally invalid index soups. Every crash, nondeterministic classification, unexpected `internal_invariant_error`, or verifier disagreement must be minimized and stored with exact source bits, options, and expected result.

### 5.9 Definition of done

Component 02 is complete only when:

- all accepted input-domain clauses have executable tests;
- edge pairing and vertex-link manifoldness are independently reconstructed;
- shell orientation and nesting are deterministic and bounded;
- duplicate coordinates never cause accidental topology;
- ambiguous geometry fails with a typed result;
- canonical output is invariant under source permutations;
- the artifact exposes all provenance needed by downstream components; and
- production and normative tests remain strict portable C++17 with no external dependencies.
