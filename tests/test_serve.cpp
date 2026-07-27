#include <gtest/gtest.h>
#include <sstream>
#include <mcp/serve.hpp>

using mcp::Dispatcher;
using mcp::json;
using mcp::Result;
using mcp::StdioTransport;

TEST(Serve, DispatchesRequestAndWritesResponse) {
  std::istringstream in(
      R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"x":42}})"
      "\n");
  std::ostringstream out;
  StdioTransport transport(in, out);

  Dispatcher d;
  d.on("echo", [](const json& p) -> Result<json> { return p; });  // echo params

  mcp::serve(transport, d);  // runs until EOF (one line here)

  json resp = json::parse(out.str());
  EXPECT_EQ(resp.at("id"), 1);
  EXPECT_EQ(resp.at("result"), (json{{"x", 42}}));
}

TEST(Serve, NotificationProducesNoOutput) {
  std::istringstream in(R"({"jsonrpc":"2.0","method":"echo","params":{}})"
                        "\n");  // no id => notification
  std::ostringstream out;
  StdioTransport transport(in, out);

  Dispatcher d;
  d.on("echo", [](const json& p) -> Result<json> { return p; });

  mcp::serve(transport, d);
  EXPECT_TRUE(out.str().empty());  // notification -> nothing written
}

TEST(Serve, ParseErrorReturnsMinus32700) {
  std::istringstream in("not json{\n");
  std::ostringstream out;
  StdioTransport transport(in, out);

  Dispatcher d;
  mcp::serve(transport, d);

  json resp = json::parse(out.str());
  EXPECT_EQ(resp.at("error").at("code"), -32700);
  EXPECT_TRUE(resp.at("id").is_null());  // parse error -> null id
}
