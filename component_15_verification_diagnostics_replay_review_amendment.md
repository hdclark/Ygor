# Component 15 Review Amendment: Independent Verification, Diagnostics, and Replay

## Status, precedence, and review conclusion

This file is a normative amendment to `component_15_verification_diagnostics_replay.md` produced by the independent Component 15 review required by `tracker.md`.

The original specification remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, Component 16, Component 17, and public-result/replay consumers must read both files together with the reviewed Component 13 and Component 14 specifications and plans.

The review found that the original Component 15 architecture is strongly aligned with `broad_plan.md` and should be retained. In particular:

- Component 15 remains the sole authority that may assign `geometry.status == tolerance_checked` and publish ordinary success;
- public indexed topology is reconstructed independently from exact public indices rather than coordinate proximity or producer maps;
- producer counts, maps, classifications, reports, search results, digests, and pass flags remain claims to verify;
- event sharing, occurrence separation, classification, selection, construction lineage, cleanup, precision, reports, canonical bytes, replay, and re-ingestion remain mandatory final-verification subjects;
- the final forbidden-interaction search remains conservative and independently organized from Component 06 and Component 07 high-level control flow;
- deterministic finding arbitration, transactional publication, bounded resources, cancellation, serial-reference semantics, strict C++17, and no external dependency remain mandatory; and
- a topology-valid but geometrically unresolved candidate fails closed.

The review identified eight mandatory integration corrections:

1. Component 15 must consume the reviewed Component 12/13/14 schemas and reviewed Component 14-to-15 handoff, including validated `T,I` compatibility and pending-only status.
2. Component 15 must preserve and audit Component 03's orthogonal `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` layers. Obsolete “exact-nominal” logic must not survive.
3. Every tolerance-facing quantity must retain a closed metric, physical dimension, derivation, and cost role. Area, squared length, determinant magnitude, angle, volume, and quality values must not be compared directly with length tolerance.
4. Component 15 must verify Component 14's semantic canonical public content separately from presentation correspondence. Exact automorphism classes invalidate any requirement to reproduce one arbitrarily selected concrete map.
5. Public-success semantic digests must be separated from source-bound presentation/replay digests.
6. Triangle acceptance, shell-side evidence, probes, operand occupancy, and forbidden-contact authorization must use uncertainty-complete bounded evidence and dimensionally valid clearance, not nominal predicates or generic epsilon.
7. Fresh re-ingestion must use a reviewed non-circular Component 02 operand-validation profile and compare semantic topology and shell equivalence rather than transient canonical representatives.
8. Existing Ygor verification functionality must be improved and reused where its exact-index logic is suitable, while legacy nominal/proximity geometry routines remain non-authoritative.

No correction authorizes topology repair, coordinate movement, tolerance welding, hidden unit conversion, arbitrary automorphism ranking, acceptance from a digest alone, weakened verification, external dependencies, or ordinary success before all mandatory gates pass.

## A. Corrected predecessor, type, and version contract

### A.1 Reviewed Component 14 handoff

Component 15 must consume the reviewed `assembled_output_candidate<T,I>` contract established by the Component 14 review amendment. Before authoritative verification allocation, it must validate:

- the qualified scalar descriptor equals `T`;
- the reviewed predecessor/source/public index compatibility descriptor equals `I`;
- the complete reviewed `triangulated_output_complex<T,I>` and cleaned-manifold dependency chain;
- reviewed Component 12 truth-layer, dimension, handoff-certificate, residual, obligation, codec, replay, and verifier versions;
- reviewed Component 13 truth-layer, action-certificate, dimensioned-budget, cleaned-artifact, codec, replay, and verifier versions;
- reviewed Component 14 semantic-canonicalization, correspondence-equivalence, report, provenance, codec, replay, candidate, and producer-verifier versions;
- the reviewed Component 14-to-15 handoff version;
- selected Component 15 verifier, finding, report, result, codec, replay, and self-audit versions; and
- common context owner, stable context digest, operation, policies, floating environment, and immutable lifetime.

A cross-`I` artifact, obsolete one-parameter predecessor spelling, mixed reviewed/obsolete schema graph, unsupported truth/dimension/correspondence record, stale downstream declaration, or candidate already marked finally verified must fail before large allocation.

### A.2 Pending-only candidate status

Component 14 may provide only pending claims and local assembly evidence. Component 15 must reject a candidate that already claims:

- final independent topology verification;
- `geometry.status == tolerance_checked`;
- final Component 15 pass evidence;
- a verified-result conversion token;
- a final public-success digest domain; or
- ordinary-success publication.

Component 14-local structural verification and round-trip evidence remain predecessor claims to compare.

### A.3 Evidence completeness

The intake audit must additionally require immutable access to:

- every decisive three-layer numerical record used by surviving geometry, cleanup, canonicalization, and report claims;
- every closed metric, physical dimension, dimensional derivation, and denominator certificate referenced by a tolerance-facing value;
- explicit advisory, proposed, reserved, committed, rejected, rolled-back, one-action, cumulative-lineage, local-removal, component-removal, swept/support/clearance, representation-effect, and tolerance roles;
- proof that Component 12 reserved and committed zero cleanup budget;
- Component 13 action-specific correspondence, displacement, removal, support, swept-region, clearance, reservation, and commit records;
- Component 14 semantic canonical-label evidence;
- correspondence-equivalence classes, orbit/correspondence certificates, and one source-correct concrete presentation correspondence;
- semantic/public-content domains separated from presentation/provenance/replay domains; and
- corrected producer-verifier dispositions and dependency digests.

Missing evidence must not be reconstructed from coordinates, matching counts, action summaries, transient IDs, or digests.

## B. Orthogonal numerical truth and dimensional validity

### B.1 Three distinct truth layers

For every predicate, orientation, residual, distance, support, clearance, intersection, construction, ray, and probe relation, Component 15 must preserve and audit:

- `rounded_nominal`: exact stored bits produced by the frozen rounded operation graph;
- `exact_relation`: exact algebraic sign or zero of the declared relation over stored nominal floating-point operands interpreted as exact real values, when supported; and
- `uncertainty_enclosure`: the conservative set admitted by inherited coordinate uncertainty and operation error.

These layers are not interchangeable:

- exact-relation zero does not prove every accepted realization is zero;
- exact-relation nonzero does not override an enclosure containing zero or both signs;
- rounded nominal bits are representation evidence, not geometric certainty;
- symbolic policy may resolve only a policy-authorized exact tie after eligibility is established; and
- only an enclosure excluding the prohibited disposition may establish a final bounded geometry claim for every accepted realization.

The original specification's phrases `exact-nominal sign`, `exact-nominal evidence`, and `exact-nominal tie` are read as explicit references to the appropriate three-layer records. They must not be collapsed into one enum or scalar.

### B.2 Required disposition evidence

Every mandatory geometric check must record:

- formula and provider versions;
- rounded nominal bits;
- exact-relation availability and outcome;
- uncertainty enclosure, causes, and outward-rounding lineage;
- the relation required for publication;
- whether the enclosure proves that relation;
- exact-tie eligibility and symbolic-policy record when applicable;
- final deterministic disposition;
- metric, dimension, and tolerance role where applicable; and
- diagnostic, replay, and report consumers.

If the enclosure admits outcomes that differ in orientation, embedding, contact authorization, winding, probe clearance, manifold geometry, or tolerance eligibility, ordinary success fails unless a separately specified topology-invariant rule proves all admitted outcomes have the same public semantics.

### B.3 Physical dimensions, metrics, and roles

Caller tolerance, output precision, ordinary displacement, feature-removal size, component-removal size, support deviation, and clearance are length-valued under explicitly versioned closed metrics.

Component 15 must not directly compare caller tolerance with projected/3D area, squared edge length, a differently dimensioned determinant, angle, aspect ratio, signed volume, topology count, or an untyped epsilon.

Every tolerance-facing record and comparison must identify:

- quantity role;
- closed metric;
- physical dimension;
- finite conservative interval or bound;
- derivation reference and denominator evidence where derived;
- ledger/action identity;
- semantic entity or equivalence-class witness; and
- authorization/tolerance class.

Area or squared length may contribute to a length-valued conclusion only through a reviewed Component 03 dimensional derivation with valid units and a finite strictly positive denominator enclosure. If the denominator enclosure contains zero, the derivation is unavailable.

Advisory Component 12 handoff evidence is never committed cleanup cost. Component 15 must independently derive committed maxima from Component 13 action and ledger records. Uncertainty is not movement; one-action displacement is not cumulative lineage displacement; local feature removal is not whole-component removal; rolled-back records do not contribute to success totals.

## C. Semantic canonicalization and presentation correspondence

### C.1 Distinct verification subjects

Component 15 must verify separately:

1. **semantic public canonicalization** — the canonical public component blocks, exact coordinate bits, oriented indexed triangles, occurrence/multiplicity roles, public-content bytes, and public-content digest; and
2. **presentation correspondence** — the relation between particular cleaned records and public positions used for provenance, diagnostics, and exact invocation replay.

Detailed source lineage, cleanup action identity, transient cleaned IDs, allocation history, and one arbitrary representative inside an exact automorphism class must not affect public mesh bytes or ordinary publication status.

### C.2 Exact automorphism classes

For every Component 14 `correspondence_equivalence_class` or equivalent record, Component 15 must independently validate:

- exact public-position and cleaned-member sets;
- complete normalized public-semantic payload;
- invariant normalized lineage multiset or class-level provenance;
- incidence and orientation constraints;
- orbit/correspondence certificate completeness;
- one source-correct concrete presentation correspondence;
- totality, bijectivity, owner/range correctness, and topology correctness of that correspondence; and
- proof that representative substitution inside the class cannot change semantic bytes, reports, primary semantic finding, or publication status.

Component 15 must not choose or require a unique representative by cleaned ID, array position, public position, traversal, source order, allocation history, hash order, pointer, worker schedule, or canonical-search branch order.

### C.3 Corrected bijection acceptance

The internal/public representation proof requires:

- equal cleaned/public vertex, triangle, directed-use, edge, component, and shell cardinalities;
- exact coordinate-bit preservation under the signed-zero policy;
- oriented facet equivalence up to forward cyclic rotation only;
- equal exact incidence, link cycles, components, shell roles, occurrence/multiplicity classes, and normalized lineage;
- no welding of topology-distinct equal-coordinate occurrences;
- uniquely distinguished entities to have exact forward/reverse correspondence; and
- automorphism-equivalent entities to have valid class membership and an induced concrete map belonging to the permitted correspondence set.

The original specification's map-comparison requirements therefore mean structural/class-membership comparison, not arbitrary representative equality. A different valid representative is acceptable only when all induced vertex/facet/edge/component correspondences remain consistent and semantic content is unchanged.

### C.4 Provenance under symmetry

When exact symmetry prevents unique per-position attribution, Component 15 must publish the strongest mathematically meaningful evidence:

- invariant shared lineage;
- normalized lineage multiset for the class;
- class-to-public-position and class-to-cleaned-member ranges;
- uniquely attributable lineage where available; and
- a presentation-only concrete map bound to exact invocation replay.

It must not fabricate unique semantic provenance.

## D. Corrected geometry verification

### D.1 Triangle acceptance

For each public triangle, Component 15 must:

- import the reviewed bounded vertex records and all three truth layers;
- independently evaluate orientation/area through Component 03 using a fixed expression graph;
- choose projection only through dimension-compatible conservative comparisons;
- require the uncertainty enclosure to prove the prescribed nonzero orientation;
- compare orientation with the cleaned predecessor and reconstructed shell topology; and
- require any narrow policy exception to be explicit, dimensionally valid, lineage-complete, and incapable of leaving an unresolved cleanup obligation.

Projected double area has dimension length squared. It may prove nonzero orientation when its enclosure excludes zero, but must not be compared directly with length tolerance. A definite positive thin triangle is not rejected solely because area, angle, or aspect ratio is small.

### D.2 Shell orientation and occupied side

Paired-edge propagation proves orientation consistency, not absolute occupied side. For every nonempty shell, Component 15 must establish occupied-side/nesting semantics using reviewed bounded evidence whose uncertainty excludes the opposite role. Signed volume may be retained as dimensioned diagnostic evidence but must not be compared with length tolerance or used as sole authority when uncertainty admits zero or sign change.

### D.3 Side probes

Every required probe must carry:

- an anchor definitely inside the intended triangle under its enclosure;
- a direction whose orientation relation is definite;
- a positive length-valued offset interval derived through reviewed formulas;
- lower bounds exceeding all uncertainty and representation floors required to obtain distinct samples;
- upper bounds from definite local prism, intended-support, unrelated-geometry, and remaining-policy-clearance constraints;
- proof that the probe segment meets only the intended authorized support; and
- exact coverage-class and fallback-attempt evidence.

No universal epsilon, normalized-direction tolerance, area-to-length comparison without a reviewed derivation, or first-worker-success rule is permitted. If a required region has no safe probe, ordinary success fails.

### D.4 Independent operand occupancy

Operand occupancy at probe points must be reconstructed independently from Components 09/10 using validated source topology, Component 03 bounded relations, checked signed crossing accumulation, and the frozen half-open feature-ownership policy. A symbolic tie may be used only for a proved exact eligible tie; ordinary uncertainty must try another canonical ray or fail.

### D.5 Forbidden interactions and authorization

For every independently enumerated non-adjacent public triangle pair, classify:

- definite possible support;
- enclosure-expanded possible support;
- exact topological authorization; and
- symbolic/coincident ownership authorization.

Shared-edge authorization permits only the exact shared edge and endpoints. Shared-vertex authorization permits only the exact public vertex. Separate equal-coordinate indices are topologically disjoint absent explicit lineage authorization. Accepted coincident/same-patch support requires exact owner, version, occurrence, multiplicity, orientation, and envelope evidence.

If possible support extends beyond authorization, definite excess is a forbidden interaction and unresolved excess fails closed. AABB overlap or coordinate proximity is neither a violation nor authorization.

## E. Cleanup, precision, reports, and final status

### E.1 Cleanup replay

Component 15 must replay Component 13 actions from the reviewed `triangulated_output_complex<T,I>` predecessor and verify:

- reviewed truth-layer and dimensional evidence for each action;
- certificate-to-obligation and action-to-budget reverse maps;
- advisory/proposed/reserved/committed/rejected/rolled-back role transitions;
- exact topology eligibility and occurrence splitting;
- action-specific displacement, local removal, component removal, swept/support, clearance, and uncertainty values;
- dimensional derivations and denominator preconditions;
- no unauthorized contact or new intersection; and
- final state equivalence with Component 13/14 including correspondence classes.

It must not rediscover a different cleanup sequence or promote advisory handoff evidence into committed cost.

### E.2 Precision and budget aggregation

Recompute final summaries from primitive reachable records while retaining separate maxima for:

- machine floor;
- input precision A/B;
- construction uncertainty;
- cleanup coordinate uncertainty;
- representation effect;
- one-action displacement;
- cumulative displacement per original lineage;
- local feature removal;
- whole-component removal;
- support/clearance; and
- each policy tolerance class.

Only compatible metrics and dimensions may be compared. Equal maxima use complete typed witness keys. Missing contributors, cycles, invalid units, understated bounds, role contradictions, reused reservations, or rolled-back contributions are failures.

### E.3 Reports and final status

Regenerated reports must preserve:

- all three truth layers and their dispositions;
- metric, dimension, role, derivation, and witness for every tolerance-facing summary;
- semantic canonicalization evidence separately from presentation correspondence evidence;
- class-level provenance where attribution is non-unique;
- side-probe, occupancy, forbidden-pair, cleanup, precision, re-ingestion, resource, and deterministic-execution evidence; and
- explicit pending/verified status boundaries.

Only Component 15 may assign independently verified topology status, assign `geometry.status == tolerance_checked`, finalize public-success semantic reports/digest, and create verified-result conversion authority.

## F. Digests, replay, findings, and re-ingestion

### F.1 Separate digest purposes

The result must distinguish at least:

- **public-content digest** — exact public mesh semantic content;
- **public-success semantic digest** — public content plus normalized semantic reports, policy versions, verification status, and class-level provenance/evidence;
- **artifact/evidence digest** — complete immutable verification artifact;
- **invocation replay digest** — exact source operands, options, concrete presentation correspondence, focused selectors, and source-bound replay content; and
- **failure-witness digest** — canonical rejecting evidence.

The ordinary public `digest` field refers to the public-success semantic domain, not source-presentation-dependent replay. A valid representative substitution may change presentation-only replay bytes but must not change public content, semantic reports, semantic digest, primary semantic finding, or status.

Digest equality remains only an accelerator; semantic equality, class membership, map validity, and replay compatibility require full canonical bytes or full structural comparison.

### F.2 Finding identity under symmetry

Semantic findings concerning an automorphism class use canonical class/orbit keys and invariant evidence. A presentation-map defect may include the concrete source-bound map and replay selector, but an arbitrary representative must not become the semantic arbitration key.

### F.3 Reviewed non-circular re-ingestion

Component 15 must invoke a reviewed `published_operand_reingestion` Component 02 profile equivalent to the ordinary path used when a prior result becomes a later operand, while avoiding recursion into Component 15.

The profile includes lexical/index/finiteness checks, exact directed-edge and vertex-link topology, components/shells/orientation/nesting, triangular bounded geometry, required self-interaction checks, duplicate-coordinate occurrence separation, imported precision/policy compatibility, and Component 02's local verifier. It must not repair, normalize, deduplicate, mutate orientation, reuse Component 14 round-trip state, consume Component 15 reconstructed arrays, or recursively call final-result verification.

Comparison uses exact public bits/cycles, edge/link/component/shell structure, shell occupied-side semantics, semantic canonical classes and automorphism equivalence, duplicate-coordinate separation, and imported precision no smaller than final precision. Transient Component 02 IDs and one concrete representative need not match.

## G. Existing Ygor assessment and mandatory reuse decisions

### G.1 `fv_surface_mesh<T,I>` remains the public carrier

Continue to use the exact Component 14 const read view. Do not invoke mutating convenience routines, optional-cache reconstruction, normal generation, simplification, duplicate merging, orientation repair, remeshing, zippering, or text serialization as verification providers.

### G.2 Improve and reuse `YgorMeshesVerification`

The existing `src/YgorMeshesVerification.h/.cc` contains useful exact-index and lexical foundations: finite-coordinate scanning, triangular-ring checks, index-range checks, undirected edge-key construction, edge-use counting, and directed orientation comparison.

Its current boolean API is insufficient as a publication gate because it lacks complete directed-use witnesses, reciprocal pair records, vertex-link cycles, component/shell reconstruction, duplicate-occurrence evidence, bounded geometry, dimensions, lineage, typed failures, resources, and replay. Its `TriangleIsDegenerate` path is nominal and is not a bounded acceptance provider.

Rather than discard it, improve it or factor from it a semantics-free read-only exact-index incidence utility that:

- emits complete `(from,to,facet,corner)` records;
- groups exact public indices deterministically;
- reports opposite direction and edge overuse with witnesses;
- supports exact corner-link construction without coordinate equivalence;
- uses overflow-checked counts;
- preserves existing public boolean interfaces through adapters; and
- is independently tested outside the Boolean subsystem.

Component 15 may reuse these low-level primitives only when Component 15 still owns independent grouping, link, component, evidence, finding, and arbitration logic. Legacy boolean helpers remain secondary checks or fixtures, not sole authority.

### G.3 Legacy orientation and spatial facilities

`src/YgorMeshesOrient.cc` uses proximity-based vertex representatives, epsilon comparisons, nominal normals, and nominal Möller–Trumbore intersections. Preserve its current API, but do not call it for topology identity, shell orientation, occupancy, contact authorization, or publication.

Existing BSP, R-tree, octree, cell, and bounding-box helpers may be reused only after a capability proof that they accept Component 03 conservative closed bounds, never prune on nominal comparisons, are deterministic, expose structural counters, carry no producer-owned control flow, and satisfy owner/version/resource/replay contracts. Otherwise the independently organized Component 15 hierarchy remains justified greenfield work.

Continue to reuse Component 01 checked arithmetic, IDs, errors, transactions, resources, cancellation, canonical bytes, SHA-256, replay, and arbitration; Component 03 bounded arithmetic and dimensioned records; and immutable predecessor query views. Do not duplicate those services.

## H. Required review tests and mutations

The Component 15 test plan must add or strengthen:

- mixed reviewed/obsolete Component 12/13/14/15 schema rejection;
- cross-`I` and mismatched-`T` handoff rejection;
- truth-layer mutations where rounded nominal, exact relation, and enclosure disagree;
- false acceptance using exact-relation sign while the enclosure crosses zero;
- metric/dimension/role mutations with corrected counts and digests;
- area-versus-length, squared-length-versus-length, volume-versus-length, hidden-unit-conversion, and zero-denominator derivation mutations;
- advisory-to-committed cleanup promotion and rolled-back-cost mutations;
- exact automorphism cases with several valid presentation correspondences;
- invalid class membership, incomplete orbit certificate, inconsistent induced facet/edge map, and transient-rank mutations;
- semantic-digest stability under representative substitution and source presentation permutation;
- replay source binding without contamination of public-success digest;
- class-level provenance and finding-arbitration tests;
- dimensioned probe-offset and no-safe-probe tests;
- uncertainty extending beyond shared-edge/shared-vertex/contact authorization tests;
- re-ingestion automorphism-equivalence and non-circularity tests;
- improved `YgorMeshesVerification` exact-index utility tests, including bow-ties that edge counts alone miss;
- matched producer/verifier mutations proving low-level shared primitives do not collapse high-level independence; and
- scaling metamorphics applying the correct power of scale to length, area, squared length, and volume evidence.

Every required mutation must be rejected by the intended independent check. Zero survivors remains mandatory.

## I. Planning-review completion

`tracker.md` records completion of this planning and independent-review step, not implementation completion. Component 15 may be marked complete after this amendment and `plan_15_verification_diagnostics_replay_review_amendment.md` are committed and mutually consistent with `broad_plan.md`, the reviewed Components 13 and 14 contracts, Component 16 qualification infrastructure, and Component 17 deterministic execution services.

The implementation definition of done in the original specification, as amended above, remains a future handoff gate and is not a prerequisite for marking the planning-review task complete.
