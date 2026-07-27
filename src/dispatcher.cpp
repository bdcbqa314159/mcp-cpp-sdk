#include "mcp/json_rpc.hpp"
#include <mcp/dispatcher.hpp>

#include <optional>
#include <utility>

namespace mcp {

// TODO (task 6): store `handler` under `method` in handlers_.
//   (std::move both into the map.)
void Dispatcher::on(std::string method, Handler handler) {
  handlers_.insert_or_assign(std::move(method), std::move(handler));
}

// TODO (task 6): route one request.
//
//   1. Work out the payload (a std::variant<json, Error>):
//        auto it = handlers_.find(req.method);
//        - not found  -> Error{-32601, "Method not found", std::nullopt}
//        - found      -> call the handler with the params. req.params is a
//          std::optional<json>, so pass req.params.value_or(json::object()).
//          The handler returns Result<json>:
//            - r.ok()  -> the payload is r.value()   (the result json)
//            - else    -> the payload is r.error()   (the Error)
//
//   2. Notifications get NO reply: if req.id has no value, return std::nullopt.
//      (The handler still ran above — side effects happen, there's just no Response.)
//
//   3. Otherwise return  Response{ *req.id, payload };
std::optional<Response> Dispatcher::dispatch(const Request& req) {

  auto it = handlers_.find(req.method);
  std::variant<json, Error> payload{};

  if (it == handlers_.end()) {
    payload = Error{.code = -32601, .message = "Method not found", .data = std::nullopt};
  } else {

    Result<json> r = it->second(req.params.value_or(json::object()));
    if (r.ok())
      payload = r.value();
    else
      payload = r.error();
  }

  if (req.id)
    return Response{.id = *req.id, .payload = payload};
  return std::nullopt;
}
} // namespace mcp
