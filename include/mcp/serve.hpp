#pragma once
#include <mcp/dispatcher.hpp>
#include <mcp/session.hpp>
#include <mcp/transport.hpp>

namespace mcp {

// The plain JSON-RPC serve loop: read -> parse -> dispatch -> write, until EOF.
// No lifecycle: every method goes straight to the dispatcher.
void serve(ITransport& transport, Dispatcher& dispatcher);

// The lifecycle-aware serve loop. Handles the MCP handshake itself via the Session
// (`initialize`, `notifications/initialized`, `ping`), rejects application methods
// with -32002 until the session is Ready, and routes everything else to the
// dispatcher. Blocks until EOF.
void serve(ITransport& transport, Session& session, Dispatcher& dispatcher);

}  // namespace mcp
