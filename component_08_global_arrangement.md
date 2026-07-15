# Component 8: Global Boundary Arrangement and Stitching

## 0. Purpose

Stitch all local facet refinements into one global, provenance-rich arrangement of both input boundaries. Preserve coincident sheets explicitly until occupancy classification determines ownership.

## 1. Input contract

Accept verified local refinements for every source facet, canonical symbolic registry, validated source adjacency/orientation, and exact kernel.

All local boundaries on the same source edge must use the same ordered symbolic sequence. Every cross-operand intersection constraint must be represented on every incident source facet.

## 2. Required behavior

### Stitch source sheets

- Match local halfedges along original source edges using canonical atomic endpoint IDs, source-edge ID, and direction.
- Replace pre-refinement facet adjacency with adjacency among refined patches.
- Preserve which side of each patch is operand interior/exterior according to source orientation.

### Stitch intersection seams

- Collect patch boundary uses along each symbolic intersection curve.
- Establish exact cyclic order of incident sheets around non-coplanar carriers using plane orientations and symbolic perturbation only for probe direction.
- Represent seam adjacency and sectors without prematurely selecting Boolean boundary.

### Coincident sheets

- Group patches occupying the same exact open planar region.
- Retain separate source-sheet provenance and relative orientation/multiplicity.
- Build a common atomic subdivision so coincident regions have matching boundaries even when source facets were tessellated differently.
- Do not arbitrarily discard duplicates; later occupancy labels decide whether zero, one, or an oppositely oriented representative contributes.

### Global complex

Create explicit geometric vertices, separate topological vertex occurrences, occurrence-ended halfedges, patches, seam sectors, and complete spherical links. The representation may be non-manifold as an overlay of two valid boundaries and must retain every disconnected surface occurrence over common geometry.

## 3. Output contract

Produce an `arrangement_complex` with globally stitched patch cycles, halfedge/twin relations within source sheets, seam-sector ordering, coincident groups, exact provenance, and adjacency suitable for classification propagation.

Invariants:

- Every local patch and halfedge maps exactly once into the global complex.
- Every source-sheet halfedge has a compatible opposite use; seam boundaries have complete incident-sector records.
- Endpoint, curve, and patch incidence agree with the symbolic registry.
- Coincident groups have exact geometric equality and a common atomic domain.
- Open sectors around each edge are completely and deterministically ordered.
- No connectivity is inferred from coordinate rounding.
- Every local sheet germ maps to exactly one topological occurrence; coordinate equality alone never joins occurrences.
- Every contact vertex has a complete exact spherical arrangement with antipodal rays, arcs split at every crossing, strict open-region witnesses, and total sector continuation, including contacts with no positive-length seam.
- Every patch side owns a certified open 3D formal probe. Its base is in the patch relative interior, or all incident constraints are resolved by a strict finite direction polynomial.

Failure conditions are incompatible local refinements, missing seam incidence, contradictory coincidence/orientation data, resource exhaustion, or internal invariant failure. Such failures return upstream evidence rather than patching gaps.

## 4. Verification and definition of done

- Reconstruct source sheets and prove they are subdivision-equivalent to validated operands.
- Verify all halfedge cycles, twins, seam radial orders, and coincident-domain coverage independently.
- Test transverse intersections, tangencies, triple-looking local incidences, coincident same/opposite sheets, and seams ending at source vertices.
- Facet subdivision, input ordering, and parallel local-refinement order produce equivalent canonical complexes.

The mandatory verifier is a separate implementation family. It reconstructs occurrence partitions, radial order, spherical links, and probe openness from Components 2, 6, and 7 and exact primitives; it may not call producer grouping, radial/link, probe, or canonical-encoding helpers. A standalone verifier-link target excludes producer objects, so forbidden dependencies fail to link. Self-consistent mutations that weld occurrences, swap radial layers, omit a continuation, or replace an open probe with a boundary probe must fail by semantic reconstruction rather than stale counts or bytes.
