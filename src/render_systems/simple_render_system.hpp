#pragma once

#include "vulkan_core/pipeline.hpp"
#include "game/my_game_object.hpp"
#include "vulkan_core/device.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_frame_info.hpp"

#include <memory>
#include <vector>

namespace my{

class SimpleRenderSystem{
public:
        SimpleRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem& operator=(const SimpleRenderSystem &) = delete;

        void renderGameObjects(FrameInfo &frameInfo);
    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        Device &myDevice;

        std::unique_ptr<PipeLine> myPipeLine;
        VkPipelineLayout pipelineLayout;
};

}