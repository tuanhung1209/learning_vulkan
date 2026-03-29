#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_texture.hpp"
#include "vulkan_core/compute_pipeline.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/graphic_pipeline.hpp"
#include "vulkan_core/my_descriptors.hpp"

#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

namespace my {

struct GrassComputePush {
    uint32_t bladeCount{2156};
    uint32_t gridWidth{512};
};

class GrassRenderSystem {
  public:
    GrassRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~GrassRenderSystem();

    GrassRenderSystem(const GrassRenderSystem &) = delete;
    GrassRenderSystem &operator=(const GrassRenderSystem &) = delete;

    void computeGrass(FrameInfo &frameInfo);
    void renderGrass(FrameInfo &frameInfo);

  private:
    void createGrassComputePoolAndSetLayout();
    void createComputePipelineLayout();
    void createComputePipeline();

    void createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createGraphicPipeline(VkRenderPass renderPass);

    std::unique_ptr<MyDescriptorPool> grassComputePool;
    std::unique_ptr<MyDescriptorSetLayout> grassComputeSetLayout;
    std::vector<VkDescriptorSet> grassComputeDescriptorSet;
    std::vector<std::unique_ptr<MyBuffer>> grassComputeBuffers;

    std::unique_ptr<ComputePipeline> myComputePipeline;
    VkPipelineLayout computePipelineLayout;

    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;
    std::shared_ptr<MyModel> grassBladeModel;

    GrassComputePush push{};
    Device &myDevice;
};

} // namespace my
