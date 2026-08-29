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

#include "mbo/proto/file.h"

#include <fstream>
#include <source_location>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "mbo/proto/silent_error_collector.h"

namespace mbo::proto {
namespace {

std::string SrcLoc(const std::source_location& sloc) {
  return absl::StrFormat("%s:%d", sloc.file_name(), sloc.line());
}

bool PrintTextMessage(const ::google::protobuf::Message& proto, std::ofstream& output) {
  ::google::protobuf::io::OstreamOutputStream zstream(&output);
  return ::google::protobuf::TextFormat::Print(proto, &zstream);
}

}  // namespace

namespace proto_internal {

absl::Status ReadBinaryProtoFile(
    const std::filesystem::path& filename,
    ::google::protobuf::Message& result,
    const std::source_location& src_loc) {
  std::ifstream input(filename, std::ios::binary);
  if (!input.good()) {
    return absl::NotFoundError(absl::StrFormat("Cannot open '%s' @ %s", filename, SrcLoc(src_loc)));
  }
  if (!result.ParseFromIstream(&input)) {
    return absl::AbortedError(absl::StrFormat("Cannot parse binary proto file '%s' @%s.", filename, SrcLoc(src_loc)));
  }
  if (!result.IsInitialized()) {
    return absl::DataLossError(absl::StrFormat(
        "Cannot read binary proto file '%s' with uninitialized '%s' @ %s: %s", filename, result.GetTypeName(),
        SrcLoc(src_loc), result.InitializationErrorString()));
  }
  return absl::OkStatus();
}

absl::Status ReadTextProtoFile(
    const std::filesystem::path& filename,
    ::google::protobuf::Message& result,
    const std::source_location& src_loc) {
  std::ifstream input(filename, std::ios::binary);
  if (!input.good()) {
    return absl::NotFoundError(absl::StrFormat("Cannot open '%s' @ %s", filename, SrcLoc(src_loc)));
  }
  ::google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(false);
  SilentErrorCollector error_collector;
  parser.RecordErrorsTo(error_collector);
  ::google::protobuf::io::IstreamInputStream zstream(&input);
  if (parser.Parse(&zstream, &result)) {
    return absl::OkStatus();
  }
  return absl::AbortedError(absl::StrFormat(
      "Cannot parse text proto file '%s' @%s: %s.", filename, SrcLoc(src_loc), error_collector.GetErrors(", ")));
}

}  // namespace proto_internal

bool HasBinaryProtoExtension(std::string_view filename) {
  return filename.ends_with(".binpb") ||  // NL
         filename.ends_with(".pb");
}

bool HasTextProtoExtension(std::string_view filename) {
  return filename.ends_with(".txtpb") ||  // NL
         filename.ends_with(".textproto");
}

absl::Status WriteBinaryProtoFile(
    const std::filesystem::path& filename,
    const ::google::protobuf::Message& proto,
    const std::source_location& src_loc) {
  std::ofstream output(filename, std::ios::binary);
  if (output.good() && proto.SerializeToOstream(&output)) {
    output.close();
    if (output.good()) {
      return absl::OkStatus();
    }
  }
  return absl::AbortedError(absl::StrFormat("Cannot write binary proto file '%s' @ %s.", filename, SrcLoc(src_loc)));
}

absl::Status WriteTextProtoFile(
    const std::filesystem::path& filename,
    const ::google::protobuf::Message& proto,
    const std::source_location& src_loc) {
  std::ofstream output(filename, std::ios::binary);
  if (output.good() && PrintTextMessage(proto, output)) {
    return absl::OkStatus();
  }
  return absl::AbortedError(absl::StrFormat("Cannot write text proto file '%s' @ %s.", filename, SrcLoc(src_loc)));
}

}  // namespace mbo::proto
