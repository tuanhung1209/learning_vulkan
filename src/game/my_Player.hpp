#pragma once

#include "game/keyboard_movement_controller.hpp"
#include "game/my_abundance_object_handler.hpp"
#include "game/my_game_object.hpp"
#include "input/input_state.hpp"
#include "render_core/my_camera.hpp"

#include <vector>

namespace my {

class MyPlayer {
  public:
    MyPlayer(MyCamera &camera, MyGameObject::id_t playerId, InputState &input);
    ~MyPlayer();

    void shoot(BulletHandler &bulletHandler, MyGameObject::Map &gameObjects);

    void update(InputState &input, float dt, MyGameObject::Map &gameObjects, BulletHandler &bulletHandler);

    MyGameObject::id_t getPlayerId() const { return playerId; }
    void setPlayerId(MyGameObject::id_t id) { playerId = id; }

  private:
    MyCamera &camera;
    MyGameObject::id_t playerId;
    KeyboardMovementController playerController{};
    InputState &input;

    float fireCooldown = 0.f;
};

} // namespace my
