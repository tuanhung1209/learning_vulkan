#pragma once

#include "first_app.hpp"
#include "render_core/my_frame_info.hpp"
#include "terrain_generation.hpp"

#include "lib/json.hpp"

namespace my {

class SaveSystem {
  public:
    SaveSystem(std::string saveFilePath);
    ~SaveSystem();

    void saveScene(MyGameObject::Map &gameObjecs, TerrainGenerator::TerrainConfig &config, SkyUbo &skyUbo);
    void loadScene(Device &device, std::string loadFilePath, MyGameObject::Map &gameObjects,
                   TerrainGenerator::TerrainConfig &config, SkyUbo &skyUbo);

  private:
    std::string saveFilePath;
    nlohmann::json sceneJson;
};

}; // namespace my
