#include "render_systems/grass_render_system.hpp"
#include "imgui.h"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_imgui.hpp"
#include "vulkan_core/compute_pipeline.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <array>
#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vulkan/vulkan_core.h>

namespace my {

GrassRenderSystem::GrassRenderSystem(Device &device, VkRenderPass renderPass,
                                     VkDescriptorSetLayout globalSetLayout)
    : myDevice{device} {
    grassBladeModel = MyModel::createModelFromFile(myDevice, "assets/models/grass_blade.obj");
    createGrassComputeBuffer();
    createHeightComputeBuffer();
    createIndirectDrawBuffer();

    createComputePoolAndSetLayout();
    createComputePipelineLayout(globalSetLayout);

    createGraphicPipelineLayout(globalSetLayout);

    createComputePipeline();
    createGraphicPipeline(renderPass);
}

GrassRenderSystem::~GrassRenderSystem() {
    vkDestroyPipelineLayout(myDevice.device(), computePipelineLayout, nullptr);
    vkDestroyPipelineLayout(myDevice.device(), graphicPipelineLayout, nullptr);
}

void GrassRenderSystem::drawGui(float terrainHeightScale) {
    ImGui::Begin("Grass");

    if (ImGui::CollapsingHeader("Field", ImGuiTreeNodeFlags_DefaultOpen)) {
        int gridSize = static_cast<int>(push.gridSize);
        if (ImGui::SliderInt("Grid Size", &gridSize, 64, MAX_GRASS_GRID)) {
            push.gridSize = static_cast<uint32_t>(gridSize);
        }
        ImGui::SliderFloat("Spacing", &push.spacing, 0.05f, 1.0f);
        ImGui::SliderFloat("Blade Height", &push.bladeHeight, 0.1f, 15.0f);
        ImGui::SliderFloat("Height Scale", &push.heightScale, 1.0f, 200.0f);
        if (ImGui::Button("Sync Terrain Height")) { push.heightScale = terrainHeightScale; }
        ImGui::Text("Blade Count: %d", push.gridSize * push.gridSize);
    }

    if (ImGui::CollapsingHeader("Wind")) {
        ImGui::SliderFloat("Dir X", &push.windDirX, -1.0f, 1.0f);
        ImGui::SliderFloat("Dir Z", &push.windDirZ, -1.0f, 1.0f);
        ImGui::SliderFloat("Frequency", &push.windFreq, 0.1f, 5.0f);
        ImGui::SliderFloat("Amplitude", &push.windAmplitude, 0.0f, 3.0f);
        ImGui::SliderFloat("X Period", &push.xPeriod, 0.001f, 0.5f);
        ImGui::SliderFloat("Y Period", &push.yPeriod, 0.001f, 0.5f);
        ImGui::SliderFloat("Bias", &push.windBias, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Turbulence Power", &push.turbPower, 0.0f, 2.0f);
        ImGui::SliderFloat("Turbulence Size", &push.turbSize, 0.001f, 0.2f);
        ImGui::SliderFloat("Droop Strength", &push.droopStrength, 0.0f, 1.5f);
    }

    if (ImGui::CollapsingHeader("Color")) {
        ImGui::ColorPicker3("Base Color", &push.baseColor.x);
        ImGui::ColorPicker3("Tip Color", &push.tipColor.x);
    }

    ImGui::End();
}

void GrassRenderSystem::updateHeightMap(const std::vector<float> &heightMap, float heightScale) {
    push.heightScale = heightScale;
    push.terrainResolution = static_cast<uint32_t>(glm::sqrt(static_cast<float>(heightMap.size())));
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        heightComputeBuffers[i]->writeToBuffer((void *)heightMap.data(), sizeof(float) * heightMap.size());
    }
}

void GrassRenderSystem::createGrassComputeBuffer() {
    grassComputeBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    visibleGrassBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < grassComputeBuffers.size(); i++) {
        grassComputeBuffers[i] = std::make_unique<MyBuffer>(
            myDevice, sizeof(GrassTransformData) * MAX_GRASS_GRID * MAX_GRASS_GRID, 1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        visibleGrassBuffers[i] = std::make_unique<MyBuffer>(
            myDevice, sizeof(GrassTransformData) * MAX_GRASS_GRID * MAX_GRASS_GRID, 1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
}

void GrassRenderSystem::createHeightComputeBuffer() {
    heightComputeBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < heightComputeBuffers.size(); i++) {
        heightComputeBuffers[i] = std::make_unique<MyBuffer>(
            myDevice, sizeof(float) * MAX_GRASS_GRID * MAX_GRASS_GRID, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        heightComputeBuffers[i]->map();
    }
}

void GrassRenderSystem::createIndirectDrawBuffer() {
    indirectDrawBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < indirectDrawBuffers.size(); i++) {
        VkDrawIndexedIndirectCommand drawCmd{};
        drawCmd.indexCount = grassBladeModel->getIndexCount();

        MyBuffer stagingBuffer{myDevice, sizeof(VkDrawIndexedIndirectCommand), 1,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

        stagingBuffer.map();
        stagingBuffer.writeToBuffer(&drawCmd);

        indirectDrawBuffers[i] = std::make_unique<MyBuffer>(myDevice, sizeof(VkDrawIndexedIndirectCommand), 1,
                                                            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        myDevice.copyBuffer(stagingBuffer.getBuffer(), indirectDrawBuffers[i]->getBuffer(),
                            sizeof(VkDrawIndexedIndirectCommand));
    }
}

void GrassRenderSystem::createComputePoolAndSetLayout() {
    grassComputePool =
        MyDescriptorPool::Builder(myDevice)
            .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4 * SwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

    grassComputeSetLayout = MyDescriptorSetLayout::Builder(myDevice)
                                .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                                .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                                .build();

    grassComputeDescriptorSet.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < grassComputeDescriptorSet.size(); i++) {
        auto grassBufferInfo = grassComputeBuffers[i]->descriptorInfo();
        auto heightBufferInfo = heightComputeBuffers[i]->descriptorInfo();
        auto visibleGrassBufferInfo = visibleGrassBuffers[i]->descriptorInfo();
        auto indirectDrawBufferInfo = indirectDrawBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*grassComputeSetLayout, *grassComputePool)
            .writeBuffer(0, &grassBufferInfo)
            .writeBuffer(1, &heightBufferInfo)
            .writeBuffer(2, &visibleGrassBufferInfo)
            .writeBuffer(3, &indirectDrawBufferInfo)
            .build(grassComputeDescriptorSet[i]);
    }
}

void GrassRenderSystem::createComputePipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(GrassComputePush);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
        globalSetLayout,
        grassComputeSetLayout->getDescriptorSetLayout(),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &computePipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void GrassRenderSystem::createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(GrassComputePush);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
        globalSetLayout,
        grassComputeSetLayout->getDescriptorSetLayout(),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &graphicPipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void GrassRenderSystem::createComputePipeline() {
    assert(computePipelineLayout != nullptr && "cannot create pipeline before pipeline layout");
    myComputePipeline =
        std::make_unique<ComputePipeline>(myDevice, "shaders/grass_shader.comp.spv", computePipelineLayout);
}

void GrassRenderSystem::createGraphicPipeline(VkRenderPass renderPass) {
    assert(graphicPipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    GraphicPipelineConfigInfo pipelineConfig{};
    GraphicPipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = graphicPipelineLayout;

    myGraphicPipeline = std::make_unique<GraphicPipeline>(myDevice, "shaders/grass_shader.vert.spv",
                                                          "shaders/grass_shader.frag.spv", pipelineConfig);
}
void GrassRenderSystem::computeGrass(FrameInfo &frameInfo) {
    myComputePipeline->bind(frameInfo.commandBuffer);

    vkCmdFillBuffer(frameInfo.commandBuffer, indirectDrawBuffers[frameInfo.frameIndex]->getBuffer(), 4,
                    sizeof(uint32_t), 0);

    VkMemoryBarrier resetBarrier{};
    resetBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    resetBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    resetBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(frameInfo.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &resetBarrier, 0, nullptr, 0, nullptr);

    vkCmdPushConstants(frameInfo.commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(GrassComputePush), &push);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0,
                            1, &frameInfo.globalDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 1,
                            1, &grassComputeDescriptorSet[frameInfo.frameIndex], 0, nullptr);

    uint32_t groupCount = static_cast<uint32_t>(std::ceil(push.gridSize * push.gridSize / 256.0));
    vkCmdDispatch(frameInfo.commandBuffer, groupCount, 1, 1);

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    vkCmdPipelineBarrier(frameInfo.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 1,
                         &barrier, 0, nullptr, 0, nullptr);
}

void GrassRenderSystem::renderGrass(FrameInfo &frameInfo) {
    myGraphicPipeline->bind(frameInfo.commandBuffer);

    vkCmdPushConstants(frameInfo.commandBuffer, graphicPipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                       sizeof(GrassComputePush), &push);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            1, 1, &grassComputeDescriptorSet[frameInfo.frameIndex], 0, nullptr);

    grassBladeModel->bind(frameInfo.commandBuffer);

    vkCmdDrawIndexedIndirect(frameInfo.commandBuffer, indirectDrawBuffers[frameInfo.frameIndex]->getBuffer(),
                             0, 1, sizeof(VkDrawIndexedIndirectCommand));
}

} // namespace my
