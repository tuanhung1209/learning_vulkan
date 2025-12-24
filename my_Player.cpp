#include "my_Player.hpp"
#include <iostream>

namespace my{
    MyPlayer::MyPlayer(MyCamera &camera, MyGameObject::id_t playerId) : playerId{playerId}, camera{camera} {} 

    MyPlayer::~MyPlayer() {}

    void MyPlayer::update(GLFWwindow *window ,float dt, MyGameObject::Map &gameObjects){
        auto &body = gameObjects.at(playerId);
        playerController.moveInPlaneXZ(window, dt, body); 
        camera.setViewYXZ(body.transform.translation, body.transform.rotation);
    }

    void MyPlayer::shoot(){
       std::cout << "shooting" << std::endl; 
    }
}