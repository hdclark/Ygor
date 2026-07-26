# Mesh Boolean corpus

The permanent corpus is additive and content-bound. Decimal geometry files are
not authoritative replay input: compact canonical construction recipes, exact
expectation contracts, and minimized regression records are.

## Files

- `manifest.tsv` is the legacy record index retained for replay compatibility.
- `inventory.tsv` is the Plan 16 P6.2 permanent qualification inventory. It is
  sorted by stable record ID and is validated by `MeshBoolean.Qualification`.
- `YgorMeshesBooleanQualificationGeneration.{h,cc}` materializes the P6.2
  ordinal ranges as exact retained recipes and defines deterministic fuzz,
  chain, minimization, and regression-promotion contracts.
- `Test_MeshesBooleanQualificationGeneration.cc` exercises those contracts as
  `MeshBoolean.QualificationGeneration`.

`inventory.tsv` records one deterministic recipe family or one permanent
regression. `first_case_ordinal` and `case_count` define a closed-open ordinal
range for the named recipe version. An ordinal denotes one operand pair or one
operation chain; operation, operand-order, type/index, result-mode, and
preparation fields describe required execution coverage and do not multiply the
case floor silently.

The current inventory binds:

- 10,500 construction-known operand pairs;
- 1,100 internally generated CAD-like operand pairs;
- 1,100 deterministic operation chains of five to twelve steps; and
- 21 permanent minimized regressions, including every distinct executable
  G1-G9 plan-gap case and every record in the legacy manifest.

Every counted family carries a nonzero recipe digest and a separate expectation
digest. Expected typed failures list exact product error codes; a generic
"failure" expectation is rejected. All five Boolean operations, both operand
orders, all supported coordinate/index combinations, all three result modes,
and strict/diagnosis/normalized preparation paths are encoded for every
required geometry category.

P6.3 regenerates every frozen inventory ordinal byte-for-byte before accepting
it. Valid fuzz mutations, invalid/preparation cases, and minimized descendants
carry a derivation kind plus their parent-case digest, so edited geometry cannot
masquerade as a counted base ordinal. Exact rational recipe coordinates are
converted to `float` or `double` only after exact representability is proved.

Records are never reused for different semantics. A changed recipe,
expectation, category, count, policy, or test binding changes the canonical
record and inventory digests and therefore invalidates any campaign manifest
that referred to the prior corpus binding.
