#include <exception>
#include <mcp/tool.hpp>

#include <utility>

namespace mcp {

// Build a single text-block result (provided — this is the shape a handler returns).
ToolResult text(std::string s) {
  ToolResult result;
  result.content = json::array();
  result.content.push_back({{"type", "text"}, {"text", std::move(s)}});
  return result;
}

// Register (or replace) a tool, keyed by its own name. (Read the name into a local
// before moving `tool` — the key/value evaluation order isn't guaranteed.)
void ToolRegistry::add(Tool tool) {
  auto name = tool.name;
  tools_.insert_or_assign(std::move(name), std::move(tool));
}

bool ToolRegistry::contains(const std::string& name) const {
  return tools_.contains(name);
}

// Run the named tool and always return a ToolResult: an unknown tool, a thrown
// ToolError, or any other exception all become an isError result — the tool
// boundary where exceptions are caught and converted back to values.
ToolResult ToolRegistry::call(const std::string& name, const json& arguments) const {
  auto it = tools_.find(name);

  if (it == tools_.end()) {
    ToolResult result = text(name);
    result.isError = true;
    return result;
  }
  try {
    return it->second.handler(arguments);
  } catch (const ToolError& e) {
    ToolResult result = text(e.what());
    result.isError = true;
    return result;

  } catch (const std::exception& e) {
    ToolResult result = text("tool failed " + std::string(e.what()));
    result.isError = true;
    return result;
  }
}

// The tools/list result: {"tools": [ {name, description, inputSchema}, ... ]}.
json ToolRegistry::list() const {
  json arr = json::array();
  for (const auto& [name, t] : tools_)
    arr.push_back({{"name", name},
                   {"description", t.description},
                   {"inputSchema", t.input_schema}});
  return json{{"tools", arr}};
}

} // namespace mcp
