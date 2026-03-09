#pragma once

#include "game/my_game_object.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_model.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/pipeline.hpp"

#include <memory>

namespace my {

class SkyRenderSystem {
  public:
    SkyRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~SkyRenderSystem();

    SkyRenderSystem(const SkyRenderSystem &) = delete;
    SkyRenderSystem &operator=(const SkyRenderSystem &) = delete;

    void renderSky(FrameInfo &frameInfo);

  private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

    Device &myDevice;

    std::shared_ptr<MyModel> skyModel;
    std::unique_ptr<PipeLine> myPipeLine;
    VkPipelineLayout pipelineLayout;
};

} // namespace my
