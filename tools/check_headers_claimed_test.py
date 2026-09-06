# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Tests for header ownership policy."""

import tempfile
import unittest
from pathlib import Path

from tools import check_headers_claimed


class CheckHeadersClaimedTest(unittest.TestCase):
    def test_claimed_header_is_accepted(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = root / "mbo" / "proto"
            package.mkdir(parents=True)
            (package / "BUILD.bazel").write_text('hdrs = ["thing.h"]\n')
            (package / "thing.h").touch()
            self.assertEqual(
                check_headers_claimed.check(["mbo/proto/thing.h"], root), []
            )

    def test_unclaimed_header_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = root / "mbo" / "proto"
            package.mkdir(parents=True)
            (package / "BUILD.bazel").write_text("")
            (package / "thing.h").touch()
            self.assertEqual(
                check_headers_claimed.check(["mbo/proto/thing.h"], root),
                ["mbo/proto/thing.h: not claimed by mbo/proto/BUILD.bazel"],
            )

    def test_header_uses_nearest_ancestor_package(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = root / "mbo" / "proto"
            nested = package / "detail"
            nested.mkdir(parents=True)
            (package / "BUILD.bazel").write_text('hdrs = ["detail/thing.h"]\n')
            (nested / "thing.h").touch()
            self.assertEqual(
                check_headers_claimed.check(["mbo/proto/detail/thing.h"], root), []
            )

    def test_missing_build_file_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "mbo").mkdir()
            self.assertEqual(
                check_headers_claimed.check(["mbo/orphan.h"], root),
                ["mbo/orphan.h: no owning BUILD file"],
            )


if __name__ == "__main__":
    unittest.main()
