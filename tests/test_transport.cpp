#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <mcp/transport.hpp>

using mcp::ITransport;
using mcp::StdioTransport;

TEST(StdioTransport, ReadsLinesThenEof) {
  std::istringstream in("hello\nworld\n");
  std::ostringstream out;
  StdioTransport t(in, out);

  EXPECT_EQ(t.read(), "hello");
  EXPECT_EQ(t.read(), "world");
  EXPECT_FALSE(t.read().has_value());   // input exhausted -> nullopt
}

TEST(StdioTransport, WritesMessageWithNewline) {
  std::istringstream in;
  std::ostringstream out;
  StdioTransport t(in, out);

  t.write("pong");
  EXPECT_EQ(out.str(), "pong\n");       // framing newline added
}

// The point of the interface: use it polymorphically through a base pointer,
// owned by a unique_ptr (single owner — no shared_ptr).
TEST(Transport, UsableThroughBasePointer) {
  std::istringstream in("x\n");
  std::ostringstream out;
  std::unique_ptr<ITransport> t = std::make_unique<StdioTransport>(in, out);

  EXPECT_EQ(t->read(), "x");
  t->write("y");
  EXPECT_EQ(out.str(), "y\n");
}
