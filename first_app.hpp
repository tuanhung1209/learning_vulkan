#pragma once

#include "window.hpp"
#include "my_game_object.hpp"
#include "device.hpp"
#include "my_renderer.hpp"
#include "my_camera.hpp"
#include "my_descriptors.hpp"

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