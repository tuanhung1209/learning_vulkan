#pragma once
#include "game/my_game_object.hpp"
#include "render_core/my_model.hpp"
#include "render_core/my_texture.hpp"
#include "vulkan_core/device.hpp"

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace my {

class TerrainGenerator {
  public:
    struct TerrainConfig {
        int seed = 2;
        float noiseScale = 250.0f;
        int octaves = 10;
        float heightScale = 120.0f;
        int resolution = 512;
        float rotationAngle = glm::radians(47.0f);
        float lacunarity = 2.0f;
        float persistence = 0.45f;
        float sandThreshold = 0.85f;
        float grassThreshold = 0.55f;
        float rockThreshold = 0.25f;
    };

    TerrainGenerator(Device &device);

    void createTerrain(MyGameObject::Map &gameObjects);

    void regenerate(MyGameObject::Map &gameObjects);

    bool drawGui();

    const std::vector<float> &getHeightMap() const { return heightMap; }

    TerrainConfig config{};

  private:
    void generateHeightMap(std::vector<float> &heightMap, std::vector<uint8_t> &noisePixels);
    void addIslandProperty(std::vector<float> &heightMap, int gridSize);
    std::unique_ptr<MyModel> generateMesh(const std::vector<float> &heightMap, int gridSize, float cellSize,
                                          float heightScale);

    std::vector<float> heightMap;

    Device &myDevice;
    MyGameObject::id_t terrainId{};
    MyGameObject::id_t heightmapViewerId{};
};

} // namespace my
