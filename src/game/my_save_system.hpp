#pragma once

#include "first_app.hpp"
#include "render_core/my_frame_info.hpp"
#include "terrain_generation.hpp"

#include "lib/json.hpp"

namespace my {

class SaveSystem {
  public:
    SaveSystem(Device &device);
    ~SaveSystem();

    void saveScene(std::string saveFilePath, MyGameObject::Map &gameObjecs,
                   TerrainGenerator::TerrainConfig &config, SkyUbo &skyUbo, GrassComputePush &grassPush);
    void loadScene(std::string loadFilePath, MyGameObject::Map &gameObjects,
                   TerrainGenerator::TerrainConfig &config, SkyUbo &skyUbo, GrassComputePush &grassPush);

  private:
    nlohmann::json sceneJson;
    Device &myDevice;
};

}; // namespace my
