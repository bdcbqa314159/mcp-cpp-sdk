#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

// The one gotcha: in nlohmann, {{"ok", true}} is an object {"ok":true}, but
// {1,2,3} is an array. The doubled braces are how you say "these are key/value
// pairs." Trip on this once and never again.

// Your task (rung 2 routing)

// Fill the three TODOs in sandbox/rung2.cpp:
// 1. Parse the line into req.
// 2. Route on req["method"]: if it's "ping", result is {"ok":true}; otherwise
// echo the method back as {"echoed": <method>}. Always set jsonrpc, id, and
// result.
// 3. Reply with reply.dump(), flushed.

// Test when built:
// cmake --build build
// echo '{"jsonrpc":"2.0","id":1,"method":"ping"}'  | ./build/rung2
// # expect: {"id":1,"jsonrpc":"2.0","result":{"ok":true}}
// echo '{"jsonrpc":"2.0","id":2,"method":"hello"}' | ./build/rung2
// # expect: {"echoed":"hello","id":2,"jsonrpc":"2.0"}
// (nlohmann sorts object keys alphabetically on dump — that's why the order
// looks shuffled. Harmless; the client parses by key, not position.)

using json =
    nlohmann::json; // now `json` behaves like a dict/list you can index

int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty())
      continue; // skip blank lines, don't try to parse them

    // --- 1. PARSE ---------------------------------------------------------
    // Turn the incoming line (a JSON string) into a json object.
    // Look up: json::parse(...). What could go wrong if the line is garbage?
    // (We'll handle parse errors properly at rung 3 — for now assume valid.)
    // TODO: json req = ...

    json req = json::parse(line);

    // --- 2. ROUTE ---------------------------------------------------------
    // Read req["method"] (a string) and req["id"] (copy it into the reply).

    std::string m = req["method"];
    auto id = req["id"];

    json reply;

    reply["jsonrpc"] = "2.0";
    reply["id"] = id;

    if (m == "ping") {
      reply["result"] = {{"ok", true}};
    } else {
      reply["result"] = {{"echoed", m}};
    }

    std::string out = reply.dump();

    std::cout << out << "\n" << std::flush;

    // Switch behaviour on the method:
    //   - "ping"  -> result should be {"ok": true}
    //   - anything else -> for now, echo the method back, e.g. {"echoed":
    //   <method>}
    // TODO: build a `json reply;` with the right shape (see the spec below).

    // --- 3. REPLY ---------------------------------------------------------
    // Serialise the reply json to a string and write it to stdout, one line,
    // FLUSHED (rung-1 gotcha #2 still applies). json has .dump() for this.
    // TODO: std::cout << reply.dump() << std::endl;   // endl = "\n" + flush
  }
  return 0;
}
