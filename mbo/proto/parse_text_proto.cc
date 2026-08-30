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

#include "mbo/proto/parse_text_proto.h"

#include <source_location>
#include <string>
#include <string_view>

#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "mbo/proto/silent_error_collector.h"

namespace mbo::proto::proto_internal {

absl::Status ParseTextInternal(
    std::string_view text_proto,
    ::google::protobuf::Message* message,
    std::string_view func,
    std::source_location loc) {
  google::protobuf::TextFormat::Parser parser;
  SilentErrorCollector error_collector;
  parser.RecordErrorsTo(error_collector);
  if (parser.ParseFromString(std::string(text_proto), message)) {
    return absl::OkStatus();
  }
  return absl::InvalidArgumentError(absl::StrFormat(
      "%s<%s>\nFile: '%s', Line: %d: %s\nError: %s", func, message->GetDescriptor()->name(), loc.file_name(),
      loc.line(), loc.function_name(), error_collector.GetErrors()));
}

void ParseTextOrDieInternal(
    std::string_view text,
    ::google::protobuf::Message* message,
    std::string_view func,
    std::source_location loc) {
  const absl::Status status = ParseTextInternal(text, message, func, loc);
  if (!status.ok()) {
    ABSL_LOG(FATAL) << "Check failed: " << status;
  }
}

}  // namespace mbo::proto::proto_internal
