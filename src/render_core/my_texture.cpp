#include "my_texture.hpp"
#include "render_core/my_frame_info.hpp"
#include "vulkan_core/my_buffer.hpp"
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION

namespace my {

MyTexture::MyTexture(Device &device) : myDevice{device} {};

MyTexture::~MyTexture() {
    vkDestroyImage(myDevice.device(), textureImage, nullptr);
    vkFreeMemory(myDevice.device(), textureImageMemory, nullptr);
};

void MyTexture::createTextureImage(FrameInfo &frameInfo) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("textures/test1.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    // VkDeviceSize imageSize = texWidth * texHeight * 4;

    assert(pixels && "cannot load pixel image");

    // will need to refracto this to later
    MyBuffer stagingBuffer{myDevice, 4, static_cast<uint32_t>(texWidth * texHeight),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(pixels);

    stbi_image_free(pixels);

    // may create helper function to do with imageInfo creation
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB; // VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.flags = 0;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    myDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage,
                                 textureImageMemory);

    myDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(texWidth),
                               static_cast<uint32_t>(texHeight), 1);
}

} // namespace my
