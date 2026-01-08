#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 
#include <string>
#include <vulkan/vulkan.h>

namespace my{

class Window
{
public:
    Window(int w, int h, std::string name);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    
    bool shouldClose(){return glfwWindowShouldClose(window);}
    VkExtent2D getExtend() {return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};}
    bool wasWindowResized() {return frameBufferResized; }
    void resetWindowResizedFlag() {frameBufferResized = false; }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
    GLFWwindow *getWindow() const {return window;}

private:
    static void frameBufferResizeCallback(GLFWwindow *window, int width, int height);
    void initwindow();

    int width;
    int height;
    bool frameBufferResized = false;

    std::string windowName;
    GLFWwindow *window;
};

}