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
