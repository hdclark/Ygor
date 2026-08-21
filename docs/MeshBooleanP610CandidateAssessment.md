# P6.10 retained candidate assessment

## Decision

The retained campaign `p610-b8427a7a70dc-b9fb5f16437d-x86_64` is rejected as
qualification evidence. It does not complete P6.10, cannot support P6.11
promotion, and qualifies no backend, result mode, preparation policy, workload,
compiler, standard library, or architecture profile.

This assessment is bound to repository commit
`b8427a7a70dc5605accd2606a27ba063e264e130` and tree
`94957b74ff212f0f25f96f23cba0640e877a34b4`. The retained `summary.tsv` reports
55 required steps, 18 passes, 14 failures, 23 blockers, 16 unresolved anomalies,
and `campaign_status=incomplete_blocking`.

## Blocking findings

- The host used CMake/CTest 3.13.4. Driver v4 passed multiple targets after one
  `cmake --build --target`; that tool accepts only one target there. All nine
  recorded profile builds failed before contract execution.
- Driver v4 used `ctest --test-dir`, which the retained CTest did not apply as
  intended. Retained smoke logs show `Test project /home/bot/Ygor` followed by
  `No tests were found!!!`. CTest returned success, so all eight apparent fuzz
  allocations credited empty invocations rather than tests.
- The eight ledgers each mechanically exceed 86,400 CPU-seconds, but none of
  that duration is accepted because no test execution was established.
- `P610_INDEPENDENT_ARCH_COMMAND` and
  `P610_NONDEFERRED_CAMPAIGN_COMMAND` were `/bin/true`. No AArch64 observation or
  non-deferred frozen-manifest completion record was produced.
- The recorded current and oldest compiler commands resolved to the same GCC 8
  and Clang 7 versions, so the required version matrix was not exercised.
- `observations.tsv` contains no observations, `resolutions.tsv` contains no
  resolutions, and every artifact directory is empty.
- The candidate checksum manifest names 4,114,113 files. Only 89 referenced
  files remain; 4,114,024 immutable log/time files are missing. The surviving
  referenced files match their recorded digests, but the manifest as a whole
  cannot verify.

The short `p610-smoke` run is independently non-qualifying and exhibits the same
zero-test and multi-target failures. Its retained logs are useful only as
diagnostic evidence for the driver defects.

## Disposition

Driver schema v5 uses the configured Ninja generator directly, runs CTest from
the selected build directory, rejects zero discovered tests, and blocks
finalization when immutable attempt evidence is missing. These changes prevent
the observed infrastructure failures from becoming passing duration in a future
campaign.

No anomaly from the retained campaign is marked resolved because there is no
successful controlled rerun or immutable resolution evidence. P6.10, P6.11,
all final release gates, and `qualified_default` remain unchanged. Additional
end-user beta testing may provide useful defect evidence but does not
retroactively satisfy the frozen Plan 16 campaign.
