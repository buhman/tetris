#pragma once

namespace tetris {
  enum color {
    YELLOW,
    CYAN,
    MAGENTA,
    COLOR_LAST
  };

  struct cell {
    tetris::color color;
  };

  extern cell board[10][20];

  void initBoard();
}
