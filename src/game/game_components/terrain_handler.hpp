#pragma once

#include "ecs/ecs_manager.hpp"
#include "render_core/asset_cache.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace my {

class Device;
class MyModel;

class TerrainHandler {
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

    TerrainHandler(Device &device, EcsManager &ecsManager, AssetCache &assetCache);

    void createTerrain();
    void regenerateTerrain();

    bool drawGui();

    const std::vector<float> &getHeightMap() const { return heightMap; }
    const TerrainConfig &getConfig() const { return config; }
    TerrainConfig &getConfig() { return config; }

  private:
    std::string createTerrainMesh();
    void generateHeightMap(std::vector<float> &heightMap, std::vector<uint8_t> &noisePixels);
    void addIslandProperty(std::vector<float> &heightMap, int gridSize);

    // how to save this mesh if only mo take file path
    std::unique_ptr<MyModel> generateMesh(const std::vector<float> &heightMap, int gridSize, float cellSize,
                                          float heightScale);
    std::vector<float> heightMap;

    TerrainConfig config{};
    Device &myDevice_;
    EcsManager &ecsManager_;
    AssetCache &assetCache_;
    Entity terrainEntity_;
};

} // namespace my
