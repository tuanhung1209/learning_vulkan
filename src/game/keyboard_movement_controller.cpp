#include "game/keyboard_movement_controller.hpp"

#include "ecs/components/transform_component.hpp"
#include "input/input_state.hpp"

#include <limits>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace my {
void KeyboardMovementController::moveInPlaneXZ(const InputState &input, float dt,
                                               TransformComponent &transformComponent) {

    glm::vec3 rotate{0};
    if (input.isDown(InputState::Key::Right)) rotate.y += 1.f;
    if (input.isDown(InputState::Key::Left)) rotate.y -= 1.f;
    if (input.isDown(InputState::Key::Up)) rotate.x += 1.f;
    if (input.isDown(InputState::Key::Down)) rotate.x -= 1.f;

    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
        transformComponent.rotation += rotationSpeed * dt * glm::normalize(rotate);
    }

    /*
    glm::vec2 mouseDelta = input.getMouseDelta();
    gameObject.transform.rotation.y += mouseSensitivity * mouseDelta.x;
    gameObject.transform.rotation.x -= mouseSensitivity * mouseDelta.y;
    */

    transformComponent.rotation.x = glm::clamp(transformComponent.rotation.x, -1.5f, 1.5f);
    transformComponent.rotation.y = glm::mod(transformComponent.rotation.y, glm::two_pi<float>());

    // to find the curretly facing direction
    float yaw = transformComponent.rotation.y;
    const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
    const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
    const glm::vec3 upDir{0.f, -1.f, 0.f};

    glm::vec3 moveDir{0.f};
    if (input.isDown(InputState::Key::W)) moveDir += forwardDir;
    if (input.isDown(InputState::Key::S)) moveDir -= forwardDir;
    if (input.isDown(InputState::Key::D)) moveDir += rightDir;
    if (input.isDown(InputState::Key::A)) moveDir -= rightDir;
    if (input.isDown(InputState::Key::E)) moveDir += upDir;
    if (input.isDown(InputState::Key::Q)) moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        transformComponent.translation += movementSpeed * dt * glm::normalize(moveDir);
    }
}

} // namespace my
