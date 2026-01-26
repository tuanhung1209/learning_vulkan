#include "game/physics_utils.hpp"

namespace my {

OBB CollisionSystem::getOBB(MyGameObject &obj) {
    OBB obb;

    glm::vec3 min = obj.model ? obj.model->getBound().min : glm::vec3(-0.5f);
    glm::vec3 max = obj.model ? obj.model->getBound().max : glm::vec3(0.5f);

    glm::vec3 scale = obj.transform.scale;
    glm::vec3 centerLocal = (min + max) * 0.5f;
    glm::vec3 extentsLocal = (max - min) * 0.5f;

    obb.extents = extentsLocal * glm::abs(scale);

    obb.axes = obj.transform.normalMatrix();

    obb.center = obj.transform.translation + (obb.axes * centerLocal);

    obb.axes[0] = glm::normalize(obb.axes[0]);
    obb.axes[1] = glm::normalize(obb.axes[1]);
    obb.axes[2] = glm::normalize(obb.axes[2]);

    return obb;
};

bool CollisionSystem::testAxis(const glm::vec3 &axis, const OBB &obbA, const OBB &obbB, float &minOverlap,
                               glm::vec3 &smallestAxis) {

    float parallelAxis = glm::dot(axis, axis);
    if (parallelAxis < 1e-8f) return true;

    // TODO : invesigate this ???
    glm::vec3 nAxis = axis / std::sqrt(parallelAxis);

    float rA = glm::abs(glm::dot(obbA.axes[0], nAxis) * obbA.extents.x) +
               glm::abs(glm::dot(obbA.axes[1], nAxis) * obbA.extents.y) +
               glm::abs(glm::dot(obbA.axes[2], nAxis) * obbA.extents.z);

    float rB = glm::abs(glm::dot(obbB.axes[0], nAxis) * obbB.extents.x) +
               glm::abs(glm::dot(obbB.axes[1], nAxis) * obbB.extents.y) +
               glm::abs(glm::dot(obbB.axes[2], nAxis) * obbB.extents.z);

    glm::vec3 distance = obbA.center - obbB.center;
    float t = glm::abs(glm::dot(distance, nAxis));

    float overLap = (rA + rB) - t;
    if (overLap < 0.f) return false;

    if (overLap < minOverlap) {
        minOverlap = overLap;
        smallestAxis = nAxis;
        if (glm::dot(distance, smallestAxis) < 0.0f) { smallestAxis = -smallestAxis; }
    }
    return true;
}

std::vector<glm::vec3> CollisionSystem::clip(const std::vector<glm::vec3> &subjectPoly,
                                             const glm::vec3 &planeNormal, float planeDist) {
    std::vector<glm::vec3> clipedPoint;
    if (subjectPoly.empty()) return clipedPoint;

    glm::vec3 v1 = subjectPoly.back();
    float d1 = glm::dot(v1, planeNormal) - planeDist;

    for (size_t i = 0; i < subjectPoly.size(); i++) {
        glm::vec3 v2 = subjectPoly[i];
        float d2 = glm::dot(v2, planeNormal) - planeDist;

        if (d1 >= 0.0f && d2 >= 0.0f) {
            clipedPoint.push_back(v2);
        } else if (d1 >= 0.0f && d2 < 0.0f) {
            float t = d1 / (d1 - d2);
            glm::vec3 intersection = v1 + t * (v2 - v1);
            clipedPoint.push_back(intersection);
        } else if (d1 < 0.0f && d2 >= 0.0f) {
            float t = d1 / (d1 - d2);
            glm::vec3 intersection = v1 + t * (v2 - v1);
            clipedPoint.push_back(intersection);
            clipedPoint.push_back(v2);
        }

        v1 = v2;
        d1 = d2;
    }

    return clipedPoint;
}

std::vector<glm::vec3> CollisionSystem::getFace(const OBB &obb, const glm::vec3 &normal) {
    int axisIndex = 0;
    float maxDot = glm::abs(glm::dot(obb.axes[0], normal));

    float dotY = glm::abs(glm::dot(obb.axes[1], normal));
    if (dotY > maxDot) {
        maxDot = dotY;
        axisIndex = 1;
    }

    float dotZ = glm::abs(glm::dot(obb.axes[2], normal));
    if (dotZ > maxDot) {
        maxDot = dotZ;
        axisIndex = 2;
    }

    glm::vec3 axis = obb.axes[axisIndex];
    bool positive = glm::dot(axis, normal) > 0.0f;
    if (!positive) axis = -axis;

    std::vector<glm::vec3> vertices;
    vertices.reserve(4);

    glm::vec3 faceCenter = obb.center + axis * obb.extents[axisIndex];

    int i1 = (axisIndex + 1) % 3;
    int i2 = (axisIndex + 2) % 3;

    glm::vec3 right = obb.axes[i1] * obb.extents[i1];
    glm::vec3 up = obb.axes[i2] * obb.extents[i2];

    vertices.push_back(faceCenter + right + up);
    vertices.push_back(faceCenter - right + up);
    vertices.push_back(faceCenter - right - up);
    vertices.push_back(faceCenter + right - up);

    return vertices;
}

collisionManifold CollisionSystem::checkCollisionOBB(MyGameObject &objA, MyGameObject &objB) {
    collisionManifold collisionResult{};
    collisionResult.depth = std::numeric_limits<float>::max();

    OBB obbA = getOBB(objA);
    OBB obbB = getOBB(objB);

    glm::vec3 axesToTest[15];
    int iaxes = 0;

    axesToTest[iaxes++] = obbA.axes[0];
    axesToTest[iaxes++] = obbA.axes[1];
    axesToTest[iaxes++] = obbA.axes[2];

    axesToTest[iaxes++] = obbB.axes[0];
    axesToTest[iaxes++] = obbB.axes[1];
    axesToTest[iaxes++] = obbB.axes[2];

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) { axesToTest[iaxes++] = glm::cross(obbA.axes[i], obbB.axes[j]); }
    }

    for (int i = 0; i < iaxes; i++) {
        if (!testAxis(axesToTest[i], obbA, obbB, collisionResult.depth, collisionResult.normal)) {
            return collisionResult;
        }
    }

    collisionResult.isColliding = true;

    OBB *refOBB = &obbA;
    OBB *incOBB = &obbB;

    if (glm::dot(collisionResult.normal, obbB.center - obbA.center) < 0.0f) {
        collisionResult.normal = -collisionResult.normal;
    }

    float dotA = 0.0f;
    for (int i = 0; i < 3; i++) {
        float d = glm::abs(glm::dot(obbA.axes[i], collisionResult.normal));
        if (d > dotA) dotA = d;
    }

    float dotB = 0.0f;
    for (int i = 0; i < 3; i++) {
        float d = glm::abs(glm::dot(obbB.axes[i], collisionResult.normal));
        if (d > dotB) dotB = d;
    }

    bool flip = false;
    if (dotB > dotA) {
        refOBB = &obbB;
        incOBB = &obbA;
        collisionResult.normal = -collisionResult.normal;
        flip = true;
    }

    std::vector<glm::vec3> incidentFace = getFace(*incOBB, -collisionResult.normal);

    int refAxisIdx = 0;
    float maxDot = 0.0f;
    for (int i = 0; i < 3; i++) {
        float d = glm::abs(glm::dot(refOBB->axes[i], collisionResult.normal));
        if (d > maxDot) {
            maxDot = d;
            refAxisIdx = i;
        }
    }

    std::vector<glm::vec3> poly = incidentFace;

    int i1 = (refAxisIdx + 1) % 3;
    int i2 = (refAxisIdx + 2) % 3;
    int sideAxes[] = {i1, i2};

    for (int ax : sideAxes) {
        {
            glm::vec3 n = refOBB->axes[ax];
            glm::vec3 p = refOBB->center + n * refOBB->extents[ax];
            float dist = glm::dot(n, p);
            poly = clip(poly, -n, -dist);
        }

        {
            glm::vec3 n = refOBB->axes[ax];
            glm::vec3 p = refOBB->center - n * refOBB->extents[ax];
            float dist = glm::dot(n, p);
            poly = clip(poly, n, dist);
        }
    }

    glm::vec3 refNormal = refOBB->axes[refAxisIdx];
    if (glm::dot(refNormal, collisionResult.normal) < 0.0f) refNormal = -refNormal;

    float refPlaneDist = glm::dot(refNormal, refOBB->center + refNormal * refOBB->extents[refAxisIdx]);

    for (const auto &pt : poly) {
        float d = glm::dot(refNormal, pt) - refPlaneDist;

        if (d <= 0.0f) { collisionResult.contactPoints.push_back(pt); }
    }

    if (flip) { collisionResult.normal = -collisionResult.normal; }

    return collisionResult;
}

void CollisionSystem::applyImpulse(MyGameObject &objA, MyGameObject &objB,
                                   collisionManifold &collisionManifold) {
    RigidBodyComponent *rbA = objA.rigidBody.get();
    RigidBodyComponent *rbB = objB.rigidBody.get();

    float invMassA = (rbA && rbA->mass > 0.0f) ? 1.0f / rbA->mass : 0.0f;
    float invMassB = (rbB && rbB->mass > 0.0f) ? 1.0f / rbB->mass : 0.0f;

    if (invMassA + invMassB == 0.0f) return;

    glm::vec3 velA = rbA ? rbA->velocity : glm::vec3(0.0f);
    glm::vec3 velB = rbB ? rbB->velocity : glm::vec3(0.0f);

    glm::vec3 normal = collisionManifold.normal;

    glm::vec3 relVel = velB - velA;
    float velAlongNormal = glm::dot(relVel, normal);

    if (velAlongNormal > 0) return;

    float restitution = 0.5f;
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= (invMassA + invMassB);

    glm::vec3 impulse = j * normal;

    if (rbA) rbA->velocity -= impulse * invMassA;
    if (rbB) rbB->velocity += impulse * invMassB;
}

void CollisionSystem::collisionResolve(MyGameObject &objA, MyGameObject &objB,
                                       collisionManifold &collisionManifold) {
    if (!collisionManifold.isColliding) return;

    const int iteration = 4;
    for (int i = 0; i < iteration; i++) { applyImpulse(objA, objB, collisionManifold); }
}

void GravitySystem::update(MyGameObject::Map &objs, float dt) {
    for (auto &kv : objs) {
        auto &obj = kv.second;
        if (obj.rigidBody == nullptr) continue;

        const float GRAVITY = 9.8f;
        if (obj.rigidBody->mass > 0.0f) {
            obj.rigidBody->velocity.y += GRAVITY * dt;
            obj.transform.translation += obj.rigidBody->velocity * dt;
        }
    }
}

}; // namespace my
