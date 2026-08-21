# Controlled CAD-like qualification corpus ingestion (P6.6)

Plan 16 P6.6 introduces a fail-closed boundary between CAD-like source material
and the permanent qualification corpus. It does not grant a backend access to
unreviewed geometry, and it does not turn a digest-only private workload into a
redistributable artifact. The in-tree backend remains experimental.

## Versioned ingestion records

`YgorMeshesBooleanQualificationIngestion.h` defines schema version 1 for three
nested immutable records:

- `qualification_cad_artifact_reference` binds one artifact role, media type,
  byte count, content digest, redistribution class, and retrieval contract;
- `qualification_cad_preparation_record` binds the source digest, preparation
  policy, report digest, output digest, edit count, success state, and strict
  revalidation result; and
- `qualification_cad_case_record` binds the source/generator class, source
  system, intended model tolerance and unit, license/provenance statement,
  case count, preparation result, expected product outcomes, exact failure
  codes, geometry/operation/result-mode coverage, and all retained artifacts.

The canonical `qualification_cad_ingestion_manifest` orders records by stable
identifier and derives its record-set, category-coverage, expected-outcome,
anonymized-summary, and complete manifest digests. Callers cannot provide a
self-consistent but false summary: every count and digest is reconstructed from
the canonical case records before publication.

## Source and redistribution classes

A case names both its modeling origin and its corpus ownership class. The source
class vocabulary distinguishes analytic CAD exports, hand-edited tessellations,
scan-derived inputs, internally generated CAD-like inputs, private customer
workloads, and an explicit reviewed `other` class.

Redistribution is independent and mandatory:

- `repository_embedded` points to one confined relative repository path;
- `content_addressed_external` stores no transport credential or source bytes,
  but binds a versioned content address and a human-reviewable retrieval
  procedure; and
- `private_digest_only` records byte count and digest only and explicitly
  prohibits materialization through the public loader.

An internally generated record cannot claim private redistribution. A private
source cannot claim repository redistribution. Licensed external records must
name their license/provenance and declare the actual redistribution path.

## Artifact integrity and retrieval

Every artifact is addressed by the domain-separated
`ygor-domain-md5-128-v1:<hex>` content identifier and a separately computed
reference digest. The digest algorithm is versioned in the schema; it is an
identity and replay binding, not a replacement for access-control or transport
security.

`materialize_qualification_cad_artifact` accepts a caller-supplied loader only
for repository or permitted content-addressed references. It verifies the exact
byte count and content digest before returning bytes. It rejects missing
loaders, stale reference digests, path traversal, wrong addresses, truncation,
corruption, loader exceptions, and every attempt to retrieve a private
`unavailable_private` artifact.

Retrieval procedures are descriptive steps only. Credentials, access tokens,
private URLs, and customer identifiers must never be committed in a manifest.
The external store is responsible for authentication and authorization outside
this schema.

## Preparation and expected outcomes

Each case retains exactly one preparation report artifact. Its digest must equal
the report digest in the preparation record, and the preparation source digest
must equal exactly one primary operand-pair or source-model artifact under the
case's declared redistribution policy.

Strict validation and diagnosis-only success cannot alter the source digest or
report edits. Normalized success must record its edit count and pass strict
revalidation. Failed preparation publishes no output digest and cannot be
mislabelled as a product success. A case expecting a typed failure must enumerate
one or more exact `product_error_code` values; vague failure expectations are
rejected.

## Private and external CI representatives

Every content-addressed external or private digest-only case requires exactly
one compact, repository-embedded CI representative no larger than 4 MiB. Its
content digest must differ from the primary artifact, which prevents accidentally
committing the full private or licensed source under a surrogate role.

The representative is a sanitized category-level regression, not proof that the
external source was executed. Full campaign evidence must still name the primary
content digest and record the actual infrastructure outcome. The checked-in
`tests/mesh_boolean/cad_ingestion/manifest.tsv` is a human-reviewable inventory;
canonical machine bindings are produced by the C++ manifest API.

## Anonymized summaries and qualification bindings

The manifest derives totals by source class, geometry category, expected
outcome, exact failure code, and redistribution class. Private records contribute
only counts and canonical digests; no source filename, customer name, geometry,
or retrieval procedure is required or permitted.

`make_qualification_cad_corpus_bindings` converts each validated case record to
the P6.1 `qualification_corpus_binding` vocabulary. Source, redistribution,
license/provenance, case count, record digest, category coverage, and expected
outcome evidence therefore participate in the frozen campaign material binding.
Changing any of them invalidates prior qualification evidence.

## Validation and continuous integration

`Test_MeshesBooleanQualificationIngestion` covers repository, external, and
private records; digest-verified materialization; path and address rejection;
preparation/report/source cross-binding; compact representative enforcement;
canonical sorting; anonymized summary reconstruction; corpus binding generation;
round-trip replay; corruption/truncation rejection; and decode resource limits.

The P6.6 workflow builds and runs this test with GCC and Clang under strict C++17
floating-point flags. P6.6 completion means the ingestion boundary and permanent
compact representatives exist. It does not mean the 1,000-case CAD-like corpus
has been executed, that private artifacts are available to CI, or that any
backend/result/preparation profile is qualified.
