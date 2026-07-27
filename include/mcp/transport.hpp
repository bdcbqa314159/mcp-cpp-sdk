#pragma once
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace mcp {

// A transport moves whole JSON-RPC *messages* — framing (where one message ends)
// is handled inside. It's an ABSTRACT INTERFACE: pure virtual functions, no data.
// Keeping transport behind an interface is what lets a different one (HTTP, later)
// drop in without touching the layers above it.
class ITransport {
public:
  virtual ~ITransport() = default;  // virtual dtor: deleting via ITransport* is safe

  // Next incoming message, or std::nullopt when the input is closed (EOF).
  virtual std::optional<std::string> read() = 0;   // "= 0" == pure virtual

  // Send one message. The transport adds framing (a newline) and flushes.
  virtual void write(std::string_view message) = 0;
};

// The stdio transport: newline-delimited JSON on two streams. The streams are
// INJECTED (defaulting to std::cin/std::cout) so tests can drive it with in-memory
// buffers instead of real stdin/stdout.
class StdioTransport : public ITransport {
public:
  StdioTransport(std::istream& in = std::cin, std::ostream& out = std::cout)
      : in_(in), out_(out) {}

  std::optional<std::string> read() override;   // override: we implement the interface
  void write(std::string_view message) override;

private:
  std::istream& in_;
  std::ostream& out_;
};

}  // namespace mcp
