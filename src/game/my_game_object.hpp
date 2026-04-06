#pragma once

#include "render_core/my_model.hpp"
#include "render_core/my_texture.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>

namespace my {
struct TransformComponent {
    glm::vec3 translation{}; // offset
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::vec3 rotation{};

    glm::mat4 mat4();
    glm::mat3 normalMatrix();
};

struct PointLightComponent {
    float lightIntensity = 1.0f;
};

struct BulletComponent {
    glm::vec3 velocity;
    float lifeTime;
    bool isActive = false;
};

struct RigidBodyComponent {
    glm::vec3 velocity{};
    glm::vec3 angularVelocity{};
    float mass{1.0f};           // 0 = static/immovable
    float restitution{0.2f};    // Bounciness (material property)
    float friction{0.6f};       // Friction coefficient
    float linearDamping{0.5f};  // Per-second linear damping factor
    float angularDamping{2.0f}; // Per-second angular damping factor

    // Inverse inertia tensor diagonal (local space, for boxes)
    glm::vec3 invInertiaDiag{0.f};

    bool isSleeping{false};
    float sleepTimer{0.0f};

    float invMass() const { return mass > 0.f ? 1.f / mass : 0.f; }

    // Transform local inverse inertia to world space via rotation matrix
    glm::mat3 invInertiaWorld(const glm::mat3 &rot) const {
        if (mass <= 0.f) return glm::mat3(0.f);
        glm::mat3 localInvI(0.f);
        localInvI[0][0] = invInertiaDiag.x;
        localInvI[1][1] = invInertiaDiag.y;
        localInvI[2][2] = invInertiaDiag.z;
        return rot * localInvI * glm::transpose(rot);
    }

    // Compute inverse inertia for a box given its half-extents (scale * 0.5)
    void computeBoxInertia(const glm::vec3 &halfExtents) {
        if (mass <= 0.f) {
            invInertiaDiag = glm::vec3(0.f);
            return;
        }
        float x2 = 4.f * halfExtents.x * halfExtents.x;
        float y2 = 4.f * halfExtents.y * halfExtents.y;
        float z2 = 4.f * halfExtents.z * halfExtents.z;
        float factor = mass / 12.f;
        invInertiaDiag.x = 1.f / (factor * (y2 + z2));
        invInertiaDiag.y = 1.f / (factor * (x2 + z2));
        invInertiaDiag.z = 1.f / (factor * (x2 + y2));
    }
};

class MyGameObject {
  public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, MyGameObject>;

    static MyGameObject createGameObject() {
        static id_t currentId = 0;
        return MyGameObject{currentId++};
    }

    static MyGameObject createPointLight(float intensity = 10.f, float radius = 0.1f,
                                         glm::vec3 color = glm::vec3(1.f));

    MyGameObject(const MyGameObject &) = delete;
    MyGameObject &operator=(const MyGameObject &) = delete;
    MyGameObject(MyGameObject &&) = default;
    MyGameObject &operator=(MyGameObject &&) = default;

    id_t const getId() { return id; }
    std::shared_ptr<MyTexture> const getTexture() { return texture; }

    glm::vec3 color{};
    TransformComponent transform{};

    std::string modelFilePath{};
    std::shared_ptr<MyModel> model{};
    std::shared_ptr<MyTexture> texture = nullptr;

    std::unique_ptr<PointLightComponent> pointLight = nullptr;
    std::unique_ptr<BulletComponent> bulletCom = nullptr;
    std::unique_ptr<RigidBodyComponent> rigidBody = nullptr;

  private:
    MyGameObject(id_t objId) : id{objId} {}

    id_t id;
};

} // namespace my
