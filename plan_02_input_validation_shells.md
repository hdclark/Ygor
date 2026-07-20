# Plan 02: Input Topology Validation and Shell Semantics

## 0. Scope and non-negotiable constraints

Implement **only Component 02** from `component_02_input_validation_shells.md`. This component accepts one immutable caller-supplied `fv_surface_mesh<T,I>` operand at a time and publishes one immutable, independently verified `validated_operand<T,I>` describing normalized source facets, exact indexed incidence, closed two-manifold vertex links, disconnected shells, shell orientation/nesting/occupied-side semantics, bounded input-geometry findings, canonical source identities, and complete diagnostic/provenance evidence.

This component must not:

- publish a source-triangle complex, internal halfedge manifold, broad-phase candidate stream, intersection event, winding classification, retained Boolean surface, output cycle, cleanup action, public result mesh, or final Boolean success;
- mutate the caller mesh, repair open or non-manifold topology, move coordinates, weld vertices, fill holes, split bow-tie vertices, delete source facets, or reverse a shell without an explicitly supported normalization policy and recorded proof;
- infer topology, adjacency, shell connectivity, edge pairing, or source identity from coordinate equality or proximity;
- use the caller tolerance as a general-purpose equality epsilon;
- trust `fv_surface_mesh::involved_faces`, caller normals, cached topology, metadata, producer booleans, or legacy Boolean artifacts as authoritative evidence;
- call `src/YgorMeshesBoolean{,2,3,4,5}*.{h,cc}`;
- use any external, vendored, downloaded, runtime-invoked, or optional geometry dependency; or
- compile authoritative code outside the strict C++17 floating-point target established by Component 01.

The implementation must use Component 01 for ownership, strong identities, checked counts, typed outcomes/errors, resources, cancellation, diagnostics, transactions, canonical bytes, replay, deterministic arbitration, and immutable artifact publication. It must use Component 03 for all topology-affecting geometric arithmetic, source-point enclosures, plane/residual/projection calculations, bounded predicates, precision classification, and conservative bounds.

The repository's implementation order places Component 03 before Component 02 even though `tracker.md` is numbered by component. Until a conforming Component 03 provider is implemented, Component 02 may compile and run against a deterministic test-only provider that implements the exact Component 03 capability interfaces. Production integration must remain disabled rather than replacing bounded arithmetic with raw `vec3` operations or assuming zero uncertainty.

All production and normative-test code must be portable C++17 and standard-library-only. Every expected invalid-input or geometric-difficulty path must return a typed failure. No failed, cancelled, partially checked, or verifier-rejected operand may publish a `validated_operand`.

Mark Component 02 complete in `tracker.md` only after Section 23 is fully satisfied.

## 1. Existing Ygor assessment and mandatory reuse decisions

### 1.1 Reuse unchanged as data carriers

Reuse these existing types only for their narrow, suitable roles:

- `fv_surface_mesh<T,I>` from `YgorMath.h` as the immutable public source container;
- `vec3<T>` as the nominal coordinate carrier exposed by that container; and
- ordinary standard-library containers and algorithms where their ordering and allocation are controlled by the Component 01 contracts.

Read `mesh.vertices` and `mesh.faces` directly. Preserve every source coordinate's exact `T` bit pattern, including signed zero. Copy all source data required after validation into transaction-owned storage; V1 must not publish an artifact that borrows caller-owned vectors, strings, maps, or nested face storage.

### 1.2 Reuse only through audited adapters or tests

`YgorMeshesAdaptivePredicates` contains useful in-tree expansion-arithmetic orientation and insphere primitives. It is not itself a Component 03 bounded-predicate provider: it does not propagate inherited input precision, return uncertainty intervals, expose conditioning, or produce Component 01 diagnostics. Therefore:

- production Component 02 code must not call `orient_sign`, `adaptive_predicate::orient3d`, or related routines directly for an authoritative acceptance decision;
- Component 03 may internally adopt audited pieces after strict-target qualification and enclosure integration; and
- Component 02 tests may use the adaptive predicates as an independent exact-floating sign oracle on finite low-complexity fixtures where their assumptions are explicitly met.

`YgorMeshesVerification.h/.cc` provides useful examples and fixtures for finite-coordinate checks, face-index checks, edge-count classification, and orientation checks. Its current interfaces are boolean/exception based, triangular-only, and insufficient for polygon rings, vertex-link manifoldness, canonical identity, shell semantics, precision, or reconstructible evidence. Do not call it as the authoritative validator. Reuse concepts only after reimplementing them inside the bounded subsystem with strong IDs, checked arithmetic, typed diagnostics, and independent verification.

Existing contour and plane utilities may be used only for non-authoritative test visualization or fixture preparation. Their tolerance defaults, coordinate-based duplicate removal, least-squares fitting, projection helpers, and point-in-polygon epsilon behavior do not satisfy this component's contract.

### 1.3 Do not reuse

Do not use any of the following in production validation:

- `fv_surface_mesh::merge_duplicate_vertices`;
- `fv_surface_mesh::convert_to_triangles`;
- `fv_surface_mesh::remove_degenerate_faces`;
- `fv_surface_mesh::remove_disconnected_vertices`;
- `fv_surface_mesh::simplify_inner_triangles`;
- `fv_surface_mesh::compute_vertex_normals` as orientation evidence;
- `fv_surface_mesh::slice_with_planes` as shell or self-intersection evidence;
- `fv_surface_mesh::recreate_involved_face_index` or `involved_faces` as authoritative incidence;
- `contour_of_points::Remove_Sequential_Duplicate_Points`, `Remove_Extraneous_Points`, `Remove_Needles`, projected point-in-polygon routines, or epsilon comparisons;
- random surface sampling or random ray selection;
- current `IsClosedManifold`, `HasConsistentOrientation`, or `ValidateClosedTriangularMesh` as a publication gate;
- logging, exceptions, assertion-only checks, pointer identity, `std::hash`, source array order, allocation order, or thread order as semantic control flow; or
- any legacy mesh-Boolean implementation named in Section 0.

### 1.4 Required improvement strategy

Implement Component 02 as new internal bounded-Boolean code under `src/YgorMeshesBooleanBounded/`. Do not broaden the old public verification helpers until this component is complete. If generally useful integer-incidence or cycle-verification logic emerges, place it in a small bounded-subsystem utility with explicit contracts and tests; later work may extract it only after preserving all strong-identity and deterministic-error requirements.

## 2. Exact file and target layout

Add these files under `src/YgorMeshesBooleanBounded/`:

- `ValidatedOperand.h` — immutable artifact schema and read-only views;
- `InputValidationTypes.h` — temporary handles, record enums, witness types, and Component 02 version constants;
- `InputValidation.h/.cc` — top-level typed validation entrypoint and phase orchestration;
- `InputRingNormalization.h/.cc` — structural ring normalization and reversible source-position maps;
- `InputFacetGeometry.h/.cc` — bounded planarity, projection, simplicity, orientation, and validation-only polygon decomposition;
- `SourceIncidence.h/.cc` — directed edge-use emission, undirected pairing, and reciprocal incidence;
- `VertexLinkValidation.h/.cc` — corner-link reconstruction and one-cycle validation;
- `CanonicalSourceTopology.h/.cc` — graph refinement, automorphism resolution, dense strong-ID assignment, and caller/canonical maps;
- `ShellSemantics.h/.cc` — shell discovery, orientation witnesses, containment, nesting, depth, occupied-side semantics, and winding checks;
- `InputGeometryValidation.h/.cc` — conservative self-intersection/contact/epsilon-validity assessment;
- `ValidatedOperandCodec.h/.cc` — canonical artifact encoding and digest production;
- `ValidatedOperandVerifier.h/.cc` — independent reconstruction and mutation rejection.

Keep implementation-only records out of the installed public header. `ValidatedOperand.h` may be included by later internal components but must expose only immutable storage and checked read-only access. Templates must either remain header-defined or be explicitly instantiated for the supported `float`/`double` and `uint32_t`/`uint64_t` combinations without accidental unsupported instantiation.

Extend `tests/mesh_boolean_bounded/` with:

- `TestInputValidationUnit.cc`;
- `TestInputFacetGeometry.cc`;
- `TestInputTopology.cc`;
- `TestInputShellSemantics.cc`;
- `TestInputEpsilonValidity.cc`;
- `TestInputCanonicalization.cc`;
- `TestInputValidationMutation.cc`;
- `TestInputValidationFuzz.cc`;
- `InputValidationFixtures.h/.cc`;
- `InputValidationExactOracle.h/.cc`; and
- `InputValidationTestProvider03.h/.cc` until the production Component 03 provider exists.

Register separate CTest cases for unit, facet geometry, topology, shell semantics, epsilon validity, canonicalization/properties, mutation, and fuzz/replay suites. Apply the strict floating-point helper from Component 01 to every Component 02 production and normative-test target. No test may download or discover a framework.

## 3. Stable versions, stages, checkpoints, and failure subcodes

### 3.1 Version registry

Add explicit V1 constants to `ContractVersions.h` for:

- input-validation provider;
- normalized source-ring schema;
- source-incidence schema;
- vertex-link schema;
- canonical source-topology provider;
- shell-semantics provider;
- input-geometry assessment provider;
- validated-operand artifact;
- validated-operand canonical encoding; and
- validated-operand verifier.

Reserve zero for invalid/uninitialized versions. Unknown required versions must be rejected; they must never be interpreted as the latest version. Include every version in artifact bytes, replay, diagnostics, and verifier checks.

### 3.2 Logical stages and checkpoints

Use the operand-A and operand-B validation stages reserved by Component 01. Define stable Component 02 checkpoints in this order:

1. capability/owner validation;
2. source-count preflight;
3. immutable source copy and scalar checks;
4. ring structural normalization;
5. source bounded-point import;
6. bounded facet-geometry validation;
7. temporary edge-incidence construction;
8. temporary vertex-link validation;
9. temporary shell discovery;
10. canonical source-topology labeling;
11. canonical incidence reconstruction;
12. shell orientation evidence;
13. shell containment and nesting;
14. occupied-side and winding validation;
15. epsilon-validity geometry search;
16. artifact construction;
17. independent artifact verification;
18. canonical encoding/digest/resource finalization; and
19. pre-publication cancellation and commit.

Do not renumber these checkpoints after release. Add new optional checks only in reserved gaps or by a schema-version change.

### 3.3 Component 02 subcodes

Allocate a disjoint Component 02 subcode range and define explicit values for at least:

- unsupported source layout;
- source count/range/byte overflow;
- non-finite source coordinate;
- malformed optional per-vertex array;
- out-of-range source index;
- empty or undersized normalized ring;
- repeated non-consecutive source index;
- topological self-edge;
- facet support plane unavailable;
- facet planarity definitely exceeded;
- facet planarity uncertain beyond input contract;
- stable projection unavailable;
- projected ring proper crossing;
- projected ring non-adjacent touch;
- projected ring collinear overlap;
- projected ring simplicity uncertain;
- facet orientation zero or uncertain;
- duplicate directed edge use;
- open boundary edge;
- non-manifold edge use count;
- same-direction paired edge uses;
- corrupt edge-use source location;
- vertex-link open chain;
- vertex-link multiple cycles;
- vertex-link repeated incidence;
- vertex-link degree mismatch;
- shell local orientation inconsistency;
- shell orientation witness unavailable;
- shell orientation conflicts with nesting parity;
- shell parent ambiguous;
- shell containment cycle;
- unsupported shell coincidence;
- shell contact semantics ambiguous;
- total winding outside the supported regular-solid model;
- validation decomposition failed;
- forbidden non-adjacent intersection;
- unresolved intersection/contact uncertainty;
- collapsed or inverted source geometry;
- incident-facet geometric incompatibility;
- shell overlap contradiction;
- source gap/overlap contradicts declared precision;
- canonical labeling work exhausted;
- canonical key collision/duplicate contradiction;
- caller-to-canonical map inconsistency;
- validated artifact verifier rejection;
- validated artifact digest mismatch; and
- topology-only artifact not eligible for ordinary publication.

Map malformed indexed topology and policy-incompatible orientation to `input_contract_error`; bounded geometric contradiction or unresolved geometric uncertainty to `input_geometry_not_epsilon_valid`; indeterminate parentage/contact/occupied-side meaning to `ambiguous_shell_semantics`; representability to `index_overflow`; accounting exhaustion to `resource_limit`; cancellation to `cancelled`; and producer/verifier contradictions to `internal_invariant_error`.

## 4. Top-level capability and API contract

Implement an internal entrypoint conceptually equivalent to:

```cpp
template<class T, class I>
boolean_outcome<artifact_handle<const validated_operand<T,I>>>
validate_operand(
    operand_id operand,
    const immutable_source_mesh<T,I>& source,
    const boolean_context_view<T,I>& context,
    const precision_context_view<T>& precision,
    const input_validation_capabilities<T,I>& capabilities);
```

`immutable_source_mesh` must be a Component 01-owned exact source description or a transaction-local immutable copy; it must not be a long-lived borrow of caller vectors. `input_validation_capabilities` must contain only narrow owner-checked views for resources, cancellation, diagnostics, transactions, canonical ID publication, replay, execution, and Component 03 bounded arithmetic. The validator must not receive the entire mutable top-level call object.

Validate the following before reading mesh contents:

- context owner and operand role;
- artifact/provider/schema versions;
- `T` and `I` qualification already frozen by Component 01;
- Component 03 owner, arithmetic model, and operand input precision match the context;
- selected solid policy and geometry-verification level are supported;
- transaction is open for the correct logical stage; and
- no ordinary-success path is attempted with a topology-only context.

The function returns exactly one verified immutable artifact or one typed error. It never logs, throws an expected error, partially publishes, or mutates the source.

## 5. Immutable artifact schema

### 5.1 Source vertex record

Each source vertex record must contain:

- canonical `source_vertex_id`;
- operand and context owner;
- exact x/y/z source bit patterns;
- nominal `vec3<T>` value reconstructed from those bits;
- Component 03 bounded-point/precision-ledger reference;
- referenced versus isolated/non-semantic status;
- canonical ranges of incident corners, directed edge uses, facets, and shell membership when referenced;
- reversible caller vertex-index mapping entries;
- opaque metadata/provenance references allowed by policy; and
- canonical record digest contribution.

Distinct caller indices always produce distinct source topological vertices, even when all coordinate bits match. Isolated vertices are preserved in source provenance but never assigned to a shell or passed to Component 04 as facet topology.

### 5.2 Normalized facet and ring record

V1 supports exactly one contour per source facet. Store:

- canonical `source_facet_id` and `source_ring_id`;
- operand and shell ID;
- canonical orientation-preserving sequence of source vertex IDs;
- one corner record per retained ring position;
- caller facet index and original ring-position reverse maps;
- normalization actions recording removed consecutive duplicates and a removed closing duplicate;
- bounded support-plane record;
- deterministic projection-frame record;
- bounded planarity residual summary;
- projected simplicity evidence;
- accepted orientation/area evidence;
- validation-only decomposition evidence; and
- canonical record bytes/digest contribution.

A ring's semantic sequence must not contain a duplicate closing vertex. Rotation is allowed; reversal is not, except under a future explicitly supported whole-shell normalization policy.

### 5.3 Directed and undirected edge records

A directed edge-use record must contain:

- canonical `source_directed_edge_id`;
- origin and destination source vertex IDs;
- facet, ring, and corner-slot IDs;
- previous and next directed uses in the same ring;
- paired reverse directed-use ID;
- canonical undirected-edge ID;
- shell ID; and
- complete caller-location provenance.

An undirected edge record must contain its ordered endpoint IDs and exactly two opposite directed uses. The canonical endpoint order is by strong source vertex ID, not coordinates.

### 5.4 Vertex-link record

For every referenced source vertex, store:

- canonical source vertex ID;
- the canonical cyclic sequence of incident corner IDs;
- corresponding incident facet and edge-use IDs;
- cycle orientation convention;
- degree/count evidence;
- a stable representative corner; and
- verifier reconstruction evidence.

One source index may have only one closed fan. Multiple disjoint fans sharing one source vertex ID are invalid even if every edge has exactly two uses.

### 5.5 Shell record

Each shell record must contain:

- canonical `source_shell_id`;
- canonical member ranges/lists for vertices, edges, facets, and rings;
- canonical representative features;
- conservative Component 03 shell bounds;
- local orientation consistency evidence;
- geometric orientation witnesses and their bounded dispositions;
- parent shell or exterior-root status;
- nesting depth;
- occupied and empty side semantics;
- containment probes and classifications;
- authorized contact/coincidence findings;
- total-winding evidence;
- input-geometry assessment summary;
- per-shell resource/work statistics; and
- canonical bytes/digest contribution.

### 5.6 Operand-level record

`validated_operand<T,I>` must contain:

- artifact and provider versions;
- immutable context owner and operand ID;
- exact source/replay digest;
- semantic canonical digest;
- normalized input precision and Component 03 qualification reference;
- all records above in canonical order;
- dense-ID range descriptors;
- caller-to-canonical and canonical-to-caller maps;
- shell nesting forest;
- geometry-verification disposition;
- topology-only/nonpublishable marker when applicable;
- complete findings/provenance needed by Components 04, 05, 14, and 15;
- exact resource leases and statistics; and
- canonical encoding bytes or digest according to replay policy.

The semantic canonical digest excludes presentation-only caller ordinals while the source/replay digest includes exact original array order and bits. Presentation maps are stored separately so an input permutation can have the same semantic artifact while still producing correct diagnostics for that presentation.

## 6. Transactional validation workflow

Execute validation in one Component 01 stage transaction with private mutable workspace. Use this fixed workflow:

1. validate owners, versions, policies, and capability compatibility;
2. compute all source counts, worst-case incidence counts, byte counts, and work preflights with checked arithmetic;
3. reserve mandatory source, temporary, canonical-sort, diagnostic, replay, and work resources before allocation;
4. copy exact source scalar bits, face index sequences, and permitted metadata into private immutable-source storage;
5. perform structural scalar and optional-array checks;
6. normalize every facet ring without geometry-based edits;
7. import every source coordinate through Component 03 as a bounded source point;
8. validate bounded facet planarity, projection, simplicity, and orientation;
9. build temporary directed/undirected edge incidence from normalized indices;
10. validate temporary vertex links independently of edge counts;
11. discover temporary shell components through paired-edge adjacency;
12. canonicalize source topology and assign dense strong IDs;
13. rebuild canonical incidence/link/shell records and verify all temporary-to-canonical maps;
14. establish shell geometric orientation evidence;
15. build deterministic containment/nesting and occupied-side semantics;
16. run the selected epsilon-validity geometry assessment;
17. construct a proposed immutable artifact;
18. independently verify the proposal without trusting producer maps/counts/digests as sole evidence;
19. encode canonical bytes, finalize digests/replay, reconcile exact resource use, and promote persistent leases;
20. poll cancellation immediately before publication; and
21. atomically publish one immutable artifact or roll back all private state.

Within each phase, collect a bounded set of independent failures and choose the Component 01 canonical minimum. Do not continue into a phase whose prerequisites are invalid. Long loops must poll cancellation at deterministic range boundaries and charge abstract work before executing the range.

## 7. Source preflight, scalar checks, and optional fields

### 7.1 Checked source counts

Convert all sizes to the Component 01 canonical count type using checked narrowing. Preflight at least:

- vertex count;
- facet count;
- total original ring positions;
- maximum and total normalized ring positions;
- directed edge-use count;
- maximum undirected-edge count;
- maximum corner/link incidence count;
- temporary and persistent record bytes;
- canonical-sort/refinement records;
- validation-decomposition triangles, conservatively `sum(max(0,n-2))`;
- potential spatial-index nodes;
- replay bytes; and
- abstract work for ring checks, canonical labeling, probes, and geometry search.

Fail before allocation on arithmetic overflow, `size_t` overflow, hard resource limit, or an output count not representable by the relevant strong-ID ordinal type. Never depend on host allocation failure as normal control flow.

### 7.2 Coordinates and indices

Require every x/y/z coordinate to be finite. Preserve signed zero and exact NaN payload information only in a failure replay record; no non-finite coordinate enters Component 03 or a proposed artifact.

Validate every face index before dereference. Widen `I` through a checked conversion, compare against vertex count, and report operand, caller facet, ring position, and invalid value. Never use `.at()` exceptions as validation.

### 7.3 Empty operands and isolated vertices

V1 accepts an empty face set as the empty regular solid, including when finite isolated vertices are present. The semantic topology then contains no facets, edges, links, or shells. Isolated vertices remain in source/replay provenance and caller mappings but are marked non-semantic and omitted from Component 04 input ranges.

A nonempty face list containing an empty or structurally invalid ring is not equivalent to an empty operand and must fail.

### 7.4 Optional arrays and metadata

Apply these exact V1 rules:

- `vertex_normals` and `vertex_colours` are non-semantic. Each must be empty or have exactly `vertices.size()` elements. A malformed length is `input_contract_error`. Their values may be copied as opaque provenance only when the frozen output/metadata policy requests it; normals never establish orientation.
- `involved_faces` is a non-authoritative cache. Ignore it for topology even when populated. Do not reject an otherwise valid mesh because the cache is stale. In diagnostic mode, optionally compare it against reconstructed incidence and emit a non-fatal, non-canonical warning. Never include it in semantic canonical bytes.
- arbitrary mesh metadata is non-semantic in V1 unless a Component 01-recognized, versioned Boolean precision field is present. Unknown metadata may be retained opaquely according to policy but must not affect topology, shell semantics, IDs, or digests used for semantic equality.

## 8. Exact ring structural normalization

For each source facet, perform one deterministic index-only pass:

1. copy the original ring indices after checked conversion;
2. remove every index equal to the immediately preceding retained index, recording each original position and retained predecessor;
3. after the pass, remove the final retained index if it equals the first, recording the closing-duplicate action;
4. count distinct indices using complete integer equality;
5. reject fewer than three retained positions or fewer than three distinct indices;
6. reject any retained index appearing more than once, because V1 has one simple contour and does not support a repeated non-consecutive source vertex;
7. emit one temporary corner and one temporary directed edge use per retained position; and
8. preserve the input direction exactly.

A pair of distinct indices with equal coordinates is never removed. A retained edge whose endpoint IDs are equal is an internal contradiction and must fail defensively.

Before canonical IDs exist, retain temporary handles that encode operand plus checked caller location but never escape the transaction. After canonical labeling, rotate each ring to the lexicographically least orientation-preserving cyclic sequence of full canonical corner keys. Use a linear minimal-rotation algorithm such as Booth's algorithm over total keys. Do not reverse a ring while choosing its start.

The normalization map must be total:

- every original ring position maps to one retained canonical corner or one explicit removal action;
- every retained corner maps to its original facet/ring position;
- no removal is justified by coordinates; and
- reapplying normalization to normalized output produces no additional action.

## 9. Bounded facet geometry validation

### 9.1 Component 03-only arithmetic

All calculations in this section must use Component 03 bounded values and predicate outcomes. Raw `vec3::Cross`, `Dot`, `unit`, length, plane constructors, raw `orient_sign`, `fabs(residual) < epsilon`, or contour tolerance helpers may not determine acceptance.

### 9.2 Deterministic support-plane selection

For each normalized ring, construct an ordered set of plane proposals:

1. cyclic consecutive triples in canonical temporary ring order;
2. triples formed from the lexicographically least vertex, the vertex with the greatest certified lower bound on squared separation, and the vertex with the greatest certified lower bound on squared area relative to that edge; and
3. any Component 03-prescribed deterministic fallback proposal family.

Evaluate proposals in a fixed provider-versioned order. Rank valid proposals by:

1. greatest certified lower bound on normal squared magnitude;
2. smallest plane uncertainty upper bound;
3. smallest complete source-feature proposal key.

If the inexpensive proposal set cannot certify a plane, perform a bounded all-triples fallback in canonical combination order, charging work before each block. Stop only after the globally best certifiable proposal is known or the work limit is reached. A work-limit failure is typed and must not be converted to geometric invalidity.

For the selected plane, verify every ring vertex's signed residual enclosure. Define the accepted planarity allowance from the source point envelopes, declared operand input precision, plane-construction uncertainty, and qualified machine floor. Do not use user tolerance as an extra allowance. Accept only when every residual's absolute upper bound is within the accepted allowance. If a residual is definitely outside, return planarity exceeded. If the decision overlaps the boundary and no deterministic alternate support/formulation certifies acceptance, return planarity uncertain.

Record the proposal set summary, selected source triple, nominal plane, plane enclosure, each worst residual witness, accepted allowance, and predicate dispositions.

### 9.3 Deterministic projection frame

Choose an orthogonal coordinate-drop projection by comparing certified lower bounds on the absolute components of the bounded plane normal. Prefer the axis with greatest certified lower bound. When intervals overlap, compare the provider's conservative distortion upper bound. Break an exact remaining tie by fixed axis order X, Y, Z.

Reject the facet when no axis yields a projection whose orientation and segment relations can be certified within input precision. Record the frame, orientation transform, distortion bound, and tie evidence.

### 9.4 Projected ring simplicity

Project each bounded source point through Component 03. Build a deterministic sweep or interval-AABB index over ring segments to avoid unconditional quadratic behavior, but gate it against exhaustive pair comparison in tests.

Classify every potentially interacting segment pair under these rules:

- consecutive segments may meet only at their shared topological endpoint;
- the first and last segments are consecutive;
- non-consecutive segments may not properly cross;
- non-consecutive segments may not touch at a point, even when distinct source vertices have identical coordinates, because that would make the V1 facet ring non-simple;
- non-consecutive collinear overlap is invalid;
- adjacent segments overlapping beyond their shared endpoint are invalid; and
- an uncertain relation must use the one provider-prescribed alternate bounded formulation, then fail if still uncertain.

Topology determines which endpoint contact is authorized; coordinate equality never does. Record the canonical segment pair and full bounded relation witness for every failure.

### 9.5 Accepted ring orientation and area

Compute a translation-reduced bounded shoelace/oriented-area sum in the selected projection using a prescribed deterministic pairwise reduction tree. Accept an orientation only when the final interval excludes zero.

If the area interval contains zero, evaluate a fixed fallback consisting of bounded corner orientations and a deterministic polygon-decomposition feasibility check. A mixture of definite corner signs is valid for a concave ring; the fallback must establish nonzero total oriented region, not convexity. If no formulation certifies a nonzero orientation, fail rather than selecting a sign from the nominal value.

Store the accepted projected orientation, its mapping to the three-dimensional source direction, area enclosure, reduction descriptor, and fallback evidence. Component 04 must receive this evidence instead of recomputing a raw face normal.

### 9.6 Validation-only polygon decomposition

Construct a private deterministic no-Steiner decomposition of each accepted facet for containment and intersection queries. This is not the Component 04 source-triangle complex and must not publish triangle IDs as source semantics.

Use bounded ear clipping with these exact rules:

- preserve every source boundary segment and corner mapping;
- select only ears with a definite orientation matching the accepted ring;
- require the ear diagonal to be definitely internal and free of prohibited intersections;
- choose the least complete ear key among all currently valid ears;
- allow a certified collinear corner to be bypassed only in the private decomposition, recording the skipped source boundary chain; never delete it from normalized topology;
- preserve coordinate-coincident but topologically distinct corners; and
- terminate with a complete decomposition, a typed geometric uncertainty failure, cancellation, or a charged work/resource failure.

Verify combinatorial boundary cancellation, opposite internal-diagonal uses, and projected coverage before using the decomposition. Area equality alone is insufficient.

## 10. Source edge incidence and reciprocal pairing

Emit exactly one directed edge-use proposal for every normalized ring edge. Each proposal key contains operand, temporary/canonical endpoint identities, facet/ring/corner location, and direction.

After canonical source vertex/facet IDs are available:

1. sort proposals by ordered undirected endpoint pair, then directed endpoints, facet ID, ring ID, and corner ID;
2. reject a self-edge;
3. reject duplicate complete directed-use keys;
4. require each undirected group to contain exactly two uses;
5. distinguish one use (`open boundary`), more than two (`non-manifold edge`), and two same-direction uses (`inconsistent orientation`);
6. require the uses to reference valid reciprocal ring positions; and
7. assign one canonical undirected-edge ID and reciprocal pair IDs only after the group is validated.

Pairing uses endpoint identities only. Exact coordinate coincidence between unrelated endpoint IDs has no effect.

The producer must additionally verify Euler-style count identities only as diagnostics; no global count identity may replace per-edge validation.

## 11. Vertex-link manifoldness

### 11.1 Corner-link construction

Represent each incidence of a source vertex in a facet as a corner. For a corner at vertex `v` with incoming edge `u->v` and outgoing edge `v->w`:

- cross the pair of the outgoing directed edge to the adjacent facet;
- locate that paired use's corner at `v` to define the producer successor; and
- analogously cross the incoming pair to define the predecessor.

Validate that every corner has exactly one predecessor and successor, all referenced IDs share owner/operand/vertex, and no ring/edge position is reused inconsistently.

### 11.2 One closed fan requirement

For each referenced source vertex:

1. collect all incident corners from reconstructed facet rings;
2. require successor and predecessor maps to be bijections over that set;
3. traverse from the least complete corner key;
4. require return to the start after exactly the incident-corner count;
5. reject an early repeat, open end, unvisited corner, second cycle, or degree mismatch; and
6. rotate the valid cycle to its least canonical corner key for publication.

This detects bow-tie vertices and multiple disjoint fans that edge-count validation misses. Distinct coordinate-equal source indices are processed independently.

The independent verifier must not call the producer successor traversal. It must build an undirected link graph whose nodes are corner incidences, prove every node has degree two, and prove the graph is connected and cyclic.

## 12. Canonical source identities

### 12.1 Canonicalization goals

Canonical source IDs and semantic bytes must be invariant under:

- caller vertex-array permutations with index remapping;
- facet permutations;
- ring rotation;
- disconnected component/shell permutations; and
- allocation, traversal, task, and thread scheduling.

Coordinates may participate through exact bit patterns but may never merge topology. Hash values may accelerate comparison but full canonical keys determine equality.

### 12.2 Colored incidence graph

Build a typed colored graph from transaction-local valid structure. Include node domains for source vertices, facets/rings, corners, directed edge uses, and undirected edges. Encode incidence as typed roles rather than relying on adjacency order.

Initial colors must contain only semantic data:

- node domain and operand;
- exact coordinate bits for a source vertex;
- ring length and policy-recognized facet metadata;
- local corner role and orientation-preserving incidence; and
- isolated-versus-referenced status.

Do not include caller ordinals, pointers, temporary allocation positions, hash table bucket order, or thread/task identity.

### 12.3 Refinement and automorphism resolution

Perform deterministic partition refinement. At each round, replace each node color by the full canonical encoding of its prior color plus sorted `(edge-role, neighbor-color)` multisets. Compare full bytes; use SHA-256 only as an accelerator with collision-forcing tests. Stop at a stable partition.

If cells remain non-singleton, run deterministic individualization/refinement search:

1. choose the unresolved cell by smallest cell color, then smallest size, then domain order;
2. branch over members in their complete current structural key order;
3. refine after each individualization;
4. encode a complete candidate graph labeling;
5. retain the lexicographically least complete encoding;
6. prune only when the partial encoding is already lexicographically greater than the current best; and
7. charge canonical-sort records and work before branching.

Resource exhaustion returns a typed failure and publishes nothing. Symmetric duplicate-coordinate topology must not fall back to source order.

### 12.4 Dense ID publication and maps

Use Component 01 canonical ID factories to assign dense source vertex, facet, ring, corner, directed edge, and undirected edge IDs from the winning labeling. No temporary ID may remain in the proposed artifact.

For disconnected structurally identical shell components, treat canonical component records as an explicit multiset. Order by complete component encoding and assign occurrence ordinals within equal encodings using the winning whole-graph labeling; never rely on discovery order.

Publish presentation maps separately:

- each caller vertex/facet/ring position maps to its canonical identity or normalization action;
- each canonical retained source record maps to one or more caller positions as appropriate; and
- semantic canonical bytes exclude presentation ordinals while replay bytes preserve them.

## 13. Shell discovery

Discover connected shells using facet adjacency through validated paired source edges only. Coordinate contact between otherwise separate components never joins shells.

Perform discovery first on temporary records, then reproduce canonical membership after ID publication. For each shell:

- collect complete sorted member vertex, edge, facet, ring, and corner lists;
- select the least canonical facet/edge/vertex representatives;
- union Component 03 feature bounds conservatively;
- record exact counts and resource statistics;
- construct a complete structural encoding and digest;
- identify potentially contacting shell pairs using inflated conservative bounds; and
- verify that every referenced source feature belongs to exactly one shell.

An empty operand has no shell. Isolated vertices have no shell. Shell membership must be independent of source facet order.

## 14. Shell geometric orientation

Edge pairing proves only local orientation consistency. Determine geometric orientation and side semantics using bounded, traversal-independent evidence.

### 14.1 Bounded signed-volume evidence

For each shell, compute a bounded signed-volume enclosure using the validation-only facet decomposition. Translate every tetrahedral term by a deterministic shell anchor chosen from the shell bound center representation or least canonical source point, as prescribed by Component 03, to reduce cancellation. Accumulate terms with a fixed pairwise reduction tree.

Use the convention that a positive definite signed volume denotes outward-oriented boundary under the repository's right-handed coordinate convention. Record the convention version. A volume interval containing zero is inconclusive, not evidence of reversal.

### 14.2 Deterministic side probes

Independently seek a certified side probe:

1. order candidate facet/decomposition triangles by greatest certified area lower bound, then greatest local clearance lower bound, then canonical key;
2. choose interior sample barycentric fractions from a fixed rational sequence, beginning with `(1/3,1/3,1/3)` and then provider-versioned alternatives;
3. construct points on both sides of the facet using a Component 03 bounded normal and the smallest certified offset larger than local uncertainty but smaller than available non-incident-feature clearance;
4. classify both probes against the shell using Section 15's deterministic point classifier; and
5. accept a witness only when one side is definitely inside and the other definitely outside without boundary-overlap ambiguity.

Try candidates in canonical order until one certifies the side or resource/work limits are reached. Never use user tolerance to push a probe through nearby geometry. Never use a random ray.

### 14.3 Evidence reconciliation

If signed volume and side probe are both definite, require agreement. A disagreement is an internal/provider contradiction and prevents publication. If only one is definite, it may establish geometric orientation. If neither is definite, return a typed input-geometry/orientation failure.

Do not yet reject an orientation solely because it is positive or negative; cavity parity is established by nesting. Store which local side is geometrically interior and the complete witnesses.

## 15. Deterministic point classification and shell nesting

### 15.1 Probe generation

For each shell, produce a canonical probe known to lie in its bounded interior. Prefer the accepted inward side probe from Section 14. If it overlaps any shell boundary enclosure, seek another facet/sample/offset in canonical order. A nominal centroid or bounding-box center is never sufficient without a bounded inside certificate.

### 15.2 Point-against-shell classifier

Implement a Component 02 classifier over validation-only triangles with these rules:

- use a fixed, versioned sequence of rational direction vectors derived without transcendental functions;
- build rays through Component 03 bounded arithmetic;
- enumerate candidate triangles with a deterministic conservative spatial index;
- classify ray/triangle relations with bounded predicates;
- apply a fixed half-open counting convention to definite vertex/edge hits;
- treat a probe enclosure overlapping a boundary as ambiguous rather than symbolically forcing inside/outside;
- reuse each canonical ray/triangle relation once within the classification record;
- try the next prescribed ray when a relation is numerically uncertain or boundary-overlapping; and
- return definite winding, ambiguous, resource failure, or cancellation—never a guessed boolean.

For small test fixtures, compare every result with an independent exact rational ray oracle.

### 15.3 Candidate parent relation

A shell can be a parent candidate only when its conservative bound can contain the child probe and the child's full bound is not definitely exterior. Bounds prune; they never prove containment.

Classify the child interior probe against all candidate shells in canonical shell order. Record definite inside, definite outside, boundary contact, or ambiguous. A point/edge contact alone does not create parentage.

Select the parent as the unique definitely containing shell with no other definitely containing shell strictly between it and the child. Verify this relation using pairwise containment records rather than bounding-box size. If two incomparable candidates remain, a required relation is ambiguous, or a containment cycle appears, return `ambiguous_shell_semantics`.

### 15.4 Contact and coincidence policy

Under V1 outward-oriented alternating-shell policy:

- disjoint shells are separate exterior roots or descendants according to definite containment;
- point- and edge-touching shells remain topologically separate and contact does not establish parentage;
- face contact is accepted only when bounded geometry proves no positive-volume overlap and occupied-side semantics remain unique;
- entirely coincident separate shells are rejected as `unsupported shell coincidence` unless a future versioned solid policy explicitly defines their multiplicity/cancellation semantics; local face-contact patches remain governed by the preceding rule; and
- an uncertain contact that can change parentage or occupancy is `ambiguous_shell_semantics`.

Do not silently choose a coincident owner in Component 02; Boolean coincident-surface ownership belongs to Components 07 and 10 after each input independently represents a valid solid.

### 15.5 Depth, orientation parity, and occupied sides

Assign exterior roots depth zero and propagate depth through the acyclic parent forest. Under V1:

- even-depth shells bound occupied material on their negative/outward-facing side and must be geometrically outward oriented;
- odd-depth shells bound cavities and must have the opposite orientation;
- depth alternates occupancy; and
- islands inside cavities alternate again.

Compare each shell's geometric orientation evidence to expected parity. Under the strict V1 policy, reject a reversed shell; do not rewrite its rings. Reserve a future policy path for whole-shell canonical reorientation, but do not implement or advertise it without a new policy/schema version and explicit provenance.

Compute and store each boundary's occupied and empty sides. Validate representative regions so total oriented winding away from boundaries is always zero or one. Positive-volume overlap between sibling/additive shells that yields winding two, contradictory cavity overlap, or any unsupported winding state is invalid.

## 16. Epsilon-valid input geometry assessment

### 16.1 Verification levels

Implement these dispositions exactly:

- `required_scalable`: run all mandatory scalable geometric checks and retain bounded primary evidence;
- `required_scalable_with_diagnostics`: run the same publication gates and retain additional bounded secondary findings;
- `exhaustive_test_only`: in test builds, additionally run exhaustive pair checks and exact low-complexity oracles; and
- internal topology-only diagnostic attempt: permit explicitly configured geometric checks to be skipped, but mark the artifact nonpublishable for ordinary Boolean success and prevent Component 04 from entering an ordinary-success pipeline without a later full revalidation.

There is no ordinary-success `none` mode.

### 16.2 Conservative validation spatial index

Build a private deterministic spatial index over validation-decomposition triangles and shell bounds. Use closed bounds inflated by all relevant Component 03 source uncertainty. A median BVH is acceptable when specified as follows:

- choose the axis with greatest certified center-span upper bound, ties X/Y/Z;
- sort by complete bounded-center key followed by canonical triangle key;
- split at the fixed lower median;
- store closed union bounds; and
- use canonical node numbering from preorder over the deterministic tree.

The index may produce false positives but must not miss a bounded interaction. Exhaustive tests must compare it against all pairs.

### 16.3 Pair exclusions by topology

Exclude or narrow a pair only from source identity:

- triangles from the same private facet decomposition are checked by the facet coverage verifier rather than treated as source self-intersections;
- adjacent facets sharing a source edge may meet on that exact shared edge but not beyond it;
- facets sharing only a source vertex may meet on that source occurrence but not along an unrelated segment or area;
- facets with no shared source feature have no authorized contact from topology; and
- separate shells never gain authorization merely because coordinates coincide.

### 16.4 Required geometric checks

At minimum, certify all of the following:

1. every facet remains within its bounded planar contract;
2. every validation triangle is orientation-compatible or is an explicitly recorded zero-area private bookkeeping triangle that cannot hide source area;
3. no non-adjacent source facets have a definite transverse intersection;
4. no non-adjacent facets have unsupported coplanar positive-area overlap;
5. every authorized adjacent-facet contact is confined to the shared source feature;
6. incident facet wedges around each edge and vertex are geometrically compatible with the validated cyclic topology;
7. no local source region is definitely collapsed or inverted beyond its input envelope;
8. shell contacts and overlaps agree with the nesting/winding model;
9. no unresolved uncertainty envelope spans two incompatible topologies;
10. no gap or overlap definitely exceeds or contradicts the declared input precision; and
11. every accepted finding remains within the operand input precision without spending user cleanup tolerance.

Classify pair relations as definitely disjoint, authorized shared-feature contact, authorized policy contact, forbidden intersection/overlap, or uncertain. A forbidden relation fails. An uncertainty that can change topology or shell semantics also fails. This component may conservatively reject a difficult potentially valid input; it must not accept a known contradiction.

### 16.5 Sufficient, not complete, epsilon-validity proof

Do not claim a complete decision procedure for arbitrary perturbation existence. Publish `epsilon_valid` only when the implemented sufficient certificates all pass. Store the exact certificate/provider version so later Component 15 can independently assess what was proven.

## 17. Canonical encoding and digests

Use Component 01 canonical bytes. Never serialize object memory, padding, `size_t`, pointers, container capacity, locale text, or implicit enum values.

Define the semantic artifact encoding in this order:

1. magic and validated-operand schema;
2. provider/version registry;
3. context digest and operand ID;
4. scalar/index descriptors and input precision bits;
5. geometry-verification disposition;
6. canonical source vertex records;
7. canonical facet/ring/corner records;
8. canonical directed/undirected edge records;
9. canonical vertex-link cycles;
10. canonical shell records and nesting forest;
11. bounded plane/probe/geometry evidence references;
12. canonical resource/statistics summary; and
13. verifier disposition.

Encode presentation maps and original source array order in a separate replay/source section. The semantic digest must be byte-identical under valid source permutations; the source digest must change when the exact presentation changes.

Full record bytes determine equality. SHA-256 is an integrity and lookup accelerator only. Collision-forcing tests must prove that equal hashes with unequal bytes do not merge records or alter canonical order.

## 18. Independent validated-operand verifier

The verifier must accept the proposed immutable records plus immutable exact source snapshot and Component 03 verification capabilities. It must not call producer orchestration, canonical labeling, successor-cycle, parent-selection, or geometry-index helpers as its sole method.

### 18.1 Structural reconstruction

Independently:

- validate versions, owner, operand, dense ID ranges, sorted ranges, and all references;
- repeat source scalar/index checks from exact source bytes;
- reconstruct ring normalization with a separate straightforward algorithm and compare every removal/map;
- prove ring rotations preserve orientation and canonical minimality;
- reconstruct all directed uses directly from published rings;
- group complete undirected endpoint keys and prove exactly two opposite uses;
- reconstruct each vertex link as a degree-two connected link graph;
- rediscover shell connected components through paired edges; and
- compare complete member sets rather than producer counts/digests alone.

### 18.2 Geometric reconstruction

Independently:

- recompute or conservatively recheck plane residuals and projection validity through Component 03 verifier APIs;
- recheck projected ring crossings using a different enumeration order;
- verify validation-decomposition boundary cancellation and coverage evidence;
- recompute signed-volume evidence with a different fixed reduction grouping;
- seek/verify shell probes using an alternate prescribed ray-order family;
- reconstruct parent/depth relations from classifications;
- verify orientation parity and occupied-side semantics;
- rerun forbidden-intersection search with a verifier-owned spatial-index construction; and
- prove every published precision bound dominates its inputs.

For bounded small artifacts in tests, the verifier must switch to exhaustive all-pairs and exact-oracle modes.

### 18.3 Canonical and publication checks

Re-encode semantic and source sections from records, recompute digests, verify no temporary/task-local ID or caller pointer remains, and verify resource leases exactly match immutable storage. Any contradiction returns `internal_invariant_error::validated artifact verifier rejection` and prevents commit.

Provide test-only corrupt constructors for every mutation category in Section 21. They must be unavailable in production builds.

## 19. Determinism and error arbitration

Before canonical IDs exist, form deterministic failure witness bytes from:

- operand role;
- failure kind;
- exact source coordinate/index values involved;
- normalized cyclic local ring encoding where available; and
- checked caller location only as the final presentation-specific diagnostic field.

After canonical IDs exist, use Component 01 entity ordering directly. Within a phase, collect failures over deterministic canonical work ranges, sort by complete error key, and submit the minimum. A parallel provider may process private ranges, but schedule, thread count, diagnostic capacity, hash collisions, and allocation order must not alter the primary error bytes.

Stable summaries are locale-independent templates. Caller indices may appear in non-authoritative display fields, but canonical failure selection must be based on semantic witness data whenever possible.

## 20. Resource accounting and cancellation

Charge Component 01 resource kinds for at least:

- source vertices, facets, rings, directed edges, and undirected edges;
- corner/link records;
- shell records;
- validation-decomposition triangles and diagonals;
- validation spatial-index nodes and candidate pairs;
- canonical-sort/refinement records;
- verification findings;
- temporary and persistent bytes;
- replay bytes; and
- abstract work units.

Reserve before allocation. Use aggregate reservations or deterministic slices before parallel work. Never let racing reservations select which shell, witness, or error wins.

Poll cancellation:

- after source preflight;
- between deterministic facet ranges;
- between edge/link/shell ranges;
- at canonical-labeling branch blocks;
- between probe/ray ranges;
- between spatial-index candidate blocks;
- before verifier entry;
- after verifier completion; and
- immediately before publication.

On cancellation or any error, stop admitting commit-producing work, join all execution scopes, roll back private IDs/storage/reservations, preserve predecessor context/precision artifacts, finalize one deterministic error/replay record, and publish no operand artifact.

## 21. Required tests and exact coverage

### 21.1 Structural and scalar unit tests

Cover at least:

- completely empty mesh;
- empty faces with finite isolated vertices;
- valid tetrahedron, box, concave polygonal shell, and high-valence vertex;
- canonicalizable consecutive duplicate indices and duplicate closing index;
- ring becoming empty, one vertex, two vertices, or fewer than three distinct vertices;
- repeated non-consecutive source index;
- out-of-range indices at zero, last valid, first invalid, and maximum `I`;
- count-to-byte and cumulative-ring overflow without host allocation;
- non-finite x/y/z including quiet/signaling NaN payloads and infinities;
- signed-zero preservation;
- malformed normals/colours lengths;
- stale, missing, duplicate, and corrupt `involved_faces` proving it is ignored semantically; and
- all Component 02 typed categories/subcodes.

### 21.2 Facet geometry tests

Include:

- convex and concave planar polygons;
- clockwise/counterclockwise source rings;
- long thin facets and extreme aspect ratios;
- exact and nearly collinear corners;
- adjacent coordinate-coincident but index-distinct corners;
- non-adjacent coordinate-coincident corners;
- proper self-crossing bow-tie rings;
- non-adjacent tangent touch and collinear overlap;
- planarity residual definitely below, exactly at, straddling, and definitely above the declared precision;
- signed zero, subnormal offsets, adjacent floats, extreme exponents, and large translations;
- projection-axis ties and fallback plane proposals;
- work-limited all-triples fallback; and
- validation-decomposition boundary/coverage checks.

Use an in-tree exact integer/rational oracle for low-complexity projected orientation, segment relation, and area. Use the adaptive exact-floating predicate only where its qualified assumptions are part of the fixture.

### 21.3 Edge and vertex topology tests

Hand-author and generate:

- genus-zero and positive-genus closed shells;
- several disconnected shells;
- open boundary edge;
- exactly two same-direction uses;
- three-use and higher-use edges;
- duplicate directed uses hidden by duplicate facets;
- one source vertex index shared by two otherwise closed fans;
- bow-tie vertex with every undirected edge paired;
- open link chain;
- repeated link incidence;
- high-valence valid cyclic fan;
- duplicate-coordinate separate vertices; and
- mutations preserving global counts while breaking reciprocal pairing or link connectivity.

Require precise subcodes and independent verifier rejection.

### 21.4 Shell semantics tests

Cover:

- one outward outer shell;
- one reversed outer shell;
- several disjoint solids;
- one cavity with correct and reversed orientation;
- islands inside cavities;
- at least six alternating nesting levels;
- shell order and facet order permutations;
- point-touching, edge-touching, and face-touching separate shells;
- definitely nested shells with near-boundary probes;
- probes whose uncertainty touches another shell;
- sibling positive-volume overlap;
- coincident same- and opposite-orientation shells under V1 rejection policy;
- ambiguous candidate parents;
- deliberately cyclic corrupt parent records; and
- total winding zero, one, and unsupported two.

For accepted cases, assert parent, depth, occupied side, orientation parity, probe witness, and winding. For ambiguous cases, require `ambiguous_shell_semantics`, never heuristic selection.

### 21.5 Epsilon-validity tests

Create exact-bit fixtures with:

- one-ULP gaps and overlaps;
- non-adjacent transverse triangle/facet intersections;
- coplanar partial overlaps;
- authorized source-edge and source-vertex contacts;
- contacts resolvable within declared input precision;
- contradictions larger than declared precision;
- near-coplanar overlapping facets;
- collapsed local triangles and wedges;
- severe shell self-intersection;
- nested boundaries whose uncertainty overlaps;
- finite bounds that overflow during inflation; and
- mandatory, diagnostic-rich, exhaustive-test-only, and topology-only modes.

Verify that topology-only artifacts are marked nonpublishable and cannot enter an ordinary Component 04 transaction.

### 21.6 Canonicalization and metamorphic tests

For valid operands, apply:

- all small vertex-array permutations with index remapping;
- thousands of fixed-PRNG larger vertex permutations;
- facet permutations;
- ring rotations;
- component/shell permutations;
- exact-coordinate duplicate occurrence permutations;
- symmetric tetrahedron/cube/duplicate-shell automorphisms;
- exactly representable translations;
- power-of-two scaling with precision scaling;
- global orientation reversal with the expected strict-policy failure/remapped evidence; and
- legal source subdivision represented as a distinct semantic mesh where appropriate.

Require identical semantic artifact bytes/digest for presentation-only permutations, correct presentation maps/source digests, and identical primary typed failures for semantically equivalent invalid presentations where canonical evidence is available.

Force all canonical-hash comparisons to collide and require unchanged full-byte order and IDs.

### 21.7 Mutation tests

Starting from valid proposed artifacts, inject independently:

- missing/extra ring position;
- incorrect normalization map;
- reduced plane or point uncertainty;
- changed projection axis;
- omitted segment crossing;
- missing edge use;
- third edge use;
- wrong pair direction;
- non-reciprocal pair ID;
- second vertex-link cycle;
- wrong link order with corrected counts;
- merged coordinate-coincident source identities;
- wrong shell membership;
- incorrect shell parent/depth;
- reversed occupied-side evidence;
- altered probe classification;
- unsupported winding value;
- omitted forbidden intersection;
- topology-only marker cleared;
- non-dense/wrong-owner/wrong-domain ID;
- inconsistent caller/canonical map;
- changed canonical bytes with unchanged digest;
- changed digest with unchanged bytes; and
- underreported persistent resources.

The independent verifier must reject every required mutation deterministically. No mutation may be rejected solely because a producer-owned cached count was not updated.

### 21.8 Fuzzing, shrinking, and replay

Use a fixed deterministic PRNG. Fuzz both:

- generated valid closed polygonal manifolds with nested/disconnected shells; and
- intentionally invalid soups produced by one controlled topology/geometry mutation at a time.

Every crash, hang, nondeterministic artifact/error, unexpected `internal_invariant_error`, producer/verifier disagreement, broad-phase false negative, or exact-oracle disagreement must be minimized and committed to the permanent replay corpus with exact coordinate bits, indices, options, expected result, and provider versions.

Implement structure-aware shrinking for vertices, facets, shells, ring chains, coordinate exponents/mantissas, precision, and contact distance while preserving the failure predicate.

### 21.9 Structural performance and build matrix

Record and gate:

- ring-segment candidates versus exhaustive segment pairs;
- canonical refinement rounds and branch nodes;
- shell probe/ray attempts;
- validation BVH nodes and triangle candidates;
- exact bounded predicate calls; and
- peak temporary/persistent resources.

Use large disjoint and clustered fixtures to catch accidental unconditional all-pairs behavior. Time alone is not a correctness gate.

Run supported GCC/Clang Debug and Release, ASan/UBSan, TSan for any parallel capability, `float`/`double`, and `uint32_t`/`uint64_t`. Prove strict Component 02 translation units remain outside inherited fast-math settings.

### 21.10 Contract traceability

Maintain a test manifest mapping every normative clause in `component_02_input_validation_shells.md` and every Section 23 completion item to one or more named tests. CI must fail when a required clause has no executable evidence or an expected mutation has no rejecting verifier test.

## 22. Implementation sequence and gates

1. **Schemas, versions, subcodes, file layout, test provider.** Gate: all headers compile under strict C++17, owners/versions/subcodes serialize, and the deterministic Component 03 test provider passes its enclosure self-tests.
2. **Preflight, immutable source copy, scalar/index checks, ring normalization.** Gate: structural unit tests, overflow boundaries, reversible maps, and normalization idempotence pass.
3. **Bounded facet geometry and validation-only decomposition.** Gate: planarity/projection/simplicity/orientation corpus and exact low-complexity oracle pass.
4. **Directed/undirected incidence.** Gate: every edge category and reciprocal pairing mutation passes.
5. **Vertex-link validation.** Gate: high-valence valid fans and all bow-tie/multiple-cycle mutations are independently distinguished.
6. **Canonical source topology and presentation maps.** Gate: permutation/automorphism/collision tests produce stable semantic bytes and dense IDs.
7. **Shell discovery and bounded orientation evidence.** Gate: disconnected/genus/numerical orientation fixtures and verifier recomputation pass.
8. **Containment, nesting, parity, occupied-side, and winding.** Gate: complete shell-semantic/contact matrix passes without random or heuristic fallback.
9. **Scalable epsilon-validity search.** Gate: exhaustive small-case comparison finds no omitted interaction; all required contradiction/uncertainty fixtures return precise typed results.
10. **Immutable artifact codec/digests and independent verifier.** Gate: round trips, source-versus-semantic digest behavior, every mutation, and resource reconstruction pass.
11. **Transactions, cancellation, diagnostics, replay, and full matrix.** Gate: cancellation at every checkpoint joins work and publishes nothing; error bytes are invariant under range/schedule permutations.
12. **Sanitizer/type/compiler/performance/traceability qualification.** Gate: Section 23 is complete before `tracker.md` is changed.

Do not integrate Component 04 against an unverified or topology-only artifact. Do not optimize canonical labeling or geometry search by weakening conservative behavior or verifier independence.

## 23. Definition of done

Component 02 is complete only when all of the following are true:

- every accepted source coordinate is finite and every source index/count/byte calculation is checked before dereference or allocation;
- permitted ring normalization is index-only, reversible, orientation-preserving, and idempotent;
- every accepted facet has certified bounded planarity, deterministic projection, a simple V1 ring, nonzero accepted orientation, and validation-decomposition evidence;
- every undirected source edge has exactly two reciprocal opposite directed uses derived from indices;
- every referenced source vertex has exactly one independently verified closed cyclic fan;
- disconnected shells are discovered solely through paired topology and have stable canonical member records;
- shell geometric orientation, parentage, depth, occupied side, and total winding are deterministic, bounded, and independently reconstructed;
- point- and edge-touching shells remain topologically separate, unsupported coincidence is rejected by policy, and no contact heuristic creates parentage;
- mandatory input geometry checks conservatively detect forbidden non-adjacent intersections, unsupported overlaps, collapsed/inverted regions, contradictory contacts, and uncertainty spanning incompatible topology;
- a topology-only diagnostic artifact is unmistakably nonpublishable for ordinary success;
- duplicate-coordinate source identities never merge and coordinate proximity never creates topology;
- canonical semantic artifact bytes/IDs are invariant under source vertex, facet, ring, component, shell, work, and thread permutations, including symmetric cases;
- caller-to-canonical and canonical-to-caller mappings are total and presentation-correct;
- all precision values and bounds originate from Component 03 and never shrink without proof;
- every accepted artifact contains reconstructible plane, incidence, link, shell, nesting, geometry, precision, and provenance evidence required downstream;
- the independent verifier reconstructs facts through materially different control flow and rejects every required mutation;
- all failures use stable Component 01 categories/subcodes/stages/entities/numeric witnesses/replay data and deterministic arbitration;
- all allocations and work are preflighted, resource-accounted, cancellation-safe, rollback-complete, and transactionally published;
- no caller storage, temporary ID, pointer value, source-order dependency, random ray, locale, wall clock, or hash-only equality enters the artifact;
- no legacy Boolean file, unsuitable mutating mesh helper, unbounded contour predicate, external dependency, or downloaded test framework is used;
- unit, known-answer, property, metamorphic, adversarial, mutation, fuzz/shrink/replay, performance, sanitizer, compiler, and type-matrix tests pass;
- every normative Component 02 clause maps to executable evidence; and
- production and normative-test code remain strict portable C++17 with no external dependency.
