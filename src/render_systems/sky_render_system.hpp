#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_model.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/graphic_pipeline.hpp"
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
    void updateUbo(FrameInfo &frameInfo, SkyUbo &skyUbo);

  private:
    void createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createGraphicPipeline(VkRenderPass renderPass);
    void createSkyTexturePoolAndSetLayout();
    void createSkyUboPoolAndSetLayout();
    VkDescriptorSet createSkyDescriptorSet(MyTexture &tex);

    std::shared_ptr<MyTexture> skyTexture;
    Device &myDevice;

    std::unique_ptr<MyDescriptorPool> skyTexturePool;
    std::unique_ptr<MyDescriptorSetLayout> skyTextureSetLayout;
    VkDescriptorSet skyTextureDescriptorSet;

    std::unique_ptr<MyDescriptorPool> skyUboPool;
    std::unique_ptr<MyDescriptorSetLayout> skyUboSetLayout;
    std::vector<std::unique_ptr<MyBuffer>> skyUboBuffers;
    std::vector<VkDescriptorSet> skyUboDescriptorSet;

    std::shared_ptr<MyModel> skyModel;
    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;
};

} // namespace my
