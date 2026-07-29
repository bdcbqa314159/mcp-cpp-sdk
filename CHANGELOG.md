# Changelog

All notable changes to this project are recorded here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/); the project
uses [Semantic Versioning](https://semver.org/) once it reaches a first release.

## [Unreleased]

### Added
- **Sandbox rungs** (`sandbox/rung1..4.cpp`) — the MCP protocol built by hand, one
  concept per step: stdio loop → JSON-RPC dispatch → lifecycle (initialize,
  notifications, errors) → tools (`tools/list`, `tools/call`).
- **Header-only SDK** (`include/mcp/server.hpp`) — `mcp::McpServer` with
  `add_tool(name, description, schema, handler)` and `run()`. Registry-driven:
  `handle()` holds zero tool logic; it dispatches whatever is registered.
- **Safe tool boundary** — handler exceptions are caught and returned as an MCP
  `isError` result with a readable message, instead of crashing or hanging.
- **Example server** (`examples/add_server.cpp`) — `add` and `multiply` tools in a
  few lines of user code, no SDK edits between them.
- **Build & tooling** — CMake with `FetchContent` for nlohmann/json; `mcp` INTERFACE
  target; `compile_commands.json` for clangd; `.clang-format`, `.clang-tidy`,
  `.gitignore`.
- **Tests** — GoogleTest via `FetchContent`; offline builds supported through
  `FETCHCONTENT_SOURCE_DIR_GOOGLETEST`. Smoke tests run under `ctest`.
- **CMake presets** — `debug` / `release` / `asan` (Ninja); the `asan` preset turns on
  Address + UB sanitizers via the `MCP_SANITIZE` option.
- **CI** — GitHub Actions matrix: macOS/Apple Clang, Linux/GCC, Windows/MSVC (build + test).

### Added — M1: JSON-RPC core (layered rebuild)
- **Value types** — `Id` (variant), `Request`, `Response`, `Error` with nlohmann
  `to_json`/`from_json`, `std::optional` fields, and `jsonrpc`-version validation.
- **`Result<T>`** — error-as-value on `std::variant` (a C++20 stand-in for `std::expected`).
- **`ITransport` / `StdioTransport`** — transport behind an interface; streams injected
  for testability.
- **`Dispatcher`** — routes a `Request` by method name to a registered handler, returns
  a `Response` (or nothing for a notification).
- **`serve()`** — the loop composing transport + dispatcher into a working JSON-RPC server.
- **`examples/echo_server`** + **`tools/mcp_probe`** — a real server binary and a POSIX
  black-box harness that spawns it and round-trips a request over stdio (M1 acceptance).

### Added — M2: lifecycle
- **`Session`** — a per-connection lifecycle state machine
  (`Uninitialized → Initializing → Ready`) that produces the `initialize` result
  (protocolVersion, capabilities, serverInfo) and gates methods by state.
- **Lifecycle-aware `serve()`** — handles `initialize`, `notifications/initialized`,
  and `ping` via the `Session`; rejects application methods with `-32002` until Ready.
- **`echo_server`** now performs the full handshake; **`mcp_probe`** drives
  initialize → initialized → echo end-to-end (M2 acceptance).

### Added — M3: tools
- **`Tool` + `ToolRegistry`** — register tools (name / description / inputSchema /
  handler); `tools/list` advertises them.
- **`ToolError` + a safe call boundary** — `ToolRegistry::call` always returns a
  `ToolResult`: an unknown tool, a thrown `ToolError`, or any exception becomes an
  `isError` result (never a crash).
- **Full MCP serve loop** — `serve(transport, session, tools)` adds `tools/list` and
  `tools/call` on top of the M2 lifecycle, gated until Ready.
- **`echo_server`** now exposes an `echo` *tool*; **`mcp_probe`** drives
  initialize → initialized → tools/call end-to-end (M3 acceptance).

### Added — M4: typed-tool layer (the centerpiece)
- **`field` / `fields`** — describe an args struct's fields once, via pointer-to-member.
- **`schema_for<Args>()`** — compile-time JSON Schema generation (`if constexpr` maps each
  field's C++ type to its schema type).
- **`parse_args<Args>()`** — parse JSON into a typed struct by writing through member
  pointers; a missing required field throws `ToolError`.
- **`add_typed_tool<Args>()`** — one call registers a tool whose inputSchema *and* argument
  parsing both derive from `Args::describe()`, so they cannot drift.
- **`echo_server`** migrated to the typed API: define a struct, write a handler that takes
  it — schema and validation are automatic.

### Notes
- The SDK's signature feature is in. Next: M5 — polish (retire the warm-up `McpServer`,
  a `Server` facade for `server.tool<Args>(...)`, logging, docs, a second real tool).
