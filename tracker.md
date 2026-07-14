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
- [ ] Full Component 14 adversarial, randomized, metamorphic, replay, and performance suites.

Additional items as determined through plan and implementation peer-review:

- [ ] Convert every plan-gap case from `tests/MeshBooleanPlanGapCases.md` into an executable CI test with an explicit success or typed-failure expectation. These tests are expected to compile, but are not expected to pass until the broad plan and implementation components are updated.
- [ ] Resolve the contract and algorithm gaps documented in `tests/MeshBooleanPlanGapCases.md`, ensuring the tests pass.

