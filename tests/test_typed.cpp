#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <mcp/typed.hpp>

namespace {
struct Sample {
  std::string name;
  int age = 0;
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
