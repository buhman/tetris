#include <glm/glm.hpp>

#include "tetris.hpp"

glm::vec3 cellColors[tetris::COLOR_LAST] = {
  [tetris::YELLOW] = glm::vec3(1.0, 1.0, 0.0),
  [tetris::CYAN] = glm::vec3(0.0, 1.0, 1.0),
  [tetris::MAGENTA] = glm::vec3(1.0, 0.0, 1.0),
};
