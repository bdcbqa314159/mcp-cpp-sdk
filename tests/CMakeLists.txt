add_executable(mcp_tests test_smoke.cpp test_json_rpc.cpp test_result.cpp test_transport.cpp test_dispatcher.cpp test_serve.cpp test_session.cpp test_lifecycle.cpp test_tool.cpp test_tools_serve.cpp test_typed.cpp)
target_link_libraries(mcp_tests PRIVATE mcp GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(mcp_tests)
