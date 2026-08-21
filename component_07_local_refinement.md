# Component 7: Exact Local Facet Refinement

## 0. Purpose

Subdivide each affected source polygon into non-overlapping symbolic patches conforming to all intersection curves and coplanar overlap boundaries. This is a planar constrained-arrangement problem performed independently per facet, with boundary splits supplied by the global symbolic registry.

## 1. Input contract

Accept validated facets, support-plane projections, symbolic complex, exact kernel, and source orientation. For each facet, all cross-operand intersections touching its closure must already be represented and all source-edge split orders must be final.

Constraints may meet at vertices, overlap source edges, form closed loops, terminate on boundaries, or coincide with one another. The input is not assumed to be in general position.

## 2. Required behavior

### Planar constraint construction

- Project exact symbolic geometry to the facet's deterministic 2D chart without losing exactness.
- Replace the source boundary by registry-ordered split segments shared with adjacent source facets.
- Insert all in-facet intersection segments and coplanar overlap boundaries.
- Normalize overlapping collinear constraints into atomic segments with complete multiplicity/provenance.
- Discover and register any constraint-constraint crossing that completeness checks show was not already explicit; publication requires reconciling it globally, not creating a facet-private duplicate.

### Arrangement formation

- Build a planar embedded graph with canonical vertices, directed halfedges, twins, exact angular order, and closed face cycles.
- Resolve angular ties by proven collinearity and multiplicity, not approximate angle.
- Extract bounded arrangement cells inside the source polygon; exclude the exterior and holes correctly.
- Label each atomic edge with all source/intersection/overlap provenance and orientation.

### Patch generation

Represent local cells as polygon-with-holes patches or deterministically decompose them into simple facets/triangles. Any decomposition diagonal is marked artificial and selected using exact visibility/orientation predicates. Avoid quality-driven choices that could vary numerically; quality refinement belongs outside the Boolean kernel.

Record source-facet orientation and side. Refinement changes subdivision only, never geometry or occupancy.

### Coverage rules

Every point of the source facet belongs to the closure of at least one output patch; open patch interiors are pairwise disjoint. Shared boundaries use identical symbolic vertex IDs and atomic edge identities. Zero-area cycles are retained only as arrangement incidence when needed and are never emitted as area patches.

## 3. Output contract

Produce per-facet `local_refinement` records containing symbolic patches, local halfedges/cycles, provenance labels, source-side orientation, artificial diagonal flags, and a coverage certificate.

Invariants:

- Patch closures exactly cover the source polygon.
- Patch interiors do not overlap.
- Every constraint is represented by a chain of arrangement edges with correct multiplicity.
- Every source boundary chain exactly matches the registry split sequence.
- Euler/incidence relations hold for each planar arrangement, accounting for holes/components.
- Every positive-area patch has exact non-zero orientation consistent with its source facet.

Failure conditions are incomplete symbolic input requiring a controlled upstream reconciliation pass, resource exhaustion, or an internal planar-arrangement defect. Sliver size is not a failure; exact zero/non-zero area decides.

## 4. Verification and definition of done

- Synthetic tests cover crossing constraints, T-junctions, stars, nested loops, collinear overlaps, repeated constraints, holes, concave boundaries, and constraints through original vertices.
- Exact area sum and boundary cancellation prove coverage.
- Adjacent source facets independently produce identical shared boundary split chains.
- Constraint insertion order and source ring rotation/reversal do not alter canonical arrangement equivalence.
- Random small arrangements are compared with an independent exhaustive planar graph builder.
