#pragma once

#include <vector>
#include <array>
#include <unordered_set>
#include <mutex>

namespace tetris {
  enum tet {
    z,
    l,
    o,
    s,
    i,
    j,
    t,
    empty,
    last,
    last_color
  };

  enum side_t {
    zero = 0,
    one = 1,
    none
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
    EVENT_SPIN_LEFT,
    EVENT_SPIN_RIGHT,
    EVENT_SPIN_180,
    EVENT_SWAP
  };

  struct cell {
    tetris::tet color;
  };

  struct coord {
    int u; // uint8_t
    int v; // uint8_t
  };

  struct piece {
    tetris::tet tet;
    tetris::coord pos;
    tetris::dir facing;
    bool swapped;
    int drop_row;
  };

  extern coord offsets[tetris::tet::last][4][4];

  constexpr int rows = 40;
  constexpr int columns = 10;
  typedef std::array<std::array<cell, rows>, columns> field;
  typedef std::unordered_set<tetris::tet> bag;
  typedef std::vector<tetris::tet> queue;

  struct frame {
    tetris::field field;
    tetris::bag bag;
    tetris::queue queue;
    tetris::piece piece;
    tetris::tet swap;
    //std::mutex lock;
  };

  constexpr int frame_count = 2;

  extern std::array<frame, frame_count> frames;
  extern tetris::side_t this_side;

  void event_reset_frame(tetris::side_t side);

  void init();
}
