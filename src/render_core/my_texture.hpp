#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace my {

class Device;

class MyTexture {
  public:
    MyTexture(Device &myDevice, const std::string filePath);
    MyTexture(Device &myDevice, uint32_t width, uint32_t height, const std::vector<uint8_t> &pixels);
    ~MyTexture();

    MyTexture &operator=(const MyTexture &) = delete;
    MyTexture(const MyTexture &) = delete;

    VkImageView getTextureImageView() { return textureImageView; }
    VkSampler getTextureSampler() { return textureSampler; }

  private:
    void createTextureImage(Device &myDevice, uint32_t width, uint32_t height, const uint8_t *pixels);
    void createTextureImageView();
    void createTextureSampler();

    Device &myDevice;

    VkImage textureImage = nullptr;
    VkDeviceMemory textureImageMemory = nullptr;
    VkImageView textureImageView = nullptr;
    VkSampler textureSampler = nullptr;
};

} // namespace my
