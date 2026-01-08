#pragma once

#include "vulkan_core/window.hpp"
#include "my_game_object.hpp"
#include "vulkan_core/device.hpp"
#include "render_core/my_renderer.hpp"
#include "render_core/my_camera.hpp"
#include "vulkan_core/my_descriptors.hpp"

#include <memory>
#include <vector>

namespace my{

class FirstApp{
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp &) = delete;
        FirstApp& operator=(const FirstApp &) = delete;

        void run();

    private:
        void loadGameObjects();

        Window window{WIDTH, HEIGHT, "cpp is hard"};
        Device device{window};
        MyRenderer myRenderer{window, device};
        // currently using mailbox not vsync(fifo) will change if run into error

        std::unique_ptr<MyDescriptorPool>globalPool;
        MyGameObject::Map gameObjects;
};

}