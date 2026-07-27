# M1 — JSON-RPC core: a C++ reading guide

A component-by-component tour of the M1 layer, focused on the **C++**. Read the
files bottom-up in the order below; each section says what the component is, which
files hold it, and the language ideas it exercises.

```
serve()            compose the loop            src/serve.cpp
  │
Dispatcher         method name -> handler      include/mcp/dispatcher.hpp, src/dispatcher.cpp
  │
Result<T>          value-or-error              include/mcp/result.hpp
  │
value types        Id/Request/Response/Error   include/mcp/json_rpc.hpp, src/json_rpc.cpp
  │
ITransport         the pipe, behind an iface   include/mcp/transport.hpp, src/transport.cpp
```

Layout convention: **declarations in `include/mcp/*.hpp`, definitions in `src/*.cpp`**
(same basename). Templates (`Result<T>`) are header-only; their `.cpp` exists only so
the editor's clangd has a same-named translation unit to read compile flags from.

---

## 1. `Id` — `std::variant`
*Files: `json_rpc.hpp` (struct + declarations), `json_rpc.cpp` (`to_json`/`from_json`).*

A JSON-RPC id is an integer, a string, or null. Modelled as:
```cpp
struct Id { std::variant<std::int64_t, std::string, std::monostate> value; };
```
- **`std::variant<...>`** — a type-safe union: holds exactly one alternative at a time.
- **`std::monostate`** — an empty placeholder type; here it *is* the "null/absent" id.
- **`Id` is a struct, not a `using` alias.** nlohmann finds `to_json`/`from_json` by
  **ADL**, which searches the *type's own namespace*. A `std::variant` alias lives in
  `std`, so our hooks in `mcp` would be invisible — a struct in `mcp` fixes that.
- **`to_json` uses the overload-set idiom** to visit the variant:
  ```cpp
  template <typename... Ts> struct overloads : Ts... { using Ts::operator()...; };
  std::visit(overloads{[&](std::int64_t i){ j = i; }, ...}, id.value);
  ```
- **`from_json`** inspects the JSON type (`is_number_integer()` / `is_string()` /
  `is_null()`) and throws on anything else.

## 2. `Request` — `std::optional`
*Files: same. Adds the `Request` struct + hooks.*

```cpp
struct Request { std::string method; std::optional<json> params; std::optional<Id> id; };
```
- **`std::optional<T>`** — value-or-nothing. Default = empty (`std::nullopt`).
  `if (opt)` tests it, `*opt` reads it.
- An **absent `id` means notification** — the type encodes the protocol rule.
- `to_json` writes optional fields **only when present** (`if (r.id) j["id"] = *r.id;`).
- `from_json` requires `method` (`j.at("method")` throws if missing), reads optionals
  with `contains()` then `.at()`, and **validates `jsonrpc == "2.0"`**.

## 3. `Error` + `Response` — illegal states unrepresentable
*Files: same.*

```cpp
struct Error { int code; std::string message; std::optional<json> data; };
struct Response { Id id; std::variant<json, Error> payload; };
```
- A response carries an id plus **either** a result (`json`) **or** an `Error` — never
  both, never neither. The **`std::variant` makes that a compile-time guarantee**; two
  `std::optional`s would allow the invalid "both set" / "neither set" states.
- Serialization picks `"result"` vs `"error"` by which alternative the variant holds.
- Built with **designated initializers** (`Error{.code = -32601, .message = ...}`), C++20.

## 4. `Result<T>` — error-as-value (a class template)
*File: `result.hpp` (header-only), `src/result.cpp` (clangd anchor only).*

```cpp
template <typename T> class Result {           // Result<int>, Result<json>, ...
  std::variant<T, Error> data_;
public:
  Result(T value);   Result(Error error);      // build from either
  bool ok() const;   T& value();   const Error& error() const;
};
```
- Our **C++20 stand-in for `std::expected<T, Error>`**: a function reports failure by
  *returning* an error, not throwing.
- **Class template** — one definition instantiates for every `T`; template members
  live in the header.
- **Checked accessors**: `value()`/`error()` throw on misuse (they wrap `std::get`,
  which already throws `bad_variant_access` on the wrong alternative). Contrast the
  standard's split: `operator*` is UB-fast, `.value()` is throw-checked.

## 5. `ITransport` / `StdioTransport` — interfaces & polymorphism
*Files: `transport.hpp`, `transport.cpp`.*

```cpp
class ITransport {
public:
  virtual ~ITransport() = default;                      // virtual dtor
  virtual std::optional<std::string> read() = 0;        // pure virtual
  virtual void write(std::string_view message) = 0;
};
class StdioTransport : public ITransport {
  std::istream& in_; std::ostream& out_;                // injected streams
public:
  StdioTransport(std::istream& in = std::cin, std::ostream& out = std::cout);
  std::optional<std::string> read() override;
  void write(std::string_view message) override;
};
```
- **Abstract base class** (pure virtual `= 0`, no data) — can't be instantiated, only
  implemented. Keeping transport behind an interface is what lets HTTP slot in later.
- **`override`** — the compiler errors if the signature doesn't match a base method.
- **virtual destructor** — so `delete` through an `ITransport*` runs the right dtor.
- **Dependency injection** — `read`/`write` use `in_`/`out_`, not `std::cin`/`cout`
  directly, which is exactly why unit tests drive it with `std::stringstream`.

## 6. `Dispatcher` — routing with function objects
*Files: `dispatcher.hpp`, `dispatcher.cpp`.*

```cpp
using Handler = std::function<Result<json>(const json& params)>;
class Dispatcher {
  std::map<std::string, Handler> handlers_;
public:
  void on(std::string method, Handler handler);
  std::optional<Response> dispatch(const Request& req);
};
```
- **`std::function`** stores any callable (lambda, function pointer) with that signature.
- A handler returns **`Result<json>`** — this is where error-as-value pays off: the
  dispatcher turns `r.ok() ? r.value() : r.error()` into the `Response` payload.
- Unknown method → `-32601`. **`dispatch` returns `std::optional<Response>`** — `nullopt`
  for a notification (the handler still ran; there's just no reply).

## 7. `serve()` — composition
*Files: `serve.hpp`, `serve.cpp`.*

```cpp
void serve(ITransport& transport, Dispatcher& dispatcher);
```
The loop where every piece connects:
`transport.read()` → `json::parse().get<Request>()` → `dispatcher.dispatch()` →
`to_json` → `transport.write()`. A `try/catch` handles the one thing dispatch can't —
input too malformed to become a `Request` → a `-32700` reply with a null id. `if (resp)`
enforces the notification rule one last time.

## 8. Proof it runs — `echo_server` + `mcp_probe`
*Files: `examples/echo_server.cpp`, `tools/mcp_probe.cpp`.*

- `echo_server` — ~10 lines: register one handler, run `serve()` over real stdio.
- `mcp_probe` — a POSIX (`fork`/`exec`/`pipe`) harness that spawns `echo_server`, sends
  a request over real OS pipes, and checks the response. Registered as the `probe_echo`
  ctest — the **M1 acceptance test**.

---

## Concept index

| C++ idea | See it in |
|---|---|
| `std::variant`, `std::monostate`, `std::visit`, overload-set | `Id`, `Response::payload` (§1, §3) |
| `std::optional` | `Request` fields, `dispatch` return (§2, §6) |
| nlohmann ADL `to_json`/`from_json` | every value type (§1–§3) |
| class template | `Result<T>` (§4) |
| error-as-value | `Result<T>`, `Dispatcher` (§4, §6) |
| abstract interface, pure virtual, `override`, virtual dtor | `ITransport` (§5) |
| dependency injection | `StdioTransport` streams (§5) |
| `std::function` + `std::map` routing | `Dispatcher` (§6) |
| designated initializers (C++20) | `Error`/`Response`/`Id` construction (§3) |
| RAII / value semantics, `unique_ptr` (no `shared_ptr`) | transport ownership (§5, tests) |

## Traps met along the way (worth remembering)

- **`if (someJson)`** converts a `json` to `bool` and *throws* for non-bools — use `==`
  for value checks (`j.at("jsonrpc") != "2.0"`).
- **Visiting a `const` variant**: lambda params must be `const T&`, or `std::visit`
  silently mis-dispatches through nlohmann's promiscuous implicit `json`→anything conversion.
- **`.at("key")` vs `["key"]`** on a `const json`: `.at` throws on a missing key (good);
  `[]` on a const json aborts.
- **clangd + FetchContent**: a header with no same-named `.cpp` gets its compile flags
  guessed from an unrelated file (often a dependency's source) that lacks the include
  path → phantom errors. The `include/` + `src/` same-basename split fixes it.
