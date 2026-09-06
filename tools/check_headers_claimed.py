#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Require every tracked first-party header to be claimed by its BUILD file."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

_ALLOWLIST: dict[str, str] = {}


def owning_build_file(root: Path, header: str) -> Path | None:
    directory = (root / header).parent
    while root == directory or root in directory.parents:
        for name in ("BUILD.bazel", "BUILD"):
            build_file = directory / name
            if build_file.is_file():
                return build_file
        if directory == root:
            break
        directory = directory.parent
    return None


def check(headers: list[str], root: Path) -> list[str]:
    problems: list[str] = []
    for header in sorted(headers):
        if header in _ALLOWLIST:
            continue
        build_file = owning_build_file(root, header)
        if build_file is None:
            problems.append(f"{header}: no owning BUILD file")
            continue
        relative = (root / header).relative_to(build_file.parent).as_posix()
        if f'"{relative}"' not in build_file.read_text():
            problems.append(
                f"{header}: not claimed by {build_file.relative_to(root)}"
            )
    return problems


def tracked_headers(root: Path) -> list[str]:
    result = subprocess.run(
        ["git", "ls-files", "mbo/**/*.h", "mbo/*.h"],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    )
    return [line for line in result.stdout.splitlines() if line]


def main() -> int:
    root = Path.cwd()
    problems = check(tracked_headers(root), root)
    for problem in problems:
        print(problem, file=sys.stderr)
    return int(bool(problems))


if __name__ == "__main__":
    sys.exit(main())
