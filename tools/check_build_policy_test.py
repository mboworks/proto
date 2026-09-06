# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Tests for BUILD target policy."""

import unittest

from tools import check_build_policy


_LIBRARY = '''\
cc_library(
    name = "thing_cc",
)
'''
_TEST = '''\
cc_test(
    name = "thing_test",
    size = "small",
    deps = [":thing_cc"],
)
'''


class CheckBuildPolicyTest(unittest.TestCase):
    def test_accepts_named_sized_directly_tested_library(self):
        self.assertEqual(
            check_build_policy.violations(_LIBRARY + _TEST, "mbo/proto"), []
        )

    def test_rejects_library_without_cc_suffix(self):
        text = (_LIBRARY + _TEST).replace("thing_cc", "thing")
        self.assertIn(
            (1, "cc_library 'thing' must end in '_cc'"),
            check_build_policy.violations(text, "mbo/proto"),
        )

    def test_rejects_untested_library(self):
        self.assertIn(
            (
                1,
                "cc_library 'thing_cc' has no same-package test with a direct dependency",
            ),
            check_build_policy.violations(_LIBRARY, "mbo/proto"),
        )

    def test_transitive_dependency_does_not_count(self):
        outer = 'cc_library(\n    name = "outer_cc",\n    deps = [":thing_cc"],\n)\n'
        test = _TEST.replace(":thing_cc", ":outer_cc")
        problems = check_build_policy.violations(_LIBRARY + outer + test, "mbo/proto")
        self.assertIn(
            (
                1,
                "cc_library 'thing_cc' has no same-package test with a direct dependency",
            ),
            problems,
        )

    def test_fully_qualified_same_package_dependency_counts(self):
        test = _TEST.replace(":thing_cc", "//mbo/proto:thing_cc")
        self.assertEqual(
            check_build_policy.violations(_LIBRARY + test, "mbo/proto"), []
        )

    def test_other_package_dependency_does_not_count(self):
        test = _TEST.replace(":thing_cc", "//mbo/other:thing_cc")
        self.assertTrue(
            any("no same-package test" in problem for _, problem in check_build_policy.violations(_LIBRARY + test, "mbo/proto"))
        )

    def test_rejects_direct_test_without_size(self):
        text = _LIBRARY + _TEST.replace('    size = "small",\n', "")
        self.assertIn(
            (4, "cc_test 'thing_test' has no explicit size"),
            check_build_policy.violations(text, "mbo/proto"),
        )

    def test_project_test_macro_is_not_treated_as_direct_rule(self):
        text = 'custom_test(\n    name = "custom",\n)\n'
        self.assertEqual(check_build_policy.violations(text, "mbo/proto"), [])


if __name__ == "__main__":
    unittest.main()
