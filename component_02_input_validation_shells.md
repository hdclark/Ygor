# Component 02: Input Topology Validation and Shell Semantics

## Status and normative language

This document specifies a required component of a dependency-free bounded floating-point surface-mesh Boolean engine. Production code and normative tests must use portable C++17 and the C++ standard library only. No external library may be required, linked, vendored, downloaded, or invoked.

The implementation strategy may change, but the observable contracts, failure behavior, invariants, evidence requirements, and test obligations in this document are normative.

## 0. Purpose

Component 02 converts each Component 01 immutable source-mesh snapshot into one immutable `validated_operand<T,I>` describing a regular closed solid under the selected V1 shell policy.

It is responsible for:

- structural validation of source counts, indices, and polygon rings;
- exact indexed edge incidence and closed two-manifold vertex-link validation;
- discovery and canonical identification of disconnected shells;
- bounded facet-planarity, projection, simplicity, and orientation validation;
- a conservative, coherent certificate that the accepted source geometry is epsilon-valid;
- intrinsic shell orientation, pairwise shell-boundary compatibility, nesting, and occupied-side semantics;
- deterministic canonical source identities and reversible presentation mappings;
- independently reconstructible validation evidence; and
- typed fail-closed behavior for malformed, ambiguous, unsupported, or insufficiently certifiable input.

This component is a validator and semantic interpreter, not a general repair system. It must not weld nearby vertices, close cracks, infer missing faces, split bow-tie vertices, delete source features, resolve self-intersections by local edits, or silently reverse shells.

The principal output is one immutable `validated_operand<T,I>` per operand, consumed by Components 04, 05, 14, and 15. Component 02 does not publish source triangles, internal halfedges, Boolean relations, output topology, or an ordinary Boolean success.

## 1. Input contract

### 1.1 Required inputs

For each operand, Component 02 must accept:

- an immutable `immutable_source_mesh_view<T,I>` or equivalent snapshot from Component 01;
- the immutable Boolean context and operand identity from Component 01;
- the frozen input precision, solid policy, input-contact policy, and verification level;
- owner-checked resource, cancellation, diagnostic, transaction, canonical-byte, replay, and identity-publication capabilities from Component 01; and
- the immutable precision context and bounded arithmetic, predicate, construction, enclosure, and conservative-bound capabilities from Component 03.

V1 source snapshots contain only the semantic public-mesh fields frozen by Component 01:

- exact source vertex coordinate bits in source order; and
- exact source facet ring lengths and index values in source order.

Component 02 must not read, validate, copy, diagnose, canonically encode, or derive semantics from `vertex_normals`, `vertex_colours`, `involved_faces`, or arbitrary `metadata`. Input precision and shell semantics come only from frozen options. Changing an ignored public field must not affect Component 02 because that field is absent from its input capability.

### 1.2 Supported type and lifetime domain

V1 ordinary production supports the Component 01 qualified combinations:

- `T == float` or `T == double`; and
- `I == std::uint32_t` or `I == std::uint64_t`.

The source snapshot, context, precision context, and all capabilities must have the same owner, operand, type profile, policy versions, and invocation lifetime. No caller-owned vector, nested face storage, string, map, pointer, or mutable reference may be retained or reread.

### 1.3 Accepted topological domain

After the permitted ring normalization in Section 2.2, every non-empty accepted operand must satisfy:

1. every source coordinate is finite;
2. every source index is representable and in range;
3. every retained facet ring has at least three positions and three distinct source vertex identities;
4. no retained ring repeats a source vertex identity;
5. every directed ring edge has a distinct origin and destination identity;
6. every undirected source edge has exactly two directed uses in opposite directions;
7. every referenced source vertex has exactly one closed cyclic incident-facet fan;
8. every connected topological shell is locally consistently oriented; and
9. all referenced source features belong to exactly one discovered shell.

An operand with no facets is the empty regular solid. Finite isolated source vertices may exist in the Component 01 source snapshot, but they are non-semantic: they receive no shell membership, no canonical topology identity, and no Component 04 handoff record.

### 1.4 Accepted geometric and solid-semantic domain

Every accepted non-empty operand must additionally satisfy:

1. each facet has a certified bounded support-plane model and a simple one-contour polygonal boundary;
2. Component 02 has a coherent epsilon-validity certificate, not merely independent per-facet residual checks;
3. every accepted facet has a non-zero orientation under that certificate;
4. every source edge and vertex neighborhood is geometrically compatible with the indexed manifold topology;
5. each shell has a definite intrinsic geometric orientation;
6. every shell pair is certified as disjoint, strictly nested, or in an explicitly authorized zero-volume contact relation before parentage is finalized;
7. shell nesting and occupied-side semantics are unique under the selected V1 policy;
8. every accepted oriented boundary has occupied material on its negative side and empty space on its positive side;
9. away from boundaries, total oriented winding is zero or one; and
10. no forbidden self-intersection, positive-volume shell overlap, unsupported coincidence, or uncertainty spanning incompatible topology remains.

V1 uses outward-oriented exterior shells with alternating nested cavity/island orientation. Point- and edge-touching disconnected shells remain topologically separate. Entirely coincident separate shells are rejected unless a future versioned solid policy defines their multiplicity semantics.

Duplicate coordinate values are permitted and never merge topology. This permission does not guarantee geometric acceptance: an adjacent source edge with no certified positive extent, a non-adjacent coordinate-coincident polygon corner, or another collapsed configuration may be rejected as non-epsilon-valid.

### 1.5 Permitted canonicalization

Component 02 may perform only these topology-preserving source normalizations:

- remove consecutive repetitions of the same source vertex index within one ring;
- remove one final ring index equal to the first retained index;
- rotate a retained ring to an orientation-preserving canonical start;
- assign canonical topology identities and canonical record order;
- canonicalize disconnected-shell record order; and
- normalize equivalent internal evidence encodings without changing their meaning.

It must not reverse an individual ring or shell under the strict V1 policy. A future whole-shell reorientation policy requires a new explicit policy and artifact version, complete provenance, and revalidation.

## 2. Required behavior

### 2.1 Capability, scalar, count, and index validation

Before dereferencing an index or allocating derived topology, Component 02 must validate:

- context, operand, owner, type, policy, provider, and schema compatibility;
- source vertex, facet, ring-position, and byte counts with checked arithmetic;
- representability in Component 01 count and strong-ID domains;
- resource ceilings and deterministic work budgets;
- finite source coordinates;
- safe widening and range validation of every source index; and
- eligibility of the requested geometry-verification disposition.

Malformed source topology is an expected typed input failure. Resource exhaustion, cancellation, and unsupported capability versions must not be reported as geometry invalidity or `internal_invariant_error`.

### 2.2 Index-only ring normalization

For every source facet, Component 02 must:

1. copy its exact index sequence from the immutable source snapshot;
2. remove consecutive equal indices while recording every removed source position;
3. remove a final retained index equal to the first while recording that action;
4. reject fewer than three retained positions or fewer than three distinct identities;
5. reject any remaining repeated source vertex identity;
6. preserve the input direction; and
7. create a total reversible mapping between source ring positions and retained corners or explicit removal actions.

No coordinate comparison may remove, merge, or reorder a source occurrence.

Before global canonical IDs exist, any local work order or error key must use an orientation-preserving cyclic semantic encoding derived from exact coordinate bits and normalized ring structure. Caller vertex indices, facet ordinals, allocation order, and a first-minimum tie chosen from source order must not affect semantic decisions. Equal minimal rotations form one local automorphism class; presentation-specific representatives are diagnostic only.

### 2.3 Exact indexed incidence

Component 02 must emit one directed edge use per retained ring edge and group uses by the unordered pair of source vertex identities.

For every undirected source edge, it must verify:

- exactly two uses exist;
- the uses have opposite directions;
- both uses reference valid retained ring positions;
- reciprocal pairing is exact and owner-consistent; and
- no duplicate record or count overflow is hidden by sorting or grouping.

Distinct diagnostics are required for an open boundary, more than two uses, same-direction pairing, self-edge, duplicate use, invalid source location, and representability failure.

Pairing is based only on indexed identity. Equal or nearby coordinates have no incidence effect.

### 2.4 Vertex-link manifoldness

For every referenced source vertex, Component 02 must reconstruct the corner link from paired edge uses and facet corners. A valid closed two-manifold occurrence has one connected cycle.

The validator must independently detect and reject:

- an open link chain;
- more than one link cycle sharing one source vertex identity;
- a repeated or missing link incidence;
- non-bijective predecessor or successor relations;
- a bow-tie vertex whose edge counts appear manifold; and
- any owner, endpoint, facet, or ring inconsistency.

The producer and independent verifier must use materially different link-reconstruction control flow.

### 2.5 Shell discovery

After edge and link validation, Component 02 must discover connected shells solely through paired-edge facet adjacency. Coordinate contact between separate components must not create connectivity.

For every shell, it must establish complete member sets for vertices, edges, facets, rings, and corners, plus conservative bounds and stable structural evidence. Every referenced source feature must belong to exactly one shell.

### 2.6 Canonical source identities

Canonical source identities must be assigned only after structurally valid topology and temporary shells are known.

The canonicalization contract must:

- be invariant under vertex-array permutations with index remapping, facet permutations, ring rotations, disconnected-shell permutations, traversal order, allocation order, and worker schedule;
- use exact coordinate bits only as vertex labels, never as a merge rule;
- use complete typed topology and orientation-preserving incidence keys;
- compare complete canonical encodings rather than hashes alone;
- resolve graph automorphisms by a bounded deterministic canonical-labeling procedure; and
- assign dense Component 01 strong IDs only after the winning semantic labeling is known.

When more than one source-to-canonical bijection realizes the same minimal canonical graph, the semantic artifact and canonical IDs must still be identical. A presentation map may choose any verified bijection within that exact automorphism class, but that choice must be excluded from semantic bytes and must not affect downstream topology, primary error semantics, or canonical digests. Diagnostics must preserve all source positions or an explicit equivalence-class record when a unique semantic source location does not exist.

### 2.7 Bounded facet geometry

Using the final canonical ring order and Component 03 only, Component 02 must:

- import every referenced source coordinate as a bounded source point;
- choose a deterministic, well-conditioned support-plane proposal;
- verify planarity against the operand input-precision contract without spending user cleanup tolerance;
- choose and record a deterministic projection frame;
- reject projected proper crossings, non-adjacent touches, collinear overlaps, and uncertifiable simplicity;
- certify non-zero orientation and area under the selected frame; and
- construct a private validation-only no-Steiner decomposition or equivalent coverage certificate for later shell and interaction queries.

The private decomposition is not the Component 04 source-triangle artifact. It owns no source-triangle IDs and may not change normalized source topology.

### 2.8 Coherent epsilon-validity certificate

Component 02 must not publish `epsilon_valid` from unrelated local interval tests alone. It must retain a coherent certificate describing one globally compatible realization or a stronger sufficient proof.

At minimum, an accepted certificate must establish:

- one shared realization record for every referenced source vertex;
- displacement or enclosure evidence showing each realization lies within that source vertex's declared input-precision allowance;
- one compatible support-plane realization per source facet;
- consistency of each shared realization vertex with every incident facet constraint;
- simple, non-zero, orientation-compatible realized facet boundaries;
- exact indexed edge and vertex-link topology unchanged by the realization;
- an embedded shell geometry except for explicitly authorized zero-volume contacts; and
- no uncertainty region that permits two incompatible topological interpretations.

A conforming V1 provider may support conservative certificate classes, for example:

- `nominal_embedded`, where the nominal source coordinates themselves satisfy the complete embedded-manifold checks; and
- `constructed_coherent_realization`, where a deterministic Component 03 bounded feasibility/construction procedure proves one shared perturbed realization.

If no supported coherent certificate can be produced, the operand must be rejected even when independent facet residuals appear small. A topology-only diagnostic artifact may record incomplete geometry evidence, but it is nonpublishable for ordinary Boolean evaluation.

### 2.9 Validation relation kernel

Component 02 must implement or consume a versioned validation-specific bounded relation kernel for:

- projected segment/segment and point/polygon relations;
- point/shell classification;
- non-coplanar triangle/triangle intersection;
- coplanar triangle overlap and contact dimension;
- adjacent-facet contact confinement to the shared source feature; and
- shell-pair boundary relation classification.

The kernel must use Component 03 primitive arithmetic and exact-nominal tie evidence, but it must not use the operation-specific symbolic ownership rules of Component 07 to make an intrinsically invalid input valid.

Every relation is classified as definite disjointness, authorized shared-feature contact, authorized policy contact, forbidden interaction, or unresolved uncertainty. Unresolved uncertainty that can change topology, parentage, or occupancy is a typed failure.

### 2.10 Shell geometric orientation

For each shell, Component 02 must determine which geometric side of its locally consistent oriented boundary is interior. Evidence may combine:

- bounded signed-volume accumulation over the coherent realization;
- deterministic two-sided facet probes with certified clearance; and
- independent bounded point-against-shell classification.

A signed-volume interval containing zero is inconclusive. A probe overlapping any boundary enclosure is ambiguous. Definite evidence paths must agree; contradiction is a producer/provider invariant failure.

### 2.11 Shell-pair compatibility before nesting

Before assigning any parent shell, Component 02 must certify each potentially interacting shell pair as one of:

- definitely disjoint;
- definitely strict containment with disjoint boundaries;
- authorized point contact;
- authorized edge contact;
- authorized face contact with no positive-volume overlap and unique occupied-side semantics; or
- unsupported, forbidden, or ambiguous.

Bounding boxes may prune candidates but may not prove containment. A child probe inside another shell is insufficient when the two boundaries have not first been certified compatible.

Entirely coincident separate shells, transverse shell intersections, coplanar positive-area overlap not covered by the input-contact policy, sibling positive-volume overlap, and uncertainty that can change parentage must fail.

### 2.12 Nesting, orientation parity, and occupied sides

Using certified shell-pair relations and canonical interior probes, Component 02 must construct an acyclic deterministic nesting forest.

Under the strict V1 alternating-shell policy:

- exterior roots have depth zero and are geometrically outward oriented;
- odd-depth shells bound cavities and have the opposite geometric orientation;
- orientation alternates again for deeper islands;
- every accepted oriented boundary has occupied material on its negative side and empty space on its positive side; and
- total oriented winding away from boundaries is zero or one.

A reversed but locally consistent shell is rejected under V1. Parent ambiguity, containment cycles, unsupported coincidence, and winding two or another unsupported state require typed failure.

### 2.13 Full input-geometry assessment

Before publication, Component 02 must conservatively assess the complete coherent realization and source uncertainty for:

- forbidden non-adjacent facet intersections;
- unsupported coplanar overlap;
- contact extending beyond an authorized shared source feature;
- collapsed or inverted source edges, facets, wedges, or local shell regions;
- geometrically incompatible incident facets;
- shell intersections or positive-volume overlap inconsistent with the nesting model;
- gaps or overlaps contradicting declared precision; and
- uncertainty spanning incompatible topology or occupancy.

The scalable search must use a deterministic conservative spatial index and must be gated against exhaustive all-pairs enumeration in bounded tests. False positives are allowed; false negatives are not.

### 2.14 Independent validation evidence

The proposed artifact must contain enough immutable evidence for a verifier to reconstruct, rather than trust:

- source ring normalization;
- directed and undirected incidence;
- vertex links;
- shell membership;
- canonical identity construction and automorphism disposition;
- support planes, projection frames, and facet geometry;
- the coherent epsilon-validity certificate;
- validation relation findings;
- shell orientation and pairwise compatibility;
- nesting, occupied sides, and winding;
- precision values and bounds; and
- source/presentation mappings and canonical encodings.

A producer-owned boolean such as `is_manifold=true` or `epsilon_valid=true` is never sufficient evidence.

## 3. Output contract

For each accepted operand, Component 02 must publish one immutable `validated_operand<T,I>` containing at least:

- context owner, operand identity, type descriptors, and all required versions;
- the Component 01 source snapshot digest and Component 03 precision-context reference;
- canonical referenced source vertex records with exact coordinate bits and bounded-point references;
- canonical normalized facet, ring, corner, directed-edge-use, and undirected-edge records;
- independently verified cyclic vertex-link records;
- canonical shell records and complete membership ranges;
- support-plane, projection, orientation, and validation-decomposition evidence;
- one coherent epsilon-validity certificate or a topology-only nonpublishable disposition;
- shell orientation, pairwise boundary relation, nesting, depth, and occupied-side records;
- deterministic canonical semantic bytes and digest;
- presentation-specific source-position mappings kept outside semantic bytes;
- complete precision, resource, diagnostic, and replay references; and
- all provenance required by Components 04, 05, 14, and 15.

Finite isolated source vertices remain available through the Component 01 source snapshot and replay record only. They are not canonical semantic vertices in `validated_operand`.

The artifact must guarantee:

- all published topology derives only from source indices and incidence;
- all referenced source features are owned by exactly one shell;
- every edge has exactly two reciprocal opposite uses;
- every referenced vertex has one closed cyclic fan;
- every facet and shell semantic decision is supported by bounded evidence;
- every accepted boundary has occupied-negative and empty-positive side semantics;
- duplicate-coordinate identities remain distinct; and
- no mutable caller or transaction-private storage is referenced.

On failure, no `validated_operand` is published. The primary failure must be the canonical minimum under Component 01 arbitration. Equivalent symmetric failures must be coalesced or represented by an equivalence-class witness rather than selected by caller order.

## 4. Required invariants and prohibited behavior

Required invariants include:

- source positions are not canonical topology IDs;
- canonical semantic bytes and IDs are invariant under presentation-only permutations;
- topology and shell connectivity never arise from coordinate equality or proximity;
- input precision never decreases;
- user cleanup tolerance is not spent during input validation;
- a coherent realization is shared across all incident facets and shell checks;
- parentage is not finalized before shell-boundary compatibility;
- shell nesting is acyclic and deterministic;
- occupied side is negative for every accepted oriented boundary;
- presentation-specific automorphism choices do not affect semantic content; and
- every published record is immutable and independently verifiable.

Component 02 must not:

- inspect ignored public mesh fields;
- weld vertices or repair malformed topology;
- triangulate malformed rings into apparent validity;
- use random rays or traversal-order tie breaks;
- infer containment from bounds or one probe alone;
- accept an uncertain relation as definite;
- use operation-specific Boolean symbolic ownership to validate one operand;
- trust legacy mesh caches, normals, producer counts, or hashes alone;
- call an external geometry library; or
- publish a topology-only artifact into an ordinary Component 04 pipeline.

## 5. Test and validation specification

### 5.1 Source-contract and structural tests

Cover:

- completely empty operands;
- empty facet sets with finite isolated vertices;
- valid tetrahedra, boxes, concave polygonal shells, positive-genus shells, and high-valence vertices;
- ignored normals, colours, involved-face caches, and metadata changed arbitrarily before Component 01 capture, proving identical Component 02 inputs and results;
- consecutive duplicate indices and a duplicate closing index;
- undersized and repeated-index rings;
- out-of-range indices and count/byte overflow;
- non-finite coordinates;
- open edges, same-direction pairs, three-use edges, duplicate uses, and self-edges;
- bow-tie vertices, multiple link cycles, and open link chains; and
- exact typed subcodes for every invalid category.

### 5.2 Canonicalization tests

Apply:

- vertex-array permutations with index remapping;
- facet permutations;
- ring rotations;
- disconnected-shell permutations;
- deterministic worker/range permutations;
- exact-coordinate duplicate occurrence permutations;
- symmetric accepted topology where coordinate labels distinguish records;
- symmetric invalid topology with equivalent failure sites; and
- forced digest/hash collisions.

Require identical semantic artifact bytes and canonical IDs. Presentation maps may differ only by a verified bijection within an exact automorphism class and must remain source-correct.

### 5.3 Facet geometry and coherent-realization tests

Include:

- convex and concave planar polygons;
- exact nominal polygon planarity;
- supported constructively planarized polygons within input precision;
- independently small facet residuals with no coherent shared-vertex realization, which must fail;
- long thin facets, nearly collinear corners, and projection-axis ties;
- self-crossing rings, non-adjacent touches, and collinear overlap;
- adjacent and non-adjacent coordinate-coincident distinct identities;
- collapsed source edges and local wedges;
- signed zero, subnormals, adjacent floats, extreme exponents, and large translations; and
- certificate displacement exactly below, at, straddling, and above input precision.

Use in-tree exact integer/rational test oracles for bounded low-complexity realizations and relation expectations.

### 5.4 Shell semantics tests

Cover:

- one outward exterior shell;
- reversed exterior and cavity shells;
- several disjoint solids;
- cavities, islands, and at least six nesting levels;
- point-, edge-, and face-touching disconnected shells;
- shell boundaries that cross even though one interior probe lies inside the other;
- sibling positive-volume overlap;
- coincident same- and opposite-orientation shells under V1 rejection;
- ambiguous parents and containment cycles;
- shell order permutations; and
- winding zero, one, and unsupported two.

For accepted cases, verify intrinsic orientation, pairwise relation, parent, depth, negative occupied side, positive empty side, and total winding.

### 5.5 Geometry-search and relation tests

For bounded fixtures, compare the scalable index and relation kernel with exhaustive all-pairs and exact-oracle classifications. Include:

- transverse, tangent, coplanar, and near-coplanar triangle pairs;
- authorized shared-edge and shared-vertex adjacency;
- unauthorized contact outside the shared feature;
- one-ULP gaps and overlaps;
- uncertainty that spans disjoint/contact/intersection categories;
- finite AABB inflation near exponent limits; and
- adversarial candidate-order and projection-order permutations.

### 5.6 Mutation tests

Starting from valid proposed artifacts, independently mutate:

- normalization maps;
- directed-use pairing;
- vertex-link cycles;
- canonical IDs and automorphism records;
- support planes and projection frames;
- realization coordinates or displacement bounds;
- facet simplicity/orientation evidence;
- shell membership and pairwise relation records;
- parent, depth, orientation parity, or occupied side;
- a forbidden-intersection finding;
- precision bounds;
- topology-only publication eligibility;
- source/presentation maps;
- canonical bytes or digest; and
- persistent resource accounting.

The independent verifier must reject every required mutation without relying solely on producer-owned cached counts or digests.

### 5.7 Fuzzing, replay, and performance

Fuzz generated valid closed manifolds, coherent near-planar inputs, nested shell trees, and controlled invalid mutations. Every crash, hang, nondeterministic result, unexpected `internal_invariant_error`, producer/verifier disagreement, spatial-index false negative, or exact-oracle disagreement must be minimized and committed to the permanent replay corpus.

Structural performance gates must track ring-relation candidates, canonical-labeling work, realization feasibility work, shell-pair candidates, triangle-pair candidates, predicate calls, and peak resources. Large disjoint fixtures must not take unconditional all-pairs paths.

### 5.8 Definition of done

Component 02 is complete only when:

- its source-field contract exactly matches Component 01;
- all count, scalar, index, ring, edge, and vertex-link checks are typed and independently verified;
- canonical semantic identities are presentation-invariant without pretending a unique source mapping exists inside exact automorphism classes;
- every accepted facet has bounded plane, projection, simplicity, orientation, and coverage evidence;
- every ordinary accepted operand has one coherent epsilon-validity certificate;
- shell-boundary compatibility is certified before nesting;
- orientation parity yields occupied-negative and empty-positive sides for every accepted boundary;
- the complete shell and geometry relation matrix is deterministic and fail-closed;
- duplicate coordinates never create topology;
- topology-only artifacts cannot enter ordinary success processing;
- all normative clauses map to executable evidence; and
- production and normative-test code remain strict portable C++17 with no external dependency.
