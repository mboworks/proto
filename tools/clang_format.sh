#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

readonly workspace="${PWD}"
args=()
for arg in "$@"; do
  if [[ "${arg}" == -* ]]; then
    args+=("${arg}")
  else
    args+=("${workspace}/${arg}")
  fi
done

exec bazel run '@llvm_toolchain_llvm//:clang-format' -- "${args[@]}"
