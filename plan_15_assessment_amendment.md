# Plan 15: Assessment-Driven Productization Amendment

## 0. Status, precedence, and intent

This document is a normative correction to the Component 1, 2, 10, 11, 12, 13, and 14 plans. It incorporates the release-blocking findings in `assessment.md` while preserving the exact-topology principles in `broad_plan.md`.

Where an earlier plan conflicts with this amendment, this amendment governs product-facing behavior. In particular, it supersedes any implication that:

- the in-tree symbolic engine is already suitable as the sole production CAD backend;
- `exact_in_T` is a practical general-purpose floating-output mode;
- a valid exact stratified result may exist only as an invocation-private intermediate;
- input normalization may be omitted while unknown-provenance meshes are advertised as supported;
- the product architecture and qualification program must avoid optional external geometry backends; or
- component-level tests alone are sufficient release evidence.

The in-tree engine remains valuable and must continue to fail closed. This amendment does not weaken exact predicates, symbolic incidence, deterministic ordering, transactional publication, or mandatory verification. It changes the product boundary and the evidence required before production promotion.

## 1. Product decision

Treat the current in-tree backend as `experimental_exact_v1` until the qualification gates in `plan_16_qualification_release.md` pass. It may be used for research, internal validation, differential testing, exact-result generation, and explicitly opted-in experimental operation. It must not be selected silently as the sole production backend.

The product architecture is split into five independently versioned layers:

1. **Input preparation**: strict validation plus an optional, explicit normalization service.
2. **Backend execution**: one or more Boolean backends behind a narrow capability-described adapter.
3. **Authoritative exact result**: a stable exact or symbolic stratified boundary that survives beyond one invocation.
4. **Output realization**: strict exact-in-`T`, exact-coordinate output, or separately typed certified approximate embedding.
5. **Qualification and publication**: independent verification, multi-backend comparison, corpus evidence, and release policy.

No layer may silently change the semantics promised by another layer.

## 2. Non-negotiable invariants retained from the broad plan

All backends and adapters used by the product path must preserve these requirements or declare that they cannot provide the corresponding capability:

- Regularized set semantics are explicit and operation order is preserved.
- Topology-changing decisions are never controlled by an undocumented epsilon.
- Geometric equality and topological occurrence identity remain distinct.
- A backend success is not published until the applicable verifier accepts it.
- Non-manifold or otherwise unsupported result topology is reported distinctly from malformed input and coordinate realization failure.
- Approximate output is never labeled as exact point-set output.
- Determinism applies to canonical artifacts, diagnostics, replay records, and backend selection.
- Fallback does not convert a failure into a semantically different success without explicit caller policy and an auditable result record.

## 3. Target architecture

### 3.1 Preparation API

Add a preparation namespace and service distinct from the exact Boolean evaluator:

```cpp
namespace ygor::mesh_boolean {

template<class T, class I>
struct strict_validation_result;

template<class T, class I>
struct prepared_operand;

struct normalization_policy;
struct normalization_report;

template<class T, class I>
status_or<prepared_operand<T, I>> validate_operand_strict(...);

template<class T, class I>
status_or<prepared_operand<T, I>> normalize_operand(
    const fv_surface_mesh<T, I>&,
    const normalization_policy&,
    normalization_report&);

} // namespace ygor::mesh_boolean
```

`validate_operand_strict` retains Component 2 semantics and must not alter geometry or topology except for already-documented canonical removal of unused storage.

`normalize_operand` is an explicitly approximate/modeling operation. It may change geometry or topology only under a caller-supplied policy and must return a deterministic report. It is not a hidden prelude to exact Boolean evaluation.

### 3.2 Backend adapter

Introduce a narrow backend interface that consumes frozen prepared operands and an operation contract and returns a backend result envelope:

```cpp
enum class backend_id : std::uint16_t {
    experimental_exact_v1,
    // Optional adapters receive stable assigned values when implemented.
};

struct backend_capabilities {
    bool exact_set_semantics;
    bool exact_coordinates;
    bool stratified_output;
    bool manifold_mesh_output;
    bool deterministic_canonical_output;
    bool certified_failure_categories;
    bool provenance_mapping;
};

class boolean_backend {
public:
    virtual ~boolean_backend() = default;
    virtual backend_id id() const noexcept = 0;
    virtual backend_capabilities capabilities() const noexcept = 0;
    virtual status_or<backend_result> evaluate(const backend_request&) const = 0;
};
```

The concrete C++ interface may avoid virtual dispatch, but the semantic boundary is required. Adapters must not expose backend-specific object ownership through the common API without a stable lifetime wrapper.

Backend selection is frozen in the invocation context. The result records the selected backend, adapter version, capability digest, and whether any explicitly permitted fallback occurred.

### 3.3 Backend selection and fallback

Define these policies:

- `explicit_backend`: caller names one backend; no fallback.
- `qualified_default`: service selects only from backends approved by the current qualification manifest.
- `diagnostic_compare`: run selected independent backends for comparison; one backend remains the declared producer.
- `explicit_fallback_chain`: caller supplies an ordered list and acceptable failure categories.

Fallback is prohibited for `internal_invariant_error`, stale certificates, replay mismatch, or verifier disagreement. A fallback success must retain the primary failure and identify the producing backend. The service must not imply that two different backend semantics are interchangeable.

## 4. Stable result model

Replace a mesh-or-error-only product boundary with a tagged, immutable result envelope.

```cpp
enum class result_representation : std::uint8_t {
    exact_stratified,
    exact_in_T_mesh,
    certified_approximate_mesh
};

struct exact_coordinate {
    // Canonical rational or versioned construction reference.
};

struct exact_stratified_boundary;

template<class T, class I>
struct certified_mesh_result;

struct boolean_product_result {
    result_representation representation;
    backend_provenance backend;
    operation_contract operation;
    preparation_provenance preparation;
    exact_result_handle exact_result;
    optional_mesh_payload mesh;
    optional_realization_certificate realization;
    attribute_transfer_report attributes;
    verification_report verification;
};
```

The exact result is the authority. Mesh payloads are realizations of that authority under an explicit policy.

### 4.1 Exact stratified output

Promote the verified `selected_exact_boundary` into a stable public or advanced-public result type with:

- immutable ownership independent of a short-lived `boolean_context`;
- versioned canonical serialization and replay;
- exact coordinates represented as canonical rationals or versioned construction records;
- explicit surface occurrences and spherical-link/topology records;
- topology classification and obstruction records;
- complete source/backend provenance; and
- an API to request later realization into one or more coordinate/output types.

A valid exact non-manifold result must not be discarded merely because `fv_surface_mesh<T, I>` cannot represent it.

### 4.2 Exact-in-`T`

Retain `exact_in_T` as the strict certified mode. Success means every emitted coordinate decodes to the exact target and all Component 11 obligations pass. This mode is useful for regression, exact dyadic workloads, and callers that require bit-exact point-set output. It must not be described as the ordinary CAD output path.

### 4.3 Certified approximate embedding

Add a separately tagged `certified_approximate_embedding_v1` mode. It must never claim exact point-set equality. Its policy contains:

- caller-supplied maximum vertex displacement in model units;
- optional per-axis displacement limits;
- maximum support-plane deviation for triangulated output;
- whether original input vertices may move;
- deterministic candidate-generation and search limits;
- required topology, embedding, and orientation obligations; and
- application acceptance metadata, including the model tolerance that justified the request.

A successful certificate must include, at minimum:

- the exact target and emitted bit pattern for every realized vertex;
- exact displacement vectors and maxima;
- proof that distinct required vertices remain distinct;
- proof that no selected facet collapses or reverses;
- proof that required incidences and edge orders are preserved;
- proof that no prohibited non-adjacent intersection is introduced;
- proof that output topology is occurrence-isomorphic to the authorized exact boundary;
- a complete list of relaxed geometric relations and their measured deviations; and
- the exact result digest to which the mesh realization is bound.

The verifier independently replays every certificate obligation. Search exhaustion is `output_not_representable` with a policy-relative subcode, not proof that no approximate embedding exists.

## 5. Normalization contract

### 5.1 Scope

The normalization service addresses application/model intent before exact Boolean evaluation. Candidate operations include:

- duplicate and near-duplicate vertex consolidation;
- crack and small-gap closure;
- orientation repair;
- duplicate/overlapping facet resolution;
- non-planar polygon triangulation or refitting;
- self-intersection diagnosis and, only when policy permits, repair;
- shell nesting/orientation correction;
- sliver handling;
- attribute seam preservation; and
- deterministic removal of irrelevant storage.

No operation is enabled implicitly. Policies must distinguish diagnosis-only, safe structural canonicalization, and geometry-changing repair.

### 5.2 Report and provenance

Every normalization result carries:

- input and output canonical digests;
- policy and tolerance values with units;
- each edit in canonical order;
- source-to-prepared vertex, edge, facet, shell, and attribute maps;
- exact or bounded displacement for modified geometry;
- topology changes and their justification;
- rejected/unresolved defects;
- post-normalization strict validation certificate; and
- reversibility information where available.

A normalized operand is accepted by the Boolean engine only after strict Component 2 validation. The Boolean context records the normalization report digest.

### 5.3 Product claim boundary

Until normalization is implemented and qualified, documentation must state that the engine accepts only strict, already-valid operands. Unknown-provenance STL/OBJ/scan/CAD tessellations are not a supported end-to-end product workflow.

## 6. Attributes and provenance

Define attribute behavior before production promotion. At minimum, support a versioned transfer policy for:

- source body/shell/facet identifiers;
- materials and per-face metadata;
- vertex normals and sharp-edge tags;
- texture-coordinate seams;
- user-defined opaque attribute channels; and
- compact construction provenance.

Attribute transfer must not influence geometric topology decisions. When one output entity derives from multiple sources, the policy must define deterministic merge, split, conflict, or omission behavior. Every omission is reported; silent attribute loss is prohibited in the product API.

The exact result retains full provenance. A mesh realization returns a compact stable mapping from output vertices/facets to exact entities and source contributors.

## 7. Simple application-facing service

Add a one-call service while preserving lower-level dependency injection:

```cpp
template<class T, class I>
status_or<boolean_product_result> boolean_operation(
    const fv_surface_mesh<T, I>& a,
    const fv_surface_mesh<T, I>& b,
    operation op,
    const boolean_service_options& options = {});
```

The default options must be conservative and explicit about maturity:

- no geometry-changing normalization unless requested;
- no silent backend fallback;
- mandatory verification enabled;
- result representation selected by the downstream product contract;
- exact result retained when mesh realization fails, if the caller requested it;
- backend and preparation provenance always returned; and
- experimental backends require explicit opt-in unless listed in the signed/frozen qualification manifest.

Advanced constructors may inject kernels, verifiers, executors, backends, and stores, but ordinary callers must not register internal verifier implementations manually.

## 8. Component impact

### Component 1

Add backend, preparation, result-representation, normalization, attribute, and maturity policies to the versioned contract. Separate semantic realization mode from search strategy. Add result-envelope and backend-provenance errors. Preserve strict deterministic serialization.

### Component 2

Remain the strict validator/canonicalizer. Accept raw strict operands or prepared operands with a bound normalization report. Verify provenance maps and reject stale preparation certificates. Do not absorb normalization heuristics into validation.

### Components 3 through 9

No semantic weakening is authorized. Their in-tree implementation remains the `experimental_exact_v1` backend. Isolate backend-private artifacts from public headers where practical.

### Component 10

Publish the verified exact stratified boundary as a durable result artifact. Topology classification remains exact and occurs before any mesh publication gate.

### Component 11

Implement exact-coordinate export and the separately tagged certified approximate mode. Keep exact-in-`T` unchanged. Generate and verify a complete obligation universe for each mode.

### Component 12

Assemble only the requested mesh representation. Bind mesh, exact result, provenance, attributes, and realization certificate in one product result. Do not erase an exact success when mesh publication fails.

### Component 13

Verify preparation certificates, backend envelopes, exact-result serialization, approximate realization certificates, attribute mappings, and cross-layer digest bindings. External-backend adapters require adapter-specific verification and capability checks.

### Component 14

Execute the qualification program in `plan_16_qualification_release.md`. External providers may be used as independent comparators; agreement is evidence, not proof.

## 9. Error-model additions

Add stable categories or subcodes for:

- `normalization_required`;
- `normalization_failed`;
- `backend_unavailable`;
- `backend_capability_mismatch`;
- `backend_disagreement`;
- `backend_unqualified`;
- `exact_result_serialization_error`;
- `attribute_transfer_conflict`;
- `approximation_policy_rejected`; and
- `qualification_policy_violation`.

Expected backend failure remains distinct from adapter defects and verifier disagreement. An exact result with failed mesh realization is representable as a successful exact-result envelope plus a failed realization attempt when the caller requested exact retention.

## 10. Replay and compatibility

Bump option, error, artifact, certificate, and replay schemas together. Replay records include:

- preparation policy/report digest;
- backend ID, version, build identifier, and capability digest;
- fallback chain and producing backend;
- exact-result schema/digest;
- realization semantic mode and search policy;
- attribute-transfer policy;
- verifier set/version; and
- qualification-manifest identifier used for default backend selection.

Unknown enum values, capability changes, or stale report bindings are hard errors. Do not reinterpret old `exact_in_T` successes as approximate successes.

## 11. Implementation sequence

### P0: freeze product contracts

1. Approve the result envelope and maturity model.
2. Add backend/preparation/result policies and schema versions.
3. Define exact-result ownership and serialization.
4. Update public documentation to remove unsupported production claims.

No approximate or normalization code begins until these contracts are reviewed.

### P1: durable exact result

1. Detach the selected exact boundary from invocation-private lifetime.
2. Add canonical serialization/deserialization and digest binding.
3. Expose topology classification, exact coordinates/constructions, and provenance.
4. Add round-trip, stale-certificate, and corruption tests.

### P2: preparation boundary

1. Extract strict validation as a reusable service.
2. Define normalization policies and reports.
3. Implement diagnosis-only mode first.
4. Add repair operations one at a time, each with independent validation and corpus evidence.

### P3: output modes

1. Preserve `exact_in_T` behavior and tests.
2. Add exact-coordinate export.
3. Implement certified approximate triangulated embedding.
4. Add independent certificate replay and policy-relative failure diagnostics.

### P4: backend adapters

1. Stabilize the common adapter and capability model.
2. Wrap the in-tree engine as `experimental_exact_v1`.
3. Add at least one independent mature backend adapter for qualification.
4. Implement diagnostic comparison and explicit fallback without semantic conflation.

### P5: attributes and service API

1. Add stable provenance maps and attribute policies.
2. Add the one-call service and conservative defaults.
3. Keep advanced dependency injection behind an expert API.
4. Add migration examples and error-handling guidance.

### P6: qualification and promotion

Execute `plan_16_qualification_release.md`, publish the report, and promote only the modes/backends that meet their declared gates.

## 12. Definition of done

This amendment is implemented only when:

- the in-tree backend is explicitly labeled experimental unless a qualification manifest says otherwise;
- strict validation and normalization are separate callable operations;
- every normalization edit is reported and followed by strict validation;
- a verified exact stratified result can be retained and serialized independently of mesh realization;
- `exact_in_T` is documented as strict, not general-purpose;
- certified approximate output has a distinct result kind and independently replayable displacement/topology certificate;
- non-manifold exact results are accessible even when manifold mesh publication is rejected;
- backend identity, capability, fallback, and qualification provenance are part of every result/replay;
- attribute/provenance behavior is specified and tested;
- ordinary callers have a one-call API with no manual verifier registration;
- no silent repair, fallback, tolerance, or semantic downgrade exists; and
- the qualification and release gates in `plan_16_qualification_release.md` pass for every mode advertised as production-ready.
