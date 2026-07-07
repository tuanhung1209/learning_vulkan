#pragma once

#include "render_core/my_frame_info.hpp"
#include "vulkan_core/my_descriptors.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/common.hpp>
#include <glm/glm.hpp>

namespace my {

class Device;
class MyModel;
class MyTexture;
class GraphicPipeline;

class SkyRenderSystem {
  public:
    struct FogSettings {
        float near{80.f};
        float far{600.f};
        float density{1.f};
        glm::vec4 fogColor{0.5f, 0.6f, 0.7f, 1.0f};
    };

    struct SkyPush {
        glm::vec4 skyColor{1.f, 0.53f, 0.2f, 1.f};
        glm::vec4 skyTextureColor{0.941f, 0.322f, 0.875f, 1.0f};
    };

    SkyRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~SkyRenderSystem();

    SkyRenderSystem(const SkyRenderSystem &) = delete;
    SkyRenderSystem &operator=(const SkyRenderSystem &) = delete;

    SkyPush &getPush() { return push; }
    const FogSettings &getFog() const { return fog; }

    void renderSky(FrameInfo &frameInfo);
    void drawGui();

  private:
    void createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createGraphicPipeline(VkRenderPass renderPass);
    void createSkyTexturePoolAndSetLayout();
    VkDescriptorSet createSkyTextureDescriptorSet(MyTexture &tex);

    std::shared_ptr<MyTexture> skyTexture;
    Device &myDevice;

    std::unique_ptr<MyDescriptorPool> skyTexturePool;
    std::unique_ptr<MyDescriptorSetLayout> skyTextureSetLayout;
    VkDescriptorSet skyTextureDescriptorSet;

    SkyPush push{};
    FogSettings fog{};
    std::shared_ptr<MyModel> skyModel;
    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;
};

} // namespace my
