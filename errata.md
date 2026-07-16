# Mesh Boolean Errata: Exact Interior Witnesses for Holed Global Patches

## Status

This erratum applies to the Component 8 global-arrangement implementation in pull request #89.

The architectural plan and component specifications are correct. The defect is confined to the implementation of exact patch-interior witness construction and to an omission in the independent verifier.

Apply-ready patches are provided in `patches/`:

1. `0001-exact-patch-interior-witness.patch`
2. `0002-verify-patch-probes-exclude-holes.patch`
3. `0003-regress-annular-patch-witness.patch`

Apply them from the repository root, in order:

```sh
git apply --check patches/0001-exact-patch-interior-witness.patch
git apply --check patches/0002-verify-patch-probes-exclude-holes.patch
git apply --check patches/0003-regress-annular-patch-witness.patch
git apply patches/0001-exact-patch-interior-witness.patch
git apply patches/0002-verify-patch-probes-exclude-holes.patch
git apply patches/0003-regress-annular-patch-witness.patch
```

## Observed failure

`MeshBooleanRealization.Unit` fails while constructing the through-column subtraction case with:

```text
boolean_error[6:6:0] arrangement_exception: global patch has no exact interior witness
```

The failing geometry contains an annular planar patch: an outer square with a centered square hole.

## Root cause

`src/YgorMeshesBooleanGlobalArrangement.cc::patch_interior()` currently enumerates triples of outer-cycle vertices and tests their centroids. This is not a complete exact-interior-witness construction.

For the centered square annulus, every centroid of three distinct outer-square vertices lies inside the square hole. The routine therefore exhausts its heuristic candidates and throws even though the patch has nonempty relative interior.

This violates the Component 8 requirement that every patch side own a certified exact probe whose base lies in the patch relative interior.

## Required producer change

Replace the centroid-of-three-outer-vertices search in `patch_interior()` with a complete deterministic exact vertical decomposition.

### Required algorithm

1. Project the outer cycle and all hole cycles using the patch's dominant projection axis.
2. Collect, sort, and deduplicate every projected vertex x-coordinate using exact-rational ordering.
3. For each open slab between consecutive x-coordinates, select the exact midpoint x.
4. Intersect that vertical line with every segment of the outer and hole rings whose endpoint x-coordinates strictly straddle the slab midpoint.
5. Sort and deduplicate the exact y-intersections.
6. For every open interval between consecutive y-intersections, test its exact midpoint:
   - require `classify_point_polygon(candidate, outer) == open_interior`;
   - require `classify_point_polygon(candidate, hole) == outside` for every hole.
7. Lift the first accepted projected point back onto the exact support plane and return it.
8. Throw `global patch has no exact interior witness` only if every exact slab interval fails. At that point the failure represents an internal invariant violation rather than failure of a heuristic candidate set.

The lifting step must solve the support-plane equation exactly. For dominant projection:

- `drop_x`: solve `a*x + b*y + c*z + d = 0` for `x`;
- `drop_y`: solve for `y`;
- `drop_z`: solve for `z`.

Account for `orientation_parity` only in side-direction logic; the geometric plane equation coefficients already define the same zero set.

No floating-point arithmetic, epsilon, random sampling, or realized coordinates may be used.

### Determinism

Candidate enumeration must be stable and canonical:

- exact x-coordinates sorted ascending;
- exact y-intersections sorted ascending;
- slabs visited from least x to greatest x;
- intervals visited from least y to greatest y;
- first valid candidate wins;
- no hash-table iteration order may affect selection.

## Required verifier change

`src/YgorMeshesBooleanGlobalArrangementVerifier.cc::reconstruct_probes()` currently verifies that a probe base is on the support plane and in the outer polygon, but it does not prove that the base is outside every hole.

After the existing outer-cycle `open_interior` check, reconstruct each projected hole ring and require:

```cpp
classify_point_polygon(project(*p.exact_base, axis), hole_ring)
    == point_region_kind::outside
```

This check must remain in the independent verifier implementation family and must not call the producer's witness-selection helper.

## Required regression tests

The supplied third patch extends `tests/Test_MeshesBooleanRealization.cc` so the through-column subtraction constructs and verifies Component 8 before realization begins:

1. Create a cube scaled to `[0,4]^3`.
2. Create a column cube remapped to `[1,3] x [1,3] x [-1,5]`.
3. Build the global arrangement for `a_minus_b`.
4. Require successful publication and a passing mandatory verification report.
5. Require at least one emitted global patch with a nonempty `holes` vector.
6. Continue with the existing realization and hole-triangulation assertions.

A further verifier mutation test should move a probe belonging to a holed patch to an exact point inside the first hole and require mandatory verification to reject the artifact.

## Acceptance criteria

The correction is complete only when all of the following hold:

- All three patch files pass `git apply --check` against the intended PR revision.
- `MeshBooleanGlobalArrangement.Unit` passes.
- `MeshBooleanRealization.Unit` passes, including the through-column subtraction case.
- The mandatory Component 8 verifier rejects a probe moved into a hole.
- Component 8 property tests remain passing.
- Canonical output remains deterministic under input ordering and scheduling variations.
- No plan or schema change is required.
