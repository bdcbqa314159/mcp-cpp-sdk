// mcp_probe — a black-box harness. Spawns an MCP server binary as a subprocess and
// drives the full lifecycle over stdio (exactly like a real client): initialize ->
// notifications/initialized -> a real method call. Checks the final response.
// Exit 0 = pass. POSIX only (fork/exec/pipe); the CMake target is gated to non-Windows.
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

using json = nlohmann::json;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: mcp_probe <server-binary>\n";
    return 2;
  }
  const char* server = argv[1];

  int to_child[2];    // parent writes -> child stdin
  int from_child[2];  // child stdout -> parent reads
  if (pipe(to_child) != 0 || pipe(from_child) != 0) {
    std::perror("pipe");
    return 2;
  }

  const pid_t pid = fork();
  if (pid < 0) {
    std::perror("fork");
    return 2;
  }

  if (pid == 0) {
    dup2(to_child[0], STDIN_FILENO);
    dup2(from_child[1], STDOUT_FILENO);
    close(to_child[0]);
    close(to_child[1]);
    close(from_child[0]);
    close(from_child[1]);
    execl(server, server, static_cast<char*>(nullptr));
    std::perror("execl");
    _exit(127);
  }

  close(to_child[0]);
  close(from_child[1]);

  // Full handshake, then an echo call. `echo` only succeeds if the server
  // completed the lifecycle (otherwise it would answer -32002).
  const std::string requests =
      R"({"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2025-06-18"}})"
      "\n"
      R"({"jsonrpc":"2.0","method":"notifications/initialized"})"
      "\n"
      R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"x":42}})"
      "\n";
  if (write(to_child[1], requests.data(), requests.size()) < 0) std::perror("write");
  close(to_child[1]);

  std::string out;
  char buf[512];
  ssize_t n = 0;
  while ((n = read(from_child[0], buf, sizeof buf)) > 0)
    out.append(buf, static_cast<size_t>(n));
  close(from_child[0]);
  waitpid(pid, nullptr, 0);

  // Find the echo response (id == 1) and check its result.
  std::istringstream lines(out);
  std::string line;
  while (std::getline(lines, line)) {
    if (line.empty()) continue;
    try {
      const json resp = json::parse(line);
      if (resp.contains("id") && resp.at("id") == 1) {
        if (resp.contains("result") && resp.at("result") == json{{"x", 42}}) {
          std::cout << "PROBE OK: " << line << "\n";
          return 0;
        }
        std::cerr << "PROBE FAIL (bad echo response): " << line << "\n";
        return 1;
      }
    } catch (const std::exception& e) {
      std::cerr << "PROBE FAIL (parse): " << e.what() << " on [" << line << "]\n";
      return 1;
    }
  }
  std::cerr << "PROBE FAIL (no echo response). Full output:\n" << out << "\n";
  return 1;
}
