# Component 1: Operation Contract and Boolean Context

## 0. Purpose

Define the mathematical operation, accepted domain, stable identity scheme, deterministic execution policy, ownership/lifetime rules, error model, and shared services for one Boolean invocation. This component prevents modules from acquiring inconsistent notions of equality, orientation, degeneracy, or failure.

## 1. Input contract

Accept:

- Two immutable `fv_surface_mesh<T, I>` operands. The context records operand role; it does not validate their geometry.
- One operation: regularized union, intersection, `A - B`, `B - A`, or symmetric difference.
- A documented solid policy. The default interprets consistently outward-oriented shells as boundaries of regular closed solids and uses nesting/orientation for cavities.
- Determinism options, mandatory verification level, tracing controls, execution/thread budget, and resource limits.
- A realization policy that is always certified. It may choose nearest `T`, search neighboring `T` values, or decline realization; it may not accept an uncertified mesh.

Preconditions:

- `T` and `I` are supported specializations. `I` is an unsigned or explicitly checked non-negative index type.
- Inputs remain alive and unmodified for the invocation, or are copied into context-owned canonical storage.
- The caller accepts regularized set semantics: lower-dimensional contacts alone do not create output boundary.

## 2. Required behavior

### Mathematical contract

Specify result occupancy as a truth function `R(in_A, in_B)`. Difference must preserve operand order. A boundary patch is selected exactly when `R` differs on its two open sides. Orient selected patches so result interior lies on the globally prescribed side.

Specify treatment of:

- Empty operands and empty results.
- Equal solids and coincident boundary regions.
- Nested shells and cavities.
- Tangential point/curve contact.
- Coplanar overlap with same or opposite orientation.
- Multiple disconnected components.

### Stable identities

Provide strongly typed IDs for operand, shell, original vertex, edge use, canonical undirected edge, facet, candidate, raw event, symbolic vertex, symbolic curve, local patch, global halfedge, global patch, and cell. IDs contain canonical integer keys, never addresses.

Original IDs derive from canonicalized input order. Constructed IDs derive from normalized provenance keys and registry order. Parallel workers may create provisional records, but publication sorts and interns them canonically.

### Shared policy and services

Own or reference:

- Exact-kernel instance and immutable arithmetic policy.
- Arena/storage with stable handles and stage-scoped publication.
- Deterministic sort and hash helpers; hash tables are lookup accelerators only, never output-order authorities.
- Diagnostic sink, counters, stage trace, cancellation, and resource accounting.
- Versioned replay metadata: input digest, operation, options, platform arithmetic facts, and engine version.

The context is logically immutable after setup except for explicitly stage-owned stores, monotonic diagnostics, and accounting. Each stage writes to a private transaction and publishes only after verification.

### Error discipline

Use typed status/result values; do not encode expected failures as assertions or empty meshes. Every error records stage and stable feature IDs. Arithmetic uncertainty is not an error: it triggers exact fallback. Assertions guard programmer invariants and must also have release-mode checked equivalents at publication boundaries.

## 3. Output contract

Produce:

- A frozen operation contract and truth table.
- Stable-ID factories and canonical comparison functions.
- A stage transaction/publication interface.
- Typed errors: input contract, unsupported platform, resource limit, index overflow, output not representable, and internal invariant error.
- A complete replay descriptor sufficient to reproduce deterministic behavior.

Invariants:

- Exactly one operation and semantics policy govern all stages.
- IDs are unique within type and invocation and remain valid for their documented lifetime.
- Published stores are immutable.
- Resource accounting cannot wrap; exceeding a limit fails before allocation/publication.
- Changing thread count or task partition cannot alter canonical IDs or outputs.

Failure conditions:

- Unsupported `T` representation or integer assumptions.
- Invalid options or contradictory shell/operation policy.
- Resource accounting overflow or caller cancellation.
- Any attempt to publish an unverified artifact.

## 4. Verification and definition of done

- Truth-table tests cover all operations and all four occupancy pairs.
- ID ordering and serialization are stable across compilers, allocation patterns, and thread counts.
- Transaction fault injection proves failed stages cannot leak partial artifacts.
- Every public failure contains stage and replay metadata.
- Context APIs contain no geometry-specific tolerance.
