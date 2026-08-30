// SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
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

#include "mbo/proto/silent_error_collector.h"

#include <string>
#include <string_view>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"

namespace mbo::proto {

std::string SilentErrorCollector::FormatError(const ErrorInfo& error) const {
  return absl::StrFormat("Line %d, Col %d: %s", error.line, error.column, error.message);
}

std::string SilentErrorCollector::GetErrors(std::string_view joiner) const {
  return absl::StrJoin(
      errors(), joiner, [this](std::string* out, const ErrorInfo& error) { absl::StrAppend(out, FormatError(error)); });
}

}  // namespace mbo::proto
