// Smoke tests: prove the framework is wired (gtest links, mcp header compiles,
// nlohmann round-trips). Real dispatch/lifecycle tests land at M1, once handle()
// has a testable seam.
#include <gtest/gtest.h>
#include <mcp/server.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TEST(Json, RoundTrip) {
  json j = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "ping"}};
  EXPECT_EQ(json::parse(j.dump()), j);
}

TEST(McpServer, RegistersToolWithoutThrowing) {
  mcp::McpServer server("test", "0.0.1");
  EXPECT_NO_THROW(server.add_tool(
      "noop", "no-op", R"({"type":"object"})"_json,
      [](const mcp::json&) -> std::string { return "ok"; }));
}
