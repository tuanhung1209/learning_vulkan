#include "bullet_handler.hpp"

#include "bullet_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/components/model_component.hpp"
#include "ecs/components/texture_component.hpp"

namespace my {
BulletHandler::BulletHandler(EcsManager &ecsManager) : ecsManager_(ecsManager) {
    for (int i = 0; i < bulletEntities.size(); i++) {
        Entity bullet = ecsManager_.createEntity();
        ecsManager_.add<BulletComponent>(bullet, BulletComponent{});
        ecsManager_.add<TransformComponent>(bullet, TransformComponent{});
        bulletEntities[i] = bullet;
    }
}

BulletHandler::~BulletHandler() {}

void BulletHandler::resetPool() {
    for (int i = 0; i < bulletEntities.size(); i++) {
        Entity bullet = ecsManager_.createEntity();
        ecsManager_.add<BulletComponent>(bullet, BulletComponent{});
        ecsManager_.add<TransformComponent>(bullet, TransformComponent{});
        bulletEntities[i] = bullet;
    }
}

void BulletHandler::spawnBullet(glm::vec3 position, glm::vec3 direction, glm::vec3 rotation) {
    for (Entity &bullet : bulletEntities) {
        BulletComponent *bc = ecsManager_.get<BulletComponent>(bullet);
        TransformComponent *trans = ecsManager_.get<TransformComponent>(bullet);

        if (!trans || !bc || bc->isActive) continue;

        // only render if have model and texture
        ecsManager_.add<ModelComponent>(bullet, ModelComponent{"assets/models/cube.obj"});
        ecsManager_.add<TextureComponent>(bullet, TextureComponent{"assets/textures/white.png"});

        trans->translation = position;
        trans->rotation = rotation;
        trans->scale = {0.02f, 0.02f, 0.2f};

        bc->isActive = true;
        bc->velocity = direction * 30.0f;
        bc->lifeTime = 5.0f;

        return;
    }
}

void BulletHandler::update(float dt) {
    for (Entity &bullet : bulletEntities) {
        BulletComponent *bc = ecsManager_.get<BulletComponent>(bullet);
        TransformComponent *trans = ecsManager_.get<TransformComponent>(bullet);

        if (!bc || !trans) continue;
        if (bc->isActive) {
            trans->translation += dt * bc->velocity;
            bc->lifeTime -= dt;

            // remove so render no more
            if (bc->lifeTime <= 0) {
                ecsManager_.remove<ModelComponent>(bullet);
                ecsManager_.remove<TextureComponent>(bullet);
                bc->isActive = false;
            }
        }
    }
}

} // namespace my
