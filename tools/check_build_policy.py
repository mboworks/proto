#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Enforce target naming, direct library tests, and explicit test sizes."""

from __future__ import annotations

import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_rules import display_path, find_build_files, parse_rules  # noqa: E402

_DIRECT_TEST_RULES = frozenset(
    {"bashtest", "cc_fuzz_test", "cc_test", "py_test", "sh_test"}
)
_UNTESTED_ALLOWLIST: dict[str, str] = {}


def violations(text: str, package: str) -> list[tuple[int, str]]:
    """Return line-numbered BUILD policy failures."""
    rules = parse_rules(text)
    tested: set[str] = set()
    for rule in rules:
        if not rule.kind.endswith("_test"):
            continue
        for dependency in rule.deps:
            if dependency.startswith(":"):
                tested.add(dependency[1:])
            elif dependency.startswith(f"//{package}:"):
                tested.add(dependency.split(":", 1)[1])

    problems: list[tuple[int, str]] = []
    for rule in rules:
        if rule.kind == "cc_library":
            if not rule.name.endswith("_cc"):
                problems.append(
                    (rule.line, f"cc_library '{rule.name}' must end in '_cc'")
                )
            qualified = f"{package}:{rule.name}"
            if rule.name not in tested and qualified not in _UNTESTED_ALLOWLIST:
                problems.append(
                    (
                        rule.line,
                        f"cc_library '{rule.name}' has no same-package test with a direct dependency",
                    )
                )
        if rule.kind in _DIRECT_TEST_RULES and "size" not in rule.attributes:
            problems.append(
                (rule.line, f"{rule.kind} '{rule.name}' has no explicit size")
            )
    return problems


def check(paths: list[Path], root: Path) -> list[str]:
    problems: list[str] = []
    for build_file in paths:
        try:
            text = build_file.read_text()
        except OSError as error:
            problems.append(f"{build_file}: cannot read ({error})")
            continue
        relative = display_path(build_file, root)
        package = "" if str(relative.parent) == "." else relative.parent.as_posix()
        for line, message in violations(text, package):
            problems.append(f"{relative}:{line}: {message}")
    return problems


def main(argv: list[str]) -> int:
    root = Path.cwd()
    paths = [Path(argument) for argument in argv[1:]] or find_build_files(root)
    problems = check(paths, root)
    for problem in problems:
        print(problem, file=sys.stderr)
    return int(bool(problems))


if __name__ == "__main__":
    sys.exit(main(sys.argv))
