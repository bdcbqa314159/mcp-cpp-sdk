// The Server facade is header-only (a class template method). This .cpp is the
// same-named translation unit that keeps clangd's flags for include/mcp/server.hpp
// resolving (same trick as result.cpp / typed.cpp).
#include <mcp/server.hpp>

namespace mcp {
namespace {
[[maybe_unused]] constexpr int server_tu_anchor = 0;
}
}  // namespace mcp
