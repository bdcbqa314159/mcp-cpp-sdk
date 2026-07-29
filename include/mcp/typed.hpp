#pragma once
#include <string_view>
#include <tuple>

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

} // namespace mcp
