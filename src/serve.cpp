#include "mcp/json_rpc.hpp"
#include <mcp/serve.hpp>

#include <exception>

namespace mcp {

void serve(ITransport& transport, Dispatcher& dispatcher) {
  // TODO (task 7): the serve loop.
  //
  //   Loop over transport.read() until it returns std::nullopt (EOF ends the loop):
  //     while (auto line = transport.read()) { ... }
  //
  //   For each line (skip it if empty):
  //     - Parse it into a Request, GUARDED by try/catch:
  //         Request req = json::parse(*line).get<Request>();
  //       On any exception (bad JSON / not a valid request), the reply is a
  //       parse-error Response with a NULL id:
  //         Response{Id{std::monostate{}}, Error{-32700, "Parse error", std::nullopt}}
  //     - Otherwise dispatch it: dispatcher.dispatch(req) returns
  //       std::optional<Response> (std::nullopt for a notification).
  //     - If you ended up with a Response, serialize and send it:
  //         json out = *resp; transport.write(out.dump());
  //       If it's std::nullopt (notification), write nothing.

  while (auto line = transport.read()) {
    std::optional<Response> resp;

    try {
      Request req = json::parse(*line).get<Request>();
      resp = dispatcher.dispatch(req);
    } catch (const std::exception& e) {
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
