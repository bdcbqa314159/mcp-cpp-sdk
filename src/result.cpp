// Result<T> is a header-only class template (definitions must be visible to every
// instantiation). This .cpp exists mainly so the header has a same-named
// translation unit — that's what lets clangd infer include/mcp/result.hpp's
// compile flags reliably (same trick as json_rpc).
#include <mcp/result.hpp>

namespace mcp {
// Anchor so this TU is not an empty archive member (silences a linker warning on
// some toolchains). No runtime meaning.
namespace {
[[maybe_unused]] constexpr int result_tu_anchor = 0;
}
}  // namespace mcp
