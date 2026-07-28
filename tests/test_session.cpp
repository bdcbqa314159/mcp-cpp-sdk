#include <gtest/gtest.h>
#include <mcp/session.hpp>

using mcp::json;
using mcp::Lifecycle;
using mcp::Session;

TEST(Session, StartsUninitialized) {
  Session s({"test-server", "0.1"});
  EXPECT_EQ(s.state(), Lifecycle::Uninitialized);
}

TEST(Session, InitializeReturnsServerInfoAndMovesToInitializing) {
  Session s({"test-server", "0.1"});

  auto r = s.initialize(json{{"protocolVersion", "2025-06-18"}});

  ASSERT_TRUE(r.ok());
  EXPECT_EQ(r.value().at("serverInfo").at("name"), "test-server");
  EXPECT_EQ(r.value().at("serverInfo").at("version"), "0.1");
  EXPECT_TRUE(r.value().at("capabilities").contains("tools"));
  EXPECT_EQ(s.state(), Lifecycle::Initializing);
}

TEST(Session, InitializeTwiceIsAnError) {
  Session s({"test-server", "0.1"});
  s.initialize(json::object());

  auto r = s.initialize(json::object());
  EXPECT_FALSE(r.ok());  // out-of-order -> error, not a crash
}

TEST(Session, InitializedNotificationMovesToReady) {
  Session s({"test-server", "0.1"});
  s.initialize(json::object());
  s.mark_initialized();
  EXPECT_EQ(s.state(), Lifecycle::Ready);
}

TEST(Session, GatesNonLifecycleUntilReady) {
  Session s({"test-server", "0.1"});
  EXPECT_TRUE(s.allows("initialize"));           // lifecycle always allowed
  EXPECT_TRUE(s.allows("ping"));
  EXPECT_TRUE(s.allows("notifications/whatever"));
  EXPECT_FALSE(s.allows("tools/list"));          // not ready yet

  s.initialize(json::object());
  s.mark_initialized();
  EXPECT_TRUE(s.allows("tools/list"));           // now ready
}
