#!/usr/bin/env bash
# Run and retain the single-machine P6.10 candidate-campaign evidence.
#
# This script deliberately continues after individual failures. It exits zero
# only when every selected step passed and no unresolved anomaly was recorded.

set -u
set -o pipefail

readonly SCRIPT_VERSION="2"
readonly QUALIFICATION_FUZZ_CPU_SECONDS=86400
readonly TSV_HEADER_ATTEMPTS=$'utc\tstep_id\tattempt\tclassification\texit_code\twall_seconds\tuser_cpu_seconds\tsystem_cpu_seconds\tmax_rss_kib\tcommand_file\tlog_file'
readonly TSV_HEADER_ANOMALIES=$'utc\tanomaly_id\tstep_id\tcase_identifier\tattempt\tcategory\tstatus\tdetail\tevidence_file\tlog_file'
readonly TSV_HEADER_STEPS=$'step_id\tphase\tprofile\trequired\tdescription'
readonly TSV_HEADER_RESOLUTIONS=$'utc\tanomaly_id\treviewer\trationale\tevidence_file\tevidence_sha256'
readonly TSV_HEADER_OBSERVATIONS=$'utc\tstep_id\tcase_identifier\toutcome\terror_code\tevidence_file\tlog_file'
readonly TSV_HEADER_INVOCATIONS=$'utc\tinvocation\tphase\tenvironment_file\tenvironment_sha256'

OUTPUT_DIR=""
WORK_DIR=""
PHASE="all"
JOBS="${P610_JOBS:-2}"
MAX_ATTEMPTS="${P610_MAX_ATTEMPTS:-3}"
STEP_TIMEOUT_SECONDS="${P610_STEP_TIMEOUT_SECONDS:-7200}"
FUZZ_CPU_SECONDS="${P610_FUZZ_CPU_SECONDS:-${QUALIFICATION_FUZZ_CPU_SECONDS}}"
FUZZ_CHUNK_WALL_SECONDS="${P610_FUZZ_CHUNK_WALL_SECONDS:-1800}"
SMOKE=0
ALLOW_DIRTY=0
SELF_TEST=0
RECORD_ONLY=0
RECORD_CATEGORY=""
RECORD_CASE=""
RECORD_MESSAGE=""
RECORD_EVIDENCE=""
RESOLVE_ONLY=0
RESOLVE_ID=""
RESOLVE_REVIEWER=""
RESOLVE_RATIONALE=""
RESOLVE_EVIDENCE=""
CURRENT_STEP=""
CURRENT_ATTEMPT="0"
LOCK_DIR=""
REPO_ROOT=""
CAMPAIGN_ID=""

usage() {
  cat <<'USAGE'
Usage:
  scripts/run_mesh_boolean_p610_campaign.sh [options]
  scripts/run_mesh_boolean_p610_campaign.sh --record-anomaly CATEGORY CASE MESSAGE [EVIDENCE]
  scripts/run_mesh_boolean_p610_campaign.sh --resolve-anomaly ID REVIEWER RATIONALE EVIDENCE

Options:
  --output DIR              Durable evidence directory. Required except for
                            --self-test. Put this outside the source tree.
  --work DIR                Restartable build directory. Default: DIR.work.
  --phase NAME              all, contracts, fuzz, or finalize (default: all).
  --jobs N                  Parallel build jobs (default: 2).
  --max-attempts N          Attempts for infrastructure/build steps (default: 3).
  --timeout-seconds N       Per contracts/build step timeout (default: 7200).
  --fuzz-cpu-seconds N      Aggregate CPU seconds per frozen fuzz allocation.
                            Values below 86400 require --smoke.
  --fuzz-chunk-seconds N    Maximum wall seconds per restartable fuzz chunk.
  --allow-dirty             Run against a dirty tree, but retain a blocking
                            dirty_repository anomaly.
  --smoke                   Current GCC/Clang Debug only; permits short fuzz
                            allocations and marks all evidence non-qualifying.
  --record-anomaly CATEGORY CASE MESSAGE [EVIDENCE]
                            Append a reviewed/manual anomaly without running.
  --resolve-anomaly ID REVIEWER RATIONALE EVIDENCE
                            Bind a reviewed resolution to an existing anomaly.
                            The evidence file must exist inside DIR.
  --self-test               Exercise checkpoint, failure, resume, and digest
                            generation without compiling Ygor.
  -h, --help                Show this help.

Optional compiler variables:
  P610_GCC_CC / P610_GCC_CXX                 current GCC (gcc/g++)
  P610_CLANG_CC / P610_CLANG_CXX             current Clang (clang/clang++)
  P610_OLDEST_GCC_CC / P610_OLDEST_GCC_CXX   oldest-supported GCC
  P610_OLDEST_CLANG_CC / P610_OLDEST_CLANG_CXX
  P610_LIBCXX_CXX_FLAGS / P610_LIBCXX_LINK_FLAGS
  P610_LIBCXX_DEBUG_FLAGS
  P610_INDEPENDENT_ARCH_COMMAND              native/emulated AArch64 case dispatcher
  P610_NONDEFERRED_CAMPAIGN_COMMAND           actual frozen non-deferred inventory dispatcher
  P610_BENCHMARK_CPUSET                       optional taskset CPU list

The architecture and non-deferred dispatchers must emit tab-separated
P610_OBSERVATION records. Any command may emit P610_ANOMALY records; see
`docs/MeshBooleanP610ManualCampaign.md` for the exact protocol.

Optional command overrides for the eight frozen fuzz allocations:
  P610_FUZZ_GCC_ASAN_VALID_COMMAND
  P610_FUZZ_GCC_ASAN_INVALID_COMMAND
  P610_FUZZ_CLANG_ASAN_VALID_COMMAND
  P610_FUZZ_CLANG_ASAN_INVALID_COMMAND
  P610_FUZZ_CLANG_TSAN_VALID_COMMAND
  P610_FUZZ_CLANG_TSAN_INVALID_COMMAND
  P610_FUZZ_OPERATION_CHAIN_COMMAND
  P610_FUZZ_LONG_RUNNING_COMMAND

Each override is a shell command executed from the repository root with
P610_BUILD_DIR, P610_ALLOCATION_ID, P610_CHUNK_ORDINAL, P610_ITERATION,
and deterministic YGOR_BOOLEAN_SEED_HIGH/LOW values exported.
USAGE
}

fail() {
  printf 'P6.10 campaign error: %s\n' "$*" >&2
  exit 2
}

is_uint() { [[ "$1" =~ ^[0-9]+$ ]]; }

require_tool() { command -v "$1" >/dev/null 2>&1 || fail "required tool unavailable: $1"; }

sanitize_tsv() {
  local value="$1"
  value=${value//$'\t'/ }
  value=${value//$'\r'/ }
  value=${value//$'\n'/\\n}
  printf '%s' "$value"
}

utc_now() { date -u +'%Y-%m-%dT%H:%M:%SZ'; }

sha256_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$1" | awk '{print $1}'
  else
    fail 'sha256sum or shasum is required'
  fi
}

quote_command() {
  local item
  for item in "$@"; do printf '%q ' "$item"; done
  printf '\n'
}

append_anomaly() {
  local category="$1" step_id="$2" attempt="$3" detail="$4"
  local evidence_file="${5:-}" log_file="${6:-}" case_identifier="${7:-$step_id}"
  local anomaly_id material
  material="${step_id}|${case_identifier}|${attempt}|${category}|${detail}|${evidence_file}|${log_file}"
  anomaly_id=$(printf '%s' "$material" | { sha256sum 2>/dev/null || shasum -a 256; } | awk '{print substr($1,1,20)}')
  if awk -F'\t' -v id="$anomaly_id" 'NR>1 && $2==id {found=1} END{exit !found}' \
      "${OUTPUT_DIR}/anomalies.tsv"; then
    return 0
  fi
  printf '%s\t%s\t%s\t%s\t%s\t%s\tunresolved\t%s\t%s\t%s\n' \
    "$(utc_now)" "$anomaly_id" "$(sanitize_tsv "$step_id")" \
    "$(sanitize_tsv "$case_identifier")" "$attempt" \
    "$(sanitize_tsv "$category")" "$(sanitize_tsv "$detail")" \
    "$(sanitize_tsv "$evidence_file")" "$(sanitize_tsv "$log_file")" \
    >> "${OUTPUT_DIR}/anomalies.tsv"
}

record_explicit_records() {
  local step_id="$1" attempt="$2" log_file="$3"
  local absolute="${OUTPUT_DIR}/${log_file}" marker category case_identifier detail evidence
  local outcome error_code
  [[ -f "$absolute" ]] || return 0
  while IFS=$'\t' read -r marker category case_identifier detail evidence _; do
    [[ "$marker" == P610_ANOMALY ]] || continue
    [[ -n "$category" && -n "$case_identifier" && -n "$detail" ]] || {
      append_anomaly malformed_evidence "$step_id" "$attempt" \
        "malformed P610_ANOMALY marker in ${log_file}" "$log_file" "$log_file"
      continue
    }
    if [[ -n "$evidence" && ( "$evidence" == /* || "$evidence" == *'..'* || ! -f "${OUTPUT_DIR}/${evidence}" ) ]]; then
      append_anomaly malformed_evidence "$step_id" "$attempt" \
        "P610_ANOMALY references missing or unsafe evidence path: ${evidence}" \
        "$log_file" "$log_file" "$case_identifier"
      evidence=""
    fi
    append_anomaly "$category" "$step_id" "$attempt" "$detail" \
      "$evidence" "$log_file" "$case_identifier"
  done < "$absolute"
  while IFS=$'\t' read -r marker case_identifier outcome error_code evidence _; do
    [[ "$marker" == P610_OBSERVATION ]] || continue
    [[ -n "$case_identifier" && -n "$outcome" ]] || {
      append_anomaly malformed_evidence "$step_id" "$attempt" \
        "malformed P610_OBSERVATION marker in ${log_file}" "$log_file" "$log_file"
      continue
    }
    if [[ -n "$evidence" && ( "$evidence" == /* || "$evidence" == *'..'* || ! -f "${OUTPUT_DIR}/${evidence}" ) ]]; then
      append_anomaly malformed_evidence "$step_id" "$attempt" \
        "P610_OBSERVATION references missing or unsafe evidence path: ${evidence}" \
        "$log_file" "$log_file" "$case_identifier"
      evidence=""
    fi
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$(utc_now)" \
      "$(sanitize_tsv "$step_id")" "$(sanitize_tsv "$case_identifier")" \
      "$(sanitize_tsv "$outcome")" "$(sanitize_tsv "$error_code")" \
      "$(sanitize_tsv "$evidence")" "$(sanitize_tsv "$log_file")" \
      >> "${OUTPUT_DIR}/observations.tsv"
  done < "$absolute"
}

record_known_anomalies() {
  local step_id="$1" attempt="$2" exit_code="$3" log_file="$4"
  local absolute="${OUTPUT_DIR}/${log_file}"
  local matched=0 category pattern line
  while IFS=$'\t' read -r category pattern; do
    [[ -n "$category" ]] || continue
    line=$(grep -Eim1 "$pattern" "$absolute" 2>/dev/null || true)
    if [[ -n "$line" ]]; then
      append_anomaly "$category" "$step_id" "$attempt" "$line" "$log_file" "$log_file"
      matched=1
    fi
  done <<'PATTERNS'
false_success	false[ _-]?success|semantic mislabel|incorrect published result
backend_disagreement	backend[ _-]?disagreement|independent backend.*disagree
verifier_disagreement	verifier[ _-]?disagreement|producer.*verifier.*disagree
nondeterministic_result	nondetermin|canonical.*mismatch|replay.*mismatch
resource_or_timeout	timeout|timed out|resource[_ -]?limit|out of memory|bad_alloc|cancel(l)?ation latency
performance_regression	performance.*regress|threshold.*fail|speed.*target|memory.*regress
sanitizer_failure	AddressSanitizer|UndefinedBehaviorSanitizer|ThreadSanitizer|runtime error:|data race
input_or_engine_failure	unexpected[_ -]?typed[_ -]?failure|internal[_ -]?invariant|FAIL[[:space:]]
PATTERNS
  if [[ "$exit_code" -eq 124 || "$exit_code" -eq 137 ]]; then
    append_anomaly "timeout" "$step_id" "$attempt" \
      "step exceeded its configured timeout; exit=${exit_code}" "$log_file" "$log_file"
    matched=1
  fi
  if [[ "$matched" -eq 0 ]]; then
    append_anomaly "other_anomaly" "$step_id" "$attempt" \
      "command exited nonzero without a recognized anomaly marker; exit=${exit_code}" \
      "$log_file" "$log_file"
  fi
}

status_value() {
  local step_id="$1" file="${OUTPUT_DIR}/state/${step_id}.status"
  [[ -f "$file" ]] && awk -F= '$1=="status" {print $2; exit}' "$file"
}

next_attempt() {
  local step_id="$1" dir="${OUTPUT_DIR}/logs/${step_id}"
  local max=0 file base
  mkdir -p "$dir"
  shopt -s nullglob
  for file in "$dir"/attempt-*.log; do
    base=${file##*/attempt-}; base=${base%.log}
    [[ "$base" =~ ^[0-9]+$ ]] && (( base > max )) && max=$base
  done
  shopt -u nullglob
  printf '%s\n' "$((max + 1))"
}

write_status() {
  local step_id="$1" status="$2" attempt="$3" exit_code="$4" log_file="$5"
  local tmp="${OUTPUT_DIR}/state/.${step_id}.status.$$"
  {
    printf 'schema=1\n'
    printf 'step_id=%s\n' "$step_id"
    printf 'status=%s\n' "$status"
    printf 'attempt=%s\n' "$attempt"
    printf 'exit_code=%s\n' "$exit_code"
    printf 'updated_utc=%s\n' "$(utc_now)"
    printf 'log_file=%s\n' "$log_file"
  } > "$tmp"
  mv "$tmp" "${OUTPUT_DIR}/state/${step_id}.status"
}

run_step() {
  local step_id="$1" classification="$2" max_attempts="$3" timeout_seconds="$4"
  shift 4
  local existing attempt log_file command_file time_file start end exit_code status
  local user_cpu=0 system_cpu=0 wall=0 max_rss=0
  existing=$(status_value "$step_id" || true)
  if [[ "$existing" == "pass" ]]; then
    printf 'SKIP %s (already passed)\n' "$step_id"
    return 0
  fi

  attempt=$(next_attempt "$step_id")
  if (( attempt > max_attempts )); then
    append_anomaly "infrastructure_issue" "$step_id" "$attempt" \
      "maximum attempts (${max_attempts}) exhausted" "" ""
    write_status "$step_id" "blocked" "$attempt" 125 ""
    return 1
  fi

  mkdir -p "${OUTPUT_DIR}/logs/${step_id}" "${OUTPUT_DIR}/commands"
  log_file="logs/${step_id}/attempt-${attempt}.log"
  command_file="commands/${step_id}.sh"
  time_file="${OUTPUT_DIR}/logs/${step_id}/attempt-${attempt}.time"
  {
    printf '#!/usr/bin/env bash\nset -u\nset -o pipefail\ncd %q\n' "$REPO_ROOT"
    quote_command "$@"
  } > "${OUTPUT_DIR}/${command_file}"
  chmod +x "${OUTPUT_DIR}/${command_file}"

  CURRENT_STEP="$step_id"; CURRENT_ATTEMPT="$attempt"
  write_status "$step_id" "running" "$attempt" 0 "$log_file"
  start=$(date +%s)
  printf 'RUN  %s (attempt %s)\n' "$step_id" "$attempt"

  set +e
  if command -v timeout >/dev/null 2>&1; then
    /usr/bin/time -f '%U\t%S\t%e\t%M' -o "$time_file" \
      timeout --signal=TERM --kill-after=30s "$timeout_seconds" \
      "$@" > "${OUTPUT_DIR}/${log_file}" 2>&1
    exit_code=$?
  else
    /usr/bin/time -f '%U\t%S\t%e\t%M' -o "$time_file" \
      "$@" > "${OUTPUT_DIR}/${log_file}" 2>&1
    exit_code=$?
  fi
  end=$(date +%s); wall=$((end - start))
  if [[ -s "$time_file" ]]; then
    IFS=$'\t' read -r user_cpu system_cpu _ max_rss < "$time_file" || true
  fi

  if [[ "$exit_code" -eq 0 ]]; then status="pass"; else status="fail"; fi
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$(utc_now)" "$step_id" "$attempt" "$classification" "$exit_code" \
    "$wall" "$user_cpu" "$system_cpu" "$max_rss" "$command_file" "$log_file" \
    >> "${OUTPUT_DIR}/attempts.tsv"
  write_status "$step_id" "$status" "$attempt" "$exit_code" "$log_file"
  record_explicit_records "$step_id" "$attempt" "$log_file"
  if [[ "$exit_code" -ne 0 ]]; then
    record_known_anomalies "$step_id" "$attempt" "$exit_code" "$log_file"
  fi
  CURRENT_STEP=""; CURRENT_ATTEMPT="0"
  [[ "$exit_code" -eq 0 ]]
}

on_interrupt() {
  local signal="$1"
  if [[ -n "$OUTPUT_DIR" && -d "$OUTPUT_DIR" && -n "$CURRENT_STEP" ]]; then
    append_anomaly "infrastructure_issue" "$CURRENT_STEP" "$CURRENT_ATTEMPT" \
      "campaign driver interrupted by ${signal}; restart the same command to resume" "" ""
  fi
  [[ -n "$LOCK_DIR" ]] && rmdir "$LOCK_DIR" 2>/dev/null || true
  exit 130
}

cleanup_lock() { [[ -n "$LOCK_DIR" ]] && rmdir "$LOCK_DIR" 2>/dev/null || true; }
trap 'on_interrupt INT' INT
trap 'on_interrupt TERM' TERM
trap cleanup_lock EXIT

initialize_output() {
  mkdir -p "$OUTPUT_DIR" "${OUTPUT_DIR}/state" "${OUTPUT_DIR}/logs" \
    "${OUTPUT_DIR}/commands" "${OUTPUT_DIR}/artifacts" "${OUTPUT_DIR}/fuzz-progress"
  [[ -f "${OUTPUT_DIR}/attempts.tsv" ]] || printf '%s\n' "$TSV_HEADER_ATTEMPTS" > "${OUTPUT_DIR}/attempts.tsv"
  [[ -f "${OUTPUT_DIR}/anomalies.tsv" ]] || printf '%s\n' "$TSV_HEADER_ANOMALIES" > "${OUTPUT_DIR}/anomalies.tsv"
  [[ -f "${OUTPUT_DIR}/steps.tsv" ]] || printf '%s\n' "$TSV_HEADER_STEPS" > "${OUTPUT_DIR}/steps.tsv"
  [[ -f "${OUTPUT_DIR}/resolutions.tsv" ]] || printf '%s\n' "$TSV_HEADER_RESOLUTIONS" > "${OUTPUT_DIR}/resolutions.tsv"
  [[ -f "${OUTPUT_DIR}/observations.tsv" ]] || printf '%s\n' "$TSV_HEADER_OBSERVATIONS" > "${OUTPUT_DIR}/observations.tsv"
  [[ -f "${OUTPUT_DIR}/invocations.tsv" ]] || printf '%s\n' "$TSV_HEADER_INVOCATIONS" > "${OUTPUT_DIR}/invocations.tsv"
  mkdir -p "${OUTPUT_DIR}/environment"
  LOCK_DIR="${OUTPUT_DIR}/.lock"
  if ! mkdir "$LOCK_DIR" 2>/dev/null; then
    fail "another campaign process appears active: ${LOCK_DIR}"
  fi
}

register_step() {
  local step_id="$1" phase="$2" profile="$3" required="$4" description="$5"
  if ! awk -F'\t' -v id="$step_id" 'NR>1 && $1==id {found=1} END{exit !found}' \
      "${OUTPUT_DIR}/steps.tsv"; then
    printf '%s\t%s\t%s\t%s\t%s\n' "$step_id" "$phase" "$profile" "$required" \
      "$(sanitize_tsv "$description")" >> "${OUTPUT_DIR}/steps.tsv"
  fi
}

capture_environment() {
  local commit tree dirty arch working_state_digest
  commit=$(git -C "$REPO_ROOT" rev-parse HEAD)
  tree=$(git -C "$REPO_ROOT" rev-parse HEAD^{tree})
  dirty=$(git -C "$REPO_ROOT" status --porcelain=v1 --untracked-files=normal)
  arch=$(uname -m)
  working_state_digest=$(
    { git -C "$REPO_ROOT" status --porcelain=v1 --untracked-files=all;
      git -C "$REPO_ROOT" diff --binary HEAD; } \
      | { sha256sum 2>/dev/null || shasum -a 256; } | awk '{print $1}'
  )
  CAMPAIGN_ID="p610-${commit:0:12}-$(hostname -s 2>/dev/null || hostname)-${arch}"
  {
    printf 'schema\t1\nscript_version\t%s\n' "$SCRIPT_VERSION"
    printf 'campaign_id\t%s\nrepository_commit\t%s\nrepository_tree\t%s\n' "$CAMPAIGN_ID" "$commit" "$tree"
    printf 'repository_dirty\t%s\nworking_state_digest\t%s\nstarted_utc\t%s\narchitecture\t%s\n' \
      "$([[ -n "$dirty" ]] && echo true || echo false)" "$working_state_digest" \
      "$(utc_now)" "$arch"
    printf 'fuzz_cpu_seconds_per_allocation\t%s\nsmoke\t%s\n' "$FUZZ_CPU_SECONDS" "$SMOKE"
  } > "${OUTPUT_DIR}/campaign.tsv"
  git -C "$REPO_ROOT" status --porcelain=v1 --untracked-files=all > "${OUTPUT_DIR}/git-status.txt"
  git -C "$REPO_ROOT" diff --binary HEAD > "${OUTPUT_DIR}/git-diff.patch"
  if [[ -n "$dirty" ]]; then
    if [[ "$ALLOW_DIRTY" -eq 0 ]]; then
      fail 'repository is dirty; commit/stash changes or pass --allow-dirty'
    fi
    append_anomaly "dirty_repository" "campaign.preflight" 0 \
      "campaign started from a dirty repository; P6.11 must not treat it as qualifying" \
      "git-status.txt" "git-diff.patch"
  fi
  if [[ "$SMOKE" -eq 1 ]]; then
    append_anomaly "non_qualifying_smoke" "campaign.preflight" 0 \
      "--smoke was used; reduced profiles or duration are not qualification evidence" \
      "campaign.tsv" ""
  fi
}


capture_invocation_environment() {
  local ordinal file relative digest compiler dirty
  ordinal=$(( $(awk 'END{print NR-1}' "${OUTPUT_DIR}/invocations.tsv") + 1 ))
  printf -v relative 'environment/invocation-%04d.txt' "$ordinal"
  file="${OUTPUT_DIR}/${relative}"
  dirty=$(git -C "$REPO_ROOT" status --porcelain=v1 --untracked-files=normal)
  {
    printf 'campaign_id=%s\ninvocation=%s\nutc=%s\nphase=%s\n' \
      "$CAMPAIGN_ID" "$ordinal" "$(utc_now)" "$PHASE"
    printf 'repository_commit=%s\nrepository_tree=%s\nrepository_dirty=%s\n' \
      "$(git -C "$REPO_ROOT" rev-parse HEAD)" \
      "$(git -C "$REPO_ROOT" rev-parse HEAD^{tree})" \
      "$([[ -n "$dirty" ]] && echo true || echo false)"
    printf 'jobs=%s\nmax_attempts=%s\nstep_timeout_seconds=%s\n' \
      "$JOBS" "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS"
    printf 'fuzz_cpu_seconds=%s\nfuzz_chunk_wall_seconds=%s\nsmoke=%s\n\n' \
      "$FUZZ_CPU_SECONDS" "$FUZZ_CHUNK_WALL_SECONDS" "$SMOKE"
    uname -a
    printf '\n--- os-release ---\n'; cat /etc/os-release 2>/dev/null || true
    printf '\n--- cpu ---\n'; (lscpu 2>/dev/null || sysctl -a 2>/dev/null | grep -E 'machdep.cpu|hw.ncpu' || true)
    printf '\n--- memory ---\n'; (free -h 2>/dev/null || vm_stat 2>/dev/null || true)
    printf '\n--- tools ---\n'
    cmake --version 2>&1 || true; ctest --version 2>&1 || true
    ninja --version 2>&1 || true
    for compiler in "${P610_GCC_CXX:-g++}" "${P610_CLANG_CXX:-clang++}" \
      "${P610_OLDEST_GCC_CXX:-}" "${P610_OLDEST_CLANG_CXX:-}"; do
      [[ -n "$compiler" ]] || continue
      printf '\n[%s]\n' "$compiler"; "$compiler" --version 2>&1 || true
    done
    printf '\n--- selected environment ---\n'
    env | LC_ALL=C sort | grep -E '^(CC|CXX|CMAKE|P610_|YGOR_|ASAN_|UBSAN_|TSAN_)' || true
  } > "$file"
  digest=$(sha256_file "$file")
  printf '%s\t%s\t%s\t%s\t%s\n' "$(utc_now)" "$ordinal" "$PHASE" \
    "$relative" "$digest" >> "${OUTPUT_DIR}/invocations.tsv"
  if [[ ! -f "${OUTPUT_DIR}/environment.txt" ]]; then
    cp "$file" "${OUTPUT_DIR}/environment.txt"
  fi
}


register_profile_inventory() {
  local id="$1" benchmarks="${2:-false}"
  register_step "profile.${id}.configure" contracts "$id" true "configure ${id}"
  register_step "profile.${id}.build" contracts "$id" true \
    "build mesh Boolean qualification and benchmark targets"
  register_step "profile.${id}.contracts" contracts "$id" true \
    "run non-fuzz mesh Boolean tests and all P6.2-P6.10 gates"
  if [[ "$benchmarks" == true ]]; then
    register_step "profile.${id}.benchmark-mesh" contracts "$id" true \
      "run controlled B0-B8 mesh benchmark records"
    register_step "profile.${id}.benchmark-exact-arithmetic" contracts "$id" true \
      "run exact-arithmetic benchmark records"
  fi
}

register_required_inventory() {
  if [[ "$SMOKE" -eq 1 ]]; then
    register_profile_inventory gcc-current-debug
    register_profile_inventory clang-current-debug-libcxx
    register_step fuzz.smoke-valid fuzz gcc-current-debug true \
      "non-qualifying shortened valid-geometry allocation"
    return
  fi
  register_profile_inventory gcc-current-debug
  register_profile_inventory gcc-current-release true
  register_profile_inventory gcc-oldest-debug
  register_profile_inventory gcc-oldest-release
  register_profile_inventory clang-current-debug-libcxx
  register_profile_inventory clang-current-release-libstdcxx true
  register_profile_inventory clang-oldest-debug-libstdcxx
  register_profile_inventory clang-oldest-release-libcxx
  register_profile_inventory gcc-current-asan-ubsan
  register_profile_inventory clang-current-asan-ubsan-libcxx
  register_profile_inventory clang-current-tsan-libstdcxx
  register_profile_inventory gcc-current-libstdcxx-debug
  register_profile_inventory clang-current-libcxx-debug
  register_step matrix.primary-architecture.x86_64 contracts x86_64 true \
    "primary frozen x86-64 execution host"
  register_step matrix.architecture.gcc-current-debug-libstdcxx-aarch64-u64 \
    contracts aarch64 true "frozen GCC AArch64 toolchain execution"
  register_step matrix.architecture.clang-current-release-libcxx-aarch64-u32 \
    contracts aarch64 true "frozen Clang/libc++ AArch64 toolchain execution"
  register_step campaign.nondeferred-frozen-manifest contracts candidate true \
    "execute every non-deferred frozen candidate-manifest entry"
  for allocation in \
    gcc-asan-ubsan-valid gcc-asan-ubsan-invalid \
    clang-asan-ubsan-valid clang-asan-ubsan-invalid \
    clang-tsan-valid clang-tsan-invalid \
    operation-chain-unsanitized long-running-unsanitized; do
    register_step "fuzz.${allocation}" fuzz "$allocation" true \
      "frozen 24 CPU-hour fuzz allocation"
  done
}

run_profile() {
  local id="$1" cc="$2" cxx="$3" build_type="$4" sanitizer="$5" stdlib="$6" debuglib="$7"
  local build_dir="${WORK_DIR}/build-${id}" artifact_dir="${OUTPUT_DIR}/artifacts/${id}"
  local configure_step="profile.${id}.configure" build_step="profile.${id}.build" test_step="profile.${id}.contracts"
  local benchmark_mesh_step="profile.${id}.benchmark-mesh"
  local benchmark_arithmetic_step="profile.${id}.benchmark-exact-arithmetic"
  local -a args targets benchmark_prefix
  if ! command -v "$cc" >/dev/null 2>&1 || ! command -v "$cxx" >/dev/null 2>&1; then
    register_step "$configure_step" contracts "$id" true "configure required compiler profile"
    append_anomaly "missing_configuration" "$configure_step" 0 \
      "required compiler commands unavailable: CC=${cc}, CXX=${cxx}" "environment.txt" ""
    write_status "$configure_step" blocked 0 127 ""
    return 1
  fi
  mkdir -p "$build_dir" "$artifact_dir"
  register_step "$configure_step" contracts "$id" true "configure ${id}"
  args=(cmake -S "$REPO_ROOT" -B "$build_dir" -G Ninja
        -DBUILD_TESTING=ON -DYGOR_BUILD_BOOLEAN_TESTS=ON
        -DYGOR_BUILD_BOOLEAN_BENCHMARKS=ON -DYGOR_BOOLEAN_STRICT_FP=ON
        -DYGOR_BOOLEAN_TEST_TIER=qualification -DWITH_EIGEN=OFF
        -DWITH_GNU_GSL=OFF -DCMAKE_BUILD_TYPE="$build_type"
        -DCMAKE_C_COMPILER="$cc" -DCMAKE_CXX_COMPILER="$cxx")
  case "$sanitizer" in
    asan-ubsan) args+=(-DWITH_ASAN=ON) ;;
    tsan) args+=(-DWITH_TSAN=ON) ;;
    none) ;;
    *) fail "unknown sanitizer ${sanitizer}" ;;
  esac
  if [[ "$stdlib" == libcxx ]]; then
    args+=("-DCMAKE_CXX_FLAGS=${P610_LIBCXX_CXX_FLAGS:--stdlib=libc++}"
          "-DCMAKE_EXE_LINKER_FLAGS=${P610_LIBCXX_LINK_FLAGS:--stdlib=libc++}"
          "-DCMAKE_SHARED_LINKER_FLAGS=${P610_LIBCXX_LINK_FLAGS:--stdlib=libc++}")
  fi
  if [[ "$debuglib" == libstdcxx-debug ]]; then
    args+=("-DCMAKE_CXX_FLAGS=-D_GLIBCXX_DEBUG")
  elif [[ "$debuglib" == libcxx-debug ]]; then
    args+=("-DCMAKE_CXX_FLAGS=${P610_LIBCXX_CXX_FLAGS:--stdlib=libc++} ${P610_LIBCXX_DEBUG_FLAGS:--D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG}")
  fi
  run_step "$configure_step" infrastructure "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" \
    env CC="$cc" CXX="$cxx" "${args[@]}" || return 1

  targets=(
    Test_MeshesBooleanContract Test_MeshesBooleanProductContract
    Test_MeshesExactArithmetic Test_MeshesExactKernel Test_MeshesExactKernelProperties
    Test_MeshesBooleanVerification Test_MeshesBooleanTransaction
    Test_MeshesBooleanExecutor Test_MeshesBooleanInputTopology
    Test_MeshesBooleanInputTopologyProperties Test_MeshesBooleanPreparation
    Test_MeshesBooleanNormalization Test_MeshesBooleanBroadPhase
    Test_MeshesBooleanBroadPhaseProperties Test_MeshesBooleanIntersectionEvents
    Test_MeshesBooleanIntersectionEventsProperties
    Test_MeshesBooleanSymbolicRegistry Test_MeshesBooleanSymbolicRegistryProperties
    Test_MeshesBooleanLocalRefinement Test_MeshesBooleanLocalRefinementProperties
    Test_MeshesBooleanGlobalArrangement Test_MeshesBooleanGlobalArrangementProperties
    Test_MeshesBooleanCellClassification Test_MeshesBooleanCellClassificationProperties
    Test_MeshesBooleanCellClassificationAdversarial Test_MeshesBooleanSelection
    Test_MeshesBooleanSelectionProperties Test_MeshesBooleanSelectionAdversarial
    Test_MeshesBooleanExactResult Test_MeshesBooleanBackend
    Test_MeshesBooleanRealization Test_MeshesBooleanRealizationProperties
    Test_MeshesBooleanApproximate Test_MeshesBooleanAttributes
    Test_MeshesBooleanService Test_MeshesBooleanQualificationSchemas
    Test_MeshesBooleanApproximateFixtureGenerator
    Test_MeshesBooleanApproximateVerifierIsolation Test_MeshesBooleanOutput
    Test_MeshesBooleanOutputProperties Test_MeshesBooleanEndToEnd
    Test_MeshesBooleanMetamorphic MeshBooleanReplay Test_MeshesBooleanFuzz
    Test_MeshesBooleanMutation MeshBooleanQualification
    Test_MeshesBooleanQualificationGeneration
    Test_MeshesBooleanQualificationComparison
    Test_MeshesBooleanQualificationAccounting
    Test_MeshesBooleanQualificationIngestion Test_MeshesBooleanQualificationSuites
    Test_MeshesBooleanQualificationMatrix Test_MeshesBooleanQualificationPerformance
    Test_MeshesBooleanQualificationCandidate Test_MeshesBooleanPerformanceBaselines
    MeshBooleanExample MeshBooleanExpertExample Test_MeshesBooleanPlanGaps
    MeshBooleanBenchmark
  )
  register_step "$build_step" contracts "$id" true "build mesh Boolean qualification and benchmark targets"
  run_step "$build_step" infrastructure "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" \
    cmake --build "$build_dir" --parallel "$JOBS" --target "${targets[@]}" || return 1

  register_step "$test_step" contracts "$id" true "run non-fuzz mesh Boolean tests and all P6.2-P6.10 gates"
  run_step "$test_step" test "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" \
    env YGOR_BOOLEAN_TEST_TIER=qualification YGOR_BOOLEAN_ARTIFACT_DIR="$artifact_dir" \
    ctest --test-dir "$build_dir" --output-on-failure -L mesh_boolean -LE fuzz \
      --timeout "$STEP_TIMEOUT_SECONDS" || return 1

  if [[ "$id" == gcc-current-release || "$id" == clang-current-release-libstdcxx ]]; then
    benchmark_prefix=()
    if [[ -n "${P610_BENCHMARK_CPUSET:-}" ]]; then
      if ! command -v taskset >/dev/null 2>&1; then
        append_anomaly missing_configuration "$benchmark_mesh_step" 0 \
          "P610_BENCHMARK_CPUSET was set but taskset is unavailable" environment.txt ""
        write_status "$benchmark_mesh_step" blocked 0 127 ""
        return 1
      fi
      benchmark_prefix=(taskset -c "$P610_BENCHMARK_CPUSET")
    fi
    register_step "$benchmark_mesh_step" contracts "$id" true \
      "run controlled B0-B8 mesh benchmark records"
    run_step "$benchmark_mesh_step" performance "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" \
      "${benchmark_prefix[@]}" "$build_dir/bin/MeshBooleanBenchmark" \
      --fixture all --size 1 --operation all --verification mandatory \
      --type double-u32 --threads 1 --warmup 2 --repetitions 11 || return 1

    register_step "$benchmark_arithmetic_step" contracts "$id" true \
      "run exact-arithmetic benchmark records"
    run_step "$benchmark_arithmetic_step" performance "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" \
      "${benchmark_prefix[@]}" "$build_dir/bin/MeshBooleanBenchmark" \
      --suite exact-arithmetic --arithmetic-case all --limbs 32 \
      --warmup 2 --repetitions 11 || return 1
  fi
}

run_primary_architecture_check() {
  local step_id=matrix.primary-architecture.x86_64 arch
  arch=$(uname -m)
  if [[ "$arch" == x86_64 || "$arch" == amd64 ]]; then
    write_status "$step_id" pass 0 0 "environment.txt"
    return 0
  fi
  append_anomaly missing_configuration "$step_id" 0 \
    "the default frozen matrix requires an x86-64 primary host; observed ${arch}" \
    "environment.txt" ""
  write_status "$step_id" blocked 0 127 "environment.txt"
  return 1
}

run_independent_architecture_case() {
  local case_id="$1" compiler_family="$2" standard_library="$3"
  local build_type="$4" coordinate="$5" index="$6"
  local step_id="matrix.architecture.${case_id}"
  local command="${P610_INDEPENDENT_ARCH_COMMAND:-}"
  if [[ -z "$command" ]]; then
    append_anomaly missing_configuration "$step_id" 0 \
      "set P610_INDEPENDENT_ARCH_COMMAND to execute frozen AArch64 case ${case_id}" \
      "environment.txt" "" "$case_id"
    write_status "$step_id" blocked 0 127 ""
    return 1
  fi
  if ! run_step "$step_id" test "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" \
      env P610_REQUIRED_ARCHITECTURE=aarch64 P610_ARCH_CASE_ID="$case_id" \
        P610_REQUIRED_COMPILER_FAMILY="$compiler_family" \
        P610_REQUIRED_STANDARD_LIBRARY="$standard_library" \
        P610_REQUIRED_BUILD_TYPE="$build_type" \
        P610_REQUIRED_COORDINATE="$coordinate" P610_REQUIRED_INDEX="$index" \
        P610_REPO_ROOT="$REPO_ROOT" P610_INDEPENDENT_ARCH_COMMAND="$command" \
        bash -lc 'cd "$P610_REPO_ROOT" && bash -lc "$P610_INDEPENDENT_ARCH_COMMAND"'; then
    return 1
  fi
  if ! awk -F'\t' -v case_id="$case_id" 'NR>1 && $3==case_id {found=1} END{exit !found}' \
      "${OUTPUT_DIR}/observations.tsv"; then
    append_anomaly missing_evidence "$step_id" 0 \
      "AArch64 command passed without a P610_OBSERVATION marker for ${case_id}" \
      "" "" "$case_id"
    write_status "$step_id" blocked 0 126 ""
    return 1
  fi
}

run_nondeferred_frozen_manifest() {
  local step_id=campaign.nondeferred-frozen-manifest
  local command="${P610_NONDEFERRED_CAMPAIGN_COMMAND:-}"
  local artifact_dir="${OUTPUT_DIR}/artifacts/nondeferred-frozen-manifest"
  if [[ -z "$command" ]]; then
    append_anomaly missing_configuration "$step_id" 0 \
      "set P610_NONDEFERRED_CAMPAIGN_COMMAND to execute the actual frozen non-deferred candidate inventory; checker-only CTest runs are not campaign evidence" \
      "environment.txt" ""
    write_status "$step_id" blocked 0 127 ""
    return 1
  fi
  mkdir -p "$artifact_dir"
  if ! run_step "$step_id" test "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" \
      env P610_REPO_ROOT="$REPO_ROOT" P610_NONDEFERRED_OUTPUT_DIR="$artifact_dir" \
        P610_NONDEFERRED_CAMPAIGN_COMMAND="$command" \
        bash -lc '
          set -u
          set -o pipefail
          cd "$P610_REPO_ROOT" || exit 127
          bash -lc "$P610_NONDEFERRED_CAMPAIGN_COMMAND" || exit $?
          completion="$P610_NONDEFERRED_OUTPUT_DIR/completion.tsv"
          [[ -f "$completion" ]] || { echo "missing completion.tsv" >&2; exit 126; }
          awk -F "\t" '\''$1=="status" && $2=="complete" {ok=1}
            $1=="executed_case_count" && $2+0>0 {cases=1}
            $1=="manifest_digest" && $2 ~ /^[0-9a-fA-F]+$/ && length($2)==64 {manifest=1}
            $1=="observation_digest" && $2 ~ /^[0-9a-fA-F]+$/ && length($2)==64 {observations=1}
            END {exit !(ok && cases && manifest && observations)}'\'' "$completion" || {
              echo "malformed or incomplete completion.tsv" >&2; exit 126;
            }
        '; then
    return 1
  fi
  local expected observed
  expected=$(awk -F'\t' '$1=="executed_case_count"{print $2}' "$artifact_dir/completion.tsv")
  observed=$(awk -F'\t' -v step="$step_id" 'NR>1 && $2==step {!seen[$3]++; if (seen[$3]==1) n++} END{print n+0}' \
    "${OUTPUT_DIR}/observations.tsv")
  if (( observed != expected )); then
    append_anomaly missing_evidence "$step_id" 0 \
      "completion.tsv claims ${expected} cases but ${observed} distinct P610_OBSERVATION cases were retained" \
      "artifacts/nondeferred-frozen-manifest/completion.tsv" ""
    write_status "$step_id" blocked 0 126 \
      "artifacts/nondeferred-frozen-manifest/completion.tsv"
    return 1
  fi
}

run_contracts() {
  local gcc_cc="${P610_GCC_CC:-gcc}" gcc_cxx="${P610_GCC_CXX:-g++}"
  local clang_cc="${P610_CLANG_CC:-clang}" clang_cxx="${P610_CLANG_CXX:-clang++}"
  local oldest_gcc_cc="${P610_OLDEST_GCC_CC:-}" oldest_gcc_cxx="${P610_OLDEST_GCC_CXX:-}"
  local oldest_clang_cc="${P610_OLDEST_CLANG_CC:-}" oldest_clang_cxx="${P610_OLDEST_CLANG_CXX:-}"
  local failed=0
  if [[ "$SMOKE" -eq 1 ]]; then
    run_profile gcc-current-debug "$gcc_cc" "$gcc_cxx" Debug none libstdcxx none || failed=1
    run_profile clang-current-debug-libcxx "$clang_cc" "$clang_cxx" Debug none libcxx none || failed=1
    return "$failed"
  fi
  run_primary_architecture_check || failed=1
  run_profile gcc-current-debug "$gcc_cc" "$gcc_cxx" Debug none libstdcxx none || failed=1
  run_profile gcc-current-release "$gcc_cc" "$gcc_cxx" Release none libstdcxx none || failed=1
  run_profile gcc-oldest-debug "$oldest_gcc_cc" "$oldest_gcc_cxx" Debug none libstdcxx none || failed=1
  run_profile gcc-oldest-release "$oldest_gcc_cc" "$oldest_gcc_cxx" Release none libstdcxx none || failed=1
  run_profile clang-current-debug-libcxx "$clang_cc" "$clang_cxx" Debug none libcxx none || failed=1
  run_profile clang-current-release-libstdcxx "$clang_cc" "$clang_cxx" Release none libstdcxx none || failed=1
  run_profile clang-oldest-debug-libstdcxx "$oldest_clang_cc" "$oldest_clang_cxx" Debug none libstdcxx none || failed=1
  run_profile clang-oldest-release-libcxx "$oldest_clang_cc" "$oldest_clang_cxx" Release none libcxx none || failed=1
  run_profile gcc-current-asan-ubsan "$gcc_cc" "$gcc_cxx" Debug asan-ubsan libstdcxx none || failed=1
  run_profile clang-current-asan-ubsan-libcxx "$clang_cc" "$clang_cxx" Debug asan-ubsan libcxx none || failed=1
  run_profile clang-current-tsan-libstdcxx "$clang_cc" "$clang_cxx" Debug tsan libstdcxx none || failed=1
  run_profile gcc-current-libstdcxx-debug "$gcc_cc" "$gcc_cxx" Debug none libstdcxx libstdcxx-debug || failed=1
  run_profile clang-current-libcxx-debug "$clang_cc" "$clang_cxx" Debug none libcxx libcxx-debug || failed=1
  run_independent_architecture_case \
    gcc-current-debug-libstdcxx-aarch64-u64 gcc libstdcxx Debug binary64 uint64 || failed=1
  run_independent_architecture_case \
    clang-current-release-libcxx-aarch64-u32 clang libcxx Release binary32 uint32 || failed=1
  run_nondeferred_frozen_manifest || failed=1
  return "$failed"
}

default_fuzz_command() {
  local family="$1" build_dir="$2" regex
  case "$family" in
    valid) regex='^MeshBoolean\.(Fuzz|EndToEnd|Metamorphic)$' ;;
    invalid) regex='^MeshBoolean\.(QualificationGeneration|Preparation|Normalization|Mutation)$' ;;
    chain) regex='^MeshBoolean\.(QualificationGeneration|EndToEnd|Metamorphic)$' ;;
    long) regex='^MeshBoolean\.(EndToEnd|QualificationPerformance|PerformanceBaselines)$' ;;
    *) fail "unknown fuzz family ${family}" ;;
  esac
  printf 'ctest --test-dir %q --output-on-failure -R %q --timeout %q' \
    "$build_dir" "$regex" "$STEP_TIMEOUT_SECONDS"
}

fuzz_cpu_total() {
  local file="$1"
  [[ -f "$file" ]] || { printf '0\n'; return; }
  awk -F'\t' 'NR>1 && $3==0 {sum += $4 + $5} END {printf "%.6f\n", sum+0}' "$file"
}

run_fuzz_allocation() {
  local id="$1" profile="$2" family="$3" override_var="$4"
  local build_dir="${WORK_DIR}/build-${profile}" progress="${OUTPUT_DIR}/fuzz-progress/${id}.tsv"
  local step_id="fuzz.${id}" command total attempt time_file log_file exit_code
  local user_cpu system_cpu elapsed max_rss chunk_ordinal command_digest command_file existing_digest
  register_step "$step_id" fuzz "$profile" true "24 CPU-hour frozen ${family} allocation"
  if [[ ! -d "$build_dir" ]]; then
    append_anomaly missing_configuration "$step_id" 0 \
      "required build profile is unavailable: ${profile}; run --phase contracts first" "" ""
    write_status "$step_id" blocked 0 127 ""
    return 1
  fi
  [[ -f "$progress" ]] || printf 'utc\tchunk\texit_code\tuser_cpu_seconds\tsystem_cpu_seconds\telapsed_seconds\tmax_rss_kib\tlog_file\tcommand_file\tcommand_digest\n' > "$progress"
  command="${!override_var:-}"
  [[ -n "$command" ]] || command=$(default_fuzz_command "$family" "$build_dir")
  command_digest=$(printf '%s\n' "$command" | { sha256sum 2>/dev/null || shasum -a 256; } | awk '{print $1}')
  existing_digest=$(awk -F'\t' 'NR>1 {print $10}' "$progress" | LC_ALL=C sort -u | sed '/^$/d')
  if [[ -n "$existing_digest" && "$existing_digest" != "$command_digest" ]]; then
    append_anomaly configuration_mismatch "$step_id" 0 \
      "fuzz command changed after duration accounting began; existing=${existing_digest}, current=${command_digest}" \
      "fuzz-progress/${id}.tsv" "" "$id"
    write_status "$step_id" blocked 0 126 "fuzz-progress/${id}.tsv"
    return 1
  fi
  total=$(fuzz_cpu_total "$progress")
  if awk -v total="$total" -v target="$FUZZ_CPU_SECONDS" 'BEGIN{exit !(total>=target)}'; then
    write_status "$step_id" pass 0 0 "fuzz-progress/${id}.tsv"
    printf 'SKIP %s (CPU target already met: %s)\n' "$step_id" "$total"
    return 0
  fi
  command_file="commands/${step_id}.${command_digest}.sh"
  if [[ ! -f "${OUTPUT_DIR}/${command_file}" ]]; then
    printf '%s\n' "$command" > "${OUTPUT_DIR}/${command_file}"
    chmod +x "${OUTPUT_DIR}/${command_file}"
  fi

  while ! awk -v total="$total" -v target="$FUZZ_CPU_SECONDS" 'BEGIN{exit !(total>=target)}'; do
    chunk_ordinal=$(( $(awk 'END{print NR-1}' "$progress") + 1 ))
    attempt="$chunk_ordinal"
    log_file="logs/${step_id}/chunk-${chunk_ordinal}.log"
    time_file="${OUTPUT_DIR}/logs/${step_id}/chunk-${chunk_ordinal}.time"
    mkdir -p "${OUTPUT_DIR}/logs/${step_id}" "${OUTPUT_DIR}/artifacts/${id}"
    CURRENT_STEP="$step_id"; CURRENT_ATTEMPT="$attempt"
    write_status "$step_id" running "$attempt" 0 "$log_file"
    printf 'RUN  %s chunk %s (CPU %s/%s)\n' "$step_id" "$chunk_ordinal" "$total" "$FUZZ_CPU_SECONDS"
    set +e
    /usr/bin/time -f '%U\t%S\t%e\t%M' -o "$time_file" \
      timeout --signal=TERM --kill-after=30s "$FUZZ_CHUNK_WALL_SECONDS" \
      env P610_BUILD_DIR="$build_dir" P610_ALLOCATION_ID="$id" \
        P610_CHUNK_ORDINAL="$chunk_ordinal" YGOR_BOOLEAN_TEST_TIER=qualification \
        YGOR_BOOLEAN_ARTIFACT_DIR="${OUTPUT_DIR}/artifacts/${id}" \
        P610_FUZZ_INNER_COMMAND="$command" \
        P610_FUZZ_CASES_PER_RUN="${P610_FUZZ_CASES_PER_RUN:-128}" \
        P610_FUZZ_RUNS_PER_CHUNK="${P610_FUZZ_RUNS_PER_CHUNK:-16}" \
        P610_REPO_ROOT="$REPO_ROOT" \
        bash -lc '
          cd "$P610_REPO_ROOT" || exit 127
          iteration=0
          while (( iteration < P610_FUZZ_RUNS_PER_CHUNK )); do
            export P610_ITERATION="$iteration"
            export YGOR_BOOLEAN_GENERATED_CASES="$P610_FUZZ_CASES_PER_RUN"
            export YGOR_BOOLEAN_SEED_HIGH=$(printf "0x%016x" $((0x59474f52 ^ P610_CHUNK_ORDINAL ^ iteration)))
            export YGOR_BOOLEAN_SEED_LOW=$(printf "0x%016x" $((0x50363130 ^ (P610_CHUNK_ORDINAL * 0x9e37) ^ (iteration * 0x85eb))))
            bash -lc "$P610_FUZZ_INNER_COMMAND" || exit $?
            iteration=$((iteration + 1))
          done
        ' > "${OUTPUT_DIR}/${log_file}" 2>&1
    exit_code=$?
    user_cpu=0; system_cpu=0; elapsed=0; max_rss=0
    [[ -s "$time_file" ]] && IFS=$'\t' read -r user_cpu system_cpu elapsed max_rss < "$time_file" || true
    # A chunk has a finite iteration count. Reaching timeout is an anomaly, not
    # successful duration accounting.
    if [[ "$exit_code" -eq 0 ]] && awk -v u="$user_cpu" -v s="$system_cpu" 'BEGIN{exit !((u+s)<=0)}'; then
      exit_code=125
      printf 'P6.10 driver: completed chunk had no measurable CPU time.\n' >> "${OUTPUT_DIR}/${log_file}"
    fi
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$(utc_now)" "$chunk_ordinal" "$exit_code" "$user_cpu" "$system_cpu" \
      "$elapsed" "$max_rss" "$log_file" "$command_file" "$command_digest" \
      >> "$progress"
    printf '%s\t%s\t%s\tfuzz\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$(utc_now)" "$step_id" "$attempt" "$exit_code" "$elapsed" "$user_cpu" \
      "$system_cpu" "$max_rss" "$command_file" "$log_file" \
      >> "${OUTPUT_DIR}/attempts.tsv"
    record_explicit_records "$step_id" "$attempt" "$log_file"
    if [[ "$exit_code" -ne 0 ]]; then
      write_status "$step_id" fail "$attempt" "$exit_code" "$log_file"
      record_known_anomalies "$step_id" "$attempt" "$exit_code" "$log_file"
      CURRENT_STEP=""; CURRENT_ATTEMPT=0
      return 1
    fi
    total=$(fuzz_cpu_total "$progress")
    CURRENT_STEP=""; CURRENT_ATTEMPT=0
  done
  write_status "$step_id" pass "$attempt" 0 "fuzz-progress/${id}.tsv"
}

run_fuzz() {
  local failed=0
  if [[ "$SMOKE" -eq 1 ]]; then
    run_fuzz_allocation smoke-valid gcc-current-debug valid P610_FUZZ_GCC_ASAN_VALID_COMMAND || failed=1
    return "$failed"
  fi
  run_fuzz_allocation gcc-asan-ubsan-valid gcc-current-asan-ubsan valid P610_FUZZ_GCC_ASAN_VALID_COMMAND || failed=1
  run_fuzz_allocation gcc-asan-ubsan-invalid gcc-current-asan-ubsan invalid P610_FUZZ_GCC_ASAN_INVALID_COMMAND || failed=1
  run_fuzz_allocation clang-asan-ubsan-valid clang-current-asan-ubsan-libcxx valid P610_FUZZ_CLANG_ASAN_VALID_COMMAND || failed=1
  run_fuzz_allocation clang-asan-ubsan-invalid clang-current-asan-ubsan-libcxx invalid P610_FUZZ_CLANG_ASAN_INVALID_COMMAND || failed=1
  run_fuzz_allocation clang-tsan-valid clang-current-tsan-libstdcxx valid P610_FUZZ_CLANG_TSAN_VALID_COMMAND || failed=1
  run_fuzz_allocation clang-tsan-invalid clang-current-tsan-libstdcxx invalid P610_FUZZ_CLANG_TSAN_INVALID_COMMAND || failed=1
  run_fuzz_allocation operation-chain-unsanitized gcc-current-release chain P610_FUZZ_OPERATION_CHAIN_COMMAND || failed=1
  run_fuzz_allocation long-running-unsanitized gcc-current-release long P610_FUZZ_LONG_RUNNING_COMMAND || failed=1
  return "$failed"
}

finalize_outputs() {
  local required=0 passed=0 failed=0 blocked=0 running=0 unresolved=0 file status
  local campaign_dirty=true campaign_smoke=true
  if [[ -f "${OUTPUT_DIR}/campaign.tsv" ]]; then
    campaign_dirty=$(awk -F'\t' '$1=="repository_dirty"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
    campaign_smoke=$(awk -F'\t' '$1=="smoke"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  fi
  while IFS=$'\t' read -r step_id _ _ is_required _; do
    [[ "$step_id" == step_id ]] && continue
    [[ "$is_required" == true ]] && required=$((required + 1))
    status=$(status_value "$step_id" || true)
    case "$status" in
      pass) passed=$((passed + 1)) ;;
      fail) failed=$((failed + 1)) ;;
      blocked) blocked=$((blocked + 1)) ;;
      running) running=$((running + 1)) ;;
      *) blocked=$((blocked + 1)) ;;
    esac
  done < "${OUTPUT_DIR}/steps.tsv"
  unresolved=$(awk -F'\t' 'FNR==NR {if (FNR>1) resolved[$2]=1; next} FNR>1 && $7=="unresolved" && !resolved[$2] {n++} END{print n+0}' "${OUTPUT_DIR}/resolutions.tsv" "${OUTPUT_DIR}/anomalies.tsv")
  {
    printf 'schema\t1\n'
    printf 'campaign_id\t%s\n' "$CAMPAIGN_ID"
    printf 'finalized_utc\t%s\n' "$(utc_now)"
    printf 'required_steps\t%s\npassed_steps\t%s\nfailed_steps\t%s\nblocked_steps\t%s\nrunning_steps\t%s\n' \
      "$required" "$passed" "$failed" "$blocked" "$running"
    printf 'unresolved_anomalies\t%s\n' "$unresolved"
    if (( required > 0 && passed == required && failed == 0 && blocked == 0 && running == 0 && unresolved == 0 )) &&
       [[ "$campaign_dirty" == false && "$campaign_smoke" == 0 ]]; then
      printf 'campaign_status\tcomplete_candidate_evidence\n'
    else
      printf 'campaign_status\tincomplete_blocking\n'
    fi
  } > "${OUTPUT_DIR}/summary.tsv"
  {
    printf '# P6.10 candidate campaign evidence\n\n'
    printf 'Campaign: `%s`  \n' "$CAMPAIGN_ID"
    printf 'Finalized: `%s`  \n' "$(utc_now)"
    printf 'Status: `%s`\n\n' "$(awk -F'\t' '$1=="campaign_status"{print $2}' "${OUTPUT_DIR}/summary.tsv")"
    printf 'Evaluate this directory using `docs/MeshBooleanP610ManualCampaign.md`. '
    printf 'Do not mark P6.10 or P6.11 complete while `summary.tsv` reports '
    printf '`incomplete_blocking` or `anomalies.tsv` contains unresolved rows.\n'
  } > "${OUTPUT_DIR}/README.md"
  rm -f "${OUTPUT_DIR}/SHA256SUMS"
  (
    cd "$OUTPUT_DIR"
    find . -type f ! -path './SHA256SUMS' ! -path './.lock/*' -print0 \
      | LC_ALL=C sort -z \
      | while IFS= read -r -d '' file; do
          if command -v sha256sum >/dev/null 2>&1; then sha256sum "$file"; else shasum -a 256 "$file"; fi
        done > SHA256SUMS
  )
  printf 'Finalized evidence: %s\n' "$OUTPUT_DIR"
  [[ "$(awk -F'\t' '$1=="campaign_status"{print $2}' "${OUTPUT_DIR}/summary.tsv")" == complete_candidate_evidence ]]
}

validate_existing_campaign() {
  local recorded_target recorded_smoke recorded_version recorded_commit
  local recorded_tree recorded_dirty recorded_working current_dirty current_working
  recorded_target=$(awk -F'\t' '$1=="fuzz_cpu_seconds_per_allocation"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  recorded_smoke=$(awk -F'\t' '$1=="smoke"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  recorded_version=$(awk -F'\t' '$1=="script_version"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  recorded_commit=$(awk -F'\t' '$1=="repository_commit"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  recorded_tree=$(awk -F'\t' '$1=="repository_tree"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  recorded_dirty=$(awk -F'\t' '$1=="repository_dirty"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  recorded_working=$(awk -F'\t' '$1=="working_state_digest"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  [[ "$recorded_target" == "$FUZZ_CPU_SECONDS" ]] ||
    fail "existing campaign fuzz target is ${recorded_target}, not ${FUZZ_CPU_SECONDS}"
  [[ "$recorded_smoke" == "$SMOKE" ]] ||
    fail "existing campaign smoke flag is ${recorded_smoke}, not ${SMOKE}"
  [[ "$recorded_version" == "$SCRIPT_VERSION" ]] ||
    fail "existing campaign was created by driver version ${recorded_version}"
  [[ "$recorded_commit" == "$(git -C "$REPO_ROOT" rev-parse HEAD)" ]] ||
    fail 'repository commit changed; start a new evidence directory'
  [[ "$recorded_tree" == "$(git -C "$REPO_ROOT" rev-parse HEAD^{tree})" ]] ||
    fail 'repository tree changed; start a new evidence directory'
  current_dirty=$(git -C "$REPO_ROOT" status --porcelain=v1 --untracked-files=normal)
  current_working=$(
    { git -C "$REPO_ROOT" status --porcelain=v1 --untracked-files=all;
      git -C "$REPO_ROOT" diff --binary HEAD; } \
      | { sha256sum 2>/dev/null || shasum -a 256; } | awk '{print $1}'
  )
  [[ "$recorded_dirty" == "$([[ -n "$current_dirty" ]] && echo true || echo false)" ]] ||
    fail 'repository dirty state changed; start a new evidence directory'
  [[ "$recorded_working" == "$current_working" ]] ||
    fail 'repository working state changed; start a new evidence directory'
}

run_self_test() {
  local tmp pass_count before after
  tmp=$(mktemp -d)
  OUTPUT_DIR="$tmp/evidence"; WORK_DIR="$tmp/work"; REPO_ROOT=$(pwd)
  initialize_output
  CAMPAIGN_ID=self-test
  run_step self.pass test 1 30 bash -c 'printf "PASS\tself.pass\nP610_OBSERVATION\tself.case\tverified_exact_success\t\t\nSUMMARY\t1\t0\n"'
  before=$(wc -l < "${OUTPUT_DIR}/attempts.tsv")
  run_step self.pass test 1 30 bash -c 'exit 99'
  after=$(wc -l < "${OUTPUT_DIR}/attempts.tsv")
  [[ "$before" -eq "$after" ]] || fail 'self-test resume duplicated a passed step'
  grep -q $'\tself.case\tverified_exact_success\t' "${OUTPUT_DIR}/observations.tsv" || fail 'self-test observation protocol failed'
  run_step self.fail test 1 30 bash -c 'printf "FAIL\tself.fail\tnondeterministic result\n" >&2; exit 1' || true
  grep -q $'\tnondeterministic_result\t' "${OUTPUT_DIR}/anomalies.tsv" || fail 'self-test anomaly classification failed'
  register_step self.pass contracts self true 'self-test passing step'
  register_step self.fail contracts self true 'self-test failing step'
  finalize_outputs || true
  [[ -s "${OUTPUT_DIR}/SHA256SUMS" ]] || fail 'self-test checksum generation failed'
  pass_count=$(awk -F'\t' '$1=="passed_steps"{print $2}' "${OUTPUT_DIR}/summary.tsv")
  [[ "$pass_count" -eq 1 ]] || fail 'self-test summary failed'
  cleanup_lock
  OUTPUT_DIR="$tmp/inventory"; SMOKE=0; initialize_output
  register_required_inventory
  [[ "$(awk 'END{print NR-1}' "${OUTPUT_DIR}/steps.tsv")" -eq 55 ]] ||
    fail 'self-test full required inventory count changed'
  [[ "$(awk -F'\t' 'NR>1 && $2=="fuzz"{n++} END{print n+0}' "${OUTPUT_DIR}/steps.tsv")" -eq 8 ]] ||
    fail 'self-test frozen fuzz inventory count changed'
  cleanup_lock; rm -rf "$tmp"
  printf 'P6.10 campaign driver self-test passed.\n'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --output) [[ $# -ge 2 ]] || fail '--output requires DIR'; OUTPUT_DIR="$2"; shift 2 ;;
    --work) [[ $# -ge 2 ]] || fail '--work requires DIR'; WORK_DIR="$2"; shift 2 ;;
    --phase) [[ $# -ge 2 ]] || fail '--phase requires NAME'; PHASE="$2"; shift 2 ;;
    --jobs) [[ $# -ge 2 ]] || fail '--jobs requires N'; JOBS="$2"; shift 2 ;;
    --max-attempts) [[ $# -ge 2 ]] || fail '--max-attempts requires N'; MAX_ATTEMPTS="$2"; shift 2 ;;
    --timeout-seconds) [[ $# -ge 2 ]] || fail '--timeout-seconds requires N'; STEP_TIMEOUT_SECONDS="$2"; shift 2 ;;
    --fuzz-cpu-seconds) [[ $# -ge 2 ]] || fail '--fuzz-cpu-seconds requires N'; FUZZ_CPU_SECONDS="$2"; shift 2 ;;
    --fuzz-chunk-seconds) [[ $# -ge 2 ]] || fail '--fuzz-chunk-seconds requires N'; FUZZ_CHUNK_WALL_SECONDS="$2"; shift 2 ;;
    --allow-dirty) ALLOW_DIRTY=1; shift ;;
    --smoke) SMOKE=1; shift ;;
    --self-test) SELF_TEST=1; shift ;;
    --record-anomaly)
      [[ $# -ge 4 ]] || fail '--record-anomaly requires CATEGORY CASE MESSAGE [EVIDENCE]'
      RECORD_ONLY=1; RECORD_CATEGORY="$2"; RECORD_CASE="$3"; RECORD_MESSAGE="$4"
      RECORD_EVIDENCE="${5:-}"; shift $(( $# >= 5 ? 5 : 4 )) ;;
    --resolve-anomaly)
      [[ $# -ge 5 ]] || fail '--resolve-anomaly requires ID REVIEWER RATIONALE EVIDENCE'
      RESOLVE_ONLY=1; RESOLVE_ID="$2"; RESOLVE_REVIEWER="$3"
      RESOLVE_RATIONALE="$4"; RESOLVE_EVIDENCE="$5"; shift 5 ;;
    -h|--help) usage; exit 0 ;;
    *) fail "unknown argument: $1" ;;
  esac
done

[[ "$SELF_TEST" -eq 0 ]] || { run_self_test; exit 0; }
(( RECORD_ONLY + RESOLVE_ONLY <= 1 )) || fail 'record and resolve modes are mutually exclusive'
for value in "$JOBS" "$MAX_ATTEMPTS" "$STEP_TIMEOUT_SECONDS" "$FUZZ_CPU_SECONDS" "$FUZZ_CHUNK_WALL_SECONDS"; do
  is_uint "$value" || fail "numeric option expected, got: ${value}"
done
(( JOBS > 0 && MAX_ATTEMPTS > 0 && STEP_TIMEOUT_SECONDS > 0 && FUZZ_CHUNK_WALL_SECONDS > 0 )) || fail 'numeric options must be positive'
case "$PHASE" in all|contracts|fuzz|finalize) ;; *) fail "invalid phase: ${PHASE}" ;; esac
if (( FUZZ_CPU_SECONDS < QUALIFICATION_FUZZ_CPU_SECONDS && SMOKE == 0 )); then
  fail "fuzz CPU target below ${QUALIFICATION_FUZZ_CPU_SECONDS}; use --smoke for non-qualifying runs"
fi
[[ -n "$OUTPUT_DIR" ]] || fail '--output DIR is required'
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR=$(cd "$OUTPUT_DIR" && pwd)
WORK_DIR="${WORK_DIR:-${OUTPUT_DIR}.work}"
mkdir -p "$WORK_DIR"; WORK_DIR=$(cd "$WORK_DIR" && pwd)
REPO_ROOT=$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel 2>/dev/null) || fail 'script must run from a Git checkout'
initialize_output

if [[ "$RESOLVE_ONLY" -eq 1 ]]; then
  [[ "$RESOLVE_EVIDENCE" != /* && "$RESOLVE_EVIDENCE" != *'..'* ]] ||
    fail 'resolution evidence path must be relative and may not contain ..'
  evidence_path="${OUTPUT_DIR}/${RESOLVE_EVIDENCE}"
  [[ -f "$evidence_path" ]] || fail "resolution evidence is not inside output directory: ${RESOLVE_EVIDENCE}"
  awk -F'\t' -v id="$RESOLVE_ID" 'NR>1 && $2==id {found=1} END{exit !found}' \
    "${OUTPUT_DIR}/anomalies.tsv" || fail "unknown anomaly id: ${RESOLVE_ID}"
  if ! awk -F'\t' -v id="$RESOLVE_ID" 'NR>1 && $2==id {found=1} END{exit !found}' \
      "${OUTPUT_DIR}/resolutions.tsv"; then
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$(utc_now)" "$RESOLVE_ID" \
      "$(sanitize_tsv "$RESOLVE_REVIEWER")" "$(sanitize_tsv "$RESOLVE_RATIONALE")" \
      "$(sanitize_tsv "$RESOLVE_EVIDENCE")" "$(sha256_file "$evidence_path")" \
      >> "${OUTPUT_DIR}/resolutions.tsv"
  fi
  [[ -f "${OUTPUT_DIR}/campaign.tsv" ]] && CAMPAIGN_ID=$(awk -F'\t' '$1=="campaign_id"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  finalize_outputs || true
  exit 0
fi

if [[ "$RECORD_ONLY" -eq 1 ]]; then
  [[ -f "${OUTPUT_DIR}/campaign.tsv" ]] && CAMPAIGN_ID=$(awk -F'\t' '$1=="campaign_id"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
  if [[ -n "$RECORD_EVIDENCE" ]]; then
    [[ "$RECORD_EVIDENCE" != /* && "$RECORD_EVIDENCE" != *'..'* && -f "${OUTPUT_DIR}/${RECORD_EVIDENCE}" ]] ||
      fail 'manual anomaly evidence must be an existing relative path inside the output directory'
  fi
  append_anomaly "$RECORD_CATEGORY" manual 0 "$RECORD_MESSAGE" "$RECORD_EVIDENCE" "" "$RECORD_CASE"
  finalize_outputs || true
  exit 0
fi

for tool in git cmake ctest ninja awk grep sed sort find; do require_tool "$tool"; done
[[ -x /usr/bin/time ]] || fail 'GNU /usr/bin/time is required'
require_tool timeout
for value in "${P610_FUZZ_RUNS_PER_CHUNK:-16}" "${P610_FUZZ_CASES_PER_RUN:-128}"; do
  is_uint "$value" && (( value > 0 )) || fail 'fuzz run/count environment values must be positive integers'
done
if [[ ! -f "${OUTPUT_DIR}/campaign.tsv" ]]; then
  capture_environment
else
  validate_existing_campaign
  CAMPAIGN_ID=$(awk -F'\t' '$1=="campaign_id"{print $2}' "${OUTPUT_DIR}/campaign.tsv")
fi
register_required_inventory
capture_invocation_environment

result=0
case "$PHASE" in
  all) run_contracts || result=1; run_fuzz || result=1; finalize_outputs || result=1 ;;
  contracts) run_contracts || result=1; finalize_outputs || result=1 ;;
  fuzz) run_fuzz || result=1; finalize_outputs || result=1 ;;
  finalize) finalize_outputs || result=1 ;;
esac
exit "$result"
