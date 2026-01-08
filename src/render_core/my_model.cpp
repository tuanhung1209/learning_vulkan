#include "render_core/my_model.hpp"
#include "vulkan_core/my_utils.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL 
#include <glm/gtx/hash.hpp>

namespace std{
    template <>
    struct hash<my::MyModel::Vertex>{
        size_t operator()(my::MyModel::Vertex const &vertex) const{
            size_t seed = 0;
            my::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}
namespace my{
    MyModel::MyModel(Device &device, const MyModel::Builder &builder) : myDevice{device} {
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indicies);

        for (const auto &vert : builder.vertices){
            bound.min = glm::min(bound.min, vert.position);
            bound.max = glm::max(bound.max, vert.position);
        }
    }

    MyModel::~MyModel(){}

    std::unique_ptr<MyModel> MyModel::createModelFromFile(Device &device, const std::string filepath){
        Builder builder{};
        builder.loadModel(filepath);
        std::cout << "number of vertex :" << builder.vertices.size() << "\n";
        return std::make_unique<MyModel>(device, builder);
    }

    void MyModel::createVertexBuffers(const std::vector<Vertex> &vertices){
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "vertex count must at least be 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
        uint32_t vertexSize = sizeof(vertices[0]);

        MyBuffer statgingBuffer{myDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

        statgingBuffer.map();
        statgingBuffer.writeToBuffer((void *)vertices.data());

        vertexBuffer = std::make_unique<MyBuffer>(myDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        myDevice.copyBuffer(statgingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }

    void MyModel::createIndexBuffers(const std::vector<uint32_t> &indicies){
        indexCount = static_cast<uint32_t>(indicies.size());
        haveIndexBuffer = indexCount > 0;
        
        if (!haveIndexBuffer){
            return;
        }

        VkDeviceSize bufferSize = sizeof(indicies[0]) * indexCount;
        uint32_t indexSize = sizeof(indicies[0]);

        MyBuffer statgingBuffer{myDevice, indexSize, indexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

        statgingBuffer.map();
        statgingBuffer.writeToBuffer((void *)indicies.data());

        indexBuffer = std::make_unique<MyBuffer>(myDevice, indexSize, indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        myDevice.copyBuffer(statgingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    void MyModel::draw(VkCommandBuffer commandBuffer){
        if (haveIndexBuffer){
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        }
        else{
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    void MyModel::bind(VkCommandBuffer commandBuffer){
        VkBuffer buffers[] = {vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (haveIndexBuffer){
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    std::vector<VkVertexInputBindingDescription> MyModel::Vertex::getBindingDescriptions(){
        std::vector<VkVertexInputBindingDescription> bindingDescription(1);
        bindingDescription[0].binding = 0;
        bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription[0].stride = sizeof(Vertex);
        return bindingDescription;
    }
    std::vector<VkVertexInputAttributeDescription> MyModel::Vertex::getAttributeDescriptions(){
        std::vector<VkVertexInputAttributeDescription> attributeDescription{};

        //binding is like a number of vector<Vertex>
        //location is the location use in the vertex shader

        attributeDescription.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescription.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescription.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        attributeDescription.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

        return attributeDescription;
    }

    void MyModel::Builder::loadModel(const std::string &filepath){
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())){
            throw std::runtime_error(warn + err);
        }

        vertices.clear();
        indicies.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVerticies{}; 
        for (const auto &shape : shapes){
            for (const auto &index : shape.mesh.indices){
                Vertex vertex{};

                if (index.vertex_index >= 0){
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2],
                    };

                    vertex.color = {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2],
                    };
               }

                if (index.normal_index >= 0){
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }

                if (index.texcoord_index >= 0){
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if (uniqueVerticies.count(vertex) == 0){
                    uniqueVerticies[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indicies.push_back(uniqueVerticies[vertex]);
            }
        }
    }
}