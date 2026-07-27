#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <stdexcept>
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
//
// TODO (task 1): implement both, operating on `id.value`.
//
//   to_json:  inspect which alternative `id.value` holds and write matching JSON:
//     - std::int64_t   -> a JSON number
//     - std::string    -> a JSON string
//     - std::monostate -> JSON null
//     (tools: std::holds_alternative<T>(v), std::get<T>(v), or std::visit)
//
//   from_json: inspect the JSON's type and set `id.value` to the right alternative:
//     - j.is_number_integer() -> std::int64_t
//     - j.is_string()         -> std::string
//     - j.is_null()           -> std::monostate
//     - anything else         -> throw (an id can't be a float/array/object)
//       (for now: `throw std::runtime_error("...")`; we standardise errors later.)

template <typename... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};

inline void to_json(json& j, const Id& id) {

  const auto visitor =
      overloads{[&](std::int64_t i) { j = i; }, [&](std::string_view s) { j = s; },
                [&](std::monostate m) { j = nullptr; }};
  std::visit(visitor, id.value);
}

inline void from_json(const json& j, Id& id) {

  if (j.is_number_integer())
    id.value = j.get<std::int64_t>();

  else if (j.is_string())
    id.value = j.get<std::string>();

  else if (j.is_null())
    id.value = std::monostate{};

  else
    throw std::runtime_error("JSON-RPC id must be int, string, or null");
}

} // namespace mcp
