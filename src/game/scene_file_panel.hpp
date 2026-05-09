#pragma once

#include <optional>
#include <string>

namespace my {

class GlfwWindow;

class SceneFilePanel {
  public:
    struct Event {
        std::optional<std::string> savePath;
        std::optional<std::string> loadPath;
    };

    Event drawGui(GlfwWindow &window);

  private:
    char saveNameBuffer[128]{};
    std::string droppedFile;
};

} // namespace my
