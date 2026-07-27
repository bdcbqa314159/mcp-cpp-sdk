#include <mcp/transport.hpp>

namespace mcp {

// TODO (task 5): read one message.
//   getline a line from in_. If getline succeeds, return that line.
//   On EOF / read failure, return std::nullopt.
//   Hint: `std::getline(in_, line)` returns the stream, which is falsy at EOF —
//   so `if (std::getline(in_, line)) return line;` then fall through to nullopt.
std::optional<std::string> StdioTransport::read() {
  std::string line{};
  if (std::getline(in_, line))
    return line;
  return std::nullopt;
}

// TODO (task 5): write one message.
//   Send `message`, then a newline, then FLUSH:  out_ << message << "\n" << std::flush;
//   (stdout-is-sacred + flush-every-reply, straight from rung 1.)
void StdioTransport::write(std::string_view message) {
  out_ << message << "\n" << std::flush;
}

} // namespace mcp
