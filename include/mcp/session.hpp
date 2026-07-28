#pragma once
#include <string>
#include <utility>
#include <mcp/json_rpc.hpp>
#include <mcp/result.hpp>

namespace mcp {

// Where a connection sits in the MCP lifecycle handshake:
//   Uninitialized --initialize--> Initializing --notifications/initialized--> Ready
enum class Lifecycle { Uninitialized, Initializing, Ready };

struct ServerInfo {
  std::string name;
  std::string version;
};

// Owns the per-connection lifecycle state and enforces the handshake order.
// A per-connection object (not a global) — the design's correction of the
// singleton-session reference implementations.
class Session {
public:
  explicit Session(ServerInfo info) : info_(std::move(info)) {}

  Lifecycle state() const { return state_; }

  // Handle `initialize`. Valid only from Uninitialized: transitions to Initializing
  // and returns the initialize result (protocolVersion, capabilities, serverInfo).
  // Called out of order -> an Error (reported as a value, not thrown).
  Result<json> initialize(const json& params);

  // Handle `notifications/initialized`: Initializing -> Ready.
  void mark_initialized();

  // May a request for `method` run right now? Lifecycle methods are always allowed;
  // everything else only once Ready.
  bool allows(const std::string& method) const;

private:
  ServerInfo info_;
  Lifecycle state_ = Lifecycle::Uninitialized;
};

}  // namespace mcp
