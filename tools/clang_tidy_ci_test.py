# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

import unittest

from tools import clang_tidy_ci


class ClangTidyCiTest(unittest.TestCase):
    def test_main_push_requests_whole_tree(self):
        self.assertEqual(clang_tidy_ci.select_inputs("push", []), ["--all-files"])

    def test_main_push_ignores_commit_diff(self):
        self.assertEqual(
            clang_tidy_ci.select_inputs("push", ["mbo/proto/file.cc"]),
            ["--all-files"],
        )

    def test_pull_request_preserves_complete_deduplicated_diff(self):
        self.assertEqual(
            clang_tidy_ci.select_inputs(
                "pull_request",
                ["README.md", "mbo/proto/file.cc", "README.md"],
            ),
            ["README.md", "mbo/proto/file.cc"],
        )

    def test_rejects_unsupported_event(self):
        with self.assertRaisesRegex(ValueError, "unsupported GitHub Actions event"):
            clang_tidy_ci.select_inputs("workflow_dispatch", [])


if __name__ == "__main__":
    unittest.main()
