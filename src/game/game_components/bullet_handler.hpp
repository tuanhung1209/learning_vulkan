#pragma once

#include "ecs/ecs_manager.hpp"
#include "render_core/my_frame_info.hpp"

#include <array>
#include <vulkan/vulkan.h>

namespace my {

class Device;
class MyModel;

class BulletHandler {
  public:
    BulletHandler(EcsManager &ecsManager);
    ~BulletHandler();

    void spawnBullet(glm::vec3 position, glm::vec3 velocity, glm::vec3 rotation);
    void update(float dt);

    // Call after EcsManager::clearEcs() — recreates the 100 bullet entities
    // (clearEcs destroys every entity in the registry, including the pool).
    void resetPool();

  private:
    std::array<Entity, MAX_BULLET> bulletEntities{};
    EcsManager &ecsManager_;
};

} // namespace my
