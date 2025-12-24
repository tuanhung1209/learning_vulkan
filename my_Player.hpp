#pragma once

#include "my_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "my_game_object.hpp"

#include <vector>


namespace my{
class MyPlayer{
public:

    MyPlayer(MyCamera &camera, MyGameObject::id_t playerId);

    ~MyPlayer();

    void shoot();

    void update(GLFWwindow *window, float dt, MyGameObject::Map &gameObjects);

private:
    MyCamera &camera;
    MyGameObject::id_t playerId;
    KeyboardMovementController playerController{};

    std::vector<MyGameObject> bullets;
};

}
