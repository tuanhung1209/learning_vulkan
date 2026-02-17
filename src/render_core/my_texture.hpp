#pragma once

#include "vulkan_core/device.hpp"
#include <string>
#include <vulkan/vulkan_core.h>

namespace my {

class MyTexture {
  public:
    MyTexture(Device &myDevice, const std::string filePath);
    ~MyTexture();

    VkImageView getTextureImageView() { return textureImageView; }
    VkSampler getTextureSampler() { return textureSampler; }

  private:
    void createTextureImage(const std::string filePath);
    void createTextureImageView();
    void createTextureSampler();

    Device &myDevice;

    VkImage textureImage = nullptr;
    VkDeviceMemory textureImageMemory = nullptr;
    VkImageView textureImageView = nullptr;
    VkSampler textureSampler = nullptr;
};

} // namespace my
