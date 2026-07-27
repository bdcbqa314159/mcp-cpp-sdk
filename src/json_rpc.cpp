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
}  // namespace

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

}  // namespace mcp
