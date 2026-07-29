#include <gtest/gtest.h>
#include <string>
#include <mcp/tool.hpp>

using mcp::json;
using mcp::Tool;
using mcp::ToolRegistry;
using mcp::ToolResult;

namespace {
Tool make_tool(const std::string& name) {
  return Tool{name, "description of " + name, json{{"type", "object"}},
              [](const json&) -> ToolResult { return mcp::text("ok"); }};
}
}  // namespace

TEST(Text, MakesTextContentBlock) {
  ToolResult r = mcp::text("hello");
  EXPECT_FALSE(r.isError);
  ASSERT_TRUE(r.content.is_array());
  EXPECT_EQ(r.content.at(0).at("type"), "text");
  EXPECT_EQ(r.content.at(0).at("text"), "hello");
}

TEST(ToolRegistry, AddAndContains) {
  ToolRegistry reg;
  EXPECT_FALSE(reg.contains("add"));
  reg.add(make_tool("add"));
  EXPECT_TRUE(reg.contains("add"));
}

TEST(ToolRegistry, ListAdvertisesRegisteredTools) {
  ToolRegistry reg;
  reg.add(make_tool("add"));
  reg.add(make_tool("multiply"));

  json l = reg.list();
  ASSERT_TRUE(l.contains("tools"));
  EXPECT_EQ(l.at("tools").size(), 2);

  bool found_add = false;
  for (const auto& t : l.at("tools")) {
    if (t.at("name") == "add") {
      found_add = true;
      EXPECT_TRUE(t.contains("description"));
      EXPECT_TRUE(t.contains("inputSchema"));
    }
  }
  EXPECT_TRUE(found_add);
}

TEST(ToolRegistry, CallRunsTheHandler) {
  ToolRegistry reg;
  reg.add(Tool{"greet", "", json::object(), [](const json& a) -> ToolResult {
                 return mcp::text("hi " + a.at("name").get<std::string>());
               }});

  ToolResult r = reg.call("greet", json{{"name", "Ada"}});
  EXPECT_FALSE(r.isError);
  EXPECT_EQ(r.content.at(0).at("text"), "hi Ada");
}

TEST(ToolRegistry, CallUnknownToolIsError) {
  ToolRegistry reg;
  ToolResult r = reg.call("nope", json::object());
  EXPECT_TRUE(r.isError);
}

TEST(ToolRegistry, CallCatchesToolError) {
  ToolRegistry reg;
  reg.add(Tool{"boom", "", json::object(),
               [](const json&) -> ToolResult { throw mcp::ToolError("kaboom"); }});

  ToolResult r = reg.call("boom", json::object());
  EXPECT_TRUE(r.isError);
  EXPECT_EQ(r.content.at(0).at("text"), "kaboom");
}
