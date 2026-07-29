# M4 — the typed-tool layer: a C++ reading guide

The SDK's signature feature: describe a tool's arguments **once**, and derive both the
JSON schema and the argument parsing from that single description, so they can't drift.
This is the deepest C++ in the project. All of it lives in `include/mcp/typed.hpp`
(header-only templates; `src/typed.cpp` is just a clangd anchor).

The user-facing shape it enables:

```cpp
struct EchoArgs {
  std::string msg;
  static constexpr auto describe() {
    return mcp::fields(mcp::field(&EchoArgs::msg, "msg", "the message to echo"));
  }
};

server.tool<EchoArgs>("echo", "Echo a message",
    [](const EchoArgs& a) -> mcp::ToolResult { return mcp::text(a.msg); });
```

---

## 1. The primitive: pointer-to-member

`&EchoArgs::msg` has type `std::string EchoArgs::*` — a **pointer-to-member**: it names
*which field* of `EchoArgs`, bound to no instance. Given an object you apply it with `.*`:
`obj.*ptr` reads/writes that field of `obj`.

Why it's the linchpin: C++ has no runtime reflection, but the SDK needs to touch fields
of a user struct it's never seen. A pointer-to-member is a **type-safe handle to a field**
the library can capture now and apply later — read its type `T` (for the schema) and write
through it (for parsing).

```cpp
template <typename Struct, typename T> struct Field {
  using value_type = T;          // recover the field's type later
  T Struct::* member;            // the pointer-to-member
  std::string_view name, description;
  bool required = true;
};
```

`field(&Struct::m, ...)` **deduces** `Struct` and `T` from the member pointer, so each call
yields a `Field<Struct, T>` with that field's own `T`. `fields(...)` bundles them into a
`std::tuple` — heterogeneous on purpose, since every field has a different `T`.

## 2. Schema generation — `if constexpr`

`json_type_name<T>()` maps a C++ type to a JSON Schema type string, at **compile time**:

```cpp
if constexpr (std::is_same_v<T, bool>)          return "boolean";
else if constexpr (std::is_integral_v<T>)       return "integer";  // bool FIRST — it's integral too
else if constexpr (std::is_floating_point_v<T>) return "number";
else if constexpr (std::is_same_v<T, std::string>) return "string";
else                                            return "object";
```

Two ideas:
- **One instantiation per distinct `T`, each keeping only its matching branch.**
  `json_type_name<std::string>` *is* a function whose body is just `return "string";` — the
  other branches produce no code. `if constexpr` discards the untaken branches (they aren't
  even instantiated for that `T`), which is what lets each branch do type-specific things a
  plain runtime `if` couldn't.
- **Order matters:** `bool` is integral, so it must be checked before `is_integral_v`.

`schema_for<Args>()` walks `Args::describe()` with `std::apply` + a **fold expression**
(`( run(field), ... )` runs the body once per tuple element) and assembles
`{"type":"object","properties":{…},"required":[…]}`.

## 3. Argument parsing — writing through the member pointer

`parse_args<Args>()` default-constructs an `Args`, then for each described field:

```cpp
if (arguments.contains(key))
  out.*(field.member) = arguments.at(key).get<T>();   // write THROUGH the member pointer
else if (field.required)
  throw ToolError("missing required argument: " + key);
// missing + optional -> keep the struct's default (do nothing)
```

- `out.*(field.member) = …` — the `.*` operator on the **left** of `=` writes into that field
  of `out`.
- `.get<T>()` extracts the JSON value **as the field's own type** (`T` from the descriptor);
  nlohmann dispatches on `T` internally, so no `if constexpr` is needed here.
- A missing required field throws `ToolError` — which the `ToolRegistry::call` boundary
  (M3) catches and turns into an `isError` result. The error path composes for free.

## 4. Tying it together — `add_typed_tool`

The bridge from a **typed** handler (`const Args& → ToolResult`) to the **raw** handler the
registry stores (`json → ToolResult`):

```cpp
registry.add(Tool{name, description, schema_for<Args>(),
    [handler = std::move(handler)](const json& arguments) -> ToolResult {
      Args a = parse_args<Args>(arguments);   // JSON -> typed struct (throws on bad input)
      return handler(a);                       // call the user's typed handler
    }});
```

- **Schema** (`schema_for<Args>()`) and **parsing** (`parse_args<Args>()`) both flow from
  `Args::describe()` — one source, no drift.
- `[handler = std::move(handler)]` is an **init-capture**: the lambda is stored in the
  registry and called long after `add_typed_tool` returns, so it must **own** the handler,
  not reference a dead local. Move it into the closure.

## 5. The facade (M5)

`Server::tool<Args>(name, description, handler)` is a one-line forward to `add_typed_tool`,
and `Server::run()` forwards to `serve`. That's the whole public API:
*define a struct with `describe()`, write a handler that takes it, register, run.*

---

## Concept index

| C++ idea | Where |
|---|---|
| pointer-to-member (`T S::*`, `.*`) | `Field`, `parse_args` (§1, §3) |
| template argument deduction | `field()` deduces `Struct`, `T` (§1) |
| variadic templates + `std::tuple` (heterogeneous) | `fields()` (§1) |
| `if constexpr` compile-time dispatch | `json_type_name` (§2) |
| type traits (`is_same_v`, `is_integral_v`, …) | `json_type_name` (§2) |
| `std::apply` + fold expression | `schema_for`, `parse_args` (§2, §3) |
| member typedef to recover a type | `Field::value_type` (§2) |
| lambda init-capture (`[x = std::move(x)]`) | `add_typed_tool` (§4) |
| type erasure (typed handler → `std::function`) | `add_typed_tool` (§4) |

## Traps met

- **`bool` is integral** — check `is_same_v<T,bool>` before `is_integral_v<T>`, or every
  bool is typed `"integer"`.
- **A stored lambda must own its captures** — capturing the handler by reference dangles once
  the registering function returns; init-capture with `std::move` fixes it.
- **`if constexpr` vs `if`** — only `if constexpr` discards untaken branches, so branches may
  contain code that wouldn't compile for other `T`.
