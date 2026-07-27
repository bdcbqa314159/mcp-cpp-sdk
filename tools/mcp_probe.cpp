// mcp_probe — a black-box harness. Spawns an MCP server binary as a subprocess,
// sends one JSON-RPC request over stdio (exactly like a real client), reads the
// response, and checks it. Exit 0 = pass, non-zero = fail.
//
// POSIX only (fork/exec/pipe). Windows would need CreateProcess + anonymous pipes;
// the CMake target is gated to non-Windows for now.
#include <nlohmann/json.hpp>

#include <cstring>
#include <iostream>
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
    // Child: rewire the pipes onto stdin/stdout, then become the server.
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

  // Parent: close the ends we don't use.
  close(to_child[0]);
  close(from_child[1]);

  // Send one request, then EOF so the server's read loop ends.
  const std::string request =
      R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"x":42}})"
      "\n";
  if (write(to_child[1], request.data(), request.size()) < 0) std::perror("write");
  close(to_child[1]);

  // Drain the server's stdout.
  std::string out;
  char buf[512];
  ssize_t n;
  while ((n = read(from_child[0], buf, sizeof buf)) > 0) out.append(buf, static_cast<size_t>(n));
  close(from_child[0]);
  waitpid(pid, nullptr, 0);

  // Check the response.
  try {
    const json resp = json::parse(out);
    if (resp.at("id") == 1 && resp.at("result") == json{{"x", 42}}) {
      std::cout << "PROBE OK: " << out;
      return 0;
    }
    std::cerr << "PROBE FAIL (unexpected response): " << out << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "PROBE FAIL (" << e.what() << "), raw output: [" << out << "]\n";
    return 1;
  }
}
