#pragma once

#include "render_core/my_camera.hpp"
#include "game/keyboard_movement_controller.hpp"
#include "game/my_game_object.hpp"
#include "game/my_abundance_object_handler.hpp"

#include <vector>

namespace my{
class MyPlayer{
public:

    MyPlayer(MyCamera &camera, MyGameObject::id_t playerId);

    ~MyPlayer();

    void shoot(BulletHandler &bulletHandler, MyGameObject::Map &gameObjects);

    void update(GLFWwindow *window, float dt, MyGameObject::Map &gameObjects, BulletHandler &bulletHandler);

    MyGameObject::id_t getPlayerId() const { return playerId; }

private:
    MyCamera &camera;
    MyGameObject::id_t playerId;
    KeyboardMovementController playerController{};

    float fireCooldown = 0.f;
};

}