#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Minimal reader for the buildifier-formatted BUILD constructs policy needs."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

_RULE_OPEN = re.compile(r"^(\w+)\(")
_NAME_ATTR = re.compile(r'^\s*name\s*=\s*"([^"]*)"')
_INLINE_NAME = re.compile(r'^\w+\(\s*name\s*=\s*"([^"]*)"')
_DEPS_ATTR = re.compile(r"^\s+deps\s*=")
_NEXT_ATTR = re.compile(r"^\s{4}\w+\s*=")
_ATTRIBUTE = re.compile(r"^\s{4}(\w+)\s*=")
_LABEL = re.compile(r'"((?::|//|@)[^"]*)"')
_SKIP_PARTS = frozenset({"external", "third_party", ".git"})


@dataclass
class Rule:
    kind: str
    name: str
    line: int
    deps: list[str] = field(default_factory=list)
    attributes: set[str] = field(default_factory=set)


def parse_rules(text: str) -> list[Rule]:
    """Return top-level rules from buildifier-formatted BUILD text."""
    rules: list[Rule] = []
    current: Rule | None = None
    in_deps = False
    for number, line in enumerate(text.splitlines(), start=1):
        opened = _RULE_OPEN.match(line)
        if opened:
            inline = _INLINE_NAME.match(line)
            current = Rule(
                kind=opened.group(1),
                name=inline.group(1) if inline else "",
                line=number,
            )
            rules.append(current)
            in_deps = False
            if line.rstrip().endswith(")") and inline:
                current = None
            continue
        if current is None:
            continue
        if line.startswith(")"):
            current = None
            in_deps = False
            continue
        attribute = _ATTRIBUTE.match(line)
        if attribute:
            current.attributes.add(attribute.group(1))
        if not current.name:
            name = _NAME_ATTR.match(line)
            if name:
                current.name = name.group(1)
        if _DEPS_ATTR.match(line):
            in_deps = True
            current.deps.extend(_LABEL.findall(line))
        elif in_deps:
            if _NEXT_ATTR.match(line):
                in_deps = False
            else:
                current.deps.extend(_LABEL.findall(line))
    return [rule for rule in rules if rule.name]


def find_build_files(root: Path) -> list[Path]:
    """Return first-party BUILD files below ``root``."""
    result: list[Path] = []
    for name in ("BUILD.bazel", "BUILD"):
        for candidate in root.rglob(name):
            relative = candidate.relative_to(root)
            if any(
                part in _SKIP_PARTS or part.startswith("bazel-")
                for part in relative.parts
            ):
                continue
            result.append(candidate)
    return sorted(result)


def display_path(path: Path, root: Path) -> Path:
    try:
        return path.relative_to(root)
    except ValueError:
        return path
