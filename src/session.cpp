#include <mcp/session.hpp>

namespace mcp {

// TODO (M2 task 1): implement initialize().
//   - If state_ is not Uninitialized, return an Error (code -32600,
//     "Server already initialized") and change nothing — an out-of-order call.
//   - Otherwise: set state_ = Lifecycle::Initializing and return a json result:
//       {"protocolVersion": <echo the client's, or "2025-06-18" if absent>,
//        "capabilities":    {"tools": {}},        // empty object: json::object()
//        "serverInfo":      {"name": info_.name, "version": info_.version}}
//     Reading the client's version without a throw:
//       params.value("protocolVersion", "2025-06-18")
Result<json> Session::initialize(const json& params) {
  if (state_ != Lifecycle::Uninitialized)
    return Error{
        .code = -32600, .message = "Server already initialized", .data = std::nullopt};
  state_ = Lifecycle::Initializing;
  json out;
  out["protocolVersion"] = params.value("protocolVersion", "2025-06-18");
  out["capabilities"]["tools"] = json::object();
  out["serverInfo"]["name"] = info_.name;
  out["serverInfo"]["version"] = info_.version;
  return out;
}

// TODO (M2 task 1): move Initializing -> Ready.
//   (Be lenient for now: just set state_ = Lifecycle::Ready.)
void Session::mark_initialized() { state_ = Lifecycle::Ready; }

// TODO (M2 task 1): may `method` run right now?
//   Return true if it's a lifecycle method allowed before Ready — "initialize",
//   "ping", or any string starting with "notifications/" — OR if state_ == Ready.
//   Otherwise false.
//   (Prefix test: method.rfind("notifications/", 0) == 0)
bool Session::allows(const std::string& method) const {
  bool condition = (method == "initialize") || (method == "ping") ||
                   (method.starts_with("notifications/")) || (state_ == Lifecycle::Ready);
  return condition;
}

} // namespace mcp
