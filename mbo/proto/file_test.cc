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

#include <filesystem>
#include <fstream>
#include <ios>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/proto/matchers.h"
#include "mbo/proto/status_matchers.h"
#include "mbo/proto/tests/simple_message.pb.h"

namespace mbo::proto {
namespace {
// NOLINTBEGIN(*-magic-numbers)

using ::mbo::proto::EqualsProto;
using ::mbo::proto::tests::SimpleMessage;
using ::testing::ExplainMatchResult;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Optional;

struct FileProtoTest : ::testing::Test {
  [[nodiscard]] static bool WriteFile(const std::filesystem::path& filename, std::string_view data) {
    std::ofstream output(filename, std::ios::binary);
    if (output.good()) {
      output << data;
      output.close();
    }
    return output.good();
  }
};

TEST_F(FileProtoTest, BinaryProto) {
  mbo::proto::tests::SimpleMessage message;
  message.set_one(25);
  message.add_two(33);
  message.add_two(42);
  ASSERT_THAT(WriteBinaryProtoFile("test.pb", message), IsOk());
  const absl::StatusOr<SimpleMessage> result_or_status = ReadBinaryProtoFile("test.pb");
  EXPECT_THAT(result_or_status, IsOkAndHolds(EqualsProto(message)));
  const std::optional<SimpleMessage> result_or_nullopt = ReadBinaryProtoFile("test.pb");
  EXPECT_THAT(result_or_nullopt, Optional(EqualsProto(message)));
  EXPECT_THAT(SimpleMessage(ReadBinaryProtoFile("test.pb")), EqualsProto(message));
  EXPECT_THAT(ReadBinaryProtoFile::As<SimpleMessage>("test.pb"), IsOkAndHolds(EqualsProto(message)));
  EXPECT_THAT(ReadBinaryProtoFile::OrNullopt<SimpleMessage>("test.pb"), Optional(EqualsProto(message)));
  EXPECT_THAT(ReadBinaryProtoFile::OrDie<SimpleMessage>("test.pb"), EqualsProto(message));
}

TEST_F(FileProtoTest, BinaryProtoError) {
  EXPECT_THAT(
      absl::StatusOr<SimpleMessage>(ReadBinaryProtoFile("DoesNotExist.pb")),
      StatusIs(absl::StatusCode::kNotFound, HasSubstr("Cannot open 'DoesNotExist.pb'")));
  ASSERT_TRUE(WriteFile("SomeFile.pb", "\xFF"));
  EXPECT_THAT(
      absl::StatusOr<SimpleMessage>(ReadBinaryProtoFile("SomeFile.pb")),
      StatusIs(absl::StatusCode::kAborted, HasSubstr("Cannot parse binary proto file 'SomeFile.pb'")));
}

TEST_F(FileProtoTest, TextProto) {
  mbo::proto::tests::SimpleMessage message;
  message.set_one(25);
  message.add_two(33);
  message.add_two(42);
  ASSERT_THAT(WriteTextProtoFile("test.textproto", message), IsOk());
  const absl::StatusOr<SimpleMessage> result_or_status = ReadTextProtoFile("test.textproto");
  EXPECT_THAT(result_or_status, IsOkAndHolds(EqualsProto(message)));
  const std::optional<SimpleMessage> result_or_nullopt = ReadTextProtoFile("test.textproto");
  EXPECT_THAT(result_or_nullopt, Optional(EqualsProto(message)));
  EXPECT_THAT(SimpleMessage(ReadTextProtoFile("test.textproto")), EqualsProto(message));
  EXPECT_THAT(ReadTextProtoFile::As<SimpleMessage>("test.textproto"), IsOkAndHolds(EqualsProto(message)));
  EXPECT_THAT(ReadTextProtoFile::OrNullopt<SimpleMessage>("test.textproto"), Optional(EqualsProto(message)));
  EXPECT_THAT(ReadTextProtoFile::OrDie<SimpleMessage>("test.textproto"), EqualsProto(message));
}

TEST_F(FileProtoTest, TextProtoError) {
  EXPECT_THAT(
      absl::StatusOr<SimpleMessage>(ReadTextProtoFile("DoesNotExist.textproto")),
      StatusIs(absl::StatusCode::kNotFound, HasSubstr("Cannot open 'DoesNotExist.textproto'")));
  ASSERT_TRUE(WriteFile("SomeFile.textproto", "\xFF"));
  EXPECT_THAT(
      absl::StatusOr<SimpleMessage>(ReadTextProtoFile("SomeFile.textproto")),
      StatusIs(absl::StatusCode::kAborted, HasSubstr("Cannot parse text proto file 'SomeFile.textproto'")));
}

MATCHER(IsBinaryProtoExtension, "") {
  return HasBinaryProtoExtension(arg);
}

MATCHER(IsTextProtoExtension, "") {
  return HasTextProtoExtension(arg);
}

TEST_F(FileProtoTest, HasBinaryProtoExtension) {
  // True: binary proto
  for (std::string_view filename : {".binpb", ".pb", "file.binpb", "dir.foo/file.pb", "..pb"}) {
    EXPECT_THAT(filename, IsBinaryProtoExtension());
  }
  // Type handling
  EXPECT_TRUE(HasBinaryProtoExtension(".binpb"));
  EXPECT_TRUE(HasBinaryProtoExtension(std::string{".binpb"}));
  EXPECT_TRUE(HasBinaryProtoExtension(std::string_view{".binpb"}));
  EXPECT_TRUE(HasBinaryProtoExtension(std::filesystem::path(".binpb")));
  // False: text prto
  for (std::string_view filename : {".txtpb", ".textproto", "file.txtpb", "dir.foo/file.txtpb", "..txtpb"}) {
    EXPECT_THAT(filename, Not(IsBinaryProtoExtension()));
  }
  // Type handling
  EXPECT_FALSE(HasBinaryProtoExtension(".txtpb"));
  EXPECT_FALSE(HasBinaryProtoExtension(std::string{".txtpb"}));
  EXPECT_FALSE(HasBinaryProtoExtension(std::string_view{".txtpb"}));
  EXPECT_FALSE(HasBinaryProtoExtension(std::filesystem::path(".txtpb")));
  // False: others
  for (std::string_view filename : {".txtproto", ".proto", "binpb", "pb", "txtpb", "textproto"}) {
    EXPECT_THAT(filename, Not(IsBinaryProtoExtension()));
  }
}

TEST_F(FileProtoTest, HasTextProtoExtension) {
  // True: text proto
  for (std::string_view filename : {".txtpb", ".textproto", "file.txtpb", "dir.foo/file.txtpb", "..txtpb"}) {
    EXPECT_THAT(filename, IsTextProtoExtension());
  }
  // Type handling
  EXPECT_TRUE(HasTextProtoExtension(".txtpb"));
  EXPECT_TRUE(HasTextProtoExtension(std::string{".txtpb"}));
  EXPECT_TRUE(HasTextProtoExtension(std::string_view{".txtpb"}));
  EXPECT_TRUE(HasTextProtoExtension(std::filesystem::path(".txtpb")));
  // False: binary proto
  for (std::string_view filename : {".binpb", ".pb", "file.binpb", "dir.foo/file.pb", "..pb"}) {
    EXPECT_THAT(filename, Not(IsTextProtoExtension()));
  }
  for (std::string_view filename : {".txtproto", ".proto", "binpb", "pb", "txtpb", "textproto"}) {
    EXPECT_THAT(filename, Not(IsBinaryProtoExtension()));
  }
  // Type handling
  EXPECT_FALSE(HasTextProtoExtension(".binpb"));
  EXPECT_FALSE(HasTextProtoExtension(std::string{".binpb"}));
  EXPECT_FALSE(HasTextProtoExtension(std::string_view{".binpb"}));
  EXPECT_FALSE(HasTextProtoExtension(std::filesystem::path(".binpb")));
  // False: others
  for (std::string_view filename : {".txtproto", ".proto", "binpb", "pb", "txtpb", "textproto"}) {
    EXPECT_THAT(filename, Not(IsBinaryProtoExtension()));
  }
}

// NOLINTEND(*-magic-numbers)
}  // namespace
}  // namespace mbo::proto
