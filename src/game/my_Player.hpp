#pragma once

#include "render_core/my_camera.hpp"
#include "game/keyboard_movement_controller.hpp"
#include "game/my_game_object.hpp"

#include <vector>

namespace my{
class MyPlayer{
public:

    MyPlayer(std::shared_ptr<MyModel> bulletModel ,MyCamera &camera, MyGameObject::id_t playerId);

    ~MyPlayer();

    void shoot(float dt, MyGameObject::Map &gameObjects);

    void update(GLFWwindow *window, float dt, MyGameObject::Map &gameObjects);

    std::vector<gameObjectBulletInfo> getBulletInfo() {return bulletsInfo;}

    MyGameObject::id_t getPlayerId() const { return playerId; }

private:
    MyCamera &camera;
    MyGameObject::id_t playerId;
    KeyboardMovementController playerController{};

    std::shared_ptr<MyModel> bulletModel;
    std::vector<gameObjectBulletInfo> bulletsInfo;
    float fireCooldown = 0.f;
};

}
