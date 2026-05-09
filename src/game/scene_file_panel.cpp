#include "scene_file_panel.hpp"
#include "platforms/glfw_window.hpp"

#include <imgui.h>

namespace my {

SceneFilePanel::Event SceneFilePanel::drawGui(GlfwWindow &window) {
    Event event;

    ImGui::Begin("Scene");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::InputText("save file", saveNameBuffer, IM_ARRAYSIZE(saveNameBuffer),
                     ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::Button("save")) { event.savePath = "assets/scenes/" + std::string(saveNameBuffer) + ".json"; }

    if (window.hasDroppedFile()) { droppedFile = window.consumeDroppedFile(); }

    if (!droppedFile.empty()) {
        ImGui::Text("File: %s", droppedFile.c_str());
        if (ImGui::Button("Load")) {
            event.loadPath = droppedFile;
            droppedFile.clear();
        }
    }

    ImGui::End();
    return event;
}

} // namespace my
