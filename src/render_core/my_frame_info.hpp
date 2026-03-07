// add frame relevant data into a single object
#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"

#include <vulkan/vulkan.h>

namespace my {
#define MAX_LIGHT 10
#define MAX_BULLET 100

struct PointLight {
    glm::vec4 position{};
    glm::vec4 color{};
};

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::mat4 inverseView{1.0f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
    PointLight pointLights[MAX_LIGHT];
    int numLights;

    alignas(16) glm::vec4 fogColor{0.5f, 0.6f, 0.7f, 1.0f};
    float fogNear{10.f};
    float fogFar{100.f};
};

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    MyCamera &camera;
    VkDescriptorSet globalDescriptorSet;
    MyGameObject::Map &gameObjecs;
};

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

} // namespace my
