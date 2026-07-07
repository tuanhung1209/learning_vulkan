#pragma once

#include "platforms/glfw_window.hpp"
#include "render_core/my_frame_info.hpp"
#include "vulkan_core/device.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

namespace my {

class ImGuiWrapper {
  public:
    ImGuiWrapper(Device &device, GlfwWindow &window, VkRenderPass renderPass);
    ~ImGuiWrapper();

    void newFrame();

    void renderGui(FrameInfo &frameInfo);

  private:
    void init();

    GlfwWindow &myWindow;
    Device &myDevice;
    VkRenderPass myRenderPass;
};

}; // namespace my
