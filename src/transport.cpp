#include <mcp/transport.hpp>

namespace mcp {

// Read one message (one line). std::nullopt signals end of input (EOF).
std::optional<std::string> StdioTransport::read() {
  std::string line{};
  if (std::getline(in_, line))
    return line;
  return std::nullopt;
}

// Write one message, add the framing newline, and flush (a reply must not sit in
// a buffer while the client waits).
void StdioTransport::write(std::string_view message) {
  out_ << message << "\n" << std::flush;
}

} // namespace mcp
