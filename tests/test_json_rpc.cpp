// Task 1 acceptance: the Id type round-trips through JSON in all three forms.
#include <gtest/gtest.h>
#include <mcp/json_rpc.hpp>

using mcp::Id;
using mcp::json;

// Helper: serialize an Id to JSON, parse it back, expect the same Id.
static void expectRoundTrip(const Id& original) {
  json j = original;                 // uses to_json
  Id parsed = j.get<Id>();           // uses from_json
  EXPECT_EQ(parsed, original);       // std::variant has operator== built in
}

TEST(Id, IntegerRoundTrip) {
  Id id{std::int64_t{42}};
  json j = id;
  EXPECT_TRUE(j.is_number_integer());
  EXPECT_EQ(j, 42);
  expectRoundTrip(id);
}

TEST(Id, StringRoundTrip) {
  Id id{std::string{"abc-123"}};
  json j = id;
  EXPECT_TRUE(j.is_string());
  EXPECT_EQ(j, "abc-123");
  expectRoundTrip(id);
}

TEST(Id, NullRoundTrip) {
  Id id{std::monostate{}};
  json j = id;
  EXPECT_TRUE(j.is_null());
  expectRoundTrip(id);
}

TEST(Id, RejectsInvalidType) {
  json j = 3.14;                     // a float is not a valid id
  Id id;
  EXPECT_ANY_THROW(id = j.get<Id>());
}
