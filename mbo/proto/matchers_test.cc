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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/proto/parse_text_proto.h"
#include "mbo/proto/tests/test.pb.h"

namespace mbo::proto {
namespace {

using ::mbo::proto::ParseTextProtoOrDie;
using ::mbo::proto::tests::TestMessage;
using ::mbo::proto::tests::TestMessage2;
using ::testing::EndsWith;
using ::testing::HasSubstr;
using ::testing::Matches;
using ::testing::Not;
using ::testing::SafeMatcherCast;

template<typename T, typename M>
inline std::string GetExplanation(const M& matcher, const T& value) {
  std::stringstream sss;
  SafeMatcherCast<const T&>(matcher).ExplainMatchResultTo(value, &sss);
  return sss.str();
}

TEST(Matchers, EqualsProto) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(num: 42 name: "name")pb");
  EXPECT_THAT(msg, EqualsProto(msg));
  EXPECT_THAT(msg, EqualsProto(R"pb(num: 42 name: "name")pb"));
  EXPECT_THAT(GetExplanation(EqualsProto(R"pb(num: 43 name: "name")pb"), msg), EndsWith("modified: num: 43 -> 42"));
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
      GetExplanation(Approximately(EqualsProto(R"pb(val: 0.9)pb"), 0.01), msg), HasSubstr("modified: val: 0.9 -> 1"));
}

TEST(Matchers, TreatingNaNsAsEqual) {
  const TestMessage msg = ParseTextProtoOrDie(R"pb(val: nan)pb");
  EXPECT_THAT(msg, TreatingNaNsAsEqual(EqualsProto(R"pb(val: nan)pb")));
  EXPECT_THAT(GetExplanation(EqualsProto(R"pb(val: nan)pb"), msg), HasSubstr("val: nan -> nan"));
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

}  // namespace
}  // namespace mbo::proto
