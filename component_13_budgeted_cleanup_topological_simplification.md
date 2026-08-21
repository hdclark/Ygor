# Component 13: Budgeted Cleanup and Topological Simplification

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete cleanup candidate queue, local patch representation, link-condition implementation, coordinate-choice provider, retriangulation provider, local intersection index, certificate encoding, and deterministic edit scheduler may change. Per-edit manifold preservation, lineage-based eligibility, reserve-before-mutate budget accounting, bounded displacement and feature-removal certificates, no proximity welding, explicit topology-change reporting, deterministic termination, complete degeneracy discharge, verification, and failure contracts in this document are normative.

## 0. Purpose

This component converts the paired triangulated internal surface from Component 12 into a clean, tolerance-bounded, oriented triangle manifold suitable for public assembly and independent final verification.

Its purposes are to:

- discharge every cleanup-required triangle, zero-length edge, collinear chain, pinched local patch, and other residual obligation emitted by Component 12;
- remove or simplify sub-tolerance artifacts only through explicit caller-authorized policies and conservative certificates;
- perform edge collapse, local retriangulation, edge swap, vertex duplication/splitting, and component removal only when exact topological eligibility and bounded geometric admissibility are established;
- preserve reciprocal edge pairing and one closed face fan per output vertex occurrence after every atomic edit;
- prevent a locally convenient collapse from creating a three-use edge, bow-tie vertex, disconnected link, false point/edge connection, or coincident-sheet weld;
- reserve and commit actual cumulative displacement or feature-removal cost through Component 03's tolerance-budget service;
- recompute bounded coordinates, local residuals, precision envelopes, and affected geometric certificates after every committed edit;
- preserve complete source, event, retained-surface, occurrence, triangle, and cleanup lineage;
- record every topological or geometric change as a deterministic replayable action with a local before/after certificate; and
- either produce a clean manifold that satisfies the Component 14 input contract or fail without publishing a partially cleaned result.

The component performs authorized cleanup, not Boolean re-evaluation. It does not change the operation truth table, recompute classifications or intersections, merge unrelated features because they are close, invent missing topology, relax the caller's tolerance, hide topology changes, merge output polygons for aesthetics, or serialize the public mesh.

The principal output is an immutable `cleaned_triangle_manifold<T>` containing:

- canonical cleaned vertex occurrences and bounded coordinates;
- canonical reciprocal paired edges and halfedges;
- canonical oriented nondegenerate triangles;
- one closed incident fan per topological vertex occurrence;
- no unresolved cleanup obligation;
- a complete cleanup action log and local certificates;
- cumulative displacement, feature-removal, precision, topology-change, and provenance reports;
- structural and geometric admissibility evidence; and
- resource, verification, digest, and replay metadata.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- the immutable `triangulated_output_complex<T>` from Component 12;
- the immutable `polygonal_output_complex<T>` from Component 11 for original face-region, cycle, boundary, and occurrence audit references;
- the immutable `retained_surface_complex` from Component 10 for selected-boundary orientation, multiplicity, and topology-separation constraints;
- the immutable `canonical_intersection_complex<T>` from Component 08 and source topology from Component 05 where event/source lineage constrains cleanup eligibility;
- source-facet and triangulation provenance from Components 04 and 05;
- the immutable `precision_context<T>`, bounded construction/predicate/residual services, mutable transactional precision ledger, and transactional tolerance-budget service from Component 03;
- the immutable Boolean context, cleanup/output/contact policies, identity domains, deterministic comparators, resources, cancellation, diagnostics, replay, and transaction services from Component 01;
- the selected cleanup-action, eligibility, certificate, topology-change, artifact, and serialization versions; and
- verification settings controlling per-edit, scalable final-stage, and exhaustive bounded cleanup checks.

The component must not read mutable caller meshes, change predecessor artifacts, recompute Boolean side classifications, or use an external repair, remeshing, simplification, exact-arithmetic, graph, or geometry library.

### 1.2 Required predecessor guarantees

The component may rely on Component 12 having established:

- a complete paired combinatorial surface;
- exact preservation of every Component 11 boundary edge and occurrence identity;
- reciprocal internal diagonal pairs;
- complete triangle/region/boundary assignments;
- prescribed output orientation for every triangle and cleanup-required patch;
- no forbidden positive-area triangle overlap within a face region;
- explicit geometric categories for every triangle;
- explicit cleanup-required residual obligations;
- no hidden movement or cleanup budget expenditure;
- complete source/event/carrier/retained-use/region/cycle provenance; and
- deterministic canonical ordering and digests.

The combinatorial surface may contain geometrically zero-area triangles, zero-nominal-length paired edges, near-collinear chains, pinched local configurations, or sub-tolerance components, but every such condition must be represented explicitly.

The component must defensively verify owner tokens, versions, index ranges, pair reciprocity, triangle cycles, orientation categories, boundary mappings, residual-obligation coverage, occurrence-separation constraints, precision-ledger references, and predecessor digests.

A contradiction in a committed predecessor artifact is an `internal_invariant_error`. Cleanup must not be used to conceal an unpaired edge, missing triangle, wrong occurrence partition, or reversed selected surface.

### 1.3 Cleanup policy preconditions

The frozen cleanup policy must explicitly define:

- which local action classes are enabled;
- the maximum authorized user tolerance;
- whether coordinate movement is allowed and how cumulative movement is combined;
- whether short-edge collapse is allowed;
- whether zero-area face removal by local retriangulation is allowed;
- whether collinear-chain simplification is allowed;
- whether edge swaps are allowed;
- whether vertex splitting/duplication is allowed to preserve manifoldness;
- whether an entire closed component may be removed;
- whether genus-changing edits are prohibited, conditionally allowed, or allowed only for named degeneracy classes;
- whether source-feature or provenance preservation imposes stricter local restrictions;
- required geometric clearance and no-new-intersection checks;
- deterministic action priority and tie rules;
- maximum edit counts and work limits; and
- report and replay detail levels.

An unrecognized, incomplete, or internally contradictory cleanup policy is an `input_contract_error` or `invalid_tolerance` before mutation begins.

The default conservative policy must prohibit genus-changing cleanup except removal of an explicitly authorized entire sub-tolerance closed component. A more permissive future policy must be separately versioned and satisfy every certificate requirement in this document.

### 1.4 Cleanup obligation domain

Every Component 12 cleanup-required record must enter the cleanup audit as one obligation with exactly one final disposition:

- discharged by one or more committed cleanup actions;
- proven already geometrically valid under a stronger bounded check;
- absorbed into another canonical obligation with complete member evidence;
- removed with an explicitly authorized component or feature certificate;
- rejected because required cleanup exceeds tolerance or policy;
- rejected because no manifold-preserving action exists;
- rejected because geometry remains unvalidated; or
- failed due to resource, cancellation, or internal invariant error.

No obligation may disappear because its owning triangle or edge was removed as a side effect. Every side effect must update the obligation map transactionally.

### 1.5 Exact topology and lineage preconditions

Every cleanup candidate must be expressed over exact internal topology and immutable lineage.

Candidate identity must include, as applicable:

- vertex occurrence IDs;
- paired-edge and halfedge IDs;
- incident triangle IDs;
- local vertex-fan sectors;
- face-region and retained-use ancestry;
- source/event/carrier provenance;
- occurrence-separation and multiplicity classes;
- boundary-vs-internal-diagonal roles;
- predecessor cleanup obligations; and
- the current cleanup generation/version of all affected entities.

A stale candidate whose generation no longer matches current topology must be discarded and, if still relevant, regenerated. It must never be applied to a changed neighborhood.

### 1.6 Capacity and lifetime preconditions

Before cleanup begins, the component must validate with overflow-safe arithmetic that it can represent and account for:

- mutable stage-private vertices, edges, halfedges, triangles, and local fan records;
- candidate queues and invalidation generations;
- local patch snapshots and rollback data;
- vertex/edge duplication and split records;
- bounded coordinate proposals and precision-ledger entries;
- budget reservations and committed costs;
- cleanup actions and before/after certificates;
- local and global intersection candidates;
- topology-change and provenance mappings;
- per-edit and final verification data;
- diagnostics and replay storage; and
- worst-case work up to configured action, candidate, patch, intersection, and byte limits.

Published cleanup records may reference immutable predecessor artifacts and immutable committed stage-owned storage whose lifetime covers Components 14-15.

## 2. Required behavior

### 2.1 Stage-private mutable manifold

Cleanup must operate on a private mutable representation initialized from Component 12.

Initialization must preserve:

- every topological vertex occurrence;
- every reciprocal paired edge and halfedge;
- every oriented triangle cycle;
- every boundary/source/carrier/internal-diagonal role;
- every occurrence-separation and multiplicity constraint;
- every bounded coordinate and precision ledger entry;
- every cleanup obligation; and
- complete reverse provenance.

The mutable representation may use generation counters, free lists, indirection, or compact patch storage, but stale handles must be detectable and no stage-private identity may escape publication without canonical remapping.

### 2.2 Cleanup target conditions

Cleanup succeeds only when the proposed final manifold satisfies all of the following:

- every vertex coordinate is finite and has a conservative precision envelope;
- every triangle has three distinct vertex occurrence IDs;
- every triangle has definite nonzero orientation in the required output direction above the final accepted degeneracy threshold;
- every directed triangle edge has one reciprocal opposite-directed halfedge;
- every undirected edge has exactly two incident triangle uses;
- every topological vertex occurrence has one closed incident triangle fan;
- no unresolved zero-length edge, zero-area triangle, invalid collinear patch, or cleanup obligation remains;
- no forbidden new nonadjacent triangle intersection is introduced by cleanup;
- all committed displacement and feature-removal costs are within policy and caller tolerance;
- output precision dominates inherited, construction, and cleanup contributions; and
- every topology change is permitted and reported.

Component 15 performs independent final verification, but Component 13 must not publish an artifact known to violate any target condition.

### 2.3 Candidate discovery

Cleanup candidates may arise from:

- Component 12 residual obligations;
- zero or insufficient bounded triangle area;
- zero or sub-threshold bounded edge length;
- near-collinear chains;
- duplicate internal diagonal patterns;
- pinched local fan geometry;
- sub-tolerance sliver triangles;
- a small closed component eligible for removal;
- a local retriangulation that can eliminate degeneracy without movement; or
- verification findings produced after an earlier edit.

Candidate discovery must be topological and lineage-aware. Spatial proximity may identify potential geometric conflicts or opportunities only after an exact topological action domain exists. It must not propose welding arbitrary nearby vertices or edges.

Every candidate must publish an eligibility pre-record containing its action class, affected patch, obligation membership, preliminary bounds, required budget class, forbidden constraints, and deterministic key.

### 2.4 Deterministic action scheduling

The cleanup provider must define a frozen total priority over eligible candidates.

The priority may consider:

- obligation severity;
- action class;
- whether the action requires zero movement;
- bounded cost;
- number of obligations discharged;
- local topology complexity;
- canonical feature and provenance keys; and
- candidate generation.

Numerical cost participates in ordering only when bounded comparison is definite. Equal or overlapping cost intervals must use a stable lineage tie key rather than nominal floating order.

After an edit, only affected neighborhoods may regenerate candidates, but final behavior must be equivalent to applying the frozen global scheduling contract. Thread timing, queue insertion order, hash order, and memory address must not affect committed action sequence or final canonical artifact.

### 2.5 Atomic cleanup action transaction

Every cleanup action must execute as a nested private transaction with these phases:

1. revalidate candidate generation and exact topology;
2. collect the complete closed affected patch and its external interface;
3. verify topological eligibility;
4. construct bounded coordinate and topology proposals;
5. conservatively compute displacement, feature-removal, precision, and clearance costs;
6. reserve all resource and tolerance budget needed by the proposal;
7. build the replacement patch privately;
8. verify reciprocal pairing, triangle orientation, vertex links, interface equivalence, and no-new-intersection constraints;
9. create the complete before/after certificate and action log entry;
10. atomically replace the old patch and commit actual budget costs; and
11. invalidate and regenerate affected candidates and obligations.

Failure before commit must leave the mutable manifold, budget ledger, resource counters, candidate generations, and obligations unchanged.

No action may temporarily publish a non-manifold intermediate patch.

### 2.6 Local patch closure and interface

The affected patch of an action must include enough topology to verify all consequences of the edit.

At minimum, the patch must include:

- every removed or moved vertex;
- every incident edge and triangle that may change orientation or pairing;
- complete vertex links for vertices whose incidence changes;
- every edge that could become duplicate or gain additional uses;
- the external boundary halfedges through which the replacement reconnects to unchanged topology;
- neighboring triangles needed for local intersection and clearance checks; and
- all occurrence-separation, source-boundary, and provenance constraints crossing the patch boundary.

The replacement patch must have an interface bijection to the unchanged exterior except where an explicitly authorized component or topology-change certificate says otherwise.

### 2.7 Manifold edge-collapse eligibility

An edge collapse may be considered only for one exact paired edge occurrence.

Before geometry or tolerance is considered, the component must verify a closed-surface manifold link condition equivalent to:

- the intersection of the endpoint vertex links equals the link of the collapsed edge, adjusted only for explicitly represented duplicate occurrences or authorized local splits;
- the two incident triangles are exactly the triangles expected to disappear for an ordinary collapse;
- no other edge will become a three-use or higher-use edge;
- no vertex link will become disconnected or self-identified;
- no two topology-separated fans will be joined;
- no point- or edge-touching components will be welded;
- no distinct coincident-sheet occurrences will be merged;
- no prohibited source/result boundary role will be erased; and
- the replacement neighborhood can be represented as an oriented two-manifold.

The implementation may use an equivalent exact combinatorial criterion, but it must publish enough local member evidence for independent verification.

A short or zero-length edge is not automatically collapsible.

### 2.8 Collapse representative and bounded coordinate choice

For a topologically eligible collapse, the proposed replacement vertex coordinate must come from a deterministic bounded provider.

Permitted policy classes may include:

- retain endpoint 0;
- retain endpoint 1;
- choose the endpoint with a lower proven displacement cost;
- choose an existing canonical source/event coordinate compatible with all contributors;
- construct a bounded point on the edge or local support under Component 03; or
- use another explicitly versioned in-tree provider with conservative error analysis.

The provider must compute, for every contributing vertex lineage:

- nominal displacement;
- displacement enclosure including both old and proposed precision;
- cumulative displacement already charged to that lineage;
- new cumulative bound under the policy's combination rule;
- local support-plane or carrier residuals;
- incident triangle orientation margins; and
- available tolerance budget.

Averaging coordinates merely because it is visually smooth is prohibited. The chosen point must be finite, reproducible, and within all required budgets.

### 2.9 Vertex lineage merge on collapse

When a collapse creates one replacement topological vertex from two or more current vertices, the new occurrence must retain the union of their lineage sets.

The action record must preserve:

- all predecessor output occurrence IDs;
- all source vertex and event provenance;
- all retained-use and face-region ancestry;
- cumulative displacement per original lineage;
- occurrence-separation constraints proven compatible with the merge;
- old-to-new entity mappings; and
- the exact reason the merge is topologically authorized.

Two vertices from unrelated occurrence requirements may merge only through a certified local topological action permitted by policy, never merely because their coordinates are near. If occurrence-separation constraints prohibit the merge, the candidate is ineligible regardless of length.

### 2.10 Vertex splitting and occurrence duplication

When removing a degenerate feature would otherwise create a bow-tie vertex, disconnected link, non-manifold edge, or false connection, the component may split or duplicate a topological occurrence if the frozen cleanup policy permits it.

A split must:

- partition the incident oriented triangle sectors by exact local fan connectivity;
- create one output occurrence per resulting closed fan;
- duplicate the bounded coordinate without introducing displacement unless a separate move is authorized;
- preserve source/event lineage and record multiplicity of the copies;
- reassign edge and triangle incidence so every edge remains two-use;
- preserve topology-separation and coincident-sheet semantics;
- avoid creating a geometric gap solely for topology; and
- publish a split certificate with complete member sets and link cycles.

A split may precede a collapse in one atomic composite action when neither intermediate state is published. The certificate must expose both conceptual steps.

The component must not split a valid connected fan merely to improve triangle quality unless an explicit policy and topology-change certificate authorize it.

### 2.11 Zero-area triangle cleanup

A zero-area or cleanup-required triangle must be removed only through a local manifold-preserving operation.

Potential action classes include:

- collapse of one eligible edge;
- collapse of a zero-length edge after required occurrence splitting;
- deletion of a paired two-triangle zero-area fold followed by interface reconnection;
- local polygonal patch reconstruction and retriangulation using existing or certified replacement vertices;
- elimination of a redundant collinear vertex chain; or
- removal of an entire authorized degenerate component.

The chosen action must account for all three triangle edges, neighboring triangles, obligations, and provenance. Deleting the triangle record alone is prohibited because it leaves a boundary or unpaired edges.

If no permitted action removes the triangle while preserving manifoldness and budget, the stage must fail with `cleanup_budget_exceeded`, `result_geometry_not_validated`, or another precise typed status.

### 2.12 Zero-length edge cleanup

A topologically distinct edge whose bounded length contains only zero or lies below the cleanup threshold must be evaluated through exact local topology.

The component must distinguish:

- an ordinary collapsible edge;
- an edge connecting separate occurrence fans that must not merge;
- an edge whose collapse requires endpoint fan splitting;
- one copy of several coincident edge occurrences;
- a zero-length edge on a thin but positive-area feature;
- a carrier/source-boundary edge whose removal would change selected topology; and
- a closed degenerate component edge.

If the edge cannot remain in the final public manifold and cannot be removed through an authorized certified action, cleanup fails. The component must not leave it because its endpoints happen to serialize to the same coordinate.

### 2.13 Collinear-chain simplification

A chain of edges may be simplified only when:

- the chain is identified through exact triangle/face-region topology;
- removed intermediate vertices have no incident topology outside the local chain fan that would be lost;
- bounded orientation and distance evidence proves the replacement is within policy;
- all affected triangles can be retriangulated with definite orientation;
- no source/carrier boundary obligation prohibits removal;
- occurrence separation and multiplicity remain valid;
- no new edge crosses or approaches unrelated geometry outside accepted clearance; and
- cumulative displacement/removal cost is reserved and certified.

A nominal collinearity test against a global epsilon is insufficient. Long chains with large translation or mixed coordinate magnitudes must use Component 03 bounds.

The action log must map every removed chain edge/vertex and every replacement triangle.

### 2.14 Edge swaps and local retriangulation

An internal diagonal may be swapped or a local polygonal patch retriangulated when doing so discharges a degeneracy or produces required definite triangle orientation without changing the selected surface boundary.

Eligibility requires:

- the edge is an internal triangulation diagonal or another explicitly swappable role;
- the replacement patch has the same external boundary halfedge cycle and orientation;
- all replacement diagonals are pair-created;
- every replacement triangle has definite accepted orientation or a strictly reduced cleanup obligation under the action's monotonic measure;
- no occurrence partition or source/result boundary role changes;
- no forbidden crossing or nonadjacent intersection is introduced; and
- the action stays within budget.

Edge quality optimization alone is not a requirement. An edge swap must not oscillate with a later swap; deterministic scheduling and progress rules must prevent cycles.

### 2.15 Sub-tolerance slivers and narrow features

A sliver triangle, thin corridor, thin handle, or narrow shell feature must not be removed merely because one scalar measure is below tolerance.

The component must determine:

- whether the feature is geometrically valid despite poor aspect ratio;
- whether it causes a final validity failure;
- whether an authorized local action can remove it within displacement/removal budget;
- whether removal changes component count, genus, cavity structure, or contact separation;
- whether retained Boolean side semantics remain plausible after the edit; and
- whether neighboring features provide sufficient clearance.

A valid thin feature may remain if it satisfies the final geometric contract. Tolerance authorizes bounded simplification; it does not require all small features to disappear.

### 2.16 Entire-component removal

A closed connected output component may be removed only when the frozen policy explicitly permits component removal and a conservative certificate proves eligibility.

The certificate must establish:

- exact connected-component membership;
- closed oriented manifold topology before removal;
- a conservative feature-size enclosure, such as diameter, distance to a canonical representative, or another versioned bound;
- that the entire removed component lies within the caller-authorized feature-removal budget under the policy's metric;
- that it is topologically separate from retained components even if coordinates touch;
- that removal does not leave unmatched coincident ownership or provenance obligations;
- that operation semantics permit regularized removal of that component under the cleanup policy; and
- the resulting component-count and occupied-volume interpretation change.

Removal must be all-or-nothing and transactionally delete every vertex, paired edge, halfedge, triangle, obligation, and local candidate owned exclusively by the component.

A component may not be removed based only on triangle count, bounding-box diagonal computed without uncertainty, or small nominal volume.

### 2.17 Genus and component-count changes

Every cleanup action must classify its topological effect:

- no change to component count or genus;
- vertex/edge/face count change only;
- component removal;
- component merge;
- component split;
- handle removal or creation;
- cavity removal or creation; or
- another versioned topology change.

The default policy must reject component merge/split and genus-changing actions. Vertex splitting used solely to preserve one manifold occurrence structure is not a geometric component split when it separates a coordinate-coincident non-manifold identification that was never an authorized final occurrence; its certificate must make this distinction explicit.

A future policy that allows handle or cavity removal must require:

- exact local/global topology effect computation;
- a conservative geometric feature-size certificate;
- complete Boolean-side plausibility evidence;
- caller authorization; and
- explicit reporting in the success wrapper.

No topology change may be inferred from an Euler-count delta alone without validating the resulting manifold.

### 2.18 Incident triangle orientation and collapse safety

For every moved, merged, split, or retriangulated vertex neighborhood, the component must evaluate all affected triangle orientations using bounded predicates.

The replacement patch must not contain:

- inverted triangles;
- triangles whose orientation is unresolved beyond the accepted final threshold;
- repeated topological corners;
- new zero-area triangles unless the composite action removes them before atomic commit;
- inconsistent outward orientation; or
- orientation disagreement across a paired edge.

Orientation checks must include triangles incident to the patch boundary whose coordinates changed. Checking only triangles that are newly allocated is insufficient.

### 2.19 No-new-intersection and clearance checks

Every action that moves geometry, creates a replacement edge, or retriangulates a patch must conservatively enumerate possible interactions between the proposed patch and unaffected nonadjacent geometry.

The check must:

- use an in-tree conservative spatial index or exhaustive enumeration for bounded fixtures;
- inflate bounds by old/new precision and movement envelopes;
- exclude adjacency only by exact topology;
- distinguish shared vertices/edges, topology-distinct coordinate contact, permitted local replacement overlap, and forbidden intersection;
- check both proposed triangles against external triangles and swept/movement envelopes where required by policy;
- reject a definite forbidden intersection; and
- reject unresolved uncertainty when it could place geometry outside the authorized tolerance or invalidate embedding.

An action must not rely solely on post-move triangle quality or local normal agreement.

### 2.20 Boolean-side plausibility preservation

Cleanup does not repeat full Component 09/10 classification, but every geometric/topological action must preserve the selected boundary interpretation within tolerance.

The action certificate must retain or derive enough evidence to show:

- output orientation remains outward;
- the patch stays within the bounded neighborhood authorized by its source/result lineage;
- it does not cross an opposite-operand boundary or selected carrier outside uncertainty envelopes;
- it does not join topology-separated point/edge contacts;
- it does not expose a previously suppressed coincident internal sheet; and
- representative local side probes remain consistent where the verification policy requires them.

Component 15 performs independent global side checks. Component 13 must reject an action whose local evidence already contradicts Boolean occupancy.

### 2.21 Budget reservation and cumulative cost

Before mutation, every action must reserve conservative budget for:

- displacement of each affected original lineage;
- uncertainty of any newly constructed cleanup coordinate;
- removed feature size;
- swept or support deviation where policy requires it;
- component removal cost; and
- any topology-change-specific allowance.

The committed cost must be based on the actual accepted proposal and may be lower than the reservation, but never higher.

For each original lineage, the ledger must preserve cumulative displacement across sequential edits. The combination rule may be sum, maximum under a common enclosure, or another proven conservative metric, but it must be explicit and independently verifiable.

The global reports must include at least:

- maximum realized point displacement;
- maximum cumulative lineage displacement;
- maximum removed feature size;
- total and per-class committed budget where meaningful;
- remaining tolerance margin;
- largest construction uncertainty; and
- resulting output precision.

If any applicable bound exceeds caller tolerance, the action is rejected or the stage fails with `cleanup_budget_exceeded`. The component must not under-report sequential motion by retaining only the largest individual step.

### 2.22 Precision recomputation

After every committed action, the component must update bounded geometry and precision for all affected entities.

Updates must include:

- replacement vertex bounded points;
- inherited source/event precision;
- cleanup construction uncertainty;
- cumulative displacement contribution;
- affected edge-length bounds;
- triangle orientation and plane residuals;
- local spatial bounds; and
- aggregate output precision candidates.

Unchanged entities may retain shared immutable precision records. Changed entities must not keep stale pre-edit bounds.

Output precision must satisfy:

```text
output_precision >= inherited input precision
output_precision >= all retained construction uncertainty
output_precision >= all cleanup construction uncertainty
output_precision >= maximum applicable cumulative cleanup displacement
```

Ordinary success additionally requires `output_precision <= user_tolerance` under the frozen policy.

### 2.23 Cleanup action certificate

Every committed action must publish a deterministic certificate containing:

- action ID, class, provider version, and candidate key;
- pre-action entity IDs, generations, topology, and bounded coordinates;
- complete affected patch and external interface;
- exact topological eligibility evidence;
- proposed and committed replacement entities;
- old-to-new and new-to-old provenance mappings;
- bounded displacement and feature-removal calculations;
- budget reservation and committed ledger entries;
- orientation, link, pair, intersection, clearance, and side-plausibility checks;
- obligations discharged, created, merged, or retained;
- component/genus effect classification;
- resource counters;
- deterministic reason codes; and
- before/after digests.

Certificates may share immutable tables to reduce storage, but an independent verifier must be able to replay each action from the prior committed state without producer-private decisions.

### 2.24 Obligation update and completion

After an action commits, the component must update the cleanup-obligation graph transactionally.

An obligation is complete only when:

- all owning degenerate entities have been removed or proven final-valid;
- no replacement entity inherits the same unresolved condition;
- all associated boundary and occurrence constraints remain represented;
- all costs and topology effects are certified; and
- independent local verification accepts the resulting patch.

A composite action may discharge several obligations. The action log must enumerate all of them. A new obligation may be created only when it has a strictly lower value under the documented progress measure and remains within resource limits.

### 2.25 Termination and anti-oscillation

The cleanup provider must define a finite monotonic progress measure over the full mutable state.

A suitable measure may lexicographically combine:

- number and severity of unresolved cleanup obligations;
- number of invalid/zero-area triangles;
- number of prohibited zero-length edges;
- number of unresolved collinear patches;
- total mutable entity count;
- action-class phase; and
- canonical candidate key.

Every committed action must strictly reduce the measure or advance a nonreturning phase in a documented finite state machine.

The implementation must prevent:

- edge-swap oscillation;
- collapse/split oscillation;
- repeated movement of one vertex without cumulative progress;
- regeneration of an identical failed candidate without changed evidence; and
- nondeterministic choice among equivalent cycles.

If no eligible action can reduce remaining obligations, cleanup fails. It must not iterate until a numerical accident changes a predicate.

### 2.26 Cleanup idempotence

Running the same cleanup policy on a successful `cleaned_triangle_manifold<T>` must perform no action and produce an equivalent canonical artifact and zero additional displacement.

Idempotence verification must check:

- no new candidate is eligible under the same thresholds;
- output precision does not increase merely from revalidation;
- no topology changes;
- no new action-log entries except an optional no-op stage record; and
- canonical topology and geometry digest remain unchanged.

### 2.27 Failure classification

Expected cleanup difficulty must use precise typed failures.

At minimum:

- required movement/removal above tolerance: `cleanup_budget_exceeded`;
- bounded geometry cannot be validated after all permitted actions: `result_geometry_not_validated`;
- cleanup construction conditioning exceeds tolerance: `geometric_condition_exceeds_tolerance`;
- final index/entity capacity exceeded: `index_overflow` or `resource_limit`;
- caller cancellation: `cancelled`;
- unsupported or invalid policy: `input_contract_error` or `invalid_tolerance`; and
- contradiction in committed predecessor or impossible internal state: `internal_invariant_error`.

The component must not report a difficult but validly represented degeneracy as `internal_invariant_error` merely because the selected cleanup provider cannot solve it.

### 2.28 Deterministic construction and concurrency

Candidate discovery, bounded evaluation, and independent patch proposals may run in parallel over nonoverlapping snapshots. Commits must preserve the frozen deterministic action sequence.

The implementation may:

- evaluate canonical candidate batches in parallel;
- choose the globally smallest accepted candidate by total key;
- commit provably independent candidates in one deterministic nonconflicting batch; or
- use another versioned deterministic scheduler.

For batched commits, the component must prove affected patches and all geometric interaction envelopes are independent. Batch ordering must be canonical and replayable.

Worker count, task delays, allocator behavior, hash order, and speculative evaluation timing must not affect committed actions, coordinates, topology, costs, failures, or digest.

### 2.29 Resource limits and pathological cleanup

The component must account separately for:

- mutable topology entities;
- candidate records and regenerations;
- patch snapshots and rollback storage;
- vertex splits and duplicate occurrences;
- replacement coordinates and precision entries;
- action certificates;
- budget ledger records;
- local/global intersection candidates;
- verification work;
- diagnostics and replay storage; and
- persistent artifact bytes.

Adversarial meshes may generate many candidates or repeated local retriangulations. The component must charge abstract work and fail with `resource_limit` rather than stop verifying, drop obligations, cap certificates, merge unrelated vertices, or publish the current partial state.

### 2.30 Cancellation and stage transactionality

The full cleanup stage is transactional even though it contains nested committed actions in private storage.

Cancellation must be polled at deterministic safe points during candidate discovery, patch collection, bounded proposal construction, budget reservation, topology verification, local intersection enumeration, action commit preparation, candidate regeneration, and final verification.

On cancellation, all workers must join, all uncommitted reservations must return, the entire private cleaned state must be discarded, and no partial action log or topology may be visible to Component 14. The result is `cancelled`.

### 2.31 Final cleanup verification

Before publication, the component must independently reconstruct and check:

- valid entity ranges and finite bounded coordinates;
- reciprocal halfedge pairs and exact reversed endpoints;
- exactly two uses per undirected edge;
- closed triangle cycles;
- three distinct corners per triangle;
- definite accepted output orientation for every triangle;
- one closed fan per topological vertex occurrence;
- no unresolved cleanup obligation;
- no prohibited zero-length edge or zero-area triangle;
- no forbidden new nonadjacent intersection detected by the stage verifier;
- all Component 10 occurrence-separation and multiplicity constraints that survive cleanup;
- complete action-log old/new mappings;
- action certificate validity in sequence;
- cumulative budget and precision bounds;
- topology-change report consistency;
- deterministic canonical content; and
- readiness for Component 14 assembly.

The verifier must not trust mutable counters or action success flags without reconstructing the relevant member sets.

### 2.32 Independent verification evidence

The component must publish enough evidence for Component 15 or a standalone verifier to replay and check:

- initial Component 12 topology and obligations;
- every candidate selected for commit;
- every exact link/manifold eligibility test;
- every split or duplication partition;
- every replacement coordinate and displacement calculation;
- every budget reservation and committed cost;
- every replacement patch and external interface;
- every orientation and intersection check;
- every obligation transition;
- every component/genus effect;
- final manifold topology and geometry;
- canonical ID remapping; and
- deterministic digest inputs.

The independent verifier must have separately implemented link reconstruction, displacement aggregation, topology-effect calculation, and final manifold checks. It must not call the producer's candidate eligibility predicate, patch mutator, or budget accumulator as its sole source of truth.

## 3. Output contract

On success, the component must produce one immutable `cleaned_triangle_manifold<T>` artifact containing or referencing:

- artifact, cleanup-policy, action, eligibility, certificate, topology-change, precision, and serialization versions;
- canonical cleaned vertex occurrence records and IDs;
- finite authoritative bounded coordinates and precision-ledger entries;
- canonical reciprocal paired-edge and halfedge records;
- canonical oriented nondegenerate triangle records and IDs;
- exact one-closed-fan vertex-link evidence;
- complete source, event, carrier, retained-use, face-region, cycle, Component 12 triangle, and caller provenance;
- old-to-new mappings for every removed, merged, split, duplicated, moved, or retriangulated entity;
- a canonical ordered cleanup action log;
- complete per-action before/after certificates;
- discharged-obligation records and proof that none remain;
- cumulative displacement, feature-removal, component-removal, and precision ledgers;
- component-count, genus, cavity, and other topology-change reports;
- no-new-intersection and local side-plausibility evidence;
- resource and structural statistics;
- canonical input and output digests; and
- replay metadata sufficient to reconstruct every committed action and final entity.

The artifact must guarantee:

- every triangle has three distinct vertex occurrence IDs and definite nonzero outward orientation;
- every edge has exactly two opposite directed triangle uses;
- every vertex occurrence has one closed incident fan;
- every coordinate is finite and bounded;
- every Component 12 cleanup obligation is discharged with complete evidence;
- no prohibited zero-length edge or zero-area triangle remains;
- no topology was inferred from coordinate proximity;
- no occurrence-separation or coincident-sheet constraint was violated;
- every geometric move and feature removal is within the frozen policy and caller tolerance;
- output precision dominates all inherited, construction, and cleanup contributions;
- every component/genus change is authorized and reported;
- every committed action is replayable and independently verifiable;
- IDs, action order, diagnostics, and digest are independent of traversal and schedule; and
- Component 14 can assemble the public candidate mesh without further cleanup, welding, reorientation, or topology repair.

A valid empty result may produce an empty cleaned manifold with no cleanup actions and complete zero-cost ledgers. An explicitly authorized removal of all sub-tolerance components may also produce an empty cleaned manifold, but the action log and topology-change report must distinguish that case from an originally empty result.

On failure, no cleaned manifold is published. The typed error must identify the outstanding obligation, candidate/action class, affected patch, topology witnesses, displacement and removal bounds, remaining budget, intersection or orientation witness, resource counters, policy versions, and deterministic replay payload.

## 4. Required invariants and prohibited behavior

Required invariants:

- every atomic committed replacement is an oriented indexed two-manifold patch with a verified exterior interface;
- reciprocal halfedge pairing and two-use edges hold after every committed action;
- every topological vertex occurrence has one closed fan after every committed action;
- cleanup eligibility begins with exact topology and lineage, not proximity;
- budget is reserved before mutation and actual conservative cost is committed afterward;
- cumulative lineage displacement is never under-reported;
- moved or reconstructed geometry receives new bounded precision records;
- no committed action introduces inverted or unresolved final triangles;
- no committed action introduces a forbidden nonadjacent intersection;
- occurrence separation, multiplicity, and coincident-sheet semantics are preserved unless an explicitly authorized topology-change certificate says otherwise;
- every action has a complete replayable before/after certificate;
- every topology change is classified and reported;
- cleanup has a strict finite progress measure and is idempotent on success;
- all artifacts are immutable at publication, context-owned, transactional, deterministic, and independently verifiable; and
- inability to clean within policy causes typed failure rather than silent degradation.

Prohibited behavior:

- welding arbitrary nearby vertices or edges;
- using tolerance as a generic equality predicate;
- collapsing an edge without an exact manifold link check;
- deleting a triangle without repairing and verifying its paired boundary;
- merging point- or edge-touching components because coordinates coincide;
- merging separate coincident-sheet occurrences;
- moving a vertex without charging every contributing lineage;
- reporting only the largest individual displacement when edits accumulate;
- removing a component from nominal volume, triangle count, or unbounded box size alone;
- changing genus or component count without explicit policy and certificate;
- choosing coordinates from unconstrained averaging or smoothing;
- accepting a local edit without checking all affected triangle orientations;
- accepting a moved/retriangulated patch without conservative external intersection checks;
- retaining stale bounds after an edit;
- dropping cleanup obligations as side effects;
- allowing edit oscillation or unbounded retries;
- assigning final IDs or action order from pointer address, hash order, or worker timing;
- publishing partial cleanup after cancellation or resource exhaustion; or
- calling an external mesh repair, remeshing, simplification, graph, exact-arithmetic, or geometry library.

## 5. Test and validation specification

### 5.1 Atomic topology unit tests

Construct local paired manifold patches for:

- an ordinary legal edge collapse;
- a collapse failing the link condition;
- a collapse that would create a three-use edge;
- a collapse that would create a bow-tie vertex;
- a collapse joining point-touching components;
- a collapse joining edge-touching components;
- a collapse merging coincident-sheet occurrences;
- a legal collapse after fan splitting;
- a legal zero-movement duplicate-coordinate split; and
- a local retriangulation with the same external boundary.

Verify exact eligibility, reciprocal pairing, vertex links, action certificates, and rejection reasons.

### 5.2 Zero-area triangle tests

Include:

- one zero-area triangle adjacent to valid triangles;
- a two-triangle zero-area fold;
- a collinear fan;
- a triangle with distinct topological corners at one coordinate;
- a degenerate carrier endpoint patch;
- a zero-area patch requiring vertex split before collapse;
- a patch removable by retriangulation without movement;
- a patch requiring movement just within tolerance; and
- a patch requiring movement just beyond tolerance.

Verify complete obligation discharge or precise failure. Deleting the triangle alone must be caught by mutation verification.

### 5.3 Zero-length edge tests

Include:

- an ordinary zero-length collapsible edge;
- a zero-length edge whose endpoints are separate point-touching fans;
- a zero-length edge in one of several coincident edge occurrences;
- a zero-length source/carrier boundary edge whose removal is prohibited;
- a zero-length edge requiring endpoint occurrence splitting;
- a short but geometrically valid edge that must remain; and
- threshold cases below, at, and above cleanup policy limits.

Verify topology-first eligibility and no coordinate-based welding.

### 5.4 Link and fan reconstruction tests

For every action class, independently reconstruct:

- endpoint links;
- edge links;
- affected patch boundary;
- replacement vertex fans;
- duplicate edge uses; and
- connected-component membership.

Inject missing sectors, extra sectors, reversed halfedges, and disconnected links with corrected superficial counts. The independent verifier must reject them.

### 5.5 Vertex split and duplication tests

Test:

- splitting a bow-tie into two closed fans;
- separating point-touching result components without moving coordinates;
- preserving two coincident-sheet occurrences;
- splitting before a zero-length collapse;
- high-valence event fan partition;
- an invalid split that disconnects a legitimate manifold fan; and
- deterministic fan partition under cyclic-order permutations.

Verify complete member sets, duplicated lineage, zero displacement, and stable occurrence IDs after canonicalization.

### 5.6 Coordinate-choice and displacement tests

For each enabled coordinate provider, test:

- retaining endpoint 0;
- retaining endpoint 1;
- a bounded edge-interior construction;
- equal nominal costs with lineage tie ordering;
- overlapping cost intervals;
- cumulative movement through several sequential collapses;
- large translation and mixed coordinate magnitude;
- signed zero and subnormal coordinates;
- construction conditioning near failure; and
- rollback after reserved budget but failed topology verification.

Verify conservative per-lineage displacement, precision propagation, deterministic choice, and complete reservation release.

### 5.7 Collinear-chain and retriangulation tests

Include:

- a removable interior collinear vertex;
- a collinear chain on a selected boundary;
- a chain containing source/carrier feature obligations;
- a concave local patch;
- a replacement diagonal with definite clearance;
- a nominally valid but uncertain crossing replacement;
- edge-swap oscillation fixtures; and
- repeated cleanup invocation.

Verify external boundary preservation, orientation, no crossing, monotonic progress, and idempotence.

### 5.8 Thin feature and topology-change tests

Include:

- a valid thin corridor that remains;
- a removable sub-tolerance spike;
- a thin handle whose removal would change genus;
- a tiny cavity;
- a tiny disconnected tetrahedral component;
- several point-touching tiny components;
- a component below, at, and above removal threshold;
- policies that prohibit and permit whole-component removal; and
- an attempted component merge through close coordinates.

Verify default rejection of genus/component merge changes, explicit component-removal certificates, and complete topology reports.

### 5.9 No-new-intersection tests

Construct cleanup proposals where:

- the replacement patch is definitely clear;
- a moved vertex causes a definite triangle crossing;
- a new diagonal intersects remote geometry;
- the swept envelope approaches a topology-distinct coincident sheet;
- uncertainty alone prevents proving clearance;
- adjacency exclusion would be incorrect if based on coordinate equality; and
- large coordinate scales stress conservative bounds.

Compare bounded production checks against exhaustive triangle pairs for small fixtures.

### 5.10 Budget boundary tests

For displacement, cumulative displacement, feature removal, and component removal, test:

- limit minus one representable unit;
- exact limit;
- limit plus one representable unit;
- several individually valid actions whose cumulative cost exceeds tolerance;
- maximum-vs-sum policy distinctions;
- rollback of an uncommitted reservation;
- several lineages merged into one replacement vertex; and
- repeated Boolean input precision already near tolerance.

Verify ordinary success only when every applicable bound remains within policy.

### 5.11 Action replay tests

For each action class:

- serialize the pre-action patch and certificate;
- replay in a fresh private state;
- reconstruct the same replacement topology and bounded geometry;
- reproduce budget costs and topology-effect classification;
- compare before/after digests; and
- verify stale generation or changed predecessor digest causes rejection.

Replay must not invoke producer-private candidate-selection state.

### 5.12 Mutation tests

Corrupt valid cleaned artifacts or action logs by:

- omitting a required vertex split;
- collapsing an ineligible edge;
- changing one link member;
- creating a three-use edge;
- merging point-touching fans;
- changing a replacement coordinate;
- under-reporting one lineage displacement;
- dropping a prior cumulative displacement;
- changing a budget combination rule;
- omitting one affected triangle orientation check;
- hiding a new intersection;
- deleting an obligation without discharge evidence;
- misreporting component/genus change;
- reordering actions without updating generations;
- scrambling canonical final IDs; and
- forging counts or digests.

Independent verification must reject every mutation.

### 5.13 Determinism and metamorphic tests

Apply:

- source and internal entity permutations;
- alternative legal Component 12 triangulations where provider policy allows;
- operand exchange with operation remapping;
- axis permutation;
- sign flip with corrected orientation;
- exactly representable translation;
- power-of-two scaling with precision/tolerance scaling;
- thread counts 1, 2, and maximum;
- forced task delays;
- reversed candidate discovery;
- hash-collision injection;
- equivalent local fan traversal starts; and
- repeated cleanup.

For a fixed policy/provider version, committed action sequence, replacement coordinates, budgets, final topology, canonical IDs, reports, diagnostics, and digest must be byte-identical after documented remapping.

### 5.14 Fuzzing and shrinking

Generate valid paired triangulated manifolds with controlled:

- zero-area triangle count;
- zero/short-edge count;
- local fan valence;
- point/edge/coincident occurrence multiplicity;
- collinear-chain length;
- sliver quality;
- thin handle/cavity/component size;
- cleanup budget;
- coordinate scale and ULP perturbation; and
- resource limits.

For bounded small cases, enumerate legal local collapses/retriangulations and compare topological eligibility and budget outcomes to an in-tree exhaustive oracle. Every crash, non-manifold edit, budget undercount, nondeterministic action, intersection introduction, obligation leak, or verifier disagreement must serialize and shrink while preserving the failure.

### 5.15 Performance and structural gates

Measure and assert structural counters for:

- candidates discovered, invalidated, regenerated, accepted, and rejected;
- link and fan inspections;
- patch sizes;
- bounded coordinate proposals;
- intersection candidates;
- action count by class;
- budget reservations;
- obligation severity progression; and
- abstract work units.

Ordinary cleanup must remain local and output-sensitive. Pathological cases may fail at configured work limits, but verification and certificates must not be skipped.

### 5.16 Resource, cancellation, and concurrency tests

For mutable entities, candidates, patch snapshots, splits, replacement coordinates, certificates, budget entries, intersection candidates, temporary bytes, and persistent bytes, test limit-minus-one, limit, and limit-plus-one.

Cancel during candidate discovery, patch collection, link checking, coordinate proposal, budget reservation, replacement construction, intersection enumeration, certificate creation, batch commit, obligation regeneration, and final verification. Confirm all workers join, all reservations return, and no partial cleaned artifact is visible.

### 5.17 Definition of done

Component 13 is complete only when:

- every Component 12 cleanup obligation is discharged or the stage fails precisely;
- every committed action preserves an oriented indexed two-manifold after atomic commit;
- no prohibited zero-area triangle or zero-length edge remains on success;
- no unrelated features are welded by tolerance or coordinate proximity;
- cumulative lineage displacement and feature-removal costs are conservative and within tolerance;
- all changed geometry has updated bounded precision;
- no committed action introduces a forbidden intersection or invalid orientation;
- topology changes are policy-authorized, classified, and reported;
- action logs and local certificates replay independently;
- cleanup terminates and is idempotent;
- independent mutation verification is effective;
- deterministic replay is byte-stable across schedules; and
- all production and normative-test code is strict portable C++17 with no external dependencies.
