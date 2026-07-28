#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>
#include <mcp/serve.hpp>

using mcp::Dispatcher;
using mcp::json;
using mcp::Result;
using mcp::Session;
using mcp::StdioTransport;

namespace {

// Drive a lifecycle-aware server with a sequence of request lines; return the
// parsed response lines it wrote (notifications produce none).
std::vector<json> run_server(const std::vector<std::string>& requests) {
  std::string input;
  for (const auto& r : requests) input += r + "\n";
  std::istringstream in(input);
  std::ostringstream out;
  StdioTransport transport(in, out);

  Session session({"test-server", "0.1"});
  Dispatcher dispatcher;
  dispatcher.on("echo", [](const json& p) -> Result<json> { return p; });

  mcp::serve(transport, session, dispatcher);

  std::vector<json> responses;
  std::istringstream lines(out.str());
  std::string line;
  while (std::getline(lines, line))
    if (!line.empty()) responses.push_back(json::parse(line));
  return responses;
}

}  // namespace

TEST(Lifecycle, InitializeThenInitializedThenPing) {
  auto r = run_server({
      R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18"}})",
      R"({"jsonrpc":"2.0","method":"notifications/initialized"})",  // notification -> no reply
      R"({"jsonrpc":"2.0","id":2,"method":"ping"})",
  });
  ASSERT_EQ(r.size(), 2u);  // initialize + ping; the notification produced nothing
  EXPECT_TRUE(r[0].at("result").at("serverInfo").contains("name"));
  EXPECT_EQ(r[1].at("id"), 2);
  EXPECT_TRUE(r[1].contains("result"));  // ping -> a (empty) result
}

TEST(Lifecycle, ApplicationMethodRejectedBeforeInitialize) {
  auto r = run_server({
      R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"x":1}})",  // too early
  });
  ASSERT_EQ(r.size(), 1u);
  EXPECT_EQ(r[0].at("error").at("code"), -32002);
}

TEST(Lifecycle, ApplicationMethodAllowedAfterInitialized) {
  auto r = run_server({
      R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})",
      R"({"jsonrpc":"2.0","method":"notifications/initialized"})",
      R"({"jsonrpc":"2.0","id":2,"method":"echo","params":{"x":42}})",
  });
  ASSERT_EQ(r.size(), 2u);
  EXPECT_EQ(r[1].at("result"), (json{{"x", 42}}));
}

TEST(Lifecycle, SecondInitializeIsRejected) {
  auto r = run_server({
      R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})",
      R"({"jsonrpc":"2.0","id":2,"method":"initialize","params":{}})",  // out of order
  });
  ASSERT_EQ(r.size(), 2u);
  EXPECT_TRUE(r[0].contains("result"));
  EXPECT_TRUE(r[1].contains("error"));
}
