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
- **Tests** — GoogleTest via `FetchContent` with an offline vendored fallback
  (`third_party/googletest/`); smoke tests run under `ctest`.

### Notes
- This is the warm-up phase. Next: rebuild in clean layers following the milestone
  roadmap in `docs/design.md` (M0→M5), toward the typed-tool centerpiece.
