#include "glfw_input_bridge.hpp"
#include "input/input_state.hpp"
#include <GLFW/glfw3.h>

namespace my {

GlfwInput::GlfwInput(GlfwWindow &glfwWindow, InputState &inputState)
    : window{glfwWindow}, inputState(inputState) {};

void GlfwInput::pollKeyboardFromGlfw(GlfwWindow &glfwWindow) {
    for (size_t k = 0; k < (size_t)InputState::Key::Count; k++) {
        int glfwKey = toGlfw[k];
        bool isPressed = (glfwGetKey(glfwWindow.getWindow(), glfwKey) == GLFW_PRESS);
        inputState.setDown(static_cast<InputState::Key>(k), isPressed);
    }

    if (inputState.wasPressed(InputState::Key::ALT)) {
        cursorVisible = !cursorVisible;
        glfwSetInputMode(glfwWindow.getWindow(), GLFW_CURSOR,
                         cursorVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

void GlfwInput::pollMouseFromGlfw(GlfwWindow &glfwWindow) {
    double xpos, ypos;
    glfwGetCursorPos(glfwWindow.getWindow(), &xpos, &ypos);
    glm::vec2 curCursorPos{static_cast<float>(xpos), static_cast<float>(ypos)};

    glm::vec2 delta = curCursorPos - lastCursorPos;
    inputState.addMouseDelta(delta);
    lastCursorPos = curCursorPos;
}

} // namespace my
