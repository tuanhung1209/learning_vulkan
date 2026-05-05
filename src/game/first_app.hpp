#pragma once

#include "my_abundance_object_handler.hpp"
#include "my_game_object.hpp"
#include "platforms/glfw_window.hpp"
#include "platforms/wayland/wayland_window.hpp"
#include "render_core/my_camera.hpp"
#include "render_core/my_renderer.hpp"
#include "vulkan_core/device.hpp"
#include "vulkan_core/my_descriptors.hpp"

#include <GLFW/glfw3.h>
#include <memory>

enum class Mode { Wallpaper, Edit };

namespace my {

class FirstApp {
  public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp(Mode mode);
    ~FirstApp();

    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

  private:
    void loadGameObjects();
    Mode mode_;

    std::unique_ptr<WaylandWindow> waylandWindow;
    std::unique_ptr<GlfwWindow> glfwWindow;

    std::unique_ptr<Device> device{};
    std::unique_ptr<MyRenderer> myRenderer{};

    std::unique_ptr<MyDescriptorPool> globalPool;
    MyGameObject::Map gameObjects;
};

} // namespace my
