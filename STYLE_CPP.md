# C++ style

This guide sits on top of the repository's machine-enforced configuration. When the prose and a
checked-in tool configuration disagree, the tool configuration wins.

## Toolchain

- Use C++20 and keep both Clang and GCC builds working.
- Format all C++ with [`.clang-format`](.clang-format). Do not hand-format around it.
- Run clang-tidy with [`.clang-tidy`](.clang-tidy). Generate `compile_commands.json` first with
  [`./compile_commands-update.sh`](compile_commands-update.sh), then run
  `pre-commit run clang-tidy --all-files`.
- Treat warnings as errors in first-party code. Do not suppress a warning repository-wide when a
  narrow code fix or documented local suppression is possible.

## Naming

- Types, classes, structs, enums, aliases, and functions use `CamelCase`.
- Variables, parameters, data members, and namespaces use `lower_case`.
- Private data members have a trailing underscore.
- Constants use `kCamelCase`.
- Macros use `UPPER_CASE`. Exported macros are prefixed `MBO_`.
- C++ library Bazel targets use the existing `_cc` suffix convention.

## Code organization

- Exported code lives below `mbo/` and uses the namespace matching its path, currently
  `mbo::proto`.
- Do not create a namespace component named only `internal`. Use a descriptive suffix such as
  `proto_internal` for implementation details.
- Header guards are the upper-cased path and filename with non-alphanumeric characters replaced by
  underscores and one trailing underscore, for example `MBO_PROTO_FILE_H_`.
- Public headers are self-contained: include what they use and verify they compile independently.
- Forward declarations and their implementations stay together unless an established pimpl-style
  layout requires otherwise.
- Avoid macros. Undefine translation-unit-local helper macros immediately after their final use.
- Use the `std::` spelling of C library names (`std::size_t`, `std::uint64_t`, `std::memcpy`).

## Interfaces and ownership

- Prefer values for small, copyable types and `const T&` for larger required inputs.
- Use pointers only when null is meaningful or pointer semantics are intrinsic to the API.
- Use `std::string_view` for non-owning text input and `std::span` for non-owning contiguous ranges.
- Use smart pointers for ownership. A raw pointer never communicates ownership.
- Mark single-argument constructors `explicit` unless implicit conversion is intentional and
  documented.
- Add `[[nodiscard]]` to results whose accidental loss is likely to be a defect, especially status
  and validation results.
- Do not expose implementation-only dependencies from a public target. Use Bazel
  `implementation_deps` where appropriate.

## Error handling

- Use `absl::Status` for operations that can fail without returning a value and
  `absl::StatusOr<T>` when they return a value.
- Include actionable context in errors: the operation, relevant path or field, and underlying
  failure where available.
- Do not log and return the same error at a library boundary; the caller owns presentation.
- Fatal checks are for violated invariants, not ordinary invalid input or environmental failures.
- Exceptions are not part of the library's public error model.

## Formatting conventions

- Let clang-format decide line wrapping and brace placement.
- Put explanatory comments above the code they describe rather than at the end of a long line.
- Use a trailing comma to opt a complex aggregate into one-field-per-line formatting.
- Prefer designated initializers where they clarify field meaning and preserve aggregate semantics.
- Keep related definitions together, but separate unrelated definition blocks with a blank line.

## Includes

- Include the matching header first in a `.cc` file.
- Then include C and C++ standard-library headers, third-party headers, and project headers.
- Do not depend on transitive includes.
- Keep include paths rooted at the repository, for example `#include "mbo/proto/file.h"`.
- Do not include dependency implementation headers unless the public API explicitly requires them.

## Tests

- Every behavior change includes a test or a clear explanation of why no test is possible.
- Prefer a fixture (`TEST_F`) when tests share setup, helpers, or state; use a plain `TEST` when a
  fixture would add no value.
- Use `EXPECT_THAT` and `ASSERT_THAT` with GoogleTest/GoogleMock matchers when they produce clearer
  failures than comparison macros.
- Use `ASSERT_*` only when continuing would be invalid or unsafe; otherwise collect failures with
  `EXPECT_*`.
- Name tests for observable behavior, not implementation details.
- Test success, malformed input, boundary values, and relevant dependency failures.
- Keep test data local and deterministic. Do not depend on the network, wall-clock timing, locale,
  or the developer's filesystem.
- Direct Bazel tests declare an appropriate `size` when new tests are added.

## Bazel

- Keep direct dependencies explicit and preserve strict include/layering checks.
- A `cc_library` lists public headers in `hdrs`, implementation files in `srcs`, and private-only
  dependencies in `implementation_deps`.
- Public libraries have a direct test in the same package unless their behavior is exercised only
  through an unavoidable generated-code boundary.
- Developer-only dependencies belong in `bazelmod/dev.MODULE.bazel`; a published module consumer
  must not need them.
- Tags such as `manual`, sanitizer exclusions, or coverage exclusions require a concrete reason.

## Documentation

- Document public types and functions in their declarations.
- Explain contracts, ownership, lifetime, error behavior, and surprising complexity.
- Comments explain why; names and structure should explain what.
- Keep examples compilable in spirit and update them with the interface they demonstrate.
