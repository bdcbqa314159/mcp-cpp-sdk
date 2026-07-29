// Smoke test: the SDK's top-level header compiles and nlohmann round-trips.
#include <gtest/gtest.h>
#include <mcp/server.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TEST(Json, RoundTrip) {
  json j = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "ping"}};
  EXPECT_EQ(json::parse(j.dump()), j);
}
