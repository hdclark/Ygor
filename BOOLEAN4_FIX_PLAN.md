# Boolean4 Fix Plan

## Scope
Evaluate and repair the `src/YgorMeshesBoolean4.{h,cc}` Boolean mesh path and the BSP-backed Boolean4 unit tests in `tests2/YgorMeshesBoolean4.cc`, while keeping changes limited to Boolean4/BSP Boolean behavior.

## Plan
1. Build the repository and run only the Boolean4 test binary/cases to identify current failures.
2. Inspect the Boolean4 wrapper and BSP conversion/merge implementation used by Boolean4.
3. Fix compile-time and runtime defects found by the targeted Boolean4 tests.
4. Add or adjust minimal targeted regression coverage only for Boolean4 behavior if needed.
5. Re-run only the Boolean4 tests, then commit and open a PR.

## Notes for follow-up attempts
- Boolean4 is a thin wrapper around `bsp_tree_volume`; most failures are expected to arise in mesh-to-BSP, BSP merge, or BSP-to-mesh conversion.
- Preserve adaptive predicate usage for plane-side classification; avoid replacing exact sign predicates with broad epsilon tests.
