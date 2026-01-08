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

class MyGameObject{
    public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, MyGameObject>;

    static MyGameObject createGameObject() {
        static id_t currentId = 0;
        return MyGameObject{currentId++};
    }

    MyGameObject(const MyGameObject &) = delete;
    MyGameObject &operator = (const MyGameObject &) = delete;
    MyGameObject(MyGameObject &&) = default;
    MyGameObject &operator = (MyGameObject &&) = default;

    id_t const getId() { return id;};

    std::shared_ptr<MyModel> model{}; 
    glm::vec3 color{};   

    TransformComponent transform{};

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