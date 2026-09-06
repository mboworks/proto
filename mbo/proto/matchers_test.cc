// SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors, The CPP Proto Builder Authors
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mbo/proto/matchers.h"

#include <sstream>
#include <string>
#include <string_view>
#include <tuple>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/proto/parse_text_proto.h"
#include "mbo/proto/tests/test.pb.h"

namespace mbo::proto {
namespace {

struct ExplanationGetter {
  template<typename T, typename M>
  std::string operator()(const M& matcher, const T& value) const {
    std::stringstream sss;
    ::testing::SafeMatcherCast<const T&>(matcher).ExplainMatchResultTo(value, &sss);
    return sss.str();
  }
};

inline constexpr ExplanationGetter kGetExplanation;

using ::mbo::proto::ParseTextProtoOrDie;
using ::mbo::proto::tests::ExtensibleMessage;
using ::mbo::proto::tests::ExtensionContainer;
using ::mbo::proto::tests::RequiredMessage;
using ::mbo::proto::tests::TestMessage;
using ::mbo::proto::tests::TestMessage2;
using ::testing::EndsWith;
using ::testing::HasSubstr;
using ::testing::Matches;
using ::testing::Not;

TEST(Matchers, EqualsProto) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(num: 42 name: "name")pb");
  EXPECT_THAT(msg, EqualsProto(msg));
  EXPECT_THAT(msg, EqualsProto(R"pb(num: 42 name: "name")pb"));
  EXPECT_THAT(kGetExplanation(EqualsProto(R"pb(num: 43 name: "name")pb"), msg), EndsWith("modified: num: 43 -> 42"));
}

TEST(Matchers, EquivToProto) {
  const TestMessage msg1 = ParseTextProtoOrDie(R"pb(name: "name")pb");
  TestMessage msg2 = msg1;
  msg2.set_num(0);
  EXPECT_THAT(msg1, EquivToProto(msg1));
  EXPECT_THAT(msg1, EquivToProto(msg2));
  EXPECT_THAT(msg1, Not(EqualsProto(msg2)));
  EXPECT_THAT(msg1, EquivToProto(R"pb(num: 0 name: "name")pb"));
  EXPECT_THAT(msg1, EquivToProto(R"pb(name: "name")pb"));
  msg2.set_num(2);
  EXPECT_THAT(msg1, Not(EquivToProto(msg2)));
}

TEST(Matchers, Approximately) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(val: 1.0)pb");
  EXPECT_THAT(msg, Approximately(EqualsProto(R"pb(val: 0.992)pb"), 0.01));
  EXPECT_THAT(
      kGetExplanation(Approximately(EqualsProto(R"pb(val: 0.9)pb"), 0.01), msg), HasSubstr("modified: val: 0.9 -> 1"));
}

TEST(Matchers, RejectsInvalidApproximateFractions) {
  constexpr double kNegativeFraction = -0.1;
  constexpr double kTooLargeFraction = 1.1;
  EXPECT_DEATH(
      (void)Approximately(EqualsProto(TestMessage{}), 0.0, kNegativeFraction),
      "Fraction for Approximately must be >= 0.0 and < 1.0");
  EXPECT_DEATH(
      (void)Approximately(EqualsProto(TestMessage{}), 0.0, 1.0), "Fraction for Approximately must be >= 0.0 and < 1.0");
  EXPECT_DEATH(
      (void)Approximately(EqualsProto(), 0.0, kNegativeFraction), "Fraction for Relatively must be >= 0.0 and <= 1.0");
  EXPECT_DEATH(
      (void)Approximately(EqualsProto(), 0.0, kTooLargeFraction), "Fraction for Relatively must be >= 0.0 and <= 1.0");
}

TEST(Matchers, TreatingNaNsAsEqual) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(val: nan)pb");
  EXPECT_THAT(msg, TreatingNaNsAsEqual(EqualsProto(R"pb(val: nan)pb")));
  EXPECT_THAT(kGetExplanation(EqualsProto(R"pb(val: nan)pb"), msg), HasSubstr("val: nan -> nan"));
}

TEST(Matchers, IgnoringFields) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(name: "name" num: 42)pb");
  EXPECT_THAT(msg, IgnoringFields({"mbo.proto.tests.TestMessage.num"}, EqualsProto(R"pb(name: "name" num: 25)pb")));
  EXPECT_THAT(msg, IgnoringFields({"mbo.proto.tests.TestMessage.name"}, EqualsProto("num: 42")));
}

TEST(Matchers, IgnoringFieldPaths) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(name: "name" num: 42)pb");
  EXPECT_THAT(msg, IgnoringFieldPaths({"num"}, EqualsProto(R"pb(name: "name" num: 25)pb")));
  EXPECT_THAT(msg, IgnoringFieldPaths({"name"}, EqualsProto(R"pb(num: 42)pb")));
}

TEST(Matchers, IgnoringFieldPathsNested) {
  const TestMessage2 msg = ParseTextProtoOrDie(R"pb(one { name: "name" num: 42 })pb");
  EXPECT_THAT(msg, IgnoringFieldPaths({"one.name"}, EqualsProto(R"pb(one { num: 42 name: "different" })pb")));
  EXPECT_THAT(msg, Not(IgnoringFieldPaths({"one.name"}, EqualsProto(R"pb(one { num: 25 name: "different" })pb"))));
}

TEST(Matchers, IgnoringFieldPathsRepeatedIndex) {
  const TestMessage2 msg = ParseTextProtoOrDie(R"pb(
    more { num: 10 }
    more { num: 20 }
  )pb");
  EXPECT_THAT(msg, IgnoringFieldPaths({"more[0].num"}, EqualsProto(R"pb(
                                        more { num: 11 }
                                        more { num: 20 }
                                      )pb")));
}

TEST(Matchers, IgnoringFieldPathsSelectsTheRequestedParent) {
  const TestMessage2 actual = ParseTextProtoOrDie(R"pb(
    one { name: "different" }
    more { name: "first" }
    more { name: "different" }
  )pb");
  const std::string expected = R"pb(
    one { name: "expected" }
    more { name: "expected" }
    more { name: "expected" }
  )pb";

  EXPECT_THAT(actual, Not(IgnoringFieldPaths({"one.name"}, EqualsProto(expected))));
  EXPECT_THAT(actual, Not(IgnoringFieldPaths({"more[0].name"}, EqualsProto(expected))));
}

TEST(Matchers, IgnoresExtensionFieldPath) {
  const ExtensibleMessage actual = ParseTextProtoOrDie(R"pb([mbo.proto.tests.extension_value]: 42)pb");

  EXPECT_THAT(
      actual, IgnoringFieldPaths(
                  {"(mbo.proto.tests.extension_value)"}, EqualsProto(R"pb([mbo.proto.tests.extension_value]: 43)pb")));
}

TEST(Matchers, IgnoresNestedExtensionFieldPath) {
  const ExtensionContainer actual = ParseTextProtoOrDie(R"pb(child {
                                                               [mbo.proto.tests.extension_value]: 42
                                                             })pb");

  EXPECT_THAT(
      actual, IgnoringFieldPaths(
                  {"child.(mbo.proto.tests.extension_value)"}, EqualsProto(R"pb(child {
                                                                                  [mbo.proto.tests.extension_value]: 43
                                                                                })pb")));
}

TEST(Matchers, IgnoringFieldPathsRepeatedNested) {
  const TestMessage2 msg = ParseTextProtoOrDie(R"pb(
    more { num: 10 }
    more { num: 20 }
  )pb");
  EXPECT_THAT(msg, IgnoringFieldPaths({"more.num"}, EqualsProto(R"pb(
                                        more { num: 20 }
                                        more { num: 10 }
                                      )pb")));
}

TEST(Matchers, IgnoringFieldPathsTerminalIndex) {
  const TestMessage2 msg = ParseTextProtoOrDie(R"pb(num: 1 num: 2)pb");
  EXPECT_DEATH(
      Matches(IgnoringFieldPaths({"num[0]"}, EqualsProto(R"pb(num: 1 num: 2)pb")))(msg),
      "Check failed: field_path.back\\(\\).index == -1 "
      "\\(0 vs. -1\\) "
      "Terminally ignoring fields by index is currently not supported "
      "\\('num\\[0\\]'\\)");
}

TEST(Matchers, IgnoringRepeatedFieldOrdering) {
  const TestMessage2 msg = ParseTextProtoOrDie(R"pb(num: 1 num: 2)pb");
  EXPECT_THAT(msg, IgnoringRepeatedFieldOrdering(EqualsProto(R"pb(num: 2 num: 1)pb")));
}

TEST(Matchers, IgnoringRepeatedFieldOrderingNested) {
  const TestMessage2 msg = ParseTextProtoOrDie(R"pb(
    more { num: 10 }
    more { num: 20 }
  )pb");
  EXPECT_THAT(msg, IgnoringRepeatedFieldOrdering(EqualsProto(R"pb(
                more { num: 20 }
                more { num: 10 }
              )pb")));
}

TEST(Matchers, Partially) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(name: "name" num: 42)pb");
  EXPECT_THAT(msg, Partially(EqualsProto(R"pb(num: 42)pb")));
  EXPECT_THAT(msg, Partially(EqualsProto(R"pb(name: "name")pb")));
}

TEST(Matchers, DescribesConfiguredComparison) {
  const auto matcher = TreatingNaNsAsEqual(Partially(IgnoringRepeatedFieldOrdering(IgnoringFields(
      {"mbo.proto.tests.TestMessage.name", "mbo.proto.tests.TestMessage.num"},
      Approximately(EqualsProto(R"pb(val: 1)pb"), 0.25, 0.125)))));

  EXPECT_THAT(
      ::testing::DescribeMatcher<TestMessage>(matcher),
      HasSubstr("is (ignoring repeated field ordering) (ignoring fields: "
                "mbo.proto.tests.TestMessage.name, mbo.proto.tests.TestMessage.num) approximately "
                "(absolute error of float or double fields <= 0.25 or relative error of float or double fields <= "
                "0.125) partially equal (treating NaNs as equal) to <val: 1>"));
  EXPECT_THAT(
      ::testing::DescribeMatcher<TestMessage>(matcher, true), HasSubstr("is not (ignoring repeated field ordering)"));
}

TEST(Matchers, DescribesIndividualComparisonModes) {
  constexpr double kFraction = 0.125;
  const TestMessage expected = ParseTextProtoOrDie(R"pb(val: 1)pb");
  const auto approximate = Approximately(EqualsProto(expected));
  const auto margin = Approximately(EqualsProto(expected), 0.25);
  auto fraction = Approximately(EqualsProto(expected));
  fraction.mutable_impl().SetFraction(kFraction);

  EXPECT_THAT(::testing::DescribeMatcher<TestMessage>(approximate), HasSubstr("approximately equal"));
  EXPECT_THAT(
      ::testing::DescribeMatcher<TestMessage>(margin), HasSubstr("absolute error of float or double fields <= 0.25"));
  EXPECT_THAT(
      ::testing::DescribeMatcher<TestMessage>(fraction),
      HasSubstr("relative error of float or double fields <= 0.125"));
  EXPECT_THAT(::testing::DescribeMatcher<TestMessage>(EquivToProto(expected)), HasSubstr("equivalent to"));
  EXPECT_THAT(::testing::DescribeMatcher<TestMessage>(Partially(EqualsProto(expected))), HasSubstr("partially equal"));
  const TestMessage close = ParseTextProtoOrDie(R"pb(val: 1.000001)pb");
  EXPECT_THAT(expected, approximate);
  EXPECT_THAT(close, fraction);
}

TEST(Matchers, RequiresInitializedMessagesWhenConfigured) {
  const RequiredMessage initialized = ParseTextProtoOrDie(R"pb(value: 42)pb");
  const RequiredMessage uninitialized;
  const internal::ProtoComparison comparison;
  const auto matcher =
      ::testing::MakePolymorphicMatcher(internal::ProtoMatcher(initialized, internal::kMustBeInitialized, comparison));

  EXPECT_THAT(initialized, matcher);
  EXPECT_THAT(kGetExplanation(matcher, uninitialized), HasSubstr("isn't fully initialized"));
  EXPECT_THAT(::testing::DescribeMatcher<RequiredMessage>(matcher), HasSubstr("fully initialized and"));
  EXPECT_THAT(::testing::DescribeMatcher<RequiredMessage>(matcher, true), HasSubstr("not fully initialized or not"));
  EXPECT_THAT(matcher.impl().must_be_initialized(), true);
}

TEST(Matchers, ReportsPointersAndNullPointers) {
  const TestMessage expected = ParseTextProtoOrDie(R"pb(num: 42)pb");
  const TestMessage actual = ParseTextProtoOrDie(R"pb(num: 43)pb");
  const TestMessage* actual_ptr = &actual;
  const TestMessage* null_ptr = nullptr;

  EXPECT_THAT(kGetExplanation(EqualsProto(expected), actual_ptr), HasSubstr("which points to"));
  EXPECT_THAT(kGetExplanation(EqualsProto(expected), actual_ptr), HasSubstr("modified: num: 42 -> 43"));
  const TestMessage* expected_ptr = &expected;
  EXPECT_THAT(expected_ptr, EqualsProto(expected));
  EXPECT_THAT(null_ptr, Not(EqualsProto(expected)));
  EXPECT_THAT(internal::PrintProtoPointee(nullptr), "");
}

TEST(Matchers, ReportsIncompatibleMessageTypes) {
  const TestMessage expected = ParseTextProtoOrDie(R"pb(num: 42)pb");
  const TestMessage2 actual = ParseTextProtoOrDie(R"pb(num: 42)pb");

  EXPECT_THAT(
      kGetExplanation(EqualsProto(expected), actual),
      HasSubstr("whose type should be mbo.proto.tests.TestMessage but actually is "
                "mbo.proto.tests.TestMessage2"));
  const internal::ProtoComparison comparison;
  EXPECT_THAT(internal::ProtoCompare(comparison, expected, actual), false);
  const TestMessage2* actual_ptr = &actual;
  EXPECT_THAT(kGetExplanation(EqualsProto(expected), actual_ptr), HasSubstr("which points to"));
}

TEST(Matchers, ReportsMalformedExpectedText) {
  const TestMessage actual;

  EXPECT_THAT(kGetExplanation(EqualsProto("not_a_field: 1"), actual), HasSubstr("doesn't parse as a"));
  EXPECT_THAT(kGetExplanation(EqualsProto("not_a_field: 1"), actual), HasSubstr("no field named"));
  EXPECT_THAT(Matches(EqualsProto("not_a_field: 1"))(actual), false);
}

TEST(Matchers, ReportsNoDifferenceForEqualMessages) {
  const TestMessage message = ParseTextProtoOrDie(R"pb(num: 42)pb");
  const internal::ProtoComparison comparison;

  EXPECT_THAT(internal::DescribeDiff(comparison, message, message), "with the difference:\n");
}

TEST(Matchers, RejectsInvalidIgnoreConfiguration) {
  const TestMessage message;

  EXPECT_DEATH(Matches(IgnoringFields({"missing.field"}, EqualsProto(message)))(message), "Could not find fields");
  EXPECT_DEATH(Matches(IgnoringFieldPaths({""}, EqualsProto(message)))(message), "field_path.empty");
  EXPECT_DEATH(Matches(IgnoringFieldPaths({"missing"}, EqualsProto(message)))(message), "No such field");
  const TestMessage2 nested;
  EXPECT_DEATH(Matches(IgnoringFieldPaths({"one.missing"}, EqualsProto(nested)))(nested), "No such field");
  EXPECT_DEATH(Matches(IgnoringFieldPaths({"name..value"}, EqualsProto(message)))(message), "expected field");
  EXPECT_DEATH(
      Matches(IgnoringFieldPaths({"(missing.extension)"}, EqualsProto(message)))(message), "No such extension");
  EXPECT_DEATH(
      Matches(IgnoringFieldPaths({"(mbo.proto.tests.extension_value)"}, EqualsProto(message)))(message),
      "does not extend message");
}

TEST(Matchers, ComparesTypedTextMatchers) {
  const TestMessage actual = ParseTextProtoOrDie(R"pb(num: 42)pb");

  EXPECT_THAT(actual, EqualsProto<TestMessage>(R"pb(num: 42)pb"));
  EXPECT_THAT(actual, EquivToProto<TestMessage>(R"pb(num: 42)pb"));
}

TEST(Matchers, ComparesTuples) {
  using MessageTuple = std::tuple<TestMessage, TestMessage>;
  const TestMessage empty;
  TestMessage explicit_default;
  explicit_default.set_num(0);
  const MessageTuple equal{empty, empty};
  const MessageTuple equivalent{empty, explicit_default};
  const auto equals = static_cast<::testing::Matcher<const MessageTuple&>>(EqualsProto());
  const auto equiv = static_cast<::testing::Matcher<const MessageTuple&>>(EquivToProto());

  EXPECT_THAT(equal, equals);
  EXPECT_THAT(equivalent, Not(equals));
  EXPECT_THAT(equivalent, equiv);
  const std::string equals_description = ::testing::DescribeMatcher<const MessageTuple&>(equals);
  const std::string equiv_negation = ::testing::DescribeMatcher<const MessageTuple&>(equiv, true);
  EXPECT_THAT(equals_description, HasSubstr("are equal"));
  EXPECT_THAT(equiv_negation, HasSubstr("are not equivalent"));
  const internal::TupleProtoMatcher copied = EqualsProto();
  const auto copied_matcher = static_cast<::testing::Matcher<const MessageTuple&>>(copied);
  EXPECT_THAT(equal, copied_matcher);
}

TEST(Matchers, ConfiguresTupleComparisons) {
  using MessageTuple = std::tuple<TestMessage, TestMessage>;
  using RepeatedTuple = std::tuple<TestMessage2, TestMessage2>;
  const TestMessage approximate = ParseTextProtoOrDie(R"pb(val: 1)pb");
  const TestMessage close = ParseTextProtoOrDie(R"pb(val: 1.05)pb");
  const TestMessage nan = ParseTextProtoOrDie(R"pb(val: nan)pb");
  const TestMessage with_name = ParseTextProtoOrDie(R"pb(name: "ignored" num: 42)pb");
  const TestMessage without_name = ParseTextProtoOrDie(R"pb(num: 42)pb");
  const TestMessage2 ordered = ParseTextProtoOrDie(R"pb(num: 1 num: 2)pb");
  const TestMessage2 reversed = ParseTextProtoOrDie(R"pb(num: 2 num: 1)pb");

  EXPECT_THAT(
      MessageTuple(approximate, close),
      static_cast<::testing::Matcher<const MessageTuple&>>(Approximately(EqualsProto(), 0.01, 0.1)));
  EXPECT_THAT(
      MessageTuple(nan, nan), static_cast<::testing::Matcher<const MessageTuple&>>(TreatingNaNsAsEqual(EqualsProto())));
  EXPECT_THAT(
      MessageTuple(with_name, without_name), static_cast<::testing::Matcher<const MessageTuple&>>(
                                                 IgnoringFields({"mbo.proto.tests.TestMessage.name"}, EqualsProto())));
  EXPECT_THAT(
      MessageTuple(with_name, without_name),
      static_cast<::testing::Matcher<const MessageTuple&>>(IgnoringFieldPaths({"name"}, EqualsProto())));
  EXPECT_THAT(
      RepeatedTuple(ordered, reversed),
      static_cast<::testing::Matcher<const RepeatedTuple&>>(IgnoringRepeatedFieldOrdering(EqualsProto())));
  const auto repeated_equals = static_cast<::testing::Matcher<const RepeatedTuple&>>(EqualsProto());
  const auto repeated_equiv = static_cast<::testing::Matcher<const RepeatedTuple&>>(EquivToProto());
  EXPECT_THAT(::testing::DescribeMatcher<const RepeatedTuple&>(repeated_equals), "are equal");
  EXPECT_THAT(::testing::DescribeMatcher<const RepeatedTuple&>(repeated_equals, true), "are not equal");
  EXPECT_THAT(::testing::DescribeMatcher<const RepeatedTuple&>(repeated_equiv), "are equivalent");
  EXPECT_THAT(::testing::DescribeMatcher<const RepeatedTuple&>(repeated_equiv, true), "are not equivalent");
  EXPECT_THAT(
      MessageTuple(with_name, without_name),
      static_cast<::testing::Matcher<const MessageTuple&>>(Partially(EqualsProto())));
}

TEST(Matchers, MatchesSerializedMessages) {
  const TestMessage expected = ParseTextProtoOrDie(R"pb(name: "name" num: 42)pb");
  const TestMessage different = ParseTextProtoOrDie(R"pb(num: 43)pb");
  const std::string serialized = expected.SerializePartialAsString();
  const std::string malformed("\x0a", 1);

  EXPECT_THAT(serialized, WhenDeserialized(EqualsProto(expected)));
  EXPECT_THAT(std::string_view(serialized), WhenDeserializedAs<TestMessage>(EqualsProto(expected)));
  EXPECT_THAT(serialized.c_str(), WhenDeserializedAs<TestMessage>(EqualsProto(expected)));
  EXPECT_THAT(malformed, Not(WhenDeserialized(EqualsProto(expected))));
  EXPECT_THAT(Matches(WhenDeserialized(EqualsProto(expected)))(serialized), true);
  EXPECT_THAT(Matches(WhenDeserialized(EqualsProto(expected)))(malformed), false);
  EXPECT_THAT(Matches(WhenDeserialized(EqualsProto(different)))(serialized), false);
  EXPECT_THAT(
      kGetExplanation(WhenDeserialized(EqualsProto(expected)), malformed),
      HasSubstr("cannot be deserialized as a mbo.proto.tests.TestMessage"));
  EXPECT_THAT(
      kGetExplanation(WhenDeserializedAs<TestMessage>(EqualsProto(expected)), serialized),
      HasSubstr("which deserializes to"));
  EXPECT_THAT(
      kGetExplanation(WhenDeserializedAs<TestMessage>(EqualsProto(R"pb(num: 43)pb")), serialized),
      HasSubstr("modified: num: 43 -> 42"));
  EXPECT_THAT(
      ::testing::DescribeMatcher<std::string>(WhenDeserialized(EqualsProto(expected))),
      HasSubstr("can be deserialized as a protobuf that is equal"));
  EXPECT_THAT(
      ::testing::DescribeMatcher<std::string>(WhenDeserializedAs<TestMessage>(EqualsProto(expected)), true),
      HasSubstr("cannot be deserialized as a mbo.proto.tests.TestMessage that is equal"));
}

}  // namespace
}  // namespace mbo::proto
