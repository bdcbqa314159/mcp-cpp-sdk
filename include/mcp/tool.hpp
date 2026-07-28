#pragma once
#include <functional>
#include <map>
#include <string>
#include <mcp/json_rpc.hpp>

namespace mcp {

// The result of a tool call: a list of content blocks, plus whether the tool
// failed. MCP models a *tool* failure as a successful response carrying
// isError=true — distinct from a JSON-RPC protocol error.
struct ToolResult {
  json content;         // a JSON array of content blocks
  bool isError = false;
};

// Build a single text-block result: {content:[{type:"text",text:s}], isError:false}.
ToolResult text(std::string s);

// A raw tool handler: JSON arguments in, a ToolResult out. (Typed handlers arrive
// in M4; this raw form is the escape hatch.)
using ToolHandler = std::function<ToolResult(const json& arguments)>;

// One registered tool: what the client sees (name/description/inputSchema) plus the
// handler that runs it.
struct Tool {
  std::string name;
  std::string description;
  json input_schema;
  ToolHandler handler;
};

// Name -> Tool. Backs tools/list and (next task) tools/call.
class ToolRegistry {
public:
  void add(Tool tool);
  bool contains(const std::string& name) const;

  // The tools/list result: {"tools": [ {name, description, inputSchema}, ... ]}.
  json list() const;

private:
  std::map<std::string, Tool> tools_;
};

}  // namespace mcp
