# Component 3 implementation plan: exact arithmetic, constructions, and predicates

## 1. Scope and outcome

Implement the sole authority for every numerical or geometric decision in the robust B-rep Boolean pipeline. The component must interpret supported input coordinates as exact dyadic rationals, provide self-contained unbounded integer/rational arithmetic, expose exact geometry and rich semantic relations, optionally accelerate proven signs with certified filters, and provide query-local symbolic decisions without changing real geometry.

The completed component must provide:

- exact, bit-level decoding of every finite binary32/binary64 input, including signed zero and subnormals;
- canonical signed arbitrary-precision integers and normalized arbitrary-precision rationals with checked allocation;
- immutable exact scalar, point, vector, plane, line, ray, interval, box, parameter, and construction values;
- exact scalar/order, orientation, incidence, containment, intersection, ray-crossing, and polygon-relation APIs required by Components 2 and 4-13;
- exact affine, line-plane, coplanar-line, projection, and plane-plane constructions with immutable provenance and substitution checks;
- explicit unperturbed relations plus stable-ID lexicographic decisions only for generic probes, ownership, and decomposition ties;
- strict floating-point environment control and independently audited filters whose uncertainty always invokes exact fallback;
- Component 1 ownership, accounting, cancellation, error, diagnostics, trace, encoding, service, and replay integration;
- independent verification hooks, exact-only reference execution, and arithmetic/predicate/construction test suites.

Do not implement mesh validation, candidate enumeration, event discovery/interning, facet refinement, global arrangement, cell classification, Boolean selection, coordinate realization, or output assembly. Provide the semantic operations those stages consume. Later components must not inspect approximate determinants, rounded constructions, expansion terms, or interval endpoints to make topology decisions.

All implementation must be in-tree, portable C++17, and free of external arithmetic, geometry, serialization, and test dependencies. Do not use `src/YgorMeshesBoolean{,2,3,4,5}.{h,cc}`; the broad plan explicitly excludes them.

## 2. Existing Ygor assessment and reuse boundary

### 2.1 Reuse and adapt

- Accept `vec2<T>` and `vec3<T>` from `src/YgorMath.h` only at checked source-conversion boundaries. Keep exact geometry in new immutable types; do not inherit the legacy vector ordering, normalization, length, angle, or rounded arithmetic behavior.
- Preserve the existing mesh specialization matrix: source coordinate `T` is exactly `float` or `double`; mesh index type remains a Component 1/2 concern. Exact values are independent of `T` after decoding.
- Reuse the right-handed projection precedent visible in `src/YgorMeshesVerification.cc`: `drop_x -> (y,z)`, `drop_y -> (z,x)`, and `drop_z -> (x,y)`. This agrees with Component 2's exact dominant-normal-axis selection and avoids an orientation reversal for the Y chart.
- Reuse algorithmic formulations only after replacing every decision: three-projection 3D collinearity, orientation-based triangle inclusion, the division-free ray-crossing structure in `src/YgorMathMonotoneDecomposition.cc`, and canonical undirected-edge/ordered-container patterns in existing triangulation code.
- Implement Component 1's `exact_kernel_services<T>` in `exact_kernel<T>` and use its `status_or`, structured errors, canonical encoder, digest wrapper, owner token, accounting allocator, resource accountant, cancellation token, diagnostics, and tracing. Component 1 is a production prerequisite because these source interfaces are planned but not yet present.
- Preserve existing public orientation conventions where they are already meaningful, but freeze each new kernel formula independently in Section 8 and test it with analytic coordinates. Do not route new APIs through legacy `orient_sign` wrappers.

### 2.2 Quarantine or reject

- Treat `src/YgorMeshesAdaptivePredicates.{h,cc}` as unaudited reference/filter-candidate code, not an exact fallback. Its `orient3d_adaptive` first rounds coordinate differences, its in-sphere path rounds differences and lifts, splitter multiplication can overflow, subnormal behavior is uncontrolled, `compress` only removes zeros, sign-bearing expansion invariants are not established, and the error constants are not tied to a checked arithmetic DAG.
- Do not alter the old adaptive API as part of this component unless an unrelated compatibility requirement is established. Implement private new filters after the exact-only kernel passes. New Boolean code must not include or call the old API.
- Do not use generic `point_on_*`, segment intersection, point-in-polygon, `line<T>`, `line_segment<T>`, or `plane<T>` operations from `YgorMath`. They normalize, divide, use epsilon bounds, or collapse distinct boundary relations.
- Do not reuse rounded lifts from Delaunay code, `long double` signed-area decisions, BSP midpoint/epsilon fallbacks, or convex-hull coordinate perturbation.
- Do not use `YgorSerialize` for exact values; its native representation and endianness behavior are not canonical.
- Never call `YLOGERR`, which can terminate the process. Expected failures return Component 1 errors. Optional `YLOGDEBUG`/`YLOGINFO`/`YLOGWARN` forwarding occurs only after deterministic diagnostic publication.

## 3. Files, namespace, and build integration

Add these top-level installed/public files:

- `src/YgorMeshesExactArithmetic.h`: immutable number types, exact source-bit descriptors, canonical comparison/hash declarations, and checked arithmetic interfaces.
- `src/YgorMeshesExactArithmetic.cc`: limb arithmetic, division, GCD, rational normalization, binary32/binary64 decoding, encoding adapters, and explicit source-type instantiations.
- `src/YgorMeshesExactKernel.h`: exact geometry values, construction/provenance types, projection and relation enums, semantic predicate interfaces, symbolic-query types, and `exact_kernel<T>`.
- `src/YgorMeshesExactKernel.cc`: exact geometry algorithms, constructions, relation normalization, symbolic decisions, policy metadata, and explicit `float`/`double` kernel instantiations.
- `src/YgorMeshesExactKernelFilters.cc`: private certified filters, floating-environment guard, proof-version metadata, and test-only force-path controls. Do not add an installed filter-internal header.

Add tests:

- `tests/Test_MeshesExactArithmetic.cc`: arithmetic, decoding, canonical encoding, accounting, cancellation, and fault tests.
- `tests/Test_MeshesExactKernel.cc`: exact geometry, construction, relation, symbolic-query, service, and error tests.
- `tests/Test_MeshesExactKernelProperties.cc`: deterministic generated identities, adversarial bit patterns, filter differential tests, and schedule/cache tests.
- `tests/MeshBooleanExactKernelFixtures.h`: test-only bit constructors, exact-value builders, checked bounded oracles, known-answer vectors, deterministic PRNG, replay records, and relation generators.

Use namespace `ygor::mesh_boolean`. Keep implementation helpers in `.cc` unnamed namespaces or a non-installed private source directory. Public templates must be thin dispatch/conversion wrappers; substantial algorithms belong in `.cc` files with explicit `float` and `double` instantiations.

Update integration established by Component 1:

- Add all three Component 3 `.cc` files to the named strict-arithmetic source list in `src/CMakeLists.txt`. Apply strict options after repository-wide flags: GCC `-fno-fast-math -frounding-math -ffp-contract=off`; Clang `-fno-fast-math -ffp-model=strict -ffp-contract=off` when supported.
- Add compile-time rejection of `__FAST_MATH__` and positive `__FINITE_MATH_ONLY__` in filter/kernel translation units. Record and validate `FLT_EVAL_METHOD`; disable filters when evaluation width is outside a filter proof.
- Add all three standalone tests to `tests/compile.sh` and the Component 1 `tests/CMakeLists.txt`. Register `MeshExactArithmetic.Unit`, `MeshExactKernel.Unit`, and `MeshExactKernel.Properties`, link only in-tree Ygor and `Threads::Threads`, and label them `mesh_boolean;component3`.
- Run named CTests authoritatively without `|| true` in GCC/Clang Debug and Release CI. Add ASan/UBSan arithmetic/decoder runs and TSan shared-service/cache runs. Normative tests must not use the network-fetched doctest path.

## 4. Invocation and service contract

### 4.1 Request scope

Every fallible operation receives an invocation-bound request/view containing:

```cpp
struct kernel_request {
    context_owner_token owner;
    boolean_stage stage;
    canonical_work_key work_key;
    kernel_accounting_facade &accounting;
    cancellation_token cancellation;
    kernel_trace_sink *trace; // nullable, observational
    predicate_execution_policy execution;
};

enum class predicate_execution_policy : std::uint8_t {
    automatic,
    exact_only,
    force_filter_attempt,
    force_exact_fallback
};
```

`kernel_accounting_facade` draws only from the calling task's Component 1 pre-granted private byte/work envelope. Kernel workers may not reserve directly from the global accountant. Before dispatch, each public operation computes a checked conservative upper bound from formula ID, operand bit lengths, ring/edge counts, and requested output taxonomy for all result limbs, scratch, components, and worst-case baseline work; Component 1 grants that complete envelope in canonical work-key order. Schoolbook multiplication, Knuth division, binary GCD, overlay pair enumeration, and symbolic polynomial formulas all have explicit finite bit/count complexity bounds. If a bound is not representable or cannot be granted, fail before dispatch; never suspend/restart partially accounted arithmetic. Unused envelope capacity returns at operation/round completion. A future resumable algorithm requires a separately specified continuation/result protocol and is not part of schema version 1. Single-threaded callers use the same envelope and key protocol.

Only tests and exhaustive verification may request a forced mode. A context freezes the production mode to `automatic`. Semantic results, exact values, canonical evidence, and construction bytes must be identical under every mode that succeeds; filter-path counters belong only in nonsemantic trace evidence.

Every operation that can allocate, normalize, exceed work/bit limits, observe cancellation, or detect an invalid precondition returns `status_or<Value>`. Trivial read-only sign/access operations over an already canonical value may be nonthrowing direct methods. Catch `std::bad_alloc` and unexpected exceptions at service boundaries using Component 1's required conversion rules.

### 4.2 Service object

`exact_kernel<T>` must implement `exact_kernel_services<T>`, be immutable and thread-safe after checked construction, and expose:

- coordinate type tag and checked production downcast/type tag;
- stable kernel implementation and arithmetic-policy schema versions;
- canonical arithmetic-policy bytes/digest;
- factories for exact source decoding and invocation-owned construction storage;
- semantic arithmetic, predicate, construction, relation, approximation, and symbolic APIs;
- exact-only verifier entry points that never call filters;
- test instrumentation installed only through an explicit test seam.

The policy record must freeze limb width, integer division/GCD algorithm versions, rational normalization policy, determinant formula versions, each enabled filter/proof version, floating-environment policy, construction schema, and symbolic-policy version. A change that can alter canonical bytes, resource precedence, filter assumptions, or tie semantics increments the corresponding version and changes the policy digest.

Do not use global mutable caches. Initially use request/transaction-owned caches. A later shared cache must be owner-bound and synchronized, may affect performance only, and cannot affect iteration order, canonical output, diagnostics, resource-failure selection, or lifetime.

Exact-value storage is immutable and reference-counted after checked creation: a nonzero `big_uint` points to an accountant-owned immutable limb representation, while canonical zero uses an allocation-free empty representation. Copying `big_uint`, `big_int`, rationals, points, and geometry records copies immutable storage references and does not allocate; arithmetic creates a new checked representation through `kernel_request`. The representation retains the accounting owner and charge until its final published/private owner releases it. Transaction publication transfers the already-reserved representation charge from private to committed accounting without copying limbs. Public constructors that could allocate are unavailable; use checked factories, while moves and ordinary copies of accepted values are nonthrowing.

## 5. Exact number system

### 5.1 Unsigned magnitude

Implement `big_uint` over little-endian `std::uint32_t` limbs stored with Component 1's accounting allocator. The fixed baseline representation and invariants are:

- zero has no limbs;
- every nonzero value has a nonzero most-significant limb;
- no mutable limb view is public;
- limb count, capacity growth, byte multiplication, and host-size conversions are checked before reservation/allocation;
- comparison and equality inspect canonical limbs; hash and encoding use the mathematical value, not limb object bytes or capacity.
- immutable representation/control-block creation is charged as one checked allocation operation; copies share it and cannot mutate its limbs.

Required checked operations are compare, bit length, trailing-zero count, power-of-two recognition, fixed-width conversion, add, `a-b` with checked `a>=b`, left/right shifts, multiply, quotient/remainder, and GCD. Use schoolbook multiplication with `uint64_t` products/carries, single-limb division, normalized Knuth Algorithm D for multi-limb division, and binary GCD in schema version 1. Check exact division and normalization assumptions in Release as well as assertions.

Do not add Karatsuba, Toom-Cook, reciprocal division, or other complexity until a separately versioned implementation is byte/result/resource-differential-tested against the baseline. Unlimited correctness mode has no fixed limb cap; only checked host representability, allocation failure, caller limits, or cancellation may stop it.

### 5.2 Signed integer

Implement `big_int` as canonical sign plus `big_uint` magnitude:

```cpp
enum class integer_sign : std::int8_t { negative = -1, zero = 0, positive = 1 };
```

Zero always has sign `zero` and an empty magnitude. Nonzero sign agrees with a nonzero magnitude. Negative zero cannot be constructed or decoded. Implement comparison, negation, absolute value, signed add/subtract/multiply, exact divisibility, quotient/remainder with explicitly documented truncation toward zero, shifts, and canonical encoding.

### 5.3 Rational

Implement `exact_rational` as a `big_int` numerator and positive `big_uint` denominator. Fix schema version 1 to eager normalization at every public constructor and escaped arithmetic result; unreduced temporaries may exist only inside one operation.

Invariants:

- denominator is nonzero and mathematically positive;
- zero is exactly `0/1`;
- `gcd(abs(numerator), denominator) == 1`;
- structural equality is mathematical equality;
- equal values compare equal, encode identically, and hash consistently.

Implement sign, compare, absolute value, negation, add/subtract, multiply, divide, nonnegative integer powers, exact integer test, floor, ceil, and truncation. Addition/subtraction first use the denominator GCD; multiplication/division cross-cancel before forming products. Comparison handles signs then compares reduced cross-products without fixed-width overflow. Division by zero and a zero denominator are invalid kernel preconditions, never geometric degeneracies.

General exact comparison may allocate scratch and is therefore a fallible semantic operation. No `std::sort` comparator may call it. Before any normalized exact value becomes part of a Component 1 key, gather all values in canonical source-work order and assign an immutable `exact_order_rank` in a fallible pre-ranking pass: repeatedly select the least remaining equivalence class using checked exact comparisons, merge only proven-equal values, and assign dense ranks. This O(n^2) reference method is deterministic and permits error propagation; a faster ranking algorithm requires differential proof. Normalized keys store ranks plus canonical tie fields, and their `bool noexcept` comparators inspect only ranks/fixed data. Apply the same pre-ranking to rational parameters, points, carriers, intervals, relation components, and construction values before canonical sorting/publication.

Provide an internal dyadic construction fast path so decoded coordinates do not perform unnecessary general GCD work, but publish the same canonical rational. The mandatory Component 11-facing conversion helpers below never control Component 3 topology.

Make target conversion mandatory for Component 11. Provide software-defined `round_binary_nearest_even<T>(exact_rational)`, `round_binary_down<T>`, `round_binary_up<T>`, `compare_binary_bits<T>(coordinate_bits<T>, exact_rational)`, `predecessor_bits<T>`, `successor_bits<T>`, and deterministic radius-bounded neighboring-bit enumeration. Implement them with integer quotient/remainder and bit assembly, independent of casts and ambient rounding. Freeze overflow as no finite candidate (or signed infinity only in a non-output diagnostic result), preserve exact subnormals, choose `+0` for exact zero unless source-provenance preservation is explicitly requested, implement ties-to-even on the retained significand bit, and return exact comparison evidence for every candidate. Component 11 may emit only finite candidates and must certify them separately.

### 5.4 Exact source decoding

Define `coordinate_bits<float>`/`coordinate_bits<double>` and a decoded record with sign, unsigned significand, binary exponent, and closed category enum. Extract bits with `std::memcpy` to `uint32_t`/`uint64_t`; do not use unions, decimal conversion, `frexp`, native serialization, or conversion through a wider floating type.

For finite nonzero inputs:

1. For a normal value, restore the hidden significand bit and set `exponent2 = unbiased_exponent - fraction_bits`.
2. For a subnormal, use the fraction field directly and set `exponent2 = minimum_normal_exponent - fraction_bits`.
3. Remove all trailing powers of two from the significand and add that count to `exponent2`.
4. Construct `significand * 2^exponent2`, placing the power of two in the numerator or denominator.

Decode `+0` and `-0` to the same exact `0/1`, while retaining distinct source bits in provenance and Component 1 input encoding. Reject either infinity and every NaN payload with a structured non-finite/unsupported input failure at the requesting stage. Kernel setup verifies exact binary32/binary64 platform facts before any decode.

## 6. Exact geometry and canonical carriers

### 6.1 Value types

Provide immutable, owner-independent mathematical values:

```cpp
using exact_scalar = exact_rational;
struct exact_point2  { exact_scalar x, y; };
struct exact_vector2 { exact_scalar x, y; };
struct exact_point3  { exact_scalar x, y, z; };
struct exact_vector3 { exact_scalar x, y, z; };
enum class projection_axis : std::uint8_t { drop_x, drop_y, drop_z };
```

Also provide exact closed/open intervals, inclusive 2D/3D boxes, directed segments, rays, affine parameters, planes, and lines. Zero-length segments are representable as points for relation APIs that document that case; constructors requiring a carrier direction reject them.

Implement checked coordinate arithmetic, dot/cross products, determinants, lexicographic X/Y/Z comparison, coordinate extrema, and inclusive exact bounds. Do not expose a normalization-by-length operation.

### 6.2 Planes

Represent an exact plane by primitive integer coefficients plus orientation parity:

```cpp
struct exact_plane3 {
    big_int a, b, c, d;          // canonical unoriented carrier
    orientation_parity oriented; // relation to establishing ordered triple
};
```

Construct from rational points by clearing positive denominators, dividing `(a,b,c,d)` by their common positive GCD, and choosing carrier sign so the first nonzero coefficient in X, Y, Z, D order is positive. Store separately whether the ordered establishing triple's right-hand normal agrees with or opposes that carrier. Require `(a,b,c) != 0` and a noncollinear establishing triple.

Define `oriented_eval(p) = parity_sign * (a*p.x + b*p.y + c*p.z + d)`, where `parity_sign` is `+1` for agreement and `-1` for opposition. Swapping any two establishing-triple points toggles parity; an even permutation preserves it; explicit plane reversal toggles parity without changing carrier coefficients. All side, projection-parity, and reversal tests use this formula.

Expose oriented equality, unoriented carrier equality, side classification, same/opposite oriented coincidence, and exact absolute-normal-component comparison. Dominant projection drops the largest exact `abs(a/b/c)`, breaking ties X, then Y, then Z. Projection maps are fixed as `drop_x:(y,z)`, `drop_y:(z,x)`, and `drop_z:(x,y)` and return their parity relative to the oriented plane.

### 6.3 Lines, rays, parameters, and intervals

Represent a line as an exact anchor and nonzero, non-unit direction. Canonical line equality/order uses a primitive integer direction with first nonzero component positive and a canonical pivot point obtained by setting the first solvable coordinate to zero and solving exactly. Preserve a separate direction parity for directed use. A ray uses a directed anchor/direction and closed parameter domain `[0,+infinity)`; a segment uses endpoints and `[0,1]`.

Extract a point's line/edge parameter using the first nonzero direction component in X/Y/Z order and prove all remaining coordinates agree. Parameter comparison is exact. Exact intervals carry open/closed endpoint flags, normalize endpoint order without losing direction provenance, and distinguish empty, point, and positive-length intervals.

## 7. Constructions, handles, and provenance

### 7.1 Construction storage

Separate mathematical value from derivation. Define a closed `construction_opcode` covering at least original point, affine interpolation, projected point, support plane, line-plane intersection, coplanar line intersection, plane-plane carrier, barycenter, and symbolic probe descriptor.

An invocation-owned `exact_handle<Value>` refers to immutable storage, keeps that storage alive, and carries the Component 1 owner token. Each construction node stores:

- eagerly evaluated canonical exact value;
- opcode and formula/schema version;
- ordered child handles where argument order affects orientation/parameterization;
- sorted/deduplicated stable `feature_ref`s where source order is irrelevant;
- exact parameters and source coordinate-bit references;
- substitution evidence naming every defining equation/incidence.

Reject cross-owner handles. Published artifacts own any construction storage they reference; no request scratch, raw mesh pointer, or evictable cache reference may escape. Construction equality is exact-value equality, not handle or expression identity. Hash collisions require full canonical value comparison. Preserve distinct valid derivations so Component 6 can merge mathematical entities while retaining all provenance.

Canonical value encoding excludes owner token, pointer, cache state, approximation, filter path, and allocator details. Provenance encoding includes opcode, ordered children, normalized feature sources, exact parameters, exact result, and formula version, but provenance equality is not a proof of mathematical value equality.

Construction storage is an acyclic DAG. A draft may reference only original leaves or already-frozen children, so cycles are impossible by construction and checked again during freeze. Before publication, recursively compute each node's normalized full-content key from opcode/version, child content keys in semantic order, normalized feature sources, parameters, result, and evidence. Fallibly pre-rank child keys, then assign dense `construction_node_id`s in child-before-parent topological layers using Component 1's canonical factory; equal full-content nodes merge only after exact comparison. Canonical provenance encodes child node IDs from this canonical DAG, never handles, pointers, hashes, or insertion order. Different derivations of the same value retain different nodes unless their full provenance also matches.

### 7.2 Required constructions

Implement:

- exact affine interpolation `a + t*(b-a)` and exact barycenters/witness points;
- support plane through an ordered noncollinear triple;
- exact projection and chart parity;
- line-plane intersection with `t = -(n dot p + d)/(n dot v)`;
- coplanar 2D line-line intersection using the exact 2x2 direction determinant;
- plane-plane relation and unique intersection carrier, with direction `n1 cross n2` and canonical point found by setting the first usable coordinate to zero and solving the remaining exact 2x2 system;
- exact edge/ray parameter extraction and affine point reconstruction;
- exact coordinate extrema and conservative approximations/bounds.

Every construction classifies preconditions before division. Line-plane distinguishes unique, parallel-disjoint, and contained. Coplanar lines distinguish unique, parallel-disjoint, and coincident. Planes distinguish nonparallel, parallel-disjoint, and coincident with same/opposite orientation. Substitute every successful result into all defining carriers immediately in mandatory-check builds and in the independent verifier; disagreement is `internal_invariant_error`.

## 8. Predicate result model and sign conventions

### 8.1 Results and evidence

Define `exact_sign { negative=-1, zero=0, positive=1 }`, stable formula IDs, and `predicate_result<Relation>`. Each successful result includes filter-independent `predicate_evidence` containing formula/version, canonical operands or stable canonical artifact references, exact boundary/construction witnesses needed to replay the relation, and stable source features.

`predicate_evidence` and analogous `construction_evidence` are canonical artifact payload when the producing component retains them, or canonical verification-report evidence when only the verifier retains them. They are data records, not Component 1's process-local `verification_certificate`, and never authorize publication. Owner-bearing ephemeral references are resolved to canonical value operands or stable artifact IDs before encoding. Filter-attempt/accept/fallback state, floating bound, and operation counters are charged trace facts only; they are excluded from semantic evidence and artifact bytes. An independent verifier recomputes evidence in exact-only mode. A zero sign means exact algebraic zero, never uncertainty.

### 8.2 Frozen determinant conventions

- `orient2d(a,b,c)` is the sign of `(b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x)`; positive is counter-clockwise in the right-handed chart.
- `orient3d(a,b,c,d)` is the sign of `dot(b-a, cross(c-a,d-a))`; positive means `(a,b,c,d)` has positive right-handed oriented volume. Do not inherit the opposite sign of any legacy wrapper accidentally.
- Plane side is the sign of the oriented coefficients evaluated at the point; positive is the front/normal side and negative is the back side, agreeing with Component 1's facet convention.
- Exact signed polygon area is one half of the shoelace sum in its chart. Return the doubled rational when callers only need sign/comparison, and expose the normalized area explicitly where required.

Document antisymmetry/permutation laws beside each API. Evaluate rational determinant signs by clearing known-positive denominators and operating on integers. Use direct formulas for dimensions 2/3; if a larger determinant is later needed, use checked fraction-free Bareiss elimination and version the formula. Do not implement in-sphere in schema version 1. Implement exact in-circle only if a selected Component 7 deterministic triangulation needs it; Component 2's planned ear clipping does not.

## 9. Semantic predicates and relation taxonomy

### 9.1 Scalar, direction, and carrier operations

Provide exact scalar sign/compare, rational/parameter order, point lexicographic order, absolute-component compare, dot/cross sign, collinearity/coplanarity, vector equality/opposition/parallelism, plane relation, and canonical direction/angular order.

Angular order partitions a 2D direction by exact upper/lower half-plane, compares directions by `orient2d(0,u,v)`, then identifies equal or opposite collinear directions explicitly. A caller-provided stable-ID tie is used only after exact direction equality and only when the algorithm needs a total decomposition order.

### 9.2 Point classifications

Return rich enums/records, never inclusive booleans:

- point/line: off carrier or on carrier;
- point/segment: off carrier, before origin, at origin, open interior, at destination, after destination;
- point/plane: negative side, on plane, positive side;
- point/triangle or simple polygon: outside, open interior, boundary vertex, boundary edge interior, with exact feature index/ID;
- point/polygon-with-holes: outside region, open region interior, outer-boundary feature, or hole-boundary feature.

Point-on-segment first proves collinearity then compares the exact canonical parameter. Point-in-triangle first proves plane incidence, projects by its recorded chart, and evaluates oriented edge signs. Point-in-simple-polygon first checks exact boundary features, then uses a division-free winding/half-open crossing rule whose vertex ownership is fixed and tested; malformed/non-simple rings are invalid API preconditions rather than silently interpreted.

For polygon-with-holes input, require one simple outer ring and zero or more simple, pairwise boundary-disjoint hole rings in one exact chart. The outer ring and holes have explicit semantic tags; ring orientation may be validated or normalized in a checked preparation step but never inferred from approximate area. Prove every hole is strictly inside the outer ring and holes are mutually nonnested/disjoint unless the caller supplies an already verified region token. Classification checks every ring boundary first and returns all coincident source incidences, then computes `inside_outer && !inside_any_hole`. Boundary records distinguish outer vertex/edge from hole vertex/edge and preserve ring/edge IDs.

### 9.3 Segment/segment

Return a structured 2D/3D relation containing:

- carrier relation: skew, nonparallel coplanar, parallel distinct, or collinear;
- intersection dimension: empty, point, or positive-length segment;
- point kind: proper interior/interior crossing, endpoint/endpoint coincidence, or endpoint/interior T-junction with both endpoint identities;
- overlap kind: partial overlap, first contains second, second contains first, equal same direction, or equal opposite direction;
- exact intersection point or normalized overlap endpoints;
- exact parameters/intervals on both directed segments and complete endpoint-incidence flags.

For 2D, use four exact orientations and collinear parameter intervals. For 3D, first classify direction cross product and coplanarity, select a nonzero 2D minor for exact line parameters, and verify the constructed point in all coordinates. Explicitly classify zero-length point-segments where the API permits them; carrier-only overloads reject zero direction.

### 9.4 Segment/plane and segment/polygon

Segment/plane returns strictly positive, strictly negative, proper crossing, origin on plane, destination on plane, or entirely contained, plus the exact crossing parameter/point when unique.

Segment/simple-polygon combines support-plane classification with exact projected polygon relations. For a transverse segment, classify the unique plane point against the polygon. For a coplanar segment, collect all exact boundary-contact parameters, intern equal values by exact comparison, sort them, classify every endpoint and each open interval using an exact affine midpoint, and return a canonical vector of disjoint point/interval components. Concave polygons may produce multiple interior intervals; never assume one component.

The polygon-with-holes overload applies the same split/classify process against every outer/hole edge. Every point/interval component is tagged `open_region_interior`, `outer_boundary`, `hole_boundary`, or `outside_contact` and carries all coincident source-edge incidences and same/opposite direction multiplicities. Adjacent equal-tag intervals merge only after proving there is no intervening vertex category that downstream arrangements must preserve.

### 9.5 Polygon/polygon

Return an aggregate relation plus a canonical vector of connected components. The taxonomy must distinguish:

- disjoint;
- isolated vertex/vertex, vertex/edge, and vertex/interior contacts;
- proper transverse point/segment crossings;
- shared boundary subsegments and their same/opposite direction;
- coplanar boundary crossing;
- coplanar strict containment without boundary crossing;
- positive-area overlap;
- exact equality with same or opposite orientation;
- multiple disconnected components for concave source polygons.

Point/curve components record exact point/carrier/interval geometry, source-feature incidences, exact parameters on every participating edge, orientation, and evidence. A positive-area coplanar component instead stores an `exact_planar_region`: canonical support plane/chart, one oriented atomic outer cycle, zero or more oppositely oriented hole cycles, exact nonzero signed area, one exact open-interior witness, and per-halfedge source-edge incidence/multiplicity. Cycles use exact vertices and split source-edge parameters, begin at their least pre-ranked directed vertex/edge key, and are sorted outer-before-holes by containment and exact order ranks.

For nonparallel support planes, construct the plane-plane carrier, intersect each polygon with it, then intersect the canonical carrier intervals. For coplanar polygons, build a planar overlay graph by exhaustively intersecting all source edges, splitting them at pre-ranked exact parameters, merging equal exact vertices, and creating paired directed atomic halfedges with complete A/B boundary incidences. Sort outgoing halfedges by exact angular order, walk every left face, discard the unbounded face, and classify an exact interior witness of each bounded face against both source regions. Retain faces selected by `inside_A && inside_B` as overlap atoms; merge adjacent selected atoms only across an atomic edge proven internal to the overlap, extract outer/hole boundary cycles, assign holes to the least containing outer cycle by exact point classification, and verify area/cycle coverage. Equality, containment, positive-area overlap, and disconnected overlap are derived from these regions plus boundary components. If certified facet triangulations accelerate work, reconcile and remove all artificial diagonal events before returning a source-polygon relation.

Sort components by canonical exact carrier, parameter interval, relation enum, and stable source feature IDs. Hash iteration or triangle discovery order must not affect results.

### 9.6 Ray crossing and point location primitives

Ray/triangle and ray/polygon relations distinguish miss, proper transverse open-interior crossing, boundary-edge hit, boundary-vertex hit, ray origin on boundary, coplanar disjoint, and coplanar overlap. Return the unperturbed relation and exact ray parameter first.

Avoid a Component 2 dependency cycle by defining Component-3-owned neutral views. `oriented_polygon3_view` contains exact support plane/chart, immutable exact outer/hole rings, orientation, exact bounds, stable facet/ring/vertex/directed-edge IDs, and a certified nonzero oriented triangulation whose triangles reference ring vertices and mark every internal diagonal nonsemantic. `oriented_shell_view` is a canonical range of those views plus shell identity, twin directed-edge pairs, facet adjacency, and for every vertex the complete cyclic incident edge/facet fan. The view constructor verifies IDs, ring/triangle coverage, twins, and fan closure before point location. Component 2 adapts transaction-private or validated facets to this view without the kernel including Component 2 types.

A shell point-location coordinator over the neutral view returns outside, inside, boundary vertex, boundary edge, or boundary facet with exact feature identity and crossing evidence. Bounds may reject only when exact or certified conservative. `ray_boundary_owner_v1` groups triangle hits through the supplied twin/fan topology, then reconciles the result to source polygon boundaries so internal triangulation diagonals cannot create ownership or events. Crossing parity/winding ownership for source edge/vertex hits is resolved by Section 11's symbolic API, not by moving the point or dropping an event.

## 10. Exact relation algorithms and normalization

Implement exact-only algorithms first and keep them permanently as the reference:

1. Normalize every input carrier and validate owner/preconditions before substantive work.
2. Charge a deterministic operation envelope and check cancellation before allocation or potentially large fallback.
3. Compute unperturbed signs/relations with integer/rational arithmetic.
4. Construct every unique intersection exactly and verify incidence by substitution.
5. Intern temporary equal parameters/points by canonical exact equality, never rounded coordinates or hash alone.
6. Complete all fallible exact comparisons in the Section 5.3 pre-ranking pass; normalize point/interval/component orientation and sort with complete `noexcept` keys containing only immutable ranks and fixed fields.
7. Run cheap relation invariants before returning: symmetry mapping, parameter-domain membership, disjoint component interiors, and aggregate/component consistency.

For segment/polygon and polygon/polygon open-cell classification, use exact rational midpoints only after all boundary parameters are known. A midpoint is a proof witness, not an output coordinate. For unbounded rays choose exact rational parameters between consecutive finite events and a proven point after the final event using checked integer/rational construction.

Every relation API documents argument permutation behavior. Reversing a segment remaps `t -> 1-t` and swaps endpoint categories; swapping relation arguments swaps source incidences; reversing polygon orientation changes orientation metadata but not the un-oriented geometric intersection set. Tests must enforce these mappings exactly.

## 11. Symbolic perturbation and deterministic generic decisions

### 11.1 Separation from geometry

Never perturb stored coordinates. Define query-local formal lexicographic decisions:

```cpp
struct perturbation_key {
    perturbation_domain domain;
    accounting_vector<feature_ref> stable_features;
    std::uint32_t local_rank;
};

template<class ExactRelation, class TieDecision>
struct symbolic_result {
    ExactRelation unperturbed;
    optional<TieDecision> decision;
    perturbation_key key;
};
```

Always evaluate and retain the unperturbed exact relation. Invoke symbolic logic only when that relation is degenerate and the caller explicitly requests a generic ray/probe, half-open boundary owner, decomposition diagonal/order, radial sheet order, or open-side query. It must not change incidence, coincidence, overlap, containment boundary, set membership, or event discovery.

Normalize perturbation feature lists and derive infinitesimal rank solely from Component 1 strong IDs and explicit role/domain tags. Raw input ordinals, pointers, object/hash addresses, allocation/iteration order, thread completion, and approximate coordinate values are forbidden tie inputs.

Freeze the perturbation-key order as `(domain numeric tag, canonical feature_ref count and encodings, local_rank)`. Feature references are sorted/deduplicated unless the formula declares oriented argument positions, in which case each position tag precedes its reference. `local_rank` identifies a documented logical candidate within one feature tuple, starts at zero, and the query builder rejects duplicate complete keys; it is not an escape hatch for insertion order. Fallibly pre-rank all complete keys, then assign infinitesimal exponents `r=1..n` by that order. Reversal/permutation creates a new explicitly position-tagged key and follows the formula mappings below.

Evaluate a symbolic polynomial as formal ordered infinitesimals and choose the sign of the first nonzero exact coefficient; never instantiate a numeric epsilon. Record formula, coefficient order, stable IDs, exact zero coefficients, and first nonzero coefficient as replayable evidence.

Use sparse polynomials `sum(c_k * epsilon^k)` with exact-rational coefficients, nonnegative checked integer exponents, lexicographic significance from the smallest exponent, and canonical combination/removal of equal/zero terms. Addition and the bounded products/determinants needed below are checked and accounted; no general symbolic algebra API is exposed. Each symbolic formula has a stable ID/version and an explicit coordinate substitution, so an implementer does not invent a perturbation per call.

Represent a symbolic ray parameter as a pair of sparse polynomials `(N,D)` rather than expanding a quotient. Remove their common power of `epsilon`, prove `D` is not identically zero, and normalize so the first nonzero coefficient of `D` is positive. Classify the forward ray domain from the first nonzero coefficient of `N` because `D>0` for sufficiently small positive `epsilon`. Compare `N1/D1` and `N2/D2` by the first nonzero coefficient of `N1*D2 - N2*D1`; exact zero means equal symbolic parameters and triggers hit grouping. An identically zero denominator is the unperturbed parallel/coplanar case: advance to the next finite primitive direction or, on the formal path, report an internal formula defect if a valid nonzero facet normal still yields zero after all direction coefficients. These rules are part of `generic_ray_v1` evidence and tests.

### 11.2 Required symbolic APIs

Provide narrowly scoped operations for:

- choosing a generic ray direction for point location;
- assigning half-open ownership to ray hits on a vertex or edge;
- selecting an open side `p +/- epsilon*n` of an oriented plane while preserving `p` as exactly on-plane;
- total ordering of exactly equal decomposition candidates;
- ordering incident noncoplanar sheets around an exact carrier when a probe direction itself is degenerate.

Freeze these schema-v1 formulas:

1. `generic_ray_v1`: try primitive integer directions in exactly this order: `(1,0,0)`, `(0,1,0)`, `(0,0,1)`, `(1,1,1)`, `(1,2,4)`, `(1,4,2)`, `(2,1,4)`, `(4,1,2)`, `(2,4,1)`, `(4,2,1)`. A direction is usable only when the complete unperturbed ray/facet pass has no origin, coplanar, edge, or vertex ambiguity. If none is usable, substitute the formal direction `d(epsilon)=(1, epsilon^r, epsilon^(2r))`, where `r` is the query key rank, into the same ray-plane determinants and ray parameters. Sort/group the resulting symbolic hit parameters by their first nonzero difference coefficient.
2. `ray_boundary_owner_v1`: for each equal symbolic hit group, evaluate the projected triangle/polygon half-open crossing rule with `d(epsilon)` and count exactly those atomic oriented triangles whose symbolic intersection lies in their open interior; paired edge triangles and complete vertex fans must contribute the same parity/winding as an arbitrarily small generic direction. The coordinator verifies each topological edge/vertex group contributes zero or one parity crossing (or the corresponding signed winding sum), never counts each incident facet independently, and retains all unperturbed boundary incidences in evidence.
3. `open_side_v1`: no coordinate polynomial is needed. Given exact `p` on oriented plane `P`, return `negative` for `p-epsilon*n_P` and `positive` for `p+epsilon*n_P`; reversing `P` swaps results. When classifying against another carrier `Q`, substitute `p + s*epsilon*n_P` into `oriented_eval_Q`, compare the constant coefficient first, then the signed linear coefficient; stable-key order is used only if both vanish because the carriers genuinely coincide, which remains reported as coincidence.
4. `equal_candidate_order_v1`: after every semantic exact key compares equal, order decomposition candidates by complete perturbation-key order. Reversing an oriented candidate toggles its position tag before comparison; this total order chooses work/decomposition only and cannot merge or separate geometry.
5. `carrier_radial_order_v1`: orient the carrier direction canonically, select the first axis from X,Y,Z whose cross product with it is nonzero, set `q=normalize_primitive(direction cross axis)` and `r=normalize_primitive(direction cross q)`, and map each incident oriented plane normal to exact coordinates `(dot(n,q), dot(n,r))`. Sort by the exact half-plane/orientation angular comparator. Equal angular vectors are coincident radial sheets and use `equal_candidate_order_v1` without changing their unperturbed coincidence. Reversing carrier direction reverses cyclic order while preserving the least canonical sheet as cycle start; reversing one plane adds half a turn.

For point location, different usable finite directions and the formal path must agree on the non-boundary inside/outside result; disagreement is an internal invariant failure. Boundary location remains boundary even if a symbolic adjacent-cell query is also requested. Unit tests must include golden coefficient sequences and reversal/permutation mappings for every formula.

## 12. Certified filters and floating-point safety

### 12.1 Baseline filter scope

The initial production filters are limited to `orient2d` and `orient3d`. All other predicates use exact arithmetic until profiling establishes a need and a separate proof/version/test suite exists. A filter returns only `certified_negative`, `certified_positive`, or `uncertain`; exact zero is established by the exact path unless a separately proved exact-zero shortcut uses bit/integer equality.

Implement a new private filter rather than wrapping the current adaptive result. For each filter:

1. Document the exact operation DAG, rounded-operation count, determinant convention, binary32/binary64 proof, and proof-version constant.
2. Validate strict compile flags, `FE_TONEAREST`, evaluation width, contraction-off behavior, IEC representation, and subnormal assumptions.
3. Inspect source exponents and bypass the filter if subtraction, scaling, product, sum, overflow, underflow, or non-normal intermediates leave the proof domain.
4. Apply a common exact power-of-two scale only when its effect and representability are proved; positive scaling preserves determinant sign.
5. Compute a rigorously derived absolute error bound for that precise DAG.
6. Accept only a finite result satisfying a strict `abs(det) > bound`; otherwise call exact fallback.

Do not claim expansion arithmetic is exact until error-free transforms, subtraction tails, nonoverlap/order, exponent-range scaling, zero elimination, and sign extraction have executable invariants. Expansion filters are optional; a simple static floating error filter plus rational fallback is acceptable and safer initially.

### 12.2 Environment and observational equivalence

Use an RAII guard around filter execution to save the thread floating environment, establish `FE_TONEAREST`, clear relevant exceptions, and restore the original environment on every return. If setup is unsupported, disable the filter and use exact arithmetic. Restore failure is an internal/platform failure because caller state cannot silently change. Integer/rational paths are independent of ambient rounding mode.

Filter acceptance cannot skip semantic precondition, owner, cancellation, or exact-result bit-limit checks. Define exact-number size limits over canonical mathematical operands/results, not whether temporary limbs happened to be allocated. Define filter scratch and exact-fallback scratch as stage-private bytes/work; under finite scratch/work budgets a forced path may fail differently, but any successful path must return identical semantic output and automatic mode must be deterministic under the frozen policy. Unlimited-mode force-filter/fallback differential tests are normative.

Never expose determinant magnitude or error bound to geometry callers. A filter/exact disagreement found in exhaustive verification is `internal_invariant_error`, cancels the stage, and records exact input bits and policy/proof versions.

## 13. Conservative approximations

Provide separately named, explicitly non-authoritative interval/approximation types only for broad-phase bounds, diagnostics, and realization search. An approximation carries outward-rounding proof/version, source exact-value reference, and flags for unbounded/overflowed conversion.

Exact-to-`T` enclosing interval conversion must use integer comparison and adjacent representable values, not ambient rounded casts alone. If a finite enclosure cannot be represented, return an unbounded conservative interval rather than exclude geometry. Approximate comparisons may prove strict separation only when intervals are disjoint; overlap/uncertainty invokes exact comparison. The API must not offer an implicit conversion from approximate sign/order to authoritative relation.

## 14. Canonical encoding, equality, and hashing

Use Component 1's canonical encoder and reject malformed/noncanonical decoding:

- `big_uint`: `u64` byte count followed by minimal unsigned big-endian magnitude bytes; zero has count zero and nonzero values forbid a leading zero byte.
- `big_int`: one sign byte plus canonical magnitude; negative zero is rejected.
- `exact_rational`: canonical signed numerator followed by canonical positive denominator; reject zero denominator, reducible forms, and zero not encoded as `0/1`.
- points/vectors: coordinates in X/Y/Z order;
- planes: primitive carrier coefficients followed by orientation parity;
- lines: canonical pivot/direction plus directed parity;
- intervals/parameters: domain and endpoint flags followed by rationals;
- relations/components: fixed one-byte enums, complete exact geometry, source incidences, and canonical vector order;
- provenance: opcode/version, ordered children, normalized sources, parameters, exact result, and substitution evidence.

Provide in-namespace lookup hashers over canonical mathematical encodings. Hash equality never proves value equality. Lazy internal caches may memoize a hash or encoded form, but synchronization and cache state cannot alter equality, encoding, ordering, diagnostics, or allocation/resource failure selected by canonical work order.

Frame the arithmetic-policy record as `YGBKER03` under Component 1's top-level grammar. Its fixed field order is implementation version, coordinate tag, limb/value-encoding versions, division/GCD versions, rational policy, determinant formula versions, enabled filter/proof versions, floating-environment policy, symbolic-policy version, and construction/provenance schema. Golden-byte tests make all primitive and policy encodings normative.

## 15. Resource accounting, cancellation, and failures

### 15.1 Accounting

Use Component 1 accounting allocators through the task-private `kernel_accounting_facade` for limbs, construction nodes, evidence, canonical bytes, relation vectors, caches, and verifier scratch. Never reserve from the global accountant inside a worker. Before mutation/allocation:

- check limb-count arithmetic and conversion to bytes;
- reserve conservative result and scratch envelopes in canonical request order;
- enforce `exact_number_bits` on every canonical operand/result and a documented bound for intermediates;
- cross-cancel rationals and remove powers of two before large multiplication;
- charge formula-specific work units for limb loops, GCD/division iterations, polygon pairs, relation components, and symbolic coefficients;
- transfer ownership only after successful construction; failure provides the strong guarantee and exposes no partial handle/vector/cache entry.

Unlimited correctness mode has no algorithmic iteration/limb cap. Host `size_t`/address-space overflow and `bad_alloc` remain structured resource failures, not permission to guess.

### 15.2 Cancellation

Check cancellation at every public entry, before filter fallback, before large allocation, at fixed documented intervals in limb/GCD/division loops and polygon pair scans, before construction storage insertion, and during verifier replay. Use a fixed interval such as every 1024 loop units, with the interval frozen in policy metadata if it affects deterministic work accounting.

Cancellation returns Component 1's `resource_limit`/`resource_kind::cancellation` at the requesting Boolean stage. It never returns an uncertain sign or partial relation.

### 15.3 Failure taxonomy and diagnostics

Expected kernel failures are limited to:

- unsupported binary representation/platform or non-finite source coordinate;
- configured memory, work, exact-number, diagnostic/trace, or cancellation limit;
- invalid API precondition such as zero denominator/direction, malformed polygon contract, wrong owner, or degenerate carrier construction;
- internal invariant defect such as noncanonical arithmetic, failed exact division, contradictory relation components, substitution failure, or filter/exact disagreement.

Exact degeneracy and near-degeneracy are successful relations. Errors include the requesting stage, stable formula/construction ID, sorted feature references, original source bits or canonical exact operands, requested/current/limit facts, policy digest, and replay metadata. Do not include pointer values, locale text, timestamps, thread IDs, or approximate decimal-only operands.

## 16. Independent verification

Expose exact-only read APIs so Component 13 can register independent checks without using filters or producer-private caches. The mandatory verifier must:

1. Validate canonical limb/sign/denominator/GCD invariants and owner/lifetime bindings.
2. Re-decode source bits and compare exact values, including signed-zero provenance.
3. Recompute predicate relations from canonical operands with exact-only formulas.
4. Recompute line/plane/parameter normalization and substitute every construction into all defining carriers.
5. Validate component sorting, parameter domains, symmetry mappings, and aggregate relation consistency.
6. Recompute formal symbolic coefficients and stable-ID order while retaining the unperturbed relation.
7. Re-encode values, provenance, evidence, and policy metadata and compare canonical bytes/digests.

Where practical, use a structurally different verification formula: direct cross multiplication versus cached parameters, an alternate determinant expansion/order, or exhaustive source-edge checks versus accelerated polygon decomposition. Verification resource exhaustion remains `resource_limit`; mismatch is `internal_invariant_error` and prevents publication.

Mutation tests must alter a limb/order/leading zero, integer sign, rational denominator/GCD, coordinate, plane coefficient/parity, line pivot/direction, parameter, construction child/opcode/result, relation enum/component, formula operand order, symbolic key/rank, owner token, policy digest, and predicate/construction evidence. Every mutation must fail in Release/NDEBUG before dependent artifact publication.

## 17. Implementation sequence

Implement in this order and retain each lower layer's standalone tests:

1. Reconcile Component 1 service, request, error, accounting, cancellation, canonical-encoding, and strict-source interfaces; freeze Component 3 policy/formula/relation schemas.
2. Implement canonical `big_uint`: compare, shifts, add/subtract, multiply, quotient/remainder, GCD, accounting, cancellation, and encoding.
3. Implement `big_int` and eagerly normalized `exact_rational`, including cross-cancellation and exact comparison.
4. Implement binary32/binary64 bit decoding, provenance, platform guards, and known-answer vectors.
5. Implement exact points/vectors/intervals/boxes, determinant helpers, scalar order, `orient2d`, and `orient3d` in exact-only mode.
6. Implement canonical construction DAG storage, owner-safe handles, evidence schemas, policy metadata/digest, and stage-local caches.
7. Implement canonical planes/lines/projections, affine interpolation, line-plane, coplanar-line, and plane-plane constructions with substitution evidence.
8. Implement point/carrier, point/segment, point/plane, point/triangle, and point/simple-region classifications.
9. Implement complete segment/segment, segment/plane, and segment/polygon relations.
10. Implement source-level polygon/polygon components, ray relations, and neutral-view shell point-location primitives.
11. Implement formal stable-ID symbolic query APIs and exact/symbolic evidence separation.
12. Implement and document strict `orient2d`/`orient3d` filters; preserve exact-only execution permanently.
13. Register independent verification adapters, mutation/fault tests, and Component 1 transaction integration.
14. Run generated, adversarial, force-path, schedule, compiler, Release, sanitizer, and platform qualification before allowing Component 2 production integration.

## 18. Arithmetic and decoding tests

### 18.1 Integer/rational

- Test zero/one, maximum one-limb values, carries/borrows across each limb, long all-zero/all-one patterns, powers of two, and sign combinations.
- Check add/subtract/multiply identities and embedded hexadecimal known-answer vectors.
- For division require `a == q*b+r` and `0 <= r < b`; cover one/multi-limb divisors, normalization shifts, exact division, and quotient-estimate correction.
- Test GCD symmetry/divisibility, common powers of two, coprime values, equal values, and Fibonacci worst cases.
- Test rational denominator/sign normalization, zero canonicalization, common-factor cancellation, all arithmetic identities, comparison antisymmetry/transitivity, and equality/hash/encoding consistency.
- Use an independent checked `uint64_t`/`int64_t` or `__int128` oracle for bounded domains and checked-in offline-generated large known-answer vectors. No runtime external oracle is allowed.
- Inject allocation failure, byte/work/bit limits at exact-limit and one-over, and cancellation during multiply/divide/GCD/normalization. Verify inputs remain unchanged and no result/cache entry escapes.
- Golden-test canonical bytes and reject leading zeros, negative zero, zero denominator, reducible rational forms, unknown schema, truncated lengths, and trailing bytes.

### 18.2 Source decoding

- Cover `+0`, `-0`, minimum/maximum subnormal, minimum normal, adjacent values around the subnormal boundary and one, powers of two, one-ULP neighbors, and maximum finite values for both types.
- Reject both infinities and representative quiet/signaling NaN payloads while preserving diagnostic bits.
- Exhaustively pass all binary16 bit patterns through a test-only independent decoder mapped exactly into binary32, and run fixed large binary32/binary64 bit-pattern corpora.
- Prove ambient rounding mode and host byte order do not alter the exact rational or canonical encoding.
- Where representable, round-trip decoded values through the exact Component 11-facing conversion helper and recover the source bits, treating signed-zero provenance separately.

## 19. Predicate, relation, and construction tests

- Test all orientation signs/zero, odd/even permutations, translations, axis permutations, exact power-of-two scales, maximum exponents, subnormals, cancellation-heavy coordinates, and one-ULP separations.
- Test every point/segment category and segment reversal mapping.
- Test every point/triangle/polygon category, concavity, collinear boundary chains, repeated chart extrema, ray through vertices, ring rotation, and orientation reversal. Malformed rings must return invalid precondition.
- Test every 2D/3D segment carrier, point-kind, endpoint-incidence, and overlap category, including skew lines and zero-length point-segment overloads.
- Test every segment/plane endpoint-side combination and coplanar concave segment/polygon cases with zero, one, and multiple intervals.
- Test plane parallel-disjoint, coincident same/opposite, and nonparallel carrier construction.
- Test polygon disjointness, containment, proper crossing, isolated contacts, T-junctions, shared edges, partial/equal positive-area overlap, opposite orientation, and multiple concave components. Ensure artificial triangulation diagonals never appear.
- Test ray interior/edge/vertex/origin/coplanar categories and shell point location for exterior, material, cavity, and every boundary feature.
- Substitute every construction into all defining equations. Generate syntactically different derivations of equal points and distinct exact points that round to the same `T`.
- Test argument swaps/reversals against each documented relation mapping and require canonical component order independent of discovery order.

## 20. Filter, symbolic, determinism, and qualification tests

### 20.1 Filters

- Force exact-only, filter attempt, certified acceptance, and exact fallback. In unlimited mode compare every accepted sign and all canonical evidence against exact-only evaluation.
- Exercise safe-range boundaries immediately before filter bypass, subtraction/product overflow and underflow hazards, subnormal inputs/intermediates, and non-finite approximate results.
- Run under every supported ambient rounding mode, prove `FE_TONEAREST` is established only inside the guard, and prove caller state restoration.
- Build on FMA-capable targets and verify contraction remains disabled. Compare GCC/Clang Debug/Release outputs and effective compile commands.
- Exhaustively enumerate bounded integer grids and selected binary16-derived grids; generate determinant-first cancellation cases and serialize failures by source bits.
- Add test variants that mutate/reduce error bounds or operation ordering and prove the differential suite detects incorrect acceptance.
- Require filter-enabled/disabled successful artifacts to have identical semantic bytes; counters and timings remain noncanonical.

### 20.2 Symbolic decisions

- Prove a nonzero exact relation never invokes or changes under symbolic logic.
- Cover equal stable-feature coordinate symmetries, ray-through-vertex/edge ownership, open-side selection, decomposition ties, and radial sheet ordering.
- Require exact incidence/contact to remain in the unperturbed result.
- Vary raw input order, storage addresses, hash seeds, allocation patterns, cache state, task order, and thread count; decisions must follow only normalized strong IDs.
- Compare multiple valid finite ray directions and formal fallback; all must give the same non-boundary shell classification.
- Test carrier direction reversal and plane orientation reversal against documented symbolic order mappings.

### 20.3 Determinism and platform qualification

- Repeat request sets with thread budgets 1, 2, and a larger available count, shuffled construction/relation discovery, cold/warm caches, and forced filter modes.
- Require identical exact values, relations, component ordering, semantic evidence, provenance bytes, and policy digest for every successful equivalent execution.
- Run ASan/UBSan over malformed encodings, limb boundaries, and relation components; run TSan over immutable shared kernels, owner validation, request caches, and cancellation.
- Run all mandatory checks in Debug and Release/NDEBUG so assertions are not publication authority.
- Benchmark decode, integer operations by limb count, rational normalization/comparison, exact/filter orientation, segment/polygon relations, polygon/polygon relations, and substitution separately. Record fallback and limb/work counts, but never weaken exact execution or tests to meet a benchmark.
- Use a fully specified in-tree PRNG state transition and bit-pattern generation. Failure replay records contain test kind, seed, iteration, coordinate bits, exact operands, formula/policy version, execution mode, and rounding mode, never decimal strings alone.

## 21. Completion criteria

Component 3 is complete only when:

- every supported finite source scalar decodes to its exact dyadic value and every unsupported/non-finite pattern fails precisely;
- integer/rational canonical form, equality, order, hash, encoding, allocation, and limit invariants have executable tests;
- every semantic query required by Component 2 and Components 4-13 is available without callers reconstructing decisions from approximate data;
- every nonzero predicate sign is mathematically exact and every zero is genuine degeneracy;
- every construction has immutable exact value, complete provenance, owner-safe lifetime, and passing substitution evidence;
- rich relations preserve all boundary/contact/overlap categories and canonical component ordering;
- symbolic APIs retain the unperturbed relation and cannot erase coincidence or alter set membership;
- certified filters have documented proofs/assumptions and are observationally equivalent to exact fallback on successful semantic output;
- strict compiler/environment assumptions are checked and filter bypass always remains correct;
- configured exhaustion/cancellation returns a structured failure without guessed geometry or partial publication;
- canonical values and semantic outputs are independent of host endian, internal limb capacity, filter path, cache state, scheduling, and pointer identity;
- exact-only verifier, mutation, adversarial, compiler, Release, sanitizer, and deterministic replay suites pass without external dependencies;
- Component 2 can implement every exact validation, construction, triangulation-support, shell-location, and open-side operation in `plan_02_input_topology.md` solely through this kernel.

## 22. Plan-gap amendment: executable defining relations

Construction storage must publish sorted, versioned `defining_relation` records with closed kinds for coordinate equality, point-on-plane, point-on-line/carrier, affine parameter, equal point, and ordered-on-carrier. Each record binds its construction node, operand nodes, source features, canonical exact coefficient payload, expected relation, and formula version.

Expose an exact-only substitution API that evaluates a relation against independently supplied exact dyadic coordinates. Component 11 and its verifier consume these immutable records; neither may infer defining equations from producer-owned realization obligations. Mutation tests change coefficients, operands, kinds, expected relations, or relation membership while rebuilding enclosing metadata and require independent substitution to fail.
