// An MCP server written against the Server facade — the SDK's public API. Two
// typed tools sharing one args struct; schema + validation are automatic.
#include <string>
#include <mcp/server.hpp>

// The operands for a binary integer op — described once, drives schema + parsing.
struct Operands {
  int a = 0;
  int b = 0;

  static constexpr auto describe() {
    return mcp::fields(mcp::field(&Operands::a, "a", "first operand"),
                       mcp::field(&Operands::b, "b", "second operand"));
  }
};

int main() {
  mcp::Server server("mcp-cpp-sdk", "0.1");

  server.tool<Operands>("add", "Add two integers", [](const Operands& x) {
    return mcp::text(std::to_string(x.a + x.b));
  });

  server.tool<Operands>("multiply", "Multiply two integers", [](const Operands& x) {
    return mcp::text(std::to_string(x.a * x.b));
  });

  server.run();  // real stdio, until the client disconnects
  return 0;
}
