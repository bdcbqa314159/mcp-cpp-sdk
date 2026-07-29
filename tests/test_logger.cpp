#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <mcp/logger.hpp>

using mcp::Logger;
using mcp::LogLevel;

TEST(Logger, DropsMessagesBelowMinLevel) {
  std::ostringstream out;
  Logger log(LogLevel::Info, out);

  log.debug("hidden");      // below Info -> dropped
  log.info("shown");
  log.warn("also shown");

  const std::string s = out.str();
  EXPECT_EQ(s.find("hidden"), std::string::npos);
  EXPECT_NE(s.find("shown"), std::string::npos);
  EXPECT_NE(s.find("also shown"), std::string::npos);
}

TEST(Logger, FormatsWithLevelLabel) {
  std::ostringstream out;
  Logger log(LogLevel::Debug, out);

  log.warn("careful");
  EXPECT_NE(out.str().find("[WARN]"), std::string::npos);
  EXPECT_NE(out.str().find("careful"), std::string::npos);
}

TEST(Logger, ErrorAlwaysPasses) {
  std::ostringstream out;
  Logger log(LogLevel::Error, out);  // highest threshold

  log.warn("dropped");   // below Error
  log.error("boom");
  const std::string s = out.str();
  EXPECT_EQ(s.find("dropped"), std::string::npos);
  EXPECT_NE(s.find("[ERROR]"), std::string::npos);
  EXPECT_NE(s.find("boom"), std::string::npos);
}
