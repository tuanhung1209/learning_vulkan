#include "my_Player.hpp"

namespace my{
    MyPlayer::MyPlayer(std::shared_ptr<MyModel> bulletModel, MyCamera &camera, MyGameObject::id_t playerId) : playerId{playerId}, camera{camera}, bulletModel{bulletModel} {} 

    MyPlayer::~MyPlayer() {}

    void MyPlayer::update(GLFWwindow *window ,float dt, MyGameObject::Map &gameObjects){
        auto &body = gameObjects.at(playerId);
        playerController.moveInPlaneXZ(window, dt, body); 
        camera.setViewYXZ(body.transform.translation, body.transform.rotation);

        if (fireCooldown > 0.f){
            fireCooldown -= dt;
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && fireCooldown <= 0.f) {
            shoot(dt, gameObjects);
            fireCooldown = 0.2f;
        }

        for (auto it = bulletsInfo.begin(); it != bulletsInfo.end();){
            auto &bulletInfo = *it;

            if (gameObjects.find(bulletInfo.gameObjectId) != gameObjects.end()){
                auto &mapBullet = gameObjects.at(bulletInfo.gameObjectId); 
                mapBullet.transform.translation += bulletInfo.velocity * dt;
            }

            bulletInfo.lifeTime -= dt;

            if (bulletInfo.lifeTime <= 0){
                gameObjects.erase(bulletInfo.gameObjectId);
                it = bulletsInfo.erase(it);
            }
            else {
                it++;
            }
        }
    }

    void MyPlayer::shoot(float dt, MyGameObject::Map &gameObjects){
        auto bullet = MyGameObject::createGameObject();
        bullet.model = bulletModel;

        auto &playerTransform = gameObjects.at(playerId).transform;
        bullet.transform.translation = playerTransform.translation; 
        bullet.transform.rotation = playerTransform.rotation; 
        bullet.transform.scale = glm::vec3{0.05f, 0.05f, 0.2f};

        float yaw = playerTransform.rotation.y;
        float pitch = playerTransform.rotation.x;
        glm::vec3 forwardDir{sin(yaw) * cos(pitch), -sin(pitch), cos(yaw) * cos(pitch)};

        bullet.transform.translation += forwardDir * 1.5f;

        gameObjectBulletInfo bulletInfo{};
        bulletInfo.gameObjectId = bullet.getId();
        bulletInfo.lifeTime = 20.0f;
        bulletInfo.velocity = forwardDir * glm::vec3{2.f};

        bulletsInfo.push_back(bulletInfo); 

        gameObjects.emplace(bullet.getId(), std::move(bullet));
    }
}