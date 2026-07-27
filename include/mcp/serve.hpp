#pragma once
#include <mcp/dispatcher.hpp>
#include <mcp/transport.hpp>

namespace mcp {

// The JSON-RPC serve loop: read a message from the transport, parse it into a
// Request, dispatch it, write the Response back. Blocks until the transport hits
// EOF. This is the "run()" of the layered core — where every M1 piece composes.
void serve(ITransport& transport, Dispatcher& dispatcher);

}  // namespace mcp
