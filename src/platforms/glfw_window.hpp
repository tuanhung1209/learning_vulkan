#pragma once

#include "platforms/display_provider.hpp"
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vulkan/vulkan.h>

namespace my {

class GlfwWindow : public DisplayProvider {
  public:
    GlfwWindow(int w, int h, std::string name);
    // have to put destroy somewhere else
    ~GlfwWindow();

    GlfwWindow(const GlfwWindow &) = delete;
    GlfwWindow &operator=(const GlfwWindow &) = delete;

    std::vector<MonitorTarget> getMonitorTarget() override { return {{surface_, getExtent()}}; }
    void waitEvents() override { glfwWaitEvents(); }
    void pollEvents() override { glfwPollEvents(); }
    bool shouldClose() override { return glfwWindowShouldClose(window); }

    VkExtent2D getExtent() const { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
    bool wasGlfwWindowResized() const { return frameBufferResized; }
    GLFWwindow *getWindow() const { return window; }
    VkSurfaceKHR getVkSurface() const { return surface_; }
    bool hasDroppedFile() const { return !droppedFilePath.empty(); }

    std::string consumeDroppedFile() {
        std::string path = droppedFilePath;
        droppedFilePath.clear();
        return path;
    }

    void resetGlfwWindowResizedFlag() { frameBufferResized = false; }
    void createVulkanSurface(VkInstance instance);
    void destroyVulkanSurfaces(VkInstance instance);

  private:
    static void frameBufferResizeCallback(GLFWwindow *window, int width, int height);
    static void dropCallback(GLFWwindow *window, int count, const char **paths);
    void initwindow();

    int width;
    int height;
    bool frameBufferResized = false;
    std::string droppedFilePath;

    std::string windowName;
    GLFWwindow *window;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
};

} // namespace my
