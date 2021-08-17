#pragma once

#include <vector>
#include <array>
#include <unordered_set>

namespace tetris {
  enum class tet {
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

  enum class side_t {
    zero = 0,
    one = 1,
    none
  };

  enum class dir {
    up,
    right,
    down,
    left,
    last
  };

  enum class event {
    left,
    right,
    down,
    drop,
    spin_cw,
    spin_ccw,
    spin_180,
    swap
  };

  struct cell {
    tetris::tet color;
  };

  struct coord {
    int u;
    int v;
  };

  struct piece {
    tetris::tet tet;
    tetris::dir facing;
    tetris::coord pos;
    int drop_row;
  };

  extern coord offsets[static_cast<int>(tetris::tet::last)][4][4];

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
    bool swapped;
  };

  constexpr int frame_count = 2;

  extern std::array<frame, frame_count> frames;
  extern tetris::side_t this_side;

  void event_reset_frame(tetris::side_t side);

  void swap();
  void _place(tetris::field& field, tetris::piece& piece);
  void drop();
  void next_piece();
  bool move(tetris::coord offset, int rotation);
  void init();
}
