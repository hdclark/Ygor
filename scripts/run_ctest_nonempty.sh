#!/usr/bin/env bash

set -u
set -o pipefail

if [[ $# -lt 1 ]]; then
    printf 'Usage: %s BUILD_DIR [CTEST_ARGUMENT ...]\n' "$0" >&2
    exit 2
fi

build_dir="$1"
shift

if [[ ! -d "$build_dir" ]]; then
    printf 'CTest build directory does not exist: %s\n' "$build_dir" >&2
    exit 2
fi

listing=$(mktemp)
trap 'rm -f "$listing"' EXIT

if ! (cd "$build_dir" && LC_ALL=C ctest -N "$@") > "$listing" 2>&1; then
    cat "$listing"
    exit 2
fi
cat "$listing"

test_count=$(awk '/^Total Tests: [0-9]+$/ {count=$3} END {print count+0}' "$listing")
if (( test_count == 0 )); then
    printf 'P6.10 CTest guard: no tests matched the requested inventory.\n' >&2
    exit 125
fi

cd "$build_dir"
exec ctest "$@"
