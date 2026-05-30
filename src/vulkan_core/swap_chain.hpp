#pragma once

#include "vulkan_core/device.hpp"

// std lib headers
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace my {

class SwapChain {
  public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    SwapChain(Device &deviceRef, VkExtent2D windowExtent, VkSurfaceKHR &vkSurface);
    SwapChain(Device &deviceRef, VkExtent2D windowExtent, VkSurfaceKHR &vkSurface,
              std::shared_ptr<SwapChain> previous);
    ~SwapChain();

    SwapChain(const SwapChain &) = delete;
    SwapChain &operator=(const SwapChain &) = delete;

    size_t imageCount() { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    VkImage getSwapChainImage(int i) { return swapChainImages[i]; }

    VkSemaphore getImageAvailableSemaphore(int frame) { return imageAvailableSemaphores[frame]; }
    VkSemaphore getRenderFinishedSemaphore(int frame) { return renderFinishedSemaphores[frame]; }

    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    VkResult acquireNextImage(uint32_t *imageIndex, int frame);
    VkResult present(VkQueue queue, VkSemaphore waitSem, uint32_t imageIndex);

    bool compareSwapChain(const SwapChain &sc) { return sc.swapChainImageFormat == swapChainImageFormat; }

  private:
    void init();
    void createSwapChain();
    void createSyncObjects();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImage> swapChainImages;

    Device &device;
    VkSurfaceKHR &vkSurface_;
    VkExtent2D windowExtent{};

    VkSwapchainKHR swapChain;
    std::shared_ptr<SwapChain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
};

} // namespace my
