#include <set>
#include <vector>
#include <iostream>
#include <cassert>

#include <GLFW/glfw3.h>

#include "input.hpp"
#include "tetris.hpp"
#include "client.hpp"

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    switch (key) {
    case GLFW_KEY_LEFT:
    case GLFW_KEY_J:
      client::input(tetris::event::left);
      break;
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_L:
      client::input(tetris::event::right);
      break;
    case GLFW_KEY_DOWN:
    case GLFW_KEY_K:
      client::input(tetris::event::down);
      break;
    case GLFW_KEY_SPACE:
      client::input(tetris::event::drop);
      break;
    case GLFW_KEY_S:
      client::input(tetris::event::spin_ccw);
      break;
    case GLFW_KEY_F:
      client::input(tetris::event::spin_cw);
      break;
    case GLFW_KEY_E:
      client::input(tetris::event::spin_180);
      break;
    case GLFW_KEY_D:
      client::input(tetris::event::swap);
      break;
    default:
      break;
    }
  }
}

static std::set<int> present;

struct state {
  unsigned char buttons[15];
};

static state s[GLFW_JOYSTICK_LAST];

void handle_button(int button) {
  switch (button) {
  case GLFW_GAMEPAD_BUTTON_A: // X
    client::input(tetris::event::drop);
    break;
  case GLFW_GAMEPAD_BUTTON_B: // O
    client::input(tetris::event::spin_cw);
    break;
  case GLFW_GAMEPAD_BUTTON_X: // square
    client::input(tetris::event::spin_ccw);
    break;
  case GLFW_GAMEPAD_BUTTON_Y: // triangle
    client::input(tetris::event::spin_180);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_UP:
    client::input(tetris::event::swap);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:
    client::input(tetris::event::right);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:
    client::input(tetris::event::down);
    break;
  case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:
    client::input(tetris::event::left);
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

void input::init(GLFWwindow *window) {
  present.clear();
  for (int jid = GLFW_JOYSTICK_1; jid < GLFW_JOYSTICK_LAST; jid++) {
    if (glfwJoystickPresent(jid)) {
      reset_buttons(jid);
      present.insert(jid);
    }
  }
  glfwSetJoystickCallback(joystick_callback);
  glfwSetKeyCallback(window, key_callback);
}
