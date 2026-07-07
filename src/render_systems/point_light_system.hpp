#pragma once

#include "render_core/my_frame_info.hpp"

#include <memory>

namespace my {

class Device;
class GraphicPipeline;

class PointLightSystem {
  public:
    PointLightSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~PointLightSystem();

    PointLightSystem(const PointLightSystem &) = delete;
    PointLightSystem &operator=(const PointLightSystem &) = delete;

    void update(FrameInfo &frameInfo, GlobalUbo &ubo);
    void renderLight(FrameInfo &frameInfo);

  private:
    void createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createGraphicPipeline(VkRenderPass renderPass);

    Device &myDevice;

    std::unique_ptr<GraphicPipeline> myGraphicPipeline;
    VkPipelineLayout graphicPipelineLayout;
};

} // namespace my
