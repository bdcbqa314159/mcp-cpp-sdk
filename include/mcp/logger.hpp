#pragma once
#include <iostream>
#include <string_view>

namespace mcp {

// Severity, low to high. The ORDER matters: a Logger emits a message only if its
// level is at or above the logger's minimum, and that comparison relies on this
// ordering (Debug < Info < Warn < Error).
enum class LogLevel { Debug, Info, Warn, Error };

// A tiny leveled logger. Writes to stderr by default — NEVER stdout, which carries
// the JSON-RPC protocol stream. The output stream is injectable so tests can
// capture it (same dependency-injection idea as StdioTransport).
class Logger {
public:
  explicit Logger(LogLevel min = LogLevel::Info, std::ostream& out = std::cerr)
      : min_(min), out_(out) {}

  void log(LogLevel level, std::string_view message);

  void debug(std::string_view m) { log(LogLevel::Debug, m); }
  void info(std::string_view m) { log(LogLevel::Info, m); }
  void warn(std::string_view m) { log(LogLevel::Warn, m); }
  void error(std::string_view m) { log(LogLevel::Error, m); }

private:
  LogLevel min_;
  std::ostream& out_;
};

}  // namespace mcp
