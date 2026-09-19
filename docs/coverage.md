# Coverage policy

[`coverage_policy.json`](../coverage_policy.json) is the single source of truth for source scope,
ratings, and CI enforcement. The console summary, retained HTML report, and policy gate consume the
same thresholds.

Each metric has a minimum and a target. Results below the minimum are low, results from the minimum
to the target are medium, and results at or above the target are high. The `enforce` setting selects
the lowest accepted rating independently for lines, functions, and branches. Patch coverage applies
the same model to changed coverable lines and branches.

Tests and generated proto sources are excluded: the report measures the maintained library sources.
Threshold changes should be based on a generated report and should never conceal a regression.

## Published report history

The trusted publisher retains the latest report at `coverage/main/`, `coverage/pr/NUMBER/`, and
`coverage/tag/VERSION/`. It also archives each identified published run and attempt under
`coverage/runs/RUN_ID/ATTEMPT/`, with a separate [run-history index](https://mboworks.github.io/proto/coverage/runs/).
Snapshots include the original summary JSON, measured commit and workflow metadata, and detailed
LCOV source pages. Only overview navigation is relocated for the archive's deeper URL; measurements,
source content, and report-local links are preserved. Once published, a snapshot is not rewritten.

Existing reports and incoming artifacts are archived before latest-report replacement. A late older
run therefore gains its own snapshot without replacing a newer report. Copies are staged and renamed
atomically, so an interrupted copy does not leave a partial snapshot at its public URL. Migration
archives currently retained reports with authentic run IDs; it cannot reconstruct previously
replaced reports or invent identities for legacy reports. Failed runs without a published report
are not archived. Run history grows with publication; no automatic pruning is introduced.

The overview keeps main first, then interleaves merged PRs and releases by actual reference time,
newest first. PRs use GitHub's `merged_at`, including merges into aggregation branches. Annotated
numeric release tags use tagger time; lightweight tags have no creation timestamp and use the tagged
commit's committer time. Open PRs and unknown references follow by CI run creation time. Closed,
unmerged PRs disappear from the overview but retain their direct reports and archived runs;
reopening restores their overview row at the next publication.

Every publication refreshes PR state and `reference_time` from paginated data for all PRs. Run
identity and latest-report replacement remain based on the original CI source fields. Main-history
positions are provenance only: normal aggregation uses ancestry, while squash aggregation verifies
the child merge against the matching parent's recorded head, fetching that exact commit if needed.
Neither aggregation nor metadata refresh transfers measurements to a different commit or run.

This publisher behavior is adapted from [xff #876](https://github.com/mboworks/xff/pull/876) and
[xff #877](https://github.com/mboworks/xff/pull/877). See
[Infrastructure and publishing](infrastructure.md) for deployment details. These changes take effect
when the publisher reaches main; this PR does not backfill or deploy the public site.
Coverage publication also refreshes PR lifecycle metadata on close and reopen, selects one
pre-merge or exact-merge-commit post-merge result per PR, and accepts a valid coverage job when
other CI jobs fail. A manual source-run input can replay a retained artifact through the same
serialized publisher without rerunning tests.
