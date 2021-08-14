#include <glm/glm.hpp>

#include "tetris.hpp"

glm::vec3 cellColors[tetris::tet::last_color] = {
  [tetris::tet::z] = glm::vec3(0.937f, 0.125f, 0.161f),
  [tetris::tet::l] = glm::vec3(0.937f, 0.475f, 0.129f),
  [tetris::tet::o] = glm::vec3(0.969f, 0.827f, 0.031f),
  [tetris::tet::s] = glm::vec3(0.259f, 0.714f, 0.259f),
  [tetris::tet::i] = glm::vec3(0.192f, 0.780f, 0.937f),
  [tetris::tet::j] = glm::vec3(0.353f, 0.396f, 0.678f),
  [tetris::tet::t] = glm::vec3(0.678f, 0.302f, 0.612f),
  [tetris::tet::last] = glm::vec3(0.3f, 0.3f, 0.3f),
};
