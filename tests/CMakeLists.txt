add_executable(mcp_tests test_smoke.cpp test_json_rpc.cpp test_result.cpp)
target_link_libraries(mcp_tests PRIVATE mcp GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(mcp_tests)
