#pragma once
#include <mcp/json_rpc.hpp>
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
  // TODO (M4 task 1): return a Field<Struct, T> holding these four values.
  return {member, name, description, required};
}

// Bundle field descriptors together — this is what a struct's describe() returns.
// A std::tuple can hold different Field<...> types (each field has its own T).
template <typename... Fs> constexpr auto fields(Fs... fs) {
  // TODO (M4 task 1): pack the descriptors into a std::tuple (std::make_tuple).
  return std::make_tuple(fs...);
}

// ---- Schema generation ------------------------------------------------------

// Map a C++ field type to its JSON Schema "type" string, chosen at COMPILE TIME.
//
// TODO (M4 task 2): fill this in with an `if constexpr` chain:
//     if constexpr (std::is_same_v<T, bool>)          return "boolean";
//     else if constexpr (std::is_integral_v<T>)       return "integer";
//     else if constexpr (std::is_floating_point_v<T>) return "number";
//     else if constexpr (std::is_same_v<T, std::string>) return "string";
//     else                                            return "object";
//
// GOTCHA — order matters: `bool` satisfies std::is_integral_v too, so the bool
// check MUST come first, or every bool would be typed "integer".
//
// Why `if constexpr` and not a normal `if`? Each branch mentions a different type;
// a runtime `if` would require every branch to compile for every T. `if constexpr`
// discards the untaken branches at compile time, so only the matching one is kept.
template <typename T> constexpr const char* json_type_name() {
  // return "unknown";  // TODO: replace with the if constexpr chain above
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

} // namespace mcp
