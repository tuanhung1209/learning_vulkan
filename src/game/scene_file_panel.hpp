#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace my {

class GlfwWindow;

class SceneFilePanel {
  public:
    struct Event {
        std::optional<std::string> savePath;
        std::optional<std::string> loadPath;
    };

    struct ScenceSelector {
        std::string scenesDir = "assets/scenes/";
        std::vector<std::string> scenesFiles{};
        int selectedIndex{};

        void scan() {
            scenesFiles.clear();
            for (auto &file : std::filesystem::directory_iterator(scenesDir)) {
                if (file.path().extension() == ".json") {
                    scenesFiles.push_back(file.path().filename().string());
                }
            }
            std::sort(scenesFiles.begin(), scenesFiles.end());
        }
    };

    Event drawGui(GlfwWindow &window);

  private:
    char saveNameBuffer[128]{};
    std::string droppedFile;
    ScenceSelector scenesSelector{};
};

} // namespace my
