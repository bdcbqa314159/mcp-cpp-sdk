#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>
#include <mcp/serve.hpp>

using mcp::json;
using mcp::Session;
using mcp::StdioTransport;
using mcp::Tool;
using mcp::ToolRegistry;
using mcp::ToolResult;

namespace {

// Drive a full MCP server (lifecycle + tools) with a sequence of request lines;
// return the parsed response lines it wrote.
std::vector<json> run_tools_server(const std::vector<std::string>& requests) {
  std::string input;
  for (const auto& r : requests) input += r + "\n";
  std::istringstream in(input);
  std::ostringstream out;
  StdioTransport transport(in, out);

  Session session({"test-server", "0.1"});
  ToolRegistry tools;
  tools.add(Tool{"add", "adds a and b", json{{"type", "object"}},
                 [](const json& a) -> ToolResult {
                   return mcp::text(std::to_string(a.at("a").get<int>() + a.at("b").get<int>()));
                 }});

  mcp::serve(transport, session, tools);

  std::vector<json> responses;
  std::istringstream lines(out.str());
  std::string line;
  while (std::getline(lines, line))
    if (!line.empty()) responses.push_back(json::parse(line));
  return responses;
}

const char* kInit = R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})";
const char* kInitialized = R"({"jsonrpc":"2.0","method":"notifications/initialized"})";

}  // namespace

TEST(ToolsServe, ListsToolsAfterInit) {
  auto r = run_tools_server({
      kInit,
      kInitialized,
      R"({"jsonrpc":"2.0","id":2,"method":"tools/list"})",
  });
  ASSERT_EQ(r.size(), 2u);  // init + tools/list; the notification produced none
  const json& tools = r[1].at("result").at("tools");
  ASSERT_EQ(tools.size(), 1u);
  EXPECT_EQ(tools.at(0).at("name"), "add");
}

TEST(ToolsServe, CallsToolAfterInit) {
  auto r = run_tools_server({
      kInit,
      kInitialized,
      R"({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"add","arguments":{"a":2,"b":3}}})",
  });
  ASSERT_EQ(r.size(), 2u);
  const json& content = r[1].at("result").at("content");
  EXPECT_EQ(content.at(0).at("text"), "5");
  EXPECT_FALSE(r[1].at("result").at("isError"));
}

TEST(ToolsServe, UnknownToolIsErrorResult) {
  auto r = run_tools_server({
      kInit,
      kInitialized,
      R"({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"nope","arguments":{}}})",
  });
  ASSERT_EQ(r.size(), 2u);
  // A missing tool is a *tool* failure (isError), not a JSON-RPC error.
  EXPECT_TRUE(r[1].at("result").at("isError"));
}

TEST(ToolsServe, ToolsGatedBeforeInit) {
  auto r = run_tools_server({
      R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})",  // before handshake
  });
  ASSERT_EQ(r.size(), 1u);
  EXPECT_EQ(r[0].at("error").at("code"), -32002);
}
