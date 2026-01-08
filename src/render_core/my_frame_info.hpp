// add frame relevant data into a single object
#pragma once

#include "render_core/my_camera.hpp"
#include "game/my_game_object.hpp"

#include <vulkan/vulkan.h>

namespace my{

struct FrameInfo{
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    MyCamera &camera;
    VkDescriptorSet globalDescriptorSet;
    MyGameObject::Map &gameObjecs;
};


}