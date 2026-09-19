# Infrastructure and publishing

Run `bazel test //...` and `pre-commit run --all-files` before submitting infrastructure changes.
The Python infrastructure tests can also run without Bazel:

```sh
python3 -m unittest discover -s tools -p '*_test.py'
```

## Local clang-tidy

Build with `bazel build --config=clang-tidy //...`, then generate the compilation database with
`./compile_commands-update.sh`. The enforcing hook runs in report-only mode and never applies fixes.
Missing tools, stale databases, parse failures, and lint findings fail the check.

Pre-commit uses `require_serial: true`: one Python coordinator owns all workers. This prevents
pre-commit's filename batching from multiplying a separate CPU-sized pool for every batch.
The local default is `max(1, min(2, CPUs - 1))`, capped again by the selected translation-unit count.
This is deliberately more conservative than xff's later CPUs-minus-one default because parsing
large C++ translation units can exhaust memory even when one CPU remains available.

```sh
CLANG_TIDY_JOBS=1 pre-commit run clang-tidy --all-files
CLANG_TIDY_JOBS=2 tools/clang_tidy.sh --all-files
```

`CLANG_TIDY_JOBS` must be a positive integer; `auto` selects the default. CI explicitly requests the
runner CPU count. These are worker limits, not OS CPU or memory quotas, and independently launched
commands do not share a global semaphore. Lower the limit on a busy or memory-constrained host.
The coordinator reports completion per file, retains diagnostics for parse-error detection, and
terminates active workers on interruption. Source changes remain focused; header/build/configuration
changes retain proto's conservative full-scan behavior.

## Build caches and CI scheduling

Shared restore/save actions cache only compiled Bazel action/content outputs. Keys include runner OS,
architecture, configuration, the module lock/Bazel-version hash, run ID, and attempt. The reusable
matrix retains compiler, compiler version, Bazel version, build mode, and protobuf-version separation.
Release coverage shares the main coverage namespace. Downloaded modules and LLVM are not uploaded.

Only `refs/heads/main` saves generations. PRs and numeric release tags restore main generations;
legacy prefixes are fallback during migration. Saves stop the correct Bazel server, including the
reusable runner's custom output root, and trim synchronously before uploading. Largest files are
removed first, oldest first at equal sizes, to preserve more small reusable compilation outputs.
Eviction can leave action records without blobs; Bazel treats these as normal cache misses.

Proto starts with a 600,000,000-byte uncompressed payload limit and a compressed ceiling strictly
below 700,000,000 bytes per entry. xff's 1.5 GB coverage and larger sanitizer allowances were responses
to measured workloads; they are not portable defaults. Main's final job reports before/after sizes,
eviction counts, exact compressed uploads when visible, and all-ref repository usage against a
10 GB planning budget. A read-only inventory on 2026-09-16 found 29 entries totaling
1,792,383,043 bytes; main coverage was 99,064,062 compressed bytes and ASan 84,582,641.
This snapshot supports starting conservatively but does not measure uncompressed build payloads.
Actual account limits may differ. One-day measurement artifacts contain
metadata only. Use these measurements and Bazel's disk-cache-hit statistics before changing budgets.

An upload retires only strictly older main generations in its OS/architecture/configuration after
its exact replacement appears once in the inventory with a positive size below the ceiling. Missing,
empty, oversized, newer, equal-timestamp, and other-ref replacements do not authorize retirement.
Trusted default-branch maintenance runs after workflows and daily, removing closed-PR/tag entries,
superseded generations, and oversized caches. API failures warn and defer immediate retirement.
No custom cache PAT is required. Historical legacy main keys can age out through GitHub eviction;
the new-generation selector does not guess their configuration boundaries.

Coverage starts alongside lint to shorten the successful CI critical path. The final required gate
still includes every job and fails for failed/cancelled dependencies. This can spend runner time on
changes whose lint later fails. Cache performance, API visibility, and total workflow time must be
confirmed after main seeds caches and a later run restores them.

### Preparation profiling

Main/PR coverage and release coverage retain `coverage-preparation-profile`, a seven-day diagnostic
artifact containing `coverage-preparation.json.gz`. Only the coverage invocation receives the
profile path, so later `bazel info` calls cannot overwrite it. Upload runs even after failure; a
missing trace warns without replacing the original coverage failure.

The JSON trace includes repository fetches, Starlark repository functions, Starlark builtins, and
fetch events. Download the artifact from the workflow run, decompress it, and inspect the
`traceEvents` durations with a compatible trace viewer. Compare long repository operations with
Bazel's elapsed time, action critical path, disk-cache hits, and executed actions before attributing
a delay to download, extraction, analysis, or compilation. A cache restore message alone does not
prove action reuse; a short action critical path does not account for all preparation time.

[xff PR 848](https://github.com/mboworks/xff/pull/848) reports an observed first-job-to-done reduction
from 562 to 499 seconds after earlier cache/scheduling changes. Its coverage invocation still took
276.8 seconds with a 3.57-second action critical path and two executed actions. Those were
uncontrolled xff runs, not measurements or predicted improvements for proto. Proto needs its own
CI traces before choosing a toolchain-fetch optimization. Diagnostic traces are separate from the
compiled-output cache; compiler/module downloads remain uncached. Proto has no fuzz preparation
stage. This guide is linked from the README and mapped in `release-site.json` for release websites.

## Coverage publishing and artwork

The trusted publisher retains complete reports per CI run and attempt, alongside the latest report
for each target. It archives existing and incoming reports before replacement, including late runs,
and exposes a separate immutable run-history index. Main stays first in the latest overview; merged
PRs and numeric releases follow by actual merge/tag timestamp. Closed-unmerged PRs are hidden only
from that overview. All PR states are refreshed with paginated GitHub data. Main-integration ancestry
is retained as provenance, including verified squash aggregation, and never determines row order.
Run identity and replacement ordering remain independent of display ordering. See
[Coverage policy and published report history](coverage.md#published-report-history) for timestamp
fallbacks, migration limits, and archive navigation.

The README uses the existing MBO Works artwork at 64 by 64 pixels, right-aligned in its heading.
The unchanged logo, PNG favicons, Apple Touch icon, and multi-resolution ICO come from xff PR #836.
Both release and coverage publishers decorate the complete staged Pages tree. Only that deployment
copy is decorated with relative favicon links; retained release snapshots and coverage
reports remain unchanged. Decoration is idempotent. GitHub funding uses the same organization
project configuration. Release archives and BCR publication keep proto's existing numeric tags,
immutable-release workflow, and development-dependency separation.

## Source review

Reviewed xff PRs 835 through 848, excluding 841. The adaptations follow the final state of each
sequence rather than preserving intermediate regressions.

| xff PR                                          | Finding                                                                                   | Application in proto                                                                                                          |
| ----------------------------------------------- | ----------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| [835](https://github.com/mboworks/xff/pull/835) | CI completion and PR/version numbers do not represent main history.                       | History-based ordering, full checkout, paginated PR metadata, real Git tests; retain numeric tags.                            |
| [836](https://github.com/mboworks/xff/pull/836) | Shared unchanged artwork and deployment-only favicons.                                    | Copy assets and decorate both trusted publishers after staging the complete site.                                             |
| [837](https://github.com/mboworks/xff/pull/837) | Shrink the README logo to 64 pixels.                                                      | Use 64-pixel heading artwork.                                                                                                 |
| [838](https://github.com/mboworks/xff/pull/838) | GitHub/Patreon funding metadata.                                                          | Already present on current main; preserve the existing funding configuration.                                                 |
| [839](https://github.com/mboworks/xff/pull/839) | Inline middle alignment rendered poorly.                                                  | Use the final right-aligned heading layout.                                                                                   |
| [840](https://github.com/mboworks/xff/pull/840) | Immutable keys freeze caches; oversized uploads get discarded.                            | Unique generations, main-only saves, synchronous trimming, trusted cleanup.                                                   |
| [842](https://github.com/mboworks/xff/pull/842) | Age-only eviction discards many reusable objects.                                         | Largest-first eviction; preserve matrix isolation and release restore-only behavior.                                          |
| [843](https://github.com/mboworks/xff/pull/843) | Old/new generation overlap consumes storage.                                              | Retire older main caches only after verifying the exact usable replacement.                                                   |
| [844](https://github.com/mboworks/xff/pull/844) | Budgets require measured compressed and uncompressed sizes.                               | Measurement artifacts and final storage report; no unsupported MSan configuration.                                            |
| [845](https://github.com/mboworks/xff/pull/845) | Coverage's measured payload needed a larger allowance.                                    | Keep proto's initial bound and measure before increasing it.                                                                  |
| [846](https://github.com/mboworks/xff/pull/846) | Concurrent fuzz campaigns need preparation, bounded workers, isolated logs, and cleanup.  | Apply single-coordinator and interruption lessons to lint; proto has no fuzz campaigns to schedule.                           |
| [847](https://github.com/mboworks/xff/pull/847) | Coverage/fuzz can overlap lint without bypassing the final gate.                          | Start coverage immediately; keep every existing job in the gate.                                                              |
| [848](https://github.com/mboworks/xff/pull/848) | Cached builds can remain dominated by preparation; traces are needed to attribute delays. | Profile main/PR and release coverage, preserve seven-day diagnostic artifacts after failure, and document measurement limits. |

The earlier clang-tidy sequence matters separately:
[708](https://github.com/mboworks/xff/pull/708) introduced CPU-wide per-wrapper parallelism;
[714](https://github.com/mboworks/xff/pull/714) introduced progress and interruption orchestration;
[763](https://github.com/mboworks/xff/pull/763) and
[764](https://github.com/mboworks/xff/pull/764) narrowed and tested source selection.
[Commit bb8c85e](https://github.com/mboworks/xff/commit/bb8c85e0b4307ee465f0564026fcbf253e5807ab)
combined serial hook invocation with a two-worker default and explicit overrides.
[Commit 76a87fd](https://github.com/mboworks/xff/commit/76a87fdf7378ccccdc1d1ad69eb5660cd164ae5d)
later changed the default to CPUs minus one. Proto takes the conservative two-worker bound and
preserves its broader header coverage instead of adopting xff's quoted-include-only dependency scan.
