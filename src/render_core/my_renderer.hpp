#pragma once

#include "vulkan_core/device.hpp"
#include "vulkan_core/render_target.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace my {

class MyRenderer {
  public:
    MyRenderer(std::function<VkExtent2D()> getExtent, std::function<void()> waitEvents, Device &device);
    ~MyRenderer();

    MyRenderer(const MyRenderer &) = delete;
    MyRenderer &operator=(const MyRenderer &) = delete;

    VkRenderPass getSceneRenderPass() const { return renderTarget->getRenderPass(); }
    float getAspectRatio() { return renderTarget->extentAspectRatio(); }

    bool isFrameInProgress() const { return isFrameStarted; }
    VkCommandBuffer getCurrentCommandBuffer() const {
        assert(isFrameStarted && "cannot get framebuffer when frame not in progress");
        return commandBuffers[currentFrameIndex];
    }

    int getFrameIndex() const {
        assert(isFrameStarted && "cannot get frame index when frame not in progress");
        return currentFrameIndex;
    }

    VkCommandBuffer beginFrame();
    void endFrame();
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

  private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();
    void createRenderTarget();

    void blitToSwapChain(VkCommandBuffer cb);

    std::function<VkExtent2D()> getExtentFn;
    std::function<void()> waitEventsFn;
    Device &myDevice;
    // currently using mailbox not vsync(fifo) will change if run into error
    // use pointer to easily delete and recreate for window resize

    std::unique_ptr<RenderTarget> renderTarget;
    std::unique_ptr<SwapChain> mySwapChain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    bool isFrameStarted{false};
};

} // namespace my
