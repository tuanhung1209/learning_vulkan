#include "my_imgui.hpp"
#include "imgui.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "render_core/my_renderer.hpp"
#include "vulkan_core/swap_chain.hpp"
#include <vulkan/vulkan_core.h>

namespace my {

ImGuiWrapper::ImGuiWrapper(Device &device, Window &window, VkRenderPass renderPass)
    : myDevice{device}, myWindow{window}, myRenderPass(renderPass) {
    init();
}

ImGuiWrapper::~ImGuiWrapper() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiWrapper::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiWrapper::renderGui(FrameInfo &frameInfo) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frameInfo.commandBuffer);
}

void ImGuiWrapper::init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(myWindow.getWindow(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = myDevice.getInstance();
    init_info.PhysicalDevice = myDevice.getPhysicaldevice();
    init_info.Device = myDevice.device();
    init_info.QueueFamily = myDevice.findPhysicalQueueFamilies().graphicsFamily;
    init_info.Queue = myDevice.graphicsQueue();
    init_info.DescriptorPoolSize = 100;
    init_info.MinImageCount = 2;
    init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.RenderPass = myRenderPass; // your existing render pass
    ImGui_ImplVulkan_Init(&init_info);
}

} // namespace my
