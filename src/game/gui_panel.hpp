#pragma once

#include "game/scene_reference.hpp"

namespace my {

class GuiPanel {
  public:
    virtual ~GuiPanel() = default;
    GuiPanel() = default;

    GuiPanel(const GuiPanel &) = delete;
    GuiPanel &operator=(const GuiPanel &) = delete;

    virtual void drawPanel(SceneEntityRef &sceneRef) = 0;
    virtual void updatePanel(SceneEntityRef &sceneRef) = 0;
};

} // namespace my
