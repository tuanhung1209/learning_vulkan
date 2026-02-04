#include "game/physics_utils.hpp"

#include <algorithm>
#include <cmath>

namespace my {

// ═══════════════════════════════════════════════════════════════════
// COLLISION DETECTION (OBB / SAT) — unchanged from original
// ═══════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════
// PHYSICS WORLD — Sequential Impulse Solver (Box2D / Bullet style)
// ═══════════════════════════════════════════════════════════════════

void PhysicsWorld::step(MyGameObject::Map &objs,
                        std::vector<std::unique_ptr<MyGameObject>> &bullets,
                        float frameTime) {
    frameTime = std::min(frameTime, 0.05f);
    accumulator += frameTime;

    while (accumulator >= FIXED_DT) {
        // 1. Apply forces (gravity) and damping
        integrateForces(objs, FIXED_DT);

        // 2. Broadphase + narrowphase collision detection
        auto contacts = detectCollisions(objs);

        // 3. Wake sleeping objects involved in collisions
        for (auto &c : contacts) {
            if (c.objA->rigidBody && c.objA->rigidBody->isSleeping) {
                c.objA->rigidBody->isSleeping = false;
                c.objA->rigidBody->sleepTimer = 0.f;
            }
            if (c.objB->rigidBody && c.objB->rigidBody->isSleeping) {
                c.objB->rigidBody->isSleeping = false;
                c.objB->rigidBody->sleepTimer = 0.f;
            }
        }

        // 4. Sequential impulse: solve velocity constraints (multiple iterations)
        for (int i = 0; i < VELOCITY_ITERATIONS; i++) {
            solveVelocityConstraints(contacts);
        }

        // 5. Integrate velocities -> update positions/rotations
        integrateVelocities(objs, FIXED_DT);

        // 6. Solve position constraints (Baumgarte correction, multiple iterations)
        for (int i = 0; i < POSITION_ITERATIONS; i++) {
            solvePositionConstraints(contacts);
        }

        // 7. Bullet collisions (separate pass — bullets are projectiles, not iterated)
        detectBulletCollisions(bullets, objs);

        // 8. Sleep system
        updateSleep(objs, FIXED_DT);

        accumulator -= FIXED_DT;
    }
}

void PhysicsWorld::integrateForces(MyGameObject::Map &objs, float dt) {
    for (auto &[id, obj] : objs) {
        if (!obj.rigidBody || obj.rigidBody->mass <= 0.f || obj.rigidBody->isSleeping) continue;
        auto *rb = obj.rigidBody.get();

        // Gravity (Y-down in this engine)
        rb->velocity.y += GRAVITY * dt;

        // Framerate-independent exponential damping
        rb->velocity *= std::exp(-rb->linearDamping * dt);
        rb->angularVelocity *= std::exp(-rb->angularDamping * dt);
    }
}

void PhysicsWorld::integrateVelocities(MyGameObject::Map &objs, float dt) {
    for (auto &[id, obj] : objs) {
        if (!obj.rigidBody || obj.rigidBody->mass <= 0.f || obj.rigidBody->isSleeping) continue;
        auto *rb = obj.rigidBody.get();

        // Velocity clamping to prevent tunneling
        constexpr float MAX_VELOCITY = 30.f;
        float speed = glm::length(rb->velocity);
        if (speed > MAX_VELOCITY) {
            rb->velocity = (rb->velocity / speed) * MAX_VELOCITY;
        }

        constexpr float MAX_ANGULAR_VELOCITY = 10.f;
        float angSpeed = glm::length(rb->angularVelocity);
        if (angSpeed > MAX_ANGULAR_VELOCITY) {
            rb->angularVelocity = (rb->angularVelocity / angSpeed) * MAX_ANGULAR_VELOCITY;
        }

        // NaN safety
        if (std::isnan(rb->velocity.x) || std::isnan(rb->velocity.y) || std::isnan(rb->velocity.z)) {
            rb->velocity = glm::vec3(0.f);
        }
        if (std::isnan(rb->angularVelocity.x) || std::isnan(rb->angularVelocity.y) ||
            std::isnan(rb->angularVelocity.z)) {
            rb->angularVelocity = glm::vec3(0.f);
        }

        // Semi-implicit Euler integration
        obj.transform.translation += rb->velocity * dt;
        obj.transform.rotation += rb->angularVelocity * dt;
    }
}

std::vector<ContactConstraint> PhysicsWorld::detectCollisions(MyGameObject::Map &objs) {
    std::vector<ContactConstraint> contacts;

    for (auto itA = objs.begin(); itA != objs.end(); ++itA) {
        if (!itA->second.rigidBody) continue;

        auto itB = itA;
        ++itB;
        for (; itB != objs.end(); ++itB) {
            if (!itB->second.rigidBody) continue;

            auto manifold = CollisionSystem::checkCollisionOBB(itA->second, itB->second);
            if (manifold.isColliding && !manifold.contactPoints.empty()) {
                ContactConstraint c{};
                c.objA = &itA->second;
                c.objB = &itB->second;
                c.manifold = std::move(manifold);
                contacts.push_back(std::move(c));
            }
        }
    }

    return contacts;
}

void PhysicsWorld::detectBulletCollisions(std::vector<std::unique_ptr<MyGameObject>> &bullets,
                                          MyGameObject::Map &objs) {
    for (auto &bullet : bullets) {
        if (!bullet->bulletCom || !bullet->bulletCom->isActive) continue;

        for (auto &[id, obj] : objs) {
            if (!obj.rigidBody) continue;

            auto manifold = CollisionSystem::checkCollisionOBB(*bullet, obj);
            if (manifold.isColliding && !manifold.contactPoints.empty()) {
                // Simple single-pass impulse for bullet impacts
                auto *rbBullet = bullet->rigidBody.get();
                auto *rbObj = obj.rigidBody.get();

                float invMassBullet = rbBullet ? rbBullet->invMass() : 0.f;
                float invMassObj = rbObj ? rbObj->invMass() : 0.f;

                if (invMassBullet + invMassObj > 0.f) {
                    glm::vec3 relVel = (rbObj ? rbObj->velocity : glm::vec3(0.f)) -
                                       (rbBullet ? rbBullet->velocity : glm::vec3(0.f));
                    float velAlongNormal = glm::dot(relVel, manifold.normal);

                    if (velAlongNormal < 0.f) {
                        float j = -(1.f + 0.0f) * velAlongNormal / (invMassBullet + invMassObj);
                        glm::vec3 impulse = j * manifold.normal;

                        if (rbObj && rbObj->mass > 0.f) {
                            rbObj->velocity += impulse * invMassObj;

                            // Wake up on bullet hit
                            if (rbObj->isSleeping) {
                                rbObj->isSleeping = false;
                                rbObj->sleepTimer = 0.f;
                            }

                            // Apply angular impulse from bullet
                            glm::vec3 cp = manifold.contactPoints[0];
                            glm::vec3 r = cp - obj.transform.translation;
                            glm::mat3 rot = obj.transform.normalMatrix();
                            rot[0] = glm::normalize(rot[0]);
                            rot[1] = glm::normalize(rot[1]);
                            rot[2] = glm::normalize(rot[2]);
                            glm::mat3 invI = rbObj->invInertiaWorld(rot);
                            rbObj->angularVelocity += invI * glm::cross(r, impulse);
                        }
                    }
                }

                bullet->bulletCom->isActive = false;
                break;
            }
        }
    }
}

void PhysicsWorld::solveVelocityConstraints(std::vector<ContactConstraint> &contacts) {
    for (auto &c : contacts) {
        auto *rbA = c.objA->rigidBody.get();
        auto *rbB = c.objB->rigidBody.get();

        float invMassA = rbA ? rbA->invMass() : 0.f;
        float invMassB = rbB ? rbB->invMass() : 0.f;
        if (invMassA + invMassB == 0.f) continue;

        // World-space inverse inertia tensors
        glm::mat3 rotA = c.objA->transform.normalMatrix();
        rotA[0] = glm::normalize(rotA[0]);
        rotA[1] = glm::normalize(rotA[1]);
        rotA[2] = glm::normalize(rotA[2]);
        glm::mat3 rotB = c.objB->transform.normalMatrix();
        rotB[0] = glm::normalize(rotB[0]);
        rotB[1] = glm::normalize(rotB[1]);
        rotB[2] = glm::normalize(rotB[2]);

        glm::mat3 invIA = rbA ? rbA->invInertiaWorld(rotA) : glm::mat3(0.f);
        glm::mat3 invIB = rbB ? rbB->invInertiaWorld(rotB) : glm::mat3(0.f);

        // Combined material properties
        float restitution = std::min(rbA ? rbA->restitution : 0.f, rbB ? rbB->restitution : 0.f);
        float friction = std::sqrt((rbA ? rbA->friction : 0.6f) * (rbB ? rbB->friction : 0.6f));

        glm::vec3 normal = c.manifold.normal;
        size_t numContacts = std::min(c.manifold.contactPoints.size(), static_cast<size_t>(4));

        for (size_t i = 0; i < numContacts; i++) {
            glm::vec3 cp = c.manifold.contactPoints[i];
            glm::vec3 rA = cp - c.objA->transform.translation;
            glm::vec3 rB = cp - c.objB->transform.translation;

            // Velocity at contact point
            glm::vec3 velA = (rbA ? rbA->velocity : glm::vec3(0.f)) +
                             glm::cross(rbA ? rbA->angularVelocity : glm::vec3(0.f), rA);
            glm::vec3 velB = (rbB ? rbB->velocity : glm::vec3(0.f)) +
                             glm::cross(rbB ? rbB->angularVelocity : glm::vec3(0.f), rB);
            glm::vec3 relVel = velB - velA;

            float velAlongNormal = glm::dot(relVel, normal);

            // Box2D technique: kill restitution for low-speed contacts (prevents micro-bouncing)
            float effectiveRestitution = (glm::abs(velAlongNormal) > 1.0f) ? restitution : 0.0f;

            // ── Normal impulse ──
            glm::vec3 rAxN = glm::cross(rA, normal);
            glm::vec3 rBxN = glm::cross(rB, normal);
            float kNormal = invMassA + invMassB +
                            glm::dot(rAxN, invIA * rAxN) +
                            glm::dot(rBxN, invIB * rBxN);

            if (kNormal <= 0.f) continue;

            float jn = -(1.f + effectiveRestitution) * velAlongNormal / kNormal;
            jn /= static_cast<float>(numContacts);

            // Accumulated impulse clamping (Box2D technique — prevents drift)
            float oldAccum = c.normalImpulseAccum[i];
            c.normalImpulseAccum[i] = std::max(oldAccum + jn, 0.f);
            jn = c.normalImpulseAccum[i] - oldAccum;

            glm::vec3 impulseN = jn * normal;

            if (rbA) {
                rbA->velocity -= impulseN * invMassA;
                rbA->angularVelocity -= invIA * glm::cross(rA, impulseN);
            }
            if (rbB) {
                rbB->velocity += impulseN * invMassB;
                rbB->angularVelocity += invIB * glm::cross(rB, impulseN);
            }

            // ── Friction impulse ──
            // Recompute velocity after normal impulse
            velA = (rbA ? rbA->velocity : glm::vec3(0.f)) +
                   glm::cross(rbA ? rbA->angularVelocity : glm::vec3(0.f), rA);
            velB = (rbB ? rbB->velocity : glm::vec3(0.f)) +
                   glm::cross(rbB ? rbB->angularVelocity : glm::vec3(0.f), rB);
            relVel = velB - velA;

            glm::vec3 tangentVel = relVel - glm::dot(relVel, normal) * normal;
            float tangentSpeed = glm::length(tangentVel);
            if (tangentSpeed < 1e-6f) continue;

            glm::vec3 tangent = tangentVel / tangentSpeed;

            glm::vec3 rAxT = glm::cross(rA, tangent);
            glm::vec3 rBxT = glm::cross(rB, tangent);
            float kTangent = invMassA + invMassB +
                             glm::dot(rAxT, invIA * rAxT) +
                             glm::dot(rBxT, invIB * rBxT);

            if (kTangent <= 0.f) continue;

            float jt = -glm::dot(relVel, tangent) / kTangent;
            jt /= static_cast<float>(numContacts);

            // Coulomb friction cone clamp
            float maxFriction = friction * c.normalImpulseAccum[i];
            jt = glm::clamp(jt, -maxFriction, maxFriction);

            glm::vec3 impulseT = jt * tangent;

            if (rbA) {
                rbA->velocity -= impulseT * invMassA;
                rbA->angularVelocity -= invIA * glm::cross(rA, impulseT);
            }
            if (rbB) {
                rbB->velocity += impulseT * invMassB;
                rbB->angularVelocity += invIB * glm::cross(rB, impulseT);
            }

            // ── Rolling friction ──
            // Damp angular velocity proportional to normal impulse (simulates rolling resistance)
            constexpr float ROLLING_FRICTION = 0.1f;
            float rollingResist = ROLLING_FRICTION * c.normalImpulseAccum[i] /
                                  static_cast<float>(numContacts);

            if (rbA && rbA->mass > 0.f) {
                float angSpeedA = glm::length(rbA->angularVelocity);
                if (angSpeedA > 1e-6f) {
                    float reductionA = std::min(rollingResist * invMassA, angSpeedA);
                    rbA->angularVelocity -= (rbA->angularVelocity / angSpeedA) * reductionA;
                }
            }
            if (rbB && rbB->mass > 0.f) {
                float angSpeedB = glm::length(rbB->angularVelocity);
                if (angSpeedB > 1e-6f) {
                    float reductionB = std::min(rollingResist * invMassB, angSpeedB);
                    rbB->angularVelocity -= (rbB->angularVelocity / angSpeedB) * reductionB;
                }
            }
        }
    }
}

void PhysicsWorld::solvePositionConstraints(std::vector<ContactConstraint> &contacts) {
    constexpr float BAUMGARTE = 0.2f;
    constexpr float SLOP = 0.005f;

    for (auto &c : contacts) {
        auto *rbA = c.objA->rigidBody.get();
        auto *rbB = c.objB->rigidBody.get();

        float invMassA = rbA ? rbA->invMass() : 0.f;
        float invMassB = rbB ? rbB->invMass() : 0.f;
        float totalInvMass = invMassA + invMassB;
        if (totalInvMass == 0.f) continue;

        float penetration = c.manifold.depth;
        float correction = std::max(penetration - SLOP, 0.f) * BAUMGARTE / totalInvMass;

        glm::vec3 corrVec = correction * c.manifold.normal;

        if (rbA && rbA->mass > 0.f)
            c.objA->transform.translation -= invMassA * corrVec;
        if (rbB && rbB->mass > 0.f)
            c.objB->transform.translation += invMassB * corrVec;
    }
}

void PhysicsWorld::updateSleep(MyGameObject::Map &objs, float dt) {
    constexpr float SLEEP_LINEAR_THRESHOLD = 0.1f;
    constexpr float SLEEP_ANGULAR_THRESHOLD = 0.1f;
    constexpr float SLEEP_TIME = 0.5f;

    for (auto &[id, obj] : objs) {
        if (!obj.rigidBody || obj.rigidBody->mass <= 0.f) continue;
        auto *rb = obj.rigidBody.get();
        if (rb->isSleeping) continue;

        float linSpeed = glm::length(rb->velocity);
        float angSpeed = glm::length(rb->angularVelocity);

        if (linSpeed < SLEEP_LINEAR_THRESHOLD && angSpeed < SLEEP_ANGULAR_THRESHOLD) {
            rb->sleepTimer += dt;
            if (rb->sleepTimer >= SLEEP_TIME) {
                rb->isSleeping = true;
                rb->velocity = glm::vec3(0.f);
                rb->angularVelocity = glm::vec3(0.f);
            }
        } else {
            rb->sleepTimer = 0.f;
        }
    }
}

}; // namespace my
