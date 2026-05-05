#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vulkan/vulkan.h>

namespace my {

class GlfwWindow {
  public:
    GlfwWindow(int w, int h, std::string name);
    ~GlfwWindow();

    GlfwWindow(const GlfwWindow &) = delete;
    GlfwWindow &operator=(const GlfwWindow &) = delete;

    bool shouldClose() { return glfwWindowShouldClose(window); }
    VkExtent2D getExtend() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
    bool wasGlfwWindowResized() { return frameBufferResized; }
    void resetGlfwWindowResizedFlag() { frameBufferResized = false; }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
    GLFWwindow *getWindow() const { return window; }

    bool hasDroppedFile() const { return !droppedFilePath.empty(); }
    std::string consumeDroppedFile() {
        std::string path = droppedFilePath;
        droppedFilePath.clear();
        return path;
    }

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
};

} // namespace my
