#pragma once
#include <mcp/json_rpc.hpp>
#include <mcp/tool.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace mcp {

// ---- Pointer-to-member, the new primitive -----------------------------------
// `T Struct::*` is a *pointer to a data member*: it names "some member of Struct,
// of type T" without pointing at any particular object. Given an instance `s`, you
// dereference it with the `.*` operator:
//
//     struct P { std::string name; };
//     std::string P::* m = &P::name;   // m names the `name` member of P
//     P p{"Ada"};
//     p.*m;                            // == "Ada"  (bind the member to instance p)
//
// This is what lets us describe a field generically: capture &Args::field once,
// then later both read its C++ type (for the schema) and write into it (for parsing).

// One described field of an args struct: which member it is, plus JSON metadata.
template <typename Struct, typename T> struct Field {
  using value_type = T;         // the field's C++ type, recovered later for the schema
  T Struct::* member;           // pointer-to-member: the field itself
  std::string_view name;        // its JSON name
  std::string_view description; // human description (for the schema)
  bool required = true;         // required in the JSON schema?
};

// Build one Field descriptor. `Struct` and `T` are *deduced* from the member
// pointer you pass, so callers just write `field(&Args::msg, "msg", "...")`.
template <typename Struct, typename T>
constexpr Field<Struct, T> field(T Struct::* member, std::string_view name,
                                 std::string_view description, bool required = true) {
  return {member, name, description, required};
}

// Bundle field descriptors together — this is what a struct's describe() returns.
// A std::tuple can hold different Field<...> types (each field has its own T).
template <typename... Fs> constexpr auto fields(Fs... fs) {
  return std::make_tuple(fs...);
}

// ---- Schema generation ------------------------------------------------------

// Map a C++ field type to its JSON Schema "type" string, chosen at COMPILE TIME.
// Order matters: `bool` satisfies std::is_integral_v too, so it must be checked
// first. `if constexpr` keeps only the matching branch per T (the untaken branches
// are discarded, not just skipped — so they may hold type-specific code).
template <typename T> constexpr const char* json_type_name() {
  if constexpr (std::is_same_v<T, bool>)
    return "boolean";
  else if constexpr (std::is_integral_v<T>)
    return "integer";
  else if constexpr (std::is_floating_point_v<T>)
    return "number";
  else if constexpr (std::is_same_v<T, std::string>)
    return "string";
  else
    return "object";
}

// Build the JSON Schema for an args struct from its describe() field list.
// (Provided — this is the tuple-walking machinery; you supply json_type_name.)
// std::apply unpacks the describe() tuple into a pack `field...`; the fold
// `( run(field), ... )` runs the per-field body once for each descriptor.
template <typename Args> json schema_for() {
  json properties = json::object();
  json required = json::array();

  std::apply(
      [&](auto... field) {
        (([&] {
           using T = typename decltype(field)::value_type;
           properties[std::string(field.name)] = {
               {"type", json_type_name<T>()},
               {"description", std::string(field.description)}};
           if (field.required)
             required.push_back(std::string(field.name));
         }()),
         ...);
      },
      Args::describe());

  json schema;
  schema["type"] = "object";
  schema["properties"] = properties;
  schema["required"] = required;
  return schema;
}

// ---- Argument parsing -------------------------------------------------------

// Parse a JSON `arguments` object into a fresh Args struct, using the describe()
// field list. (Provided — the tuple-walk; you supply the per-field body.)
template <typename Args> Args parse_args(const json& arguments) {
  Args out{}; // start from the struct's defaults

  std::apply(
      [&](auto... field) {
        (([&] {
           using T = typename decltype(field)::value_type;
           const std::string key(field.name);
           // Present -> write through the member pointer as the field's type T.
           // Missing + required -> ToolError. Missing + optional -> keep default.
           if (arguments.contains(key))
             out.*(field.member) = arguments.at(key).get<T>();
           else if (field.required)
             throw ToolError("missing required argument: " + key);
         }()),
         ...);
      },
      Args::describe());

  return out;
}

// ---- Tying it together ------------------------------------------------------

// Register a TYPED tool: the schema and the parsing both come from Args::describe().
// The caller writes a handler taking a parsed `const Args&`; this wraps it in a raw
// ToolHandler that parse_args()es the JSON first, so schema and parsing can't drift.
//
// TODO (M4 task 4): build and register the tool.
//   registry.add(Tool{
//       std::move(name),
//       std::move(description),
//       schema_for<Args>(),                       // inputSchema from the struct
//       [handler = std::move(handler)](const json& arguments) -> ToolResult {
//         Args a = parse_args<Args>(arguments);   // throws ToolError on bad input
//         return handler(a);                      // call the typed handler
//       }});
template <typename Args, typename Handler>
void add_typed_tool(ToolRegistry& registry, std::string name, std::string description,
                    Handler handler) {
  registry.add(Tool{std::move(name), std::move(description), schema_for<Args>(),
                    [handler = std::move(handler)](const json& arguments) -> ToolResult {
                      Args a = parse_args<Args>(arguments);
                      return handler(a);
                    }});
}

} // namespace mcp
