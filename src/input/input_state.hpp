#pragma once

#include <array>
#include <glm/glm.hpp>

namespace my {
class InputState {
  public:
    enum class Key { W, A, S, D, Q, E, Up, Down, Left, Right, SPACE, ALT, Count };

    bool isDown(Key k) const { return isDownArray[(size_t)k]; }
    void setDown(Key k, bool state) { isDownArray[(size_t)k] = state; }

    bool wasPressed(Key k) const { return isDownArray[(size_t)k] && !prevDownArray[(size_t)k]; }
    bool wasReleased(Key k) const { return !isDownArray[(size_t)k] && prevDownArray[(size_t)k]; }

    glm::vec2 getMouseDelta() const { return mouseDelta; }
    void addMouseDelta(glm::vec2 dir) { mouseDelta += dir; }

    void endFrame() {
        prevDownArray = isDownArray;
        mouseDelta = {0.0f, 0.0f};
    }

  private:
    std::array<bool, (size_t)Key::Count> isDownArray{};
    std::array<bool, (size_t)Key::Count> prevDownArray{};
    glm::vec2 mouseDelta{0.0f};
};

} // namespace my
