#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Select an exact protobuf version in MODULE.bazel for compatibility CI."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys


_PATCHED_VERSION = "35.0"
_VERSION_RE = re.compile(r"^[0-9]+\.[0-9]+(?:\.[A-Za-z0-9]+)*$")
_PROTOBUF_DEP_RE = re.compile(
    r'(?m)^(bazel_dep\(name = "protobuf", version = ")[^"]+(".*\))$'
)
_OVERRIDE_RE = re.compile(
    r"(?ms)^single_version_override\(\n"
    r"(?:(?!^\)\n).)*?"
    r"^\)\n"
)
_GRAPH_VERSION_RE = re.compile(r'protobuf@([^ "\\]+)')


def _protobuf_override(version: str) -> str:
    lines = [
        "single_version_override(",
        '    module_name = "protobuf",',
    ]
    if version == _PATCHED_VERSION:
        lines.extend(
            [
                "    patch_strip = 1,",
                '    patches = ["//bazelmod:protobuf-35.0-abseil.patch"],',
            ]
        )
    lines.extend([f'    version = "{version}",', ")", ""])
    return "\n".join(lines)


def select_protobuf_version(module_text: str, version: str) -> str:
    """Return MODULE.bazel text selecting exactly ``version`` of protobuf."""
    if not _VERSION_RE.fullmatch(version):
        raise ValueError(f"invalid protobuf version: {version!r}")

    dependency_matches = list(_PROTOBUF_DEP_RE.finditer(module_text))
    if len(dependency_matches) != 1:
        raise ValueError(
            "expected exactly one single-line protobuf bazel_dep, "
            f"found {len(dependency_matches)}"
        )
    module_text = _PROTOBUF_DEP_RE.sub(rf"\g<1>{version}\g<2>", module_text)

    override_matches = [
        match
        for match in _OVERRIDE_RE.finditer(module_text)
        if re.search(r'(?m)^    module_name = "protobuf",$', match.group())
    ]
    if len(override_matches) != 1:
        raise ValueError(
            "expected exactly one protobuf single_version_override, "
            f"found {len(override_matches)}"
        )
    override = override_matches[0]
    return (
        module_text[: override.start()]
        + _protobuf_override(version)
        + module_text[override.end() :]
    )


def resolved_protobuf_versions(graph: str) -> list[str]:
    """Return the distinct protobuf versions in Bazel graph output."""
    return sorted(set(_GRAPH_VERSION_RE.findall(graph)))


def verify_resolved_version(graph: str, expected: str) -> None:
    """Raise when Bazel's graph does not resolve exactly ``expected``."""
    resolved = resolved_protobuf_versions(graph)
    if resolved != [expected]:
        actual = ", ".join(resolved) if resolved else "<none>"
        raise ValueError(f"expected protobuf {expected!r}, resolved {actual!r}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("version", help="exact protobuf BCR version")
    parser.add_argument("module", nargs="?", default="MODULE.bazel")
    parser.add_argument(
        "--verify-graph",
        action="store_true",
        help="verify graph input on stdin instead of modifying MODULE.bazel",
    )
    args = parser.parse_args(argv[1:])

    if args.verify_graph:
        try:
            verify_resolved_version(sys.stdin.read(), args.version)
        except ValueError as error:
            parser.error(str(error))
        print(f"OK: resolved protobuf version matches expected {args.version!r}.")
        return 0

    module_path = pathlib.Path(args.module)
    try:
        updated = select_protobuf_version(module_path.read_text(), args.version)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    module_path.write_text(updated)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
