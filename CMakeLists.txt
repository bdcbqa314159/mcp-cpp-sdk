cmake_minimum_required(VERSION 3.14)  # FetchContent_MakeAvailable needs 3.14+
project(mcp_cpp_sdk CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # emit build/compile_commands.json for clangd

# Pull nlohmann/json as a header-only dep. URL of the prebuilt release tarball,
# not a git clone — the repo is huge, the tarball is tiny.
include(FetchContent)
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(nlohmann_json)

# The SDK is header-only: an interface target that carries the include dir + dep.
add_library(mcp INTERFACE)
target_include_directories(mcp INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mcp INTERFACE nlohmann_json::nlohmann_json)

# Examples that use the SDK.
add_executable(add_server examples/add_server.cpp)
target_link_libraries(add_server PRIVATE mcp)

# ponytail: glob the sandbox so a new sandbox/rung*.cpp builds with no CMake edit.
# CONFIGURE_DEPENDS re-checks the glob at build time, so the usual "globs go stale"
# objection doesn't bite here.
file(GLOB SANDBOX_SRCS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/sandbox/rung*.cpp)
foreach(src ${SANDBOX_SRCS})
  get_filename_component(name ${src} NAME_WE)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE nlohmann_json::nlohmann_json)
endforeach()
