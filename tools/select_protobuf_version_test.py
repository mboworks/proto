#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Tests for exact protobuf compatibility-version selection."""

from __future__ import annotations

import os
import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import select_protobuf_version  # noqa: E402


_MODULE = '''\
single_version_override(
    module_name = "protobuf",
    patch_strip = 1,
    patches = ["//bazelmod:protobuf-35.0-abseil.patch"],
    version = "35.0",
)

bazel_dep(name = "protobuf", version = "36.1.bcr.1", repo_name = "com_google_protobuf")
'''


class SelectProtobufVersionTest(unittest.TestCase):
    def test_readme_lists_compatibility_matrix_versions(self):
        root = Path(__file__).parents[1]
        workflow = (root / ".github/workflows/compatibility.yml").read_text(
            encoding="utf-8"
        )
        readme = (root / "README.md").read_text(encoding="utf-8")
        matrix = re.search(r"proto_version: \[(.*)\]", workflow)
        self.assertIsNotNone(matrix)
        versions = re.findall(r'"([^"]+)"', matrix.group(1))
        self.assertIn(
            f"[{', '.join(versions[:-1])}, and {versions[-1]}]"
            "(.github/workflows/compatibility.yml)",
            readme,
        )

    def test_compatibility_passes_cache_cleanup_secret(self):
        workflow = (
            Path(__file__).parents[1] / ".github/workflows/compatibility.yml"
        ).read_text(encoding="utf-8")
        self.assertIn("    secrets: inherit\n", workflow)

    def test_selects_unpatched_older_version(self):
        updated = select_protobuf_version.select_protobuf_version(_MODULE, "34.1")
        self.assertIn('version = "34.1"', updated)
        self.assertEqual(updated.count('version = "34.1"'), 2)
        self.assertNotIn("patch_strip", updated)
        self.assertNotIn("protobuf-35.0-abseil.patch", updated)

    def test_retains_patch_for_35(self):
        updated = select_protobuf_version.select_protobuf_version(_MODULE, "35.0")
        self.assertEqual(updated.count('version = "35.0"'), 2)
        self.assertIn("patch_strip = 1", updated)
        self.assertIn("protobuf-35.0-abseil.patch", updated)

    def test_selects_unpatched_current_version(self):
        updated = select_protobuf_version.select_protobuf_version(
            _MODULE, "36.1.bcr.1"
        )
        self.assertEqual(updated.count('version = "36.1.bcr.1"'), 2)
        self.assertNotIn("patch_strip", updated)

    def test_rejects_invalid_version(self):
        with self.assertRaisesRegex(ValueError, "invalid protobuf version"):
            select_protobuf_version.select_protobuf_version(_MODULE, "latest")

    def test_rejects_missing_dependency(self):
        module = _MODULE.replace("bazel_dep(name = \"protobuf\"", "bazel_dep(name = \"other\"")
        with self.assertRaisesRegex(ValueError, "protobuf bazel_dep"):
            select_protobuf_version.select_protobuf_version(module, "34.1")

    def test_rejects_missing_override(self):
        module = _MODULE[_MODULE.index("bazel_dep") :]
        with self.assertRaisesRegex(ValueError, "protobuf single_version_override"):
            select_protobuf_version.select_protobuf_version(module, "34.1")

    def test_extracts_quoted_bazel_graph_version(self):
        graph = '''\
<root> -> "protobuf@34.1"
"protobuf@34.1" -> "abseil-cpp@20250512.1"
'''
        self.assertEqual(
            select_protobuf_version.resolved_protobuf_versions(graph), ["34.1"]
        )

    def test_verifies_exact_resolved_version(self):
        select_protobuf_version.verify_resolved_version(
            '<root> -> "protobuf@36.1.bcr.1"\n', "36.1.bcr.1"
        )

    def test_rejects_a_different_resolved_version(self):
        with self.assertRaisesRegex(ValueError, "resolved '35.0'"):
            select_protobuf_version.verify_resolved_version(
                '<root> -> "protobuf@35.0"\n', "34.1"
            )


if __name__ == "__main__":
    unittest.main()
