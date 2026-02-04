#pragma once

#include "game/my_game_object.hpp"

#include <vector>

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

struct ContactConstraint {
    MyGameObject *objA = nullptr;
    MyGameObject *objB = nullptr;
    collisionManifold manifold;
    // Accumulated impulses for sequential impulse clamping (max 4 contact points)
    float normalImpulseAccum[4] = {};
    float tangentImpulseAccum[4] = {};
};

class CollisionSystem {
  public:
    static collisionManifold checkCollisionOBB(MyGameObject &objA, MyGameObject &objB);

  private:
    static OBB getOBB(MyGameObject &obj);
    static bool testAxis(const glm::vec3 &axis, const OBB &obbA, const OBB &obbB, float &minOverlap,
                         glm::vec3 &smallestAxis);

    static std::vector<glm::vec3> getFace(const OBB &obb, const glm::vec3 &normal);
    static std::vector<glm::vec3> clip(const std::vector<glm::vec3> &subjectPoly,
                                       const glm::vec3 &planeNormal, float planeDist);
};

class PhysicsWorld {
  public:
    static constexpr float FIXED_DT = 1.f / 120.f;
    static constexpr int VELOCITY_ITERATIONS = 10;
    static constexpr int POSITION_ITERATIONS = 3;
    static constexpr float GRAVITY = 9.8f;

    void step(MyGameObject::Map &objs,
              std::vector<std::unique_ptr<MyGameObject>> &bullets,
              float frameTime);

  private:
    float accumulator = 0.f;

    void integrateForces(MyGameObject::Map &objs, float dt);
    void integrateVelocities(MyGameObject::Map &objs, float dt);
    void solveVelocityConstraints(std::vector<ContactConstraint> &contacts);
    void solvePositionConstraints(std::vector<ContactConstraint> &contacts);
    void updateSleep(MyGameObject::Map &objs, float dt);

    std::vector<ContactConstraint> detectCollisions(MyGameObject::Map &objs);
    void detectBulletCollisions(std::vector<std::unique_ptr<MyGameObject>> &bullets,
                                MyGameObject::Map &objs);
};

}; // namespace my
