#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Enforce matcher, StatusOr, range-for, and CHECK source conventions."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

_COMPARISON_MACRO = re.compile(
    r"\b(?:ASSERT|EXPECT)_(?:EQ|NE|LT|LE|GT|GE|STREQ|STRNE|STRCASEEQ|STRCASENE|FLOAT_EQ|DOUBLE_EQ|NEAR)\s*\("
)
_QUALIFIED_MATCHER = re.compile(r"(?:::mbo::)?testing::([A-Z][A-Za-z0-9_]*)\(")
_QUALIFIED_UTILITIES = frozenset(
    {
        "ExplainMatchResult",
        "MakePolymorphicMatcher",
        "PrintToString",
        "Return",
        "RunfilesDirOrDie",
        "TempDir",
        "Test",
        "TestWithParam",
        "Values",
    }
)
_BRACED_RANGE_FOR = re.compile(r"for\s*\([^)]*:\s*\{")
_BOOLEAN_CHECK_COMPARISON = re.compile(
    r"\b(?:ABSL_)?(?:D|Q)?CHECK\([^;]*(?:!=|==|<=|>=)"
)
_STATUS_ASSERT = re.compile(
    r"\bASSERT_THAT\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*IsOk\(\)\s*\)"
)


@dataclass(frozen=True)
class Violation:
    line: int
    message: str


def violations(text: str) -> list[Violation]:
    lines = text.splitlines()
    problems: list[Violation] = []
    for index, raw_line in enumerate(lines):
        line = raw_line.split("//", 1)[0]
        if _COMPARISON_MACRO.search(line):
            problems.append(
                Violation(index + 1, "use EXPECT_THAT or ASSERT_THAT with a matcher")
            )
        qualified = _QUALIFIED_MATCHER.search(line)
        if qualified and qualified.group(1) not in _QUALIFIED_UTILITIES:
            problems.append(
                Violation(index + 1, "import matchers with a using declaration")
            )
        if _BRACED_RANGE_FOR.search(line):
            problems.append(
                Violation(index + 1, "range-for requires a named constexpr array")
            )
        if _BOOLEAN_CHECK_COMPARISON.search(line):
            problems.append(
                Violation(index + 1, "use a comparison-specific CHECK macro")
            )
        status_assert = _STATUS_ASSERT.search(line)
        if status_assert:
            name = re.escape(status_assert.group(1))
            following = "\n".join(lines[index + 1 : index + 9])
            if re.search(rf"\*{name}\b|\b{name}->", following):
                problems.append(
                    Violation(
                        index + 1,
                        "use IsOkAndHolds instead of asserting IsOk then dereferencing",
                    )
                )
    return problems


def main(paths: list[str]) -> int:
    problems: list[str] = []
    for filename in paths:
        source = Path(filename)
        for problem in violations(source.read_text()):
            problems.append(f"{filename}:{problem.line}: {problem.message}")
    for problem in problems:
        print(problem, file=sys.stderr)
    return int(bool(problems))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
