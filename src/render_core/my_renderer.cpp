// #include <GLFW/glfw3.h>
#include "render_core/my_renderer.hpp"
#include "vulkan_core/render_target.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace my {

MyRenderer::MyRenderer(std::function<VkExtent2D()> getExtent, std::function<void()> waitEvents,
                       Device &device)
    : getExtentFn{std::move(getExtent)}, waitEventsFn{std::move(waitEvents)}, myDevice{device} {
    recreateSwapChain();
    createRenderTarget();
    createCommandBuffers();
}

MyRenderer::~MyRenderer() { freeCommandBuffers(); }

void MyRenderer::createRenderTarget() {
    renderTarget.reset();
    renderTarget = std::make_unique<RenderTarget>(myDevice, mySwapChain->getSwapChainExtent(),
                                                  mySwapChain->getSwapChainImageFormat());
}

void MyRenderer::recreateSwapChain() {
    auto extend = getExtentFn();
    while (extend.width == 0 || extend.height == 0) {
        waitEventsFn();
        extend = getExtentFn();
    }

    vkDeviceWaitIdle(myDevice.device());
    if (mySwapChain == nullptr) {
        mySwapChain = std::make_unique<SwapChain>(myDevice, extend);
    } else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(mySwapChain);
        mySwapChain = std::make_unique<SwapChain>(myDevice, extend, oldSwapChain);

        if (!oldSwapChain->compareSwapChain(*mySwapChain.get())) {
            throw std::runtime_error("SwapChain image(depth) format have changed");
        }
    }
    createRenderTarget();
}

void MyRenderer::createCommandBuffers() {
    commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = myDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(myDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("unable to create(allocate) command buffer");
    }
}

void MyRenderer::freeCommandBuffers() {
    vkFreeCommandBuffers(myDevice.device(), myDevice.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

VkCommandBuffer MyRenderer::beginFrame() {
    assert(!isFrameStarted && "cant call beginFrame while already in progress");
    auto result = mySwapChain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to aquire swap chain image");
    }

    isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("faied to begin recording command buffer");
    }

    return commandBuffer;
}

void MyRenderer::endFrame() {
    assert(isFrameStarted && "can't not end frame when frame is not in progress");
    auto commandBuffer = getCurrentCommandBuffer();

    blitToSwapChain(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("fail to end command buffer");
    }

    auto result = mySwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }

    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
    isFrameStarted = false;
}

void MyRenderer::blitToSwapChain(VkCommandBuffer cb) {
    VkImage src = renderTarget->getColorImage(currentFrameIndex);
    VkImage dst = mySwapChain->getSwapChainImage(currentImageIndex);
    VkExtent2D srcExtent = renderTarget->getExtent();
    VkExtent2D dstExtent = mySwapChain->getSwapChainExtent();

    VkImageMemoryBarrier startBarrier{};
    startBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    startBarrier.image = dst;
    startBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    startBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    startBarrier.srcAccessMask = 0;
    startBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    startBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    startBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    startBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &startBarrier);

    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {(int32_t)srcExtent.width, (int32_t)srcExtent.height, 1};
    blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {(int32_t)dstExtent.width, (int32_t)dstExtent.height, 1};

    vkCmdBlitImage(cb, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, VK_FILTER_LINEAR);

    VkImageMemoryBarrier endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    endBarrier.image = dst;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    endBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    endBarrier.dstAccessMask = 0;
    endBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    endBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    endBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &endBarrier);
}

void MyRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "cannot begin swapchain if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "cannot begin renderpass on command buffer from a diffrent frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderTarget->getRenderPass();
    renderPassInfo.framebuffer = renderTarget->getFrameBuffer(currentFrameIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = renderTarget->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.05f, 0.1f, 0.3f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(renderTarget->getExtent().width);
    viewport.height = static_cast<float>(renderTarget->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, renderTarget->getExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void MyRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "cannot call begin swapchain if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "cannot begin renderpass on command buffer from a diffrent frame");

    vkCmdEndRenderPass(commandBuffer);
}
} // namespace my
