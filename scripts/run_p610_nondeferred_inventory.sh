#!/usr/bin/env bash
# Execute the in-tree non-deferred frozen candidate-manifest inventory and emit
# the P6.10 observation protocol.
#
# This is the built-in available-toolchain dispatcher for
# `campaign.nondeferred-frozen-manifest`. It runs the non-fuzz `mesh_boolean`
# CTest qualification inventory (the in-tree non-deferred frozen manifest) in a
# built profile and emits one `P610_OBSERVATION` marker per executed case, then
# writes the `completion.tsv` the campaign driver requires.
#
# Expected environment (set by `run_mesh_boolean_p610_campaign.sh`):
#   P610_REPO_ROOT              repository root
#   P610_BUILD_DIR              built profile directory (e.g. .../build-gcc-current-debug)
#   P610_NONDEFERRED_OUTPUT_DIR destination for observations and completion.tsv
#
# The protocol matches `docs/MeshBooleanP610ManualCampaign.md`.

set -u
set -o pipefail

REPO_ROOT="${P610_REPO_ROOT:?P610_REPO_ROOT required}"
BUILD_DIR="${P610_BUILD_DIR:?P610_BUILD_DIR required}"
OUTPUT_DIR="${P610_NONDEFERRED_OUTPUT_DIR:?P610_NONDEFERRED_OUTPUT_DIR required}"

sha256_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$1" | awk '{print $1}'
  else
    printf 'sha256sum or shasum is required\n' >&2
    exit 2
  fi
}

mkdir -p "$OUTPUT_DIR"

if [[ ! -d "$BUILD_DIR" ]]; then
  printf 'non-deferred dispatcher: build directory unavailable: %s\n' "$BUILD_DIR" >&2
  exit 127
fi

# Discover the non-fuzz mesh_boolean inventory exactly once.
discover_log="$OUTPUT_DIR/discover.log"
if ! (cd "$BUILD_DIR" && LC_ALL=C ctest -N -L mesh_boolean -LE fuzz) > "$discover_log" 2>&1; then
  printf 'non-deferred dispatcher: test discovery failed\n' >&2
  exit 125
fi

mapfile -t TESTS < <(awk '/^[ \t]*Test[ \t]+#[0-9]+:/ { sub(/^.*:[ \t]*/, ""); if ($0 != "") print $0 }' "$discover_log")
if [[ ${#TESTS[@]} -eq 0 ]]; then
  printf 'non-deferred dispatcher: no non-deferred inventory tests discovered\n' >&2
  exit 126
fi

observations_file="$OUTPUT_DIR/observations.tsv"
: > "$observations_file"

overall=0
for test_name in "${TESTS[@]}"; do
  log_file="${test_name}.log"
  if (cd "$BUILD_DIR" && LC_ALL=C ctest -R "^${test_name}$" --output-on-failure) \
      > "$OUTPUT_DIR/$log_file" 2>&1; then
    outcome="verified_exact_success"
    error_code=""
  else
    outcome="unexpected_typed_failure"
    error_code=""
    overall=1
  fi
  line=$(printf 'P610_OBSERVATION\t%s\t%s\t%s\t%s\n' \
    "$test_name" "$outcome" "$error_code" "$log_file")
  printf '%s\n' "$line" >> "$observations_file"
  printf '%s\n' "$line"
done

executed_case_count=${#TESTS[@]}
manifest_digest=$(sha256_file "$discover_log")
observation_digest=$(sha256_file "$observations_file")

{
  printf 'schema\t1\n'
  printf 'status\tcomplete\n'
  printf 'executed_case_count\t%s\n' "$executed_case_count"
  printf 'manifest_digest\t%s\n' "$manifest_digest"
  printf 'observation_digest\t%s\n' "$observation_digest"
} > "$OUTPUT_DIR/completion.tsv"

exit "$overall"
