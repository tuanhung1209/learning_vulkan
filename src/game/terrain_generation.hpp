#pragma once
#include "render_core/my_model.hpp"
#include "vulkan_core/device.hpp"

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <vector>

namespace my {

class TerrainGenerator {
  public:
    static void addIslandProperty(std::vector<float> &heightMap, int gridSize) {
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

    static std::unique_ptr<MyModel> generate(Device &myDevice, const std::vector<float> &heightMap,
                                             int gridSize, float cellSize, float heightScale) {
        MyModel::Builder builder{};

        builder.vertices.clear();
        builder.indicies.clear();

        float offSet = (gridSize - 1) * cellSize * 0.5f;

        glm::vec3 sandColor = {1.f, 1.f, 0.3f};
        glm::vec3 grassColor = {.0f, .52f, .0f};
        glm::vec3 rockColor = {0.5f, 0.5f, 0.5f};
        glm::vec3 snowColor = {1.f, 1.f, 1.f};

        float sandEnd = -0.5f;  // sand
        float grassEnd = -2.0f; // grass
        float rockEnd = -4.5f;  // rock
        float snowEnd = -6.0f;  // snow

        for (int x = 0; x < gridSize; x++) {
            for (int z = 0; z < gridSize; z++) {
                MyModel::Vertex vertex{};
                vertex.position.x = x * cellSize - offSet;
                vertex.position.y = heightMap[z * gridSize + x] * heightScale;
                vertex.position.z = z * cellSize - offSet;

                vertex.uv.x = (float)x / (gridSize - 1);
                vertex.uv.y = (float)z / (gridSize - 1);

                float y = vertex.position.y;
                if (y > sandEnd) {
                    vertex.color = sandColor;
                } else if (y > grassEnd) {
                    float t = glm::clamp((y - sandEnd) / (grassEnd - sandEnd), 0.0f, 1.0f);
                    vertex.color = glm::mix(sandColor, grassColor, t);
                } else if (y > rockEnd) {
                    float t = glm::clamp((y - grassEnd) / (rockEnd - grassEnd), 0.0f, 1.0f);
                    vertex.color = glm::mix(grassColor, rockColor, t);
                } else {
                    float t = glm::clamp((y - rockEnd) / (snowEnd - rockEnd), 0.0f, 1.0f);
                    vertex.color = glm::mix(rockColor, snowColor, t);
                }

                builder.vertices.push_back(vertex);
            }
        }

        int top = 1000000;
        int bottom = -1000000;
        for (auto vertex : builder.vertices) {
            if (vertex.position.y < top) { top = vertex.position.y; }
            if (vertex.position.y > bottom) { bottom = vertex.position.y; }
        }
        std::cout << "top : " << top << std::endl;
        std::cout << "bottom : " << bottom << std::endl;

        // Compute normals using central differences of neighboring vertices
        for (int x = 0; x < gridSize; x++) {
            for (int z = 0; z < gridSize; z++) {
                int idx = x * gridSize + z;

                glm::vec3 left = (x > 0) ? builder.vertices[(x - 1) * gridSize + z].position
                                         : builder.vertices[idx].position;
                glm::vec3 right = (x < gridSize - 1) ? builder.vertices[(x + 1) * gridSize + z].position
                                                     : builder.vertices[idx].position;
                glm::vec3 back = (z > 0) ? builder.vertices[x * gridSize + (z - 1)].position
                                         : builder.vertices[idx].position;
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
};

} // namespace my
