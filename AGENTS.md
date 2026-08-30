# Agent and contributor rules - proto

These rules apply to human and automated contributors. Keep changes reviewable, preserve existing
project-specific behavior, and use automation for mechanical policy.

## Development workflow

Build and test with `bazel test //...`. Run `pre-commit run --all-files` for repository-wide
changes, or `pre-commit run --files FILE...` for focused changes. Generate `compile_commands.json`
with `./compile_commands-update.sh` before running the clang-tidy hook locally.

Do not commit Bazel output links, compilation databases, coverage output, local rc files, or tool
caches. Do not weaken warnings, sanitizer checks, lint rules, or coverage thresholds merely to make
a change pass; fix the cause or document a narrowly scoped exception.

## C++ and Bazel

- Format C++ with the checked-in `.clang-format` configuration.
- Keep public headers self-contained and use the repository's path-based header guards.
- Add or update tests with behavior changes. Prefer GoogleTest matchers and assertions that expose
  useful failure values.
- Keep direct dependencies explicit in BUILD files and retain strict include checking.
- Name C++ library targets with the existing `_cc` suffix convention.
- Keep developer-only Bazel dependencies in `bazelmod/dev.MODULE.bazel`; published module users must
  not require them.

## Releases

Versions in `MODULE.bazel` and `CHANGELOG.md` must agree. Release tags are numeric semantic versions
without a `v` prefix. Use `tools/trigger_release.sh`; release workflows create the GitHub release and
open the Bazel Central Registry publication pull request.

## Documentation and shell

Keep user-visible behavior and build requirements documented in the same change. Shell scripts use
strict mode where practical and must pass shfmt and ShellCheck. Keep Markdown tables aligned with
`tools/align_markdown_tables.py`.
