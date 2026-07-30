# mcp-cpp-sdk

A small, modern-C++20 SDK for building [Model Context Protocol](https://modelcontextprotocol.io)
servers — built to be genuinely useful *and* to teach C++ by writing every layer ourselves.

Full plan, architecture, and milestone roadmap: [`docs/design.md`](docs/design.md).

## Status

A working, layered MCP server SDK (milestones M0–M5). It speaks MCP `2025-06-18` over
stdio: a JSON-RPC core, the lifecycle handshake, `tools/list` / `tools/call`, and a
**typed-tool layer** where one struct describes a tool's arguments and the SDK derives
*both* the JSON schema and the argument parsing from it. The `sandbox/` rungs remain as
the by-hand learning path the layered code was built from.

## Quickstart

Describe a tool's arguments once; the schema and the parsing both come from it:

```cpp
#include <string>
#include <mcp/server.hpp>

struct AddArgs {
  int a = 0;
  int b = 0;
  static constexpr auto describe() {
    return mcp::fields(mcp::field(&AddArgs::a, "a", "first addend"),
                       mcp::field(&AddArgs::b, "b", "second addend"));
  }
};

int main() {
  mcp::Server server{"my-server", "1.0"};
  server.tool<AddArgs>("add", "Add two integers", [](const AddArgs& x) {
    return mcp::text(std::to_string(x.a + x.b));
  });
  server.run();  // MCP over stdio; logs to stderr, JSON-RPC on stdout
}
```

`tools/list` advertises the generated schema; `tools/call` hands your handler a parsed,
validated `AddArgs`. A missing required argument comes back as an `isError` result, not a
crash.

## Use it in your project

Link the **`mcp::mcp`** target. Two ways to get the SDK:

**FetchContent / add_subdirectory** — zero install; fetches nlohmann/json for you, and
skips the SDK's own examples/tests/sandbox when consumed this way:

```cmake
include(FetchContent)
FetchContent_Declare(mcp
  GIT_REPOSITORY https://github.com/bdcbqa314159/mcp-cpp-sdk.git
  GIT_TAG main)
FetchContent_MakeAvailable(mcp)

add_executable(my_server main.cpp)
target_link_libraries(my_server PRIVATE mcp::mcp)
```

**Installed package** — after `cmake --install` (requires `nlohmann_json` discoverable via
`find_package`):

```cmake
find_package(mcp 0.1 REQUIRED)
target_link_libraries(my_server PRIVATE mcp::mcp)
```

Build options (each defaults ON only when the SDK is the **top-level** project, so
consumers don't build them): `MCP_BUILD_EXAMPLES`, `MCP_BUILD_SANDBOX`, `MCP_BUILD_TESTS`,
plus `MCP_SANITIZE`.

## Build

With CMake presets (recommended — needs CMake ≥3.25 and Ninja):

```sh
cmake --preset debug        # or: release | asan   (fetches deps via FetchContent)
cmake --build build/debug
ctest --preset debug
```

Or plain CMake (what CI uses, any generator):

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The `asan` preset builds with Address + UB sanitizers (Clang/GCC) — use it while
developing to catch lifetime/UB bugs early.

## Run the example server

`examples/add_server` exposes `add` and `multiply`. It speaks MCP over stdio, so a client
must complete the handshake before calling tools. Drive it by hand:

```sh
printf '%s\n%s\n%s\n' \
 '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{}}' \
 '{"jsonrpc":"2.0","method":"notifications/initialized"}' \
 '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"add","arguments":{"a":2,"b":3}}}' \
 | ./build/debug/add_server
```

## Connect to Claude Desktop

Add the built binary to Claude Desktop's config
(`~/Library/Application Support/Claude/claude_desktop_config.json` on macOS), then restart it:

```json
{
  "mcpServers": {
    "mcp-cpp-sdk": {
      "command": "/absolute/path/to/mcp-cpp-sdk/build/debug/add_server"
    }
  }
}
```

The tool then appears to Claude. Remember: **stdout is the protocol channel** — all logging
goes to stderr, or you corrupt the stream.

## Tests

```sh
cmake -B build && cmake --build build
cd build && ctest --output-on-failure
```

GoogleTest is pulled automatically via CMake `FetchContent`. **Offline?** Download the
GoogleTest source by hand and point CMake at it — FetchContent uses that copy instead
of fetching:

```sh
cmake --preset debug -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest
```

Disable tests entirely with `-DMCP_BUILD_TESTS=OFF`.

## Layout

| Path | What |
|------|------|
| `include/mcp/` | The SDK (public headers): `server`, `session`, `tool`, `typed`, `serve`, `transport`, `json_rpc`, `result`, `logger` |
| `src/` | Non-template implementations |
| `examples/` | Servers written *against* the SDK (`add_server`, `echo_server`) |
| `tests/` | GoogleTest unit tests + `tools/mcp_probe` black-box harness |
| `sandbox/rung*.cpp` | Learning steps — the protocol built by hand, one concept at a time |
| `docs/design.md` | The design & milestone plan |
| `docs/m1-json-rpc-core.md`, `docs/m4-typed-tools.md` | C++ reading guides for the core and the typed layer |
