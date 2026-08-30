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

#ifndef MBO_PROTO_FILE_H_
#define MBO_PROTO_FILE_H_

#include <concepts>
#include <filesystem>
#include <optional>
#include <source_location>

#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "google/protobuf/message.h"
#include "mbo/proto/file_impl.h"  // IWYU pragma: export

// Functionality for reading and writing protos:
// - functions        Has(Binary|Text)ProtoExtension
// - concept          IsProtoType
// - struct/function  Read(Binary|Text)ProtoFile(::(As|OrDie|OrNullopt))?
// - struct/function  Write(Binary|Text)ProtoFile

namespace mbo::proto {

// Identifies binary proto filenames.
// See https://protobuf.dev/programming-guides/techniques/
// See https://en.wikipedia.org/wiki/Filename_extension (extension with dot)
// Added [".pb"] which is also fairly common.
bool HasBinaryProtoExtension(std::string_view filename);

inline bool HasBinaryProtoExtension(const std::same_as<std::filesystem::path> auto& filename) {
  return HasBinaryProtoExtension(filename.native());
}

// Identifies text proto filenames.
// See https://protobuf.dev/programming-guides/techniques/
// See https://en.wikipedia.org/wiki/Filename_extension (extension with dot)
bool HasTextProtoExtension(std::string_view filename);

inline bool HasTextProtoExtension(const std::same_as<std::filesystem::path> auto& filename) {
  return HasTextProtoExtension(filename.native());
}

// Concept that identifies Proto types as opposed to the base proto `Message`.
template<typename ProtoType>
concept IsProtoType =
    std::derived_from<ProtoType, ::google::protobuf::Message> && !std::same_as<ProtoType, ::google::protobuf::Message>;

// Type erasure proto reader for binary proto files.
//
// Example:
//
// ```c++
// const MyProto proto = ReadBinaryProtoFile(proto_filename);
// const absl::StatusOr<MyProto> proto_or_error = ReadBinaryProtoFile(proto_filename);
// const std::optional<MyProto> proto_or_nullopt = ReadBinaryProtoFile(proto_filename);
// ```
//
//
// In the above the first call requires the file to be readable as a `MyProto`.
// The program will abort with a check violation if the file cannot be read.
//
// The second call returns either the read protocol buffer or an error status.
// In this version the return value forces the caller to handle any errors.
//
// The third call returns either the read protocol buffer ot std::nullopt.
// In this verion the caller is responsible for error handling.
//
// The class also supports static direct typed access by functions whose
// addresses can be taken.
class ReadBinaryProtoFile {
 public:
  // Static read function - so it's address can be taken.
  template<IsProtoType ProtoType>
  static absl::StatusOr<ProtoType> As(
      const std::filesystem::path& filename,
      const std::source_location& src_loc = std::source_location::current()) {
    ProtoType result;
    const auto status = proto_internal::ReadBinaryProtoFile(filename, result, src_loc);
    if (!status.ok()) {
      return status;
    }
    return result;
  }

  // Static read function - so it's address can be taken.
  template<IsProtoType ProtoType>
  static std::optional<ProtoType> OrNullopt(
      const std::filesystem::path& filename,
      const std::source_location& src_loc = std::source_location::current()) {
    if (auto result = As<ProtoType>(filename, src_loc); result.ok()) {
      return *std::move(result);
    }
    return std::nullopt;
  }

  // Static read function - so it's address can be taken.
  template<IsProtoType ProtoType>
  static ProtoType OrDie(
      const std::filesystem::path& filename,
      const std::source_location& src_loc = std::source_location::current()) {
    return *As<ProtoType>(filename, src_loc);
  }

  ReadBinaryProtoFile() = delete;

  explicit ReadBinaryProtoFile(
      std::filesystem::path filename,
      const std::source_location& src_loc = std::source_location::current())
      : filename_(std::move(filename)), src_loc_(src_loc) {}

  ReadBinaryProtoFile(const ReadBinaryProtoFile&) = delete;
  ReadBinaryProtoFile& operator=(const ReadBinaryProtoFile&) = delete;
  ReadBinaryProtoFile(ReadBinaryProtoFile&&) = delete;
  ReadBinaryProtoFile& operator=(ReadBinaryProtoFile&&) = delete;

  ~ReadBinaryProtoFile() { ABSL_LOG_IF(DFATAL, !converted_) << "ReadBinaryProtoFile has not read the file."; }

  template<int&..., IsProtoType ProtoType>
  operator absl::StatusOr<ProtoType>() const {  // NOLINT(*-explicit-*)
    converted_ = true;
    ProtoType result;
    const auto status = proto_internal::ReadBinaryProtoFile(filename_, result, src_loc_);
    if (!status.ok()) {
      return status;
    }
    return result;
  }

  template<int&..., IsProtoType ProtoType>
  operator std::optional<ProtoType>() const {  // NOLINT(*-explicit-*)
    absl::StatusOr<ProtoType> proto = *this;
    if (!proto.ok()) {
      return std::nullopt;
    }
    return *proto;
  }

  template<int&..., IsProtoType ProtoType>
  operator ProtoType() const {  // NOLINT(*-explicit-*)
    absl::StatusOr<ProtoType> proto = *this;
    ABSL_LOG_IF(FATAL, !proto.ok()).AtLocation(src_loc_.file_name(), static_cast<int>(src_loc_.line()))
        << proto.status();
    return *proto;
  }

 private:
  const std::filesystem::path filename_;
  const std::source_location src_loc_;
  mutable bool converted_ = false;
};

// Writes a binary proto file.
absl::Status WriteBinaryProtoFile(
    const std::filesystem::path& filename,
    const ::google::protobuf::Message& proto,
    const std::source_location& src_loc = std::source_location::current());

// Type erasure proto reader for text proto files.
//
// Example:
//
// ```c++
// const MyProto proto = ReadTextProtoFile(filename);
// const absl::StatusOr<MyProto> proto_or_error = ReadTextProtoFile(filename);
// const std::optional<MyProto> proto_or_nullopt = ReadTextProtoFile(filename);
// ```
//
// In the above the first call requires the file to be readable as a `MyProto`.
// The program will abort with a check violation if the file cannot be read.
//
// The second call returns either the read protocol buffer or an error status.
// In this version the return value forces the caller to handle any errors.
//
// The third call returns either the read protocol buffer ot std::nullopt.
// In this verion the caller is responsible for error handling.
//
// The class also supports static direct typed access by functions whose
// addresses can be taken.

class ReadTextProtoFile {
 public:
  // Static read function - so it's address can be taken.
  template<IsProtoType ProtoType>
  static absl::StatusOr<ProtoType> As(
      const std::filesystem::path& filename,
      const std::source_location& src_loc = std::source_location::current()) {
    ProtoType result;
    const auto status = proto_internal::ReadTextProtoFile(filename, result, src_loc);
    if (!status.ok()) {
      return status;
    }
    return result;
  }

  // Static read function - so it's address can be taken.
  template<IsProtoType ProtoType>
  static std::optional<ProtoType> OrNullopt(
      const std::filesystem::path& filename,
      const std::source_location& src_loc = std::source_location::current()) {
    if (auto result = As<ProtoType>(filename, src_loc); result.ok()) {
      return *std::move(result);
    }
    return std::nullopt;
  }

  // Static read function - so it's address can be taken.
  template<IsProtoType ProtoType>
  static ProtoType OrDie(
      const std::filesystem::path& filename,
      const std::source_location& src_loc = std::source_location::current()) {
    return *As<ProtoType>(filename, src_loc);
  }

  ReadTextProtoFile() = delete;

  explicit ReadTextProtoFile(
      std::filesystem::path filename,
      const std::source_location& src_loc = std::source_location::current())
      : filename_(std::move(filename)), src_loc_(src_loc) {}

  ReadTextProtoFile(const ReadTextProtoFile&) = delete;
  ReadTextProtoFile& operator=(const ReadTextProtoFile&) = delete;
  ReadTextProtoFile(ReadTextProtoFile&&) = delete;
  ReadTextProtoFile& operator=(ReadTextProtoFile&&) = delete;

  ~ReadTextProtoFile() { ABSL_LOG_IF(DFATAL, !converted_) << "ReadTextProtoFile has not read the file."; }

  template<int&..., IsProtoType ProtoType>
  operator absl::StatusOr<ProtoType>() const {  // NOLINT(*-explicit-*)
    converted_ = true;
    ProtoType result;
    const auto status = proto_internal::ReadTextProtoFile(filename_, result, src_loc_);
    if (!status.ok()) {
      return status;
    }
    return result;
  }

  template<int&..., IsProtoType ProtoType>
  operator std::optional<ProtoType>() const {  // NOLINT(*-explicit-*)
    absl::StatusOr<ProtoType> proto = *this;
    if (!proto.ok()) {
      return std::nullopt;
    }
    return *proto;
  }

  template<int&..., IsProtoType ProtoType>
  operator ProtoType() const {  // NOLINT(*-explicit-*)
    absl::StatusOr<ProtoType> proto = *this;
    ABSL_LOG_IF(FATAL, !proto.ok()).AtLocation(src_loc_.file_name(), static_cast<int>(src_loc_.line()))
        << proto.status();
    return *proto;
  }

 private:
  const std::filesystem::path filename_;
  const std::source_location src_loc_;
  mutable bool converted_ = false;
};

// Writes a text proto file.
absl::Status WriteTextProtoFile(
    const std::filesystem::path& filename,
    const ::google::protobuf::Message& proto,
    const std::source_location& src_loc = std::source_location::current());

}  // namespace mbo::proto

#endif  // MBO_PROTO_FILE_H_
