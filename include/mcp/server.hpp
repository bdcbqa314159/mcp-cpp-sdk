#pragma once
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace mcp {

using json = nlohmann::json;

using Handler = std::function<std::string(const json& args)>;

class McpServer {
public:
  McpServer(std::string name, std::string version)
      : name_(std::move(name)), version_(std::move(version)) {}

  void add_tool(const std::string& name, std::string description, json input_schema,
                Handler handler) {
    tools_[name] =
        Tool{std::move(description), std::move(input_schema), std::move(handler)};
  }

  void run() {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty())
        continue;

      try {
        json req = json::parse(line);

        if (!req.contains("id")) {
          std::cerr << "no id in the request.. continue...\n";
          continue;
        }

        auto id = req["id"];
        json reply;
        reply["jsonrpc"] = "2.0";
        reply["id"] = id;

        handle(req, reply);

        std::cout << reply.dump() << "\n" << std::flush;
        continue;

      } catch (json::parse_error& error) {
        json reply;
        reply["jsonrpc"] = "2.0";
        reply["id"] = nullptr;
        reply["error"] = {{"code", -32700}, {"message", "Parse error"}};
        std::cout << reply.dump() << "\n" << std::flush;

      } catch (...) {
        std::cerr << "Unknown error\n";
      }
    }
  }

private:
  struct Tool {
    std::string description;
    json input_schema;
    Handler handler;
  };

  std::string name_, version_;
  std::map<std::string, Tool> tools_;

  void handle(const json& req, json& reply) {
    const std::string method = req["method"];

    if (method == "initialize") {
      reply["result"] = {{"protocolVersion", req["params"]["protocolVersion"]},
                         {"capabilities", {{"tools", json::object()}}},
                         {"serverInfo", {{"name", name_}, {"version", version_}}}};
    } else if (method == "ping") {
      reply["result"] = {{"ok", true}};
    }

    else if (method == "tools/list") {

      json arr = json::array();

      for (const auto& [name, tool] : tools_) {
        json obj;
        obj["name"] = name;
        obj["description"] = tool.description;
        obj["inputSchema"] = tool.input_schema;

        arr.push_back(obj);
      }

      reply["result"] = {{"tools", arr}};

    }

    else if (method == "tools/call") {
      std::string name = req.at("params").at("name");

      auto it = tools_.find(name);

      if (it != tools_.end()) {

        try {

          json block;
          block["type"] = "text";
          block["text"] = it->second.handler(req.at("params").at("arguments"));
          reply["result"]["content"] = {block};

        } catch (std::exception& e) {

          json block;
          block["type"] = "text";
          block["text"] = std::string("tool error: ") + e.what();
          reply["result"]["content"] = {block};
          reply["result"]["isError"] = true;
        }
      }

      else {
        json block;
        block["type"] = "text";
        block["text"] = "unknown tool :" + name;
        reply["result"]["content"] = {block};
        reply["result"]["isError"] = true;
      }

    } else {
      reply["error"] = {{"code", -32601}, {"message", "Method not found"}};
    }
  }
};

} // namespace mcp
