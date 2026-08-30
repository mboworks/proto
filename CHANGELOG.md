# 1.2.2

* Force a Bazel-9-compatible `rules_android` (0.7.2) via MVS, so downstream consumers (and the BCR presubmit) don't hit protobuf's transitive `rules_android` 0.6.4 failing to load on Bazel 9.

# 1.2.1

* Require Bazel 8 or newer and bzlmod; dropped WORKSPACE support.
* Switched to a hermetic LLVM/clang toolchain (`toolchains_llvm` 1.8.0, LLVM 20.1.8).
* Updated dependencies: abseil-cpp 20250814.2, re2 2025-11-05.bcr.1, googletest 1.17.0.bcr.2, rules_cc 0.2.19, bazel_skylib 1.9.0, platforms 1.1.0.
* Bumped the default protobuf to 34.1; CI now tests protobuf versions [32, 33, 34, 35].
* Migrated `SilentErrorCollector` to protobuf's `RecordError`/`RecordWarning` API so it builds against protobuf 35 (which removed the deprecated `AddError`/`AddWarning`).

# 1.2.0

* Added std::optional variants for file reading.
* Added `HasBinaryProtoExtension` and `HasTextProtoExtension`.

# 1.1.0

* Added `ReadBinaryProtoFile`, `ReadTextProtoFile`, `WriteBinaryProtoFile`, `WriteTextProtoFile`.

# 1.0.2

* Applied DWYU cleanup.

# 1.0.1

* Repackage 1.0.0.

# 1.0.0

* Changed to empty root BUILD file for release packages.
* Cut and packaged version 1.0.0 (no functionality changes compared to 0.6.4).

# 0.6.4

* Added LLVM-20.1.0 config for Linux-Arm64 and MacOs-Arm64 platforms.

# 0.6.3

* Made macro `PARSE_TEXT_PROTO` issue a deprecation warning.
* Raised minimum zlib version when built with proto version 27.0 for MacOS.
* Enabled Bazel layering_check.

# 0.6.2

* Updated protobuf dev dependency to v30.0 to ensure compatibility.
* Updated CI matrix to cover more combinations.
* Implemented (again) the elusive `PARSE_TEXT_PROTO` from 2010 as referenced by clang-tidy's documentation.

# 0.6.1

* Updated license/copyright to add SPDX headers.
* Updated internal tooling.
* Released to baze-central-registry for the first time.

# 0.6.0

* Added Bazel module support.
* Added new modern `SilentErrorCollector`.
* Updated Clang to 19.1.6.
* Verified and updated protobuf to v30-rc.2. Note that at least v28.3 ... v29.3 are affected by https://github.com/protocolbuffers/protobuf/issues/19364 which is fixed in v30-rc.2 Note that the affected versions work fine with Clang 19.1.6.
* Added a patch for MODULE.bazel to be set to protobuf version 28, in order to reduce requirements on complex projects.
* Added a build matrix merge check to guarantee various combinations work.
* Pinned bazel version to 7.2.1.
* Updated other dependencies to latest.

# 0.5

* Added support for com_google_protobuf >= Protocol Buffers v26.
* Moved `ParseTextProtoHelper` into namespace `mbo::proto::proto_internal`.
* Changed `ParseTextProtoHelper` to only accept derived proto messages on operator access.
* Changed `ParseTextProtoHelper` to die on parsing failure.
* Added `ParseText<T>() -> absl::StatusOr<T>`.
* Improved error message detail for all parsing functions.
* Updated all dependencies.
* Verified protobuf versions 25.2, 26.0, 26.1, 27.0, 27.1, 27.2, 28.0-rc1.
* Added Clang support.
* Updated GH actions.

# 0.4

* Changed `ParseTextProto`, `ParseTextProtoOrDie` to only work for actual derived proto types.
* Added `ParseProto` which returns `absl::StatusOr<MessageType>`.
* Updated external dependencies.

# 0.3

* Added missing dependency.

# 0.2

* Small fixes, mostly to support a few older abseil/re2 versions.

# 0.1

* Initial release
