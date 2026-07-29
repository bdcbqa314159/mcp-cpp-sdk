#pragma once
#include <functional>
#include <map>
#include <stdexcept>
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

// Tool authors throw this to signal a failure with a message. The registry catches
// it at the call boundary and turns it into an isError ToolResult — the ergonomic
// exception at the tool edge, converted to a value the moment it crosses back in.
class ToolError : public std::runtime_error {
public:
  explicit ToolError(const std::string& message) : std::runtime_error(message) {}
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

  // Run a tool by name. Always returns a ToolResult (never throws out): an unknown
  // tool or a thrown ToolError/exception becomes an isError result.
  ToolResult call(const std::string& name, const json& arguments) const;

private:
  std::map<std::string, Tool> tools_;
};

}  // namespace mcp
