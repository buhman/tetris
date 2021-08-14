#include "tetris.hpp"

#pragma once

namespace message {
  enum type {
    new_piece,
    piece_move,
    piece_place
  };

  struct new_piece {
    tetris::type type;
  };

  struct piece_move {
    tetris::coord coord;
    tetris::dir dir;
  };

  struct piece_place {
    tetris::coord coord;
    tetris::dir dir;
  };
}
