#pragma once

#include "game/gui_panel.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace my {

class GuiManager {
  public:
    GuiManager() = default;
    ~GuiManager() = default;

    GuiManager(const GuiManager &) = delete;
    GuiManager &operator=(const GuiManager &) = delete;

    void registerGuiPanel(std::unique_ptr<GuiPanel> panel) { panels.push_back(std::move(panel)); }
    void drawGuiPanel(SceneEntityRef &sceneRef) {
        for (auto &panel : panels) { panel->drawPanel(sceneRef); }
    };
    void updatePanels(SceneEntityRef &sceneRef) {
        for (auto &panel : panels) { panel->updatePanel(sceneRef); }
    }

  private:
    std::vector<std::unique_ptr<GuiPanel>> panels{};
};

} // namespace my
