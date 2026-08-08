# Component 04: Source Polygon Facet Triangulation and Provenance

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production and normative-test code must be portable C++17 and use no external dependency.

The concrete triangulation provider may change. The source-boundary, geometry-basis, coverage, provenance, determinism, verification, resource, and failure contracts in this document are normative.

## 0. Purpose

Component 04 converts every geometry-eligible polygonal source facet in one immutable `validated_operand<T,I>` into a deterministic oriented triangle complex suitable for Component 05 halfedge construction and the later collision, relation, event, winding, and selection stages.

The conversion must preserve:

- the exact indexed source ring and every retained source vertex identity;
- every source directed-edge use and its reciprocal source-edge provenance;
- the source facet's accepted orientation, shell membership, and semantic identity;
- the bounded planar region certified by Component 02's coherent geometry basis;
- complete source-to-triangle and triangle-to-source provenance; and
- independence of Boolean source-feature semantics from the chosen legal internal triangulation.

Internal diagonals are facet-local bookkeeping. They refine one source facet into triangles but are not original geometric features. They must not become source-edge collision features, symbolic-contact owners, winding barriers, or source-feature identities.

The principal output is one immutable, independently verified `source_triangle_complex<T,I>` per validated operand.

## 1. Input contract

### 1.1 Required inputs

The component must accept:

- one immutable ordinary-pipeline-eligible `validated_operand<T,I>` from Component 02;
- the immutable Boolean context, identity domains, error services, resources, cancellation, transactions, replay, canonical-byte, and deterministic-execution services from Component 01;
- the immutable precision context and bounded operation, predicate, construction, feature-bound, and verification services from Component 03;
- the frozen source-triangulation policy and provider versions; and
- a narrow immutable view of Component 02 source vertices, facets, rings, corners, directed and undirected source edges, shells, projection/plane records, coherent-realization certificate, and stable facet-coverage evidence.

The runtime context owner is required for handle validation and lifetime safety. It is not semantic data and must never enter canonical keys, canonical ordering, digests, replay bytes, deterministic diagnostics, or output.

### 1.2 Required predecessor guarantees

For every accepted non-empty facet, Component 02 must already have established and published evidence for:

- one normalized simple source ring with at least three retained positions;
- no repeated retained source vertex identity;
- one source corner and one directed source-edge use per retained ring position;
- exact reciprocal source-edge pairing across adjacent source facets;
- shell membership and accepted source orientation;
- one certified support plane and deterministic projection frame;
- a definite non-zero projected orientation under the coherent geometry certificate;
- a coherent operand-level geometry disposition of either `nominal_embedded` or `constructed_coherent_realization`;
- a stable reference to the point realization used to certify the facet geometry; and
- reconstructible boundary/coverage evidence from the validation-only decomposition or an equivalent certificate.

A topology-only predecessor is ineligible for ordinary Component 04 processing.

### 1.3 Authoritative geometry basis

Component 04 must not choose an independent geometric realization.

For each facet it must consume the exact geometry basis committed by Component 02:

- for `nominal_embedded`, the committed geometry basis references the nominal source points together with their inherited Component 03 enclosures;
- for `constructed_coherent_realization`, the committed geometry basis references the single shared realized point for each source vertex, its proof of containment in the source precision envelope, and the same support-plane/projection/orientation convention used by Component 02.

All topology-affecting triangulation predicates, local embedding checks, and coverage checks must use that committed coherent geometry basis. The source nominal coordinate bits and source precision lineage remain preserved as source provenance. Consuming a constructed coherent realization is validation against an accepted representative; it is not a source-topology edit, cleanup displacement, coordinate weld, or permission to replace public source coordinate lineage.

If the geometry-basis records are missing, inconsistent, stale, cross-owner, or cannot be reproduced through Component 03, the component must fail deterministically. It must not silently fall back from realized points to nominal points or select a different plane or projection.

### 1.4 V1 accepted policy

V1 is one-ring, no-Steiner, no-source-edge-subdivision, no-source-vertex-removal, and non-degenerate-output.

For a facet with `n` retained ring positions, a successful V1 result contains exactly:

- `n - 2` definitely oriented triangles;
- `n - 3` facet-local internal diagonals;
- `n` source-boundary triangle edge uses; and
- `2(n - 3)` internal-diagonal triangle edge uses.

Every retained source vertex identity must occur in the triangle complex. If the committed coherent geometry cannot support such a triangulation without a zero or uncertain triangle, uncertified diagonal, boundary edit, or identity merge, return a typed deterministic geometry failure.

This component does not accept arbitrary polygon soup and does not repair malformed rings.

## 2. Required behavior

### 2.1 Contract and capability validation

Before allocating or evaluating a facet, validate:

- owner, operand, type, artifact, provider, policy, formula, and codec versions;
- predecessor semantic and geometry-certificate digest links;
- ordinary geometry eligibility;
- source count and ID-domain representability;
- facet/ring/corner/edge/shell ownership and range consistency;
- resource and work limits;
- transaction stage and cancellation state; and
- availability of the required Component 03 predicate families.

Expected malformed-input, geometric-condition, resource, cancellation, and unsupported-version failures must remain typed. A committed predecessor contradiction is an internal invariant failure.

### 2.2 Deterministic projection workspace

For each facet, validate and reuse Component 02's support-plane and projection record. Materialize the projected bounded points from the committed coherent geometry basis through Component 03 using the frozen formula IDs.

The workspace must:

- preserve the accepted source orientation;
- retain source vertex and corner identities separately from projected values;
- preserve exact source nominal bits and realized-point lineage for diagnostics;
- remain independent of caller ring rotation, source array order, allocation order, and worker scheduling; and
- fail if the stored frame cannot support finite conservative predicates.

Equal or overlapping projected coordinates never merge identities. A nominal coordinate coincidence within one ring is eligible only when Component 02's coherent realization and positive-extent checks made the retained source topology geometry-eligible.

### 2.3 Deterministic triangle generation

The provider must partition the committed coherent source-facet region into oriented triangles whose vertices are existing retained source vertex identities.

A conforming provider must:

- preserve every source boundary edge exactly and in the source ring direction;
- introduce only facet-local diagonals between nonconsecutive retained source vertices;
- emit no positive-area overlap and no positive-area gap;
- emit no triangle whose bounded orientation overlaps zero or disagrees with the source orientation;
- terminate with a complete result or a typed failure;
- use only Component 03 bounded predicates for topology-affecting geometry;
- use stable semantic identities for every tie and ordering decision; and
- retain enough evidence for independent reconstruction.

V1 should use a deterministic certified ear-clipping provider with a separately executable serial semantic reference. Another in-tree provider is permitted only if it produces the same frozen V1 result or uses a new provider/policy version with equivalent contracts.

### 2.4 Ear eligibility and complete candidate maintenance

For an active ring node `c` with predecessor `p` and successor `q`, an ear candidate `(p,c,q)` is eligible only when all prescribed bounded relations are definite:

- accepted triangle orientation;
- diagonal inside both endpoint local cones;
- no prohibited relation with a nonincident active boundary segment;
- no other active source vertex on the proposed diagonal;
- no other active source vertex in or on the closed ear triangle; and
- all predicate evidence belongs to the frozen owner, geometry basis, and formula set.

The selected ear is the least complete semantic candidate key. Runtime owner values, pointer values, task IDs, heap insertion order, and caller positions are excluded from the key.

An optimized incremental provider must maintain a complete eligible set. After every removal it must invalidate and re-evaluate every candidate whose result depended on:

- the removed active vertex;
- either removed active boundary segment;
- the newly introduced active boundary segment;
- a changed predecessor or successor relation;
- an active point or segment whose membership in a candidate's conservative query set changed; or
- any cached predicate/evidence object whose generation changed.

A conforming implementation may satisfy this by a full canonical rescan after every removal. An optimized implementation must maintain explicit reverse dependency or blocker records and prove byte-for-byte equivalence to the full-rescan serial reference. Re-evaluating only the two new neighboring nodes is not sufficient.

### 2.5 Boundary-edge preservation

Every Component 02 source directed-edge use must map to exactly one directed triangle boundary edge. Under V1 no source edge may be subdivided.

The mapping must preserve:

- source directed and undirected edge IDs;
- source corner and ring-position identity;
- origin and destination source vertex IDs;
- source-facet and shell identity;
- direction within the source ring;
- reciprocal directed source use on the adjacent source facet; and
- caller-source provenance for diagnostics.

The two incident source facets may triangulate independently, but their shared source edge must retain the same endpoint identities and opposite directions. No snapping or coordinate-based pairing is permitted.

### 2.6 Internal diagonal semantics

Every non-boundary triangle edge must be assigned one facet-local internal-diagonal identity and paired with exactly one opposite use in another triangle of the same source facet.

An internal diagonal is schema-defined as:

- not a source edge;
- not eligible for source-feature relation ownership;
- not eligible for the Component 06 canonical source-edge-versus-triangle candidate domain;
- not eligible for operation-specific symbolic contact ownership;
- not a connectivity or winding barrier inside an uncut source facet;
- not a source-boundary provenance substitute; and
- replaceable by another legal triangulation without changing source-facet semantics.

Internal diagonals may be used for triangle adjacency, local traversal, conservative triangle bounds, and verification. Downstream code must distinguish them from source edges by identity alone.

### 2.7 Provenance and relation lineage

The component must publish total immutable mappings for:

- source facet to generated triangles;
- generated triangle to source facet, ring, shell, and operand;
- triangle vertices to source vertex/corner provenance;
- triangle source-boundary edges to exact source directed and undirected edge identities;
- triangle internal edges to facet-local internal-diagonal identities and opposite uses;
- triangle orientation and geometry-basis references;
- caller facet/ring/vertex positions through Component 02 presentation maps; and
- semantic source-relation lineage required by Components 05-10.

Triangle-local acceleration identities are layout identities, not source-feature identities. Any later relation found while testing one generated triangle must retain the authoritative source facet and applicable source edge/vertex lineage so a different legal triangulation cannot change semantic ownership.

### 2.8 Canonical identities and ordering

Canonical semantic ordering must use only versioned semantic keys derived from:

- operand role;
- canonical source facet, ring, shell, vertex, corner, and source-edge identities;
- orientation-preserving vertex cycles;
- internal-diagonal endpoint IDs;
- frozen policy/provider/formula versions; and
- explicit role tags.

The runtime owner token may be stored in handles and records for validation but is excluded from all canonical keys, bytes, digests, deterministic errors, replay, and output.

Triangle IDs and diagonal IDs are dense artifact-local strong IDs assigned transactionally after complete key sorting and duplicate validation. Hashes may accelerate lookup only when full keys decide equality.

### 2.9 Coverage and local-manifold evidence

For every facet, producer verification must reconstruct from final triangle proposals:

- exact source boundary equality and direction;
- exactly two opposite uses for each internal diagonal;
- expected triangle, diagonal, edge-use, vertex, and Euler counts;
- one connected triangle-dual tree;
- definite accepted orientation for every triangle;
- no forbidden triangle crossing or positive-area overlap under the committed coherent geometry basis;
- each triangle and internal-diagonal interior lying inside the source ring;
- stable independent source-region witness coverage;
- conservative source-polygon versus triangle-area agreement; and
- complete provenance and geometry-basis consistency.

Area agreement alone is never sufficient. Coverage evidence must combine exact indexed boundary cancellation, dual connectivity, pairwise embedding checks, interior classifications, and witnesses.

Component 02 validation-decomposition records may provide additional stable source-region witnesses, but Component 04 must not treat Component 02 private triangle IDs or a producer-owned `coverage=true` flag as authority. The Component 04 independent verifier must generate or classify its own deterministic witness set from immutable source-ring and geometry-basis records.

### 2.10 Transactionality, resources, and cancellation

Facet work may use private mutable state. No facet or operand result is published until all facets pass producer checks, the operand artifact is canonically assembled, and the independent verifier accepts it.

Resource accounting must cover:

- projected bounded points and geometry-basis references;
- active-ring nodes and generation records;
- candidate sets, reverse dependencies, blocker records, and conservative indexes;
- predicate and coverage evidence;
- triangle, edge-use, diagonal, provenance, and mapping proposals;
- sort keys, canonical bytes, digests, diagnostics, replay, and verifier work; and
- abstract work units for all potentially adversarial loops.

A pathological facet must terminate through a deterministic typed resource/work failure. Cancellation is polled only at stable boundaries; all private work, provisional IDs, and reservations are rolled back before return.

## 3. Output contract

For each operand, publish one immutable `source_triangle_complex<T,I>` containing at least:

- artifact/provider/policy/formula/codec/verifier versions;
- runtime owner validation metadata excluded from canonical semantics;
- operand role and predecessor semantic/geometry-certificate digest links;
- source vertex references with nominal source bits, precision lineage, and coherent-realization references;
- canonical oriented source triangles;
- canonical facet-local internal diagonals;
- one edge-role record for every directed triangle edge;
- source-facet triangle and diagonal membership maps;
- source directed-edge to triangle-boundary-use maps;
- source undirected-edge to its two cross-facet triangle-boundary uses;
- source vertex to incident triangle mappings;
- deterministic projection and geometry-basis records;
- per-triangle orientation, plane, bound, and provenance evidence;
- per-facet coverage and verification evidence;
- separate source-semantic, exact-triangulation, replay/presentation, and artifact digests;
- canonical arrays and dense ID ranges; and
- exact resource/work statistics and immutable predecessor lifetime references.

The source-semantic digest excludes internal triangle/diagonal choices. The exact-triangulation digest includes them. Neither digest includes the runtime owner token or presentation-only caller ordering.

The output guarantees:

- every accepted source facet is completely triangulated under the committed coherent geometry basis;
- source boundary topology and identity are unchanged;
- every source boundary use appears exactly once in source direction;
- every internal diagonal has exactly two opposite same-facet uses;
- every triangle has definite accepted orientation;
- all provenance and reverse mappings are total and immutable;
- source features and triangulation artifacts are distinguishable by identity alone;
- internal diagonals cannot enter the canonical source-edge candidate domain or own source semantics; and
- no failed, partial, cancelled, resource-exhausted, or verifier-rejected artifact is published.

On failure, return one typed deterministic error identifying the least canonical offending operand/facet/ring/corner/vertex/edge/relation key, relevant bounded evidence, geometry-basis reference, resource state, and replay payload.

## 4. Required invariants and prohibited behavior

Required invariants include:

- source-ring boundary equality in exact indexed topology;
- one committed coherent geometry basis reused from Component 02;
- no runtime owner value in canonical semantics;
- exact `n - 2` triangle and `n - 3` diagonal counts under V1;
- all retained source vertices represented;
- definite source-consistent triangle orientation;
- opposite-paired facet-local diagonals;
- total immutable provenance;
- complete eligible-ear maintenance;
- deterministic full-key ordering and ID assignment;
- legal source re-triangulation changes only exact layout identity, never source-feature semantics; and
- producer and independent verifier acceptance before publication.

Prohibited behavior includes:

- tolerance-based welding or coordinate-key deduplication;
- deleting collinear or repeated-coordinate identities to make triangulation easier;
- choosing a different plane, projection, or point realization than Component 02 committed;
- using nominal points when a constructed coherent realization is the committed geometry basis;
- spending cleanup tolerance during source triangulation;
- publishing zero/uncertain triangles under V1;
- classifying an internal diagonal as a source edge or broad-phase source-edge feature;
- using internal diagonals as symbolic owners or winding barriers;
- re-evaluating only local ear neighbors without complete dependency proof;
- including owner pointers/tokens, addresses, task order, source order, or unordered-container iteration in semantic ordering;
- relying only on area equality or producer booleans for coverage;
- calling a legacy triangulator as the production provider; or
- using any external dependency.

## 5. Test and validation specification

### 5.1 Known-answer geometry-basis corpus

Include hand-auditable facets covering:

- triangles, convex polygons, and concave polygons with several reflex vertices;
- several simultaneously legal ears;
- long thin polygons and narrow channels;
- retained collinear boundary chains;
- nearly collinear ears immediately inside and outside certainty;
- large translations with small local features and mixed exponents;
- signed zero, subnormal coordinates, and projection-axis ties;
- nominally coincident distinct source identities separated by a valid constructed coherent realization;
- corresponding collapsed or uncertifiable cases rejected by Component 02 or Component 04; and
- large retained ring counts near resource limits.

Every successful fixture stores the Component 02 geometry-basis disposition, projection/formula versions, expected canonical triangles/diagonals, provenance, coverage evidence summary, and golden canonical bytes.

### 5.2 Geometry-basis integration tests

Verify that Component 04:

- uses nominal geometry only for a `nominal_embedded` predecessor;
- uses the exact committed shared realized points for `constructed_coherent_realization`;
- preserves source nominal bits and precision lineage in both cases;
- rejects a mutated realization point, plane, projection, formula, or predecessor digest;
- never silently switches geometry basis; and
- obtains identical semantic results under presentation-only input permutations.

### 5.3 Coverage and independent verification tests

Independently reconstruct:

- source boundary use equality;
- internal-diagonal grouping and direction;
- triangle-dual connectivity;
- count and Euler identities;
- triangle orientations;
- pairwise triangle embedding;
- triangle and diagonal interior-in-source classifications;
- verifier-generated source-region witness coverage;
- Component 02 stable witness coverage where available; and
- conservative area agreement.

Tests must not call producer ear-selection, active-ring, dependency, boundary-grouping, dual-traversal, or cached-coverage helpers.

### 5.4 Complete candidate-maintenance tests

Construct polygons where removing one ear:

- removes a vertex that blocked a distant candidate's point-in-ear test;
- removes a boundary segment that blocked a distant diagonal;
- introduces a new diagonal that invalidates another cached candidate;
- changes conservative index query membership far from the removed node; and
- causes several stale candidates to become eligible or ineligible simultaneously.

Compare every optimized provider step with a full canonical rescan. Mutation tests that omit any reverse dependency or blocker invalidation must be rejected or produce a differential failure.

### 5.5 Determinism and owner-separation tests

Apply ring rotation, facet/vertex/shell permutation, exactly representable translation, power-of-two scale, axis permutation, corrected whole-shell orientation reversal, worker scheduling permutation, and repeated identical invocations with different runtime owner anchors.

Require:

- identical semantic keys, canonical bytes, and digests for semantically identical artifacts;
- identical V1 exact triangulation when all predicate dispositions are unchanged;
- no runtime owner bytes in canonical encoding or deterministic diagnostics; and
- correct owner rejection for cross-context handle misuse despite semantic byte equality.

### 5.6 Alternative-triangulation and downstream tests

For source facets with multiple legal triangulations:

- enumerate alternative legal triangle sets with the test exact oracle;
- run them through the same provenance, codec, and independent verifier path;
- require identical source-semantic digests and different exact-triangulation digests when layouts differ;
- verify Component 05 reconstructs equivalent source-facet groups;
- verify Component 06 excludes internal diagonals from the source-edge candidate domain;
- verify Components 07-10 use source facet/edge/vertex lineage rather than triangle layout identity; and
- verify final Boolean topology is unchanged for cross-operand interactions that cross different internal diagonals.

### 5.7 Boundary-sharing tests

Construct adjacent source facets with different projections, long edges, high-valence vertices, duplicate coordinate values elsewhere, signed-zero endpoints, subnormal offsets, and presentation permutations. Require exactly two opposite triangle boundary uses for each Component 02 source undirected edge.

### 5.8 Mutation tests

The independent verifier must reject at least:

- deleted, duplicated, reversed, overlapping, or wrong-facet triangles;
- changed triangle vertex cycle or edge-slot alignment;
- zero/uncertain or substituted orientation evidence;
- missing, duplicated, same-direction, or cross-facet diagonal uses;
- source boundary use missing, reversed, relabeled, or given wrong endpoints;
- internal diagonal marked source-feature eligible or source-edge-candidate eligible;
- source edge marked internal;
- changed coherent geometry-basis disposition/reference;
- nominal geometry substituted for a constructed realization;
- owner token inserted into canonical bytes or keys;
- changed semantic/exact digest separation;
- disconnected triangle dual;
- source coverage gap hidden by area compensation;
- incomplete candidate dependency records in optimized-provider trace tests;
- underreported resources or escaped private handles; and
- unknown versions, roles, fields, or nonzero reserved bits.

### 5.9 Exact-oracle, fuzzing, and shrinking

Use Component 16's in-tree test-only arbitrary-precision integer/rational facilities, or the shared provisional qualification implementation, to determine exact orientation, segment relations, legal triangulations, V1 least-ear selection, coverage, and witness classifications for bounded fixtures.

Fuzz valid simple polygons and controlled invalid mutations across ring size, reflex patterns, collinear runs, narrow channels, exponent range, projection ties, coherent-realization displacement, and uncertainty thresholds. Every failure must preserve exact source bits, geometry-basis records, policies, limits, and failure category during deterministic shrinking and must become a permanent regression when fixed.

### 5.10 Performance, limits, cancellation, and fault injection

Track structural counters for projected points, candidate evaluations, dependency edges, invalidations, full rescans, segment and point relations, triangles, diagonals, pair audits, verifier work, bytes, resources, and canonical encoding.

Require:

- linear persistent output storage;
- no accidental quadratic persistent dependency storage beyond the documented provider;
- near-linear or `O(n log n)` ordinary production behavior after optimized indexing is enabled;
- byte-for-byte equivalence with the bounded full-rescan reference;
- deterministic work-limit failure for adversarial polygons;
- limit-minus-one, limit, and limit-plus-one tests for each resource class;
- cancellation and allocation failure at every stable checkpoint;
- zero partial publication, zero leaked reservations, and no predecessor mutation; and
- identical selected failures across worker counts and schedules.

### 5.11 Definition of done

Component 04 is complete only when:

- the committed coherent geometry basis from Component 02 is explicit and reused consistently;
- runtime owner validation is separated from all semantic ordering and encoding;
- V1 no-Steiner/non-degenerate count and boundary contracts are implemented;
- every topology-affecting decision uses Component 03 bounded predicates;
- complete ear-candidate maintenance is proven against the full-rescan reference;
- source boundary and internal diagonal roles are unambiguous and independently verified;
- internal diagonals are excluded from source-edge collision and symbolic ownership domains;
- canonical IDs, bytes, and digests are stable under all required metamorphisms and owner-anchor changes;
- producer and independent coverage verification reject every required mutation;
- alternative legal triangulations preserve downstream source-feature semantics and Boolean topology;
- all resource, cancellation, overflow, transaction, replay, codec, fuzz, exact-oracle, and performance obligations pass; and
- production and normative-test code remain strict portable C++17 and dependency-free.
