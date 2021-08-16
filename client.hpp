#pragma once

#include "tetris.hpp"

namespace client {
  void init();

  void event_field_state(tetris::field& field, tetris::side_t side);
}
