#include <mcp/json_rpc.hpp>

#include <stdexcept>
#include <string_view>

namespace mcp {

namespace {
// The "overload set" idiom: bundle several lambdas into one callable so
// std::visit can pick the right one per variant alternative.
template <typename... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

void to_json(json& j, const Id& id) {
  const auto visitor =
      overloads{[&](std::int64_t i) { j = i; }, [&](std::string_view s) { j = s; },
                [&](std::monostate m) { j = nullptr; }};
  std::visit(visitor, id.value);
}

void from_json(const json& j, Id& id) {
  if (j.is_number_integer())
    id.value = j.get<std::int64_t>();
  else if (j.is_string())
    id.value = j.get<std::string>();
  else if (j.is_null())
    id.value = std::monostate{};
  else
    throw std::runtime_error("JSON-RPC id must be int, string, or null");
}

// Request -> JSON. jsonrpc + method always; params/id only when present
// (an absent id is what makes a request a notification).
void to_json(json& j, const Request& r) {
  j["jsonrpc"] = "2.0";
  j["method"] = r.method;

  if (r.params)
    j["params"] = *r.params;
  if (r.id)
    j["id"] = *r.id;
}

// JSON -> Request. Rejects a wrong jsonrpc version; method required; params/id
// optional (absent -> the optional stays std::nullopt).
void from_json(const json& j, Request& r) {
  if (j.at("jsonrpc") != "2.0") {
    throw std::runtime_error{"unsupported version"};
  }

  r.method = j.at("method");
  if (j.contains("params"))
    r.params = j.at("params");
  if (j.contains("id"))
    r.id = j.at("id").get<Id>();
}

// Error -> JSON. code + message always; data only when present.
void to_json(json& j, const Error& e) {
  j["code"] = e.code;
  j["message"] = e.message;

  if (e.data)
    j["data"] = *e.data;
}

// JSON -> Error. code + message required; data optional.
void from_json(const json& j, Error& e) {
  e.code = j.at("code");
  e.message = j.at("message");
  if (j.contains("data"))
    e.data = j.at("data");
}

// Response -> JSON. jsonrpc + id always; then "result" (json) or "error" (Error),
// chosen by which alternative the payload variant holds.
void to_json(json& j, const Response& r) {
  j["jsonrpc"] = "2.0";
  j["id"] = r.id;

  const auto visitor = overloads{[&](Error e) { j["error"] = e; },
                                 [&](const json& res) { j["result"] = res; }};
  std::visit(visitor, r.payload);
}

// JSON -> Response. id required; exactly one of result/error must be present.
void from_json(const json& j, Response& r) {
  r.id = j.at("id");

  if (j.contains("result"))
    r.payload = j.at("result");
  else if (j.contains("error"))
    r.payload = j.at("error").get<Error>();
  else
    throw std::runtime_error("JSON-RPC payload is either error or json");
}

} // namespace mcp
