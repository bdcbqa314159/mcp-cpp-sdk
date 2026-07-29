#include <mcp/serve.hpp>

#include <exception>
#include <functional>

namespace mcp {

namespace {

// The shared loop: read a line, parse it into a Request, hand it to `route` for a
// (possible) Response, and write the reply. Input too malformed to parse becomes a
// -32700 reply with a null id. std::nullopt from `route` means "no reply" (a
// notification). Written once; the two serve() overloads differ only in `route`.
void run(ITransport& transport,
         const std::function<std::optional<Response>(const Request&)>& route) {
  while (auto line = transport.read()) {
    std::optional<Response> resp;
    try {
      Request req = json::parse(*line).get<Request>();
      resp = route(req);
    } catch (const std::exception&) {
      resp = Response{.id = Id{.value = std::monostate{}},
                      .payload =
                          Error{.code = -32700, .message = "Parse error", .data = std::nullopt}};
    }
    if (resp) {
      json out = *resp;
      transport.write(out.dump());
    }
  }
}

// Lifecycle-aware routing: the handshake methods are handled here via the Session;
// everything else is gated on Ready and forwarded to the dispatcher.
std::optional<Response> route_lifecycle(Session& session, Dispatcher& dispatcher,
                                        const Request& req) {
  const std::string& method = req.method;
  std::variant<json, Error> payload;

  if (method == "initialize") {
    Result<json> r = session.initialize(req.params.value_or(json::object()));
    payload = r.ok() ? std::variant<json, Error>{r.value()}
                     : std::variant<json, Error>{r.error()};
  } else if (method == "notifications/initialized") {
    session.mark_initialized();
    return std::nullopt;  // a notification: no reply
  } else if (method == "ping") {
    payload = json::object();  // empty result == pong
  } else if (!session.allows(method)) {
    payload = Error{.code = -32002, .message = "Server not initialized", .data = std::nullopt};
  } else {
    return dispatcher.dispatch(req);  // application method
  }

  if (!req.id)
    return std::nullopt;  // a lifecycle request arriving as a notification
  return Response{.id = *req.id, .payload = payload};
}

// Full MCP routing: the lifecycle (as above) plus tools/list and tools/call backed
// by the registry.
std::optional<Response> route_mcp(Session& session, ToolRegistry& tools, const Request& req) {
  const std::string& method = req.method;
  std::variant<json, Error> payload;

  if (method == "initialize") {
    Result<json> r = session.initialize(req.params.value_or(json::object()));
    payload = r.ok() ? std::variant<json, Error>{r.value()}
                     : std::variant<json, Error>{r.error()};
  } else if (method == "notifications/initialized") {
    session.mark_initialized();
    return std::nullopt;
  } else if (method == "ping") {
    payload = json::object();
  } else if (!session.allows(method)) {
    payload = Error{.code = -32002, .message = "Server not initialized", .data = std::nullopt};
  } else if (method == "tools/list") {
    payload = tools.list();
  } else if (method == "tools/call") {
    const json p = req.params.value_or(json::object());
    const std::string name = p.at("name");
    const json args = p.value("arguments", json::object());
    payload = json(tools.call(name, args));  // ToolResult -> json via to_json
  } else {
    payload = Error{.code = -32601, .message = "Method not found", .data = std::nullopt};
  }

  if (!req.id)
    return std::nullopt;
  return Response{.id = *req.id, .payload = payload};
}

}  // namespace

void serve(ITransport& transport, Dispatcher& dispatcher) {
  run(transport, [&](const Request& req) { return dispatcher.dispatch(req); });
}

void serve(ITransport& transport, Session& session, Dispatcher& dispatcher) {
  run(transport, [&](const Request& req) { return route_lifecycle(session, dispatcher, req); });
}

void serve(ITransport& transport, Session& session, ToolRegistry& tools) {
  run(transport, [&](const Request& req) { return route_mcp(session, tools, req); });
}

}  // namespace mcp
