#include <glm/glm.hpp>

#include "tetris.hpp"

glm::vec3 cellColors[tetris::TET_LAST] = {
  [tetris::TET_Z] = glm::vec3(0.937f, 0.125f, 0.161f),
  [tetris::TET_L] = glm::vec3(0.937f, 0.475f, 0.129f),
  [tetris::TET_O] = glm::vec3(0.969f, 0.827f, 0.031f),
  [tetris::TET_S] = glm::vec3(0.259f, 0.714f, 0.259f),
  [tetris::TET_I] = glm::vec3(0.192f, 0.780f, 0.937f),
  [tetris::TET_J] = glm::vec3(0.353f, 0.396f, 0.678f),
  [tetris::TET_T] = glm::vec3(0.678f, 0.302f, 0.612f),
};
