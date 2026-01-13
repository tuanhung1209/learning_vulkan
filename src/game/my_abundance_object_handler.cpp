#include "my_abundance_object_handler.hpp"

namespace my{

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

BulletHandler::BulletHandler(std::shared_ptr<MyModel> model) : bulletModel{model} {
    bullets.reserve(MAX_BULLET);
    for (int i = 0; i < MAX_BULLET; i++) {
        auto bullet = MyGameObject::createGameObject();
        bullet.bulletCom = std::make_unique<BulletComponent>();
        bullet.transform.scale = {0.1f, 0.1f, 0.1f};
        bullets.push_back(std::make_unique<MyGameObject>(std::move(bullet)));
    }
}

BulletHandler::~BulletHandler(){}

void BulletHandler::spawnBullet(glm::vec3 position, glm::vec3 direction, glm::vec3 rotation){
    for (auto &bullet : bullets){
        if (!bullet->bulletCom->isActive){
            bullet->bulletCom->isActive = true;
            bullet->transform.translation = position;
            bullet->transform.rotation = rotation;
            bullet->transform.scale = {0.02f, 0.02f, 0.2f};
            bullet->bulletCom->velocity = direction * 15.0f;
            bullet->bulletCom->lifeTime = 5.0f; 
            return;
        }
    }   
}

void BulletHandler::update(float dt){
    for (auto &bullet : bullets){
        if (bullet->bulletCom->isActive){
            bullet->transform.translation += dt * bullet->bulletCom->velocity;
            bullet->bulletCom->lifeTime -= dt;

            if (bullet->bulletCom->lifeTime <= 0){
                bullet->bulletCom->isActive = false;
            }
        }
    }   
}

void BulletHandler::renderBullet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    if (bulletModel == nullptr) return;

    bulletModel->bind(commandBuffer);

    for (auto &bullet : bullets) {
        if (!bullet->bulletCom->isActive) continue;

        SimplePushConstantData push{};
        push.modelMatrix = bullet->transform.mat4();
        push.normalMatrix = bullet->transform.normalMatrix();

        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SimplePushConstantData),
            &push);

        bulletModel->draw(commandBuffer);
    }
}

}