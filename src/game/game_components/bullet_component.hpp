#pragma once

#include <glm/glm.hpp>

namespace my {

struct BulletComponent {
    glm::vec3 velocity;
    float lifeTime;
    bool isActive = false;
};

} // namespace my
