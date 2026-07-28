// A minimal MCP server: the lifecycle handshake (via Session) plus one "echo"
// method, run over real stdin/stdout. `echo` only works once the client has
// completed initialize + notifications/initialized.
#include <mcp/serve.hpp>

int main() {
  mcp::Session session({"mcp-cpp-sdk-echo", "0.1"});

  mcp::Dispatcher dispatcher;
  dispatcher.on("echo", [](const mcp::json& params) -> mcp::Result<mcp::json> {
    return params;  // echo the params straight back
  });

  mcp::StdioTransport transport;  // real std::cin / std::cout
  mcp::serve(transport, session, dispatcher);
  return 0;
}
