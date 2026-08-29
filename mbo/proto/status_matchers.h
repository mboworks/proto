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

// -----------------------------------------------------------------------------
// Do not use this elsewhere. Instead:
//   - just upgrade your Abseil dependency.
//     #include "absl/status/status_matchers"
//   - use @mboworks_mbo//mbo/testing:status_cc
//     #include "mbo/testing/status.h"
//
// These are layman's implementations to simplify testwriting without incuring
// additional requirements for external libraries. The code will deleted in the
// future and replaced with Abseil functionality.
// -----------------------------------------------------------------------------

#ifndef MBO_PROTO_STATUS_MATCHERS_H_
#define MBO_PROTO_STATUS_MATCHERS_H_

#include <concepts>
#include <type_traits>

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::proto {

using ::testing::ExplainMatchResult;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Optional;

MATCHER(IsOk, "") {
  const absl::Status status = [&] {
    if constexpr (std::same_as<absl::Status, std::remove_cvref_t<decltype(arg)>>) {
      return arg;
    } else {
      return arg.status();
    }
  }();
  if (status.ok()) {
    *result_listener << "argument is OK";
    return true;
  }
  *result_listener << "argument is not OK and has status " << status.ToString();
  return false;
}

MATCHER_P(IsOkAndHolds, matcher, "") {
  return ExplainMatchResult(IsOk(), arg, result_listener) && ExplainMatchResult(matcher, *arg, result_listener);
}

MATCHER_P2(StatusIs, code_matcher, message_matcher, "") {
  const absl::Status status = [&] {
    if constexpr (std::same_as<absl::Status, std::remove_cvref_t<decltype(arg)>>) {
      return arg;
    } else {
      return arg.status();
    }
  }();
  return ExplainMatchResult(Not(IsOk()), status, result_listener)
         && ExplainMatchResult(code_matcher, status.code(), result_listener)
         && ExplainMatchResult(message_matcher, status.message(), result_listener);
}

}  // namespace mbo::proto

#endif  // MBO_PROTO_STATUS_MATCHERS_H_
