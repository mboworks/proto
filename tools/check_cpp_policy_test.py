# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Tests for C++ source policy."""

import unittest

from tools import check_cpp_policy


class CheckCppPolicyTest(unittest.TestCase):
    def messages(self, text: str) -> list[str]:
        return [problem.message for problem in check_cpp_policy.violations(text)]

    def test_rejects_gtest_comparison_macro(self):
        self.assertIn(
            "use EXPECT_THAT or ASSERT_THAT with a matcher",
            self.messages("EXPECT_EQ(actual, expected);"),
        )

    def test_rejects_qualified_matcher(self):
        self.assertIn(
            "import matchers with a using declaration",
            self.messages("EXPECT_THAT(value, testing::ElementsAre(1));"),
        )

    def test_allows_qualified_testing_utility(self):
        self.assertEqual(
            self.messages("return ::testing::MakePolymorphicMatcher(value);"), []
        )

    def test_rejects_inline_braced_range(self):
        self.assertIn(
            "range-for requires a named constexpr array",
            self.messages("for (int value : {1, 2}) {}"),
        )

    def test_rejects_comparison_inside_boolean_check(self):
        self.assertIn(
            "use a comparison-specific CHECK macro",
            self.messages("ABSL_CHECK(value == expected);"),
        )

    def test_allows_comparison_specific_check(self):
        self.assertEqual(self.messages("ABSL_CHECK_EQ(value, expected);"), [])

    def test_rejects_is_ok_then_dereference(self):
        text = "ASSERT_THAT(result, IsOk());\nauto value = *result;\n"
        self.assertIn(
            "use IsOkAndHolds instead of asserting IsOk then dereferencing",
            self.messages(text),
        )

    def test_allows_is_ok_without_dereference(self):
        self.assertEqual(self.messages("ASSERT_THAT(status, IsOk());"), [])

    def test_ignores_line_comments(self):
        self.assertEqual(self.messages("// EXPECT_EQ(actual, expected);"), [])


if __name__ == "__main__":
    unittest.main()
