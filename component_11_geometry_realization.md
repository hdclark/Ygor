# Component 11: Lazy Geometry Realization and Certification

## 0. Purpose

Realize each selected symbolic vertex once, convert exact coordinates to `vec3<T>`, and prove that the finite-precision embedding preserves the exact selected boundary's required topology and geometry. Explicitly reject results that cannot be represented safely in `T`.

## 1. Input contract

Accept a topology-authorized verified selected exact boundary, canonical symbolic registry, exact kernel, target `T`, explicit realization semantics, search limits, and all defining-relation and embedding obligations.

Original vertices normally retain their exact input `T` bit patterns. Constructed vertices have exact rational coordinates/provenance. One symbolic ID must map to one output vertex candidate globally.

## 2. Required behavior

### Lazy exact evaluation

- Evaluate exact Cartesian coordinates only for selected vertices and any witnesses needed by certification.
- Memoize one exact coordinate and one eventual `T` coordinate per canonical symbolic ID.
- Prove construction consistency when multiple provenance paths define the same vertex.

### Candidate conversion and semantics

- Under schema-v1 `exact_in_T`, decode the correctly rounded candidate and require exact equality to the symbolic rational on every axis. A mismatch is `output_not_representable`; neighboring search cannot alter this semantic requirement.
- If certification fails, optionally search a deterministic finite neighborhood of representable values or solve a constrained rounding assignment.
- Shared vertices are moved only as one global variable; facets cannot realize the same symbol differently.
- Never snap distinct symbols together or split one symbol to satisfy local consumers.

### Certification obligations

Build an explicit finite set of exact sign/order constraints sufficient for the emitted mesh, including:

- Distinct selected vertices remain distinct where topology requires it.
- Every output facet remains non-zero-area, planar under the output facet contract, simple, and consistently oriented.
- Vertex order along every selected edge/carrier is preserved.
- Adjacent facets share bit-identical indexed vertices and compatible edges.
- Required incidences remain incidences, and prohibited non-adjacent intersections are not introduced.
- Accepted bits satisfy every source-plane, carrier, affine-parameter, equality, and carrier-order defining relation by exact substitution.
- Local radial/seam ordering and selected patch side orientation are preserved.
- The realized boundary remains embedded and subdivision-equivalent to the exact selected boundary.

Evaluate certification against exact interpretations of candidate `T` values. Conservative interval separation may prove constraints, but uncertain constraints require exact evaluation.

Polygonal facets create an additional constraint: independently rounded vertices that were exactly coplanar may cease to be coplanar. The policy must either find certified coplanar `T` representatives, emit a certified triangulation (triangles are always planar), or fail. Deterministic triangulated emission is the baseline reliable policy.

### Impossibility behavior

If an exact coordinate is not representable in `T`, return `output_not_representable` with the symbol, axis, exact target, nearest bits, and nonzero difference. Search exhaustion may be called policy-relative only for a future separately tagged approximate mode. Do not drop features, widen tolerances, or claim exact success.

## 3. Output contract

Produce either:

- `realized_boundary<T>`: one `vec3<T>` per selected symbolic vertex, realized facets/cycles, exact-to-output maps, and a passed realization certificate; or
- A typed failure with the minimal known conflicting constraints and exact provenance.

Invariants:

- Realization is a single global mapping from symbolic IDs to `T` bit patterns.
- All mandatory certification predicates match their exact-boundary signs/relations.
- No output coordinate is NaN or infinite.
- Original-coordinate preservation follows the declared policy exactly.
- Conversion and search order are deterministic across supported platforms.

Resource-limit failure is distinct from a proven or policy-relative representability failure.

## 4. Verification and definition of done

- Tests include rational intersections not exactly representable in `T`, distinct intersections rounding to one point, tiny facets, huge dynamic range, subnormals, and rounding-induced inversions.
- Known realizable cases certify; known impossible cases fail without partial output.
- Every emitted certificate is independently replayed from serialized `T` bit patterns.
- Different expression/provenance paths for one symbol emit bit-identical coordinates.
- Ambient rounding-mode and thread-count changes do not affect output.

Generate the complete obligation universe before solving. Build the bipartite variable-obligation graph, solve each connected component independently in canonical least-variable order, and compose the lexicographically first component assignments. Publish per-component variables, obligations, accepted ranks, rejected-prefix witnesses, `visited_nodes`, and `complete_assignments`; the verifier replays those certificates without a global DFS.

Generate triangle-pair obligations through a deterministic conservative broad phase over exact candidate-domain AABBs. Equality counts as overlap; uncertain pairs are retained. Production uses `conservative_domain_aabb_v1`, exhaustive all-pairs remains a bounded oracle, and limits return `resource_limit` rather than incomplete pruning. The verifier independently reconstructs candidates with a different implementation family.
