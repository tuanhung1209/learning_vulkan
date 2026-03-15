#pragma once

#include "render_core/my_frame_info.hpp"
#include "render_core/my_renderer.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/my_descriptors.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

namespace my {

class ImGuiWrapper {
  public:
    ImGuiWrapper(Device &device, Window &window, VkRenderPass renderPass);
    ~ImGuiWrapper();

    void newFrame();

    void renderGui(FrameInfo &frameInfo);

  private:
    void init();

    Window &myWindow;
    Device &myDevice;
    VkRenderPass myRenderPass;
};

}; // namespace my
