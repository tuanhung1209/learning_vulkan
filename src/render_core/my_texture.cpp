#include "my_texture.hpp"

#include "vulkan_core/device.hpp"
#include "vulkan_core/my_buffer.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace my {

// make this an textureInstance so can have multiple image
MyTexture::MyTexture(Device &device, const std::string filepath) : myDevice{device} {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    assert(pixels && "cannot load pixel image");

    createTextureImage(myDevice, texWidth, texHeight, pixels);
    stbi_image_free(pixels);

    createTextureImageView();
    createTextureSampler();
}

MyTexture::MyTexture(Device &device, uint32_t width, uint32_t height, const std::vector<uint8_t> &pixels)
    : myDevice{device} {
    createTextureImage(myDevice, width, height, pixels.data());
    createTextureImageView();
    createTextureSampler();
}

MyTexture::~MyTexture() {
    vkDestroyImage(myDevice.device(), textureImage, nullptr);
    vkDestroySampler(myDevice.device(), textureSampler, nullptr);
    vkDestroyImageView(myDevice.device(), textureImageView, nullptr);
    vkFreeMemory(myDevice.device(), textureImageMemory, nullptr);
};

void MyTexture::createTextureImage(Device &device, uint32_t width, uint32_t height, const uint8_t *pixels) {
    MyBuffer stagingBuffer{myDevice, 4, static_cast<uint32_t>(width * height),
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void *)pixels);

    // may create helper function to do with imageInfo creation
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
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

    myDevice.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    myDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height), 1);
    myDevice.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void MyTexture::createTextureImageView() {
    textureImageView = myDevice.createImageViewWithInfo(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}

void MyTexture::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = myDevice.properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    if (vkCreateSampler(myDevice.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("can create texture image sampler");
    }
}

} // namespace my
