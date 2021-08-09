#pragma once

namespace tetris {
  enum type {
    TET_Z,
    TET_L,
    TET_O,
    TET_S,
    TET_I,
    TET_J,
    TET_T,
    TET_LAST
  };

  enum dir {
    DIR_0,
    DIR_1,
    DIR_2,
    DIR_3,
    DIR_LAST
  };

  enum event {
    EVENT_LEFT,
    EVENT_RIGHT,
    EVENT_DOWN,
    EVENT_DROP,
    EVENT_SPIN,
    EVENT_SWAP
  };

  struct cell {
    tetris::type color;
    bool empty;
  };

  struct coord {
    int u;
    int v;
  };

  struct tetronimo {
    tetris::type type;
    tetris::coord pos;
    tetris::dir facing;
  };


  extern cell board[10][20];

  extern coord offsets[TET_LAST][4][4];

  extern tetronimo piece;
  extern std::vector<tetris::type> queue;
  extern tetris::type swap;

  void initBoard();

  void tick();

  void input(event ev);

  void shiftDown();
}
