// A minimal MCP-style JSON-RPC server built from the layered core: it registers
// one "echo" method and runs the serve loop over real stdin/stdout.
#include <mcp/serve.hpp>

int main() {
  mcp::Dispatcher dispatcher;
  dispatcher.on("echo", [](const mcp::json& params) -> mcp::Result<mcp::json> {
    return params;  // echo the params straight back as the result
  });

  mcp::StdioTransport transport;  // real std::cin / std::cout
  mcp::serve(transport, dispatcher);
  return 0;
}
