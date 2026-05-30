#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

namespace my {

struct MonitorTarget {
    VkSurfaceKHR vkSurface{};
    VkExtent2D extent{};
};

class DisplayProvider {
  public:
    virtual ~DisplayProvider() = default;

    DisplayProvider(const DisplayProvider &) = delete;
    DisplayProvider &operator=(const DisplayProvider &) = delete;

    virtual std::vector<MonitorTarget> getMonitorTarget() = 0;
    virtual void waitEvents() = 0;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() = 0;

  protected:
    DisplayProvider() = default;
};

} // namespace my
