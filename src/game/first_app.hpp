#pragma once

#include "my_abundance_object_handler.hpp"
#include "my_game_object.hpp"
#include "platfroms/wayland/wayland_window.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_renderer.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/window.hpp"

#include <GLFW/glfw3.h>
#include <memory>

namespace my {

class FirstApp {
  public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();
    ~FirstApp();

    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

  private:
    void loadGameObjects();

    WaylandWindow window1{"wallpaper"};
    Window window{WIDTH, HEIGHT, "cpp is hard"};
    Device device{[this](VkInstance inst) {
        VkSurfaceKHR s;
        window.createWindowSurface(inst, &s);
        return s;
    }};
    MyRenderer myRenderer{[this] { return window.getExtend(); }, [] { glfwWaitEvents(); }, device};
    // may change back to mail box but vsync is power saving ?

    std::unique_ptr<MyDescriptorPool> globalPool;
    MyGameObject::Map gameObjects;
};

} // namespace my
