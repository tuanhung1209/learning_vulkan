#include "game/my_player.hpp"

#include "input/input_state.hpp"
#include "ecs/components/transform_component.hpp"

namespace my {

MyPlayer::MyPlayer(MyCamera &camera, EcsManager &ecsManager, InputState &input, Entity playerEntity)
    : camera_{camera}, playerEntity_(playerEntity), ecsManager_(ecsManager), input_{input} {}

MyPlayer::~MyPlayer() {}

void MyPlayer::update(InputState &input, float dt, BulletHandler &bulletHandler) {

    TransformComponent *player_trans = ecsManager_.get<TransformComponent>(playerEntity_);
    if (!player_trans) return;

    playerController.moveInPlaneXZ(input, dt, *player_trans);

    camera_.setViewYXZ(player_trans->translation, player_trans->rotation);

    if (fireCooldown > 0) fireCooldown -= dt;

    if (input.isDown(InputState::Key::SPACE) && fireCooldown <= 0) {
        shoot(bulletHandler);
        fireCooldown = 0.2f;
    }
}

void MyPlayer::shoot(BulletHandler &bulletHandler) {
    TransformComponent *player_trans = ecsManager_.get<TransformComponent>(playerEntity_);
    if (!player_trans) return;

    float yaw = player_trans->rotation.y;
    float pitch = player_trans->rotation.x;
    glm::vec3 forwardDir{sin(yaw) * cos(pitch), -sin(pitch), cos(yaw) * cos(pitch)};

    glm::vec3 spawnPos = player_trans->translation + forwardDir * 1.1f;
    bulletHandler.spawnBullet(spawnPos, forwardDir, player_trans->rotation);
}

} // namespace my
