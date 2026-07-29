#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>
#include <mcp/server.hpp>

using mcp::json;
using mcp::Server;
using mcp::StdioTransport;

namespace {
struct Operands {
  int a = 0;
  int b = 0;
  static constexpr auto describe() {
    return mcp::fields(mcp::field(&Operands::a, "a", "first"),
                       mcp::field(&Operands::b, "b", "second"));
  }
};

// Drive a Server (built via the facade) with a request sequence; return the
// parsed response lines.
std::vector<json> drive(Server& server, const std::vector<std::string>& requests) {
  std::string input;
  for (const auto& r : requests) input += r + "\n";
  std::istringstream in(input);
  std::ostringstream out;
  StdioTransport transport(in, out);

  server.run(transport);

  std::vector<json> responses;
  std::istringstream lines(out.str());
  std::string line;
  while (std::getline(lines, line))
    if (!line.empty()) responses.push_back(json::parse(line));
  return responses;
}
}  // namespace

TEST(Server, RegistersTypedToolAndServesIt) {
  Server server("test-server", "0.1");
  server.tool<Operands>("add", "adds", [](const Operands& x) {
    return mcp::text(std::to_string(x.a + x.b));
  });

  auto r = drive(server, {
                             R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})",
                             R"({"jsonrpc":"2.0","method":"notifications/initialized"})",
                             R"({"jsonrpc":"2.0","id":2,"method":"tools/list"})",
                             R"({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"add","arguments":{"a":2,"b":3}}})",
                         });

  ASSERT_EQ(r.size(), 3u);  // initialize + tools/list + tools/call
  // tools/list carries the auto-generated schema.
  EXPECT_EQ(r[1].at("result").at("tools").at(0).at("name"), "add");
  // tools/call parsed the operands and ran the handler.
  EXPECT_EQ(r[2].at("result").at("content").at(0).at("text"), "5");
}
