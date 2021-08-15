#pragma once

#include "tetris.hpp"
#include "message.hpp"

namespace client {
  void init();

  void event_field_state(tetris::field& field, message::side_t side);
}
