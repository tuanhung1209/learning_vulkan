#include "wayland_window.hpp"

#include <cassert>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

namespace my {

void WaylandWindow::initRegistryListener() {
    static const wl_registry_listener REGISTRY_LISTENER{
        .global = &WaylandWindow::onRegistryGlobal,
        .global_remove = &WaylandWindow::onRegistryGlobalRemove,
    };
    wl_registry_add_listener(registry, &REGISTRY_LISTENER, this);
}

void WaylandWindow::initSurfaceListener(Monitor &m) {
    static const zwlr_layer_surface_v1_listener ZWLR_SURFACE_LISTENER{
        .configure = WaylandWindow::onLayerConfigure,
        .closed = WaylandWindow::onLayerClosed,
    };
    zwlr_layer_surface_v1_add_listener(m.layerSurface, &ZWLR_SURFACE_LISTENER, &m);
}

WaylandWindow::WaylandWindow(std::string name) : name{name} {
    monitors.reserve(8);

    display = wl_display_connect(nullptr);
    assert(display && "connot connect display");

    registry = wl_display_get_registry(display);
    initRegistryListener();

    wl_display_roundtrip(display);

    assert(compositor && "no wl_compositor");
    assert(layerShell && "zwlr_layer_shell_v1");
    assert(!monitors.empty() && "no wl_outputs found");

    setUpLayerSurface();

    wl_display_roundtrip(display);
}

WaylandWindow::~WaylandWindow() {
    for (auto &m : monitors) {
        if (m.layerSurface) zwlr_layer_surface_v1_destroy(m.layerSurface);
        if (m.surface) wl_surface_destroy(m.surface);
        if (m.output) wl_output_destroy(m.output);
    }
    if (layerShell) zwlr_layer_shell_v1_destroy(layerShell);
    if (compositor) wl_compositor_destroy(compositor);
    if (registry) wl_registry_destroy(registry);
    if (display) wl_display_disconnect(display);
}

std::vector<MonitorTarget> WaylandWindow::getMonitorTarget() {
    std::vector<MonitorTarget> monitorsOut;
    monitorsOut.reserve(monitors.size());
    for (auto &m : monitors) { monitorsOut.push_back({m.vkSurface, m.vkExtent}); }
    return monitorsOut;
}

void WaylandWindow::pollEvents() {
    while (wl_display_prepare_read(display) != 0) { wl_display_dispatch_pending(display); }
    wl_display_flush(display);
    wl_display_read_events(display);
    wl_display_dispatch_pending(display);
}

void WaylandWindow::createVulkanSurfaces(VkInstance instance) {
    for (auto &m : monitors) {
        VkWaylandSurfaceCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        info.display = display;
        info.surface = m.surface;

        if (vkCreateWaylandSurfaceKHR(instance, &info, nullptr, &m.vkSurface) != VK_SUCCESS) {
            throw std::runtime_error("can not create wayland surface");
        }
    }
}

void WaylandWindow::destroyVulkanSurfaces(VkInstance instance) {
    for (auto &m : monitors) {
        vkDestroySurfaceKHR(instance, m.vkSurface, nullptr);
        m.vkSurface = VK_NULL_HANDLE;
    }
}

void WaylandWindow::setUpLayerSurface() {
    for (auto &m : monitors) {
        m.parent = this;
        m.surface = wl_compositor_create_surface(compositor);

        m.layerSurface = zwlr_layer_shell_v1_get_layer_surface(
            layerShell, m.surface, m.output, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "wallpaper");

        initSurfaceListener(m);

        zwlr_layer_surface_v1_set_size(m.layerSurface, 0, 0);
        zwlr_layer_surface_v1_set_anchor(
            m.layerSurface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        zwlr_layer_surface_v1_set_exclusive_zone(m.layerSurface, -1);

        wl_surface_commit(m.surface);
    }
}

void WaylandWindow::handleRegistryGlobal(wl_registry *reg, uint32_t name, const char *iface,
                                         uint32_t version) {
    if (std::strcmp(iface, wl_compositor_interface.name) == 0) {
        compositor = static_cast<wl_compositor *>(wl_registry_bind(reg, name, &wl_compositor_interface, 4));
    } else if (std::strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0) {
        layerShell = static_cast<zwlr_layer_shell_v1 *>(
            wl_registry_bind(reg, name, &zwlr_layer_shell_v1_interface, 1));
    } else if (std::strcmp(iface, wl_output_interface.name) == 0) {
        Monitor m{};
        m.output = static_cast<wl_output *>(wl_registry_bind(reg, name, &wl_output_interface, 2));
        monitors.push_back(m);
    }
}

void WaylandWindow::handleLayerConfigure(Monitor *m, zwlr_layer_surface_v1 *layerSurface, uint32_t serial,
                                         uint32_t w, uint32_t h) {
    m->vkExtent.width = w;
    m->vkExtent.height = h;
    m->configured = true;
    zwlr_layer_surface_v1_ack_configure(layerSurface, serial);
}

void WaylandWindow::handleLayerClosed(Monitor *m, zwlr_layer_surface_v1 *layerSurface) {
    m->parent->isClose = true;
}

void WaylandWindow::onRegistryGlobal(void *data, wl_registry *reg, uint32_t name, const char *iface,
                                     uint32_t version) {
    static_cast<WaylandWindow *>(data)->handleRegistryGlobal(reg, name, iface, version);
}

void WaylandWindow::onRegistryGlobalRemove(void *data, wl_registry *reg, uint32_t) {}

void WaylandWindow::onLayerConfigure(void *data, zwlr_layer_surface_v1 *layerSurface, uint32_t serial,
                                     uint32_t w, uint32_t h) {
    auto *m = static_cast<WaylandWindow::Monitor *>(data);
    m->parent->handleLayerConfigure(m, layerSurface, serial, w, h);
}

void WaylandWindow::onLayerClosed(void *data, zwlr_layer_surface_v1 *layerSurface) {
    auto *m = static_cast<WaylandWindow::Monitor *>(data);
    m->parent->handleLayerClosed(m, layerSurface);
}

} // namespace my
