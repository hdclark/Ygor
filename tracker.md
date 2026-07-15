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

- [ ] Convert every plan-gap case from `tests/MeshBooleanPlanGapCases.md` into an executable CI test with an explicit success or typed-failure expectation. These tests are expected to compile, but are not expected to pass until the broad plan and implementation components are updated.
- [ ] Resolve the contract and algorithm gaps documented in `tests/MeshBooleanPlanGapCases.md`, ensuring the tests pass.
