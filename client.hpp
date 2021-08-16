#pragma once

#include "tetris.hpp"

namespace client {
  void input(tetris::event ev);
  void tick();
  void init();
}
