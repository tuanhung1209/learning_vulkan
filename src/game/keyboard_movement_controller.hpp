#pragma once

#include "ecs/components/transform_component.hpp"

namespace my {

class InputState;

class KeyboardMovementController {
  public:
    void moveInPlaneXZ(const InputState &input, float dt, TransformComponent &transformComponent);

    float movementSpeed{30.0f};
    float rotationSpeed{1.5f};
    float mouseSensitivity{0.0015f};
};
} // namespace my
