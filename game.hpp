#include <glm/glm.hpp>

#include "tetris.hpp"

glm::vec4 cellColors[static_cast<int>(tetris::tet::last_color)] = {
  [static_cast<int>(tetris::tet::z)] = glm::vec4(0.937f, 0.125f, 0.161f, 1.0f),
  [static_cast<int>(tetris::tet::l)] = glm::vec4(0.937f, 0.475f, 0.129f, 1.0f),
  [static_cast<int>(tetris::tet::o)] = glm::vec4(0.969f, 0.827f, 0.031f, 1.0f),
  [static_cast<int>(tetris::tet::s)] = glm::vec4(0.259f, 0.714f, 0.259f, 1.0f),
  [static_cast<int>(tetris::tet::i)] = glm::vec4(0.192f, 0.780f, 0.937f, 1.0f),
  [static_cast<int>(tetris::tet::j)] = glm::vec4(0.353f, 0.396f, 0.678f, 1.0f),
  [static_cast<int>(tetris::tet::t)] = glm::vec4(0.678f, 0.302f, 0.612f, 1.0f),
  [static_cast<int>(tetris::tet::empty)] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
  [static_cast<int>(tetris::tet::last)] = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f),
};
