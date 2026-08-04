# Working agreement — mcp-cpp-sdk

Full plan in `docs/design.md`. This file is the short version Claude Code reads each session.

## The point of this project

A real, spec-correct MCP C++ SDK. It also doubles as a deep modern-C++ learning project for the
author, who writes the load-bearing code by hand — which is why the history is deliberately
incremental. The conventions below are what any contributor (or agent) needs.

> Maintainer's personal working mode ("learn-by-building") lives in the git-ignored
> `CLAUDE.local.md` — not needed to build or contribute; ignore it if you're not the author.

## Conventions

- **C++20.** Value semantics by default; `unique_ptr` for single-owner resources.
  **No singletons, no shared_ptr-everywhere** (a deliberate correction of the references).
- **stdout is sacred** — it carries only JSON-RPC. All logs go to **stderr**. #1 beginner bug.
- **Flush every reply** written to stdout.
- **JSON:** nlohmann/json via CMake `FetchContent`. Wrap it behind our own types at the public API.
- **MCP revision:** target **2025-06-18**. Transport: **stdio first**; HTTP is a later phase.
- **Errors:** `mcp::Result<T>` (on `std::variant`) internally; `throw mcp::ToolError` only at
  the tool boundary, which the framework converts to a JSON-RPC error.
- **Typed-tool layer is the centerpiece:** one struct description generates *both* the JSON
  schema and the argument parsing, so they cannot drift.

## Build

```sh
cmake --preset debug   # or release | asan
cmake --build build/debug
ctest --preset debug
```
`compile_commands.json` is symlinked to `build/debug/` for clangd (Zed reads it automatically).

## Settled decisions

- **Toolchain: cross-platform.** macOS → Apple Clang, Linux → latest GCC, Windows → MSVC.
  Code stays portable C++20; CI is a matrix over all three (`.github/workflows/ci.yml`).
- **External test harness: C++** (not Python) — `tools/mcp_probe`, built at M1.
- **Tests: GoogleTest** via FetchContent, offline fallback at `third_party/googletest/`.
- **Presets:** `debug` / `release` / `asan` (Ninja). The `asan` preset enables
  Address+UB sanitizers on Clang/GCC via the `MCP_SANITIZE` CMake option.

## Git flow

Branch per milestone (`m1-json-rpc-core`), small meaningful commits, PR per milestone into
`main` with acceptance test green. Bernardo authors commits. Repo will be published to GitHub.

## Where we are

Warm-up in `sandbox/` (protocol by hand, rungs 1–4 done: stdio loop → JSON-RPC → lifecycle →
tools). Next: consolidate into a header-only `McpServer` as a bridge, then start **M0** and
rebuild the real layered architecture from `docs/design.md`.
