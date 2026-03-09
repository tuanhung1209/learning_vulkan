#include "render_systems/sky_render_system.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace my {

SkyRenderSystem::SkyRenderSystem(Device &device, VkRenderPass renderPass,
                                 VkDescriptorSetLayout globalSetLayout)
    : myDevice{device} {
    skyModel = MyModel::createModelFromFile(myDevice, "assets/models/sphere.obj");
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

SkyRenderSystem::~SkyRenderSystem() { vkDestroyPipelineLayout(myDevice.device(), pipelineLayout, nullptr); }

void SkyRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void SkyRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PipeLine::defaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    myPipeLine = std::make_unique<PipeLine>(myDevice, "shaders/sky_shader.vert.spv",
                                            "shaders/sky_shader.frag.spv", pipelineConfig);
}

void SkyRenderSystem::renderSky(FrameInfo &frameInfo) {
    myPipeLine->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &frameInfo.globalDescriptorSet, 0, nullptr);

    skyModel->bind(frameInfo.commandBuffer);
    skyModel->draw(frameInfo.commandBuffer);
}

} // namespace my
