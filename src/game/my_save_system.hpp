#pragma once

#include "game/scene_reference.hpp"
#include "vulkan_core/device.hpp"

#include <string>

namespace my {

class SaveSystem {
  public:
    SaveSystem(Device &device);
    ~SaveSystem();

    void saveScene(const std::string &saveFilePath, SceneEntityRef scene);
    void loadScene(const std::string &loadFilePath, SceneEntityRef scene);

  private:
    Device &myDevice;
};

}; // namespace my
