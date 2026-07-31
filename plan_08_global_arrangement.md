# Component 8 implementation plan: global boundary arrangement and stitching

## 1. Scope and outcome

Implement the operation-independent stage that consumes Component 7's immutable `refined_facet_patches<T, I>` and its retained Component 6/2 dependencies, stitches every refined source sheet, constructs complete non-manifold seam and coincidence topology, and publishes one immutable `arrangement_complex<T, I>`. The artifact must preserve every source-sheet occurrence while exposing a common exact geometric domain and a complete graph of open patch sides/seam sectors suitable for Component 9 occupancy propagation.

The completed component must provide:

- one global arrangement vertex for every symbolic vertex used by a local patch, seam, coincidence boundary, or isolated incidence;
- exact-covering maps from every Component 7 local patch, local halfedge, and local vertex occurrence into global records, including one-to-many maps where common atomization splits a local entity;
- globally closed oriented source-sheet subdivisions with a distinct sheet-mate relation for every patch-boundary use;
- canonical atomic seams with complete incident sheet uses, exact cyclic radial order, endpoint links, and all open sectors;
- maximal positive-area coincident domains with common atomic subdivision, separate oriented source-sheet members, and exact membership multiplicities;
- explicit positive/negative patch-side fragments and complete local transparent, sheet-crossing, seam-sector, coincidence, and vertex-neighborhood transitions needed by Component 9, without falsely identifying disconnected fragments of one 3D cell;
- exact source reconstruction, cycle, mate, seam, radial-order, coincidence-coverage, side-graph, and total-mapping certificates;
- deterministic IDs, encoding, diagnostics, accounting, cancellation, replay, independent verification, and transactional publication.

Component 8 does not classify either operand, evaluate a Boolean operation, choose among coincident representatives, realize `T` coordinates, simplify selected geometry, or assemble an `fv_surface_mesh`. It records source boundaries and open-region adjacency without assigning `(inside_A, inside_B)`. Component 9 owns occupancy; Component 10 owns representative selection.

All implementation must be self-contained in Ygor, portable C++17, and free of external dependencies. Do not include or call `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Consume Component 1's `boolean_context`, strong IDs, owner tokens, accounting containers, checked arithmetic, deterministic executor, canonical encoder, diagnostics, trace, replay binding, verifier registry, artifact transactions, cancellation, typed failures, and immutable dependency store.
- Consume Component 2's exact source facets, edge uses/twins, vertex links, shells, support planes/charts, source orientation, and occupied-side policy. Never reconstruct authoritative topology from the borrowed `fv_surface_mesh` or `involved_faces`.
- Consume Component 3's exact plane/carrier equality, projected region relations, point/segment membership, angular order, `carrier_radial_order_v1`, `open_side_v1`, exact witness construction, and batch pre-ranking. Standard comparators inspect only frozen ranks and fixed fields.
- Consume Component 6's canonical symbolic vertices, carriers, atomic intervals, source-edge split sequences, incidence/radial records, overlap-region mappings, orientation multiplicities, and provenance. Component 8 never interns or moves a symbolic point.
- Consume Component 7's complete local point incidences, shared semantic edges, local DCEL records, source-edge chains, positive-area patches, artificial-cut records, orientation/occupied-side metadata, and exact coverage certificates. Retain strong ownership of the exact refined, symbolic, and validated artifacts.
- Adapt canonical undirected-edge aggregation and directed-use checks from `src/YgorMeshesVerification.*`, adjacency traversal from `src/YgorMeshesOrient.cc`, and half-plane/radial-cycle rotation patterns from `src/YgorMathMonotoneDecomposition.cc`. These are structural patterns only; replace mutable raw indices, approximate geometry, positional ties, and repair behavior.
- Use `YLOGDEBUG`, `YLOGINFO`, or `YLOGWARN` only to forward optional summaries after deterministic records are frozen. Never use `YLOGERR` for expected or invariant failures.

No current Ygor facility implements exact global sheet stitching, non-manifold radial sectors, positive-area coincidence overlay, owner-safe immutable topology, or a classification-complete side graph. Components 1-7 are planned production prerequisites and must be implemented and passing before Component 8 production integration.

### 2.2 Reject as production behavior

- Do not use `fv_surface_mesh::involved_faces`, `ClassifyEdges`, `EnsureConsistentFaceOrientation`, hole-chain repair, mesh welding, BSP splitting, or legacy Boolean adjacency as an authority. They use raw indices, floating positions/tolerances, mutable repair, or manifold assumptions that do not model an overlay.
- Do not use `vec2<T>`, `vec3<T>`, `long double`, epsilon, snapping, coordinate welding, rounded equality, normalized floating normals, `atan2`, pointer order, hash order, worker order, or random perturbation for identity, incidence, coincidence, orientation, or radial order.
- Do not identify a global patch with a source-sheet occurrence. One exact geometric domain may have several separately oriented sheet uses; one local patch may cover several global domains.
- Do not overload one `twin` field with local geometric reverse, source-sheet mate, radial predecessor/successor, or coincident membership. Publish separate typed relations.
- Do not retain an artificial Component 7 decomposition diagonal as a mathematical seam or coincidence boundary. It is transparent subdivision topology and may only partition source-sheet uses temporarily.
- Do not discard duplicate/coincident sheets, cancel multiplicity, choose an operand-preferred representative, or infer occupancy. Exact side labels and the Boolean truth table in Components 9/10 decide those outcomes.
- Do not create a new exact point when overlay boundaries cross. Every semantic crossing must already be a Component 6 symbolic vertex represented by Component 7. A missing split is an upstream invariant failure.

## 3. Files, namespace, and integration

Add:

- `src/YgorMeshesBooleanGlobalArrangement.h`: closed schemas, owner-bound references, immutable vertices/edges/domains/sheet uses/seams/sectors/coincidence groups/side graph/certificates, read-only accessors, artifact constants, and stage entry point.
- `src/YgorMeshesBooleanGlobalArrangement.cc`: dependency validation, source-sheet stitching, semantic-domain reconstruction, coincidence overlay, seam atomization/radial ordering, vertex-link construction, side-graph assembly, canonical merge/encoding, coordinator, verifier adapter, and four `<T, I>` explicit instantiations.
- `tests/Test_MeshesBooleanGlobalArrangement.cc`: focused contract, stitching, seam, coincidence, side-graph, failure, mutation, and round-trip tests.
- `tests/Test_MeshesBooleanGlobalArrangementProperties.cc`: generated permutation, subdivision, differential, metamorphic, sharding, and schedule tests.
- `tests/MeshBooleanGlobalArrangementFixtures.h`: synthetic validated/symbolic/refined artifacts, exact sheet/seam/coincidence builders, independent low-complexity overlay/radial oracle, bit-pattern fixtures, deterministic PRNG, mutation helpers, and replay records.
- Modify `src/YgorMeshesBooleanContract.{h,cc}`: add the deliberate ID/reference domains, limits, feature references, accounting categories, diagnostics, and encoding support listed below instead of using untyped indices.
- Modify `src/YgorMeshesExactKernel.{h,cc}` only if the exact radial-order, planar-overlay witness, and vertex-link probe operations below are not already exposed by Component 3; extend Component 3 tests at the same time.

Use namespace `ygor::mesh_boolean`. Keep provisional occurrence tables, plane-overlay DCELs, radial rank tables, endpoint-link worklists, worker shards, and verifier implementations private to the `.cc` or tests. Public templates remain thin and use explicit instantiation for `float`/`double` with `std::uint32_t`/`std::uint64_t`.

Add the `.cc` to Component 1's explicit strict-arithmetic source list in `src/CMakeLists.txt`; effective fast-math and contraction must be disabled and compile-time guarded. Add both tests to the authoritative in-tree CTest integration and `tests/compile.sh`, register `MeshBooleanGlobalArrangement.Unit` and `.Properties`, and label them `mesh_boolean;component8`. Do not depend on network-fetched doctest. Add authoritative GCC/Clang Debug/Release, ASan/UBSan malformed-artifact/rollback, and TSan seam-shard/coincidence-shard/cancellation/publication runs.

Before coding, reconcile these prerequisite interfaces rather than adding parallel facilities:

1. Extend Component 1 with strong `global_vertex_id`, `global_atomic_edge_id`, `global_halfedge_id`, `global_patch_id`, `source_sheet_member_id`, `sheet_use_id`, `seam_id`, `seam_sector_id`, `source_edge_sector_id`, `coincident_group_id`, `patch_side_id`, and `vertex_sector_id`, or owner-safe artifact-local references with exactly those non-convertible domains. Retain `cell_id` for Component 9; Component 8 does not claim that side fragments are classified cells.
2. Extend resource policy with explicit or documented aggregate limits for global vertices/edges/halfedges/patch domains/source-sheet members/sheet uses/seams/seam sectors/source-edge sectors/coincident memberships/side nodes/vertex sectors/transitions/probe descriptors/mapping entries/certificate entries and planar/radial/link scratch. Existing `global_halfedges`, `global_patches`, and `cells` are not enough to bound construction.
3. Component 3 must expose fallible batch pre-ranking for oriented plane rays around a canonical carrier, exact open angular-sector witnesses, plane-overlay points/segments/regions, and the finite homogeneous direction/sign predicates and rational cone-witness construction required by the spherical-link algorithm in Section 5.4. All resulting sort keys are immutable and `noexcept`.
4. Component 7 must expose every local halfedge, including exterior/zero-area incidences, through owner-checked stable references; distinguish geometric reverse from patch-boundary use; expose exact-covering semantic-domain reconstruction across artificial cuts; and retain enough parent-face data to prove artificial boundaries transparent.
5. Component 6/7 must guarantee that every crossing between semantic source/intersection/overlap boundaries is registered and atomized. Component 8 may run a read-only completeness audit, but it does not start a new reconciliation generation. A defect restarts upstream Components 6/7 under their protocol before Component 8 is invoked, or fails `internal_invariant_error` once verified dependencies have been accepted.

## 4. Stage and ownership contract

Expose a coordinator equivalent to:

```cpp
template<class T, class I>
status_or<std::shared_ptr<const published_artifact<arrangement_complex<T, I>>>>
build_global_arrangement(boolean_context<T, I>& context);
```

The coordinator must:

1. Require the exact published `refined_facet_patches<T, I>` from `artifact_slot::refined_facet_patches`, its strongly retained `symbolic_complex<T, I>` and `validated_operands<T, I>`, matching owner/setup/kernel policy, and the registered Component 8 verifier specification.
2. Reject copied, stale, or replacement dependencies even if bytes/digests match. Validate generation chains and require the latest refined artifact after any Component 7 reconciliation restart.
3. Validate all dense IDs, role-qualified facets/shells, local ranges, shared-edge mappings, source-edge chains, patch cycles, artificial-cut certificates, symbolic incidence, plane ownership, and construction owners before geometric work.
4. Open one `global_arrangement` transaction targeting `artifact_slot::arrangement_complex`. All global drafts, rank tables, overlays, mappings, encodings, reports, and diagnostics remain private until every source sheet, seam, coincidence domain, and vertex neighborhood succeeds.
5. Process canonical source facet, carrier interval, support-plane class, and symbolic-vertex frontiers independent of requested operation. Union/intersection/difference/xor over identical upstream artifacts produce identical `YGBCAN08` bytes.
6. Normalize, assign final IDs, encode, independently verify, and publish atomically only after a final cancellation check and matching certificate.
7. Return `resource_limit` for declared bytes/work/entity/exact-number/trace limits, allocation failure, kernel resource exhaustion, or cancellation. Return `internal_invariant_error` for malformed dependencies, missing semantic splits/incidence, impossible sheet mates, contradictory orientation/coincidence, incomplete side topology, encoding disagreement, or producer/verifier mismatch.

The artifact retains typed strong handles to the exact refined, symbolic, validated, and escaped construction storage artifacts. It contains no borrowed mesh pointer, worker/provisional index, mutable adjacency map, rank scratch, rounded coordinate, selected Boolean representative, or occupancy label.

## 5. Public artifact schema and conventions

### 5.1 Geometric domains and source-sheet uses

Freeze closed numeric enums for edge kind (`source_edge`, `intersection_seam`, `coincidence_boundary`, `transparent_artificial`), halfedge relation kind, sheet-use role, seam relation, radial coincidence class, seam-sector kind, source-edge-sector kind, vertex-germ kind, sector transition kind, probe-base-stratum kind, coincidence membership orientation, local-map kind, and certificate invariant. Reject unknown values during decode.

Define one `arrangement_vertex` per referenced canonical `symbolic_vertex_id`. It stores exact symbolic provenance plus sorted ranges of local vertex/point incidences, incident global edges, seams, sheet uses, and vertex sectors. Exact-equal source vertices map together only when Component 6 assigned the same symbolic ID; coordinate equality is never recomputed as topology. Isolated point contacts remain vertices with incidence/vertex-neighborhood records but no invented positive-length edge.

Separate these concepts:

- `global_atomic_edge`: one exact endpoint pair/open segment after exact straight-segment equality proof, with complete source-edge, carrier, interval, and semantic identities retained only as sorted payload/proof obligations. A segment carrying several roles is one edge, and distinct sheet occurrences share it.
- `global_patch`: one maximal atomic positive-area geometric domain with exact support plane, outer/hole cycles, exact area, and canonical plane-side convention. It has no operand priority or occupancy.
- `source_sheet_member`: one maximal connected coplanar oriented boundary layer within one `(operand, shell)`, formed across source edges only when exact planes/orientation/occupied side agree and deleting the edge preserves the same embedded sheet germ. Different shells or duplicate boundary occurrences always have different members. The identity is independent of facet tessellation, local artificial cuts, and global patch atomization; it retains the exact covering source facet/local-patch ranges.
- `sheet_patch_use`: one actual oriented source-sheet-member occurrence covering one global patch, with member/operand/shell/facet/local-patch coverage provenance, source normal parity, and which global plane side is operand-interior according to Component 2.
- `global_halfedge`: one directed boundary use of one `sheet_patch_use`, with origin/destination, edge, patch-cycle next/previous, and direction. Its `sheet_mate` is the unique opposite boundary use in the same source sheet subdivision. Geometric reverse, source-sheet mate, radial neighbors, and coincident counterparts are separate fields/records.

The canonical support-plane orientation is the Component 3 primitive plane carrier orientation. `global_patch::positive_side` means positive `oriented_eval` for that plane and `negative_side` means negative. Every sheet use stores parity from source facet orientation to the canonical plane and therefore explicitly identifies its occupied side. Reversing a source facet swaps sheet-use orientation/occupied-side mapping but never redefines the global sides.

### 5.2 Total local-to-global maps

Publish owner-checked maps for every Component 7 local vertex, point incidence, atomic edge, halfedge, boundary walk, face/cell, patch, shared semantic edge, source-edge chain, and artificial cut.

- Every local record has exactly one mapping record.
- A mapping record may contain a nonempty ordered range of global fragments when coincidence/common-domain atomization splits its geometry; the fragments have disjoint open interiors and exact union equal to the local entity.
- Each global fragment stores the reverse sorted range of all covering local records.
- A local halfedge not used by a positive-area source patch maps once to a retained `local_incidence_image` containing its exact global vertex/edge image when present, local face/walk role, zero-area/exterior status, and reverse local incidence; it does not become a fictitious sheet boundary. Boundary-walk and face/cell image records similarly preserve their complete child images and source-domain/outside roles. A patch-boundary halfedge maps to one or more global halfedge fragments.
- A local patch maps to one or more `sheet_patch_use` records whose exact domains partition it. Thus “maps exactly once” means one total mapping record, not one global ID.
- Artificial edges map to transparent adjacency records and their two sides; they have no symbolic seam, source multiplicity, or coincidence-boundary identity.

No global atomization may add a point. Split endpoints must be existing arrangement vertices with Component 6 identity and Component 7 incidence. A semantic crossing without such an endpoint is `internal_invariant_error` with upstream evidence.

### 5.3 Source-sheet adjacency

Every boundary of every sheet-use domain receives exactly one `sheet_mate` in the same operand/shell source sheet:

- across an original source edge, match Component 2 twin facets through Component 7's identical reversed shared-atom chains;
- across an intersection or overlap subdivision edge inside one source facet, match the two adjacent local source-domain patch sides;
- across a transparent artificial cut, match its two decomposition pieces and mark the transition transparent;
- across a boundary split introduced by common coincidence atomization, derive both children from the previously proven mate and split both directions identically.

Require mate involution, opposite direction, identical global atomic edge/domain, compatible source orientation, and no cross-operand mating. A seam may have many sheet uses geometrically, but each sheet boundary use still has exactly one source-sheet mate. Closed source sheets have no unmatched use.

### 5.4 Seams, radial sectors, and endpoint links

Define a `seam` as one maximal positive-length atomic carrier interval over which endpoint set, incident sheet-use set, support-plane relation, coincidence membership, source ownership/multiplicity, and radial order are constant. Split seams at every Component 6 interval endpoint, source-edge crossing, local patch boundary, overlap-domain boundary, radial-membership transition, or seam junction. Coplanar positive-area membership belongs to coincidence groups; boundary-only coplanar/tangent curve contact remains a seam/contact relation with its unperturbed relation preserved.

An edge is seam-eligible only when Component 6/7 labels it as an intersection curve, coplanar/contact curve, overlap boundary, or a continuation atom of such a curve. An ordinary source edge with no cross-operand event is not a seam. For every retained non-seam source-edge atom not eliminated by certified coplanar transparent adjacency, publish `source_edge_sector` records using the same exact radial-layer/open-interval construction; represent every distinct open radial interval and its two incident source-sheet sides. Thus ordinary creases have complete neighborhood topology without being mislabeled as symbolic intersection seams.

For each seam:

1. Orient its carrier by Component 3's canonical carrier direction and collect every incident directed sheet use from all local constraint occurrence maps.
2. Prove completeness against Component 6 curve/facet incidence and Component 7 edge coverage. Distinguish transverse crossing, tangent contact, source-edge continuation, coincidence boundary, and coincident angular layer.
3. Pre-rank oriented source planes with `carrier_radial_order_v1`. Exact equal angular rays form an explicit radial coincidence layer; stable keys order serialization inside the layer but do not create geometric sectors between equal rays.
4. Canonically rotate the cyclic order to the least complete sheet-use/layer key. Reversing carrier direction reverses the cycle under Component 3's documented mapping.
5. Publish one `seam_sector` for every nonempty open angular interval between consecutive distinct radial layers. Store bounding layers, exact/symbolic open-direction witness, incident positive/negative patch sides, and endpoint continuation references.
6. Record tangent layers and zero-angle coincidence without pretending they separate an open sector. Multiplicity remains attached to source-sheet members, not expanded into duplicate angular positions.

An edge radial order alone is insufficient at endpoints. For every arrangement vertex incident to seams, source-edge continuations, tangent contacts, or isolated multi-sheet contacts, publish a finite exact spherical link:

1. Translate every incident sheet germ to the vertex and represent its support plane by the homogeneous great circle `n dot d = 0`. Use a closed germ variant: a full great circle for an isolated facet-interior point incidence; the source-domain semicircle selected by exact inward edge-cone signs for an edge-interior incidence; the exact convex or reflex arc set between consecutive source/patch boundary rays for a source or patch vertex; and an ordinary closed wedge arc for a seam endpoint on a patch boundary. Derive the variant and inclusion signs from Component 2 vertex/edge links and Component 7 point location/patch cycles; an isolated incidence need not be a patch-cycle vertex.
2. Intern only exact-equal directed rays. Preserve antipodal rays as distinct IDs connected by an explicit involutive `antipode` relation. Add every pairwise great-circle intersection directed ray and its antipode, and split each sheet-germ arc at all rays proven to lie in its selected full/semicircle/convex/reflex/wedge domain. Equal coplanar arcs form explicit coincident arc layers with separate source members.
3. At each direction vertex, pre-rank incident directed arcs by exact triple-product orientation in the tangent plane, construct paired spherical halfarcs, and apply the left-region walk to obtain all link regions. Zero-angle/tangent arcs remain incidence layers and do not create a fictitious open region.
4. Construct one exact rational direction witness for every open region by solving its finite strict homogeneous sign system with Component 3's checked cone-witness API; substitute it into every incident plane/wedge. Symbolic perturbation may choose among boundary-degenerate probe candidates but never changes the unperturbed arc arrangement.
5. Map every seam sector, source-edge sector, patch side, and terminal/isolated contact to its unique link region or explicit boundary incidence. Canonicalize rays, arcs, walks, and regions by exact ranks and stable source member IDs only after equality.

This spherical DCEL is combinatorial and stores exact directions/evidence, not offset Cartesian points. Require every incoming sector to continue to exactly the appropriate outgoing/terminal link region, and require isolated tangencies to preserve distinct sides without manufacturing a crossing.

### 5.5 Coincident domains and common subdivision

Positive-area coincidence is exact equality of unoriented support planes plus positive-area intersection of open sheet domains. Construct, per canonical plane class, an exact planar overlay after first removing transparent artificial Component 7 cuts from each source-sheet domain. Source edges, intersection constraints, and overlap boundaries remain semantic.

The overlay must:

- include all member domain boundaries and every existing symbolic vertex on them;
- audit all segment pairs exact-only and require every crossing/overlap endpoint to be registered and atomized upstream;
- walk exact bounded cells and classify a certified open witness against every covering sheet domain;
- emit maximal connected positive-area `global_patch` atoms with a constant sorted `source_sheet_member_id`/orientation vector, splitting wherever membership, orientation, source occurrence, or semantic boundary provenance changes;
- preserve same-facing/opposite-facing orientation relative to the canonical plane for every `sheet_patch_use`;
- treat partial overlap, containment, equal domains, holes, disconnected regions, and differently tessellated source sheets as ordinary cases;
- retain zero-area coplanar contacts only in seam/vertex incidence, never as a coincidence group;
- prove each source local patch is exactly covered by its emitted uses and each coincidence member covers the full common atom.

Determine maximality without greedy polygon merging: delete exactly those overlay edges whose two incident cells have equal complete source-member/orientation/provenance-transition vectors and whose edge has no retained semantic role, compute canonical edge-connected components of the remaining cell adjacency, and derive one normalized polygon-with-holes domain per component. Point-only contact does not connect components; a non-Jordan union is decomposed by the same deterministic exact no-Steiner policy as Component 7 while retaining one component identity.

Publish one `coincident_group` for each maximal connected range of positive-area global patch atoms having more than one `source_sheet_member_id` and the same normalized member/orientation vector. Store its atomic-domain range, common exact boundary, all member uses/provenance, and relative orientations. There is one `sheet_patch_use` per actual source-sheet occurrence; true duplicate shells/sheets are never compressed into a multiplicity. Multiplicity fields are restricted to repeated constraint/derivation provenance and do not replace member records. Do not cancel, net, discard, or select members. Component 9 derives crossing effects from source orientation and occupancy policy; Component 10 chooses one result representative only after truth-table evaluation.

### 5.6 Classification-complete side graph

Explicit volumetric cells and global complement connectivity are deferred to Component 9, but complete local open-region adjacency is mandatory. Publish two `patch_side` fragments for every global patch, using the canonical negative/positive plane convention, and a canonical transition graph containing:

- transparent transitions across artificial decomposition boundaries and harmless source-facet tessellation boundaries that lie in the same sheet plane/domain;
- seam-sector incidence connecting each patch side to the correct open angular sector;
- vertex-sector continuation connecting incident seam sectors and patch sides through every seam endpoint/junction;
- exactly one oriented individual sheet-crossing transition on a noncoincident domain, carrying its operand, source-sheet use, and occupied-side relation;
- exactly one coincident-group crossing transition on a coincident domain, carrying the complete separate member-use vector and signed source-orientation contribution per operand; individual member crossing transitions are prohibited there because no open region exists between exactly coincident sheets;
- tangent/contact transitions that preserve open-region connectivity and do not assert a crossing;
- canonical connected components of this local transition graph as conservative topology fragments, plus one exact/symbolic open probe descriptor per `patch_side_id` for Component 9 direct classification.

Every transition states whether it is region-preserving (`transparent`, seam/source-edge/vertex-sector continuation, or incidence-only) or region-crossing (one sheet use or one coincident group). Compute conservative open-region components using only region-preserving transitions; crossing transitions are stored strictly as directed adjacency between two resulting component IDs and never participate in their equivalence closure. The graph is a conservative fragmentation of the open 3D complement: it may leave disconnected components that belong to the same mathematical cell around disjoint or nested shells, but it never joins different cells and never omits local radial/vertex adjacency. Component 9 exact-classifies every patch side independently and audits equal/transfer labels over this graph. Component 8 does not assign `cell_id`, identify occupancy, or claim a unique exterior graph component.

Define each component's `open_probe_descriptor` as a canonical formal point local to its least patch-side/seam-sector/source-edge-sector/vertex-sector fragment:

- a patch-side probe stores the patch's exact open-interior witness `p` and `open_side_v1(p, +/-n)`;
- an edge-sector probe stores an exact open point `p` on the seam/source-edge atom plus the sector's checked rational cone direction `d`, interpreted as `p + epsilon*d`;
- a vertex-sector probe stores the exact symbolic vertex `p` plus its spherical-region rational direction `d`, interpreted as `p + epsilon*d`;
- the empty-arrangement universe probe is the exact origin with no side constraints.

The record stores base-stratum kind/ID, exact base point/construction, canonical rational direction when present, perturbation key/formula version, and the complete finite signed plane/wedge/parameter constraints proving that sufficiently small positive `epsilon` lies in the intended open fragment. Construction chooses the least valid descriptor key only after proving the base point is in the relative open stratum, `d` is in the open cone, and all nonincident boundaries have nonzero constant separation; incident boundaries are decided by the first nonzero formal coefficient. Transparent graph traversal proves the chosen fragment represents its whole conservative component. Component 9 evaluates the formal probe directly through Component 3 exact/symbolic point-location APIs; no finite epsilon or realized coordinate is selected.

### 5.7 Certificates and top-level artifact

Publish a `global_arrangement_certificate` containing facts, not only booleans:

- local/global entity counts and total map cardinalities;
- source-sheet-member counts/ranges, per-shell Euler/orientation facts, mate counts, and reconstructed facet/edge coverage;
- global vertex/edge/patch/use/halfedge cycle counts and incidence checksums;
- seam atoms, radial layers, open sectors, source-edge sectors, endpoint/vertex-link regions, and complete incidence counts;
- support-plane overlays, coincidence atoms/groups/member multiplicities, exact area/domain coverage, and boundary-cancellation facts;
- patch-side fragments and transitions by kind, conservative graph-component facts, and certified open-probe descriptors;
- deterministic policy/schema versions and combined semantic digest.

The top-level bundle stores dense canonical ranges, dependency bindings, normalized statistics, reports, and certificate digest. Empty operands produce a valid empty complex; disjoint untouched operands reconstruct as separate closed sheets with no seams or coincident groups and still expose complete patch sides.

## 6. Construction algorithms

### 6.1 Dependency audit and global vertices

1. Enumerate every validated facet and require exactly one Component 7 refinement range, including identity refinements.
2. Recheck every source-edge split chain against Component 6 and its reverse against the Component 2 twin facet before opening geometric work shards.
3. Validate every local patch's plane, exact area, orientation, parent domain, semantic/artificial boundary labels, and occupied-side relation.
4. Collect all symbolic vertices referenced by local vertices, point incidences, semantic edges, and constraint mappings. Assign arrangement-vertex IDs from canonical symbolic ID/exact rank after complete incidence union.
5. Retain isolated point-only incidences and classify their local sheet germs; do not add zero-length edges.

### 6.2 Source-sheet reconstruction and mating

1. Remove artificial-cut boundaries logically by grouping local patch pieces across their certified transparent mates; retain mapping records so every local entity remains traceable.
2. Promote all semantic boundaries to provisional global atoms keyed only by the canonical unordered symbolic endpoint pair after exact proof that every occurrence is the same straight segment. Retain source-edge, carrier, interval, and role identities as sorted validating payload; union labels/provenance without cancelling source occurrences.
3. Split promoted boundaries at every existing global vertex on their closed segment. Require the resulting child chain to equal upstream mappings and reject an unregistered crossing.
4. Build provisional oriented sheet-use boundaries from local patch cycles. Match interior-facet mates, original-source-edge mates, and transparent mates by separate provenance keys.
5. Build `source_sheet_member` components from the canonical graph of coplanar same-operand/shell source domains, deleting a source edge only after exact plane/orientation/occupied-side equality and proof that both incident germs form one embedded planar layer. Point contact never joins members; distinct shells never join. Assign member IDs from complete normalized source coverage keys before global sheet uses.
6. Walk every reconstructed source facet/shell, proving closed cycles, mate involution, source orientation, member coverage, and subdivision equivalence to Component 2 before seam/coincidence processing.

### 6.3 Coincidence overlay and patch atomization

1. Group reconstructed positive-area sheet domains by exact unoriented support-plane equality; noncoincident planes never enter one planar overlay.
2. Project through Component 3's canonical plane chart, overlay every semantic boundary, and exhaustively audit pair relations. Hash/bounds may add candidates but exhaustive verification remains authoritative.
3. Walk overlay cells, construct exact open witnesses, classify membership against each source-sheet domain, and discard only cells covered by no sheet.
4. Delete exactly the overlay edges whose two incident cells have equal complete source-member/orientation/provenance-transition vectors and no retained semantic role; compute canonical edge-connected components of the remaining adjacency and derive normalized polygon-with-holes domains, applying the specified deterministic no-Steiner decomposition only for a non-Jordan component. Do not perform iterative pairwise polygon merging.
5. Materialize global patches and sheet uses. Build exact-covering local-patch maps and split existing mate relations consistently at all new boundaries.
6. Independently prove area and normalized boundary equality for every input local patch, reconstructed source facet, emitted global domain, and coincidence group.

### 6.4 Seam and vertex-sector construction

1. Select seam-eligible non-artificial global edge occurrences, then group them by Component 6 carrier/atomic interval and exact endpoint domain; atomize wherever incidence or radial payload changes. Build separate source-edge-sector records for ordinary creases that require vertex-link topology.
2. Build complete sheet-use incidence from both forward local maps and reverse symbolic mappings, then compare the two sets before ordering.
3. Pre-rank oriented planes, construct radial layers/order/sectors, and canonicalize cyclic rotations. Never invoke fallible exact arithmetic inside a comparator.
4. At seam endpoints and isolated contacts, gather complete source-facet germs from Component 2 vertex links and Component 7 incidences. Construct the exact symbolic link arrangement and all vertex sectors.
5. Connect seam sectors through vertex sectors and verify each patch-side boundary occurrence appears exactly once in the appropriate radial/link incidence.

### 6.5 Side graph, IDs, and publication

1. Create canonical negative/positive side nodes for every global patch and attach all oriented source-sheet uses.
2. Add transparent, noncoincident crossing, coincident-group crossing, tangent, seam-sector, source-edge-sector, and vertex-sector transitions from certified local topology. Require exactly one crossing representation per domain, verify no transition crosses an omitted sheet, and verify no open sector lacks continuation.
3. Compute conservative graph components from region-preserving transitions only, rewrite every individual/coincident crossing as adjacency between two component IDs, enumerate valid formal probe candidates from each component's fragments, prove every descriptor's open constraints and nonincident separation, and retain the least canonical descriptor per component.
4. Fallibly pre-rank all exact values, then assign IDs in dependency order: vertices, atomic edges, source-sheet members, geometric patch domains, sheet uses, halfedges/cycles, seams, radial layers/seam sectors, source-edge sectors, coincidence groups, patch sides, vertex sectors, transitions, and probe descriptors. Final comparators use complete immutable normalized keys.
5. Build reverse indices only from the frozen canonical relation tables; do not maintain independently mutable forward/reverse adjacency.
6. Encode, run the independent mandatory verifier, compare certificate facts, check cancellation, and publish transactionally.

## 7. Exact invariants and failure handling

Before a draft can succeed, prove:

1. Every local vertex/point/edge/halfedge/boundary-walk/face/cell/patch/artificial record has exactly one mapping record, and every one-to-many image is an exact disjoint-interior partition with complete reverse coverage.
2. Every global patch cycle closes; every global halfedge has valid next/previous and exactly one involutive same-sheet mate; no mate joins operands or unrelated shells.
3. Reconstructed source facets and shells are subdivision-equivalent to Component 2, preserve orientation/occupied side, and have no missing or extra positive-area domain.
4. Every seam atom has complete symbolic/local incidence, constant payload, canonical direction, complete radial layers, exactly one open sector between distinct adjacent layers, and consistent endpoint links.
5. Every coincident atom has exact plane/domain equality, a common atomic boundary, full member coverage, and retained orientation/multiplicity/provenance; partial overlaps are split into constant-membership domains.
6. Artificial boundaries are transparent and absent from semantic seam/coincidence identity; removing them preserves exact source-sheet coverage.
7. Every patch-side fragment and open seam/source-edge/vertex sector participates in a complete conservative classification graph; transitions are typed, use exactly one individual/group crossing representation, and cannot bypass a source sheet.
8. Endpoint, curve, plane, patch, and source incidence agree bidirectionally with Components 2, 6, and 7. No relation is inferred from `T` realization.
9. IDs, canonical bytes, selected first failure, diagnostics, and certificates are independent of facet/ring order, local worker order, hash behavior, allocation, threads, exact filter path, and requested Boolean operation.

Treat incompatible already-verified upstream artifacts, unresolved symbolic crossings, missing seam uses, impossible mates, contradictory source orientation, false coincidence, radial/link disagreement, or incomplete side topology as `internal_invariant_error`. Limits, cancellation, allocation failure, and exact-kernel resource exhaustion are `resource_limit`. Component 8 never reports `index_overflow` or `output_not_representable` because it does not serialize `I` mesh indices or realize coordinates.

Before work, use checked arithmetic to bound local mapping expansion, overlay pair work, possible planar cells/domains, sheet uses, halfedges, seams, radial incidences/sectors, vertex-link arcs/regions, coincidence memberships, side transitions, certificate entries, sort scratch, and canonical bytes. Use conservative sparse bounds and deterministic chunk grants instead of unchecked products. Never omit provenance, a seam use, coincident member, or graph transition to fit a limit.

Check cancellation before dependency validation, each grant/worker phase, bounded overlay-pair interval, exact fallback, cycle/radial/link walk, witness classification, mapping expansion, encoding, verification, and publication. Catch `bad_alloc` and unexpected exceptions only at task/stage boundaries, join siblings, select the canonical failure, and roll back. Diagnostics identify source/local/symbolic/global/seam/coincidence/sector IDs, invariant code, exact evidence, requested/current/limit facts, dependency generations/digests, and replay token.

## 8. Canonical encoding and deterministic execution

Define `YGBCAN08` as operation/invocation-independent arrangement semantics for one exact frozen refined dependency: schema/type versions; provenance-free validated/symbolic/refined semantic digests; stitching/overlay/radial/link/side-graph policy versions; global vertices/edges; source-sheet members; geometric domains/sheet uses/halfedges/cycles; total maps; seams/radial layers/seam sectors/source-edge sectors; coincident groups; patch-side fragments/vertex sectors/typed transitions; conservative component records and their crossing adjacency; certified open-probe descriptors; exact certificate facts; and deterministic semantic counts. Exclude owner tokens, pointers, setup digest, operation, raw invocation ordinals, diagnostics, traces, workers, timings, hashes, and approximations. Decode validates every corresponding store, ID/range, relation kind, component membership, crossing endpoint, and probe constraint before publication.

Also define `YGBCAN08Q`, an artificial-cut quotient view used only for metamorphic comparison: omit refined dependency identity, local mapping records, artificial edges, and transparent-cut provenance; encode normalized exact geometric domains, source-sheet-member coverage, semantic seams, coincidence groups, and the quotient side graph. Different valid Component 7 decomposition artifacts may have different `YGBCAN08`/`YGBARR08` bytes but must have identical `YGBCAN08Q` when their semantic source refinement is equal. For one identical frozen upstream artifact, schedule/filter/hash changes still require byte-identical full encoding.

Frame invocation-bound `YGBARR08` with schema/type versions, setup digest, exact strong dependency identities/generations/digests, policy versions, deterministic statistics, length-prefixed `YGBCAN08`, invocation-bound provenance/evidence, construction-storage binding, and verification report/certificate binding. Wrap it with Component 1's `YGBART01` framing for `arrangement_complex`. Decode rejects unknown enums/versions, bad owners/IDs/ranges, noncanonical exact values, unsorted/duplicate relations, broken cycles, bad lengths, and trailing bytes.

Partition canonical facet, plane-class, carrier, and vertex frontiers by a fixed versioned policy independent of thread count. Workers write accounting-backed private shards and cannot assign final IDs or publish. Grant work/kernel/storage envelopes in sorted `canonical_work_key(global_arrangement, domain, rank, phase)` rounds; join all workers and select failures by canonical precedence. The coordinator alone merges relation tables, assigns IDs, encodes, invokes verification, and publishes.

Canonical keys use exact pre-ranks first and stable semantic fields only after geometric equality. Canonicalize cycles by orientation-aware minimum rotation and radial cycles by the least complete layer key. Hash tables are lookup accelerators only. For one frozen invocation, valid changes to task count, shard partition, insertion order, cache/filter path, or allocator produce byte-identical `YGBARR08` and `YGBART01` payloads.

## 9. Mandatory independent verifier

Register a stable Component 8 artifact tag, schema/checker versions, and invariant set with Components 1/13. The read-only verifier receives the artifact, exact refined/symbolic/validated dependencies, exact-only kernel services, accounting, and cancellation. It must not call producer grouping, mate matching, overlay cell extraction, radial/link construction, ID assignment, certificate, or encoding helpers.

Mandatory checks:

1. Validate owner, slot/tag/schema, strong dependency identities/generations, setup/kernel/dependency digests, dense IDs, enum domains, role-qualified references, exact construction owners, and all ranges.
2. Independently reconstruct every local-to-global mapping, including boundary-walk and face/cell images, and prove totality, exact partition coverage, reverse coverage, and correct handling of exterior/zero-area local incidences and artificial cuts.
3. Rebuild every patch cycle and same-sheet mate from source provenance and exact directed geometry. Prove mate involution, original-edge twin reversal, interior adjacency, and no cross-sheet false mate.
4. Reconstruct every validated source facet/shell from global sheet uses. Compare exact domain/area, source-edge adjacency, orientation, vertex links, Euler facts, and occupied-side relation with Component 2.
5. Independently group semantic curve uses from Component 6/7, atomize constant-incidence intervals, and compare every seam endpoint, relation, source ownership, sheet-use incidence, and multiplicity.
6. Recompute each radial order with an independently written exact angular comparator, preserving equal-angle layers. Verify cyclic canonicalization and every seam/source-edge sector and witness.
7. Rebuild every seam-endpoint/isolated-contact spherical link from source facet germs, including full-circle, semicircle, convex/reflex, and wedge variants. Compare directed-ray antipodes, split arcs, vertex sectors, incoming/outgoing seam continuation, tangent topology, and patch-side incidence.
8. Independently remove artificial cuts, overlay each exact support-plane class, classify open cells, and compare global domains, local coverage, constant-membership atoms, coincident groups, member orientation/multiplicity, exact areas, and normalized boundaries.
9. Reconstruct the complete patch-side-fragment transition graph and prove each transition's transparent/individual-crossing/group-crossing/tangent semantics, exactly one crossing representation per domain, no missing sector continuation, and no sheet bypass. Independently substitute every formal open-probe descriptor into all incident and nonincident constraints and prove one valid canonical descriptor for every patch side.
10. Recompute all certificate facts/statistics and independently re-encode `YGBCAN08Q`, `YGBCAN08`, `YGBARR08`, and `YGBART01`; verify report, trace, replay, and construction-storage bindings separately.

Verifier resource exhaustion prevents publication with `resource_limit`; producer/verifier disagreement is `internal_invariant_error`. Exhaustive mode compares bounded cases with separately implemented rational planar-overlay, radial-order, source-sheet reconstruction, and combinatorial-link oracles that may share exact number values but not producer control flow.

Mutation tests alter every owner/ID/range, local/global map, symbolic endpoint, edge kind, source-sheet member, patch cycle/area/plane side, sheet use/orientation, mate relation, seam interval/incidence, radial layer/order, seam/source-edge sector witness/adjacency, vertex germ kind/directed ray/antipode/link, coincidence domain/member, artificial transparency, side transition, open-probe base/direction/sign constraint, dependency generation/digest, certificate fact, and serialization field/order. Every mutation must fail in Release/NDEBUG.

## 10. Test plan

### 10.1 Focused stitching and topology

- Empty operands; one untouched closed shell; disjoint shells; and identity refinements with no seam/coincidence records.
- Two adjacent source facets sharing an unsplit and multiply split source edge; concave facets; cavities; nested/disconnected shells; and high-valence source vertices.
- Internal local constraints and Component 7 artificial cut forests, proving same-sheet mates and transparent graph transitions without semantic seams.
- Open and closed transverse seams, seams continuing across source edges, several disjoint intervals on one carrier, and seams ending at original vertices, source-edge interiors, or other seam vertices.
- Stars of several noncoplanar sheets, equal/opposite radial rays, tangent sheet contacts, source edge collinear with a seam, and multiple carriers meeting at one symbolic vertex.
- Isolated facet-interior, edge-interior, and source-vertex point contacts and point-only carriers, exercising full-circle, semicircle, convex/reflex, and wedge link germs with complete vertex sectors but no fictitious edge/patch.

### 10.2 Coincidence and degeneracy

- Coplanar boundary-only contacts; partial/full positive-area overlap; containment; equal facets/solids; same-facing and opposite-facing sheets; and mixed orientation multiplicity.
- Coincident polygons with radically different source facet tessellations, source-edge subdivisions, local patch decompositions, concave outlines, holes, disconnected overlap regions, and overlap boundaries coincident with source edges.
- Several coincident members, a transverse seam entering/leaving a coincidence group, coincidence membership changing along a seam, and zero-area tangent contact adjacent to positive-area coincidence.
- Repeated constraint/provenance records, distinct exact vertices that round to one `T`, exact-equal coordinates with topologically distinct source vertices, signed zero, subnormals, extreme exponents, one-ULP gaps, and cancellation-heavy plane/radial predicates for all four template specializations.

Every focused case checks total mappings, patch cycles, mates, source reconstruction, radial/link sectors, coincidence area/domain coverage, side transitions, canonical bytes, and independent verifier output.

### 10.3 Differential and metamorphic tests

- Compare producer, mandatory verifier, and independent exact source-sheet/planar-overlay/radial/link oracles over deterministic generated small arrangements.
- Permute input vertex/facet/component order, rotate/reverse rings through revalidation, permute local record/provenance order, and change temporary overlay insertion order; full bytes remain identical where canonical upstream semantics are identical. Vary valid Component 7 artificial decompositions and require identical `YGBCAN08Q`, while dependency-bound maps/full bytes may differ.
- Vary threads 1/2/many, shard/frontier partition, worker delays, hash seed/collision mode, allocation addresses, exact filter path, and cache state; IDs, selected failure, diagnostics, and bytes remain identical.
- Swap operands and require source-role/orientation mapping with equal unoriented geometry. Re-run all Boolean operations and require operation-independent `YGBCAN08`.
- Legally subdivide source edges/facets or change coplanar tessellation; compare exact normalized domains, sheet coverage, seams, coincidence groups, and side graph through the feature-refinement map.
- Apply exactly representable translations, axis permutations, orientation-corrected sign flips, and power-of-two scaling; require the documented exact equivariance and identical combinatorial canonical form.
- Round-trip canonical and invocation encodings; golden bytes freeze empty, split-source-edge, transverse seam, tangent vertex, same/opposite coincidence, differing tessellation, radial star, and vertex-link grammars.

### 10.4 Failure and qualification

- Wrong owner, stale/replacement dependency, missing/duplicate local refinement, malformed range/cycle, reversed source-edge chain, missing semantic split, impossible mate, omitted seam use, contradictory radial layer, false coincidence, incomplete common subdivision, lost multiplicity, bad artificial transparency, and incomplete side graph fail closed.
- Exact-at-limit and one-over global vertex/edge/patch/source-sheet-member/use/halfedge/seam/seam-sector/source-edge-sector/coincidence/member/side/vertex-sector/transition/probe/private-byte/work/exact-number/diagnostic/trace cases; allocation failure and cancellation at every phase expose no partial artifact.
- Debug/Release and GCC/Clang outputs match. ASan/UBSan cover malformed references, cycle/radial/link guards, and rollback; TSan covers source/plane/carrier/vertex shards, cancellation, verifier, and publication.
- Benchmark untouched-sheet-heavy, split-source-edge-heavy, transverse-seam-heavy, high-radial-valence, point-contact-heavy, coplanar-overlay-heavy, and differing-tessellation coincidence cases. Record exact relation counts, mapping expansion, entities by kind, verifier work, peak private bytes, and limb growth. Performance changes may alter only versioned execution policy, never semantics or verification strength.
- Serialize failures with source coordinate bits, exact values, all source/local/symbolic/global IDs, seam/radial/coincidence/link evidence, dependency/policy digests, PRNG state, and expected/actual normalized records. Decimal coordinates alone are insufficient.

## 11. Component 9 handoff contract

Before Component 9 integration, prove it can consume only `arrangement_complex<T, I>` and retained dependencies to:

- enumerate every atomic geometric patch and all separately oriented source-sheet uses covering it;
- obtain stable canonical negative/positive `patch_side_id`s and translate each source use to operand-interior/exterior side;
- traverse all represented transparent local connectivity without crossing a source sheet;
- enumerate exactly one noncoincident sheet crossing or coincident-group crossing per domain with complete operand/orientation/source-occurrence provenance;
- distinguish transverse crossing, tangency, coincidence, incidence-only contact, and artificial transparency;
- traverse every seam sector through its endpoint `vertex_sector_id` without reconstructing radial or vertex links;
- exact-classify the supplied seed descriptor for every conservative graph component, including all fragments of the exterior-at-infinity cell, and optionally prove global complement connectivity/merge fragments without mutating Component 8;
- assign `cell_id`s to explicit cells or retain an equivalent over-segmented labeled-region graph, requiring equal labels whenever independently proven fragments belong to one cell;
- report any missing adjacency against stable Component 8 IDs as an upstream `internal_invariant_error`.

The artifact must not contain precomputed occupancy, net Boolean contributions, or a preferred coincident representative. It contains exactly the topology and source-side facts needed for Component 9 to derive those values with exact seed classification and path-independent propagation.

## 12. Implementation sequence and completion criteria

Implement in this order:

1. Reconcile Components 1-7 ID/reference, resource, exact radial/link/overlay, complete local-halfedge, artificial-transparency, verifier-environment, and dependency-generation interfaces; freeze Component 8 enums, relation semantics, policy versions, invariant codes, and encodings.
2. Implement immutable vertex/edge/domain/sheet-use/halfedge/seam/sector/coincidence/side-graph/map/certificate schemas with checked accessors and canonical encode/decode unit tests.
3. Implement single-threaded dependency audit, global vertex mapping, source-edge reversal checks, source-sheet reconstruction, semantic-edge promotion, and all same-sheet mate kinds.
4. Implement exact support-plane overlay after artificial-cut removal, common patch atomization, one-to-many local coverage maps, sheet uses, and coincidence groups.
5. Implement seam atomization, exact radial layers/order/sectors, endpoint symbolic links, isolated-contact topology, and completeness audits.
6. Implement classification-complete patch-side/sector/vertex transition graph and prove the Component 9 handoff on focused fixtures.
7. Add canonical IDs, resource envelopes, deterministic workers/merge, cancellation, canonical failure selection, diagnostics, replay, and transaction rollback.
8. Implement independent verifier, canonical encodings/digests, mutation suite, certificate gate, and atomic publication.
9. Run oracle, permutation, operation-independence, operand/orientation mapping, source-subdivision/tessellation, exact/filter, thread/schedule, adversarial-bit, sanitizer, replay, and benchmark qualification before Component 9 integration.

Component 8 is complete only when:

- every Component 7 local entity, including boundary walks and face/cell records, has one total mapping record with exact-covering global images and complete reverse provenance;
- every refined source sheet reconstructs its validated operand exactly, all patch cycles close, and every sheet boundary use has one compatible involutive same-sheet mate;
- every semantic seam has complete constant incidence, exact canonical radial layers/order, all open sectors, and complete endpoint/vertex-link continuation;
- every positive-area coincident region has a deterministic common atomic subdivision with all same/opposite member sheets and multiplicities retained, including partial overlap and differing tessellation;
- artificial decomposition boundaries are proven transparent and never become mathematical seams or coincidence ownership;
- the patch-side/seam-sector/source-edge-sector/vertex-sector graph represents every local open-region transition without crossing an unrecorded boundary, supplies one seed descriptor per conservative component, and is sufficient for Component 9 to classify every fragment without reconstructing local arrangement topology;
- exact source reconstruction, domain area/boundary coverage, mate, incidence, radial, link, coincidence, mapping, and graph certificates pass independent recomputation;
- IDs and encodings are independent of source/local order, hashes, partitions, schedules, threads, pointers, approximations, and requested Boolean operation;
- independent oracle/verifier comparison, mutation detection, resource/cancellation rollback, replay, Debug/Release, GCC/Clang, ASan/UBSan, and TSan suites pass;
- Component 9 can assign path-independent `(inside_A, inside_B)` labels and both-side patch labels using only this immutable artifact and retained exact dependencies, without repairing or reconstructing omitted topology.

## 13. Plan-gap amendment: occurrences, links, probes, and verifier isolation

This section supersedes any one-vertex-per-symbol topological interpretation and any claim that Component 8 components are complete 3D cells.

For each geometric `global_vertex_id`, gather one provisional node per oriented source-sheet germ and join nodes only through a recorded same-sheet, transparent-cut, atomized-boundary, seam-continuation, or coincident-member continuation witness. Canonically connected witness components become `vertex_occurrence_id`s. Halfedge endpoints and link components reference occurrences; geometric edges retain geometric endpoints. Coordinate equality, common support plane, crossing link arcs, or common symbol never joins occurrences.

Build the complete spherical DCEL at every multi-germ contact, including isolated contacts without seams. Generate rays from germ endpoints, incident edge directions, and every pairwise support-plane intersection; retain antipodes as distinct involutive records. Split every closed germ arc at every exact in-domain crossing, order tangent halfarcs by exact triple products, retain coincident layers, walk all spherical regions, and construct a strict exact witness for each open region. Map every patch side, edge/seam sector, and terminal contact exactly once and require each continuation to remain within a proven occurrence component.

Publish exactly two patch sides and one certified formal open probe per global patch. Prefer an exact relative-interior patch witness plus the canonical side normal. Every descriptor enumerates all incident constraints and proves nonincident separation; the first nonzero formal coefficient must have the required sign. A boundary-valued descriptor fails before classification.

Region IDs are conservative local-topology fragments only. Component 9 directly classifies every patch side under `independent_patch_side_v1`; transitions are topology evidence and consistency checks, not a complete-cell proof.

Move mandatory verification to a separate implementation family and standalone link target that excludes Component 8 producer objects. Independently rebuild occurrence partitions, radial order, spherical links, probes, and transitions from Components 2, 6, and 7. Self-consistent occurrence-weld, radial-swap, missing-continuation, unsplit-arc, and boundary-probe mutations must fail by exact semantic reconstruction after all producer-shaped counts, IDs, bytes, and digests are rebuilt.
