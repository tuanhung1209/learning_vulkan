#include "glfw_window.hpp"
#include <stdexcept>
#include <string>

namespace my {

GlfwWindow::GlfwWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
    initwindow();
}

GlfwWindow::~GlfwWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GlfwWindow::initwindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
    glfwSetDropCallback(window, dropCallback);
}

void GlfwWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("fail to create surface");
    }
}

void GlfwWindow::dropCallback(GLFWwindow *window, int count, const char **paths) {
    if (count > 0) {
        auto curGlfwWindow = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
        curGlfwWindow->droppedFilePath = paths[0];
    }
}

void GlfwWindow::frameBufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto curGlfwWindow = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
    curGlfwWindow->frameBufferResized = true;
    curGlfwWindow->width = width;
    curGlfwWindow->height = height;
}

} // namespace my
