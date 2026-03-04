#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "games/chess.hpp"

const std::string kPassed = "\033[1;32mPASSED\033[0m";
const std::string kFailed = "\033[1;31mFAILED\033[0m";

int main() {
  auto game = Chess(/*white_perspective=*/true);

  // https://www.chessprogramming.org/Perft_Results
  const std::vector<size_t> true_perft = {1, 20, 400, 8902, 197281, 4865609};

  for (size_t depth = 0; depth < true_perft.size(); ++depth) {
    auto start = std::chrono::high_resolution_clock::now();
    const size_t perft = game.Perft(depth);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count();

    bool passed = (perft == true_perft[depth]);
    std::cout << "Perft " << (passed ? kPassed : kFailed)
              << " for depth=" << depth << " in " << duration << "µs";
    if (!passed) {
      std::cout << " (perft=" << perft << ", true_perft=" << true_perft[depth]
                << ")";
    }
    std::cout << '\n';
  }
  return 0;
}
