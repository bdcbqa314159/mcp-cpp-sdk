#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty())
      continue;

    // --- PARSE (now with error handling) ----------------------------------
    // json::parse THROWS json::parse_error on garbage. Wrap it in try/catch.
    // On failure: send an error reply with code -32700, id = null, then
    // `continue` to the next line.
    // TODO: json req;  try { req = json::parse(line); } catch (...) { ...;
    // continue; }

    try {
      json req = json::parse(line); // <-- replace this with the guarded version

      // --- NOTIFICATION? ----------------------------------------------------
      // A message with NO "id" is a notification: do the work, send NOTHING.
      // For now the only notification we expect is "notifications/initialized".
      // TODO: if (!req.contains("id")) { /* maybe log to stderr */ continue; }

      if (!req.contains("id")) {
        std::cerr << "no id in the request.. continue...\n";
        continue;
      }

      std::string method = req["method"];
      auto id = req["id"];

      json reply;
      reply["jsonrpc"] = "2.0";
      reply["id"] = id;

      // --- ROUTE ------------------------------------------------------------
      if (method == "initialize") {
        // TODO: reply["result"] = { protocolVersion, capabilities, serverInfo }
        //   protocolVersion: "2024-11-05" (or echo
        //   req["params"]["protocolVersion"]) capabilities:    {"tools": {}}
        //   (empty object = "I have tools") serverInfo:      {"name": "...",
        //   "version": "..."}
        // Remember the double-brace shape for nested objects.
        reply["result"] = {
            {"protocolVersion", req["params"]["protocolVersion"]},
            {"capabilities", {{"tools", json::object()}}},
            {"serverInfo", {{"name", "mcp-cpp-sdk"}, {"version", "0.0.1"}}}};
      } else if (method == "ping") {
        reply["result"] = {{"ok", true}};
      } else {
        // Unknown method -> JSON-RPC error, not a result.
        // TODO: reply["error"] = {{"code", -32601}, {"message", "Method not
        // found"}};
        //   (note: set "error" INSTEAD of "result", never both)
        reply["id"] = id;
        reply["error"] = {{"code", -32601}, {"message", "Method not found"}};
      }
      std::cout << reply.dump() << "\n" << std::flush;
      continue;

    } catch (...) {
      json reply;
      reply["jsonrpc"] = "2.0";
      reply["id"] = nullptr;
      reply["error"] = {{"code", -32700}, {"message", "Parse error"}};
      std::cout << reply.dump() << "\n" << std::flush;
    };
  }

  return 0;
}
