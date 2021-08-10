#include <set>
#include <vector>
#include <iostream>
#include <cassert>

#include <GLFW/glfw3.h>

#include "input.hpp"
#include "tetris.hpp"

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    tetris::input(tetris::EVENT_LEFT);
  else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    tetris::input(tetris::EVENT_RIGHT);
  else if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    tetris::input(tetris::EVENT_SPIN_RIGHT);
  else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    tetris::input(tetris::EVENT_DOWN);
  else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    tetris::input(tetris::EVENT_DROP);
  else if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
    tetris::input(tetris::EVENT_SWAP);
}

static std::set<int> present;

struct state {
  unsigned char buttons[15];
};

static state s[GLFW_JOYSTICK_LAST];

void handle_button(int button) {
  switch (button) {
  case GLFW_GAMEPAD_BUTTON_A: // X
    tetris::input(tetris::EVENT_DROP);
    break;
  case GLFW_GAMEPAD_BUTTON_B: // O
    tetris::input(tetris::EVENT_SPIN_RIGHT);
    break;
  case GLFW_GAMEPAD_BUTTON_X: // square
    tetris::input(tetris::EVENT_SPIN_LEFT);
    break;
  case GLFW_GAMEPAD_BUTTON_Y: // triangle
    tetris::input(tetris::EVENT_SPIN_180);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_UP:
    tetris::input(tetris::EVENT_SWAP);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:
    tetris::input(tetris::EVENT_RIGHT);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:
    tetris::input(tetris::EVENT_DOWN);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:
    tetris::input(tetris::EVENT_LEFT);
    break;
  default:
    break;
  }
}

void input::poll_gamepads() {
  int ret;
  std::set<int>::iterator it;
  int jid;
  GLFWgamepadstate gamepad;
  for (it = present.begin(); it != present.end(); it++) {
    jid = *it;
    if (glfwGetGamepadState(jid, &gamepad))
      for (int button = 0; button < 15; button++) {
        if (gamepad.buttons[button] == GLFW_PRESS && s[jid].buttons[button] != GLFW_PRESS) {
          //std::cerr << jid << " press " << button << '\n';
          handle_button(button);
        }
        s[jid].buttons[button] = gamepad.buttons[button];
      }
  }
}

void reset_buttons(int jid) {
  for (int button = 0; button < 15; button++) {
    s[jid].buttons[button] = GLFW_RELEASE;
  }
}

void joystick_callback(int jid, int event) {
  assert(jid < GLFW_JOYSTICK_LAST);
  if (event == GLFW_CONNECTED) {
    std::cerr << "joystick connected " << jid << '\n';
    if (glfwJoystickIsGamepad(jid)) {
      reset_buttons(jid);
      present.insert(jid);
    } else
      std::cerr << "not a gamepad " << jid << '\n';
  } else if (event == GLFW_DISCONNECTED)
    present.erase(jid);
}

void input::init_presence() {
  present.clear();
  for (int jid = GLFW_JOYSTICK_1; jid < GLFW_JOYSTICK_LAST; jid++) {
    if (glfwJoystickPresent(jid)) {
      reset_buttons(jid);
      present.insert(jid);
    }
  }
  glfwSetJoystickCallback(joystick_callback);
}
