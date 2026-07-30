cmake_minimum_required(VERSION 3.24)  # 3.24+ makes CMP0135 default NEW: correct
                                      # timestamps on FetchContent URL downloads
project(mcp_cpp_sdk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MCP_SANITIZE "Enable Address + UB sanitizers (Clang/GCC)" OFF)
if(MCP_SANITIZE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# nlohmann/json (header-only, public dependency). Prefer an installed package so
# that `find_package(mcp)` consumers resolve the same target via find_dependency;
# fall back to fetching the release tarball for a zero-setup local build.
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# ---- The SDK library --------------------------------------------------------
add_library(mcp STATIC
  src/json_rpc.cpp src/result.cpp src/transport.cpp src/dispatcher.cpp
  src/serve.cpp src/session.cpp src/tool.cpp src/typed.cpp src/server.cpp
  src/logger.cpp)
add_library(mcp::mcp ALIAS mcp)   # consumers link mcp::mcp (namespaced convention)

target_compile_features(mcp PUBLIC cxx_std_20)
target_include_directories(mcp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(mcp PUBLIC nlohmann_json::nlohmann_json)

# Examples, sandbox, and tests are for developing the SDK itself — build them only
# when this IS the top-level project (so a consumer fetching us doesn't build them).
option(MCP_BUILD_EXAMPLES "Build the example servers" ${PROJECT_IS_TOP_LEVEL})
option(MCP_BUILD_SANDBOX  "Build the learning sandbox rungs" ${PROJECT_IS_TOP_LEVEL})
option(MCP_BUILD_TESTS    "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})

if(MCP_BUILD_EXAMPLES)
  add_executable(add_server examples/add_server.cpp)
  target_link_libraries(add_server PRIVATE mcp)
  add_executable(echo_server examples/echo_server.cpp)
  target_link_libraries(echo_server PRIVATE mcp)
endif()

if(MCP_BUILD_SANDBOX)
  file(GLOB SANDBOX_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/rung*.cpp)
  foreach(src ${SANDBOX_SRCS})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE nlohmann_json::nlohmann_json)
  endforeach()
endif()

if(MCP_BUILD_TESTS)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_subdirectory(tests)

  # Black-box harness: spawn echo_server, round-trip a request over real stdio.
  # POSIX only (fork/exec/pipe); skipped on Windows.
  if(NOT WIN32)
    add_executable(mcp_probe tools/mcp_probe.cpp)
    target_link_libraries(mcp_probe PRIVATE mcp)
    add_test(NAME probe_echo COMMAND mcp_probe $<TARGET_FILE:echo_server>)
  endif()
endif()

# ---- Install / find_package(mcp) --------------------------------------------
# Lets a consumer `find_package(mcp)` and link `mcp::mcp`. Only wired up when
# nlohmann_json came from find_package (an imported target the export can reference,
# and that the consumer can re-find via find_dependency). If nlohmann was fetched it
# isn't installable, so we skip the install rules — FetchContent/add_subdirectory
# consumers don't need them anyway.
if(NOT nlohmann_json_FOUND)
  message(STATUS
    "mcp: nlohmann_json was fetched (not a system package) — skipping install/export. "
    "For `find_package(mcp)`, configure with a discoverable nlohmann_json "
    "(e.g. -DCMAKE_PREFIX_PATH=/path/to/nlohmann).")
  return()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS mcp EXPORT mcpTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/mcp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT mcpTargets
  NAMESPACE mcp::
  FILE mcpTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcp)

configure_package_config_file(cmake/mcpConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/mcpConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcp)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/mcpConfigVersion.cmake
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/mcpConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/mcpConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcp)
