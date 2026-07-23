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
        reply["result"] = {
            {"protocolVersion", req["params"]["protocolVersion"]},
            {"capabilities", {{"tools", json::object()}}},
            {"serverInfo", {{"name", "mcp-cpp-sdk"}, {"version", "0.0.1"}}}};
      } else if (method == "ping") {
        reply["result"] = {{"ok", true}};
      }

      // --- tools/list -------------------------------------------------------
      // Return { "tools": [ <one tool: add> ] }.
      // Each tool = {name, description, inputSchema}. inputSchema is JSON
      // Schema. TIP: don't hand-nest braces — parse a raw literal instead:
      //   json addTool = R"({ "name":"add","description":"Add two numbers",
      //     "inputSchema":{"type":"object",
      //       "properties":{"a":{"type":"number"},"b":{"type":"number"}},
      //       "required":["a","b"]} })"_json;
      //   reply["result"] = {{"tools", {addTool}}};   // {addTool} = 1-elem
      //   array
      else if (method == "tools/list") {
        json addTool = R"({ "name":"add","description":"Add two numbers",
            "inputSchema":{"type":"object",
              "properties":{"a":{"type":"number"},"b":{"type":"number"}},
              "required":["a","b"]} })"_json;
        reply["result"] = {{"tools", {addTool}}};
      }

      // --- tools/call -------------------------------------------------------
      // params = {"name":"<tool>", "arguments":{...}}.
      // 1. Read the tool name:  std::string name = req["params"]["name"];
      // 2. if (name == "add"): read a,b as int (get<int>() -> clean "5"),
      //      compute sum, return the MCP tool-result shape:
      //        json block; block["type"]="text";
      //        block["text"]=std::to_string(sum); reply["result"]["content"] =
      //        {block};   // {block} = 1-elem ARRAY reply["result"]["isError"]
      //        = false;
      // 3. else (unknown tool): a SUCCESSFUL reply whose payload flags failure:
      //        reply["result"]["content"] = { text block "unknown tool: ..." };
      //        reply["result"]["isError"] = true;      // NOT a JSON-RPC error
      else if (method == "tools/call") {
        std::string name = req["params"]["name"];

        if (name == "add") {
          auto a = req["params"]["arguments"]["a"].get<int>();
          auto b = req["params"]["arguments"]["b"].get<int>();

          json block;
          block["type"] = "text";
          block["text"] = std::to_string(a + b);
          reply["result"]["content"] = {block};
        } else {
          json block;
          block["type"] = "text";
          block["text"] = "unknown tool :" + name;
          reply["result"]["content"] = {block};
          reply["result"]["isError"] = true;
        }

      } else {
        reply["error"] = {{"code", -32601}, {"message", "Method not found"}};
      }
      std::cout << reply.dump() << "\n" << std::flush;
      continue;
    } catch (...) {
      // std::cerr << "Parse error at byte: " << e.byte;
      json reply;
      reply["jsonrpc"] = "2.0";
      reply["id"] = nullptr;
      reply["error"] = {{"code", -32700}, {"message", "Parse error"}};
      std::cout << reply.dump() << "\n" << std::flush;
    };
  }

  return 0;
}
