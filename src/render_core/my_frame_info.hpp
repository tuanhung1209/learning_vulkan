// add frame relevant data into a single object
#pragma once

#include "game/my_game_object.hpp"
#include "game/terrain_generation.hpp"
#include "render_core/my_camera.hpp"

namespace my {
#define MAX_LIGHT 10
#define MAX_BULLET 100
#define MAX_OCEAN_WAVES 16
#define MAX_GRASS_GRID 4096

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
    float fogDensity{1.f};
    float time{0.f};
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

struct FogSettingsUbo {
    float near{80.f};
    float far{600.f};
    float density{1.f};
};

struct SkyPush {
    alignas(16) glm::vec4 horizonColor{0.05f, 0.3f, 1.f, 1.f};
    glm::vec4 skyColor{1.f, 0.53f, 0.2f, 1.f};
    glm::vec4 skyTextureColor{0.941f, 0.322f, 0.875f, 1.0f};
    glm::vec4 sunDirection{1.f, 0.f, 0.5f, 0.f};
};

struct WaterWave {
    alignas(16) glm::vec2 direction{1.f, 0.f};
    float frequency{1.f};
    float amplitude{0.5f};
    float steepness{0.5f};
    float speed{1.f};
};

struct OceanUbo {
    WaterWave waves[MAX_OCEAN_WAVES];
    /*
    WaterWave waves[MAX_OCEAN_WAVES] = {

        {normalize(glm::vec2(1.0, 0.0)), 60.0, 1.20, 0.35, 1.0},
        {normalize(glm::vec2(0.9, 0.3)), 45.0, 0.90, 0.30, 1.0},
        {normalize(glm::vec2(0.8, -0.4)), 50.0, 1.00, 0.28, 1.0},
        {normalize(glm::vec2(0.7, 0.6)), 35.0, 0.70, 0.25, 1.0},

        {normalize(glm::vec2(1.0, 0.1)), 12.0, 0.22, 0.55, 1.0},
        {normalize(glm::vec2(0.6, 0.8)), 10.0, 0.18, 0.60, 1.0},
        {normalize(glm::vec2(0.5, -0.7)), 14.0, 0.20, 0.50, 1.0},
        {normalize(glm::vec2(0.9, -0.3)), 8.0, 0.12, 0.65, 1.0},
    };
    */

    alignas(16) glm::vec4 sunDirection{1.f, 0.3f, 0.5f, 1.f};
    alignas(16) glm::vec4 horizonColor{0.05f, 0.3f, 1.f, 1.f};
    alignas(16) glm::vec4 skyColor{1.f, 0.53f, 0.2f, 1.f};
    alignas(16) glm::vec4 deepColor{0.01f, 0.05f, 0.15f, 1.f};
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

struct SceneEntityRef {
    MyGameObject::Map &gameObjects;
    MyGameObject::id_t playerId;
    TerrainGenerator::TerrainConfig &terrainConfig;
    SkyPush &skyConfig;
    GrassComputePush &grassConfig;
    OceanUbo &oceanConfig;
};

} // namespace my
