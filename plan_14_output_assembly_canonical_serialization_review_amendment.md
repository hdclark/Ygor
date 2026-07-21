# Plan 14 Review Amendment: Output Assembly and Canonical Serialization

## Status, precedence, and implementation intent

This file is a normative implementation-plan amendment to `plan_14_output_assembly_canonical_serialization.md`. It records the independent Component 14 review required by `tracker.md` and integrates the reviewed Component 02 automorphism contract, the reviewed Component 13 numerical/dimensional handoff, and the Component 15 final-publication boundary.

The original plan remains normative except where this amendment is more specific or conflicts with it. In a conflict, this amendment controls. Implementers, reviewers, test authors, codec authors, and Component 15 must read both files and the corresponding Component 14 specification amendment.

The original fixed V1 architecture remains suitable: private construction of `fv_surface_mesh<T,I>`, exact coordinate-bit copy, exact indexed topology, complete canonical graph labeling, checked public indices, public-topology reconstruction, monotonic precision/report assembly, normalized provenance, versioned logical bytes, domain-separated SHA-256, structural round-trip, an independent Component 14 verifier, a serial semantic reference, strict C++17, and no external dependencies.

This amendment corrects the reviewed integration defects without weakening that architecture:

1. consume Component 13's reviewed artifact and embedded predecessor `T,I` compatibility;
2. preserve all three Component 03 truth layers;
3. retain closed metrics, physical dimensions, derivations, and distinct cleanup-cost roles;
4. keep Component 12 handoff certificates advisory;
5. separate semantic public canonicalization from non-unique presentation correspondence; and
6. version every affected schema, codec, replay record, capability declaration, verifier report, and Component 15 handoff.

## 1. Reviewed provider and schema versions

Retain the original provider set, but assign new nonzero reviewed versions wherever observable evidence, correspondence semantics, or compatibility changes. Names may follow local registries, but use distinctions conceptually equivalent to:

```text
predecessor_contract:              component13_reviewed_truth_dimension_input_v2
public_mesh_adapter:               direct_private_fv_surface_mesh_triangles_v1
public_mesh_readback:              direct_const_fv_surface_mesh_view_v1
canonical_graph_model:             typed_oriented_incidence_graph_v1
semantic_labeling:                 minimum_oriented_public_mesh_block_v2
artifact_correspondence:           automorphism_equivalence_correspondence_v2
correspondence_certificate:        complete_orbit_bijection_certificate_v2
component_ordering:                public_bytes_then_invariant_artifact_class_v2
precision_aggregation:             component03_dimensioned_output_lineage_v2
cleanup_report_schema:             reviewed_dimensioned_cleanup_report_v2
provenance_schema:                 invariant_lineage_and_presentation_map_v2
candidate_schema:                  assembled_output_candidate_v2
logical_serialization:             output_domains_truth_dimension_correspondence_v2
replay_schema:                     output_assembly_replay_v2
component15_handoff:               reviewed_output_candidate_handoff_v2
producer_verifier:                 independent_dimension_correspondence_verify_v2
execution_reference:               serial_complete_canonicalization_v1
```

Unchanged carrier, checked-index, topology-rebuild, SHA-256, and strict-execution providers may retain existing versions only when their serialized inputs and outputs are unaffected.

Capability negotiation must reject:

- reviewed Component 14 with obsolete Component 12/13 truth, category, handoff, action, budget, codec, replay, or verifier schemas;
- an obsolete Component 14 consumer for a reviewed Component 13 artifact;
- mismatched embedded predecessor/public `I` descriptors;
- a candidate codec that lacks dimension/cost-role or correspondence-class fields;
- a Component 15 handoff version unable to inspect the reviewed evidence; and
- unknown, zero, mixed, or contradictory versions before authoritative allocation.

## 2. Corrected API and type compatibility

Keep the cleaned artifact independent of public index storage:

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

Before reading semantic records, validate:

- the `cleaned_triangle_manifold<T>` reviewed schema;
- the embedded scalar descriptor equals `T`;
- the embedded predecessor/source/public index compatibility descriptor equals the invocation's `I`;
- Components 11-13 used compatible `triangulated_output_complex<T,I>` and downstream descriptors;
- Component 13 advertises the reviewed Component 14 and 15 contracts; and
- every immutable predecessor handle shares the context owner, stable digest, operation, policies, and lifetime.

Do not change the cleaned artifact to store public indices or alias Component-owned IDs to `I`. Public positions remain checked internal ordinals converted to `I` only after the global capacity proof.

## 3. File-level implementation amendments

Apply these additions to the original production file plan.

### 3.1 `OutputAssemblyTypes.h`

Add closed enums and strong IDs for:

- physical dimension;
- tolerance-facing metric;
- numerical truth-layer kind and availability;
- cleanup value role: advisory, proposed, reserved, committed, rejected, one-action, cumulative-lineage, local-removal, component-removal, swept/support/clearance, representation, tolerance;
- dimensional derivation kind;
- `correspondence_equivalence_class_id`;
- `correspondence_orbit_id`;
- `presentation_correspondence_id`;
- correspondence disposition: uniquely distinguished, automorphism-equivalent, invalid;
- provenance attribution disposition: unique-entity, invariant-class, presentation-only, invalid;
- semantic/presentation serialization section kind; and
- corrected schema/provider constants.

Use explicit fixed-width nonzero enumerators. Reject unknown values and nonzero reserved fields. Do not use free-form strings as metric, dimension, role, truth, or correspondence authorities.

### 3.2 `OutputAssemblyPreflight.h/.cc`

Extend preflight to:

- validate the full reviewed Component 12/13/14/15 version graph;
- validate embedded `T,I` descriptors;
- verify all required truth-layer records and reverse references exist;
- verify every tolerance-facing predecessor value has a supported metric and dimension;
- verify every dimensional derivation resolves through Component 03 and has valid denominator evidence;
- verify Component 12 reserved and committed zero cleanup budget;
- verify advisory, proposed, reserved, committed, rejected, and cumulative roles are mutually coherent;
- verify Component 13's action and budget records, not handoff summaries, drive committed tolerance use;
- preflight storage and work for equivalence classes, orbit certificates, class-level provenance, presentation maps, and independent verification; and
- reject mixed schemas before graph or public-mesh allocation.

No public vertex, graph node, or mutable map slot may be allocated before these checks pass.

### 3.3 `AssembledOutputCandidate.h` and `OutputAssemblyQueries.h`

Add immutable candidate sections and checked queries for:

- reviewed predecessor compatibility;
- truth-layer references;
- dimensioned value and derivation tables;
- handoff-certificate/action/budget reverse maps;
- Component 12 zero-budget evidence;
- semantic canonical-label evidence;
- correspondence equivalence classes and orbit certificates;
- one concrete presentation correspondence;
- class-level and unique per-entity provenance;
- dimensioned reports and maximum witnesses;
- corrected logical-domain encodings/digests; and
- reviewed Component 15 handoff compatibility.

Queries distinguish semantic fields from presentation-only fields. A public-content encoder must not be able to access presentation-only records through an untyped generic iterator.

### 3.4 `OutputComponentReconstruction.h/.cc`

Retain exact reconstruction from Component 13 paired topology. Additionally:

- carry reviewed component/entity complete keys without assuming they uniquely distinguish exact automorphism members;
- build normalized member multisets for later equivalence-class checks;
- preserve the embedded predecessor `I` compatibility evidence;
- attach truth/dimension/action evidence ranges to reconstructed entities without interpreting them; and
- expose deterministic invariant component views to semantic canonicalization.

Temporary dense ordinals remain storage devices only.

### 3.5 `CanonicalIncidenceLabeling.h/.cc`

The shared generic kernel continues to own deterministic refinement, individualization, search, safe pruning, and minimum comparison. Amend its contract so callers can request separately:

- a semantic canonical minimum and automorphism group/orbit evidence; and
- verified presentation correspondences inside the winning semantic equivalence class.

The kernel must not promise a unique original-node representative for an exact orbit unless the caller supplies a presentation-independent unique semantic discriminator.

Preserve Component 02 golden semantic bytes. Add generic tests proving source-node permutations may change a presentation correspondence while leaving canonical semantic bytes and orbit partitions unchanged.

### 3.6 `CanonicalIncidenceGraph.h/.cc` and `OutputCanonicalLabeling.h/.cc`

Keep public-semantic initial colors free of detailed provenance, action identity, cleaned IDs, and presentation ordinals.

Split winning selection into:

1. select the minimum complete public-content block;
2. collect every accepted labeling that produces that same minimum, or a complete independently verifiable certificate of the induced automorphism/orbit classes;
3. compare normalized artifact-invariant class payloads only to order distinguishable components and dictionaries that do not alter public content;
4. determine which cleaned entities are uniquely distinguished and which remain in exact correspondence equivalence classes; and
5. emit semantic labeling evidence plus class/orbit evidence.

Do not force a unique leaf using cleaned numeric IDs, transient occurrence rank, source order, or branch enumeration. A unique normalized predecessor key may distinguish members only after validating that the key is genuinely presentation-independent and unique.

### 3.7 `OutputPermutation.h/.cc`

Build two related products:

- the canonical public component/vertex/facet positions determined by semantic labeling; and
- a source-correct cleaned/public presentation correspondence.

For uniquely distinguished entities, store the direct mapping.

For each exact equivalence class, store:

- cleaned member set;
- public-position set;
- incidence/orientation constraints;
- orbit/correspondence certificate;
- one verified concrete bijection used for lookup;
- normalized invariant provenance multiset; and
- proof that representative substitution cannot alter public semantic bytes/status.

Every concrete map remains total, bijective, range-valid, owner-correct, and topology-correct. The concrete representative may appear in replay/presentation sections but not in the public-content domain or semantic primary-failure key.

Triangle and edge-use correspondences must be induced consistently from the selected vertex/graph correspondence; independently choosing representatives per entity domain is prohibited.

### 3.8 `PublicMeshBuilder.h/.cc` and `PublicMeshAdapter.h/.cc`

The original carrier rules remain unchanged:

- exact `T` bit copy;
- frozen signed-zero rule;
- checked `I` conversion;
- exactly three indices per facet;
- optional normals, colours, `involved_faces`, and metadata empty;
- no mutating convenience methods;
- private transactional construction; and
- readback through the shared immutable adapter.

Add one assertion that no hidden identity, precision, unit, or correspondence data is stored in optional public fields. All such data belongs to the surrounding candidate.

### 3.9 `OutputPrecisionReports.h/.cc`

Consume reviewed Component 03 and Component 13 views. Implement aggregation over strong typed records:

```text
dimensioned_value =
    (role,
     metric,
     physical_dimension,
     conservative_interval_or_bound,
     derivation_reference,
     ledger_or_action_reference,
     canonical semantic entity/equivalence-class reference)
```

Required behavior:

- preserve `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` references separately;
- require length for every tolerance/budget-facing comparison;
- keep uncertainty, one-action displacement, cumulative displacement, local removal, component removal, clearance, representation effect, and tolerance separate;
- derive committed maxima from Component 13 action/ledger records;
- retain handoff values only under `advisory`;
- validate derivations and role transitions;
- select equal maxima by complete typed witness keys; and
- fail known budget violations before candidate publication.

Do not recompute geometry, convert units, infer a length from area, or use raw floating nominal order.

### 3.10 `OutputProvenance.h/.cc`

Extend normalized dictionaries with:

- truth-layer references;
- handoff certificate and action/budget role records;
- metric/dimension/derivation records;
- correspondence equivalence classes;
- invariant shared lineage and normalized lineage multisets;
- uniquely attributable per-entity lineage; and
- presentation-only source replay maps.

A class with indistinguishable members must not be assigned false per-public-position semantic provenance. Encode invariant class provenance in the artifact/provenance domains and concrete presentation attribution only in an explicitly presentation/replay-framed section.

### 3.11 `OutputLogicalCodec.h/.cc`

Revise logical layouts:

- **public mesh content domain:** unchanged semantic coordinates and canonical oriented index triples; contains no detailed provenance, truth evidence, units, cost roles, cleaned IDs, equivalence representatives, or presentation maps;
- **artifact domain:** reviewed versions, semantic component records, dimensioned reports, equivalence-class summaries, and dependency digests;
- **provenance domain:** unique lineage plus invariant class lineage/multisets;
- **assembly evidence domain:** truth-layer coverage, dimensions/derivations/roles, canonical-search/orbit certificates, class membership, and map-validity evidence;
- **replay-reference domain:** concrete presentation correspondence and exact source replay binding when required;
- **aggregate candidate domain:** fixed framed references to all reviewed domains.

Decoders reject unknown required dimensions/metrics/roles, impossible role transitions, mixed schema versions, invalid class ranges, duplicate/missing members, presentation fields in semantic sections, semantic omissions from mandatory evidence, noncanonical ordering, and trailing bytes.

Use Component 01 `CanonicalBytes` and SHA-256 only. Full structural bytes remain the authority when equality/order matters.

### 3.12 `OutputAssemblyVerifier.h/.cc`

Add an independently organized verification path that:

1. validates reviewed versions and embedded `T,I`;
2. validates all truth-layer, metric, dimension, role, derivation, and reverse-map records;
3. confirms Component 12 zero cleanup budget;
4. reconstructs committed report maxima without using producer summaries;
5. proves advisory records were not promoted;
6. reconstructs semantic canonical public bytes independently;
7. reconstructs exact orbit/correspondence classes for bounded cases and validates complete certificates for larger cases;
8. verifies the concrete map lies within the permitted class and induces consistent vertex/triangle/edge/component correspondences;
9. verifies provenance is unique only where mathematically justified;
10. regenerates all logical domains through separate traversals; and
11. rejects every reviewed mutation.

It may share Component 01 and Component 03 primitive services, but not the producer's higher-level class builder, presentation-map chooser, report aggregation, or codec buffer.

## 4. Corrected intake algorithm

Before authoritative allocation:

1. validate context owner, stable digest, operation, policies, floating profile, `T`, `I`, and Component 15 mandatory floor;
2. validate reviewed Component 12 and Component 13 artifact/category/truth/handoff/obligation/action/budget/codec/replay/verifier versions;
3. validate the cleaned artifact's embedded predecessor scalar/index descriptors;
4. validate dependency digests and local producer-verifier dispositions;
5. validate every required truth-layer reference and coverage map;
6. validate every tolerance-facing metric and physical dimension;
7. validate dimensional derivations and denominator preconditions;
8. validate certificate/action/budget role transitions and Component 12 zero-budget evidence;
9. validate cleaned topology, coordinate bits, precision lineage, action coverage, and no outstanding obligations;
10. derive checked counts, bytes, work, and resources for semantic labeling, orbit/class evidence, presentation maps, reports, codecs, replay, and independent verification;
11. reserve persistent and peak resources; and
12. only then reconstruct components and build canonical graph workspaces.

A valid but highly symmetric input may fail `resource_limit` if complete semantic minimum or class certification cannot be established. It must not fall back to presentation order.

## 5. Semantic canonicalization and correspondence algorithm

### 5.1 Semantic minimum

For each reconstructed component:

1. build the typed oriented incidence graph with public-semantic colors only;
2. run full-signature refinement;
3. individualize/refine every admitted unresolved branch or apply only proof-producing safe prunes;
4. encode complete public component bytes at every leaf;
5. select the exact lexicographic minimum using full bytes; and
6. retain complete search evidence sufficient to establish that no smaller public block exists.

This stage determines canonical public component, vertex, and facet content. Detailed lineage and presentation facts cannot change it.

### 5.2 Winning equivalence class and orbits

Among all legal labelings producing the minimum public block:

1. construct the orientation-preserving automorphism relation;
2. partition cleaned vertices, corners, edges, and triangles into induced correspondence orbits;
3. validate normalized artifact payloads inside each orbit;
4. split an orbit only when a reviewed presentation-independent semantic/artifact discriminator is genuinely unequal;
5. keep indistinguishable members in one correspondence equivalence class; and
6. produce a complete class/orbit certificate or fail `resource_limit`.

The production implementation may use generators, stabilizer-chain-like certificates, complete branch records, or another in-tree bounded representation. No external graph library is permitted. The certificate must let the independent verifier prove class completeness.

### 5.3 Concrete presentation correspondence

Choose one source-correct representative bijection only after semantic classes are fixed.

A permitted deterministic implementation may replay the exact source presentation and choose the least source-position tuple within each certified class for the concrete lookup map, provided:

- this choice is explicitly marked presentation-only;
- it is excluded from public content, semantic reports, publication status, and primary semantic failure identity;
- it is bound to the exact source replay;
- changing source presentation changes at most the presentation map/replay section; and
- all induced triangle, edge-use, component, and provenance mappings remain consistent.

Alternatively retain a set-valued class mapping and materialize a representative only for APIs that require a concrete lookup. Do not claim the representative is semantic canonical identity.

### 5.4 Global component ordering

Order distinct component public blocks by full public bytes. Where component blocks are identical:

- their exchange cannot alter public mesh content;
- class-level artifact/provenance records use invariant multisets and multiplicities;
- any concrete block-to-cleaned-component correspondence is presentation-only unless an invariant discriminator exists; and
- global offsets and facet blocks must remain internally consistent for the chosen concrete correspondence.

Do not use source operand, cleaned component ID, traversal discovery, action order, or worker completion as semantic ordering.

## 6. Dimensioned precision/report algorithm

Construct the exact set of reviewed records contributing to surviving output and required removal reports.

For every record:

1. validate owner, schema, role, metric, dimension, finiteness, interval/bound, derivation, and lineage;
2. preserve all three truth layers where referenced;
3. classify the value without converting roles;
4. require length before comparing with tolerance or a cleanup budget;
5. independently verify Component 03 aggregation parentage;
6. aggregate sequential and global quantities under the frozen outward-rounded rule;
7. choose deterministic witnesses by complete typed keys;
8. compute remaining margin only from policy-declared commensurate length quantities; and
9. fail any known precision, displacement, removal, or role violation.

Advisory handoff bounds may be reported but never participate as committed cost unless a separate Component 13 action record independently computes and commits an equal value.

## 7. Candidate and transaction amendments

Insert these checks into the original commit protocol:

1. finalize semantic public minima;
2. finalize complete orbit/equivalence-class certificates;
3. construct checked canonical public positions;
4. construct and validate one concrete presentation correspondence;
5. build the private public mesh;
6. independently reconstruct public topology;
7. aggregate reviewed dimensioned reports;
8. build unique and class-level provenance;
9. encode semantic and presentation fields in separate prescribed domains;
10. construct the pending immutable candidate;
11. run the independent Component 14 verifier, including class and role checks;
12. finalize digests/replay references;
13. reconcile resources and join all workers;
14. poll cancellation; and
15. atomically publish.

Failure at any point destroys the private mesh/candidate, releases reservations, preserves predecessor artifacts, and exposes no ordinary result.

## 8. Error and diagnostic implementation amendments

Extend `OutputAssemblyTypes.h`, Component 01's Component 14 subcode registry, and diagnostics with stable records for:

- predecessor `T,I` mismatch;
- reviewed schema incompatibility;
- missing/conflated truth layer;
- unknown or wrong physical dimension;
- incompatible metric aggregation;
- invalid derivation/denominator;
- advisory-role promotion;
- nonzero Component 12 cleanup budget;
- semantic coloring contaminated by presentation data;
- incomplete orbit/class certificate;
- class member/range/incidence mismatch;
- concrete map outside permitted class;
- fabricated unique rank;
- false unique provenance;
- semantic/presentation codec contamination; and
- reviewed Component 15 handoff mismatch.

Diagnostics include semantic component/class keys, presentation references where relevant, role/metric/dimension, bounds, derivation, truth layers, expected and observed class/map facts, versions, resource counters, deterministic failure key, and replay identity.

Do not use pointer values, branch visitation order, worker IDs, hash buckets, wall-clock time, or exception text as authoritative diagnostics.

## 9. Test file amendments

Add or extend:

- `TestOutputAssemblyPreflight.cc` for reviewed schemas and `T,I`;
- `TestOutputPrecisionReports.cc` for truth layers, dimensions, derivations, and roles;
- `TestOutputCanonicalOrdering.cc` for semantic/presentation separation;
- `TestOutputAutomorphisms.cc` for orbit/equivalence classes;
- `TestOutputEquivalenceMaps.cc` for concrete map membership and induced maps;
- `TestOutputProvenance.cc` for invariant class lineage versus unique attribution;
- `TestOutputLogicalSerialization.cc` for semantic/presentation domain separation;
- `TestOutputAssemblyMutation.cc` for reviewed producer-shaped mutations;
- `TestOutputAssemblyProperties.cc` for presentation representative metamorphics;
- `TestOutputAssemblyFuzzReplay.cc` for class-preserving shrinking/replay;
- `TestOutputAssemblyResourcesCancellation.cc` for certificate/map resources; and
- `OutputCanonicalExhaustiveOracle.h/.cc` for independent minimum and correspondence-class enumeration.

Add focused fixtures:

- symmetric components with identical normalized lineage;
- identical disconnected components;
- topology-distinct equal-coordinate vertices inside automorphism classes;
- one-bit or lineage differences that genuinely split an orbit;
- multiple source presentations of the same semantic cleaned manifold;
- valid and invalid dimensioned derivations;
- advisory and committed values with identical numeric bits but different roles; and
- mixed reviewed/obsolete artifacts.

## 10. Required review test gates

### 10.1 Schema and compatibility gate

All supported type/index combinations pass. Every cross-`I`, obsolete/reviewed mix, stale owner, wrong dependency digest, or unsupported Component 15 handoff fails before allocation.

### 10.2 Truth/dimension gate

Every required truth layer survives exact encoding and query. All wrong-dimension, wrong-metric, invalid-derivation, and role-conflation mutations fail even when counts and digests are repaired.

### 10.3 Advisory-cost gate

No Component 12 handoff value influences committed maxima or tolerance margin without an independent Component 13 action/ledger record. Numeric equality between advisory and committed fields does not merge roles.

### 10.4 Semantic canonicalization gate

Public mesh bytes, public-content digest, semantic topology report, and publication status are identical across cleaned-ID, source-presentation, traversal, branch, worker, allocator, and concrete correspondence permutations.

### 10.5 Correspondence-class gate

For bounded fixtures, exhaustive independent enumeration agrees with:

- the minimum public block;
- orientation-preserving automorphism classes;
- cleaned/public correspondence orbits;
- uniquely distinguishable members; and
- all permitted concrete bijections.

Every omitted/extra member, cross-orbit mapping, incidence violation, fabricated rank, or false uniqueness claim fails.

### 10.6 Provenance gate

Unique attribution is present only when invariantly justified. Exact classes preserve complete invariant shared lineage and normalized lineage multisets. Presentation replay remains exact without contaminating semantic bytes.

### 10.7 Codec/digest gate

Public-content bytes contain no presentation records. Artifact/evidence/provenance/replay domains contain all required reviewed fields in canonical order. Forced hash collisions never change minima, classes, maps, or verifier outcomes.

### 10.8 Resource/cancellation gate

Limit-minus-one, limit, and limit-plus-one tests cover orbit search, certificates, class tables, presentation maps, dimension tables, codecs, and independent verification. Cancellation joins all workers and publishes nothing.

## 11. Ordered implementation amendments

Integrate these review corrections into the original Section 25 sequence:

1. **Reviewed schemas and empty candidate.** Add versions, enums, class IDs, dimension/role tables, semantic/presentation codec skeletons, and pending-only empty candidate. Gate: empty reviewed candidate round-trips and rejects every old/mixed version.
2. **Reviewed predecessor preflight.** Validate `T,I`, truth layers, dimensions, derivations, handoff/action roles, zero predecessor budget, and resources. Gate: all contract mutations fail before allocation.
3. **Shared labeling-kernel contract correction.** Preserve semantic minima while exposing automorphism/orbit evidence without unique-original-node overclaim. Gate: Component 02 and generic graph goldens remain stable.
4. **Component 14 semantic graph/minimum.** Keep public-semantic colors and full minimum search. Gate: existing canonical public bytes remain stable.
5. **Orbit/equivalence-class certification.** Build complete class evidence and exhaustive bounded comparisons. Gate: every symmetry and branch-order fixture agrees with the oracle.
6. **Public permutations and concrete correspondence.** Build canonical public positions plus certified presentation maps and induced triangle/edge/component mappings. Gate: all map/class mutations fail.
7. **Private public mesh and topology reconstruction.** Retain the original exact copy/index/topology gates.
8. **Dimensioned reports and provenance.** Aggregate reviewed records and unique/class-level lineage. Gate: zero truth/dimension/role/provenance mutation survivors.
9. **Reviewed codecs and digests.** Encode semantic and presentation domains separately. Gate: exact goldens, collision, endian, truncation, and contamination tests pass.
10. **Round-trip and Component 15 handoff.** Expose reviewed queries and exact replay binding. Gate: Component 15 intake fixtures reconstruct all required evidence.
11. **Independent producer verifier.** Verify semantic minima, classes, maps, reports, provenance, codecs, and digests independently. Gate: zero required mutation survivors.
12. **Deterministic parallel integration and qualification.** Retain the serial semantic reference and complete the original type/index/platform/property/fuzz/resource/sanitizer/performance matrix.

Keep implementation commits reviewable and bisectable. Do not mark implementation complete or begin relying on Component 14 output in Component 15 until every original and amended gate passes.

## 12. Reviewed definition of done

The Component 14 implementation plan is fulfilled only when the original Section 26 requirements and all of the following are true:

- the reviewed Component 13 schema and embedded predecessor `T,I` descriptors are validated;
- all required `rounded_nominal`, `exact_relation`, and `uncertainty_enclosure` records remain distinct and queryable;
- every tolerance-facing value has a closed metric, physical dimension, derivation, and explicit role;
- Component 12 handoff evidence remains advisory and Component 13 action/ledger evidence solely determines committed usage;
- semantic public canonicalization is independent of detailed lineage and source presentation;
- exact automorphism classes produce complete orbit/correspondence certificates rather than fabricated unique ranks;
- canonical public positions and concrete cleaned/public maps are represented as distinct products;
- every concrete map is a total, topology-correct member of its permitted equivalence class;
- public semantic bytes, reports, failures, and status are invariant under changing only the concrete representative;
- provenance distinguishes invariant unique attribution, invariant class-level attribution, and presentation-only replay attribution;
- candidate, report, provenance, codec, replay, verifier, and Component 15 handoff versions reject obsolete/mixed evidence;
- independent verification reconstructs dimensions, roles, derivations, minima, classes, maps, reports, provenance, logical domains, and digests;
- `fv_surface_mesh<T,I>` remains a private-built carrier with optional fields empty and no hidden semantics;
- all original topology, duplicate-coordinate, index, orientation, precision, round-trip, pending-status, transaction, cancellation, determinism, and no-external-dependency requirements remain intact;
- zero required reviewed mutations survive; and
- all production and normative-test code is strict portable C++17 and self-contained within Ygor.
