#include <glm/glm.hpp>

#include "tetris.hpp"

glm::vec3 cellColors[static_cast<int>(tetris::tet::last_color)] = {
  [static_cast<int>(tetris::tet::z)] = glm::vec3(0.937f, 0.125f, 0.161f),
  [static_cast<int>(tetris::tet::l)] = glm::vec3(0.937f, 0.475f, 0.129f),
  [static_cast<int>(tetris::tet::o)] = glm::vec3(0.969f, 0.827f, 0.031f),
  [static_cast<int>(tetris::tet::s)] = glm::vec3(0.259f, 0.714f, 0.259f),
  [static_cast<int>(tetris::tet::i)] = glm::vec3(0.192f, 0.780f, 0.937f),
  [static_cast<int>(tetris::tet::j)] = glm::vec3(0.353f, 0.396f, 0.678f),
  [static_cast<int>(tetris::tet::t)] = glm::vec3(0.678f, 0.302f, 0.612f),
  [static_cast<int>(tetris::tet::empty)] = glm::vec3(0.0f, 0.0f, 0.0f),
  [static_cast<int>(tetris::tet::last)] = glm::vec3(0.3f, 0.3f, 0.3f),
};
