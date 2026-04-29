#pragma once

#include "game/my_game_object.hpp"
#include "input/input_state.hpp"
#include "vulkan_core/window.hpp"

namespace my {
class KeyboardMovementController {
  public:
    void moveInPlaneXZ(const InputState &input, float dt, MyGameObject &gameObject);

    float movementSpeed{30.0f};
    float rotationSpeed{1.5f};
    float mouseSensitivity{0.0015f};
};
} // namespace my
