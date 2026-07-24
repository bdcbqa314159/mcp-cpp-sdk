# mcp-cpp-sdk

A small, modern-C++20 SDK for building [Model Context Protocol](https://modelcontextprotocol.io)
servers — built to be genuinely useful *and* to teach C++ by writing every layer ourselves.

Full plan, architecture, and milestone roadmap: [`docs/design.md`](docs/design.md).

## Status

Warm-up phase: building a minimal MCP server by hand (`sandbox/`) to internalise the
protocol, before rebuilding it properly in layers (milestones M0–M5). Current sandbox
covers the full stdio + tools flow: JSON-RPC framing, `initialize`, notifications,
errors, `tools/list`, `tools/call`.

## Build

```sh
cmake -B build          # configures + fetches nlohmann/json via FetchContent
cmake --build build
```

## Run the example server

It speaks MCP over stdio (newline-delimited JSON-RPC). Drive it by hand:

```sh
printf '%s\n%s\n%s\n' \
 '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2025-06-18"}}' \
 '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' \
 '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"add","arguments":{"a":2,"b":3}}}' \
 | ./build/add_server
```

## Connect to Claude Desktop

Add the built binary to Claude Desktop's config
(`~/Library/Application Support/Claude/claude_desktop_config.json` on macOS), then restart it:

```json
{
  "mcpServers": {
    "mcp-cpp-sdk": {
      "command": "/absolute/path/to/mcp-cpp-sdk/build/add_server"
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
GoogleTest repo by hand and drop it at `third_party/googletest/` — CMake prefers a
vendored copy there over the network fetch, so the build works with no internet.
Disable tests entirely with `-DMCP_BUILD_TESTS=OFF`.

## Layout

| Path | What |
|------|------|
| `sandbox/rung*.cpp` | Learning steps — the protocol built by hand, one concept at a time |
| `include/mcp/` | The SDK (public headers) |
| `examples/` | Servers written *against* the SDK |
| `docs/design.md` | The design & milestone plan |
