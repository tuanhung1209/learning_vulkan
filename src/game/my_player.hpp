#pragma once

#include "game_components/bullet_handler.hpp"
#include "ecs/ecs_manager.hpp"
#include "game/keyboard_movement_controller.hpp"
#include "render_core/my_camera.hpp"

namespace my {

class MyPlayer {
  public:
    MyPlayer(MyCamera &camera, EcsManager &ecsManager, InputState &input, Entity playerEntity);
    ~MyPlayer();

    void shoot(BulletHandler &bulletHandler);

    void update(InputState &input, float dt, BulletHandler &bulletHandler);

    uint32_t getPlayerId() const { return playerEntity_.id; }

    void setPlayerEntity(Entity playerEntity) { playerEntity_ = playerEntity; }

  private:
    Entity playerEntity_;
    MyCamera &camera_;
    InputState &input_;
    EcsManager &ecsManager_;
    KeyboardMovementController playerController{};

    float fireCooldown = 0.f;
};

} // namespace my
