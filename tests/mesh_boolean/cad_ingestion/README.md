# Controlled CAD-like ingestion fixtures

This directory contains only compact, redistributable metadata and sanitized CI representatives. The production-scale licensed and private artifacts are represented by content digests in the qualification ingestion manifest. Tests construct deterministic stand-in bytes for the unavailable primary artifacts, verify the recorded content addresses and retrieval policy, and prove that private records cannot be materialized.

`manifest.tsv` is a human-reviewable inventory. Canonical machine bindings are produced by `qualification_cad_ingestion_manifest`; the TSV is not an alternate serialization schema.
