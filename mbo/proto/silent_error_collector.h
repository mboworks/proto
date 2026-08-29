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

#ifndef MBO_PROTO_SILENT_ERROR_COLLECTOR_H_
#define MBO_PROTO_SILENT_ERROR_COLLECTOR_H_

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "absl/base/log_severity.h"
#include "google/protobuf/io/tokenizer.h"

namespace mbo::proto {

// Collects errors from proto parsing.
//
// This class does not implement `::google::protobuf::io::ErrorCollector` but
// can be used as such due to its conversion operator. The reason this is done,
// is to disable direct access to the underlying functions which change accross
// protobuf library versions.
class SilentErrorCollector {
 public:
  // NOLINTBEGIN(readability-identifier-naming)
  struct ErrorInfo {
    int line = 0;
    int column = 0;
    std::string message;
    absl::LogSeverity severity = absl::LogSeverity::kError;

    friend auto operator<=>(const ErrorInfo&, const ErrorInfo&) = default;
  };

  using value_type = ErrorInfo;

  SilentErrorCollector() : impl_(std::make_unique<Impl>(*this)) {}

  SilentErrorCollector(const SilentErrorCollector&) = delete;
  SilentErrorCollector& operator=(const SilentErrorCollector&) = delete;
  SilentErrorCollector(SilentErrorCollector&&) noexcept = default;
  SilentErrorCollector& operator=(SilentErrorCollector&&) noexcept = default;

  virtual ~SilentErrorCollector() = default;

  // Records a single message. In client code that manually needs to add messages always use this
  // variant and never RecordError/Warning or AddError/Warning.
  void Record(int line, int column, std::string_view message, absl::LogSeverity severity) {
    errors_.push_back({.line = line, .column = column, .message = std::string(message), .severity = severity});
  }

  // NOLINTNEXTLINE(*-explicit-constructor,*-explicit-conversions)
  operator ::google::protobuf::io::ErrorCollector&() & { return *impl_; }

  // NOLINTNEXTLINE(*-explicit-constructor,*-explicit-conversions)
  operator ::google::protobuf::io::ErrorCollector*() & { return &*impl_; }

  virtual std::string FormatError(const ErrorInfo& error) const;

  std::string GetErrors(std::string_view joiner = "\n") const;

  const std::vector<ErrorInfo>& errors() const { return errors_; }

  bool empty() const { return errors_.empty(); }

  std::size_t size() const { return errors_.size(); }

  auto begin() const { return errors_.begin(); }

  auto end() const { return errors_.end(); }

  // NOLINTEND(readability-identifier-naming)

 private:
  class Impl : public ::google::protobuf::io::ErrorCollector {
   public:
    Impl() = delete;

    explicit Impl(SilentErrorCollector& collector) : collector_(collector) {}

    // protobuf's `io::ErrorCollector` API: the pre-v26 `AddError`/`AddWarning`
    // virtuals were deprecated in favour of `RecordError`/`RecordWarning` and
    // removed entirely in protobuf v35. This repo only supports protobuf >= 32,
    // so we override the modern methods unconditionally.
    void RecordError(int line, int column, std::string_view message) override {
      collector_.Record(line, column, message, absl::LogSeverity::kError);
    }

    void RecordWarning(int line, int column, std::string_view message) override {
      collector_.Record(line, column, message, absl::LogSeverity::kWarning);
    }

   private:
    SilentErrorCollector& collector_;
  };

  std::unique_ptr<Impl> impl_;
  std::vector<ErrorInfo> errors_;
};

}  // namespace mbo::proto

#endif  // MBO_PROTO_SILENT_ERROR_COLLECTOR_H_
