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
class MyBuffer;
class MyTexture;
class MyModel;
class GraphicPipeline;

class OceanRenderSystem {
  public:
    struct WaterWave {
        alignas(16) glm::vec2 direction{1.f, 0.f};
        float frequency{1.f};
        float amplitude{0.5f};
        float steepness{0.5f};
        float speed{1.f};
    };

    struct OceanUbo {
        WaterWave waves[MAX_OCEAN_WAVES];
        alignas(16) glm::vec4 horizonColor{0.05f, 0.3f, 1.f, 1.f};
        alignas(16) glm::vec4 skyColor{1.f, 0.53f, 0.2f, 1.f};
        alignas(16) glm::vec4 deepColor{0.01f, 0.05f, 0.15f, 1.f};
    };

    OceanRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~OceanRenderSystem();

    OceanRenderSystem(const OceanRenderSystem &) = delete;
    OceanRenderSystem &operator=(const OceanRenderSystem &) = delete;

    void renderOcean(FrameInfo &frameInfo);
    void drawGui();
    OceanUbo &getOceanUbo() { return oceanUbo; }

  private:
    void createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createGraphicPipeline(VkRenderPass renderPass);

    void createOceanUboBuffer();
    void createOceanUboPoolAndSetLayout();

    void createOceanTexturePoolAndSetLayout();
    VkDescriptorSet createOceanTextureDescriptorSet(MyTexture &tex);

    std::shared_ptr<MyTexture> oceanPlaneTexture;
    std::shared_ptr<MyModel> oceanPlaneModel;
    OceanUbo oceanUbo{};
    Device &myDevice;

    std::vector<std::unique_ptr<MyBuffer>> uboBuffers;
    std::unique_ptr<MyDescriptorPool> oceanUboPool;
    std::unique_ptr<MyDescriptorSetLayout> oceanUboSetLayout;
    std::vector<VkDescriptorSet> oceanUboDescriptorSet;

    std::unique_ptr<MyDescriptorPool> oceanTexturePool;
    std::unique_ptr<MyDescriptorSetLayout> oceanTextureSetLayout;
    VkDescriptorSet oceanTextureDescriptorSet;

    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;
};

} // namespace my
