#pragma once

#include "game/my_game_object.hpp"

namespace my {
struct OBB {
    glm::vec3 center{};
    glm::vec3 extents{};
    glm::mat3 axes{};
};

struct collisionManifold {
    bool isColliding = false;
    glm::vec3 normal{};
    float depth = 0.0f;
    std::vector<glm::vec3> contactPoints{};
};

class CollisionSystem {
  public:
    static collisionManifold checkCollisionOBB(MyGameObject &objA, MyGameObject &objB);

    static void collisionResolve(MyGameObject &objA, MyGameObject &objB,
                                 collisionManifold &collisionManifold);

  private:
    static OBB getOBB(MyGameObject &obj);
    static bool testAxis(const glm::vec3 &axis, const OBB &obbA, const OBB &obbB, float &minOverlap,
                         glm::vec3 &smallestAxis);

    static std::vector<glm::vec3> getFace(const OBB &obb, const glm::vec3 &normal);
    static std::vector<glm::vec3> clip(const std::vector<glm::vec3> &subjectPoly,
                                       const glm::vec3 &planeNormal, float planeDist);

    static void applyImpulse(MyGameObject &objA, MyGameObject &objB, collisionManifold &collisionManifold);
    static void linearProjection(MyGameObject &objA, MyGameObject &objB,
                                 collisionManifold &collisionManifold);
};

class GravitySystem {
  public:
    static void update(MyGameObject::Map &objs, float dt);

  private:
};

}; // namespace my
