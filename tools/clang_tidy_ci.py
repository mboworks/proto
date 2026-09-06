#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

"""Translate a GitHub Actions event and branch diff into clang-tidy inputs."""

from __future__ import annotations

import argparse
import sys


def select_inputs(event_name: str, changed_files: list[str]) -> list[str]:
    """Return clang-tidy scope inputs for a GitHub Actions event."""
    if event_name == "push":
        return ["--all-files"]
    if event_name == "pull_request":
        return sorted(set(changed_files))
    raise ValueError(f"unsupported GitHub Actions event: {event_name!r}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("event_name", choices=("pull_request", "push"))
    args = parser.parse_args(argv[1:])
    changed_files = [line for line in (line.strip() for line in sys.stdin) if line]
    for path in select_inputs(args.event_name, changed_files):
        print(path)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
