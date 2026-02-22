#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_texture.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/pipeline.hpp"

#include <memory>
#include <unordered_map>

namespace my {

class SimpleRenderSystem {
  public:
    SimpleRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(FrameInfo &frameInfo);

    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

  private:
    void createTexturePoolAndSetLayout();
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);
    VkDescriptorSet getOrCreateTextureDescriptorSet(MyTexture &tex);

    Device &myDevice;

    std::shared_ptr<MyTexture> defaultWhiteTexture;

    std::unique_ptr<MyDescriptorPool> texturePool;
    std::unique_ptr<MyDescriptorSetLayout> textureSetLayout;
    std::unordered_map<MyTexture *, VkDescriptorSet> textureDescriptorSets;

    std::unique_ptr<PipeLine> myPipeLine;
    VkPipelineLayout pipelineLayout;
};

} // namespace my
