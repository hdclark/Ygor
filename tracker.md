# Implementation tracker

- [x] Component 1 contracts, immutable IDs, diagnostics, and deterministic utilities.
- [x] Component 3 exact arithmetic and predicates, alongside Component 14 arithmetic/predicate oracles.
- [x] Component 13 artifact verifiers and Component 2 topology validation.
- [x] Component 4 broad phase, tested against exhaustive pair enumeration.
- [x] Components 5 and 6 symbolic events and registry.
- [x] Component 7 local arrangements, first on synthetic exact constraints and then on discovered events.
- [x] Component 8 global stitching and manifold/arrangement checks.
- [x] Component 9 classification, then Component 10 selection.
- [x] Component 11 checked realization and impossibility reporting.
- [x] Component 12 output assembly and canonicalization.
- [x] Full Component 14 adversarial, randomized, metamorphic, replay, and performance suites.
- [x] Determine the root problem causing the following test failures, then identify whether the components and plans need to be updated, then update them and fix the issue. Root cause: GCC `-ffast-math` remained active on final compiler-driver link commands and linked `crtfastmath`, enabling process-wide FTZ/DAZ despite strict Boolean translation-unit flags. Components 9 and 10 were correct; Plans 1 and 14, strict link policy, runtime platform validation, and diagnostics were updated.

```
...
      Start 20: MeshBooleanCellClassification.Adversarial         20/33 Test #20: MeshBooleanCellClassification.Adversarial ...***Failed   34.02 sec                                                  analytic classification case op=0 bounds=-0.000000,0.000000,0.000000,0.000000: boolean_error[0:1:5] zero_length_edge
...
      Start 23: MeshBooleanSelection.Adversarial                  23/33 Test #23: MeshBooleanSelection.Adversarial ............***Failed   32.78 sec                                                  analytic selection case op=0 bounds=-0.000000,0.000000,0.000000,0.000000: boolean_error[0:1:5] zero_length_edge
...
```

Additional items as determined through plan and implementation peer-review:

- [x] Convert every plan-gap case from `tests/MeshBooleanPlanGapCases.md` into an executable CI test with an explicit success or typed-failure expectation. These tests are expected to compile, but are not expected to pass until the broad plan and implementation components are updated.
- [x] Resolve the contract and algorithm gaps documented in `tests/MeshBooleanPlanGapCases.md`. The broad plan and Components 1, 3, 6, and 8-14 now distinguish exact geometry from topological occurrences, stratified selection from manifold publication, direct side classification from transition auditing, and exact-in-`T` semantics from future approximate output.
- [x] Components 1, 8, 9, and 13: implement topological occurrence IDs, complete spherical vertex links, certified open patch-side probes, frozen `independent_patch_side_v1` classification, independent arrangement reconstruction, and G3-G6/G9a/G9b/G9d gates.
- [x] Components 1, 10, 12, 13, and 14: implement stratified selected-boundary occurrences and topology classification, manifold-result preflight, `result_topology_not_supported`, and the complete G1 operation/type matrix.
- [x] Components 1, 3, 11, 13, and 14: implement `exact_in_T`, executable defining relations and exact substitution obligations, independent realization verification, and G2/G7/G9c gates.
- [x] Components 1, 11, 13, and 14: implement realization constraint-component decomposition, canonical component transcripts, conservative candidate-domain triangle broad phase, bounded exhaustive oracles, and G8 scalability gates.
- [x] Component 14: remove the plan-gap red-test label only after all G1-G9 tests pass in required Debug/Release, compiler, type, permutation, and replay matrices; retain the touching-cube and one-third fixtures permanently in the regression corpus.
