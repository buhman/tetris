#include <iostream>

#include "tetris.hpp"

using namespace tetris;

cell tetris::board[10][20];

void tetris::initBoard() {
  std::cout << &board << '\n';
  for (int u = 0; u < 10; u++) {
    for (int v = 0; v < 20; v++) {
      board[u][v].color = static_cast<color>((u + (10 * v)) % ((int)COLOR_LAST));
    }
  }
}
