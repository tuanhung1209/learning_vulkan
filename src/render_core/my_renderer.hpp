#pragma once

#include "platforms/display_provider.hpp"
#include "vulkan_core/render_target.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <cassert>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace my {

class Device;

class MyRenderer {
  public:
    MyRenderer(DisplayProvider &provider, Device &device);
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
    void recreateSwapChains();
    void createRenderTarget();
    void createSyncObjects();

    void blitToSwapChains(VkCommandBuffer cb);

    DisplayProvider &displayProvider_;
    Device &myDevice_;

    std::vector<std::unique_ptr<SwapChain>> swapChains;
    std::unique_ptr<RenderTarget> renderTarget;
    std::vector<VkCommandBuffer> commandBuffers;

    bool isFrameStarted{false};

    std::vector<VkFence> inFlightFences{};
    std::vector<uint32_t> currentImageIndex{};
    size_t currentFrameIndex = 0;
};

} // namespace my
