#pragma once

#include "vulkan_core/device.hpp"
#include <vulkan/vulkan_core.h>

namespace my {

class RenderTarget {

  public:
    RenderTarget(Device &device, VkExtent2D extent, VkFormat colorFormat);
    ~RenderTarget();

    RenderTarget(const RenderTarget &) = delete;
    RenderTarget &operator=(const RenderTarget &) = delete;

    VkRenderPass getRenderPass() const { return renderPass; }

    VkFramebuffer getFrameBuffer(int i) const { return framebuffers[i]; }
    VkImage getColorImage(int i) const { return colorImages[i]; }

    VkExtent2D getExtent() { return extent; }
    float extentAspectRatio() { return static_cast<float>(extent.width) / static_cast<float>(extent.height); }

    VkFormat findDepthFormat();

  private:
    void createRenderPass();
    void createDepthResources();
    void createColorResources();
    void createFramebuffers();

    Device &myDevice;
    VkExtent2D extent;
    VkFormat colorFormat;
    VkFormat depthFormat;
    VkRenderPass renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;

    std::vector<VkImage> colorImages;
    std::vector<VkDeviceMemory> colorImageMemorys;
    std::vector<VkImageView> colorImageViews;

    std::vector<VkFramebuffer> framebuffers;
};

} // namespace my
