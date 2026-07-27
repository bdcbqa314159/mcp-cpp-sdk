#include <mcp/dispatcher.hpp>

#include <optional>
#include <utility>

namespace mcp {

// Register (or replace) the handler for a method name.
void Dispatcher::on(std::string method, Handler handler) {
  handlers_.insert_or_assign(std::move(method), std::move(handler));
}

// Route one request: look up the handler (unknown method -> -32601), run it, and
// wrap its Result as the Response payload. A notification (no id) gets no reply.
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
