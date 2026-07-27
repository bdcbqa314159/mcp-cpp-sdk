#include <gtest/gtest.h>
#include <variant>
#include <mcp/dispatcher.hpp>

using mcp::Dispatcher;
using mcp::Error;
using mcp::Id;
using mcp::json;
using mcp::Request;
using mcp::Response;
using mcp::Result;

TEST(Dispatcher, RoutesToHandlerAndReturnsResult) {
  Dispatcher d;
  d.on("add", [](const json& p) -> Result<json> {
    return json(p.at("a").get<int>() + p.at("b").get<int>());
  });

  Request req;
  req.method = "add";
  req.params = json{{"a", 2}, {"b", 3}};
  req.id = Id{std::int64_t{1}};

  auto resp = d.dispatch(req);
  ASSERT_TRUE(resp.has_value());
  ASSERT_TRUE(std::holds_alternative<json>(resp->payload));
  EXPECT_EQ(std::get<json>(resp->payload), 5);
}

TEST(Dispatcher, UnknownMethodReturnsError) {
  Dispatcher d;
  Request req;
  req.method = "nope";
  req.id = Id{std::int64_t{2}};

  auto resp = d.dispatch(req);
  ASSERT_TRUE(resp.has_value());
  ASSERT_TRUE(std::holds_alternative<Error>(resp->payload));
  EXPECT_EQ(std::get<Error>(resp->payload).code, -32601);
}

TEST(Dispatcher, HandlerErrorBecomesErrorResponse) {
  Dispatcher d;
  d.on("boom", [](const json&) -> Result<json> {
    return Error{-32000, "boom", std::nullopt};
  });
  Request req;
  req.method = "boom";
  req.id = Id{std::int64_t{3}};

  auto resp = d.dispatch(req);
  ASSERT_TRUE(resp.has_value());
  ASSERT_TRUE(std::holds_alternative<Error>(resp->payload));
  EXPECT_EQ(std::get<Error>(resp->payload).code, -32000);
}

TEST(Dispatcher, NotificationGetsNoReply) {
  Dispatcher d;
  bool called = false;
  d.on("notify", [&](const json&) -> Result<json> {
    called = true;
    return json(nullptr);
  });
  Request req;
  req.method = "notify";  // no id => notification

  auto resp = d.dispatch(req);
  EXPECT_FALSE(resp.has_value());  // no reply for a notification
  EXPECT_TRUE(called);             // but the handler still ran
}
