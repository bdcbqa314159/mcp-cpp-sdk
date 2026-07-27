#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>

namespace mcp {

using json = nlohmann::json;

// A JSON-RPC id is one of: an integer, a string, or null/absent.
//
// Id is a *struct* (not a bare `using Id = std::variant<...>`) on purpose: nlohmann
// finds to_json/from_json by ADL, which searches the type's own namespace. A variant
// alias lives in `std`, so our hooks in `mcp` would be invisible. A struct in `mcp`
// fixes that — and makes Id a real value type. std::monostate is the "empty"
// alternative; it stands for the null/absent id.
struct Id {
  std::variant<std::int64_t, std::string, std::monostate> value;
  bool operator==(const Id&) const = default; // C++20 gives us == for free
};

// ADL hooks — `json j = id;` and `id = j.get<Id>();` dispatch to these.
// Declared here, defined in src/json_rpc.cpp.
void to_json(json& j, const Id& id);
void from_json(const json& j, Id& id);

} // namespace mcp
