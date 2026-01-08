//#include <GLFW/glfw3.h>
#include "render_core/my_renderer.hpp"

#include <stdexcept>
#include <array>
#include <memory>

namespace my{

MyRenderer::MyRenderer(Window &window, Device &device) : myWindow{window}, myDevice{device} {
    recreateSwapChain();
    createCommandBuffers();
}

MyRenderer::~MyRenderer(){ freeCommandBuffers(); }

void MyRenderer::recreateSwapChain(){
    auto extend = myWindow.getExtend();
    while (extend.width == 0 || extend.height == 0){
        extend = myWindow.getExtend();
        glfwWaitEvents();
    } 

    vkDeviceWaitIdle(myDevice.device());
    if (mySwapChain == nullptr){
        mySwapChain = std::make_unique<SwapChain>(myDevice, extend);
    }
    else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(mySwapChain);
        mySwapChain = std::make_unique<SwapChain>(myDevice, extend, oldSwapChain);

        if (!oldSwapChain->compareSwapChain(*mySwapChain.get())){
            throw std::runtime_error("SwapChain image(depth) format have changed");
        }
    }
}

void MyRenderer::createCommandBuffers(){
    commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = myDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    
    if (vkAllocateCommandBuffers(myDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS){
        throw std::runtime_error("unable to create(allocate) command buffer");
    }
}

void MyRenderer::freeCommandBuffers(){
    vkFreeCommandBuffers(myDevice.device(), myDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

VkCommandBuffer MyRenderer::beginFrame(){
    assert(!isFrameStarted && "cant call beginFrame while already in progress");
    auto result = mySwapChain->acquireNextImage(&currentImageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR){
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to aquire swap chain image");
    }

    isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("faied to begin recording command buffer");
    }

    return commandBuffer;
}

void MyRenderer::endFrame(){
    assert(isFrameStarted && "can't not end frame when frame is not in progress");
    auto commandBuffer = getCurrentCommandBuffer();

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("fail to end command buffer");
    }

    auto result = mySwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || myWindow.wasWindowResized() == true){
        myWindow.resetWindowResizedFlag();
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS){
        throw std::runtime_error("failed to present swap chain image");
    }

    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
    isFrameStarted = false;
}

void MyRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer){
    assert(isFrameStarted && "cannot begin swapchain if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() && "cannot begin renderpass on command buffer from a diffrent frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mySwapChain->getRenderPass();
    renderPassInfo.framebuffer = mySwapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = mySwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // testing
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(mySwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(mySwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, mySwapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void MyRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer){
    assert(isFrameStarted && "cannot call begin swapchain if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() && "cannot begin renderpass on command buffer from a diffrent frame");

    vkCmdEndRenderPass(commandBuffer);

}
}