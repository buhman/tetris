#pragma once

#include <vector>
#include <array>
#include <unordered_set>
#include <chrono>
#include <deque>

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

  using clock = std::chrono::high_resolution_clock;
  using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
  using duration = std::chrono::duration<float, std::chrono::seconds::period>;

  struct piece {
    tetris::tet tet;
    tetris::dir facing;
    tetris::coord pos;
    int drop_row;
    struct {
      int moves;
      time_point point;
      bool locking;
    } lock_delay;
  };

  extern coord offsets[static_cast<int>(tetris::tet::last)][4][4];

  constexpr int rows = 40;
  constexpr int columns = 10;
  typedef std::array<std::array<cell, rows>, columns> field;
  typedef std::unordered_set<tetris::tet> bag;
  typedef std::vector<tetris::tet> queue;

  struct attack_t {
    int rows;
    int column;
  };

  struct garbage_t {
    int total;
    std::deque<attack_t> attacks;
  };

  struct frame {
    tetris::field field;
    tetris::bag bag;
    tetris::queue queue;
    tetris::piece piece;
    tetris::tet swap;
    bool swapped;
    int points;
    int level;
    time_point point;
    garbage_t garbage;
  };

  constexpr int frame_count = 2;

  extern std::array<frame, frame_count> frames;
  extern tetris::side_t this_side;

  void event_reset_frame(tetris::side_t side);

  void swap();
  int place(tetris::frame& frame);
  void drop();
  void next_piece();
  bool lock_delay(tetris::piece& piece);
  bool move(tetris::coord offset, int rotation);
  bool gravity(tetris::frame& frame);
  void _garbage(tetris::field& field, tetris::garbage_t& garbage);
  void attack(tetris::frame& frame, tetris::attack_t& attack);
  void init();
}
