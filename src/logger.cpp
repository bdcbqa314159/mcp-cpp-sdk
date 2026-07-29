#include <mcp/logger.hpp>

namespace mcp {

namespace {
// The text label for a level. A switch means the compiler warns if a case is missed.
const char* label(LogLevel level) {
  switch (level) {
  case mcp::LogLevel::Debug:
    return "DEBUG";
  case mcp::LogLevel::Info:
    return "INFO";
  case mcp::LogLevel::Error:
    return "ERROR";
  case mcp::LogLevel::Warn:
    return "WARN";
  }
  return "?";
}
} // namespace

// Write one line to out_ (stderr by default — never stdout), if the level clears
// the logger's minimum threshold.
void Logger::log(LogLevel level, std::string_view message) {
  if (level < min_)
    return;
  out_ << "[" << label(level) << "] " << message << "\n";
}

} // namespace mcp
