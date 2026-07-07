#include "sun_panel.hpp"

#include "math/NOAA_solar_position.hpp"

#include <imgui.h>

namespace my {

void SunPanel::updatePanel(SceneEntityRef &sceneRef) {
    float utcOffset = longitude / 15.0f;
    float utc = timeOfDay - utcOffset;
    int uh = static_cast<int>(utc);
    int um = static_cast<int>((utc - uh) * 60.0f);
    float us = ((utc - uh) * 60.0f - um) * 60.0f;

    SolarResult sr = SolarPosition::calculate(latitude, longitude, year, month, day, uh, um, us);

    cached.elevation = glm::pi<float>() / 2.0f - sr.zenith;
    cached.direction = SolarPosition::toDirection(sr);

    sceneRef.sunDirection = glm::vec4(-cached.direction, cached.elevation);
}

void SunPanel::drawPanel(SceneEntityRef &) {
    ImGui::Begin("Sun");
    ImGui::SliderFloat("Time of Day (hours)", &timeOfDay, 0.0f, 24.0f);
    ImGui::Separator();
    ImGui::InputFloat("Latitude", &latitude);
    ImGui::InputFloat("Longitude", &longitude);
    ImGui::InputInt("Year", &year);
    ImGui::InputInt("Month", &month);
    ImGui::InputInt("Day", &day);
    ImGui::Separator();
    ImGui::Text("Elevation: %.1f deg", glm::degrees(cached.elevation));
    ImGui::Text("Above horizon: %s", cached.elevation > 0.0f ? "yes" : "no");
    ImGui::End();
}

} // namespace my
