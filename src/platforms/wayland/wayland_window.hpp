#pragma once

#include "platforms/display_provider.hpp"
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#define namespace _namespace
#include "wlr-layer-shell-client-protocol.h"
#undef namespace

#include <string>

namespace my {

class WaylandWindow : public DisplayProvider {
  public:
    WaylandWindow(std::string name);
    ~WaylandWindow();

    WaylandWindow(const WaylandWindow &) = delete;
    WaylandWindow &operator=(const WaylandWindow &) = delete;

    std::vector<MonitorTarget> getMonitorTarget() override;
    void waitEvents() override { pollEvents(); };
    void pollEvents() override;
    bool shouldClose() override { return isClose; };

    struct Monitor {
        wl_output *output{};
        wl_surface *surface{};
        zwlr_layer_surface_v1 *layerSurface{};
        VkSurfaceKHR vkSurface{};
        VkExtent2D vkExtent{};
        bool configured = false;
        WaylandWindow *parent = nullptr;
    };

    void createVulkanSurfaces(VkInstance instance);
    void destroyVulkanSurfaces(VkInstance instance);

    // static std::vector<const char *> getRequiredInstanceExtensions();
    const std::vector<Monitor> &getMonitor() const { return monitors; }
    const VkExtent2D getExtent() const { return monitors[0].vkExtent; }

  private:
    std::string name;
    bool isClose = false;

    std::vector<Monitor> monitors;
    void setUpLayerSurface();

    // listener
    void initRegistryListener();
    void initSurfaceListener(Monitor &m);

    // registry global
    wl_display *display{};
    wl_registry *registry{};
    wl_compositor *compositor{};
    zwlr_layer_shell_v1 *layerShell{};

    void handleRegistryGlobal(wl_registry *reg, uint32_t name, const char *iface, uint32_t version);
    void handleLayerConfigure(Monitor *m, zwlr_layer_surface_v1 *layerSurface, uint32_t serial, uint32_t w,
                              uint32_t h);
    void handleLayerClosed(Monitor *m, zwlr_layer_surface_v1 *layerSurface);

    static void onRegistryGlobal(void *, wl_registry *, uint32_t, const char *, uint32_t);
    static void onRegistryGlobalRemove(void *, wl_registry *, uint32_t);

    static void onLayerConfigure(void *, zwlr_layer_surface_v1 *, uint32_t, uint32_t, uint32_t);
    static void onLayerClosed(void *, zwlr_layer_surface_v1 *);
};
} // namespace my
