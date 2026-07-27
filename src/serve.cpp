#include <mcp/serve.hpp>

#include <exception>

namespace mcp {

// Read -> parse -> dispatch -> write, one message at a time, until the transport
// hits EOF. Input too malformed to become a Request becomes a -32700 reply with a
// null id; notifications (and unparseable notifications aside) get no reply.
void serve(ITransport& transport, Dispatcher& dispatcher) {
  while (auto line = transport.read()) {
    std::optional<Response> resp;

    try {
      Request req = json::parse(*line).get<Request>();
      resp = dispatcher.dispatch(req);
    } catch (const std::exception&) {
      resp =
          Response{.id = Id{.value = std::monostate{}},
                   .payload = Error{
                       .code = -32700, .message = "Parse error", .data = std::nullopt}};
    }
    if (resp) {
      json out = *resp;
      transport.write(out.dump());
    }
  }
}

} // namespace mcp
