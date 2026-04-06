#include "window.hpp"
#include <string>
#include <stdexcept>

namespace my{

Window::Window(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
    initwindow();
}

Window::~Window(){
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::initwindow(){
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
    glfwSetDropCallback(window, dropCallback);
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface){
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS){
        throw std::runtime_error("fail to create surface");
    }
}

void Window::dropCallback(GLFWwindow *window, int count, const char **paths){
    if (count > 0) {
        auto curWindow = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        curWindow->droppedFilePath = paths[0];
    }
}

void Window::frameBufferResizeCallback(GLFWwindow *window, int width, int height){
    auto curWindow = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
    curWindow->frameBufferResized = true;
    curWindow->width = width;
    curWindow->height = height;
}

}