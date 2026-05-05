#pragma once

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

class WaylandWindow {
  public:
    WaylandWindow(std::string name);
    ~WaylandWindow();

    WaylandWindow(const WaylandWindow &) = delete;
    WaylandWindow &operator=(const WaylandWindow &) = delete;

    bool shouldClose() const { return isClose; }
    void pollEvents();
    VkExtent2D getExtent() const { return {monitors[0].width, monitors[0].height}; }

    struct Monitor {
        wl_output *output{};
        wl_surface *surface{};
        zwlr_layer_surface_v1 *layerSurface{};
        VkSurfaceKHR vkSurface{};
        uint32_t width{};
        uint32_t height{};
        bool configured = false;
        WaylandWindow *parent = nullptr;
    };

    void createVulkanSurfaces(VkInstance instance);
    void destroyVulkanSurfaces(VkInstance instance);

    // static std::vector<const char *> getRequiredInstanceExtensions();
    const std::vector<Monitor> &getMonitor() const { return monitors; }

  private:
    std::string name;
    bool isClose = false;

    // listener
    void initRegistryListener();
    void initSurfaceListener(Monitor &m);

    // registry global
    wl_display *display{};
    wl_registry *registry{};
    wl_compositor *compositor{};
    zwlr_layer_shell_v1 *layerShell{};
    std::vector<Monitor> monitors;

    void setUpLayerSurface();

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
