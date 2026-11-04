#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

# Build and resolve the formatter without changing the caller's working directory.
bazel build --config=clang --ui_event_filters=-info --noshow_progress \
  @llvm_toolchain_llvm//:clang-format
formatter_files="$(bazel cquery --config=clang --ui_event_filters=-info --noshow_progress \
  --output=files @llvm_toolchain_llvm//:clang-format)"
execution_root="$(bazel info execution_root)"
# Target and exec configurations can each contribute an output path. Select one
# existing executable from the pinned toolchain, preserving argument boundaries.
while IFS= read -r formatter; do
  [[ -n "${formatter}" ]] || continue
  if [[ "${formatter}" != /* ]]; then
    formatter="${execution_root}/${formatter}"
  fi
  if [[ -x "${formatter}" ]]; then
    exec "${formatter}" "$@"
  fi
done <<<"${formatter_files}"
printf '%s\n' "ERROR: no executable clang-format output from the pinned toolchain" >&2
exit 1
