#pragma once

#include "render_core/my_frame_info.hpp"
#include "vulkan_core/device.hpp"
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace my {

class MyTexture {
  public:
    MyTexture(Device &myDevice);
    ~MyTexture();

  private:
    void createTextureImage(FrameInfo &frameInfo);

    Device &myDevice;

    VkImage textureImage = nullptr;
    VkDeviceMemory textureImageMemory = nullptr;
};

} // namespace my
