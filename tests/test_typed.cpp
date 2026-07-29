#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include <tuple>
#include <mcp/typed.hpp>

using mcp::json;

namespace {
struct Sample {
  std::string name;
  int age = 0;
};

struct EchoArgs {
  std::string msg;
  bool shout = false;
  int count = 1;
  static constexpr auto describe() {
    return mcp::fields(mcp::field(&EchoArgs::msg, "msg", "the message"),
                       mcp::field(&EchoArgs::shout, "shout", "uppercase it", false),
                       mcp::field(&EchoArgs::count, "count", "repeat count", false));
  }
};
}  // namespace

TEST(Field, CapturesMemberAndMetadata) {
  auto f = mcp::field(&Sample::name, "name", "the person's name");

  EXPECT_EQ(f.name, "name");
  EXPECT_EQ(f.description, "the person's name");
  EXPECT_TRUE(f.required);  // defaults to true

  // The member pointer really points at Sample::name: bind it to an instance.
  ASSERT_NE(f.member, nullptr);
  Sample s{"Ada", 30};
  EXPECT_EQ(s.*(f.member), "Ada");  // .* dereferences a pointer-to-member
}

TEST(Field, RequiredCanBeFalse) {
  auto f = mcp::field(&Sample::age, "age", "years old", false);
  EXPECT_FALSE(f.required);
}

TEST(Fields, BundlesDescriptorsIntoATuple) {
  auto fs = mcp::fields(mcp::field(&Sample::name, "name", "n"),
                        mcp::field(&Sample::age, "age", "a"));
  // Two descriptors in, a 2-element tuple out (each keeps its own Field<...> type).
  EXPECT_EQ(std::tuple_size_v<decltype(fs)>, 2u);
}

TEST(Schema, MapsFieldTypesToJsonSchemaTypes) {
  json s = mcp::schema_for<EchoArgs>();
  EXPECT_EQ(s.at("type"), "object");
  EXPECT_EQ(s.at("properties").at("msg").at("type"), "string");
  EXPECT_EQ(s.at("properties").at("shout").at("type"), "boolean");  // NOT "integer"
  EXPECT_EQ(s.at("properties").at("count").at("type"), "integer");
}

TEST(Schema, RequiredReflectsFieldFlags) {
  json s = mcp::schema_for<EchoArgs>();
  const json& req = s.at("required");
  EXPECT_NE(std::find(req.begin(), req.end(), "msg"), req.end());     // required
  EXPECT_EQ(std::find(req.begin(), req.end(), "shout"), req.end());   // optional
  EXPECT_EQ(std::find(req.begin(), req.end(), "count"), req.end());   // optional
}

TEST(ParseArgs, PopulatesEveryFieldFromJson) {
  json args = {{"msg", "hello"}, {"shout", true}, {"count", 3}};
  EchoArgs a = mcp::parse_args<EchoArgs>(args);
  EXPECT_EQ(a.msg, "hello");
  EXPECT_TRUE(a.shout);
  EXPECT_EQ(a.count, 3);
}

TEST(ParseArgs, OptionalMissingKeepsDefaults) {
  json args = {{"msg", "hi"}};  // shout, count omitted (both optional)
  EchoArgs a = mcp::parse_args<EchoArgs>(args);
  EXPECT_EQ(a.msg, "hi");
  EXPECT_FALSE(a.shout);  // struct default
  EXPECT_EQ(a.count, 1);  // struct default
}

TEST(ParseArgs, MissingRequiredThrowsToolError) {
  json args = json::object();  // "msg" is required and absent
  EXPECT_THROW(mcp::parse_args<EchoArgs>(args), mcp::ToolError);
}
