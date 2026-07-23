#include <iostream>
#include <string>

int main() {

  std::string line{};
  while (std::getline(std::cin, line)) {
    std::cout << "got: " << line << "\n" << std::flush;
    std::cerr << "[server received] " << line << "\n";
  }

  return 0;
}
