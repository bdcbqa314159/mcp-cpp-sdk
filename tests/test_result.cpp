#include <gtest/gtest.h>
#include <optional>
#include <mcp/result.hpp>

using mcp::Error;
using mcp::Result;

TEST(Result, HoldsSuccessValue) {
  Result<int> r = 42;
  EXPECT_TRUE(r.ok());
  EXPECT_EQ(r.value(), 42);
}

TEST(Result, HoldsError) {
  Result<int> r = Error{-32601, "Method not found", std::nullopt};
  EXPECT_FALSE(r.ok());
  EXPECT_EQ(r.error().code, -32601);
  EXPECT_EQ(r.error().message, "Method not found");
}

// error-as-value in action: return success OR failure from one function.
static Result<int> onlyPositive(int n) {
  if (n > 0)
    return n;                                            // success (T ctor)
  return Error{-1, "must be positive", std::nullopt};    // failure (Error ctor)
}

TEST(Result, UsedAsReturnType) {
  EXPECT_TRUE(onlyPositive(5).ok());
  EXPECT_EQ(onlyPositive(5).value(), 5);
  EXPECT_FALSE(onlyPositive(-5).ok());
  EXPECT_EQ(onlyPositive(-5).error().code, -1);
}
