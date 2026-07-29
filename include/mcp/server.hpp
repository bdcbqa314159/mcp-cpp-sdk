#pragma once
#include <mcp/serve.hpp>
#include <mcp/session.hpp>
#include <mcp/tool.hpp>
#include <mcp/transport.hpp>
#include <mcp/typed.hpp>
#include <string>
#include <utility>

namespace mcp {

// The public facade — the API a user of the SDK writes against. It owns the
// lifecycle Session and the ToolRegistry, lets you register typed tools, and runs
// the MCP loop. Everything under it (transport, JSON-RPC, lifecycle, tools, typed
// layer) is what the earlier milestones built.
//
//   mcp::Server server{"my-server", "1.0"};
//   server.tool<Args>("name", "description", [](const Args& a){ return mcp::text(...);
//   }); server.run();
class Server {
public:
  Server(std::string name, std::string version)
      : session_({std::move(name), std::move(version)}) {}

  // Register a typed tool: its schema and its argument parsing both come from
  // Args::describe(). The handler receives a parsed, validated Args.
  template <typename Args, typename Handler>
  void tool(std::string name, std::string description, Handler handler) {
    add_typed_tool<Args>(tools_, std::move(name), std::move(description),
                         std::move(handler));
  }

  // Run the MCP serve loop until the transport closes.
  void run(ITransport& transport) { serve(transport, session_, tools_); }

  // Convenience: run over real stdin/stdout.
  void run() {
    StdioTransport stdio;
    run(stdio);
  }

private:
  Session session_;
  ToolRegistry tools_;
};

} // namespace mcp
