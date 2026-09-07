# Repository strictness follow-up

This document tracks repository-wide gaps found by comparing `mboworks/proto` with the current
`mboworks/mbo` and `mboworks/xff` default branches on 2026-09-06. It records applicable controls,
not mechanical parity: project-specific xff policies are excluded unless they protect behavior
that proto also has.

Each item is complete only when its acceptance criteria are satisfied on `main`. Prefer focused,
reviewable pull requests, but combine related changes when one change would otherwise make CI
temporarily misleading or broken.

## P0 - CI correctness

### CI-1: Make protobuf compatibility cells select the requested version

- [x] Rewrite or remove the `single_version_override` for protobuf when `proto_version` is set;
      changing only `bazel_dep` is insufficient because the checked-in override still forces 35.0.
- [x] Assert the resolved protobuf version in every compatibility cell so selection cannot silently
      regress.
- [x] Add tests for the version-selection logic.
- [x] Define a rolling compatibility window consisting of the current protobuf major release and
      the two preceding major release lines; initially test 34.1, 35.0, and 36.1.bcr.1.
- [x] Exercise that window with one deliberately narrow configuration: Ubuntu, Clang 22.1.8, Bazel
      9.2.0, and `opt`. Compiler, sanitizer, OS, and Bazel-version coverage remains owned by the
      main matrix and must not be crossed with protobuf versions.
- [x] Correct the stale protobuf 34.1 comment in `.bcr/presubmit.yml`.

Acceptance: CI logs prove that each cell resolved its named protobuf version, all three release
lines pass on the single compatibility configuration, and advancing the current major makes the
oldest line's removal an explicit reviewable change.

### CI-2: Make clang-tidy meaningful on pull requests and `main`

- [x] Lint changed source files and translation units affected by changed headers on pull requests.
- [x] Remove the explicit `mbo/proto/matchers.h` exclusion.
- [x] Promote compilation-wide inputs, including public headers and clang-tidy orchestration files,
      to the appropriate full sweep.
- [x] Run a whole-tree clang-tidy sweep on `main`; do not diff `origin/main...HEAD` after a main push.
- [x] Resolve the current first-party full-sweep findings rather than hiding them through broader
      exclusions.
- [x] Extend the clang-tidy scope/orchestration tests to cover pull-request and main-push behavior.

Acceptance: a deliberately introduced diagnostic in `matchers.h` fails a pull request; a full-tree
run passes on `main`; and the main job reports a non-empty, auditable scope.

## P1 - Portable policy enforcement

### POLICY-1: Adopt applicable mbo C++ and Bazel checks

- [x] Require an explicit `size` on direct Bazel test rules. Add sizes to all four current
      `cc_test` rules.
- [x] Enforce the `_cc` suffix on `cc_library` targets.
- [x] Enforce that every `cc_library` has a same-package test with a direct dependency, with only
      documented allowlist exceptions.
- [x] Enforce that every header is claimed by its package BUILD file.
- [x] Enforce GoogleTest matcher style and the repository's status assertion conventions.
- [x] Replace and forbid range-for loops over inline braced initializer lists; use named
      `constexpr std::array` values.
- [x] Replace and forbid comparisons inside boolean `CHECK`/`ABSL_CHECK`; use comparison-specific
      macros that report both operands.
- [x] Add tests for each non-trivial policy checker.

Acceptance: the adopted checks run in pre-commit and CI, their own tests pass, and the existing tree
has no undocumented exceptions.

### POLICY-2: Align contributor and pull-request governance

- [x] Add a project-appropriate Git and pull-request rules document based on mbo's `GIT_RULES.md`.
- [x] Reference it from `AGENTS.md` and `CONTRIBUTING.md`.
- [x] Adopt the two-layer pull-request description with a human summary followed by `## AG;DR`.
- [x] Document state-changing Git/GitHub operations, review readiness, dependency ordering, CI
      recovery, and merge completion checks.

Acceptance: contributor documentation has one unambiguous source of truth for Git and pull-request
operations and no contradictory instructions.

## P1 - Coverage regression controls

### COVERAGE-1: Protect coverage above the absolute thresholds

- [x] Add a baseline maximum-drop policy, initially no more than 0.1 percentage points for lines,
      functions, and branches, matching mbo.
- [x] Add unit tests for missing, malformed, improved, unchanged, and regressed baselines.
- [x] Preserve the current overall policy: lines 90/95/high, functions 90/95/high, and branches
      85/90/high.

Acceptance: a change that remains above the high target but drops a protected metric beyond the
allowed delta fails CI.

### COVERAGE-2: Raise patch coverage

- [x] Raise patch minimums from 85% lines and 70% branches toward the mbo/xff baseline of 95% lines
      and 85% branches.
- [x] Keep patch targets at least 98% lines and 90% branches when adopting the comparison policy.
- [x] Do not raise enforcement until representative changes pass with practical headroom.

Evidence: PR 84's production patch covered 16/16 changed lines and 4/4 changed branches. Its
100%/100% result exceeds the adopted minimums by 5 and 15 percentage points respectively.

Acceptance: the final patch policy is enforced by tests and CI without weakening overall coverage
or excluding additional production files.

## P1 - CI performance and failure bounds

### CI-3: Cache coverage and clang-tidy builds

- [x] Route coverage and clang-tidy Bazel outputs through bounded disk caches.
- [x] Restore from main before branch-specific keys and save only when useful.
- [x] Add cache-size checks and cleanup behavior so toolchains do not crowd out compiled outputs.
- [x] Investigate and eliminate the recurring GitHub Actions cache HTTP 400 warnings.
- [x] Add explicit timeouts to long-running jobs; use mbo/xff timings as the initial bounds.

Acceptance: consecutive representative runs show cache restoration and materially less cold-build
work, cache failures are actionable, and hung jobs terminate within their documented bounds.

## P1 - Release and registry hardening

### RELEASE-1: Harden and test release preparation

- [ ] Replace `mktemp -u` with a securely created temporary directory and an EXIT cleanup trap.
- [ ] Explicitly decide whether `.gitattributes`, `.gitignore`, and `.trunk` belong in release
      archives; exclude development-only files consistently with mbo where applicable.
- [ ] Test changelog/version guards, patch application, archive contents, generated root BUILD,
      release notes, cleanup, and standalone archive builds.
- [ ] Verify release archives with every supported Bazel major.

Acceptance: an automated test constructs the archive from a fixture or checkout, inspects its exact
contents, and builds/tests it as an external module.

### RELEASE-2: Bring `trigger_release.sh` up to mbo's tested behavior

- [ ] Add `--dry-run`.
- [ ] Validate required tools, numeric semantic versions, clean synchronized main, unused local and
      remote tags, absent GitHub releases, and an applicable release patch before mutation.
- [ ] Push only the intended signed tag.
- [ ] Create a reviewable next-version branch and `## AG;DR` pull request, then return to clean main.
- [ ] Add tests for success, validation failures, next-version calculation, and partial external
      failures.

Acceptance: the release trigger is covered by deterministic tests and cannot tag before all local
preflight checks succeed.

### RELEASE-3: Expand BCR presubmit coverage

- [ ] Cover every published public library surface, including `file_cc` and
      `silent_error_collector_cc`, rather than only two test targets.
- [ ] Confirm the BCR matrix uses both supported Bazel majors and intended Linux/macOS platforms.
- [ ] Keep development-only dependencies absent from the staged archive.

Acceptance: the staged release archive builds all public surfaces in every BCR presubmit cell.

## P2 - Documentation and mechanical cleanup

### DOC-1: Correct user-facing version and support information

- [x] Replace the README's obsolete protobuf 27-30 claim with the verified compatibility range.
- [x] Update the installation example from `mboworks_proto` 1.2.2 to the latest published release,
      without advertising an unreleased version.
- [x] Replace `MODULES.bazel` with `MODULE.bazel` throughout documentation and generated release
      notes.
- [x] Fix the broken `RULES.dm` link in `CONTRIBUTING.md`.
- [x] Reconcile the documented clang-format requirement with the checked-in formatter version.

Acceptance: README, contributor docs, release notes, CI, MODULE metadata, and BCR metadata agree.

### DOC-2: Remove spelling-debt exclusions

- [ ] Fix the existing repeated `file.h` documentation typos around `std::nullopt` and error
      handling.
- [ ] Fix "text prto" in `file_test.cc`.
- [ ] Correct inherited spelling mistakes in hook descriptions when the corresponding policy is
      touched.
- [ ] Remove whole-file codespell exclusions after the files pass normally.

Acceptance: codespell checks all C++ and Markdown files without broad file exclusions.

### TOOLING-1: Align formatter versions

- [x] Upgrade the clang-format pre-commit hook from 19.1.6 to the hermetic LLVM 22.1.8 baseline.
- [x] Format and review the resulting changes as a dedicated mechanical commit.
- [x] Remove the `parse_text_proto` formatting exclusions or document narrow, line-level reasons if
      LLVM 22 still cannot preserve required preprocessor layout.

Acceptance: the checked-in formatter, clang compiler, and clang-tidy major versions agree, and all
C++ files participate in formatting enforcement.

## P2 - Additional runtime assurance

### TEST-1: Add parser fuzzing

- [ ] Add isolated fuzz targets for binary protobuf reads, text protobuf parsing, and matcher
      deserialization where practical.
- [ ] Ensure arbitrary input cannot write outside a managed temporary or in-memory test boundary.
- [ ] Add a bounded pull-request smoke run and a scheduled deeper campaign if runtime permits.

Acceptance: malformed and arbitrary inputs are continuously exercised under sanitizers without
host-side effects.

### TEST-2: Evaluate MemorySanitizer

- [ ] Prototype a Linux Clang 22 MSan build with compatible instrumented dependencies.
- [ ] Add a required cell only if it produces reliable signal and acceptable maintenance cost.
- [ ] Record a rejection with concrete evidence if dependency instrumentation makes MSan
      impractical.

Acceptance: MSan is either enforced or explicitly rejected with a reproducible technical reason.

## Confirmed parity - do not reopen without new evidence

- Repository rules match mbo/xff for signed commits, protected main, linear history, required
  reviews, stale-review dismissal, last-push approval, resolved review threads, and required CI.
- Release tags are protected against unsigned creation, update, non-fast-forward changes, and
  deletion.
- Secret scanning, push protection, Dependabot security updates, auto-merge, automatic branch
  deletion, and update-branch support are enabled where applicable.
- Bazel packages default to private visibility and public libraries opt in explicitly.
- Current library target names, direct test relationships, and header ownership satisfy the mbo
  checks when evaluated manually.
- Development-only compile-command and dependency-analysis tooling stays in
  `bazelmod/dev.MODULE.bazel`; published consumers do not need `mboworks_mbo`.
- The intentional helly25 compile-commands-extractor fork remains pinned by commit because it is the
  working implementation required by this repository.
- Immutable GitHub releases and BCR publication use the corrected shared workflow.
- Overall measured coverage exceeds each high target by more than one percentage point and is
  enforced at lines 90/95/high, functions 90/95/high, and branches 85/90/high.

## Explicitly not adopted from xff

The following xff controls are application-specific and are not proto gaps: raw-pointer API bans,
VFS-only file access, CLI option/documentation registries, archive and FUSE policies, composable
extras checks, MIME/language database generation, shell bashtest conventions, and xff's custom C++
rule wrappers. Reconsider only if proto gains the corresponding architecture.
