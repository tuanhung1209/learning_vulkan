#include "game/terrain_generation.hpp"
#include "math/perlin_noise.hpp"

#include <glm/glm.hpp>
#include <imgui.h>
#include <iostream>

namespace my {

TerrainGenerator::TerrainGenerator(Device &device) : myDevice{device} {}

void TerrainGenerator::createTerrain(MyGameObject::Map &gameObjects, std::shared_ptr<MyModel> quadModel) {
    int res = config.resolution;
    std::vector<uint8_t> noisePixels(res * res * 4);
    heightMap.resize(res * res);
    generateHeightMap(heightMap, noisePixels);

    auto perlinViewer = MyGameObject::createGameObject();
    perlinViewer.model = quadModel;
    perlinViewer.transform.translation = {4.f, -2.f, 1.f};
    perlinViewer.transform.scale = {2.f, 2.f, 2.f};
    perlinViewer.transform.rotation = {0.f, glm::quarter_pi<float>(), glm::half_pi<float>()};
    perlinViewer.texture = std::make_shared<MyTexture>(myDevice, res, res, noisePixels);
    heightmapViewerId = perlinViewer.getId();
    gameObjects.emplace(perlinViewer.getId(), std::move(perlinViewer));

    auto terrain = MyGameObject::createGameObject();
    terrain.model = generateMesh(heightMap, res, 1, config.heightScale);
    terrain.transform.translation = {0.f, 0.5f, 0.f};
    terrainId = terrain.getId();
    gameObjects.emplace(terrain.getId(), std::move(terrain));
}

void TerrainGenerator::regenerate(MyGameObject::Map &gameObjects) {
    int res = config.resolution;
    std::vector<uint8_t> noisePixels(res * res * 4);
    heightMap.resize(res * res);
    generateHeightMap(heightMap, noisePixels);

    gameObjects.at(terrainId).model = generateMesh(heightMap, res, 1, config.heightScale);
    gameObjects.at(heightmapViewerId).texture = std::make_shared<MyTexture>(myDevice, res, res, noisePixels);
}

bool TerrainGenerator::drawGui() {
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

void TerrainGenerator::generateHeightMap(std::vector<float> &heightMap, std::vector<uint8_t> &noisePixels) {
    int res = config.resolution;
    PerlinGenerator::populateNoise(config.octaves, config.noiseScale, res, res, config.rotationAngle,
                                   noisePixels, heightMap, config.seed, config.lacunarity,
                                   config.persistence);
    addIslandProperty(heightMap, res);
}

void TerrainGenerator::addIslandProperty(std::vector<float> &heightMap, int gridSize) {
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

std::unique_ptr<MyModel> TerrainGenerator::generateMesh(const std::vector<float> &heightMap, int gridSize,
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
    return std::make_unique<MyModel>(myDevice, builder);
}

} // namespace my
