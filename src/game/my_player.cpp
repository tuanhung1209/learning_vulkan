#include "game/my_player.hpp"

#include "input/input_state.hpp"

namespace my {

MyPlayer::MyPlayer(MyCamera &camera, MyGameObject::id_t playerId, InputState &input)
    : playerId{playerId}, camera{camera}, input{input} {}

MyPlayer::~MyPlayer() {}

void MyPlayer::update(InputState &input, float dt, MyGameObject::Map &gameObjects,
                      BulletHandler &bulletHandler) {
    auto &body = gameObjects.at(playerId);
    playerController.moveInPlaneXZ(input, dt, body);
    camera.setViewYXZ(body.transform.translation, body.transform.rotation);

    if (fireCooldown > 0) fireCooldown -= dt;

    if (input.isDown(InputState::Key::SPACE) && fireCooldown <= 0) {
        shoot(bulletHandler, gameObjects);
        fireCooldown = 0.2f;
    }
}

void MyPlayer::shoot(BulletHandler &bulletHandler, MyGameObject::Map &gameObjects) {
    auto &playerTransform = gameObjects.at(playerId).transform;

    float yaw = playerTransform.rotation.y;
    float pitch = playerTransform.rotation.x;
    glm::vec3 forwardDir{sin(yaw) * cos(pitch), -sin(pitch), cos(yaw) * cos(pitch)};

    glm::vec3 spawnPos = playerTransform.translation + forwardDir * 1.1f;
    bulletHandler.spawnBullet(spawnPos, forwardDir, playerTransform.rotation);
}

} // namespace my
