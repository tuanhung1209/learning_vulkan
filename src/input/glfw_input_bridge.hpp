#pragma once

#include "input/input_state.hpp"
#include <GLFW/glfw3.h>
#include <vulkan_core/window.hpp>

namespace my {

class GlfwInput {
  public:
    GlfwInput(Window &glfwWindow, InputState &inputState);
    ~GlfwInput() {};

    void pollKeyboardFromGlfw(Window &glfwWindow);
    void pollMouseFromGlfw(Window &glfwWindow);

  private:
    std::array<int, (size_t)InputState::Key::Count> toGlfw = {
        GLFW_KEY_W,        // Key::W
        GLFW_KEY_A,        // Key::A
        GLFW_KEY_S,        // Key::S
        GLFW_KEY_D,        // Key::D
        GLFW_KEY_Q,        // Key::Q
        GLFW_KEY_E,        // Key::E
        GLFW_KEY_UP,       // Key::Up
        GLFW_KEY_DOWN,     // Key::Down
        GLFW_KEY_LEFT,     // Key::Left
        GLFW_KEY_RIGHT,    // Key::Right
        GLFW_KEY_SPACE,    // Key::SPACE
        GLFW_KEY_LEFT_ALT, // Key::ALT
    };

    Window &window;
    InputState &inputState;
    glm::vec2 lastCursorPos{0.0f};
    bool cursorVisible = true;
};

} // namespace my
