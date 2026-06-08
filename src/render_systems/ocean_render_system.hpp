#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_model.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/graphic_pipeline.hpp"
#include "vulkan_core/my_descriptors.hpp"

#include <memory>
#include <vector>

namespace my {

class OceanRenderSystem {
  public:
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
