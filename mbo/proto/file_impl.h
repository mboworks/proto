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

#ifndef MBO_PROTO_FILE_IMPL_H_
#define MBO_PROTO_FILE_IMPL_H_

// IWYU pragma: private, include "mbo/proto/file.h"

#include <filesystem>
#include <source_location>

#include "absl/status/status.h"
#include "google/protobuf/message.h"

namespace mbo::proto::proto_internal {

absl::Status ReadBinaryProtoFile(
    const std::filesystem::path& filename,
    ::google::protobuf::Message& result,
    const std::source_location& src_loc);

absl::Status ReadTextProtoFile(
    const std::filesystem::path& filename,
    ::google::protobuf::Message& result,
    const std::source_location& src_loc);

}  // namespace mbo::proto::proto_internal

#endif  // MBO_PROTO_FILE_IMPL_H_
