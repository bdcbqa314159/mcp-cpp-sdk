cmake_minimum_required(VERSION 3.24)  # 3.24+ makes CMP0135 default NEW: correct
                                      # timestamps on FetchContent URL downloads
project(mcp_cpp_sdk CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MCP_SANITIZE "Enable Address + UB sanitizers (Clang/GCC)" OFF)
if(MCP_SANITIZE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Pull nlohmann/json as a header-only dep. URL of the prebuilt release tarball,
# not a git clone — the repo is huge, the tarball is tiny.
include(FetchContent)
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(nlohmann_json)

add_library(mcp STATIC src/json_rpc.cpp src/result.cpp)
target_include_directories(mcp PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mcp PUBLIC nlohmann_json::nlohmann_json)

# Examples that use the SDK.
add_executable(add_server examples/add_server.cpp)
target_link_libraries(add_server PRIVATE mcp)

file(GLOB SANDBOX_SRCS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/sandbox/rung*.cpp)
foreach(src ${SANDBOX_SRCS})
  get_filename_component(name ${src} NAME_WE)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE nlohmann_json::nlohmann_json)
endforeach()

# ---- Tests (GoogleTest via FetchContent) ------------------------------------
option(MCP_BUILD_TESTS "Build unit tests with GoogleTest" ON)
if(MCP_BUILD_TESTS)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)

  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_subdirectory(tests)
endif()
