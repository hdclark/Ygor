# Component 14 Review Amendment: Output Assembly and Canonical Serialization

## Status, precedence, and review conclusion

This file is a normative amendment to `component_14_output_assembly_canonical_serialization.md` produced by the independent Component 14 review required by `tracker.md`.

The original specification remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, Component 15, and replay/diagnostic consumers must read both files together with the reviewed Component 02 and Component 13 specifications and plans.

The review found that the original Component 14 architecture is aligned with `broad_plan.md` and should be retained. In particular:

- Component 14 remains a lossless representation boundary from the immutable cleaned triangle manifold to a private-built `fv_surface_mesh<T,I>`;
- indexed topology remains authoritative, and coordinate equality or tolerance never creates public adjacency;
- every cleaned vertex occurrence and triangle remains represented exactly once;
- topology-distinct equal-coordinate occurrences remain separate public indices;
- triangle orientation is preserved and only forward cyclic rotation is permitted;
- canonical public content is independent of caller presentation, transient identities, cleanup allocation history, hash iteration, and worker schedule;
- `fv_surface_mesh<T,I>` is reused only as the final carrier, while its mutating convenience methods and optional derived arrays are not correctness providers;
- Component 01 owns checked arithmetic, transactions, resources, canonical bytes, SHA-256, diagnostics, replay, and deterministic arbitration;
- Component 03 owns exact scalar-bit handling and precision/tolerance accounting;
- Component 14 publishes only an immutable pending candidate; and
- Component 15 remains the sole final publication authority.

The review identified six mandatory integration corrections:

1. Component 14 must consume the reviewed Component 13 artifact and compatibility contract, including the embedded predecessor `T,I` descriptors and corrected schema versions.
2. Component 14 must preserve Component 03's orthogonal `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` evidence rather than flattening them into an obsolete exact-nominal or generic-precision summary.
3. Every tolerance-facing cleanup, removal, displacement, residual, support, clearance, and derivation record must preserve its closed metric and physical dimension. Component 14 must not reinterpret differently dimensioned values as lengths.
4. Component 12 handoff bounds remain advisory predecessor evidence. Component 14 must preserve the distinction among advisory, proposed, reserved, committed, rejected, one-action, cumulative-lineage, local-removal, and whole-component cost roles.
5. Canonical public content and cleaned-entity presentation correspondence must be separated explicitly. Exact automorphism classes can make a unique cleaned-entity-to-public representative mathematically unavailable even though the canonical public mesh bytes are unique.
6. Candidate, codec, report, provenance, replay, verifier, and Component 15 handoff versions must distinguish reviewed evidence from obsolete evidence so mixed contracts fail before assembly.

No correction authorizes topology repair, coordinate movement, tolerance welding, Boolean reclassification, hidden unit conversion, provenance-dependent public mesh ordering, external dependencies, or ordinary success before Component 15.

## A. Corrected predecessor and capability contract

### A.1 Reviewed Component 13 intake

The Component 14 entrypoint may continue to consume `cleaned_triangle_manifold_view<T>` because the cleaned artifact does not store public indices as `I`. Before authoritative allocation, it must nevertheless validate that the artifact contains and agrees with:

- the invocation's qualified scalar descriptor for `T`;
- the reviewed predecessor public/source index descriptor for `I`;
- the reviewed `triangulated_output_complex<T,I>` dependency chain;
- the reviewed Component 12 truth-layer, category, handoff-certificate, residual, obligation, codec, replay, and verifier versions;
- the reviewed Component 13 cleaned-manifold, action-certificate, budget, codec, replay, and verifier versions;
- the selected Component 14 adapter, candidate, report, codec, replay, and verifier versions;
- the selected Component 15 intake/handoff version; and
- the same context owner, stable context digest, operation, policy set, and immutable lifetime.

A cross-`I` artifact, an obsolete one-parameter predecessor contract, a mixed reviewed/obsolete schema graph, or a stale downstream compatibility declaration is rejected before public mesh construction.

The exact local API names may differ, but the contract is conceptually equivalent to:

```cpp
template<class T, class I>
stage_outcome<artifact_handle<const assembled_output_candidate<T,I>>>
assemble_output_candidate(
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const output_verification_dependencies_view<T,I>& predecessors,
    const cleaned_triangle_manifold_view<T>& cleaned,
    const output_assembly_capabilities<T,I>& capabilities);
```

The predecessor bundle must expose immutable owner-checked views and dependency digests. It must not expose mutable caller meshes, mutable producer storage, unversioned callbacks, external graph providers, external serializers, or arbitrary allocator behavior that can alter semantics.

### A.2 Reviewed evidence completeness

Before canonicalization, Component 14 must validate that the cleaned artifact exposes or immutably references:

- every decisive `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` record required by surviving geometry and cleanup lineage;
- every Component 12 `cleanup_handoff_length_certificate` imported by Component 13;
- certificate-to-obligation, action-to-certificate, and reverse mappings;
- proof that Component 12 reserved and committed zero cleanup budget;
- closed metric and physical-dimension identifiers for every tolerance-facing value;
- every reviewed dimensional derivation and denominator precondition;
- explicit advisory, proposed, reserved, committed, rejected, one-action, cumulative-lineage, local-removal, and component-removal roles;
- action-specific correspondence, displacement, removal, swept/support, and clearance evidence;
- complete precision-ledger parentage for every surviving public coordinate; and
- corrected Component 13 producer/verifier dispositions and digests.

Missing, stale, non-finite, dimensionally invalid, role-contradictory, or unresolvable reviewed evidence is not reconstructed from coordinates, action summaries, or matching counts. A malformed capability or mixed schema is an input/contract failure. A contradiction in a committed predecessor artifact is an `internal_invariant_error`.

## B. Numerical truth, dimensions, and tolerance accounting

### B.1 Orthogonal truth layers remain orthogonal

Component 14 performs no new topology-affecting geometry construction. Copying public coordinate bits, canonicalizing signed zero under the frozen policy, and aggregating reports do not authorize it to collapse predecessor numerical evidence.

For every referenced predicate, residual, orientation, distance, support, clearance, or intersection record, the candidate and its immutable handoff must preserve these distinct facts:

- `rounded_nominal`: the exact stored bits produced by the frozen rounded operation graph;
- `exact_relation`: the exact algebraic sign or zero of the declared relation over the stored nominal floating-point operands, when supported; and
- `uncertainty_enclosure`: the conservative set of values admitted by inherited coordinate uncertainty and operation error.

Component 14 must not:

- rename all three facts as exact-nominal evidence;
- infer a definite geometric disposition from exact-relation zero or sign alone;
- discard an enclosure because the cleaned topology has already been accepted;
- recompute a predicate with a different expression, grouping, operand order, or epsilon;
- claim that serialization or exact bit copying reduces uncertainty; or
- promote inherited local evidence into Component 15's final geometric acceptance.

The geometry report remains pending. Component 15 must be able to reconstruct why every predecessor decision was definite, advisory, rejected, or accepted.

### B.2 Physical dimensions and metrics

The user tolerance and ordinary cleanup displacement/removal budgets are lengths. Every tolerance-facing candidate field, summary, witness, logical record, and verifier comparison must retain a closed metric and physical dimension.

Component 14 must keep separate at least:

- inherited and construction uncertainty;
- one-action displacement;
- cumulative displacement for each original lineage;
- local feature-removal or deformation length;
- whole-component removal size;
- swept/support/clearance length;
- output representation effect;
- topology-change authorization; and
- caller-authorized tolerance.

Area, squared length, determinant magnitude, angle, aspect ratio, signed volume, topology count, and other differently dimensioned or dimensionless quantities may remain diagnostic evidence but may not be copied into a length field or compared directly with tolerance.

A dimensional derivation, such as area divided by a certified positive base-length lower bound to obtain an altitude bound, must remain an explicit reviewed derivation record. Component 14 copies, indexes, aggregates, and encodes that record; it does not invent a substitute derivation or silently change the metric.

### B.3 Advisory evidence is never committed cost

A Component 12 handoff certificate remains advisory after passing through Component 13 and Component 14. It may explain why a cleanup obligation existed, but it does not establish:

- that an action was topologically legal;
- that occurrence merging was permitted;
- that a feature or component could be removed;
- that remote geometry was clear;
- that an action fit the remaining budget; or
- that the value was reserved or committed.

Component 14 must derive report maxima and tolerance usage from the reviewed Component 13 action-specific and ledger records. An advisory handoff bound may be reported in its own role, but it cannot become the sole source of `maximum_realized_displacement`, `maximum_removed_feature_size`, `output_precision`, remaining margin, or ordinary publication eligibility.

Equal maxima use a deterministic complete witness key that includes quantity role, metric, dimension, conservative value, canonical semantic entity/equivalence-class reference, and ledger/action record key. Discovery order and raw nominal floating order are prohibited tie breakers.

## C. Canonical public content versus presentation correspondence

### C.1 Two distinct products

Component 14 must distinguish:

1. **semantic public canonicalization**, which selects canonical component blocks, public vertex order, forward-oriented facet rotations, facet order, public mesh content bytes, and the public-content digest; and
2. **cleaned-entity presentation correspondence**, which supplies a verified bijection from particular cleaned records to the selected public positions and preserves provenance, diagnostics, replay, and Component 15 lookup.

Semantic public canonicalization must depend only on the versioned public content model: exact output coordinate bits, oriented indexed incidence, component structure, and public-semantic occurrence/multiplicity roles. Detailed source lineage, cleanup action identity, caller ordinals, and presentation-specific cleaned IDs must not change public mesh bytes.

### C.2 Exact automorphism classes

For an asymmetric cleaned component, or where normalized reviewed lineage genuinely distinguishes every semantically equivalent node, one unique presentation-independent correspondence may be available.

For an exact automorphism class whose members have identical public-semantic and normalized artifact payloads, no unique distinguished member exists. The implementation must not manufacture one by:

- cleaned numeric ID;
- array position;
- traversal seed;
- source operand or source index;
- allocation/free-list history;
- hash bucket order;
- worker schedule;
- pointer value; or
- an occurrence rank derived from any of those facts.

Instead, the candidate must represent a `correspondence_equivalence_class` or equivalent immutable record containing:

- the canonical public-position set;
- the cleaned-entity member set;
- the complete public-semantic and normalized artifact payload shared by the class;
- the set or compact certificate of permitted bijections induced by accepted automorphisms;
- one source-correct presentation correspondence used for concrete lookup;
- proof that changing only that representative cannot change public mesh bytes, semantic reports, primary failure identity, or publication status; and
- normalized provenance as a multiset or equivalence-class record when per-position attribution is not uniquely meaningful.

The concrete presentation correspondence must be total, bijective, owner-correct, topology-correct, and replayable. It may choose any verified representative inside the certified equivalence class. That representative is presentation evidence, not semantic canonical content.

If a predecessor claims a presentation-independent complete unique key, Component 14 may use it only after independently validating uniqueness and invariance. Duplicate supposedly unique keys are an invariant failure; they must not be repaired by transient ranking.

### C.3 Map and digest consequences

The public mesh content domain and canonical public ordering remain byte-identical under every allowed presentation-only permutation.

Artifact, provenance, evidence, and replay domains must encode correspondence equivalence classes explicitly. They may also encode a concrete presentation correspondence where required to trace the actual invocation, but must mark it as non-semantic presentation data and bind it to exact source replay.

Component 15 must verify:

- the semantic public canonical minimum;
- class membership and automorphism validity;
- that the concrete map belongs to the permitted correspondence class;
- total cleaned/public vertex, triangle, edge-use, and component bijection;
- provenance coverage at the strongest mathematically meaningful granularity; and
- that replacing a representative inside an exact class does not alter public semantic bytes or acceptance.

Digest equality never substitutes for full structural or equivalence-class comparison.

## D. Candidate, reports, provenance, and Component 15 handoff

### D.1 Candidate additions

The reviewed `assembled_output_candidate<T,I>` must additionally contain or immutably reference:

- reviewed Component 12, 13, 14, and Component 15 compatibility versions;
- validated `T,I` descriptors from the complete predecessor chain;
- all required three-layer truth evidence or stable immutable references;
- closed metrics and physical dimensions for every tolerance-facing value;
- dimensional derivation records and denominator evidence;
- advisory/proposed/reserved/committed/rejected cost roles and reverse maps;
- Component 12 zero-cleanup-budget-use evidence;
- correspondence-equivalence-class records;
- the concrete source-correct presentation correspondence and its certificate;
- semantic public canonicalization evidence separated from presentation correspondence evidence;
- dimensioned precision, cleanup, removal, and tolerance summaries;
- pending-only topology and geometry statuses; and
- corrected codec, report, provenance, replay, and producer-verifier versions.

The candidate must not serialize runtime owner addresses or mutable handles. Stable dependency digests, schema versions, and exact replay references bind the reviewed records.

### D.2 Report requirements

The geometry-pending and cleanup/topology reports must preserve:

- separate truth-layer availability/disposition counts;
- separate quantities by role, metric, and dimension;
- advisory handoff values without promotion;
- action-specific proposed, reserved, committed, and cumulative costs;
- deterministic maximum witnesses;
- derivation and denominator references;
- unknown or unsupported evidence counts, which must be zero for ordinary candidacy; and
- explicit remaining Component 15 obligations.

A summary that combines uncertainty, movement, feature removal, component removal, and tolerance into one epsilon is invalid.

### D.3 Provenance requirements

Per-public-entity provenance remains required where the cleaned record is uniquely distinguished. For an exact correspondence equivalence class, the candidate may attach:

- invariant shared lineage;
- the normalized lineage multiset for the class;
- class-to-public-position and class-to-cleaned-member ranges; and
- a presentation-only concrete map for exact source replay.

It must not falsely claim a unique semantic source attribution when the public topology and normalized reviewed lineage admit an exact automorphism.

### D.4 Component 15 query surface

`OutputAssemblyQueries.h` or an equivalent immutable API must allow Component 15 to inspect and independently validate:

- predecessor `T,I` and reviewed schema compatibility;
- every truth-layer record required by surviving lineage;
- metrics, dimensions, derivations, and cost roles;
- advisory and committed budget separation;
- Component 12 zero-budget evidence;
- semantic public labeling and complete canonical-search certificates;
- correspondence equivalence classes and the concrete representative map;
- dimensioned reports and maximum witnesses;
- logical encodings and digest domains; and
- replay material sufficient to permute representatives within exact automorphism classes.

Component 14 must not prescribe Component 15's final geometry algorithms, but it must not omit evidence the reviewed contracts require.

## E. Independent Component 14 verification additions

The Component 14 producer verifier must additionally:

1. validate the complete reviewed Component 12/13/14 schema graph and embedded `T,I` compatibility;
2. reject mixed obsolete/reviewed truth, handoff, obligation, budget, candidate, codec, replay, or verifier versions;
3. independently validate every referenced metric, dimension, role, interval, derivation, and reverse map;
4. verify Component 12 committed zero cleanup budget;
5. verify no advisory handoff value was promoted to action authorization, reservation, commit, maximum, or output precision;
6. reconstruct required report maxima from Component 03 and Component 13 reviewed records;
7. verify public semantic canonicalization without using presentation correspondence as a tie breaker;
8. independently reconstruct exact automorphism/correspondence equivalence classes for bounded fixtures and validate production certificates for larger cases;
9. prove every concrete cleaned/public map is a permitted total bijection within its class;
10. reject a fabricated unique occurrence rank when no presentation-independent discriminator exists;
11. regenerate logical domains with semantic and presentation fields in their prescribed separate sections; and
12. reject producer-shaped mutations even when counts, maps, digests, and summary maxima are superficially repaired.

Shared Component 01 byte, checked-arithmetic, SHA-256, and Component 03 primitive ledger services remain permitted. Higher-level grouping, class construction, map validation, report aggregation, and codec traversal must remain independently organized.

## F. Existing Ygor assessment confirmed

The review confirms the original plan's reuse decisions:

- `fv_surface_mesh<T,I>` and `vec3<T>` remain suitable carriers for exact copied coordinates and explicit facet indices.
- The public mesh's mutable fields and convenience methods are not transaction, manifold, lineage, precision, canonicalization, or verification providers.
- `YgorMeshesVerification` remains useful only as a coarse differential reference; it cannot establish the reviewed evidence, correspondence, or publication contracts.
- Existing text/XML/STL/OBJ/OFF/PLY serialization remains unsuitable for canonical logical bytes because it does not define exact scalar-bit, versioned domain, report, provenance, or replay semantics.
- Legacy Boolean, BSP, hole-filling, orientation, remeshing, cleanup, and triangulation helpers remain unsuitable as Component 14 providers where they infer identity from coordinates, mutate topology, use unrelated tolerances, omit bounded evidence, or lack deterministic transactional contracts.
- A shared in-tree canonical-incidence-labeling kernel may be extracted from Component 02, but semantic labeling and presentation correspondence must remain separate at both call sites.
- Component 01 and Component 03 remain the sole providers of shared contract, byte, digest, transaction, scalar-bit, precision, tolerance, and dimensional services.

No second physical-unit system, interval package, graph library, hash library, serializer, mesh container, or task runtime may be introduced. Production and normative-test code remain strict portable C++17 and self-contained within Ygor.

## G. Failure and diagnostic additions

Add stable Component 14 subcodes for at least:

- reviewed predecessor scalar/index descriptor mismatch;
- obsolete or mixed Component 12/13/14 schema versions;
- missing or conflated numerical truth layers;
- unsupported, unknown, or non-length tolerance-facing dimension;
- metric mismatch during aggregation or maximum comparison;
- invalid dimensional derivation or denominator evidence;
- advisory handoff promoted to authorization, reservation, commit, or report maximum;
- Component 12 nonzero cleanup budget use;
- semantic public labeling contaminated by provenance or presentation data;
- correspondence equivalence class incomplete or contradictory;
- concrete presentation map outside the permitted automorphism class;
- fabricated transient occurrence rank;
- false unique provenance attribution inside an exact equivalence class;
- semantic/presentation codec-domain contamination; and
- Component 15 reviewed handoff incompatibility.

Expected valid ambiguity inside a certified correspondence equivalence class is not a failure. Unsupported or incomplete canonical search is `resource_limit`. Malformed policy/capability data is `input_contract_error` or the existing version/type category. A contradiction in a committed predecessor artifact or producer/verifier disagreement is `internal_invariant_error`.

Every relevant failure includes canonical semantic component/entity or equivalence-class keys, cleaned presentation references when available, metric, dimension, role, truth-layer summaries, conservative bounds, derivation/version data, expected and observed correspondence facts, deterministic arbitration key, and replay payload.

## H. Required test amendments

In addition to the original Component 14 suite, add:

1. **Reviewed schema/type tests** covering every supported `float`/`double` and `uint32_t`/`uint64_t` combination, embedded predecessor `I` mismatch, and every obsolete/reviewed schema mixture.
2. **Truth-layer preservation tests** proving `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` remain distinct through reports, provenance, codecs, replay, and Component 15 queries.
3. **Dimensional mutation tests** replacing length with area, squared length, determinant, angle, aspect ratio, or an unknown dimension and requiring deterministic rejection even after repairing counts and digests.
4. **Advisory-role tests** proving Component 12 handoff bounds cannot become authorization, reservation, commit, maximum displacement/removal, output precision, or tolerance margin.
5. **Derivation tests** covering valid and invalid area-to-length derivations, zero-containing denominator enclosures, extreme exponents, and changed derivation traces.
6. **Cost-role tests** distinguishing one-action, cumulative-lineage, local-removal, component-removal, swept/support/clearance, representation, and tolerance values.
7. **Exact automorphism correspondence tests** using symmetric tetrahedra, boxes, cyclic shells, repeated identical components, repeated coordinate colors, and identical normalized lineage payloads.
8. **Presentation-representative metamorphics** that permute cleaned IDs, arrays, traversal seeds, source presentation, and allowed automorphism representatives while requiring identical public mesh bytes, semantic reports, primary failures, and publication status.
9. **Correspondence certificate mutations** that omit a class member, add an invalid bijection, map across components, violate oriented incidence, invent a transient rank, or assert false uniqueness.
10. **Provenance-class tests** verifying uniquely distinguished entities retain precise lineage while exact classes use invariant shared lineage and normalized lineage multisets without false attribution.
11. **Codec-domain tests** proving presentation correspondence cannot perturb the public-content domain and semantic fields cannot be omitted from artifact/evidence domains.
12. **Component 15 handoff tests** proving every truth layer, dimension, metric, cost role, derivation, equivalence class, and presentation map is independently queryable and reconstructible.
13. **Existing-carrier tests** proving optional `fv_surface_mesh` fields remain empty and cannot carry hidden precision, identity, or topology semantics.
14. **Mutation completeness tests** requiring zero survivors after producer-shaped repairs to maps, counts, reports, logical bytes, and digests.

For bounded graphs, the test-only exhaustive oracle must enumerate all orientation-preserving legal labelings and the induced cleaned/public correspondence classes independently of production refinement and search. Disabling pruning, memoization, or changing branch order must not change the semantic minimum or accepted class.

## I. Reviewed definition of done

Component 14's review step is complete only when the original specification and plan, as amended, require all of the following:

- reviewed Component 13 artifact/schema intake and embedded `T,I` compatibility;
- explicit preservation of `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure`;
- closed metrics and physical dimensions on every tolerance-facing value;
- immutable dimensional derivations and denominator evidence;
- strict separation of Component 12 advisory handoff bounds from Component 13 action-specific proposed, reserved, committed, rejected, and cumulative costs;
- no reinterpretation or promotion of predecessor evidence during assembly;
- semantic public canonicalization independent of provenance and presentation correspondence;
- explicit correspondence equivalence classes for exact automorphisms;
- no fabricated unique mapping or provenance attribution where the mathematics supplies only an equivalence class;
- total source-correct concrete maps certified as members of the permitted class;
- corrected candidate, report, provenance, codec, replay, verifier, and Component 15 handoff versions;
- independent verification of dimensions, roles, derivations, semantic minima, correspondence classes, maps, reports, logical domains, and digests;
- preservation of the original lossless public-mesh, topology, index, duplicate-coordinate, orientation, round-trip, resource, cancellation, and pending-status contracts;
- zero required mutation survivors for reviewed schema, truth-layer, dimensional, advisory-role, automorphism-correspondence, and codec-domain corruption;
- strict portable C++17 production and normative-test code with no external dependency; and
- Component 15 can perform final independent verification without topology repair, unit reinterpretation, advisory-cost promotion, or trust in a non-unique presentation representative.
