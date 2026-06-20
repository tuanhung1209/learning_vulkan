#include "scene_file_panel.hpp"
#include "platforms/glfw_window.hpp"

#include <imgui.h>

namespace my {

SceneFilePanel::Event SceneFilePanel::drawGui(GlfwWindow &window) {
    Event event;

    ImGui::Begin("Scene");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();
    ImGui::InputText("save file", saveNameBuffer, IM_ARRAYSIZE(saveNameBuffer),
                     ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::Button("save")) { event.savePath = "assets/scenes/" + std::string(saveNameBuffer) + ".json"; }

    ImGui::Separator();
    if (window.hasDroppedFile()) { droppedFile = window.consumeDroppedFile(); }
    if (!droppedFile.empty()) {
        ImGui::Text("File: %s", droppedFile.c_str());
        if (ImGui::Button("Load")) {
            event.loadPath = droppedFile;
            droppedFile.clear();
        }
    }

    if (ImGui::Button("Refresh")) scenesSelector.scan();
    ImGui::BeginChild("##child", ImVec2(0, 100), true);
    for (int i = 0; i < scenesSelector.scenesFiles.size(); i++) {
        bool selected = (scenesSelector.selectedIndex == i);
        if (ImGui::Selectable(scenesSelector.scenesFiles[i].c_str(), selected)) {
            scenesSelector.selectedIndex = i;
        }
        if (selected) { ImGui::SetItemDefaultFocus(); }
    }
    ImGui::EndChild();

    if (ImGui::Button("Load")) {
        event.loadPath = scenesSelector.scenesDir + scenesSelector.scenesFiles[scenesSelector.selectedIndex];
    }

    ImGui::End();
    return event;
}

} // namespace my
