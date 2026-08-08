#pragma once

#include "render_core/my_frame_info.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "render_core/asset_cache.hpp"

#include <memory>
#include <unordered_map>

namespace my {

class Device;
class MyTexture;
class GraphicPipeline;

class SimpleRenderSystem {
  public:
    SimpleRenderSystem(Device &device, AssetCache &assetCache, VkRenderPass renderPass,
                       VkDescriptorSetLayout globalSetLayout);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(FrameInfo &frameInfo);

    VkPipelineLayout getGraphicPipelineLayout() const { return graphicPipelineLayout; }

  private:
    void createTexturePoolAndSetLayout();
    void createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createGraphicPipeline(VkRenderPass renderPass);
    VkDescriptorSet getOrCreateTextureDescriptorSet(MyTexture &tex);

    Device &myDevice_;
    AssetCache &assetCache_;

    std::shared_ptr<MyTexture> defaultWhiteTexture;

    std::unique_ptr<MyDescriptorPool> texturePool;
    std::unique_ptr<MyDescriptorSetLayout> textureSetLayout;
    std::unordered_map<MyTexture *, VkDescriptorSet> textureDescriptorSets;

    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;
};

} // namespace my
