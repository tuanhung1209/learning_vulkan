#include <GLFW/glfw3.h>
#include "render_systems/simple_render_system.hpp"

#include <stdexcept>
#include <array>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace my{

struct SimplePushConstantData{
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

SimpleRenderSystem::SimpleRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : myDevice{device}{
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem(){
    vkDestroyPipelineLayout(myDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout){
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS){
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass){
    assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PipeLine::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    myPipeLine = std::make_unique<PipeLine>(myDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo){
    myPipeLine->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    // can split into multiple vector to have object with different component/attribute 
    for (auto &kv : frameInfo.gameObjecs){
        auto& obj = kv.second;
        if (obj.model == nullptr) continue;

        SimplePushConstantData push{};
        push.modelMatrix = obj.transform.mat4();
        push.normalMatrix = obj.transform.normalMatrix();

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);

        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}
}