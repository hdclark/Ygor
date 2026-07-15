# Component 13: Invariant Verification, Certificates, and Diagnostics

## 0. Purpose

Make every stage's correctness obligations executable, localize defects at the first broken invariant, and provide deterministic evidence suitable for replay and minimization. Verification is infrastructure implemented alongside the earliest components, not a final cleanup pass.

## 1. Input contract

Accept any published stage artifact, its declared contract/version, immutable upstream artifacts, exact kernel, context/replay metadata, and a verification level.

Verification levels may trade cost for coverage, but mandatory publication checks cannot be disabled in production. Expensive independent reconstruction and global cross-checks may be verification-build options.

## 2. Required behavior

### Stage checkers

Implement independent checkers for:

- Context immutability, IDs, resource accounting, and deterministic ordering.
- Input indices, halfedges, vertex links, facets, shell embedding/orientation, and solid semantics.
- Exact-number canonical forms, predicate certificates, and construction substitution.
- Broad-phase bound containment and exhaustive no-false-negative comparison in test mode.
- Raw-event incidence/completeness witnesses and registry equivalence/order.
- Local arrangement planarity, cycles, constraint coverage, and exact source-facet coverage.
- Global stitching, topological occurrence partitions, twins, seam radial order, complete spherical links, open probes, coincidence domains, and source reconstruction.
- Frozen classification strategy, one direct certificate per patch side, transition-audit agreement, and side classifications.
- Truth-table selection, orientation, closed stratified incidence, occurrence links, and independent topology classification.
- Exact-target equality, defining relations, decomposed realization certificates, pair broad phase, sign/order/incidence obligations, and realized embedding.
- Final output index, topology, geometry, orientation, canonical order, and re-ingestion.

Mandatory semantic checkers reconstruct facts from lower-level data and may not call the producer helper family for grouping, ordering, triangulation, assignment validation, obligation generation, pair generation, or canonical encoding. Standalone verifier-link targets enforce this separation for Components 8 and 11.

### Certificates

Define versioned, serializable certificate records containing stable IDs, exact signs/relations, provenance, and digests. Certificates need not be formal proofs accepted by an external theorem prover, but must enable independent deterministic replay of each claimed invariant.

### Diagnostics

- Report the first causal violation plus related entities, not thousands of downstream symptoms.
- Include exact values as canonical integer/rational text or binary limbs and original floats as bit patterns.
- Include local subgraphs, predicate operands, stage options, and input digest.
- Never use approximate decimal output as the sole evidence.

### Fault containment

A failed mandatory check prevents stage publication. Verification itself is read-only and cannot repair artifacts. Internal invariant errors are distinct from user input errors and resource failures.

### Minimization hooks

Expose dependency/provenance graphs so the test harness can remove unrelated facets/events while preserving a failure. Replay files are versioned and reject incompatible silent reinterpretation.

## 3. Output contract

Produce `verification_report` with pass/fail status, checker version, artifact digest, checked invariant set, compact certificates, diagnostics, and optional minimized replay seed.

Invariants:

- A pass identifies exactly which checks ran.
- Check ordering and reports are deterministic.
- Verification does not mutate the artifact.
- Every failed check references valid stable IDs or explicitly reports corrupted handles.
- Artifact and certificate digests detect stale/mismatched evidence.

Failure conditions include a violated artifact invariant, malformed/stale certificate, verifier resource limit, or verifier implementation defect. A verifier resource limit cannot be reported as artifact success.

## 4. Verification and definition of done

- Mutation testing corrupts each major field and demonstrates detection by an appropriate checker.
- Producer and checker implementations do not share unchecked derived state.
- Reports replay identically across process runs and thread counts.
- Certificate versioning and stale-artifact detection are tested.
- Production mandatory checks have bounded documented cost or explicit resource accounting.
- Self-consistent producer-shaped mutations are rejected after counts, IDs, certificates, bytes, and digests are rebuilt.
