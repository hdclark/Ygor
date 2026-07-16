# Mesh Boolean Errata: Exact Interior Witnesses for Holed Global Patches

## Status

This erratum applies to the Component 8 global-arrangement implementation in pull request #89.

The architectural plan and component specifications are correct. The defect is confined to the implementation of exact patch-interior witness construction and to an omission in the independent verifier.

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

Replace the centroid-of-three-outer-vertices search in `patch_interior()` with a complete deterministic exact construction.

### Required algorithm

1. Project the outer cycle and all hole cycles using the patch's dominant projection axis.
2. Collect every distinct projected x-coordinate and y-coordinate from all outer and hole vertices.
3. Sort and deduplicate both coordinate sets using exact-rational ordering.
4. For each open slab cell formed by consecutive x coordinates and consecutive y coordinates, in lexicographic `(x interval, y interval)` order:
   - construct the exact midpoint `(x_i+x_{i+1})/2, (y_j+y_{j+1})/2`;
   - require `classify_point_polygon(candidate, outer) == open_interior`;
   - require `classify_point_polygon(candidate, hole) == outside` for every hole;
   - lift the projected candidate back onto the exact support plane and return it.
5. If no slab-cell midpoint is accepted, run an exact fallback over boundary-derived vertical slabs:
   - for each consecutive distinct x pair, use the midpoint x;
   - intersect that vertical line with every outer/hole segment not parallel to it;
   - sort and deduplicate all exact y intersections;
   - test the midpoint of every consecutive y pair against the same outer/hole predicates;
   - lift and return the first accepted point.
6. Throw `global patch has no exact interior witness` only if both exact searches fail. At that point the failure represents an internal invariant violation rather than failure of a heuristic candidate set.

The lifting step must solve the support-plane equation exactly. For dominant projection:

- `drop_x`: solve `a*x + b*y + c*z + d = 0` for `x`;
- `drop_y`: solve for `y`;
- `drop_z`: solve for `z`.

Account for `orientation_parity` only in side-direction logic; the geometric plane equation coefficients already define the same zero set.

No floating-point arithmetic, epsilon, random sampling, or realized coordinates may be used.

### Determinism

Candidate enumeration must be stable and canonical:

- exact coordinates sorted ascending;
- x slabs before y slabs;
- first valid candidate wins;
- no hash-table iteration order may affect selection.

## Required verifier change

`src/YgorMeshesBooleanGlobalArrangementVerifier.cc::reconstruct_probes()` currently verifies that a probe base is on the support plane and in the outer polygon, but it does not prove that the base is outside every hole.

After the existing outer-cycle `open_interior` check, add:

```cpp
for (const auto &hole : patch.holes) {
    std::vector<exact_point2> ring;
    ring.reserve(hole.size());
    for (auto v : hole) {
        ring.push_back(project(
            a.symbolic->payload->vertices[
                a.vertices[v.value_for_debug()].symbolic.value_for_debug()
            ].point,
            axis));
    }
    const auto relation = classify_point_polygon(
        project(*p.exact_base, axis), ring);
    if (!relation.has_value() ||
        relation.value().kind != point_region_kind::outside)
        return false;
}
```

This check must remain in the independent verifier implementation family and must not call the producer's witness-selection helper.

## Required regression tests

Extend `tests/Test_MeshesBooleanGlobalArrangement.cc` with the same through-column construction used by `MeshBooleanRealization.Unit`:

1. Create a cube scaled to `[0,4]^3`.
2. Create a column cube remapped to `[1,3] x [1,3] x [-1,5]`.
3. Build the global arrangement for `a_minus_b` or through the operation-independent Component 8 context.
4. Require successful publication and a passing mandatory verification report.
5. Require at least one emitted global patch with a nonempty `holes` vector.
6. For every non-universe probe:
   - verify its base is in the corresponding outer cycle's open interior;
   - verify its base is outside every corresponding hole.
7. Create a mutable copy of the artifact and move one probe belonging to a holed patch to an exact point inside the first hole, for example a valid exact centroid of three hole vertices or the exact bounding-box midpoint when it classifies as `open_interior`.
8. Invoke the registered mandatory Component 8 verifier and require rejection.

The existing `MeshBooleanRealization.Unit` through-column case must pass after this change.

## Acceptance criteria

The correction is complete only when all of the following hold:

- `MeshBooleanGlobalArrangement.Unit` passes with the new annular-patch regression.
- `MeshBooleanRealization.Unit` passes, including the through-column subtraction case.
- The mandatory Component 8 verifier rejects a probe moved into a hole.
- Component 8 property tests remain passing.
- Canonical output remains deterministic under input ordering and scheduling variations.
- No plan or schema change is required.
