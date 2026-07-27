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

using mcp::Request;

TEST(Request, WithIdAndParams) {
  Request r;
  r.method = "tools/call";
  r.params = json{{"name", "add"}};
  r.id = Id{std::int64_t{7}};

  json j = r;
  EXPECT_EQ(j.at("jsonrpc"), "2.0");
  EXPECT_EQ(j.at("method"), "tools/call");
  EXPECT_TRUE(j.contains("params"));
  EXPECT_TRUE(j.contains("id"));
  EXPECT_EQ(j.get<Request>(), r);          // round trip
}

TEST(Request, NotificationHasNoId) {
  Request r;
  r.method = "notifications/initialized";  // no id, no params

  json j = r;
  EXPECT_FALSE(j.contains("id"));          // absent id => notification
  EXPECT_FALSE(j.contains("params"));
  EXPECT_EQ(j.get<Request>(), r);
}

TEST(Request, RequiresMethod) {
  json j = {{"jsonrpc", "2.0"}, {"id", 1}};  // method missing
  Request r;
  EXPECT_ANY_THROW(r = j.get<Request>());
}

TEST(Request, RejectsWrongVersion) {
  json j = {{"jsonrpc", "1.0"}, {"method", "x"}};  // wrong protocol version
  Request r;
  EXPECT_ANY_THROW(r = j.get<Request>());
}
