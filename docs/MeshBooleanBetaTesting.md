# Mesh Boolean end-user beta-testing failure report guide

The qualification campaign was shortened by a leadership cost-saving decision,
so field feedback is a first-class part of the release path. This guide tells a
beta tester exactly what to capture so that a failure can be passed back to the
development team unambiguously and replayed without guessing.

A report is only actionable when the development team can reproduce the exact
invocation. Capture the fields below for every failure. Do not paraphrase error
text, and do not retry with edits before recording the original failure.

## 1. The typed error

Every failure is a typed `product_error`. Record all of its fields verbatim:

- numeric `code` (the `product_error_code` value);
- `subcode`;
- `message_key`;
- `detail` (including any nested engine `boolean_error` rendered into it);
- the backend `id`/`version`/`build` if `backend` is present; and
- `replay_binding_digest` as a 32-character hex string if nonzero.

A minimal renderer equivalent to the example is:

```cpp
void report_error(const char *where, const product_error &error) {
  std::cerr << where << ": code=" << static_cast<unsigned>(error.code)
            << " subcode=" << error.subcode
            << " key=" << error.message_key;
  if (!error.detail.empty())
    std::cerr << " detail=" << error.detail;
  if (error.backend)
    std::cerr << " backend=" << static_cast<unsigned>(error.backend->id);
  std::cerr << " replay=" << error.replay_binding_digest.hex() << '\n';
}
```

## 2. The exact invocation

Record the operation and every option that differs from the default
`boolean_service_options`:

- operation (union, intersection, difference direction, symmetric difference);
- backend selection mode and requested backend, including `allow_experimental_backend`
  and `allow_explicit_unqualified` opt-ins;
- preparation mode and, when normalized, the full `normalization_policy`
  (units, tolerance, enabled operations, non-planar policy);
- requested result representation and realization semantics/search policy;
- attribute-transfer mode; and
- qualification policy mode and manifest reference.

## 3. The two operands

Attach the exact `fv_surface_mesh<T,I>` inputs. Preferred, in order:

1. the canonical serialized bytes produced by `encode_prepared_operand(...)`
   (or `encode_prepared_operand` output for each prepared operand), which round
   trips exactly through `decode_prepared_operand(...)`;
2. the exact vertex array and face index array, printed bit-for-bit (for
   floating coordinates print the underlying bit pattern, not a decimal
   approximation);
3. as a last resort, the source file plus the exact loader/version used.

Also record the preparation certificate digests from `strict_validation_certificate`:
`input_digest` and `prepared_digest`. These let the development team confirm the
replayed operands are byte-identical to what failed.

## 4. Coordinate and index types

State `T` (`float`/`double`) and `I` (`std::uint32_t`/`std::uint64_t`), plus the
exact compiler flags if any strict floating-point mode was selected.

## 5. Environment and version

Record:

- Ygor version and repository commit (`git rev-parse HEAD` if built from source);
- compiler and version (`g++ --version` / `clang++ --version`);
- standard library and build type (Debug/Release);
- operating system and architecture (`uname -a`).

## 6. Reproducibility

Report whether the identical call fails deterministically on the same inputs.
If the outcome varies between runs, say so explicitly and attach the outputs of
two consecutive runs: nondeterminism is itself a high-severity finding.

## Report template

```text
ygor mesh boolean beta failure

error:
  code:            <product_error_code numeric value>
  subcode:         <numeric>
  message_key:     <exact string>
  detail:          <exact string>
  backend:         <id/version/build or "none">
  replay_digest:   <32 hex digits or "zero">

invocation:
  operation:       <union|intersection|difference|symmetric_difference>
  backend mode:    <explicit_backend|qualified_default|...>
  backend:         <experimental_exact_v1|...>
  preparation:     <strict_validation|normalized|diagnosis_only>
  representation:  <exact_stratified|exact_in_T_mesh|certified_approximate_mesh>
  attributes:      <...>

operands:
  prepared_serialized:  <attached file or "not available">
  input_digest:         <32 hex digits or "unknown">
  prepared_digest:      <32 hex digits or "unknown">

types:
  T: <float|double>
  I: <uint32|uint64>

environment:
  ygor version/commit: <...>
  compiler:            <...>
  stdlib/build:        <...>
  os/arch:             <...>

reproducibility:
  <deterministic|nondeterministic, with both outputs attached>
```

## How the development team uses the report

The typed code/subcode/message-key selects the exact failure path. The
`replay_binding_digest` and the `input_digest`/`prepared_digest` bind the report
to the exact replay schema, options, backend, verifier set, and operand bytes,
so the failure is replayed rather than re-inferred. A deterministic minimal
reproducer is then minimized into the permanent regression corpus.

Reported failures do not by themselves qualify a profile. They feed the
fail-closed, transactional verification path documented in
`docs/MeshBooleanProductContract.md`, `docs/MeshBooleanService.md`, and
`docs/MeshBooleanQualification.md`.
