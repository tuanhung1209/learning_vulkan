#pragma once

#include "render_core/my_frame_info.hpp"
#include "vulkan_core/my_descriptors.hpp"

#include <memory>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/common.hpp>
#include <glm/glm.hpp>

namespace my {

class Device;
class MyModel;
class MyBuffer;
class GraphicPipeline;
class ComputePipeline;

class GrassRenderSystem {
  public:
    struct alignas(16) GrassTransformData {
        glm::vec4 translation{};
        glm::vec2 scale{1.f};
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

    GrassRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~GrassRenderSystem();

    GrassRenderSystem(const GrassRenderSystem &) = delete;
    GrassRenderSystem &operator=(const GrassRenderSystem &) = delete;

    void computeGrass(FrameInfo &frameInfo);
    void renderGrass(FrameInfo &frameInfo);

    void drawGui(float terrainHeightScale);
    void updateHeightMap(const std::vector<float> &heightMap, float heightScale);

    GrassComputePush &getPush() { return push; }

  private:
    void createComputePoolAndSetLayout();
    void createGrassComputeBuffer();
    void createHeightComputeBuffer();
    void createIndirectDrawBuffer();

    void createComputePipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createComputePipeline();

    void createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createGraphicPipeline(VkRenderPass renderPass);

    std::unique_ptr<MyDescriptorPool> grassComputePool;
    std::unique_ptr<MyDescriptorSetLayout> grassComputeSetLayout;
    std::vector<VkDescriptorSet> grassComputeDescriptorSet;

    std::vector<std::unique_ptr<MyBuffer>> grassComputeBuffers;
    std::vector<std::unique_ptr<MyBuffer>> visibleGrassBuffers;
    std::vector<std::unique_ptr<MyBuffer>> indirectDrawBuffers;
    std::vector<std::unique_ptr<MyBuffer>> heightComputeBuffers;

    std::unique_ptr<ComputePipeline> myComputePipeline;
    VkPipelineLayout computePipelineLayout;

    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;

    std::shared_ptr<MyModel> grassBladeModel;

    GrassComputePush push{};
    Device &myDevice;
};

} // namespace my
