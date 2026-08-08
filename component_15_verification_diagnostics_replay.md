# Component 15: Independent Verification, Diagnostics, and Replay

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete verifier decomposition, independent acceleration structure, local topology tables, side-probe construction, report representation, diagnostic schema, replay container, logical encoding, and digest implementation may change. Independent reconstruction of authoritative facts, the mandatory publication gates, deterministic finding arbitration, fail-closed behavior, replay sufficiency, and the input/output contracts in this document are normative.

## 0. Purpose

This component is the final publication authority for the Boolean pipeline. It accepts the immutable `assembled_output_candidate<T, I>` from Component 14 together with the predecessor artifacts and evidence required by the frozen verification policy, independently reconstructs the facts needed to judge the result, and either:

- publishes one ordinary `bounded_boolean_success<T, I>` whose topology and bounded geometry have passed every mandatory gate; or
- returns one deterministic typed failure and a replayable diagnostic record without exposing the candidate through the ordinary success type.

Its purposes are to:

- independently reconstruct public-mesh indices, edge uses, halfedge pairing, vertex links, connected components, shell orientation, and triangle incidence;
- verify the bijection between the cleaned internal manifold and the assembled public mesh without trusting Component 14 maps as the sole source of truth;
- verify that canonical intersection events and output occurrences are shared, separated, and propagated consistently with their lineage;
- verify classification, winding transitions, Boolean selection, orientation, multiplicity, and coincident-surface ownership across the retained result;
- verify construction residuals, precision envelopes, cleanup displacements, removed-feature costs, and cumulative tolerance use;
- detect forbidden non-adjacent triangle interactions using an independently implemented conservative search and bounded relation path;
- verify deterministic canonical content, logical serialization, report coverage, digests, and replay metadata;
- re-ingest the candidate through the public input contract and compare the reconstructed solid semantics with the requested operation;
- produce deterministic findings whose primary error does not depend on traversal order, hash order, task partition, or worker schedule; and
- finalize result status only after all required checks succeed.

This component verifies and publishes. It does not repair topology, move coordinates, change triangulation, weld occurrences, remove components, alter classification, reinterpret tolerance, choose a different symbolic policy, or canonicalize a rejected candidate into acceptance.

The principal successful artifact is an immutable `verified_boolean_result<T, I>` or equivalent publication record containing the public mesh, final reports, deterministic digests, verification evidence, and replay metadata. The ordinary public success wrapper may be a view or move-out of that artifact, but it must not omit any required status or bound.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `assembled_output_candidate<T, I>` from Component 14;
- immutable access to the cleaned triangle manifold and cleanup certificates from Component 13;
- immutable access to polygonal and triangulated output artifacts from Components 11 and 12 when required by the selected verification level;
- immutable retained-use, multiplicity, orientation, and occurrence-accounting artifacts from Component 10;
- immutable classification groups, quotient graphs, winding values, and side labels from Component 09;
- immutable intersection-event, carrier-order, cut/contact, and construction-lineage artifacts from Component 08;
- immutable signed feature relations, crossing multiplicities, and symbolic decisions from Component 07;
- immutable canonical topology, source-triangle, source-facet, shell, and operand artifacts from Components 02, 04, and 05;
- the immutable precision context, bounded arithmetic services, and complete precision ledger from Component 03;
- the immutable Boolean context, truth tables, policy versions, deterministic comparators, resource services, cancellation, diagnostics, transactions, and replay services from Component 01;
- the requested verification level, including the mandatory publication floor;
- the selected verifier, diagnostic, report, logical-serialization, and replay schema versions; and
- any test-only exhaustive-oracle services explicitly enabled by Component 16 for bounded qualification cases.

The component must not read mutable caller meshes, mutable predecessor storage, producer scratch buffers, allocator addresses, unordered iteration state, wall-clock time, or worker-completion order.

### 1.2 Required predecessor guarantees

The component may rely on every predecessor artifact having been transactionally published and locally verified by its producing stage. It may rely on Component 14 providing:

- a fully assembled canonical public `fv_surface_mesh<T, I>` candidate;
- exact public coordinate bit records;
- internal/public maps claimed to be bijective;
- canonical component, vertex, and facet ordering;
- cleaned-edge to public directed-use mappings;
- candidate precision, cleanup, topology, geometry, provenance, resource, and determinism reports;
- canonical logical bytes or a reproducible encoder;
- candidate digests;
- structural round-trip evidence; and
- immutable references to all verification-required predecessor artifacts.

These are claims to verify, not facts that may be accepted solely because Component 14 recorded them. The component must defensively validate owner tokens, artifact versions, context identity, entity ranges, digest domains, report references, and predecessor dependency links before using any claim.

A contradiction in a committed predecessor artifact is an `internal_invariant_error`. A candidate whose geometry cannot be accepted within the frozen tolerance is `result_geometry_not_validated` or another more specific expected geometric failure. The verifier must not convert expected numerical or tolerance difficulty into `internal_invariant_error` merely because a producer believed the candidate was acceptable.

### 1.3 Verification-level contract

Verification policy may define several levels, but every ordinary success must include a non-disableable mandatory floor.

A conforming policy may distinguish:

- **mandatory publication verification**, scalable and always required;
- **extended verification**, adding denser probes, broader cross-artifact checks, and more expensive independent reconstruction;
- **diagnostic verification**, retaining additional non-authoritative findings and witnesses; and
- **test-only exhaustive verification**, available only for bounded fixtures under Component 16.

No option may disable:

- public index and finiteness checks;
- edge-use and vertex-link manifold checks;
- orientation checks;
- accepted-triangle nondegeneracy checks;
- precision and cleanup-budget checks;
- forbidden-intersection checks required by the publication contract;
- Boolean side-consistency checks required by the publication contract;
- public round-trip and re-ingestion checks;
- deterministic digest verification; or
- fail-closed publication.

A lower verification level may reduce optional diagnostics or sampling density only where the contract explicitly permits it. It must not weaken the ordinary-success semantics.

### 1.4 Independence contract

The verifier must be independently implemented from the producers of the facts it checks.

Independence requires, at minimum:

- reconstructing directed edge uses from public facets rather than trusting Component 14 edge maps;
- reconstructing vertex links from incident public face corners rather than trusting stored link cycles;
- reconstructing connected components from public adjacency rather than trusting component labels;
- recomputing report maxima from ledger entries rather than trusting producer summaries;
- checking cleanup action effects from before/after certificates rather than trusting cumulative counters alone;
- constructing an independent conservative triangle-interaction search rather than invoking Component 06 as the sole verifier;
- using an independently organized bounded relation path for final forbidden-intersection decisions rather than replaying Component 07 control flow verbatim;
- recomputing selected Boolean occupancy witnesses rather than accepting Component 09/10 labels without geometric evidence;
- regenerating canonical logical bytes from the documented encoding contract rather than copying Component 14 byte storage; and
- comparing full structural content when a digest is used as an accelerator.

The verifier may reuse foundational, audited primitives whose observable semantics are part of Component 03, such as checked integer arithmetic, scalar bit encoding, interval containment, and bounded arithmetic operations. It must not reuse a producer helper in a way that makes the verifier repeat the same grouping, pairing, ordering, or aggregation mistake.

For every shared primitive, the verification report must identify the primitive version and the independently reconstructed higher-level fact that was checked.

### 1.5 Evidence-completeness contract

The input handoff must contain enough immutable evidence to trace every public entity back through the pipeline.

At minimum:

- every public vertex must trace to one cleaned vertex occurrence;
- every cleaned vertex occurrence must trace to retained source lineage, an intersection event, or an authorized cleanup construction;
- every public facet must trace to one cleaned output triangle;
- every cleaned output triangle must trace to one Component 12 triangle and one Component 11 retained region;
- every retained region must trace to Component 10 selection and occurrence accounting;
- every event-derived coordinate must trace to one Component 08 event and one Component 07 canonical relation;
- every cleanup-modified coordinate or topology relation must trace to one Component 13 action certificate;
- every precision contributor must have one ledger identity and conservative value;
- every removed feature or component must have an authorization record; and
- every report summary must enumerate or digest the records it summarizes.

Missing evidence is a verification failure. The verifier must not infer absent lineage from coordinate equality, source proximity, matching counts, or a digest alone.

### 1.6 Precision and tolerance input requirements

The verifier must receive the frozen distinctions among:

- machine roundoff floor;
- operand input precision;
- construction uncertainty;
- cleanup displacement;
- removed-feature size or topology-change cost;
- output precision; and
- user-authorized tolerance.

The component must be able to recompute or conservatively re-aggregate:

- the maximum inherited input precision;
- every published construction envelope used by an output vertex;
- every cleanup displacement and cumulative displacement path;
- every authorized removed-feature cost;
- the candidate output precision;
- the maximum realized displacement;
- the maximum authorized tolerance; and
- deterministic witnesses for all equal maxima.

The verifier must reject a report that merges these quantities into one undifferentiated epsilon or that claims a smaller bound than any contributing record.

### 1.7 Diagnostic finding contract

Every detected discrepancy or noteworthy accepted boundary condition must be representable as a versioned `verification_finding` or equivalent record containing:

- stable finding identity;
- severity and publication effect;
- component and stage;
- stable subcode;
- canonical public and predecessor entity references;
- operation and policy versions;
- nominal numerical witnesses and conservative bounds;
- tolerance and remaining budget;
- expected fact and reconstructed fact;
- logical serialization of the witness;
- optional local topology or geometry excerpt;
- deterministic human-readable summary; and
- replay reference.

Finding identity and ordering must not depend on discovery order. Equivalent findings discovered by several checks may be coalesced only by a canonical evidence key; coordinate proximity is not an identity key.

### 1.8 Replay input requirements

The verifier must receive or be able to regenerate a replay payload containing:

- exact source coordinate bits and indices;
- concrete `T` and `I` descriptors;
- normalized options and operation;
- all artifact and policy versions;
- platform and floating-environment qualification identifiers;
- resource ceilings and execution policy;
- canonical input digest;
- predecessor artifact digests required to locate the failure;
- candidate public mesh and report digests;
- deterministic primary finding; and
- sufficient stage-selection information to rerun full or focused verification.

A focused replay may omit large predecessor bodies only when the canonical source input and frozen options are sufficient to reproduce them byte-for-byte. Otherwise, the required artifact content must be embedded or referenced by a stable content-addressed record under the in-tree replay format.

### 1.9 Capacity, resource, and lifetime preconditions

Before verification begins, the component must validate that it can account for:

- public vertices, facets, corners, directed edge uses, undirected edges, and link arcs;
- independent component and shell reconstruction;
- independent broad-phase nodes and candidate triangle pairs;
- bounded relation work and side probes;
- lineage, cleanup, and precision ledgers;
- report-regeneration tables;
- diagnostic findings and witnesses;
- logical serialization and digest state;
- re-ingestion artifacts;
- replay storage;
- temporary and persistent bytes; and
- abstract verification work units.

All count arithmetic must be overflow-checked. The candidate and all predecessor references must remain immutable and valid until verification completes, all workers join, and either publication commits or rollback finishes.

## 2. Required behavior

### 2.1 Verification transaction

The component must execute in one private final-stage transaction.

The transaction must contain:

- independent public-topology reconstruction storage;
- independent internal/public equivalence-check storage;
- independent geometry-search and relation workspace;
- side-classification probe workspace;
- precision and cleanup re-aggregation state;
- report and diagnostic builders;
- logical serialization and digest state;
- re-ingestion workspace;
- replay finalization state; and
- proposed immutable success storage.

No final report, `tolerance_checked` status, success digest, or ordinary public result may become visible before every mandatory gate succeeds. Failure or cancellation must roll back the proposed success while preserving immutable predecessor artifacts according to diagnostic policy.

### 2.2 Candidate intake and dependency audit

The component must first validate the complete dependency graph of the candidate.

It must check:

- context owner tokens and operand identities;
- artifact schema and policy versions;
- predecessor digest references;
- absence of stale or cross-context IDs;
- entity range and count consistency;
- public type descriptors;
- verification-level validity;
- required evidence availability;
- report status values, including that Component 14 has not already declared final geometric success;
- canonical logical-serialization version; and
- replay schema compatibility.

Unknown required versions must fail cleanly. Optional unknown diagnostic fields may be skipped only when the enclosing version contract permits length-delimited forward compatibility and no mandatory check depends on them.

### 2.3 Public mesh lexical audit

Before constructing topology, the verifier must audit the public mesh representation itself.

It must verify:

- the mesh is readable through the supported in-tree adapter;
- vertex and facet counts equal the candidate's declared counts;
- every coordinate scalar is finite;
- every coordinate bit pattern equals the corresponding candidate bit record;
- every facet has exactly three indices under the v1 policy;
- every index is representable, non-sentinel, and less than the vertex count;
- every triangle has three distinct indices;
- no unexpected adapter-side deduplication, reordering, normalization, or narrowing occurred; and
- empty output has no isolated public vertices or facets.

This audit must not merge equal coordinates or classify topology from coordinate bits.

### 2.4 Independent directed-edge reconstruction

The verifier must emit three directed edge-use records from every public triangle, preserving facet orientation and corner position.

It must then:

- canonicalize each undirected endpoint pair by public index only;
- group uses by exact endpoint pair;
- require exactly two uses per undirected edge;
- require opposite direction;
- derive reciprocal pair records independently;
- detect repeated directed uses, missing reverse uses, and three-or-more-use edges;
- retain triangle and corner witnesses for diagnostics; and
- compare the reconstructed edge set with, but not derive it from, Component 13/14 edge maps.

Coordinate-equal vertices with different public indices remain different endpoints.

### 2.5 Independent vertex-link reconstruction

For each public vertex occurrence, the verifier must reconstruct the incident face-corner link from the public facet array and reconstructed edge pairing.

The reconstruction must:

- include every incident corner exactly once;
- connect adjacent corners through the two paired edges incident to the vertex;
- require one closed cycle for a non-isolated manifold vertex;
- reject several cycles, open chains, repeated corners, and inconsistent pair transitions;
- handle arbitrary valence;
- preserve separate links for coordinate-coincident public indices; and
- compare reconstructed valence and cyclic incidence with cleaned-manifold evidence.

A bow-tie vertex must be rejected even when every undirected edge has exactly two uses.

### 2.6 Connected-component and shell reconstruction

The verifier must reconstruct connected components by traversing triangle adjacency through reciprocal paired edges.

It must:

- assign every triangle to exactly one component;
- include every used public vertex and edge in the corresponding component;
- keep point-touching and edge-touching topologically separate components separate when public indices separate them;
- detect isolated vertices or edges;
- compute component counts and canonical member-set signatures;
- compare them with Component 13 topology-change and Component 14 component reports; and
- identify orientation-connected shells needed by the solid contract.

The verifier must not merge components because their coordinate sets overlap or are identical.

### 2.7 Internal/public bijection verification

The verifier must independently verify the Component 14 claim that assembly was a bijective representation change.

It must establish:

- public vertex count equals cleaned vertex-occurrence count;
- public facet count equals cleaned triangle count;
- every public vertex maps to exactly one cleaned occurrence;
- every cleaned occurrence maps to exactly one public vertex;
- every public facet maps to exactly one cleaned triangle up to cyclic rotation preserving orientation;
- every cleaned triangle maps to exactly one public facet;
- public coordinate bits equal the cleaned nominal `T` values under the frozen signed-zero policy;
- public edge and vertex-link incidence equals cleaned incidence; and
- topology-distinct duplicate coordinates were not welded.

Candidate maps may accelerate lookup, but the verifier must validate them against independently derived structural descriptors and full incidence comparisons. A map permutation with matching counts is not sufficient.

### 2.8 Triangle orientation and accepted nondegeneracy

For every public triangle, the verifier must evaluate an independent bounded orientation/area certificate in a deterministic projection or 3D normal formulation.

It must verify:

- the three public indices are distinct;
- the bounded area or orientation is definitely compatible with the accepted nondegeneracy policy;
- the triangle orientation agrees with its cleaned predecessor and shell orientation;
- no coordinate-copy or canonical-rotation step inverted the triangle;
- triangles accepted under a narrow degeneracy exception carry the exact policy authorization and cleanup provenance; and
- no unresolved zero-area or ambiguous-orientation triangle remains in ordinary success.

A triangle whose orientation cannot be established within the available precision and tolerance must cause `result_geometry_not_validated` or `geometric_condition_exceeds_tolerance`, not heuristic acceptance.

### 2.9 Shell orientation and solid-side consistency

The verifier must confirm consistent orientation on every reconstructed connected shell.

It must:

- propagate orientation through paired edges;
- verify every adjacent triangle induces opposite direction on the shared edge;
- compute deterministic bounded orientation evidence for each shell when needed;
- compare shell orientation and nesting roles with the requested output solid policy;
- reject a shell whose occupied side cannot be established within the published precision; and
- verify that cavities and islands alternate occupancy as required.

For empty output, the occupied region is empty and no shell-orientation witness is required.

### 2.10 Event sharing and occurrence separation audit

For every event-derived public vertex, edge, or face boundary, the verifier must check lineage consistency across Components 07, 08, 10, 11, 12, 13, and 14.

It must verify:

- one canonical event identity has one nominal coordinate and one precision envelope;
- all consumers claiming the same event use the same constructed coordinate record;
- no canonical relation was duplicated into inconsistent event records;
- distinct event identities remain distinct even when nominal coordinates match;
- source-edge and carrier event ordering agrees with the immutable ordering records within their published bounds;
- occurrence duplication required by multiplicity or topology separation remains explicit; and
- cleanup did not weld unrelated event or source occurrences solely because they were near.

The verifier must distinguish event identity, output occurrence identity, and public vertex index. Equality in one domain does not imply equality in another.

### 2.11 Winding and classification consistency audit

The verifier must validate the cross-artifact classification logic without simply replaying Component 09's grouping implementation.

It must check:

- zero-delta connectivity traverses only exact uncut topology;
- no classification group crosses a cut or nonzero signed-delta relation;
- signed crossing multiplicities agree with canonical Component 07 relations;
- quotient-graph edge deltas are antisymmetric and cycle-consistent;
- seed winding values satisfy the validated operand shell semantics;
- propagated winding values remain in the supported domain under the selected solid policy; and
- every retained surface atom has consistent side labels.

Extended or test-only verification should reconstruct classification groups from primitive cut and relation evidence using an independently organized algorithm and compare the full partition and winding assignment.

### 2.12 Boolean selection, orientation, and multiplicity audit

For every retained public triangle or its authoritative retained-region ancestor, the verifier must apply the frozen Component 01 truth table to the reconstructed side occupancy.

It must verify:

- the surface is retained exactly when the operation changes occupancy across it;
- its orientation points from occupied result to unoccupied result according to the public convention;
- operand exchange and directed-difference remapping were applied correctly;
- coincident same- and opposite-orientation surfaces follow the frozen symbolic ownership policy;
- internal two-sided surfaces are absent;
- required multiplicity is represented by separate manifold occurrences; and
- point-touching and edge-touching regularized results do not gain a false occupied connection.

A triangle that is topologically manifold but inconsistent with the Boolean truth table must be rejected.

### 2.13 Deterministic side-probe verification

The verifier must perform bounded side probes around the output surface to obtain geometric evidence that the published orientation and Boolean occupancy agree.

The probe policy must be versioned and deterministic. It must define:

- which triangles or retained patches receive probes;
- canonical probe locations, including fallback locations when centroids are poorly conditioned;
- an offset magnitude derived from local precision, feature separation, and available tolerance rather than a universal epsilon;
- how probes avoid crossing unrelated nearby surfaces;
- how operand occupancy is evaluated independently;
- how exact ties and contacts are interpreted under the symbolic policy; and
- when uncertainty requires denser probing or failure.

The mandatory floor must cover every output connected component, every distinct provenance/selection class, every topology-change region, every coincident-ownership class, every extremal precision witness region, and every region implicated by nearby non-adjacent geometry. A policy may probe every triangle.

For each probe pair, the verifier must confirm:

- one side is occupied by the requested result and the other is unoccupied;
- the transition direction agrees with triangle orientation;
- the result occupancy equals the frozen Boolean operation applied to independently evaluated operand occupancy; and
- the probe segment does not pass through an unresolved third surface within its uncertainty envelope.

If no safe probe can be constructed for a required region, ordinary success must fail rather than silently skip the check.

### 2.14 Independent forbidden-intersection search

The verifier must detect forbidden interactions among non-adjacent output triangles.

It must construct an independent conservative spatial search over public triangles. The provider may use an in-tree BVH, spatial hierarchy, sweep, uniform partition, or exhaustive search for bounded cases, but it must satisfy:

- bounds are inflated by both triangles' precision envelopes and any required verification margin;
- no interaction that could violate the output contract is omitted;
- adjacency exclusions are based only on exact public topology and accepted same-patch relations;
- false positives are allowed;
- candidate identity and processing order are deterministic; and
- structural counters reveal accidental all-pairs behavior on disjoint large inputs.

The verifier must classify candidate triangle pairs with an independently organized bounded relation path. It must detect at least:

- proper transverse intersections;
- non-authorized coplanar overlap;
- a vertex penetrating a non-incident triangle;
- edge-face and edge-edge crossings;
- non-manifold geometric overlap between topologically unrelated sheets; and
- cleanup-introduced interactions outside accepted uncertainty envelopes.

Permitted contact must be justified by explicit topology, symbolic ownership, duplicate-occurrence policy, or a documented tolerance-aware acceptance rule. Geometric proximity alone is neither a violation nor an authorization.

When the bounded relation cannot separate an allowed configuration from a forbidden one within the available tolerance, the component must fail closed.

### 2.15 Construction residual and envelope verification

For every output coordinate with constructed lineage, the verifier must independently check the published residual and enclosure claims.

Checks must include, as applicable:

- source-edge parameter containment;
- source-face plane residual;
- barycentric or projected containment;
- carrier consistency;
- equality of all consumers of one canonical event record;
- finite nominal coordinate and finite non-negative bounds;
- bound dominance over inherited operand uncertainty and arithmetic roundoff;
- declared conditioning category; and
- consistency between the stored coordinate and the versioned rounding policy.

The verifier need not recompute an alternative nominal coordinate. It must establish that the published nominal value lies within a conservative admissible region and that the claimed envelope is not understated.

A construction whose required enclosure exceeds available tolerance must not be accepted because later topology is valid.

### 2.16 Cleanup certificate and budget verification

The verifier must replay or independently check every Component 13 cleanup action certificate in canonical action order.

For each action, it must verify:

- the referenced before-state entities existed and matched the certificate;
- exact topological eligibility conditions held;
- required link conditions and occurrence splitting were satisfied;
- the after-state local topology matches the recorded mutation;
- no unrelated feature was welded by coordinate proximity;
- moved coordinates remain within the action's reserved local budget;
- displacement is measured against the correct original or predecessor reference;
- precision envelopes were updated monotonically;
- removed-feature or component authorization exists;
- topology changes match the reported component/genus/cavity delta; and
- local no-new-intersection evidence is complete.

The verifier must recompute cumulative displacement paths and feature-removal maxima from individual actions. It must reject double-count omission, missing actions, reused budget, understated maxima, or a topology change not covered by policy.

### 2.17 Precision-ledger and tolerance aggregation

The verifier must recompute final precision and tolerance summaries from primitive ledger entries.

It must establish:

```text
final_output_precision >= every inherited input precision contributor
final_output_precision >= every used construction uncertainty
final_output_precision >= every cleanup coordinate uncertainty
maximum_realized_displacement >= every realized cleanup displacement path
reported_removed_feature_cost >= every authorized removed feature/component witness
ordinary_success requires every policy-bounded quantity <= authorized tolerance
```

Equal maxima must select the same witness under a frozen canonical tie rule. NaN, negative bounds, missing contributors, non-monotonic entries, overflowed counts, and unversioned units are failures.

The verifier must keep numerical uncertainty, geometric displacement, and feature-removal authorization distinct in the final reports.

### 2.18 Report completeness and cross-report consistency

The component must rebuild or audit all final reports.

It must verify consistency among:

- topology counts and reconstructed public topology;
- component/genus/cavity changes and cleanup certificates;
- geometry status and completed checks;
- precision maxima and ledger witnesses;
- cleanup totals and action records;
- provenance coverage and public entity counts;
- resource counters and stage reservations;
- determinism versions and canonical logical bytes;
- replay metadata and candidate digests; and
- verification findings and primary status.

No final report may claim a stronger status than the evidence supports. In particular, `geometry.status == tolerance_checked` may be assigned only by this component after every mandatory check succeeds.

### 2.19 Canonical serialization and digest verification

The verifier must independently regenerate canonical logical bytes from the public mesh, normalized final reports, policy versions, and the documented domain-separation rules.

It must verify:

- fixed byte order and scalar bit encoding;
- integer width or varint framing rules;
- signed-zero policy;
- sequence lengths and overflow checks;
- exclusion of native padding, pointers, allocator state, locale, and unordered iteration;
- public-content, artifact, provenance, replay, and aggregate digest domain separation;
- candidate digest equality where the candidate is expected to remain unchanged; and
- final success digest incorporation of Component 15 status and evidence.

Digest equality may accelerate comparison but never replaces structural verification. On an injected or detected digest collision, full canonical bytes and structural content remain authoritative.

### 2.20 Re-ingestion verification

The verifier must pass the assembled public mesh and published precision metadata through the structural and solid-semantics portions of the Component 02 input contract using a fresh verification transaction.

Re-ingestion must confirm:

- valid indices and finite coordinates;
- exactly two opposite uses per undirected edge;
- one cyclic link per public vertex occurrence;
- consistent orientation;
- recognized disconnected-shell, cavity, and island semantics;
- no accidental coordinate-based welding;
- accepted planarity for triangular facets;
- output precision metadata is sufficient for the reconstructed coordinates; and
- the public mesh can serve as an operand in a subsequent Boolean call under the declared contract.

The re-ingested topology must be structurally equivalent to the independently reconstructed Component 15 topology. Re-ingestion must not silently normalize or repair the candidate.

### 2.21 Deterministic finding arbitration

All checks may emit findings into private task-local buffers. The component must choose the externally visible primary failure using the frozen Component 01 total ordering.

The arbitration key must include enough information to distinguish:

- stage and check class;
- severity/publication effect;
- canonical component, vertex, edge, facet, event, action, or ledger witness;
- stable subcode;
- policy version; and
- deterministic numerical witness encoding.

Worker timing, discovery order, container order, and pointer values are prohibited tie-breaks. Additional findings may be included in deterministic sorted order subject to diagnostic resource limits. Truncation must be explicit and must not remove the primary finding.

### 2.22 Diagnostic localization

For each rejecting finding, diagnostics should include the smallest deterministic local witness sufficient to explain the failure, such as:

- one malformed public facet;
- one undirected edge and its incident uses;
- one vertex link cycle or set of cycles;
- one forbidden triangle pair;
- one event and its inconsistent consumers;
- one quotient-graph cycle;
- one retained triangle and side-probe pair;
- one cleanup action before/after neighborhood;
- one precision maximum path; or
- one serialization field mismatch.

Localization must preserve exact IDs and scalar bits. Human-readable decimal formatting is supplemental and must not be the replay authority.

### 2.23 Replay finalization

On success or failure, the component must finalize a canonical replay record according to diagnostic policy.

The record must identify:

- exact source operands and normalized options or a content-addressed reference to them;
- all relevant versions;
- platform qualification;
- resource and execution settings;
- selected verification level;
- candidate and predecessor digests;
- primary status and finding;
- final public-content or failure-witness digest;
- canonical progress counters; and
- any focused verification selector needed to reproduce the local failure.

Replay decoding must validate lengths, counts, types, versions, and digests before allocation or execution. Malformed replay input must return a typed input/replay error in the test or diagnostic API and must never cause undefined behavior.

### 2.24 Success publication

Only after every mandatory gate succeeds may the transaction construct and commit ordinary success.

Publication must:

- preserve the exact public mesh assembled by Component 14;
- set final topology status to independently verified;
- set geometry status to `tolerance_checked`;
- install independently verified output precision, tolerance, and maximum-displacement summaries;
- finalize topology, geometry, cleanup, topology-change, provenance, resource, determinism, and verification reports;
- finalize canonical logical serialization and all digest domains;
- attach replay metadata according to policy;
- ensure the result is immutable or uniquely owned according to the public API; and
- atomically expose one complete `bounded_boolean_success<T, I>`.

The verifier must not modify the mesh to make it pass. Any required change returns control to an earlier development cycle; it is not performed inside final verification.

### 2.25 Failure publication

On any rejecting finding, resource exhaustion, or cancellation:

- no ordinary success is published;
- `geometry.status` is not exposed as `tolerance_checked`;
- all workers join before rollback;
- temporary reservations return;
- the deterministic primary error is finalized;
- diagnostic artifacts are retained only according to policy;
- the assembled candidate remains internal and immutable; and
- replay information is returned or stored according to policy.

Expected result-geometry failure must normally use `result_geometry_not_validated`, `geometric_condition_exceeds_tolerance`, `cleanup_budget_exceeded`, or another specific typed category. `internal_invariant_error` is reserved for contradictions in committed artifacts or verifier implementation invariants.

### 2.26 Deterministic parallel verification

Independent checks may run concurrently when they consume immutable artifacts and produce private outputs.

Permitted parallel work may include:

- public edge-use extraction by facet ranges;
- component-local link verification after deterministic ownership assignment;
- triangle nondegeneracy checks;
- independent broad-phase build and candidate classification;
- event-lineage audits by canonical event ranges;
- cleanup certificate checks by independent non-overlapping evidence groups;
- precision-ledger aggregation in deterministic reduction trees; and
- report block generation.

Final edge grouping, cross-task topology consistency, finding arbitration, report finalization, digest encoding, and publication must merge by full canonical keys. Thread count must not change the result, primary failure, complete retained finding set within limits, reports, replay bytes, or digests.

### 2.27 Resource limits and work accounting

The component must account separately for:

- public topology reconstruction;
- internal/public equivalence work;
- link and component traversal;
- independent broad-phase nodes and candidates;
- bounded pair-relation work;
- side probes and operand occupancy queries;
- event/classification/selection lineage checks;
- cleanup replay work;
- precision aggregation;
- report and finding storage;
- serialization and digest work;
- re-ingestion;
- replay bytes;
- temporary bytes;
- persistent success bytes; and
- abstract work units.

Crossing a configured hard limit must return `resource_limit` before publication. A verifier must not skip a mandatory check because its advisory performance target was exceeded.

### 2.28 Cancellation and exception safety

Cancellation must be polled at deterministic safe points during every potentially long verification phase.

On cancellation:

- no new verification subtask may commit output;
- active workers stop at safe points and join;
- final publication is prohibited;
- private candidate-success storage is destroyed;
- all resource reservations return; and
- the result is `cancelled` with the latest completed canonical phase and counters.

All operations must provide strong transaction-level exception safety. Allocation failure maps to the appropriate typed resource failure. No exception may escape across the public Boolean API unless the frozen public contract explicitly defines that behavior.

### 2.29 Independent verification evidence

The final artifact must contain enough compact evidence to audit that each mandatory gate ran and passed.

Evidence must identify:

- verifier and check versions;
- checked entity counts;
- reconstructed topology digests;
- independent broad-phase and pair-check counters;
- side-probe coverage classes and witnesses;
- precision and cleanup maximum witnesses;
- re-ingestion digest;
- report-regeneration digest;
- logical-serialization digest; and
- any accepted policy exceptions.

Evidence is not a substitute for the checks. It is a deterministic record that the checks occurred under the stated versions and input content.

### 2.30 Test-only exhaustive verification path

For bounded fixtures, Component 16 must be able to invoke a deliberately slow exhaustive path that:

- compares all non-adjacent triangle pairs;
- reconstructs all edge and vertex-link facts without acceleration;
- compares canonical mappings against exhaustive bounded permutations where applicable;
- evaluates low-complexity predicates through the in-tree exact oracle;
- enumerates alternative side-probe witnesses when practical;
- recomputes winding on bounded exact templates; and
- compares all scalable-verifier conclusions with exhaustive results.

Production ordinary success must not depend on the test-only exact oracle. Disagreement in qualification is a test failure and permanent regression candidate.

## 3. Output contract

On ordinary success, the component must produce one immutable `verified_boolean_result<T, I>` or equivalent artifact containing or referencing:

- the canonical public `fv_surface_mesh<T, I>`;
- final output precision;
- maximum authorized tolerance;
- maximum realized displacement;
- independently verified topology report;
- `tolerance_checked` geometry report;
- cleanup and topology-change reports;
- complete provenance report;
- resource and deterministic-execution report;
- verification report and compact pass evidence;
- canonical logical serialization or reproducible encoder state;
- public-content, artifact, provenance, replay, and final aggregate digests;
- final replay metadata; and
- the immutable context and version references required to interpret the result.

The artifact must guarantee:

- all public indices are valid and coordinates are finite;
- every undirected public edge has exactly two opposite uses;
- every public vertex occurrence has one cyclic incident face fan;
- every public triangle has three distinct indices and accepted nonzero oriented area;
- connected components and shell orientation satisfy the public solid policy;
- the public mesh is bijective with the cleaned internal manifold;
- event sharing and topology-distinct duplicate occurrences are preserved;
- classification, Boolean selection, orientation, multiplicity, and coincident ownership are consistent with the frozen operation and symbolic policy;
- no forbidden non-adjacent triangle interaction was found outside accepted uncertainty envelopes;
- construction residuals and precision envelopes are conservative;
- cleanup actions and topology changes are authorized and within budget;
- final precision and displacement reports dominate every contributor and remain within ordinary-success limits;
- public re-ingestion succeeds under the same solid and precision contract;
- canonical logical bytes and digests are reproducible; and
- result status is independent of thread count and schedule.

On failure, no `verified_boolean_result<T, I>` or ordinary `bounded_boolean_success<T, I>` is published. The component must produce one deterministic typed error containing the primary finding, relevant canonical witnesses, numerical bounds, policy versions, resource counters, and replay information.

## 4. Required invariants and prohibited behavior

Required invariants:

- final verification is independent of producer-owned topology, grouping, aggregation, and search control flow;
- every mandatory publication gate is non-disableable;
- public topology is reconstructed from exact indices, never coordinates;
- coordinate-equal topological occurrences remain distinct unless exact identity says otherwise;
- every accepted geometric statement is supported by bounded evidence under the frozen precision model;
- tolerance is audited as a budget, not used as a universal equality predicate;
- every rejecting condition selects the same primary finding under every schedule;
- all report maxima and status fields are independently justified;
- digest equality never replaces structural comparison;
- a rejected candidate is never repaired or exposed as ordinary success;
- success publication is atomic and transactional;
- replay bytes are canonical and sufficient for deterministic reproduction; and
- production and normative tests remain portable C++17 with no external dependency.

Prohibited behavior:

- trusting Component 14 maps, counts, digests, or round-trip evidence as the sole verification;
- rebuilding topology by coordinate sorting, snapping, or tolerance-based welding;
- skipping a mandatory triangle pair because a producer broad phase omitted it;
- treating adjacency by geometric proximity rather than exact public indices;
- accepting an ambiguous side probe without escalation or failure;
- reusing Component 09/10 classifications without independent occupancy evidence;
- under-reporting precision, cleanup displacement, or removed-feature cost;
- changing triangle orientation, indices, coordinates, topology, or reports to make a candidate pass;
- accepting a digest match when full content disagrees;
- choosing the first worker-discovered failure;
- reducing verification because the candidate is large without a typed resource outcome;
- declaring `tolerance_checked` before every mandatory check completes;
- returning a topology-only ordinary success;
- using native-struct serialization, locale-dependent text, pointer values, or unordered iteration in diagnostics or replay; or
- invoking an external geometry, graph, verification, serialization, hashing, fuzzing, or replay dependency.

## 5. Test and validation specification

### 5.1 Known-valid publication matrix

Verify successful publication for:

- empty results;
- one tetrahedron;
- one triangulated box;
- several disconnected solids;
- nested outer shells, cavities, and islands;
- point-touching components with separate public indices;
- edge-touching components with duplicated endpoint occurrences;
- coincident but separately represented occurrences authorized by symbolic policy;
- cleanup-generated vertex splits;
- topology-preserving short-edge cleanup; and
- repeated Boolean-chain outputs with accumulated precision.

For each case, independently reconstruct all mandatory topology, geometry, report, digest, and re-ingestion facts.

### 5.2 Public topology mutation tests

Starting from valid candidates, inject:

- one out-of-range index;
- one sentinel index;
- one repeated vertex within a triangle;
- one missing reverse edge use;
- one duplicated directed edge use;
- one three-use edge;
- one bow-tie vertex with all edges still two-use;
- one disconnected facet omitted from component reports;
- one isolated public vertex;
- one reversed triangle;
- one duplicated triangle; and
- one missing triangle.

Repair producer counts and candidate digests where needed so the independent verifier, not a superficial checksum, catches the mutation.

### 5.3 Internal/public bijection mutation tests

Inject:

- two cleaned occurrences mapped to one public vertex;
- one cleaned occurrence mapped to two public vertices;
- a public facet mapped to the wrong cleaned triangle;
- a reversed rather than cyclic facet mapping;
- one coordinate bit changed while topology remains equal;
- one duplicate-coordinate pair accidentally deduplicated;
- one map permutation with correct counts;
- one public edge mapping to the wrong cleaned edge; and
- one topology-distinct event/source occurrence merged by coordinate.

Every mutation must be rejected deterministically.

### 5.4 Event and lineage mutation tests

Inject:

- duplicate canonical event records with inconsistent coordinates;
- one event consumer using a recomputed coordinate;
- two distinct events merged because their nominal coordinates match;
- wrong source edge or face lineage;
- carrier event order changed at an equal-parameter tie;
- one multiplicity occurrence omitted;
- one topology-separation rank changed;
- cleanup welding unrelated event occurrences; and
- stale relation or event IDs from another context.

### 5.5 Classification and Boolean-selection mutation tests

Inject:

- one zero-delta union across a cut;
- one missing zero-delta connection;
- one flipped crossing multiplicity;
- one inconsistent quotient-graph cycle;
- one wrong seed winding;
- one wrong side label;
- one retained internal surface;
- one omitted result boundary;
- one reversed retained orientation;
- one wrong coincident owner;
- one omitted multiplicity copy; and
- one false occupied connection across point or edge contact.

The verifier must reject these even when the public mesh remains a valid indexed manifold.

### 5.6 Triangle geometry tests

Cover:

- ordinary well-conditioned triangles;
- sliver triangles above the accepted threshold;
- triangles exactly at the policy boundary;
- triangles below the boundary;
- repeated-coordinate but distinct-index degeneracy;
- signed-zero coordinate combinations;
- subnormal coordinates;
- extreme finite exponents;
- large translations;
- near-collinear vertices; and
- coordinate-copy mutations changing one ULP.

Expected ambiguous cases must fail closed with the correct typed category.

### 5.7 Forbidden-intersection known-answer tests

Include:

- widely separated components;
- adjacent triangles sharing an edge;
- triangles sharing only a public vertex;
- point-touching separate components;
- edge-touching separate components;
- proper transverse non-adjacent intersections;
- vertex-through-triangle penetration;
- edge-edge crossing;
- coplanar partial overlap;
- coincident triangles with authorized and unauthorized ownership;
- two topology-distinct sheets inside overlapping uncertainty envelopes;
- cleanup-introduced crossing; and
- one-ULP separations above and below the bounded decision threshold.

For bounded fixtures, compare the independent spatial search against exhaustive all-pairs enumeration.

### 5.8 Broad-phase false-negative mutation tests

Deliberately corrupt independent search inputs or implementation seams by:

- shrinking one triangle bound;
- omitting precision inflation;
- excluding a pair by coordinate proximity;
- treating a non-adjacent pair as adjacent;
- dropping a node during hierarchy merge;
- using an unstable equal-key order; and
- overflowing a bound or index calculation.

Qualification must prove the exhaustive bounded oracle catches each omission.

### 5.9 Side-probe tests

Test deterministic probes on:

- planar triangles with generous clearance;
- narrow gaps;
- thin shells;
- nearby unrelated surfaces;
- high-curvature triangulated neighborhoods;
- coincident ownership cases;
- point and edge contacts;
- triangles near cleanup-modified regions;
- extrema of the precision ledger;
- cavities and nested islands; and
- probes whose preferred location or normal offset is uncertain.

Verify deterministic fallback locations, coverage-class accounting, independent operand occupancy, and failure when no safe probe exists.

### 5.10 Precision-envelope tests

Use in-tree exact low-complexity oracles to check:

- every nominal construction lies within its published enclosure;
- edge parameters contain exact template values;
- plane residual bounds are conservative;
- inherited input precision is not lost;
- equal event consumers share one envelope;
- near-parallel conditioning crosses the acceptance threshold correctly;
- bounds never shrink without an explicit proof record; and
- one deliberately understated bound is rejected.

### 5.11 Cleanup replay and budget tests

Cover:

- zero-area triangle removal;
- zero-length edge handling;
- short-edge collapse with valid link condition;
- collapse requiring occurrence splitting;
- rejected collapse that would create a three-use edge;
- collinear-chain simplification;
- tiny component removal when authorized;
- thin-handle removal with explicit topology change;
- cumulative displacement just below, at, and above tolerance;
- missing, duplicated, reordered, and forged action certificates; and
- under-reported removed-feature cost.

Replaying accepted certificates must reproduce the cleaned local topology and deterministic cumulative witnesses.

### 5.12 Report and status mutation tests

Mutate:

- topology counts;
- component/genus/cavity summaries;
- geometry status;
- output precision;
- maximum displacement;
- cleanup action count;
- removed-feature maximum;
- provenance coverage;
- resource counters;
- verifier version;
- side-probe coverage;
- re-ingestion status; and
- accepted policy-exception lists.

Set corrected candidate digests where practical. Independent report reconstruction must detect every inconsistency.

### 5.13 Serialization and digest tests

Commit known logical byte sequences for representative success and failure records.

Test:

- integer framing boundaries;
- exact `float` and `double` bit encodings;
- signed-zero policy;
- simulated host endianness;
- sequence-length overflow;
- unknown optional and required fields;
- domain separation;
- digest-collision injection;
- native padding differences;
- locale changes;
- reordered findings; and
- forged candidate/final digests.

Full structural content must remain authoritative under collision injection.

### 5.14 Re-ingestion tests

For every known-valid result:

- re-ingest the public mesh with published precision;
- compare reconstructed components, links, and orientation;
- verify duplicate-coordinate occurrences remain separate;
- verify cavities and islands retain their semantics;
- use the result as an operand in a follow-on Boolean fixture; and
- confirm precision propagation is monotonic.

Inject adapters or validator seams that reorder, deduplicate, repair, or understate precision and require rejection.

### 5.15 Replay compatibility tests

Test:

- success replay;
- each typed final-verification failure;
- focused replay of one topology, geometry, cleanup, and serialization finding;
- old supported replay versions;
- unknown required versions;
- truncated records;
- corrupted lengths and counts;
- corrupted content digests;
- exact scalar-bit preservation;
- deterministic primary finding reproduction; and
- replay under thread counts 1, 2, and maximum.

Replay must reproduce the same public-content digest or the same primary error and canonical witness under the frozen version contract.

### 5.16 Independent-verifier separation tests

Introduce matched producer mutations that would survive if the verifier reused producer control flow, including:

- identical wrong edge-pair tables in Component 14 maps;
- identical wrong component labels;
- identical understated report maxima;
- identical omitted broad-phase pair;
- identical wrong classification group;
- identical wrong canonical digest; and
- identical wrong cleanup cumulative counter.

The independent reconstruction path must reject each artifact.

### 5.17 Exhaustive bounded oracle tests

For small meshes and integer-coordinate templates:

- enumerate all non-adjacent triangle pairs;
- compare bounded relation classifications with the in-tree exact oracle;
- enumerate public edge and link reconstruction directly;
- compare component partitions;
- compare bounded winding/classification cases;
- compare canonical mappings where exhaustive permutation is feasible; and
- verify all scalable checks agree with exhaustive conclusions.

Every disagreement must serialize, minimize, and enter the permanent regression corpus through Component 16.

### 5.18 Metamorphic tests

For applicable successful results, verify invariance under:

- source vertex, facet, ring, shell, and component permutation;
- legal source subdivision and alternative triangulation;
- facet cyclic rotation;
- operand swap with operation remapping;
- axis permutation;
- sign flips with corrected orientation;
- exactly representable translation;
- power-of-two scale with corresponding tolerance/precision scaling;
- internal entity-ID permutation;
- cleanup allocation-history permutation;
- thread-count and work-partition changes; and
- repeated execution.

Final public mesh bytes, reports, primary status, replay bytes, and digest domains must obey the frozen deterministic contract.

### 5.19 Determinism and concurrency tests

Run all final-verification phases with:

- thread counts 1, 2, and configured maximum;
- forced task delays;
- reversed task submission;
- alternative deterministic partitions;
- hash-collision injection;
- equal-maximum witness ties;
- several simultaneous failures; and
- cancellation at every safe point.

The primary result, retained findings within limits, counters, reports, replay bytes, and digests must be identical.

### 5.20 Resource boundary tests

For every separately accounted resource, test limit-minus-one, limit, and limit-plus-one.

Cover:

- edge-use records;
- edge groups;
- link arcs;
- component traversal;
- internal/public match candidates;
- independent hierarchy nodes;
- triangle-pair candidates;
- bounded relation work;
- side probes;
- lineage checks;
- cleanup replay records;
- precision records;
- findings;
- report bytes;
- serialization bytes;
- re-ingestion storage;
- replay bytes;
- temporary bytes; and
- abstract work units.

Crossing a hard limit must produce one deterministic typed failure with no ordinary success.

### 5.21 Cancellation and allocation-failure tests

Cancel during each major phase and inject allocation failure through in-tree test allocators at every reservation boundary.

Confirm:

- all workers join;
- private buffers are destroyed;
- reservations return;
- no final status or success leaks;
- predecessor artifacts remain valid;
- the same canonical completed-phase counters are reported; and
- replay is valid when policy requests it.

### 5.22 Fuzzing and shrinking

Generate valid assembled candidates and mutate topology, geometry, lineage, cleanup, reports, serialization, and digests independently.

Every crash, undefined behavior, false acceptance, false rejection against a bounded exact oracle, nondeterministic finding, or replay mismatch must:

- serialize exact source bits and options;
- retain all needed predecessor artifacts or reproduction inputs;
- shrink while preserving the same primary finding class and canonical witness relation where practical; and
- become a permanent regression through Component 16.

### 5.23 Structural performance gates

Measure deterministic counters for:

- facet and edge-use scans;
- edge grouping;
- link reconstruction;
- component traversal;
- independent hierarchy build;
- candidate triangle pairs;
- exact bounded pair checks;
- side probes;
- cleanup actions replayed;
- precision records aggregated;
- report records emitted;
- serialization bytes; and
- abstract work units.

Large disjoint outputs must not exhibit accidental all-pairs pair checks. Performance optimization must not reduce candidate conservatism, side-probe coverage, report completeness, or any mandatory gate.

### 5.24 Definition of done

Component 15 is complete only when:

- every ordinary success passes the non-disableable mandatory verification floor;
- public topology is independently reconstructed and valid;
- internal/public assembly is independently proven bijective;
- event sharing and occurrence separation are verified;
- winding, selection, orientation, multiplicity, and coincident ownership are verified;
- triangle geometry and shell orientation are accepted within bounded precision;
- no forbidden non-adjacent interaction is found;
- deterministic side probes confirm requested Boolean occupancy;
- all construction, cleanup, precision, and tolerance claims are independently audited;
- public re-ingestion succeeds;
- final reports and all digest domains are independently regenerated and consistent;
- every mutation class is rejected by the intended independent check;
- replay reproduces successes and failures deterministically;
- thread count and schedule do not change results;
- resource and cancellation behavior is transactional; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
