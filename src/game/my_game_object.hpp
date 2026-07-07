#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace my {

class MyTexture;
class MyModel;

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

class MyGameObject {
  public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, MyGameObject>;

    static id_t &nextIdRef() {
        static id_t currentId = 0;
        return currentId;
    }

    static MyGameObject createGameObject() { return MyGameObject{nextIdRef()++}; }

    static MyGameObject createGameObjectWithId(id_t id) {
        auto &next = nextIdRef();
        if (id >= next) next = id + 1;
        return MyGameObject(id);
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

    ~MyGameObject();
    std::string modelFilePath{};
    std::shared_ptr<MyModel> model{};
    std::string textureFilePath{};
    std::shared_ptr<MyTexture> texture = nullptr;

    std::unique_ptr<PointLightComponent> pointLight = nullptr;
    std::unique_ptr<BulletComponent> bulletCom = nullptr;

  private:
    MyGameObject(id_t objId) : id{objId} {}

    id_t id;
};

} // namespace my
