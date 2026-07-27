#pragma once
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <mcp/json_rpc.hpp>
#include <mcp/result.hpp>

namespace mcp {

// A handler runs one method: it receives the request's params and returns either
// a result value (json) or an Error — which is exactly Result<json>. This is where
// Result<T> earns its keep: a handler reports failure by *returning*, not throwing.
using Handler = std::function<Result<json>(const json& params)>;

// Dispatcher: method name -> handler. Given a Request, it routes to the right
// handler and packages the outcome as a Response — or nothing, for a notification.
// This is the layer where the value types, Result, and routing finally compose.
class Dispatcher {
public:
  // Register a handler for a method name.
  void on(std::string method, Handler handler);

  // Route one request. Returns std::nullopt for a notification (a Request with no
  // id gets no reply). Unknown method -> an error Response (-32601).
  std::optional<Response> dispatch(const Request& req);

private:
  std::map<std::string, Handler> handlers_;
};

}  // namespace mcp
