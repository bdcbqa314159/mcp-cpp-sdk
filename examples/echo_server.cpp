// A minimal MCP server built from the layered core: the lifecycle handshake plus
// one registered tool ("echo"), exposed via tools/list and tools/call. Run over
// real stdin/stdout; a client must complete initialize + notifications/initialized
// before it can list or call tools.
#include <mcp/serve.hpp>

int main() {
  mcp::Session session({"mcp-cpp-sdk-echo", "0.1"});

  mcp::ToolRegistry tools;
  tools.add(mcp::Tool{
      "echo", "Echo back the 'msg' argument",
      mcp::json::parse(R"({"type":"object",
                           "properties":{"msg":{"type":"string"}},
                           "required":["msg"]})"),
      [](const mcp::json& args) -> mcp::ToolResult {
        return mcp::text(args.value("msg", std::string("(no msg)")));
      }});

  mcp::StdioTransport transport;  // real std::cin / std::cout
  mcp::serve(transport, session, tools);
  return 0;
}
