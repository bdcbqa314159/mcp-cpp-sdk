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

// TODO (task 2): serialize a Request.
//   Always write: "jsonrpc" = "2.0" and "method" = r.method.
//   params and id are std::optional — only write them to `j` when they hold a
//   value. Test an optional with `if (opt)`; read the value with `*opt`.
//   Writing `*r.id` reuses the Id to_json you already wrote.
void to_json(json& j, const Request& r) {
  j["jsonrpc"] = "2.0";
  j["method"] = r.method;

  if (r.params)
    j["params"] = *r.params;
  if (r.id)
    j["id"] = *r.id;
}

// TODO (task 2): parse a Request.
//   method is REQUIRED  -> read it with j.at("method") (throws if missing).
//   params / id are OPTIONAL -> if the key is present set the optional, else
//   leave it as std::nullopt. Test presence with j.contains("key").
//   Reading the id: j.at("id").get<Id>() reuses the Id from_json.
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

} // namespace mcp
