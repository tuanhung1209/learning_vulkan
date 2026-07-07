#include "render_core/my_renderer.hpp"

#include "vulkan_core/device.hpp"
#include "vulkan_core/render_target.hpp"
#include "vulkan_core/swap_chain.hpp"
#include "platforms/display_provider.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace my {

MyRenderer::MyRenderer(DisplayProvider &provider, Device &device)
    : displayProvider_{provider}, myDevice_{device} {
    recreateSwapChains();
    createCommandBuffers();
    createSyncObjects();
}

MyRenderer::~MyRenderer() {
    freeCommandBuffers();
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(myDevice_.device(), inFlightFences[i], nullptr);
    }
}

void MyRenderer::createRenderTarget() {
    VkExtent2D maxExtent{};

    for (auto &sc : swapChains) {
        maxExtent.width = std::max(maxExtent.width, sc->getSwapChainExtent().width);
        maxExtent.height = std::max(maxExtent.height, sc->getSwapChainExtent().height);
    }

    renderTarget =
        std::make_unique<RenderTarget>(myDevice_, maxExtent, swapChains[0]->getSwapChainImageFormat());
}

void MyRenderer::recreateSwapChains() {
    std::vector<MonitorTarget> monitorTargets = displayProvider_.getMonitorTarget();

    for (int i = 0; i < monitorTargets.size(); i++) {
        while (monitorTargets[i].extent.width == 0 || monitorTargets[i].extent.height == 0) {
            displayProvider_.waitEvents();
            monitorTargets = displayProvider_.getMonitorTarget();
        }
    }

    vkDeviceWaitIdle(myDevice_.device());
    swapChains.clear();

    for (MonitorTarget &mt : monitorTargets) {
        swapChains.push_back(std::make_unique<SwapChain>(myDevice_, mt.extent, mt.vkSurface));
    }

    // TODO : change when swapChain is no longer campatable (change resolution for glfw)

    /*
    for (auto &sc : swapChains) {
        if (sc == nullptr){

        }
    }

    if (swapChains == nullptr) {
        swapChains = std::make_unique<SwapChain>(myDevice_, extend, surface);
    } else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChains);
        swapChains = std::make_unique<SwapChain>(myDevice_, extend, surface, oldSwapChain);

        if (!oldSwapChain->compareSwapChain(*swapChains.get())) {
            throw std::runtime_error("SwapChain image(depth) format have changed");
        }
    }
    */

    currentImageIndex.resize(swapChains.size());
    createRenderTarget();
}

void MyRenderer::createCommandBuffers() {
    commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = myDevice_.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(myDevice_.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("unable to create(allocate) command buffer");
    }
}

void MyRenderer::freeCommandBuffers() {
    vkFreeCommandBuffers(myDevice_.device(), myDevice_.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

VkCommandBuffer MyRenderer::beginFrame() {
    assert(!isFrameStarted && "cant call beginFrame while already in progress");

    vkWaitForFences(myDevice_.device(), 1, &inFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX);

    for (size_t i = 0; i < swapChains.size(); i++) {
        VkResult result = swapChains[i]->acquireNextImage(&currentImageIndex[i], currentFrameIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChains();
            return nullptr;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to aquire swap chain image");
        }
    }

    vkResetFences(myDevice_.device(), 1, &inFlightFences[currentFrameIndex]);

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

    blitToSwapChains(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("fail to end command buffer");
    }

    std::vector<VkSemaphore> waitSemaphores, signalSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;

    for (size_t i = 0; i < swapChains.size(); i++) {
        waitSemaphores.push_back(swapChains[i]->getImageAvailableSemaphore(currentFrameIndex));
        signalSemaphores.push_back(swapChains[i]->getRenderFinishedSemaphore(currentFrameIndex));
        waitStages.push_back(VK_PIPELINE_STAGE_TRANSFER_BIT);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    if (vkQueueSubmit(myDevice_.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrameIndex]) !=
        VK_SUCCESS) {
        throw std::runtime_error("faied to submit queue");
    }

    bool needRecreate = false;
    for (size_t i = 0; i < swapChains.size(); ++i) {
        VkSemaphore sem = signalSemaphores[i];
        auto result = swapChains[i]->present(myDevice_.presentQueue(), sem, currentImageIndex[i]);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            needRecreate = true;
        } else if (result != VK_SUCCESS)
            throw std::runtime_error("failed to present");
    }
    if (needRecreate) recreateSwapChains();

    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
    isFrameStarted = false;
}

void MyRenderer::blitToSwapChains(VkCommandBuffer cb) {
    VkImage src = renderTarget->getColorImage(currentFrameIndex);
    VkExtent2D srcExtent = renderTarget->getExtent();

    for (size_t i = 0; i < swapChains.size(); i++) {
        VkImage dst = swapChains[i]->getSwapChainImage(currentImageIndex[i]);
        VkExtent2D dstExtent = swapChains[i]->getSwapChainExtent();

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
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &startBarrier);

        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {(int32_t)srcExtent.width, (int32_t)srcExtent.height, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {(int32_t)dstExtent.width, (int32_t)dstExtent.height, 1};

        vkCmdBlitImage(cb, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

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

void MyRenderer::createSyncObjects() {
    inFlightFences.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(myDevice_.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

} // namespace my
