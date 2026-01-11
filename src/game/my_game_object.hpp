#pragma once

#include "render_core/my_model.hpp"

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

struct PointLightComponent{
    float lightIntensity = 1.0f;
};

class MyGameObject{
    public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, MyGameObject>;

    static MyGameObject createGameObject() {
        static id_t currentId = 0;
        return MyGameObject{currentId++};
    }

    static MyGameObject createPointLight (float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

    MyGameObject(const MyGameObject &) = delete;
    MyGameObject &operator = (const MyGameObject &) = delete;
    MyGameObject(MyGameObject &&) = default;
    MyGameObject &operator = (MyGameObject &&) = default;

    id_t const getId() { return id;};

    glm::vec3 color{};   

    TransformComponent transform{};

    std::shared_ptr<MyModel> model{}; 
    std::unique_ptr<PointLightComponent> pointLight = nullptr;

    private:
    MyGameObject(id_t objId) : id{objId} {}

    id_t id;
};

struct gameObjectBulletInfo{
    MyGameObject::id_t gameObjectId;
    glm::vec3 velocity;
    float lifeTime;
};

}