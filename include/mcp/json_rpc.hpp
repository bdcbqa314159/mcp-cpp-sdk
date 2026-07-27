#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
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

// A JSON-RPC request: a method call. Two fields may be ABSENT, so they're
// std::optional (a value-or-nothing box):
//   - params: many methods take no arguments.
//   - id:     an absent id means this is a *notification* — no reply is expected.
//             (This is the notification rule you hand-coded back in the rungs,
//              now expressed in the type itself.)
struct Request {
  std::string method;
  std::optional<json> params;
  std::optional<Id> id;
  bool operator==(const Request&) const = default;
};

void to_json(json& j, const Request& r);
void from_json(const json& j, Request& r);

// A JSON-RPC error object: a numeric code, a human message, and optional extra
// data. (Codes: -32700 parse error, -32601 method not found, etc.)
struct Error {
  int code;
  std::string message;
  std::optional<json> data;
  bool operator==(const Error&) const = default;
};

void to_json(json& j, const Error& e);
void from_json(const json& j, Error& e);

// A JSON-RPC response: an id, plus EITHER a result OR an error — never both.
// The std::variant enforces that at the type level: `payload` is exactly one of
//   - json   : the success result value
//   - Error  : the failure
// You can't represent "both" or "neither", which two optionals would allow.
struct Response {
  Id id;
  std::variant<json, Error> payload;
  bool operator==(const Response&) const = default;
};

void to_json(json& j, const Response& r);
void from_json(const json& j, Response& r);

} // namespace mcp
