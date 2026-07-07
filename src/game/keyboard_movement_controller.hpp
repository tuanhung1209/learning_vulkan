#pragma once

#include "game/my_game_object.hpp"

namespace my {

class InputState;

class KeyboardMovementController {
  public:
    void moveInPlaneXZ(const InputState &input, float dt, MyGameObject &gameObject);

    float movementSpeed{30.0f};
    float rotationSpeed{1.5f};
    float mouseSensitivity{0.0015f};
};
} // namespace my
