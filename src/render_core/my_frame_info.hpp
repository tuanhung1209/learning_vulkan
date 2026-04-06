// add frame relevant data into a single object
#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"

namespace my {
#define MAX_LIGHT 10
#define MAX_BULLET 100
#define MAX_GRASS_GRID 2048

struct alignas(16) GrassTransformData {
    glm::vec4 translation{};
    glm::vec2 scale{1.f};
};

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
    float fogNear{300.f};
    float fogFar{1500.f};
    float time{0.f};
};

struct SkyUbo {
    alignas(16) glm::vec4 horizonColor{0.05f, 0.3f, 1.f, 1.f};
    glm::vec4 skyColor{1.f, 0.53f, 0.2f, 1.f};
    glm::vec4 skyTextureColor{0.941f, 0.322f, 0.875f, 1.0f};
    glm::vec4 sunDirection{1.f, 0.f, 0.5f, 0.f};
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

struct GrassComputePush {
    // Grass
    uint32_t gridSize{512};
    uint32_t terrainResolution{512};
    float heightScale{120.0f};
    float spacing{0.125f};
    float bladeHeight{1.0f};
    // Wind
    float windDirX{1.0f};
    float windDirZ{0.3f};
    float windFreq{1.15f};
    float windAmplitude{0.8f};
    float turbPower{0.4f};
    float turbSize{0.03f};
    float droopStrength{0.35f};
    float xPeriod{0.05f};
    float yPeriod{0.1f};
    float windBias{0.65f};
    // Color
    alignas(16) glm::vec4 baseColor{0.02f, 0.18f, 0.01f, 1.0f};
    alignas(16) glm::vec4 tipColor{0.1f, 0.55f, 0.08f, 1.0f};
};

} // namespace my
