// The typed-tool layer is header-only (all templates). This .cpp exists only to
// give clangd a same-named translation unit for include/mcp/typed.hpp (the same
// trick as result.cpp), so the header's flags resolve in the editor.
#include <mcp/typed.hpp>

namespace mcp {
namespace {
[[maybe_unused]] constexpr int typed_tu_anchor = 0;
}
}  // namespace mcp
