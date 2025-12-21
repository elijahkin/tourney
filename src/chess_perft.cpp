#include <cstddef>
#include <iostream>
#include <vector>

#include "games/chess.hpp"

int main() {
  auto game = Chess(/*white_perspective=*/true);

  // https://www.chessprogramming.org/Perft_Results
  const std::vector<size_t> true_perft = {1, 20, 400, 8902, 197281, 4865609};

  for (size_t depth = 0; depth < true_perft.size(); ++depth) {
    const size_t perft = game.Perft(depth);

    if (perft != true_perft[depth]) {
      std::cout << "Perft failed for depth=" << depth << " (perft=" << perft
                << ", true_perft=" << true_perft[depth] << ")" << '\n';
    }
  }
  return 0;
}
