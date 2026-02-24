#pragma once
#include "render_core/my_model.hpp"
#include "vulkan_core/device.hpp"

#include <glm/glm.hpp>
#include <iostream>
#include <memory>

namespace my {

class TerrainGenerator {
  public:
    static std::unique_ptr<MyModel> generate(Device &myDevice, const std::vector<float> &heightMap,
                                             int gridSize, float cellSize, float heightScale) {
        MyModel::Builder builder{};

        builder.vertices.clear();
        builder.indicies.clear();

        for (int x = 0; x < gridSize; x++) {
            for (int z = 0; z < gridSize; z++) {
                MyModel::Vertex vertex{};
                vertex.position.x = x * cellSize;
                vertex.position.y = heightMap[z * gridSize + x] * heightScale;
                vertex.position.z = z * cellSize;

                vertex.uv.x = (float)x / (gridSize - 1);
                vertex.uv.y = (float)z / (gridSize - 1);

                vertex.color = {1.0f, 1.0f, 1.0f};

                builder.vertices.push_back(vertex);
            }
        }

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
