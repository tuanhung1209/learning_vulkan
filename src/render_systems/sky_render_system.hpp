#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_model.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/graphic_pipeline.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <memory>

namespace my {

class SkyRenderSystem {
  public:
    SkyRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~SkyRenderSystem();

    SkyRenderSystem(const SkyRenderSystem &) = delete;
    SkyRenderSystem &operator=(const SkyRenderSystem &) = delete;

    void renderSky(FrameInfo &frameInfo);
    SkyPush &getPush() { return push; }
    const FogSettingsUbo &getFog() const { return fog; }

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
    FogSettingsUbo fog{};
    std::shared_ptr<MyModel> skyModel;
    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;
};

} // namespace my
