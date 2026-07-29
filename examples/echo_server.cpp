// A minimal MCP server using the TYPED-tool API — the SDK's signature feature.
//
// You describe the tool's arguments ONCE (the struct + describe()); the SDK derives
// BOTH the JSON schema advertised by tools/list AND the parsing of tools/call from
// that single description, so they can't drift. The handler receives a parsed,
// validated struct — no json poking, no hand-written schema.
#include <mcp/serve.hpp>
#include <mcp/typed.hpp>

struct EchoArgs {
  std::string msg;

  static constexpr auto describe() {
    return mcp::fields(mcp::field(&EchoArgs::msg, "msg", "the message to echo"));
  }
};

int main() {
  mcp::Session session({"mcp-cpp-sdk-echo", "0.1"});

  mcp::ToolRegistry tools;
  mcp::add_typed_tool<EchoArgs>(
      tools, "echo", "Echo back the message",
      [](const EchoArgs& args) -> mcp::ToolResult { return mcp::text(args.msg); });

  mcp::StdioTransport transport;  // real std::cin / std::cout
  mcp::serve(transport, session, tools);
  return 0;
}
