#include "my_texture.hpp"
#include "render_core/my_frame_info.hpp"
#include "vulkan_core/my_buffer.hpp"
#include <cassert>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION

namespace my {

// make this an textureInstance so can have multiple image
MyTexture::MyTexture(Device &device) : myDevice{device} {};

MyTexture::~MyTexture() {
    vkDestroyImage(myDevice.device(), textureImage, nullptr);
    vkFreeMemory(myDevice.device(), textureImageMemory, nullptr);
};

void MyTexture::createTextureImage(FrameInfo &frameInfo) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("textures/test1.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    assert(pixels && "cannot load pixel image");

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

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    myDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(texWidth),
                               static_cast<uint32_t>(texHeight), 1);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

// this is currently only support normalish layout transition for picture
void MyTexture::transitionImageLayout(VkImage &image, VkFormat format, VkImageLayout oldLayout,
                                      VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = myDevice.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    } else {
        throw std::runtime_error("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
                         &barrier);

    myDevice.endSingleTimeCommands(commandBuffer);
}

} // namespace my
