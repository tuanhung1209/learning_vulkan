#include "render_systems/grass_render_system.hpp"
#include "render_core/my_frame_info.hpp"
#include "vulkan_core/compute_pipeline.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <array>
#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

namespace my {

GrassRenderSystem::GrassRenderSystem(Device &device, VkRenderPass renderPass,
                                     VkDescriptorSetLayout globalSetLayout)
    : myDevice{device} {
    grassBladeModel = MyModel::createModelFromFile(myDevice, "assets/models/grass_blade.obj");
    createGrassComputePoolAndSetLayout();
    createComputePipelineLayout();
    createComputePipeline();
    createGraphicPipelineLayout(globalSetLayout);
    createGraphicPipeline(renderPass);
}

GrassRenderSystem::~GrassRenderSystem() {
    vkDestroyPipelineLayout(myDevice.device(), computePipelineLayout, nullptr);
    vkDestroyPipelineLayout(myDevice.device(), graphicPipelineLayout, nullptr);
}

void GrassRenderSystem::createGrassComputePoolAndSetLayout() {
    grassComputeBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < grassComputeBuffers.size(); i++) {
        grassComputeBuffers[i] = std::make_unique<MyBuffer>(
            myDevice, sizeof(GrassTransformData) * MAX_GRASS, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        grassComputeBuffers[i]->map();
    }

    grassComputePool = MyDescriptorPool::Builder(myDevice)
                           .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                           .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
                           .build();

    grassComputeSetLayout = MyDescriptorSetLayout::Builder(myDevice)
                                .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .build();

    grassComputeDescriptorSet.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < grassComputeDescriptorSet.size(); i++) {
        auto bufferInfo = grassComputeBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*grassComputeSetLayout, *grassComputePool)
            .writeBuffer(0, &bufferInfo)
            .build(grassComputeDescriptorSet[i]);
    }
}

void GrassRenderSystem::createComputePipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(GrassComputePush);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
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
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
        globalSetLayout,
        grassComputeSetLayout->getDescriptorSetLayout(),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

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

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0,
                            1, &grassComputeDescriptorSet[frameInfo.frameIndex], 0, nullptr);

    vkCmdPushConstants(frameInfo.commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(GrassComputePush), &push);

    uint32_t groupCount = static_cast<uint32_t>(std::ceil(push.bladeCount / 256.0));
    vkCmdDispatch(frameInfo.commandBuffer, groupCount, 1, 1);

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(frameInfo.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void GrassRenderSystem::renderGrass(FrameInfo &frameInfo) {
    myGraphicPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            1, 1, &grassComputeDescriptorSet[frameInfo.frameIndex], 0, nullptr);

    grassBladeModel->bind(frameInfo.commandBuffer);
    vkCmdDrawIndexed(frameInfo.commandBuffer, 42, push.bladeCount, 0, 0, 0);
}

} // namespace my
