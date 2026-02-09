#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_frame_info.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/pipeline.hpp"

#include <memory>

namespace my {

class PointLightSystem {
  public:
    PointLightSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~PointLightSystem();

    PointLightSystem(const PointLightSystem &) = delete;
    PointLightSystem &operator=(const PointLightSystem &) = delete;

    void update(FrameInfo &frameInfo, GlobalUbo &ubo);
    void renderLight(FrameInfo &frameInfo);

  private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

    Device &myDevice;

    std::unique_ptr<PipeLine> myPipeLine;
    VkPipelineLayout pipelineLayout;
};

} // namespace my
