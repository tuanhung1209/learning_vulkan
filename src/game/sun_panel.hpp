#pragma once

#include "game/gui_panel.hpp"

#include <glm/glm.hpp>

namespace my {

class SunPanel : public GuiPanel {
  public:
    SunPanel() = default;
    ~SunPanel() = default;

    SunPanel(const SunPanel &) = delete;
    SunPanel &operator=(const SunPanel &) = delete;

    void updatePanel(SceneEntityRef &sceneRef) override;
    void drawPanel(SceneEntityRef &) override;

  private:
    struct SunResult {
        glm::vec3 direction{};
        float elevation{};
    };

    float timeOfDay = 10.5f;
    float latitude = 21.0278f;
    float longitude = 105.8342f;
    int year = 2026;
    int month = 6;
    int day = 23;

    SunResult cached{};
};

} // namespace my
