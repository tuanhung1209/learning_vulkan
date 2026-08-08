#include "terrain_handler.hpp"

#include "ecs/components/model_component.hpp"
#include "ecs/components/texture_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/entity.hpp"

#include "vulkan_core/device.hpp"
#include "render_core/my_model.hpp"
#include "math/perlin_noise.hpp"

#include <imgui.h>
#include <iostream>
#include <memory>

namespace my {

TerrainHandler::TerrainHandler(Device &device, EcsManager &ecsManager, AssetCache &assetCache)
    : myDevice_{device}, ecsManager_(ecsManager), assetCache_(assetCache) {
    terrainEntity_ = {Entity::null};
}

void TerrainHandler::createTerrain() {
    std::string filepath = createTerrainMesh();
    ecsManager_.add<ModelComponent>(terrainEntity_, ModelComponent{.modelPath = filepath});
    ecsManager_.add<TextureComponent>(terrainEntity_, TextureComponent{"assets/textures/white.png"});
    ecsManager_.add<TransformComponent>(terrainEntity_, TransformComponent{.translation = {0.f, 0.5f, 0.f}});
}

void TerrainHandler::regenerateTerrain() {
    std::string filepath = createTerrainMesh();
    ecsManager_.add<ModelComponent>(terrainEntity_, ModelComponent{.modelPath = filepath});
}

std::string TerrainHandler::createTerrainMesh() {
    if (!ecsManager_.isEntityAlive(terrainEntity_)) { terrainEntity_ = ecsManager_.createEntity(); }
    int res = config.resolution;
    std::vector<uint8_t> noisePixels(res * res * 4);
    heightMap.resize(res * res);
    generateHeightMap(heightMap, noisePixels);

    std::shared_ptr<MyModel> terrainModel = generateMesh(heightMap, res, 1, config.heightScale);
    std::string filepath = "assets/terrainModel";
    assetCache_.insertModel(filepath, terrainModel);

    return filepath;
}

bool TerrainHandler::drawGui() {
    bool regenerate = false;
    ImGui::Begin("Terrain");
    ImGui::SliderInt("Seed", &config.seed, 0, 100);
    ImGui::SliderFloat("Noise Scale", &config.noiseScale, 50.0f, 500.0f);
    ImGui::SliderInt("Octaves", &config.octaves, 1, 15);
    ImGui::SliderFloat("Height Scale", &config.heightScale, 1.0f, 200.0f);
    ImGui::SliderFloat("Rotation Angle", &config.rotationAngle, 0.f, 180.f);
    ImGui::SliderFloat("Lacunarity", &config.lacunarity, 1.0f, 4.0f);
    ImGui::SliderFloat("Persistence", &config.persistence, 0.1f, 1.0f);
    ImGui::SliderInt("Resolution", &config.resolution, 64, 1024);
    ImGui::Separator();
    ImGui::SliderFloat("Sand Threshold", &config.sandThreshold, 0.0f, 1.0f);
    ImGui::SliderFloat("Grass Threshold", &config.grassThreshold, 0.0f, 1.0f);
    ImGui::SliderFloat("Rock Threshold", &config.rockThreshold, 0.0f, 1.0f);
    if (ImGui::Button("Regenerate Terrain")) { regenerate = true; }
    ImGui::End();
    return regenerate;
}

void TerrainHandler::generateHeightMap(std::vector<float> &heightMap, std::vector<uint8_t> &noisePixels) {
    PerlinGenerator::populateNoise(config.octaves, config.noiseScale, config.resolution, config.resolution,
                                   config.rotationAngle, noisePixels, heightMap, config.seed,
                                   config.lacunarity, config.persistence);

    addIslandProperty(heightMap, config.resolution);
    // might add stuff later here for biome percific or something
}

void TerrainHandler::addIslandProperty(std::vector<float> &heightMap, int gridSize) {
    float centerX = (gridSize - 1) / 2.0f;
    float centerZ = (gridSize - 1) / 2.0f;

    for (int x = 0; x < gridSize; x++) {
        for (int z = 0; z < gridSize; z++) {
            float dx = (x - centerX) / centerX;
            float dz = (z - centerZ) / centerZ;
            float dist = dx * dx + dz * dz;

            float t = glm::max(0.0f, 1.0f - dist * 0.6f);
            float falloff = t * t * (3.0f - 2.0f * t); // smoothstep
            heightMap[z * gridSize + x] *= falloff;
        }
    }
}

std::unique_ptr<MyModel> TerrainHandler::generateMesh(const std::vector<float> &heightMap, int gridSize,
                                                      float cellSize, float heightScale) {
    MyModel::Builder builder{};

    float offSet = (gridSize - 1) * cellSize * 0.5f;

    glm::vec3 sandColor = {1.f, 1.f, 0.3f};
    glm::vec3 grassColor = {.0f, .52f, .0f};
    glm::vec3 rockColor = {0.5f, 0.5f, 0.5f};
    glm::vec3 snowColor = {1.f, 1.f, 1.f};

    // Find min/max of raw heightmap to normalize to 0–1
    float minH = heightMap[0], maxH = heightMap[0];
    for (int i = 1; i < gridSize * gridSize; i++) {
        minH = glm::min(minH, heightMap[i]);
        maxH = glm::max(maxH, heightMap[i]);
    }

    float hRange = maxH - minH;
    if (hRange == 0.0f) hRange = 1.0f;

    float sandThreshold = config.sandThreshold;
    float grassThreshold = config.grassThreshold;
    float rockThreshold = config.rockThreshold;

    for (int x = 0; x < gridSize; x++) {
        for (int z = 0; z < gridSize; z++) {
            MyModel::Vertex vertex{};
            vertex.position.x = x * cellSize - offSet;
            vertex.position.y = heightMap[z * gridSize + x] * heightScale;
            vertex.position.z = z * cellSize - offSet;

            vertex.uv.x = (float)x / (gridSize - 1);
            vertex.uv.y = (float)z / (gridSize - 1);

            // 0.0 = peaks, 1.0 = sea level
            float h = (heightMap[z * gridSize + x] - minH) / hRange;

            if (h > sandThreshold) {
                vertex.color = sandColor;
            } else if (h > grassThreshold) {
                float t = (sandThreshold - h) / (sandThreshold - grassThreshold);
                vertex.color = glm::mix(sandColor, grassColor, t);
            } else if (h > rockThreshold) {
                float t = (grassThreshold - h) / (grassThreshold - rockThreshold);
                vertex.color = glm::mix(grassColor, rockColor, t);
            } else {
                float t = (rockThreshold - h) / rockThreshold;
                vertex.color = glm::mix(rockColor, snowColor, t);
            }

            builder.vertices.push_back(vertex);
        }
    }

    // Compute normals using central differences of neighboring vertices
    for (int x = 0; x < gridSize; x++) {
        for (int z = 0; z < gridSize; z++) {
            int idx = x * gridSize + z;

            glm::vec3 left =
                (x > 0) ? builder.vertices[(x - 1) * gridSize + z].position : builder.vertices[idx].position;
            glm::vec3 right = (x < gridSize - 1) ? builder.vertices[(x + 1) * gridSize + z].position
                                                 : builder.vertices[idx].position;
            glm::vec3 back =
                (z > 0) ? builder.vertices[x * gridSize + (z - 1)].position : builder.vertices[idx].position;
            glm::vec3 fwd = (z < gridSize - 1) ? builder.vertices[x * gridSize + (z + 1)].position
                                               : builder.vertices[idx].position;

            glm::vec3 dx = right - left;
            glm::vec3 dz = fwd - back;
            builder.vertices[idx].normal = glm::normalize(glm::cross(dx, dz));
        }
    }

    // Build index buffer — two triangles per grid cell
    for (int x = 0; x < gridSize - 1; x++) {
        for (int z = 0; z < gridSize - 1; z++) {
            uint32_t topLeft = x * gridSize + z;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (x + 1) * gridSize + z;
            uint32_t bottomRight = bottomLeft + 1;

            builder.indicies.push_back(topLeft);
            builder.indicies.push_back(bottomLeft);
            builder.indicies.push_back(topRight);

            builder.indicies.push_back(topRight);
            builder.indicies.push_back(bottomLeft);
            builder.indicies.push_back(bottomRight);
        }
    }

    std::cout << "number of terrain vertex :" << builder.vertices.size() << "\n";
    return std::make_unique<MyModel>(myDevice_, builder);
}

} // namespace my
