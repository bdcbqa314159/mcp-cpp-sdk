// The whole point of the SDK: a working MCP server in ~10 lines of *your* code.
// All the rung-1..4 plumbing lives inside McpServer::run().
#include <mcp/server.hpp>

int main() {
  mcp::McpServer server("mcp-cpp-sdk", "0.0.1");

  server.add_tool(
      "add", "Add two numbers",
      R"({"type":"object",
          "properties":{"a":{"type":"number"},"b":{"type":"number"}},
          "required":["a","b"]})"_json,
      [](const mcp::json& args) -> std::string {          // <- the only logic you write
        int sum = args["a"].get<int>() + args["b"].get<int>();
        return std::to_string(sum);
      });

  server.run();   // blocks: stdio loop + JSON-RPC + lifecycle, until stdin closes
}
