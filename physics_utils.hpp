#pragma once 

#include "my_game_object.hpp"

namespace my{
struct OBB{
    glm::vec3 center{};
    glm::vec3 extents{};
    glm::mat3 axes{};
};

struct collisionManifold{
    bool isColliding = false;
    glm::vec3 normal{};
    float depth = 0.0f;
    std::vector<glm::vec3> contactPoints{};
};

class CollisionSystem{
public:

private:
    static OBB getOBB(MyGameObject &obj);

    static bool testAxis(const glm::vec3& axis, const OBB& obbA, const OBB& obbB,
                         float& minOverlap, glm::vec3& smallestAxis);

};

};